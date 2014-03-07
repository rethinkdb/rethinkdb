/*
********************************************************************************
*   Copyright (C) 2005-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File WINUTIL.CPP
*
********************************************************************************
*/

#include "unicode/utypes.h"

#ifdef U_WINDOWS

#if !UCONFIG_NO_FORMATTING

#include "winutil.h"
#include "locmap.h"
#include "unicode/uloc.h"

#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>
#   include <stdio.h>
#   include <string.h>

static Win32Utilities::LCIDRecord *lcidRecords = NULL;
static int32_t lcidCount  = 0;
static int32_t lcidMax = 0;

BOOL CALLBACK EnumLocalesProc(LPSTR lpLocaleString)
{
    const char* localeID = NULL;
    UErrorCode status = U_ZERO_ERROR;

    if (lcidCount >= lcidMax) {
        Win32Utilities::LCIDRecord *newRecords = new Win32Utilities::LCIDRecord[lcidMax + 32];

        for (int i = 0; i < lcidMax; i += 1) {
            newRecords[i] = lcidRecords[i];
        }

        delete[] lcidRecords;
        lcidRecords = newRecords;
        lcidMax += 32;
    }

    sscanf(lpLocaleString, "%8x", &lcidRecords[lcidCount].lcid);

    localeID = uprv_convertToPosix(lcidRecords[lcidCount].lcid, &status);

    lcidRecords[lcidCount].localeID = new char[strlen(localeID)];

    strcpy(lcidRecords[lcidCount].localeID, localeID);

    lcidCount += 1;

    return TRUE;
}

Win32Utilities::LCIDRecord *Win32Utilities::getLocales(int32_t &localeCount)
{
    LCIDRecord *result;

    EnumSystemLocalesA(EnumLocalesProc, LCID_INSTALLED);

    localeCount = lcidCount;
    result      = lcidRecords;

    lcidCount = lcidMax = 0;
    lcidRecords = NULL;

    return result;
}

void Win32Utilities::freeLocales(LCIDRecord *records)
{
    for (int i = 0; i < lcidCount; i++) {
        delete lcidRecords[i].localeID;
    }
    delete[] records;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* #ifdef U_WINDOWS */
