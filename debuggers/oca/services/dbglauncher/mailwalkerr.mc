;/*
;   Copyright 2000 Microsoft Corporation
;
;   Definitions for MailWalkerSvc.
;
;   This file is generated by the MC tool from the netevent.mc file.
;
;*/
;
;#pragma once
;

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=1000 
Severity=Error 
SymbolicName=EVENT_MESSAGE
Language=English
%1
.

MessageId=1001 
Severity=Error 
SymbolicName=EVENT_ERROR
Language=English
%1
.

