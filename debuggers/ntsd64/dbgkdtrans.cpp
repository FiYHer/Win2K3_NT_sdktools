//----------------------------------------------------------------------------
//
// KD hard-line communication support.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include <portio.h>

#include <setupapi.h>
#include <newdev.h>

#define THROTTLE_WRITES 0
#define DBG_SYNCH 0

struct KD_FILE_ASSOC
{
    LIST_ENTRY List;
    PWSTR From;
    PSTR To;
};

ULONG g_LastProcessorToPrint = (ULONG) -1;
CHAR g_PrintBuf[PACKET_MAX_SIZE];

PCSTR g_DbgKdTransportNames[] =
{
    "COM", "1394"
};

// This log is for debugging the protocol so leave it
// a simple global for easy examination.
ULONG g_PacketLogIndex;
ULONG64 g_PacketLog[16];

#define PACKET_LOG_SIZE (sizeof(g_PacketLog) / sizeof(g_PacketLog[0]))

UCHAR DbgKdTransport::s_BreakinPacket[1] =
{
    BREAKIN_PACKET_BYTE
};

UCHAR DbgKdTransport::s_PacketTrailingByte[1] =
{
    PACKET_TRAILING_BYTE
};

UCHAR DbgKdTransport::s_PacketLeader[4] =
{
    PACKET_LEADER_BYTE,
    PACKET_LEADER_BYTE,
    PACKET_LEADER_BYTE,
    PACKET_LEADER_BYTE
};

UCHAR DbgKdTransport::s_Packet[PACKET_MAX_MANIP_SIZE];
KD_PACKET DbgKdTransport::s_PacketHeader;

UCHAR DbgKdTransport::s_SavedPacket[PACKET_MAX_MANIP_SIZE];
KD_PACKET DbgKdTransport::s_SavedPacketHeader;

#define COPYSE(p64,p32,f) p64->f = (ULONG64)(LONG64)(LONG)p32->f

__inline
void
DbgkdGetVersion32To64(
    IN PDBGKD_GET_VERSION32 vs32,
    OUT PDBGKD_GET_VERSION64 vs64,
    OUT PKDDEBUGGER_DATA64 dd64
    )
{
    vs64->MajorVersion = vs32->MajorVersion;
    vs64->MinorVersion = vs32->MinorVersion;
    vs64->ProtocolVersion = vs32->ProtocolVersion;
    vs64->Flags = vs32->Flags;
    vs64->MachineType = vs32->MachineType;
    COPYSE(vs64,vs32,PsLoadedModuleList);
    COPYSE(vs64,vs32,DebuggerDataList);
    COPYSE(vs64,vs32,KernBase);

    COPYSE(dd64,vs32,KernBase);
    COPYSE(dd64,vs32,PsLoadedModuleList);
    dd64->ThCallbackStack = vs32->ThCallbackStack;
    dd64->NextCallback = vs32->NextCallback;
    dd64->FramePointer = vs32->FramePointer;
    COPYSE(dd64,vs32,KiCallUserMode);
    COPYSE(dd64,vs32,KeUserCallbackDispatcher);
    COPYSE(dd64,vs32,BreakpointWithStatus);
}

void
OutputIo(PSTR Format, PVOID _Buffer, ULONG Request, ULONG Done)
{
    ULONG i, Chunk;
    PUCHAR Buffer = (PUCHAR)_Buffer;

    dprintf(Format, Done, Request);
    while (Done > 0)
    {
        Chunk = min(Done, 16);
        Done -= Chunk;
        dprintf("   ");
        for (i = 0; i < Chunk; i++)
        {
            dprintf(" %02X", *Buffer++);
        }
        dprintf("\n");
    }
}

//----------------------------------------------------------------------------
//
// DbgKdTransport.
//
//----------------------------------------------------------------------------

DbgKdTransport::DbgKdTransport(ConnLiveKernelTargetInfo* Target)
{
    m_Refs = 1;
    m_Target = Target;
    m_ReadOverlapped.hEvent = NULL;
    m_WriteOverlapped.hEvent = NULL;
    InitializeListHead(&m_KdFiles);

    InitKdFileAssoc();

    Restart();
}

DbgKdTransport::~DbgKdTransport(void)
{
    if (m_ReadOverlapped.hEvent != NULL)
    {
        CloseHandle(m_ReadOverlapped.hEvent);
    }
    if (m_WriteOverlapped.hEvent != NULL)
    {
        CloseHandle(m_WriteOverlapped.hEvent);
    }
    while (!IsListEmpty(&m_KdFiles))
    {
        CloseKdFile(CONTAINING_RECORD(m_KdFiles.Flink, KD_FILE, List));
    }
    ClearKdFileAssoc();
}

ULONG
DbgKdTransport::GetNumberParameters(void)
{
    return 1;
}

void
DbgKdTransport::GetParameter(ULONG Index,
                             PSTR Name, ULONG NameSize,
                             PSTR Value, ULONG ValueSize)
{
    switch(Index)
    {
    case 0:
        if (m_OutputIo)
        {
            CopyString(Name, "OutputIo", NameSize);
            PrintString(Value, ValueSize, "0x%x", m_OutputIo);
        }
        break;
    }
}

void
DbgKdTransport::ResetParameters(void)
{
    m_OutputIo = 0;
}

BOOL
DbgKdTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_strcmpi(Name, "OutputIo"))
    {
        if (!Value || sscanf(Value, "%i", &m_OutputIo) != 1)
        {
            ErrOut("OutputIo requires a numeric value\n");
            return FALSE;
        }
    }
    else
    {
        ErrOut("%s parameters: %s is not a valid parameter\n",
               m_Name, Name);
        return FALSE;
    }

    return TRUE;
}

void
DbgKdTransport::Restart(void)
{
    //
    // Reinitialize per-connection values.
    //

    while (!IsListEmpty(&m_KdFiles))
    {
        CloseKdFile(CONTAINING_RECORD(m_KdFiles.Flink, KD_FILE, List));
    }

    m_PacketsRead = 0;
    m_BytesRead = 0;
    m_PacketsWritten = 0;
    m_BytesWritten = 0;

    m_PacketExpected = INITIAL_PACKET_ID;
    m_NextPacketToSend = INITIAL_PACKET_ID;

    m_WaitingThread = 0;

    m_AllowInitialBreak = TRUE;
    m_Resync = TRUE;
    m_BreakIn = FALSE;
    m_SyncBreakIn = FALSE;
    m_ValidUnaccessedPacket = FALSE;
}

void
DbgKdTransport::OutputInfo(void)
{
    char Params[2 * (MAX_PARAM_NAME + MAX_PARAM_VALUE)];

    GetParameters(Params, sizeof(Params));
    dprintf("Transport %s\n", Params);
    dprintf("Packets read: %u, bytes read %I64u\n",
            m_PacketsRead, m_BytesRead);
    dprintf("Packets written: %u, bytes written %I64u\n",
            m_PacketsWritten, m_BytesWritten);
}

HRESULT
DbgKdTransport::Initialize(void)
{
    HRESULT Status;

    //
    // Create the events used by the overlapped structures for the
    // read and write.
    //

    if ((Status = CreateOverlappedPair(&m_ReadOverlapped,
                                       &m_WriteOverlapped)) != S_OK)
    {
        ErrOut("Unable to create overlapped info, %s\n",
               FormatStatusCode(Status));
    }

    return Status;
}

void
DbgKdTransport::CycleSpeed(void)
{
    WarnOut("KD transport cannot change speeds\n");
}

HRESULT
DbgKdTransport::ReadTargetPhysicalMemory(
    IN ULONG64 MemoryOffset,
    IN PVOID Buffer,
    IN ULONG SizeofBuffer,
    IN PULONG BytesRead
    )
{
    WarnOut("Not valid KD transport operation\n");
    return E_UNEXPECTED;
}

ULONG
DbgKdTransport::HandleDebugIo(PDBGKD_DEBUG_IO Packet)
{
    ULONG ReadStatus = DBGKD_WAIT_AGAIN;

    switch(Packet->ApiNumber)
    {
    case DbgKdPrintStringApi:
        HandlePrint(Packet->Processor,
                    (PSTR)(Packet + 1),
                    (SHORT)Packet->u.PrintString.LengthOfString,
                    DEBUG_OUTPUT_DEBUGGEE);
        break;
    case DbgKdGetStringApi:
        HandlePromptString(Packet);
        break;
    default:
        KdOut("READ: Received INVALID DEBUG_IO packet type %x.\n",
              Packet->ApiNumber);
        ReadStatus = DBGKD_WAIT_RESEND;
        break;
    }

    return ReadStatus;
}

ULONG
DbgKdTransport::HandleTraceIo(PDBGKD_TRACE_IO Packet)
{
    ULONG ReadStatus = DBGKD_WAIT_AGAIN;

    switch(Packet->ApiNumber)
    {
    case DbgKdPrintTraceApi:
        HandlePrintTrace(Packet->Processor,
                         (PUCHAR)(Packet + 1),
                         (USHORT)Packet->u.PrintTrace.LengthOfData,
                         DEBUG_OUTPUT_DEBUGGEE);
        break;
    default:
        KdOut("READ: Received INVALID TRACE_IO packet type %x.\n",
              Packet->ApiNumber);
        ReadStatus = DBGKD_WAIT_RESEND;
        break;
    }

    return ReadStatus;
}

ULONG
DbgKdTransport::HandleControlRequest(PDBGKD_CONTROL_REQUEST Packet)
{
    ULONG ReadStatus = DBGKD_WAIT_AGAIN;

    switch(Packet->ApiNumber)
    {
    case DbgKdRequestHardwareBp:
        Packet->u.RequestBreakpoint.Available = FALSE;
        WritePacket(Packet,
                    sizeof(*Packet),
                    PACKET_TYPE_KD_CONTROL_REQUEST,
                    NULL,
                    0);
        break;
    case DbgKdReleaseHardwareBp:
        Packet->u.ReleaseBreakpoint.Released = TRUE;
        WritePacket(Packet,
                    sizeof(*Packet),
                    PACKET_TYPE_KD_CONTROL_REQUEST,
                    NULL,
                    0);
        break;
    default:
        KdOut("READ: Received INVALID CONTROL_REQUEST packet type %x.\n",
              Packet->ApiNumber);
        ReadStatus = DBGKD_WAIT_RESEND;
        break;
    }

    return ReadStatus;
}

ULONG
DbgKdTransport::HandleFileIo(PDBGKD_FILE_IO Packet)
{
    KD_FILE* File = NULL;
    PVOID ExtraData = NULL;
    USHORT ExtraDataLength = 0;
    LARGE_INTEGER FilePtr;

    // Reenter the engine lock to protect the file list.
    RESUME_ENGINE();

    switch(Packet->ApiNumber)
    {
    case DbgKdCreateFileApi:
        Packet->Status = CreateKdFile((PWSTR)(Packet + 1),
                                      Packet->u.CreateFile.DesiredAccess,
                                      Packet->u.CreateFile.FileAttributes,
                                      Packet->u.CreateFile.ShareAccess,
                                      Packet->u.CreateFile.CreateDisposition,
                                      Packet->u.CreateFile.CreateOptions,
                                      &File,
                                      &Packet->u.CreateFile.Length);
        Packet->u.CreateFile.Handle = (ULONG_PTR)File;
        KdOut("KdFile request for '%ws' returns %08X\n",
              (PWSTR)(Packet + 1), Packet->Status);
        break;
    case DbgKdReadFileApi:
        File = TranslateKdFileHandle(Packet->u.ReadFile.Handle);
        if (File == NULL ||
            Packet->u.ReadFile.Length > PACKET_MAX_SIZE - sizeof(*Packet))
        {
            Packet->Status = STATUS_INVALID_PARAMETER;
            break;
        }
        FilePtr.QuadPart = Packet->u.ReadFile.Offset;
        if (SetFilePointer(File->Handle, FilePtr.LowPart, &FilePtr.HighPart,
                           FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
            GetLastError())
        {
            Packet->Status = STATUS_END_OF_FILE;
            break;
        }
        if (!ReadFile(File->Handle, Packet + 1, Packet->u.ReadFile.Length,
                      &Packet->u.ReadFile.Length, NULL))
        {
            Packet->Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            dprintf(".");
            Packet->Status = STATUS_SUCCESS;
            ExtraData = Packet + 1;
            ExtraDataLength = (USHORT)Packet->u.ReadFile.Length;
        }
        break;
    case DbgKdWriteFileApi:
        File = TranslateKdFileHandle(Packet->u.WriteFile.Handle);
        if (File == NULL ||
            Packet->u.WriteFile.Length > PACKET_MAX_SIZE - sizeof(*Packet))
        {
            Packet->Status = STATUS_INVALID_PARAMETER;
            break;
        }
        FilePtr.QuadPart = Packet->u.WriteFile.Offset;
        if (SetFilePointer(File->Handle, FilePtr.LowPart, &FilePtr.HighPart,
                           FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
            GetLastError())
        {
            Packet->Status = STATUS_END_OF_FILE;
            break;
        }
        if (!WriteFile(File->Handle, Packet + 1, Packet->u.WriteFile.Length,
                      &Packet->u.WriteFile.Length, NULL))
        {
            Packet->Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            dprintf(".");
            Packet->Status = STATUS_SUCCESS;
        }
        break;
    case DbgKdCloseFileApi:
        File = TranslateKdFileHandle(Packet->u.CloseFile.Handle);
        if (File != NULL)
        {
            // Finish line of progress dots.
            dprintf("\n");
            CloseKdFile(File);
            Packet->Status = STATUS_SUCCESS;
        }
        else
        {
            Packet->Status = STATUS_INVALID_PARAMETER;
        }
        break;
    default:
        KdOut("READ: Received INVALID FILE_IO packet type %x.\n",
              Packet->ApiNumber);
        SUSPEND_ENGINE();
        return DBGKD_WAIT_RESEND;
    }

    //
    // Send response data.
    //

    WritePacket(Packet, sizeof(*Packet),
                PACKET_TYPE_KD_FILE_IO,
                ExtraData, ExtraDataLength);

    SUSPEND_ENGINE();
    return DBGKD_WAIT_AGAIN;
}

ULONG
DbgKdTransport::WaitForPacket(
    IN USHORT PacketType,
    OUT PVOID Packet
    )
{
    ULONG InvPacketRetry = 0;

    // Packets can only be read when the kernel transport
    // is not in use.
    if (m_WaitingThread != 0 &&
        m_WaitingThread != GetCurrentThreadId())
    {
        ErrOut("Kernel transport in use, packet read failed\n");
        return DBGKD_WAIT_FAILED;
    }

    if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
    {
        KdOut("READ: Wait for ACK packet with id = %lx\n",
              m_NextPacketToSend);
    }
    else
    {
        KdOut("READ: Wait for type %x packet exp id = %lx\n",
              PacketType, m_PacketExpected);
    }

    g_PacketLog[g_PacketLogIndex++ & (PACKET_LOG_SIZE - 1)] =
        ((ULONG64)PacketType << 32);

    if (PacketType != PACKET_TYPE_KD_ACKNOWLEDGE)
    {
        if (m_ValidUnaccessedPacket)
        {
            KdOut("READ: Grab packet from buffer.\n");
            goto ReadBuffered;
        }
    }

 ReadContents:

    for (;;)
    {
        ULONG ReadStatus = ReadPacketContents(PacketType);

        //
        // If we read an internal packet such as IO or Resend, then
        // handle it and continue waiting.
        //
        if (ReadStatus == DBGKD_WAIT_PACKET)
        {
            m_PacketsRead++;

            switch(s_PacketHeader.PacketType)
            {
            case PACKET_TYPE_KD_DEBUG_IO:
                ReadStatus = HandleDebugIo((PDBGKD_DEBUG_IO)s_Packet);
                break;
            case PACKET_TYPE_KD_TRACE_IO:
                ReadStatus = HandleTraceIo((PDBGKD_TRACE_IO)s_Packet);
                break;
            case PACKET_TYPE_KD_CONTROL_REQUEST:
                ReadStatus =
                    HandleControlRequest((PDBGKD_CONTROL_REQUEST)s_Packet);
                break;
            case PACKET_TYPE_KD_FILE_IO:
                ReadStatus = HandleFileIo((PDBGKD_FILE_IO)s_Packet);
                break;
            }
        }
        else if (ReadStatus == DBGKD_WAIT_ACK)
        {
            m_PacketsRead++;

            // If we're waiting for an ack we're done,
            // otherwise the communication is confused
            // so ask for a resend.
            if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
            {
                return DBGKD_WAIT_ACK;
            }
            else
            {
                KdOut("READ: Received ACK while waiting for type %d\n",
                      PacketType);
                ReadStatus = DBGKD_WAIT_RESEND;
            }
        }

        if (ReadStatus == DBGKD_WAIT_PACKET)
        {
            // If we're waiting for an ack and received
            // a normal packet leave it in the buffer
            // and record the fact that we have one
            // stored.  Consider it an ack and return.
            if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
            {
                m_ValidUnaccessedPacket = TRUE;

                KdOut("READ: Packet Read ahead.\n");
                FlushCallbacks();

                return DBGKD_WAIT_ACK;
            }

            // We're waiting for a data packet and we
            // just got one so process it.
            break;
        }
        else if (ReadStatus == DBGKD_WAIT_RESEND)
        {
            // If the other end didn't wait for an
            // ack then we can't ask for a resend.
            if (!m_AckWrites)
            {
                return DBGKD_WAIT_FAILED;
            }

            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
            {
                return DBGKD_WAIT_ACK;
            }

            KdOut("READ: Ask for resend.\n");
        }
        else if (ReadStatus == DBGKD_WAIT_AGAIN)
        {
            // Internal packets count as acknowledgements,
            // so if we processed one while waiting for an
            // ack consider things done.
            if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
            {
                return DBGKD_WAIT_ACK;
            }
        }
        else
        {
            return ReadStatus;
        }
    }

 ReadBuffered:

    //
    // Check PacketType is what we are waiting for.
    //

    if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
    {
        if (s_PacketHeader.PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
        {
            m_Target->m_KdApi64 = TRUE;
        }
        else if (s_PacketHeader.PacketType == PACKET_TYPE_KD_STATE_CHANGE32)
        {
            PacketType = PACKET_TYPE_KD_STATE_CHANGE32;
            m_Target->m_KdApi64 = FALSE;
        }

        KdOut("READ: Packet type = %x, KdApi64 = %x\n",
              s_PacketHeader.PacketType, m_Target->m_KdApi64);
    }

    if (PacketType != s_PacketHeader.PacketType)
    {
        KdOut("READ: Unexpected Packet type %x (Acked). "
              "Expecting Packet type %x\n",
              s_PacketHeader.PacketType, PacketType);

        if (m_InvPacketRetryLimit > 0 &&
            ++InvPacketRetry >= m_InvPacketRetryLimit)
        {
            return DBGKD_WAIT_FAILED;
        }

        goto ReadContents;
    }

    if (!m_Target->m_KdApi64 &&
        PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        DBGKD_MANIPULATE_STATE64 Packet64;
        DWORD AdditionalDataSize;

        DbgkdManipulateState32To64((PDBGKD_MANIPULATE_STATE32)&s_Packet,
                                   &Packet64, &AdditionalDataSize);
        if (Packet64.ApiNumber == DbgKdGetVersionApi)
        {
            DbgkdGetVersion32To64(&((PDBGKD_MANIPULATE_STATE32)&s_Packet)->
                                  u.GetVersion32,
                                  &Packet64.u.GetVersion64,
                                  &m_Target->m_KdDebuggerData);
        }
        else if (AdditionalDataSize)
        {
            //
            // Move the trailing data to make room for the larger packet header
            //
            MoveMemory(s_Packet + sizeof(DBGKD_MANIPULATE_STATE64),
                       s_Packet + sizeof(DBGKD_MANIPULATE_STATE32),
                       AdditionalDataSize);
        }
        *(PDBGKD_MANIPULATE_STATE64)s_Packet = Packet64;
    }

    *(PVOID *)Packet = &s_Packet;
    m_ValidUnaccessedPacket = FALSE;
    return DBGKD_WAIT_PACKET;
}

VOID
DbgKdTransport::WriteBreakInPacket(VOID)
{
    DWORD BytesWritten;
    BOOL Succ;

    KdOut("Send Break in ...\n");
    FlushCallbacks();

    for (;;)
    {
        Succ = Write(&s_BreakinPacket[0], sizeof(s_BreakinPacket),
                   &BytesWritten);
        if (Succ && BytesWritten == sizeof(s_BreakinPacket))
        {
            break;
        }

        // Avoid consuming 100% of the CPU when spinning.
        Sleep(10);
    }

    m_BreakIn = FALSE;
    m_PacketsWritten++;
}

VOID
DbgKdTransport::WriteControlPacket(
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL
    )

/*++

Routine Description:

    This function writes a control packet to target machine.

    N.B. a CONTROL Packet header is sent with the following information:
         PacketLeader - indicates it's a control packet
         PacketType - indicates the type of the control packet
         ByteCount - aways zero to indicate no data following the header
         PacketId - Valid ONLY for PACKET_TYPE_KD_ACKNOWLEDGE to indicate
                    which packet is acknowledged.

Arguments:

    PacketType - Supplies the type of the control packet.

    PacketId - Supplies the PacketId.  Used by Acknowledge packet only.

Return Value:

    None.

--*/
{
    DWORD BytesWritten;
    BOOL Succ;
    KD_PACKET Packet;

    DBG_ASSERT( (m_Target->m_KdMaxPacketType == 0 &&
                 PacketType < PACKET_TYPE_MAX) ||
                (m_Target->m_KdMaxPacketType > 0 &&
                 PacketType < m_Target->m_KdMaxPacketType) );

    Packet.PacketLeader = CONTROL_PACKET_LEADER;
    Packet.ByteCount = 0;
    Packet.PacketType = PacketType;
    if (PacketId)
    {
        Packet.PacketId = PacketId;
    }
    else
    {
        Packet.PacketId = 0;
    }
    Packet.Checksum = 0;

    for (;;)
    {
        // Write the control packet header.  We need this
        // to be sent so retry until the write succeeds.
        Succ = Write(&Packet, sizeof(Packet), &BytesWritten);
        if (Succ && BytesWritten == sizeof(Packet))
        {
            break;
        }

        // Avoid consuming 100% of the CPU when spinning.
        Sleep(10);
    }

    m_PacketsWritten++;
}

VOID
DbgKdTransport::WriteDataPacket(
    IN PVOID PacketData,
    IN USHORT PacketDataLength,
    IN USHORT PacketType,
    IN PVOID MorePacketData OPTIONAL,
    IN USHORT MorePacketDataLength OPTIONAL,
    IN BOOL NoAck
    )
{
    KD_PACKET Packet;
    USHORT TotalBytesToWrite;
    DBGKD_MANIPULATE_STATE32 m32;
    PVOID ConvertedPacketData = NULL;

    DBG_ASSERT( (m_Target->m_KdMaxPacketType == 0 &&
                 PacketType < PACKET_TYPE_MAX) ||
                (m_Target->m_KdMaxPacketType > 0 &&
                 PacketType < m_Target->m_KdMaxPacketType) );

    // Packets can only be written when the kernel transport
    // is not in use.
    if (m_WaitingThread != 0 &&
        m_WaitingThread != GetCurrentThreadId())
    {
        ErrOut("Kernel transport in use, packet write failed\n");
        return;
    }

    KdOut("WRITE: Write type %x packet id= %lx.\n",
          PacketType, m_NextPacketToSend);

    if (!m_Target->m_KdApi64 &&
        PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PacketDataLength = (USHORT)
            DbgkdManipulateState64To32((PDBGKD_MANIPULATE_STATE64)PacketData,
                                       &m32);
        PacketData = (PVOID)&m32;
        if (m32.ApiNumber == DbgKdWriteBreakPointExApi)
        {
            ConvertedPacketData = malloc(MorePacketDataLength / 2);
            if (!ConvertedPacketData)
            {
                ErrOut("Failed to allocate Packet Data\n");
                return;
            }
            ConvertQwordsToDwords((PULONG64)PacketData,
                                  (PULONG)ConvertedPacketData,
                                  MorePacketDataLength / 8);
            MorePacketData = ConvertedPacketData;
            MorePacketDataLength /= 2;
        }
    }

    if ( ARGUMENT_PRESENT(MorePacketData) )
    {
        TotalBytesToWrite = PacketDataLength + MorePacketDataLength;
        Packet.Checksum = ComputeChecksum((PUCHAR)MorePacketData,
                                          MorePacketDataLength);
    }
    else
    {
        TotalBytesToWrite = PacketDataLength;
        Packet.Checksum = 0;
    }
    Packet.Checksum += ComputeChecksum((PUCHAR)PacketData,
                                       PacketDataLength);
    Packet.PacketLeader = PACKET_LEADER;
    Packet.ByteCount = TotalBytesToWrite;
    Packet.PacketType = PacketType;

    g_PacketLog[g_PacketLogIndex++ & (PACKET_LOG_SIZE - 1)] =
        ((ULONG64)0xF << 60) | ((ULONG64)PacketType << 32) | TotalBytesToWrite;

    for (;;)
    {
        Packet.PacketId = m_NextPacketToSend;

        if (WritePacketContents(&Packet, PacketData, PacketDataLength,
                                MorePacketData, MorePacketDataLength,
                                NoAck) == DBGKD_WRITE_PACKET)
        {
            m_PacketsWritten++;
            break;
        }
    }

    if (ConvertedPacketData)
    {
        free(ConvertedPacketData);
    }
}

ULONG
DbgKdTransport::ComputeChecksum(
    IN PUCHAR Buffer,
    IN ULONG Length
    )
{
    ULONG Checksum = 0;

    while (Length > 0)
    {
        Checksum = Checksum + (ULONG)*Buffer++;
        Length--;
    }

    return Checksum;
}

void
DbgKdTransport::HandlePrint(IN ULONG Processor,
                            IN PCSTR String,
                            IN USHORT StringLength,
                            IN ULONG Mask)
{
    DWORD i;
    DWORD j;
    CHAR c;
    PSTR d;

    DBG_ASSERT(StringLength < PACKET_MAX_SIZE - 2);

    // This routine can be called during a wait when the
    // engine lock isn't held and can also be called when
    // the lock is held.  RESUME handles both of these
    // cases so that the lock is reacquired or reentered.
    RESUME_ENGINE();

    if (m_Target->m_NumProcessors > 1 &&
        Processor != g_LastProcessorToPrint)
    {
        g_LastProcessorToPrint = Processor;
        MaskOut(Mask, "%d:", Processor);
    }

    StartOutLine(Mask, OUT_LINE_NO_PREFIX);

    //
    // Add the original data to the print buffer.
    //

    d = g_PrintBuf;

    for (i = 0; i < StringLength ; i++)
    {
        c = *(String + i);
        if ( c == '\n' )
        {
            g_LastProcessorToPrint = -1;
            *d++ = '\n';
            *d++ = '\r';
        }
        else
        {
            if ( c )
            {
                *d++ = c;
            }
        }
    }

    j = (DWORD)(d - g_PrintBuf);

    //
    // print the string.
    //

    MaskOut(Mask, "%*.*s", j, j, g_PrintBuf);

    SUSPEND_ENGINE();
}

void
DbgKdTransport::HandlePromptString(IN PDBGKD_DEBUG_IO IoMessage)
{
    PSTR IoData;
    DWORD j;

    // This routine can be called during a wait when the
    // engine lock isn't held and can also be called when
    // the lock is held.  RESUME handles both of these
    // cases so that the lock is reacquired or reentered.
    RESUME_ENGINE();

    IoData = (PSTR)(IoMessage + 1);

    HandlePrint(IoMessage->Processor,
                IoData,
                (USHORT)IoMessage->u.GetString.LengthOfPromptString,
                DEBUG_OUTPUT_DEBUGGEE_PROMPT);

    //
    // read the prompt data
    //

    j = GetInput(NULL, IoData,
                 IoMessage->u.GetString.LengthOfStringRead,
                 GETIN_LOG_INPUT_LINE);
    if (j == 0)
    {
        j = IoMessage->u.GetString.LengthOfStringRead;
        memset(IoData, 0, j);
    }

    g_LastProcessorToPrint = -1;
    if ( j < (USHORT)IoMessage->u.GetString.LengthOfStringRead )
    {
        IoMessage->u.GetString.LengthOfStringRead = j;
    }

    SUSPEND_ENGINE();

    //
    // Send data to the debugger-target
    //

    WritePacket(IoMessage, sizeof(*IoMessage),
                PACKET_TYPE_KD_DEBUG_IO, IoData,
                (USHORT)IoMessage->u.GetString.LengthOfStringRead);
}

void
DbgKdTransport::HandlePrintTrace(IN ULONG Processor,
                                 IN PUCHAR Data,
                                 IN USHORT DataLength,
                                 IN ULONG Mask)
{
    // This routine can be called during a wait when the
    // engine lock isn't held and can also be called when
    // the lock is held.  RESUME handles both of these
    // cases so that the lock is reacquired or reentered.
    RESUME_ENGINE();

    DebugClient* Client;

    // Find a client with output callbacks to use for output.
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_OutputCb != NULL)
        {
            break;
        }
    }
    if (Client == NULL)
    {
        // No clients have output callbacks so nobody
        // cares about output and we can just quit.
        goto Exit;
    }

    // Prefix the entire output block with the processor
    // number as we can't (and don't want to) get involved
    // in the individual messages.
    if (m_Target->m_NumProcessors > 1 &&
        Processor != g_LastProcessorToPrint)
    {
        g_LastProcessorToPrint = Processor;
        MaskOut(Mask, "%d", Processor);
    }

    if (g_WmiFormatTraceData == NULL)
    {
        EXTDLL* WmiExt;

        // Prevent notifications from this low level call.
        g_EngNotify++;

        WmiExt = AddExtensionDll("wmitrace", FALSE, m_Target, NULL);
        if (WmiExt)
        {
            LoadExtensionDll(m_Target, WmiExt);
        }

        g_EngNotify--;
    }

    if (g_WmiFormatTraceData == NULL)
    {
        ErrOut("Missing or incorrect wmitrace.dll - "
               "0x%X byte trace data buffer ignored\n",
               DataLength);
    }
    else
    {
        g_WmiFormatTraceData((PDEBUG_CONTROL)(IDebugControlN*)Client,
                             Mask, DataLength, Data);
    }

 Exit:
    SUSPEND_ENGINE();
}

KD_FILE_ASSOC*
DbgKdTransport::FindKdFileAssoc(PWSTR From)
{
    PLIST_ENTRY Entry;
    KD_FILE_ASSOC* Assoc;

    for (Entry = m_KdFileAssoc.Flink;
         Entry != &m_KdFileAssoc;
         Entry = Entry->Flink)
    {
        Assoc = CONTAINING_RECORD(Entry, KD_FILE_ASSOC, List);

        if (!_wcsicmp(From, Assoc->From))
        {
            return Assoc;
        }
    }

    return NULL;
}

void
DbgKdTransport::ClearKdFileAssoc(void)
{
    while (!IsListEmpty(&m_KdFileAssoc))
    {
        KD_FILE_ASSOC* Assoc;

        Assoc = CONTAINING_RECORD(m_KdFileAssoc.Flink, KD_FILE_ASSOC, List);
        RemoveEntryList(&Assoc->List);
        free(Assoc);
    }

    m_KdFileAssocSource[0] = 0;
}

HRESULT
DbgKdTransport::LoadKdFileAssoc(PSTR FileName)
{
    HRESULT Status;
    FILE* File;
    char Op[32], From[MAX_PATH], To[MAX_PATH];

    File = fopen(FileName, "r");
    if (File == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    ClearKdFileAssoc();

    Status = S_OK;
    for (;;)
    {
        ULONG Len;

        if (fgets(Op, sizeof(Op), File) == NULL)
        {
            break;
        }

        // Remove newline if present.
        Len = strlen(Op);
        if (Len > 0 && Op[Len - 1] == '\n')
        {
            Op[Len - 1] = 0;
        }

        // Skip blank lines between sections and
        // allow comments starting with '#'.
        if (Op[0] == '#' || !Op[0])
        {
            continue;
        }

        if (_stricmp(Op, "map") != 0)
        {
            Status = E_INVALIDARG;
            break;
        }

        if (fgets(From, sizeof(From), File) == NULL ||
            fgets(To, sizeof(To), File) == NULL)
        {
            Status = E_INVALIDARG;
            break;
        }

        // Remove newlines if present.
        Len = strlen(From);
        if (Len > 0 && From[Len - 1] == '\n')
        {
            From[Len - 1] = 0;
        }
        Len = strlen(To);
        if (Len > 0 && To[Len - 1] == '\n')
        {
            To[Len - 1] = 0;
        }

        KD_FILE_ASSOC* Assoc;

        Assoc = (KD_FILE_ASSOC*)malloc(sizeof(KD_FILE_ASSOC) +
                                       (strlen(From) + 1) * sizeof(WCHAR) +
                                       strlen(To) + 1);
        if (Assoc == NULL)
        {
            Status = E_OUTOFMEMORY;
            break;
        }

        Assoc->From = (PWSTR)(Assoc + 1);
        if (MultiByteToWideChar(CP_ACP, 0, From, -1, Assoc->From,
                                sizeof(From) / sizeof(WCHAR)) == 0)
        {
            Status = WIN32_LAST_STATUS();
            break;
        }

        Assoc->To = (PSTR)(Assoc->From + strlen(From) + 1);
        strcpy(Assoc->To, To);

        InsertHeadList(&m_KdFileAssoc, &Assoc->List);
    }

    fclose(File);

    if (Status == S_OK)
    {
        CopyString(m_KdFileAssocSource, FileName, DIMA(m_KdFileAssocSource));
    }

    return Status;
}

void
DbgKdTransport::InitKdFileAssoc(void)
{
    PSTR Env;

    InitializeListHead(&m_KdFileAssoc);
    m_KdFileAssocSource[0] = 0;

    Env = getenv("_NT_KD_FILES");
    if (Env != NULL)
    {
        LoadKdFileAssoc(Env);
    }
}

void
DbgKdTransport::ParseKdFileAssoc(void)
{
    if (PeekChar() == ';' || *g_CurCmd == 0)
    {
        if (m_KdFileAssocSource[0])
        {
            dprintf("KD file assocations loaded from '%s'\n",
                    m_KdFileAssocSource);
        }
        else
        {
            dprintf("No KD file associations set\n");
        }
        return;
    }

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        g_CurCmd++;
        switch(*g_CurCmd++)
        {
        case 'c':
            ClearKdFileAssoc();
            dprintf("KD file associations cleared\n");
            return;
        default:
            ErrOut("Unknown option '%c'\n", *(g_CurCmd - 1));
            break;
        }
    }

    PSTR FileName;
    CHAR Save;

    FileName = StringValue(STRV_TRIM_TRAILING_SPACE, &Save);
    if (LoadKdFileAssoc(FileName) == S_OK)
    {
        dprintf("KD file assocations loaded from '%s'\n", FileName);
    }
    else
    {
        dprintf("Unable to load KD file associations from '%s'\n", FileName);
    }
    *g_CurCmd = Save;
}

NTSTATUS
DbgKdTransport::CreateKdFile(PWSTR FileName,
                             ULONG DesiredAccess, ULONG FileAttributes,
                             ULONG ShareAccess, ULONG CreateDisposition,
                             ULONG CreateOptions,
                             KD_FILE** FileEntry, PULONG64 Length)
{
    ULONG Access, Create;
    KD_FILE* File;
    KD_FILE_ASSOC* Assoc;

    Assoc = FindKdFileAssoc(FileName);
    if (Assoc == NULL)
    {
        return STATUS_NO_SUCH_FILE;
    }

    File = new KD_FILE;
    if (File == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    Access = 0;
    if (DesiredAccess & FILE_READ_DATA)
    {
        Access |= GENERIC_READ;
    }
    if (DesiredAccess & FILE_WRITE_DATA)
    {
        Access |= GENERIC_WRITE;
    }

    switch(CreateDisposition)
    {
    case FILE_OPEN:
        Create = OPEN_EXISTING;
        break;
    case FILE_CREATE:
        Create = CREATE_NEW;
        break;
    case FILE_OPEN_IF:
        Create = OPEN_ALWAYS;
        break;
    case FILE_OVERWRITE_IF:
        Create = CREATE_ALWAYS;
        break;
    default:
        delete File;
        return STATUS_INVALID_PARAMETER;
    }

    // No interesting CreateOptions at this point.

    File->Handle = CreateFile(Assoc->To, Access, ShareAccess, NULL,
                              Create, FileAttributes, NULL);
    if (File->Handle == NULL || File->Handle == INVALID_HANDLE_VALUE)
    {
        delete File;
        switch(GetLastError())
        {
        case ERROR_FILE_NOT_FOUND:
            return STATUS_NO_SUCH_FILE;
        case ERROR_ACCESS_DENIED:
            return STATUS_ACCESS_DENIED;
        default:
            return STATUS_UNSUCCESSFUL;
        }
    }

    ULONG SizeLow;
    LONG SizeHigh = 0;

    SizeLow = SetFilePointer(File->Handle, 0, &SizeHigh, FILE_END);
    if (SizeLow == INVALID_SET_FILE_POINTER && GetLastError())
    {
        CloseHandle(File->Handle);
        delete File;
        return STATUS_UNSUCCESSFUL;
    }

    *Length = ((ULONG64)SizeHigh << 32) | SizeLow;

    dprintf("KD: Accessing '%s' (%ws)\n  ", Assoc->To, FileName);
    if (*Length > 0)
    {
        dprintf("File size %dK", KBYTES(*Length));
    }
    // Progress dots will be printed for each read/write.

    File->Signature = KD_FILE_SIGNATURE;
    InsertHeadList(&m_KdFiles, &File->List);
    *FileEntry = File;
    return STATUS_SUCCESS;
}

void
DbgKdTransport::CloseKdFile(KD_FILE* File)
{
    RemoveEntryList(&File->List);
    CloseHandle(File->Handle);
    File->Signature = 0;
    delete File;
}

KD_FILE*
DbgKdTransport::TranslateKdFileHandle(ULONG64 Handle)
{
    KD_FILE* File = (KD_FILE*)(ULONG_PTR)Handle;

    if (IsBadWritePtr(File, sizeof(*File)) ||
        File->Signature != KD_FILE_SIGNATURE)
    {
        return NULL;
    }

    return File;
}

//----------------------------------------------------------------------------
//
// DbgKdComTransport.
//
//----------------------------------------------------------------------------

// Environment variable names.
#define COM_PORT_NAME   "_NT_DEBUG_PORT"
#define COM_PORT_BAUD   "_NT_DEBUG_BAUD_RATE"

// Parameter string names.
#define PARAM_COM_BAUD    "Baud"
#define PARAM_COM_IP_PORT "IpPort"
#define PARAM_COM_MODEM   "Modem"
#define PARAM_COM_PIPE    "Pipe"
#define PARAM_COM_PORT    "Port"
#define PARAM_COM_RESETS  "Resets"
#define PARAM_COM_TIMEOUT "Timeout"

DbgKdComTransport::DbgKdComTransport(ConnLiveKernelTargetInfo* Target)
    : DbgKdTransport(Target)
{
    m_Index = DBGKD_TRANSPORT_COM;
    m_Name = g_DbgKdTransportNames[m_Index];
    m_InvPacketRetryLimit = 0;
    m_AckWrites = TRUE;
    m_Handle = NULL;
    m_EventOverlapped.hEvent = NULL;
}

DbgKdComTransport::~DbgKdComTransport(void)
{
    if (m_Handle != NULL)
    {
        CloseHandle(m_Handle);
    }
    if (m_EventOverlapped.hEvent != NULL)
    {
        CloseHandle(m_EventOverlapped.hEvent);
    }
}

ULONG
DbgKdComTransport::GetNumberParameters(void)
{
    return 5 + DbgKdTransport::GetNumberParameters();
}

void
DbgKdComTransport::GetParameter(ULONG Index,
                                PSTR Name, ULONG NameSize,
                                PSTR Value, ULONG ValueSize)
{
    switch(Index)
    {
    case 0:
        CopyString(Name, PARAM_COM_PORT, NameSize);
        CopyString(Value, m_PortName, ValueSize);
        break;
    case 1:
        CopyString(Name, PARAM_COM_BAUD, NameSize);
        PrintString(Value, ValueSize, "%d", m_BaudRate);
        break;
    case 2:
        switch(m_PortType)
        {
        case COM_PORT_MODEM:
            CopyString(Name, PARAM_COM_MODEM, NameSize);
            break;
        case COM_PORT_PIPE:
            CopyString(Name, PARAM_COM_PIPE, NameSize);
            break;
        case COM_PORT_SOCKET:
            CopyString(Name, PARAM_COM_IP_PORT, NameSize);
            PrintString(Value, ValueSize, "%d", m_IpPort);
            break;
        }
        break;
    case 3:
        CopyString(Name, PARAM_COM_TIMEOUT, NameSize);
        PrintString(Value, ValueSize, "%d", m_Timeout);
        break;
    case 4:
        if (m_MaxSyncResets)
        {
            CopyString(Name, PARAM_COM_RESETS, NameSize);
            PrintString(Value, ValueSize, "%d", m_MaxSyncResets);
        }
        break;
    default:
        DbgKdTransport::GetParameter(Index - 4,
                                     Name, NameSize, Value, ValueSize);
        break;
    }
}

void
DbgKdComTransport::ResetParameters(void)
{
    PSTR Env;

    DbgKdTransport::ResetParameters();

    if ((Env = getenv(COM_PORT_NAME)) == NULL)
    {
        Env = "com1";
    }
    SetComPortName(Env, m_PortName, DIMA(m_PortName));

    if ((Env = getenv(COM_PORT_BAUD)) != NULL)
    {
        m_BaudRate = atol(Env);
    }
    else
    {
        m_BaudRate = 19200;
    }

    m_PortType = COM_PORT_STANDARD;
    m_Timeout = 4000;
    m_CurTimeout = m_Timeout;
    m_MaxSyncResets = 0;
    m_IpPort = 0;
}

BOOL
DbgKdComTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_strcmpi(Name, PARAM_COM_PORT))
    {
        if (!Value)
        {
            ErrOut(PARAM_COM_PORT " requires a value\n");
            return FALSE;
        }

        if (!SetComPortName(Value, m_PortName, DIMA(m_PortName)))
        {
            return FALSE;
        }
    }
    else if (!_strcmpi(Name, PARAM_COM_BAUD))
    {
        if (!Value)
        {
            ErrOut(PARAM_COM_BAUD " requires a numeric value\n");
            return FALSE;
        }

        m_BaudRate = atol(Value);
    }
    else if (!_strcmpi(Name, PARAM_COM_IP_PORT))
    {
        if (!Value)
        {
            ErrOut(PARAM_COM_IP_PORT " requires a numeric value\n");
            return FALSE;
        }

        m_PortType = COM_PORT_SOCKET;
        m_IpPort = atol(Value);
    }
    else if (!_strcmpi(Name, PARAM_COM_MODEM))
    {
        m_PortType = COM_PORT_MODEM;
    }
    else if (!_strcmpi(Name, PARAM_COM_PIPE))
    {
        m_PortType = COM_PORT_PIPE;
        m_MaxSyncResets = 2;
    }
    else if (!_strcmpi(Name, PARAM_COM_RESETS))
    {
        if (!Value)
        {
            ErrOut(PARAM_COM_RESETS " requires a numeric value\n");
            return FALSE;
        }

        m_MaxSyncResets = atol(Value);
    }
    else if (!_strcmpi(Name, PARAM_COM_TIMEOUT))
    {
        if (!Value)
        {
            ErrOut(PARAM_COM_TIMEOUT " requires a numeric value\n");
            return FALSE;
        }

        m_Timeout = atol(Value);
        m_CurTimeout = m_Timeout;
    }
    else
    {
        return DbgKdTransport::SetParameter(Name, Value);
    }

    return TRUE;
}

HRESULT
DbgKdComTransport::Initialize(void)
{
    HRESULT Status;

    if ((Status = DbgKdTransport::Initialize()) != S_OK)
    {
        return Status;
    }

    m_DirectPhysicalMemory = FALSE;

    COM_PORT_PARAMS ComParams;

    ZeroMemory(&ComParams, sizeof(ComParams));
    ComParams.Type = m_PortType;
    ComParams.PortName = m_PortName;
    ComParams.BaudRate = m_BaudRate;
    ComParams.Timeout = m_Timeout;
    ComParams.IpPort = m_IpPort;
    if ((Status = OpenComPort(&ComParams, &m_Handle, &m_BaudRate)) != S_OK)
    {
        ErrOut("Failed to open %s\n", m_PortName);
        return Status;
    }

    dprintf("Opened %s\n", m_PortName);

    m_ComEvent = 0;
    if (m_PortType == COM_PORT_MODEM)
    {
        DWORD Mask;

        //
        //  Debugger is being run over a modem.  Set event to watch
        //  carrier detect.
        //

        GetCommMask (m_Handle, &Mask);
        // set DDCD event
        if (!SetCommMask (m_Handle, Mask | 0xA0))
        {
            ErrOut("Failed to set event for %s.\n", m_PortName);
            return WIN32_LAST_STATUS();
        }

        m_EventOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_EventOverlapped.hEvent)
        {
            ErrOut("Failed to create EventOverlapped\n");
            return WIN32_LAST_STATUS();
        }

        m_EventOverlapped.Offset = 0;
        m_EventOverlapped.OffsetHigh = 0;

        // Fake an event, so modem status will be checked
        m_ComEvent = 1;
    }

    return S_OK;
}

BOOL
DbgKdComTransport::Read(
    IN PVOID    Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesRead
    )
{
    if (IS_DUMP_TARGET(m_Target))
    {
        ErrOut( "Attempted to read KD transport while "
                "debugging a crash dump\n" );
        DebugBreak();
    }

    if (m_ComEvent)
    {
        CheckComStatus ();
    }

    //
    // In pipe mode it's possible that only part of the
    // desired data is available, so loop reading pieces
    // of data as long as there are successful reads.
    //

    *BytesRead = 0;
    while (SizeOfBuffer > 0)
    {
        ULONG _BytesRead=0;

        if (ComPortRead(m_Handle, m_PortType, m_CurTimeout,
                        Buffer, SizeOfBuffer, &_BytesRead, &m_ReadOverlapped))
        {
            if (m_OutputIo & DBGKD_OUTPUT_READS)
            {
                OutputIo("COM: Read %d bytes of %d\n",
                         Buffer, SizeOfBuffer, _BytesRead);
            }

            Buffer = (PVOID)((PUCHAR)Buffer + _BytesRead);
            SizeOfBuffer -= _BytesRead;
            *BytesRead += _BytesRead;
            m_BytesRead += _BytesRead;

            // If this isn't a net connection stop reading if
            // we got less than the requested amount of data.
            if (!NET_COM_PORT(m_PortType) &&
                _BytesRead < SizeOfBuffer)
            {
                break;
            }
        }
        else if (*BytesRead == 0)
        {
            // If a read failed with nothing read return an error.
            return FALSE;
        }
        else
        {
            // Read failed, so stop trying to read.
            break;
        }
    }

    return TRUE;
}

BOOL
DbgKdComTransport::Write(
    IN PVOID    Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesWritten
    )
{
    if (IS_DUMP_TARGET(m_Target))
    {
        ErrOut( "Attempted to write KD transport "
                "while debugging a crash dump\n" );
        DebugBreak();
    }

    if (m_ComEvent)
    {
        CheckComStatus ();
    }

    //
    // Break up large writes in smaller chunks
    // to try and avoid sending too much data
    // to the target all at once.  Sleep a bit
    // between chunks to let the target retrieve
    // data.
    //

    BOOL Succ = TRUE;
    *BytesWritten = 0;
    while (SizeOfBuffer > 0)
    {
        ULONG Request, Done;

        // By default we want to encourage vendors
        // to create machines with robust serial
        // support so we don't actually limit
        // the write size.
#if THROTTLE_WRITES
        Request = 96;
#else
        Request = 0xffffffff;
#endif
        if (SizeOfBuffer < Request)
        {
            Request = SizeOfBuffer;
        }

        if (!ComPortWrite(m_Handle, m_PortType, Buffer, Request, &Done,
                          &m_WriteOverlapped))
        {
            Succ = FALSE;
            break;
        }

        if (m_OutputIo & DBGKD_OUTPUT_WRITES)
        {
            OutputIo("COM: Wrote %d bytes of %d\n",
                     Buffer, Request, Done);
        }

        *BytesWritten += Done;
        if (Done <= Request)
        {
            break;
        }

        Buffer = (PVOID)((PUCHAR)Buffer + Done);
        SizeOfBuffer -= Done;

        Sleep(10);
    }

    m_BytesWritten += *BytesWritten;
    return Succ;
}

void
DbgKdComTransport::CycleSpeed(void)
{
    if (NET_COM_PORT(m_PortType))
    {
        ErrOut("Net COM port baud is ignored\n");
        return;
    }

    if (SetComPortBaud(m_Handle, 0, &m_BaudRate) != S_OK)
    {
        ErrOut("New Baud rate Could not be set on Com %I64x - remains %d.\n",
               (ULONG64)m_Handle, m_BaudRate);
    }
    else
    {
        dprintf("Baud rate set to %d\n", m_BaudRate);
    }
}

VOID
DbgKdComTransport::Synchronize(VOID)
{
    USHORT Index;
    UCHAR DataByte, PreviousDataByte;
    USHORT PacketType = 0;
    ULONG TimeoutCount = 0;
    COMMTIMEOUTS CommTimeouts;
    COMMTIMEOUTS OldTimeouts;
    DWORD BytesRead;
    BOOL Succ;
    BOOL SendReset;
    ULONG DataLoops = 0;
    ULONG ResetsSent = 0;

    //
    // Get the old time out values and hold them.
    // We then set a new total timeout value of
    // a fraction of the base timeout.
    //

#define TIMEOUT_ITERATIONS 6

    m_CurTimeout = m_Timeout / 8;
    if (!NET_COM_PORT(m_PortType))
    {
        GetCommTimeouts(m_Handle, &OldTimeouts);

        CommTimeouts = OldTimeouts;
        CommTimeouts.ReadIntervalTimeout = 0;
        CommTimeouts.ReadTotalTimeoutMultiplier = 0;
        CommTimeouts.ReadTotalTimeoutConstant = m_CurTimeout;

        SetCommTimeouts(m_Handle, &CommTimeouts);
    }

    FlushCallbacks();

    // Emulators that fill pipes from emulated serial ports
    // can end up buffering a huge amount of data.  Drain
    // everything off before we start resyncing.
    if (NET_COM_PORT(m_PortType))
    {
        while (Read(&DataByte, 1, &BytesRead) &&
               BytesRead == 1)
        {
            // Loop
        }
    }

    // Always send a reset the first time around.
    SendReset = TRUE;

    while (TRUE)
    {

Timeout:
        // When sending data to a pipe assume that it's stored
        // rather than being discarded as would happen with an
        // overfull true serial port.  Therefore, limit the
        // total number of reset packets sent as stuffing more
        // in the pipe will just cause a huge number of responses.
        if (SendReset &&
            (!m_MaxSyncResets || ResetsSent < m_MaxSyncResets))
        {
            WriteControlPacket(PACKET_TYPE_KD_RESET, 0L);
            ResetsSent++;
        }

        //
        // Read packet leader
        //

        BOOL First = TRUE;

        Index = 0;
        do
        {
            if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
            {
                KdOut("Synchronize interrupted by exit request\n");
                goto Exit;
            }

            //
            // if we don't get response from kernel in 3 seconds we
            // will resend the reset packet if user does not type ctrl_c.
            // Otherwise, we send breakin character and wait for data again.
            //

            Succ = Read(&DataByte, 1, &BytesRead);
            if ((!Succ) || (BytesRead != 1))
            {
                //
                // Check user input for control_c.  If user types control_c,
                // we will send a breakin packet to the target.  Hopefully,
                // target will send us a StateChange packet and we'll
                // stop waiting.
                //

                if (m_BreakIn || m_SyncBreakIn)
                {
                    m_SyncBreakIn = FALSE;
                    WriteBreakInPacket();
                    TimeoutCount = 0;
                    continue;
                }
                TimeoutCount++;

                //
                // if we have been waiting for 3 seconds, resend RESYNC packet
                //

                if (TimeoutCount < TIMEOUT_ITERATIONS)
                {
                    continue;
                }
                TimeoutCount = 0;

                KdOut("SYNCTARGET: Timeout.\n");
                FlushCallbacks();

                SendReset = TRUE;
                goto Timeout;
            }

#if DBG_SYNCH
            if (Succ && BytesRead == 1 && First)
            {
                dprintf("First byte %X\n", DataByte);
                First = FALSE;
            }
#endif

            if (Succ && BytesRead == 1 &&
                ( DataByte == PACKET_LEADER_BYTE ||
                  DataByte == CONTROL_PACKET_LEADER_BYTE)
                )
            {
                if ( Index == 0 )
                {
                    PreviousDataByte = DataByte;
                    Index++;
                }
                else if ( DataByte == PreviousDataByte )
                {
                    Index++;
                }
                else
                {
                    PreviousDataByte = DataByte;
                    Index = 1;
                }
            }
            else
            {
                Index = 0;

                if (Succ && BytesRead == 1)
                {
                    // The target machine is alive and talking but
                    // the received data is in the middle of
                    // a packet.  Break out of the header byte
                    // loop and consume up to a trailer byte.
                    break;
                }
            }
        }
        while ( Index < 4 );

        if (Index == 4 && DataByte == CONTROL_PACKET_LEADER_BYTE)
        {
            //
            // Read 2 byte Packet type
            //

            Succ = Read((PUCHAR)&PacketType,
                      sizeof(PacketType), &BytesRead);

            if (Succ && BytesRead == sizeof(PacketType) &&
                PacketType == PACKET_TYPE_KD_RESET)
            {
                KdOut("SYNCTARGET: Received KD_RESET ACK packet.\n");

                m_PacketExpected = INITIAL_PACKET_ID;
                m_NextPacketToSend = INITIAL_PACKET_ID;

                KdOut("SYNCTARGET: Target synchronized successfully...\n");
                FlushCallbacks();

                goto Exit;
            }
        }

        //
        // If we receive Data Packet leader, it means target has not
        // receive our reset packet. So we loop back and send it again.
        // N.B. We need to wait until target finishes sending the packet.
        // Otherwise, we may be sending the reset packet while the target
        // is sending the packet. This might cause target loss the reset
        // packet.
        //
        // Sometimes machines just send continuous streams of
        // garbage, which can cause an infinite loop here if
        // the garbage never contains a trailing byte.  Break
        // this loop after a certain amount of garbage is received.
        //

        Index = 0;
        while (DataByte != PACKET_TRAILING_BYTE &&
               Index < 2 * PACKET_MAX_SIZE)
        {
            Succ = Read(&DataByte, 1, &BytesRead);
            if (!Succ || BytesRead != 1)
            {
                DataByte = ~PACKET_TRAILING_BYTE;
                break;
            }

            Index++;
        }

#if DBG_SYNCH
        dprintf("  ate %x bytes\n", Index);
        FlushCallbacks();
#endif

        if (DataByte == PACKET_TRAILING_BYTE)
        {
            // We've consumed up to a trailing byte but
            // there's no guarantee that the byte is not
            // part of the payload of a packet.  However,
            // the target is still talking to us so
            // avoid sending a reset and provoking more
            // packets.  There are cases, though, where
            // the target machine continuously sends data
            // and we end up not sending any more reset
            // packets.  Send a reset packet every once
            // in a while to make sure we don't get stuck here.
            if (++DataLoops == 4)
            {
                DataLoops = 0;
                SendReset = TRUE;
            }
            else
            {
                SendReset = FALSE;
            }
        }
        else
        {
            // Target stopped talking before we got a
            // data byte, so attempt to reset.
            SendReset = TRUE;
        }
    }

 Exit:
    m_CurTimeout = m_Timeout;
    if (!NET_COM_PORT(m_PortType))
    {
        SetCommTimeouts(m_Handle, &OldTimeouts);
    }
}

ULONG
DbgKdComTransport::ReadPacketContents(IN USHORT PacketType)
{
    DWORD BytesRead;
    BOOL Succ;
    UCHAR DataByte;
    ULONG Checksum;
    ULONG SyncBit;
    ULONG WaitStatus;

    //
    // First read a packet leader
    //

WaitForPacketLeader:

    WaitStatus = ReadPacketLeader(PacketType, &s_PacketHeader.PacketLeader);
    if (WaitStatus != DBGKD_WAIT_PACKET)
    {
        return WaitStatus;
    }
    if (m_AllowInitialBreak && (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK))
    {
        KdOut("Attempting to get initial breakpoint.\n");
        WriteBreakInPacket();
    }

    // We've either sent the initial break or we don't want
    // one.  Either way we don't need to send another one.
    m_AllowInitialBreak = FALSE;

    //
    // Read packetLeader ONLY read two Packet Leader bytes.  This do loop
    // filters out the remaining leader byte.
    //

    do
    {
        Succ = Read(&DataByte, 1, &BytesRead);
        if ((Succ) && BytesRead == 1)
        {
            if (DataByte == PACKET_LEADER_BYTE ||
                DataByte == CONTROL_PACKET_LEADER_BYTE)
            {
                continue;
            }
            else
            {
                *(PUCHAR)&s_PacketHeader.PacketType = DataByte;
                break;
            }
        }
        else
        {
            goto WaitForPacketLeader;
        }
    } while (TRUE);

    //
    // Now we have valid packet leader. Read rest of the packet type.
    //

    Succ = Read(((PUCHAR)&s_PacketHeader.PacketType) + 1,
              sizeof(s_PacketHeader.PacketType) - 1, &BytesRead);
    if ((!Succ) || BytesRead != sizeof(s_PacketHeader.PacketType) - 1)
    {
        //
        // If we cannot read the packet type and if the packet leader
        // indicates this is a data packet, we need to ask for resend.
        // Otherwise we simply ignore the incomplete packet.
        //

        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            KdOut("READ: Data packet header Type error (short read).\n");
        }

        goto WaitForPacketLeader;
    }

    //
    // Check the Packet type.
    //

    if ((m_Target->m_KdMaxPacketType == 0 &&
         s_PacketHeader.PacketType >= PACKET_TYPE_MAX) ||
        (m_Target->m_KdMaxPacketType > 0 &&
         s_PacketHeader.PacketType >= m_Target->m_KdMaxPacketType))
    {
        KdOut("READ: Received INVALID packet type.\n");

        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
        }

        goto WaitForPacketLeader;
    }

    KdOut("      PacketType=%x, ", s_PacketHeader.PacketType);

    //
    // Read ByteCount
    //

    Succ = Read(&s_PacketHeader.ByteCount, sizeof(s_PacketHeader.ByteCount),
              &BytesRead);
    if ((!Succ) || BytesRead != sizeof(s_PacketHeader.ByteCount))
    {
        //
        // If we cannot read the packet type and if the packet leader
        // indicates this is a data packet, we need to ask for resend.
        // Otherwise we simply ignore the incomplete packet.
        //

        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            KdOut("READ: Data packet header ByteCount error (short read).\n");
        }

        goto WaitForPacketLeader;
    }

    //
    // Check ByteCount
    //

    if (s_PacketHeader.ByteCount > PACKET_MAX_SIZE)
    {
        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            KdOut("READ: Data packet header ByteCount error (short read).\n");
        }

        goto WaitForPacketLeader;
    }

    KdOut("ByteCount=%x, ", s_PacketHeader.ByteCount);

    //
    // Read Packet Id
    //

    Succ = Read(&s_PacketHeader.PacketId, sizeof(s_PacketHeader.PacketId),
              &BytesRead);
    if ((!Succ) || BytesRead != sizeof(s_PacketHeader.PacketId))
    {
        //
        // If we cannot read the packet Id and if the packet leader
        // indicates this is a data packet, we need to ask for resend.
        // Otherwise we simply ignore the incomplete packet.
        //

        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            KdOut("READ: Data packet header Id error (short read).\n");
        }

        goto WaitForPacketLeader;
    }

    KdOut("PacketId=%x,\n", s_PacketHeader.PacketId);

    //
    // Don't read checksum here as in some cases
    // it isn't sent with control packets.
    //

    if (s_PacketHeader.PacketLeader == CONTROL_PACKET_LEADER )
    {
        if (s_PacketHeader.PacketType == PACKET_TYPE_KD_ACKNOWLEDGE )
        {
            //
            // If we received an expected ACK packet and we are not
            // waiting for any new packet, update outgoing packet id
            // and return.  If we are NOT waiting for ACK packet
            // we will keep on waiting.  If the ACK packet
            // is not for the packet we send, ignore it and keep on waiting.
            //

            if (s_PacketHeader.PacketId != m_NextPacketToSend)
            {
                KdOut("READ: Received unmatched packet id = %lx, Type = %x\n",
                      s_PacketHeader.PacketId, s_PacketHeader.PacketType);
                goto WaitForPacketLeader;
            }
            else if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
            {
                m_NextPacketToSend ^= 1;

                KdOut("READ: Received correct ACK packet.\n");
                FlushCallbacks();

                return DBGKD_WAIT_ACK;
            }
            else
            {
                goto WaitForPacketLeader;
            }
        }
        else if (s_PacketHeader.PacketType == PACKET_TYPE_KD_RESET)
        {
            //
            // if we received Reset packet, reset the packet control variables
            // and resend earlier packet.
            //

            m_NextPacketToSend = INITIAL_PACKET_ID;
            m_PacketExpected = INITIAL_PACKET_ID;
            WriteControlPacket(PACKET_TYPE_KD_RESET, 0L);

            KdOut("DbgKdpWaitForPacket(): Recieved KD_RESET packet, "
                  "send KD_RESET ACK packet\n");
            FlushCallbacks();

            return DBGKD_WAIT_FAILED;
        }
        else if (s_PacketHeader.PacketType == PACKET_TYPE_KD_RESEND)
        {
            KdOut("READ: Received RESEND packet\n");
            FlushCallbacks();

            return DBGKD_WAIT_FAILED;
        }
        else
        {
            //
            // Invalid packet header, ignore it.
            //

            KdOut("READ: Received Control packet with UNKNOWN type\n");
            goto WaitForPacketLeader;
        }
    }
    else
    {
        //
        // The packet header is for data packet (not control packet).
        // Read Checksum.
        //

        Succ = Read(&s_PacketHeader.Checksum, sizeof(s_PacketHeader.Checksum),
                  &BytesRead);
        if ((!Succ) || BytesRead != sizeof(s_PacketHeader.Checksum))
        {
            WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            KdOut("READ: Data packet header "
                  "checksum error (short read).\n");
            goto WaitForPacketLeader;
        }

        if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
        {
            //
            // If we are waiting for ACK packet ONLY
            // and we receive a data packet header, check if the packet id
            // is what we expected.  If yes, assume the acknowledge is lost
            // (but sent) and process the packet.
            //

            if (s_PacketHeader.PacketId == m_PacketExpected)
            {
                m_NextPacketToSend ^= 1;
                KdOut("READ: Received VALID data packet "
                      "while waiting for ACK.\n");
            }
            else
            {
                KdOut("READ: Received Data packet with unmatched ID = %lx\n",
                      s_PacketHeader.PacketId);
                WriteControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                   s_PacketHeader.PacketId);
                goto WaitForPacketLeader;
            }
        }
    }

    //
    // We are waiting for data packet and we received the packet header
    // for data packet. Perform the following checkings to make sure
    // it is the packet we are waiting for.
    //

    if ((s_PacketHeader.PacketId & ~SYNC_PACKET_ID) != INITIAL_PACKET_ID &&
        (s_PacketHeader.PacketId & ~SYNC_PACKET_ID) != (INITIAL_PACKET_ID ^ 1))
    {
        KdOut("READ: Received INVALID packet Id.\n");
        return DBGKD_WAIT_RESEND;
    }

    Succ = Read(s_Packet, s_PacketHeader.ByteCount, &BytesRead);
    if ( (!Succ) || BytesRead != s_PacketHeader.ByteCount )
    {
        KdOut("READ: Data packet error (short read).\n");
        return DBGKD_WAIT_RESEND;
    }

    //
    // Make sure the next byte is packet trailing byte
    //

    Succ = Read(&DataByte, sizeof(DataByte), &BytesRead);
    if ( (!Succ) || BytesRead != sizeof(DataByte) ||
         DataByte != PACKET_TRAILING_BYTE )
    {
        KdOut("READ: Packet trailing byte timeout.\n");
        return DBGKD_WAIT_RESEND;
    }

    //
    // Make sure the checksum is valid.
    //

    Checksum = ComputeChecksum(s_Packet, s_PacketHeader.ByteCount);
    if (Checksum != s_PacketHeader.Checksum)
    {
        KdOut("READ: Checksum error.\n");
        return DBGKD_WAIT_RESEND;
    }

    //
    // We have a valid data packet.  If the packetid is bad, we just
    // ack the packet to the sender will step ahead.  If packetid is bad
    // but SYNC_PACKET_ID bit is set, we sync up.  If packetid is good,
    // or SYNC_PACKET_ID is set, we take the packet.
    //

    KdOut("READ: Received Type %x data packet with id = %lx successfully.\n\n",
          s_PacketHeader.PacketType, s_PacketHeader.PacketId);

    SyncBit = s_PacketHeader.PacketId & SYNC_PACKET_ID;
    s_PacketHeader.PacketId = s_PacketHeader.PacketId & ~SYNC_PACKET_ID;

    //
    // Ack the packet.  SYNC_PACKET_ID bit will ALWAYS be OFF.
    //

    WriteControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                       s_PacketHeader.PacketId);

    //
    // Check the incoming packet Id.
    //

    if ((s_PacketHeader.PacketId != m_PacketExpected) &&
        (SyncBit != SYNC_PACKET_ID))
    {
        KdOut("READ: Unexpected Packet Id (Acked).\n");
        goto WaitForPacketLeader;
    }
    else
    {
        if (SyncBit == SYNC_PACKET_ID)
        {
            //
            // We know SyncBit is set, so reset Expected Ids
            //

            KdOut("READ: Got Sync Id, reset PacketId.\n");

            m_PacketExpected = s_PacketHeader.PacketId;
            m_NextPacketToSend = INITIAL_PACKET_ID;
        }

        m_PacketExpected ^= 1;
    }

    return DBGKD_WAIT_PACKET;
}

ULONG
DbgKdComTransport::WritePacketContents(IN KD_PACKET* Packet,
                                       IN PVOID PacketData,
                                       IN USHORT PacketDataLength,
                                       IN PVOID MorePacketData OPTIONAL,
                                       IN USHORT MorePacketDataLength OPTIONAL,
                                       IN BOOL NoAck)
{
    BOOL Succ;
    ULONG BytesWritten;

    // Lock to ensure all parts of the data are
    // sequential in the stream.
    RESUME_ENGINE();

    //
    // Write the packet header
    //

    Succ = Write(Packet, sizeof(*Packet), &BytesWritten);
    if ( (!Succ) || BytesWritten != sizeof(*Packet))
    {
        //
        // An error occured writing the header, so write it again
        //

        KdOut("WRITE: Packet header error.\n");
        SUSPEND_ENGINE();
        return DBGKD_WRITE_RESEND;
    }

    //
    // Write the primary packet data
    //

    Succ = Write(PacketData, PacketDataLength, &BytesWritten);
    if ( (!Succ) || BytesWritten != PacketDataLength )
    {
        //
        // An error occured writing the primary packet data,
        // so write it again
        //

        KdOut("WRITE: Message header error.\n");
        SUSPEND_ENGINE();
        return DBGKD_WRITE_RESEND;
    }

    //
    // If secondary packet data was specified (WriteMemory, SetContext...)
    // then write it as well.
    //

    if ( ARGUMENT_PRESENT(MorePacketData) )
    {
        Succ = Write(MorePacketData, MorePacketDataLength, &BytesWritten);
        if ( (!Succ) || BytesWritten != MorePacketDataLength )
        {
            //
            // An error occured writing the secondary packet data,
            // so write it again
            //

            KdOut("WRITE: Message data error.\n");
            SUSPEND_ENGINE();
            return DBGKD_WRITE_RESEND;
        }
    }

    //
    // Output a packet trailing byte
    //

    do
    {
        Succ = Write(&s_PacketTrailingByte[0],
                   sizeof(s_PacketTrailingByte),
                   &BytesWritten);
    }
    while ((!Succ) || (BytesWritten != sizeof(s_PacketTrailingByte)));

    SUSPEND_ENGINE();

    if (!NoAck)
    {
        ULONG Received;

        //
        // Wait for ACK
        //

        Received = WaitForPacket(PACKET_TYPE_KD_ACKNOWLEDGE, NULL);
        if (Received != DBGKD_WAIT_ACK)
        {
            KdOut("WRITE: Wait for ACK failed. Resend Packet.\n");
            return DBGKD_WRITE_RESEND;
        }
    }

    return DBGKD_WRITE_PACKET;
}

ULONG
DbgKdComTransport::ReadPacketLeader(
    IN ULONG PacketType,
    OUT PULONG PacketLeader
    )
{
    DWORD BytesRead;
    BOOL Succ;
    USHORT Index;
    UCHAR DataByte, PreviousDataByte;

    Index = 0;
    do
    {
        if (m_BreakIn)
        {
            if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
            {
                WriteBreakInPacket();
                return DBGKD_WAIT_RESYNC;
            }
        }

        if (m_Resync)
        {
            m_Resync = FALSE;

            KdOut(" Resync packet id ...");

            Synchronize();

            KdOut(" Done.\n");
            FlushCallbacks();

            return DBGKD_WAIT_RESYNC;
        }

        if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
        {
            KdOut("Packet read interrupted by exit request\n");
            return DBGKD_WAIT_INTERRUPTED;
        }

        FlushCallbacks();

        Succ = Read(&DataByte, 1, &BytesRead);
        if (Succ && BytesRead == 1 &&
            ( DataByte == PACKET_LEADER_BYTE ||
              DataByte == CONTROL_PACKET_LEADER_BYTE))
        {
            if ( Index == 0 )
            {
                PreviousDataByte = DataByte;
                Index++;
            }
            else if ( DataByte == PreviousDataByte )
            {
                Index++;
            }
            else
            {
                PreviousDataByte = DataByte;
                Index = 1;
            }
        }
        else
        {
            Index = 0;
            if (BytesRead == 0)
            {
                KdOut("READ: Timeout.\n");
                FlushCallbacks();

                if (m_AllowInitialBreak &&
                    (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK))
                {
                    KdOut("Attempting to get initial breakpoint.\n");
                    WriteBreakInPacket();
                }
                return DBGKD_WAIT_FAILED;
            }
        }
    } while ( Index < 2 );

    if ( DataByte != CONTROL_PACKET_LEADER_BYTE )
    {
        *PacketLeader = PACKET_LEADER;
    }
    else
    {
        *PacketLeader = CONTROL_PACKET_LEADER;
    }
    return DBGKD_WAIT_PACKET;
}

void
DbgKdComTransport::CheckComStatus(void)
/*++

Routine Description:

    Called when the com port status trigger signals a change.
    This function handles the change in status.

    Note: status is only monitored when being used over the modem.

--*/
{
    DWORD   CommStat;
    BOOL    Succ;
    ULONG   BytesRead;
    CHAR    Buf[128];
    DWORD   CommErr;
    COMSTAT FullCommStat;
    ULONG   Len;

    if (!m_ComEvent || NET_COM_PORT(m_PortType))
    {
        //
        // Not triggered, just return
        //

        m_ComEvent = 0;
        return;
    }

    // This should succeed since we were just notified,
    // but check the return value to keep PREfix happy.
    if (!GetCommModemStatus(m_Handle, &CommStat))
    {
        // Leave m_ComEvent set for another try.
        return;
    }

    m_ComEvent = 0;

    if (!(CommStat & 0x80))
    {
        dprintf ("No carrier detect - in terminal mode\n");

        // This routine can be called during a wait when the
        // engine lock isn't held and can also be called when
        // the lock is held.  RESUME handles both of these
        // cases so that the lock is reacquired or reentered.
        RESUME_ENGINE();

        //
        // Loop and read any com input
        //

        while (!(CommStat & 0x80))
        {
            //
            // Get some input to send to the modem.
            //

            Len = GetInput("Term> ", Buf, DIMA(Buf), GETIN_DEFAULT);
            if (Len > 0)
            {
                Write(Buf, Len, &Len);
                Buf[0] = '\n';
                Buf[1] = '\r';
                Write(Buf, 2, &Len);
            }

            GetCommModemStatus(m_Handle, &CommStat);
            Succ = Read(Buf, sizeof(Buf), &BytesRead);
            if (Succ != TRUE || BytesRead == 0)
            {
                continue;
            }

            //
            // print the string.
            //

            dprintf("%s", Buf);
            FlushCallbacks();

            //
            // if logging is on, log the output
            //

            if (g_LogFile != -1)
            {
                _write(g_LogFile, Buf, BytesRead);
            }
        }

        dprintf ("Carrier detect - returning to debugger\n");
        FlushCallbacks();

        ClearCommError(m_Handle,
                       &CommErr,
                       &FullCommStat);

        SUSPEND_ENGINE();
    }
    else
    {
        CommErr = 0;
        ClearCommError(m_Handle,
                       &CommErr,
                       &FullCommStat);

        if (CommErr & CE_FRAME)
        {
            dprintf (" [FRAME ERR] ");
        }

        if (CommErr & CE_OVERRUN)
        {
            dprintf (" [OVERRUN ERR] ");
        }

        if (CommErr & CE_RXPARITY)
        {
            dprintf (" [PARITY ERR] ");
        }
    }

    //
    // Reset trigger
    //

    WaitCommEvent (m_Handle, &m_ComEvent, &m_EventOverlapped);
}

//----------------------------------------------------------------------------
//
// DbgKd1394Transport.
//
//----------------------------------------------------------------------------

#define PARAM_1394_SYMLINK "Symlink"
#define PARAM_1394_CHANNEL "Channel"

#define ENV_1394_SYMLINK "_NT_DEBUG_1394_SYMLINK"
#define ENV_1394_CHANNEL "_NT_DEBUG_1394_CHANNEL"

DbgKd1394Transport::DbgKd1394Transport(ConnLiveKernelTargetInfo* Target)
    : DbgKdTransport(Target)
{
    m_Index = DBGKD_TRANSPORT_1394;
    m_Name = g_DbgKdTransportNames[m_Index];
    m_InvPacketRetryLimit = 3;
    m_AckWrites = FALSE;
    m_Handle = NULL;
    m_Handle2 = NULL;
    ZeroMemory(&m_ReadOverlapped2, sizeof(m_ReadOverlapped2));
}

DbgKd1394Transport::~DbgKd1394Transport(void)
{
    if (m_Handle != NULL)
    {
        CloseHandle(m_Handle);
    }

    if (m_Handle2 != NULL)
    {
        CloseHandle(m_Handle2);
    }

    if (m_ReadOverlapped2.hEvent != NULL)
    {
        CloseHandle(m_ReadOverlapped2.hEvent);
    }
}

ULONG
DbgKd1394Transport::GetNumberParameters(void)
{
    return 2 + DbgKdTransport::GetNumberParameters();
}

void
DbgKd1394Transport::GetParameter(ULONG Index,
                                 PSTR Name, ULONG NameSize,
                                 PSTR Value, ULONG ValueSize)
{
    switch(Index)
    {
    case 0:
        if (m_SymlinkSpecified)
        {
            CopyString(Name, PARAM_1394_SYMLINK, NameSize);
            CopyString(Value, m_Symlink, ValueSize);
        }
        break;
    case 1:
        CopyString(Name, PARAM_1394_CHANNEL, NameSize);
        PrintString(Value, ValueSize, "%d", m_Channel);
        break;
    default:
        DbgKdTransport::GetParameter(Index - 2,
                                     Name, NameSize, Value, ValueSize);
        break;
    }
}

void
DbgKd1394Transport::ResetParameters(void)
{
    PSTR Env;

    DbgKdTransport::ResetParameters();

    if ((Env = getenv(ENV_1394_SYMLINK)) == NULL)
    {
        m_SymlinkSpecified = FALSE;
        strcpy(m_Symlink, "Channel");
        strcpy(m_Symlink2, "Instance");
    }
    else
    {
        m_SymlinkSpecified = TRUE;
        CopyString(m_Symlink, Env, DIMA(m_Symlink));
    }

    if ((Env = getenv(ENV_1394_CHANNEL)) == NULL)
    {
        m_Channel = 0;
    }
    else
    {
        m_Channel = atol(Env);
    }
}

BOOL
DbgKd1394Transport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_strcmpi(Name, PARAM_1394_SYMLINK))
    {
        if (!Value)
        {
            ErrOut(PARAM_1394_SYMLINK " requires a value\n");
            return FALSE;
        }

        if (!CopyString(m_Symlink, Value, DIMA(m_Symlink)))
        {
            return FALSE;
        }
        m_SymlinkSpecified = TRUE;
    }
    else if (!_strcmpi(Name, PARAM_1394_CHANNEL))
    {
        if (!Value)
        {
            ErrOut(PARAM_1394_CHANNEL " requires a numeric value\n");
            return FALSE;
        }

        m_Channel = atol(Value);
    }
    else
    {
        return DbgKdTransport::SetParameter(Name, Value);
    }

    return TRUE;
}

HRESULT
DbgKd1394Transport::Initialize(void)
{
    CHAR InfFile[MAX_PATH];
    char Name[64];
    HRESULT Status;
    HANDLE InfHandle;

    dprintf("Using 1394 for debugging\n");

    if ((Status = DbgKdTransport::Initialize()) != S_OK)
    {
        return Status;
    }

    m_ReadOverlapped2.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_ReadOverlapped2.hEvent == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    //
    // Debugging over 1394 requires drivers to be installed.
    // The the drivers registered so installation can succeed.
    //

    // Get the directory the debugger executable is in.
    // Remove the executable name and add the inf name.
    if (GetEngineDirectory(InfFile, DIMA(InfFile)) &&
        CatString(InfFile, "\\1394\\1394dbg.inf", DIMA(InfFile)))
    {
        InfHandle = CreateFile(InfFile,
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

        if (InfHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(InfHandle);

            typedef HDESK (__stdcall *PFN_SETUPCOPYOEMINFPROC)(
                 PCSTR   SourceInfFileName,
                 PCSTR   OEMSourceMediaLocation,
                 DWORD   OEMSourceMediaType,
                 DWORD   CopyStyle,
                 PSTR    DestinationInfFileName,
                 DWORD   DestinationInfFileNameSize,
                 PDWORD  RequiredSize,
                 PSTR   *DestinationInfFileNameComponent);

            HINSTANCE hmod = LoadLibrary("setupapi.dll");

            PFN_SETUPCOPYOEMINFPROC pfnCopyOemInf = (PFN_SETUPCOPYOEMINFPROC)
                GetProcAddress(hmod, "SetupCopyOEMInfA");

            CHAR DestinationInfFile[MAX_PATH];
            ULONG RequiredSize;

            if (!pfnCopyOemInf)
            {
                dprintf("1394 debug drivers can not be installed on this platform\n");
                return E_FAIL;
            }

            if (!(*pfnCopyOemInf)(InfFile,
                                  NULL,
                                  SPOST_PATH,
                                  SP_COPY_DELETESOURCE,
                                  DestinationInfFile,
                                  sizeof(DestinationInfFile),
                                  &RequiredSize,
                                  NULL))
            {
                dprintf("Could not install 1394 device drivers - error %x\n",
                         GetLastError());
                return E_FAIL;
            }

            typedef BOOL (__stdcall *PFN_UPDATEDRIVER)(
                 HWND   HwndParent,
                 LPCSTR HardwareId,
                 LPCSTR FullInfPath,
                 DWORD  InstallFlags,
                 PBOOL  bRebootRequired);

            hmod = LoadLibrary("newdev.dll");

            PFN_UPDATEDRIVER pfnUpdateDriver = (PFN_UPDATEDRIVER)
                GetProcAddress(hmod, "UpdateDriverForPlugAndPlayDevicesA");

            if (pfnUpdateDriver)
            {
                if (!(*pfnUpdateDriver)(NULL,
                                        "V1394\\VIRTUAL_HOST_DEBUGGER",
                                        DestinationInfFile,
                                        INSTALLFLAG_FORCE,
                                        NULL) ||
                    !(*pfnUpdateDriver)(NULL,
                                        "V1394\\HOST_DEBUGGER",
                                        DestinationInfFile,
                                        INSTALLFLAG_FORCE,
                                        NULL))
                {
                    dprintf("Could not update 1394 device drivers - error %x\n",
                             GetLastError());
                }
            }
        }
    }

    m_DirectPhysicalMemory = TRUE;

    Status = Create1394Channel(m_Symlink, m_Channel,
                               Name, DIMA(Name), &m_Handle);
    if (!m_SymlinkSpecified)
    {
        // The user didn't specify a symlink so we'll open
        // both and see which one responds.

        HRESULT Status2;

        Status2 = Create1394Channel(m_Symlink2, m_Channel,
                                    Name, DIMA(Name), &m_Handle2);
        if (Status2 == S_OK)
        {
            Status = S_OK;
        }
    }
    if (Status != S_OK)
    {
        ErrOut("Failed to open 1394 channel %d\n", m_Channel);
        ErrOut("If this is the first time KD was run, this is"
               " why this failed.\nVirtual 1394 "
               "Debugger Driver Installation will now be attempted\n");
        return Status;
    }
    else
    {
        dprintf("Opened %s\n", Name);
    }

    //
    // put the virtual driver in the right operating mode..
    //

    if (!SwitchVirtualDebuggerDriverMode
        (V1394DBG_API_CONFIGURATION_MODE_DEBUG))
    {
        return E_FAIL;
    }

    return S_OK;
}

BOOL
DbgKd1394Transport::Read(
    IN PVOID    Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesRead
    )
{
    BOOL Status;
    HANDLE Handles[2];
    ULONG Count = 0;
    HANDLE FirstHandle = NULL;
    OVERLAPPED *FirstOverlapped = NULL;

    if (IS_DUMP_TARGET(m_Target))
    {
        ErrOut("Attempted to read KD transport while "
               "debugging a crash dump\n");
        DebugBreak();
    }

    if (!SwitchVirtualDebuggerDriverMode
        (V1394DBG_API_CONFIGURATION_MODE_DEBUG))
    {
        return FALSE;
    }

    //
    // We may have two handles open as we haven't decided
    // which symlink to use yet.  Read on both and
    // pick whichever one answers first.
    //

    Status = ReadFile(m_Handle,
                      Buffer,
                      SizeOfBuffer,
                      BytesRead,
                      &m_ReadOverlapped);
    if (Status)
    {
        // Success on m_Handle, close m_Handle2.
        CloseSecond(FALSE);
        goto Exit;
    }
    else if (GetLastError() == ERROR_IO_PENDING)
    {
        Handles[Count++] = m_ReadOverlapped.hEvent;
        FirstHandle = m_Handle;
        FirstOverlapped = &m_ReadOverlapped;
    }

    if (m_Handle2)
    {
        Status = ReadFile(m_Handle2,
                          Buffer,
                          SizeOfBuffer,
                          BytesRead,
                          &m_ReadOverlapped2);
        if (Status)
        {
            // Success on m_Handle2, close m_Handle.
            CloseSecond(TRUE);
            goto Exit;
        }
        else if (GetLastError() == ERROR_IO_PENDING)
        {
            if (!Count)
            {
                FirstHandle = m_Handle2;
                FirstOverlapped = &m_ReadOverlapped2;
            }
            Handles[Count++] = m_ReadOverlapped2.hEvent;
        }
    }

    //
    // If both requests failed, Prevent looping on read errors from
    // burning 100% of the CPU.

    if (!Count)
    {
        Sleep(50);
        goto Exit;
    }

    //
    // We now have one or two pending I/Os, so wait to see
    // what completes.
    //

    ULONG Wait;

    Wait = WaitForMultipleObjects(Count, Handles, FALSE, INFINITE);
    switch(Wait)
    {
    case WAIT_OBJECT_0:
        if (Count == 2)
        {
            CancelIo(m_Handle2);
        }

        Status = GetOverlappedResult(FirstHandle,
                                     FirstOverlapped,
                                     BytesRead,
                                     FALSE);

        // Close the handle we are not using
        CloseSecond(FirstHandle == m_Handle2);

        break;
    case WAIT_OBJECT_0 + 1:
        CancelIo(m_Handle);
        Status = GetOverlappedResult(m_Handle2,
                                     &m_ReadOverlapped2,
                                     BytesRead,
                                     FALSE);
        CloseSecond(TRUE);
        break;
    default:
        CancelIo(FirstHandle);
        if (Count == 2)
        {
            CancelIo(m_Handle2);
        }
        Status = FALSE;
        break;
    }

 Exit:
    if (Status)
    {
        if (m_OutputIo & DBGKD_OUTPUT_READS)
        {
            OutputIo("1394: Read %d bytes of %d\n",
                     Buffer, SizeOfBuffer, *BytesRead);
        }

        m_BytesRead += *BytesRead;
    }

    return Status;
}

BOOL
DbgKd1394Transport::Write(
    IN PVOID    Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesWritten
    )
{
    BOOL Status;

    if (IS_DUMP_TARGET(m_Target))
    {
        ErrOut("Attempted to write KD transport "
               "while debugging a crash dump\n");
        DebugBreak();
    }

    if (!SwitchVirtualDebuggerDriverMode
        (V1394DBG_API_CONFIGURATION_MODE_DEBUG))
    {
        return FALSE;
    }

    //
    // We may have two handles open as we haven't decided
    // which symlink to use yet.  Write to both.
    //

    Status = WriteFile(m_Handle,
                       Buffer,
                       SizeOfBuffer,
                       BytesWritten,
                       &m_WriteOverlapped);
    if (!Status)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            Status = GetOverlappedResult(m_Handle,
                                         &m_WriteOverlapped,
                                         BytesWritten,
                                         TRUE);
            if (!Status)
            {
                return Status;
            }
        }
    }

    if (m_Handle2)
    {
        Status = WriteFile(m_Handle2,
                           Buffer,
                           SizeOfBuffer,
                           BytesWritten,
                           &m_WriteOverlapped);
        if (!Status)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                Status = GetOverlappedResult(m_Handle2,
                                             &m_WriteOverlapped,
                                             BytesWritten,
                                             TRUE);
                if (!Status)
                {
                    return Status;
                }
            }
        }
    }

    if (Status)
    {
        if (m_OutputIo & DBGKD_OUTPUT_WRITES)
        {
            OutputIo("1394: Wrote %d bytes of %d\n",
                     Buffer, SizeOfBuffer, *BytesWritten);
        }

        m_BytesWritten += *BytesWritten;
    }

    return Status;
}

HRESULT
DbgKd1394Transport::ReadTargetPhysicalMemory(
    IN ULONG64 MemoryOffset,
    IN PVOID Buffer,
    IN ULONG SizeofBuffer,
    IN PULONG BytesRead
    )
{
    DWORD dwRet, dwBytesRet;
    PV1394DBG_API_REQUEST pApiReq;

    if (IS_DUMP_TARGET(m_Target))
    {
        ErrOut("Attempted to access KD transport while "
               "debugging a crash dump\n");
        DebugBreak();
    }

    if (m_Handle2)
    {
        // We should know what kind of communication is
        // occurring by now.
        ErrOut("Symlink must be established\n");
        return E_UNEXPECTED;
    }

    //
    // first setup the read i/o parameters in the virtual driver
    //

    pApiReq = (PV1394DBG_API_REQUEST)
        LocalAlloc(LPTR, sizeof(V1394DBG_API_REQUEST));
    if (pApiReq == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // if the virtual driver is not set in raw access mode, we need to
    // tell it to change modes..
    //

    if (!SwitchVirtualDebuggerDriverMode
        (V1394DBG_API_CONFIGURATION_MODE_RAW_MEMORY_ACCESS))
    {
        LocalFree(pApiReq);
        return E_UNEXPECTED;
    }

    pApiReq->RequestNumber = V1394DBG_API_SET_IO_PARAMETERS;
    pApiReq->Flags = V1394DBG_API_FLAG_READ_IO;

    pApiReq->u.SetIoParameters.fulFlags = 0;
    pApiReq->u.SetIoParameters.StartingMemoryOffset.QuadPart = MemoryOffset;

    dwRet = DeviceIoControl( m_Handle,
                             IOCTL_V1394DBG_API_REQUEST,
                             pApiReq,
                             sizeof(V1394DBG_API_REQUEST),
                             NULL,
                             0,
                             &dwBytesRet,
                             NULL
                             );
    if (!dwRet)
    {
        dwRet = GetLastError();
        ErrOut("Failed to send SetIoParameters 1394 "
               "Virtual Driver Request, error %x\n",dwRet);

        LocalFree(pApiReq);
        return E_UNEXPECTED;
    }

    LocalFree(pApiReq);

    //
    // now do anormal read. The virtual driver will read SizeofBuffer bytes
    // starting at the remote PCs physical address we specified above
    //

    dwRet = ReadFile(
             m_Handle,
             Buffer,
             SizeofBuffer,
             BytesRead,
             &m_ReadOverlapped
             );
    if (!dwRet)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            dwRet = GetOverlappedResult(m_Handle,
                                        &m_ReadOverlapped,
                                        BytesRead,
                                        TRUE);
        }
    }

    return (dwRet != 0) ? S_OK : E_UNEXPECTED;
}

BOOL
DbgKd1394Transport::SwitchVirtualDebuggerDriverMode(
    IN ULONG    DesiredOperationMode
    )
{
    DWORD   dwRet, dwBytesRet;
    PV1394DBG_API_REQUEST pApiReq;

    //
    // If the virtual driver is not set in raw access mode, we need to
    // tell it to change modes..
    //
    // We may have two handles open as we haven't decided
    // which symlink to use yet.  Write to both.
    //

    if (m_OperationMode != DesiredOperationMode)
    {
        //
        // first setup the read i/o parameters in the virtual driver
        //

        pApiReq = (PV1394DBG_API_REQUEST)
            LocalAlloc(LPTR, sizeof(V1394DBG_API_REQUEST));
        if (pApiReq == NULL)
        {
            return FALSE;
        }

        pApiReq->RequestNumber = V1394DBG_API_SET_CONFIGURATION;
        pApiReq->Flags = 0;

        pApiReq->u.SetConfiguration.OperationMode = DesiredOperationMode;

        dwRet = DeviceIoControl(m_Handle,
                                IOCTL_V1394DBG_API_REQUEST,
                                pApiReq,
                                sizeof(V1394DBG_API_REQUEST),
                                NULL,
                                0,
                                &dwBytesRet,
                                NULL);
        if (!dwRet)
        {
            dwRet = GetLastError();
            ErrOut("Failed to send SetConfiguration 1394 "
                   "Virtual Driver Request, error %x\n", dwRet);
            LocalFree(pApiReq);
            return FALSE;
        }

        if (m_Handle2)
        {
            dwRet = DeviceIoControl(m_Handle2,
                                    IOCTL_V1394DBG_API_REQUEST,
                                    pApiReq,
                                    sizeof(V1394DBG_API_REQUEST),
                                    NULL,
                                    0,
                                    &dwBytesRet,
                                    NULL);
            if (!dwRet)
            {
                dwRet = GetLastError();
                ErrOut("Failed to send SetConfiguration 1394 "
                       "Virtual Driver Request, error %x\n", dwRet);
                LocalFree(pApiReq);
                return FALSE;
            }
        }

        m_OperationMode = DesiredOperationMode;

        LocalFree(pApiReq);
    }

    return TRUE;
}

VOID
DbgKd1394Transport::Synchronize(VOID)
{
    ULONG Index;
    ULONG BytesRead;
    BOOL Succ;

    // XXX drewb - Why is this code disabled?
    return;

    Index = 3;
    while (TRUE)
    {
        if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
        {
            KdOut("Synchronize interrupted by exit request\n");
            return;
        }

        WriteControlPacket(PACKET_TYPE_KD_RESET, 0L);

        FlushCallbacks();

        Succ = Read(s_Packet, sizeof(s_Packet), &BytesRead);

        CopyMemory(&s_PacketHeader, &s_Packet[0], sizeof(KD_PACKET));

        if (Succ && (BytesRead >= sizeof(s_PacketHeader)))
        {
            if (s_PacketHeader.PacketType == PACKET_TYPE_KD_RESET)
            {
                break;
            }
        }

        if (!Index--)
        {
            break;
        }
    }
}

ULONG
DbgKd1394Transport::ReadPacketContents(IN USHORT PacketType)
{
    DWORD BytesRead;
    BOOL Succ;
    ULONG Checksum;

WaitForPacket1394:

    if (m_AllowInitialBreak && (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK))
    {
        KdOut("Attempting to get initial breakpoint.\n");
        WriteBreakInPacket();
        m_AllowInitialBreak = FALSE;
    }

    if (m_Resync)
    {
        m_Resync = FALSE;

        KdOut(" Resync packet id ...");

        Synchronize();

        KdOut(" Done.\n");
        FlushCallbacks();

        return DBGKD_WAIT_RESYNC;
    }

    if (m_BreakIn)
    {
        WriteBreakInPacket();
        return DBGKD_WAIT_RESYNC;
    }

    if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
    {
        KdOut("Packet read interrupted by exit request\n");
        return DBGKD_WAIT_INTERRUPTED;
    }

    FlushCallbacks();

    //
    // read the whole packet at once.
    // we try to read MAX_PACKET worth of data and then check how much
    // we really read. Also since the packet header (KD_PACKET) is part of what
    // we read, we later have to move the data packet back sizeof(KD_PACKET)
    //

    Succ = Read(s_Packet, sizeof(s_Packet), &BytesRead);
    CopyMemory(&s_PacketHeader, &s_Packet[0], sizeof(KD_PACKET));

    if (!Succ || (BytesRead < sizeof(s_PacketHeader)))
    {
        if (!Succ)
        {
            KdOut("READ: Error %x.\n",GetLastError());
        }
        else
        {
            KdOut("READ: Data ByteCount error (short read) %x, %x.\n",
                  BytesRead, sizeof(s_PacketHeader));
        }

        if (Succ && (BytesRead >= sizeof(s_PacketHeader)) )
        {
            if (s_PacketHeader.PacketLeader == PACKET_LEADER)
            {
                WriteControlPacket(PACKET_TYPE_KD_RESEND, 0L);
                KdOut("READ: Data packet header "
                      "ByteCount error (short read).\n");
            }
        }

        goto WaitForPacket1394;
    }

    //
    // move data portion to start of packet.
    //

    MoveMemory(s_Packet, ((PUCHAR)s_Packet + sizeof(KD_PACKET)),
               BytesRead - sizeof(KD_PACKET));

    //
    // Check the Packet type.
    //

    if ((m_Target->m_KdMaxPacketType == 0 &&
         s_PacketHeader.PacketType >= PACKET_TYPE_MAX) ||
        (m_Target->m_KdMaxPacketType > 0 &&
         s_PacketHeader.PacketType >= m_Target->m_KdMaxPacketType))
    {
        KdOut("READ: Received INVALID packet type.\n");

        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            return DBGKD_WAIT_RESEND;
        }

        return DBGKD_WAIT_FAILED;
    }

    KdOut("      PacketType=%x, ", s_PacketHeader.PacketType);

    //
    // Check ByteCount
    //

    if (s_PacketHeader.ByteCount > PACKET_MAX_SIZE )
    {
        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            KdOut("READ: Data packet header ByteCount error (short read).\n");
            return DBGKD_WAIT_RESEND;
        }

        return DBGKD_WAIT_FAILED;
    }

    KdOut("ByteCount=%x, PacketId=%x,\n",
          s_PacketHeader.ByteCount,
          s_PacketHeader.PacketId);

    if (s_PacketHeader.ByteCount != (BytesRead - sizeof(s_PacketHeader)))
    {
        if (s_PacketHeader.PacketLeader == PACKET_LEADER)
        {
            KdOut("READ: Data packet header ByteCount error (short read).\n");
            return DBGKD_WAIT_RESEND;
        }

        return DBGKD_WAIT_FAILED;
    }

    //
    // Make sure the checksum is valid.
    //

    Checksum = ComputeChecksum(s_Packet, s_PacketHeader.ByteCount);
    if (Checksum != s_PacketHeader.Checksum)
    {
        KdOut("READ: Checksum error.\n");
        return DBGKD_WAIT_RESEND;
    }

    if (s_PacketHeader.PacketLeader == CONTROL_PACKET_LEADER)
    {
        if (s_PacketHeader.PacketType == PACKET_TYPE_KD_RESET)
        {
            //
            // if we received Reset packet, reset the packet control variables
            // and resend earlier packet.
            //

            m_NextPacketToSend = INITIAL_PACKET_ID;
            m_PacketExpected = INITIAL_PACKET_ID;
            WriteControlPacket(PACKET_TYPE_KD_RESET, 0L);

            KdOut("DbgKdpWaitForPacket(): "
                  "Recieved KD_RESET packet, send KD_RESET ACK packet\n");
            FlushCallbacks();

            return DBGKD_WAIT_FAILED;
        }
        else if (s_PacketHeader.PacketType == PACKET_TYPE_KD_RESEND)
        {
            KdOut("READ: Received RESEND packet\n");
            FlushCallbacks();

            return DBGKD_WAIT_FAILED;
        }
        else
        {
            //
            // Invalid packet header, ignore it.
            //

            KdOut("READ: Received Control packet with UNKNOWN type\n");
            FlushCallbacks();

            return DBGKD_WAIT_FAILED;
        }
    }

    //
    // we are waiting for data packet and we received the packet header
    // for data packet. Perform the following checkings to make sure
    // it is the packet we are waiting for.
    //

    KdOut("READ: Received Type %x data packet with id = %lx successfully.\n\n",
          s_PacketHeader.PacketType, s_PacketHeader.PacketId);

    return DBGKD_WAIT_PACKET;
}

ULONG
DbgKd1394Transport::WritePacketContents(IN KD_PACKET* Packet,
                                        IN PVOID PacketData,
                                        IN USHORT PacketDataLength,
                                        IN PVOID MorePacketData OPTIONAL,
                                        IN USHORT MorePacketDataLength OPTIONAL,
                                        IN BOOL NoAck)
{
    BOOL Succ;
    ULONG BytesWritten;
    PUCHAR Tx;

    // Lock to ensure only one thread is using
    // the transmit buffer.
    RESUME_ENGINE();

    //
    // On 1394 we double buffer all packet segments into one contigious
    // buffer and write it all at once
    //

    Tx = m_TxPacket;

    memcpy(Tx, Packet, sizeof(*Packet));
    Tx += sizeof(*Packet);

    memcpy(Tx, PacketData, PacketDataLength);
    Tx += PacketDataLength;

    if ( ARGUMENT_PRESENT(MorePacketData) )
    {
        memcpy(Tx, MorePacketData, MorePacketDataLength);
        Tx += MorePacketDataLength;
    }

    //
    // The 1394 Debug protocol does not use trailer bytes
    //

    //
    // Write the whole packet out to the bus
    //

    do
    {
        Succ = Write(&m_TxPacket[0], (ULONG)(Tx - m_TxPacket), &BytesWritten);
    }
    while ((!Succ) || (BytesWritten != (ULONG)(Tx - m_TxPacket)));

    SUSPEND_ENGINE();

    return DBGKD_WRITE_PACKET;
}

void
DbgKd1394Transport::CloseSecond(BOOL MakeFirst)
{
    if (!m_Handle2)
    {
        // No secondary open.
        return;
    }

    if (MakeFirst)
    {
        CloseHandle(m_Handle);
        m_Handle = m_Handle2;
        m_Handle2 = NULL;
        strcpy(m_Symlink, m_Symlink2);
    }
    else
    {
        CloseHandle(m_Handle2);
        m_Handle2 = NULL;
    }
}
