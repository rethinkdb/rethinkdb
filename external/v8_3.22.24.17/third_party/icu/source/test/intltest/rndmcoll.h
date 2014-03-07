/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2005-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * RandomCollatorTest is ported from RandomCollatorTest.java of ICU4J
 */

#ifndef _RANDCOLL
#define _RANDCOLL

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "tscoll.h"

class RandomCollatorTest: public IntlTestCollator {
public:
    virtual void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
    void Test();
    void Test2();
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif /* _RANDCOLL */ 
