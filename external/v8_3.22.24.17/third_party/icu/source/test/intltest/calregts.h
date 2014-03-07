/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _CALENDARREGRESSIONTEST_
#define _CALENDARREGRESSIONTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "intltest.h"

/** 
 * Performs regression test for Calendar
 **/
class CalendarRegressionTest: public IntlTest {    
    
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    void test4100311(void);
    void test4074758(void);
    void test4028518(void);
    void test4031502(void) ;
    void test4035301(void) ;
    void test4040996(void) ;
    void test4051765(void) ;
    void test4059654(void) ;
    void test4061476(void) ;
    void test4070502(void) ;
    void test4071197(void) ;
    void test4071385(void) ;
    void test4073929(void) ;
    void test4083167(void) ;
    void test4086724(void) ;
    void test4092362(void) ;
    void test4095407(void) ;
    void test4096231(void) ;
    void test4096539(void) ;
    void test41003112(void) ;
    void test4103271(void) ;
    void test4106136(void) ;
    void test4108764(void) ;
    void test4114578(void) ;
    void test4118384(void) ;
    void test4125881(void) ;
    void test4125892(void) ;
    void test4141665(void) ;
    void test4142933(void) ;
    void test4145158(void) ;
    void test4145983(void) ;
    void test4147269(void) ;

    void Test4149677(void) ;
    void Test4162587(void) ;
    void Test4165343(void) ;
    void Test4166109(void) ;
    void Test4167060(void) ;
    void Test4197699(void);
    void TestJ81(void);
    void TestJ438(void);
    void TestT5555(void);
    void TestT6745(void);
    void TestLeapFieldDifference(void);
    void TestMalaysianInstance(void);
    void TestWeekShift(void);
    void TestTimeZoneTransitionAdd(void);
    void TestDeprecates(void);

    void printdate(GregorianCalendar *cal, const char *string);
    void dowTest(UBool lenient) ;


    static UDate getAssociatedDate(UDate d, UErrorCode& status);
    static UDate makeDate(int32_t y, int32_t m = 0, int32_t d = 0, int32_t hr = 0, int32_t min = 0, int32_t sec = 0);

    static const UDate EARLIEST_SUPPORTED_MILLIS;
    static const UDate LATEST_SUPPORTED_MILLIS;
    static const char* FIELD_NAME[];

protected:
    UBool failure(UErrorCode status, const char* msg);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _CALENDARREGRESSIONTEST_
//eof
