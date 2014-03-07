/*
********************************************************************************
*   Copyright (C) 2005-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File WINNMTST.CPP
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

#include "winnmfmt.h"
#include "winutil.h"
#include "winnmtst.h"

#include "cmemory.h"
#include "cstring.h"
#include "locmap.h"
#include "wintz.h"
#include "uassert.h"

#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>
#   include <stdio.h>
#   include <time.h>
#   include <float.h>
#   include <locale.h>

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])
#define NEW_ARRAY(type,count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))

#define STACK_BUFFER_SIZE 32

#define LOOP_COUNT 1000

static UBool initialized = FALSE;

/**
 * Return a random int64_t where U_INT64_MIN <= ran <= U_INT64_MAX.
 */
static uint64_t randomInt64(void)
{
    int64_t ran = 0;
    int32_t i;

    if (!initialized) {
        srand((unsigned)time(NULL));
        initialized = TRUE;
    }

    /* Assume rand has at least 12 bits of precision */
    for (i = 0; i < sizeof(ran); i += 1) {
        ((char*)&ran)[i] = (char)((rand() & 0x0FF0) >> 4);
    }

    return ran;
}

/**
 * Return a random double where U_DOUBLE_MIN <= ran <= U_DOUBLE_MAX.
 */
static double randomDouble(void)
{
    double ran = 0;

    if (!initialized) {
        srand((unsigned)time(NULL));
        initialized = TRUE;
    }
#if 0
    int32_t i;
    do {
        /* Assume rand has at least 12 bits of precision */
        for (i = 0; i < sizeof(ran); i += 1) {
            ((char*)&ran)[i] = (char)((rand() & 0x0FF0) >> 4);
        }
    } while (_isnan(ran));
#else
	int64_t numerator = randomInt64();
	int64_t denomenator;
    do {
        denomenator = randomInt64();
    }
    while (denomenator == 0);

	ran = (double)numerator / (double)denomenator;
#endif

    return ran;
}

/**
 * Return a random int32_t where U_INT32_MIN <= ran <= U_INT32_MAX.
 */
static uint32_t randomInt32(void)
{
    int32_t ran = 0;
    int32_t i;

    if (!initialized) {
        srand((unsigned)time(NULL));
        initialized = TRUE;
    }

    /* Assume rand has at least 12 bits of precision */
    for (i = 0; i < sizeof(ran); i += 1) {
        ((char*)&ran)[i] = (char)((rand() & 0x0FF0) >> 4);
    }

    return ran;
}

static UnicodeString &getWindowsFormat(int32_t lcid, UBool currency, UnicodeString &appendTo, const wchar_t *fmt, ...)
{
    wchar_t nStackBuffer[STACK_BUFFER_SIZE];
    wchar_t *nBuffer = nStackBuffer;
    va_list args;
    int result;

    nBuffer[0] = 0x0000;

    /* Due to the arguments causing a result to be <= 23 characters (+2 for NULL and minus),
    we don't need to reallocate the buffer. */
    va_start(args, fmt);
    result = _vsnwprintf(nBuffer, STACK_BUFFER_SIZE, fmt, args);
    va_end(args);

    /* Just to make sure of the above statement, we add this assert */
    U_ASSERT(result >=0);
    // The following code is not used because _vscwprintf isn't available on MinGW at the moment.
    /*if (result < 0) {
        int newLength;

        va_start(args, fmt);
        newLength = _vscwprintf(fmt, args);
        va_end(args);

        nBuffer = NEW_ARRAY(UChar, newLength + 1);

        va_start(args, fmt);
        result = _vsnwprintf(nBuffer, newLength + 1, fmt, args);
        va_end(args);
    }*/


    // vswprintf is sensitive to the locale set by setlocale. For some locales
    // it doesn't use "." as the decimal separator, which is what GetNumberFormatW
    // and GetCurrencyFormatW both expect to see.
    //
    // To fix this, we scan over the string and replace the first non-digits, except
    // for a leading "-", with a "."
    //
    // Note: (nBuffer[0] == L'-') will evaluate to 1 if there is a leading '-' in the
    // number, and 0 otherwise.
    for (wchar_t *p = &nBuffer[nBuffer[0] == L'-']; *p != L'\0'; p += 1) {
        if (*p < L'0' || *p > L'9') {
            *p = L'.';
            break;
        }
    }

    wchar_t stackBuffer[STACK_BUFFER_SIZE];
    wchar_t *buffer = stackBuffer;

    buffer[0] = 0x0000;

    if (currency) {
        result = GetCurrencyFormatW(lcid, 0, nBuffer, NULL, buffer, STACK_BUFFER_SIZE);

        if (result == 0) {
            DWORD lastError = GetLastError();

            if (lastError == ERROR_INSUFFICIENT_BUFFER) {
                int newLength = GetCurrencyFormatW(lcid, 0, nBuffer, NULL, NULL, 0);

                buffer = NEW_ARRAY(UChar, newLength);
                buffer[0] = 0x0000;
                GetCurrencyFormatW(lcid, 0, nBuffer, NULL, buffer, newLength);
            }
        }
    } else {
        result = GetNumberFormatW(lcid, 0, nBuffer, NULL, buffer, STACK_BUFFER_SIZE);

        if (result == 0) {
            DWORD lastError = GetLastError();

            if (lastError == ERROR_INSUFFICIENT_BUFFER) {
                int newLength = GetNumberFormatW(lcid, 0, nBuffer, NULL, NULL, 0);

                buffer = NEW_ARRAY(UChar, newLength);
                buffer[0] = 0x0000;
                GetNumberFormatW(lcid, 0, nBuffer, NULL, buffer, newLength);
            }
        }
    }

    appendTo.append(buffer, (int32_t) wcslen(buffer));

    if (buffer != stackBuffer) {
        DELETE_ARRAY(buffer);
    }

    /*if (nBuffer != nStackBuffer) {
        DELETE_ARRAY(nBuffer);
    }*/

    return appendTo;
}

static void testLocale(const char *localeID, int32_t lcid, NumberFormat *wnf, UBool currency, TestLog *log)
{
    for (int n = 0; n < LOOP_COUNT; n += 1) {
        UnicodeString u3Buffer, u6Buffer, udBuffer;
        UnicodeString w3Buffer, w6Buffer, wdBuffer;
        double d    = randomDouble();
        int32_t i32 = randomInt32();
        int64_t i64 = randomInt64();

        getWindowsFormat(lcid, currency, wdBuffer, L"%.16f", d);

        getWindowsFormat(lcid, currency, w3Buffer, L"%I32d", i32);

        getWindowsFormat(lcid, currency, w6Buffer, L"%I64d", i64);

        wnf->format(d,   udBuffer);
        if (udBuffer.compare(wdBuffer) != 0) {
            UnicodeString locale(localeID);

            log->errln("Double format error for locale " + locale +
                                    ": got " + udBuffer + " expected " + wdBuffer);
        }

        wnf->format(i32, u3Buffer);
        if (u3Buffer.compare(w3Buffer) != 0) {
            UnicodeString locale(localeID);

            log->errln("int32_t format error for locale " + locale +
                                    ": got " + u3Buffer + " expected " + w3Buffer);
        }

        wnf->format(i64, u6Buffer);
        if (u6Buffer.compare(w6Buffer) != 0) {
            UnicodeString locale(localeID);

            log->errln("int64_t format error for locale " + locale +
                                    ": got " + u6Buffer + " expected " + w6Buffer);
        }
    }
}

void Win32NumberTest::testLocales(TestLog *log)
{
    int32_t lcidCount = 0;
    Win32Utilities::LCIDRecord *lcidRecords = Win32Utilities::getLocales(lcidCount);

    for(int i = 0; i < lcidCount; i += 1) {
        UErrorCode status = U_ZERO_ERROR;
        char localeID[128];

        // NULL localeID means ICU didn't recognize the lcid
        if (lcidRecords[i].localeID == NULL) {
            continue;
        }

        strcpy(localeID, lcidRecords[i].localeID);

        if (strchr(localeID, '@') > 0) {
            strcat(localeID, ";");
        } else {
            strcat(localeID, "@");
        }

        strcat(localeID, "compat=host");

        Locale ulocale(localeID);
        NumberFormat *wnf = NumberFormat::createInstance(ulocale, status);
        NumberFormat *wcf = NumberFormat::createCurrencyInstance(ulocale, status);

        testLocale(lcidRecords[i].localeID, lcidRecords[i].lcid, wnf, FALSE, log);
        testLocale(lcidRecords[i].localeID, lcidRecords[i].lcid, wcf, TRUE,  log);

#if 0
        char *old_locale = strdup(setlocale(LC_ALL, NULL));
        
        setlocale(LC_ALL, "German");

        testLocale(lcidRecords[i].localeID, lcidRecords[i].lcid, wnf, FALSE, log);
        testLocale(lcidRecords[i].localeID, lcidRecords[i].lcid, wcf, TRUE,  log);

        setlocale(LC_ALL, old_locale);

        free(old_locale);
#endif

        delete wcf;
        delete wnf;
    }

    Win32Utilities::freeLocales(lcidRecords);
}

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* #ifdef U_WINDOWS */
