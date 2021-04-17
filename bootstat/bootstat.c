/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cdp.c

Abstract:

    A user mode app that allows simple commands to be sent to a
    selected scsi device.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <bootstatus.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <windows.h>

// #include "bootstatus.h"

#ifdef DBG
#define dbg(x) x
#define HELP_ME() printf("Reached line %4d\n", __LINE__);
#else
#define dbg(x)    /* x */
#define HELP_ME() /* printf("Reached line %4d\n", __LINE__); */
#endif

#define ARGUMENT_USED(x)    (x == NULL)


//
// The default size of the safemode.dat file - it must be large enough that the 
// data stream isn't a resident attribute of the metadata or the loader's 
// NTFS implementation can't write to it.
//

#define DEFAULT_SAFEMODE_FILE_SIZE  0x800

typedef struct {
    PCHAR Name;
    PCHAR Description;
    BOOLEAN LockData;
    ULONG (*Function)(HANDLE BootStatusData, int argc, char *argv[]);
} COMMAND;

DWORD TestCommand(HANDLE BootStatusData, int argc, char *argv[]);
DWORD Disable(HANDLE BootStatusData, int argc, char *argv[]);
DWORD Enable(HANDLE BootStatusData, int argc, char *argv[]);
DWORD Create(HANDLE BootStatusData, int argc, char *argv[]);
DWORD CreateEx(HANDLE BootStatusData, int argc, char *argv[]);
DWORD SetFlags(HANDLE BootStatusData, int argc, char *argv[]);
DWORD ClearFlags(HANDLE BootStatusData, int argc, char *argv[]);
DWORD ListSettings(HANDLE BootStatusData, int argc, char *argv[]);

DWORD ListCommand(int argc, char *argv[]);

//
// List of commands
// all command names are case sensitive
// arguments are passed into command routines
// list must be terminated with NULL command
// command will not be listed in help if description == NULL
//

COMMAND CommandArray[] = {
    {"create",  "Creates the boot status data file",     FALSE, Create},
    {"createex",  "Tests creation code",                 FALSE, CreateEx},
    {"disable", "Disables Auto Advanced Boot",           TRUE,  Disable},
    {"enable",  "Enables Auto Advanced Boot",            TRUE,  Enable},
    {"set",     "Sets the boot status flags",            TRUE,  SetFlags},
    {"settings","Lists the Auto Advanced Boot settings", TRUE,  ListSettings},
    {NULL, NULL, FALSE, NULL}
    };

int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    HANDLE bootStatusData;
    BOOLEAN bsdLocked = FALSE;

    if(argc < 2) {
        printf("Usage: %s <command> [parameters]\n", argv[0]);
        printf("possible commands: \n");
        ListCommand(argc, argv);
        printf("\n");
        return -1;
    }

    //
    // Iterate through the command array and find the correct function to
    // call.
    //

    while(CommandArray[i].Name != NULL) {

        if(strcmp(argv[1], CommandArray[i].Name) == 0) {

            NTSTATUS status;

            if(CommandArray[i].LockData) {
                status = RtlLockBootStatusData(&bootStatusData);
    
                if(!NT_SUCCESS(status)) {
                    printf("Error %#08lx opening boot status bootStatusData\n", 
                           status);
                    return -1;
                }

                bsdLocked = TRUE;
            } else {
                bootStatusData = NULL;
            }

            (CommandArray[i].Function)(bootStatusData, (argc - 1), &(argv[1]));

            break;
        }

        i++;
    }

    if(CommandArray[i].Name == NULL) {
        printf("Unknown command %s\n", argv[1]);
    }

    if(bsdLocked) {
        RtlUnlockBootStatusData(bootStatusData);
    }
    return 0;
}



DWORD TestCommand(HANDLE BootStatusData, int argc, char *argv[])
/*++

Routine Description:

    Tests the command "parsing"

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/

{
    int i;

    UNREFERENCED_PARAMETER(BootStatusData);

    printf("Test - %d additional arguments\n", argc);

    for(i = 0; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }

    return STATUS_SUCCESS;
}

DWORD ListCommand(int argc, char *argv[])
/*++

Routine Description:

    Prints out the command list

Arguments:
    device - unused

    argc - unused

    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    int i = 0;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    while(CommandArray[i].Name != NULL) {

        if(CommandArray[i].Description != NULL) {

            printf("\t%s - %s\n",
                   CommandArray[i].Name,
                   CommandArray[i].Description);
        }

        i++;
    }

    return STATUS_SUCCESS;
}

DWORD 
Create(
    HANDLE Unused, 
    int argc, 
    char *argv[]
    )
{
    BOOLEAN enabled;

    NTSTATUS status;

    status = RtlCreateBootStatusDataFile();

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx creating boot status data\n", status);
    }

    return status;
}

DWORD 
Enable(
    HANDLE BootStatusData, 
    int argc, 
    char *argv[]
    )
{
    BOOLEAN enabled;

    NTSTATUS status;

    status = RtlGetSetBootStatusData(BootStatusData,
                                     TRUE,
                                     RtlBsdItemAabEnabled,
                                     &enabled,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx reading boot status data\n", status);
        return status;
    }

    printf("Enabling Automatic Advanced Boot\n");
    printf("Previous setting was %s\n", enabled ? "enabled" : "disabled");

    enabled = TRUE;

    status = RtlGetSetBootStatusData(BootStatusData,
                                     FALSE,
                                     RtlBsdItemAabEnabled,
                                     &enabled,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx writing boot status data\n", status);
    }

    return status;
}

DWORD 
Disable(
    HANDLE BootStatusData, 
    int argc, 
    char *argv[]
    )
{
    BOOLEAN enabled;

    NTSTATUS status;

    status = RtlGetSetBootStatusData(BootStatusData,
                                     TRUE,
                                     RtlBsdItemAabEnabled,
                                     &enabled,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx reading boot status data\n", status);
        return status;
    }

    printf("Disabling Automatic Advanced Boot\n");
    printf("Previous setting was %s\n", enabled ? "enabled" : "disabled");

    enabled = FALSE;

    status = RtlGetSetBootStatusData(BootStatusData,
                                     FALSE,
                                     RtlBsdItemAabEnabled,
                                     &enabled,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx writing boot status data\n", status);
    }

    return status;
}

DWORD 
ListSettings(
    HANDLE BootStatusData, 
    int argc, 
    char *argv[]
    )
{
    BOOLEAN flag;

    NTSTATUS status;

    status = RtlGetSetBootStatusData(BootStatusData,
                                     TRUE,
                                     RtlBsdItemAabEnabled,
                                     &flag,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx reading boot status data\n", status);
        return status;
    }

    printf("Automatic Advanced Boot is %s\n", flag ? "enabled" : "disabled");

    status = RtlGetSetBootStatusData(BootStatusData,
                                     TRUE,
                                     RtlBsdItemAabTimeout,
                                     &flag,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx reading boot status data\n", status);
        return status;
    }

    printf("Automatic Advanced Boot timeout is %d seconds\n", (UCHAR) flag);

    status = RtlGetSetBootStatusData(BootStatusData,
                                     TRUE,
                                     RtlBsdItemBootGood,
                                     &flag,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx reading boot status data\n", status);
        return status;
    }

    printf("LastBootSucceeded = %#x\n", flag);

    status = RtlGetSetBootStatusData(BootStatusData,
                                     TRUE,
                                     RtlBsdItemBootShutdown,
                                     &flag,
                                     sizeof(BOOLEAN),
                                     NULL);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx reading boot status data\n", status);
        return status;
    }

    printf("LastBootShutdown = %#x\n", flag);

    return STATUS_SUCCESS;
}

DWORD SetFlags(HANDLE BootStatusData, int argc, char *argv[])
{
    int count;

    if(argc <= 1) {
        printf("usage: autosafeboot set <Boot|Shutdown>=<value> ...");
        return 0;
    }

    for(count = 1; count < argc; count++) {
        PUCHAR nameString;
        PUCHAR valueString;

        ULONG index;
        UCHAR value;

        index = -1;

        //
        // match the string.
        //

        nameString = argv[count];
        valueString = strrchr(nameString, '=');

        if(valueString == NULL) {
            printf("** element \"%s\" not understood\n", nameString);
            continue;
        }

        valueString[0] = '\0';
        valueString += 1;

        value = (UCHAR) atoi(valueString);

        if(_stricmp(nameString, "boot") == 0) {
            printf("Setting LastBootSucceeded to %#x\n", value);
            index = RtlBsdItemBootGood;

        } else if(_stricmp(nameString, "shutdown") == 0) {

            printf("Setting LastBootShutdown to %#x\n", value);
            index = RtlBsdItemBootShutdown;
        }

        if(index != -1) {

            NTSTATUS status;

            status = RtlGetSetBootStatusData(BootStatusData,
                                             FALSE,
                                             index,
                                             &value,
                                             sizeof(UCHAR),
                                             NULL);

            if(!NT_SUCCESS(status)) {
                printf("Error %#08lx reading boot status data\n", status);
                continue;
            }
        } else {
            printf("** element \"%s=%s\" not understood\n", nameString, valueString);
        }
    }

    return 0;
}

NTSTATUS
MyCreateBootStatusDataFile(
    PUCHAR DataFileName
    )
{
    OBJECT_ATTRIBUTES objectAttributes;

    WCHAR fileNameBuffer[MAXIMUM_FILENAME_LENGTH+1];
    ANSI_STRING ansiFileName;
    UNICODE_STRING fileName;

    HANDLE dataFileHandle;

    IO_STATUS_BLOCK ioStatusBlock;

    LARGE_INTEGER t;
    UCHAR zero = 0;

    BSD_BOOT_STATUS_DATA defaultValues;

    NTSTATUS status;

    RtlZeroMemory(fileNameBuffer, sizeof(fileNameBuffer));

    RtlInitAnsiString(&ansiFileName, DataFileName);

    fileName.Length = 0;
    fileName.MaximumLength = sizeof(fileNameBuffer);
    fileName.Buffer = fileNameBuffer;


    RtlAnsiStringToUnicodeString(&fileName, &ansiFileName, FALSE);

    InitializeObjectAttributes(&objectAttributes,
                               &fileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // The file must be large enough that it doesn't reside in the MFT entry 
    // or the loader won't be able to write to it.
    // 

    t.QuadPart = 2048;

    //
    // Create the file.
    //

    status = NtCreateFile(&dataFileHandle,
                          FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                          &objectAttributes,
                          &(ioStatusBlock),
                          &t,
                          FILE_ATTRIBUTE_SYSTEM,
                          0,
                          FILE_CREATE,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Write a single zero byte to the 0x7ffth byte in the file to make
    // sure that 2k are actually allocated.  This is to ensure that the 
    // file will not become attribute resident even after a conversion
    // from FAT to NTFS.
    //

    t.QuadPart = t.QuadPart - 1;
    status = NtWriteFile(dataFileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         &zero,
                         1,
                         &t,
                         NULL);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {
        goto CreateDone;
    }

    //
    // Now write out the default values to the beginning of the file.
    //

    defaultValues.Version = sizeof(BSD_BOOT_STATUS_DATA);
    RtlGetNtProductType(&(defaultValues.ProductType));
    defaultValues.AutoAdvancedBoot = FALSE;
    defaultValues.AdvancedBootMenuTimeout = 30;
    defaultValues.LastBootSucceeded = TRUE;
    defaultValues.LastBootShutdown = FALSE;

    t.QuadPart = 0;

    status = NtWriteFile(dataFileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         &defaultValues,
                         sizeof(BSD_BOOT_STATUS_DATA),
                         &t,
                         NULL);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {

        //
        // The data file was created and we can assume the contents were zeroed
        // even if we couldn't write out the defaults.  Since this wouldn't 
        // enable auto-advanced boot we'll leave the data file in place with 
        // its zeroed contents.
        //

    }

CreateDone:

    NtClose(dataFileHandle);

    return status;
}

DWORD 
CreateEx(
    HANDLE Unused, 
    int argc, 
    char *argv[]
    )
{
    BOOLEAN enabled;

    NTSTATUS status;

    if(argc < 2) {

        printf("Error - must supply file name\n");
        return -1;
    }

    printf("Will Create file %s\n", argv[1]);

    status = MyCreateBootStatusDataFile(argv[1]);

    if(!NT_SUCCESS(status)) {
        printf("Error %#08lx creating boot status data\n", status);
    }

    return status;
}

