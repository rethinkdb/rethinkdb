/*
*******************************************************************************
*
*   Copyright (C) 1998-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File error.c
*
* Modification History:
*
*   Date        Name        Description
*   05/28/99    stephen     Creation.
*******************************************************************************
*/

#include <stdarg.h>
#include <stdio.h>
#include "cstring.h"
#include "errmsg.h"

void error(uint32_t linenumber, const char *msg, ...)
{
    va_list va;

    va_start(va, msg);
    fprintf(stderr, "%s:%u: ", gCurrentFileName, (int)linenumber);
    vfprintf(stderr, msg, va);
    fprintf(stderr, "\n");
    va_end(va);
}

static UBool gShowWarning = TRUE;

void setShowWarning(UBool val)
{
    gShowWarning = val;
}

UBool getShowWarning(){
    return gShowWarning;
}

static UBool gStrict =FALSE;
UBool isStrict(){
    return gStrict;
}
void setStrict(UBool val){
    gStrict = val;
}
static UBool gVerbose =FALSE;
UBool isVerbose(){
    return gVerbose;
}
void setVerbose(UBool val){
    gVerbose = val;
}
void warning(uint32_t linenumber, const char *msg, ...)
{
    if (gShowWarning)
    {
        va_list va;

        va_start(va, msg);
        fprintf(stderr, "%s:%u: warning: ", gCurrentFileName, (int)linenumber);
        vfprintf(stderr, msg, va);
        fprintf(stderr, "\n");
        va_end(va);
    }
}

