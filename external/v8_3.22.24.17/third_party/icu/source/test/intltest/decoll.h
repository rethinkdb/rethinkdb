/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * CollationGermanTest is a third level test class.  This tests the locale
 * specific primary, secondary and tertiary rules.  For example, o-umlaut
 * is sorted with expanding char e.
 */
#ifndef _DECOLL
#define _DECOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class CollationGermanTest: public IntlTestCollator {
public:
    // If this is too small for the test data, just increase it.
    // Just don't make it too large, otherwise the executable will get too big
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationGermanTest();
    virtual ~CollationGermanTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // perform test with strength PRIMARY
    void TestPrimary(/* char* par */);

    // perform test with strength SECONDARY
    void TestSecondary(/* char* par */);

    // perform tests with strength TERTIARY
    void TestTertiary(/* char* par */);

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[][2];

    Collator *myCollation;
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
