//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       Globals.h
//
//--------------------------------------------------------------------------
#if !defined(GLOBALS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_)
#define GLOBALS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

//
// Speculating as to what the _VC_VER value should be... should this be _MSC_VER
//
#ifndef _VC_VER_INC
#define _VC_VER_INC
#ifndef _VC_VER
#define _VC_VER 620
#endif
#endif

//#include "DelayLoad.h"
//#include "ProgramOptions.h"
//#include "SymbolVerification.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif
#include <WINDOWS.H>

// Used for transparent ANSI/UNICODE suppport...
#include <TCHAR.H>

// Used for cool macros (mostly for UNREFERENCED_PARM(x))
#include <DEBNOT.H>

// Forward declarations
class CDelayLoad;
class CProgramOptions;
class CSymbolVerification;

// Implement a few objects as globals to greatly simplify access to the objects...
// When we multi-thread ourselves with different user-contexts... we'll likely
// need to use _cdecl (thread) to provide per-user/thread global contexts...

enum CollectionTypes { Processes, Process, Modules, KernelModeDrivers };

struct CollectionStruct
{
	LPTSTR tszLocalContext;		// When collected locally, what should we say...
	LPTSTR tszCSVContext;		// When collected from a CSV file, what should we say...
	LPTSTR tszCSVLabel;			// CSV file label, what should we look for...
	LPTSTR tszCSVColumnHeaders;	// CSV file headers...
}; 

extern CDelayLoad * g_lpDelayLoad;
extern CProgramOptions * g_lpProgramOptions;
extern CSymbolVerification * g_lpSymbolVerification;

// Global String Structure
extern CollectionStruct g_tszCollectionArray[];

#endif
