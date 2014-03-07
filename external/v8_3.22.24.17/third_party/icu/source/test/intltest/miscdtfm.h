/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _DATEFORMATMISCTEST_
#define _DATEFORMATMISCTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

/** 
 * Performs miscellaneous tests for DateFormat, SimpleDateFormat, DateFormatSymbols
 **/
class DateFormatMiscTests : public IntlTest {    
    
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

    void test4097450(void);
    void test4099975(void);
    void test4117335(void);

protected:
    UBool failure(UErrorCode status, const char* msg);

};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _DATEFORMATMISCTEST_
//eof
