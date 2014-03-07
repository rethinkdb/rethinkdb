/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * CollationKanaTest is a third level test class.  This tests the locale
 * specific tertiary rules.  For example, the term 'A-' (/u3041/u30fc) is 
 * equivalent to 'AA' (/u3041/u3041).
 */

#ifndef _JACOLL
#define _JACOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationKanaTest: public IntlTestCollator {
public:
    // If this is too small for the test data, just increase it.
    // Just don't make it too large, otherwise the executable will get too big
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationKanaTest();
    virtual ~CollationKanaTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // performs test with strength TERIARY
    void TestTertiary(/* char* par */);

    /* Testing base letters */
    void TestBase();

    /* Testing plain, Daku-ten, Handaku-ten letters */
    void TestPlainDakutenHandakuten();

    /* Test Small, Large letters */
    void TestSmallLarge();

    /* Test Katakana, Hiragana letters */
    void TestKatakanaHiragana();

    /* Test Choo-on kigoo */
    void TestChooonKigoo();

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];
    static const UChar testBaseCases[][MAX_TOKEN_LEN];
    static const UChar testPlainDakutenHandakutenCases[][MAX_TOKEN_LEN];
    static const UChar testSmallLargeCases[][MAX_TOKEN_LEN];
    static const UChar testKatakanaHiraganaCases[][MAX_TOKEN_LEN];
    static const UChar testChooonKigooCases[][MAX_TOKEN_LEN];

    Collator *myCollation;
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
