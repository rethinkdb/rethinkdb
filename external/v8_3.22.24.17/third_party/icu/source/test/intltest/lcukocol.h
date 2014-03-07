/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef LOTUSCOLLATIONKOREANTEST_H
#define LOTUSCOLLATIONKOREANTEST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class LotusCollationKoreanTest: public IntlTestCollator {
public:
    // If this is too small for the test data, just increase it.
    // Just don't make it too large, otherwise the executable will get too big
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    LotusCollationKoreanTest();
    virtual ~LotusCollationKoreanTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // performs test with strength TERIARY
    void TestTertiary(/* char* par */);

private:
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];

    Collator *myCollation;
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
