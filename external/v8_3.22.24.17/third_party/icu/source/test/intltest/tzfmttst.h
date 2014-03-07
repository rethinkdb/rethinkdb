/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#ifndef _TIMEZONEFORMATTEST_
#define _TIMEZONEFORMATTEST_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

class TimeZoneFormatTest : public IntlTest {
    // IntlTest override
    void runIndexedTest(int32_t index, UBool exec, const char*& name, char* par);

    void TestTimeZoneRoundTrip(void);
    void TestTimeRoundTrip(void);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _TIMEZONEFORMATTEST_
