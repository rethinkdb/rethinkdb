/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _INTLTESTDATEFORMATAPI
#define _INTLTESTDATEFORMATAPI

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"


/*
 * This is an API test, not a unit test.  It doesn't test very many cases, and doesn't
 * try to test the full functionality.  It just calls each function in the class and
 * verifies that it works on a basic level.
 */
class IntlTestDateFormatAPI: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    /**
     * Tests basic functionality of various generic API methods in DateFormat 
     */
    void testAPI(/* char* par */);
    /**
     * Test that the equals method works correctly.
     */
    void TestEquals(void);

    /**
     * Test that no parse or format methods are hidden.
     */
    void TestNameHiding(void);

    /**
     * Add better code coverage.
     */
    void TestCoverage(void);
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
