//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       utils.js
//
//  Contents:   Utility file for use by all script modules
//
//----------------------------------------------------------------------------

Include('sendmail.js');
// ---------------------------------------------------------------------------
//
// Global Variables
//
// ---------------------------------------------------------------------------

var g_aDebugOnce         = new Array();

// The following should be "true" in the main branch and "false" in the ntdrop
// branch. Do not integrate this change from one side to the other.
var g_fUseMTScriptAssert = false;

var g_IgnoreAllAsserts   = false;
var g_fAssertOnDispatchException = true;
var g_SuppressErrorDialogs = true;
var MAX_MSGS_FILES = 10;
var strUtilsVersion = /* $DROPVERSION: */ "V(########) F(!!!!!!!!!!!!!!)" /* $ */;
var g_reBuildNum   = new RegExp("V\\(([#0-9. ]+)\\)");
//var g_reBuildFile = new RegExp("F\\(([!A-Za-z_. ]+)\\)");
var g_aPublicBuildNum;

initUneval();
unevalInitialized = true;

// ---------------------------------------------------------------------------
//
// String Prototype Additions
//
// ---------------------------------------------------------------------------

function String_TrimTrailingWS()
{
    var end = this.length - 1;

    while (this.charAt(end) == ' ')
    {
        end--;
    }

    return this.slice(0, end+1);
}

function String_IsEqualNoCase(str)
{
    return this.toLowerCase() == str.toLowerCase();
}

/*function String_TrimLeadingWS()
{
    var end = 0;

    while (this.charAt(end) == ' ')
    {
        ++end;
    }

    return this.slice(end);
}
*/

// String_SplitFileName
// Return an array of 3 elements, path,filename,extension
// [0] == "C:\path\"
// [1] == "filename"
// [2] == ".ext"
//
function String_SplitFileName()
{
    var nDot   = this.lastIndexOf('.');
    var nSlash = this.lastIndexOf('\\');
    var nColon = this.lastIndexOf(':');

    if (nDot >= 0 && nDot > nSlash && nDot > nColon)
    {
        return [this.slice(0, nSlash + 1), this.slice(nSlash + 1, nDot), this.slice(nDot)];
    }
    // We get here if the file had no extension
    if (nSlash >= 2) // do not slice the UNC double \ at the start of a filename.
    {
        return [this.slice(0, nSlash + 1), this.slice(nSlash + 1, nDot), ''];
    }
    return ['', this, ''];
}

function String_RemoveExtension()
{
    var nDot   = this.lastIndexOf('.');
    var nSlash = this.lastIndexOf('\\');
    var nColon = this.lastIndexOf(':');


    if (nDot >= 0 && nDot > nSlash && nDot > nColon)
    {
        return this.slice(0, nDot);
    }
    return this;
}

//+---------------------------------------------------------------------------
//
//  Function:   Substitute
//
//  Synopsis:   Given the replacement information in aStrMap, replace any
//              'name' occurrances with the appropriate 'value'.
//
//  Arguments:  [aStrMap] -- Array of regular expressions and replacement text
//
//  Returns:    The string with any appropriate substitutions made.
//
//  Notes:      aStrMap is the array generated by InitStringMaps()
//
//----------------------------------------------------------------------------

function String_Substitute(aStrMap)
{
    var i;
    var text = this;

    for (i = 0; i < aStrMap.length; i++)
    {
        text = text.replace(aStrMap[i].re, aStrMap[i].newtext);
    }

    return text;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitStringMaps
//
//  Synopsis:   Creates an array with regular expression objects that we can
//              use to do substitutions of strings.
//
//  Arguments:  [aStrMap]    -- Array of strings of form "name=value"
//              [aNewStrMap] -- New array containing regular expressions and
//                              replacement text. Used by String_Substitute().
//
//----------------------------------------------------------------------------

function InitStringMaps(aStrMap, aNewStrMap)
{
    var i;
    var index;

    for (i = 0; i < aStrMap.length; i++)
    {
        aNewStrMap[i] = new Object();

        index = aStrMap[i].indexOf('=');

        aNewStrMap[i].re = new RegExp(aStrMap[i].slice(0, index), 'gi');
        aNewStrMap[i].re.compile(aStrMap[i].slice(0, index), 'gi');

        aNewStrMap[i].newtext = aStrMap[i].slice(index+1);
    }
}

// PadDigits(n, cDigits)
// Return a string representation of the given
// number. Leading zeros are added to make the
// string cDigits long.
function PadDigits(n, cDigits)
{
    var strDigits = '';
    var i;
    var strNumber = n.toString();

    for(i = 0; i < cDigits - strNumber.length; ++i)
        strDigits += '0';

    return strDigits + n;
}

// DateToString(dDate)
// Format a date like this: "YYYY/MM/DD:HH:MM:SS"
// this format does not change with locale, and is
// lends itself to sorting.
function Date_DateToSDString(fDateOnly)
{
    var str =
        PadDigits(this.getFullYear(), 4) + "/" +
        PadDigits(this.getMonth() + 1, 2) + "/" +
        PadDigits(this.getDate(), 2);

    if (!fDateOnly)
    {
        str += ":" +
           PadDigits(this.getHours(), 2) + ":" +
           PadDigits(this.getMinutes(), 2) + ":" +
           PadDigits(this.getSeconds(), 2);
    }

    return str;
}

function Error_ToString()
{
    var i;
    var str = 'Exception(';

    /*
        Only some error messages get filled in for "ex".
        Specifically the text for disk full never seems
        to get set by functions such as CreateTextFile().


     */
    if (this.number != null && this.description == "")
    {
        switch(this.number)
        {
            case -2147024784:
                this.description = "There is not enough space on the disk.";
                break;
            case -2147024894:
                this.description = "The system cannot find the file specified.";
                break;
            case -2147023585:
                this.description = "There are currently no logon servers available to service the logon request.";
                break;
            case -2147023170:
                this.description = "The remote procedure call failed.";
                break;
            case -2147024837:
                this.description = "An unexpected network error occurred.";
                break;
            case -2147024893:
                this.description = "The system cannot find the path specified.";
                break;
            case -2147024890:
                this.description = "The handle is invalid.";
                break;
            default:
                this.description = "Error text not set for (" + this.number + ")";
                break;
        }
    }
    for(i in this)
    {
        if (!this.__isPublicMember(i))
            continue;

        str += i + ": " + this[i] + " ";
    }
    return str + ")";
}

String.prototype.IsEqualNoCase   = String_IsEqualNoCase;
String.prototype.TrimTrailingWS  = String_TrimTrailingWS;
//String.prototype.TrimLeadingWS = String_TrimLeadingWS;
String.prototype.RemoveExtension = String_RemoveExtension;
String.prototype.SplitFileName   = String_SplitFileName;
String.prototype.Substitute      = String_Substitute;
Date.prototype.DateToSDString    = Date_DateToSDString;
Error.prototype.toString         = Error_ToString;

// ---------------------------------------------------------------------------
//
//  Miscellaneous Stuff
//
// ---------------------------------------------------------------------------

function CommonOnScriptError(strThread, strFile, nLine, nChar, strText, sCode, strSource, strDescription)
{
    var dlg = new Dialog();
    dlg.fShowDialog   = true;
    dlg.strTitle      = "Script Error in " + strThread + ".js";
    dlg.strMessage    = "File: " + strFile +
        "\nLine: " + nLine +
        "\nChar: " + nChar +
        "\nText: " + strText +
        "\nsCode: " + sCode +
        "\nSource: " + strSource +
        "\nDescription: " + strDescription +
        "\nThread:" + strThread;
    dlg.aBtnText[0]   = "OK";

    LogMsg("On error!\n" + dlg.strMessage);

    CreateErrorDialog(dlg);

    if (g_SuppressErrorDialogs)
        return 1; // Enter the script debugger if present

    return 0; // display the error
}

function CommonVersionCheck(strLocalVersion)
{
    var aLocal;
    if (g_aPublicBuildNum == null)
    {
        g_aPublicBuildNum = g_reBuildNum.exec(PublicData.strDataVersion);
        if (g_aPublicBuildNum) // First time around, lets check the version of utils.js too.
        {
            if (!CommonVersionCheck(strUtilsVersion))
                return false;
        }
    }

    aLocal = g_reBuildNum.exec(strLocalVersion);
    if (!aLocal || !g_aPublicBuildNum)
    {
        SimpleErrorDialog("Invalid version format: ", strLocalVersion + "," + PublicData.strDataVersion, false);
        return false;
    }
    if (aLocal[1] != g_aPublicBuildNum[1])
    {
        SimpleErrorDialog("Script Version mismatch", strLocalVersion + "," + PublicData.strDataVersion, false);
        return false;
    }
    return true;
}

/*
    CreateErrorDialog()

    If we are runnning standalone:
        1: Send email, throttled by MAIL_RESEND_INTERVAL
        2: Update PublicData.objDialog, throttled by PublicData.objDialog.fShowDialog

    If we are running distributed:
        1: If the error is EMail only, throttle it by MAIL_RESEND_INTERVAL
        2: Update PublicData.objDialog, throttled by PublicData.objDialog.fShowDialog
        3: Notify BuildManager via 'ScriptError' notification.
 */
function CreateErrorDialog(objDialog)
{
    var fEMailSent = false;
    var fSendMail = false;
    var fUpdateDialog = false;

    if (PublicData.objDialog)
        objDialog.cDialogIndex = PublicData.objDialog.cDialogIndex;

    objDialog.cDialogIndex++;

    TakeThreadLock('OnTaskError');
    var curDate  = new Date().getTime();
    if ((curDate - PrivateData.dateErrorMailSent) > MAIL_RESEND_INTERVAL)
    {
        PrivateData.dateErrorMailSent = curDate;
        fSendMail = true;
    }
    ReleaseThreadLock('OnTaskError');

    if (fSendMail)
    {
        if (PrivateData.fIsStandalone)
        {
            if (SendErrorMail(objDialog.strTitle, objDialog.strMessage))
                fEMailSent = true;
            else
            {
                // If EMail failed, then create a dialog instead.
                objDialog.fEMailOnly = false;
            }
        }
    }

    if (!fEMailSent)
        LogMsg("On error NOT EMAIL MESSAGE: " + objDialog.strTitle + ", " + objDialog.strMessage);

    if (fSendMail || !objDialog.fEMailOnly)
    {
        TakeThreadLock('Dialog');
        try
        {
            if (!PublicData.objDialog || PublicData.objDialog.fShowDialog == false)
            {
                PublicData.objDialog.strTitle     = objDialog.strTitle;
                PublicData.objDialog.strMessage   = objDialog.strMessage;
                PublicData.objDialog.aBtnText[0]  = objDialog.aBtnText[0];
                PublicData.objDialog.cDialogIndex = objDialog.cDialogIndex;
                PublicData.objDialog.fShowDialog  = objDialog.fShowDialog;
                PublicData.objDialog.fEMailOnly   = objDialog.fEMailOnly;
                fUpdateDialog = true;
            }
        }
        catch(ex)
        {
        }
        ReleaseThreadLock('Dialog');
        if (!PrivateData.fIsStandalone && fUpdateDialog)
        { // If distributed build, notify our parent
            if (PrivateData.objUtil != null && PrivateData.objUtil.fnUneval != null)
            {
                NotifyScript('ScriptError', PrivateData.objUtil.fnUneval(objDialog));
                // At this point or soon thereafter, "slaveproxy" should exec the 'dialog' command
                // to clear fShowDialog.
            }
        }
    }
}

/*
    CreateFileOpenErrorDialog()

    Build a simple file error dialog and push it to the UI.

    Argument:
        strMsg:      Supply a single word description of the failed operation (eg: "open", "copy")
        strFilename: The filename in question
        ex:          The exception object created when the error occured.
 */
function CreateFileOpenErrorDialog(strMsg, strFilename, ex)
{
    SimpleErrorDialog(strMsg, strFilename + "\n" + ex, false);
}

/*
    SimpleErrorDialog()

    Easy way to create and set the error dialog - just
    pass in a title and a message. Set fEMailOnly
    to just email the message.
 */
function SimpleErrorDialog(strTitle, strMsg, fEMailOnly)
{
    var dlg = new Dialog();
    dlg.fShowDialog   = true;
    dlg.strTitle      = strTitle;
    dlg.strMessage    = strMsg;
    dlg.aBtnText[0]   = "OK";
    dlg.fEMailOnly    = fEMailOnly;
    CreateErrorDialog(dlg);
}

/*
    LogOpenTextFile

    This is a simple wrapper for OpenTextFile().
    The only difference is that this function automatically
    handles error reporting - putting the error message in the log
    file and in the error dialog.
 */
function LogOpenTextFile(fsobj, filename, iomode, create, format)
{
    try
    {
        return fsobj.OpenTextFile(filename, iomode, create, format);
    }
    catch(ex)
    {
        LogMsg("Exception occurred calling OpenTextFile(" + [filename, iomode, create, format].toString() + ")" + ex);
        CreateFileOpenErrorDialog("An error occurred opening a file", "File: " + filename, ex);
        throw ex;
    }
}

/*
    LogCreateTextFile

    This is a simple wrapper for CreateTextFile().
    The only difference is that this function automatically
    handles error reporting - putting the error message in the log
    file and in the error dialog.

    This function is mainly used with UNC paths.
    Apparently, file handles opened with UNC paths can
    go stale sometimes (even when the UNC refers to the
    local machine).

    Therefore, I've created a simple wrapper for the TextStream object
    to handle this problem by attempting to reopen the file if
    we get an error writing to it.

    If we continue to not be able to write to the file, generate an error
    message.
 */
function LogCreateTextFile(fsobj, filename, overwrite, unicode)
{
    try
    {
        var obj   = new Object;
        obj.hFile = fsobj.CreateTextFile(filename, overwrite, unicode);

        obj.strFileName = filename;
        obj.fFailed     = false;
        obj.fsobj       = fsobj;
        obj.WriteLine   = CreateTextFile_WriteLine;
        obj.Close       = CreateTextFile_Close;
    }
    catch(ex)
    {
        LogMsg("Exception occurred calling CreateTextFile(" + [filename, overwrite, unicode].toString() + ")" + ex);
        CreateFileOpenErrorDialog("An error occurred creating a file", "File: " + filename, ex);
        throw ex;
    }
    return obj;
}

function CreateTextFile_WriteLine(msg)
{
    var n;
    for(n = 0 ; n < 3; ++n)
    {
        try
        {
            if (this.hFile == null)
            {
                this.hFile = this.fsobj.OpenTextFile(this.strFileName, 8); // 8==For append
                LogMsg("Reopened TextFile '" + this.strFileName + "'");
                this.fFailed = false;
            }
            this.hFile.WriteLine(msg);
            return;
        }
        catch(ex)
        {
            if (!this.fFailed)
            {
                LogMsg("Error During WriteLine on file '" + this.strFileName + "', " + ex);
                LogMsg("TRY Reopen TextFile '" + this.strFileName + "'");
                Sleep(250);
            }
            this.hFile = null;
        }
    }
    if (!this.fFailed)
    {
        LogMsg("Failed to reopen " + this.strFileName);
        CreateFileOpenErrorDialog("An error occurred reopening a file", "File: " + this.strFileName, ex);
        this.fFailed = true;
    }
}

function CreateTextFile_Close()
{
    try
    {
        this.hFile.Close();
    }
    catch(ex)
    {
        LogMsg("Error During Close() on file '" + this.strFileName + "', " + ex);
    }
}

function EnsureArray(obj, name)
{
    if (obj[name])
    {
        if (obj[name].length == null || typeof(obj[name]) != 'object')
        {
            var oldobj = obj[name];

            obj[name] = new Array();

            obj[name][0] = oldobj;
        }
    }
    else
    {
        obj[name] = new Array();
    }
}

function ThrowError(msg, detail)
{
    var ex = new Error(-1, msg);
    ex.detail = detail;
    throw ex;

}

function GetCallerName(cIgnoreCaller)
{
    var tokens;
    if (cIgnoreCaller == null)
        cIgnoreCaller = 0;

    ++cIgnoreCaller;
    var caller = GetCallerName.caller;
    while (caller != null && cIgnoreCaller)
    {
        caller = caller.caller;
        --cIgnoreCaller;
    }
    if (caller != null)
    {
        tokens = caller.toString().split(/ |\t|\)|,/);
        if (tokens.length > 1 && tokens[0] == "function")
        {
            return tokens[1] + ")";
        }
    }
    return "<undefined>";
}

function DebugOnce(id)
{
    caller = GetCallerName();
    if (g_aDebugOnce[id + caller] == null)
    {
        g_aDebugOnce[id + caller] = 1;
        debugger;
    }
}

function JAssert(fSuccess, msg)
{
    var caller;
    var i;
    if (!fSuccess)
    {
        if (msg == null)
            msg = '';

        caller = GetCallerName();
        LogMsg("ASSERTION FAILED :(" + caller + ") " + msg);
        if (!g_IgnoreAllAsserts)
            debugger;

        if (g_fUseMTScriptAssert)
        {
            if (arguments.length > 1)
                ASSERT(false, "JScript: " + msg);
            else
                ASSERT(false, "JScript Assertion");
        }
    }
}

function LogObject(strMsg, obj)
{
    LogMsg("DUMP OBJECT " + strMsg);
    var i;

    for(i in obj)
    {
        if (!obj.__isPublicMember(i))
            continue;
        LogMsg("\tobj[" + i + "] = '" + obj[i] + "'");
    }
}

function GetPrivPub()
{
    var obj = new Object;

    var strData = PrivateData.objUtil.fnUneval(PrivateData);
    obj.PrivateData = MyEval(strData);

    strData = PrivateData.objUtil.fnUneval(PublicData);
    obj.PublicData = MyEval(strData);

    return obj;
}

//  Record MTScript message to a log file.
//  If the log file is not open, create a new one.
//  This function should be called eary during the intialization
//  of mtscript.js.
function LogMsg(strMsg, cIgnoreCaller)
{
    try
    {
        var fileLog;
        var d = (new Date()).DateToSDString();
        var strFormattedMsg = LocalMachine + "\t" + d + "\t" + GetCallerName(cIgnoreCaller) + "\t" + strMsg;
        if (PrivateData.objUtil.fnCreateNumberedFile == null)
            OUTPUTDEBUGSTRING("Utilthrd Initialize() has not been called yet");
        else if (PrivateData.fEnableLogging)
        {
            if (!PrivateData.fileLog)
            {
                // First check to see if the file has been created yet.
                // The grab a critical section.
                // Checking "fileLog" first makes the usual
                // case (the file is already open) quicker.
                if (PrivateData.fInLogMsg)
                {
                    OUTPUTDEBUGSTRING(d.slice(11) + "\t" + GetCallerName(cIgnoreCaller) + "\t" + strMsg + "\tLOGMSG RECURSION!");
                    return;
                }
                TakeThreadLock("PrivateData.fileLog");
                if (!PrivateData.fileLog && PrivateData.fEnableLogging)
                {
                    PrivateData.fInLogMsg = true;
                    JAssert(PrivateData.objUtil.fnCreateNumberedFile != null, "Utilthrd Initialize() has not been called yet");
                    fileLog = PrivateData.objUtil.fnCreateNumberedFile("MTScript.log", MAX_MSGS_FILES);
                    fileLog[0].WriteLine("Log of " + LocalMachine + " Created " + d);
                    PrivateData.fileLog = fileLog;
                }
                ReleaseThreadLock("PrivateData.fileLog");
                PrivateData.fInLogMsg = false;
            }
            if (PrivateData.fEnableLogging)
                PrivateData.fileLog[0].WriteLine(strFormattedMsg);
        }
        OUTPUTDEBUGSTRING(d.slice(11) + "\t" + GetCallerName(cIgnoreCaller) + "\t" + strMsg);
    }
    catch(ex)
    {
        OUTPUTDEBUGSTRING("Error in AppendLog: " + ex);
        OUTPUTDEBUGSTRING(strMsg);
        PrivateData.fInLogMsg = false;
    }
}

/*
    Given parameters:
        testmachine
        D:\foo\bar\newnt
        \build_logs\sync_root.log

    change it to:
        \\testmachine\BC_D_foo_bar_newnt\build_logs\sync_root.log
 */
function MakeUNCPath(strMachineName, strEnlistmentPath, strPath)
{
//    var strPathInput = strPath;

    var aParts = strEnlistmentPath.split(/[\\:]/g);

    strPath = "\\\\" +
                strMachineName +
                "\\BC_" +
                aParts.join("_") +
                strPath;

//    LogMsg("Input: ('" + strEnlistmentPath + "', '" + strPathInput + "'), output='" + strPath + "' parts=(" + aParts.join(",") + ")");
    return strPath;
}

// MyEval(expr)
// evaluating uneval'ed objects creates a bunch of junk local variables.
// by putting the eval call in a little subroutine, we avoid keeping those
// locals around.
function MyEval(expr)
{
    try
    {
        return eval(expr);
    }
    catch(ex)
    {
        LogMsg("caught an exception: " + ex);
        LogMsg("evaluating " + expr);

        throw ex;
    }
}

// ---------------------------------------------------------------------------
//
//  Uneval implementation
//
//  Calling 'uneval(object)' will return a string that when eval'd will result
//  in the same object. This is used to marshal things between threads and
//  across machines.
//
//  This was stolen from the Scripting team's RemoteScripting implementation
//
// ---------------------------------------------------------------------------

//*****************************************************************
// function uneval(obj)
//    This function takes a given jscript object and creates a
//    string representing the object instance in its current state,
//    such that when the string is "evaluated" with the "eval"
//    function, the object will be recreated. This function is used
//    to to "marshall" jscript objects across the client/server
//    boundary.
//
//*****************************************************************
var unevalInitialized;
function unevalGetNextID()
{
    return '_o' + PrivateData.objUtil.unevalNextID++;
}

function unevalDecl(obj)
{
    var s = '';
    switch (typeof(obj))
    {
        case 'undefined':
        case 'boolean':
        case 'number':
        case 'string':
            break;

        default:
        try
        {
            if (null != obj && !obj.__mark && 'string' != typeof(obj.__decl))
                try
                {
                // if "obj" is an IDispatch, the call to
                // unevalDecl may throw.
                // In this case we just need to remove the __mark and move on.
                    obj.__mark = true;
                    obj.__decl = unevalGetNextID();
                    s = obj.__unevalDecl();
                    delete obj.__mark;
                }
                catch(ex)
                {
                    LogMsg("uneval exception " + ex); // BUGBUG remove this message
                    delete obj.__mark;
                }
            }
        catch(ex)
        {
            // if obj is inaccessible, we don't need to declare anything.
            LogMsg("uneval exception inaccessible" + ex); // BUGBUG remove this message
        }
        break;
    }
    return s;
}

function unevalInst(obj)
{
    var s = '';
    switch (typeof(obj))
    {
        case 'undefined':
            s = 'undefined';
            break;
        case 'boolean':
        case 'number':
            s = String(obj);
            break;
        case 'string':
            s = UnevalString(obj);
            break;

        default:
            try
            {
                if (null == obj)
                    s = 'null';
                else if (obj.__mark)
                    s = '"ERROR: Cycle in uneval graph."';
                else
                    try
                    {
                    // if "obj" is an IDispatch, the call to
                    // unevalDecl may throw.
                    // In this case we need to remove the
                    // __mark and supply a value (null).
                        obj.__mark = true;
                        s = obj.__unevalInst();
                        delete obj.__mark;
                    }
                    catch(ex)
                    {
                        s = 'null';
                        delete obj.__mark;
                        LogMsg("unevalInst exception " + ex); // BUGBUG remove this message
                    }
            }
            catch(ex)
            {
                // if "obj" is inacessible, say so.
                s = UnevalString('inaccessible object');
                LogMsg("unevalInst exception inaccessible" + ex); // BUGBUG remove this message
            }
            break;
    }
    return s;
}

function unevalClear(obj)
{
    switch (typeof(obj))
    {
        case 'undefined':
        case 'boolean':
        case 'number':
        case 'string':
            break;

        default:
            if (null != obj && !obj.__mark && 'string' == typeof(obj.__decl))
            {
                obj.__mark = true;
                try
                {
                // if "obj" is an IDispatch, the call to
                // unevalClear may throw.
                // In this case we need to remove the
                // __mark.
                    obj.__unevalClear();
                }
                catch(ex)
                {
                    LogMsg("unevalClear exception " + ex); // BUGBUG remove this message
                }
                delete obj.__mark;
            }
            break;
    }
    return ;
}

function unevalDoNothing()
{    }

function unevalConvertToString(obj)
{    return String(obj);        }

/*
 Reimplemented in the host interface for performance.
function unevalString(str)
{
    //TODO does this function kill performance?
    var i;
    var newstr = '"';
    var c;
    for(i = 0; i < str.length; ++i)
    {
        c = str.charAt(i);

        switch(c)
        {
        case'\\':
            newstr += "\\\\";
            break;
        case '"':
            newstr += '\\"';
            break;
        case "'":
            newstr += "\\'";
            break;
        case "\n":
            newstr += "\\n";
            break;
        case "\r":
            newstr += "\\r";
            break;
        case "\t":
            newstr += "\\t";
            break;
        default:
            newstr += c;
            break;
        }
    }

    return newstr + '"';
}
*/
//*****************************************************************
// function initUneval()
//
//    This function sets up the prototype __uneval functions that
//  are used to support uneval function for all data types.
//
//*****************************************************************
function initUneval()
{
    // instrinsic objects
    Object.__unevalDecl = unevalDoNothing;
    Object.__unevalInst = function () { return 'Object'; }
    Object.__unevalClear = unevalDoNothing;

    Boolean.__unevalDecl = unevalDoNothing;
    Boolean.__unevalInst = function () { return 'Boolean'; }
    Boolean.__unevalClear = unevalDoNothing;

    Number.__unevalDecl = unevalDoNothing;
    Number.__unevalInst = function () { return 'Number'; }
    Number.__unevalClear = unevalDoNothing;

    String.__unevalDecl = unevalDoNothing;
    String.__unevalInst = function () { return 'String'; }
    String.__unevalClear = unevalDoNothing;

    Date.__unevalDecl = unevalDoNothing;
    Date.__unevalInst = function () { return 'Date'; }
    Date.__unevalClear = unevalDoNothing;

    Function.__unevalDecl = unevalDoNothing;
    Function.__unevalInst = function () { return 'Function'; }
    Function.__unevalClear = unevalDoNothing;

    Array.__unevalDecl = unevalDoNothing;
    Array.__unevalInst = function () { return 'Array'; }
    Array.__unevalClear = unevalDoNothing;

    // object members
    Object.prototype.__enumMembers = function(retval,func)
    {
        var isPublicMember = this.__isPublicMember;
        if ('object' == typeof(this.__unevalProperties))
        {
            var unevalProperties = this.__unevalProperties;
            var length = unevalProperties.length;
            for (var i = 0; i < length; i++)
            {
                try
                {
                    if (isPublicMember(unevalProperties[i]))
                        func(retval, this, unevalProperties[i]);
                }
                catch(ex)
                {
                      // This happens if accessing "obj.foo" is inaccessible.
                      // This can happen if "foo" is an object and the thread
                      // which created "foo" has terminated.
                    LogMsg("enumMembers(1) caught an exception on member " + i); // BUGBUG remove this message
                }
            }
        }
        else
        {
            for (var i in this)
            {
                try
                {
                    if (isPublicMember(i))
                        func(retval, this, i);
                }
                catch(ex)
                {
                      // This happens if accessing "obj.foo" is inaccessible.
                      // This can happen if "foo" is an object and the thread
                      // which created "foo" has terminated.
                    LogMsg("enumMembers(2) caught an exception on member " + i); // BUGBUG remove this message
                }
            }
        }
    }

    Object.prototype.__unevalDeclMember = function (retval, obj, member)
    {
        retval.otherDecl += unevalDecl(obj[member]);
        retval.myDecl += obj.__decl + '[' + UnevalString(member) + ']=' + unevalInst(obj[member]) + ';\n';
    }

    Object.prototype.__unevalDecl = function()
    {
        var retval = { otherDecl:'', myDecl:'' };
        this.__enumMembers(retval, this.__unevalDeclMember);
        return retval.otherDecl + 'var ' + this.__decl + '=' + this.__unevalConstructor() + ';\n' + retval.myDecl;
    }

    Object.prototype.__unevalInst = function ()
    {    return this.__decl;        }

    Object.prototype.__unevalClearMember = function(retval, obj, member)
    {    unevalClear(obj[member]);    }

    Object.prototype.__unevalClear = function()
    {
        delete this.__decl;
        this.__enumMembers(null, this.__unevalClearMember);
    }

    Object.prototype.__isPublicMember = function(member)
    {    return '__' != member.substr(0,2);        }

    Object.prototype.__unevalConstructor = function ()
    {    return 'new Object';        }

    // overrides for simple types
    Boolean.prototype.__unevalConstructor = function()
    {    return 'new Boolean(' + String(this) + ')';    }
    Number.prototype.__unevalConstructor = function()
    {    return 'new Number(' + String(this) + ')';    }
    String.prototype.__unevalConstructor = function()
    {    return 'new String(' + UnevalString(this) + ')';    }
    Date.prototype.__unevalConstructor = function()
    {    return 'new Date(Date.parse("' + String(this) + '"))';    }

    // overrides for function
    Function.prototype.__unevalDecl = function()
    {    return String(this).replace(/function ([^\(]*)/,'function ' + this.__decl);    }

    // overrides for array
    Array.prototype.__unevalDecl = function()
    {
        var decl = this.__decl;
        var r = '';
        var s = 'var ' +  decl + '= new Array(' + this.length + ');\n';
        var length = this.length;
        for (var i = 0; i < length; i++)
        {
            r += unevalDecl(this[i]);
            s += decl + '[' + i + ']=' + unevalInst(this[i]) + ';\n';
        }
        return r + s;
    }

}    // end of initUneval
