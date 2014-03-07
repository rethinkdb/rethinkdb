/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2003-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*
 * File tracetst.c
 *
 */


#include "unicode/utypes.h"
#include "unicode/utrace.h"
#include "unicode/uclean.h"
#include "unicode/uchar.h"
#include "unicode/ures.h"
#include "unicode/ucnv.h"
#include "cintltst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* We define the following to always test tracing, even when it's off in the library. */
#if U_ENABLE_TRACING
#define ENABLE_TRACING_ORIG_VAL 1
#else
#define ENABLE_TRACING_ORIG_VAL 0
#endif
#undef U_ENABLE_TRACING
#define U_ENABLE_TRACING 1
#include "utracimp.h"


static void TestTraceAPI(void);


void
addUTraceTest(TestNode** root);

void
addUTraceTest(TestNode** root)
{
    addTest(root, &TestTraceAPI,            "tsutil/TraceTest/TestTraceAPI"  );
}


/*
 * Macro for assert style tests.
 */
#define TEST_ASSERT(expr) \
if (!(expr)) { \
    log_err("FAILED Assertion \"" #expr "\" at  %s:%d.\n", __FILE__, __LINE__); \
}


/*
 *  test_format.   Helper function for checking the results of a formatting
 *                 operation.  Executes the format op and compares actual 
 *                 results with the expected results.
 *                 
 *       params:   format:  the format to be applied.
 *                 bufCap   buffer size to pass to formatter.
 *                 indent:  indent value to give to formatter
 *                 result   expected result.  Do not truncate for short bufCap -
 *                          this function will do it.
 *                 line     __LINE__, so we can report where failure happened.
 *                 ...      variable args to pass to formatter
 *
 */
static void test_format(const char *format, int32_t bufCap, int32_t indent, 
                        const char *result, int32_t line, ...) {
    int32_t  len;
    va_list  args;
    char  buf[300];
    char  expectedResult[300];

    /* check that local buffers are big enough for the test case */
    if (sizeof(buf) <= bufCap) {
        log_err("At file:line %s:%d, requested bufCap too large.\n");
        return;
    }
    if (strlen(result) >= sizeof(expectedResult)) {
        log_err("At file:line %s:%d, expected result too large.\n");
        return;
    }

   /* Guarantee a nul term if buffer is smaller than output */
    strcpy(expectedResult, result);
    expectedResult[bufCap] = 0;

    /* run the formatter */
    va_start(args, line);
    memset(buf, 0, sizeof(buf));
    len = utrace_vformat(buf, bufCap, indent, format, args);

    /* Check results.   */
    if (strcmp(expectedResult, buf) != 0) {
        log_err("At file:line %s:%d  Expected \"%s\", got \"%s\"  \n",
             __FILE__, line, expectedResult, buf);
    }
    va_end(args);
}


/*
 *  define trace functions for use in this test.
 */
static int    gTraceEntryCount;
static int    gTraceExitCount;
static int    gTraceDataCount;
static UBool  gFnNameError   = FALSE;
static UBool  gFnFormatError = FALSE;

static void U_CALLCONV testTraceEntry(const void *context, int32_t fnNumber) {
    const char *fnName;
    const char *bogusFnName;

    gTraceEntryCount++;

    /* Verify that a name is available for the fnNumber passed to us */
    bogusFnName = utrace_functionName(-1);
    fnName = utrace_functionName(fnNumber);
    if (strcmp(fnName, bogusFnName) == 0) {
        gFnNameError = TRUE;
    }
    /* printf("%s() Enter\n", fnName); */

}

static void U_CALLCONV testTraceExit(const void *context, int32_t fnNumber,
                   const char *fmt, va_list args) {
    char        buf[1000];
    const char *fnName;
    const char *bogusFnName;

    gTraceExitCount++;
   
    /* Verify that a name is available for the fnNumber passed to us */
    bogusFnName = utrace_functionName(-1);
    fnName = utrace_functionName(fnNumber);
    if (strcmp(fnName, bogusFnName) == 0) {
        gFnNameError = TRUE;
    }

    /* Verify that the format can be used.  */
    buf[0] = 0;
    utrace_vformat(buf, sizeof(buf), 0, fmt, args);
    if (strlen(buf) == 0) {
        gFnFormatError = TRUE;
    }

    /* printf("%s() %s\n", fnName, buf); */

}

static void U_CALLCONV testTraceData(const void *context, int32_t fnNumber, int32_t level,
                   const char *fmt, va_list args) {
    char        buf[1000];
    const char *fnName;
    const char *bogusFnName;

    gTraceDataCount++;

    /* Verify that a name is available for the fnNumber passed to us */
    bogusFnName = utrace_functionName(-1);
    fnName = utrace_functionName(fnNumber);
    if (strcmp(fnName, bogusFnName) == 0) {
        gFnNameError = TRUE;
    }

    /* Verify that the format can be used.  */
    buf[0] = 0;
    utrace_vformat(buf, sizeof(buf), 0, fmt, args);
    if (strlen(buf) == 0) {
        gFnFormatError = TRUE;
    }

    /* printf("  %s()   %s\n", fnName, buf); */
}

static UConverter * psuedo_ucnv_open(const char *name, UErrorCode * err)
{
    UTRACE_ENTRY_OC(UTRACE_UCNV_LOAD);

    UTRACE_DATA2(UTRACE_OPEN_CLOSE, "error code is %s for %s", u_errorName(*err), name);

    UTRACE_EXIT_PTR_STATUS(NULL, *err);
    return NULL;
}
static void psuedo_ucnv_close(UConverter * cnv)
{
    UTRACE_ENTRY_OC(UTRACE_UCNV_UNLOAD);
    UTRACE_DATA1(UTRACE_OPEN_CLOSE, "unload converter %p", cnv);
    UTRACE_EXIT_VALUE((int32_t)TRUE);
}


/*
 *   TestTraceAPI
 */
static void TestTraceAPI() {


    UTraceEntry   *originalTEntryFunc;
    UTraceExit    *originalTExitFunc;
    UTraceData    *originalTDataFunc;
    const void    *originalTContext;
    int32_t        originalLevel;

    /*
     * Save the original tracing state so that we can restore it after the test.
     */
    utrace_getFunctions(&originalTContext, &originalTEntryFunc, &originalTExitFunc,
                        &originalTDataFunc);
    originalLevel = utrace_getLevel();


    /* verify that set/get of tracing functions returns what was set.  */
    {
        UTraceEntry *e;
        UTraceExit  *x;
        UTraceData  *d;
        const void  *context;
        const void  *newContext = (const char *)originalTContext + 1;
        
        TEST_ASSERT(originalTEntryFunc != testTraceEntry);
        TEST_ASSERT(originalTExitFunc != testTraceExit);
        TEST_ASSERT(originalTDataFunc != testTraceData);
        
        utrace_setFunctions(newContext, testTraceEntry, testTraceExit, testTraceData);
        utrace_getFunctions(&context, &e, &x, &d);
        TEST_ASSERT(e == testTraceEntry);
        TEST_ASSERT(x == testTraceExit);
        TEST_ASSERT(d == testTraceData);
        TEST_ASSERT(context == newContext);
    }

    /* verify that set/get level work as a pair, and that the level
     * identifiers all exist.
     */

    {
        int32_t  level;

        utrace_setLevel(UTRACE_OFF);
        level = utrace_getLevel();
        TEST_ASSERT(level==UTRACE_OFF);
        utrace_setLevel(UTRACE_VERBOSE);
        level = utrace_getLevel();
        TEST_ASSERT(level==UTRACE_VERBOSE);
        utrace_setLevel(UTRACE_ERROR);
        utrace_setLevel(UTRACE_WARNING);
        utrace_setLevel(UTRACE_OPEN_CLOSE);
        utrace_setLevel(UTRACE_INFO);
    }

    /*
     * Open and close a converter with tracing enabled.
     *   Verify that our tracing callback functions get called.
     */
    {
        UErrorCode  status = U_ZERO_ERROR;
        UConverter *cnv;
        
        gTraceEntryCount = 0;
        gTraceExitCount  = 0;
        gTraceDataCount  = 0;
        gFnNameError     = FALSE;
        gFnFormatError   = FALSE;
        utrace_setLevel(UTRACE_OPEN_CLOSE);
#if ENABLE_TRACING_ORIG_VAL
        cnv = ucnv_open(NULL, &status);
        TEST_ASSERT(U_SUCCESS(status));
        ucnv_close(cnv);
#else
        cnv = psuedo_ucnv_open(NULL, &status);
        TEST_ASSERT(U_SUCCESS(status));
        psuedo_ucnv_close(cnv);
#endif
        TEST_ASSERT(gTraceEntryCount > 0);
        TEST_ASSERT(gTraceExitCount  > 0);
        TEST_ASSERT(gTraceDataCount  > 0);
        TEST_ASSERT(gFnNameError   == FALSE);
        TEST_ASSERT(gFnFormatError == FALSE);
    }



    /*
     * trace data formatter operation.
     */
    {
        UChar s1[] = {0x41fe, 0x42, 0x43, 00};
        const char  *a1[] = {"s1", "s2", "s3"};
        void  *ptr;

        test_format("hello, world", 50, 0, "hello, world", __LINE__);
        test_format("hello, world", 50, 4, "    hello, world", __LINE__);
        test_format("hello, world", 3, 0,  "hello, world", __LINE__);

        test_format("a character %c", 50, 0, "a character x", __LINE__, 'x');
        test_format("a string %s ", 50, 0, "a string hello ", __LINE__, "hello");
        test_format("uchars %S ", 50, 0, "uchars 41fe 0042 0043 0000  ", __LINE__, s1, -1); 
        test_format("uchars %S ", 50, 0, "uchars 41fe 0042  ", __LINE__, s1, 2); 

        test_format("a byte %b--", 50, 0, "a byte dd--", __LINE__, 0xdd);
        test_format("a 16 bit val %h", 50, 0, "a 16 bit val 1234", __LINE__, 0x1234);
        test_format("a 32 bit val %d...", 50, 0, "a 32 bit val 6789abcd...", __LINE__, 0x6789abcd);
        test_format("a 64 bit val %l", 50, 0, "a 64 bit val 123456780abcdef0"
            , __LINE__, INT64_C(0x123456780abcdef0));

        if (sizeof(void *) == 4) {
            ptr = (void *)0xdeadbeef;
            test_format("a 32 bit ptr %p", 50, 0, "a 32 bit ptr deadbeef", __LINE__, ptr);
        } else if (sizeof(void *) == 8) {
            ptr = (void *) INT64_C(0x1000200030004000);
            test_format("a 64 bit ptr %p", 50, 0, "a 64 bit ptr 1000200030004000", __LINE__, ptr);
        } else if (sizeof(void *) == 16) {
            /* iSeries */
            union {
                int32_t arr[4];
                void *ptr;
            } massiveBigEndianPtr = {{ 0x10002000, 0x30004000, 0x50006000, 0x70008000 }};
            ptr = massiveBigEndianPtr.ptr;
            test_format("a 128 bit ptr %p", 50, 0, "a 128 bit ptr 10002000300040005000600070008000", __LINE__, ptr);
        } else {
            TEST_ASSERT(FALSE);
            /*  TODO:  others? */
        }

        test_format("%vc", 100, 0, "abc[ffffffff]", __LINE__, "abc", -1);
        test_format("%vs", 100, 0, "s1\ns2\n[00000002]", __LINE__, a1, 2);
        test_format("%vs", 100, 4, "    s1\n    s2\n    [00000002]", __LINE__, a1, 2);

        test_format("%vb", 100, 0, "41 42 43 [00000003]", __LINE__, "\x41\x42\x43", 3);

        /* Null ptrs for strings, vectors  */
        test_format("Null string - %s", 50, 0, "Null string - *NULL*", __LINE__, NULL);
        test_format("Null string - %S", 50, 0, "Null string - *NULL*", __LINE__, NULL);
        test_format("Null vector - %vc", 50, 0, "Null vector - *NULL* [00000002]", __LINE__, NULL, 2);
        test_format("Null vector - %vC", 50, 0, "Null vector - *NULL* [00000002]", __LINE__, NULL, 2);
        test_format("Null vector - %vd", 50, 0, "Null vector - *NULL* [00000002]", __LINE__, NULL, 2);

    }

    /*
     * utrace_format.  Only need a minimal test to see that the function works at all.
     *                 Full functionality is tested via utrace_vformat.
     */
    {
        char      buf[100];
        int32_t   x;
        x = utrace_format(buf, 100, 0, "%s", "Hello, World.");
        TEST_ASSERT(strcmp(buf, "Hello, World.") == 0);
        TEST_ASSERT(x == 14);
    }

    /*
     * utrace_functionName.  Just spot-check a couple of them.
     */
    {
        const char   *name;
        name = utrace_functionName(UTRACE_U_INIT);
        TEST_ASSERT(strcmp(name, "u_init") == 0);
        name = utrace_functionName(UTRACE_UCNV_OPEN);
        TEST_ASSERT(strcmp(name, "ucnv_open") == 0);
        name = utrace_functionName(UTRACE_UCOL_GET_SORTKEY);
        TEST_ASSERT(strcmp(name, "ucol_getSortKey") == 0);
    }



    /*  Restore the trace function settings to their original values. */
    utrace_setFunctions(originalTContext, originalTEntryFunc, originalTExitFunc, originalTDataFunc);
    utrace_setLevel(originalLevel);
}



