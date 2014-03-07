/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ********************************************************************/
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "calregts.h"

#include "unicode/gregocal.h"
#include "unicode/simpletz.h"
#include "unicode/smpdtfmt.h"
#include "unicode/strenum.h"
#include "cmemory.h"
#include "caltest.h"

#include <float.h>

// *****************************************************************************
// class CalendarRegressionTest
// *****************************************************************************

// these numbers correspond to using LONG_MIN and LONG_MAX in Java
// this is 2^52 - 1, the largest allowable mantissa with a 0 exponent in a 64-bit double
const UDate CalendarRegressionTest::EARLIEST_SUPPORTED_MILLIS = - 4503599627370495.0;
const UDate CalendarRegressionTest::LATEST_SUPPORTED_MILLIS    =   4503599627370495.0;

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break

void 
CalendarRegressionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite NumberFormatRegressionTest");
    switch (index) {
        CASE(0,test4100311);
        CASE(1,test4074758);
        CASE(2,test4028518);
        CASE(3,test4031502);
        CASE(4,test4035301);
        CASE(5,test4040996);
        CASE(6,test4051765);
        CASE(7,test4061476);
        CASE(8,test4070502);
        CASE(9,test4071197);
        CASE(10,test4071385);
        CASE(11,test4073929);
        CASE(12,test4083167);
        CASE(13,test4086724);
        CASE(14,test4095407);
        CASE(15,test4096231);
        CASE(16,test4096539);
        CASE(17,test41003112);
        CASE(18,test4103271);
        CASE(19,test4106136);
        CASE(20,test4108764);
        CASE(21,test4114578);
        CASE(22,test4118384);
        CASE(23,test4125881);
        CASE(24,test4125892);
        CASE(25,test4141665);
        CASE(26,test4142933);
        CASE(27,test4145158);
        CASE(28,test4145983);
        CASE(29,test4147269);
        
        CASE(30,Test4149677);
        CASE(31,Test4162587);
        CASE(32,Test4165343);
        CASE(33,Test4166109);
        CASE(34,Test4167060);
        CASE(35,Test4197699);
        CASE(36,TestJ81);
        CASE(37,TestJ438);
        CASE(38,TestLeapFieldDifference);
        CASE(39,TestMalaysianInstance);
        CASE(40,test4059654);
        CASE(41,test4092362);
        CASE(42,TestWeekShift);
        CASE(43,TestTimeZoneTransitionAdd);
        CASE(44,TestDeprecates);
        CASE(45,TestT5555);
        CASE(46,TestT6745);
    default: name = ""; break;
    }
}

const char* CalendarRegressionTest::FIELD_NAME [] = {
    "ERA", 
    "YEAR", 
    "MONTH", 
    "WEEK_OF_YEAR", 
    "WEEK_OF_MONTH", 
    "DAY_OF_MONTH", 
    "DAY_OF_YEAR", 
    "DAY_OF_WEEK", 
    "DAY_OF_WEEK_IN_MONTH", 
    "AM_PM", 
    "HOUR", 
    "HOUR_OF_DAY", 
    "MINUTE", 
    "SECOND", 
    "MILLISECOND", 
    "ZONE_OFFSET", 
    "DST_OFFSET",
    "YEAR_WOY",
    "DOW_LOCAL"
};

UBool 
CalendarRegressionTest::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errcheckln(status, UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}

/*
 * bug 4100311
 */
void 
CalendarRegressionTest::test4100311()
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = (GregorianCalendar*)Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    failure(status, "Calendar::createInstance(status)");
    cal->set(UCAL_YEAR, 1997);
    cal->set(UCAL_DAY_OF_YEAR, 1);
    UDate d = cal->getTime(status);             // Should be Jan 1
    failure(status, "cal->getTime");
    logln(UnicodeString("") + d);
    delete cal;
}


/**
 * @bug 4074758
 */
void
CalendarRegressionTest::test4074758()
{       //Set system time to between 12-1 (am or pm) and then run
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = new GregorianCalendar(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    failure(status, "new GregorianCalendar");
    for (int32_t h=0; h<25; ++h) {
        cal->set(97, UCAL_JANUARY, 1, h, 34);
        //System.out.print(d);
        logln(UnicodeString("HOUR=") + cal->get(UCAL_HOUR, status)); //prints 0
        failure(status, "cal->get");
        logln(UnicodeString("HOUR_OF_DAY=") + cal->get(UCAL_HOUR_OF_DAY, status));
        failure(status, "cal->get");
    }

    delete cal;
}

void
CalendarRegressionTest::test4028518()
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal1 = new GregorianCalendar(status) ;
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal1;
      return;
    }
    failure(status, "new GregorianCalendar");
    GregorianCalendar *cal2 = (GregorianCalendar*) cal1->clone() ;

    printdate(cal1, "cal1: ") ;
    printdate(cal2, "cal2 - cloned(): ") ;
    cal1->add(UCAL_DATE, 1, status) ;
    failure(status, "cal1->add");
    printdate(cal1, "cal1 after adding 1 day:") ;
    printdate(cal2, "cal2 should be unmodified:") ;
    delete cal1;
    delete cal2;
}

void 
CalendarRegressionTest::printdate(GregorianCalendar *cal, const char *string)
{
    UErrorCode status = U_ZERO_ERROR;
    logln(UnicodeString(string, ""));
    log(UnicodeString("") + cal->get(UCAL_MONTH, status)) ;
    failure(status, "cal->get");
    int32_t date = cal->get(UCAL_DATE, status) + 1 ;
    failure(status, "cal->get");
    log(UnicodeString("/") + date) ;
    logln(UnicodeString("/") + cal->get(UCAL_YEAR, status)) ;
    failure(status, "cal->get");
}

/**
 * @bug 4031502
 */
void 
CalendarRegressionTest::test4031502() 
{
    // This bug actually occurs on Windows NT as well, and doesn't
    // require the host zone to be set; it can be set in Java.
    UErrorCode status = U_ZERO_ERROR;
    StringEnumeration* ids = TimeZone::createEnumeration();
    UBool bad = FALSE;
    TimeZone* tz =TimeZone::createTimeZone("Asia/Riyadh87");
    failure(status, "new TimeZone");
    GregorianCalendar *cl = new GregorianCalendar(tz, status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new GregorianCalendar: %s", u_errorName(status));
        delete tz;
        return;
    }
    cl->clear();
    cl->set(1900, 15, 5, 5, 8, 13);
    cl->get(UCAL_HOUR, status);
    failure(status, "cl->get(UCAL_HOUR, status)");
    status = U_ZERO_ERROR;
    delete cl;
    for (int32_t i=0; i<ids->count(status); ++i) {
        TimeZone *zone = TimeZone::createTimeZone(*ids->snext(status));
        GregorianCalendar *cal = new GregorianCalendar(zone, status);
        failure(status, "new GregorianCalendar");
        cal->clear();
        cal->set(1900, 15, 5, 5, 8, 13);
        if (cal->get(UCAL_HOUR, status) != 5 || U_FAILURE(status)) {
            UnicodeString temp;
            logln(zone->getID(temp) + " " +
                               //zone.useDaylightTime() + " " +
                               cal->get(UCAL_DST_OFFSET,status) / (60*60*1000) + " " +
                               zone->getRawOffset() / (60*60*1000) +
                               ": HOUR = " + cal->get(UCAL_HOUR,status));
            bad = TRUE;
        }
        delete cal;
    }
    if (bad) 
        errln("TimeZone problems with GC");
    // delete [] ids;  // TODO: bad APIs
    delete ids;
}

/**
 * @bug 4035301
 */
void CalendarRegressionTest::test4035301() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *c = new GregorianCalendar(98, 8, 7,status);
    GregorianCalendar *d = new GregorianCalendar(98, 8, 7,status);
    if (c->after(*d,status) ||
        c->after(*c,status) ||
        c->before(*d,status) ||
        c->before(*c,status) ||
        *c != *c ||
        *c != *d)
        dataerrln("Fail");
    delete c;
    delete d;
}

/**
 * @bug 4040996
 */
void CalendarRegressionTest::test4040996() 
{
    int32_t count = 0;
    StringEnumeration* ids = TimeZone::createEnumeration(-8 * 60 * 60 * 1000);
    UErrorCode status = U_ZERO_ERROR;    
    count = ids->count(status);
    SimpleTimeZone *pdt = new SimpleTimeZone(-8 * 60 * 60 * 1000, *ids->snext(status));
    pdt->setStartRule(UCAL_APRIL, 1, UCAL_SUNDAY, 2 * 60 * 60 * 1000, status);
    pdt->setEndRule(UCAL_OCTOBER, -1, UCAL_SUNDAY, 2 * 60 * 60 * 1000, status);
    Calendar *calendar = new GregorianCalendar(pdt, status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new GregorianCalendar: %s", u_errorName(status));
        return;
    }
    calendar->set(UCAL_MONTH,3);
    calendar->set(UCAL_DATE,18);
    calendar->set(UCAL_SECOND, 30);

    logln(UnicodeString("MONTH: ") + calendar->get(UCAL_MONTH, status));
    logln(UnicodeString("DAY_OF_MONTH: ") + 
                       calendar->get(UCAL_DATE, status));
    logln(UnicodeString("MINUTE: ") + calendar->get(UCAL_MINUTE, status));
    logln(UnicodeString("SECOND: ") + calendar->get(UCAL_SECOND, status));

    calendar->add(UCAL_SECOND,6, status);
    //This will print out todays date for MONTH and DAY_OF_MONTH
    //instead of the date it was set to.
    //This happens when adding MILLISECOND or MINUTE also
    logln(UnicodeString("MONTH: ") + calendar->get(UCAL_MONTH, status));
    logln(UnicodeString("DAY_OF_MONTH: ") + 
                       calendar->get(UCAL_DATE, status));
    logln(UnicodeString("MINUTE: ") + calendar->get(UCAL_MINUTE, status));
    logln(UnicodeString("SECOND: ") + calendar->get(UCAL_SECOND, status));
    if (calendar->get(UCAL_MONTH, status) != 3 ||
        calendar->get(UCAL_DATE, status) != 18 ||
        calendar->get(UCAL_SECOND, status) != 36)
        errln(UnicodeString("Fail: Calendar::add misbehaves"));

    delete calendar;
    delete ids;
    // delete ids;   // TODO:  BAD API
}

/**
 * @bug 4051765
 */
void CalendarRegressionTest::test4051765() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    cal->setLenient(FALSE);
    cal->set(UCAL_DAY_OF_WEEK, 0);
    //try {
        cal->getTime(status);
        if( ! U_FAILURE(status))
            errln("Fail: DAY_OF_WEEK 0 should be disallowed");
    /*}
    catch (IllegalArgumentException e) {
        return;
    }*/

    delete cal;
}

/* User error - no bug here
void CalendarRegressionTest::test4059524() {
    // Create calendar for April 10, 1997
    GregorianCalendar calendar  = new GregorianCalendar(status);
    // print out a bunch of interesting things
    logln("ERA: " + Calendar::get(Calendar::ERA));
    logln("YEAR: " + Calendar::get(Calendar::YEAR));
    logln("MONTH: " + Calendar::get(Calendar::MONTH));
    logln("WEEK_OF_YEAR: " + 
                       Calendar::get(Calendar::WEEK_OF_YEAR));
    logln("WEEK_OF_MONTH: " + 
                       Calendar::get(Calendar::WEEK_OF_MONTH));
    logln("DATE: " + Calendar::get(Calendar::DATE));
    logln("DAY_OF_MONTH: " + 
                       Calendar::get(Calendar::DAY_OF_MONTH));
    logln("DAY_OF_YEAR: " + Calendar::get(Calendar::DAY_OF_YEAR));
    logln("DAY_OF_WEEK: " + Calendar::get(Calendar::DAY_OF_WEEK));
    logln("DAY_OF_WEEK_IN_MONTH: " +
                       Calendar::get(Calendar::DAY_OF_WEEK_IN_MONTH));
    logln("AM_PM: " + Calendar::get(Calendar::AM_PM));
    logln("HOUR: " + Calendar::get(Calendar::HOUR));
    logln("HOUR_OF_DAY: " + Calendar::get(Calendar::HOUR_OF_DAY));
    logln("MINUTE: " + Calendar::get(Calendar::MINUTE));
    logln("SECOND: " + Calendar::get(Calendar::SECOND));
    logln("MILLISECOND: " + Calendar::get(Calendar::MILLISECOND));
    logln("ZONE_OFFSET: "
                       + (Calendar::get(Calendar::ZONE_OFFSET)/(60*60*1000)));
    logln("DST_OFFSET: "
                       + (Calendar::get(Calendar::DST_OFFSET)/(60*60*1000)));
    calendar  = new GregorianCalendar(1997,3,10); 
    Calendar::getTime();                        
    logln("April 10, 1997");
    logln("ERA: " + Calendar::get(Calendar::ERA));
    logln("YEAR: " + Calendar::get(Calendar::YEAR));
    logln("MONTH: " + Calendar::get(Calendar::MONTH));
    logln("WEEK_OF_YEAR: " + 
                       Calendar::get(Calendar::WEEK_OF_YEAR));
    logln("WEEK_OF_MONTH: " + 
                       Calendar::get(Calendar::WEEK_OF_MONTH));
    logln("DATE: " + Calendar::get(Calendar::DATE));
    logln("DAY_OF_MONTH: " + 
                       Calendar::get(Calendar::DAY_OF_MONTH));
    logln("DAY_OF_YEAR: " + Calendar::get(Calendar::DAY_OF_YEAR));
    logln("DAY_OF_WEEK: " + Calendar::get(Calendar::DAY_OF_WEEK));
    logln("DAY_OF_WEEK_IN_MONTH: " + Calendar::get(Calendar::DAY_OF_WEEK_IN_MONTH));
    logln("AM_PM: " + Calendar::get(Calendar::AM_PM));
    logln("HOUR: " + Calendar::get(Calendar::HOUR));
    logln("HOUR_OF_DAY: " + Calendar::get(Calendar::HOUR_OF_DAY));
    logln("MINUTE: " + Calendar::get(Calendar::MINUTE));
    logln("SECOND: " + Calendar::get(Calendar::SECOND));
    logln("MILLISECOND: " + Calendar::get(Calendar::MILLISECOND));
    logln("ZONE_OFFSET: "
                       + (Calendar::get(Calendar::ZONE_OFFSET)/(60*60*1000))); // in hours
    logln("DST_OFFSET: "
                       + (Calendar::get(Calendar::DST_OFFSET)/(60*60*1000))); // in hours
}
*/

/**
 * @bug 4059654
 */
void CalendarRegressionTest::test4059654() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *gc = new GregorianCalendar(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete gc;
      return;
    }
    
    gc->set(1997, 3, 1, 15, 16, 17); // April 1, 1997

    gc->set(UCAL_HOUR, 0);
    gc->set(UCAL_AM_PM, UCAL_AM);
    gc->set(UCAL_MINUTE, 0);
    gc->set(UCAL_SECOND, 0);
    gc->set(UCAL_MILLISECOND, 0);

    UDate cd = gc->getTime(status);
    GregorianCalendar *exp = new GregorianCalendar(1997, 3, 1, 0, 0, 0, status);
    if (cd != exp->getTime(status))
        errln(UnicodeString("Fail: Calendar::set broken. Got ") + cd + " Want " + exp->getTime(status));

    delete gc;
    delete exp;
}

/**
 * @bug 4061476
 */
void CalendarRegressionTest::test4061476() 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("ddMMMyy"), Locale::getUK(),status);
    Calendar *cal = Calendar::createInstance(TimeZone::createTimeZone("GMT"), 
                                    Locale::getUK(),status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      delete fmt;
      return;
    }
    fmt->adoptCalendar(cal);
    // try {
            UDate date = fmt->parse("29MAY97", status);
            failure(status, "fmt->parse");
            cal->setTime(date, status);
            failure(status, "cal->setTime");
     //   }
    //catch (Exception e) {;}
    cal->set(UCAL_HOUR_OF_DAY, 13);
    logln(UnicodeString("Hour: ")+cal->get(UCAL_HOUR_OF_DAY, status));
    cal->add(UCAL_HOUR_OF_DAY, 6,status);
    logln(UnicodeString("Hour: ")+cal->get(UCAL_HOUR_OF_DAY, status));
    if (cal->get(UCAL_HOUR_OF_DAY, status) != 19)
        errln(UnicodeString("Fail: Want 19 Got ") + cal->get(UCAL_HOUR_OF_DAY, status));

    delete fmt;
}

/**
 * @bug 4070502
 */
void CalendarRegressionTest::test4070502() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = new GregorianCalendar(status);
    if(status == U_USING_FALLBACK_WARNING || U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    UDate d = getAssociatedDate(makeDate(1998,0,30), status);
    cal->setTime(d,status);
    if (cal->get(UCAL_DAY_OF_WEEK,status) == UCAL_SATURDAY ||
        cal->get(UCAL_DAY_OF_WEEK,status) == UCAL_SUNDAY)
        errln(UnicodeString("Fail: Want weekday Got ") + d);

    delete cal;
}

/**
 * Get the associated date starting from a specified date
 * NOTE: the unnecessary "getTime()'s" below are a work-around for a
 * bug in jdk 1.1.3 (and probably earlier versions also)
 * <p>
 * @param date The date to start from
 */
UDate 
CalendarRegressionTest::getAssociatedDate(UDate d, UErrorCode& status) 
{
    GregorianCalendar *cal = new GregorianCalendar(status);
    cal->setTime(d,status);
    //cal.add(field, amount); //<-- PROBLEM SEEN WITH field = DATE,MONTH 
    // cal.getTime();  // <--- REMOVE THIS TO SEE BUG
    for (;;) {
        int32_t wd = cal->get(UCAL_DAY_OF_WEEK, status);
        if (wd == UCAL_SATURDAY || wd == UCAL_SUNDAY) {
            cal->add(UCAL_DATE, 1, status);
            // cal.getTime();
        }
        else
            break;
    }
    
    UDate dd = cal->getTime(status);
    delete cal;
    return dd;
}

/**
 * @bug 4071197
 */
void CalendarRegressionTest::test4071197() 
{
    dowTest(FALSE);
    dowTest(TRUE);
}

void CalendarRegressionTest::dowTest(UBool lenient) 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = new GregorianCalendar(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    cal->set(1997, UCAL_AUGUST, 12); // Wednesday
    // cal.getTime(); // Force update
    cal->setLenient(lenient);
    cal->set(1996, UCAL_DECEMBER, 1); // Set the date to be December 1, 1996
    int32_t dow = cal->get(UCAL_DAY_OF_WEEK, status);
    int32_t min = cal->getMinimum(UCAL_DAY_OF_WEEK);
    int32_t max = cal->getMaximum(UCAL_DAY_OF_WEEK);
    //logln(cal.getTime().toString());
    if (min != UCAL_SUNDAY || max != UCAL_SATURDAY)
        errln("FAIL: Min/max bad");
    if (dow < min || dow > max) 
        errln("FAIL: Day of week %d out of range [%d,%d]\n", dow, min, max);
    if (dow != UCAL_SUNDAY) 
        errln("FAIL: Day of week should be SUNDAY Got " + dow);

    if(U_FAILURE(status)) {
      errln("Error checking Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }

    if(cal->getActualMinimum(UCAL_DAY_OF_WEEK, status) != min) {
        errln("FAIL: actual minimum differs from minimum");
    }
    if(cal->getActualMinimum(Calendar::DAY_OF_WEEK, status) != min) {
        errln("FAIL: actual minimum (Calendar::DAY_OF_WEEK, status) differs from minimum");
    }
    if(cal->getActualMinimum(Calendar::DAY_OF_WEEK) != min) {
        errln("FAIL: actual minimum (Calendar::DAY_OF_WEEK) differs from minimum");
    }
    if(((Calendar*)cal)->getActualMinimum(UCAL_DAY_OF_WEEK, status) != min) {
        errln("FAIL: actual minimum (UCAL_DAY_OF_WEEK, status) differs from minimum");
    }
// NOTE: This function does not exist!  jitterbug #3016
//    if(((Calendar*)cal)->getActualMinimum(Calendar::DAY_OF_WEEK, status) != min) {
//        errln("FAIL: actual minimum (Calendar::DAY_OF_WEEK, status) differs from minimum");
//    }
    if(U_FAILURE(status)) {
      errln("Error getting actual minimum: %s", u_errorName(status));
      return;
    }
    
    delete cal;
}

/**
 * @bug 4071385
 */
void CalendarRegressionTest::test4071385() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    cal->setTime(makeDate(1998, UCAL_JUNE, 24),status);
    cal->set(UCAL_MONTH, UCAL_NOVEMBER); // change a field
    //logln(cal.getTime().toString());
    if (cal->getTime(status) != makeDate(1998, UCAL_NOVEMBER, 24))
        errln("Fail");

    delete cal;
}

/**
 * @bug 4073929
 */
void CalendarRegressionTest::test4073929() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *foo1 = new GregorianCalendar(1997, 8, 27,status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating Calendar: %s", u_errorName(status));
      delete foo1;
      return;
    }
    logln("foo1@%.0f - %d-%d-%d %d:%d:%d.%ds\n", foo1->getTime(status),
          foo1->get(UCAL_YEAR, status),
          foo1->get(UCAL_MONTH, status),
          foo1->get(UCAL_DATE, status),
          foo1->get(UCAL_HOUR, status),
          foo1->get(UCAL_MINUTE, status),
          foo1->get(UCAL_SECOND, status),
          foo1->get(UCAL_MILLISECOND,status));
    foo1->add(UCAL_DATE, + 1, status);
    logln("foo1@%.0f - %d-%d-%d %d:%d:%d.%ds after +\n", foo1->getTime(status),
          foo1->get(UCAL_YEAR, status),
          foo1->get(UCAL_MONTH, status),
          foo1->get(UCAL_DATE, status),
          foo1->get(UCAL_HOUR, status),
          foo1->get(UCAL_MINUTE, status),
          foo1->get(UCAL_SECOND, status),
          foo1->get(UCAL_MILLISECOND ,status));
    foo1->add(UCAL_DATE, - 1, status);
    logln("foo1@%.0f - %d-%d-%d %d:%d:%d.%ds after -\n", foo1->getTime(status),
          foo1->get(UCAL_YEAR, status),
          foo1->get(UCAL_MONTH, status),
          foo1->get(UCAL_DATE, status),
          foo1->get(UCAL_HOUR, status),
          foo1->get(UCAL_MINUTE, status),
          foo1->get(UCAL_SECOND, status),
          foo1->get(UCAL_MILLISECOND, status));

    foo1->add(UCAL_DATE, + 1, status);
    int32_t testyear = foo1->get(UCAL_YEAR, status);
    int32_t testmonth = foo1->get(UCAL_MONTH, status);
    int32_t testday = foo1->get(UCAL_DATE, status);
    if (testyear != 1997 ||
        testmonth != 8 ||
        testday != 28)
        errln("Fail: Calendar not initialized");

    delete foo1;
}

/**
 * @bug 4083167
 */
void CalendarRegressionTest::test4083167() 
{
    UErrorCode status = U_ZERO_ERROR;
    TimeZone *saveZone = TimeZone::createDefault();
    //try {
    TimeZone *newZone = TimeZone::createTimeZone("UTC");
    TimeZone::setDefault(*newZone);
    UDate firstDate = Calendar::getNow();
        Calendar *cal = new GregorianCalendar(status);
        if(U_FAILURE(status)) {
          dataerrln("Error creating Calendar: %s", u_errorName(status));
          delete cal;
          return;
        }
        cal->setTime(firstDate,status);
        int32_t hr        = cal->get(UCAL_HOUR_OF_DAY, status);
        int32_t min        = cal->get(UCAL_MINUTE, status);
        int32_t sec        = cal->get(UCAL_SECOND, status);
        int32_t msec    = cal->get(UCAL_MILLISECOND, status);
        double firstMillisInDay = hr * 3600000 + min * 60000 + sec * 1000 + msec;
        
        //logln("Current time: " + firstDate.toString());

        for (int32_t validity=0; validity<30; validity++) {
            UDate lastDate = firstDate + validity*1000*24*60*60.0;
            cal->setTime(lastDate, status);
            hr        = cal->get(UCAL_HOUR_OF_DAY, status);
            min        = cal->get(UCAL_MINUTE, status);
            sec        = cal->get(UCAL_SECOND, status);
            msec    = cal->get(UCAL_MILLISECOND, status);
            double millisInDay = hr * 3600000.0 + min * 60000.0 + sec * 1000.0 + msec;
            if (firstMillisInDay != millisInDay) 
                errln(UnicodeString("Day has shifted ") + lastDate);
        }
    //}
    //finally {
        TimeZone::setDefault(*saveZone);
    //}

    delete saveZone;
    delete newZone;
    delete cal;
}

/**
 * @bug 4086724
 */
void CalendarRegressionTest::test4086724() 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *date;
    TimeZone *saveZone = TimeZone::createDefault();
    Locale saveLocale = Locale::getDefault();
    //try {
    Locale::setDefault(Locale::getUK(),status); 
    TimeZone *newZone = TimeZone::createTimeZone("GMT");
    TimeZone::setDefault(*newZone);
        date = new SimpleDateFormat(UnicodeString("dd MMM yyy (zzzz) 'is in week' ww"),status); 
        Calendar *cal = Calendar::createInstance(status); 
        if(U_FAILURE(status)) {
          dataerrln("Error creating Calendar: %s", u_errorName(status));
          delete cal;
          delete newZone;
          delete date;
          return;
        }
        cal->set(1997,UCAL_SEPTEMBER,30); 
        UDate now = cal->getTime(status); 
        UnicodeString temp;
        FieldPosition pos(FieldPosition::DONT_CARE);
        logln(date->format(now, temp, pos)); 
        cal->set(1997,UCAL_JANUARY,1); 
        now=cal->getTime(status); 
        logln(date->format(now,temp, pos)); 
        cal->set(1997,UCAL_JANUARY,8); 
        now=cal->getTime(status); 
        logln(date->format(now,temp, pos)); 
        cal->set(1996,UCAL_DECEMBER,31); 
        now=cal->getTime(status); 
        logln(date->format(now,temp, pos)); 
    //}
    //finally {
        Locale::setDefault(saveLocale,status);
        TimeZone::setDefault(*saveZone);
    //}
    logln("*** THE RESULTS OF THIS TEST MUST BE VERIFIED MANUALLY ***");

delete newZone;
delete cal;
delete date;
delete saveZone;
}

/**
 * @bug 4092362
 */
void CalendarRegressionTest::test4092362() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal1 = new GregorianCalendar(1997, 10, 11, 10, 20, 40,status); 
    if (U_FAILURE(status)) {
        dataerrln("Fail new GregorianCalendar: %s", u_errorName(status));
        delete cal1;
        return;
    }
    /*cal1.set( Calendar::YEAR, 1997 ); 
    cal1.set( Calendar::MONTH, 10 ); 
    cal1.set( Calendar::DATE, 11 ); 
    cal1.set( Calendar::HOUR, 10 ); 
    cal1.set( Calendar::MINUTE, 20 ); 
    cal1.set( Calendar::SECOND, 40 ); */

    logln( UnicodeString(" Cal1 = ") + cal1->getTime(status) ); 
    logln( UnicodeString(" Cal1 time in ms = ") + cal1->get(UCAL_MILLISECOND,status) ); 
    for( int32_t k = 0; k < 100 ; k++ ); 

    GregorianCalendar *cal2 = new GregorianCalendar(1997, 10, 11, 10, 20, 40,status); 
    /*cal2.set( Calendar::YEAR, 1997 ); 
    cal2.set( Calendar::MONTH, 10 ); 
    cal2.set( Calendar::DATE, 11 ); 
    cal2.set( Calendar::HOUR, 10 ); 
    cal2.set( Calendar::MINUTE, 20 ); 
    cal2.set( Calendar::SECOND, 40 ); */

    logln( UnicodeString(" Cal2 = ") + cal2->getTime(status) ); 
    logln( UnicodeString(" Cal2 time in ms = ") + cal2->get(UCAL_MILLISECOND,status) ); 
    if( *cal1 != *cal2 ) 
        errln("Fail: Milliseconds randomized");

    delete cal1;
    delete cal2;
}

/**
 * @bug 4095407
 */
void CalendarRegressionTest::test4095407() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *a = new GregorianCalendar(1997,UCAL_NOVEMBER, 13,status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new GregorianCalendar: %s", u_errorName(status));
        delete a;
        return;
    }
    int32_t dow = a->get(UCAL_DAY_OF_WEEK, status);
    if (dow != UCAL_THURSDAY)
        errln("Fail: Want THURSDAY Got " + dow);

    delete a;
}

/**
 * @bug 4096231
 */
void CalendarRegressionTest::test4096231() 
{
    UErrorCode status = U_ZERO_ERROR;
    TimeZone *GMT = TimeZone::createTimeZone("GMT");
    TimeZone *PST = TimeZone::createTimeZone("PST");
    int32_t sec = 0, min = 0, hr = 0, day = 1, month = 10, year = 1997;
                        
    Calendar *cal1 = new GregorianCalendar(*PST,status);
    if (U_FAILURE(status)) {
        dataerrln("Failure new GregorianCalendar: %s", u_errorName(status));
        delete GMT;
        delete PST;
        delete cal1;
        return;
    }
    cal1->setTime(880698639000.0,status);
    // Issue 1: Changing the timezone doesn't change the
    //          represented time.  The old API, pre 1.2.2a requires 
    // setTime to be called in order to update the time fields after the time
    // zone has been set.
    int32_t h1,h2;
    logln(UnicodeString("PST 1 is: ") + (h1=cal1->get(UCAL_HOUR_OF_DAY, status)));
    cal1->setTimeZone(*GMT);
    logln(UnicodeString("GMT 2 is: ") + (h2=cal1->get(UCAL_HOUR_OF_DAY, status)));
    if ((*GMT != *PST) && (h1 == h2))
        errln("Fail: Hour same in different zones");

    Calendar *cal2 = new GregorianCalendar(*GMT,status);
    Calendar *cal3 = new GregorianCalendar(*PST,status);
    cal2->set(UCAL_MILLISECOND, 0);
    cal3->set(UCAL_MILLISECOND, 0);

    cal2->set(cal1->get(UCAL_YEAR,status),
             cal1->get(UCAL_MONTH,status),
             cal1->get(UCAL_DATE,status),
             cal1->get(UCAL_HOUR_OF_DAY,status),
             cal1->get(UCAL_MINUTE,status),
             cal1->get(UCAL_SECOND,status));

    double t1,t2,t3,t4;
    logln(UnicodeString("RGMT 1 is: ") + (t1=cal2->getTime(status)));
    cal3->set(year, month, day, hr, min, sec);
    logln(UnicodeString("RPST 1 is: ") + (t2=cal3->getTime(status)));
    cal3->setTimeZone(*GMT);
    logln(UnicodeString("RGMT 2 is: ") + (t3=cal3->getTime(status)));
    cal3->set(cal1->get(UCAL_YEAR,status),
             cal1->get(UCAL_MONTH,status),
             cal1->get(UCAL_DATE,status),
             cal1->get(UCAL_HOUR_OF_DAY,status),
             cal1->get(UCAL_MINUTE,status),
             cal1->get(UCAL_SECOND,status));
    // Issue 2: Calendar continues to use the timezone in its
    //          constructor for set() conversions, regardless
    //          of calls to setTimeZone()
    logln(UnicodeString("RGMT 3 is: ") + (t4=cal3->getTime(status)));
    if (t1 == t2 ||
        t1 != t4 ||
        t2 != t3)
        errln("Fail: Calendar zone behavior faulty");

    delete cal1;
    delete cal2;
    delete cal3;
    delete GMT;
    delete PST;
}

/**
 * @bug 4096539
 */
void CalendarRegressionTest::test4096539() 
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t y [] = {31,28,31,30,31,30,31,31,30,31,30,31};

    for (int32_t x=0;x<12;x++) {
        GregorianCalendar *gc = new 
            GregorianCalendar(1997,x,y[x], status);
        if (U_FAILURE(status)) {
            dataerrln("Fail new GregorianCalendar: %s", u_errorName(status));
            delete gc;
            return;
        }
        int32_t m1,m2;
        log(UnicodeString("") + (m1=gc->get(UCAL_MONTH,status)+1)+UnicodeString("/")+
                         gc->get(UCAL_DATE,status)+"/"+gc->get(UCAL_YEAR,status)+
                         " + 1mo = ");

        gc->add(UCAL_MONTH, 1,status);
        logln(UnicodeString("") + (m2=gc->get(UCAL_MONTH,status)+1)+UnicodeString("/")+
                           gc->get(UCAL_DATE,status)+"/"+gc->get(UCAL_YEAR,status)
                           );
        int32_t m = (m1 % 12) + 1;
        if (m2 != m)
            errln(UnicodeString("Fail: Want ") + m + " Got " + m2);
        delete gc;
    }
    
}

/**
 * @bug 4100311
 */
void CalendarRegressionTest::test41003112() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = (GregorianCalendar*)Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar: %s", u_errorName(status));
      delete cal;
      return;
    }
    cal->set(UCAL_YEAR, 1997);
    cal->set(UCAL_DAY_OF_YEAR, 1);
    //UDate d = cal->getTime(status);             // Should be Jan 1
    //logln(d.toString());
    if (cal->get(UCAL_DAY_OF_YEAR, status) != 1)
        errln("Fail: DAY_OF_YEAR not set");
    delete cal;
}

/**
 * @bug 4103271
 */
void CalendarRegressionTest::test4103271() 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat sdf(status); 
    int32_t numYears=40, startYear=1997, numDays=15; 
    UnicodeString output, testDesc, str, str2; 
    GregorianCalendar *testCal = (GregorianCalendar*)Calendar::createInstance(status); 
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar: %s", u_errorName(status));
      delete testCal;
      return;
    }
    testCal->clear();
    sdf.adoptCalendar(testCal); 
    sdf.applyPattern("EEE dd MMM yyyy 'WOY'ww'-'YYYY 'DOY'DDD"); 
    UBool fail = FALSE;
    for (int32_t firstDay=1; firstDay<=2; firstDay++) { 
        for (int32_t minDays=1; minDays<=7; minDays++) { 
            testCal->setMinimalDaysInFirstWeek((uint8_t)minDays); 
            testCal->setFirstDayOfWeek((UCalendarDaysOfWeek)firstDay); 
            testDesc = (UnicodeString("Test") + firstDay + minDays); 
            logln(testDesc + " => 1st day of week=" +
                               firstDay +
                               ", minimum days in first week=" +
                               minDays); 
            for (int32_t j=startYear; j<=startYear+numYears; j++) { 
                testCal->set(j,11,25); 
                for(int32_t i=0; i<numDays; i++) { 
                    testCal->add(UCAL_DATE,1,status); 
                    UnicodeString calWOY; 
                    int32_t actWOY = testCal->get(UCAL_WEEK_OF_YEAR,status);
                    if (actWOY < 1 || actWOY > 53) {
                        UDate d = testCal->getTime(status); 
                        //calWOY = String.valueOf(actWOY);
                        UnicodeString temp;
                        FieldPosition pos(FieldPosition::DONT_CARE);
                        output = testDesc + " - " + sdf.format(d,temp,pos) + "\t"; 
                        output = output + "\t" + actWOY; 
                        logln(output); 
                        fail = TRUE;
                    }
                } 
            } 
        } 
    } 

    int32_t DATA [] = {
        3, 52, 52, 52, 52, 52, 52, 52,
            1,  1,  1,  1,  1,  1,  1,
            2,  2,  2,  2,  2,  2,  2,
        4, 52, 52, 52, 52, 52, 52, 52,
           53, 53, 53, 53, 53, 53, 53,
            1,  1,  1,  1,  1,  1,  1,
    };
    testCal->setFirstDayOfWeek(UCAL_SUNDAY);
    for (int32_t j=0; j<44; j+=22) {
        logln(UnicodeString("Minimal days in first week = ") + DATA[j] +
                           "  Week starts on Sunday");
        testCal->setMinimalDaysInFirstWeek((uint8_t)DATA[j]);
        testCal->set(1997, UCAL_DECEMBER, 21);
        for (int32_t i=0; i<21; ++i) {
            int32_t woy = testCal->get(UCAL_WEEK_OF_YEAR,status);
            str.remove();
            log(UnicodeString("") + sdf.format(testCal->getTime(status), str) +
                UnicodeString(" ") + woy);
            if (woy != DATA[j + 1 + i]) {
                log(" ERROR");
                fail = TRUE;
            }
            logln("");
            
            // Now compute the time from the fields, and make sure we
            // get the same answer back.  This is a round-trip test.
            UDate save = testCal->getTime(status);
            testCal->clear();
            testCal->set(UCAL_YEAR_WOY, DATA[j+1+i] < 25 ? 1998 : 1997);
            testCal->set(UCAL_WEEK_OF_YEAR, DATA[j+1+i]);
            testCal->set(UCAL_DAY_OF_WEEK, (i%7) + UCAL_SUNDAY);
            if (testCal->getTime(status) != save) {
                str.remove();
                logln(UnicodeString("  Parse failed: ") +
                      sdf.format(testCal->getTime(status), str));
                fail= TRUE;
            }

            testCal->setTime(save,status);
            testCal->add(UCAL_DATE, 1,status);
        }
    }
    // Test field disambiguation with a few special hard-coded cases.
    // This shouldn't fail if the above cases aren't failing.
    int32_t DISAM_int [] = {
        // y y_woy woy dow
        1997, 1998, 1, UCAL_SUNDAY,
        (1998), (1998), (2), (UCAL_SATURDAY),
        (1998), (1998), (53), (UCAL_THURSDAY),
        (1999), (1998), (53), (UCAL_FRIDAY)
    };

    UDate DISAM_date [] = {
            makeDate(1997, UCAL_DECEMBER, 28),
            makeDate(1998, UCAL_JANUARY, 10),
            makeDate(1998, UCAL_DECEMBER, 31),
            makeDate(1999, UCAL_JANUARY, 1)
    };
    
    testCal->setMinimalDaysInFirstWeek(3);
    testCal->setFirstDayOfWeek(UCAL_SUNDAY);
    int32_t i = 0;

    /* Enable this code to display various WOY values
    testCal->clear();
    for (i=25; i<38; ++i) {
        testCal->set(1996, Calendar::DECEMBER, i);
        UDate got = testCal->getTime(status);
        str.remove();
        logln(UnicodeString("") + sdf.format(got, str));
    }
    for (i=25; i<38; ++i) {
        testCal->set(1997, Calendar::DECEMBER, i);
        UDate got = testCal->getTime(status);
        str.remove();
        logln(UnicodeString("") + sdf.format(got, str));
    }
    for (i=25; i<38; ++i) {
        testCal->set(1998, UCAL_DECEMBER, i);
        UDate got = testCal->getTime(status);
        str.remove();
        logln(UnicodeString("") + sdf.format(got, str));
    }
    */

    for (i=0; i < 16; i += 4) {
        int32_t y = DISAM_int[i];
        int32_t ywoy = DISAM_int[i+1];
        int32_t woy = DISAM_int[i+2];
        int32_t dow = DISAM_int[i+3];
        UDate exp = DISAM_date[i/4];
        testCal->clear();
        testCal->set(UCAL_YEAR, y);
        testCal->set(UCAL_WEEK_OF_YEAR, woy);
        testCal->set(UCAL_DAY_OF_WEEK, dow);
        UDate got = testCal->getTime(status);
        str.remove();
        str2.remove();
        log(UnicodeString("Y") + y + "-W" + woy +
                         "-DOW" + dow + " expect:" + sdf.format(exp, str) +
                         " got:" + sdf.format(got, str2));
        if (got != exp) {
            log("  FAIL (%s:%d, i=%d)", __FILE__, __LINE__, i);
            logln(CalendarTest::calToStr(*testCal));
            testCal->setTime(exp, status);
            logln(CalendarTest::calToStr(*testCal) + UnicodeString( " <<< expected "));
            fail = TRUE;
        }
        logln("");

        testCal->clear();
        testCal->set(UCAL_YEAR_WOY, ywoy);
        testCal->set(UCAL_WEEK_OF_YEAR, woy);
        testCal->set(UCAL_DAY_OF_WEEK, dow);
        got = testCal->getTime(status);
        str.remove();
        str2.remove();
        log(UnicodeString("YWOY") + ywoy + "-W" + woy +
                         "-DOW" + dow + " expect:" + sdf.format(exp, str) +
                         " got:" + sdf.format(got, str2));
        if (got != exp) {
            log("  FAIL");
            fail = TRUE;
        }
        logln("");
    }
    // Now try adding and rolling
    UDate ADDROLL_date [] = {
        makeDate(1998, UCAL_DECEMBER, 25), makeDate(1999, UCAL_JANUARY, 1),
        makeDate(1997, UCAL_DECEMBER, 28), makeDate(1998, UCAL_JANUARY, 4),
        makeDate(1998, UCAL_DECEMBER, 27), makeDate(1997, UCAL_DECEMBER, 28),
        makeDate(1999, UCAL_JANUARY, 2), makeDate(1998, UCAL_JANUARY, 3),
    };

    int32_t ADDROLL_int []= {
        (1),
        (1),
        (1),
        (1)
    };


    UBool ADDROLL_bool [] = {
        TRUE,//ADD,
        TRUE,
        FALSE,
        FALSE
    };

    testCal->setMinimalDaysInFirstWeek(3);
    testCal->setFirstDayOfWeek(UCAL_SUNDAY);
    for (i=0; i<8; i += 2) {
        int32_t amount = ADDROLL_int[i/2];
        UDate before = ADDROLL_date[i];
        UDate after = ADDROLL_date[i+1];

        testCal->setTime(before,status);
        if (ADDROLL_bool[i/2]) 
            testCal->add(UCAL_WEEK_OF_YEAR, amount,status);
        else 
            testCal->roll(UCAL_WEEK_OF_YEAR, amount,status);
        UDate got = testCal->getTime(status);
        str.remove();
        str2.remove();
        UnicodeString opTypeStr;
        if (ADDROLL_bool[i/2]) {
            opTypeStr = UnicodeString("add(WOY,", "");
        } else {
            opTypeStr = UnicodeString("roll(WOY,", "");
        }
        log(opTypeStr + amount + ") " + sdf.format(before, str) + " => " +
                    sdf.format(got, str2));
        if (after != got) {
            str.remove();
            logln(UnicodeString("  exp:") + sdf.format(after, str) + "  FAIL");
            fail = TRUE;
        }
        else logln(" ok");

        testCal->setTime(after,status);
        if (ADDROLL_bool[i/2]) 
            testCal->add(UCAL_WEEK_OF_YEAR, -amount,status);
        else 
            testCal->roll(UCAL_WEEK_OF_YEAR, -amount,status);
        got = testCal->getTime(status);
        str.remove();
        str2.remove();
        log(opTypeStr + (-amount) + ") " + sdf.format(after, str) + " => " +
                sdf.format(got, str2));
        if (before != got) {
            str.remove();
            logln(UnicodeString("  exp:") + sdf.format(before, str) + "  FAIL");
            fail = TRUE;
        }
        else logln(" ok");
    }
    if (fail) 
        errln("Fail: Week of year misbehaving");
} 

/**
 * @bug 4106136
 */
void CalendarRegressionTest::test4106136() 
{
    UErrorCode status = U_ZERO_ERROR;
    Locale saveLocale = Locale::getDefault();
    //try {
    Locale locales [] = { Locale::getChinese(), Locale::getChina() };
        for (int32_t i=0; i<2; ++i) {
            Locale::setDefault(locales[i], status);
            failure(status, "Locale::setDefault");
            int32_t count1, count2, count3;
            Calendar::getAvailableLocales(count1);
            DateFormat::getAvailableLocales(count2);
            NumberFormat::getAvailableLocales(count3);
            int32_t n [] = {
                count1, count2, count3
            };
            for (int32_t j=0; j<3; ++j) {
                UnicodeString temp;
                if (n[j] == 0)
                    dataerrln(UnicodeString("Fail: No locales for ") + locales[i].getName());
            }
        }
    //}
    //finally {
        Locale::setDefault(saveLocale,status);
    //}
}

/**
 * @bug 4108764
 */
void CalendarRegressionTest::test4108764() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status); 
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    UDate d00 = makeDate(1997, UCAL_MARCH, 15, 12, 00, 00);
    UDate d01 = makeDate(1997, UCAL_MARCH, 15, 12, 00, 56);
    UDate d10 = makeDate(1997, UCAL_MARCH, 15, 12, 34, 00);
    UDate d11 = makeDate(1997, UCAL_MARCH, 15, 12, 34, 56);
    UDate epoch = makeDate(1970, UCAL_JANUARY, 1);

    cal->setTime(d11,status);

    cal->clear( UCAL_MINUTE ); 
    logln(UnicodeString("") + cal->getTime(status)); 
    if (cal->getTime(status)  != d01)
        errln("Fail: clear(MINUTE) broken");

    cal->set( UCAL_SECOND, 0 ); 
    logln(UnicodeString("") + cal->getTime(status)); 
    if (cal->getTime(status)  != d00)
        errln("Fail: set(SECOND, 0) broken");

    cal->setTime(d11,status);
    cal->set( UCAL_SECOND, 0 ); 
    logln(UnicodeString("") + cal->getTime(status)); 
    if (cal->getTime(status)  != d10)
        errln("Fail: set(SECOND, 0) broken #2");

    cal->clear( UCAL_MINUTE ); 
    logln(UnicodeString("") + cal->getTime(status)); 
    if (cal->getTime(status)  != d00)
        errln("Fail: clear(MINUTE) broken #2");

    cal->clear();
    logln(UnicodeString("") + cal->getTime(status)); 
    if (cal->getTime(status)  != epoch)
        errln(UnicodeString("Fail: clear() broken Want ") + epoch);

    delete cal;
}

/**
 * @bug 4114578
 */
void CalendarRegressionTest::test4114578() 
{
    UErrorCode status = U_ZERO_ERROR;
    double ONE_HOUR = 60*60*1000;
    Calendar *cal = Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    cal->adoptTimeZone(TimeZone::createTimeZone("PST"));
    UDate onset = makeDate(1998, UCAL_APRIL, 5, 1, 0) + ONE_HOUR;
    UDate cease = makeDate(1998, UCAL_OCTOBER, 25, 0, 0) + 2*ONE_HOUR;

    UBool fail = FALSE;
    
    const int32_t ADD = 1;
    const int32_t ROLL = 2;

    double DATA []= {
        // Start            Action   Amt    Expected_change
        onset - ONE_HOUR,   ADD,      1,     ONE_HOUR,
        onset,              ADD,     -1,    -ONE_HOUR,
        onset - ONE_HOUR,   ROLL,     1,     ONE_HOUR,
        onset,              ROLL,    -1,    -ONE_HOUR,
        cease - ONE_HOUR,   ADD,      1,     ONE_HOUR,
        cease,              ADD,     -1,    -ONE_HOUR,
        cease - ONE_HOUR,   ROLL,     1,     ONE_HOUR,
        cease,              ROLL,    -1,    -ONE_HOUR,
    };

    for (int32_t i=0; i<32; i+=4) {
        UDate date = DATA[i];
        int32_t amt = (int32_t) DATA[i+2];
        double expectedChange = DATA[i+3];
        
        log(UnicodeString("") + date);
        cal->setTime(date,status);

        switch ((int32_t) DATA[i+1]) {
        case ADD:
            log(UnicodeString(" add (HOUR,") + (amt<0?"":"+")+amt + ")= ");
            cal->add(UCAL_HOUR, amt,status);
            break;
        case ROLL:
            log(UnicodeString(" roll(HOUR,") + (amt<0?"":"+")+amt + ")= ");
            cal->roll(UCAL_HOUR, amt,status);
            break;
        }

        log(UnicodeString("") + cal->getTime(status));

        double change = cal->getTime(status) - date;
        if (change != expectedChange) {
            fail = TRUE;
            logln(" FAIL");
        }
        else logln(" OK");
    }

    if (fail) errln("Fail: roll/add misbehaves around DST onset/cease");

    delete cal;
}

/**
 * @bug 4118384
 * Make sure maximum for HOUR field is 11, not 12.
 */
void CalendarRegressionTest::test4118384() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    if (cal->getMaximum(UCAL_HOUR) != 11 ||
        cal->getLeastMaximum(UCAL_HOUR) != 11 ||
        cal->getActualMaximum(UCAL_HOUR,status) != 11)
        errln("Fail: maximum of HOUR field should be 11");

    // test deprecated functions
    if (cal->getLeastMaximum(Calendar::HOUR) != 11 ||
        cal->getMaximum(Calendar::HOUR) != 11) {
        errln("Fail: [deprecated functions] maximum of HOUR field should be 11\n");
    }

    if (cal->getGreatestMinimum(Calendar::HOUR) != 0 ||
        cal->getMinimum(Calendar::HOUR) != 0) {
        errln("Fail: [deprecated functions] minimum of HOUR field should be 1\n");
    }

    delete cal;
    cal = Calendar::createInstance(Locale("th_TH@calendar=buddhist"),status);
    // test deprecated functions
    if (cal->getLeastMaximum(Calendar::HOUR) != 11 ||
        cal->getMaximum(Calendar::HOUR) != 11) {
        errln("Fail: Buddhist:[deprecated functions] maximum of HOUR field should be 11\n");
    }

    if (cal->getGreatestMinimum(Calendar::HOUR) != 0 ||
        cal->getMinimum(Calendar::HOUR) != 0) {
        errln("Fail: Buddhist:[deprecated functions] minimum of HOUR field should be 1\n");
    }

    delete cal;
    // test deprecated functions
    cal = Calendar::createInstance(Locale("ja_JP@calendar=japanese"),status);
    if (cal->getLeastMaximum(Calendar::HOUR) != 11 ||
        cal->getMaximum(Calendar::HOUR) != 11) {
        errln("Fail: Japanese:[deprecated functions] maximum of HOUR field should be 11\n");
    }

    if (cal->getGreatestMinimum(Calendar::HOUR) != 0 ||
        cal->getMinimum(Calendar::HOUR) != 0) {
        errln("Fail: Japanese:[deprecated functions] minimum of HOUR field should be 1\n");
    }

    delete cal;
}

/**
 * @bug 4125881
 * Check isLeapYear for BC years.
 */
void CalendarRegressionTest::test4125881() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = (GregorianCalendar*) Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    DateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"),status);
    if(!assertSuccess("trying to construct", status))return;
    cal->clear();
    for (int32_t y=-20; y<=10; ++y) {
        cal->set(UCAL_ERA, y < 1 ? GregorianCalendar::BC : GregorianCalendar::AD);
        cal->set(UCAL_YEAR, y < 1 ? 1 - y : y);
        UnicodeString temp;
        logln(UnicodeString("") + y + UnicodeString(" = ") + fmt->format(cal->getTime(status), temp) + " " +
                           cal->isLeapYear(y));
        if (cal->isLeapYear(y) != ((y+40)%4 == 0))
            errln("Leap years broken");
    }

    delete cal;
    delete fmt;
}

/**
 * @bug 4125892
 * Prove that GregorianCalendar is proleptic (it used to cut off
 * at 45 BC, and not have leap years before then).
 */
void CalendarRegressionTest::test4125892() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = (GregorianCalendar*) Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    DateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"),status);
    if(!assertSuccess("trying to construct", status))return;
    cal->clear();
    cal->set(UCAL_ERA, GregorianCalendar::BC);
    cal->set(UCAL_YEAR, 81); // 81 BC is a leap year (proleptically)
    cal->set(UCAL_MONTH, UCAL_FEBRUARY);
    cal->set(UCAL_DATE, 28);
    cal->add(UCAL_DATE, 1,status);
    if(U_FAILURE(status))
        errln("add(DATE,1) failed");
    if (cal->get(UCAL_DATE,status) != 29 ||
        !cal->isLeapYear(-80)) // -80 == 81 BC
        errln("Calendar not proleptic");

    delete cal;
    delete fmt;
}

/**
 * @bug 4141665
 * GregorianCalendar::equals() ignores cutover date
 */
void CalendarRegressionTest::test4141665() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *cal = new GregorianCalendar(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    GregorianCalendar *cal2 = (GregorianCalendar*)cal->clone();
    UDate cut = cal->getGregorianChange();
    UDate cut2 = cut + 100*24*60*60*1000.0; // 100 days later
    if (*cal != *cal2) {
        errln("Cloned GregorianCalendars not equal");
    }
    cal2->setGregorianChange(cut2,status);
    if ( *cal == *cal2) {
        errln("GregorianCalendar::equals() ignores cutover");
    }

    delete cal;
    delete cal2;
}

/**
 * @bug 4142933
 * Bug states that ArrayIndexOutOfBoundsException is thrown by GregorianCalendar::roll()
 * when IllegalArgumentException should be.
 */
void CalendarRegressionTest::test4142933() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *calendar = new GregorianCalendar(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete calendar;
      return;
    }
    //try {
    calendar->roll((UCalendarDateFields)-1, TRUE, status);
        if(U_SUCCESS(status))
            errln("Test failed, no exception thrown");
    //}
    //catch (IllegalArgumentException e) {
        // OK: Do nothing
        // logln("Test passed");
    //}
    //catch (Exception e) {
        //errln("Test failed. Unexpected exception is thrown: " + e);
        //e.printStackTrace();
    //} 

    delete calendar;
}

/**
 * @bug 4145158
 * GregorianCalendar handling of Dates Long.MIN_VALUE and Long.MAX_VALUE is
 * confusing; unless the time zone has a raw offset of zero, one or the
 * other of these will wrap.  We've modified the test given in the bug
 * report to therefore only check the behavior of a calendar with a zero raw
 * offset zone.
 */
void CalendarRegressionTest::test4145158() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *calendar = new GregorianCalendar(status);
    if(status == U_USING_FALLBACK_WARNING || U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete calendar;
      return;
    }

    calendar->adoptTimeZone(TimeZone::createTimeZone("GMT"));

    calendar->setTime(makeDate(INT32_MIN),status);
    int32_t year1 = calendar->get(UCAL_YEAR,status);
    int32_t era1 = calendar->get(UCAL_ERA,status);

    calendar->setTime(makeDate(INT32_MAX),status);
    int32_t year2 = calendar->get(UCAL_YEAR,status);
    int32_t era2 = calendar->get(UCAL_ERA,status);
    
    if (year1 == year2 && era1 == era2) {
        errln("Fail: Long.MIN_VALUE or Long.MAX_VALUE wrapping around");
    }

    delete calendar;
}

/**
 * @bug 4145983
 * Maximum value for YEAR field wrong.
 */
// {sfb} this is not directly applicable in C++, since all
// possible doubles are not representable by our Calendar.
// In Java, all longs are representable.  
// We can determine limits programmatically
// Using DBL_MAX is a bit of a hack, since for large doubles
// Calendar gets squirrely and doesn't behave in any sort
// of linear fashion (ie years jump around, up/down, etc) for a
// small change in millis.
void CalendarRegressionTest::test4145983() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *calendar = new GregorianCalendar(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete calendar;
      return;
    }
    calendar->adoptTimeZone(TimeZone::createTimeZone("GMT"));
    UDate DATES [] = { LATEST_SUPPORTED_MILLIS, EARLIEST_SUPPORTED_MILLIS };
    for (int32_t i=0; i<2; ++i) {
        calendar->setTime(DATES[i], status);
        int32_t year = calendar->get(UCAL_YEAR,status);
        int32_t maxYear = calendar->getMaximum(UCAL_YEAR);
        if (year > maxYear) {
            errln(UnicodeString("Failed for ")+DATES[i]+" ms: year=" +
                  year + ", maxYear=" + maxYear);
        }
    }

    delete calendar;
}

/**
 * @bug 4147269
 * This is a bug in the validation code of GregorianCalendar::  As reported,
 * the bug seems worse than it really is, due to a bug in the way the bug
 * report test was written.  In reality the bug is restricted to the DAY_OF_YEAR
 * field. - liu 6/29/98
 */
void CalendarRegressionTest::test4147269() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *calendar = new GregorianCalendar(status);
    if(status == U_USING_FALLBACK_WARNING || U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete calendar;
      return;
    }
    calendar->setLenient(FALSE);
    UDate date = makeDate(1996, UCAL_JANUARY, 3); // Arbitrary date
    for (int32_t field = 0; field < UCAL_FIELD_COUNT; field++) {
        calendar->setTime(date,status);
        // Note: In the bug report, getActualMaximum() was called instead
        // of getMaximum() -- this was an error.  The validation code doesn't
        // use getActualMaximum(), since that's too costly.
        int32_t max = calendar->getMaximum((UCalendarDateFields)field);
        int32_t value = max+1;
        calendar->set((UCalendarDateFields)field, value); 
        //try {
            calendar->getTime(status); // Force time computation
            // We expect an exception to be thrown. If we fall through
            // to the next line, then we have a bug.
            if(U_SUCCESS(status))
            errln(UnicodeString("Test failed with field ") + FIELD_NAME[field] +
                  ", date before: " + date +
                  ", date after: " + calendar->getTime(status) +
                  ", value: " + value + " (max = " + max +")");
        //} catch (IllegalArgumentException e) {} 
    }

    delete calendar;
}

/**
 * @bug 4149677
 * Reported bug is that a GregorianCalendar with a cutover of Date(Long.MAX_VALUE)
 * doesn't behave as a pure Julian calendar.
 * CANNOT REPRODUCE THIS BUG
 */
void 
CalendarRegressionTest::Test4149677() 
{
    UErrorCode status = U_ZERO_ERROR;

    TimeZone *zones [] = { 
        TimeZone::createTimeZone("GMT"),
        TimeZone::createTimeZone("PST"),
        TimeZone::createTimeZone("EAT") 
    };
    if(U_FAILURE(status)) {
        errln("Couldn't create zones");
        return;
        // could leak memory
    }

    for (int32_t i=0; i < 3; ++i) {
        GregorianCalendar *calendar = new GregorianCalendar(zones[i], status);
        if(U_FAILURE(status)) {
            dataerrln("Couldnt' create calendar.: %s", u_errorName(status));
            return;
        }

        // Make sure extreme values don't wrap around
        calendar->setTime(EARLIEST_SUPPORTED_MILLIS, status);
        if(U_FAILURE(status))
            errln("setTime failed");
        if (calendar->get(UCAL_ERA, status) != GregorianCalendar::BC || U_FAILURE(status)) {
            errln("Fail: Date(EARLIEST_SUPPORTED_MILLIS) has an AD year");
        }
        calendar->setTime(LATEST_SUPPORTED_MILLIS, status);
        if(U_FAILURE(status))
            errln("setTime failed");
        if (calendar->get(UCAL_ERA, status) != GregorianCalendar::AD || U_FAILURE(status)) {
            errln("Fail: Date(LATEST_SUPPORTED_MILLIS) has a BC year");
        }

        calendar->setGregorianChange(LATEST_SUPPORTED_MILLIS, status);
        if(U_FAILURE(status))
            errln("setGregorianChange failed");
        // to obtain a pure Julian calendar
        
        UBool is100Leap = calendar->isLeapYear(100);
        if (!is100Leap) {
            UnicodeString temp;
            errln("test failed with zone " + zones[i]->getID(temp));
            errln(" cutover date is Date(Long.MAX_VALUE)");
            errln(" isLeapYear(100) returns: " + is100Leap);
        }
        delete calendar;
    }
    
    // no need for cleanup- zones were adopted
}

/**
 * @bug 4162587
 * Calendar and Date HOUR broken.  If HOUR is out-of-range, Calendar
 * and Date classes will misbehave.
 */
void 
CalendarRegressionTest::Test4162587() 
{
    UErrorCode status = U_ZERO_ERROR;
    TimeZone *savedef = TimeZone::createDefault();
    TimeZone *tz = TimeZone::createTimeZone("PST");
    //TimeZone::adoptDefault(tz);
    TimeZone::setDefault(*tz);
    
    GregorianCalendar *cal = new GregorianCalendar(tz, status);
    if(U_FAILURE(status)) {
        dataerrln("Couldn't create calendar.: %s", u_errorName(status));
        return;
    }
    UDate d0, dPlus, dMinus;
    
    for(int32_t i=0; i<5; ++i) {
        if (i>0) logln("---");

        cal->clear();
        cal->set(1998, UCAL_APRIL, 5, i, 0);
        d0 = cal->getTime(status);
        if(U_FAILURE(status))
            errln("Coudln't get time (1)");
        //String s0 = d.toString();
        logln(UnicodeString("0 ") + i + ": " + d0/*s0*/);

        cal->clear();
        cal->set(1998, UCAL_APRIL, 4, i+24, 0);
        dPlus = cal->getTime(status);
        if(U_FAILURE(status))
            errln("Coudln't get time (2)");
        //String sPlus = d.toString();
        logln(UnicodeString("+ ") + i + ": " + dPlus/*sPlus*/);

        cal->clear();
        cal->set(1998, UCAL_APRIL, 6, i-24, 0);
        dMinus = cal->getTime(status);
        if(U_FAILURE(status))
            errln("Coudln't get time (3)");
        //String sMinus = d.toString();
        logln(UnicodeString("- ") + i + ": " + dMinus/*sMinus*/);

        if (d0 != dPlus || d0 != dMinus) {
            errln("Fail: All three lines must match");
        }
    }
    TimeZone::setDefault(*savedef);
    //delete tz;
    delete cal;
    delete savedef;
}

/**
 * @bug 4165343
 * Adding 12 months behaves differently from adding 1 year
 */
void 
CalendarRegressionTest::Test4165343() 
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *calendar = new GregorianCalendar(1996, UCAL_FEBRUARY, 29, status);
    if(U_FAILURE(status)) {
        dataerrln("Couldn't create calendar.: %s", u_errorName(status));
        return;
    }
    UDate start = calendar->getTime(status);
    if(U_FAILURE(status))
        errln("Couldn't getTime (1)");
    logln(UnicodeString("init date: ") + start);
    calendar->add(UCAL_MONTH, 12, status); 
    if(U_FAILURE(status))
        errln("Couldn't add(MONTH, 12)");
    UDate date1 = calendar->getTime(status);
    if(U_FAILURE(status))
        errln("Couldn't getTime (2)");
    logln(UnicodeString("after adding 12 months: ") + date1);
    calendar->setTime(start, status);
    if(U_FAILURE(status))
        errln("Couldn't setTime");
    calendar->add(UCAL_YEAR, 1, status);
    if(U_FAILURE(status))
        errln("Couldn't add(YEAR, 1)");
    UDate date2 = calendar->getTime(status);
    if(U_FAILURE(status))
        errln("Couldn't getTime (3)");
    logln(UnicodeString("after adding one year : ") + date2);
    if (date1 == date2) {
        logln("Test passed");
    } else {
        errln("Test failed");
    }
    delete calendar;
}

/**
 * @bug 4166109
 * GregorianCalendar.getActualMaximum() does not account for first day of week.
 */
void 
CalendarRegressionTest::Test4166109() 
{
    /* Test month:
     *
     *      March 1998
     * Su Mo Tu We Th Fr Sa
     *  1  2  3  4  5  6  7
     *  8  9 10 11 12 13 14
     * 15 16 17 18 19 20 21
     * 22 23 24 25 26 27 28
     * 29 30 31
     */
    UBool passed = TRUE;
    UErrorCode status = U_ZERO_ERROR;
    UCalendarDateFields field = UCAL_WEEK_OF_MONTH;

    GregorianCalendar *calendar = new GregorianCalendar(Locale::getUS(), status);
    if(U_FAILURE(status)) {
        dataerrln("Couldn't create calendar.: %s", u_errorName(status));
        return;
    }
    calendar->set(1998, UCAL_MARCH, 1);
    calendar->setMinimalDaysInFirstWeek(1);
    logln(UnicodeString("Date:  ") + calendar->getTime(status)); // 888817448000
    
    int32_t firstInMonth = calendar->get(UCAL_DATE, status);
    if(U_FAILURE(status))
        errln("get(D_O_M) failed");

    for(int32_t firstInWeek = UCAL_SUNDAY; firstInWeek <= UCAL_SATURDAY; firstInWeek++) {
        calendar->setFirstDayOfWeek((UCalendarDaysOfWeek)firstInWeek);
        int32_t returned = calendar->getActualMaximum(field, status);
        int32_t expected = (31 + ((firstInMonth - firstInWeek + 7)% 7) + 6) / 7;

        logln(UnicodeString("First day of week = ") + firstInWeek +
              "  getActualMaximum(WEEK_OF_MONTH, status) = " + returned +
              "  expected = " + expected +
              ((returned == expected) ? "  ok" : "  FAIL"));

        if (returned != expected) {
            passed = FALSE;
        }
    }
    if (!passed) {
        errln("Test failed");
    }

    delete calendar;
}

/**
 * @bug 4167060
 * Calendar.getActualMaximum(YEAR) works wrong.
 */
void 
CalendarRegressionTest::Test4167060() 
{
    UErrorCode status = U_ZERO_ERROR;
    UCalendarDateFields field = UCAL_YEAR;
    DateFormat *format = new SimpleDateFormat(UnicodeString("EEE MMM dd HH:mm:ss zzz yyyy G"),
        Locale::getUS(), status);
    if(U_FAILURE(status)) {
        dataerrln("Couldn't create SimpleDateFormat - %s", u_errorName(status));
        return;
    }

    GregorianCalendar *calendars [] = {
        new GregorianCalendar(100, UCAL_NOVEMBER, 1, status),
        new GregorianCalendar(-99 /*100BC*/, UCAL_JANUARY, 1, status),
        new GregorianCalendar(1996, UCAL_FEBRUARY, 29, status),
    };
    if(U_FAILURE(status)) {
        errln("Couldn't create GregorianCalendars");
        return;
        // could leak
    }

    UnicodeString id [] = { "Hybrid", "Gregorian", "Julian" };

    for (int32_t k=0; k<3; ++k) {
        logln("--- " + id[k] + " ---");

        for (int32_t j=0; j < 3; ++j) {
            GregorianCalendar *calendar = calendars[j];
            if (k == 1) {
                calendar->setGregorianChange(EARLIEST_SUPPORTED_MILLIS, status);
            } 
            else if (k == 2) {
                calendar->setGregorianChange(LATEST_SUPPORTED_MILLIS, status);
            }

            if(U_FAILURE(status))
                errln("setGregorianChange() failed");
            format->adoptCalendar((Calendar*)calendar->clone());

            UDate dateBefore = calendar->getTime(status);
            if(U_FAILURE(status))
                errln("getTime() failed");

            int32_t maxYear = calendar->getActualMaximum(field, status);
            UnicodeString temp;
            logln(UnicodeString("maxYear: ") + maxYear + " for " + format->format(calendar->getTime(status), temp));
            temp.remove();
            logln("date before: " + format->format(dateBefore, temp));

            int32_t years[] = {2000, maxYear-1, maxYear, maxYear+1};

            for (int32_t i = 0; i < 4; i++) {
                UBool valid = years[i] <= maxYear;
                calendar->set(field, years[i]);
                UDate dateAfter = calendar->getTime(status);
                if(U_FAILURE(status))
                    errln("getTime() failed");
                int32_t newYear = calendar->get(field, status);
                if(U_FAILURE(status))
                    errln(UnicodeString("get(") + (int32_t)field + ") failed");
                calendar->setTime(dateBefore, status); // restore calendar for next use
                if(U_FAILURE(status))
                    errln("setTime() failed");

                temp.remove();
                logln(UnicodeString(" Year ") + years[i] + (valid? " ok " : " bad") +
                      " => " + format->format(dateAfter, temp));
                if (valid && newYear != years[i]) {
                    errln(UnicodeString("  FAIL: ") + newYear + " should be valid; date, month and time shouldn't change");
                } 
                // {sfb} this next line is a hack, but it should work since if a
                // double has an exponent, adding 1 should not yield the same double
                else if (!valid && /*newYear == years[i]*/ dateAfter + 1.0 == dateAfter)  {
                    errln(UnicodeString("  FAIL: ") + newYear + " should be invalid");
                }
            }
        }
    }

    delete format;
    delete calendars[0];
    delete calendars[1];
    delete calendars[2];
}

/**
 * Week of year is wrong at the start and end of the year.
 */
void CalendarRegressionTest::Test4197699() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar cal(status);
    cal.setFirstDayOfWeek(UCAL_MONDAY);
    cal.setMinimalDaysInFirstWeek(4);
    SimpleDateFormat fmt("E dd MMM yyyy  'DOY='D 'WOY='w",
                         Locale::getUS(), status);
    fmt.setCalendar(cal);
    if (U_FAILURE(status)) {
        dataerrln("Couldn't initialize test - %s", u_errorName(status));
        return;
    }

    int32_t DATA[] = {
        2000,  UCAL_JANUARY,   1,   52,
        2001,  UCAL_DECEMBER,  31,  1,
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    UnicodeString str;
    DateFormat& dfmt = *(DateFormat*)&fmt;
    for (int32_t i=0; i<DATA_length; ) {
        cal.clear();
        cal.set(DATA[i], DATA[i+1], DATA[i+2]);
        i += 3;
        int32_t expWOY = DATA[i++];
        int32_t actWOY = cal.get(UCAL_WEEK_OF_YEAR, status);
        if (expWOY == actWOY) {
            logln(UnicodeString("Ok: ") + dfmt.format(cal.getTime(status), str.remove()));
        } else {
            errln(UnicodeString("FAIL: ") + dfmt.format(cal.getTime(status), str.remove())
                  + ", expected WOY=" + expWOY);
            cal.add(UCAL_DATE, -8, status);
            for (int j=0; j<14; ++j) {
                cal.add(UCAL_DATE, 1, status);
                logln(dfmt.format(cal.getTime(status), str.remove()));
            }
        }
        if (U_FAILURE(status)) {
            errln("FAIL: Unexpected error from Calendar");
            return;
        }
    }
}

    enum Action { ADD=1, ROLL=2 };
    enum Sign { PLUS=1, MINUS=2 };

#define     ONE_HOUR (60*60*1000)
#define ONE_DAY (24*ONE_HOUR)

    typedef struct {
        UCalendarDateFields field;
        int8_t actionMask; // ADD or ROLL or both
        int8_t signMask; // PLUS or MINUS or both
        int32_t amount;
        int32_t before; // ms before cutover
        int32_t after;  // ms after cutover
    } J81_DATA;

/**
 * Rolling and adding across the Gregorian cutover should work as expected.
 * Jitterbug 81.
 */
void CalendarRegressionTest::TestJ81() {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString temp, temp2, temp3;
    int32_t i;
    GregorianCalendar cal(TimeZone::createTimeZone("GMT"), Locale::getUS(), status);
    SimpleDateFormat fmt("HH:mm 'w'w 'd'D E d MMM yyyy", Locale::getUS(), status);
    if (U_FAILURE(status)) {
        dataerrln("Error: Cannot create calendar or format - %s", u_errorName(status));
        return;
    }
    fmt.setCalendar(cal);
    // Get the Gregorian cutover
    UDate cutover = cal.getGregorianChange();
    UDate days = ONE_DAY;
    days = cutover/days;
    logln(UnicodeString("Cutover: {") +
          fmt.format(cutover, temp) + "}(epoch days-" + (int)days + ", jd" + (2440588 + days) +")");

    // Check woy and doy handling.  Reference data:
    /* w40 d274 Mon 1 Oct 1582
       w40 d275 Tue 2 Oct 1582
       w40 d276 Wed 3 Oct 1582
       w40 d277 Thu 4 Oct 1582
       w40 d278 Fri 15 Oct 1582
       w40 d279 Sat 16 Oct 1582
       w41 d280 Sun 17 Oct 1582
       w41 d281 Mon 18 Oct 1582
       w41 d282 Tue 19 Oct 1582
       w41 d283 Wed 20 Oct 1582
       w41 d284 Thu 21 Oct 1582
       w41 d285 Fri 22 Oct 1582
       w41 d286 Sat 23 Oct 1582
       w42 d287 Sun 24 Oct 1582
       w42 d288 Mon 25 Oct 1582
       w42 d289 Tue 26 Oct 1582
       w42 d290 Wed 27 Oct 1582
       w42 d291 Thu 28 Oct 1582
       w42 d292 Fri 29 Oct 1582
       w42 d293 Sat 30 Oct 1582
       w43 d294 Sun 31 Oct 1582
       w43 d295 Mon 1 Nov 1582 */
    int32_t DOY_DATA[] = {
        // dom, woy, doy
        1, 40, 274, UCAL_MONDAY,
        4, 40, 277, UCAL_THURSDAY,
        15, 40, 278, UCAL_FRIDAY,
        17, 41, 280, UCAL_SUNDAY,
        24, 42, 287, UCAL_SUNDAY,
        25, 42, 288, UCAL_MONDAY,
        26, 42, 289, UCAL_TUESDAY,
        27, 42, 290, UCAL_WEDNESDAY,
        28, 42, 291, UCAL_THURSDAY,
        29, 42, 292, UCAL_FRIDAY,
        30, 42, 293, UCAL_SATURDAY,
        31, 43, 294, UCAL_SUNDAY
    };
    int32_t DOY_DATA_length = (int32_t)(sizeof(DOY_DATA) / sizeof(DOY_DATA[0]));

    for (i=0; i<DOY_DATA_length; i+=4) {
        // Test time->fields
        cal.set(1582, UCAL_OCTOBER, DOY_DATA[i]);
        int32_t woy = cal.get(UCAL_WEEK_OF_YEAR, status);
        int32_t doy = cal.get(UCAL_DAY_OF_YEAR, status);
        int32_t dow = cal.get(UCAL_DAY_OF_WEEK, status);
        if (U_FAILURE(status)) {
            errln("Error: get() failed");
            break;
        }
        if (woy != DOY_DATA[i+1] || doy != DOY_DATA[i+2] || dow != DOY_DATA[i+3]) {
            errln((UnicodeString)"Fail: expect woy=" + DOY_DATA[i+1] +
                  ", doy=" + DOY_DATA[i+2] + ", dow=" + DOY_DATA[i+3] + " on " +
                  fmt.format(cal.getTime(status), temp.remove()) +
                  " set(1582,OCTOBER, " + DOY_DATA[i] + ")");
            logln(CalendarTest::calToStr(cal));
            status = U_ZERO_ERROR;
        }  else {
          logln((UnicodeString)"PASS: expect woy=" + DOY_DATA[i+1] +
                ", doy=" + DOY_DATA[i+2] + ", dow=" + DOY_DATA[i+3] + " on " +
                fmt.format(cal.getTime(status), temp.remove()));
          logln(CalendarTest::calToStr(cal));
          status = U_ZERO_ERROR;
        }
        // Test fields->time for WOY
        cal.clear();
        cal.set(UCAL_YEAR, 1582);
        cal.set(UCAL_WEEK_OF_YEAR, DOY_DATA[i+1]);
        cal.set(UCAL_DAY_OF_WEEK, DOY_DATA[i+3]);
        int32_t dom = cal.get(UCAL_DATE, status);
        if (U_FAILURE(status)) {
            errln("Error: get() failed");
            break;
        }
        if (dom != DOY_DATA[i]) {
            errln((UnicodeString)"Fail: set woy=" + DOY_DATA[i+1] +
                  " dow=" + DOY_DATA[i+3] + " => " +
                  fmt.format(cal.getTime(status), temp.remove()) +
                  ", expected 1582 Oct " + DOY_DATA[i]);
            logln(CalendarTest::calToStr(cal));
            status = U_ZERO_ERROR;
        }

        // Test fields->time for DOY
        cal.clear();
        cal.set(UCAL_YEAR, 1582);
        cal.set(UCAL_DAY_OF_YEAR, DOY_DATA[i+2]);
        dom = cal.get(UCAL_DATE, status);
        if (U_FAILURE(status)) {
            errln("Error: get() failed");
            break;
        }
        if (dom != DOY_DATA[i]) {
            errln((UnicodeString)"Fail: set doy=" + DOY_DATA[i+2] +
                  " => " +
                  fmt.format(cal.getTime(status), temp.remove()) +
                  ", expected 1582 Oct " + DOY_DATA[i]);
            status = U_ZERO_ERROR;
        }
    }
    status = U_ZERO_ERROR;

#define ADD_ROLL  ADD|ROLL
#define PLUS_MINUS PLUS|MINUS
    // Test cases
    J81_DATA DATA[] = {
        { UCAL_WEEK_OF_YEAR, ADD_ROLL, PLUS_MINUS, 1, -ONE_DAY, +6*ONE_DAY },
        { UCAL_WEEK_OF_YEAR, ADD_ROLL, PLUS_MINUS, 1, -ONE_DAY, +6*ONE_DAY },
        { UCAL_WEEK_OF_MONTH, ADD|ROLL, PLUS|MINUS, 1, -ONE_DAY, +6*ONE_DAY },
        { UCAL_DATE, ADD|ROLL, PLUS|MINUS, 2, -ONE_DAY, +1*ONE_DAY },
        { UCAL_DATE, ROLL, PLUS, -6, -ONE_DAY, +14*ONE_DAY },
        { UCAL_DATE, ROLL, PLUS, -7, 0, +14*ONE_DAY },
        { UCAL_DATE, ROLL, PLUS, -7, +ONE_DAY, +15*ONE_DAY },
        { UCAL_DATE, ROLL, PLUS, +18, -ONE_DAY, -4*ONE_DAY },
        { UCAL_DAY_OF_YEAR, ADD|ROLL, PLUS|MINUS, 2, -ONE_DAY, +1*ONE_DAY },
        { UCAL_DAY_OF_WEEK, ADD|ROLL, PLUS|MINUS, 2, -ONE_DAY, +1*ONE_DAY },
        { UCAL_DAY_OF_WEEK_IN_MONTH, ADD|ROLL, PLUS|MINUS, 1, -ONE_DAY, +6*ONE_DAY },
        { UCAL_AM_PM, ADD, PLUS|MINUS, 4, -12*ONE_HOUR, +36*ONE_HOUR },
        { UCAL_HOUR, ADD, PLUS|MINUS, 48, -12*ONE_HOUR, +36*ONE_HOUR },
        { UCAL_HOUR_OF_DAY, ADD, PLUS|MINUS, 48, -12*ONE_HOUR, +36*ONE_HOUR },
        { UCAL_MINUTE, ADD, PLUS|MINUS, 48*60, -12*ONE_HOUR, +36*ONE_HOUR },
        { UCAL_SECOND, ADD, PLUS|MINUS, 48*60*60, -12*ONE_HOUR, +36*ONE_HOUR },
        { UCAL_MILLISECOND, ADD, PLUS|MINUS, 48*ONE_HOUR, -12*ONE_HOUR, +36*ONE_HOUR },
        // NOTE: These are not supported yet.  See jitterbug 180.
        // Uncomment these lines when add/roll supported on these fields.
        // { Calendar::YEAR_WOY, ADD|ROLL, 1, -ONE_DAY, +6*ONE_DAY },
        // { Calendar::DOW_LOCAL, ADD|ROLL, 2, -ONE_DAY, +1*ONE_DAY }
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    // Now run the tests
    for (i=0; i<DATA_length; ++i) {
        for (Action action=ADD; action<=ROLL; action=(Action)(action+1)) {
            if (!(DATA[i].actionMask & action)) {
                continue;
            }
            for (Sign sign=PLUS; sign<=MINUS; sign=(Sign)(sign+1)) {
                if (!(DATA[i].signMask & sign)) {
                    continue;
                }
                status = U_ZERO_ERROR;
                int32_t amount = DATA[i].amount * (sign==MINUS?-1:1);
                UDate date = cutover + 
                    (sign==PLUS ? DATA[i].before : DATA[i].after);
                UDate expected = cutover + 
                    (sign==PLUS ? DATA[i].after : DATA[i].before);
                cal.setTime(date, status);
                if (U_FAILURE(status)) {
                    errln((UnicodeString)"FAIL: setTime returned error code " + u_errorName(status));
                    continue;
                }
                if (action == ADD) {
                    cal.add(DATA[i].field, amount, status);
                } else {
                    cal.roll(DATA[i].field, amount, status);
                }
                if (U_FAILURE(status)) {
                    errln((UnicodeString)"FAIL: " +
                          (action==ADD?"add ":"roll ") + FIELD_NAME[DATA[i].field] +
                          " returned error code " + u_errorName(status));
                    continue;
                }
                UDate result = cal.getTime(status);
                if (U_FAILURE(status)) {
                    errln((UnicodeString)"FAIL: getTime returned error code " + u_errorName(status));
                    continue;
                }
                if (result == expected) {
                    logln((UnicodeString)"Ok: {" + 
                          fmt.format(date, temp.remove()) +
                          "}(" + date/ONE_DAY +
                          (action==ADD?") add ":") roll ") +                  
                          amount + " " + FIELD_NAME[DATA[i].field] + " -> {" +
                          fmt.format(result, temp2.remove()) +
                          "}(" + result/ONE_DAY + ")");                  
                } else {
                    errln((UnicodeString)"FAIL: {" + 
                          fmt.format(date, temp.remove()) +
                          "}(" + date/ONE_DAY +
                          (action==ADD?") add ":") roll ") +                  
                          amount + " " + FIELD_NAME[DATA[i].field] + " -> {" +
                          fmt.format(result, temp2.remove()) +
                          "}(" + result/ONE_DAY + "), expect {" +
                          fmt.format(expected, temp3.remove()) +
                          "}(" + expected/ONE_DAY + ")");
                }
            }
        }
    }
}
        
/**
 * Test fieldDifference().
 */
void CalendarRegressionTest::TestJ438(void) {
    UErrorCode ec = U_ZERO_ERROR;
    int32_t DATA[] = {
        2000, UCAL_JANUARY, 20,   2010, UCAL_JUNE, 15,
        2010, UCAL_JUNE, 15,      2000, UCAL_JANUARY, 20,
        1964, UCAL_SEPTEMBER, 7,  1999, UCAL_JUNE, 4,
        1999, UCAL_JUNE, 4,       1964, UCAL_SEPTEMBER, 7,
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA)/sizeof(DATA[0]));
    Calendar* pcal = Calendar::createInstance(Locale::getUS(), ec);
    if(U_FAILURE(ec)) {
      dataerrln("Error creating calendar %s", u_errorName(ec));
      delete pcal;
      return;
    }
    Calendar& cal = *pcal;
    int32_t i;
    SimpleDateFormat fmt(UnicodeString("MMM dd yyyy",""), ec);
    fmt.setCalendar(cal);
    UnicodeString s, t, u;
    if (!failure(ec, "setup")) {
        for (i=0; i<DATA_length; i+=6) {
            int32_t y1 = DATA[i];
            int32_t m1 = DATA[i+1];
            int32_t d1 = DATA[i+2];
            int32_t y2 = DATA[i+3];
            int32_t m2 = DATA[i+4];
            int32_t d2 = DATA[i+5];

            cal.clear();
            cal.set(y1, m1, d1);
            UDate date1 = cal.getTime(ec);
            if (failure(ec, "getTime"))
                break;
            cal.set(y2, m2, d2);
            UDate date2 = cal.getTime(ec);
            if (failure(ec, "getTime"))
                break;

            cal.setTime(date1, ec);
            if (failure(ec, "setTime"))
                break;
            int32_t dy = cal.fieldDifference(date2, UCAL_YEAR, ec);
            int32_t dm = cal.fieldDifference(date2, UCAL_MONTH, ec);
            int32_t dd = cal.fieldDifference(date2, UCAL_DATE, ec);
            if (failure(ec, "fieldDifference"))
                break;

            { 
                Calendar *cal2 = cal.clone();
                UErrorCode ec2 = U_ZERO_ERROR;

                cal2->setTime(date1, ec2);                

                int32_t dy2 = cal2->fieldDifference(date2, Calendar::YEAR, ec2);
                int32_t dm2 = cal2->fieldDifference(date2, Calendar::MONTH, ec2);
                int32_t dd2 = cal2->fieldDifference(date2, Calendar::DATE, ec2);
                if (failure(ec2, "fieldDifference(date, Calendar::DATE, ec)"))
                    break;
                if( (dd2 != dd) ||
                    (dm2 != dm) ||
                    (dy2 != dy)){
                    errln("fieldDifference(UCAL_...) and fieldDifference(Calendar::...) give different results!\n");
                }
                delete cal2;
            }


            logln(UnicodeString("") +
                  fmt.format(date2, s.remove()) + " - " +
                  fmt.format(date1, t.remove()) + " = " +
                  dy + "y " + dm + "m " + dd + "d");

            cal.setTime(date1, ec);
            if (failure(ec, "setTime"))
                break;
            cal.add(UCAL_YEAR, dy, ec);
            cal.add(UCAL_MONTH, dm, ec);
            cal.add(UCAL_DATE, dd, ec);
            if (failure(ec, "add"))
                break;
            UDate date22 = cal.getTime(ec);
            if (failure(ec, "getTime"))
                break;
            if (date2 != date22) {
                errln(UnicodeString("FAIL: ") +
                      fmt.format(date1, s.remove()) + " + " +
                      dy + "y " + dm + "m " + dd + "d = " +
                      fmt.format(date22, t.remove()) + ", exp " +
                      fmt.format(date2, u.remove()));
            } else {
                logln(UnicodeString("Ok: ") +
                      fmt.format(date1, s.remove()) + " + " +
                      dy + "y " + dm + "m " + dd + "d = " +
                      fmt.format(date22, t.remove()));
            }
        }
    }
    delete pcal;
}

void CalendarRegressionTest::TestT5555()
{
    UErrorCode ec = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(ec);

    if (cal == NULL || U_FAILURE(ec)) {
        dataerrln("FAIL: Calendar::createInstance(): %s", u_errorName(ec));
        delete cal;
        return;
    }

    // Set to Wednesday, February 21, 2007
    cal->set(2007, UCAL_FEBRUARY, 21);

    // Advance three years
    cal->add(UCAL_MONTH, 36, ec);

    // Set to last Wednesday of the month
    cal->set(UCAL_DAY_OF_WEEK_IN_MONTH, -1);

    cal->getTime(ec);

    int32_t yy, mm, dd, ee;

    yy = cal->get(UCAL_YEAR, ec);
    mm = cal->get(UCAL_MONTH, ec);
    dd = cal->get(UCAL_DATE, ec);
    ee = cal->get(UCAL_DAY_OF_WEEK, ec);

    // Should be set to Wednesday, February 24, 2010
    if (U_FAILURE(ec) || yy != 2010 || mm != UCAL_FEBRUARY || dd != 24 || ee != UCAL_WEDNESDAY) {
        errln("FAIL: got date %4d/%02d/%02d, expected 210/02/24: ", yy, mm + 1, dd);
    }
    delete cal;
}

typedef struct {
    int32_t             startYear;
    int32_t             startMonth; // 0-based
    int32_t             startDay;   // 1-based
    UCalendarDateFields fieldToChange;
    int32_t             fieldDelta;
    int32_t             endYear;
    int32_t             endMonth;   // 0-based
    int32_t             endDay;     // 1-based
} CoptEthCalTestItem;

// year 1724 in coptic calendar =
// year 2000 in ethiopic calendar (276 more than coptic) =
// year 7500 in ethiopic-amete-alem calendar (5776 more than coptic)
// (2007-2008 in gregorian calendar depending on month)
static const CoptEthCalTestItem coptEthCalTestItems[] = {
    { 1724, 12, 1, UCAL_MONTH, +1, 1725,  0, 1 },
    { 1724, 12, 1, UCAL_MONTH, +9, 1725,  8, 1 },
    { 1723, 12, 2, UCAL_MONTH, +1, 1724,  0, 2 }, // 1723 is a leap year
    { 1723, 12, 2, UCAL_MONTH, +9, 1724,  8, 2 },
    { 1725,  0, 1, UCAL_MONTH, -1, 1724, 12, 1 },
    { 1725,  0, 1, UCAL_MONTH, -6, 1724,  7, 1 },
    { 1724, 12, 1, UCAL_DATE,  +8, 1725,  0, 4 },
    { 1723, 12, 1, UCAL_DATE,  +8, 1724,  0, 3 }, // 1723 is a leap year
    { 1724,  0, 1, UCAL_DATE,  -1, 1723, 12, 6 }, // 1723 is a leap year
    { 0, 0, 0, (UCalendarDateFields)0, 0, 0, 0, 0 } // terminator
};

typedef struct {
    const char * locale;
    int32_t      yearOffset;
} CoptEthCalLocale;

static const CoptEthCalLocale copEthCalLocales[] = {
    { "en@calendar=coptic",   0    },
    { "en@calendar=ethiopic", 276  },
    { NULL,                   0    } // terminator
};

void CalendarRegressionTest::TestT6745()
{
    const CoptEthCalLocale * testLocalePtr;
    for ( testLocalePtr = copEthCalLocales; testLocalePtr->locale != NULL; ++testLocalePtr) {
        UErrorCode status = U_ZERO_ERROR;
        Calendar *cal = Calendar::createInstance(Locale(testLocalePtr->locale), status);
        if ( U_FAILURE(status) ) {
            dataerrln((UnicodeString)"FAIL: Calendar::createInstance, locale " + testLocalePtr->locale + ", status " + u_errorName(status));
            continue;
        }
        const CoptEthCalTestItem * testItemPtr;
        for (testItemPtr = coptEthCalTestItems; testItemPtr->fieldDelta != 0; ++testItemPtr) {
            status = U_ZERO_ERROR;
            cal->set( testItemPtr->startYear + testLocalePtr->yearOffset, testItemPtr->startMonth, testItemPtr->startDay, 9, 0 );
            cal->add( testItemPtr->fieldToChange, testItemPtr->fieldDelta, status );
            if ( U_FAILURE(status) ) {
                errln((UnicodeString)"FAIL: Calendar::add, locale " + testLocalePtr->locale + ", field/delta " +
                        testItemPtr->fieldToChange + "/" + testItemPtr->fieldDelta + ", status " + u_errorName(status));
                continue;
            }
            int32_t endYear = testItemPtr->endYear + testLocalePtr->yearOffset;
            int32_t year  = cal->get(UCAL_YEAR, status);
            int32_t month = cal->get(UCAL_MONTH, status);
            int32_t day   = cal->get(UCAL_DATE, status);
            if ( U_FAILURE(status) || year != endYear || month != testItemPtr->endMonth || day != testItemPtr->endDay ) {
                errln((UnicodeString)"ERROR: Calendar::add, locale " + testLocalePtr->locale + ", field/delta " +
                        testItemPtr->fieldToChange + "/" + testItemPtr->fieldDelta + ", status " + u_errorName(status) +
                        ", expected " + endYear + "/" + testItemPtr->endMonth + "/" + testItemPtr->endDay +
                        ", got " + year + "/" + month + "/" + day );
            }
        }
        delete cal;
    }
}

/**
 * Test behavior of fieldDifference around leap years.  Also test a large
 * field difference to check binary search.
 */
void CalendarRegressionTest::TestLeapFieldDifference() {
    UErrorCode ec = U_ZERO_ERROR;
    Calendar* cal = Calendar::createInstance(ec);
    if (cal == NULL || U_FAILURE(ec)) {
        dataerrln("FAIL: Calendar::createInstance(): %s", u_errorName(ec));
        delete cal;
        return;
    }
    cal->set(2004, UCAL_FEBRUARY, 29);
    UDate date2004 = cal->getTime(ec);
    cal->set(2000, UCAL_FEBRUARY, 29);
    UDate date2000 = cal->getTime(ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: getTime()");
        delete cal;
        return;
    }
    int32_t y = cal->fieldDifference(date2004, UCAL_YEAR, ec);
    int32_t d = cal->fieldDifference(date2004, UCAL_DAY_OF_YEAR, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: fieldDifference()");
        delete cal;
        return;
    }
    if (d == 0) {
        logln((UnicodeString)"Ok: 2004/Feb/29 - 2000/Feb/29 = " + y + " years, " + d + " days");
    } else {
        errln((UnicodeString)"FAIL: 2004/Feb/29 - 2000/Feb/29 = " + y + " years, " + d + " days");
    }
    cal->setTime(date2004, ec);
    y = cal->fieldDifference(date2000, UCAL_YEAR, ec);
    d = cal->fieldDifference(date2000, UCAL_DAY_OF_YEAR, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: setTime() / fieldDifference()");
        delete cal;
        return;
    }
    if (d == 0) {
        logln((UnicodeString)"Ok: 2000/Feb/29 - 2004/Feb/29 = " + y + " years, " + d + " days");
    } else {
        errln((UnicodeString)"FAIL: 2000/Feb/29 - 2004/Feb/29 = " + y + " years, " + d + " days");
    }
    // Test large difference
    cal->set(2001, UCAL_APRIL, 5); // 2452005
    UDate ayl = cal->getTime(ec);
    cal->set(1964, UCAL_SEPTEMBER, 7); // 2438646
    UDate asl = cal->getTime(ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: getTime()");
        delete cal;
        return;
    }
    d = cal->fieldDifference(ayl, UCAL_DATE, ec);
    cal->setTime(ayl, ec);
    int32_t d2 = cal->fieldDifference(asl, UCAL_DATE, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: setTime() / fieldDifference()");
        delete cal;
        return;
    }
    if (d == -d2 && d == 13359) {
        logln((UnicodeString)"Ok: large field difference symmetrical " + d);
    } else {
        logln((UnicodeString)"FAIL: large field difference incorrect " + d + ", " + d2 +
              ", expect +/- 13359");
    }
    delete cal;
}

/**
 * Test ms_MY "Malay (Malaysia)" locale.  Bug 1543.
 */
void CalendarRegressionTest::TestMalaysianInstance() {
    Locale loc("ms", "MY");  // Malay (Malaysia)
    UErrorCode ec = U_ZERO_ERROR;
    Calendar* cal = Calendar::createInstance(loc, ec);
    if (U_FAILURE(ec)) {
        dataerrln("FAIL: Can't construct calendar for ms_MY: %s", u_errorName(ec));
    }
    delete cal;
}

/**
 * setFirstDayOfWeek and setMinimalDaysInFirstWeek may change the
 * field <=> time mapping, since they affect the interpretation of
 * the WEEK_OF_MONTH or WEEK_OF_YEAR fields.
 */
void CalendarRegressionTest::TestWeekShift() {
    UErrorCode ec = U_ZERO_ERROR;
    GregorianCalendar cal(TimeZone::createTimeZone("America/Los_Angeles"),
                          Locale("en", "US"), ec);
    if (U_FAILURE(ec)) {
        dataerrln("Fail GregorianCalendar: %s", u_errorName(ec));
        return;
    }
    cal.setTime(UDate(997257600000.0), ec); // Wed Aug 08 01:00:00 PDT 2001
    // In pass one, change the first day of week so that the weeks
    // shift in August 2001.  In pass two, change the minimal days
    // in the first week so that the weeks shift in August 2001.
    //     August 2001     
    // Su Mo Tu We Th Fr Sa
    //           1  2  3  4
    //  5  6  7  8  9 10 11
    // 12 13 14 15 16 17 18
    // 19 20 21 22 23 24 25
    // 26 27 28 29 30 31   
    for (int32_t pass=0; pass<2; ++pass) {
        if (pass==0) {
            cal.setFirstDayOfWeek(UCAL_WEDNESDAY);
            cal.setMinimalDaysInFirstWeek(4);
        } else {
            cal.setFirstDayOfWeek(UCAL_SUNDAY);
            cal.setMinimalDaysInFirstWeek(4);
        }
        cal.add(UCAL_DATE, 1, ec); // Force recalc
        cal.add(UCAL_DATE, -1, ec);

        UDate time1 = cal.getTime(ec); // Get time -- should not change

        // Now change a week parameter and then force a recalc.
        // The bug is that the recalc should not be necessary --
        // calendar should do so automatically.
        if (pass==0) {
            cal.setFirstDayOfWeek(UCAL_THURSDAY);
        } else {
            cal.setMinimalDaysInFirstWeek(5);
        }

        int32_t woy1 = cal.get(UCAL_WEEK_OF_YEAR, ec);
        int32_t wom1 = cal.get(UCAL_WEEK_OF_MONTH, ec);

        cal.add(UCAL_DATE, 1, ec); // Force recalc
        cal.add(UCAL_DATE, -1, ec);

        int32_t woy2 = cal.get(UCAL_WEEK_OF_YEAR, ec);
        int32_t wom2 = cal.get(UCAL_WEEK_OF_MONTH, ec);

        UDate time2 = cal.getTime(ec);

        if (U_FAILURE(ec)) {
            errln("FAIL: internal test error");
            return;
        }

        if (time1 != time2) {
            errln("FAIL: shifting week should not alter time");
        } else {
            // logln(time1);
        }
        if (woy1 == woy2 && wom1 == wom2) {
            logln((UnicodeString)"Ok: WEEK_OF_YEAR: " + woy1 +
                  ", WEEK_OF_MONTH: " + wom1);
        } else {
            errln((UnicodeString)"FAIL: WEEK_OF_YEAR: " + woy1 + " => " + woy2 +
                  ", WEEK_OF_MONTH: " + wom1 + " => " + wom2 +
                  " after week shift");
        }
    }
}

/**
 * Make sure that when adding a day, we actually wind up in a
 * different day.  The DST adjustments we use to keep the hour
 * constant across DST changes can backfire and change the day.
 */
void CalendarRegressionTest::TestTimeZoneTransitionAdd() {
    UErrorCode ec = U_ZERO_ERROR;
    Locale locale(Locale::getUS()); // could also be CHINA
    SimpleDateFormat dateFormat("MM/dd/yyyy HH:mm z", locale, ec);

    StringEnumeration *tz = TimeZone::createEnumeration();
    if (tz == NULL) {
        errln("FAIL: TimeZone::createEnumeration");
        return;
    }

    UnicodeString buf1, buf2;

    const UChar* id;
    while ((id = tz->unext(NULL, ec)) != NULL && U_SUCCESS(ec)) {
        if (U_FAILURE(ec)) {
            errln("FAIL: StringEnumeration::unext");
            break;
        }
        
        TimeZone *t = TimeZone::createTimeZone(id);
        if (t == NULL) {
            errln("FAIL: TimeZone::createTimeZone");
            break;
        }
        dateFormat.setTimeZone(*t);

        Calendar *cal = Calendar::createInstance(t, locale, ec);
        if (cal == NULL || U_FAILURE(ec)) {
            errln("FAIL: Calendar::createTimeZone");
            delete cal;
            break;
        }

        cal->clear();
        // Scan the year 2003, overlapping the edges of the year
        cal->set(UCAL_YEAR, 2002);
        cal->set(UCAL_MONTH, UCAL_DECEMBER);
        cal->set(UCAL_DATE, 25);

        for (int32_t i=0; i<365+10 && U_SUCCESS(ec); ++i) {
            UDate yesterday = cal->getTime(ec);
            int32_t yesterday_day = cal->get(UCAL_DATE, ec);
            cal->add(UCAL_DATE, 1, ec);
            if (yesterday_day == cal->get(UCAL_DATE, ec)) {
                errln(UnicodeString(id) + " " +
                      dateFormat.format(yesterday, buf1) + " +1d= " +
                      dateFormat.format(cal->getTime(ec), buf2));
                buf1.truncate(0);
                buf2.truncate(0);
            }
        }
        delete cal;
    }

    if (U_FAILURE(ec)) {
        dataerrln("FAIL: %s", u_errorName(ec));
    }

    delete tz;
}

UDate
CalendarRegressionTest::makeDate(int32_t y, int32_t m, int32_t d,
                                    int32_t hr, int32_t min, int32_t sec)
{
    UDate result;

    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    cal->clear();

    cal->set(UCAL_YEAR, y);
    
    if(m != 0)        cal->set(UCAL_MONTH, m);
    if(d != 0)        cal->set(UCAL_DATE, d);
    if(hr != 0)        cal->set(UCAL_HOUR, hr);
    if(min != 0)    cal->set(UCAL_MINUTE, min);
    if(sec != 0)    cal->set(UCAL_SECOND, sec);

    result = cal->getTime(status);

    delete cal;

    return result;
}

void CalendarRegressionTest::TestDeprecates(void)
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *c1 = Calendar::createInstance("ja_JP@calendar=japanese",status);
    Calendar *c2 = Calendar::createInstance("ja_JP_TRADITIONAL",status);

    if(!c1 || !c2 || U_FAILURE(status)) {
        dataerrln("Couldn't create calendars for roll of HOUR: %s", u_errorName(status));
        return;
    }

    c2->set(UCAL_HOUR,2);
    c1->setTime(c2->getTime(status),status);
    // *c1 = *c2;

    c1->roll(Calendar::HOUR,(int32_t)3,status);
    c2->roll(UCAL_HOUR,(int32_t)3,status);

    if(U_FAILURE(status)) {
        errln("Error code when trying to roll");
    } else if(*c1 != *c2) {
        errln("roll(EDateField, int32_t) had different effect than roll(UCalendarField, int32_t)");
    }

    c1->setTime(c2->getTime(status),status);
    c1->roll(Calendar::HOUR,(UBool)FALSE,status);
    c2->roll(UCAL_HOUR,(UBool)FALSE,status);

    if(U_FAILURE(status)) {
        errln("Error code when trying to roll(UBool)");
    } else if(*c1 != *c2) {
        errln("roll(EDateField, UBool) had different effect than roll(UCalendarField, UBool)");
    }

    delete c1;
    delete c2;

    status = U_ZERO_ERROR;

    c1 = Calendar::createInstance("th_TH_TRADITIONAL",status);
    c2 = Calendar::createInstance("th_TH@calendar=buddhist",status);

    if(!c1 || !c2 || U_FAILURE(status)) {
        errln("Couldn't create calendars for add of HOUR");
        return;
    }

    c2->set(UCAL_HOUR,2);
    c1->setTime(c2->getTime(status),status);
    //*c1 = *c2;

    c1->add(Calendar::HOUR,(int32_t)1,status);

    if(U_FAILURE(status)) {
        errln("Error code when trying to add Calendar::HOUR - %s", u_errorName(status));
    }

    c2->add(UCAL_HOUR,(int32_t)1,status);

    if(U_FAILURE(status)) {
        errln("Error code when trying to add - UCAL_HOUR %s", u_errorName(status));
    } else if(*c1 != *c2) {
        errln("add(EDateField) had different effect than add(UCalendarField)");
    }

    delete c1;
    delete c2;

    status = U_ZERO_ERROR;

    c1 = Calendar::createInstance("es_ES",status);
    c2 = Calendar::createInstance("es_ES",status);

    if(!c1 || !c2 || U_FAILURE(status)) {
        errln("Couldn't create calendars for add of YEAR");
        return;
    }

    c2->set(UCAL_YEAR,1900);
    c1->setTime(c2->getTime(status),status);
    //*c1 = *c2;

    c1->add(Calendar::YEAR,(int32_t)9,status);
    c2->add(UCAL_YEAR,(int32_t)9,status);

    if(U_FAILURE(status)) {
        errln("Error code when trying to add YEARs");
    } else if(*c1 != *c2) {
        errln("add(EDateField YEAR) had different effect than add(UCalendarField YEAR)");
    }

    delete c1;
    delete c2;
}


#endif /* #if !UCONFIG_NO_FORMATTING */
