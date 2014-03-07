/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * Normalizer basic tests
 */

#ifndef _TSTNORM
#define _TSTNORM

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/normlzr.h"
#include "intltest.h"

class BasicNormalizerTest : public IntlTest {
public:
    BasicNormalizerTest();
    virtual ~BasicNormalizerTest();

    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    void TestHangulCompose(void);
    void TestHangulDecomp(void);
    void TestPrevious(void);
    void TestDecomp(void);
    void TestCompatDecomp(void);
    void TestCanonCompose(void);
    void TestCompatCompose(void);
    void TestTibetan(void);
    void TestCompositionExclusion(void);
    void TestZeroIndex(void);
    void TestVerisign(void);
    void TestPreviousNext(void);
    void TestNormalizerAPI(void);
    void TestConcatenate(void);
    void TestCompare(void);
    void FindFoldFCDExceptions();
    void TestSkippable();
    void TestCustomComp();
    void TestCustomFCC();
    void TestFilteredNormalizer2Coverage();

private:
    UnicodeString canonTests[24][3];
    UnicodeString compatTests[11][3];
    UnicodeString hangulCanon[2][3];

    void
    TestPreviousNext(const UChar *src, int32_t srcLength,
                     const UChar32 *expext, int32_t expectLength,
                     const int32_t *expectIndex, // its length=expectLength+1
                     int32_t srcMiddle, int32_t expectMiddle,
                     const char *moves,
                     UNormalizationMode mode,
                     const char *name);

    int32_t countFoldFCDExceptions(uint32_t foldingOptions);

    //------------------------------------------------------------------------
    // Internal utilities
    //
    void backAndForth(Normalizer* iter, const UnicodeString& input);

    void staticTest(UNormalizationMode mode, int options,
                    UnicodeString tests[][3], int length, int outCol);

    void iterateTest(Normalizer* iter, UnicodeString tests[][3], int length, int outCol);

    void assertEqual(const UnicodeString& input,
             const UnicodeString& expected, 
             Normalizer* result,
             const UnicodeString& errPrefix);

    static UnicodeString hex(UChar ch);
    static UnicodeString hex(const UnicodeString& str);

};

#endif /* #if !UCONFIG_NO_NORMALIZATION */

#endif // _TSTNORM
