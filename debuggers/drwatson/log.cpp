/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    log.cpp

Abstract:

    This file implements the access to the postmortem log file.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


#define BYTE_ORDER_MARK           0xFEFF
#define MAX_PRINTF_BUF_SIZE (1024 * 4)

enum LOG_TYPE {
    LT_ANSI,
    LT_UNICODE
};

//
// global variables for this module
//
static HANDLE  hFile = NULL;
static DWORD   dwStartingPos = 0;
static WCHAR   wchBOM = BYTE_ORDER_MARK;
static DWORD   dwLogType = LT_ANSI;

void
WriteAnsiCharsToLogFile(
    WCHAR *pszUnicode,
    DWORD dwCharCount
    )
{
    char szAnsiBuf[MAX_PRINTF_BUF_SIZE] = {0};
    DWORD nCharTranslated;
    WCHAR szWcharBuf[MAX_PRINTF_BUF_SIZE] = {0};

    nCharTranslated = WideCharToMultiByte(CP_ACP, WC_SEPCHARS | WC_COMPOSITECHECK,
        pszUnicode, dwCharCount,
        szAnsiBuf, sizeof(szAnsiBuf), NULL, NULL);

    WriteFile( hFile, szAnsiBuf, nCharTranslated, &nCharTranslated, NULL );
#if 0
    nCharTranslated = MultiByteToWideChar(CP_ACP, WC_SEPCHARS | WC_COMPOSITECHECK,
                                          szAnsiBuf, nCharTranslated,
                                          szWcharBuf, sizeof(szWcharBuf)/sizeof(WCHAR));

    WriteFile( hFile, szWcharBuf, nCharTranslated, &nCharTranslated, NULL );
#endif
}

void
__cdecl
lprintf(
    DWORD dwFormatId,
    ...
    )

/*++

Routine Description:

    This is function is a printf style function for printing messages
    in a message file.

Arguments:

    dwFormatId    - format id in the message file

    ...           - var args

Return Value:

    None.

--*/

{
    _TCHAR       buf[MAX_PRINTF_BUF_SIZE] = {0};
    DWORD       dwCount;
    va_list     args;

    va_start( args, dwFormatId );

    dwCount = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE,
                NULL,
                dwFormatId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), // Default language
                buf,
                sizeof(buf) / sizeof(_TCHAR),
                &args
                );

    va_end(args);

    if (dwCount == 0) {
        // Failure, nothing to write.
        return;
    }

#ifdef UNICODE
    // We currently want all of the output to be in ANSI
    if (dwLogType == LT_ANSI) {
        WriteAnsiCharsToLogFile(buf, dwCount);
    } else {
        WriteFile( hFile, buf, dwCount * sizeof(_TCHAR), &dwCount, NULL );
    }
#else
    WriteFile( hFile, buf, dwCount, &dwCount, NULL );
#endif

    return;
}

void
__cdecl
lprintfs(
    _TCHAR *format,
    ...
    )

/*++

Routine Description:

    This is function is a printf replacement that writes the output to
    the DrWatson log file.

Arguments:

    format        - print format

    ...           - var args

Return Value:

    None.

--*/

{
    _TCHAR   buf[MAX_PRINTF_BUF_SIZE] = {0};
    int      chars;
    DWORD    cb;

    va_list arg_ptr;
    va_start(arg_ptr, format);
    chars = _vsntprintf(buf, _tsizeof(buf), format, arg_ptr);
    buf[_tsizeof(buf) - 1] = 0;
    if (chars < 0 || chars == _tsizeof(buf)) {
        cb = _tsizeof(buf) - 1;
    } else {
        cb = chars;
    }
    va_end(arg_ptr);
    Assert( hFile != NULL );

#ifdef UNICODE
    // We currently want all of the output to be in ANSI
    if (dwLogType == LT_ANSI) {
        WriteAnsiCharsToLogFile(buf, cb);
    } else {
        WriteFile( hFile, buf, cb * sizeof(_TCHAR), &cb, NULL );
    }
#else
    WriteFile( hFile, buf, cb * sizeof(_TCHAR), &cb, NULL );
#endif
}

void
OpenLogFile(
    _TCHAR *szFileName,
    BOOL fAppend,
    BOOL fVisual
    )

/*++

Routine Description:

    Opens the DrWatson logfile for reading & writting.

Arguments:

    szFileName    - logfile name

    fAppend       - append the new data to the end of the file or
                    create a new file

    fVisual       - visual notification

Return Value:

    None.

--*/

{
    DWORD size;
    DWORD Retries = 10;

    //
    // The log file may be in use by another instance
    // of Dr. Watson, so try and open it several times
    // with a wait inbetween.
    //
    
    for (;;) {
        
        hFile = CreateFile( szFileName,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
        if (hFile != INVALID_HANDLE_VALUE) {
            break;
        }
        
        if (fVisual) {
            NonFatalError( LoadRcString(IDS_INVALID_LOGFILE) );
            _tgetcwd( szFileName, MAX_PATH );
            if (!BrowseForDirectory(NULL, szFileName, MAX_PATH )) {
                FatalError( GetLastError(),
                            LoadRcString(IDS_CANT_OPEN_LOGFILE) );
            }
            MakeLogFileName( szFileName );
        }
        else if (Retries-- == 0) {
            ExitProcess( 1 );
        }
        else {
            Sleep(1000);
        }
    }

    if (!fAppend || GetLastError() != ERROR_ALREADY_EXISTS) {

        //
        // The file was just created, so put a header in it.
        //

        dwLogType = LT_UNICODE;
        WriteFile( hFile, &wchBOM, sizeof(_TCHAR), &size, NULL );
        lprintfs( _T("\r\n") );
        lprintf( MSG_BANNER );
        lprintfs( _T("\r\n") );
        
    } else {
        
#ifdef UNICODE
        //
        // Check if the file is Unicode or ANSI
        //
        WCHAR wchHdr = 0;

        dwLogType = LT_ANSI;
        if (ReadFile( hFile, &wchHdr, sizeof(TCHAR), &size, NULL )) {
            if (wchHdr == wchBOM) {
                dwLogType = LT_UNICODE;
            }
        }
#endif

        SetFilePointer( hFile, 0, 0, FILE_END );
    }

    dwStartingPos = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
}

void
CloseLogFile(
    void
    )

/*++

Routine Description:

    Closes the DrWatson logfile & releases the semaphore that
    protects it.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CloseHandle( hFile );
}

_TCHAR *
GetLogFileData(
    PDWORD pdwLogFileDataSize
    )

/*++

Routine Description:

    Reads in all of the logfile data that has been written since it was
    opened.  The data is placed into a buffer allocated by this function.
    The caller is responsible for freeing the memory.

Arguments:

    pdwLogFileDataSize     -  pointer to a dword that contains the size
                              in bytes of the data that is read.

Return Value:

    Valid character pointer to the logfile data

    NULL - could not read the data.

--*/

{
    DWORD   dwCurrPos;
    _TCHAR   *p;
    DWORD   size;


    dwCurrPos = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );

    *pdwLogFileDataSize = 0;
    size = dwCurrPos - dwStartingPos;

    p = (_TCHAR *) calloc( size, sizeof(_TCHAR) );
    if (p == NULL) {
        return NULL;
    }

    SetFilePointer( hFile, dwStartingPos, NULL, FILE_BEGIN );

    if (!ReadFile( hFile, p, size, &size, NULL )) {
        free( p );
        p = NULL;
        size = 0;
    }

    SetFilePointer( hFile, dwCurrPos, NULL, FILE_BEGIN );

    *pdwLogFileDataSize = size;

    return p;
}

void
MakeLogFileName(
    _TCHAR *szName
    )

/*++

Routine Description:

    Concatenates the base logfile name on to the string passed in.

Arguments:

    szName                 -  buffer for the logfile name.

Return Value:

    None.

--*/

{
    if (_tcslen(szName) + 16 < MAX_PATH) {
        _tcscat( szName, _T("\\drwtsn32.log") );
    }
}
