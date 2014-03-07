/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2008-2009 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
#ifndef _STRSRCHPERF_H
#define _STRSRCHPERF_H

#include "unicode/ubrk.h"
#include "unicode/usearch.h"
#include "unicode/colldata.h"
#include "unicode/bmsearch.h"
#include "unicode/uperf.h"
#include <stdlib.h>
#include <stdio.h>

#define TEST_BOYER_MOORE_SEARCH

#ifdef TEST_BOYER_MOORE_SEARCH
typedef void (*StrSrchFn) (BoyerMooreSearch * bms, const UChar *src, int32_t srcLen, const UChar *pttrn, int32_t pttrnLen, UErrorCode *status);
#else
typedef void (*StrSrchFn)(UStringSearch* srch, const UChar* src,int32_t srcLen, const UChar* pttrn, int32_t pttrnLen, UErrorCode* status);
#endif

class StringSearchPerfFunction : public UPerfFunction {
private:
    StrSrchFn fn;
    const UChar* src;
    int32_t srcLen;
    const UChar* pttrn;
    int32_t pttrnLen;
#ifdef TEST_BOYER_MOORE_SEARCH
    BoyerMooreSearch *bms;
#else
    UStringSearch* srch;
#endif
    
public:
    virtual void call(UErrorCode* status) {
#ifdef TEST_BOYER_MOORE_SEARCH
        (*fn)(bms, src, srcLen, pttrn, pttrnLen, status);
#else
        (*fn)(srch, src, srcLen, pttrn, pttrnLen, status);
#endif
    }
    
    virtual long getOperationsPerIteration() {
#if 0
        return (long)(srcLen/pttrnLen);
#else
        return (long) srcLen;
#endif
    }
    
#ifdef TEST_BOYER_MOORE_SEARCH
    StringSearchPerfFunction(StrSrchFn func, BoyerMooreSearch *search, const UChar *source, int32_t sourceLen, const UChar *pattern, int32_t patternLen) {
        fn       = func;
        src      = source;
        srcLen   = sourceLen;
        pttrn    = pattern;
        pttrnLen = patternLen;
        bms      = search;
    }
#else
    StringSearchPerfFunction(StrSrchFn func, UStringSearch* search, const UChar* source,int32_t sourceLen, const UChar* pattern, int32_t patternLen) {
        fn = func;
        src = source;
        srcLen = sourceLen;
        pttrn = pattern;
        pttrnLen = patternLen;
        srch = search;
    }
#endif
};

class StringSearchPerformanceTest : public UPerfTest {
private:
    const UChar* src;
    int32_t srcLen;
    UChar* pttrn;
    int32_t pttrnLen;
#ifdef TEST_BOYER_MOORE_SEARCH
    UnicodeString *targetString;
    BoyerMooreSearch *bms;
#else
    UStringSearch* srch;
#endif
    
public:
    StringSearchPerformanceTest(int32_t argc, const char *argv[], UErrorCode &status);
    ~StringSearchPerformanceTest();
    virtual UPerfFunction* runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = NULL);
    
    UPerfFunction* Test_ICU_Forward_Search();

    UPerfFunction* Test_ICU_Backward_Search();
};


#ifdef TEST_BOYER_MOORE_SEARCH
void ICUForwardSearch(BoyerMooreSearch *bms, const UChar *source, int32_t sourceLen, const UChar *pattern, int32_t patternLen, UErrorCode * /*status*/) { 
    int32_t offset = 0, start = -1, end = -1;

    while (bms->search(offset, start, end)) {
        offset = end;
    }
}

void ICUBackwardSearch(BoyerMooreSearch *bms, const UChar *source, int32_t sourceLen, const UChar *pattern, int32_t patternLen, UErrorCode * /*status*/) { 
    int32_t offset = 0, start = -1, end = -1;

    /* NOTE: No Boyer-Moore backward search yet... */
    while (bms->search(offset, start, end)) {
        offset = end;
    }
}
#else
void ICUForwardSearch(UStringSearch *srch, const UChar* source, int32_t sourceLen, const UChar* pattern, int32_t patternLen, UErrorCode* status) {
    int32_t match;
    
    match = usearch_first(srch, status);
    while (match != USEARCH_DONE) {
        match = usearch_next(srch, status);
    }
}

void ICUBackwardSearch(UStringSearch *srch, const UChar* source, int32_t sourceLen, const UChar* pattern, int32_t patternLen, UErrorCode* status) {
    int32_t match;
    
    match = usearch_last(srch, status);
    while (match != USEARCH_DONE) {
        match = usearch_previous(srch, status);
    }
}
#endif

#endif /* _STRSRCHPERF_H */
