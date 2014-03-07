/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2008-2009 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
/** 
 * This program tests string search performance.
 * APIs tested: 
 * ICU4C 
 */

#include "strsrchperf.h"

StringSearchPerformanceTest::StringSearchPerformanceTest(int32_t argc, const char *argv[], UErrorCode &status)
:UPerfTest(argc,argv,status){
    int32_t start, end;

#ifdef TEST_BOYER_MOORE_SEARCH
    bms = NULL;
#else
    srch = NULL;
#endif

    pttrn = NULL;
    if(status== U_ILLEGAL_ARGUMENT_ERROR || line_mode){
       fprintf(stderr,gUsageString, "strsrchperf");
       return;
    }
    /* Get the Text */
    src = getBuffer(srcLen, status);

#if 0
    /* Get a word to find. Do this by selecting a random word with a word breakiterator. */
    UBreakIterator* brk = ubrk_open(UBRK_WORD, locale, src, srcLen, &status);
    if(U_FAILURE(status)){
        fprintf(stderr, "FAILED to create pattern for searching. Error: %s\n", u_errorName(status));
        return;
    }
    start = ubrk_preceding(brk, 1000);
    end = ubrk_following(brk, start);
    pttrnLen = end - start;
    UChar* temp = (UChar*)malloc(sizeof(UChar)*(pttrnLen));
    for (int i = 0; i < pttrnLen; i++) {
        temp[i] = src[start++];
    }
    pttrn = temp; /* store word in pttrn */
    ubrk_close(brk);
#else
    /* The first line of the file contains the pattern */
    start = 0;

    for(end = start; ; end += 1) {
        UChar ch = src[end];

        if (ch == 0x000A || ch == 0x000D || ch == 0x2028) {
            break;
        }
    }

    pttrnLen = end - start;
    UChar* temp = (UChar*)malloc(sizeof(UChar)*(pttrnLen));
    for (int i = 0; i < pttrnLen; i++) {
        temp[i] = src[start++];
    }
    pttrn = temp; /* store word in pttrn */
#endif
    
#ifdef TEST_BOYER_MOORE_SEARCH
    UnicodeString patternString(pttrn, pttrnLen);
    UCollator *coll = ucol_open(locale, &status);
    CollData *data = CollData::open(coll, status);

    targetString = new UnicodeString(src, srcLen);
    bms = new BoyerMooreSearch(data, patternString, targetString, status);
#else
    /* Create the StringSearch object to be use in performance test. */
    srch = usearch_open(pttrn, pttrnLen, src, srcLen, locale, NULL, &status);
#endif

    if(U_FAILURE(status)){
        fprintf(stderr, "FAILED to create UPerfTest object. Error: %s\n", u_errorName(status));
        return;
    }
    
}

StringSearchPerformanceTest::~StringSearchPerformanceTest() {
    CollData *data  = bms->getData();
    UCollator *coll = data->getCollator();

    delete bms;
    delete targetString;
    CollData::close(data);
    ucol_close(coll);

    if (pttrn != NULL) {
        free(pttrn);
    }

#ifndef TEST_BOYER_MOORE_SEARCH
    if (srch != NULL) {
        usearch_close(srch);
    }
#endif
}

UPerfFunction* StringSearchPerformanceTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char *par) {
    switch (index) {
        TESTCASE(0,Test_ICU_Forward_Search);
        TESTCASE(1,Test_ICU_Backward_Search);

        default: 
            name = ""; 
            return NULL;
    }
    return NULL;
}

UPerfFunction* StringSearchPerformanceTest::Test_ICU_Forward_Search(){
#ifdef TEST_BOYER_MOORE_SEARCH
    StringSearchPerfFunction *func = new StringSearchPerfFunction(ICUForwardSearch, bms, src, srcLen, pttrn, pttrnLen);
#else
    StringSearchPerfFunction* func = new StringSearchPerfFunction(ICUForwardSearch, srch, src, srcLen, pttrn, pttrnLen);
#endif
    return func;
}

UPerfFunction* StringSearchPerformanceTest::Test_ICU_Backward_Search(){
#ifdef TEST_BOYER_MOORE_SEARCH
    StringSearchPerfFunction *func = new StringSearchPerfFunction(ICUBackwardSearch, bms, src, srcLen, pttrn, pttrnLen);
#else
    StringSearchPerfFunction* func = new StringSearchPerfFunction(ICUBackwardSearch, srch, src, srcLen, pttrn, pttrnLen);
#endif
    return func;
}

int main (int argc, const char* argv[]) {
    UErrorCode status = U_ZERO_ERROR;
    StringSearchPerformanceTest test(argc, argv, status);
    if(U_FAILURE(status)){
        return status;
    }
    if(test.run()==FALSE){
        fprintf(stderr,"FAILED: Tests could not be run please check the arguments.\n");
        return -1;
    }
    return 0;
}
