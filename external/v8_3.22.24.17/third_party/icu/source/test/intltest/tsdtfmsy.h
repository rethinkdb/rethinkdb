/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2005, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _INTLTESTDATEFORMATSYMBOLS
#define _INTLTESTDATEFORMATSYMBOLS

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

/**
 * Tests for DateFormatSymbols
 **/
class IntlTestDateFormatSymbols: public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    /**
     * Test the API of DateFormatSymbols; primarily a simple get/set set.
     */
    void TestSymbols(/* char *par */);
    /**
     * Test getMonths.
     */
    void TestGetMonths(void);
    void TestGetMonths2(void);

    void TestGetWeekdays2(void);
    void TestGetEraNames(void);
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
