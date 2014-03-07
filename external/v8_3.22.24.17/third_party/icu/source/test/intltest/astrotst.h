/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2008, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef __AstroTest__
#define __AstroTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/smpdtfmt.h"
#include "astro.h"
#include "caltztst.h"

class AstroTest: public CalendarTimeZoneTest {
public:
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    AstroTest();

    void TestSolarLongitude(void);

    void TestLunarPosition(void);

    void TestCoordinates(void);

    void TestCoverage(void);

    void TestSunriseTimes(void);

    void TestBasics(void);
    
    void TestMoonAge(void);
 private:
    void initAstro(UErrorCode&);
    void closeAstro(UErrorCode&);
    
    CalendarAstronomer *astro;
    Calendar *gc;
    
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // __AstroTest__
