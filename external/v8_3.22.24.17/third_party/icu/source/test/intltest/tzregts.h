/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1998-2005, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _TIMEZONEREGRESSIONTEST_
#define _TIMEZONEREGRESSIONTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/timezone.h"
#include "unicode/gregocal.h"
#include "unicode/simpletz.h"
#include "intltest.h"



/** 
 * Performs regression test for Calendar
 **/
class TimeZoneRegressionTest: public IntlTest {    
    
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    
    void Test4052967(void);
    void Test4073209(void);
    void Test4073215(void);
    void Test4084933(void);
    void Test4096952(void);
    void Test4109314(void);
    void Test4126678(void);
    void Test4151406(void);
    void Test4151429(void);
    void Test4154537(void);
    void Test4154542(void);
    void Test4154650(void);
    void Test4154525(void);
    void Test4162593(void);
    void Test4176686(void);
    void TestJ186(void);
    void TestJ449(void);
    void TestJDK12API(void);
    void Test4184229(void);
    UBool checkCalendar314(GregorianCalendar *testCal, TimeZone *testTZ);


protected:
    UDate findTransitionBinary(const SimpleTimeZone& tz, UDate min, UDate max);
    UDate findTransitionStepwise(const SimpleTimeZone& tz, UDate min, UDate max);
    UBool failure(UErrorCode status, const char* msg);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _CALENDARREGRESSIONTEST_
//eof
