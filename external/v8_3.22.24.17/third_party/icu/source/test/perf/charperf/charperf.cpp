/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2002-2006 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
/*****************************************************************************
* File charperf.cpp
*
* Modification History:
* Name                     Description
* Syn Wee Quek             First Version
******************************************************************************
*/

/** 
 * This program tests character properties performance.
 * APIs tested: 
 * ICU4C 
 * Windows
 */

#include "charperf.h"
#include "uoptions.h"

UOption options[] = {
    UOPTION_DEF("min", 'n', UOPT_REQUIRES_ARG),
        UOPTION_DEF("min", 'x', UOPT_REQUIRES_ARG),
};
int MIN_OPTION_ = 0;
int MAX_OPTION_ = 1;

int main(int argc, const char *argv[])
{
    UErrorCode status = U_ZERO_ERROR;
    CharPerformanceTest test(argc, argv, status);
    if (U_FAILURE(status)){
        return status;
    }
    if (test.run() == FALSE){
        fprintf(stderr, "FAILED: Tests could not be run please check the "
            "arguments.\n");
        return -1;
    }
    return 0;
}

CharPerformanceTest::CharPerformanceTest(int32_t argc, const char *argv[],
                                         UErrorCode &status)
                                         : UPerfTest(argc, argv, status)
{
    if (status== U_ILLEGAL_ARGUMENT_ERROR){
        fprintf(stderr,gUsageString, "charperf");
        return;
    }
    if (U_FAILURE(status)){
        fprintf(stderr, "FAILED to create UPerfTest object. Error: %s\n", 
            u_errorName(status));
        return;
    }

    if (_remainingArgc < 0) {
        // that means there are some -names not matched in the super class
        // first tag is always skipped in u_parseArgs
        int size = - _remainingArgc;
        argv += argc - size;
        argc = size;
        _remainingArgc = u_parseArgs(argc, (char**)argv, 
            (int32_t)(sizeof(options)/sizeof(options[0])), options);
    }
    MIN_ = 0;
    if (sizeof(wchar_t) > 2)  {
        // for stdlibs like glibc that supports 32 bits wchar
        // we test for the whole unicode character set by default
        MAX_ = 0x10ffff;
    }
    else {
        MAX_ = 0xffff;
    }
    printf("MAX_ size will be 0x%x\n", MAX_);
    if (options[MIN_OPTION_].doesOccur) {
        MIN_ = atoi(options[MIN_OPTION_].value);
    }
    if (options[MAX_OPTION_].doesOccur) {
        MAX_ = atoi(options[MAX_OPTION_].value);
    }
}

CharPerformanceTest::~CharPerformanceTest()
{
}

UPerfFunction* CharPerformanceTest::runIndexedTest(int32_t index, UBool exec,
                                                   const char *&name, 
                                                   char* par) 
{
    switch (index) {
        TESTCASE(0, TestIsAlpha);
        TESTCASE(1, TestIsUpper);
        TESTCASE(2, TestIsLower);
        TESTCASE(3, TestIsDigit);
        TESTCASE(4, TestIsSpace);
        TESTCASE(5, TestIsAlphaNumeric);
        TESTCASE(6, TestIsPrint);
        TESTCASE(7, TestIsControl);
        TESTCASE(8, TestToLower);
        TESTCASE(9, TestToUpper);
        TESTCASE(10, TestIsWhiteSpace);
        TESTCASE(11, TestStdLibIsAlpha);
        TESTCASE(12, TestStdLibIsUpper);
        TESTCASE(13, TestStdLibIsLower);
        TESTCASE(14, TestStdLibIsDigit);
        TESTCASE(15, TestStdLibIsSpace);
        TESTCASE(16, TestStdLibIsAlphaNumeric);
        TESTCASE(17, TestStdLibIsPrint);
        TESTCASE(18, TestStdLibIsControl);
        TESTCASE(19, TestStdLibToLower);
        TESTCASE(20, TestStdLibToUpper);
        TESTCASE(21, TestStdLibIsWhiteSpace);
        default: 
            name = ""; 
            return NULL;
    }
    return NULL;
}

UPerfFunction* CharPerformanceTest::TestIsAlpha()
{
    return new CharPerfFunction(isAlpha, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsUpper()
{
    return new CharPerfFunction(isUpper, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsLower()
{
    return new CharPerfFunction(isLower, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsDigit()
{
    return new CharPerfFunction(isDigit, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsSpace()
{
    return new CharPerfFunction(isSpace, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsAlphaNumeric()
{
    return new CharPerfFunction(isAlphaNumeric, MIN_, MAX_);
}

/**
* This test may be different since c lib has a type PUNCT and it is printable.
* iswgraph is not used for testing since it is a subset of iswprint with the
* exception of returning true for white spaces. no match found in icu4c.
*/
UPerfFunction* CharPerformanceTest::TestIsPrint()
{
    return new CharPerfFunction(isPrint, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsControl()
{
    return new CharPerfFunction(isControl, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestToLower()
{
    return new CharPerfFunction(toLower, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestToUpper()
{
    return new CharPerfFunction(toUpper, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestIsWhiteSpace()
{
    return new CharPerfFunction(isWhiteSpace, MIN_, MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsAlpha()
{
    return new StdLibCharPerfFunction(StdLibIsAlpha, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsUpper()
{
    return new StdLibCharPerfFunction(StdLibIsUpper, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsLower()
{
    return new StdLibCharPerfFunction(StdLibIsLower, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsDigit()
{
    return new StdLibCharPerfFunction(StdLibIsDigit, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsSpace()
{
    return new StdLibCharPerfFunction(StdLibIsSpace, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsAlphaNumeric()
{
    return new StdLibCharPerfFunction(StdLibIsAlphaNumeric, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

/**
* This test may be different since c lib has a type PUNCT and it is printable.
* iswgraph is not used for testing since it is a subset of iswprint with the
* exception of returning true for white spaces. no match found in icu4c.
*/
UPerfFunction* CharPerformanceTest::TestStdLibIsPrint()
{
    return new StdLibCharPerfFunction(StdLibIsPrint, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsControl()
{
    return new StdLibCharPerfFunction(StdLibIsControl, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibToLower()
{
    return new StdLibCharPerfFunction(StdLibToLower, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibToUpper()
{
    return new StdLibCharPerfFunction(StdLibToUpper, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}

UPerfFunction* CharPerformanceTest::TestStdLibIsWhiteSpace()
{
    return new StdLibCharPerfFunction(StdLibIsWhiteSpace, (wchar_t)MIN_, 
        (wchar_t)MAX_);
}
