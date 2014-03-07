/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tzbdtest.h"
#include "unicode/timezone.h"
#include "unicode/simpletz.h"
#include "unicode/gregocal.h"
#include "putilimp.h"

void TimeZoneBoundaryTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite TestTimeZoneBoundary");
    switch (index) {
        case 0:
            name = "TestBoundaries";
            if (exec) {
                logln("TestBoundaries---"); logln("");
                TestBoundaries();
            }
            break;
        case 1:
            name = "TestNewRules";
            if (exec) {
                logln("TestNewRules---"); logln("");
                TestNewRules();
            }
            break;
        case 2:
            name = "TestStepwise";
            if (exec) {
                logln("TestStepwise---"); logln("");
                TestStepwise();
            }
            break;
        default: name = ""; break;
    }
}

// *****************************************************************************
// class TimeZoneBoundaryTest
// *****************************************************************************

TimeZoneBoundaryTest::TimeZoneBoundaryTest()
:
ONE_SECOND(1000),
ONE_MINUTE(60 * ONE_SECOND),
ONE_HOUR(60 * ONE_MINUTE),
ONE_DAY(24 * ONE_HOUR),
ONE_YEAR(uprv_floor(365.25 * ONE_DAY)),
SIX_MONTHS(ONE_YEAR / 2)
{
}

const int32_t TimeZoneBoundaryTest::MONTH_LENGTH[] = {
    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
 
const UDate TimeZoneBoundaryTest::PST_1997_BEG = 860320800000.0;
 
const UDate TimeZoneBoundaryTest::PST_1997_END = 877856400000.0;
 
const UDate TimeZoneBoundaryTest::INTERVAL = 10;
 
// -------------------------------------
 
void
TimeZoneBoundaryTest::findDaylightBoundaryUsingDate(UDate d, const char* startMode, UDate expectedBoundary)
{
    UnicodeString str;
    if (dateToString(d, str).indexOf(startMode) == - 1) {
        logln(UnicodeString("Error: ") + startMode + " not present in " + str);
    }
    UDate min = d;
    UDate max = min + SIX_MONTHS;
    while ((max - min) > INTERVAL) {
        UDate mid = (min + max) / 2;
        UnicodeString* s = &dateToString(mid, str);
        if (s->indexOf(startMode) != - 1) {
            min = mid;
        }
        else {
            max = mid;
        }
    }
    logln("Date Before: " + showDate(min));
    logln("Date After:  " + showDate(max));
    UDate mindelta = expectedBoundary - min;
    UDate maxdelta = max - expectedBoundary;
    if (mindelta >= 0 &&
        mindelta <= INTERVAL &&
        maxdelta >= 0 &&
        maxdelta <= INTERVAL) logln(UnicodeString("PASS: Expected boundary at ") + expectedBoundary);
    else dataerrln(UnicodeString("FAIL: Expected boundary at ") + expectedBoundary);
}
 
// -------------------------------------
 
void
TimeZoneBoundaryTest::findDaylightBoundaryUsingTimeZone(UDate d, UBool startsInDST, UDate expectedBoundary)
{
    TimeZone *zone = TimeZone::createDefault();
    findDaylightBoundaryUsingTimeZone(d, startsInDST, expectedBoundary, zone);
    delete zone;
}
 
// -------------------------------------
 
void
TimeZoneBoundaryTest::findDaylightBoundaryUsingTimeZone(UDate d, UBool startsInDST, UDate expectedBoundary, TimeZone* tz)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    UDate min = d;
    UDate max = min + SIX_MONTHS;
    if (tz->inDaylightTime(d, status) != startsInDST) {
        dataerrln("FAIL: " + tz->getID(str) + " inDaylightTime(" + dateToString(d) + ") != " + (startsInDST ? "true" : "false"));
        startsInDST = !startsInDST;
    }
    if (failure(status, "TimeZone::inDaylightTime", TRUE)) return;
    if (tz->inDaylightTime(max, status) == startsInDST) {
        dataerrln("FAIL: " + tz->getID(str) + " inDaylightTime(" + dateToString(max) + ") != " + (startsInDST ? "false" : "true"));
        return;
    }
    if (failure(status, "TimeZone::inDaylightTime")) return;
    while ((max - min) > INTERVAL) {
        UDate mid = (min + max) / 2;
        UBool isIn = tz->inDaylightTime(mid, status);
        if (failure(status, "TimeZone::inDaylightTime")) return;
        if (isIn == startsInDST) {
            min = mid;
        }
        else {
            max = mid;
        }
    }
    logln(tz->getID(str) + " Before: " + showDate(min));
    logln(tz->getID(str) + " After:  " + showDate(max));
    UDate mindelta = expectedBoundary - min;
    UDate maxdelta = max - expectedBoundary;
    if (mindelta >= 0 &&
        mindelta <= INTERVAL &&
        maxdelta >= 0 &&
        maxdelta <= INTERVAL) logln(UnicodeString("PASS: Expected boundary at ") + expectedBoundary);
    else errln(UnicodeString("FAIL: Expected boundary at ") + expectedBoundary);
}
 
// -------------------------------------
/*
UnicodeString*
TimeZoneBoundaryTest::showDate(int32_t l)
{
    return showDate(new Date(l));
}
*/
// -------------------------------------
 
UnicodeString
TimeZoneBoundaryTest::showDate(UDate d)
{
    int32_t y, m, day, h, min, sec;
    dateToFields(d, y, m, day, h, min, sec);
    return UnicodeString("") + y + "/" + showNN(m + 1) + "/" +
        showNN(day) + " " + showNN(h) + ":" + showNN(min) +
        " \"" + dateToString(d) + "\" = " + uprv_floor(d+0.5);
}
 
// -------------------------------------
 
UnicodeString
TimeZoneBoundaryTest::showNN(int32_t n)
{
    UnicodeString nStr;
    if (n < 10) {
        nStr += UnicodeString("0", "");
    }
    return nStr + n;
}
 
// -------------------------------------
 
void
TimeZoneBoundaryTest::verifyDST(UDate d, TimeZone* time_zone, UBool expUseDaylightTime, UBool expInDaylightTime, UDate expZoneOffset, UDate expDSTOffset)
{
    UnicodeString str;
    UErrorCode status = U_ZERO_ERROR;
    logln("-- Verifying time " + dateToString(d) + " in zone " + time_zone->getID(str));
    if (time_zone->inDaylightTime(d, status) == expInDaylightTime)
        logln(UnicodeString("PASS: inDaylightTime = ") + (time_zone->inDaylightTime(d, status)?"true":"false"));
    else 
        dataerrln(UnicodeString("FAIL: inDaylightTime = ") + (time_zone->inDaylightTime(d, status)?"true":"false"));
    if (failure(status, "TimeZone::inDaylightTime", TRUE)) 
        return;
    if (time_zone->useDaylightTime() == expUseDaylightTime)
        logln(UnicodeString("PASS: useDaylightTime = ") + (time_zone->useDaylightTime()?"true":"false"));
    else 
        dataerrln(UnicodeString("FAIL: useDaylightTime = ") + (time_zone->useDaylightTime()?"true":"false"));
    if (time_zone->getRawOffset() == expZoneOffset) 
        logln(UnicodeString("PASS: getRawOffset() = ") + (expZoneOffset / ONE_HOUR));
    else
        dataerrln(UnicodeString("FAIL: getRawOffset() = ") + (time_zone->getRawOffset() / ONE_HOUR) + ";  expected " + (expZoneOffset / ONE_HOUR));
    
    GregorianCalendar *gc = new GregorianCalendar(time_zone->clone(), status);
    gc->setTime(d, status);
    if (failure(status, "GregorianCalendar::setTime")) return;
    int32_t offset = time_zone->getOffset((uint8_t)gc->get(UCAL_ERA, status),
        gc->get(UCAL_YEAR, status), gc->get(UCAL_MONTH, status),
        gc->get(UCAL_DATE, status), (uint8_t)gc->get(UCAL_DAY_OF_WEEK, status),
        ((gc->get(UCAL_HOUR_OF_DAY, status) * 60 + gc->get(UCAL_MINUTE, status)) * 60 + gc->get(UCAL_SECOND, status)) * 1000 + gc->get(UCAL_MILLISECOND, status),
        status);
    if (failure(status, "GregorianCalendar::get")) return;
    if (offset == expDSTOffset) logln(UnicodeString("PASS: getOffset() = ") + (offset / ONE_HOUR));
    else dataerrln(UnicodeString("FAIL: getOffset() = ") + (offset / ONE_HOUR) + "; expected " + (expDSTOffset / ONE_HOUR));
    delete gc;
}
 
// -------------------------------------
/**
    * Check that the given year/month/dom/hour maps to and from the
    * given epochHours.  This verifies the functioning of the
    * calendar and time zone in conjunction with one another,
    * including the calendar time->fields and fields->time and
    * the time zone getOffset method.
    *
    * @param epochHours hours after Jan 1 1970 0:00 GMT.
    */
void TimeZoneBoundaryTest::verifyMapping(Calendar& cal, int year, int month, int dom, int hour,
                    double epochHours) {
    double H = 3600000.0;
    UErrorCode status = U_ZERO_ERROR;
    cal.clear();
    cal.set(year, month, dom, hour, 0, 0);
    UDate e = cal.getTime(status)/ H;
    UDate ed = (epochHours * H);
    if (e == epochHours) {
        logln(UnicodeString("Ok: ") + year + "/" + (month+1) + "/" + dom + " " + hour + ":00 => " +
                e + " (" + ed + ")");
    } else {
        dataerrln(UnicodeString("FAIL: ") + year + "/" + (month+1) + "/" + dom + " " + hour + ":00 => " +
                e + " (" + (e * H) + ")" +
                ", expected " + epochHours + " (" + ed + ")");
    }
    cal.setTime(ed, status);
    if (cal.get(UCAL_YEAR, status) == year &&
        cal.get(UCAL_MONTH, status) == month &&
        cal.get(UCAL_DATE, status) == dom &&
        cal.get(UCAL_MILLISECONDS_IN_DAY, status) == hour * 3600000) {
        logln(UnicodeString("Ok: ") + epochHours + " (" + ed + ") => " +
                cal.get(UCAL_YEAR, status) + "/" +
                (cal.get(UCAL_MONTH, status)+1) + "/" +
                cal.get(UCAL_DATE, status) + " " +
                cal.get(UCAL_MILLISECOND, status)/H);
    } else {
        dataerrln(UnicodeString("FAIL: ") + epochHours + " (" + ed + ") => " +
                cal.get(UCAL_YEAR, status) + "/" +
                (cal.get(UCAL_MONTH, status)+1) + "/" +
                cal.get(UCAL_DATE, status) + " " +
                cal.get(UCAL_MILLISECOND, status)/H +
                ", expected " + year + "/" + (month+1) + "/" + dom +
                " " + hour);
    }
}

/**
 * Test the behavior of SimpleTimeZone at the transition into and out of DST.
 * Use a binary search to find boundaries.
 */
void
TimeZoneBoundaryTest::TestBoundaries()
{
    UErrorCode status = U_ZERO_ERROR;
    TimeZone* pst = TimeZone::createTimeZone("PST");
    Calendar* tempcal = Calendar::createInstance(pst, status);
    if(U_SUCCESS(status)){
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 3,  0, 238904.0);
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 4,  0, 238928.0);
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 5,  0, 238952.0);
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 5, 23, 238975.0);
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 6,  0, 238976.0);
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 6,  1, 238977.0);
        verifyMapping(*tempcal, 1997, Calendar::APRIL, 6,  3, 238978.0);
    }else{
        dataerrln("Could not create calendar. Error: %s", u_errorName(status));
    }
    TimeZone* utc = TimeZone::createTimeZone("UTC");
    Calendar* utccal =  Calendar::createInstance(utc, status);
    if(U_SUCCESS(status)){
        verifyMapping(*utccal, 1997, Calendar::APRIL, 6, 0, 238968.0);
    }else{
        dataerrln("Could not create calendar. Error: %s", u_errorName(status));
    }
    TimeZone* save = TimeZone::createDefault();
    TimeZone::setDefault(*pst);
   
    if (tempcal != NULL) { 
        // DST changeover for PST is 4/6/1997 at 2 hours past midnight
        // at 238978.0 epoch hours.
        tempcal->clear();
        tempcal->set(1997, Calendar::APRIL, 6);
        UDate d = tempcal->getTime(status);

        // i is minutes past midnight standard time
        for (int i=-120; i<=180; i+=60)
        {
            UBool inDST = (i >= 120);
            tempcal->setTime(d + i*60*1000, status);
            verifyDST(tempcal->getTime(status),pst, TRUE, inDST, -8*ONE_HOUR,inDST ? -7*ONE_HOUR : -8*ONE_HOUR);
        }
    }
    TimeZone::setDefault(*save);
    delete save;
    delete utccal;
    delete tempcal;

#if 1
    {
        logln("--- Test a ---");
        UDate d = date(97, UCAL_APRIL, 6);
        TimeZone *z = TimeZone::createTimeZone("PST");
        for (int32_t i = 60; i <= 180; i += 15) {
            UBool inDST = (i >= 120);
            UDate e = d + i * 60 * 1000;
            verifyDST(e, z, TRUE, inDST, - 8 * ONE_HOUR, inDST ? - 7 * ONE_HOUR: - 8 * ONE_HOUR);
        }
        delete z;
    }
#endif
#if 1
    {
        logln("--- Test b ---");
        TimeZone *tz;
        TimeZone::setDefault(*(tz = TimeZone::createTimeZone("PST")));
        delete tz;
        logln("========================================");
        findDaylightBoundaryUsingDate(date(97, 0, 1), "PST", PST_1997_BEG);
        logln("========================================");
        findDaylightBoundaryUsingDate(date(97, 6, 1), "PDT", PST_1997_END);
    }
#endif
#if 1
    {
        logln("--- Test c ---");
        logln("========================================");
        TimeZone* z = TimeZone::createTimeZone("Australia/Adelaide");
        findDaylightBoundaryUsingTimeZone(date(97, 0, 1), TRUE, 859653000000.0, z);
        logln("========================================");
        findDaylightBoundaryUsingTimeZone(date(97, 6, 1), FALSE, 877797000000.0, z);
        delete z;
    }
#endif
#if 1
    {
        logln("--- Test d ---");
        logln("========================================");
        findDaylightBoundaryUsingTimeZone(date(97, 0, 1), FALSE, PST_1997_BEG);
        logln("========================================");
        findDaylightBoundaryUsingTimeZone(date(97, 6, 1), TRUE, PST_1997_END);
    }
#endif
#if 0
    {
        logln("--- Test e ---");
        TimeZone *z = TimeZone::createDefault();
        logln(UnicodeString("") + z->getOffset(1, 97, 3, 4, 6, 0) + " " + date(97, 3, 4));
        logln(UnicodeString("") + z->getOffset(1, 97, 3, 5, 7, 0) + " " + date(97, 3, 5));
        logln(UnicodeString("") + z->getOffset(1, 97, 3, 6, 1, 0) + " " + date(97, 3, 6));
        logln(UnicodeString("") + z->getOffset(1, 97, 3, 7, 2, 0) + " " + date(97, 3, 7));
        delete z;
    }
#endif
}
 
// -------------------------------------
 
void
TimeZoneBoundaryTest::testUsingBinarySearch(SimpleTimeZone* tz, UDate d, UDate expectedBoundary)
{
    UErrorCode status = U_ZERO_ERROR;
    UDate min = d;
    UDate max = min + SIX_MONTHS;
    UBool startsInDST = tz->inDaylightTime(d, status);
    if (failure(status, "SimpleTimeZone::inDaylightTime", TRUE)) return;
    if (tz->inDaylightTime(max, status) == startsInDST) {
        errln("Error: inDaylightTime(" + dateToString(max) + ") != " + ((!startsInDST)?"true":"false"));
    }
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return;
    while ((max - min) > INTERVAL) {
        UDate mid = (min + max) / 2;
        if (tz->inDaylightTime(mid, status) == startsInDST) {
            min = mid;
        }
        else {
            max = mid;
        }
        if (failure(status, "SimpleTimeZone::inDaylightTime")) return;
    }
    logln("Binary Search Before: " + showDate(min));
    logln("Binary Search After:  " + showDate(max));
    UDate mindelta = expectedBoundary - min;
    UDate maxdelta = max - expectedBoundary;
    if (mindelta >= 0 &&
        mindelta <= INTERVAL &&
        maxdelta >= 0 &&
        maxdelta <= INTERVAL) logln(UnicodeString("PASS: Expected boundary at ") + expectedBoundary);
    else errln(UnicodeString("FAIL: Expected boundary at ") + expectedBoundary);
}
 
// -------------------------------------
 
/**
 * Test the handling of the "new" rules; that is, rules other than nth Day of week.
 */
void
TimeZoneBoundaryTest::TestNewRules()
{
#if 1
    {
        UErrorCode status = U_ZERO_ERROR;
        SimpleTimeZone *tz;
        logln("-----------------------------------------------------------------");
        logln("Aug 2ndTues .. Mar 15");
        tz = new SimpleTimeZone(- 8 * (int32_t)ONE_HOUR, "Test_1", UCAL_AUGUST, 2, UCAL_TUESDAY, 2 * (int32_t)ONE_HOUR, UCAL_MARCH, 15, 0, 2 * (int32_t)ONE_HOUR, status);
        logln("========================================");
        testUsingBinarySearch(tz, date(97, 0, 1), 858416400000.0);
        logln("========================================");
        testUsingBinarySearch(tz, date(97, 6, 1), 871380000000.0);
        delete tz;
        logln("-----------------------------------------------------------------");
        logln("Apr Wed>=14 .. Sep Sun<=20");
        tz = new SimpleTimeZone(- 8 * (int32_t)ONE_HOUR, "Test_2", UCAL_APRIL, 14, - UCAL_WEDNESDAY, 2 *(int32_t)ONE_HOUR, UCAL_SEPTEMBER, - 20, - UCAL_SUNDAY, 2 * (int32_t)ONE_HOUR, status);
        logln("========================================");
        testUsingBinarySearch(tz, date(97, 0, 1), 861184800000.0);
        logln("========================================");
        testUsingBinarySearch(tz, date(97, 6, 1), 874227600000.0);
        delete tz;
    }
#endif
}
 
// -------------------------------------
 
void
TimeZoneBoundaryTest::findBoundariesStepwise(int32_t year, UDate interval, TimeZone* z, int32_t expectedChanges)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    UDate d = date(year - 1900, UCAL_JANUARY, 1);
    UDate time = d;
    UDate limit = time + ONE_YEAR + ONE_DAY;
    UBool lastState = z->inDaylightTime(d, status);
    if (failure(status, "TimeZone::inDaylightTime", TRUE)) return;
    int32_t changes = 0;
    logln(UnicodeString("-- Zone ") + z->getID(str) + " starts in " + year + " with DST = " + (lastState?"true":"false"));
    logln(UnicodeString("useDaylightTime = ") + (z->useDaylightTime()?"true":"false"));
    while (time < limit) {
        d = time;
        UBool state = z->inDaylightTime(d, status);
        if (failure(status, "TimeZone::inDaylightTime")) return;
        if (state != lastState) {
            logln(UnicodeString(state ? "Entry ": "Exit ") + "at " + d);
            lastState = state;++changes;
        }
        time += interval;
    }
    if (changes == 0) {
        if (!lastState &&
            !z->useDaylightTime()) logln("No DST");
        else errln("FAIL: DST all year, or no DST with true useDaylightTime");
    }
    else if (changes != 2) {
        errln(UnicodeString("FAIL: ") + changes + " changes seen; should see 0 or 2");
    }
    else if (!z->useDaylightTime()) {
        errln("FAIL: useDaylightTime false but 2 changes seen");
    }
    if (changes != expectedChanges) {
        dataerrln(UnicodeString("FAIL: ") + changes + " changes seen; expected " + expectedChanges);
    }
}
 
// -------------------------------------

/**
 * This test is problematic. It makes assumptions about the behavior
 * of specific zones. Since ICU's zone table is based on the Olson
 * zones (the UNIX zones), and those change from time to time, this
 * test can fail after a zone table update. If that happens, the
 * selected zones need to be updated to have the behavior
 * expected. That is, they should have DST, not have DST, and have DST
 * -- other than that this test isn't picky. 12/3/99 aliu
 *
 * Test the behavior of SimpleTimeZone at the transition into and out of DST.
 * Use a stepwise march to find boundaries.
 */
void
TimeZoneBoundaryTest::TestStepwise()
{
    TimeZone *zone =  TimeZone::createTimeZone("America/New_York");
    findBoundariesStepwise(1997, ONE_DAY, zone, 2);
    delete zone;
    zone = TimeZone::createTimeZone("UTC"); // updated 12/3/99 aliu
    findBoundariesStepwise(1997, ONE_DAY, zone, 0);
    delete zone;
    zone = TimeZone::createTimeZone("Australia/Adelaide");
    findBoundariesStepwise(1997, ONE_DAY, zone, 2);
    delete zone;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
