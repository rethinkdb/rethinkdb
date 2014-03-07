/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef CONTRACTIONTABLETEST_H
#define CONTRACTIONTABLETEST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"
#include "ucol_cnt.h"
#include "utrie.h"

class ContractionTableTest: public IntlTestCollator {
public:
    ContractionTableTest();
    virtual ~ContractionTableTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // performs test with strength TERIARY
    void TestGrowTable(/* char* par */);
    void TestSetContraction();
    void TestAddATableElement();
    void TestClone();
    void TestChangeContraction();
    void TestChangeLastCE();
    void TestErrorCodeChecking();
private:
    CntTable *testTable, *testClone;
    /*CompactEIntArray *testMapping;*/
    UNewTrie *testMapping;
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
