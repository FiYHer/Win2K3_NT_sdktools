#include "muisetup.h"
#include <shlwapi.h>
#include <shellapi.h>
#define STRSAFE_LIB
#include <strsafe.h>

#define README_FILENAME TEXT("README.TXT")
#define EULA_FILENAME   TEXT("EULA.TXT")

BOOL g_bLicenseAccepted;


INT_PTR 
CALLBACK
WelcomeDialogProc(HWND   hWndDlg, UINT   uMsg, WPARAM wParam, LPARAM lParam)
{ 
    HANDLE hFile=INVALID_HANDLE_VALUE;
    DWORD  dwFileSize;
    DWORD  dwActual;
    LPVOID pFileBuffer; 
    TCHAR   szEulaPath[MAX_PATH];
    BOOL bResult = FALSE;
    HRESULT hresult;

    switch ( uMsg ) {

        case WM_INITDIALOG:
            
            //
            // Load EULA file from the path where MUISETUP was lunched
            //
            GetModuleFileName( NULL, szEulaPath, ARRAYSIZE( szEulaPath ));

            //*STRSAFE*             lstrcpy(StrRChrI(szEulaPath, NULL, TEXT('\\'))+1, EULA_FILENAME);
            hresult = StringCchCopy(StrRChrI(szEulaPath, NULL, TEXT('\\'))+1, ARRAYSIZE(szEulaPath) - lstrlen(szEulaPath), EULA_FILENAME);            
            if (!SUCCEEDED(hresult))
            {
               return 1;
            }

            hFile = CreateFile(
                        szEulaPath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

            if ( hFile != INVALID_HANDLE_VALUE ) {

                dwFileSize = GetFileSize( hFile, NULL );

                if ( dwFileSize != -1 ) {

                    pFileBuffer = LocalAlloc(LPTR, dwFileSize + 1 );

                    if ( pFileBuffer ) {

                        if ( ReadFile( hFile, pFileBuffer, dwFileSize, &dwActual, NULL )) {

                            //
                            // Make sure to NULL terminate the string
                            //
                            *((PCHAR)((PCHAR)pFileBuffer + dwFileSize)) = 0x00;

                            //
                            // Use ANSI text
                            //
                            SetDlgItemTextA( hWndDlg, IDC_EDIT_LICENSE, (LPCSTR)pFileBuffer );
                            bResult = TRUE;
                            }                        
                        LocalFree( pFileBuffer );
                        }
                    }
                }
            SetFocus( GetDlgItem( hWndDlg, IDC_CHECK_LICENSE ));
            if ( hFile != INVALID_HANDLE_VALUE )
            {
                CloseHandle(hFile);
            }
            if (! bResult)
            {
               EndDialog( hWndDlg, ERROR_CANCELLED );
               return 1;
            }
            else
            {            
               return 0;
            }

        case WM_COMMAND:

            switch ( LOWORD( wParam )) 
            {
                case IDOK:
                    g_bLicenseAccepted = ( IsDlgButtonChecked( hWndDlg, IDC_CHECK_LICENSE ) == BST_CHECKED );
                    EndDialog( hWndDlg, 0 );
                    return 1;

                case IDCANCEL:
                    EndDialog( hWndDlg, ERROR_CANCELLED );
                    return 1;

                case IDC_README:
                    {
                        // invoke notepad.exe open readme.txt
                        TCHAR szReadMePath[MAX_PATH];
                        SHELLEXECUTEINFO ExecInfo = {0};                        

                        GetModuleFileName(NULL, szReadMePath, sizeof(szReadMePath)/sizeof(TCHAR));
                        //*STRSAFE*                         lstrcpy(StrRChrI(szReadMePath, NULL, TEXT('\\'))+1, README_FILENAME);                       
                        hresult = StringCchCopy(StrRChrI(szReadMePath, NULL, TEXT('\\'))+1, ARRAYSIZE(szReadMePath) - lstrlen(szReadMePath), README_FILENAME);            
                        if (!SUCCEEDED(hresult))
                        {
                           return 1;
                        }
                        
                        ExecInfo.lpParameters    = szReadMePath;
                        ExecInfo.lpFile          = TEXT("NOTEPAD.EXE");
                        ExecInfo.nShow           = SW_SHOWNORMAL;
                        ExecInfo.cbSize          = sizeof(SHELLEXECUTEINFO);                 
                        ShellExecuteEx(&ExecInfo);
                    }
                    return 1;

                case IDC_CHECK_LICENSE:
                    EnableWindow( GetDlgItem( hWndDlg, IDOK ), IsDlgButtonChecked( hWndDlg, IDC_CHECK_LICENSE ) == BST_CHECKED );
                    return 1;

            }
            break;

        case WM_CLOSE:
            EndDialog( hWndDlg, ERROR_CANCELLED );
            return 1;

        }

    return 0;
}


BOOL
WelcomeDialog(HWND hWndParent)
{
    INT_PTR Status;

    Status = DialogBox(
                 NULL,
                 MAKEINTRESOURCE( IDD_WELCOME ),
                 hWndParent,
                 WelcomeDialogProc
                 );

    return (( Status == ERROR_SUCCESS ) && ( g_bLicenseAccepted ));
}
