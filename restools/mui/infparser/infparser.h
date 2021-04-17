///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    infparser.h
//
//  Abstract:
//
//    This file contains global definition for the infparser.exe utility.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _INFPARSER_H_
#define _INFPARSER_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <setupapi.h>
#include <rpc.h>
#include <tchar.h>
#include <strsafe.h>

#include "Component.h"
#include "ComponentList.h"
#include "FileLayout.h"
#include "FileLayoutExceptionList.h"
#include "File.h"
#include "FileList.h"
#include "Uuid.h"




///////////////////////////////////////////////////////////////////////////////
//
//  Enumeration
//
///////////////////////////////////////////////////////////////////////////////
enum flavorList
{
    FLV_UNDEFINED    	= 0x0000,
    FLV_CORE         	= 0x0001,
    FLV_PERSONAL     	= 0x0002,
    FLV_PROFESSIONAL 	= 0x0004,
    FLV_SERVER       	= 0x0008,
    FLV_ADVSERVER    	= 0x0010,
    FLV_DATACENTER   	= 0x0040,
    FLV_WEBBLADE   		= 0x0080,    
    FLV_SMALLBUSINESS   = 0x0100        
};

enum argumentToken
{
    ARG_UNDEFINED 	= 0x0000,
    ARG_BINARY    	= 0x0001,
    ARG_LANG      	= 0x0002,
    ARG_FLAVOR    	= 0x0004,
    ARG_DIR       	= 0x0008,
    ARG_OUT       	= 0x0010,
    ARG_SILENT    	= 0x0020,
    ARG_LOCBIN		= 0x0040,
    ARG_CDLAYOUT 	= 0x0080,
    ARG_CORELOCBIN	= 0x0100
};

enum binaryType
{
    BIN_UNDEFINED = 0x0000,
    BIN_32        = 0x0001,
    BIN_64        = 0x0002,
};


///////////////////////////////////////////////////////////////////////////////
//
//  Type definitions
//
///////////////////////////////////////////////////////////////////////////////
typedef struct LayoutInf
{
    CHAR strLayoutInfPaths[MAX_PATH];
    FileLayoutExceptionList flLayoutCore;
    FileLayoutExceptionList flLayoutX86;
    FileLayoutExceptionList flLayoutIA64;    
    CHAR strSkuName[4];
} LAYOUTINF;

///////////////////////////////////////////////////////////////////////////////
//
//  Global
//
///////////////////////////////////////////////////////////////////////////////
extern BOOL bSilence;
extern DWORD dwComponentCounter;
extern DWORD dwDirectoryCounter;
extern WORD  gBuildNumber;

#endif //_INFPARSER_H_
