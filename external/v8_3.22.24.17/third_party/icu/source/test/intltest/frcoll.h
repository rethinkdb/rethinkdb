/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * CollationFrenchTest is a third level test class. This tests the locale
 * specific tertiary rules.  For example, the French secondary sorting on
 * accented characters.
 */
#ifndef _FRCOLL
#define _FRCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationFrenchTest: public IntlTestCollator {
public:
    // If this is too small for the test data, just increase it.
    // Just don't make it too large, otherwise the executable will get too big
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationFrenchTest();
    virtual ~CollationFrenchTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // perform tests with strength SECONDARY
    void TestSecondary(/* char* par */);

    // perform tests with strength TERTIARY
    void TestTertiary(/* char* par */);

    // perform extra tests
    void TestExtra(/* char* par */);

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const UChar testBugs[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];
    static const UChar testAcute[][MAX_TOKEN_LEN];

    Collator *myCollation;
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
