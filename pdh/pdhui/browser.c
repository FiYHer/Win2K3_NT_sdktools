/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    browser.c

Abstract:

    counter name browsing functions exposed by the PDH.DLL

--*/
#include <windows.h>
#include <math.h>
#include <mbctype.h>
#include "strsafe.h"
#include "pdh.h"
#include "pdhmsg.h"
#include "strings.h"
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "perfdata.h"
#include "browsdlg.h"

#pragma warning ( disable : 4213)

PDH_FUNCTION
PdhSetDefaultRealTimeDataSource(
    IN  DWORD   dwDataSourceId
)
{
    DWORD dwReturn = ERROR_SUCCESS;

    if (dwCurrentRealTimeDataSource <= 0) {
        switch (dwDataSourceId) {
        case DATA_SOURCE_WBEM:
        case DATA_SOURCE_REGISTRY:
            // this is OK so set local variable
            dwPdhiLocalDefaultDataSource = dwDataSourceId;
            break;

        case DATA_SOURCE_LOGFILE:
        default:
            // these are not OK so insert default
            dwReturn = PDH_INVALID_ARGUMENT;
            break;
        }
    }
    else {
        // a default realtime data source has already been defined
        dwReturn = PDH_CANNOT_SET_DEFAULT_REALTIME_DATASOURCE;
    }
    return dwReturn;
}

DWORD
DataSourceTypeH(
    HLOG hDataSource
)
{
    return (hDataSource == H_REALTIME_DATASOURCE) ? (dwPdhiLocalDefaultDataSource)
            : (hDataSource == H_WBEM_DATASOURCE ? DATA_SOURCE_WBEM : DATA_SOURCE_LOGFILE);
}

DWORD
DataSourceTypeA(
    LPCSTR   szDataSource
)
{
    DWORD dwDataSource = dwPdhiLocalDefaultDataSource;

    if (szDataSource != NULL) {
        // see if the prefix to the file name is "WBEM:"
        // indicating this is a WBEM name space instead of a
        // log file name
        // if the file name has a "WBEM:" in the front, then
        // set the flag appropriately

        dwDataSource = (_strnicmp(szDataSource, caszWBEM, lstrlenA(caszWBEM)) != 0
                               && _strnicmp(szDataSource, caszWMI, lstrlenA(caszWMI)) != 0)
                     ? DATA_SOURCE_LOGFILE : DATA_SOURCE_WBEM;
    }
    return dwDataSource;
}

DWORD
DataSourceTypeW(
    LPCWSTR  szDataSource
)
{
    DWORD dwDataSource = dwPdhiLocalDefaultDataSource;

    if (szDataSource != NULL) {
        // see if the prefix to the file name is "WBEM:"
        // indicating this is a WBEM name space instead of a
        // log file name
        // if the file name has a "WBEM:" in the front, then
        // set the flag appropriately
        // Else check if it is "SQL:" prefixed

        dwDataSource = (_wcsnicmp(szDataSource, cszWBEM, lstrlenW(cszWBEM)) != 0
                               && _wcsnicmp(szDataSource, cszWMI, lstrlenW(cszWMI)) != 0)
                     ? DATA_SOURCE_LOGFILE : DATA_SOURCE_WBEM;
    }
    return dwDataSource;
}

PDH_FUNCTION
PdhConnectMachineW(
    IN  LPCWSTR  szMachineName
)
/*++
Routine Description:
  Establishes a connection to the specified machine for reading perforamance
  data from the machine.

Arguments:
    LPCWSTR szMachineName
        The name of the machine to connect to. If this argument is NULL,
        then the local machine is opened.

Return Value:
  PDH Error status value
    ERROR_SUCCESS   indicates the machine was successfully connected and the
        performance data from that machine was loaded.
    PDH_ error code indicates that the machine could not be located or opened.
        The status code indicates the problem.
--*/
{
    PPERF_MACHINE pMachine  = NULL;
    PDH_STATUS    pdhStatus = ERROR_SUCCESS;

    if (szMachineName != NULL) {
        __try {
            // test buffer access
            if (* szMachineName == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szMachineName) > PDH_MAX_COUNTER_NAME) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pMachine = GetMachine((LPWSTR) szMachineName, 0, PDH_GM_UPDATE_NAME | PDH_GM_UPDATE_PERFNAME_ONLY);
        if (pMachine != NULL) {
            // then return the machine status
            pdhStatus = pMachine->dwStatus;
            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
        }
        else {
            // return the status from the GetMachine call
            pdhStatus = GetLastError();
        }
    } // else pass the status to the caller
    return pdhStatus;
}

PDH_FUNCTION
PdhConnectMachineA(
    IN  LPCSTR  szMachineName
)
/*++
Routine Description:
  Establishes a connection to the specified machine for reading perforamance
  data from the machine.

Arguments:
    LPCSTR  szMachineName
        The name of the machine to connect to. If this argument is NULL,
        then the local machine is opened.

Return Value:
  PDH Error status value
    ERROR_SUCCESS   indicates the machine was successfully connected and the
        performance data from that machine was loaded.
    PDH_ error code indicates that the machine could not be located or opened.
        The status code indicates the problem.
--*/
{
    LPWSTR        szWideName   = NULL;
    DWORD         dwNameLength = 0;
    PDH_STATUS    pdhStatus    = ERROR_SUCCESS;
    PPERF_MACHINE pMachine     = NULL;

    if (szMachineName != NULL) {
        __try {
            // test buffer access
            if (* szMachineName == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szMachineName) > PDH_MAX_COUNTER_NAME) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }

        if (pdhStatus == ERROR_SUCCESS) {
            szWideName = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szMachineName);
            if (szWideName == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pMachine = GetMachine(szWideName, 0, PDH_GM_UPDATE_NAME | PDH_GM_UPDATE_PERFNAME_ONLY);
        if (pMachine != NULL) {
            // then return the machine status
            pdhStatus = pMachine->dwStatus;
            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
        }
        else {
            // return the status from the GetMachine call
            pdhStatus = GetLastError();
        }
    }

    G_FREE(szWideName);
    return pdhStatus;
}

STATIC_PDH_FUNCTION
PdhiEnumConnectedMachines(
    LPVOID  pMachineList,
    LPDWORD pcchBufferSize,
    BOOL    bUnicode
)
/*++
Routine Description:
    Builds a MSZ list of the machines currently known by the PDH. This
        list includes machines with open sessions as well as those that
        are off-line.

Arguments:
    IN      LPVOID  pMachineList
            A pointer to the buffer to receive the enumerated machine list.
            The strings written to this buffer will contain the characters
            specified by the bUnicode argument
    IN      LPVOID  pMachineList
            A pointer to the buffer to receive the enumerated machine list.
            The strings written to this buffer will contain the characters
            specified by the bUnicode argument
    IN      LPDWORD pcchBufferSize
            The size of the buffer referenced by pMachineList in characters
    IN      BOOL    bUnicode
            TRUE = UNICODE characters will be written to the pMachineList
                    buffer
            FALSE = ANSI characters will be writtn to the pMachinList buffer

Return Value:
    ERROR_SUCCESS if this the function completes successfully. a PDH error
        value if not.
    PDH_MORE_DATA some entries were returned, but there was not enough
        room in the buffer to store all entries.
    PDH_INVALID_ARGUMENT unable to write to the size buffers or the
        data buffer
--*/
{
    PPERF_MACHINE   pThisMachine;
    DWORD           dwRequiredLength = 0;
    DWORD           dwMaximumLength;
    DWORD           dwNameLength;
    PDH_STATUS      pdhStatus        = ERROR_SUCCESS;
    LPVOID          szNextName;

    // reset the last error value
    SetLastError(ERROR_SUCCESS);

    // first walk down list to compute required length

    pThisMachine = pFirstMachine;

    __try {
        // get a local copy of the size and try writing to the variable
        // to test read & write access of args before continuing

        dwMaximumLength  = * pcchBufferSize;
        * pcchBufferSize = 0;
        * pcchBufferSize = dwMaximumLength;

        // point to first machine entry in list
        szNextName = pMachineList;

        // walk around entire list
        if (pThisMachine != NULL) {
            do {
                if (bUnicode) {
                    dwNameLength = lstrlenW(pThisMachine->szName) + 1;
                    if (szNextName != NULL && dwRequiredLength + dwNameLength <= dwMaximumLength) {
                        StringCchCopyW((LPWSTR) szNextName, dwMaximumLength - dwRequiredLength, pThisMachine->szName);
                        (LPBYTE) szNextName += sizeof(WCHAR) * (dwNameLength - 1);
                        * ((LPWSTR) szNextName) ++ = L'\0';
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }
                }
                else {
                    dwNameLength = (dwRequiredLength <= dwMaximumLength)
                                 ? (dwMaximumLength - dwRequiredLength) : (0);
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                         pThisMachine->szName,
                                                         (LPSTR) szNextName,
                                                         & dwNameLength);
                    if (pdhStatus == ERROR_SUCCESS) {
                        (LPBYTE) szNextName += sizeof(CHAR) * dwNameLength;
                    }
                }
                dwRequiredLength += dwNameLength;
                // go to next machine in list
                pThisMachine      = pThisMachine->pNext;
            }
            while (pThisMachine != pFirstMachine);
        }
        else {
            // no machines in list, so insert an empty string
            if (++ dwRequiredLength <= dwMaximumLength) {
                if (bUnicode) {
                    * ((LPWSTR) szNextName) ++ = L'\0';
                }
                else {
                    * ((LPSTR) szNextName) ++ = '\0';
                }
                pdhStatus = ERROR_SUCCESS;
            }
            else if (dwMaximumLength != 0) {
                // then the buffer is too small
                pdhStatus = PDH_MORE_DATA;
            }
        }
        // all entries have been checked and /or copied
        //  so terminate the MSZ or at least account for the required size
        dwRequiredLength ++;
        if (szNextName != NULL && dwRequiredLength <= dwMaximumLength) {
            if (bUnicode) {
                * ((LPWSTR) szNextName) ++ = L'\0';
            }
            else {
                * ((LPSTR) szNextName) ++ = '\0';
            }
            pdhStatus = ERROR_SUCCESS;
        }
        else if (pdhStatus == ERROR_SUCCESS) {
            // then the buffer is too small
            pdhStatus = PDH_MORE_DATA;
        }
        //return the required size or size used
        * pcchBufferSize = dwRequiredLength;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhEnumMachinesHW(
    IN  HLOG    hDataSource,
    IN  LPWSTR  mszMachineList,
    IN  LPDWORD pcchBufferSize
)
{
    PDH_STATUS pdhStatus    = ERROR_SUCCESS;
    DWORD      dwBufferSize = 0;
    DWORD      dwDataSource = 0;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwDataSource = DataSourceTypeH(hDataSource);
            dwBufferSize = * pcchBufferSize;
            if (dwBufferSize > 0) {
                if (mszMachineList != NULL) {
                    mszMachineList[0]                = L'\0';
                    mszMachineList[dwBufferSize - 1] = L'\0';
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
         __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pdhStatus = PdhiEnumConnectedMachines((LPVOID) mszMachineList, & dwBufferSize, TRUE);
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiEnumWbemMachines((LPVOID) mszMachineList, & dwBufferSize, TRUE);
                break;

            case DATA_SOURCE_LOGFILE:
                pdhStatus = PdhiEnumLoggedMachines(hDataSource, (LPVOID) mszMachineList, & dwBufferSize, TRUE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }
        }
         __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }

        RELEASE_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumMachinesW(
    IN  LPCWSTR  szDataSource,
    IN  LPWSTR   mszMachineList,
    IN  LPDWORD  pcchBufferSize
)
/*++
Routine Description:
    Builds a MSZ list of the machines currently known by the PDH. This
        list includes machines with open sessions as well as those that
        are off-line.

Arguments:
    IN      LPCWSTR  szDataSource
            NULL for current real-time data or the name of a log file
    IN      LPWSTR  szMachineList
            A pointer to the buffer to receive the enumerated machine list.
            The strings written to this buffer will contain UNICODE chars
    IN      LPDWORD pcchBufferSize
            The size of the buffer referenced by pMachineList in characters
            The value of the buffer referenced by this pointer may be 0
            if the required size is requested.

Return Value:
    ERROR_SUCCESS if this the function completes successfully. a PDH error
        value if not.
    PDH_MORE_DATA some entries were returned, but there was not enough
        room in the buffer to store all entries.
    PDH_INVALID_ARGUMENT unable to write to the size buffers or the
        data buffer
--*/
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    DWORD       dwDataSource = 0;
    HLOG        hDataSource  = H_REALTIME_DATASOURCE;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szDataSource) >= PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource = DataSourceTypeW(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;
            pdhStatus = PdhOpenLogW(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhEnumMachinesHW(hDataSource, mszMachineList, pcchBufferSize);
        if (dwDataSource == DATA_SOURCE_LOGFILE) {
            PdhCloseLog(hDataSource, 0);
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumMachinesHA(
    IN  HLOG    hDataSource,
    IN  LPSTR   mszMachineList,
    IN  LPDWORD pcchBufferSize
)
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    DWORD       dwBufferSize = 0;
    DWORD       dwDataSource = 0;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {        
        __try {
            dwDataSource = DataSourceTypeH(hDataSource);
            dwBufferSize = * pcchBufferSize;
            if (dwBufferSize > 0) {
                if (mszMachineList != NULL) {
                    mszMachineList[0]                = '\0';
                    mszMachineList[dwBufferSize - 1] = '\0';
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
         __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pdhStatus = PdhiEnumConnectedMachines((LPVOID) mszMachineList, & dwBufferSize, FALSE);
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiEnumWbemMachines((LPVOID) mszMachineList, & dwBufferSize, FALSE);
                break;

            case DATA_SOURCE_LOGFILE:
                pdhStatus = PdhiEnumLoggedMachines(hDataSource, (LPVOID) mszMachineList, & dwBufferSize, FALSE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }
        }
         __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX (hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwBufferSize;
        }
         __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumMachinesA(
    IN  LPCSTR   szDataSource,
    IN  LPSTR    mszMachineList,
    IN  LPDWORD  pcchBufferSize
)
/*++
Routine Description:
    Builds a MSZ list of the machines currently known by the PDH. This
        list includes machines with open sessions as well as those that
        are off-line.

Arguments:
    IN      LPCSTR  szDataSource
            NULL for current real-time data or the name of a log file
    IN      LPWSTR  szMachineList
            A pointer to the buffer to receive the enumerated machine list.
            The strings written to this buffer will contain UNICODE chars
    IN      LPDWORD pcchBufferSize
            The size of the buffer referenced by pMachineList in characters
            The value of the buffer referenced by this pointer may be 0
            if the required size is requested.

Return Value:
    ERROR_SUCCESS if this the function completes successfully. a PDH error
        value if not.
    PDH_MORE_DATA some entries were returned, but there was not enough
        room in the buffer to store all entries.
    PDH_INVALID_ARGUMENT unable to write to the size buffers or the
        data buffer
--*/
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    DWORD       dwDataSource = 0;
    HLOG        hDataSource  = H_REALTIME_DATASOURCE;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource = DataSourceTypeA(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogA(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhEnumMachinesHA(hDataSource, mszMachineList, pcchBufferSize);
        if (dwDataSource == DATA_SOURCE_LOGFILE) {
            PdhCloseLog(hDataSource, 0);
        }
    }
    return pdhStatus;
}

#pragma warning ( disable : 4127 )
PDH_FUNCTION
PdhiEnumObjects(
    LPWSTR  szMachineName,
    LPVOID  mszObjectList,
    LPDWORD pcchBufferSize,
    DWORD   dwDetailLevel,
    BOOL    bRefresh,
    BOOL    bUnicode
)
/*++
Routine Description:
    Lists the performance objects found on the specified machine as
        a MSZ list.

Arguments:
    IN      LPWSTR  szMachineName
            The machine to list objects from
    IN      LPVOID  mszObjectList
            a pointer to the  buffer to receive the list of performance
            objects
    IN      LPDWORD pcchBufferSize
            a pointer to the DWORD containing the size of the mszObjectList
            buffer in characters. The characters assumed are determined by
            the bUnicode argument.
    IN      DWORD   dwDetailLevel
            The detail level to use as a filter of objects. All objects
            with a detail level less than or equal to that specified
            by this argument will be returned.
    IN      BOOL    bRefresh
            TRUE = retrive a new perf. data buffer for this machine before
                listing the objects
            FALSE = use the currently cached perf data buffer for this
                machine to enumerate objects
    IN      BOOL    bUnicode
            TRUE = return the listed objects as UNICODE strings
            FALSE = return the listed objects as ANSI strings

Return Value:
    ERROR_SUCCESS if the function completes successfully. Otherwise a
        PDH_ error status if not.
    PDH_MORE_DATA is returned when there are more entries available to
        return than there is room in the buffer. Some entries may be
        returned in the buffer though.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
--*/
{
    PPERF_MACHINE       pMachine;
    PDH_STATUS          pdhStatus        = ERROR_SUCCESS;
    DWORD               NumTypeDef;
    PPERF_OBJECT_TYPE   pObjectDef;
    DWORD               dwRequiredLength = 0;
    LPVOID              szNextName;
    DWORD               dwNameLength;
    DWORD               dwMaximumLength;
    LPWSTR              szObjNameString;
    WCHAR               wszNumberString[32];
    DWORD               dwGmFlags;
    DWORD               dwLocalDetailLevel;

    LPDWORD             pdwIndex = NULL;
    DWORD               dwAlloc  = MAX_PATH;
    DWORD               dwUsed   = 0;
    DWORD               i;

    // connect to machine and update data if desired
    if (bRefresh) {
        dwGmFlags  = PDH_GM_UPDATE_PERFDATA;
        dwGmFlags |= ((dwDetailLevel & PERF_DETAIL_COSTLY) == PERF_DETAIL_COSTLY) ? PDH_GM_READ_COSTLY_DATA : 0;
    }
    else {
        dwGmFlags = 0;
    }

    // connect to machine and update data if desired
    pMachine        = GetMachine(szMachineName, 0, (bRefresh ? PDH_GM_UPDATE_PERFDATA : 0));
    dwMaximumLength = * pcchBufferSize;

    if (pMachine != NULL) {
        if (pMachine->dwStatus == ERROR_SUCCESS) {
            if ((dwDetailLevel & PERF_DETAIL_COSTLY) && ! (pMachine->dwMachineFlags & PDHIPM_FLAGS_HAVE_COSTLY)) {
                // then get them
                pMachine->dwRefCount --;
                RELEASE_MUTEX(pMachine->hMutex);
                dwGmFlags = PDH_GM_UPDATE_PERFDATA | PDH_GM_READ_COSTLY_DATA;
                pMachine  = GetMachine(szMachineName, 0, dwGmFlags);
            }
        }
    }
    if (pMachine != NULL) {
        // make sure the machine connection is valid
        if (pMachine->dwStatus == ERROR_SUCCESS) {
            dwRequiredLength = 0;
            szNextName       = mszObjectList;

            // start walking object list
            pObjectDef = FirstObject(pMachine->pSystemPerfData);
            if ((pMachine->pSystemPerfData->NumObjectTypes > 0) && (pObjectDef != NULL)) {
                // convert detail level to the PerfLib detail level
                dwLocalDetailLevel = dwDetailLevel & PERF_DETAIL_STANDARD;
                // build list
                NumTypeDef = 0;
                pdwIndex = (LPDWORD) G_ALLOC(dwAlloc * sizeof(DWORD));
                if (pdwIndex == NULL) {
                    pMachine->dwRefCount --;
                    RELEASE_MUTEX(pMachine->hMutex);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
                while (1) {
                    // only look at entries matching the desired Detail Level
                    if (pObjectDef->DetailLevel <= dwLocalDetailLevel) {
                        for (i = 0; i < dwUsed; i ++) {
                            if (pdwIndex[i] == pObjectDef->ObjectNameTitleIndex) break;
                        }
                        if (i == dwUsed) {
                            if (dwUsed >= dwAlloc) {
                                LPDWORD pdwTmp = pdwIndex;
                                dwAlloc += dwAlloc;
                                pdwIndex = (LPDWORD) G_REALLOC(pdwTmp, dwAlloc * sizeof(DWORD));
                                if (pdwIndex == NULL) {
                                    G_FREE(pdwTmp);
                                    pMachine->dwRefCount --;
                                    RELEASE_MUTEX(pMachine->hMutex);
                                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                    goto Cleanup;
                                }
                            }
                            pdwIndex[dwUsed] = pObjectDef->ObjectNameTitleIndex;
                            dwUsed ++;

                            if (pObjectDef->ObjectNameTitleIndex < pMachine->dwLastPerfString) {
                                szObjNameString = (LPWSTR) PdhiLookupPerfNameByIndex(pMachine,
                                                                                     pObjectDef->ObjectNameTitleIndex);
                            }
                            else {
                                // no match since the index is larger that that found
                                // in the data buffer
                                szObjNameString = NULL;
                            }

                            if (szObjNameString == NULL) {
                                // then this object has no string name so use
                                // the object number
                                _ltow(pObjectDef->ObjectNameTitleIndex, wszNumberString, 10);
                                szObjNameString = wszNumberString;
                            }

                            // compute length
                            if (bUnicode) {
                                dwNameLength = lstrlenW(szObjNameString) + 1;
                                if (szNextName != NULL && dwRequiredLength + dwNameLength <= dwMaximumLength) {
                                    StringCchCopyW((LPWSTR) szNextName,
                                                   dwMaximumLength - dwRequiredLength,
                                                   szObjNameString);
                                    (LPBYTE) szNextName += (dwNameLength - 1) * sizeof(WCHAR);
                                    * ((LPWSTR) szNextName) ++ = L'\0';
                                }
                                else {
                                    pdhStatus = PDH_MORE_DATA;
                                }
                            }
                            else {
                                dwNameLength = (dwRequiredLength <= dwMaximumLength)
                                             ? (dwMaximumLength - dwRequiredLength) : (0);
                                pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                     szObjNameString,
                                                                     (LPSTR) szNextName,
                                                                     & dwNameLength);
                                if (pdhStatus == ERROR_SUCCESS) {
                                    (LPBYTE) szNextName += sizeof(CHAR) * dwNameLength;
                                }
                            }
                            dwRequiredLength += dwNameLength;
                        }
                    }

                    // go to next object in the data block
                    NumTypeDef ++;
                    if (NumTypeDef >= pMachine->pSystemPerfData->NumObjectTypes) {
                        // that's enough so break out of the loop
                        break;
                    }
                    else {
                        // goto the next one and make sure it's valid
                        pObjectDef = NextObject(pMachine->pSystemPerfData, pObjectDef); // get next
                        //make sure next object is legit
                        if (pObjectDef != NULL) {
                            if (pObjectDef->TotalByteLength == 0) {
                                // 0-length object buffer returned
                                break;
                            }
                        }
                        else {
                            // and continue
                            break;
                        }
                    }
                }
                // add MSZ terminator to string
                ++ dwRequiredLength; // add the size of the MSZ char
                if (pdhStatus == ERROR_SUCCESS) {
                    if (szNextName != NULL && dwRequiredLength <= dwMaximumLength) {
                        if (bUnicode) {
                            * ((LPWSTR) szNextName)++ = L'\0';
                        }
                        else {
                            * ((LPSTR)szNextName) ++ = '\0';
                        }
                        // make sure pointers and lengths stay in sync
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }
                }
            }
            else {
                // no objects found for this machine
                dwRequiredLength = 2;
                if (szNextName != NULL && dwRequiredLength <= dwMaximumLength) {
                    if (bUnicode) {
                        * ((LPWSTR) szNextName) ++ = L'\0';
                        * ((LPWSTR) szNextName) ++ = L'\0';
                    } else {
                        * ((LPSTR) szNextName) ++ = '\0';
                        * ((LPSTR) szNextName) ++ = '\0';
                    }
                }
                else {
                    pdhStatus = ERROR_MORE_DATA;
                } // else this is just a size request
            }
            // return length info
        }
        else {
            pdhStatus = pMachine->dwStatus;  // computer off line
        }
        pMachine->dwRefCount --;
        RELEASE_MUTEX(pMachine->hMutex);
    }
    else {
        pdhStatus = GetLastError(); // computer not found
    }

Cleanup:
    G_FREE(pdwIndex);
    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwRequiredLength;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = GetExceptionCode();
        }
    }
    return pdhStatus;
}
#pragma warning ( default : 4127 )

PDH_FUNCTION
PdhEnumObjectsHW(
    IN  HLOG    hDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPWSTR  mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh
)
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    DWORD       dwBufferSize = 0;
    DWORD       dwDataSource = 0;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwDataSource = DataSourceTypeH(hDataSource);
            dwBufferSize = * pcchBufferSize;
            if (szMachineName != NULL) {
                if (* szMachineName == L'\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenW(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwBufferSize > 0) {
                    if (mszObjectList != NULL) {
                        mszObjectList[0]                = L'\0';
                        mszObjectList[dwBufferSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }

        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pdhStatus = PdhiEnumObjects((LPWSTR) szMachineName,
                                            (LPVOID) mszObjectList,
                                            & dwBufferSize,
                                            dwDetailLevel,
                                            bRefresh,
                                            TRUE);
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiEnumWbemObjects((LPWSTR) szMachineName,
                                                (LPVOID) mszObjectList,
                                                & dwBufferSize,
                                                dwDetailLevel,  // not used
                                                bRefresh,
                                                TRUE);
                break;

            case DATA_SOURCE_LOGFILE:
                pdhStatus = PdhiEnumLoggedObjects(hDataSource,
                                                  (LPWSTR) szMachineName,
                                                  (LPVOID) mszObjectList,
                                                  & dwBufferSize,
                                                  dwDetailLevel,
                                                  bRefresh,
                                                  TRUE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }

        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX (hPdhDataMutex);

        if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
            __try {
                * pcchBufferSize = dwBufferSize;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectsW(
    IN  LPCWSTR szDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPWSTR  mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh
)
/*++
Routine Description:
    Lists the performance objects found on the specified machine as
        a MSZ UNICODE string list.

Arguments:
    IN      LPCWSTR  szDataSource
            NULL for current real-time data or the name of a log file
    IN      LPCWSTR  szMachineName
            The machine to list objects from
    IN      LPWSTR mszObjectList
            a pointer to the  buffer to receive the list of performance
            objects
    IN      LPDWORD pcchBufferSize
            a pointer to the DWORD containing the size of the mszObjectList
            buffer in characters.
    IN      DWORD   dwDetailLevel
            The detail level to use as a filter of objects. All objects
            with a detail level less than or equal to that specified
            by this argument will be returned.
    IN      BOOL    bRefresh
            TRUE = retrive a new perf. data buffer for this machine before
                listing the objects
            FALSE = use the currently cached perf data buffer for this
                machine to enumerate objects

Return Value:
    ERROR_SUCCESS if the function completes successfully. Otherwise a
        PDH_ error status if not.
    PDH_MORE_DATA is returned when there are more entries available to
        return than there is room in the buffer. Some entries may be
        returned in the buffer though.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_INVALID_ARGUMENT is returned if a required argument is not provided
        or a reserved argument is not NULL
--*/
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    DWORD       dwDataSource = 0;
    HLOG        hDataSource  = H_REALTIME_DATASOURCE;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource = DataSourceTypeW(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogW(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhEnumObjectsHW(hDataSource,
                                     szMachineName,
                                     mszObjectList,
                                     pcchBufferSize,
                                     dwDetailLevel,
                                     bRefresh);
        if (dwDataSource == DATA_SOURCE_LOGFILE) {
            PdhCloseLog(hDataSource, 0);
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectsHA(
    IN  HLOG    hDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPSTR   mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh
)
{
    PDH_STATUS pdhStatus    = ERROR_SUCCESS;
    LPWSTR     szWideName;
    DWORD      dwDataSource = 0;
    DWORD      dwBufferSize = 0;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwDataSource = DataSourceTypeH(hDataSource);
            if (szMachineName != NULL) {
                // test buffer access
                if (* szMachineName == '\0') {
                    // null machine names are not permitted
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenA(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                dwBufferSize = * pcchBufferSize;
                if (dwBufferSize > 0) {
                    if (mszObjectList != NULL) {
                        mszObjectList[0]                = '\0';
                        mszObjectList[dwBufferSize - 1] = '\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (szMachineName != NULL) {
            szWideName = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szMachineName);
        }
        else {
            szWideName = NULL;
        }
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pdhStatus = PdhiEnumObjects(szWideName,
                                            (LPVOID) mszObjectList,
                                            & dwBufferSize,
                                            dwDetailLevel,
                                            bRefresh,
                                            FALSE);
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiEnumWbemObjects((LPWSTR) szWideName,
                                                (LPVOID) mszObjectList,
                                                & dwBufferSize,
                                                dwDetailLevel,  // not used
                                                bRefresh,
                                                FALSE);
                break;

            case DATA_SOURCE_LOGFILE:
                pdhStatus = PdhiEnumLoggedObjects(hDataSource,
                                                  (LPWSTR) szWideName,
                                                  (LPVOID) mszObjectList,
                                                  & dwBufferSize,
                                                  dwDetailLevel,
                                                  bRefresh,
                                                  FALSE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }

            if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
                * pcchBufferSize = dwBufferSize;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX (hPdhDataMutex);
        G_FREE(szWideName);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectsA(
    IN  LPCSTR  szDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPSTR   mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh
)
/*++
Routine Description:
    Lists the performance objects found on the specified machine as
        a MSZ ANSI string list.

Arguments:
    IN      LPCSTR  szDataSource
            NULL for current real-time data or the name of a log file
    IN      LPCSTR  szMachineName
            The machine to list objects from
    IN      LPSTR mszObjectList
            a pointer to the  buffer to receive the list of performance
            objects
    IN      LPDWORD pcchBufferSize
            a pointer to the DWORD containing the size of the mszObjectList
            buffer in characters.
    IN      DWORD   dwDetailLevel
            The detail level to use as a filter of objects. All objects
            with a detail level less than or equal to that specified
            by this argument will be returned.
    IN      BOOL    bRefresh
            TRUE = retrive a new perf. data buffer for this machine before
                listing the objects
            FALSE = use the currently cached perf data buffer for this
                machine to enumerate objects

Return Value:
    ERROR_SUCCESS if the function completes successfully. Otherwise a
        PDH_ error status if not.
    PDH_MORE_DATA is returned when there are more entries available to
        return than there is room in the buffer. Some entries may be
        returned in the buffer though.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_INVALID_ARGUMENT is returned if a required argument is not provided
        or a reserved argument is not NULL
--*/
{
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;
    HLOG        hDataSource = H_REALTIME_DATASOURCE;
    DWORD       dwDataSource = 0;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource = DataSourceTypeA(szDataSource);
        }
    }
     __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogA(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhEnumObjectsHA(hDataSource,
                                     szMachineName,
                                     mszObjectList,
                                     pcchBufferSize,
                                     dwDetailLevel,
                                     bRefresh);
        if (dwDataSource == DATA_SOURCE_LOGFILE) {
            PdhCloseLog(hDataSource, 0);
        }
    }
    return pdhStatus;
}

STATIC_PDH_FUNCTION
PdhiEnumObjectItems(
    LPCWSTR szMachineName,
    LPCWSTR szObjectName,
    LPVOID  mszCounterList,
    LPDWORD pcchCounterListLength,
    LPVOID  mszInstanceList,
    LPDWORD pcchInstanceListLength,
    DWORD   dwDetailLevel,
    DWORD   dwFlags,
    BOOL    bUnicode
)
/*++
Routine Description:
    Lists the items found in the specified performance object on the
        specified machine. Thie includes the performance counters and,
        if supported by the object, the object instances.

Arguments:
    IN      LPCWSTR szMachineName
            The name of the machine to list the objects
    IN      LPCWSTR szObjectName
            the name of the object to list items from
    IN      LPVOID  mszCounterList
            pointer to the buffer that will receive the list of counters
            provided by this object. This argument may be NULL if
            the value of pcchCounterLIstLength is 0.
    IN      LPDWORD pcchCounterListLength
            pointer to a DWORD that contains the size in characters
            of the buffer referenced by mszCounterList. The characters
            assumed are defined by bUnicode.
    IN      LPVOID  mszInstanceList
            pointer to the buffer that will receive the list of instances
            of the specified performance object. This argument may be
            NULL if the value of pcchInstanceListLength is 0.
    IN      LPDWORD pcchInstanceListLength
            pointer to the DWORD containing the size, in characters, of
            the buffer referenced by the mszInstanceList argument. If the
            value in this DWORD is 0, then no data will be written to the
            buffer, only the required size will be returned.

            If the value returned is 0, then this object does not
            return instances, if the value returned is 2, then the
            object supports instances, but does not currently have
            any instances to return  (2 = the size of an MSZ list in
            characters)
    IN      DWORD   dwDetailLevel
            The detail level of the performance items to return. All items
            that are of the specified detail level or less will be
            returned.
    IN      DWORD   dwFlags
            Not Used, must be 0.
    IN      BOOL    bUnicode
            TRUE = UNICODE characters will be written to the pMachineList
                    buffer
            FALSE = ANSI characters will be writtn to the pMachinList buffer

Return Value:
    ERROR_SUCCESS if the function completes successfully. Otherwise a
        PDH_ error status if not.
    PDH_MORE_DATA is returned when there are more entries available to
        return than there is room in the buffer. Some entries may be
        returned in the buffer though.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_CSTATUS_NO_OBJECT is returned if the specified object could
        not be found on the specified machine.
--*/
{
    PPERF_MACHINE              pMachine            = NULL;
    DWORD                      dwObjectId          = 0;
    PDH_STATUS                 pdhStatus           = ERROR_SUCCESS;
    PDH_STATUS                 pdhCtrStatus        = ERROR_SUCCESS;
    PDH_STATUS                 pdhInstStatus       = ERROR_SUCCESS;
    DWORD                      DefNdx;
    PPERF_OBJECT_TYPE          pObjectDef;
    PPERF_COUNTER_DEFINITION   pCounterDef;
    PPERF_INSTANCE_DEFINITION  pInstanceDef;
    DWORD                      dwReqCounterLength  = 0;
    DWORD                      dwReqInstanceLength = 0;
    LPVOID                     szNextName;
    DWORD                      dwNameLength;
    LPWSTR                     szInstanceName      = NULL;
    DWORD                      dwInstanceName;
    WCHAR                      szNumberString[32];
    DWORD                      dwMaxInstanceLength;
    DWORD                      dwMaxCounterLength;
    LPWSTR                     szCounterName;
    DWORD                      dwGmFlags;

    DBG_UNREFERENCED_PARAMETER(dwFlags);

    pMachine = GetMachine((LPWSTR) szMachineName, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
    if (pMachine != NULL) {
        if (pMachine->dwStatus == ERROR_SUCCESS) {
            dwObjectId = GetObjectId(pMachine, (LPWSTR) szObjectName, NULL);
        }
        if (dwObjectId == (DWORD) -1) {
            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
            pMachine  = NULL;
            pdhStatus = PDH_CSTATUS_NO_OBJECT;
        }
    }

    if (pMachine != NULL) {
        if ((dwDetailLevel & PERF_DETAIL_COSTLY) && ! (pMachine->dwMachineFlags & PDHIPM_FLAGS_HAVE_COSTLY)) {
            // then get them
            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
            dwGmFlags = PDH_GM_UPDATE_PERFDATA | PDH_GM_READ_COSTLY_DATA;
            pMachine  = GetMachine((LPWSTR) szMachineName, dwObjectId, dwGmFlags);
        }
    }
    if (pMachine != NULL) {
        dwMaxCounterLength  = * pcchCounterListLength;
        dwMaxInstanceLength = * pcchInstanceListLength;

        // make sure the machine connection is valid
        if (pMachine->dwStatus == ERROR_SUCCESS) {
            pObjectDef = GetObjectDefByName(pMachine->pSystemPerfData,
                                            pMachine->dwLastPerfString,
                                            pMachine->szPerfStrings,
                                            szObjectName);
            if (pObjectDef == NULL) {
                DWORD dwObjectTitle = wcstoul(szObjectName, NULL, 10);
                if (dwObjectTitle != 0) {
                    pObjectDef = GetObjectDefByTitleIndex(pMachine->pSystemPerfData, dwObjectTitle);
                }
            }
            if (pObjectDef != NULL) {
                // add up counter name sizes
                pCounterDef = FirstCounter(pObjectDef);
                szNextName  = mszCounterList;

                for (DefNdx = 0; pCounterDef != NULL && DefNdx < pObjectDef->NumCounters; DefNdx++) {
                    if (! ((pCounterDef->CounterType & PERF_DISPLAY_NOSHOW) &&
                                // this is a hack because this type is not defined correctly
                                    (pCounterDef->CounterType != PERF_AVERAGE_BULK)) &&
                                    (pCounterDef->DetailLevel <= dwDetailLevel)) {
                                // then this is a visible counter so get its name.
                        if ((pCounterDef->CounterNameTitleIndex > 0) &&
                                        (pCounterDef->CounterNameTitleIndex < pMachine->dwLastPerfString)) {
                            // look up name of each object & store size
                            szCounterName = (LPWSTR) PdhiLookupPerfNameByIndex(pMachine,
                                                                               pCounterDef->CounterNameTitleIndex);
                        }
                        else {
                            // no matching string found for this index
                            szCounterName = NULL;
                        }
                        if (szCounterName == NULL) {
                            // then use the index numbe for lack of a better
                            // string to use
                            _ltow(pCounterDef->CounterNameTitleIndex, szNumberString, 10);
                            szCounterName = szNumberString;
                        }

                        if (bUnicode) {
                            dwNameLength = lstrlenW(szCounterName) + 1;
                            if (szNextName != NULL
                                            && (dwReqCounterLength + dwNameLength) <= dwMaxCounterLength) {
                                StringCchCopyW((LPWSTR) szNextName, dwNameLength, szCounterName);
                                (LPBYTE) szNextName += sizeof(WCHAR) * (dwNameLength - 1);
                                * ((LPWSTR) szNextName) ++ = L'\0';
                            }
                            else {
                                pdhCtrStatus = PDH_MORE_DATA;
                            }
                        }
                        else {
                            dwNameLength = (dwReqCounterLength <= dwMaxCounterLength)
                                         ? (dwMaxCounterLength - dwReqCounterLength)
                                         : (0);
                            pdhCtrStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                    szCounterName,
                                                                    (LPSTR) szNextName,
                                                                    & dwNameLength);
                            if (pdhCtrStatus == ERROR_SUCCESS) {
                                (LPBYTE) szNextName += sizeof(CHAR) * dwNameLength;
                            }
                        }
                        dwReqCounterLength += dwNameLength;
                    }
                    else {
                        // this counter is not displayed either because
                        // it's hidden (e.g. the 2nd part of a 2 part counter
                        // or it's the wrong detail level
                    }
                    pCounterDef = NextCounter(pObjectDef, pCounterDef); // get next
                }

                if (DefNdx == 0) {
                    // no counters found so at least one NULL is required
                    dwReqCounterLength += 1;

                    // see if this string will fit
                    if (dwReqCounterLength <= dwMaxCounterLength) {
                        if (bUnicode) {
                            * ((LPWSTR) szNextName) ++ = L'\0';
                        }
                        else {
                            * ((LPSTR) szNextName) ++ = '\0';
                        }
                    }
                    else {
                        // more space needed than was reported
                        pdhCtrStatus = PDH_MORE_DATA;
                    }
                }
                // add terminating NULL
                dwReqCounterLength += 1;

                // see if this string will fit
                if (dwReqCounterLength <= dwMaxCounterLength) {
                    if (bUnicode) {
                        * ((LPWSTR) szNextName) ++ = L'\0';
                    }
                    else {
                        * ((LPSTR) szNextName) ++ = '\0';
                    }
                }
                else {
                    // more space needed than was reported
                    pdhCtrStatus = PDH_MORE_DATA;
                }

                // do instances now.
                szNextName = mszInstanceList;

                // add up instance name sizes
                if (pObjectDef->NumInstances != PERF_NO_INSTANCES) {
                    if (pObjectDef->DetailLevel <= dwDetailLevel && pObjectDef->NumInstances > 0) {
                        dwInstanceName = SMALL_BUFFER_SIZE;
                        szInstanceName = G_ALLOC(dwInstanceName * sizeof(WCHAR));
                        if (szInstanceName == NULL) {
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                        else {
                            // the object HAS instances and is of the
                            // approrpriate detail level, so list them
                            pInstanceDef = FirstInstance(pObjectDef);
                            for (DefNdx = 0; pInstanceDef != NULL && DefNdx < (DWORD)pObjectDef->NumInstances; DefNdx++) {
                                dwNameLength = GetFullInstanceNameStr(pMachine->pSystemPerfData,
                                                                      pObjectDef,
                                                                      pInstanceDef,
                                                                      szInstanceName,
                                                                      dwInstanceName);
                                while (pdhStatus == ERROR_SUCCESS && dwNameLength > dwInstanceName) {
                                    G_FREE(szInstanceName);
                                    dwInstanceName = QWORD_MULTIPLE(dwNameLength);
                                    szInstanceName = G_ALLOC(dwInstanceName * sizeof(WCHAR));
                                    if (szInstanceName != NULL) {
                                        dwNameLength = GetFullInstanceNameStr(pMachine->pSystemPerfData,
                                                                              pObjectDef,
                                                                              pInstanceDef,
                                                                              szInstanceName,
                                                                              dwInstanceName);
                                    }
                                    else {
                                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                    }
                                }
                                if (pdhStatus != ERROR_SUCCESS) break;
                                if (dwNameLength > 0) {
                                    if (bUnicode) {
                                        // add length of this string + it's null
                                        dwNameLength += 1;
                                        if (szNextName != NULL
                                                        && dwReqInstanceLength + dwNameLength <= dwMaxInstanceLength) {
                                            StringCchCopyW((LPWSTR) szNextName, dwNameLength, szInstanceName);
                                            (LPBYTE) szNextName += sizeof(WCHAR) * (dwNameLength - 1);
                                            * ((LPWSTR) szNextName) ++ = L'\0';
                                        }
                                        else {
                                            pdhInstStatus = PDH_MORE_DATA;
                                        }
                                    }
                                    else {
                                        dwNameLength = (dwReqInstanceLength <= dwMaxInstanceLength)
                                                     ? (dwMaxInstanceLength - dwReqInstanceLength) : (0);
                                        pdhInstStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                                 szInstanceName,
                                                                                 (LPSTR) szNextName,
                                                                                 & dwNameLength);
                                        if (pdhInstStatus == ERROR_SUCCESS) {
                                            (LPBYTE) szNextName += sizeof(CHAR) * dwNameLength;
                                        }
                                    }
                                    dwReqInstanceLength += dwNameLength;
                                }

                                // go to next instance of this object
                                pInstanceDef = NextInstance(pObjectDef, pInstanceDef); // get next
                            }
                            G_FREE(szInstanceName);
                        }
                        // add the terminating NULL char
                        dwReqInstanceLength += 1;
                        if (dwMaxInstanceLength > 0) {
                            // see if this string will fit
                            if (dwReqInstanceLength <= dwMaxInstanceLength) {
                                if (bUnicode) {
                                    * ((LPWSTR) szNextName) ++ = L'\0';
                                }
                                else {
                                    * ((LPSTR) szNextName) ++ = '\0';
                                }
                            }
                            else {
                                // more space needed than was reported
                                pdhInstStatus = PDH_MORE_DATA;
                            }
                        }
                    }
                    else {
                        // there are no instances present, but the object does
                        // support instances so return a zero length MSZ (which
                        // actually contains 2 NULL chars
                        dwReqInstanceLength = 2;

                        // see if this string will fit
                        if (dwReqInstanceLength <= dwMaxInstanceLength) {
                            if (bUnicode) {
                                * ((LPWSTR) szNextName) ++ = L'\0';
                                * ((LPWSTR) szNextName) ++ = L'\0';
                            }
                            else {
                                * ((LPSTR) szNextName) ++ = '\0';
                                * ((LPSTR) szNextName) ++ = '\0';
                            }
                        }
                        else {
                            // more space needed than was reported
                            pdhInstStatus = PDH_MORE_DATA;
                        }
                    }
                }
                else {
                    // the object has no instances and never will
                    // so return a 0 length and NO string
                    dwReqInstanceLength = 0;
                }

                if (pdhStatus == ERROR_SUCCESS) {
                    pdhStatus = (pdhCtrStatus == ERROR_SUCCESS) ? (pdhInstStatus) : (pdhCtrStatus);
                }
                if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
                    * pcchCounterListLength  = dwReqCounterLength;
                    * pcchInstanceListLength = dwReqInstanceLength;
                }
            }
            else {
                // object not found on this machine
                pdhStatus = PDH_CSTATUS_NO_OBJECT;
            }
        }
        else {
            // machine is off line
            pdhStatus = pMachine->dwStatus;
        }
        pMachine->dwRefCount --;
        RELEASE_MUTEX(pMachine->hMutex);
    }
    else if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = GetLastError();
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectItemsHW(
    IN  HLOG    hDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szObjectName,
    IN  LPWSTR  mszCounterList,
    IN  LPDWORD pcchCounterListLength,
    IN  LPWSTR  mszInstanceList,
    IN  LPDWORD pcchInstanceListLength,
    IN  DWORD   dwDetailLevel,
    IN  DWORD   dwFlags
)
{
    PDH_STATUS  pdhStatus     = ERROR_SUCCESS;
    DWORD       dwCBufferSize = 0;
    DWORD       dwIBufferSize = 0;
    DWORD       dwDataSource  = 0;

    if (pcchCounterListLength == NULL || pcchInstanceListLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwDataSource  = DataSourceTypeH(hDataSource);
            dwCBufferSize = * pcchCounterListLength;
            dwIBufferSize = * pcchInstanceListLength;

            if (szMachineName != NULL) {
                if (* szMachineName == L'\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenW(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (szObjectName != NULL) {
                    if (* szObjectName == L'\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                    else if (lstrlenW(szObjectName) > PDH_MAX_COUNTER_NAME) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwCBufferSize > 0) {
                    if (mszCounterList != NULL) {
                        mszCounterList[0]                 = L'\0';
                        mszCounterList[dwCBufferSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwIBufferSize > 0) {
                    if (mszInstanceList != NULL) {
                        mszInstanceList[0]                = L'\0';
                        mszInstanceList[dwIBufferSize -1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (dwFlags != 0L) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pdhStatus = PdhiEnumObjectItems((LPWSTR) szMachineName,
                                                szObjectName,
                                                (LPVOID) mszCounterList,
                                                & dwCBufferSize,
                                                (LPVOID) mszInstanceList,
                                                & dwIBufferSize,
                                                dwDetailLevel,
                                                dwFlags,
                                                TRUE);
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiEnumWbemObjectItems((LPWSTR) szMachineName,
                                                    szObjectName,
                                                    (LPVOID) mszCounterList,
                                                    & dwCBufferSize,
                                                    (LPVOID) mszInstanceList,
                                                    & dwIBufferSize,
                                                    dwDetailLevel,
                                                    dwFlags,
                                                    TRUE);
                break;

            case DATA_SOURCE_LOGFILE:
                pdhStatus = PdhiEnumLoggedObjectItems(hDataSource,
                                                      (LPWSTR) szMachineName,
                                                      szObjectName,
                                                      (LPVOID) mszCounterList,
                                                      & dwCBufferSize,
                                                      (LPVOID) mszInstanceList,
                                                      & dwIBufferSize,
                                                      dwDetailLevel,
                                                      dwFlags,
                                                      TRUE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX(hPdhDataMutex);

        if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
            __try {
                * pcchCounterListLength  = dwCBufferSize;
                * pcchInstanceListLength = dwIBufferSize;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectItemsW(
    IN  LPCWSTR szDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szObjectName,
    IN  LPWSTR  mszCounterList,
    IN  LPDWORD pcchCounterListLength,
    IN  LPWSTR  mszInstanceList,
    IN  LPDWORD pcchInstanceListLength,
    IN  DWORD   dwDetailLevel,
    IN  DWORD   dwFlags
)
/*++
Routine Description:
    Lists the items found in the specified performance object on the
        specified machine. Thie includes the performance counters and,
        if supported by the object, the object instances.

Arguments:
    IN      LPCWSTR  szDataSource
            NULL for current real-time data or the name of a log file
    IN      LPCWSTR szMachineName
            The name of the machine to list the objects
    IN      LPCWSTR szObjectName
            the name of the object to list items from
    IN      LPWSTR  mszCounterList
            pointer to the buffer that will receive the list of counters
            provided by this object. This argument may be NULL if
            the value of pcchCounterLIstLength is 0.
    IN      LPDWORD pcchCounterListLength
            pointer to a DWORD that contains the size in characters
            of the buffer referenced by mszCounterList. The characters
            assumed are defined by bUnicode.
    IN      LPWSTR  mszInstanceList
            pointer to the buffer that will receive the list of instances
            of the specified performance object. This argument may be
            NULL if the value of pcchInstanceListLength is 0.
    IN      LPDWORD pcchInstanceListLength
            pointer to the DWORD containing the size, in characters, of
            the buffer referenced by the mszInstanceList argument. If the
            value in this DWORD is 0, then no data will be written to the
            buffer, only the required size will be returned.

            If the value returned is 0, then this object does not
            return instances, if the value returned is 2, then the
            object supports instances, but does not currently have
            any instances to return  (2 = the size of an MSZ list in
            characters)
    IN      DWORD   dwDetailLevel
            The detail level of the performance items to return. All items
            that are of the specified detail level or less will be
            returned.
    IN      DWORD   dwFlags
            Not Used, must be 0.

Return Value:
    ERROR_SUCCESS if the function completes successfully. Otherwise a
        PDH_ error status if not.
    PDH_MORE_DATA is returned when there are more entries available to
        return than there is room in the buffer. Some entries may be
        returned in the buffer though.
    PDH_INVALID_ARGUMENT a required argument is not correct or reserved
        argument is not 0 or NULL.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_CSTATUS_NO_OBJECT is returned if the specified object could
        not be found on the specified machine.
--*/
{
    PDH_STATUS  pdhStatus    = ERROR_SUCCESS;
    HLOG        hDataSource  = H_REALTIME_DATASOURCE;
    DWORD       dwDataSource = 0;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource = DataSourceTypeW(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogW(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhEnumObjectItemsHW(hDataSource,
                                         szMachineName,
                                         szObjectName,
                                         mszCounterList,
                                         pcchCounterListLength,
                                         mszInstanceList,
                                         pcchInstanceListLength,
                                         dwDetailLevel,
                                         dwFlags);
        if (dwDataSource == DATA_SOURCE_LOGFILE) {
            PdhCloseLog(hDataSource, 0);
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectItemsHA(
    IN  HLOG    hDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPCSTR  szObjectName,
    IN  LPSTR   mszCounterList,
    IN  LPDWORD pcchCounterListLength,
    IN  LPSTR   mszInstanceList,
    IN  LPDWORD pcchInstanceListLength,
    IN  DWORD   dwDetailLevel,
    IN  DWORD   dwFlags
)
{
    PDH_STATUS  pdhStatus           = ERROR_SUCCESS;
    LPWSTR      szWideMachineName   = NULL;
    LPWSTR      szWideObjectName    = NULL;
    DWORD       dwDataSource        = 0;
    DWORD       dwCBufferSize       = 0;
    DWORD       dwIBufferSize       = 0;

    if ((pcchCounterListLength == NULL) || (pcchInstanceListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwDataSource  = DataSourceTypeH(hDataSource);
            dwCBufferSize = * pcchCounterListLength;
            dwIBufferSize = * pcchInstanceListLength;

            if (szMachineName != NULL) {
                if (* szMachineName == '\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenA(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (szObjectName != NULL) {
                    if (* szObjectName == '\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                    else if (lstrlenA(szObjectName) > PDH_MAX_COUNTER_NAME) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    // object cannot be NULL
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwCBufferSize > 0) {
                    if (mszCounterList != NULL) {
                        mszCounterList[0]                 = '\0';
                        mszCounterList[dwCBufferSize - 1] = '\0';
                    } 
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwIBufferSize > 0) {
                    if (mszInstanceList != NULL) {
                        mszInstanceList[0]                 = '\0';
                        mszInstanceList[dwIBufferSize - 1] = '\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (dwFlags != 0L) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            if (szMachineName != NULL) {
                szWideMachineName = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szMachineName);
            }
            else {
                szWideMachineName = NULL;
            }
            if (szObjectName != NULL) {
                szWideObjectName  = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szObjectName);
            }
            else {
                szWideObjectName  = NULL;
            }
            if (pdhStatus == ERROR_SUCCESS) {
                switch (dwDataSource) {
                case DATA_SOURCE_REGISTRY:
                    pdhStatus = PdhiEnumObjectItems(szWideMachineName,
                                                    szWideObjectName,
                                                    (LPVOID) mszCounterList,
                                                    & dwCBufferSize,
                                                    (LPVOID) mszInstanceList,
                                                    & dwIBufferSize,
                                                    dwDetailLevel,
                                                    dwFlags,
                                                    FALSE);
                    break;

                case DATA_SOURCE_WBEM:
                   pdhStatus = PdhiEnumWbemObjectItems((LPWSTR) szWideMachineName,
                                                       szWideObjectName,
                                                       (LPVOID) mszCounterList,
                                                       & dwCBufferSize,
                                                       (LPVOID) mszInstanceList,
                                                       & dwIBufferSize,
                                                       dwDetailLevel,
                                                       dwFlags,
                                                       FALSE);
                   break;

                case DATA_SOURCE_LOGFILE:
                    pdhStatus = PdhiEnumLoggedObjectItems(hDataSource,
                                                          szWideMachineName,
                                                          szWideObjectName,
                                                          (LPVOID) mszCounterList,
                                                          & dwCBufferSize,
                                                          (LPVOID) mszInstanceList,
                                                          & dwIBufferSize,
                                                          dwDetailLevel,
                                                          dwFlags,
                                                          FALSE);
                    break;

                default:
                    pdhStatus = PDH_INVALID_ARGUMENT;
                    break;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX(hPdhDataMutex);
        G_FREE(szWideMachineName);
        G_FREE(szWideObjectName);
        if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
            __try {
                * pcchCounterListLength  = dwCBufferSize;
                * pcchInstanceListLength = dwIBufferSize;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumObjectItemsA(
    IN  LPCSTR  szDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPCSTR  szObjectName,
    IN  LPSTR   mszCounterList,
    IN  LPDWORD pcchCounterListLength,
    IN  LPSTR   mszInstanceList,
    IN  LPDWORD pcchInstanceListLength,
    IN  DWORD   dwDetailLevel,
    IN  DWORD   dwFlags
)
/*++
Routine Description:
    Lists the items found in the specified performance object on the
        specified machine. Thie includes the performance counters and,
        if supported by the object, the object instances.

Arguments:
    IN      LPCSTR  szDataSource
            NULL for current real-time data or the name of a log file
    IN      LPCSTR szMachineName
            The name of the machine to list the objects
    IN      LPCSTR szObjectName
            the name of the object to list items from
    IN      LPSTR  mszCounterList
            pointer to the buffer that will receive the list of counters
            provided by this object. This argument may be NULL if
            the value of pcchCounterLIstLength is 0.
    IN      LPDWORD pcchCounterListLength
            pointer to a DWORD that contains the size in characters
            of the buffer referenced by mszCounterList. The characters
            assumed are defined by bUnicode.
    IN      LPSTR  mszInstanceList
            pointer to the buffer that will receive the list of instances
            of the specified performance object. This argument may be
            NULL if the value of pcchInstanceListLength is 0.
    IN      LPDWORD pcchInstanceListLength
            pointer to the DWORD containing the size, in characters, of
            the buffer referenced by the mszInstanceList argument. If the
            value in this DWORD is 0, then no data will be written to the
            buffer, only the required size will be returned.

            If the value returned is 0, then this object does not
            return instances, if the value returned is 2, then the
            object supports instances, but does not currently have
            any instances to return  (2 = the size of an MSZ list in
            characters)
    IN      DWORD   dwDetailLevel
            The detail level of the performance items to return. All items
            that are of the specified detail level or less will be
            returned.
    IN      DWORD   dwFlags
            Not Used, must be 0.

Return Value:
    ERROR_SUCCESS if the function completes successfully. Otherwise a
        PDH_ error status if not.
    PDH_MORE_DATA is returned when there are more entries available to
        return than there is room in the buffer. Some entries may be
        returned in the buffer though.
    PDH_INVALID_ARGUMENT a required argument is not correct or reserved
        argument is not 0 or NULL.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a required temporary
        buffer could not be allocated.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_CSTATUS_NO_OBJECT is returned if the specified object could
        not be found on the specified machine.
--*/
{
    PDH_STATUS pdhStatus    = ERROR_SUCCESS;
    HLOG       hDataSource  = H_REALTIME_DATASOURCE;
    DWORD      dwDataSource = 0;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource = DataSourceTypeA(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogA(szDataSource,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hDataSource);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhEnumObjectItemsHA(hDataSource,
                                         szMachineName,
                                         szObjectName,
                                         mszCounterList,
                                         pcchCounterListLength,
                                         mszInstanceList,
                                         pcchInstanceListLength,
                                         dwDetailLevel,
                                         dwFlags);
        if (dwDataSource == DATA_SOURCE_LOGFILE) {
            PdhCloseLog(hDataSource, 0);
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhMakeCounterPathW(
    IN  PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElements,
    IN  LPWSTR                       szFullPathBuffer,
    IN  LPDWORD                      pcchBufferSize,
    IN  DWORD                        dwFlags
)
/*++
Routine Description:
    Constructs a counter path using the elemeents defined in the
        pCounterPathElements structure and returns the path string
        in the buffer provided by the caller. The resulting path
        is not validated.

Arguments:
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements
                The pointer to the structure containing the
                individual counter path fields that are to be
                assembled in to a path string
    IN      LPWSTR                      szFullPathBuffer
                The buffer to receive the path string. This value
                may be NULL if the value of the DWORD pointed to
                by pcchBufferSize is 0 indicating this is just a
                request for the required buffer size.
    IN      LPDWORD                     pcchBufferSize
                The pointer to the DWORD containing the size
                of the string buffer in characters. On return
                it contains the size of the buffer used in
                characters (including the terminating NULL char).
                If the value is 0 on entry then no data will be
                written to the buffer, but the required size will
                still be returned.
    IN      DWORD                       dwFlags
            if 0, then return the path as a REGISTRY path items
            if PDH_PATH_WBEM_RESULT then return the items in WBEM format
            if PDH_PATH_WBEM_INPUT then assume the input is in WBEM format

Return Value:
    ERROR_SUCCESS if the function completes successfully, otherwise a
        PDH error is returned.
    PDH_INVALID_ARGUMENT is returned when one of the arguments passed
        by the caller is incorrect or not accesible.
--*/
{
    PDH_STATUS  pdhStatus         = ERROR_SUCCESS;
    double      dIndex;
    double      dLen;
    DWORD       dwSizeRequired    = 0;
    LPWSTR      szNextChar;
    DWORD       dwMaxSize;
    DWORD       dwLocalBufferSize = 0;

    if (pCounterPathElements == NULL || pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
        // test access to the input structure
            if (pCounterPathElements->szMachineName != NULL) {
                // then see if it's accessible
                if (* pCounterPathElements->szMachineName == L'\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szObjectName != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szObjectName == L'\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    //NULL is NOT ok for this field
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szInstanceName != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szInstanceName == L'\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szParentInstance != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szParentInstance == L'\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szCounterName != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szCounterName == L'\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    //NULL is NOT ok for this field
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            // test the output buffers
            if (pdhStatus == ERROR_SUCCESS) {
                if (pcchBufferSize != NULL) {
                    dwLocalBufferSize = * pcchBufferSize;
                }
                else {
                    // NULL is NOT OK
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if ((pdhStatus == ERROR_SUCCESS) && (szFullPathBuffer == NULL && dwLocalBufferSize > 0)) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }

            if (pdhStatus == ERROR_SUCCESS) {
                if (dwLocalBufferSize > 0) {
                    if (szFullPathBuffer != NULL) {
                        szFullPathBuffer[0]                     = L'\0';
                        szFullPathBuffer[dwLocalBufferSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            if (dwFlags == 0) {
                // then process as:
                //      registry path elements in
                //      registry path out

                dwMaxSize = dwLocalBufferSize;

                if (pCounterPathElements->szMachineName != NULL) {
                    dwSizeRequired = lstrlenW(pCounterPathElements->szMachineName);
                    // compare the first two words of the machine name
                    // to see if the double backslash is already present in the string
                    if (* ((LPDWORD) (pCounterPathElements->szMachineName)) != * ((LPDWORD) (cszDoubleBackSlash))) {
                            // double backslash not found
                        dwSizeRequired += 2; // to include the backslashes
                    }
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        if (* ((LPDWORD) (pCounterPathElements->szMachineName)) != * ((LPDWORD) (cszDoubleBackSlash))) {
                            // double backslash not found
                            StringCchCopyW(szFullPathBuffer, dwMaxSize, cszDoubleBackSlash);
                            StringCchCatW(szFullPathBuffer, dwMaxSize, pCounterPathElements->szMachineName);
                        }
                        else {
                            StringCchCopyW(szFullPathBuffer, dwMaxSize, pCounterPathElements->szMachineName);
                        }
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }
                }

                dwSizeRequired += 1; // for delimiting slash
                dwSizeRequired += lstrlenW(pCounterPathElements->szObjectName);
                if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                    StringCchCatW(szFullPathBuffer, dwMaxSize, cszBackSlash);
                    StringCchCatW(szFullPathBuffer, dwMaxSize, pCounterPathElements->szObjectName);
                }
                else {
                    pdhStatus = PDH_MORE_DATA;
                }

                if (pCounterPathElements->szInstanceName != NULL) {
                    dwSizeRequired += 1; // for delimiting left paren
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        StringCchCatW(szFullPathBuffer, dwMaxSize, cszLeftParen);
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }

                    if (lstrcmpiW(pCounterPathElements->szInstanceName, cszSplat) != 0
                                    && pCounterPathElements->szParentInstance != NULL) {
                        dwSizeRequired += (lstrlenW(pCounterPathElements->szParentInstance) + 1);
                                                // extra 1 for delimiting slash
                        if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                            StringCchCatW(szFullPathBuffer, dwMaxSize, pCounterPathElements->szParentInstance);
                            StringCchCatW(szFullPathBuffer, dwMaxSize, cszSlash);
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                    }

                    dwSizeRequired += lstrlenW(pCounterPathElements->szInstanceName);
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        StringCchCatW(szFullPathBuffer, dwMaxSize, pCounterPathElements->szInstanceName);
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }

                    if (lstrcmpiW(pCounterPathElements->szInstanceName, cszSplat) != 0
                                    && pCounterPathElements->dwInstanceIndex != PERF_NO_UNIQUE_ID
                                    && pCounterPathElements->dwInstanceIndex != 0) {
                        // the length of the index is computed by getting the log of the number
                        // yielding the largest power of 10 less than or equal to the index.
                        // e.g. the power of 10 of an index value of 356 would 2.0 (which is the
                        // result of (floor(log10(index))). The actual number of characters in
                        // the string would always be 1 greate than that value so 1 is added.
                        // 1 more is added to include the delimiting character

                        dIndex = (double) pCounterPathElements->dwInstanceIndex; // cast to float
                        dLen   = floor(log10(dIndex));                           // get integer log
                        dwSizeRequired += (DWORD) dLen;                          // cast to integer
                        dwSizeRequired += 2;                                     // increment
                        if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                            szNextChar      = & szFullPathBuffer[lstrlenW(szFullPathBuffer)];
                            * szNextChar ++ = POUNDSIGN_L;
                            _ltow((long) pCounterPathElements->dwInstanceIndex, szNextChar, 10);
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                    }
                    dwSizeRequired += 1; // for delimiting parenthesis
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        StringCchCatW(szFullPathBuffer, dwMaxSize, cszRightParen);
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }
                }

                dwSizeRequired ++;   // include delimiting Backslash
                dwSizeRequired += lstrlenW(pCounterPathElements->szCounterName);
                if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                    StringCchCatW(szFullPathBuffer, dwMaxSize, cszBackSlash);
                    StringCchCatW(szFullPathBuffer, dwMaxSize, pCounterPathElements->szCounterName);
                }
                else {
                    pdhStatus = PDH_MORE_DATA;
                }
                dwSizeRequired ++;   // include trailing Null char
                if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                    szFullPathBuffer[dwSizeRequired - 1] = L'\0';
                }
                else {
                    pdhStatus = PDH_MORE_DATA;
                }
            }
            else {
                // there is some WBEM component involved so send to WBEM function
                // to figure it out
                pdhStatus = PdhiEncodeWbemPathW(pCounterPathElements,
                                                szFullPathBuffer,
                                                & dwLocalBufferSize,
                                                (LANGID) ((dwFlags >> 16) & 0x0000FFFF),
                                                (DWORD) (dwFlags & 0x0000FFFF));
                dwSizeRequired = dwLocalBufferSize;
            }

            if (pdhStatus == ERROR_SUCCESS && (szFullPathBuffer == NULL || * pcchBufferSize == 0)) {
                pdhStatus = PDH_MORE_DATA;
            }
            * pcchBufferSize = dwSizeRequired;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhMakeCounterPathA(
    IN  PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElements,
    IN  LPSTR                        szFullPathBuffer,
    IN  LPDWORD                      pcchBufferSize,
    IN  DWORD                        dwFlags
)
/*++
Routine Description:
    Constructs a counter path using the elemeents defined in the
        pCounterPathElements structure and returns the path string
        in the buffer provided by the caller. The resulting path
        is not validated.

Arguments:
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements
                The pointer to the structure containing the
                individual counter path fields that are to be
                assembled in to a path string
    IN      LPWSTR                      szFullPathBuffer
                The buffer to receive the path string. This value
                may be NULL if the value of the DWORD pointed to
                by pcchBufferSize is 0 indicating this is just a
                request for the required buffer size.
    IN      LPDWORD                     pcchBufferSize
                The pointer to the DWORD containing the size
                of the string buffer in characters. On return
                it contains the size of the buffer used in
                characters (including the terminating NULL char).
                If the value is 0 on entry then no data will be
                written to the buffer, but the required size will
                still be returned.
    IN      DWORD                       dwFlags
            if 0, then return the path as a REGISTRY path items
            if PDH_PATH_WBEM_RESULT then return the items in WBEM format
            if PDH_PATH_WBEM_INPUT then assume the input is in WBEM format

Return Value:
    ERROR_SUCCESS if the function completes successfully, otherwise a
        PDH error is returned.
    PDH_INVALID_ARGUMENT is returned when one of the arguments passed
        by the caller is incorrect or not accesible.
--*/
{
    PDH_STATUS  pdhStatus         = ERROR_SUCCESS;
    double      dIndex;
    double      dLen;
    DWORD       dwSizeRequired    = 0;
    LPSTR       szNextChar;
    DWORD       dwMaxSize;
    DWORD       dwLocalBufferSize = 0;

    if (pCounterPathElements == NULL|| pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            // test access to the input structure
            if (pCounterPathElements->szMachineName != NULL) {
                // then see if it's accessible
                if (* pCounterPathElements->szMachineName == '\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szObjectName != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szObjectName == '\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    //NULL is NOT ok for this field
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szInstanceName != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szInstanceName == '\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szParentInstance != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szParentInstance == '\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (pCounterPathElements->szCounterName != NULL) {
                    // then see if it's accessible
                    if (* pCounterPathElements->szCounterName == '\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    //NULL is NOT ok for this field
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

            // test the output buffers
            if (pdhStatus == ERROR_SUCCESS) {
                if (pcchBufferSize != NULL) {
                    dwLocalBufferSize = * pcchBufferSize;
                }
                else {
                    // NULL is NOT OK
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

            if (pdhStatus == ERROR_SUCCESS && szFullPathBuffer == NULL && dwLocalBufferSize > 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            if (pdhStatus == ERROR_SUCCESS) {
                // write to both ends of the buffer if one was sent
                if (dwLocalBufferSize > 0) {
                    if (szFullPathBuffer != NULL) {
                        * szFullPathBuffer = '\0';
                        szFullPathBuffer[dwLocalBufferSize - 1] = '\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            if (dwFlags == 0) {
                // this is a registry path in and out

                dwMaxSize = dwLocalBufferSize;

                if (pCounterPathElements->szMachineName != NULL) {
                    dwSizeRequired = lstrlenA(pCounterPathElements->szMachineName);
                    // compare the first two words of the machine name
                    // to see if the double backslash is already present in the string
                    if (* ((LPWORD) (pCounterPathElements->szMachineName)) != * ((LPWORD) (caszDoubleBackSlash))) {
                            // double backslash not found
                        dwSizeRequired += 2; // to include the backslashes
                    }
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        if (* ((LPWORD) (pCounterPathElements->szMachineName)) != * ((LPWORD) (caszDoubleBackSlash))) {
                            // double backslash not found
                            StringCchCopyA(szFullPathBuffer, dwMaxSize, caszDoubleBackSlash);
                            StringCchCatA(szFullPathBuffer, dwMaxSize, pCounterPathElements->szMachineName);
                        }
                        else {
                            StringCchCopyA(szFullPathBuffer, dwMaxSize, pCounterPathElements->szMachineName);
                        }
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }
                }

                dwSizeRequired += 1; // for delimiting slash
                dwSizeRequired += lstrlenA (pCounterPathElements->szObjectName);
                if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                    StringCchCatA(szFullPathBuffer, dwMaxSize, caszBackSlash);
                    StringCchCatA(szFullPathBuffer, dwMaxSize, pCounterPathElements->szObjectName);
                }
                else {
                    pdhStatus = PDH_MORE_DATA;
                }

                if (pCounterPathElements->szInstanceName != NULL) {
                    dwSizeRequired += 1; // for delimiting left paren
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        StringCchCatA(szFullPathBuffer, dwMaxSize, caszLeftParen);
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }

                    if (pCounterPathElements->szParentInstance != NULL) {
                        dwSizeRequired += lstrlenA(pCounterPathElements->szParentInstance);
                        dwSizeRequired += 1; // for delimiting slash
                        if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                            StringCchCatA(szFullPathBuffer, dwMaxSize, pCounterPathElements->szParentInstance);
                            StringCchCatA(szFullPathBuffer, dwMaxSize, caszSlash);
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                    }

                    dwSizeRequired += lstrlenA(pCounterPathElements->szInstanceName);
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        StringCchCatA(szFullPathBuffer, dwMaxSize, pCounterPathElements->szInstanceName);
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }

                    if (pCounterPathElements->dwInstanceIndex != PERF_NO_UNIQUE_ID
                            && pCounterPathElements->dwInstanceIndex != 0) {
                        // the length of the index is computed by getting the log of the number
                        // yielding the largest power of 10 less than or equal to the index.
                        // e.g. the power of 10 of an index value of 356 would 2.0 (which is the
                        // result of (floor(log10(index))). The actual number of characters in
                        // the string would always be 1 greate than that value so 1 is added.
                        // 1 more is added to include the delimiting character

                        dIndex          = (double) pCounterPathElements->dwInstanceIndex; // cast to float
                        dLen            = floor(log10(dIndex));                           // get integer log
                        dwSizeRequired  = (DWORD) dLen;                                   // cast to integer
                        dwSizeRequired += 2;                                              // increment

                        if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                            szNextChar      = & szFullPathBuffer[lstrlenA(szFullPathBuffer)];
                            * szNextChar ++ = POUNDSIGN_L;
                            _ltoa((long) pCounterPathElements->dwInstanceIndex, szNextChar, 10);
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                    }

                    dwSizeRequired += 1; // for delimiting parenthesis
                    if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                        StringCchCatA(szFullPathBuffer, dwMaxSize, caszRightParen);
                    }
                    else {
                        pdhStatus = PDH_MORE_DATA;
                    }
                }

                dwSizeRequired ++;   // include delimiting Backslash
                dwSizeRequired += lstrlenA(pCounterPathElements->szCounterName);
                if (szFullPathBuffer != NULL && dwSizeRequired <= dwMaxSize) {
                    StringCchCatA(szFullPathBuffer, dwMaxSize, caszBackSlash);
                    StringCchCatA(szFullPathBuffer, dwMaxSize, pCounterPathElements->szCounterName);
                }
                else {
                        pdhStatus = PDH_MORE_DATA;
                }
                dwSizeRequired ++;   // include trailing Null char
            }
            else {
                // this is a WBEM path so have the WBEM function figure
                // it out
                // there is some WBEM component involved so send to WBEM function
                // to figure it out
                pdhStatus = PdhiEncodeWbemPathA(pCounterPathElements,
                                                szFullPathBuffer,
                                                & dwLocalBufferSize,
                                                (LANGID) ((dwFlags >> 16) & 0x0000FFFF),
                                                (DWORD) (dwFlags & 0x0000FFFF));
                dwSizeRequired = dwLocalBufferSize;
            }

            if (pdhStatus == ERROR_SUCCESS && (szFullPathBuffer == NULL || * pcchBufferSize == 0)) {
                pdhStatus = PDH_MORE_DATA;
            }
            * pcchBufferSize = dwSizeRequired;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhParseCounterPathW(
    IN  LPCWSTR                      szFullPathBuffer,
    IN  PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElements,
    IN  LPDWORD                      pcchBufferSize,
    IN  DWORD                        dwFlags
)
/*++
Routine Description:
    Reads a perf counter path string and parses out the
        component fields, returning them in a buffer
        supplied by the calling function.

Arguments:
    IN      LPCWSTR                     szFullPathBuffer
                counter path string to parse.
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements
                pointer to buffer supplied by the caller in
                which the component fields will be written
                This buffer is cast as a structure, however, the
                string data is written to the space after
                the buffer.
    IN      LPDWORD                     pcchBufferSize
                the size of the buffer in BYTES. If specified size
                is 0, then the size is estimated and returned
                in this field and the buffer referenced by the
                agrument above is ignored.
    IN      DWORD                       dwFlags
            if 0, then return the path elements as REGISTRY path items
            if PDH_PATH_WBEM_RESULT then return the items in WBEM format
            if PDH_PATH_WBEM_INPUT then assume the input is in WBEM format

Return Value:
    ERROR_SUCCESS if the function completes successfully, otherwise
        a PDH error if not
    PDH_INVALID_ARGUMENT is returned when an argument is inocrrect or
        this function does not have the necessary access to that arg.
    PDH_INVALID_PATH is returned when the path is not formatted correctly
        and cannot be parsed.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a temporary buffer
        cannot be allocated
--*/
{
    PPDHI_COUNTER_PATH  pLocalCounterPath;
    PDH_STATUS          pdhStatus     = ERROR_SUCCESS;
    DWORD               dwSize;
    LPWSTR              szString      = NULL;
    DWORD               dwLocalBufferSize;
    DWORD               dwString;

    //validate incoming arguments
    if (szFullPathBuffer == NULL || pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwLocalBufferSize = * pcchBufferSize;
            // string cannot be null
            if (* szFullPathBuffer == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szFullPathBuffer) > PDH_MAX_COUNTER_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwLocalBufferSize > 0) {
                    if (pCounterPathElements != NULL) {
                        // try both "ends" of the buffer to see if an AV occurs
                        ((LPBYTE) pCounterPathElements) [0]                     = 0;
                        ((LPBYTE) pCounterPathElements) [dwLocalBufferSize - 1] = 0;
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
            if (dwFlags != 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwFlags == 0) {
            // allocate a temporary work buffer
            dwString          = 2 * (lstrlenW(szFullPathBuffer) + lstrlenW(szStaticLocalMachineName)
                                                                + PDH_MAX_COUNTER_PATH);
            dwSize            = sizeof(PDHI_COUNTER_PATH) + dwString * sizeof(WCHAR);
            pLocalCounterPath = G_ALLOC(dwSize);
            if (pLocalCounterPath != NULL) {
                if (ParseFullPathNameW(szFullPathBuffer, & dwSize, pLocalCounterPath, FALSE)) {
                    // parsed successfully so load into user's buffer
                    dwSize   = sizeof(PDH_COUNTER_PATH_ELEMENTS_W);
                    if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                        szString = (LPWSTR) & pCounterPathElements[1];
                    }
                    else {
                        szString  = NULL;
                        pdhStatus = PDH_MORE_DATA;
                    }
                    if (pLocalCounterPath->szMachineName != NULL) {
                        dwString = lstrlenW(pLocalCounterPath->szMachineName) + 1;
                        if (szString != NULL && dwSize + dwString * sizeof(WCHAR) <= dwLocalBufferSize) {
                            pCounterPathElements->szMachineName = szString;
                            StringCchCopyW(szString, dwString, pLocalCounterPath->szMachineName);
                            szString += dwString;
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                        dwSize += dwString * sizeof(WCHAR);
                    }
                    else if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                        pCounterPathElements->szMachineName = NULL;
                    }

                    if (pLocalCounterPath->szObjectName != NULL) {
                        dwString = lstrlenW(pLocalCounterPath->szObjectName) + 1;
                        if (szString != NULL && dwSize + dwString * sizeof(WCHAR) <= dwLocalBufferSize) {
                            pCounterPathElements->szObjectName = szString;
                            StringCchCopyW(szString, dwString, pLocalCounterPath->szObjectName);
                            szString += dwString;
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                        dwSize += dwString * sizeof(WCHAR);
                    }
                    else if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                        pCounterPathElements->szObjectName = NULL;
                    }

                    if (pLocalCounterPath->szInstanceName != NULL) {
                        dwString = lstrlenW(pLocalCounterPath->szInstanceName) + 1;
                        if (szString != NULL && dwSize + dwString * sizeof(WCHAR) <= dwLocalBufferSize) {
                            pCounterPathElements->szInstanceName = szString;
                            StringCchCopyW(szString, dwString, pLocalCounterPath->szInstanceName);
                            szString += dwString;
                        }
                        else {
                            pdhStatus = PDH_MORE_DATA;
                        }
                        dwSize += dwString * sizeof(WCHAR);

                        if (pLocalCounterPath->szParentName != NULL) {
                            dwString = lstrlenW(pLocalCounterPath->szParentName) + 1;
                            if (szString != NULL && dwSize + dwString * sizeof(WCHAR) <= dwLocalBufferSize) {
                                pCounterPathElements->szParentInstance = szString;
                                StringCchCopyW(szString, dwString, pLocalCounterPath->szParentName);
                                szString += dwString;
                            }
                            else {
                                pdhStatus = PDH_MORE_DATA;
                            }
                            dwSize += dwString * sizeof(WCHAR);
                        }
                        else if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                            pCounterPathElements->szParentInstance = NULL;
                        }
                        if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                            pCounterPathElements->dwInstanceIndex = pLocalCounterPath->dwIndex;
                        }
                    }
                    else if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                        pCounterPathElements->szInstanceName   = NULL;
                        pCounterPathElements->szParentInstance = NULL;
                        pCounterPathElements->dwInstanceIndex  = PERF_NO_UNIQUE_ID;
                    }

                    if (pLocalCounterPath->szCounterName != NULL) {
                        dwString = lstrlenW(pLocalCounterPath->szCounterName) + 1;
                        if (szString != NULL && dwSize + dwString * sizeof(WCHAR) <= dwLocalBufferSize) {
                            pCounterPathElements->szCounterName = szString;
                            StringCchCopyW(szString, dwString, pLocalCounterPath->szCounterName);
                            szString += dwString;
                        }
                        dwSize += dwString * sizeof(WCHAR);
                    }
                    else if (pCounterPathElements != NULL && dwSize <= dwLocalBufferSize) {
                        pCounterPathElements->szCounterName = NULL;
                    }
                    dwLocalBufferSize = dwSize;
                }
                else {
                    // unable to read path
                    pdhStatus = PDH_INVALID_PATH;
                }
                G_FREE(pLocalCounterPath);
            }
            else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            pdhStatus = PdhiDecodeWbemPathW(szFullPathBuffer,
                                            pCounterPathElements,
                                            & dwLocalBufferSize,
                                            (LANGID) ((dwFlags >> 16) & 0x0000FFFF),
                                            (DWORD) (dwFlags & 0x0000FFFF));
        }
    }

    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        * pcchBufferSize = dwLocalBufferSize;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhParseCounterPathA(
    IN  LPCSTR                       szFullPathBuffer,
    IN  PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElements,
    IN  LPDWORD                      pcchBufferSize,
    IN  DWORD                        dwFlags
)
/*++
Routine Description:
    Reads a perf counter path string and parses out the
        component fields, returning them in a buffer
        supplied by the calling function.

Arguments:
    IN      LPCSTR                     szFullPathBuffer
                counter path string to parse.
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements
                pointer to buffer supplied by the caller in
                which the component fields will be written
                This buffer is cast as a structure, however, the
                string data is written to the space after
                the buffer.
    IN      LPDWORD                     pcchBufferSize
                the size of the buffer in BYTES. If specified size
                is 0, then the size is estimated and returned
                in this field and the buffer referenced by the
                agrument above is ignored.
    IN      DWORD                       dwFlags
            if 0, then return the path as a REGISTRY path items
            if PDH_PATH_WBEM_RESULT then return the items in WBEM format
            if PDH_PATH_WBEM_INPUT then assume the input is in WBEM format

Return Value:
    ERROR_SUCCESS if the function completes successfully, otherwise
        a PDH error if not
    PDH_INVALID_ARGUMENT is returned when an argument is inocrrect or
        this function does not have the necessary access to that arg.
    PDH_INVALID_PATH is returned when the path is not formatted correctly
        and cannot be parsed.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a temporary buffer
        cannot be allocated
--*/
{
    PPDHI_COUNTER_PATH  pLocalCounterPath = NULL;
    LPWSTR              wszFullPath       = NULL;
    PDH_STATUS          pdhStatus         = ERROR_SUCCESS;
    PDH_STATUS          pdhStatus1        = ERROR_SUCCESS;
    DWORD               dwSize;
    DWORD               dwSizeUsed;
    LPSTR               szString;
    DWORD               dwSizeofLocalCounterPath;
    DWORD               dwLocalBufferSize = 0;

    //validate incoming arguments
    if (szFullPathBuffer == NULL || pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            // capture buffer size locally
            dwLocalBufferSize = * pcchBufferSize;

            if (dwFlags != 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            // the name must be non null
            else if (* szFullPathBuffer == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szFullPathBuffer) > PDH_MAX_COUNTER_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (dwLocalBufferSize > 0) {
                if (pCounterPathElements != NULL) {
                    // try both "ends" of the buffer to see if an AV occurs
                    * ((LPBYTE) pCounterPathElements)                      = 0;
                    ((LPBYTE) pCounterPathElements)[dwLocalBufferSize - 1] = 0;
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        if (dwFlags == 0) {
            dwSize = lstrlenA(szFullPathBuffer) + lstrlenW(szStaticLocalMachineName) + 1;
            dwSize = QWORD_MULTIPLE(dwSize);
            dwSizeofLocalCounterPath = sizeof(PDHI_COUNTER_PATH) + (2 * dwSize * sizeof(WCHAR));
            dwSizeofLocalCounterPath = QWORD_MULTIPLE(dwSizeofLocalCounterPath);

            wszFullPath = G_ALLOC(dwSize * sizeof(WCHAR) + dwSizeofLocalCounterPath);
            if (wszFullPath != NULL) {
                pLocalCounterPath = (PPDHI_COUNTER_PATH) ((LPBYTE) wszFullPath + dwSize * sizeof(WCHAR));
                MultiByteToWideChar(_getmbcp(), 0, szFullPathBuffer, -1, (LPWSTR) wszFullPath, dwSize);
                dwSize            = (DWORD) dwSizeofLocalCounterPath;
                if (ParseFullPathNameW(wszFullPath, & dwSize, pLocalCounterPath, FALSE)) {
                    // parsed successfully so load into user's buffer
                    // adjust dwSize to account for single-byte characters
                    // as they'll be packed in user's buffer.

                    dwSizeUsed = sizeof(PDH_COUNTER_PATH_ELEMENTS);
                    if (pCounterPathElements != NULL && dwSizeUsed <= dwLocalBufferSize) {
                        szString = (LPSTR) & pCounterPathElements[1];
                    }
                    else {
                        szString  = NULL;
                        pdhStatus = PDH_MORE_DATA;
                    }

                    if (pLocalCounterPath->szMachineName != NULL) {
                        dwSize = (dwLocalBufferSize >= dwSizeUsed) ? (dwLocalBufferSize - dwSizeUsed) : (0);
                        pdhStatus1 = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                              pLocalCounterPath->szMachineName,
                                                              szString,
                                                              & dwSize);
                        if (dwSize % sizeof(DWORD) != 0) {
                            dwSize = sizeof(DWORD) * ((dwSize / sizeof(DWORD)) + 1);
                        }
                        dwSizeUsed += dwSize;
                        if (pdhStatus1 == ERROR_SUCCESS) {
                            pCounterPathElements->szMachineName = szString;
                            szString += dwSize;
                        }
                        else if (pdhStatus == ERROR_SUCCESS) {
                            pdhStatus = pdhStatus1;
                        }
                    }
                    else if (pCounterPathElements != NULL && dwSizeUsed <= dwLocalBufferSize) {
                        pCounterPathElements->szMachineName = NULL;
                    }

                    if (pLocalCounterPath->szObjectName != NULL) {
                        dwSize = (dwLocalBufferSize >= dwSizeUsed) ? (dwLocalBufferSize - dwSizeUsed) : (0);
                        pdhStatus1 = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                              pLocalCounterPath->szObjectName,
                                                              szString,
                                                              & dwSize);
                        if (dwSize % sizeof(DWORD) != 0) {
                            dwSize = sizeof(DWORD) * ((dwSize / sizeof(DWORD)) + 1);
                        }
                        dwSizeUsed += dwSize;
                        if (pdhStatus1 == ERROR_SUCCESS) {
                            pCounterPathElements->szObjectName = szString;
                            szString += dwSize;
                        }
                        else if (pdhStatus == ERROR_SUCCESS) {
                            pdhStatus = pdhStatus1;
                        }
                    }
                    else if (pCounterPathElements != NULL && dwSizeUsed <= dwLocalBufferSize) {
                        pCounterPathElements->szObjectName = NULL;
                    }

                    if (pLocalCounterPath->szInstanceName != NULL) {
                        dwSize = (dwLocalBufferSize >= dwSizeUsed) ? (dwLocalBufferSize - dwSizeUsed) : (0);
                        pdhStatus1 = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                              pLocalCounterPath->szInstanceName,
                                                              szString,
                                                              & dwSize);
                        if (dwSize % sizeof(DWORD) != 0) {
                            dwSize = sizeof(DWORD) * ((dwSize / sizeof(DWORD)) + 1);
                        }
                        dwSizeUsed += dwSize;
                        if (pdhStatus1 == ERROR_SUCCESS) {
                            pCounterPathElements->szInstanceName = szString;
                            szString += dwSize;
                        }
                        else if (pdhStatus == ERROR_SUCCESS) {
                            pdhStatus = pdhStatus1;
                        }
                        if (pLocalCounterPath->szParentName != NULL) {
                            dwSize = (dwLocalBufferSize >= dwSizeUsed) ? (dwLocalBufferSize - dwSizeUsed) : (0);
                            pdhStatus1 = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                  pLocalCounterPath->szParentName,
                                                                  szString,
                                                                  & dwSize);
                            if (dwSize % sizeof(DWORD) != 0) {
                                dwSize = sizeof(DWORD) * ((dwSize / sizeof(DWORD)) + 1);
                            }
                            dwSizeUsed += dwSize;
                            if (pdhStatus1 == ERROR_SUCCESS) {
                                pCounterPathElements->szParentInstance = szString;
                                szString += dwSize;
                            }
                            else if (pdhStatus == ERROR_SUCCESS) {
                                pdhStatus = pdhStatus1;
                            }
                        }
                        else if (pCounterPathElements != NULL && dwSizeUsed <= dwLocalBufferSize) {
                            pCounterPathElements->szParentInstance = NULL;
                        }
                        if (pCounterPathElements != NULL && dwSizeUsed <= dwLocalBufferSize) {
                            pCounterPathElements->dwInstanceIndex = pLocalCounterPath->dwIndex;
                        }
                    }
                    else if (pCounterPathElements != NULL && dwSizeUsed <= dwLocalBufferSize) {
                        pCounterPathElements->szInstanceName   = NULL;
                        pCounterPathElements->szParentInstance = NULL;
                        pCounterPathElements->dwInstanceIndex  = PERF_NO_UNIQUE_ID;
                    }

                    if (pLocalCounterPath->szCounterName != NULL) {
                        dwSize = (dwLocalBufferSize >= dwSizeUsed) ? (dwLocalBufferSize - dwSizeUsed) : (0);
                        pdhStatus1 = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                              pLocalCounterPath->szCounterName,
                                                              szString,
                                                              & dwSize);
                        if (dwSize % sizeof(DWORD) != 0) {
                            dwSize = sizeof(DWORD) * ((dwSize / sizeof(DWORD)) + 1);
                        }
                        dwSizeUsed += dwSize;
                        if (pdhStatus1 == ERROR_SUCCESS) {
                            pCounterPathElements->szCounterName = szString;
                            szString += dwSize;
                        }
                        else if (pdhStatus == ERROR_SUCCESS) {
                            pdhStatus = pdhStatus1;
                        }
                    }
                    else if (pCounterPathElements != NULL && dwSizeUsed < dwLocalBufferSize) {
                        pCounterPathElements->szCounterName = NULL;
                    }

                    dwLocalBufferSize = dwSizeUsed;
                }
                else {
                    pdhStatus = PDH_INVALID_PATH;
                }
                G_FREE(wszFullPath);
            }
            else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            // this is a WBEM path so have the WBEM function figure
            // it out
            // there is some WBEM component involved so send to WBEM function
            // to figure it out
            pdhStatus = PdhiDecodeWbemPathA(szFullPathBuffer,
                                            pCounterPathElements,
                                            & dwLocalBufferSize,
                                            (LANGID) ((dwFlags >> 16) & 0x0000FFFF),
                                            (DWORD) (dwFlags & 0x0000FFFF));
        }
    }

    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        * pcchBufferSize = dwLocalBufferSize;
    }
    return pdhStatus;

}

PDH_FUNCTION
PdhParseInstanceNameW(
    IN  LPCWSTR szInstanceString,
    IN  LPWSTR  szInstanceName,
    IN  LPDWORD pcchInstanceNameLength,
    IN  LPWSTR  szParentName,
    IN  LPDWORD pcchParentNameLength,
    IN  LPDWORD lpIndex
)
/*++
Routine Description:
    parses the fields of an instance string and returns them in the
    buffers supplied by the caller

Arguments:
    szInstanceString
            is the pointer to the string containing the instance substring
            to parse into individual components. This string can contain the
        following formats and less than MAX_PATH chars in length:
        instance
        instance#index
        parent/instance
        parent/instance#index
    szInstanceName
        is the pointer to the buffer that will receive the instance
        name parsed from the instance string. This pointer can be
        NULL if the DWORD referenced by the pcchInstanceNameLength
        argument is 0.
    pcchInstanceNameLength
        is the pointer to the DWORD that contains the length of the
        szInstanceName buffer. If the value of this DWORD is 0, then
        the buffer size required to hold the instance name will be
        returned.
    szParentName
        is the pointer to the buffer that will receive the name
        of the parent index if one is specified. This argument can
        be NULL if the value of the DWORD referenced by the
        pcchParentNameLength argument is 0.
    lpIndex
        is the pointer to the DWORD that will receive the index
        value of the instance. If an index entry is not present in
        the string, then this value will be 0. This argument can
        be NULL if this information is not needed.

Return Value:
    ERROR_SUCCESS if the function completes successfully, otherwise
        a PDH error is returned.
    PDH_INVALID_ARGUMENT is returned when one or more of the
        arguments is invalid or incorrect.
    PDH_INVALID_INSTANCE is returned if the instance string is incorrectly
        formatted and cannot be parsed
--*/
{
    BOOL        bReturn;
    PDH_STATUS  pdhStatus           = ERROR_SUCCESS;
    DWORD       dwSize;
    DWORD       dwLocalIndex;
    LPWSTR      szLocalInstanceName = NULL;
    LPWSTR      szLocalParentName   = NULL;
    DWORD       dwLocalInstanceNameLength;
    DWORD       dwLocalParentNameLength;

    // test access to arguments

    if (szInstanceString == NULL || pcchInstanceNameLength == NULL || pcchParentNameLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } 
    else {
        __try {
            dwLocalInstanceNameLength = * pcchInstanceNameLength;
            dwLocalParentNameLength   = * pcchParentNameLength;

            if (* szInstanceString == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szInstanceString) > PDH_MAX_INSTANCE_NAME) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwLocalInstanceNameLength > 0) {
                    if (szInstanceName != NULL) {
                        WCHAR wChar      = * szInstanceName;
                        * szInstanceName = L'\0';
                        * szInstanceName = wChar;
                        wChar            = szInstanceName[dwLocalInstanceNameLength - 1];
                        szInstanceName[dwLocalInstanceNameLength - 1] = L'\0';
                        szInstanceName[dwLocalInstanceNameLength - 1] = wChar;
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } // else size only request
                if (dwLocalParentNameLength > 0) {
                    if (szParentName != NULL) {
                        WCHAR wChar    = * szParentName;
                        * szParentName = L'\0';
                        * szParentName = wChar;
                        wChar          = szParentName[dwLocalParentNameLength - 1];
                        szParentName[dwLocalParentNameLength - 1] = L'\0';
                        szParentName[dwLocalParentNameLength - 1] = wChar;
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } // else size only request
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        DWORD dwLength = lstrlenW(szInstanceString) + 1;
        if (dwLength < PDH_MAX_INSTANCE_NAME) dwLength = PDH_MAX_INSTANCE_NAME;

        szLocalInstanceName = G_ALLOC(2 * dwLength * sizeof(WCHAR));
        if (szLocalInstanceName != NULL) {
            szLocalParentName = (szLocalInstanceName + dwLength);
            bReturn = ParseInstanceName(
                            szInstanceString, szLocalInstanceName, szLocalParentName, dwLength, & dwLocalIndex);
        }
        else {
            bReturn = FALSE;
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }

        if (bReturn) {
            dwSize = lstrlenW(szLocalInstanceName) + 1;
            if (szInstanceName != NULL && dwSize <= dwLocalInstanceNameLength) {
                StringCchCopyW(szInstanceName, dwLocalInstanceNameLength, szLocalInstanceName);
            }
            else {
                pdhStatus = PDH_MORE_DATA;
            }
            dwLocalInstanceNameLength = dwSize; // include the trailing NULL

            dwSize = lstrlenW(szLocalParentName) + 1;
            if (szParentName != NULL && dwSize <= dwLocalParentNameLength) {
                StringCchCopyW(szParentName, dwLocalParentNameLength, szLocalParentName);
            }
            else {
                pdhStatus = PDH_MORE_DATA;
            }
            dwLocalParentNameLength = dwSize; // include the trailing NULL

            __try {
                * pcchInstanceNameLength = dwLocalInstanceNameLength;
                * pcchParentNameLength   = dwLocalParentNameLength;

                if (lpIndex != NULL) {
                    * lpIndex = dwLocalIndex;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        else if (pdhStatus == ERROR_SUCCESS) {
            // unable to parse string
            pdhStatus = PDH_INVALID_INSTANCE;
        }
    }

    G_FREE(szLocalInstanceName);
    return pdhStatus;
}

PDH_FUNCTION
PdhParseInstanceNameA(
    IN  LPCSTR  szInstanceString,
    IN  LPSTR   szInstanceName,
    IN  LPDWORD pcchInstanceNameLength,
    IN  LPSTR   szParentName,
    IN  LPDWORD pcchParentNameLength,
    IN  LPDWORD lpIndex
)
/*++
Routine Description:
    parses the fields of an instance string and returns them in the
    buffers supplied by the caller

Arguments:
    szInstanceString
            is the pointer to the string containing the instance substring
            to parse into individual components. This string can contain the
        following formats and less than MAX_PATH chars in length:
        instance
        instance#index
        parent/instance
        parent/instance#index
    szInstanceName
        is the pointer to the buffer that will receive the instance
        name parsed from the instance string. This pointer can be
        NULL if the DWORD referenced by the pcchInstanceNameLength
        argument is 0.
    pcchInstanceNameLength
        is the pointer to the DWORD that contains the length of the
        szInstanceName buffer. If the value of this DWORD is 0, then
        the buffer size required to hold the instance name will be
        returned.
    szParentName
        is the pointer to the buffer that will receive the name
        of the parent index if one is specified. This argument can
        be NULL if the value of the DWORD referenced by the
        pcchParentNameLength argument is 0.
    lpIndex
        is the pointer to the DWORD that will receive the index
        value of the instance. If an index entry is not present in
        the string, then this value will be 0. This argument can
        be NULL if this information is not needed.

Return Value:
    ERROR_SUCCESS if the function completes successfully, otherwise
        a PDH error is returned.
    PDH_INVALID_ARGUMENT is returned when one or more of the
        arguments is invalid or incorrect.
    PDH_INVALID_INSTANCE is returned if the instance string is incorrectly
        formatted and cannot be parsed
--*/
{
    BOOL    bReturn;
    LONG    pdhStatus            = ERROR_SUCCESS;
    DWORD   dwSize;
    LPWSTR  wszInstanceString    = NULL;
    LPWSTR  wszLocalInstanceName = NULL;
    LPWSTR  wszLocalParentName   = NULL;
    DWORD   dwLocalIndex         = 0;
    DWORD   dwLocalInstanceNameLength;
    DWORD   dwLocalParentNameLength;

    // test access to arguments

    if (szInstanceString == NULL || pcchInstanceNameLength == NULL || pcchParentNameLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } 
    else {
        __try {
            dwLocalInstanceNameLength = * pcchInstanceNameLength;
            dwLocalParentNameLength   = * pcchParentNameLength;

            if (* szInstanceString == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szInstanceString) > PDH_MAX_INSTANCE_NAME) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else {
                if (dwLocalInstanceNameLength > 0) {
                    if (szInstanceName != NULL) {
                        CHAR cChar       = * szInstanceName;
                        * szInstanceName = '\0';
                        * szInstanceName = cChar;
                        cChar            = szInstanceName[dwLocalInstanceNameLength - 1];
                        szInstanceName[dwLocalInstanceNameLength - 1] = '\0';
                        szInstanceName[dwLocalInstanceNameLength - 1] = cChar;
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } // else size only request
                if (dwLocalParentNameLength > 0) {
                    if (szParentName != NULL) {
                        CHAR cChar     = * szParentName;
                        * szParentName = '\0';
                        * szParentName = cChar;
                        cChar          = szParentName[dwLocalParentNameLength - 1];
                        szParentName[dwLocalParentNameLength - 1] = '\0';
                        szParentName[dwLocalParentNameLength - 1] = cChar;
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } // else size only request
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        dwSize = lstrlenA(szInstanceString) + 1;
        if (dwSize < PDH_MAX_INSTANCE_NAME) dwSize = PDH_MAX_INSTANCE_NAME;

        wszInstanceString = G_ALLOC(3 * dwSize * sizeof(WCHAR));
        if (wszInstanceString != NULL) {
            wszLocalInstanceName = (wszInstanceString    + dwSize);
            wszLocalParentName   = (wszLocalInstanceName + dwSize);
            MultiByteToWideChar(_getmbcp(), 0, szInstanceString, -1, wszInstanceString, dwSize);
            bReturn = ParseInstanceName(
                            wszInstanceString, wszLocalInstanceName, wszLocalParentName, dwSize, & dwLocalIndex);
        }
        else {
            // instance string is too long
            bReturn = FALSE;
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }

        if (bReturn) {
            PDH_STATUS pdhInstStatus   = ERROR_SUCCESS;
            PDH_STATUS pdhParentStatus = ERROR_SUCCESS;

            dwSize = dwLocalInstanceNameLength;
            pdhInstStatus = PdhiConvertUnicodeToAnsi(_getmbcp(), wszLocalInstanceName, szInstanceName, & dwSize);
            dwLocalInstanceNameLength = dwSize;

            dwSize = dwLocalParentNameLength;
            pdhParentStatus = PdhiConvertUnicodeToAnsi(_getmbcp(), wszLocalParentName, szParentName, & dwSize);
            if (pdhParentStatus == PDH_INVALID_ARGUMENT) {
                pdhParentStatus = ERROR_SUCCESS;
            }
            dwLocalParentNameLength = dwSize;
            if (pdhInstStatus != ERROR_SUCCESS) {
                pdhStatus = pdhInstStatus;
            }
            else {
                pdhStatus = pdhParentStatus;
            }

            __try {
                * pcchInstanceNameLength = dwLocalInstanceNameLength;
                * pcchParentNameLength   = dwLocalParentNameLength;

                if (lpIndex != NULL) {
                    * lpIndex = dwLocalIndex;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        else if (pdhStatus == ERROR_SUCCESS) {
            // unable to parse string
            pdhStatus = PDH_INVALID_INSTANCE;
        }
    } // else pass status through to caller

    G_FREE(wszInstanceString);
    return pdhStatus;
}

PDH_FUNCTION
PdhValidatePathW(
    IN  LPCWSTR szFullPathBuffer
)
/*++
Routine Description:
    breaks the specified path into its component parts and evaluates
        each of the part to make sure the specified path represents
        a valid and operational performance counter. The return value
        indicates the pdhStatus of the counter defined in the path string.

Arguments:
    IN      LPCWSTR szFullPathBuffer
                the full path string of the counter to validate.

Return Value:
    ERROR_SUCCESS of the counter was successfully located otherwise
        a PDH error.
    PDH_CSTATUS_NO_INSTANCE is returned if the specified instance of
        the performance object wasn't found
    PDH_CSTATUS_NO_COUNTER is returned if the specified counter was not
        found in the object.
    PDH_CSTATUS_NO_OBJECT is returned if the specified object was not
        found on the machine
    PDH_CSTATUS_NO_MACHINE is returned if the specified machine could
        not be found or connected to
    PDH_CSTATUS_BAD_COUNTERNAME is returned when the counter path string
        could not be parsed.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when the function is unable
        to allocate a required temporary buffer
    PDH_INVALID_ARGUMENT is returned when the counter path string argument
        could not be accessed
--*/
{
    PPERF_MACHINE             pMachine;
    PPDHI_COUNTER_PATH        pLocalCounterPath;
    DWORD                     dwSize;
    PPERF_OBJECT_TYPE         pPerfObjectDef = NULL;
    PPERF_INSTANCE_DEFINITION pPerfInstanceDef;
    PPERF_COUNTER_DEFINITION  pPerfCounterDef;
    PDH_STATUS                CStatus        = ERROR_SUCCESS;

    if (szFullPathBuffer != NULL) {
        // validate access to arguments
        __try {
            // make sure the name isn't empty
            if (* szFullPathBuffer == L'\0') {
                CStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szFullPathBuffer) > PDH_MAX_COUNTER_PATH) {
                CStatus = PDH_INVALID_ARGUMENT;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            CStatus = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        // cannot be null
        CStatus = PDH_INVALID_ARGUMENT;
    }
    if (CStatus == ERROR_SUCCESS) {
        CStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (CStatus == ERROR_SUCCESS) {
        __try {
            pLocalCounterPath = G_ALLOC(sizeof(PDHI_COUNTER_PATH) + 2 * sizeof(WCHAR)
                            * (lstrlenW(szFullPathBuffer) + 1 + lstrlenW(szStaticLocalMachineName) + 1));
            if (pLocalCounterPath != NULL) {
                dwSize = (DWORD) G_SIZE(pLocalCounterPath);

                if (ParseFullPathNameW(szFullPathBuffer, & dwSize, pLocalCounterPath, FALSE)) {
                    // parsed successfully so try to connect to machine
                    // and get machine pointer
                    pMachine = GetMachine(pLocalCounterPath->szMachineName, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
                    if (pMachine == NULL) {
                        CStatus = GetLastError();
                        if (CStatus == ERROR_SUCCESS) CStatus = PDH_CSTATUS_NO_MACHINE;
                    }
                    else if (pMachine->dwStatus != ERROR_SUCCESS) {
                        CStatus = pMachine->dwStatus;
                        pMachine->dwRefCount --;
                        RELEASE_MUTEX(pMachine->hMutex);
                        pMachine = NULL;
                    }
                    else {
                        DWORD dwObject = GetObjectId(pMachine, pLocalCounterPath->szObjectName, NULL);
                        if (dwObject == (DWORD) -1) {
                            pMachine->dwRefCount --;
                            RELEASE_MUTEX(pMachine->hMutex);
                            if (CStatus == ERROR_SUCCESS) CStatus = PDH_CSTATUS_NO_OBJECT;
                            pMachine = NULL;
                        }
                    }

                    if (pMachine != NULL) {
                        if (pMachine->dwStatus == ERROR_SUCCESS) {
                            // look up object name
                            pPerfObjectDef = GetObjectDefByName(pMachine->pSystemPerfData,
                                                                pMachine->dwLastPerfString,
                                                                pMachine->szPerfStrings,
                                                                pLocalCounterPath->szObjectName);
                        }
                        else {
                            pPerfObjectDef = NULL;
                        }

                        if (pPerfObjectDef != NULL) {
                            // look up instances if necessary
                            if (pPerfObjectDef->NumInstances != PERF_NO_INSTANCES) {
                                if (pLocalCounterPath->szInstanceName != NULL) {
                                    if (* pLocalCounterPath->szInstanceName != SPLAT_L) {
                                        pPerfInstanceDef = GetInstanceByName(pMachine->pSystemPerfData,
                                                        pPerfObjectDef,
                                                        pLocalCounterPath->szInstanceName,
                                                        pLocalCounterPath->szParentName,
                                                        (pLocalCounterPath->dwIndex != PERF_NO_UNIQUE_ID ?
                                                                        pLocalCounterPath->dwIndex : 0));
                                        if (pPerfInstanceDef == NULL) {
                                            // unable to lookup instance
                                            CStatus = PDH_CSTATUS_NO_INSTANCE;
                                        }
                                    }
                                    // wild card instances are OK IF multiple instances
                                    // are supported!!!
                                }
                                else {
                                    // no instance was specified for a counter
                                    // that should have an instance so this is
                                    // an invalid path
                                    CStatus = PDH_CSTATUS_NO_INSTANCE;
                                }
                            }
                            else {
                                // no instances in this counter, see if one
                                // is defined
                                if ((pLocalCounterPath->szInstanceName != NULL) ||
                                                (pLocalCounterPath->szParentName != NULL)) {
                                    // unable to lookup instance
                                    CStatus = PDH_CSTATUS_NO_INSTANCE;
                                }
                            }
                            if (CStatus == ERROR_SUCCESS) {
                                // and look up counter
                                pPerfCounterDef = GetCounterDefByName(pPerfObjectDef,
                                                                      pMachine->dwLastPerfString,
                                                                      pMachine->szPerfStrings,
                                                                      pLocalCounterPath->szCounterName);
                                if (pPerfCounterDef != NULL) {
                                    // counter found so return TRUE & valid
                                    CStatus = ERROR_SUCCESS;
                                }
                                else {
                                    // unable to lookup counter
                                    CStatus = PDH_CSTATUS_NO_COUNTER;
                                }
                            }
                        }
                        else {
                            // unable to lookup object
                            CStatus = PDH_CSTATUS_NO_OBJECT;
                        }
                        pMachine->dwRefCount --;
                        RELEASE_MUTEX (pMachine->hMutex);
                    }
                    else if (CStatus == ERROR_SUCCESS) {
                        // unable to find machine
                        CStatus = PDH_CSTATUS_NO_MACHINE;
                    }
                }
                else {
                    // unable to parse counter name
                    CStatus = PDH_CSTATUS_BAD_COUNTERNAME;
                }
                G_FREE(pLocalCounterPath);
            }
            else {
                // unable to allocate memory
                CStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            CStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX (hPdhDataMutex);

    } // else pass error to caller
    return CStatus;
}

PDH_FUNCTION
PdhValidatePathA(
    IN  LPCSTR  szFullPathBuffer
)
/*++
Routine Description:
    breaks the specified path into its component parts and evaluates
        each of the part to make sure the specified path represents
        a valid and operational performance counter. The return value
        indicates the pdhStatus of the counter defined in the path string.

Arguments:
    IN      LPCSTR szFullPathBuffer
                the full path string of the counter to validate.

Return Value:
    ERROR_SUCCESS of the counter was successfully located otherwise
        a PDH error.
    PDH_CSTATUS_NO_INSTANCE is returned if the specified instance of
        the performance object wasn't found
    PDH_CSTATUS_NO_COUNTER is returned if the specified counter was not
        found in the object.
    PDH_CSTATUS_NO_OBJECT is returned if the specified object was not
        found on the machine
    PDH_CSTATUS_NO_MACHINE is returned if the specified machine could
        not be found or connected to
    PDH_CSTATUS_BAD_COUNTERNAME is returned when the counter path string
        could not be parsed.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when the function is unable
        to allocate a required temporary buffer
--*/
{
    LPWSTR     wszFullPath = NULL;
    PDH_STATUS Status      = ERROR_SUCCESS;

    if (szFullPathBuffer != NULL) {
        __try {
            // cannot be blank
            if (* szFullPathBuffer == '\0') {
                Status = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szFullPathBuffer) > PDH_MAX_COUNTER_PATH) {
                Status = PDH_INVALID_ARGUMENT;
            }
            else {
                wszFullPath = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szFullPathBuffer);
                if (wszFullPath == NULL) Status = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        // cannot be null
        Status = PDH_INVALID_ARGUMENT;
    }

    if (Status == ERROR_SUCCESS) {
        Status = PdhValidatePathW(wszFullPath);
    }
    G_FREE(wszFullPath);
    return Status;
}

PDH_FUNCTION
PdhiGetDefaultPerfObjectW(
    DWORD   dwDataSource,
    LPCWSTR szMachineName,
    LPWSTR  szDefaultObjectName,
    LPDWORD pcchBufferSize
)
/*++
Routine Description:
    Obtains the default performance object from the specified machine.

Arguments:
    IN      DWORD    dwDataSourcetype
    IN      LPCWSTR szMachineName
                NULL indicates the local machine, othewise this is the
                name of the remote machine to query. If this machine is
                not known to the PDH DLL, then it will be connected.
    IN      LPWSTR  szDefaultObjectName
                pointer to the buffer that will receive the default object
                name. This pointer can be NULL if the value of the DWORD
                referenced by bcchBufferSize is 0.
    IN      LPDWORD pcchBufferSize
                pointer to a DWORD containing the size of the buffer, in
                characters, referenced by the szDefaultObjectName argument.
                If the value of this DWORD is 0, then no data will be written
                to the szDefaultObjectNameBuffer, however the required
                buffer size will be returned in the DWORD referenced by
                this pointer.

Return Value:
    ERROR_SUCCESS if this function completes normally otherwise a PDH error.
    PDH_INVALID_ARGUMENT a required argument is not correct or reserved
        argument is not 0 or NULL.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a required temporary
        buffer could not be allocated.
    PDH_CSTATUS_NO_COUNTERNAME is returned when the default object
        name cannot be read or found.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
--*/
{
    PPERF_MACHINE   pMachine;
    PDH_STATUS      pdhStatus         = ERROR_SUCCESS;
    LONG            lDefault;
    DWORD           dwStringLen;
    DWORD           dwLocalBufferSize = 0;
    LPWSTR          szDefault;
    LPWSTR          szThisMachine     = NULL;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        // test the access the arguments
        __try {
            if (szMachineName != NULL) {
                // if there's a machine name then it cannot be blank
                if (* szMachineName == L'\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenW(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            } // else NULL machine Name is OK

            if (pdhStatus == ERROR_SUCCESS) {
                dwLocalBufferSize = * pcchBufferSize;
                if (dwLocalBufferSize > 0) {
                    if (szDefaultObjectName != NULL) {
                        // test both ends of the caller's buffer for
                        // write access
                        szDefaultObjectName[0]                    = L'\0';
                        szDefaultObjectName[dwLocalBufferSize -1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (pdhStatus == ERROR_SUCCESS && szMachineName != NULL) {
        DWORD dwMachineName = (* szMachineName == L'\\') ? (lstrlenW(szMachineName) + 1)
                                                         : (lstrlenW(szMachineName) + 3);
        dwMachineName *= sizeof(WCHAR);
        szThisMachine = G_ALLOC(dwMachineName);
        if (szThisMachine == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else if (* szMachineName == L'\\') {
            StringCchCopyW(szThisMachine, dwMachineName, szMachineName);
        }
        else {
            StringCchCopyW(szThisMachine, dwMachineName, cszDoubleBackSlash);
            StringCchCatW(szThisMachine, dwMachineName, szMachineName);
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pMachine  = GetMachine((LPWSTR) szThisMachine, 0, 0);
                pdhStatus = (pMachine != NULL ? pMachine->dwStatus : GetLastError());
                if (pMachine != NULL) {
                    if (pdhStatus == ERROR_SUCCESS) {
                        // only look at buffers from machines that are "on line"
                        lDefault = pMachine->pSystemPerfData->DefaultObject;
                        if ((lDefault > 0) && ((DWORD) lDefault < pMachine->dwLastPerfString)) {
                            // then there should be a string in the table
                            szDefault = (LPWSTR) PdhiLookupPerfNameByIndex(pMachine, lDefault);
                            if (szDefault != NULL) {
                                // determine string buffer length including term. NULL char
                                dwStringLen = lstrlenW(szDefault) + 1;
                                if (szDefaultObjectName != NULL && dwStringLen <= dwLocalBufferSize) {
                                    StringCchCopyW(szDefaultObjectName, dwLocalBufferSize, szDefault);
                                    pdhStatus = ERROR_SUCCESS;
                                }
                                else {
                                    pdhStatus = PDH_MORE_DATA;
                                }
                            }
                            else {
                                // unable to find a matching counter name
                                pdhStatus   = PDH_CSTATUS_NO_COUNTERNAME;
                                dwStringLen = 0;
                            }
                        }
                        else {
                            // string not in table
                            pdhStatus   = PDH_CSTATUS_NO_COUNTERNAME;
                            dwStringLen = 0;
                        }
                        dwLocalBufferSize = dwStringLen;
                    }
                    pMachine->dwRefCount --;
                    RELEASE_MUTEX(pMachine->hMutex);
                } // else pass error pdhStatus on to the caller
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiGetDefaultWbemObject(szThisMachine,
                                                     (LPVOID) szDefaultObjectName,
                                                     & dwLocalBufferSize,
                                                     TRUE); // unicode function
                break;

            case DATA_SOURCE_LOGFILE:
                // log files don't support this (for now)
                // but this is still successful.
                dwLocalBufferSize = 0;
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }
        }
         __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX(hPdhDataMutex);
    }

    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    G_FREE(szThisMachine);
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfObjectHW(
    IN  HLOG    hDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPWSTR  szDefaultObjectName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    __try {
        dwDataSourceType = DataSourceTypeH(hDataSource);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfObjectW(dwDataSourceType, szMachineName, szDefaultObjectName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfObjectW(
    IN  LPCWSTR szDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPWSTR  szDefaultObjectName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        if (szDataSource != NULL) {
            if (* szDataSource == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSourceType = DataSourceTypeW(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfObjectW(dwDataSourceType, szMachineName, szDefaultObjectName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetDefaultPerfObjectA(
    DWORD   dwDataSource,
    LPCSTR  szMachineName,
    LPSTR   szDefaultObjectName,
    LPDWORD pcchBufferSize
)
/*++
Routine Description:
    Obtains the default performance object from the specified machine.

Arguments:
    IN      DWORD   dwDataSourceType
    IN      LPCSTR szMachineName
                NULL indicates the local machine, othewise this is the
                name of the remote machine to query. If this machine is
                not known to the PDH DLL, then it will be connected.
    IN      LPSTR  szDefaultObjectName
                pointer to the buffer that will receive the default object
                name. This pointer can be NULL if the value of the DWORD
                referenced by bcchBufferSize is 0.
    IN      LPDWORD pcchBufferSize
                pointer to a DWORD containing the size of the buffer, in
                characters, referenced by the szDefaultObjectName argument.
                If the value of this DWORD is 0, then no data will be written
                to the szDefaultObjectNameBuffer, however the required
                buffer size will be returned in the DWORD referenced by
                this pointer.

Return Value:
    ERROR_SUCCESS if this function completes normally otherwise a PDH error.
    PDH_INVALID_ARGUMENT a required argument is not correct or reserved
        argument is not 0 or NULL.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a required temporary
        buffer could not be allocated.
    PDH_CSTATUS_NO_COUNTERNAME is returned when the default object
        name cannot be read or found.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
--*/
{
    LPWSTR         szWideName        = NULL;
    DWORD          dwNameLength;
    PDH_STATUS     pdhStatus         = ERROR_SUCCESS;
    PPERF_MACHINE  pMachine          = NULL;
    LONG           lDefault;
    DWORD          dwStringLen;
    DWORD          dwLocalBufferSize = 0;
    LPWSTR         szDefault         = NULL;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    // test the access the arguments
    __try {
        if (szMachineName != NULL) {
            if (* szMachineName == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szMachineName) > PDH_MAX_COUNTER_NAME) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL machine Name is OK

        if (pdhStatus == ERROR_SUCCESS) {
            dwLocalBufferSize = * pcchBufferSize;
            if (dwLocalBufferSize > 0) {
                if (szDefaultObjectName != NULL) {
                    // test both ends of the caller's buffer for
                    // write access
                    szDefaultObjectName[0]                    = '\0';
                    szDefaultObjectName[dwLocalBufferSize -1] = '\0';
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS && szMachineName != NULL) {
        LPWSTR szTmpMachine = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szMachineName);
        if (szTmpMachine == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else if (* szTmpMachine == '\\') {
            szWideName = szTmpMachine;
        }
        else {
            szWideName = G_ALLOC((lstrlenW(szTmpMachine) + 3) * sizeof(WCHAR));
            if (szWideName == NULL) {
                szWideName = szTmpMachine;
            }
            else {
                StringCchPrintfW(szWideName,
                                 lstrlenW(szTmpMachine) + 3,
                                 L"%ws%ws",
                                 cszDoubleBackSlash,
                                 szTmpMachine);
                G_FREE(szTmpMachine);
            }
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                if (pdhStatus == ERROR_SUCCESS) {
                    pMachine  = GetMachine(szWideName, 0, 0);
                    pdhStatus = (pMachine != NULL ? pMachine->dwStatus : GetLastError());
                }
                if (pMachine != NULL) {
                    if (pdhStatus == ERROR_SUCCESS) {
                        // only look at buffers from machines that are "on line"
                        lDefault = pMachine->pSystemPerfData->DefaultObject;
                        if ((lDefault > 0) && ((DWORD) lDefault < pMachine->dwLastPerfString)) {
                            // then there should be a string in the table
                            szDefault = (LPWSTR) PdhiLookupPerfNameByIndex(pMachine, lDefault);
                            if (szDefault != NULL) {
                                // determine string buffer length including term. NULL char
                                dwStringLen = dwLocalBufferSize;
                                pdhStatus   = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                       szDefault,
                                                                       szDefaultObjectName,
                                                                       & dwStringLen);
                            }
                            else {
                                // unable to find a matching counter name
                                pdhStatus   = PDH_CSTATUS_NO_COUNTERNAME;
                                dwStringLen = 0;
                            }
                        }
                        else {
                            // string not in table
                            pdhStatus = PDH_CSTATUS_NO_COUNTERNAME;
                            dwStringLen = 0;
                        }
                        dwLocalBufferSize = dwStringLen;
                    }
                    pMachine->dwRefCount --;
                    RELEASE_MUTEX(pMachine->hMutex);
                } // else pass error pdhStatus on to the caller
                break;

            case DATA_SOURCE_WBEM:
            case DATA_SOURCE_LOGFILE:
                if (pdhStatus == ERROR_SUCCESS && dwDataSource == DATA_SOURCE_WBEM) {
                    pdhStatus = PdhiGetDefaultWbemObject(szWideName,
                                                         (LPVOID) szDefaultObjectName,
                                                         & dwLocalBufferSize,
                                                         FALSE); // ANSI function
                }
                else {
                    //log files don't support this (for now)
                    dwLocalBufferSize = 0;
                }
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX (hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

Cleanup:
    G_FREE(szWideName);
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfObjectHA(
    IN  HLOG    hDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPSTR   szDefaultObjectName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        dwDataSourceType = DataSourceTypeH(hDataSource);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfObjectA(dwDataSourceType, szMachineName, szDefaultObjectName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfObjectA(
    IN  LPCSTR  szDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPSTR   szDefaultObjectName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        if (szDataSource != NULL) {
            if (* szDataSource == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSourceType = DataSourceTypeA(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfObjectA(dwDataSourceType, szMachineName, szDefaultObjectName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetDefaultPerfCounterW(
    IN  DWORD   dwDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szObjectName,
    IN  LPWSTR  szDefaultCounterName,
    IN  LPDWORD pcchBufferSize
)
/*++
Routine Description:
    Obtains the default performance counter from the specified object on
        the specified machine.

Arguments:
    IN      DWORD   dwDataSource
    IN      LPCWSTR szMachineName
                NULL indicates the local machine, othewise this is the
                name of the remote machine to query. If this machine is
                not known to the PDH DLL, then it will be connected.
    IN      LPCWSTR szObjectName
                a pointer to the buffer that contains the name of the object
                on the machine to find the default counter for.
    IN      LPWSTR  szDefaultCounterName
                pointer to the buffer that will receive the default counter
                name. This pointer can be NULL if the value of the DWORD
                referenced by bcchBufferSize is 0.
    IN      LPDWORD pcchBufferSize
                pointer to a DWORD containing the size of the buffer, in
                characters, referenced by the szDefaultObjectName argument.
                If the value of this DWORD is 0, then no data will be written
                to the szDefaultObjectNameBuffer, however the required
                buffer size will be returned in the DWORD referenced by
                this pointer.

Return Value:
    ERROR_SUCCESS if this function completes normally otherwise a PDH error.
    PDH_INVALID_ARGUMENT a required argument is not correct or reserved
        argument is not 0 or NULL.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a required temporary
        buffer could not be allocated.
    PDH_CSTATUS_NO_COUNTERNAME is returned when the name string for the
        default counter could not be found.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_CSTATUS_NO_OBJECT is returned when the specified object could
        not be found on the specified computer.
    PDH_CSTATUS_NO_COUNTER is returned when the default counter is not
        found in the data buffer.
--*/
{
    PPERF_MACHINE            pMachine;
    PPERF_OBJECT_TYPE        pObjectDef;
    PPERF_COUNTER_DEFINITION pCounterDef;
    PDH_STATUS               pdhStatus         = ERROR_SUCCESS;
    LONG                     lDefault;
    DWORD                    dwStringLen;
    DWORD                    dwLocalBufferSize = 0;
    LPWSTR                   szDefault;
    LPWSTR                   szThisMachine     = NULL;

    if (szObjectName == NULL || pcchBufferSize == NULL) {
         pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        // test the access the arguments
        __try {
            if (szMachineName != NULL) {
                if (* szMachineName == L'\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenW(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            } // else NULL machine Name is OK

            if (pdhStatus == ERROR_SUCCESS) {
                if (szObjectName == NULL) {
                    // Null Object is not allowed
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (* szObjectName == L'\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenW(szObjectName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

            if (pdhStatus == ERROR_SUCCESS) {
                dwLocalBufferSize = * pcchBufferSize;
                if (dwLocalBufferSize > 0) {
                    if (szDefaultCounterName) {
                        // test both ends of the caller's buffer for
                        // write access
                        szDefaultCounterName[0]                     = L'\0';
                        szDefaultCounterName[dwLocalBufferSize - 1] = L'\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (szMachineName != NULL) {
            DWORD dwMachineName = (* szMachineName == L'\\') ? (lstrlenW(szMachineName) + 1)
                                                             : (lstrlenW(szMachineName) + 3);
            szThisMachine = G_ALLOC(dwMachineName * sizeof(WCHAR));
            if (szThisMachine == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else if (* szMachineName == L'\\') {
                StringCchCopyW(szThisMachine, dwMachineName, szMachineName);
            }
            else {
                StringCchPrintfW(szThisMachine, dwMachineName, L"%ws%ws", cszDoubleBackSlash, szMachineName);
            }
        }
        else {
            szThisMachine = NULL;
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                pMachine  = GetMachine((LPWSTR) szThisMachine, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
                if (pMachine == NULL) {
                    pdhStatus = GetLastError();
                    if (pdhStatus == ERROR_SUCCESS) pdhStatus = PDH_CSTATUS_NO_MACHINE;
                }
                else if (pMachine->dwStatus != ERROR_SUCCESS) {
                    pdhStatus = pMachine->dwStatus;
                    pMachine->dwRefCount --;
                    RELEASE_MUTEX(pMachine->hMutex);
                    pMachine = NULL;
                }
                else {
                    DWORD dwObject = GetObjectId(pMachine, (LPWSTR) szObjectName, NULL);
                    if (dwObject == (DWORD) -1) {
                        pMachine->dwRefCount --;
                        RELEASE_MUTEX(pMachine->hMutex);
                        if (pdhStatus == ERROR_SUCCESS) pdhStatus = PDH_CSTATUS_NO_OBJECT;
                        pMachine = NULL;
                    }
                }

                if (pMachine != NULL) {
                    if (pdhStatus == ERROR_SUCCESS) {
                        // get object pointer
                        pObjectDef = GetObjectDefByName(pMachine->pSystemPerfData,
                                                        pMachine->dwLastPerfString,
                                                        pMachine->szPerfStrings,
                                                        szObjectName);
                        if (pObjectDef != NULL) {
                            // default counter reference is an index into the list
                            // of counter definition entries so walk down list of
                            // counters defs to find the default one
                            if (pObjectDef->DefaultCounter > (LONG) pObjectDef->NumCounters) {
                                // Something wrong inside PERF_OBJ_DEF block. DefaultCounter
                                // should be in the range between 0 and NumCounters, but apparently
                                // it is not. Reset DefaultCounter to 0.
                                //
                                pObjectDef->DefaultCounter = 0;
                            }
                            if (pObjectDef->DefaultCounter < (LONG) pObjectDef->NumCounters) {
                                // then the default index should be this buffer
                                lDefault    = 0;
                                pCounterDef = FirstCounter(pObjectDef);
                                while (pCounterDef != NULL && (lDefault < pObjectDef->DefaultCounter)
                                                           && (lDefault < (LONG) pObjectDef->NumCounters)) {
                                    pCounterDef = NextCounter(pObjectDef, pCounterDef);
                                    lDefault ++;
                                }
                                lDefault = pCounterDef->CounterNameTitleIndex;
                                if ((lDefault > 0) && ((DWORD)lDefault < pMachine->dwLastPerfString)) {
                                    // then there should be a string in the table
                                    szDefault   = (LPWSTR)PdhiLookupPerfNameByIndex(pMachine, lDefault);
                                    if (szDefault != NULL) {
                                        dwStringLen = lstrlenW(szDefault) + 1;
                                        if (szDefaultCounterName != NULL && dwStringLen <= dwLocalBufferSize) {
                                            StringCchCopyW(szDefaultCounterName, dwLocalBufferSize, szDefault);
                                            pdhStatus = ERROR_SUCCESS;
                                        }
                                        else {
                                            pdhStatus = PDH_MORE_DATA;
                                        }
                                        dwLocalBufferSize = dwStringLen;
                                    }
                                    else {
                                        dwLocalBufferSize = 0;
                                        pdhStatus         = PDH_CSTATUS_NO_COUNTER;
                                    }
                                }
                                else {
                                    // string index is not valid
                                    dwLocalBufferSize = 0;
                                    pdhStatus         = PDH_CSTATUS_NO_COUNTER;
                                }
                            }
                            else {
                                // the counter entry is not in the buffer
                                dwLocalBufferSize = 0;
                                pdhStatus         = PDH_CSTATUS_NO_COUNTER;
                            }
                        }
                        else {
                            // unable to find object
                            dwLocalBufferSize = 0;
                            pdhStatus         = PDH_CSTATUS_NO_OBJECT;
                        }
                    }
                    pMachine->dwRefCount --;
                    RELEASE_MUTEX(pMachine->hMutex);
                } // else pass pdhStatus value to caller
                break;

            case DATA_SOURCE_WBEM:
                pdhStatus = PdhiGetDefaultWbemProperty(szThisMachine,
                                                       szObjectName,
                                                       szDefaultCounterName,
                                                       & dwLocalBufferSize,
                                                       TRUE);
                break;

            case DATA_SOURCE_LOGFILE:
                dwLocalBufferSize = 0;
                break;

            default:
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX (hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    G_FREE(szThisMachine);
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfCounterHW(
    IN  HLOG    hDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szObjectName,
    IN  LPWSTR  szDefaultCounterName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        dwDataSourceType = DataSourceTypeH(hDataSource);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfCounterW(
                        dwDataSourceType, szMachineName, szObjectName, szDefaultCounterName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfCounterW(
    IN  LPCWSTR szDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szObjectName,
    IN  LPWSTR  szDefaultCounterName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        if (szDataSource != NULL) {
            if (* szDataSource == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenW(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSourceType = DataSourceTypeW(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfCounterW(
                        dwDataSourceType, szMachineName, szObjectName, szDefaultCounterName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetDefaultPerfCounterA(
    DWORD   dwDataSource,
    LPCSTR  szMachineName,
    LPCSTR  szObjectName,
    LPSTR   szDefaultCounterName,
    LPDWORD pcchBufferSize
)
/*++
Routine Description:
    Obtains the default performance counter from the specified object on
        the specified machine.

Arguments:
    IN      DWORD  dwDataSource
    IN      LPCSTR szMachineName
                NULL indicates the local machine, othewise this is the
                name of the remote machine to query. If this machine is
                not known to the PDH DLL, then it will be connected.
    IN      LPCSTR szObjectName
                a pointer to the buffer that contains the name of the object
                on the machine to find the default counter for.
    IN      LPSTR  szDefaultCounterName
                pointer to the buffer that will receive the default counter
                name. This pointer can be NULL if the value of the DWORD
                referenced by bcchBufferSize is 0.
    IN      LPDWORD pcchBufferSize
                pointer to a DWORD containing the size of the buffer, in
                characters, referenced by the szDefaultObjectName argument.
                If the value of this DWORD is 0, then no data will be written
                to the szDefaultObjectNameBuffer, however the required
                buffer size will be returned in the DWORD referenced by
                this pointer.

Return Value:
    ERROR_SUCCESS if this function completes normally otherwise a PDH error.
    PDH_INVALID_ARGUMENT a required argument is not correct or reserved
        argument is not 0 or NULL.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a required temporary
        buffer could not be allocated.
    PDH_CSTATUS_NO_COUNTERNAME is returned when the name string for the
        default counter could not be found.
    PDH_CSTATUS_NO_MACHINE  is returned when the specified machine
        is offline or unavailable.
    PDH_CSTATUS_NO_OBJECT is returned when the specified object could
        not be found on the specified computer.
    PDH_CSTATUS_NO_COUNTER is returned when the default counter is not
        found in the data buffer.
--*/
{
    LPWSTR                   szWideObject      = NULL;
    LPWSTR                   szWideName        = NULL;
    PDH_STATUS               pdhStatus         = ERROR_SUCCESS;
    PPERF_MACHINE            pMachine          = NULL;
    PPERF_OBJECT_TYPE        pObjectDef        = NULL;
    PPERF_COUNTER_DEFINITION pCounterDef;
    LONG                     lDefault;
    DWORD                    dwStringLen;
    DWORD                    dwLocalBufferSize = 0;
    LPWSTR                   szDefault;

    if (szObjectName == NULL || pcchBufferSize == NULL) {
         pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        // test the access the arguments
        __try {
            if (szMachineName != NULL) {
                if (* szMachineName == '\0') {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else if (lstrlenA(szMachineName) > PDH_MAX_COUNTER_NAME) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            } // else NULL machine Name is OK

            if (pdhStatus == ERROR_SUCCESS) {
                if (szObjectName != NULL) {
                    if (* szObjectName == '\0') {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                    else if (lstrlenA(szObjectName) > PDH_MAX_COUNTER_NAME) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
                else {
                    // null objects are not allowed
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

            if (pdhStatus == ERROR_SUCCESS) {
                dwLocalBufferSize = * pcchBufferSize;
                if (dwLocalBufferSize > 0) {
                    if (szDefaultCounterName != NULL) {
                        // test both ends of the caller's buffer for
                        // write access
                        szDefaultCounterName[0]                    = '\0';
                        szDefaultCounterName[dwLocalBufferSize -1] = '\0';
                    }
                    else {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (szMachineName != NULL) {
            LPWSTR szTmpMachine = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szMachineName);

            if (szTmpMachine == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else if (* szMachineName == '\\') {
                szWideName = szTmpMachine;
            }
            else {
                szWideName = G_ALLOC((lstrlenW(szTmpMachine) + 3) * sizeof(WCHAR));
                if (szWideName == NULL) {
                    szWideName = szTmpMachine;
                }
                else {
                    StringCchPrintfW(szWideName,
                                     lstrlenW(szTmpMachine) + 3,
                                     L"%ws%ws",
                                     cszDoubleBackSlash,
                                     szTmpMachine);
                    G_FREE(szTmpMachine);
                }
            }
        }
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    }
    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            switch (dwDataSource) {
            case DATA_SOURCE_REGISTRY:
                szWideObject = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szObjectName);
                if (szWideObject == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    pMachine  = GetMachine(szWideName, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
                    if (pMachine == NULL) {
                        pdhStatus = GetLastError();
                        if (pdhStatus == ERROR_SUCCESS) pdhStatus = PDH_CSTATUS_NO_MACHINE;
                    }
                    else if (pMachine->dwStatus != ERROR_SUCCESS) {
                        pdhStatus = pMachine->dwStatus;
                        pMachine->dwRefCount --;
                        RELEASE_MUTEX(pMachine->hMutex);
                        pMachine = NULL;
                    }
                    else {
                        DWORD dwObject = GetObjectId(pMachine, szWideObject, NULL);
                        if (dwObject == (DWORD) -1) {
                            pMachine->dwRefCount --;
                            RELEASE_MUTEX(pMachine->hMutex);
                            if (pdhStatus == ERROR_SUCCESS) pdhStatus = PDH_CSTATUS_NO_OBJECT;
                            pMachine = NULL;
                        }
                    }
                }
                if (pdhStatus == ERROR_SUCCESS && pMachine != NULL) {
                    // get selected object
                    pObjectDef = GetObjectDefByName(pMachine->pSystemPerfData,
                                                    pMachine->dwLastPerfString,
                                                    pMachine->szPerfStrings,
                                                    szWideObject);
                    if (pObjectDef != NULL) {
                        // default counter reference is an index into the list
                        // of counter definition entries so walk down list of
                        // counters defs to find the default one
                        if (pObjectDef->DefaultCounter > (LONG) pObjectDef->NumCounters) {
                            // Something wrong inside PERF_OBJ_DEF block. DefaultCounter
                            // should be in the range between 0 and NumCounters, but apparently
                            // it is not. Reset DefaultCounter to 0.
                            //
                            pObjectDef->DefaultCounter = 0;
                        }
                        if (pObjectDef->DefaultCounter < (LONG) pObjectDef->NumCounters) {
                            // then the default index should be this buffer
                            lDefault    = 0;
                            pCounterDef = FirstCounter(pObjectDef);
                            while (pCounterDef != NULL && (lDefault < pObjectDef->DefaultCounter)
                                                       && (lDefault < (LONG) pObjectDef->NumCounters)) {
                                pCounterDef = NextCounter(pObjectDef, pCounterDef);
                                lDefault ++;
                            }
                            lDefault = pCounterDef->CounterNameTitleIndex;
                            if ((lDefault > 0) && ((DWORD) lDefault < pMachine->dwLastPerfString)) {
                                // then there should be a string in the table
                                szDefault   = (LPWSTR) PdhiLookupPerfNameByIndex(pMachine, lDefault);
                                if (szDefault != NULL) {
                                    dwStringLen = dwLocalBufferSize;
                                    pdhStatus   = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                                                           szDefault,
                                                                           szDefaultCounterName,
                                                                           & dwStringLen);
                                    dwLocalBufferSize = dwStringLen;
                                }
                                else {
                                    dwLocalBufferSize = 0;
                                    pdhStatus         = PDH_CSTATUS_NO_COUNTER;
                                }
                            }
                            else {
                                // string index is not valid
                                dwLocalBufferSize = 0;
                                pdhStatus         = PDH_CSTATUS_NO_COUNTER;
                            }
                        }
                        else {
                            // the counter entry is not in the buffer
                            dwLocalBufferSize = 0;
                            pdhStatus         = PDH_CSTATUS_NO_COUNTER;
                        }
                    }
                    else {
                        // unable to find object
                        dwLocalBufferSize = 0;
                        pdhStatus         = PDH_CSTATUS_NO_OBJECT;
                    }
                    pMachine->dwRefCount --;
                    RELEASE_MUTEX(pMachine->hMutex);
                }
                G_FREE(szWideObject);
                break;

            case DATA_SOURCE_WBEM:
            case DATA_SOURCE_LOGFILE:
                if (pdhStatus == ERROR_SUCCESS && dwDataSource == DATA_SOURCE_WBEM) {
                    szWideObject = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szObjectName);
                    if (szWideObject == NULL) {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        pdhStatus = PdhiGetDefaultWbemProperty(szWideName,
                                                               szWideObject,
                                                               (LPVOID) szDefaultCounterName,
                                                               & dwLocalBufferSize,
                                                               FALSE); // ANSI function
                        G_FREE(szWideObject);
                    }
                }
                else {
                    //log files don't support this (for now)
                    dwLocalBufferSize = 0;
                }
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        RELEASE_MUTEX(hPdhDataMutex);
    }

    if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        __try {
            * pcchBufferSize = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    G_FREE(szWideName);
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfCounterHA(
    IN  HLOG    hDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPCSTR  szObjectName,
    IN  LPSTR   szDefaultCounterName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        dwDataSourceType = DataSourceTypeH(hDataSource);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfCounterA(
                        dwDataSourceType, szMachineName, szObjectName, szDefaultCounterName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhGetDefaultPerfCounterA(
    IN  LPCSTR  szDataSource,
    IN  LPCSTR  szMachineName,
    IN  LPCSTR  szObjectName,
    IN  LPSTR   szDefaultCounterName,
    IN  LPDWORD pcchBufferSize
)
{
    DWORD      dwDataSourceType = 0;
    PDH_STATUS pdhStatus        = ERROR_SUCCESS;

    __try {
        if (szDataSource != NULL) {
            if (* szDataSource == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else if (lstrlenA(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSourceType = DataSourceTypeA(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetDefaultPerfCounterA(
                        dwDataSourceType, szMachineName, szObjectName, szDefaultCounterName, pcchBufferSize);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhBrowseCountersHW(
    IN  PPDH_BROWSE_DLG_CONFIG_HW  pBrowseDlgData
)
{
    PDHI_BROWSE_DLG_INFO  pInfo;
    LPWSTR                szResource;
    int                   nDlgReturn;
    DWORD                 dwReturn = ERROR_SUCCESS;

    pInfo.pWideStruct = (PPDH_BROWSE_DLG_CONFIG_W) pBrowseDlgData;
    pInfo.pAnsiStruct = NULL;

    if (pBrowseDlgData != NULL) {
        __try {
            // copy the data source since it wide characters already
            pInfo.hDataSource = pBrowseDlgData->hDataSource;

            szResource = MAKEINTRESOURCEW(pBrowseDlgData->bShowObjectBrowser
                       ? IDD_BROWSE_OBJECTS
                       : pBrowseDlgData->bSingleCounterPerDialog ? IDD_BROWSE_COUNTERS_SIM : IDD_BROWSE_COUNTERS_EXT);
            nDlgReturn = (int) DialogBoxParamW(ThisDLLHandle,
                                               szResource,
                                               pBrowseDlgData->hWndOwner,
                                               BrowseCounterDlgProc,
                                               (LPARAM) & pInfo);
            if (nDlgReturn == -1) {
                dwReturn = GetLastError();
            }
            else {
                dwReturn = (nDlgReturn == IDOK) ? ERROR_SUCCESS : PDH_DIALOG_CANCELLED;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        dwReturn = PDH_INVALID_ARGUMENT;
    }
    return dwReturn;
}

PDH_FUNCTION
PdhBrowseCountersHA(
    IN  PPDH_BROWSE_DLG_CONFIG_HA  pBrowseDlgData)
{
    PDHI_BROWSE_DLG_INFO  pInfo;
    LPWSTR                szResource;
    int                   nDlgReturn;
    DWORD                 dwReturn = ERROR_SUCCESS;

    pInfo.pAnsiStruct = (PPDH_BROWSE_DLG_CONFIG_A) pBrowseDlgData;
    pInfo.pWideStruct = NULL;

    if (pBrowseDlgData != NULL) {
        __try {
            // copy the data source since it wide characters already
            pInfo.hDataSource = pBrowseDlgData->hDataSource;

            szResource = MAKEINTRESOURCEW(pBrowseDlgData->bShowObjectBrowser
                       ? IDD_BROWSE_OBJECTS
                       : pBrowseDlgData->bSingleCounterPerDialog ? IDD_BROWSE_COUNTERS_SIM : IDD_BROWSE_COUNTERS_EXT);
            nDlgReturn = (int) DialogBoxParamW(ThisDLLHandle,
                                               szResource,
                                               pBrowseDlgData->hWndOwner,
                                               BrowseCounterDlgProc,
                                               (LPARAM) & pInfo);
            if (nDlgReturn == -1) {
                dwReturn = GetLastError();
            }
            else {
                dwReturn = (nDlgReturn == IDOK) ? ERROR_SUCCESS : PDH_DIALOG_CANCELLED;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        dwReturn = PDH_INVALID_ARGUMENT;
    }
    return dwReturn;
}

PDH_FUNCTION
PdhBrowseCountersW(
    IN  PPDH_BROWSE_DLG_CONFIG_W    pBrowseDlgData
)
{
    PDHI_BROWSE_DLG_INFO pInfo;
    LPWSTR               szResource;
    int                  nDlgReturn;
    DWORD                dwReturn = ERROR_SUCCESS;

    pInfo.pWideStruct = pBrowseDlgData;
    pInfo.pAnsiStruct = NULL;

    if (pBrowseDlgData != NULL) {
        __try {
            DWORD dwDataSource = DataSourceTypeW (pBrowseDlgData->szDataSource);
            if (dwDataSource == DATA_SOURCE_WBEM) {
                pInfo.hDataSource = H_WBEM_DATASOURCE;
            }
            else if (dwDataSource == DATA_SOURCE_LOGFILE) {
                DWORD dwLogType = 0;

                dwReturn = PdhOpenLogW(pBrowseDlgData->szDataSource,
                                       PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                       & dwLogType,
                                       NULL,
                                       0,
                                       NULL,
                                       & pInfo.hDataSource);
            }
            else {
                pInfo.hDataSource = H_REALTIME_DATASOURCE;
            }

            if (dwReturn == ERROR_SUCCESS) {
                szResource = MAKEINTRESOURCEW(pBrowseDlgData->bShowObjectBrowser
                           ? IDD_BROWSE_OBJECTS
                           : pBrowseDlgData->bSingleCounterPerDialog ? IDD_BROWSE_COUNTERS_SIM
                                                                     : IDD_BROWSE_COUNTERS_EXT);
                nDlgReturn = (int) DialogBoxParamW(ThisDLLHandle,
                                                   szResource,
                                                   pBrowseDlgData->hWndOwner,
                                                   BrowseCounterDlgProc,
                                                   (LPARAM) & pInfo);

                if (nDlgReturn == -1) {
                    dwReturn = GetLastError();
                }
                else {
                    dwReturn = nDlgReturn == IDOK ? ERROR_SUCCESS : PDH_DIALOG_CANCELLED;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        dwReturn = PDH_INVALID_ARGUMENT;
    }
    return dwReturn;
}

PDH_FUNCTION
PdhBrowseCountersA(
    IN  PPDH_BROWSE_DLG_CONFIG_A    pBrowseDlgData
)
{
    PDHI_BROWSE_DLG_INFO pInfo;
    LPWSTR               szResource;
    int                  nDlgReturn;
    DWORD                dwReturn = ERROR_SUCCESS;

    pInfo.pAnsiStruct = pBrowseDlgData;
    pInfo.pWideStruct = NULL;

    if (pBrowseDlgData != NULL) {
        __try {
            DWORD dwDataSource = DataSourceTypeA(pBrowseDlgData->szDataSource);
            if (dwDataSource == DATA_SOURCE_WBEM) {
                pInfo.hDataSource = H_WBEM_DATASOURCE;
            }
            else if (dwDataSource == DATA_SOURCE_LOGFILE) {
                DWORD dwLogType = 0;

                dwReturn = PdhOpenLogA(pBrowseDlgData->szDataSource,
                                       PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                       & dwLogType,
                                       NULL,
                                       0,
                                       NULL,
                                       & pInfo.hDataSource);
            }
            else {
                pInfo.hDataSource = H_REALTIME_DATASOURCE;
            }

            if (dwReturn == ERROR_SUCCESS) {
                szResource = MAKEINTRESOURCEW(pBrowseDlgData->bShowObjectBrowser
                           ? IDD_BROWSE_OBJECTS
                           : pBrowseDlgData->bSingleCounterPerDialog ? IDD_BROWSE_COUNTERS_SIM
                                                                     :  IDD_BROWSE_COUNTERS_EXT);
                nDlgReturn = (int) DialogBoxParamW(ThisDLLHandle,
                                                   szResource,
                                                   pBrowseDlgData->hWndOwner,
                                                   BrowseCounterDlgProc,
                                                   (LPARAM) & pInfo);
                if (nDlgReturn == -1) {
                    dwReturn = GetLastError();
                }
                else {
                    dwReturn = nDlgReturn == IDOK ? ERROR_SUCCESS : PDH_DIALOG_CANCELLED;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        dwReturn = PDH_INVALID_ARGUMENT;
    }
    return dwReturn;
}

LPWSTR
PdhiGetExplainText(
    LPCWSTR  szMachineName,
    LPCWSTR  szObjectName,
    LPCWSTR  szCounterName
)
{
    PPERF_MACHINE            pMachine;
    PPERF_OBJECT_TYPE        pObjectDef;
    PPERF_COUNTER_DEFINITION pCounterDef;
    LPWSTR                   szReturnString = NULL;

    pMachine  = GetMachine((LPWSTR) szMachineName, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
    if (pMachine != NULL) {
        DWORD dwObject = (pMachine->dwStatus == ERROR_SUCCESS)
                       ? GetObjectId(pMachine, (LPWSTR) szObjectName, NULL) : (DWORD) -1;
        if (dwObject == (DWORD) -1) {
            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
            pMachine = NULL;
        }
    }
    if (pMachine != NULL) {
        // make sure the machine connection is valid
        if (pMachine->dwStatus == ERROR_SUCCESS) {
            pObjectDef = GetObjectDefByName(pMachine->pSystemPerfData,
                                            pMachine->dwLastPerfString,
                                            pMachine->szPerfStrings,
                                            szObjectName);
            if (pObjectDef != NULL) {
                if (szCounterName != NULL) {
                    pCounterDef = GetCounterDefByName(pObjectDef,
                                                      pMachine->dwLastPerfString,
                                                      pMachine->szPerfStrings,
                                                      (LPWSTR) szCounterName);
                    if ((pCounterDef != NULL) && 
                                    (pCounterDef->CounterHelpTitleIndex <= pMachine->dwLastPerfString)) {
                        // return string from array
                        szReturnString = pMachine->szPerfStrings[pCounterDef->CounterHelpTitleIndex];
                    }
                }
                else if (pObjectDef->ObjectHelpTitleIndex <= pMachine->dwLastPerfString) {
                    szReturnString = pMachine->szPerfStrings[pObjectDef->ObjectHelpTitleIndex];
                }
            }
        }
        pMachine->dwRefCount --;
        RELEASE_MUTEX(pMachine->hMutex);
    }
    return szReturnString;
}

BOOL
IsWbemDataSource(
    LPCWSTR  szDataSource
)
{
    BOOL bReturn = FALSE;
    __try {
        if (DataSourceTypeW(szDataSource) == DATA_SOURCE_WBEM)
            bReturn = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }
    return bReturn;
}
