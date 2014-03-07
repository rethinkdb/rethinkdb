/*
**********************************************************************
*   Copyright (C) 2002-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  iotest.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002feb21
*   created by: George Rhoten
*/


#include "unicode/ustream.h"

#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "ustr_cnv.h"
#include "iotest.h"

#if U_IOSTREAM_SOURCE >= 199711
#if defined(__GNUC__) && __GNUC__ >= 4
#define USE_SSTREAM 1
#include <sstream>
#else
// <strstream> is deprecated on some platforms, and the compiler complains very loudly if you use it.
#include <strstream>
#endif
#include <fstream>
#include <iomanip>
using namespace std;
#elif U_IOSTREAM_SOURCE >= 198506
#define USE_OLD_IOSTREAM 1
#include <strstream.h>
#include <fstream.h>
#endif

#include <string.h>

U_CDECL_BEGIN
#ifdef U_WINDOWS
const UChar NEW_LINE[] = {0x0d,0x0a,0};
const char C_NEW_LINE[] = {0x0d,0x0a,0};
#define UTF8_NEW_LINE "\x0d\x0a"
#else
const UChar NEW_LINE[] = {0x0a,0};
const char C_NEW_LINE[] = {'\n',0};
#define UTF8_NEW_LINE "\x0a"
#endif
U_CDECL_END

#if U_IOSTREAM_SOURCE
U_CDECL_BEGIN
static void U_CALLCONV TestStream(void)
{
#if U_IOSTREAM_SOURCE >= 198506
    const UChar thisMu[] = { 0x74, 0x48, 0x69, 0x73, 0x3BC, 0};
    const UChar mu[] = { 0x6D, 0x75, 0};
    UnicodeString str1 = UNICODE_STRING_SIMPLE("str1");
    UnicodeString str2 = UNICODE_STRING_SIMPLE(" <<");
    UnicodeString str3 = UNICODE_STRING_SIMPLE("2");
    UnicodeString str4 = UNICODE_STRING_SIMPLE(" UTF-8 ");
    UnicodeString inStr = UNICODE_STRING_SIMPLE(" UTF-8 ");
    UnicodeString inStr2;
    char defConvName[UCNV_MAX_CONVERTER_NAME_LENGTH*2];
    char inStrC[128];
    UErrorCode status = U_ZERO_ERROR;
    UConverter *defConv;
    static const char testStr[] = "\x42\x65\x67\x69\x6E\x6E\x69\x6E\x67\x20\x6F\x66\x20\x74\x65\x73\x74\x20\x73\x74\x72\x31\x20\x20\x20\x3C\x3C\x32\x31\x20" UTF8_NEW_LINE "\x20\x55\x54\x46\x2D\x38\x20\xCE\xBC\xF0\x90\x80\x81\xF0\x90\x80\x82";

    str4.append((UChar32)0x03BC);   /* mu */
    str4.append((UChar32)0x10001);
    str4.append((UChar32)0x10002);

    /* release the default converter and use utf-8 for a bit */
    defConv = u_getDefaultConverter(&status);
    if (U_FAILURE(status)) {
        log_err("Can't get default converter\n");
        return;
    }
    ucnv_close(defConv);
    strncpy(defConvName, ucnv_getDefaultName(), sizeof(defConvName)/sizeof(defConvName[0]));
    ucnv_setDefaultName("UTF-8");

    static const char * const TESTSTRING = "\x20\x74\x48\x69\x73\xCE\xBC\xE2\x80\x82\x20\x6D\x75\x20\x77\x6F\x72\x6C\x64";
#ifdef USE_SSTREAM
    ostringstream outTestStream;
    istringstream inTestStream(TESTSTRING);
#else
    char testStreamBuf[512];
    ostrstream outTestStream(testStreamBuf, sizeof(testStreamBuf));
    istrstream inTestStream(TESTSTRING, 0);

    /* initialize testStreamBuf */
    memset(testStreamBuf, '*', sizeof(testStreamBuf));
    testStreamBuf[sizeof(testStreamBuf)-1] = 0;
#endif

    outTestStream << "\x42\x65\x67\x69\x6E\x6E\x69\x6E\x67\x20\x6F\x66\x20\x74\x65\x73\x74\x20";
    outTestStream << str1 << "\x20\x20" << str2 << str3 << "\x31\x20" << UTF8_NEW_LINE << str4 << ends;
#ifdef USE_SSTREAM
    string tempStr = outTestStream.str();
    const char *testStreamBuf = tempStr.c_str();
#endif
    if (strcmp(testStreamBuf, testStr) != 0) {
        log_err("Got: \"%s\", Expected: \"%s\"\n", testStreamBuf, testStr);
    }

    inTestStream >> inStr >> inStr2;
    if (inStr.compare(thisMu) != 0) {
        u_austrncpy(inStrC, inStr.getBuffer(), inStr.length());
        inStrC[inStr.length()] = 0;
        log_err("Got: \"%s\", Expected: \"tHis\\u03BC\"\n", inStrC);
    }
    if (inStr2.compare(mu) != 0) {
        u_austrncpy(inStrC, inStr.getBuffer(), inStr.length());
        inStrC[inStr.length()] = 0;
        log_err("Got: \"%s\", Expected: \"mu\"\n", inStrC);
    }

    /* return the default converter to the original state. */
    ucnv_setDefaultName(defConvName);
    defConv = u_getDefaultConverter(&status);
    if (U_FAILURE(status)) {
        log_err("Can't get default converter");
        return;
    }

    /* Test formatting when using '<<' and UnicodeString */
#ifdef USE_SSTREAM
    ostringstream outFormatStream;
#else
    char testFormatStreamBuf[512];
    memset(testFormatStreamBuf, 0, sizeof(testFormatStreamBuf));
    ostrstream outFormatStream(testFormatStreamBuf, sizeof(testFormatStreamBuf));
#endif
    UnicodeString ustr("string");

    outFormatStream << "1234567890" << setw(10) << left << ustr << " " << "0123456789";

#ifdef USE_SSTREAM
    tempStr = outFormatStream.str();
    const char *testFormatStreamBuf = tempStr.c_str();
#endif
    const char *format_test_expected = "1234567890string     0123456789";
    if (strcmp(format_test_expected, testFormatStreamBuf) != 0) {
        log_err("UnicodeString format test using << operator Got: '%s' Expected: '%s'\n", testFormatStreamBuf, format_test_expected);
    }

    /* Test large buffer (size > 200) when using '<<' and UnicodeString */
#ifdef USE_SSTREAM
    ostringstream outLargeStream;
#else
    char testLargeStreamBuf[512];
    memset(testLargeStreamBuf, 0, sizeof(testLargeStreamBuf));
    ostrstream outLargeStream(testLargeStreamBuf, sizeof(testLargeStreamBuf));
#endif
    UChar large_array[200];
    int32_t large_array_length = sizeof(large_array)/sizeof(UChar);
    for (int32_t i = 0; i < large_array_length; i++) {
        large_array[i] = 0x41;
    }
    UnicodeString large_array_unistr(large_array, large_array_length);

    outLargeStream << large_array_unistr;

#ifdef USE_SSTREAM
    string tmpString = outLargeStream.str();
    const char *testLargeStreamBuf = tmpString.c_str();
#endif
    char expectedLargeStreamBuf[300];
    int32_t expectedBufLength = sizeof(expectedLargeStreamBuf);

    ucnv_fromUChars(defConv, expectedLargeStreamBuf, expectedBufLength, large_array, large_array_length, &status);
    if (U_SUCCESS(status)) {
        if (strcmp(testLargeStreamBuf, expectedLargeStreamBuf) != 0) {
            log_err("Large UnicodeString operator << output incorrect.\n");
        }
    } else {
        log_err("Error converting string for large stream buffer testing.\n");
    }
    ucnv_close(defConv);
#else
    log_info("U_IOSTREAM_SOURCE is disabled\n");
#endif
}

#define IOSTREAM_GOOD_SHIFT 3
#define IOSTREAM_GOOD (1<<IOSTREAM_GOOD_SHIFT)
#define IOSTREAM_BAD_SHIFT 2
#define IOSTREAM_BAD (1<<IOSTREAM_BAD_SHIFT)
#define IOSTREAM_EOF_SHIFT 1
#define IOSTREAM_EOF (1<<IOSTREAM_EOF_SHIFT)
#define IOSTREAM_FAIL_SHIFT 0
#define IOSTREAM_FAIL (1<<IOSTREAM_FAIL_SHIFT)

static int32_t getBitStatus(const iostream&  stream) {
    return (stream.good()<<IOSTREAM_GOOD_SHIFT)
        | (stream.bad()<<IOSTREAM_BAD_SHIFT)
        | (stream.eof()<<IOSTREAM_EOF_SHIFT)
        | (stream.fail()<<IOSTREAM_FAIL_SHIFT);
}

void
printBits(const iostream&  stream)
{
    int32_t status = getBitStatus(stream);
    log_verbose("status 0x%02X (", status);
    if (status & IOSTREAM_GOOD) {
        log_verbose("good");
    }
    if (status & IOSTREAM_BAD) {
        log_verbose("bad");
    }
    if (status & IOSTREAM_EOF) {
        log_verbose("eof");
    }
    if (status & IOSTREAM_FAIL) {
        log_verbose("fail");
    }
    log_verbose(")\n");
}

void
testString(
            UnicodeString&  str,
            const char*     testString,
            const UChar*    expectedString,
            int32_t         expectedStatus)
{
#ifdef USE_SSTREAM
    stringstream sstrm;
#else
    strstream sstrm;
#endif

    sstrm << testString;

    /*log_verbose("iostream before operator::>>() call \"%s\" ", testString);
    printBits(sstrm);*/

    sstrm >> str;

    log_verbose("iostream after operator::>>() call \"%s\" ", testString);
    printBits(sstrm);

    if (getBitStatus(sstrm) != expectedStatus) {
        printBits(sstrm);
#ifdef USE_OLD_IOSTREAM
        log_info("Warning. Expected status %d, Got %d. This maybe caused by the fact that the non-standardized iostream is being used.\n", expectedStatus, getBitStatus(sstrm));
        log_info("See verbose output for details.\n");
#else
        log_err("Expected status %d, Got %d. See verbose output for details\n", expectedStatus, getBitStatus(sstrm));
#endif
    }
    if (str != UnicodeString(expectedString)) {
        log_err("Did not get expected results from \"%s\", expected \"%s\"\n", testString, expectedString);
    }
}


static void U_CALLCONV TestStreamEOF(void)
{
    UnicodeString dest;
    fstream fs(STANDARD_TEST_FILE, fstream::in | fstream::out | fstream::trunc);
#ifdef USE_SSTREAM
    stringstream ss;
#else
    strstream ss;
#endif

#ifdef USE_OLD_IOSTREAM
    log_info("Old non-standardized iostream being used. This may result in inconsistent state flag settings. (e.g. failbit may not be set properly)\n");
    log_info("In such a case, warnings will be issued instead of errors.\n");
#endif

    fs << "EXAMPLE";
    fs.seekg(0);
    ss << "EXAMPLE";

    if (!(fs >> dest)) {
        log_err("Reading of file did not return expected status result\n");
    }
    if (dest != "EXAMPLE") {
        log_err("Reading of file did not return expected string\n");
    }

    if (!(ss >> dest)) {
        log_err("Reading of string did not return expected status result\n");
    }
    if (dest != "EXAMPLE") {
        log_err("Reading of string did not return expected string\n");
    }
    fs.close();

    log_verbose("Testing operator >> for UnicodeString...\n");

    /* The test cases needs to be converted to the default codepage.  However, the stream operator needs char* so U_STRING_* is called. */
    U_STRING_DECL(testCase1, "", 0);
    U_STRING_INIT(testCase1, "", 0);
    U_STRING_DECL(testCase2, "foo", 3);
    U_STRING_INIT(testCase2, "foo", 3);
    U_STRING_DECL(testCase3, "   ", 3);
    U_STRING_INIT(testCase3, "   ", 3);
    U_STRING_DECL(testCase4, "   bar", 6);
    U_STRING_INIT(testCase4, "   bar", 6);
    U_STRING_DECL(testCase5, "bar   ", 6);
    U_STRING_INIT(testCase5, "bar   ", 6);
    U_STRING_DECL(testCase6, "   bar   ", 9);
    U_STRING_INIT(testCase6, "   bar   ", 9);


    U_STRING_DECL(expectedResultA, "", 0);
    U_STRING_INIT(expectedResultA, "", 0);
    U_STRING_DECL(expectedResultB, "foo", 3);
    U_STRING_INIT(expectedResultB, "foo", 3);
    U_STRING_DECL(expectedResultC, "unchanged", 9);
    U_STRING_INIT(expectedResultC, "unchanged", 9);
    U_STRING_DECL(expectedResultD, "bar", 3);
    U_STRING_INIT(expectedResultD, "bar", 3);


    UnicodeString UStr;
    UnicodeString expectedResults;
    char testcase[10];
    testString(UStr, u_austrcpy(testcase, testCase1), expectedResultA, IOSTREAM_EOF|IOSTREAM_FAIL);
    testString(UStr, u_austrcpy(testcase, testCase2), expectedResultB, IOSTREAM_EOF);
    UStr = UnicodeString(expectedResultC);
    testString(UStr, u_austrcpy(testcase, testCase3), expectedResultC, IOSTREAM_EOF|IOSTREAM_FAIL);
    testString(UStr, u_austrcpy(testcase, testCase4), expectedResultD, IOSTREAM_EOF);
    testString(UStr, u_austrcpy(testcase, testCase5), expectedResultD, IOSTREAM_GOOD);
    testString(UStr, u_austrcpy(testcase, testCase6), expectedResultD, IOSTREAM_GOOD);
}
U_CDECL_END

U_CFUNC void addStreamTests(TestNode** root) {
    addTest(root, &TestStream, "stream/TestStream");
    addTest(root, &TestStreamEOF, "stream/TestStreamEOF");
}
#endif
