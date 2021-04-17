#include the project.mk file from the dir above also...
# define _PROJECT_ for the debugger people that don't enlist in sdktools

!IF EXIST ($(_PROJECT_MK_PATH)\..\project.mk)
!INCLUDE $(_PROJECT_MK_PATH)\..\project.mk
!ELSE
_PROJECT_=SdkTools
MSC_WARNING_LEVEL=/W3 /WX
BUFFER_OVERFLOW_CHECKS=1
!ENDIF

!IF EXIST ($(_PROJECT_MK_PATH)\product.inc)
!INCLUDE $(_PROJECT_MK_PATH)\product.inc
!ENDIF

DBG_ROOT=$(_PROJECT_MK_PATH)
DEBUGGER_LIBS=$(DBG_ROOT)\dbglibs
IA64_DIS_INC=$(DBG_ROOT)\ia64tools\include

BINPLACE_PLACEFILE=$(DBG_ROOT)\placefil.txt
COFFBASE_TXT_FILE=$(DBG_ROOT)\coffbase.txt

DBG_BIN=dbg\files\bin
DBG_EXTS_NT4FRE=$(DBG_BIN)\nt4fre
DBG_EXTS_NT4CHK=$(DBG_BIN)\nt4chk
DBG_EXTS_W2KFRE=$(DBG_BIN)\w2kfre
DBG_EXTS_W2KCHK=$(DBG_BIN)\w2kchk
