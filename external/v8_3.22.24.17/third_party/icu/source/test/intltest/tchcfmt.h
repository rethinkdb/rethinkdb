
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2008, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * TestChoiceFormat is a third level test class
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"


/**
 * tests Choice Format, functionality of examples, as well as API functionality
 **/
class TestChoiceFormat: public IntlTest {
    /** 
     *    tests basic functionality in a simple example
     **/
    void TestSimpleExample(void); 
    /**
     *    tests functionality in a more complex example,
     *    and extensive API functionality.
     *    See verbose message output statements for specifically tested API
     **/
    void TestComplexExample(void);

    /**
     * Test new closure API
     */
    void TestClosures(void);

    /**
     * Test applyPattern
     */
    void TestPatterns(void);
    void TestChoiceFormatToPatternOverflow(void);

    void _testPattern(const char* pattern,
                      UBool isValid,
                      double v1, const char* str1,
                      double v2, const char* str2,
                      double v3, const char* str3);
    /** 
     *    runs tests in local funtions:
     **/
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

#endif /* #if !UCONFIG_NO_FORMATTING */
