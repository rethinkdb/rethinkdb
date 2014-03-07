/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2002-2006 International Business Machines Corporation
 * and others. All Rights Reserved.
 *
 ********************************************************************/
/*****************************************************************************
* File stringperf.cpp
*
* Modification History:
* Name                     Description
* Doug Wang                Second version
* Doug Wang                First Version
******************************************************************************
*/

/** 
 * This program tests UnicodeString performance.
 * APIs tested: UnicodeString
 * ICU4C  
 * Windows 2000/XP, Linux
 */

#include "stringperf.h"


int main(int argc, const char *argv[])
{
    UErrorCode status = U_ZERO_ERROR;

    bCatenatePrealloc=TRUE;

    StringPerformanceTest test(argc, argv, status);
    if (U_FAILURE(status)){
        return status;
    }

    int loops = LOOPS;
    if (bCatenatePrealloc) {
        int to_alloc = loops * MAXNUMLINES * (MAXSRCLEN + catenate_STRLEN);
        catICU = new UnicodeString(to_alloc,'a',0);
        //catICU = new UnicodeString();

        catStd = new stlstring();
        catStd -> reserve(loops * MAXNUMLINES * (MAXSRCLEN + catenate_STRLEN));
        //catStd -> reserve(110000000);
    } else {
        catICU = new UnicodeString();
        catStd = new stlstring();
    }

    if (test.run() == FALSE){
        fprintf(stderr, "FAILED: Tests could not be run please check the "
            "arguments.\n");
        return -1;
    }

    delete catICU;
    delete catStd;
    return 0;
}

StringPerformanceTest::StringPerformanceTest(int32_t argc, const char *argv[],
                                             UErrorCode &status)
                                             : UPerfTest(argc, argv, status)
{
    filelines_=NULL;
    StrBuffer=NULL;
    StrBufferLen=0;

    int32_t len =0;

    if (status== U_ILLEGAL_ARGUMENT_ERROR){
        //fprintf(stderr,gUsageString, "stringperf");
        return;
    }
    if (U_FAILURE(status)){
        fprintf(stderr, "FAILED to create UPerfTest object. Error: %s\n", 
            u_errorName(status));
        return;
    }


    if(line_mode){
        ULine* filelines = getLines(status);
        if(U_FAILURE(status)){
            fprintf(stderr, "FAILED to read lines from file and create UPerfTest object. Error: %s\n", u_errorName(status));
            return;
        }

        filelines_ = new ULine[numLines];
        for (int i =0; i < numLines; i++) {
            len = filelines[i].len;
            filelines_[i].name  = new UChar[len];
            filelines_[i].len   = len;
            memcpy(filelines_[i].name, filelines[i].name, len * U_SIZEOF_UCHAR);
        }

    }else if(bulk_mode){
        int32_t srcLen = 0;
        const UChar* src = getBuffer(srcLen,status);
        if(U_FAILURE(status)){
            fprintf(stderr, "FAILED to read buffer from file and create UPerfTest object. Error: %s\n", u_errorName(status));
            return;
        }

        StrBuffer = new UChar[srcLen];
        StrBufferLen = srcLen;
        memcpy(StrBuffer, src, srcLen * U_SIZEOF_UCHAR);

    }
}

StringPerformanceTest::~StringPerformanceTest()
{
    delete[] filelines_;
    delete[] StrBuffer;
}

UPerfFunction* StringPerformanceTest::runIndexedTest(int32_t index, UBool exec,
                                                   const char *&name, 
                                                   char* par) 
{
    switch (index) {
        TESTCASE(0, TestCtor);
        TESTCASE(1, TestCtor1);
        TESTCASE(2, TestCtor2);
        TESTCASE(3, TestCtor3);
        TESTCASE(4, TestAssign);
        TESTCASE(5, TestAssign1);
        TESTCASE(6, TestAssign2);
        TESTCASE(7, TestGetch);
        TESTCASE(8, TestCatenate);
        TESTCASE(9, TestScan);
        TESTCASE(10, TestScan1);
        TESTCASE(11, TestScan2);

        TESTCASE(12, TestStdLibCtor);
        TESTCASE(13, TestStdLibCtor1);
        TESTCASE(14, TestStdLibCtor2);
        TESTCASE(15, TestStdLibCtor3);
        TESTCASE(16, TestStdLibAssign);
        TESTCASE(17, TestStdLibAssign1);
        TESTCASE(18, TestStdLibAssign2);
        TESTCASE(19, TestStdLibGetch);
        TESTCASE(20, TestStdLibCatenate);
        TESTCASE(21, TestStdLibScan);
        TESTCASE(22, TestStdLibScan1);
        TESTCASE(23, TestStdLibScan2);

        default: 
            name = ""; 
            return NULL;
    }
    return NULL;
}

UPerfFunction* StringPerformanceTest::TestCtor()
{
    if (line_mode) {
        return new StringPerfFunction(ctor, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(ctor, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestCtor1()
{
    if (line_mode) {
        return new StringPerfFunction(ctor1, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(ctor1, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestCtor2()
{
    if (line_mode) {
        return new StringPerfFunction(ctor2, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(ctor2, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestCtor3()
{
    if (line_mode) {
        return new StringPerfFunction(ctor3, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(ctor3, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestAssign()
{
    if (line_mode) {
        return new StringPerfFunction(assign, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(assign, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestAssign1()
{
    if (line_mode) {
        return new StringPerfFunction(assign1, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(assign1, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestAssign2()
{
    if (line_mode) {
        return new StringPerfFunction(assign2, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(assign2, StrBuffer, StrBufferLen, uselen);
    }
}


UPerfFunction* StringPerformanceTest::TestGetch()
{
    if (line_mode) {
        return new StringPerfFunction(getch, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(getch, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestCatenate()
{
    if (line_mode) {
        return new StringPerfFunction(catenate, filelines_, numLines, uselen);
    } else {
        //return new StringPerfFunction(catenate, buffer, bufferLen, uselen);
        return new StringPerfFunction(catenate, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestScan()
{
    if (line_mode) {
        return new StringPerfFunction(scan, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(scan, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestScan1()
{
    if (line_mode) {
        return new StringPerfFunction(scan1, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(scan1, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestScan2()
{
    if (line_mode) {
        return new StringPerfFunction(scan2, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(scan2, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibCtor()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibCtor, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibCtor, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibCtor1()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibCtor1, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibCtor1, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibCtor2()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibCtor2, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibCtor2, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibCtor3()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibCtor3, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibCtor3, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibAssign()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibAssign, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibAssign, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibAssign1()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibAssign1, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibAssign1, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibAssign2()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibAssign2, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibAssign2, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibGetch()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibGetch, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibGetch, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibCatenate()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibCatenate, filelines_, numLines, uselen);
    } else {
        //return new StringPerfFunction(StdLibCatenate, buffer, bufferLen, uselen);
        return new StringPerfFunction(StdLibCatenate, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibScan()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibScan, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibScan, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibScan1()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibScan1, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibScan1, StrBuffer, StrBufferLen, uselen);
    }
}

UPerfFunction* StringPerformanceTest::TestStdLibScan2()
{
    if (line_mode) {
        return new StringPerfFunction(StdLibScan2, filelines_, numLines, uselen);
    } else {
        return new StringPerfFunction(StdLibScan2, StrBuffer, StrBufferLen, uselen);
    }
}


