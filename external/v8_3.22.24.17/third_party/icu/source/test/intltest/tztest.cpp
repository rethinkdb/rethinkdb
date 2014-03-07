/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/timezone.h"
#include "unicode/simpletz.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/resbund.h"
#include "unicode/strenum.h"
#include "tztest.h"
#include "cmemory.h"
#include "putilimp.h"
#include "cstring.h"
#include "olsontz.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define CASE(id,test) case id:                               \
                          name = #test;                      \
                          if (exec) {                        \
                              logln(#test "---"); logln(""); \
                              test();                        \
                          }                                  \
                          break

// *****************************************************************************
// class TimeZoneTest
// *****************************************************************************

// TODO: We should probably read following data at runtime, so we can update
// these values every release with necessary data changes.
const int32_t TimeZoneTest::REFERENCE_YEAR = 2009;
const char * TimeZoneTest::REFERENCE_DATA_VERSION = "2009d";

void TimeZoneTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite TestTimeZone");
    switch (index) {
        CASE(0, TestPRTOffset);
        CASE(1, TestVariousAPI518);
        CASE(2, TestGetAvailableIDs913);
        CASE(3, TestGenericAPI);
        CASE(4, TestRuleAPI);
        CASE(5, TestShortZoneIDs);
        CASE(6, TestCustomParse);
        CASE(7, TestDisplayName);
        CASE(8, TestDSTSavings);
        CASE(9, TestAlternateRules);
        CASE(10,TestCountries); 
        CASE(11,TestHistorical);
        CASE(12,TestEquivalentIDs);
        CASE(13, TestAliasedNames);
        CASE(14, TestFractionalDST);
        CASE(15, TestFebruary);
        CASE(16, TestCanonicalID);
        CASE(17, TestDisplayNamesMeta);
       default: name = ""; break;
    }
}

const int32_t TimeZoneTest::millisPerHour = 3600000;

// ---------------------------------------------------------------------------------

/**
 * Generic API testing for API coverage.
 */
void
TimeZoneTest::TestGenericAPI()
{
    UnicodeString id("NewGMT");
    int32_t offset = 12345;

    SimpleTimeZone *zone = new SimpleTimeZone(offset, id);
    if (zone->useDaylightTime()) errln("FAIL: useDaylightTime should return FALSE");

    TimeZone* zoneclone = zone->clone();
    if (!(*zoneclone == *zone)) errln("FAIL: clone or operator== failed");
    zoneclone->setID("abc");
    if (!(*zoneclone != *zone)) errln("FAIL: clone or operator!= failed");
    delete zoneclone;

    zoneclone = zone->clone();
    if (!(*zoneclone == *zone)) errln("FAIL: clone or operator== failed");
    zoneclone->setRawOffset(45678);
    if (!(*zoneclone != *zone)) errln("FAIL: clone or operator!= failed");

    SimpleTimeZone copy(*zone);
    if (!(copy == *zone)) errln("FAIL: copy constructor or operator== failed");
    copy = *(SimpleTimeZone*)zoneclone;
    if (!(copy == *zoneclone)) errln("FAIL: assignment operator or operator== failed");

    TimeZone* saveDefault = TimeZone::createDefault();
    logln((UnicodeString)"TimeZone::createDefault() => " + saveDefault->getID(id));
    TimeZone* pstZone = TimeZone::createTimeZone("America/Los_Angeles");

    logln("call uprv_timezone() which uses the host");
    logln("to get the difference in seconds between coordinated universal");
    logln("time and local time. E.g., -28,800 for PST (GMT-8hrs)");

    int32_t tzoffset = uprv_timezone();
    logln(UnicodeString("Value returned from uprv_timezone = ") + tzoffset);
    // Invert sign because UNIX semantics are backwards
    if (tzoffset < 0)
        tzoffset = -tzoffset;
    if ((*saveDefault == *pstZone) && (tzoffset != 28800)) {
        errln("FAIL: t_timezone may be incorrect.  It is not 28800");
    }

    if ((tzoffset % 900) != 0) {
        /*
         * Ticket#6364 and #7648
         * A few time zones are using GMT offests not a multiple of 15 minutes.
         * Therefore, we should not interpret such case as an error.
         * We downgrade this from errln to infoln. When we see this message,
         * we should examine if it is ignorable or not.
         */
        infoln("WARNING: t_timezone may be incorrect. It is not a multiple of 15min.", tzoffset);
    }

    TimeZone::adoptDefault(zone);
    TimeZone* defaultzone = TimeZone::createDefault();
    if (defaultzone == zone ||
        !(*defaultzone == *zone))
        errln("FAIL: createDefault failed");
    TimeZone::adoptDefault(saveDefault);
    delete defaultzone;
    delete zoneclone;
    delete pstZone;

    UErrorCode status = U_ZERO_ERROR;
    const char* tzver = TimeZone::getTZDataVersion(status);
    if (U_FAILURE(status)) {
        errcheckln(status, "FAIL: getTZDataVersion failed - %s", u_errorName(status));
    } else if (uprv_strlen(tzver) != 5 /* 4 digits + 1 letter */) {
        errln((UnicodeString)"FAIL: getTZDataVersion returned " + tzver);
    } else {
        logln((UnicodeString)"tzdata version: " + tzver);
    }
}

// ---------------------------------------------------------------------------------

/**
 * Test the setStartRule/setEndRule API calls.
 */
void
TimeZoneTest::TestRuleAPI()
{
    UErrorCode status = U_ZERO_ERROR;

    UDate offset = 60*60*1000*1.75; // Pick a weird offset
    SimpleTimeZone *zone = new SimpleTimeZone((int32_t)offset, "TestZone");
    if (zone->useDaylightTime()) errln("FAIL: useDaylightTime should return FALSE");

    // Establish our expected transition times.  Do this with a non-DST
    // calendar with the (above) declared local offset.
    GregorianCalendar *gc = new GregorianCalendar(*zone, status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    gc->clear();
    gc->set(1990, UCAL_MARCH, 1);
    UDate marchOneStd = gc->getTime(status); // Local Std time midnight
    gc->clear();
    gc->set(1990, UCAL_JULY, 1);
    UDate julyOneStd = gc->getTime(status); // Local Std time midnight
    if (failure(status, "GregorianCalendar::getTime")) return;

    // Starting and ending hours, WALL TIME
    int32_t startHour = (int32_t)(2.25 * 3600000);
    int32_t endHour   = (int32_t)(3.5  * 3600000);

    zone->setStartRule(UCAL_MARCH, 1, 0, startHour, status);
    zone->setEndRule  (UCAL_JULY,  1, 0, endHour, status);

    delete gc;
    gc = new GregorianCalendar(*zone, status);
    if (failure(status, "new GregorianCalendar")) return;

    UDate marchOne = marchOneStd + startHour;
    UDate julyOne = julyOneStd + endHour - 3600000; // Adjust from wall to Std time

    UDate expMarchOne = 636251400000.0;
    if (marchOne != expMarchOne)
    {
        errln((UnicodeString)"FAIL: Expected start computed as " + marchOne +
          " = " + dateToString(marchOne));
        logln((UnicodeString)"      Should be                  " + expMarchOne +
          " = " + dateToString(expMarchOne));
    }

    UDate expJulyOne = 646793100000.0;
    if (julyOne != expJulyOne)
    {
        errln((UnicodeString)"FAIL: Expected start computed as " + julyOne +
          " = " + dateToString(julyOne));
        logln((UnicodeString)"      Should be                  " + expJulyOne +
          " = " + dateToString(expJulyOne));
    }

    testUsingBinarySearch(*zone, date(90, UCAL_JANUARY, 1), date(90, UCAL_JUNE, 15), marchOne);
    testUsingBinarySearch(*zone, date(90, UCAL_JUNE, 1), date(90, UCAL_DECEMBER, 31), julyOne);

    if (zone->inDaylightTime(marchOne - 1000, status) ||
        !zone->inDaylightTime(marchOne, status))
        errln("FAIL: Start rule broken");
    if (!zone->inDaylightTime(julyOne - 1000, status) ||
        zone->inDaylightTime(julyOne, status))
        errln("FAIL: End rule broken");

    zone->setStartYear(1991);
    if (zone->inDaylightTime(marchOne, status) ||
        zone->inDaylightTime(julyOne - 1000, status))
        errln("FAIL: Start year broken");

    failure(status, "TestRuleAPI");
    delete gc;
    delete zone;
}

void
TimeZoneTest::findTransition(const TimeZone& tz,
                             UDate min, UDate max) {
    UErrorCode ec = U_ZERO_ERROR;
    UnicodeString id,s;
    UBool startsInDST = tz.inDaylightTime(min, ec);
    if (failure(ec, "TimeZone::inDaylightTime")) return;
    if (tz.inDaylightTime(max, ec) == startsInDST) {
        logln("Error: " + tz.getID(id) + ".inDaylightTime(" + dateToString(min) + ") = " + (startsInDST?"TRUE":"FALSE") +
              ", inDaylightTime(" + dateToString(max) + ") = " + (startsInDST?"TRUE":"FALSE"));
        return;
    }
    if (failure(ec, "TimeZone::inDaylightTime")) return;
    while ((max - min) > INTERVAL) {
        UDate mid = (min + max) / 2;
        if (tz.inDaylightTime(mid, ec) == startsInDST) {
            min = mid;
        } else {
            max = mid;
        }
        if (failure(ec, "TimeZone::inDaylightTime")) return;
    }
    min = 1000.0 * uprv_floor(min/1000.0);
    max = 1000.0 * uprv_floor(max/1000.0);
    logln(tz.getID(id) + " Before: " + min/1000 + " = " +
          dateToString(min,s,tz));
    logln(tz.getID(id) + " After:  " + max/1000 + " = " +
          dateToString(max,s,tz));
}

void
TimeZoneTest::testUsingBinarySearch(const TimeZone& tz,
                                    UDate min, UDate max,
                                    UDate expectedBoundary)
{
    UErrorCode status = U_ZERO_ERROR;
    UBool startsInDST = tz.inDaylightTime(min, status);
    if (failure(status, "TimeZone::inDaylightTime")) return;
    if (tz.inDaylightTime(max, status) == startsInDST) {
        logln("Error: inDaylightTime(" + dateToString(max) + ") != " + ((!startsInDST)?"TRUE":"FALSE"));
        return;
    }
    if (failure(status, "TimeZone::inDaylightTime")) return;
    while ((max - min) > INTERVAL) {
        UDate mid = (min + max) / 2;
        if (tz.inDaylightTime(mid, status) == startsInDST) {
            min = mid;
        } else {
            max = mid;
        }
        if (failure(status, "TimeZone::inDaylightTime")) return;
    }
    logln(UnicodeString("Binary Search Before: ") + uprv_floor(0.5 + min) + " = " + dateToString(min));
    logln(UnicodeString("Binary Search After:  ") + uprv_floor(0.5 + max) + " = " + dateToString(max));
    UDate mindelta = expectedBoundary - min;
    UDate maxdelta = max - expectedBoundary;
    if (mindelta >= 0 &&
        mindelta <= INTERVAL &&
        maxdelta >= 0 &&
        maxdelta <= INTERVAL)
        logln(UnicodeString("PASS: Expected bdry:  ") + expectedBoundary + " = " + dateToString(expectedBoundary));
    else
        errln(UnicodeString("FAIL: Expected bdry:  ") + expectedBoundary + " = " + dateToString(expectedBoundary));
}

const UDate TimeZoneTest::INTERVAL = 100;

// ---------------------------------------------------------------------------------

// -------------------------------------

/**
 * Test the offset of the PRT timezone.
 */
void
TimeZoneTest::TestPRTOffset()
{
    TimeZone* tz = TimeZone::createTimeZone("PRT");
    if (tz == 0) {
        errln("FAIL: TimeZone(PRT) is null");
    }
    else {
      int32_t expectedHour = -4;
      double expectedOffset = (((double)expectedHour) * millisPerHour);
      double foundOffset = tz->getRawOffset();
      int32_t foundHour = (int32_t)foundOffset / millisPerHour;
      if (expectedOffset != foundOffset) {
        dataerrln("FAIL: Offset for PRT should be %d, found %d", expectedHour, foundHour);
      } else {
        logln("PASS: Offset for PRT should be %d, found %d", expectedHour, foundHour);
      }
    }
    delete tz;
}

// -------------------------------------

/**
 * Regress a specific bug with a sequence of API calls.
 */
void
TimeZoneTest::TestVariousAPI518()
{
    UErrorCode status = U_ZERO_ERROR;
    TimeZone* time_zone = TimeZone::createTimeZone("PST");
    UDate d = date(97, UCAL_APRIL, 30);
    UnicodeString str;
    logln("The timezone is " + time_zone->getID(str));
    if (!time_zone->inDaylightTime(d, status)) dataerrln("FAIL: inDaylightTime returned FALSE");
    if (failure(status, "TimeZone::inDaylightTime", TRUE)) return;
    if (!time_zone->useDaylightTime()) dataerrln("FAIL: useDaylightTime returned FALSE");
    if (time_zone->getRawOffset() != - 8 * millisPerHour) dataerrln("FAIL: getRawOffset returned wrong value");
    GregorianCalendar *gc = new GregorianCalendar(status);
    if (U_FAILURE(status)) { errln("FAIL: Couldn't create GregorianCalendar"); return; }
    gc->setTime(d, status);
    if (U_FAILURE(status)) { errln("FAIL: GregorianCalendar::setTime failed"); return; }
    if (time_zone->getOffset(gc->AD, gc->get(UCAL_YEAR, status), gc->get(UCAL_MONTH, status),
        gc->get(UCAL_DATE, status), (uint8_t)gc->get(UCAL_DAY_OF_WEEK, status), 0, status) != - 7 * millisPerHour)
        dataerrln("FAIL: getOffset returned wrong value");
    if (U_FAILURE(status)) { errln("FAIL: GregorianCalendar::set failed"); return; }
    delete gc;
    delete time_zone;
}

// -------------------------------------

/**
 * Test the call which retrieves the available IDs.
 */
void
TimeZoneTest::TestGetAvailableIDs913()
{
    UErrorCode ec = U_ZERO_ERROR;
    int32_t i;

#ifdef U_USE_TIMEZONE_OBSOLETE_2_8
    // Test legacy API -- remove these tests when the corresponding API goes away (duh)
    int32_t numIDs = -1;
    const UnicodeString** ids = TimeZone::createAvailableIDs(numIDs);
    if (ids == 0 || numIDs < 1) {
        errln("FAIL: createAvailableIDs()");
    } else {
        UnicodeString buf("TimeZone::createAvailableIDs() = { ");
        for(i=0; i<numIDs; ++i) {
            if (i) buf.append(", ");
            buf.append(*ids[i]);
        }
        buf.append(" } ");
        logln(buf + numIDs);
        // we own the array; the caller owns the contained strings (yuck)
        uprv_free(ids);
    }

    numIDs = -1;
    ids = TimeZone::createAvailableIDs(-8*U_MILLIS_PER_HOUR, numIDs);
    if (ids == 0 || numIDs < 1) {
        errln("FAIL: createAvailableIDs(-8:00)");
    } else {
        UnicodeString buf("TimeZone::createAvailableIDs(-8:00) = { ");
        for(i=0; i<numIDs; ++i) {
            if (i) buf.append(", ");
            buf.append(*ids[i]);
        }
        buf.append(" } ");
        logln(buf + numIDs);
        // we own the array; the caller owns the contained strings (yuck)
        uprv_free(ids);
    }
    numIDs = -1;
    ids = TimeZone::createAvailableIDs("US", numIDs);
    if (ids == 0 || numIDs < 1) {
      errln("FAIL: createAvailableIDs(US) ids=%d, numIDs=%d", ids, numIDs);
    } else {
        UnicodeString buf("TimeZone::createAvailableIDs(US) = { ");
        for(i=0; i<numIDs; ++i) {
            if (i) buf.append(", ");
            buf.append(*ids[i]);
        }
        buf.append(" } ");
        logln(buf + numIDs);
        // we own the array; the caller owns the contained strings (yuck)
        uprv_free(ids);
    }
#endif

    UnicodeString str;
    UnicodeString *buf = new UnicodeString("TimeZone::createEnumeration() = { ");
    int32_t s_length;
    StringEnumeration* s = TimeZone::createEnumeration();
    s_length = s->count(ec);
    for (i = 0; i < s_length;++i) {
        if (i > 0) *buf += ", ";
        if ((i & 1) == 0) {
            *buf += *s->snext(ec);
        } else {
            *buf += UnicodeString(s->next(NULL, ec), "");
        }

        if((i % 5) == 4) {
            // replace s with a clone of itself
            StringEnumeration *s2 = s->clone();
            if(s2 == NULL || s_length != s2->count(ec)) {
                errln("TimezoneEnumeration.clone() failed");
            } else {
                delete s;
                s = s2;
            }
        }
    }
    *buf += " };";
    logln(*buf);

    /* Confirm that the following zones can be retrieved: The first
     * zone, the last zone, and one in-between.  This tests the binary
     * search through the system zone data.
     */
    s->reset(ec);
    int32_t middle = s_length/2;
    for (i=0; i<s_length; ++i) {
        const UnicodeString* id = s->snext(ec);
        if (i==0 || i==middle || i==(s_length-1)) {
        TimeZone *z = TimeZone::createTimeZone(*id);
        if (z == 0) {
            errln(UnicodeString("FAIL: createTimeZone(") +
                  *id + ") -> 0");
        } else if (z->getID(str) != *id) {
            errln(UnicodeString("FAIL: createTimeZone(") +
                  *id + ") -> zone " + str);
        } else {
            logln(UnicodeString("OK: createTimeZone(") +
                  *id + ") succeeded");
        }
        delete z;
        }
    }
    delete s;

    buf->truncate(0);
    *buf += "TimeZone::createEnumeration(GMT+01:00) = { ";

    s = TimeZone::createEnumeration(1 * U_MILLIS_PER_HOUR);
    s_length = s->count(ec);
    for (i = 0; i < s_length;++i) {
        if (i > 0) *buf += ", ";
        *buf += *s->snext(ec);
    }
    delete s;
    *buf += " };";
    logln(*buf);


    buf->truncate(0);
    *buf += "TimeZone::createEnumeration(US) = { ";

    s = TimeZone::createEnumeration("US");
    s_length = s->count(ec);
    for (i = 0; i < s_length;++i) {
        if (i > 0) *buf += ", ";
        *buf += *s->snext(ec);
    }
    *buf += " };";
    logln(*buf);

    TimeZone *tz = TimeZone::createTimeZone("PST");
    if (tz != 0) logln("getTimeZone(PST) = " + tz->getID(str));
    else errln("FAIL: getTimeZone(PST) = null");
    delete tz;
    tz = TimeZone::createTimeZone("America/Los_Angeles");
    if (tz != 0) logln("getTimeZone(America/Los_Angeles) = " + tz->getID(str));
    else errln("FAIL: getTimeZone(PST) = null");
    delete tz;

    // @bug 4096694
    tz = TimeZone::createTimeZone("NON_EXISTENT");
    UnicodeString temp;
    if (tz == 0)
        errln("FAIL: getTimeZone(NON_EXISTENT) = null");
    else if (tz->getID(temp) != "GMT")
        errln("FAIL: getTimeZone(NON_EXISTENT) = " + temp);
    delete tz;

    delete buf;
    delete s;
}


/**
 * NOTE: As of ICU 2.8, this test confirms that the "tz.alias"
 * file, used to build ICU alias zones, is working.  It also
 * looks at some genuine Olson compatibility IDs. [aliu]
 *
 * This test is problematic. It should really just confirm that
 * the list of compatibility zone IDs exist and are somewhat
 * meaningful (that is, they aren't all aliases of GMT). It goes a
 * bit further -- it hard-codes expectations about zone behavior,
 * when in fact zones are redefined quite frequently. ICU's build
 * process means that it is easy to update ICU to contain the
 * latest Olson zone data, but if a zone tested here changes, then
 * this test will fail.  I have updated the test for 1999j data,
 * but further updates will probably be required. Note that some
 * of the concerts listed below no longer apply -- in particular,
 * we do NOT overwrite real UNIX zones with 3-letter IDs. There
 * are two points of overlap as of 1999j: MET and EET. These are
 * both real UNIX zones, so we just use the official
 * definition. This test has been updated to reflect this.
 * 12/3/99 aliu
 *
 * Added tests for additional zones and aliases from the icuzones file.
 * Markus Scherer 2006-nov-06
 *
 * [srl - from java - 7/5/1998]
 * @bug 4130885
 * Certain short zone IDs, used since 1.1.x, are incorrect.
 *
 * The worst of these is:
 *
 * "CAT" (Central African Time) should be GMT+2:00, but instead returns a
 * zone at GMT-1:00. The zone at GMT-1:00 should be called EGT, CVT, EGST,
 * or AZOST, depending on which zone is meant, but in no case is it CAT.
 *
 * Other wrong zone IDs:
 *
 * ECT (European Central Time) GMT+1:00: ECT is Ecuador Time,
 * GMT-5:00. European Central time is abbreviated CEST.
 *
 * SST (Solomon Island Time) GMT+11:00. SST is actually Samoa Standard Time,
 * GMT-11:00. Solomon Island time is SBT.
 *
 * NST (New Zealand Time) GMT+12:00. NST is the abbreviation for
 * Newfoundland Standard Time, GMT-3:30. New Zealanders use NZST.
 *
 * AST (Alaska Standard Time) GMT-9:00. [This has already been noted in
 * another bug.] It should be "AKST". AST is Atlantic Standard Time,
 * GMT-4:00.
 *
 * PNT (Phoenix Time) GMT-7:00. PNT usually means Pitcairn Time,
 * GMT-8:30. There is no standard abbreviation for Phoenix time, as distinct
 * from MST with daylight savings.
 *
 * In addition to these problems, a number of zones are FAKE. That is, they
 * don't match what people use in the real world.
 *
 * FAKE zones:
 *
 * EET (should be EEST)
 * ART (should be EEST)
 * MET (should be IRST)
 * NET (should be AMST)
 * PLT (should be PKT)
 * BST (should be BDT)
 * VST (should be ICT)
 * CTT (should be CST) +
 * ACT (should be CST) +
 * AET (should be EST) +
 * MIT (should be WST) +
 * IET (should be EST) +
 * PRT (should be AST) +
 * CNT (should be NST)
 * AGT (should be ARST)
 * BET (should be EST) +
 *
 * + A zone with the correct name already exists and means something
 * else. E.g., EST usually indicates the US Eastern zone, so it cannot be
 * used for Brazil (BET).
 */
void TimeZoneTest::TestShortZoneIDs()
{
    UErrorCode status = U_ZERO_ERROR;

    // This test case is tzdata version sensitive.
    UBool isNonReferenceTzdataVersion = FALSE;
    const char *tzdataVer = TimeZone::getTZDataVersion(status);
    if (failure(status, "getTZDataVersion")) return;
    if (uprv_strcmp(tzdataVer, TimeZoneTest::REFERENCE_DATA_VERSION) != 0) {
        // Note: We want to display a warning message here if
        // REFERENCE_DATA_VERSION is out of date - so we
        // do not forget to update the value before GA.
        isNonReferenceTzdataVersion = TRUE;
        logln(UnicodeString("Warning: Active tzdata version (") + tzdataVer +
            ") does not match the reference tzdata version ("
            + REFERENCE_DATA_VERSION + ") for this test case data.");
    }

    // Note: useDaylightTime returns true if DST is observed
    // in the time zone in the current calendar year.  The test
    // data is valid for the date after the reference year below.
    // If system clock is before the year, some test cases may
    // fail.
    GregorianCalendar cal(*TimeZone::getGMT(), status);
    if (failure(status, "GregorianCalendar")) return;
    cal.set(TimeZoneTest::REFERENCE_YEAR, UCAL_JANUARY, 2); // day 2 in GMT

    UBool isDateBeforeReferenceYear = ucal_getNow() < cal.getTime(status);
    if (failure(status, "Calendar::getTime")) return;
    if (isDateBeforeReferenceYear) {
        logln("Warning: Past time is set to the system clock.  Some test cases may not return expected results.");
    }

    int32_t i;
    // Create a small struct to hold the array
    struct
    {
        const char *id;
        int32_t    offset;
        UBool      daylight;
    }
    kReferenceList [] =
    {
        {"MIT", -660, FALSE},
        {"HST", -600, FALSE},
        {"AST", -540, TRUE},
        {"PST", -480, TRUE},
        {"PNT", -420, FALSE},
        {"MST", -420, FALSE}, // updated Aug 2003 aliu
        {"CST", -360, TRUE},
        {"IET", -300, TRUE},  // updated Jan 2006 srl
        {"EST", -300, FALSE}, // updated Aug 2003 aliu
        {"PRT", -240, FALSE},
        {"CNT", -210, TRUE},
        {"AGT", -180, TRUE}, // updated by tzdata2007k
        {"BET", -180, TRUE},
        {"GMT", 0, FALSE},
        {"UTC", 0, FALSE}, // ** srl: seems broken in C++
        {"ECT", 60, TRUE},
        {"MET", 60, TRUE}, // updated 12/3/99 aliu
        {"ART", 120, TRUE},
        {"EET", 120, TRUE},
        {"CAT", 120, FALSE}, // Africa/Harare
        {"EAT", 180, FALSE},
        {"NET", 240, TRUE}, // updated 12/3/99 aliu
        {"PLT", 300, FALSE}, // updated by 2008c - no DST after 2008
        {"IST", 330, FALSE},
        {"BST", 360, FALSE},
        {"VST", 420, FALSE},
        {"CTT", 480, FALSE}, // updated Aug 2003 aliu
        {"JST", 540, FALSE},
        {"ACT", 570, FALSE}, // updated Aug 2003 aliu
        {"AET", 600, TRUE},
        {"SST", 660, FALSE},
        {"NST", 720, TRUE}, // Pacific/Auckland

        // From icuzones:
        {"Etc/Unknown", 0, FALSE},

        {"SystemV/AST4ADT", -240, TRUE},
        {"SystemV/EST5EDT", -300, TRUE},
        {"SystemV/CST6CDT", -360, TRUE},
        {"SystemV/MST7MDT", -420, TRUE},
        {"SystemV/PST8PDT", -480, TRUE},
        {"SystemV/YST9YDT", -540, TRUE},
        {"SystemV/AST4", -240, FALSE},
        {"SystemV/EST5", -300, FALSE},
        {"SystemV/CST6", -360, FALSE},
        {"SystemV/MST7", -420, FALSE},
        {"SystemV/PST8", -480, FALSE},
        {"SystemV/YST9", -540, FALSE},
        {"SystemV/HST10", -600, FALSE},

        {"",0,FALSE}
    };

    for(i=0;kReferenceList[i].id[0];i++) {
        UnicodeString itsID(kReferenceList[i].id);
        UBool ok = TRUE;
        // Check existence.
        TimeZone *tz = TimeZone::createTimeZone(itsID);
        if (!tz || (kReferenceList[i].offset != 0 && *tz == *TimeZone::getGMT())) {
            errln("FAIL: Time Zone " + itsID + " does not exist!");
            continue;
        }

        // Check daylight usage.
        UBool usesDaylight = tz->useDaylightTime();
        if (usesDaylight != kReferenceList[i].daylight) {
            if (isNonReferenceTzdataVersion || isDateBeforeReferenceYear) {
                logln("Warning: Time Zone " + itsID + " use daylight is " +
                      (usesDaylight?"TRUE":"FALSE") +
                      " but it should be " +
                      ((kReferenceList[i].daylight)?"TRUE":"FALSE"));
            } else {
                errln("FAIL: Time Zone " + itsID + " use daylight is " +
                      (usesDaylight?"TRUE":"FALSE") +
                      " but it should be " +
                      ((kReferenceList[i].daylight)?"TRUE":"FALSE"));
            }
            ok = FALSE;
        }

        // Check offset
        int32_t offsetInMinutes = tz->getRawOffset()/60000;
        if (offsetInMinutes != kReferenceList[i].offset) {
            if (isNonReferenceTzdataVersion || isDateBeforeReferenceYear) {
                logln("FAIL: Time Zone " + itsID + " raw offset is " +
                      offsetInMinutes +
                      " but it should be " + kReferenceList[i].offset);
            } else {
                errln("FAIL: Time Zone " + itsID + " raw offset is " +
                      offsetInMinutes +
                      " but it should be " + kReferenceList[i].offset);
            }
            ok = FALSE;
        }

        if (ok) {
            logln("OK: " + itsID +
                  " useDaylightTime() & getRawOffset() as expected");
        }
        delete tz;
    }


    // OK now test compat
    logln("Testing for compatibility zones");

    const char* compatibilityMap[] = {
        // This list is copied from tz.alias.  If tz.alias
        // changes, this list must be updated.  Current as of Mar 2007
        "ACT", "Australia/Darwin",
        "AET", "Australia/Sydney",
        "AGT", "America/Buenos_Aires",
        "ART", "Africa/Cairo",
        "AST", "America/Anchorage",
        "BET", "America/Sao_Paulo",
        "BST", "Asia/Dhaka", // # spelling changed in 2000h; was Asia/Dacca
        "CAT", "Africa/Harare",
        "CNT", "America/St_Johns",
        "CST", "America/Chicago",
        "CTT", "Asia/Shanghai",
        "EAT", "Africa/Addis_Ababa",
        "ECT", "Europe/Paris",
        // EET Europe/Istanbul # EET is a standard UNIX zone
        // "EST", "America/New_York", # Defined as -05:00
        // "HST", "Pacific/Honolulu", # Defined as -10:00
        "IET", "America/Indianapolis",
        "IST", "Asia/Calcutta",
        "JST", "Asia/Tokyo",
        // MET Asia/Tehran # MET is a standard UNIX zone
        "MIT", "Pacific/Apia",
        // "MST", "America/Denver", # Defined as -07:00
        "NET", "Asia/Yerevan",
        "NST", "Pacific/Auckland",
        "PLT", "Asia/Karachi",
        "PNT", "America/Phoenix",
        "PRT", "America/Puerto_Rico",
        "PST", "America/Los_Angeles",
        "SST", "Pacific/Guadalcanal",
        "UTC", "Etc/GMT",
        "VST", "Asia/Saigon",
         "","",""
    };

    for (i=0;*compatibilityMap[i];i+=2) {
        UnicodeString itsID;

        const char *zone1 = compatibilityMap[i];
        const char *zone2 = compatibilityMap[i+1];

        TimeZone *tz1 = TimeZone::createTimeZone(zone1);
        TimeZone *tz2 = TimeZone::createTimeZone(zone2);

        if (!tz1) {
            errln(UnicodeString("FAIL: Could not find short ID zone ") + zone1);
        }
        if (!tz2) {
            errln(UnicodeString("FAIL: Could not find long ID zone ") + zone2);
        }

        if (tz1 && tz2) {
            // make NAME same so comparison will only look at the rest
            tz2->setID(tz1->getID(itsID));

            if (*tz1 != *tz2) {
                errln("FAIL: " + UnicodeString(zone1) +
                      " != " + UnicodeString(zone2));
            } else {
                logln("OK: " + UnicodeString(zone1) +
                      " == " + UnicodeString(zone2));
            }
        }

        delete tz1;
        delete tz2;
    }
}


/**
 * Utility function for TestCustomParse
 */
UnicodeString& TimeZoneTest::formatOffset(int32_t offset, UnicodeString &rv) {
    rv.remove();
    UChar sign = 0x002B;
    if (offset < 0) {
        sign = 0x002D;
        offset = -offset;
    }

    int32_t s = offset % 60;
    offset /= 60;
    int32_t m = offset % 60;
    int32_t h = offset / 60;

    rv += (UChar)(sign);
    if (h >= 10) {
        rv += (UChar)(0x0030 + (h/10));
    } else {
        rv += (UChar)0x0030;
    }
    rv += (UChar)(0x0030 + (h%10));

    rv += (UChar)0x003A; /* ':' */
    if (m >= 10) {
        rv += (UChar)(0x0030 + (m/10));
    } else {
        rv += (UChar)0x0030;
    }
    rv += (UChar)(0x0030 + (m%10));

    if (s) {
        rv += (UChar)0x003A; /* ':' */
        if (s >= 10) {
            rv += (UChar)(0x0030 + (s/10));
        } else {
            rv += (UChar)0x0030;
        }
        rv += (UChar)(0x0030 + (s%10));
    }
    return rv;
}

/**
 * Utility function for TestCustomParse, generating time zone ID
 * string for the give offset.
 */
UnicodeString& TimeZoneTest::formatTZID(int32_t offset, UnicodeString &rv) {
    rv.remove();
    UChar sign = 0x002B;
    if (offset < 0) {
        sign = 0x002D;
        offset = -offset;
    }

    int32_t s = offset % 60;
    offset /= 60;
    int32_t m = offset % 60;
    int32_t h = offset / 60;

    rv += "GMT";
    rv += (UChar)(sign);
    if (h >= 10) {
        rv += (UChar)(0x0030 + (h/10));
    } else {
        rv += (UChar)0x0030;
    }
    rv += (UChar)(0x0030 + (h%10));
    rv += (UChar)0x003A;
    if (m >= 10) {
        rv += (UChar)(0x0030 + (m/10));
    } else {
        rv += (UChar)0x0030;
    }
    rv += (UChar)(0x0030 + (m%10));

    if (s) {
        rv += (UChar)0x003A;
        if (s >= 10) {
            rv += (UChar)(0x0030 + (s/10));
        } else {
            rv += (UChar)0x0030;
        }
        rv += (UChar)(0x0030 + (s%10));
    }
    return rv;
}

/**
 * As part of the VM fix (see CCC approved RFE 4028006, bug
 * 4044013), TimeZone.getTimeZone() has been modified to recognize
 * generic IDs of the form GMT[+-]hh:mm, GMT[+-]hhmm, and
 * GMT[+-]hh.  Test this behavior here.
 *
 * @bug 4044013
 */
void TimeZoneTest::TestCustomParse()
{
    int32_t i;
    const int32_t kUnparseable = 604800; // the number of seconds in a week. More than any offset should be.

    struct
    {
        const char *customId;
        int32_t expectedOffset;
    }
    kData[] =
    {
        // ID        Expected offset in seconds
        {"GMT",       kUnparseable},   //Isn't custom. [returns normal GMT]
        {"GMT-YOUR.AD.HERE", kUnparseable},
        {"GMT0",      kUnparseable},
        {"GMT+0",     (0)},
        {"GMT+1",     (1*60*60)},
        {"GMT-0030",  (-30*60)},
        {"GMT+15:99", kUnparseable},
        {"GMT+",      kUnparseable},
        {"GMT-",      kUnparseable},
        {"GMT+0:",    kUnparseable},
        {"GMT-:",     kUnparseable},
        {"GMT-YOUR.AD.HERE",    kUnparseable},
        {"GMT+0010",  (10*60)}, // Interpret this as 00:10
        {"GMT-10",    (-10*60*60)},
        {"GMT+30",    kUnparseable},
        {"GMT-3:30",  (-(3*60+30)*60)},
        {"GMT-230",   (-(2*60+30)*60)},
        {"GMT+05:13:05",    ((5*60+13)*60+5)},
        {"GMT-71023",       (-((7*60+10)*60+23))},
        {"GMT+01:23:45:67", kUnparseable},
        {"GMT+01:234",      kUnparseable},
        {"GMT-2:31:123",    kUnparseable},
        {"GMT+3:75",        kUnparseable},
        {"GMT-01010101",    kUnparseable},
        {0,           0}
    };

    for (i=0; kData[i].customId != 0; i++) {
        UnicodeString id(kData[i].customId);
        int32_t exp = kData[i].expectedOffset;
        TimeZone *zone = TimeZone::createTimeZone(id);
        UnicodeString   itsID, temp;

        if (dynamic_cast<OlsonTimeZone *>(zone) != NULL) {
            logln(id + " -> Olson time zone");
        } else {
            zone->getID(itsID);
            int32_t ioffset = zone->getRawOffset()/1000;
            UnicodeString offset, expectedID;
            formatOffset(ioffset, offset);
            formatTZID(ioffset, expectedID);
            logln(id + " -> " + itsID + " " + offset);
            if (exp == kUnparseable && itsID != "GMT") {
                errln("Expected parse failure for " + id +
                      ", got offset of " + offset +
                      ", id " + itsID);
            }
            // JDK 1.3 creates custom zones with the ID "Custom"
            // JDK 1.4 creates custom zones with IDs of the form "GMT+02:00"
            // ICU creates custom zones with IDs of the form "GMT+02:00"
            else if (exp != kUnparseable && (ioffset != exp || itsID != expectedID)) {
                dataerrln("Expected offset of " + formatOffset(exp, temp) +
                      ", id " + expectedID +
                      ", for " + id +
                      ", got offset of " + offset +
                      ", id " + itsID);
            }
        }
        delete zone;
    }
}

void
TimeZoneTest::TestAliasedNames()
{
    struct {
        const char *from;
        const char *to;
    } kData[] = {
        /* Generated by org.unicode.cldr.tool.CountItems */

        /* zoneID, canonical zoneID */
        {"Africa/Timbuktu", "Africa/Bamako"},
        {"America/Argentina/Buenos_Aires", "America/Buenos_Aires"},
        {"America/Argentina/Catamarca", "America/Catamarca"},
        {"America/Argentina/ComodRivadavia", "America/Catamarca"},
        {"America/Argentina/Cordoba", "America/Cordoba"},
        {"America/Argentina/Jujuy", "America/Jujuy"},
        {"America/Argentina/Mendoza", "America/Mendoza"},
        {"America/Atka", "America/Adak"},
        {"America/Ensenada", "America/Tijuana"},
        {"America/Fort_Wayne", "America/Indiana/Indianapolis"},
        {"America/Indianapolis", "America/Indiana/Indianapolis"},
        {"America/Knox_IN", "America/Indiana/Knox"},
        {"America/Louisville", "America/Kentucky/Louisville"},
        {"America/Porto_Acre", "America/Rio_Branco"},
        {"America/Rosario", "America/Cordoba"},
        {"America/Virgin", "America/St_Thomas"},
        {"Asia/Ashkhabad", "Asia/Ashgabat"},
        {"Asia/Chungking", "Asia/Chongqing"},
        {"Asia/Dacca", "Asia/Dhaka"},
        {"Asia/Istanbul", "Europe/Istanbul"},
        {"Asia/Macao", "Asia/Macau"},
        {"Asia/Tel_Aviv", "Asia/Jerusalem"},
        {"Asia/Thimbu", "Asia/Thimphu"},
        {"Asia/Ujung_Pandang", "Asia/Makassar"},
        {"Asia/Ulan_Bator", "Asia/Ulaanbaatar"},
        {"Australia/ACT", "Australia/Sydney"},
        {"Australia/Canberra", "Australia/Sydney"},
        {"Australia/LHI", "Australia/Lord_Howe"},
        {"Australia/NSW", "Australia/Sydney"},
        {"Australia/North", "Australia/Darwin"},
        {"Australia/Queensland", "Australia/Brisbane"},
        {"Australia/South", "Australia/Adelaide"},
        {"Australia/Tasmania", "Australia/Hobart"},
        {"Australia/Victoria", "Australia/Melbourne"},
        {"Australia/West", "Australia/Perth"},
        {"Australia/Yancowinna", "Australia/Broken_Hill"},
        {"Brazil/Acre", "America/Rio_Branco"},
        {"Brazil/DeNoronha", "America/Noronha"},
        {"Brazil/East", "America/Sao_Paulo"},
        {"Brazil/West", "America/Manaus"},
        {"Canada/Atlantic", "America/Halifax"},
        {"Canada/Central", "America/Winnipeg"},
        {"Canada/East-Saskatchewan", "America/Regina"},
        {"Canada/Eastern", "America/Toronto"},
        {"Canada/Mountain", "America/Edmonton"},
        {"Canada/Newfoundland", "America/St_Johns"},
        {"Canada/Pacific", "America/Vancouver"},
        {"Canada/Saskatchewan", "America/Regina"},
        {"Canada/Yukon", "America/Whitehorse"},
        {"Chile/Continental", "America/Santiago"},
        {"Chile/EasterIsland", "Pacific/Easter"},
        {"Cuba", "America/Havana"},
        {"Egypt", "Africa/Cairo"},
        {"Eire", "Europe/Dublin"},
        {"Etc/GMT+0", "Etc/GMT"},
        {"Etc/GMT-0", "Etc/GMT"},
        {"Etc/GMT0", "Etc/GMT"},
        {"Etc/Greenwich", "Etc/GMT"},
        {"Etc/UCT", "Etc/GMT"},
        {"Etc/UTC", "Etc/GMT"},
        {"Etc/Universal", "Etc/GMT"},
        {"Etc/Zulu", "Etc/GMT"},
        {"Europe/Belfast", "Europe/London"},
        {"Europe/Nicosia", "Asia/Nicosia"},
        {"Europe/Tiraspol", "Europe/Chisinau"},
        {"GB", "Europe/London"},
        {"GB-Eire", "Europe/London"},
        {"GMT", "Etc/GMT"},
        {"GMT+0", "Etc/GMT"},
        {"GMT-0", "Etc/GMT"},
        {"GMT0", "Etc/GMT"},
        {"Greenwich", "Etc/GMT"},
        {"Hongkong", "Asia/Hong_Kong"},
        {"Iceland", "Atlantic/Reykjavik"},
        {"Iran", "Asia/Tehran"},
        {"Israel", "Asia/Jerusalem"},
        {"Jamaica", "America/Jamaica"},
        {"Japan", "Asia/Tokyo"},
        {"Kwajalein", "Pacific/Kwajalein"},
        {"Libya", "Africa/Tripoli"},
        {"Mexico/BajaNorte", "America/Tijuana"},
        {"Mexico/BajaSur", "America/Mazatlan"},
        {"Mexico/General", "America/Mexico_City"},
        {"NZ", "Pacific/Auckland"},
        {"NZ-CHAT", "Pacific/Chatham"},
        {"Navajo", "America/Shiprock"},
        {"PRC", "Asia/Shanghai"},
        {"Pacific/Samoa", "Pacific/Pago_Pago"},
        {"Pacific/Yap", "Pacific/Truk"},
        {"Poland", "Europe/Warsaw"},
        {"Portugal", "Europe/Lisbon"},
        {"ROC", "Asia/Taipei"},
        {"ROK", "Asia/Seoul"},
        {"Singapore", "Asia/Singapore"},
        {"Turkey", "Europe/Istanbul"},
        {"UCT", "Etc/GMT"},
        {"US/Alaska", "America/Anchorage"},
        {"US/Aleutian", "America/Adak"},
        {"US/Arizona", "America/Phoenix"},
        {"US/Central", "America/Chicago"},
        {"US/East-Indiana", "America/Indiana/Indianapolis"},
        {"US/Eastern", "America/New_York"},
        {"US/Hawaii", "Pacific/Honolulu"},
        {"US/Indiana-Starke", "America/Indiana/Knox"},
        {"US/Michigan", "America/Detroit"},
        {"US/Mountain", "America/Denver"},
        {"US/Pacific", "America/Los_Angeles"},
        {"US/Pacific-New", "America/Los_Angeles"},
        {"US/Samoa", "Pacific/Pago_Pago"},
        {"UTC", "Etc/GMT"},
        {"Universal", "Etc/GMT"},
        {"W-SU", "Europe/Moscow"},
        {"Zulu", "Etc/GMT"},
        /* Total: 113 */

    };

    TimeZone::EDisplayType styles[] = { TimeZone::SHORT, TimeZone::LONG };
    UBool useDst[] = { FALSE, TRUE };
    int32_t noLoc = uloc_countAvailable();

    int32_t i, j, k, loc;
    UnicodeString fromName, toName;
    TimeZone *from = NULL, *to = NULL;
    for(i = 0; i < (int32_t)(sizeof(kData)/sizeof(kData[0])); i++) {
        from = TimeZone::createTimeZone(kData[i].from);
        to = TimeZone::createTimeZone(kData[i].to);
        if(!from->hasSameRules(*to)) {
            errln("different at %i\n", i);
        }
        if(!quick) {
            for(loc = 0; loc < noLoc; loc++) {
                const char* locale = uloc_getAvailable(loc); 
                for(j = 0; j < (int32_t)(sizeof(styles)/sizeof(styles[0])); j++) {
                    for(k = 0; k < (int32_t)(sizeof(useDst)/sizeof(useDst[0])); k++) {
                        fromName.remove();
                        toName.remove();
                        from->getDisplayName(useDst[k], styles[j],locale, fromName);
                        to->getDisplayName(useDst[k], styles[j], locale, toName);
                        if(fromName.compare(toName) != 0) {
                            errln("Fail: Expected "+toName+" but got " + prettify(fromName) 
                                + " for locale: " + locale + " index: "+ loc 
                                + " to id "+ kData[i].to
                                + " from id " + kData[i].from);
                        }
                    }
                }
            }
        } else {
            fromName.remove();
            toName.remove();
            from->getDisplayName(fromName);
            to->getDisplayName(toName);
            if(fromName.compare(toName) != 0) {
                errln("Fail: Expected "+toName+" but got " + fromName);
            }
        }
        delete from;
        delete to;
    }
}

/**
 * Test the basic functionality of the getDisplayName() API.
 *
 * @bug 4112869
 * @bug 4028006
 *
 * See also API change request A41.
 *
 * 4/21/98 - make smarter, so the test works if the ext resources
 * are present or not.
 */
void
TimeZoneTest::TestDisplayName()
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t i;
    TimeZone *zone = TimeZone::createTimeZone("PST");
    UnicodeString name;
    zone->getDisplayName(Locale::getEnglish(), name);
    logln("PST->" + name);
    if (name.compare("Pacific Standard Time") != 0)
        dataerrln("Fail: Expected \"Pacific Standard Time\" but got " + name);

    //*****************************************************************
    // THE FOLLOWING LINES MUST BE UPDATED IF THE LOCALE DATA CHANGES
    // THE FOLLOWING LINES MUST BE UPDATED IF THE LOCALE DATA CHANGES
    // THE FOLLOWING LINES MUST BE UPDATED IF THE LOCALE DATA CHANGES
    //*****************************************************************
    struct
    {
        UBool useDst;
        TimeZone::EDisplayType style;
        const char *expect;
    } kData[] = {
        {FALSE, TimeZone::SHORT, "PST"},
        {TRUE,  TimeZone::SHORT, "PDT"},
        {FALSE, TimeZone::LONG,  "Pacific Standard Time"},
        {TRUE,  TimeZone::LONG,  "Pacific Daylight Time"},

        {FALSE, TimeZone::SHORT_GENERIC, "PT"},
        {TRUE,  TimeZone::SHORT_GENERIC, "PT"},
        {FALSE, TimeZone::LONG_GENERIC,  "Pacific Time"},
        {TRUE,  TimeZone::LONG_GENERIC,  "Pacific Time"},

        {FALSE, TimeZone::SHORT_GMT, "-0800"},
        {TRUE,  TimeZone::SHORT_GMT, "-0700"},
        {FALSE, TimeZone::LONG_GMT,  "GMT-08:00"},
        {TRUE,  TimeZone::LONG_GMT,  "GMT-07:00"},

        {FALSE, TimeZone::SHORT_COMMONLY_USED, "PST"},
        {TRUE,  TimeZone::SHORT_COMMONLY_USED, "PDT"},
        {FALSE, TimeZone::GENERIC_LOCATION,  "United States (Los Angeles)"},
        {TRUE,  TimeZone::GENERIC_LOCATION,  "United States (Los Angeles)"},

        {FALSE, TimeZone::LONG, ""}
    };

    for (i=0; kData[i].expect[0] != '\0'; i++)
    {
        name.remove();
        name = zone->getDisplayName(kData[i].useDst,
                                   kData[i].style,
                                   Locale::getEnglish(), name);
        if (name.compare(kData[i].expect) != 0)
            dataerrln("Fail: Expected " + UnicodeString(kData[i].expect) + "; got " + name);
        logln("PST [with options]->" + name);
    }
    for (i=0; kData[i].expect[0] != '\0'; i++)
    {
        name.remove();
        name = zone->getDisplayName(kData[i].useDst,
                                   kData[i].style, name);
        if (name.compare(kData[i].expect) != 0)
            dataerrln("Fail: Expected " + UnicodeString(kData[i].expect) + "; got " + name);
        logln("PST [with options]->" + name);
    }


    // Make sure that we don't display the DST name by constructing a fake
    // PST zone that has DST all year long.
    SimpleTimeZone *zone2 = new SimpleTimeZone(0, "PST");

    zone2->setStartRule(UCAL_JANUARY, 1, 0, 0, status);
    zone2->setEndRule(UCAL_DECEMBER, 31, 0, 0, status);

    UnicodeString inDaylight;
    if (zone2->inDaylightTime(UDate(0), status)) {
        inDaylight = UnicodeString("TRUE");
    } else {
        inDaylight = UnicodeString("FALSE");
    }
    logln(UnicodeString("Modified PST inDaylightTime->") + inDaylight );
    if(U_FAILURE(status))
    {
        dataerrln("Some sort of error..." + UnicodeString(u_errorName(status))); // REVISIT
    }
    name.remove();
    name = zone2->getDisplayName(Locale::getEnglish(),name);
    logln("Modified PST->" + name);
    if (name.compare("Pacific Standard Time") != 0)
        dataerrln("Fail: Expected \"Pacific Standard Time\"");

    // Make sure we get the default display format for Locales
    // with no display name data.
    Locale mt_MT("mt_MT");
    name.remove();
    name = zone->getDisplayName(mt_MT,name);
    //*****************************************************************
    // THE FOLLOWING LINE MUST BE UPDATED IF THE LOCALE DATA CHANGES
    // THE FOLLOWING LINE MUST BE UPDATED IF THE LOCALE DATA CHANGES
    // THE FOLLOWING LINE MUST BE UPDATED IF THE LOCALE DATA CHANGES
    //*****************************************************************
    logln("PST(mt_MT)->" + name);

    // *** REVISIT SRL how in the world do I check this? looks java specific.
    // Now be smart -- check to see if zh resource is even present.
    // If not, we expect the en fallback behavior.
    ResourceBundle enRB(NULL,
                            Locale::getEnglish(), status);
    if(U_FAILURE(status))
        dataerrln("Couldn't get ResourceBundle for en - %s", u_errorName(status));

    ResourceBundle mtRB(NULL,
                         mt_MT, status);
    //if(U_FAILURE(status))
    //    errln("Couldn't get ResourceBundle for mt_MT");

    UBool noZH = U_FAILURE(status);

    if (noZH) {
        logln("Warning: Not testing the mt_MT behavior because resource is absent");
        if (name != "Pacific Standard Time")
            dataerrln("Fail: Expected Pacific Standard Time");
    }


    if      (name.compare("GMT-08:00") &&
             name.compare("GMT-8:00") &&
             name.compare("GMT-0800") &&
             name.compare("GMT-800")) {
      dataerrln(UnicodeString("Fail: Expected GMT-08:00 or something similar for PST in mt_MT but got ") + name );
        dataerrln("************************************************************");
        dataerrln("THE ABOVE FAILURE MAY JUST MEAN THE LOCALE DATA HAS CHANGED");
        dataerrln("************************************************************");
    }

    // Now try a non-existent zone
    delete zone2;
    zone2 = new SimpleTimeZone(90*60*1000, "xyzzy");
    name.remove();
    name = zone2->getDisplayName(Locale::getEnglish(),name);
    logln("GMT+90min->" + name);
    if (name.compare("GMT+01:30") &&
        name.compare("GMT+1:30") &&
        name.compare("GMT+0130") &&
        name.compare("GMT+130"))
        dataerrln("Fail: Expected GMT+01:30 or something similar");
    name.truncate(0);
    zone2->getDisplayName(name);
    logln("GMT+90min->" + name);
    if (name.compare("GMT+01:30") &&
        name.compare("GMT+1:30") &&
        name.compare("GMT+0130") &&
        name.compare("GMT+130"))
        dataerrln("Fail: Expected GMT+01:30 or something similar");
    // clean up
    delete zone;
    delete zone2;
}

/**
 * @bug 4107276
 */
void
TimeZoneTest::TestDSTSavings()
{
    UErrorCode status = U_ZERO_ERROR;
    // It might be better to find a way to integrate this test into the main TimeZone
    // tests above, but I don't have time to figure out how to do this (or if it's
    // even really a good idea).  Let's consider that a future.  --rtg 1/27/98
    SimpleTimeZone *tz = new SimpleTimeZone(-5 * U_MILLIS_PER_HOUR, "dstSavingsTest",
                                           UCAL_MARCH, 1, 0, 0, UCAL_SEPTEMBER, 1, 0, 0,
                                           (int32_t)(0.5 * U_MILLIS_PER_HOUR), status);
    if(U_FAILURE(status))
        errln("couldn't create TimeZone");

    if (tz->getRawOffset() != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("Got back a raw offset of ") + (tz->getRawOffset() / U_MILLIS_PER_HOUR) +
              " hours instead of -5 hours.");
    if (!tz->useDaylightTime())
        errln("Test time zone should use DST but claims it doesn't.");
    if (tz->getDSTSavings() != 0.5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("Set DST offset to 0.5 hour, but got back ") + (tz->getDSTSavings() /
                                                             U_MILLIS_PER_HOUR) + " hours instead.");

    int32_t offset = tz->getOffset(GregorianCalendar::AD, 1998, UCAL_JANUARY, 1,
                              UCAL_THURSDAY, 10 * U_MILLIS_PER_HOUR,status);
    if (offset != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10 AM, 1/1/98 should have been -5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz->getOffset(GregorianCalendar::AD, 1998, UCAL_JUNE, 1, UCAL_MONDAY,
                          10 * U_MILLIS_PER_HOUR,status);
    if (offset != -4.5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10 AM, 6/1/98 should have been -4.5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    tz->setDSTSavings(U_MILLIS_PER_HOUR, status);
    offset = tz->getOffset(GregorianCalendar::AD, 1998, UCAL_JANUARY, 1,
                          UCAL_THURSDAY, 10 * U_MILLIS_PER_HOUR,status);
    if (offset != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10 AM, 1/1/98 should have been -5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz->getOffset(GregorianCalendar::AD, 1998, UCAL_JUNE, 1, UCAL_MONDAY,
                          10 * U_MILLIS_PER_HOUR,status);
    if (offset != -4 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10 AM, 6/1/98 (with a 1-hour DST offset) should have been -4 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    delete tz;
}

/**
 * @bug 4107570
 */
void
TimeZoneTest::TestAlternateRules()
{
    // Like TestDSTSavings, this test should probably be integrated somehow with the main
    // test at the top of this class, but I didn't have time to figure out how to do that.
    //                      --rtg 1/28/98

    SimpleTimeZone tz(-5 * U_MILLIS_PER_HOUR, "alternateRuleTest");

    // test the day-of-month API
    UErrorCode status = U_ZERO_ERROR;
    tz.setStartRule(UCAL_MARCH, 10, 12 * U_MILLIS_PER_HOUR, status);
    if(U_FAILURE(status))
        errln("tz.setStartRule failed");
    tz.setEndRule(UCAL_OCTOBER, 20, 12 * U_MILLIS_PER_HOUR, status);
    if(U_FAILURE(status))
        errln("tz.setStartRule failed");

    int32_t offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_MARCH, 5,
                              UCAL_THURSDAY, 10 * U_MILLIS_PER_HOUR,status);
    if (offset != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 3/5/98 should have been -5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_MARCH, 15,
                          UCAL_SUNDAY, 10 * millisPerHour,status);
    if (offset != -4 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 3/15/98 should have been -4 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_OCTOBER, 15,
                          UCAL_THURSDAY, 10 * millisPerHour,status);
    if (offset != -4 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 10/15/98 should have been -4 hours, but we got ")              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_OCTOBER, 25,
                          UCAL_SUNDAY, 10 * millisPerHour,status);
    if (offset != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 10/25/98 should have been -5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    // test the day-of-week-after-day-in-month API
    tz.setStartRule(UCAL_MARCH, 10, UCAL_FRIDAY, 12 * millisPerHour, TRUE, status);
    if(U_FAILURE(status))
        errln("tz.setStartRule failed");
    tz.setEndRule(UCAL_OCTOBER, 20, UCAL_FRIDAY, 12 * millisPerHour, FALSE, status);
    if(U_FAILURE(status))
        errln("tz.setStartRule failed");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_MARCH, 11,
                          UCAL_WEDNESDAY, 10 * millisPerHour,status);
    if (offset != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 3/11/98 should have been -5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_MARCH, 14,
                          UCAL_SATURDAY, 10 * millisPerHour,status);
    if (offset != -4 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 3/14/98 should have been -4 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_OCTOBER, 15,
                          UCAL_THURSDAY, 10 * millisPerHour,status);
    if (offset != -4 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 10/15/98 should have been -4 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");

    offset = tz.getOffset(GregorianCalendar::AD, 1998, UCAL_OCTOBER, 17,
                          UCAL_SATURDAY, 10 * millisPerHour,status);
    if (offset != -5 * U_MILLIS_PER_HOUR)
        errln(UnicodeString("The offset for 10AM, 10/17/98 should have been -5 hours, but we got ")
              + (offset / U_MILLIS_PER_HOUR) + " hours.");
}

void TimeZoneTest::TestFractionalDST() {
    const char* tzName = "Australia/Lord_Howe"; // 30 min offset
    TimeZone* tz_icu = TimeZone::createTimeZone(tzName);
	int dst_icu = tz_icu->getDSTSavings();
    UnicodeString id;
    int32_t expected = 1800000;
	if (expected != dst_icu) {
	    dataerrln(UnicodeString("java reports dst savings of ") + expected +
	        " but icu reports " + dst_icu + 
	        " for tz " + tz_icu->getID(id));
	} else {
	    logln(UnicodeString("both java and icu report dst savings of ") + expected + " for tz " + tz_icu->getID(id));
	}
    delete tz_icu;
}

/**
 * Test country code support.  Jitterbug 776.
 */
void TimeZoneTest::TestCountries() {
    // Make sure America/Los_Angeles is in the "US" group, and
    // Asia/Tokyo isn't.  Vice versa for the "JP" group.
    UErrorCode ec = U_ZERO_ERROR;
    int32_t n;
    StringEnumeration* s = TimeZone::createEnumeration("US");
    n = s->count(ec);
    UBool la = FALSE, tokyo = FALSE;
    UnicodeString laZone("America/Los_Angeles", "");
    UnicodeString tokyoZone("Asia/Tokyo", "");
    int32_t i;

    if (s == NULL || n <= 0) {
        dataerrln("FAIL: TimeZone::createEnumeration() returned nothing");
        return;
    }
    for (i=0; i<n; ++i) {
        const UnicodeString* id = s->snext(ec);
        if (*id == (laZone)) {
            la = TRUE;
        }
        if (*id == (tokyoZone)) {
            tokyo = TRUE;
        }
    }
    if (!la || tokyo) {
        errln("FAIL: " + laZone + " in US = " + la);
        errln("FAIL: " + tokyoZone + " in US = " + tokyo);
    }
    delete s;
    
    s = TimeZone::createEnumeration("JP");
    n = s->count(ec);
    la = FALSE; tokyo = FALSE;
    
    for (i=0; i<n; ++i) {
        const UnicodeString* id = s->snext(ec);
        if (*id == (laZone)) {
            la = TRUE;
        }
        if (*id == (tokyoZone)) {
            tokyo = TRUE;
        }
    }
    if (la || !tokyo) {
        errln("FAIL: " + laZone + " in JP = " + la);
        errln("FAIL: " + tokyoZone + " in JP = " + tokyo);
    }
    StringEnumeration* s1 = TimeZone::createEnumeration("US");
    StringEnumeration* s2 = TimeZone::createEnumeration("US");
    for(i=0;i<n;++i){
        const UnicodeString* id1 = s1->snext(ec);
        if(id1==NULL || U_FAILURE(ec)){
            errln("Failed to fetch next from TimeZone enumeration. Length returned : %i Current Index: %i", n,i);
        }
        TimeZone* tz1 = TimeZone::createTimeZone(*id1);
        for(int j=0; j<n;++j){
            const UnicodeString* id2 = s2->snext(ec);
            if(id2==NULL || U_FAILURE(ec)){
                errln("Failed to fetch next from TimeZone enumeration. Length returned : %i Current Index: %i", n,i);
            }
            TimeZone* tz2 = TimeZone::createTimeZone(*id2);
            if(tz1->hasSameRules(*tz2)){
                logln("ID1 : " + *id1+" == ID2 : " +*id2);
            }
            delete tz2;
        }
        delete tz1;
    }
    delete s1;
    delete s2;
    delete s;
}

void TimeZoneTest::TestHistorical() {
    const int32_t H = U_MILLIS_PER_HOUR;
    struct {
        const char* id;
        int32_t time; // epoch seconds
        int32_t offset; // total offset (millis)
    } DATA[] = {
        // Add transition points (before/after) as desired to test historical
        // behavior.
        {"America/Los_Angeles", 638963999, -8*H}, // Sun Apr 01 01:59:59 GMT-08:00 1990
        {"America/Los_Angeles", 638964000, -7*H}, // Sun Apr 01 03:00:00 GMT-07:00 1990
        {"America/Los_Angeles", 657104399, -7*H}, // Sun Oct 28 01:59:59 GMT-07:00 1990
        {"America/Los_Angeles", 657104400, -8*H}, // Sun Oct 28 01:00:00 GMT-08:00 1990
        {"America/Goose_Bay", -116445601, -4*H}, // Sun Apr 24 01:59:59 GMT-04:00 1966
        {"America/Goose_Bay", -116445600, -3*H}, // Sun Apr 24 03:00:00 GMT-03:00 1966
        {"America/Goose_Bay", -100119601, -3*H}, // Sun Oct 30 01:59:59 GMT-03:00 1966
        {"America/Goose_Bay", -100119600, -4*H}, // Sun Oct 30 01:00:00 GMT-04:00 1966
        {"America/Goose_Bay", -84391201, -4*H}, // Sun Apr 30 01:59:59 GMT-04:00 1967
        {"America/Goose_Bay", -84391200, -3*H}, // Sun Apr 30 03:00:00 GMT-03:00 1967
        {"America/Goose_Bay", -68670001, -3*H}, // Sun Oct 29 01:59:59 GMT-03:00 1967
        {"America/Goose_Bay", -68670000, -4*H}, // Sun Oct 29 01:00:00 GMT-04:00 1967
        {0, 0, 0}
    };
    
    for (int32_t i=0; DATA[i].id!=0; ++i) {
        const char* id = DATA[i].id;
        TimeZone *tz = TimeZone::createTimeZone(id);
        UnicodeString s;
        if (tz == 0) {
            errln("FAIL: Cannot create %s", id);
        } else if (tz->getID(s) != UnicodeString(id)) {
            dataerrln((UnicodeString)"FAIL: createTimeZone(" + id + ") => " + s);
        } else {
            UErrorCode ec = U_ZERO_ERROR;
            int32_t raw, dst;
            UDate when = (double) DATA[i].time * U_MILLIS_PER_SECOND;
            tz->getOffset(when, FALSE, raw, dst, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: getOffset");
            } else if ((raw+dst) != DATA[i].offset) {
                errln((UnicodeString)"FAIL: " + DATA[i].id + ".getOffset(" +
                      //when + " = " +
                      dateToString(when) + ") => " +
                      raw + ", " + dst);
            } else {
                logln((UnicodeString)"Ok: " + DATA[i].id + ".getOffset(" +
                      //when + " = " +
                      dateToString(when) + ") => " +
                      raw + ", " + dst);
            }
        }
        delete tz;
    }
}

void TimeZoneTest::TestEquivalentIDs() {
    int32_t n = TimeZone::countEquivalentIDs("PST");
    if (n < 2) {
        dataerrln((UnicodeString)"FAIL: countEquivalentIDs(PST) = " + n);
    } else {
        UBool sawLA = FALSE;
        for (int32_t i=0; i<n; ++i) {
            UnicodeString id = TimeZone::getEquivalentID("PST", i);
            logln((UnicodeString)"" + i + " : " + id);
            if (id == UnicodeString("America/Los_Angeles")) {
                sawLA = TRUE;
            }
        }
        if (!sawLA) {
            errln("FAIL: America/Los_Angeles should be in the list");
        }
    }
}

// Test that a transition at the end of February is handled correctly.
void TimeZoneTest::TestFebruary() {
    UErrorCode status = U_ZERO_ERROR;

    // Time zone with daylight savings time from the first Sunday in November
    // to the last Sunday in February.
    // Similar to the new rule for Brazil (Sao Paulo) in tzdata2006n.
    //
    // Note: In tzdata2007h, the rule had changed, so no actual zones uses
    // lastSun in Feb anymore.
    SimpleTimeZone tz1(-3 * U_MILLIS_PER_HOUR,          // raw offset: 3h before (west of) GMT
                       UNICODE_STRING("nov-feb", 7),
                       UCAL_NOVEMBER, 1, UCAL_SUNDAY,   // start: November, first, Sunday
                       0,                               //        midnight wall time
                       UCAL_FEBRUARY, -1, UCAL_SUNDAY,  // end:   February, last, Sunday
                       0,                               //        midnight wall time
                       status);
    if (U_FAILURE(status)) {
        errln("Unable to create the SimpleTimeZone(nov-feb): %s", u_errorName(status));
        return;
    }

    // Now hardcode the same rules as for Brazil in tzdata 2006n, so that
    // we cover the intended code even when in the future zoneinfo hardcodes
    // these transition dates.
    SimpleTimeZone tz2(-3 * U_MILLIS_PER_HOUR,          // raw offset: 3h before (west of) GMT
                       UNICODE_STRING("nov-feb2", 8),
                       UCAL_NOVEMBER, 1, -UCAL_SUNDAY,  // start: November, 1 or after, Sunday
                       0,                               //        midnight wall time
                       UCAL_FEBRUARY, -29, -UCAL_SUNDAY,// end:   February, 29 or before, Sunday
                       0,                               //        midnight wall time
                       status);
    if (U_FAILURE(status)) {
        errln("Unable to create the SimpleTimeZone(nov-feb2): %s", u_errorName(status));
        return;
    }

    // Gregorian calendar with the UTC time zone for getting sample test date/times.
    GregorianCalendar gc(*TimeZone::getGMT(), status);
    if (U_FAILURE(status)) {
        dataerrln("Unable to create the UTC calendar: %s", u_errorName(status));
        return;
    }

    struct {
        // UTC time.
        int32_t year, month, day, hour, minute, second;
        // Expected time zone offset in hours after GMT (negative=before GMT).
        int32_t offsetHours;
    } data[] = {
        { 2006, UCAL_NOVEMBER,  5, 02, 59, 59, -3 },
        { 2006, UCAL_NOVEMBER,  5, 03, 00, 00, -2 },
        { 2007, UCAL_FEBRUARY, 25, 01, 59, 59, -2 },
        { 2007, UCAL_FEBRUARY, 25, 02, 00, 00, -3 },

        { 2007, UCAL_NOVEMBER,  4, 02, 59, 59, -3 },
        { 2007, UCAL_NOVEMBER,  4, 03, 00, 00, -2 },
        { 2008, UCAL_FEBRUARY, 24, 01, 59, 59, -2 },
        { 2008, UCAL_FEBRUARY, 24, 02, 00, 00, -3 },

        { 2008, UCAL_NOVEMBER,  2, 02, 59, 59, -3 },
        { 2008, UCAL_NOVEMBER,  2, 03, 00, 00, -2 },
        { 2009, UCAL_FEBRUARY, 22, 01, 59, 59, -2 },
        { 2009, UCAL_FEBRUARY, 22, 02, 00, 00, -3 },

        { 2009, UCAL_NOVEMBER,  1, 02, 59, 59, -3 },
        { 2009, UCAL_NOVEMBER,  1, 03, 00, 00, -2 },
        { 2010, UCAL_FEBRUARY, 28, 01, 59, 59, -2 },
        { 2010, UCAL_FEBRUARY, 28, 02, 00, 00, -3 }
    };

    TimeZone *timezones[] = { &tz1, &tz2 };

    TimeZone *tz;
    UDate dt;
    int32_t t, i, raw, dst;
    for (t = 0; t < LENGTHOF(timezones); ++t) {
        tz = timezones[t];
        for (i = 0; i < LENGTHOF(data); ++i) {
            gc.set(data[i].year, data[i].month, data[i].day,
                   data[i].hour, data[i].minute, data[i].second);
            dt = gc.getTime(status);
            if (U_FAILURE(status)) {
                errln("test case %d.%d: bad date/time %04d-%02d-%02d %02d:%02d:%02d",
                      t, i,
                      data[i].year, data[i].month + 1, data[i].day,
                      data[i].hour, data[i].minute, data[i].second);
                status = U_ZERO_ERROR;
                continue;
            }
            tz->getOffset(dt, FALSE, raw, dst, status);
            if (U_FAILURE(status)) {
                errln("test case %d.%d: tz.getOffset(%04d-%02d-%02d %02d:%02d:%02d) fails: %s",
                      t, i,
                      data[i].year, data[i].month + 1, data[i].day,
                      data[i].hour, data[i].minute, data[i].second,
                      u_errorName(status));
                status = U_ZERO_ERROR;
            } else if ((raw + dst) != data[i].offsetHours * U_MILLIS_PER_HOUR) {
                errln("test case %d.%d: tz.getOffset(%04d-%02d-%02d %02d:%02d:%02d) returns %d+%d != %d",
                      t, i,
                      data[i].year, data[i].month + 1, data[i].day,
                      data[i].hour, data[i].minute, data[i].second,
                      raw, dst, data[i].offsetHours * U_MILLIS_PER_HOUR);
            }
        }
    }
}
void TimeZoneTest::TestCanonicalID() {

    // Some canonical IDs in CLDR are defined as "Link"
    // in Olson tzdata.
    static const struct {
        const char *alias;
        const char *zone;
    } excluded1[] = {
        {"America/Shiprock", "America/Denver"}, // America/Shiprock is defined as a Link to America/Denver in tzdata
        {"America/Marigot", "America/Guadeloupe"}, 
        {"America/St_Barthelemy", "America/Guadeloupe"},
        {"Antarctica/South_Pole", "Antarctica/McMurdo"},
        {"Atlantic/Jan_Mayen", "Europe/Oslo"},
        {"Arctic/Longyearbyen", "Europe/Oslo"},
        {"Europe/Guernsey", "Europe/London"},
        {"Europe/Isle_of_Man", "Europe/London"},
        {"Europe/Jersey", "Europe/London"},
        {"Europe/Ljubljana", "Europe/Belgrade"},
        {"Europe/Podgorica", "Europe/Belgrade"},
        {"Europe/Sarajevo", "Europe/Belgrade"},
        {"Europe/Skopje", "Europe/Belgrade"},
        {"Europe/Zagreb", "Europe/Belgrade"},
        {"Europe/Bratislava", "Europe/Prague"},
        {"Europe/Mariehamn", "Europe/Helsinki"},
        {"Europe/San_Marino", "Europe/Rome"},
        {"Europe/Vatican", "Europe/Rome"},
        {0, 0}
    };

    // Following IDs are aliases of Etc/GMT in CLDR,
    // but Olson tzdata has 3 independent definitions
    // for Etc/GMT, Etc/UTC, Etc/UCT.
    // Until we merge them into one equivalent group
    // in zoneinfo.res, we exclude them in the test
    // below.
    static const char* excluded2[] = {
        "Etc/UCT", "UCT",
        "Etc/UTC", "UTC",
        "Etc/Universal", "Universal",
        "Etc/Zulu", "Zulu", 0
    };

    // Walk through equivalency groups
    UErrorCode ec = U_ZERO_ERROR;
    int32_t s_length, i, j, k;
    StringEnumeration* s = TimeZone::createEnumeration();
    UnicodeString canonicalID, tmpCanonical;
    s_length = s->count(ec);
    for (i = 0; i < s_length;++i) {
        const UnicodeString *tzid = s->snext(ec);
        int32_t nEquiv = TimeZone::countEquivalentIDs(*tzid);
        if (nEquiv == 0) {
            continue;
        }
        UBool bFoundCanonical = FALSE;
        // Make sure getCanonicalID returns the exact same result
        // for all entries within a same equivalency group with some
        // exceptions listed in exluded1.
        // Also, one of them must be canonical id.
        for (j = 0; j < nEquiv; j++) {
            UnicodeString tmp = TimeZone::getEquivalentID(*tzid, j);
            TimeZone::getCanonicalID(tmp, tmpCanonical, ec);
            if (U_FAILURE(ec)) {
                errln((UnicodeString)"FAIL: getCanonicalID(" + tmp + ") failed.");
                ec = U_ZERO_ERROR;
                continue;
            }
            // Some exceptional cases
            for (k = 0; excluded1[k].alias != 0; k++) {
                if (tmpCanonical == excluded1[k].alias) {
                    tmpCanonical = excluded1[k].zone;
                    break;
                }
            }
            if (j == 0) {
                canonicalID = tmpCanonical;
            } else if (canonicalID != tmpCanonical) {
                errln("FAIL: getCanonicalID(" + tmp + ") returned " + tmpCanonical + " expected:" + canonicalID);
            }

            if (canonicalID == tmp) {
                bFoundCanonical = TRUE;
            }
        }
        // At least one ID in an equvalency group must match the
        // canonicalID
        if (bFoundCanonical == FALSE) {
            // test exclusion because of differences between Olson tzdata and CLDR
            UBool isExcluded = FALSE;
            for (k = 0; excluded2[k] != 0; k++) {
                if (*tzid == UnicodeString(excluded2[k])) {
                    isExcluded = TRUE;
                    break;
                }
            }
            if (isExcluded) {
                continue;
            }
            errln((UnicodeString)"FAIL: No timezone ids match the canonical ID " + canonicalID);
        }
    }
    delete s;

    // Testing some special cases
    static const struct {
        const char *id;
        const char *expected;
        UBool isSystem;
    } data[] = {
        {"GMT-03", "GMT-03:00", FALSE},
        {"GMT+4", "GMT+04:00", FALSE},
        {"GMT-055", "GMT-00:55", FALSE},
        {"GMT+430", "GMT+04:30", FALSE},
        {"GMT-12:15", "GMT-12:15", FALSE},
        {"GMT-091015", "GMT-09:10:15", FALSE},
        {"GMT+1:90", 0, FALSE},
        {"America/Argentina/Buenos_Aires", "America/Buenos_Aires", TRUE},
        {"bogus", 0, FALSE},
        {"", 0, FALSE},
        {0, 0, FALSE}
    };

    UBool isSystemID;
    for (i = 0; data[i].id != 0; i++) {
        TimeZone::getCanonicalID(UnicodeString(data[i].id), canonicalID, isSystemID, ec);
        if (U_FAILURE(ec)) {
            if (ec != U_ILLEGAL_ARGUMENT_ERROR || data[i].expected != 0) {
                errln((UnicodeString)"FAIL: getCanonicalID(\"" + data[i].id
                    + "\") returned status U_ILLEGAL_ARGUMENT_ERROR");
            }
            ec = U_ZERO_ERROR;
            continue;
        }
        if (canonicalID != data[i].expected) {
            dataerrln((UnicodeString)"FAIL: getCanonicalID(\"" + data[i].id
                + "\") returned " + canonicalID + " - expected: " + data[i].expected);
        }
        if (isSystemID != data[i].isSystem) {
            dataerrln((UnicodeString)"FAIL: getCanonicalID(\"" + data[i].id
                + "\") set " + isSystemID + " to isSystemID");
        }
    }
}

//
//  Test Display Names, choosing zones and lcoales where there are multiple
//                      meta-zones defined.
//
static struct   {
    const char            *zoneName;
    const char            *localeName;
    UBool                  summerTime;
    TimeZone::EDisplayType style;
    const char            *expectedDisplayName; } 
 zoneDisplayTestData [] =  {
     //  zone id         locale   summer   format          expected display name
      {"Europe/London",     "en", FALSE, TimeZone::SHORT, "GMT"},
      {"Europe/London",     "en", FALSE, TimeZone::LONG,  "Greenwich Mean Time"},
      {"Europe/London",     "en", TRUE,  TimeZone::SHORT, "GMT+01:00" /*"BST"*/},
      {"Europe/London",     "en", TRUE,  TimeZone::LONG,  "British Summer Time"},
      
      {"America/Anchorage", "en", FALSE, TimeZone::SHORT, "AKST"},
      {"America/Anchorage", "en", FALSE, TimeZone::LONG,  "Alaska Standard Time"},
      {"America/Anchorage", "en", TRUE,  TimeZone::SHORT, "AKDT"},
      {"America/Anchorage", "en", TRUE,  TimeZone::LONG,  "Alaska Daylight Time"},
      
      // Southern Hemisphere, all data from meta:Australia_Western
      {"Australia/Perth",   "en", FALSE, TimeZone::SHORT, "GMT+08:00"/*"AWST"*/},
      {"Australia/Perth",   "en", FALSE, TimeZone::LONG,  "Australian Western Standard Time"},
      {"Australia/Perth",   "en", TRUE,  TimeZone::SHORT, "GMT+09:00"/*"AWDT"*/},
      {"Australia/Perth",   "en", TRUE,  TimeZone::LONG,  "Australian Western Daylight Time"},
       
      {"America/Sao_Paulo",  "en", FALSE, TimeZone::SHORT, "GMT-03:00"/*"BRT"*/},
      {"America/Sao_Paulo",  "en", FALSE, TimeZone::LONG,  "Brasilia Time"},
      {"America/Sao_Paulo",  "en", TRUE,  TimeZone::SHORT, "GMT-02:00"/*"BRST"*/},
      {"America/Sao_Paulo",  "en", TRUE,  TimeZone::LONG,  "Brasilia Summer Time"},
       
      // No Summer Time, but had it before 1983.
      {"Pacific/Honolulu",   "en", FALSE, TimeZone::SHORT, "HST"},
      {"Pacific/Honolulu",   "en", FALSE, TimeZone::LONG,  "Hawaii-Aleutian Standard Time"},
      {"Pacific/Honolulu",   "en", TRUE,  TimeZone::SHORT, "HST"},
      {"Pacific/Honolulu",   "en", TRUE,  TimeZone::LONG,  "Hawaii-Aleutian Standard Time"},
       
      // Northern, has Summer, not commonly used.
      {"Europe/Helsinki",    "en", FALSE, TimeZone::SHORT, "GMT+02:00"/*"EET"*/},
      {"Europe/Helsinki",    "en", FALSE, TimeZone::LONG,  "Eastern European Time"},
      {"Europe/Helsinki",    "en", TRUE,  TimeZone::SHORT, "GMT+03:00"/*"EEST"*/},
      {"Europe/Helsinki",    "en", true,  TimeZone::LONG,  "Eastern European Summer Time"},
      {NULL, NULL, FALSE, TimeZone::SHORT, NULL}   // NULL values terminate list
    };

void TimeZoneTest::TestDisplayNamesMeta() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar cal(*TimeZone::getGMT(), status);
    if (failure(status, "GregorianCalendar", TRUE)) return;

    UBool isReferenceYear = TRUE;
    if (cal.get(UCAL_YEAR, status) != TimeZoneTest::REFERENCE_YEAR) {
        isReferenceYear = FALSE;
    }

    UBool sawAnError = FALSE;
    for (int testNum   = 0; zoneDisplayTestData[testNum].zoneName != NULL; testNum++) {
        Locale locale  = Locale::createFromName(zoneDisplayTestData[testNum].localeName);
        TimeZone *zone = TimeZone::createTimeZone(zoneDisplayTestData[testNum].zoneName);
        UnicodeString displayName;
        zone->getDisplayName(zoneDisplayTestData[testNum].summerTime,
                             zoneDisplayTestData[testNum].style,
                             locale,
                             displayName);
        if (displayName != zoneDisplayTestData[testNum].expectedDisplayName) {
            char  name[100];
            UErrorCode status = U_ZERO_ERROR;
            displayName.extract(name, 100, NULL, status);
            if (isReferenceYear) {
                sawAnError = TRUE;
                dataerrln("Incorrect time zone display name.  zone = \"%s\",\n"
                      "   locale = \"%s\",   style = %s,  Summertime = %d\n"
                      "   Expected \"%s\", "
                      "   Got \"%s\"\n   Error: %s", zoneDisplayTestData[testNum].zoneName,
                                         zoneDisplayTestData[testNum].localeName,
                                         zoneDisplayTestData[testNum].style==TimeZone::SHORT ?
                                            "SHORT" : "LONG",
                                         zoneDisplayTestData[testNum].summerTime,
                                         zoneDisplayTestData[testNum].expectedDisplayName,
                                         name,
                                         u_errorName(status));
            } else {
                logln("Incorrect time zone display name.  zone = \"%s\",\n"
                      "   locale = \"%s\",   style = %s,  Summertime = %d\n"
                      "   Expected \"%s\", "
                      "   Got \"%s\"\n", zoneDisplayTestData[testNum].zoneName,
                                         zoneDisplayTestData[testNum].localeName,
                                         zoneDisplayTestData[testNum].style==TimeZone::SHORT ?
                                            "SHORT" : "LONG",
                                         zoneDisplayTestData[testNum].summerTime,
                                         zoneDisplayTestData[testNum].expectedDisplayName,
                                         name);
            }
        }
        delete zone;
    }
    if (sawAnError) {
        dataerrln("***Note: Errors could be the result of changes to zoneStrings locale data");
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
