/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    util.c

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


ULONG
GetBitFieldOffset (
   IN LPSTR     Type,
   IN LPSTR     Field,
   OUT PULONG   pOffset,
   OUT PULONG   pSize
   )
{
   FIELD_INFO flds = {
       Field, "", 0,
       DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS | DBG_DUMP_FIELD_SIZE_IN_BITS,
       0, NULL};
   SYM_DUMP_PARAM Sym = {
      sizeof (SYM_DUMP_PARAM), Type, DBG_DUMP_NO_PRINT, 0,
      NULL, NULL, NULL, 1, &flds
   };
   ULONG Err, i=0;
   LPSTR dot, last=Field;

   Sym.nFields = 1;
   Err = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );
   *pOffset = (ULONG) (flds.address - Sym.addr);
   *pSize   = flds.size;
   return Err;
}

ULONG
GetUlongFromAddress (
    ULONG64 Location
    )
{
    ULONG Value;
    ULONG result;

    if ((!ReadMemory(Location,&Value,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG))) {
        dprintf("unable to read from %08x\n",Location);
        return 0;
    }

    return Value;
}

ULONG64
GetPointerFromAddress (
    ULONG64 Location
    )
{
    ULONG64 Value;
    ULONG result;

    if (!ReadPointer(Location,&Value)) {
        dprintf("unable to read from %08p\n",Location);
        return 0;
    }

    return Value;
}

ULONG
GetUlongValue (
    PCHAR String
    )
{
    ULONG64 Location;
    ULONG Value;
    ULONG result;


    Location = GetExpression( String );
    if (!Location) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    return GetUlongFromAddress( Location );
}


ULONG64
GetPointerValue (
    PCHAR String
    )
{
    ULONG64 Location, Val=0;


    Location = GetExpression( String );
    if (!Location) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    ReadPointer(Location, &Val);

    return Val;
}
#if 0
VOID
DumpImageName(
    IN ULONG64 Process
    )
{
    ULONG64 ImageFileName;
    STRING String;
    ULONG Result;
    IN WCHAR Buf[512];


    if ( !GetFieldValue(Process, "EPROCESS", "ImageFileName.Buffer", ImageFileName ) ){

        wcscpy(Buf,L"*** image name unavailable ***");
        if ( ReadMemory( ImageFileName,
                         &String,
                         sizeof(STRING),
                         &Result) ) {
            if ( ReadMemory( (DWORD)String.Buffer,
                             &Buf[0],
                             String.Length,
                             &Result) ) {
                Buf[String.Length/sizeof(WCHAR)] = UNICODE_NULL;
            }
        }
    } else {
        wcscpy(Buf,L"System Process");
    }
    dprintf("%ws",Buf);
}
#endif

BOOLEAN
DbgRtlIsRightChild(
    ULONG64 pLinks,
    ULONG64 Parent
    )
{
    ULONG64 RightChild;
    if (Parent == pLinks) {

        return FALSE;
    }

    if (GetFieldValue(Parent, "RTL_SPLAY_LINKS", "RightChild", RightChild)) {

        return FALSE;
    }

    if (RightChild == pLinks) {

        return TRUE;
    }

    return FALSE;
}

BOOLEAN
DbgRtlIsLeftChild(
    ULONG64 pLinks,
    ULONG64 Parent
    )
{
    ULONG64 LeftChild;
    if (Parent == pLinks) {

        return FALSE;
    }

    if (GetFieldValue(Parent, "RTL_SPLAY_LINKS", "LeftChild", LeftChild)) {

        return FALSE;
    }

    if (LeftChild == pLinks) {

        return TRUE;
    }

    return FALSE;
}


ULONG
DumpSplayTree(
    IN ULONG64 pSplayLinks,
    IN PDUMP_SPLAY_NODE_FN DumpNodeFn
    )
/*++
    Purpose:

        Perform an in-order iteration across a splay tree, calling a
        user supplied function with a pointer to each RTL_SPLAY_LINKS
        structure encountered in the tree, and the level in the tree
        at which it was encountered (zero based).

    Arguments:

        pSplayLinks     - pointer to root of a splay tree

        DumpNodeFn      - user supplied dumping function

   Returns:

        Count of nodes encountered in the tree.

   Notes:

        Errors reading memory do not terminate the iteration if more
        work is possible.

        Consumes the Control-C flag to terminate possible loops in
        corrupt structures.

--*/
{
    ULONG Level = 0;
    ULONG NodeCount = 0;

    if (pSplayLinks) {
        ULONG64 LeftChild, RightChild, Parent, Current;

        //
        //  Retrieve the root links, find the leftmost node in the tree
        //

        if (GetFieldValue(Current = pSplayLinks,
                          "RTL_SPLAY_LINKS",
                          "LeftChild",
                          LeftChild)) {

            return NodeCount;
        }

        while (LeftChild != 0) {

            if ( CheckControlC() ) {

                return NodeCount;
            }

            if (GetFieldValue(Current = LeftChild,
                              "RTL_SPLAY_LINKS",
                              "LeftChild",
                              LeftChild)) {

                //
                //  We can try to continue from this
                //

                break;
            }

            Level++;
        }

        while (TRUE) {

            if ( CheckControlC() ) {

                return NodeCount;
            }

            NodeCount++;
            pSplayLinks = Current;
            (*DumpNodeFn)(pSplayLinks, Level);

            /*
                first check to see if there is a right subtree to the input link
                if there is then the real successor is the left most node in
                the right subtree.  That is find and return P in the following diagram

                      Links
                         \
                          .
                         .
                        .
                       /
                      P
                       \
            */

            GetFieldValue(Current, "RTL_SPLAY_LINKS", "RightChild", RightChild);
            if (RightChild != 0) {

                if (GetFieldValue(Current = RightChild,
                                  "RTL_SPLAY_LINKS",
                                  "RightChild",
                                  RightChild)) {

                    //
                    //  We've failed to step through to a successor, so
                    //  there is no more to do
                    //

                    return NodeCount;
                }

                Level++;

                GetFieldValue(Current,"RTL_SPLAY_LINKS","LeftChild",LeftChild);
                while (LeftChild != 0) {

                    if ( CheckControlC() ) {

                        return NodeCount;
                    }

                    if (GetFieldValue(Current = LeftChild,
                                     "RTL_SPLAY_LINKS",
                                     "LeftChild",
                                     LeftChild)) {

                        //
                        //  We can continue from this
                        //

                        break;
                    }

                    Level++;
                }

            } else {

                /*
                    we do not have a right child so check to see if have a parent and if
                    so find the first ancestor that we are a left decendent of. That
                    is find and return P in the following diagram

                               P
                              /
                             .
                              .
                               .
                              Links
                */

                //
                //  If the IsLeft or IsRight functions fail to read through a parent
                //  pointer, then we will quickly exit through the break below
                //

                GetFieldValue(Current, "RTL_SPLAY_LINKS", "Parent", Parent);
                while (DbgRtlIsRightChild(Current, Parent)) {

                    if ( CheckControlC() ) {

                        return NodeCount;
                    }

                    Level--;
                    pSplayLinks = (Current = Parent);
                }

                GetFieldValue(Current, "RTL_SPLAY_LINKS", "Parent", Parent);
                if (!DbgRtlIsLeftChild(Current, Parent)) {

                    //
                    //  we do not have a real successor so we break out
                    //

                    break;

                } else {

                    Level--;
                    pSplayLinks = (Current = Parent);
                }
            }
        }
    }

    return NodeCount;
}

VOID
DumpUnicode64(
    UNICODE_STRING64 u
    )
{
    UNICODE_STRING v;
    DWORD BytesRead;

 //   dprintf("L %x, M %x, B %p ", u.Length, u.MaximumLength, u.Buffer);
    if ((u.Length <= u.MaximumLength) &&
        (u.Buffer) &&
        (u.Length > 0)) {

        v.Buffer = LocalAlloc(LPTR, u.MaximumLength+2);
        if (v.Buffer != NULL) {
            v.MaximumLength = u.MaximumLength;
            v.Length = u.Length;
            if (ReadMemory(u.Buffer,
                           v.Buffer,
                           u.Length,
                           (PULONG) &u.Buffer) &&
                (v.Buffer[0] != 0)) {
                v.Buffer[v.Length/sizeof(WCHAR)] = 0;
                dprintf("%ws", v.Buffer);
            } else {
                dprintf("< Name not readable >");
            }
            LocalFree(v.Buffer);

            return;
        }
    }
    dprintf("< Name not readable >");
}

BOOLEAN
IsHexNumber(
   const char *szExpression
   )
{
   if (!szExpression[0]) {
      return FALSE ;
   }

   for(;*szExpression; szExpression++) {

      if      ((*szExpression)< '0') { return FALSE ; }
      else if ((*szExpression)> 'f') { return FALSE ; }
      else if ((*szExpression)>='a') { continue ;     }
      else if ((*szExpression)> 'F') { return FALSE ; }
      else if ((*szExpression)<='9') { continue ;     }
      else if ((*szExpression)>='A') { continue ;     }
      else                           { return FALSE ; }
   }
   return TRUE ;
}


BOOLEAN
IsDecNumber(
   const char *szExpression
   )
{
   if (!szExpression[0]) {
      return FALSE ;
   }

   while(*szExpression) {

      if      ((*szExpression)<'0') { return FALSE ; }
      else if ((*szExpression)>'9') { return FALSE ; }
      szExpression ++ ;
   }
   return TRUE ;
}
