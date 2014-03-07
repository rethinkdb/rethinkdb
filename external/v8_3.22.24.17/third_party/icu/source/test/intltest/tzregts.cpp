/********************************************************************
 * Copyright (c) 1997-2010, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************/
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h"
#include "unicode/smpdtfmt.h"
#include "unicode/strenum.h"
#include "tzregts.h"
#include "calregts.h"
#include "cmemory.h"

// *****************************************************************************
// class TimeZoneRegressionTest
// *****************************************************************************
/* length of an array */
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))
#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break

void 
TimeZoneRegressionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite NumberFormatRegressionTest");
    switch (index) {

        CASE(0, Test4052967);
        CASE(1, Test4073209);
        CASE(2, Test4073215);
        CASE(3, Test4084933);
        CASE(4, Test4096952);
        CASE(5, Test4109314);
        CASE(6, Test4126678);
        CASE(7, Test4151406);
        CASE(8, Test4151429);
        CASE(9, Test4154537);
        CASE(10, Test4154542);
        CASE(11, Test4154650);
        CASE(12, Test4154525);
        CASE(13, Test4162593);
        CASE(14, TestJ186);
        CASE(15, TestJ449);
        CASE(16, TestJDK12API);
        CASE(17, Test4176686);
        CASE(18, Test4184229);
        default: name = ""; break;
    }
}

UBool 
TimeZoneRegressionTest::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}

/**
 * @bug 4052967
 */
void TimeZoneRegressionTest:: Test4052967() {
    // {sfb} not applicable in C++ ?
    /*logln("*** CHECK TIMEZONE AGAINST HOST OS SETTING ***");
    logln("user.timezone:" + System.getProperty("user.timezone", "<not set>"));
    logln(new Date().toString());
    logln("*** THE RESULTS OF THIS TEST MUST BE VERIFIED MANUALLY ***");*/
}

/**
 * @bug 4073209
 */
void TimeZoneRegressionTest:: Test4073209() {
    TimeZone *z1 = TimeZone::createTimeZone("PST");
    TimeZone *z2 = TimeZone::createTimeZone("PST");
    if (z1 == z2) 
        errln("Fail: TimeZone should return clones");
    delete z1;
    delete z2;
}

UDate TimeZoneRegressionTest::findTransitionBinary(const SimpleTimeZone& tz, UDate min, UDate max) {
    UErrorCode status = U_ZERO_ERROR;
    UBool startsInDST = tz.inDaylightTime(min, status);
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    if (tz.inDaylightTime(max, status) == startsInDST) {
        logln((UnicodeString)"Error: inDaylightTime() != " + ((!startsInDST)?"TRUE":"FALSE"));
        return 0;
    }
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    while ((max - min) > 100) { // Min accuracy in ms
        UDate mid = (min + max) / 2;
        if (tz.inDaylightTime(mid, status) == startsInDST) {
            min = mid;
        } else {
            max = mid;
        }
        if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    }
    return (min + max) / 2;
}

UDate TimeZoneRegressionTest::findTransitionStepwise(const SimpleTimeZone& tz, UDate min, UDate max) {
    UErrorCode status = U_ZERO_ERROR;
    UBool startsInDST = tz.inDaylightTime(min, status);
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    while (min < max) {
        if (tz.inDaylightTime(min, status) != startsInDST) {
            return min;
        }
        if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
        min += (UDate)24*60*60*1000; // one day
    }
    return 0;
}

/**
 * @bug 4073215
 */
// {sfb} will this work using a Calendar?
void TimeZoneRegressionTest:: Test4073215() 
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str, str2;
    SimpleTimeZone *z = new SimpleTimeZone(0, "GMT");
    if (z->useDaylightTime())
        errln("Fail: Fix test to start with non-DST zone");
    z->setStartRule(UCAL_FEBRUARY, 1, UCAL_SUNDAY, 0, status);
    failure(status, "z->setStartRule()");
    z->setEndRule(UCAL_MARCH, -1, UCAL_SUNDAY, 0, status);
    failure(status, "z->setStartRule()");
    if (!z->useDaylightTime())
        errln("Fail: DST not active");

    GregorianCalendar cal(1997, UCAL_JANUARY, 31, status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      return;
    }
    failure(status, "new GregorianCalendar");
    cal.adoptTimeZone(z);

    SimpleDateFormat sdf((UnicodeString)"E d MMM yyyy G HH:mm", status); 
    if(U_FAILURE(status)) {
      errcheckln(status, "Error creating date format %s", u_errorName(status));
      return;
    }
    sdf.setCalendar(cal); 
    failure(status, "new SimpleDateFormat");

    UDate jan31, mar1, mar31;

    UBool indt = z->inDaylightTime(jan31=cal.getTime(status), status);
    failure(status, "inDaylightTime or getTime call on Jan 31");
    if (indt) {
        errln("Fail: Jan 31 inDaylightTime=TRUE, exp FALSE");
    }
    cal.set(1997, UCAL_MARCH, 1);
    indt = z->inDaylightTime(mar1=cal.getTime(status), status);
    failure(status, "inDaylightTime or getTime call on Mar 1");
    if (!indt) {
        UnicodeString str;
        sdf.format(cal.getTime(status), str);
        failure(status, "getTime");
        errln((UnicodeString)"Fail: " + str + " inDaylightTime=FALSE, exp TRUE");
    }
    cal.set(1997, UCAL_MARCH, 31);
    indt = z->inDaylightTime(mar31=cal.getTime(status), status);
    failure(status, "inDaylightTime or getTime call on Mar 31");
    if (indt) {
        errln("Fail: Mar 31 inDaylightTime=TRUE, exp FALSE");
    }

    /*
    cal.set(1997, Calendar::DECEMBER, 31);
    UDate dec31 = cal.getTime(status);
    failure(status, "getTime");
    UDate trans = findTransitionStepwise(*z, jan31, dec31);
    logln((UnicodeString)"Stepwise from " +
          sdf.format(jan31, str.remove()) + "; transition at " +
          (trans?sdf.format(trans, str2.remove()):(UnicodeString)"NONE"));
    trans = findTransitionStepwise(*z, mar1, dec31);
    logln((UnicodeString)"Stepwise from " +
          sdf.format(mar1, str.remove()) + "; transition at " +
          (trans?sdf.format(trans, str2.remove()):(UnicodeString)"NONE"));
    trans = findTransitionStepwise(*z, mar31, dec31);
    logln((UnicodeString)"Stepwise from " +
          sdf.format(mar31, str.remove()) + "; transition at " +
          (trans?sdf.format(trans, str2.remove()):(UnicodeString)"NONE"));
    */
}

/**
 * @bug 4084933
 * The expected behavior of TimeZone around the boundaries is:
 * (Assume transition time of 2:00 AM)
 *    day of onset 1:59 AM STD  = display name 1:59 AM ST
 *                 2:00 AM STD  = display name 3:00 AM DT
 *    day of end   0:59 AM STD  = display name 1:59 AM DT
 *                 1:00 AM STD  = display name 1:00 AM ST
 */
void TimeZoneRegressionTest:: Test4084933() {
    UErrorCode status = U_ZERO_ERROR;
    TimeZone *tz = TimeZone::createTimeZone("PST");

    int32_t offset1 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000), status);
    int32_t offset2 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000)-1, status);

    int32_t offset3 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (1*60*60*1000), status);
    int32_t offset4 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (1*60*60*1000)-1, status);

    /*
     *  The following was added just for consistency.  It shows that going *to* Daylight
     *  Savings Time (PDT) does work at 2am.
     */
    int32_t offset5 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (2*60*60*1000), status);
    int32_t offset6 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (2*60*60*1000)-1, status);
    int32_t offset5a = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (3*60*60*1000), status);
    int32_t offset6a = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (3*60*60*1000)-1, status);
    int32_t offset7 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (1*60*60*1000), status);
    int32_t offset8 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (1*60*60*1000)-1, status);
    int32_t SToffset = (int32_t)(-8 * 60*60*1000L);
    int32_t DToffset = (int32_t)(-7 * 60*60*1000L);
        
#define ERR_IF_FAIL(x) if(x) { dataerrln("FAIL: TimeZone misbehaving - %s", #x); }

        ERR_IF_FAIL(U_FAILURE(status))
        ERR_IF_FAIL(offset1 != SToffset)
        ERR_IF_FAIL(offset2 != SToffset)
        ERR_IF_FAIL(offset3 != SToffset)
        ERR_IF_FAIL(offset4 != DToffset)
        ERR_IF_FAIL(offset5 != DToffset)
        ERR_IF_FAIL(offset6 != SToffset)
        ERR_IF_FAIL(offset5a != DToffset)
        ERR_IF_FAIL(offset6a != DToffset)
        ERR_IF_FAIL(offset7 != SToffset)
        ERR_IF_FAIL(offset8 != SToffset)

#undef ERR_IF_FAIL

    delete tz;
}

/**
 * @bug 4096952
 */
void TimeZoneRegressionTest:: Test4096952() {
    // {sfb} serialization not applicable
/*
    UnicodeString ZONES [] = { UnicodeString("GMT"), UnicodeString("MET"), UnicodeString("IST") };
    UBool pass = TRUE;
    //try {
        for (int32_t i=0; i < ZONES.length; ++i) {
            TimeZone *zone = TimeZone::createTimeZone(ZONES[i]);
            UnicodeString id;
            if (zone->getID(id) != ZONES[i])
                errln("Fail: Test broken; zones not instantiating");

            ByteArrayOutputStream baos;
            ObjectOutputStream ostream =
                new ObjectOutputStream(baos = new 
                                       ByteArrayOutputStream());
            ostream.writeObject(zone);
            ostream.close();
            baos.close();
            ObjectInputStream istream =
                new ObjectInputStream(new 
                                      ByteArrayInputStream(baos.toByteArray()));
            TimeZone frankenZone = (TimeZone) istream.readObject();
            //logln("Zone:        " + zone);
            //logln("FrankenZone: " + frankenZone);
            if (!zone.equals(frankenZone)) {
                logln("TimeZone " + zone.getID() +
                      " not equal to serialized/deserialized one");
                pass = false;
            }
        }
        if (!pass) errln("Fail: TimeZone serialization/equality bug");
    }
    catch (IOException e) {
        errln("Fail: " + e);
        e.print32_tStackTrace();
    }
    catch (ClassNotFoundException e) {
        errln("Fail: " + e);
        e.print32_tStackTrace();
    }
*/
}

/**
 * @bug 4109314
 */
void TimeZoneRegressionTest:: Test4109314() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *testCal = (GregorianCalendar*)Calendar::createInstance(status); 
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete testCal;
      return;
    }
    failure(status, "Calendar::createInstance");
    TimeZone *PST = TimeZone::createTimeZone("PST");
    /*Object[] testData = {
        PST, new Date(98,Calendar.APRIL,4,22,0), new Date(98, Calendar.APRIL, 5,6,0),
        PST, new Date(98,Calendar.OCTOBER,24,22,0), new Date(98,Calendar.OCTOBER,25,6,0),
    };*/
    UDate testData [] = {
        CalendarRegressionTest::makeDate(98,UCAL_APRIL,4,22,0),    
        CalendarRegressionTest::makeDate(98, UCAL_APRIL,5,6,0),
        CalendarRegressionTest::makeDate(98,UCAL_OCTOBER,24,22,0), 
        CalendarRegressionTest::makeDate(98,UCAL_OCTOBER,25,6,0)
    };
    UBool pass = TRUE;
    for (int32_t i = 0; i < 4; i+=2) {
        //testCal->setTimeZone((TimeZone) testData[i]);
        testCal->setTimeZone(*PST);
        UDate t        = testData[i];
        UDate end    = testData[i+1];
        while(testCal->getTime(status) < end) { 
            testCal->setTime(t, status);
            if ( ! checkCalendar314(testCal, PST))
                pass = FALSE;
            t += 60*60*1000.0;
        } 
    }
    if ( ! pass) 
        errln("Fail: TZ API inconsistent");

    delete testCal;
    delete PST;
} 

UBool 
TimeZoneRegressionTest::checkCalendar314(GregorianCalendar *testCal, TimeZone *testTZ) 
{
    UErrorCode status = U_ZERO_ERROR;
    // GregorianCalendar testCal = (GregorianCalendar)aCal.clone(); 

    int32_t tzOffset, tzRawOffset; 
    float tzOffsetFloat,tzRawOffsetFloat; 
    // Here is where the user made an error.  They were passing in the value of
    // the MILLSECOND field; you need to pass in the millis in the day in STANDARD
    // time.
    UDate millis = testCal->get(UCAL_MILLISECOND, status) +
        1000.0 * (testCal->get(UCAL_SECOND, status) +
        60.0 * (testCal->get(UCAL_MINUTE, status) +
        60.0 * (testCal->get(UCAL_HOUR_OF_DAY, status)))) -
        testCal->get(UCAL_DST_OFFSET, status);

    /* Fix up millis to be in range.  ASSUME THAT WE ARE NOT AT THE
     * BEGINNING OR END OF A MONTH.  We must add this code because
     * getOffset() has been changed to be more strict about the parameters
     * it receives -- it turns out that this test was passing in illegal
     * values. */
    int32_t date = testCal->get(UCAL_DATE, status);
    int32_t dow  = testCal->get(UCAL_DAY_OF_WEEK, status);
    while(millis < 0) {
        millis += U_MILLIS_PER_DAY;
        --date;
        dow = UCAL_SUNDAY + ((dow - UCAL_SUNDAY + 6) % 7);
    }
    while (millis >= U_MILLIS_PER_DAY) {
        millis -= U_MILLIS_PER_DAY;
        ++date;
        dow = UCAL_SUNDAY + ((dow - UCAL_SUNDAY + 1) % 7);
    }

    tzOffset = testTZ->getOffset((uint8_t)testCal->get(UCAL_ERA, status), 
                                testCal->get(UCAL_YEAR, status), 
                                testCal->get(UCAL_MONTH, status), 
                                date, 
                                (uint8_t)dow, 
                                (int32_t)millis,
                                status); 
    tzRawOffset = testTZ->getRawOffset(); 
    tzOffsetFloat = (float)tzOffset/(float)3600000; 
    tzRawOffsetFloat = (float)tzRawOffset/(float)3600000; 

    UDate testDate = testCal->getTime(status); 

    UBool inDaylightTime = testTZ->inDaylightTime(testDate, status); 
    SimpleDateFormat *sdf = new SimpleDateFormat((UnicodeString)"MM/dd/yyyy HH:mm", status); 
    sdf->setCalendar(*testCal); 
    UnicodeString inDaylightTimeString; 

    UBool passed; 

    if(inDaylightTime) 
    { 
        inDaylightTimeString = " DST "; 
        passed = (tzOffset == (tzRawOffset + 3600000));
    } 
    else 
    { 
        inDaylightTimeString = "     "; 
        passed = (tzOffset == tzRawOffset);
    } 

    UnicodeString output;
    FieldPosition pos(0);
    output = testTZ->getID(output) + " " + sdf->format(testDate, output, pos) +
        " Offset(" + tzOffsetFloat + ")" +
        " RawOffset(" + tzRawOffsetFloat + ")" + 
        " " + millis/(float)3600000 + " " +
        inDaylightTimeString; 

    if (passed) 
        output += "     "; 
    else 
        output += "ERROR"; 

    if (passed) 
        logln(output); 
    else 
        errln(output);

    delete sdf;
    return passed;
} 

/**
 * @bug 4126678
 * CANNOT REPRODUDE
 *
 * Yet another _alleged_ bug in TimeZone::getOffset(), a method that never
 * should have been made public.  It's simply too hard to use correctly.
 *
 * The original test code failed to do the following:
 * (1) Call Calendar::setTime() before getting the fields!
 * (2) Use the right millis (as usual) for getOffset(); they were passing
 *     in the MILLIS field, instead of the STANDARD MILLIS IN DAY.
 * When you fix these two problems, the test passes, as expected.
 */
void TimeZoneRegressionTest:: Test4126678() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    failure(status, "Calendar::createInstance");
    TimeZone *tz = TimeZone::createTimeZone("PST");
    cal->adoptTimeZone(tz);

    cal->set(1998, UCAL_APRIL, 5, 10, 0);
    
    if (! tz->useDaylightTime() || U_FAILURE(status))
        dataerrln("We're not in Daylight Savings Time and we should be. - %s", u_errorName(status));

    //cal.setTime(dt);
    int32_t era = cal->get(UCAL_ERA, status);
    int32_t year = cal->get(UCAL_YEAR, status);
    int32_t month = cal->get(UCAL_MONTH, status);
    int32_t day = cal->get(UCAL_DATE, status);
    int32_t dayOfWeek = cal->get(UCAL_DAY_OF_WEEK, status);
    int32_t millis = cal->get(UCAL_MILLISECOND, status) +
        (cal->get(UCAL_SECOND, status) +
         (cal->get(UCAL_MINUTE, status) +
          (cal->get(UCAL_HOUR, status) * 60) * 60) * 1000) -
        cal->get(UCAL_DST_OFFSET, status);

    failure(status, "cal->get");
    int32_t offset = tz->getOffset((uint8_t)era, year, month, day, (uint8_t)dayOfWeek, millis, status);
    int32_t raw_offset = tz->getRawOffset();

    if (offset == raw_offset)
        errln("Offsets should match");

    delete cal;
}

/**
 * @bug 4151406
 * TimeZone::getAvailableIDs(int32_t) throws exception for certain values,
 * due to a faulty constant in TimeZone::java.
 */
void TimeZoneRegressionTest:: Test4151406() {
    int32_t max = 0;
    for (int32_t h=-28; h<=30; ++h) {
        // h is in half-hours from GMT; rawoffset is in millis
        int32_t rawoffset = h * 1800000;
        int32_t hh = (h<0) ? -h : h;
        UnicodeString hname = UnicodeString((h<0) ? "GMT-" : "GMT+") +
            ((hh/2 < 10) ? "0" : "") +
            (hh/2) + ':' +
            ((hh%2==0) ? "00" : "30");
        //try {
            UErrorCode ec = U_ZERO_ERROR;
            int32_t count;
            StringEnumeration* ids = TimeZone::createEnumeration(rawoffset);
            count = ids->count(ec);
            if (count> max) 
                max = count;
            if (count > 0) {
                logln(hname + ' ' + (UnicodeString)count + (UnicodeString)" e.g. " + *ids->snext(ec));
            } else {
                logln(hname + ' ' + count);
            }
            // weiv 11/27/2002: why uprv_free? This should be a delete
            delete ids;
            //delete [] ids;
            //uprv_free(ids);
        /*} catch (Exception e) {
            errln(hname + ' ' + "Fail: " + e);
        }*/
    }
    logln("Maximum zones per offset = %d", max);
}

/**
 * @bug 4151429
 */
void TimeZoneRegressionTest:: Test4151429() {
    // {sfb} silly test in C++, since we are using an enum and not an int
    //try {
        /*TimeZone *tz = TimeZone::createTimeZone("GMT");
        UnicodeString name;
        tz->getDisplayName(TRUE, TimeZone::LONG,
                                        Locale.getDefault(), name);
        errln("IllegalArgumentException not thrown by TimeZone::getDisplayName()");*/
    //} catch(IllegalArgumentException e) {}
}

/**
 * @bug 4154537
 * SimpleTimeZone::hasSameRules() doesn't work for zones with no DST
 * and different DST parameters.
 */
void TimeZoneRegressionTest:: Test4154537() {
    UErrorCode status = U_ZERO_ERROR;
    // tz1 and tz2 have no DST and different rule parameters
    SimpleTimeZone *tz1 = new SimpleTimeZone(0, "1", 0, 0, 0, 0, 2, 0, 0, 0, status);
    SimpleTimeZone *tz2 = new SimpleTimeZone(0, "2", 1, 0, 0, 0, 3, 0, 0, 0, status);
    // tza and tzA have the same rule params
    SimpleTimeZone *tza = new SimpleTimeZone(0, "a", 0, 1, 0, 0, 3, 2, 0, 0, status);
    SimpleTimeZone *tzA = new SimpleTimeZone(0, "A", 0, 1, 0, 0, 3, 2, 0, 0, status);
    // tzb differs from tza
    SimpleTimeZone *tzb = new SimpleTimeZone(0, "b", 0, 1, 0, 0, 3, 1, 0, 0, status);
    
    if(U_FAILURE(status))
        errln("Couldn't create TimeZones");

    if (tz1->useDaylightTime() || tz2->useDaylightTime() ||
        !tza->useDaylightTime() || !tzA->useDaylightTime() ||
        !tzb->useDaylightTime()) {
        errln("Test is broken -- rewrite it");
    }
    if (!tza->hasSameRules(*tzA) || tza->hasSameRules(*tzb)) {
        errln("Fail: hasSameRules() broken for zones with rules");
    }
    if (!tz1->hasSameRules(*tz2)) {
        errln("Fail: hasSameRules() returns false for zones without rules");
        //errln("zone 1 = " + tz1);
        //errln("zone 2 = " + tz2);
    }

    delete tz1;
    delete tz2;
    delete tza;
    delete tzA;
    delete tzb;
}

/**
 * @bug 4154542
 * SimpleTimeZOne constructors, setStartRule(), and setEndRule() don't
 * check for out-of-range arguments.
 */
void TimeZoneRegressionTest:: Test4154542() 
{
    const int32_t GOOD = 1;
    const int32_t BAD  = 0;

    const int32_t GOOD_MONTH       = UCAL_JANUARY;
    const int32_t GOOD_DAY         = 1;
    const int32_t GOOD_DAY_OF_WEEK = UCAL_SUNDAY;
    const int32_t GOOD_TIME        = 0;

    int32_t DATA [] = {
        GOOD, INT32_MIN,    0,  INT32_MAX,   INT32_MIN,
        GOOD, UCAL_JANUARY,    -5,  UCAL_SUNDAY,     0,
        GOOD, UCAL_DECEMBER,    5,  UCAL_SATURDAY,   24*60*60*1000,
        BAD,  UCAL_DECEMBER,    5,  UCAL_SATURDAY,   24*60*60*1000+1,
        BAD,  UCAL_DECEMBER,    5,  UCAL_SATURDAY,  -1,
        BAD,  UCAL_JANUARY,    -6,  UCAL_SUNDAY,     0,
        BAD,  UCAL_DECEMBER,    6,  UCAL_SATURDAY,   24*60*60*1000,
        GOOD, UCAL_DECEMBER,    1,  0,                   0,
        GOOD, UCAL_DECEMBER,   31,  0,                   0,
        BAD,  UCAL_APRIL,      31,  0,                   0,
        BAD,  UCAL_DECEMBER,   32,  0,                   0,
        BAD,  UCAL_JANUARY-1,   1,  UCAL_SUNDAY,     0,
        BAD,  UCAL_DECEMBER+1,  1,  UCAL_SUNDAY,     0,
        GOOD, UCAL_DECEMBER,   31, -UCAL_SUNDAY,     0,
        GOOD, UCAL_DECEMBER,   31, -UCAL_SATURDAY,   0,
        BAD,  UCAL_DECEMBER,   32, -UCAL_SATURDAY,   0,
        BAD,  UCAL_DECEMBER,  -32, -UCAL_SATURDAY,   0,
        BAD,  UCAL_DECEMBER,   31, -UCAL_SATURDAY-1, 0,
    };
    SimpleTimeZone *zone = new SimpleTimeZone(0, "Z");
    for (int32_t i=0; i < 18*5; i+=5) {
        UBool shouldBeGood = (DATA[i] == GOOD);
        int32_t month     = DATA[i+1];
        int32_t day       = DATA[i+2];
        int32_t dayOfWeek = DATA[i+3];
        int32_t time      = DATA[i+4];

        UErrorCode status = U_ZERO_ERROR;

        //Exception ex = null;
        //try {
            zone->setStartRule(month, day, dayOfWeek, time, status);
        //} catch (IllegalArgumentException e) {
        //    ex = e;
        //}
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("setStartRule(month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (") should work")
                   : ") should fail but doesn't"));
        }

        //ex = null;
        //try {
        status = U_ZERO_ERROR;
            zone->setEndRule(month, day, dayOfWeek, time, status);
        //} catch (IllegalArgumentException e) {
        //   ex = e;
        //}
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("setEndRule(month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (") should work")
                   : ") should fail but doesn't"));
        }

        //ex = null;
        //try {
        // {sfb} need to look into ctor problems! (UErrorCode vs. dst signature confusion)
        status = U_ZERO_ERROR;
            SimpleTimeZone *temp = new SimpleTimeZone(0, "Z",
                    (int8_t)month, (int8_t)day, (int8_t)dayOfWeek, time,
                    (int8_t)GOOD_MONTH, (int8_t)GOOD_DAY, (int8_t)GOOD_DAY_OF_WEEK, 
                    GOOD_TIME,status);
        //} catch (IllegalArgumentException e) {
        //    ex = e;
        //}
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("SimpleTimeZone(month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (", <end>) should work")// + ex)
                   : ", <end>) should fail but doesn't"));
        }
  
        delete temp;
        //ex = null;
        //try {
        status = U_ZERO_ERROR;
            temp = new SimpleTimeZone(0, "Z",
                    (int8_t)GOOD_MONTH, (int8_t)GOOD_DAY, (int8_t)GOOD_DAY_OF_WEEK, 
                    GOOD_TIME,
                    (int8_t)month, (int8_t)day, (int8_t)dayOfWeek, time,status);
        //} catch (IllegalArgumentException e) {
        //    ex = e;
        //}
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("SimpleTimeZone(<start>, month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (") should work")// + ex)
                   : ") should fail but doesn't"));
        }            
        delete temp;
    }
    delete zone;
}


/**
 * @bug 4154525
 * SimpleTimeZone accepts illegal DST savings values.  These values
 * must be non-zero.  There is no upper limit at this time.
 */
void 
TimeZoneRegressionTest::Test4154525() 
{
    const int32_t GOOD = 1, BAD = 0;
    
    int32_t DATA [] = {
        1, GOOD,
        0, BAD,
        -1, BAD,
        60*60*1000, GOOD,
        INT32_MIN, BAD,
        // Integer.MAX_VALUE, ?, // no upper limit on DST savings at this time
    };

    UErrorCode status = U_ZERO_ERROR;
    for(int32_t i = 0; i < 10; i+=2) {
        int32_t savings = DATA[i];
        UBool valid = DATA[i+1] == GOOD;
        UnicodeString method;
        for(int32_t j=0; j < 2; ++j) {
            SimpleTimeZone *z=NULL;
            switch (j) {
                case 0:
                    method = "constructor";
                    z = new SimpleTimeZone(0, "id",
                        UCAL_JANUARY, 1, 0, 0,
                        UCAL_MARCH, 1, 0, 0,
                        savings, status); // <- what we're interested in
                    break;
                case 1:
                    method = "setDSTSavings()";
                    z = new SimpleTimeZone(0, "GMT");
                    z->setDSTSavings(savings, status);
                    break;
            }

            if(U_FAILURE(status)) {
                if(valid) {
                    errln(UnicodeString("Fail: DST savings of ") + savings + " to " + method + " gave " + u_errorName(status));
                }
                else {
                    logln(UnicodeString("Pass: DST savings of ") + savings + " to " + method + " gave " + u_errorName(status));
                }
            }
            else {
                if(valid) {
                    logln(UnicodeString("Pass: DST savings of ") + savings + " accepted by " + method);
                } 
                else {
                    errln(UnicodeString("Fail: DST savings of ") + savings + " accepted by " + method);
                }
            }
            status = U_ZERO_ERROR;
            delete z;
        }
    }
}

/**
 * @bug 4154650
 * SimpleTimeZone.getOffset accepts illegal arguments.
 */
void 
TimeZoneRegressionTest::Test4154650() 
{
    const int32_t GOOD = 1, BAD = 0;
    const int32_t GOOD_ERA = GregorianCalendar::AD, GOOD_YEAR = 1998, GOOD_MONTH = UCAL_AUGUST;
    const int32_t GOOD_DAY = 2, GOOD_DOW = UCAL_SUNDAY, GOOD_TIME = 16*3600000;

    int32_t DATA []= {
        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,

        GOOD, GregorianCalendar::BC, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        GOOD, GregorianCalendar::AD, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GregorianCalendar::BC-1, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GregorianCalendar::AD+1, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,

        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_DECEMBER, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_JANUARY-1, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_DECEMBER+1, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        
        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 1, GOOD_DOW, GOOD_TIME,
        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 31, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 0, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 32, GOOD_DOW, GOOD_TIME,

        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SUNDAY, GOOD_TIME,
        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SATURDAY, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SUNDAY-1, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SATURDAY+1, GOOD_TIME,

        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, 0,
        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, 24*3600000-1,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, -1,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, 24*3600000,
    };

    int32_t dataLen = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    UErrorCode status = U_ZERO_ERROR;
    TimeZone *tz = TimeZone::createDefault();
    for(int32_t i = 0; i < dataLen; i += 7) {
        UBool good = DATA[i] == GOOD;
        //IllegalArgumentException e = null;
        //try {
            /*int32_t offset = */
        tz->getOffset((uint8_t)DATA[i+1], DATA[i+2], DATA[i+3],
                      DATA[i+4], (uint8_t)DATA[i+5], DATA[i+6], status); 
        //} catch (IllegalArgumentException ex) {
        //   e = ex;
        //}
        if(good != U_SUCCESS(status)) {
            UnicodeString errMsg;
            if (good) {
                errMsg = (UnicodeString(") threw ") + u_errorName(status));
            }
            else {
                errMsg = UnicodeString(") accepts invalid args", "");
            }
            errln(UnicodeString("Fail: getOffset(") +
                  DATA[i+1] + ", " + DATA[i+2] + ", " + DATA[i+3] + ", " +
                  DATA[i+4] + ", " + DATA[i+5] + ", " + DATA[i+6] +
                  errMsg);
        }
        status = U_ZERO_ERROR; // reset
    }
    delete tz;
}

/**
 * @bug 4162593
 * TimeZone broken at midnight.  The TimeZone code fails to handle
 * transitions at midnight correctly.
 */
void 
TimeZoneRegressionTest::Test4162593() 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *fmt = new SimpleDateFormat("z", Locale::getUS(), status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete fmt;
      return;
    }
    const int32_t ONE_HOUR = 60*60*1000;

    SimpleTimeZone *asuncion = new SimpleTimeZone(-4*ONE_HOUR, "America/Asuncion" /*PY%sT*/,
        UCAL_OCTOBER, 1, 0 /*DOM*/, 0*ONE_HOUR,
        UCAL_MARCH, 1, 0 /*DOM*/, 0*ONE_HOUR, 1*ONE_HOUR, status);

    /* Zone
     * Starting time
     * Transition expected between start+1H and start+2H
     */
    TimeZone *DATA_TZ [] = {
      0, 0, 0 };

    int32_t DATA_INT [] [5] = {
        // These years must be AFTER the Gregorian cutover
        {1998, UCAL_SEPTEMBER, 30, 22, 0},
        {2000, UCAL_FEBRUARY, 28, 22, 0},
        {2000, UCAL_FEBRUARY, 29, 22, 0},
     };

    UBool DATA_BOOL [] = {
        TRUE,
        FALSE,
        TRUE,
    };
    
    UnicodeString zone [4];// = new String[4];
    DATA_TZ[0] =  
        new SimpleTimeZone(2*ONE_HOUR, "Asia/Damascus" /*EE%sT*/,
            UCAL_APRIL, 1, 0 /*DOM*/, 0*ONE_HOUR,
            UCAL_OCTOBER, 1, 0 /*DOM*/, 0*ONE_HOUR, 1*ONE_HOUR, status);
    DATA_TZ[1] = asuncion;  DATA_TZ[2] = asuncion;  

    for(int32_t j = 0; j < 3; j++) {
        TimeZone *tz = (TimeZone*)DATA_TZ[j];
        TimeZone::setDefault(*tz);
        fmt->setTimeZone(*tz);

        // Must construct the Date object AFTER setting the default zone
        int32_t *p = (int32_t*)DATA_INT[j];
        UDate d = CalendarRegressionTest::makeDate(p[0], p[1], p[2], p[3], p[4]);
       UBool transitionExpected = DATA_BOOL[j];

        UnicodeString temp;
        logln(tz->getID(temp) + ":");
        for (int32_t i = 0; i < 4; ++i) {
            FieldPosition pos(0);
            zone[i].remove();
            zone[i] = fmt->format(d+ i*ONE_HOUR, zone[i], pos);
            logln(UnicodeString("") + i + ": " + d + " / " + zone[i]);
            //d += (double) ONE_HOUR;
        }
        if(zone[0] == zone[1] &&
            (zone[1] == zone[2]) != transitionExpected &&
            zone[2] == zone[3]) {
            logln(UnicodeString("Ok: transition ") + transitionExpected);
        } 
        else {
            errln("Fail: boundary transition incorrect");
        }
    }
    delete fmt;
    delete asuncion;
    delete DATA_TZ[0];
}

  /**
    * getDisplayName doesn't work with unusual savings/offsets.
    */
void TimeZoneRegressionTest::Test4176686() {
    // Construct a zone that does not observe DST but
    // that does have a DST savings (which should be ignored).
    UErrorCode status = U_ZERO_ERROR;
    int32_t offset = 90 * 60000; // 1:30
    SimpleTimeZone* z1 = new SimpleTimeZone(offset, "_std_zone_");
    z1->setDSTSavings(45 * 60000, status); // 0:45

    // Construct a zone that observes DST for the first 6 months.
    SimpleTimeZone* z2 = new SimpleTimeZone(offset, "_dst_zone_");
    z2->setDSTSavings(45 * 60000, status); // 0:45
    z2->setStartRule(UCAL_JANUARY, 1, 0, status);
    z2->setEndRule(UCAL_JULY, 1, 0, status);

    // Also check DateFormat
    DateFormat* fmt1 = new SimpleDateFormat(UnicodeString("z"), status);
    if (U_FAILURE(status)) {
        dataerrln("Failure trying to construct: %s", u_errorName(status));
        return;
    }
    fmt1->setTimeZone(*z1); // Format uses standard zone
    DateFormat* fmt2 = new SimpleDateFormat(UnicodeString("z"), status);
    if(!assertSuccess("trying to construct", status))return;
    fmt2->setTimeZone(*z2); // Format uses DST zone
    Calendar* tempcal = Calendar::createInstance(status);
    tempcal->clear();
    tempcal->set(1970, UCAL_FEBRUARY, 1);
    UDate dst = tempcal->getTime(status); // Time in DST
    tempcal->set(1970, UCAL_AUGUST, 1);
    UDate std = tempcal->getTime(status); // Time in standard

    // Description, Result, Expected Result
    UnicodeString a,b,c,d,e,f,g,h,i,j,k,l;
    UnicodeString DATA[] = {
        "z1->getDisplayName(false, SHORT)/std zone",
        z1->getDisplayName(FALSE, TimeZone::SHORT, a), "GMT+01:30",
        "z1->getDisplayName(false, LONG)/std zone",
        z1->getDisplayName(FALSE, TimeZone::LONG, b), "GMT+01:30",
        "z1->getDisplayName(true, SHORT)/std zone",
        z1->getDisplayName(TRUE, TimeZone::SHORT, c), "GMT+01:30",
        "z1->getDisplayName(true, LONG)/std zone",
        z1->getDisplayName(TRUE, TimeZone::LONG, d ), "GMT+01:30",
        "z2->getDisplayName(false, SHORT)/dst zone",
        z2->getDisplayName(FALSE, TimeZone::SHORT, e), "GMT+01:30",
        "z2->getDisplayName(false, LONG)/dst zone",
        z2->getDisplayName(FALSE, TimeZone::LONG, f ), "GMT+01:30",
        "z2->getDisplayName(true, SHORT)/dst zone",
        z2->getDisplayName(TRUE, TimeZone::SHORT, g), "GMT+02:15",
        "z2->getDisplayName(true, LONG)/dst zone",
        z2->getDisplayName(TRUE, TimeZone::LONG, h ), "GMT+02:15",
        "DateFormat.format(std)/std zone", fmt1->format(std, i), "GMT+01:30",
        "DateFormat.format(dst)/std zone", fmt1->format(dst, j), "GMT+01:30",
        "DateFormat.format(std)/dst zone", fmt2->format(std, k), "GMT+01:30",
        "DateFormat.format(dst)/dst zone", fmt2->format(dst, l), "GMT+02:15",
    };

    for (int32_t idx=0; idx<(int32_t)ARRAY_LENGTH(DATA); idx+=3) {
        if (DATA[idx+1]!=(DATA[idx+2])) {
            errln("FAIL: " + DATA[idx] + " -> " + DATA[idx+1] + ", exp " + DATA[idx+2]);
        }
    }
    delete z1;
    delete z2;
    delete fmt1;
    delete fmt2;
    delete tempcal;
}

/**
 * Make sure setStartRule and setEndRule set the DST savings to nonzero
 * if it was zero.
 */
void TimeZoneRegressionTest::TestJ186() {
    UErrorCode status = U_ZERO_ERROR;
    // NOTE: Setting the DST savings to zero is illegal, so we
    // are limited in the testing we can do here.  This is why
    // lines marked //~ are commented out.
    SimpleTimeZone z(0, "ID");
    //~z.setDSTSavings(0, status); // Must do this!
    z.setStartRule(UCAL_FEBRUARY, 1, UCAL_SUNDAY, 0, status);
    failure(status, "setStartRule()");
    if (z.useDaylightTime()) {
        errln("Fail: useDaylightTime true with start rule only");
    }
    //~if (z.getDSTSavings() != 0) {
    //~    errln("Fail: dst savings != 0 with start rule only");
    //~}
    z.setEndRule(UCAL_MARCH, -1, UCAL_SUNDAY, 0, status);
    failure(status, "setStartRule()");
    if (!z.useDaylightTime()) {
        errln("Fail: useDaylightTime false with rules set");
    }
    if (z.getDSTSavings() == 0) {
        errln("Fail: dst savings == 0 with rules set");
    }
}

/**
 * Test to see if DateFormat understands zone equivalency groups.  It
 * might seem that this should be a DateFormat test, but it's really a
 * TimeZone test -- the changes to DateFormat are minor.
 *
 * We use two known, stable zones that shouldn't change much over time
 * -- America/Vancouver and America/Los_Angeles.  However, they MAY
 * change at some point -- if that happens, replace them with any two
 * zones in an equivalency group where one zone has localized name
 * data, and the other doesn't, in some locale.
 */
void TimeZoneRegressionTest::TestJ449() {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;

    // Modify the following three as necessary.  The two IDs must
    // specify two zones in the same equivalency group.  One must have
    // locale data in 'loc'; the other must not.
    const char* idWithLocaleData = "America/Los_Angeles";
    const char* idWithoutLocaleData = "US/Pacific";
    const Locale loc("en", "", "");

    TimeZone *zoneWith = TimeZone::createTimeZone(idWithLocaleData);
    TimeZone *zoneWithout = TimeZone::createTimeZone(idWithoutLocaleData);
    // Make sure we got valid zones
    if (zoneWith->getID(str) != UnicodeString(idWithLocaleData) ||
        zoneWithout->getID(str) != UnicodeString(idWithoutLocaleData)) {
      dataerrln(UnicodeString("Fail: Unable to create zones - wanted ") + idWithLocaleData + ", got " + zoneWith->getID(str) + ", and wanted " + idWithoutLocaleData + " but got " + zoneWithout->getID(str));
    } else {
        GregorianCalendar calWith(*zoneWith, status);
        GregorianCalendar calWithout(*zoneWithout, status);
        SimpleDateFormat fmt("MMM d yyyy hh:mm a zzz", loc, status);
        if (U_FAILURE(status)) {
            errln("Fail: Unable to create GregorianCalendar/SimpleDateFormat");
        } else {
            UDate date = 0;
            UnicodeString strWith, strWithout;
            fmt.setCalendar(calWith);
            fmt.format(date, strWith);
            fmt.setCalendar(calWithout);
            fmt.format(date, strWithout);
            if (strWith == strWithout) {
                logln((UnicodeString)"Ok: " + idWithLocaleData + " -> " +
                      strWith + "; " + idWithoutLocaleData + " -> " +
                      strWithout);
            } else {
                errln((UnicodeString)"FAIL: " + idWithLocaleData + " -> " +
                      strWith + "; " + idWithoutLocaleData + " -> " +
                      strWithout);
            }
        }
    }

    delete zoneWith;
    delete zoneWithout;
}

// test new API for JDK 1.2 8/31 putback
void
TimeZoneRegressionTest::TestJDK12API()
{
    // TimeZone *pst = TimeZone::createTimeZone("PST");
    // TimeZone *cst1 = TimeZone::createTimeZone("CST");
    UErrorCode ec = U_ZERO_ERROR;
    //d,-28800,3,1,-1,120,w,9,-1,1,120,w,60
    TimeZone *pst = new SimpleTimeZone(-28800*U_MILLIS_PER_SECOND,
                                       "PST",
                                       3,1,-1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       9,-1,1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       60*U_MILLIS_PER_MINUTE,ec);
    //d,-21600,3,1,-1,120,w,9,-1,1,120,w,60
    TimeZone *cst1 = new SimpleTimeZone(-21600*U_MILLIS_PER_SECOND,
                                       "CST",
                                       3,1,-1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       9,-1,1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       60*U_MILLIS_PER_MINUTE,ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: SimpleTimeZone constructor");
        return;
    }

    SimpleTimeZone *cst = dynamic_cast<SimpleTimeZone *>(cst1);

    if(pst->hasSameRules(*cst)) {
        errln("FAILURE: PST and CST have same rules");
    }

    UErrorCode status = U_ZERO_ERROR;
    int32_t offset1 = pst->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000), status);
    failure(status, "getOffset() failed");


    int32_t offset2 = cst->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000), 31, status);
    failure(status, "getOffset() failed");

    if(offset1 == offset2)
        errln("FAILURE: Sunday Oct. 26 1997 2:00 has same offset for PST and CST");

    // verify error checking
    pst->getOffset(1,
        1997, UCAL_FIELD_COUNT+1, 26, UCAL_SUNDAY, (2*60*60*1000), status);
    if(U_SUCCESS(status))
        errln("FAILURE: getOffset() succeeded with -1 for month");

    status = U_ZERO_ERROR;
    cst->setDSTSavings(60*60*1000, status);
    failure(status, "setDSTSavings() failed");

    int32_t savings = cst->getDSTSavings();
    if(savings != 60*60*1000) {
        errln("setDSTSavings() failed");
    }

    delete pst;
    delete cst;
}
/**
 * SimpleTimeZone allows invalid DOM values.
 */
void TimeZoneRegressionTest::Test4184229() {
    SimpleTimeZone* zone = NULL;
    UErrorCode status = U_ZERO_ERROR;
    zone = new SimpleTimeZone(0, "A", 0, -1, 0, 0, 0, 0, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 startDay");
    }else{
       logln("(a) " + UnicodeString( u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;

    zone = new SimpleTimeZone(0, "A", 0, 0, 0, 0, 0, -1, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 endDay");
    }else{
       logln("(b) " + UnicodeString(u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;

    zone = new SimpleTimeZone(0, "A", 0, -1, 0, 0, 0, 0, 0, 1000, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 startDay+savings");
    }else{
       logln("(c) " + UnicodeString(u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;
    zone = new SimpleTimeZone(0, "A", 0, 0, 0, 0, 0, -1, 0, 0, 1000, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 endDay+ savings");
    }else{
       logln("(d) " + UnicodeString(u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;
    // Make a valid constructor call for subsequent tests.
    zone = new SimpleTimeZone(0, "A", 0, 1, 0, 0, 0, 1, 0, 0, status);
    
    zone->setStartRule(0, -1, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setStartRule +savings");
    } else{
        logln("(e) " + UnicodeString(u_errorName(status)));
    }
    zone->setStartRule(0, -1, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setStartRule");
    } else{
        logln("(f) " + UnicodeString(u_errorName(status)));
    }

    zone->setEndRule(0, -1, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setEndRule+savings");
    } else{
        logln("(g) " + UnicodeString(u_errorName(status)));
    }   

    zone->setEndRule(0, -1, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setEndRule");
    } else{
        logln("(h) " + UnicodeString(u_errorName(status)));
    }
    delete zone;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
