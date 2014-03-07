/*
**********************************************************************
*   Copyright (C) 2004-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  strtst.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004apr06
*   created by: George Rhoten
*/

#include "unicode/ustdio.h"
#include "unicode/ustring.h"
#include "iotest.h"

#include <string.h>

static void TestString(void) {
#if !UCONFIG_NO_FORMATTING
    int32_t n[1];
    float myFloat = -1234.0;
    int32_t newValuePtr[1];
    double newDoubleValuePtr[1];
    UChar myUString[512];
    UChar uStringBuf[512];
    char myString[512] = "";
    int32_t retVal;
    void *origPtr, *ptr;
    U_STRING_DECL(myStringOrig, "My-String", 9);

    U_STRING_INIT(myStringOrig, "My-String", 9);
    u_memset(myUString, 0x0a, sizeof(myUString)/ sizeof(*myUString));
    u_memset(uStringBuf, 0x0a, sizeof(uStringBuf) / sizeof(*uStringBuf));

    *n = -1234;
    if (sizeof(void *) == 4) {
        origPtr = (void *)0xdeadbeef;
    } else if (sizeof(void *) == 8) {
        origPtr = (void *) INT64_C(0x1000200030004000);
    } else if (sizeof(void *) == 16) {
        /* iSeries */
        union {
            int32_t arr[4];
            void *ptr;
        } massiveBigEndianPtr = {{ 0x10002000, 0x30004000, 0x50006000, 0x70008000 }};
        origPtr = massiveBigEndianPtr.ptr;
    } else {
        log_err("sizeof(void*)=%d hasn't been tested before", (int)sizeof(void*));
    }

    /* Test sprintf */
    u_sprintf(uStringBuf, "Signed decimal integer d: %d", *n);
    *newValuePtr = 1;
    u_sscanf(uStringBuf, "Signed decimal integer d: %d", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%d Got: %d, Expected: %d\n", *newValuePtr, *n);
    }

    u_sprintf(uStringBuf, "Signed decimal integer i: %i", *n);
    *newValuePtr = 1;
    u_sscanf(uStringBuf, "Signed decimal integer i: %i", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%i Got: %i, Expected: %i\n", *newValuePtr, *n);
    }

    u_sprintf(uStringBuf, "Unsigned octal integer o: %o", *n);
    *newValuePtr = 1;
    u_sscanf(uStringBuf, "Unsigned octal integer o: %o", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%o Got: %o, Expected: %o\n", *newValuePtr, *n);
    }

    u_sprintf(uStringBuf, "Unsigned decimal integer %%u: %u", *n);
    *newValuePtr = 1;
    u_sscanf(uStringBuf, "Unsigned decimal integer %%u: %u", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%u Got: %u, Expected: %u\n", *newValuePtr, *n);
    }

    u_sprintf(uStringBuf, "Lowercase unsigned hexadecimal integer x: %x", *n);
    *newValuePtr = 1;
    u_sscanf(uStringBuf, "Lowercase unsigned hexadecimal integer x: %x", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%x Got: %x, Expected: %x\n", *newValuePtr, *n);
    }

    u_sprintf(uStringBuf, "Uppercase unsigned hexadecimal integer X: %X", *n);
    *newValuePtr = 1;
    u_sscanf(uStringBuf, "Uppercase unsigned hexadecimal integer X: %X", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%X Got: %X, Expected: %X\n", *newValuePtr, *n);
    }

    u_sprintf(uStringBuf, "Float f: %f", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Float f: %lf", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%f Got: %f, Expected: %f\n", *newDoubleValuePtr, myFloat);
    }

    u_sprintf(uStringBuf, "Lowercase float e: %e", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Lowercase float e: %le", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%e Got: %e, Expected: %e\n", *newDoubleValuePtr, myFloat);
    }

    u_sprintf(uStringBuf, "Uppercase float E: %E", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Uppercase float E: %lE", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%E Got: %E, Expected: %E\n", *newDoubleValuePtr, myFloat);
    }

    u_sprintf(uStringBuf, "Lowercase float g: %g", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Lowercase float g: %lg", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%g Got: %g, Expected: %g\n", *newDoubleValuePtr, myFloat);
    }

    u_sprintf(uStringBuf, "Uppercase float G: %G", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Uppercase float G: %lG", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%G Got: %G, Expected: %G\n", *newDoubleValuePtr, myFloat);
    }

    ptr = NULL;
    u_sprintf(uStringBuf, "Pointer %%p: %p\n", origPtr);
    u_sscanf(uStringBuf, "Pointer %%p: %p\n", &ptr);
    if (ptr != origPtr || u_strlen(uStringBuf) != 13+(sizeof(void*)*2)) {
        log_err("%%p Got: %p, Expected: %p\n", ptr, origPtr);
    }

    u_sprintf(uStringBuf, "Char c: %c", 'A');
    u_sscanf(uStringBuf, "Char c: %c", myString);
    if (*myString != 'A') {
        log_err("%%c Got: %c, Expected: A\n", *myString);
    }

    u_sprintf(uStringBuf, "UChar %%C: %C", (UChar)0x0041); /*'A'*/
    u_sscanf(uStringBuf, "UChar %%C: %C", myUString);
    if (*myUString != (UChar)0x0041) { /*'A'*/
        log_err("%%C Got: %C, Expected: A\n", *myUString);
    }

    u_sprintf(uStringBuf, "String %%s: %s", "My-String");
    u_sscanf(uStringBuf, "String %%s: %s", myString);
    if (strcmp(myString, "My-String")) {
        log_err("%%s Got: %s, Expected: My-String\n", myString);
    }
    if (uStringBuf[20] != 0) {
        log_err("String not terminated. Got %c\n", uStringBuf[20] );
    }
    u_sprintf(uStringBuf, "NULL String %%s: %s", NULL);
    u_sscanf(uStringBuf, "NULL String %%s: %s", myString);
    if (strcmp(myString, "(null)")) {
        log_err("%%s Got: %s, Expected: My-String\n", myString);
    }

    u_sprintf(uStringBuf, "Unicode String %%S: %S", myStringOrig);
    u_sscanf(uStringBuf, "Unicode String %%S: %S", myUString);
    u_austrncpy(myString, myUString, sizeof(myString)/sizeof(*myString));
    if (strcmp(myString, "My-String")) {
        log_err("%%S Got: %s, Expected: My String\n", myString);
    }

    u_sprintf(uStringBuf, "NULL Unicode String %%S: %S", NULL);
    u_sscanf(uStringBuf, "NULL Unicode String %%S: %S", myUString);
    u_austrncpy(myString, myUString, sizeof(myString)/sizeof(*myString));
    if (strcmp(myString, "(null)")) {
        log_err("%%S Got: %s, Expected: (null)\n", myString);
    }

    u_sprintf(uStringBuf, "Percent %%P (non-ANSI): %P", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Percent %%P (non-ANSI): %P", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%P Got: %P, Expected: %P\n", *newDoubleValuePtr, myFloat);
    }

    u_sprintf(uStringBuf, "Spell Out %%V (non-ANSI): %V", myFloat);
    *newDoubleValuePtr = -1.0;
    u_sscanf(uStringBuf, "Spell Out %%V (non-ANSI): %V", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%V Got: %f, Expected: %f\n", *newDoubleValuePtr, myFloat);
    }

    *newValuePtr = 1;
    u_sprintf(uStringBuf, "\t\nPointer to integer (Count) %%n: n=%d %n n=%d\n", *newValuePtr, newValuePtr, *newValuePtr);
    if (*newValuePtr != 37) {
        log_err("%%V Got: %f, Expected: %f\n", *newDoubleValuePtr, myFloat);
    }

/*  u_sscanf(uStringBuf, "Pointer %%p: %p\n", myFile);*/

    {
        static const char longStr[] = "This is a long test12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";

        retVal = u_sprintf(uStringBuf, longStr);
        u_austrncpy(myString, uStringBuf, sizeof(uStringBuf)/sizeof(*uStringBuf));
        if (strcmp(myString, longStr)) {
            log_err("%%S Got: %s, Expected: %s\n", myString, longStr);
        }
        if (retVal != (int32_t)strlen(longStr)) {
            log_err("%%S returned different sizes. Got: %d  Expected: %d\n", retVal, strlen(longStr));
        }

        retVal = u_sprintf(uStringBuf, "%s", longStr);
        u_austrncpy(myString, uStringBuf, sizeof(uStringBuf)/sizeof(*uStringBuf));
        if (strcmp(myString, longStr)) {
            log_err("%%S Got: %s, Expected: %s\n", myString, longStr);
        }
        if (retVal != (int32_t)strlen(longStr)) {
            log_err("%%S returned different sizes. Got: %d  Expected: %d\n", retVal, strlen(longStr));
        }

        u_uastrncpy(myUString, longStr, sizeof(longStr)/sizeof(*longStr));
        u_sprintf_u(uStringBuf, myUString);
        if (u_strcmp(myUString, uStringBuf)) {
            log_err("%%S Long strings differ. Expected: %s\n", longStr);
        }

        u_uastrncpy(myUString, longStr, sizeof(longStr)/sizeof(*longStr));
        retVal = u_sprintf_u(uStringBuf, myUString+10);
        if (u_strcmp(myUString+10, uStringBuf)) {
            log_err("%%S Long strings differ. Expected: %s\n", longStr + 10);
        }
        if (retVal != (int32_t)strlen(longStr + 10)) {
            log_err("%%S returned different sizes. Got: %d  Expected: %d\n", retVal, strlen(longStr));
        }

        u_memset(uStringBuf, 1, sizeof(longStr)/sizeof(*longStr));
        u_uastrncpy(myUString, longStr, sizeof(longStr)/sizeof(*longStr));
        retVal = u_snprintf_u(uStringBuf, 10, myUString);
        if (u_strncmp(myUString, uStringBuf, 10) || uStringBuf[10] != 1 || retVal != 10) {
            log_err("%%S Long strings differ. Expected the first 10 characters of %s\n", longStr);
        }
    }
#endif
}

static void TestLocalizedString(void) {
#if !UCONFIG_NO_FORMATTING
    UChar testStr[256];
    UChar uBuffer[256];
    char cBuffer[256];
    int32_t numResult = -1;
    const char *locale;
    UFILE *strFile = u_fstropen(testStr, sizeof(testStr)/sizeof(testStr[0]), "en_US");

    if (!strFile) {
        log_err("u_fstropen failed to work\n");
        return;
    }
    u_fprintf(strFile, "%d", 1234);
    u_frewind(strFile);
    u_fscanf(strFile, "%d", &numResult);
    u_uastrcpy(uBuffer,"1,234");
    u_austrcpy(cBuffer,testStr);
    if (u_strcmp(testStr, uBuffer) != 0) {
        log_err("u_fprintf failed to work on an en string Got: %s\n", cBuffer);
    }
    if (numResult != 1234) {
        log_err("u_fscanf failed to work on an en string Got: %d\n", numResult);
    }

    u_frewind(strFile);
    locale = u_fgetlocale(strFile);
    if (locale == NULL || strcmp(locale, "en_US") != 0) {
        log_err("u_fgetlocale didn't return \"en\" Got: %d\n", u_fgetlocale(strFile));
    }
    u_fsetlocale(strFile, "de_DE");
    locale = u_fgetlocale(strFile);
    if (locale == NULL || strcmp(locale, "de_DE") != 0) {
        log_err("u_fgetlocale didn't return \"de\" Got: %d\n", u_fgetlocale(strFile));
    }

    u_fprintf(strFile, "%d", 1234);
    u_frewind(strFile);
    numResult = -1;
    u_fscanf(strFile, "%d", &numResult);
    u_fclose(strFile);
    u_uastrcpy(uBuffer,"1.234");
    u_austrcpy(cBuffer,testStr);
    if (u_strcmp(testStr, uBuffer) != 0) {
        log_err("u_fprintf failed to work on a de string Got: %s\n", cBuffer);
    }
    if (numResult != 1234) {
        log_err("u_fscanf failed to work on a de string Got: %d\n", numResult);
    }

    strFile = u_fstropen(testStr, sizeof(testStr)/sizeof(testStr[0]), NULL);
    u_fprintf(strFile, "%d", 1234);
    u_frewind(strFile);
    numResult = -1;
    u_fscanf(strFile, "%d", &numResult);
    u_fclose(strFile);
    if (numResult != 1234) {
        log_err("u_fscanf failed to work on a default locale string Got: %d, Expected: 1234\n", numResult);
    }
    if (u_fstropen(testStr, -1, NULL) != NULL) {
        log_err("u_fstropen returned a UFILE* on a negative buffer size\n", numResult);
    }
#endif
}

#if !UCONFIG_NO_FORMATTING
#define Test_u_snprintf(limit, format, value, expectedSize, expectedStr) \
    u_uastrncpy(testStr, "xxxxxxxxxxxxxx", sizeof(testStr)/sizeof(testStr[0]));\
    size = u_snprintf(testStr, limit, format, value);\
    u_austrncpy(cTestResult, testStr, sizeof(cTestResult)/sizeof(cTestResult[0]));\
    if (size != expectedSize || strcmp(cTestResult, expectedStr) != 0) {\
        log_err("Unexpected formatting. size=%d expectedSize=%d cTestResult=%s expectedStr=%s\n",\
            size, expectedSize, cTestResult, expectedStr);\
    }\
    else {\
        log_verbose("Got: %s\n", cTestResult);\
    }\

#endif

static void TestSnprintf(void) {
#if !UCONFIG_NO_FORMATTING
    UChar testStr[256];
    char cTestResult[256];
    int32_t size;

    Test_u_snprintf(0, "%d", 123, 3, "xxxxxxxxxxxxxx");
    Test_u_snprintf(2, "%d", 123, 3, "12xxxxxxxxxxxx");
    Test_u_snprintf(3, "%d", 123, 3, "123xxxxxxxxxxx");
    Test_u_snprintf(4, "%d", 123, 3, "123");

    Test_u_snprintf(0, "%s", "abcd", 4, "xxxxxxxxxxxxxx");
    Test_u_snprintf(3, "%s", "abcd", 4, "abcxxxxxxxxxxx");
    Test_u_snprintf(4, "%s", "abcd", 4, "abcdxxxxxxxxxx");
    Test_u_snprintf(5, "%s", "abcd", 4, "abcd");

    Test_u_snprintf(0, "%e", 12.34, 13, "xxxxxxxxxxxxxx");
    Test_u_snprintf(1, "%e", 12.34, 13, "1xxxxxxxxxxxxx");
    Test_u_snprintf(2, "%e", 12.34, 13, "1.xxxxxxxxxxxx");
    Test_u_snprintf(3, "%e", 12.34, 13, "1.2xxxxxxxxxxx");
    Test_u_snprintf(5, "%e", 12.34, 13, "1.234xxxxxxxxx");
    Test_u_snprintf(6, "%e", 12.34, 13, "1.2340xxxxxxxx");
    Test_u_snprintf(8, "%e", 12.34, 13, "1.234000xxxxxx");
    Test_u_snprintf(9, "%e", 12.34, 13, "1.234000exxxxx");
    Test_u_snprintf(10, "%e", 12.34, 13, "1.234000e+xxxx");
    Test_u_snprintf(11, "%e", 12.34, 13, "1.234000e+0xxx");
    Test_u_snprintf(13, "%e", 12.34, 13, "1.234000e+001x");
    Test_u_snprintf(14, "%e", 12.34, 13, "1.234000e+001");
#endif
}

#define TestSPrintFormat(uFormat, uValue, cFormat, cValue) \
    /* Reinitialize the buffer to verify null termination works. */\
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));\
    memset(buffer, '*', sizeof(buffer)/sizeof(*buffer));\
    \
    uNumPrinted = u_sprintf(uBuffer, uFormat, uValue);\
    u_austrncpy(compBuffer, uBuffer, sizeof(uBuffer)/sizeof(uBuffer[0]));\
    cNumPrinted = sprintf(buffer, cFormat, cValue);\
    if (strcmp(buffer, compBuffer) != 0) {\
        log_err("%" uFormat " Got: \"%s\", Expected: \"%s\"\n", compBuffer, buffer);\
    }\
    if (cNumPrinted != uNumPrinted) {\
        log_err("%" uFormat " number printed Got: %d, Expected: %d\n", uNumPrinted, cNumPrinted);\
    }\
    if (buffer[uNumPrinted+1] != '*') {\
        log_err("%" uFormat " too much stored\n");\
    }\

static void TestSprintfFormat(void) {
#if !UCONFIG_NO_FORMATTING
    static const UChar abcUChars[] = {0x61,0x62,0x63,0};
    static const char abcChars[] = "abc";
    const char *reorderFormat = "%2$d==>%1$-10.10s %6$lld %4$-10.10s %3$#x((%5$d"; /* reordering test*/
    const char *reorderResult = "99==>truncateif 1311768467463790322 1234567890 0xf1b93((10";
    UChar uBuffer[256];
    char buffer[256];
    char compBuffer[256];
    int32_t uNumPrinted;
    int32_t cNumPrinted;


    TestSPrintFormat("%8S", abcUChars, "%8s", abcChars);
    TestSPrintFormat("%-8S", abcUChars, "%-8s", abcChars);
    TestSPrintFormat("%.2S", abcUChars, "%.2s", abcChars); /* strlen is 3 */

    TestSPrintFormat("%8s", abcChars, "%8s", abcChars);
    TestSPrintFormat("%-8s", abcChars, "%-8s", abcChars);
    TestSPrintFormat("%.2s", abcChars, "%.2s", abcChars); /* strlen is 3 */

    TestSPrintFormat("%8c", (char)'e', "%8c", (char)'e');
    TestSPrintFormat("%-8c", (char)'e', "%-8c", (char)'e');

    TestSPrintFormat("%8C", (UChar)0x65, "%8c", (char)'e');
    TestSPrintFormat("%-8C", (UChar)0x65, "%-8c", (char)'e');

    TestSPrintFormat("%f", 1.23456789, "%f", 1.23456789);
    TestSPrintFormat("%f", 12345.6789, "%f", 12345.6789);
    TestSPrintFormat("%f", 123456.789, "%f", 123456.789);
    TestSPrintFormat("%f", 1234567.89, "%f", 1234567.89);
    TestSPrintFormat("%10f", 1.23456789, "%10f", 1.23456789);
    TestSPrintFormat("%-10f", 1.23456789, "%-10f", 1.23456789);
    TestSPrintFormat("%10f", 123.456789, "%10f", 123.456789);
    TestSPrintFormat("%10.4f", 123.456789, "%10.4f", 123.456789);
    TestSPrintFormat("%-10f", 123.456789, "%-10f", 123.456789);

/*    TestSPrintFormat("%g", 12345.6789, "%g", 12345.6789);
    TestSPrintFormat("%g", 123456.789, "%g", 123456.789);
    TestSPrintFormat("%g", 1234567.89, "%g", 1234567.89);
    TestSPrintFormat("%G", 123456.789, "%G", 123456.789);
    TestSPrintFormat("%G", 1234567.89, "%G", 1234567.89);*/
    TestSPrintFormat("%10g", 1.23456789, "%10g", 1.23456789);
    TestSPrintFormat("%10.4g", 1.23456789, "%10.4g", 1.23456789);
    TestSPrintFormat("%-10g", 1.23456789, "%-10g", 1.23456789);
    TestSPrintFormat("%10g", 123.456789, "%10g", 123.456789);
    TestSPrintFormat("%-10g", 123.456789, "%-10g", 123.456789);

    TestSPrintFormat("%8x", 123456, "%8x", 123456);
    TestSPrintFormat("%-8x", 123456, "%-8x", 123456);
    TestSPrintFormat("%08x", 123456, "%08x", 123456);

    TestSPrintFormat("%8X", 123456, "%8X", 123456);
    TestSPrintFormat("%-8X", 123456, "%-8X", 123456);
    TestSPrintFormat("%08X", 123456, "%08X", 123456);
    TestSPrintFormat("%#x", 123456, "%#x", 123456);
    TestSPrintFormat("%#x", -123456, "%#x", -123456);

    TestSPrintFormat("%8o", 123456, "%8o", 123456);
    TestSPrintFormat("%-8o", 123456, "%-8o", 123456);
    TestSPrintFormat("%08o", 123456, "%08o", 123456);
    TestSPrintFormat("%#o", 123, "%#o", 123);
    TestSPrintFormat("%#o", -123, "%#o", -123);

    TestSPrintFormat("%8u", 123456, "%8u", 123456);
    TestSPrintFormat("%-8u", 123456, "%-8u", 123456);
    TestSPrintFormat("%08u", 123456, "%08u", 123456);
    TestSPrintFormat("%8u", -123456, "%8u", -123456);
    TestSPrintFormat("%-8u", -123456, "%-8u", -123456);
    TestSPrintFormat("%.5u", 123456, "%.5u", 123456);
    TestSPrintFormat("%.6u", 123456, "%.6u", 123456);
    TestSPrintFormat("%.7u", 123456, "%.7u", 123456);

    TestSPrintFormat("%8d", 123456, "%8d", 123456);
    TestSPrintFormat("%-8d", 123456, "%-8d", 123456);
    TestSPrintFormat("%08d", 123456, "%08d", 123456);
    TestSPrintFormat("% d", 123456, "% d", 123456);
    TestSPrintFormat("% d", -123456, "% d", -123456);

    TestSPrintFormat("%8i", 123456, "%8i", 123456);
    TestSPrintFormat("%-8i", 123456, "%-8i", 123456);
    TestSPrintFormat("%08i", 123456, "%08i", 123456);

    log_verbose("Get really crazy with the formatting.\n");

    TestSPrintFormat("%-#12x", 123, "%-#12x", 123);
    TestSPrintFormat("%-#12x", -123, "%-#12x", -123);
    TestSPrintFormat("%#12x", 123, "%#12x", 123);
    TestSPrintFormat("%#12x", -123, "%#12x", -123);

    TestSPrintFormat("%-+12d", 123,  "%-+12d", 123);
    TestSPrintFormat("%-+12d", -123, "%-+12d", -123);
    TestSPrintFormat("%- 12d", 123,  "%- 12d", 123);
    TestSPrintFormat("%- 12d", -123, "%- 12d", -123);
    TestSPrintFormat("%+12d", 123,   "%+12d", 123);
    TestSPrintFormat("%+12d", -123,  "%+12d", -123);
    TestSPrintFormat("% 12d", 123,   "% 12d", 123);
    TestSPrintFormat("% 12d", -123,  "% 12d", -123);
    TestSPrintFormat("%12d", 123,    "%12d", 123);
    TestSPrintFormat("%12d", -123,   "%12d", -123);
    TestSPrintFormat("%.12d", 123,   "%.12d", 123);
    TestSPrintFormat("%.12d", -123,  "%.12d", -123);

    TestSPrintFormat("%-+12.1f", 1.234,  "%-+12.1f", 1.234);
    TestSPrintFormat("%-+12.1f", -1.234, "%-+12.1f", -1.234);
    TestSPrintFormat("%- 12.10f", 1.234, "%- 12.10f", 1.234);
    TestSPrintFormat("%- 12.1f", -1.234, "%- 12.1f", -1.234);
    TestSPrintFormat("%+12.1f", 1.234,   "%+12.1f", 1.234);
    TestSPrintFormat("%+12.1f", -1.234,  "%+12.1f", -1.234);
    TestSPrintFormat("% 12.1f", 1.234,   "% 12.1f", 1.234);
    TestSPrintFormat("% 12.1f", -1.234,  "% 12.1f", -1.234);
    TestSPrintFormat("%12.1f", 1.234,    "%12.1f", 1.234);
    TestSPrintFormat("%12.1f", -1.234,   "%12.1f", -1.234);
    TestSPrintFormat("%.2f", 1.234,      "%.2f", 1.234);
    TestSPrintFormat("%.2f", -1.234,     "%.2f", -1.234);
    TestSPrintFormat("%3f", 1.234,       "%3f", 1.234);
    TestSPrintFormat("%3f", -1.234,      "%3f", -1.234);
    
    /* Test reordering format */   
    u_sprintf(uBuffer, reorderFormat,"truncateiftoolong", 99, 990099, "12345678901234567890", 10, 0x123456789abcdef2LL);
    u_austrncpy(compBuffer, uBuffer, sizeof(uBuffer)/sizeof(uBuffer[0]));
   
    if (strcmp(compBuffer, reorderResult) != 0) {
        log_err("%s Got: \"%s\", Expected: \"%s\"\n", reorderFormat, compBuffer, buffer);
    }    
#endif
}

#undef TestSPrintFormat

static void TestStringCompatibility(void) {
#if !UCONFIG_NO_FORMATTING
    UChar myUString[256];
    UChar uStringBuf[256];
    char myString[256] = "";
    char testBuf[256] = "";
    int32_t num;

    u_memset(myUString, 0x0a, sizeof(myUString)/ sizeof(*myUString));
    u_memset(uStringBuf, 0x0a, sizeof(uStringBuf) / sizeof(*uStringBuf));

    /* Compare against C API compatibility */
    for (num = -STANDARD_TEST_NUM_RANGE; num < STANDARD_TEST_NUM_RANGE; num++) {
        sprintf(testBuf, "%x", (int)num);
        u_sprintf(uStringBuf, "%x", num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%x Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        sprintf(testBuf, "%X", (int)num);
        u_sprintf(uStringBuf, "%X", num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%X Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        sprintf(testBuf, "%o", (int)num);
        u_sprintf(uStringBuf, "%o", num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%o Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        /* sprintf is not compatible on all platforms e.g. the iSeries*/
        sprintf(testBuf, "%d", (int)num);
        u_sprintf(uStringBuf, "%d", num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%d Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        sprintf(testBuf, "%i", (int)num);
        u_sprintf(uStringBuf, "%i", num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%i Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        sprintf(testBuf, "%f", (double)num);
        u_sprintf(uStringBuf, "%f", (double)num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%f Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

/*        sprintf(testBuf, "%e", (double)num);
        u_sprintf(uStringBuf, "%e", (double)num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%e Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        sprintf(testBuf, "%E", (double)num);
        u_sprintf(uStringBuf, "%E", (double)num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%E Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }*/

        sprintf(testBuf, "%g", (double)num);
        u_sprintf(uStringBuf, "%g", (double)num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%g Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }

        sprintf(testBuf, "%G", (double)num);
        u_sprintf(uStringBuf, "%G", (double)num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (strcmp(myString, testBuf) != 0) {
            log_err("%%G Got: \"%s\", Expected: \"%s\"\n", myString, testBuf);
        }
    }

    for (num = 0; num < 0x80; num++) {
        testBuf[0] = (char)0xFF;
        uStringBuf[0] = (UChar)0xfffe;
        sprintf(testBuf, "%c", (char)num);
        u_sprintf(uStringBuf, "%c", num);
        u_austrncpy(myString, uStringBuf, sizeof(myString)/sizeof(myString[0]));
        if (testBuf[0] != myString[0] || myString[0] != num) {
            log_err("%%c Got: 0x%x, Expected: 0x%x\n", myString[0], testBuf[0]);
        }
    }
#endif
}

static void TestSScanSetFormat(const char *format, const UChar *uValue, const char *cValue, UBool expectedToPass) {
#if !UCONFIG_NO_FORMATTING
    UChar uBuffer[256];
    char buffer[256];
    char compBuffer[256];
    int32_t uNumScanned;
    int32_t cNumScanned;

    /* Reinitialize the buffer to verify null termination works. */
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));
    uBuffer[sizeof(uBuffer)/sizeof(*uBuffer)-1] = 0;
    memset(buffer, '*', sizeof(buffer)/sizeof(*buffer));
    buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;

    uNumScanned = u_sscanf(uValue, format, uBuffer);
    if (expectedToPass) {
        u_austrncpy(compBuffer, uBuffer, sizeof(uBuffer)/sizeof(uBuffer[0]));
        cNumScanned = sscanf(cValue, format, buffer);
        if (strncmp(buffer, compBuffer, sizeof(uBuffer)/sizeof(uBuffer[0])) != 0) {
            log_err("%s Got: \"%s\", Expected: \"%s\"\n", format, compBuffer, buffer);
        }
        if (cNumScanned != uNumScanned) {
            log_err("%s number scanned Got: %d, Expected: %d\n", format, uNumScanned, cNumScanned);
        }
        if (uNumScanned > 0 && uBuffer[u_strlen(uBuffer)+1] != 0x2a) {
            log_err("%s too much stored\n", format);
        }
    }
    else {
        if (uNumScanned != 0 || uBuffer[0] != 0x2a || uBuffer[1] != 0x2a) {
            log_err("%s too much stored on a failure\n", format);
        }
    }
#endif
}

static void TestSScanset(void) {
#if !UCONFIG_NO_FORMATTING
    static const UChar abcUChars[] = {0x61,0x62,0x63,0x63,0x64,0x65,0x66,0x67,0};
    static const char abcChars[] = "abccdefg";

    TestSScanSetFormat("%[bc]S", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[cb]S", abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%[ab]S", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[ba]S", abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%[ab]", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[ba]", abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%[abcdefgh]", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[;hgfedcba]", abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%[^a]", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[^e]", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[^ed]", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[^dc]", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%[^e]  ", abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%1[ab]  ", abcUChars, abcChars, TRUE);
    TestSScanSetFormat("%2[^f]", abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%[qrst]", abcUChars, abcChars, TRUE);

    /* Extra long string for testing */
    TestSScanSetFormat("                                                                                                                         %[qrst]",
        abcUChars, abcChars, TRUE);

    TestSScanSetFormat("%[a-]", abcUChars, abcChars, TRUE);

    /* Bad format */
    TestSScanSetFormat("%[a", abcUChars, abcChars, FALSE);
    TestSScanSetFormat("%[f-a]", abcUChars, abcChars, FALSE);
    TestSScanSetFormat("%[c-a]", abcUChars, abcChars, FALSE);
    /* The following is not deterministic on Windows */
/*    TestSScanSetFormat("%[a-", abcUChars, abcChars);*/

    /* TODO: Need to specify precision with a "*" */
#endif
}

static void TestBadSScanfFormat(const char *format, const UChar *uValue, const char *cValue) {
#if !UCONFIG_NO_FORMATTING
    UChar uBuffer[256];
    int32_t uNumScanned;

    /* Reinitialize the buffer to verify null termination works. */
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));
    uBuffer[sizeof(uBuffer)/sizeof(*uBuffer)-1] = 0;

    uNumScanned = u_sscanf(uValue, format, uBuffer);
    if (uNumScanned != 0 || uBuffer[0] != 0x2a || uBuffer[1] != 0x2a) {
        log_err("%s too much stored on a failure\n", format);
    }
#endif
}

static void TestBadScanfFormat(void) {
#if !UCONFIG_NO_FORMATTING
    static const UChar abcUChars[] = {0x61,0x62,0x63,0x63,0x64,0x65,0x66,0x67,0};
    static const char abcChars[] = "abccdefg";

    TestBadSScanfFormat("%[]  ", abcUChars, abcChars);
#endif
}

static void Test_u_vfprintf(const char *expectedResult, const char *format, ...) {
#if !UCONFIG_NO_FORMATTING
    UChar uBuffer[256];
    UChar uBuffer2[256];
    va_list ap;
    int32_t count;

    va_start(ap, format);
    count = u_vsprintf(uBuffer, format, ap);
    va_end(ap);
    u_uastrcpy(uBuffer2, expectedResult);
    if (u_strcmp(uBuffer, uBuffer2) != 0) {
        log_err("Got two different results for \"%s\" expected \"%s\"\n", format, expectedResult);
    }

    u_uastrcpy(uBuffer2, format);
    va_start(ap, format);
    count = u_vsprintf_u(uBuffer, uBuffer2, ap);
    va_end(ap);
    u_uastrcpy(uBuffer2, expectedResult);
    if (u_strcmp(uBuffer, uBuffer2) != 0) {
        log_err("Got two different results for \"%s\" expected \"%s\"\n", format, expectedResult);
    }
#endif
}

static void TestVargs(void) {
#if !UCONFIG_NO_FORMATTING
    Test_u_vfprintf("8 9 a B 8.9", "%d %u %x %X %.1f", 8, 9, 10, 11, 8.9);
#endif
}

static void TestCount(void) {
#if !UCONFIG_NO_FORMATTING
    static const UChar x15[] = { 0x78, 0x31, 0x35, 0 };
    UChar testStr[16];
    UChar character;
    int16_t i16 = -1;
    int32_t i32 = -1, actual_count, actual_result;
    int64_t i64 = -1;
    u_uastrcpy(testStr, "1233456789");
    if (u_sscanf(testStr, "%*3[123]%n%*[1-9]", &i32) != 0) {
        log_err("test 1: scanf did not return 0\n");
    }
    if (i32 != 3) {
        log_err("test 1: scanf returned %hd instead of 3\n", i32);
    }
    if (u_sscanf(testStr, "%*4[123]%hn%*[1-9]", &i16) != 0) {
        log_err("test 2: scanf did not return 0\n");
    }
    if (i16 != 4) {
        log_err("test 2: scanf returned %d instead of 4\n", i16);
    }
    if (u_sscanf(testStr, "%*[123]%*[1-9]%lln", &i64) != 0) {
        log_err("test 3: scanf did not return 0\n");
    }
    if (i64 != 10) {
        log_err("test 3: scanf did not return 10\n", i64);
    }
    actual_result = u_sscanf(x15, "%C%d%n", &character, &i32, &actual_count);
    if (actual_result != 2) {
        log_err("scanf should return 2, but returned %d\n", actual_result);
    }
    if (character != 0x78) {
        log_err("scanf should return 0x78 for the character, but returned %X\n", character);
    }
    if (i32 != 15) {
        log_err("scanf should return 15 for the number, but returned %d\n", i32);
    }
    if (actual_count != 3) {
        log_err("scanf should return 3 for actual_count, but returned %d\n", actual_count);
    }
#endif
}

U_CFUNC void
addStringTest(TestNode** root) {
#if !UCONFIG_NO_FORMATTING
    addTest(root, &TestString, "string/TestString");
    addTest(root, &TestLocalizedString, "string/TestLocalizedString");
    addTest(root, &TestSprintfFormat, "string/TestSprintfFormat");
    addTest(root, &TestSnprintf, "string/TestSnprintf");
    addTest(root, &TestSScanset, "string/TestSScanset");
    addTest(root, &TestStringCompatibility, "string/TestStringCompatibility");
    addTest(root, &TestBadScanfFormat, "string/TestBadScanfFormat");
    addTest(root, &TestVargs, "string/TestVargs");
    addTest(root, &TestCount, "string/TestCount");
#endif
}


