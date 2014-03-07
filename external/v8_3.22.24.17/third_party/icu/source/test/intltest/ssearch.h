/*
 **********************************************************************
 *   Copyright (C) 2005-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __SSEARCH_H
#define __SSEARCH_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/ucol.h"
#include "unicode/bmsearch.h"

#include "intltest.h"

#if !UCONFIG_NO_COLLATION

//
//  Test of the function usearch_search()
//
//     See srchtest.h for the tests for the rest of the string search functions.
//
class SSearchTest: public IntlTest {
public:
  
    SSearchTest();
    virtual ~SSearchTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* params = NULL );
#if !UCONFIG_NO_BREAK_ITERATION

    virtual void searchTest();
    virtual void offsetTest();
    virtual void monkeyTest(char *params);

    virtual void bmMonkeyTest(char *params);
    virtual void boyerMooreTest();
    virtual void goodSuffixTest();
    virtual void searchTime();
    
    virtual void bmsTest();
    virtual void bmSearchTest();

    virtual void udhrTest();

    virtual void stringListTest();
private:
    virtual const char   *getPath(char buffer[2048], const char *filename);
    virtual       int32_t monkeyTestCase(UCollator *coll, const UnicodeString &testCase, const UnicodeString &pattern, const UnicodeString &altPattern,
                                         const char *name, const char *strength, uint32_t seed);

    virtual       int32_t bmMonkeyTestCase(UCollator *coll, const UnicodeString &testCase, const UnicodeString &pattern, const UnicodeString &altPattern,
                                         BoyerMooreSearch *bms, BoyerMooreSearch *abms,
                                         const char *name, const char *strength, uint32_t seed);
#endif
                                         
};

#endif

#endif                                         
