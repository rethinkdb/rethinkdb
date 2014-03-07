/*
********************************************************************************
*   Copyright (C) 2005-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File WINDTTST.CPP
*
********************************************************************************
*/

#include "unicode/utypes.h"

#ifdef U_WINDOWS

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "unicode/ustring.h"
#include "unicode/testlog.h"
#include "unicode/utmscale.h"

#include "windtfmt.h"
#include "winutil.h"
#include "windttst.h"

#include "cmemory.h"
#include "cstring.h"
#include "locmap.h"
#include "wintzimpl.h"

#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

static const char *getCalendarType(int32_t type)
{
    switch (type)
    {
    case 1:
    case 2:
        return "@calendar=gregorian";

    case 3:
        return "@calendar=japanese";

    case 6:
        return "@calendar=islamic";

    case 7:
        return "@calendar=buddhist";

    case 8:
        return "@calendar=hebrew";

    default:
        return "";
    }
}

void Win32DateTimeTest::testLocales(TestLog *log)
{
    SYSTEMTIME winNow;
    UDate icuNow = 0;
    SYSTEMTIME st;
    FILETIME ft;
    UnicodeString zoneID;
    const TimeZone *tz = TimeZone::createDefault();
    TIME_ZONE_INFORMATION tzi;

    tz->getID(zoneID);
    if (! uprv_getWindowsTimeZoneInfo(&tzi, zoneID.getBuffer(), zoneID.length())) {
        UBool found = FALSE;
        int32_t ec = TimeZone::countEquivalentIDs(zoneID);

        for (int z = 0; z < ec; z += 1) {
            UnicodeString equiv = TimeZone::getEquivalentID(zoneID, z);

            if (found = uprv_getWindowsTimeZoneInfo(&tzi, equiv.getBuffer(), equiv.length())) {
                break;
            }
        }

        if (! found) {
            GetTimeZoneInformation(&tzi);
        }
    }

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    SystemTimeToTzSpecificLocalTime(&tzi, &st, &winNow);

    int64_t wftNow = ((int64_t) ft.dwHighDateTime << 32) + ft.dwLowDateTime;
    UErrorCode status = U_ZERO_ERROR;

    int64_t udtsNow = utmscale_fromInt64(wftNow, UDTS_WINDOWS_FILE_TIME, &status);

    icuNow = (UDate) utmscale_toInt64(udtsNow, UDTS_ICU4C_TIME, &status);

    int32_t lcidCount = 0;
    Win32Utilities::LCIDRecord *lcidRecords = Win32Utilities::getLocales(lcidCount);

    for(int i = 0; i < lcidCount; i += 1) {
        UErrorCode status = U_ZERO_ERROR;
        WCHAR longDateFormat[81], longTimeFormat[81], wdBuffer[256], wtBuffer[256];
        int32_t calType = 0;

        // NULL localeID means ICU didn't recognize this locale
        if (lcidRecords[i].localeID == NULL) {
            continue;
        }

        GetLocaleInfoW(lcidRecords[i].lcid, LOCALE_SLONGDATE,   longDateFormat, 81);
        GetLocaleInfoW(lcidRecords[i].lcid, LOCALE_STIMEFORMAT, longTimeFormat, 81);
        GetLocaleInfoW(lcidRecords[i].lcid, LOCALE_RETURN_NUMBER|LOCALE_ICALENDARTYPE, (LPWSTR) calType, sizeof(int32_t));

        char localeID[64];

        uprv_strcpy(localeID, lcidRecords[i].localeID);
        uprv_strcat(localeID, getCalendarType(calType));

        UnicodeString ubBuffer, udBuffer, utBuffer;
        Locale ulocale(localeID);
        int32_t wdLength, wtLength;

        wdLength = GetDateFormatW(lcidRecords[i].lcid, DATE_LONGDATE, &winNow, NULL, wdBuffer, ARRAY_SIZE(wdBuffer));
        wtLength = GetTimeFormatW(lcidRecords[i].lcid, 0, &winNow, NULL, wtBuffer, ARRAY_SIZE(wtBuffer));

        if (uprv_strchr(localeID, '@') > 0) {
            uprv_strcat(localeID, ";");
        } else {
            uprv_strcat(localeID, "@");
        }

        uprv_strcat(localeID, "compat=host");

        Locale wlocale(localeID);
        DateFormat *wbf = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, wlocale);
        DateFormat *wdf = DateFormat::createDateInstance(DateFormat::kFull, wlocale);
        DateFormat *wtf = DateFormat::createTimeInstance(DateFormat::kFull, wlocale);

        wbf->format(icuNow, ubBuffer);
        wdf->format(icuNow, udBuffer);
        wtf->format(icuNow, utBuffer);

        if (ubBuffer.indexOf(wdBuffer, wdLength - 1, 0) < 0) {
            UnicodeString baseName(wlocale.getBaseName());
            UnicodeString expected(wdBuffer);

            log->errln("DateTime format error for locale " + baseName + ": expected date \"" + expected +
                       "\" got \"" + ubBuffer + "\"");
        }

        if (ubBuffer.indexOf(wtBuffer, wtLength - 1, 0) < 0) {
            UnicodeString baseName(wlocale.getBaseName());
            UnicodeString expected(wtBuffer);

            log->errln("DateTime format error for locale " + baseName + ": expected time \"" + expected +
                       "\" got \"" + ubBuffer + "\"");
        }

        if (udBuffer.compare(wdBuffer) != 0) {
            UnicodeString baseName(wlocale.getBaseName());
            UnicodeString expected(wdBuffer);

            log->errln("Date format error for locale " + baseName + ": expected \"" + expected +
                       "\" got \"" + udBuffer + "\"");
        }

        if (utBuffer.compare(wtBuffer) != 0) {
            UnicodeString baseName(wlocale.getBaseName());
            UnicodeString expected(wtBuffer);

            log->errln("Time format error for locale " + baseName + ": expected \"" + expected +
                       "\" got \"" + utBuffer + "\"");
        }
        delete wbf;
        delete wdf;
        delete wtf;
    }

    Win32Utilities::freeLocales(lcidRecords);
    delete tz;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* #ifdef U_WINDOWS */
