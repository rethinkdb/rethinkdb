/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtrule.h"
#include "unicode/tzrule.h"
#include "unicode/rbtz.h"
#include "unicode/simpletz.h"
#include "unicode/tzrule.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/ucal.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "unicode/tztrans.h"
#include "unicode/vtzone.h"
#include "tzrulets.h"
#include "zrule.h"
#include "ztrans.h"
#include "vzone.h"
#include "cmemory.h"

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break
#define HOUR (60*60*1000)

static const UVersionInfo ICU_453 = {4,5,3,0};

static const char *const TESTZIDS[] = {
        "AGT",
        "America/New_York",
        "America/Los_Angeles",
        "America/Indiana/Indianapolis",
        "America/Havana",
        "Europe/Lisbon",
        "Europe/Paris",
        "Asia/Tokyo",
        "Asia/Sakhalin",
        "Africa/Cairo",
        "Africa/Windhoek",
        "Australia/Sydney",
        "Etc/GMT+8"
};

static UBool hasEquivalentTransitions(/*const*/ BasicTimeZone& tz1, /*const*/BasicTimeZone& tz2,
                                        UDate start, UDate end,
                                        UBool ignoreDstAmount, int32_t maxTransitionTimeDelta,
                                        UErrorCode& status);

class TestZIDEnumeration : public StringEnumeration {
public:
    TestZIDEnumeration(UBool all = FALSE);
    ~TestZIDEnumeration();

    virtual int32_t count(UErrorCode& /*status*/) const {
        return len;
    }
    virtual const UnicodeString *snext(UErrorCode& status);
    virtual void reset(UErrorCode& status);
    static inline UClassID getStaticClassID() {
        return (UClassID)&fgClassID;
    }
    virtual UClassID getDynamicClassID() const {
        return getStaticClassID();
    }
private:
    static const char fgClassID;
    int32_t idx;
    int32_t len;
    StringEnumeration   *tzenum;
};

const char TestZIDEnumeration::fgClassID = 0;

TestZIDEnumeration::TestZIDEnumeration(UBool all)
: idx(0) {
    UErrorCode status = U_ZERO_ERROR;
    if (all) {
        tzenum = TimeZone::createEnumeration();
        len = tzenum->count(status);
    } else {
        tzenum = NULL;
        len = (int32_t)sizeof(TESTZIDS)/sizeof(TESTZIDS[0]);
    }
}

TestZIDEnumeration::~TestZIDEnumeration() {
    if (tzenum != NULL) {
        delete tzenum;
    }
}

const UnicodeString*
TestZIDEnumeration::snext(UErrorCode& status) {
    if (tzenum != NULL) {
        return tzenum->snext(status);
    } else if (U_SUCCESS(status) && idx < len) {
        unistr = UnicodeString(TESTZIDS[idx++], "");
        return &unistr;
    }
    return NULL;
}

void
TestZIDEnumeration::reset(UErrorCode& status) {
    if (tzenum != NULL) {
        tzenum->reset(status);
    } else {
        idx = 0;
    }
}


void TimeZoneRuleTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) {
        logln("TestSuite TestTimeZoneRule");
    }
    switch (index) {
        CASE(0, TestSimpleRuleBasedTimeZone);
        CASE(1, TestHistoricalRuleBasedTimeZone);
        CASE(2, TestOlsonTransition);
        CASE(3, TestRBTZTransition);
        CASE(4, TestHasEquivalentTransitions);
        CASE(5, TestVTimeZoneRoundTrip);
        CASE(6, TestVTimeZoneRoundTripPartial);
        CASE(7, TestVTimeZoneSimpleWrite);
        CASE(8, TestVTimeZoneHeaderProps);
        CASE(9, TestGetSimpleRules);
        CASE(10, TestTimeZoneRuleCoverage);
        CASE(11, TestSimpleTimeZoneCoverage);
        CASE(12, TestVTimeZoneCoverage);
        CASE(13, TestVTimeZoneParse);
        CASE(14, TestT6216);
        CASE(15, TestT6669);
        CASE(16, TestVTimeZoneWrapper);
        default: name = ""; break;
    }
}

/*
 * Compare SimpleTimeZone with equivalent RBTZ
 */
void
TimeZoneRuleTest::TestSimpleRuleBasedTimeZone(void) {
    UErrorCode status = U_ZERO_ERROR;
    SimpleTimeZone stz(-1*HOUR, "TestSTZ",
        UCAL_SEPTEMBER, -30, -UCAL_SATURDAY, 1*HOUR, SimpleTimeZone::WALL_TIME,
        UCAL_FEBRUARY, 2, UCAL_SUNDAY, 1*HOUR, SimpleTimeZone::WALL_TIME,
        1*HOUR, status);
    if (U_FAILURE(status)) {
        errln("FAIL: Couldn't create SimpleTimezone.");
    }

    DateTimeRule *dtr;
    AnnualTimeZoneRule *atzr;
    int32_t STARTYEAR = 2000;

    InitialTimeZoneRule *ir = new InitialTimeZoneRule(
        "RBTZ_Initial", // Initial time Name
        -1*HOUR,        // Raw offset
        1*HOUR);        // DST saving amount

    // Original rules
    RuleBasedTimeZone *rbtz1 = new RuleBasedTimeZone("RBTZ1", ir->clone());
    dtr = new DateTimeRule(UCAL_SEPTEMBER, 30, UCAL_SATURDAY, FALSE,
        1*HOUR, DateTimeRule::WALL_TIME); // SUN<=30 in September, at 1AM wall time
    atzr = new AnnualTimeZoneRule("RBTZ_DST1",
        -1*HOUR /*rawOffset*/, 1*HOUR /*dstSavings*/, dtr,
        STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz1->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 1-1.");
    }
    dtr = new DateTimeRule(UCAL_FEBRUARY, 2, UCAL_SUNDAY,
        1*HOUR, DateTimeRule::WALL_TIME);  // 2nd Sunday in February, at 1AM wall time
    atzr = new AnnualTimeZoneRule("RBTZ_STD1",
        -1*HOUR /*rawOffset*/, 0 /*dstSavings*/, dtr,
        STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz1->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 1-2.");
    }
    rbtz1->complete(status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't complete RBTZ 1.");
    }

    // Equivalent, but different date rule type
    RuleBasedTimeZone *rbtz2 = new RuleBasedTimeZone("RBTZ2", ir->clone());
    dtr = new DateTimeRule(UCAL_SEPTEMBER, -1, UCAL_SATURDAY,
        1*HOUR, DateTimeRule::WALL_TIME); // Last Sunday in September at 1AM wall time
    atzr = new AnnualTimeZoneRule("RBTZ_DST2", -1*HOUR, 1*HOUR, dtr, STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz2->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 2-1.");
    }
    dtr = new DateTimeRule(UCAL_FEBRUARY, 8, UCAL_SUNDAY, true,
        1*HOUR, DateTimeRule::WALL_TIME); // SUN>=8 in February, at 1AM wall time
    atzr = new AnnualTimeZoneRule("RBTZ_STD2", -1*HOUR, 0, dtr, STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz2->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 2-2.");
    }
    rbtz2->complete(status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't complete RBTZ 2");
    }

    // Equivalent, but different time rule type
    RuleBasedTimeZone *rbtz3 = new RuleBasedTimeZone("RBTZ3", ir->clone());
    dtr = new DateTimeRule(UCAL_SEPTEMBER, 30, UCAL_SATURDAY, false,
        2*HOUR, DateTimeRule::UTC_TIME);
    atzr = new AnnualTimeZoneRule("RBTZ_DST3", -1*HOUR, 1*HOUR, dtr, STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz3->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 3-1.");
    }
    dtr = new DateTimeRule(UCAL_FEBRUARY, 2, UCAL_SUNDAY,
        0*HOUR, DateTimeRule::STANDARD_TIME);
    atzr = new AnnualTimeZoneRule("RBTZ_STD3", -1*HOUR, 0, dtr, STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz3->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 3-2.");
    }
    rbtz3->complete(status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't complete RBTZ 3");
    }

    // Check equivalency for 10 years
    UDate start = getUTCMillis(STARTYEAR, UCAL_JANUARY, 1);
    UDate until = getUTCMillis(STARTYEAR + 10, UCAL_JANUARY, 1);

    if (!(stz.hasEquivalentTransitions(*rbtz1, start, until, TRUE, status))) {
        errln("FAIL: rbtz1 must be equivalent to the SimpleTimeZone in the time range.");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions");
    }
    if (!(stz.hasEquivalentTransitions(*rbtz2, start, until, TRUE, status))) {
        errln("FAIL: rbtz2 must be equivalent to the SimpleTimeZone in the time range.");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions");
    }
    if (!(stz.hasEquivalentTransitions(*rbtz3, start, until, TRUE, status))) {
        errln("FAIL: rbtz3 must be equivalent to the SimpleTimeZone in the time range.");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions");
    }

    // hasSameRules
    if (rbtz1->hasSameRules(*rbtz2)) {
        errln("FAIL: rbtz1 and rbtz2 have different rules, but returned true.");
    }
    if (rbtz1->hasSameRules(*rbtz3)) {
        errln("FAIL: rbtz1 and rbtz3 have different rules, but returned true.");
    }
    RuleBasedTimeZone *rbtz1c = (RuleBasedTimeZone*)rbtz1->clone();
    if (!rbtz1->hasSameRules(*rbtz1c)) {
        errln("FAIL: Cloned RuleBasedTimeZone must have the same rules with the original.");
    }

    // getOffset
    int32_t era, year, month, dayOfMonth, dayOfWeek, millisInDay;
    UDate time;
    int32_t offset, dstSavings;
    UBool dst;

    GregorianCalendar *cal = new GregorianCalendar(status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: Could not create a Gregorian calendar instance.: %s", u_errorName(status));
        delete rbtz1;
        delete rbtz2;
        delete rbtz3;
        delete rbtz1c;
        return;
    }
    cal->setTimeZone(*rbtz1);
    cal->clear();

    // Jan 1, 1000 BC
    cal->set(UCAL_ERA, GregorianCalendar::BC);
    cal->set(1000, UCAL_JANUARY, 1);

    era = cal->get(UCAL_ERA, status);
    year = cal->get(UCAL_YEAR, status);
    month = cal->get(UCAL_MONTH, status);
    dayOfMonth = cal->get(UCAL_DAY_OF_MONTH, status);
    dayOfWeek = cal->get(UCAL_DAY_OF_WEEK, status);
    millisInDay = cal->get(UCAL_MILLISECONDS_IN_DAY, status);
    time = cal->getTime(status);
    if (U_FAILURE(status)) {
        errln("FAIL: Could not get calendar field values.");
    }
    offset = rbtz1->getOffset(era, year, month, dayOfMonth, dayOfWeek, millisInDay, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(7 args) failed.");
    }
    if (offset != 0) {
        errln(UnicodeString("FAIL: Invalid time zone offset: ") + offset + " /expected: 0");
    }
    dst = rbtz1->inDaylightTime(time, status);
    if (U_FAILURE(status)) {
        errln("FAIL: inDaylightTime failed.");
    }
    if (!dst) {
        errln("FAIL: Invalid daylight saving time");
    }
    rbtz1->getOffset(time, TRUE, offset, dstSavings, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(5 args) failed.");
    }
    if (offset != -3600000) {
        errln(UnicodeString("FAIL: Invalid time zone raw offset: ") + offset + " /expected: -3600000");
    }
    if (dstSavings != 3600000) {            
        errln(UnicodeString("FAIL: Invalid DST amount: ") + dstSavings + " /expected: 3600000");
    }

    // July 1, 2000, AD
    cal->set(UCAL_ERA, GregorianCalendar::AD);
    cal->set(2000, UCAL_JULY, 1);

    era = cal->get(UCAL_ERA, status);
    year = cal->get(UCAL_YEAR, status);
    month = cal->get(UCAL_MONTH, status);
    dayOfMonth = cal->get(UCAL_DAY_OF_MONTH, status);
    dayOfWeek = cal->get(UCAL_DAY_OF_WEEK, status);
    millisInDay = cal->get(UCAL_MILLISECONDS_IN_DAY, status);
    time = cal->getTime(status);
    if (U_FAILURE(status)) {
        errln("FAIL: Could not get calendar field values.");
    }
    offset = rbtz1->getOffset(era, year, month, dayOfMonth, dayOfWeek, millisInDay, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(7 args) failed.");
    }
    if (offset != -3600000) {
        errln((UnicodeString)"FAIL: Invalid time zone offset: " + offset + " /expected: -3600000");
    }
    dst = rbtz1->inDaylightTime(time, status);
    if (U_FAILURE(status)) {
        errln("FAIL: inDaylightTime failed.");
    }
    if (dst) {
        errln("FAIL: Invalid daylight saving time");
    }
    rbtz1->getOffset(time, TRUE, offset, dstSavings, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(5 args) failed.");
    }
    if (offset != -3600000) {
        errln((UnicodeString)"FAIL: Invalid time zone raw offset: " + offset + " /expected: -3600000");
    }
    if (dstSavings != 0) {            
        errln((UnicodeString)"FAIL: Invalid DST amount: " + dstSavings + " /expected: 0");
    }

    // getRawOffset
    offset = rbtz1->getRawOffset();
    if (offset != -1*HOUR) {
        errln((UnicodeString)"FAIL: Invalid time zone raw offset returned by getRawOffset: "
            + offset + " /expected: -3600000");
    }

    // operator=/==/!=
    RuleBasedTimeZone rbtz0("RBTZ1", ir->clone());
    if (rbtz0 == *rbtz1 || !(rbtz0 != *rbtz1)) {
        errln("FAIL: RuleBasedTimeZone rbtz0 is not equal to rbtz1, but got wrong result");
    }
    rbtz0 = *rbtz1;
    if (rbtz0 != *rbtz1 || !(rbtz0 == *rbtz1)) {
        errln("FAIL: RuleBasedTimeZone rbtz0 is equal to rbtz1, but got wrong result");
    }

    // setRawOffset
    const int32_t RAW = -10*HOUR;
    rbtz0.setRawOffset(RAW);
    if (rbtz0.getRawOffset() != RAW) {
        logln("setRawOffset is implemented in RuleBasedTimeZone");
    }

    // useDaylightTime
    if (!rbtz1->useDaylightTime()) {
        errln("FAIL: useDaylightTime returned FALSE");
    }

    // Try to add 3rd final rule
    dtr = new DateTimeRule(UCAL_OCTOBER, 15, 1*HOUR, DateTimeRule::WALL_TIME);
    atzr = new AnnualTimeZoneRule("3RD_ATZ", -1*HOUR, 2*HOUR, dtr, STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbtz1->addTransitionRule(atzr, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: 3rd final rule must be rejected");
    } else {
        delete atzr;
    }

    // Try to add an initial rule
    InitialTimeZoneRule *ir1 = new InitialTimeZoneRule("Test Initial", 2*HOUR, 0);
    rbtz1->addTransitionRule(ir1, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: InitialTimeZoneRule must be rejected");
    } else {
        delete ir1;
    }

    delete ir;
    delete rbtz1;
    delete rbtz2;
    delete rbtz3;
    delete rbtz1c;
    delete cal;
}

/*
 * Test equivalency between OlsonTimeZone and custom RBTZ representing the
 * equivalent rules in a certain time range
 */
void
TimeZoneRuleTest::TestHistoricalRuleBasedTimeZone(void) {
    UErrorCode status = U_ZERO_ERROR;

    // Compare to America/New_York with equivalent RBTZ
    BasicTimeZone *ny = (BasicTimeZone*)TimeZone::createTimeZone("America/New_York");

    //RBTZ
    InitialTimeZoneRule *ir = new InitialTimeZoneRule("EST", -5*HOUR, 0);
    RuleBasedTimeZone *rbtz = new RuleBasedTimeZone("EST5EDT", ir);

    DateTimeRule *dtr;
    AnnualTimeZoneRule *tzr;

    // Standard time
    dtr = new DateTimeRule(UCAL_OCTOBER, -1, UCAL_SUNDAY,
        2*HOUR, DateTimeRule::WALL_TIME); // Last Sunday in October, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EST", -5*HOUR /*rawOffset*/, 0 /*dstSavings*/, dtr, 1967, 2006);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 1.");
    }

    dtr = new DateTimeRule(UCAL_NOVEMBER, 1, UCAL_SUNDAY,
        true, 2*HOUR, DateTimeRule::WALL_TIME); // SUN>=1 in November, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EST", -5*HOUR, 0, dtr, 2007, AnnualTimeZoneRule::MAX_YEAR);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 2.");
    }

    // Daylight saving time
    dtr = new DateTimeRule(UCAL_APRIL, -1, UCAL_SUNDAY,
        2*HOUR, DateTimeRule::WALL_TIME); // Last Sunday in April, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EDT", -5*HOUR, 1*HOUR, dtr, 1967, 1973);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 3.");
    }

    dtr = new DateTimeRule(UCAL_JANUARY, 6,
        2*HOUR, DateTimeRule::WALL_TIME); // January 6, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EDT", -5*HOUR, 1*HOUR, dtr, 1974, 1974);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 4.");
    }
    
    dtr = new DateTimeRule(UCAL_FEBRUARY, 23,
        2*HOUR, DateTimeRule::WALL_TIME); // February 23, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EDT", -5*HOUR, 1*HOUR, dtr, 1975, 1975);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 5.");
    }

    dtr = new DateTimeRule(UCAL_APRIL, -1, UCAL_SUNDAY,
        2*HOUR, DateTimeRule::WALL_TIME); // Last Sunday in April, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EDT", -5*HOUR, 1*HOUR, dtr, 1976, 1986);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 6.");
    }

    dtr = new DateTimeRule(UCAL_APRIL, 1, UCAL_SUNDAY,
        true, 2*HOUR, DateTimeRule::WALL_TIME); // SUN>=1 in April, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EDT", -5*HOUR, 1*HOUR, dtr, 1987, 2006);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 7.");
    }

    dtr = new DateTimeRule(UCAL_MARCH, 8, UCAL_SUNDAY,
        true, 2*HOUR, DateTimeRule::WALL_TIME); // SUN>=8 in March, at 2AM wall time
    tzr = new AnnualTimeZoneRule("EDT", -5*HOUR, 1*HOUR, dtr, 2007, AnnualTimeZoneRule::MAX_YEAR);
    rbtz->addTransitionRule(tzr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't add AnnualTimeZoneRule 7.");
    }

    rbtz->complete(status);
    if (U_FAILURE(status)) {
        errln("FAIL: couldn't complete RBTZ.");
    }

    // hasEquivalentTransitions
    UDate jan1_1950 = getUTCMillis(1950, UCAL_JANUARY, 1);
    UDate jan1_1967 = getUTCMillis(1971, UCAL_JANUARY, 1);
    UDate jan1_2010 = getUTCMillis(2010, UCAL_JANUARY, 1);        

    if (!ny->hasEquivalentTransitions(*rbtz, jan1_1967, jan1_2010, TRUE, status)) {
        dataerrln("FAIL: The RBTZ must be equivalent to America/New_York between 1967 and 2010");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions for ny/rbtz 1967-2010");
    }
    if (ny->hasEquivalentTransitions(*rbtz, jan1_1950, jan1_2010, TRUE, status)) {
        errln("FAIL: The RBTZ must not be equivalent to America/New_York between 1950 and 2010");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions for ny/rbtz 1950-2010");
    }

    // Same with above, but calling RBTZ#hasEquivalentTransitions against OlsonTimeZone
    if (!rbtz->hasEquivalentTransitions(*ny, jan1_1967, jan1_2010, TRUE, status)) {
        dataerrln("FAIL: The RBTZ must be equivalent to America/New_York between 1967 and 2010 ");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions for rbtz/ny 1967-2010");
    }
    if (rbtz->hasEquivalentTransitions(*ny, jan1_1950, jan1_2010, TRUE, status)) {
        errln("FAIL: The RBTZ must not be equivalent to America/New_York between 1950 and 2010");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions for rbtz/ny 1950-2010");
    }

    // TimeZone APIs
    if (ny->hasSameRules(*rbtz) || rbtz->hasSameRules(*ny)) {
        errln("FAIL: hasSameRules must return false");
    }
    RuleBasedTimeZone *rbtzc = (RuleBasedTimeZone*)rbtz->clone();
    if (!rbtz->hasSameRules(*rbtzc) || !rbtz->hasEquivalentTransitions(*rbtzc, jan1_1950, jan1_2010, TRUE, status)) {
        errln("FAIL: hasSameRules/hasEquivalentTransitions must return true for cloned RBTZs");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error returned from hasEquivalentTransitions for rbtz/rbtzc 1950-2010");
    }

    UDate times[] = {
        getUTCMillis(2006, UCAL_MARCH, 15),
        getUTCMillis(2006, UCAL_NOVEMBER, 1),
        getUTCMillis(2007, UCAL_MARCH, 15),
        getUTCMillis(2007, UCAL_NOVEMBER, 1),
        getUTCMillis(2008, UCAL_MARCH, 15),
        getUTCMillis(2008, UCAL_NOVEMBER, 1),
        0
    };
    int32_t offset1, dst1;
    int32_t offset2, dst2;

    for (int i = 0; times[i] != 0; i++) {
        // Check getOffset - must return the same results for these time data
        rbtz->getOffset(times[i], FALSE, offset1, dst1, status);
        if (U_FAILURE(status)) {
            errln("FAIL: rbtz->getOffset failed");
        }
        ny->getOffset(times[i], FALSE, offset2, dst2, status);
        if (U_FAILURE(status)) {
            errln("FAIL: ny->getOffset failed");
        }
        if (offset1 != offset2 || dst1 != dst2) {
            dataerrln("FAIL: Incompatible time zone offset/dstSavings for ny and rbtz");
        }

        // Check inDaylightTime
        if (rbtz->inDaylightTime(times[i], status) != ny->inDaylightTime(times[i], status)) {
            dataerrln("FAIL: Incompatible daylight saving time for ny and rbtz");
        }
        if (U_FAILURE(status)) {
            errln("FAIL: inDaylightTime failed");
        }
    }

    delete ny;
    delete rbtz;
    delete rbtzc;
}

/*
 * Check if transitions returned by getNextTransition/getPreviousTransition
 * are actual time transitions.
 */
void
TimeZoneRuleTest::TestOlsonTransition(void) {

    const int32_t TESTYEARS[][2] = {
        {1895, 1905}, // including int32 minimum second
        {1965, 1975}, // including the epoch
        {1995, 2015}, // practical year range
        {0,0}
    };

    UErrorCode status = U_ZERO_ERROR;
    TestZIDEnumeration tzenum(!quick);
    while (TRUE) {
        const UnicodeString *tzid = tzenum.snext(status);
        if (tzid == NULL) {
            break;
        }
        if (U_FAILURE(status)) {
            errln("FAIL: error returned while enumerating timezone IDs.");
            break;
        }
        BasicTimeZone *tz = (BasicTimeZone*)TimeZone::createTimeZone(*tzid);
        for (int32_t i = 0; TESTYEARS[i][0] != 0 || TESTYEARS[i][1] != 0; i++) {
            UDate lo = getUTCMillis(TESTYEARS[i][0], UCAL_JANUARY, 1);
            UDate hi = getUTCMillis(TESTYEARS[i][1], UCAL_JANUARY, 1);
            verifyTransitions(*tz, lo, hi);
        }
        delete tz;
    }
}

/*
 * Check if an OlsonTimeZone and its equivalent RBTZ have the exact same
 * transitions.
 */
void
TimeZoneRuleTest::TestRBTZTransition(void) {
    const int32_t STARTYEARS[] = {
        1900,
        1960,
        1990,
        2010,
        0
    };

    UErrorCode status = U_ZERO_ERROR;
    TestZIDEnumeration tzenum(!quick);
    while (TRUE) {
        const UnicodeString *tzid = tzenum.snext(status);
        if (tzid == NULL) {
            break;
        }
        if (U_FAILURE(status)) {
            errln("FAIL: error returned while enumerating timezone IDs.");
            break;
        }
        BasicTimeZone *tz = (BasicTimeZone*)TimeZone::createTimeZone(*tzid);
        int32_t ruleCount = tz->countTransitionRules(status);

        const InitialTimeZoneRule *initial;
        const TimeZoneRule **trsrules = new const TimeZoneRule*[ruleCount];
        tz->getTimeZoneRules(initial, trsrules, ruleCount, status);
        if (U_FAILURE(status)) {
            errln((UnicodeString)"FAIL: failed to get the TimeZoneRules from time zone " + *tzid);
        }
        RuleBasedTimeZone *rbtz = new RuleBasedTimeZone(*tzid, initial->clone());
        if (U_FAILURE(status)) {
            errln((UnicodeString)"FAIL: failed to get the transition rule count from time zone " + *tzid);
        }
        for (int32_t i = 0; i < ruleCount; i++) {
            rbtz->addTransitionRule(trsrules[i]->clone(), status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"FAIL: failed to add a transition rule at index " + i + " to the RBTZ for " + *tzid);
            }
        }
        rbtz->complete(status);
        if (U_FAILURE(status)) {
            errln((UnicodeString)"FAIL: complete() failed for the RBTZ for " + *tzid);
        }

        for (int32_t idx = 0; STARTYEARS[idx] != 0; idx++) {
            UDate start = getUTCMillis(STARTYEARS[idx], UCAL_JANUARY, 1);
            UDate until = getUTCMillis(STARTYEARS[idx] + 20, UCAL_JANUARY, 1);
            // Compare the original OlsonTimeZone with the RBTZ starting the startTime for 20 years

            // Ascending
            compareTransitionsAscending(*tz, *rbtz, start, until, FALSE);
            // Ascending/inclusive
            compareTransitionsAscending(*tz, *rbtz, start + 1, until, TRUE);
            // Descending
            compareTransitionsDescending(*tz, *rbtz, start, until, FALSE);
            // Descending/inclusive
            compareTransitionsDescending(*tz, *rbtz, start + 1, until, TRUE);
        }
        delete [] trsrules;
        delete rbtz;
        delete tz;
    }
}

void
TimeZoneRuleTest::TestHasEquivalentTransitions(void) {
    // America/New_York and America/Indiana/Indianapolis are equivalent
    // since 2006
    UErrorCode status = U_ZERO_ERROR;
    BasicTimeZone *newyork = (BasicTimeZone*)TimeZone::createTimeZone("America/New_York");
    BasicTimeZone *indianapolis = (BasicTimeZone*)TimeZone::createTimeZone("America/Indiana/Indianapolis");
    BasicTimeZone *gmt_5 = (BasicTimeZone*)TimeZone::createTimeZone("Etc/GMT+5");

    UDate jan1_1971 = getUTCMillis(1971, UCAL_JANUARY, 1);
    UDate jan1_2005 = getUTCMillis(2005, UCAL_JANUARY, 1);
    UDate jan1_2006 = getUTCMillis(2006, UCAL_JANUARY, 1);
    UDate jan1_2007 = getUTCMillis(2007, UCAL_JANUARY, 1);
    UDate jan1_2011 = getUTCMillis(2010, UCAL_JANUARY, 1);

    if (newyork->hasEquivalentTransitions(*indianapolis, jan1_2005, jan1_2011, TRUE, status)) {
        dataerrln("FAIL: New_York is not equivalent to Indianapolis between 2005 and 2010");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition");
    }
    if (!newyork->hasEquivalentTransitions(*indianapolis, jan1_2006, jan1_2011, TRUE, status)) {
        errln("FAIL: New_York is equivalent to Indianapolis between 2006 and 2010");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition");
    }

    if (!indianapolis->hasEquivalentTransitions(*gmt_5, jan1_1971, jan1_2006, TRUE, status)) {
        errln("FAIL: Indianapolis is equivalent to GMT+5 between 1971 and 2005");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition");
    }
    if (indianapolis->hasEquivalentTransitions(*gmt_5, jan1_1971, jan1_2007, TRUE, status)) {
        dataerrln("FAIL: Indianapolis is not equivalent to GMT+5 between 1971 and 2006");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition");
    }

    // Cloned TimeZone
    BasicTimeZone *newyork2 = (BasicTimeZone*)newyork->clone();
    if (!newyork->hasEquivalentTransitions(*newyork2, jan1_1971, jan1_2011, FALSE, status)) {
        errln("FAIL: Cloned TimeZone must have the same transitions");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition for newyork/newyork2");
    }
    if (!newyork->hasEquivalentTransitions(*newyork2, jan1_1971, jan1_2011, TRUE, status)) {
        errln("FAIL: Cloned TimeZone must have the same transitions");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition for newyork/newyork2");
    }

    // America/New_York and America/Los_Angeles has same DST start rules, but
    // raw offsets are different
    BasicTimeZone *losangeles = (BasicTimeZone*)TimeZone::createTimeZone("America/Los_Angeles");
    if (newyork->hasEquivalentTransitions(*losangeles, jan1_2006, jan1_2011, TRUE, status)) {
        dataerrln("FAIL: New_York is not equivalent to Los Angeles, but returned true");
    }
    if (U_FAILURE(status)) {
        errln("FAIL: error status is returned from hasEquivalentTransition for newyork/losangeles");
    }

    delete newyork;
    delete newyork2;
    delete indianapolis;
    delete gmt_5;
    delete losangeles;
}

/*
 * Write out time zone rules of OlsonTimeZone into VTIMEZONE format, create a new
 * VTimeZone from the VTIMEZONE data, then compare transitions
 */
void
TimeZoneRuleTest::TestVTimeZoneRoundTrip(void) {
    UDate startTime = getUTCMillis(1850, UCAL_JANUARY, 1);
    UDate endTime = getUTCMillis(2050, UCAL_JANUARY, 1);

    UErrorCode status = U_ZERO_ERROR;
    TestZIDEnumeration tzenum(!quick);
    while (TRUE) {
        const UnicodeString *tzid = tzenum.snext(status);
        if (tzid == NULL) {
            break;
        }
        if (U_FAILURE(status)) {
            errln("FAIL: error returned while enumerating timezone IDs.");
            break;
        }
        BasicTimeZone *tz = (BasicTimeZone*)TimeZone::createTimeZone(*tzid);
        VTimeZone *vtz_org = VTimeZone::createVTimeZoneByID(*tzid);
        vtz_org->setTZURL("http://source.icu-project.org/timezone");
        vtz_org->setLastModified(Calendar::getNow());
        VTimeZone *vtz_new = NULL;
        UnicodeString vtzdata;
        // Write out VTIMEZONE data
        vtz_org->write(vtzdata, status);
        if (U_FAILURE(status)) {
            errln((UnicodeString)"FAIL: error returned while writing time zone rules for " +
                *tzid + " into VTIMEZONE format.");
        } else {
            // Read VTIMEZONE data
            vtz_new = VTimeZone::createVTimeZone(vtzdata, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"FAIL: error returned while reading VTIMEZONE data for " + *tzid);
            } else {
                // Write out VTIMEZONE one more time
                UnicodeString vtzdata1;
                vtz_new->write(vtzdata1, status);
                if (U_FAILURE(status)) {
                    errln((UnicodeString)"FAIL: error returned while writing time zone rules for " +
                        *tzid + "(vtz_new) into VTIMEZONE format.");
                } else {
                    // Make sure VTIMEZONE data is exactly same with the first one
                    if (vtzdata != vtzdata1) {
                        errln((UnicodeString)"FAIL: different VTIMEZONE data after round trip for " + *tzid);
                    }
                }
                // Check equivalency after the first transition.
                // The DST information before the first transition might be lost
                // because there is no good way to represent the initial time with
                // VTIMEZONE.
                int32_t raw1, raw2, dst1, dst2;
                tz->getOffset(startTime, FALSE, raw1, dst1, status);
                vtz_new->getOffset(startTime, FALSE, raw2, dst2, status);
                if (U_FAILURE(status)) {
                    errln("FAIL: error status is returned from getOffset");
                } else {
                    if (raw1 + dst1 != raw2 + dst2) {
                        errln("FAIL: VTimeZone for " + *tzid +
                            " is not equivalent to its OlsonTimeZone corresponding at "
                            + dateToString(startTime));
                    }
                    TimeZoneTransition trans;
                    UBool avail = tz->getNextTransition(startTime, FALSE, trans);
                    if (avail) {
                        if (!vtz_new->hasEquivalentTransitions(*tz, trans.getTime(),
                                endTime, TRUE, status)) {
                            int32_t maxDelta = 1000;
                            if (!hasEquivalentTransitions(*vtz_new, *tz, trans.getTime() + maxDelta,
                                endTime, TRUE, maxDelta, status)) {
                                errln("FAIL: VTimeZone for " + *tzid +
                                    " is not equivalent to its OlsonTimeZone corresponding.");
                            } else {
                                logln("VTimeZone for " + *tzid +
                                    "  differs from its OlsonTimeZone corresponding with maximum transition time delta - " + maxDelta);
                            }
                        }
                        if (U_FAILURE(status)) {
                            errln("FAIL: error status is returned from hasEquivalentTransition");
                        }
                    }
                }
            }
            if (vtz_new != NULL) {
                delete vtz_new;
                vtz_new = NULL;
            }
        }
        delete tz;
        delete vtz_org;
    }
}

/*
 * Write out time zone rules of OlsonTimeZone after a cutover date into VTIMEZONE format,
 * create a new VTimeZone from the VTIMEZONE data, then compare transitions
 */
void
TimeZoneRuleTest::TestVTimeZoneRoundTripPartial(void) {
    const int32_t STARTYEARS[] = {
        1900,
        1950,
        2020,
        0
    };
    UDate endTime = getUTCMillis(2050, UCAL_JANUARY, 1);

    UErrorCode status = U_ZERO_ERROR;
    TestZIDEnumeration tzenum(!quick);
    while (TRUE) {
        const UnicodeString *tzid = tzenum.snext(status);
        if (tzid == NULL) {
            break;
        }
        if (U_FAILURE(status)) {
            errln("FAIL: error returned while enumerating timezone IDs.");
            break;
        }
        BasicTimeZone *tz = (BasicTimeZone*)TimeZone::createTimeZone(*tzid);
        VTimeZone *vtz_org = VTimeZone::createVTimeZoneByID(*tzid);
        VTimeZone *vtz_new = NULL;
        UnicodeString vtzdata;

        for (int32_t i = 0; STARTYEARS[i] != 0; i++) {
            // Write out VTIMEZONE
            UDate startTime = getUTCMillis(STARTYEARS[i], UCAL_JANUARY, 1);
            vtz_org->write(startTime, vtzdata, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"FAIL: error returned while writing time zone rules for " +
                    *tzid + " into VTIMEZONE format since " + dateToString(startTime));
            } else {
                // Read VTIMEZONE data
                vtz_new = VTimeZone::createVTimeZone(vtzdata, status);
                if (U_FAILURE(status)) {
                    errln((UnicodeString)"FAIL: error returned while reading VTIMEZONE data for " + *tzid
                        + " since " + dateToString(startTime));
                } else {
                    // Check equivalency after the first transition.
                    // The DST information before the first transition might be lost
                    // because there is no good way to represent the initial time with
                    // VTIMEZONE.
                    int32_t raw1, raw2, dst1, dst2;
                    tz->getOffset(startTime, FALSE, raw1, dst1, status);
                    vtz_new->getOffset(startTime, FALSE, raw2, dst2, status);
                    if (U_FAILURE(status)) {
                        errln("FAIL: error status is returned from getOffset");
                    } else {
                        if (raw1 + dst1 != raw2 + dst2) {
                            errln("FAIL: VTimeZone for " + *tzid +
                                " is not equivalent to its OlsonTimeZone corresponding at "
                                + dateToString(startTime));
                        }
                        TimeZoneTransition trans;
                        UBool avail = tz->getNextTransition(startTime, FALSE, trans);
                        if (avail) {
                            if (!vtz_new->hasEquivalentTransitions(*tz, trans.getTime(),
                                    endTime, TRUE, status)) {
                                int32_t maxDelta = 1000;
                                if (!hasEquivalentTransitions(*vtz_new, *tz, trans.getTime() + maxDelta,
                                    endTime, TRUE, maxDelta, status)) {
                                    errln("FAIL: VTimeZone for " + *tzid +
                                        " is not equivalent to its OlsonTimeZone corresponding.");
                                } else {
                                    logln("VTimeZone for " + *tzid +
                                        "  differs from its OlsonTimeZone corresponding with maximum transition time delta - " + maxDelta);
                                }

                            }
                            if (U_FAILURE(status)) {
                                errln("FAIL: error status is returned from hasEquivalentTransition");
                            }
                        }
                    }
                }
            }
            if (vtz_new != NULL) {
                delete vtz_new;
                vtz_new = NULL;
            }
        }
        delete tz;
        delete vtz_org;
    }
}

/*
 * Write out simple time zone rules from an OlsonTimeZone at various time into VTIMEZONE
 * format and create a new VTimeZone from the VTIMEZONE data, then make sure the raw offset
 * and DST savings are same in these two time zones.
 */
void
TimeZoneRuleTest::TestVTimeZoneSimpleWrite(void) {
    const int32_t TESTDATES[][3] = {
        {2006,  UCAL_JANUARY,   1},
        {2006,  UCAL_MARCH,     15},
        {2006,  UCAL_MARCH,     31},
        {2006,  UCAL_OCTOBER,   25},
        {2006,  UCAL_NOVEMBER,  1},
        {2006,  UCAL_NOVEMBER,  5},
        {2007,  UCAL_JANUARY,   1},
        {0,     0,              0}
    };

    UErrorCode status = U_ZERO_ERROR;
    TestZIDEnumeration tzenum(!quick);
    while (TRUE) {
        const UnicodeString *tzid = tzenum.snext(status);
        if (tzid == NULL) {
            break;
        }
        if (U_FAILURE(status)) {
            errln("FAIL: error returned while enumerating timezone IDs.");
            break;
        }
        VTimeZone *vtz_org = VTimeZone::createVTimeZoneByID(*tzid);
        VTimeZone *vtz_new = NULL;
        UnicodeString vtzdata;

        for (int32_t i = 0; TESTDATES[i][0] != 0; i++) {
            // Write out VTIMEZONE
            UDate time = getUTCMillis(TESTDATES[i][0], TESTDATES[i][1], TESTDATES[i][2]);
            vtz_org->writeSimple(time, vtzdata, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"FAIL: error returned while writing simple time zone rules for " +
                    *tzid + " into VTIMEZONE format at " + dateToString(time));
            } else {
                // Read VTIMEZONE data
                vtz_new = VTimeZone::createVTimeZone(vtzdata, status);
                if (U_FAILURE(status)) {
                    errln((UnicodeString)"FAIL: error returned while reading simple VTIMEZONE data for " + *tzid
                        + " at " + dateToString(time));
                } else {
                    // Check equivalency
                    int32_t raw0, dst0;
                    int32_t raw1, dst1;
                    vtz_org->getOffset(time, FALSE, raw0, dst0, status);
                    vtz_new->getOffset(time, FALSE, raw1, dst1, status);
                    if (U_SUCCESS(status)) {
                        if (raw0 != raw1 || dst0 != dst1) {
                            errln("FAIL: VTimeZone writeSimple for " + *tzid + " at "
                                + dateToString(time) + " failed to the round trip.");
                        }
                    } else {
                        errln("FAIL: getOffset returns error status");
                    }
                }
            }
            if (vtz_new != NULL) {
                delete vtz_new;
                vtz_new = NULL;
            }
        }
        delete vtz_org;
    }
}

/*
 * Write out time zone rules of OlsonTimeZone into VTIMEZONE format with RFC2445 header TZURL and
 * LAST-MODIFIED, create a new VTimeZone from the VTIMEZONE data to see if the headers are preserved.
 */
void
TimeZoneRuleTest::TestVTimeZoneHeaderProps(void) {
    const UnicodeString TESTURL1("http://source.icu-project.org");
    const UnicodeString TESTURL2("http://www.ibm.com");

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString tzurl;
    UDate lmod;
    UDate lastmod = getUTCMillis(2007, UCAL_JUNE, 1);
    VTimeZone *vtz = VTimeZone::createVTimeZoneByID("America/Chicago");
    vtz->setTZURL(TESTURL1);
    vtz->setLastModified(lastmod);

    // Roundtrip conversion
    UnicodeString vtzdata;
    vtz->write(vtzdata, status);
    VTimeZone *newvtz1 = NULL;
    if (U_FAILURE(status)) {
        errln("FAIL: error returned while writing VTIMEZONE data 1");
        return;
    }
    // Create a new one
    newvtz1 = VTimeZone::createVTimeZone(vtzdata, status);
    if (U_FAILURE(status)) {
        errln("FAIL: error returned while loading VTIMEZONE data 1");
    } else {
        // Check if TZURL and LAST-MODIFIED properties are preserved
        newvtz1->getTZURL(tzurl);
        if (tzurl != TESTURL1) {
            errln("FAIL: TZURL 1 was not preserved");
        }
        vtz->getLastModified(lmod);
        if (lastmod != lmod) {
            errln("FAIL: LAST-MODIFIED was not preserved");
        }
    }

    if (U_SUCCESS(status)) {
        // Set different tzurl
        newvtz1->setTZURL(TESTURL2);

        // Second roundtrip, with a cutover
        newvtz1->write(vtzdata, status);
        if (U_FAILURE(status)) {
            errln("FAIL: error returned while writing VTIMEZONE data 2");
        } else {
            VTimeZone *newvtz2 = VTimeZone::createVTimeZone(vtzdata, status);
            if (U_FAILURE(status)) {
                errln("FAIL: error returned while loading VTIMEZONE data 2");
            } else {
                // Check if TZURL and LAST-MODIFIED properties are preserved
                newvtz2->getTZURL(tzurl);
                if (tzurl != TESTURL2) {
                    errln("FAIL: TZURL was not preserved in the second roundtrip");
                }
                vtz->getLastModified(lmod);
                if (lastmod != lmod) {
                    errln("FAIL: LAST-MODIFIED was not preserved in the second roundtrip");
                }
            }
            delete newvtz2;
        }
    }
    delete newvtz1;
    delete vtz;
}

/*
 * Extract simple rules from an OlsonTimeZone and make sure the rule format matches
 * the expected format.
 */
void
TimeZoneRuleTest::TestGetSimpleRules(void) {
    UDate testTimes[] = {
        getUTCMillis(1970, UCAL_JANUARY, 1),
        getUTCMillis(2000, UCAL_MARCH, 31),
        getUTCMillis(2005, UCAL_JULY, 1),
        getUTCMillis(2010, UCAL_NOVEMBER, 1),        
    };
    int32_t numTimes = sizeof(testTimes)/sizeof(UDate);
    UErrorCode status = U_ZERO_ERROR;
    TestZIDEnumeration tzenum(!quick);
    InitialTimeZoneRule *initial;
    AnnualTimeZoneRule *std, *dst;
    for (int32_t i = 0; i < numTimes ; i++) {
        while (TRUE) {
            const UnicodeString *tzid = tzenum.snext(status);
            if (tzid == NULL) {
                break;
            }
            if (U_FAILURE(status)) {
                errln("FAIL: error returned while enumerating timezone IDs.");
                break;
            }
            BasicTimeZone *tz = (BasicTimeZone*)TimeZone::createTimeZone(*tzid);
            initial = NULL;
            std = dst = NULL;
            tz->getSimpleRulesNear(testTimes[i], initial, std, dst, status);
            if (U_FAILURE(status)) {
                errln("FAIL: getSimpleRules failed.");
                break;
            }
            if (initial == NULL) {
                errln("FAIL: initial rule must not be NULL");
                break;
            } else if (!((std == NULL && dst == NULL) || (std != NULL && dst != NULL))) {
                errln("FAIL: invalid std/dst pair.");
                break;
            }
            if (std != NULL) {
                const DateTimeRule *dtr = std->getRule();
                if (dtr->getDateRuleType() != DateTimeRule::DOW) {
                    errln("FAIL: simple std rull must use DateTimeRule::DOW as date rule.");
                    break;
                }
                if (dtr->getTimeRuleType() != DateTimeRule::WALL_TIME) {
                    errln("FAIL: simple std rull must use DateTimeRule::WALL_TIME as time rule.");
                    break;
                }
                dtr = dst->getRule();
                if (dtr->getDateRuleType() != DateTimeRule::DOW) {
                    errln("FAIL: simple dst rull must use DateTimeRule::DOW as date rule.");
                    break;
                }
                if (dtr->getTimeRuleType() != DateTimeRule::WALL_TIME) {
                    errln("FAIL: simple dst rull must use DateTimeRule::WALL_TIME as time rule.");
                    break;
                }                
            }
            // Create an RBTZ from the rules and compare the offsets at the date
            RuleBasedTimeZone *rbtz = new RuleBasedTimeZone(*tzid, initial);
            if (std != NULL) {
                rbtz->addTransitionRule(std, status);
                if (U_FAILURE(status)) {
                    errln("FAIL: couldn't add std rule.");
                }
                rbtz->addTransitionRule(dst, status);
                if (U_FAILURE(status)) {
                    errln("FAIL: couldn't add dst rule.");
                }
            }
            rbtz->complete(status);
            if (U_FAILURE(status)) {
                errln("FAIL: couldn't complete rbtz for " + *tzid);
            }

            int32_t raw0, dst0, raw1, dst1;
            tz->getOffset(testTimes[i], FALSE, raw0, dst0, status);
            if (U_FAILURE(status)) {
                errln("FAIL: couldn't get offsets from tz for " + *tzid);
            }
            rbtz->getOffset(testTimes[i], FALSE, raw1, dst1, status);
            if (U_FAILURE(status)) {
                errln("FAIL: couldn't get offsets from rbtz for " + *tzid);
            }
            if (raw0 != raw1 || dst0 != dst1) {
                errln("FAIL: rbtz created by simple rule does not match the original tz for tzid " + *tzid);
            }
            delete rbtz;
            delete tz;
        }
    }
}

/*
 * API coverage tests for TimeZoneRule 
 */
void
TimeZoneRuleTest::TestTimeZoneRuleCoverage(void) {
    UDate time1 = getUTCMillis(2005, UCAL_JULY, 4);
    UDate time2 = getUTCMillis(2015, UCAL_JULY, 4);
    UDate time3 = getUTCMillis(1950, UCAL_JULY, 4);

    DateTimeRule *dtr1 = new DateTimeRule(UCAL_FEBRUARY, 29, UCAL_SUNDAY, FALSE,
            3*HOUR, DateTimeRule::WALL_TIME); // Last Sunday on or before Feb 29, at 3 AM, wall time
    DateTimeRule *dtr2 = new DateTimeRule(UCAL_MARCH, 11, 2*HOUR,
            DateTimeRule::STANDARD_TIME); // Mar 11, at 2 AM, standard time
    DateTimeRule *dtr3 = new DateTimeRule(UCAL_OCTOBER, -1, UCAL_SATURDAY,
            6*HOUR, DateTimeRule::UTC_TIME); //Last Saturday in Oct, at 6 AM, UTC
    DateTimeRule *dtr4 = new DateTimeRule(UCAL_MARCH, 8, UCAL_SUNDAY, TRUE,
            2*HOUR, DateTimeRule::WALL_TIME); // First Sunday on or after Mar 8, at 2 AM, wall time

    AnnualTimeZoneRule *a1 = new AnnualTimeZoneRule("a1", -3*HOUR, 1*HOUR, *dtr1,
            2000, AnnualTimeZoneRule::MAX_YEAR);
    AnnualTimeZoneRule *a2 = new AnnualTimeZoneRule("a2", -3*HOUR, 1*HOUR, *dtr1,
            2000, AnnualTimeZoneRule::MAX_YEAR);
    AnnualTimeZoneRule *a3 = new AnnualTimeZoneRule("a3", -3*HOUR, 1*HOUR, *dtr1,
            2000, 2010);

    InitialTimeZoneRule *i1 = new InitialTimeZoneRule("i1", -3*HOUR, 0);
    InitialTimeZoneRule *i2 = new InitialTimeZoneRule("i2", -3*HOUR, 0);
    InitialTimeZoneRule *i3 = new InitialTimeZoneRule("i3", -3*HOUR, 1*HOUR);

    UDate trtimes1[] = {0.0};
    UDate trtimes2[] = {0.0, 10000000.0};

    TimeArrayTimeZoneRule *t1 = new TimeArrayTimeZoneRule("t1", -3*HOUR, 0, trtimes1, 1, DateTimeRule::UTC_TIME);
    TimeArrayTimeZoneRule *t2 = new TimeArrayTimeZoneRule("t2", -3*HOUR, 0, trtimes1, 1, DateTimeRule::UTC_TIME);
    TimeArrayTimeZoneRule *t3 = new TimeArrayTimeZoneRule("t3", -3*HOUR, 0, trtimes2, 2, DateTimeRule::UTC_TIME);
    TimeArrayTimeZoneRule *t4 = new TimeArrayTimeZoneRule("t4", -3*HOUR, 0, trtimes1, 1, DateTimeRule::STANDARD_TIME);
    TimeArrayTimeZoneRule *t5 = new TimeArrayTimeZoneRule("t5", -4*HOUR, 1*HOUR, trtimes1, 1, DateTimeRule::WALL_TIME);

    // DateTimeRule::operator=/clone
    DateTimeRule dtr0(UCAL_MAY, 31, 2*HOUR, DateTimeRule::WALL_TIME);
    if (dtr0 == *dtr1 || !(dtr0 != *dtr1)) {
        errln("FAIL: DateTimeRule dtr0 is not equal to dtr1, but got wrong result");
    }
    dtr0 = *dtr1;
    if (dtr0 != *dtr1 || !(dtr0 == *dtr1)) {
        errln("FAIL: DateTimeRule dtr0 is equal to dtr1, but got wrong result");
    }
    DateTimeRule *dtr0c = dtr0.clone();
    if (*dtr0c != *dtr1 || !(*dtr0c == *dtr1)) {
        errln("FAIL: DateTimeRule dtr0c is equal to dtr1, but got wrong result");
    }
    delete dtr0c;

    // AnnualTimeZonerule::operator=/clone
    AnnualTimeZoneRule a0("a0", 5*HOUR, 1*HOUR, *dtr1, 1990, AnnualTimeZoneRule::MAX_YEAR);
    if (a0 == *a1 || !(a0 != *a1)) {
        errln("FAIL: AnnualTimeZoneRule a0 is not equal to a1, but got wrong result");
    }
    a0 = *a1;
    if (a0 != *a1 || !(a0 == *a1)) {
        errln("FAIL: AnnualTimeZoneRule a0 is equal to a1, but got wrong result");
    }
    AnnualTimeZoneRule *a0c = a0.clone();
    if (*a0c != *a1 || !(*a0c == *a1)) {
        errln("FAIL: AnnualTimeZoneRule a0c is equal to a1, but got wrong result");
    }
    delete a0c;

    // AnnualTimeZoneRule::getRule
    if (*(a1->getRule()) != *(a2->getRule())) {
        errln("FAIL: The same DateTimeRule must be returned from AnnualTimeZoneRule a1 and a2");
    }

    // AnnualTimeZoneRule::getStartYear
    int32_t startYear = a1->getStartYear();
    if (startYear != 2000) {
        errln((UnicodeString)"FAIL: The start year of AnnualTimeZoneRule a1 must be 2000 - returned: " + startYear);
    }

    // AnnualTimeZoneRule::getEndYear
    int32_t endYear = a1->getEndYear();
    if (endYear != AnnualTimeZoneRule::MAX_YEAR) {
        errln((UnicodeString)"FAIL: The start year of AnnualTimeZoneRule a1 must be MAX_YEAR - returned: " + endYear);
    }
    endYear = a3->getEndYear();
    if (endYear != 2010) {
        errln((UnicodeString)"FAIL: The start year of AnnualTimeZoneRule a3 must be 2010 - returned: " + endYear);
    }

    // AnnualTimeZone::getStartInYear
    UBool b1, b2;
    UDate d1, d2;
    b1 = a1->getStartInYear(2005, -3*HOUR, 0, d1);
    b2 = a3->getStartInYear(2005, -3*HOUR, 0, d2);
    if (!b1 || !b2 || d1 != d2) {
        errln("FAIL: AnnualTimeZoneRule::getStartInYear did not work as expected");
    }
    b2 = a3->getStartInYear(2015, -3*HOUR, 0, d2);
    if (b2) {
        errln("FAIL: AnnualTimeZoneRule::getStartInYear returned TRUE for 2015 which is out of rule range");
    }

    // AnnualTimeZone::getFirstStart
    b1 = a1->getFirstStart(-3*HOUR, 0, d1);
    b2 = a1->getFirstStart(-4*HOUR, 1*HOUR, d2);
    if (!b1 || !b2 || d1 != d2) {
        errln("FAIL: The same start time should be returned by getFirstStart");
    }

    // AnnualTimeZone::getFinalStart
    b1 = a1->getFinalStart(-3*HOUR, 0, d1);
    if (b1) {
        errln("FAIL: getFinalStart returned TRUE for a1");
    }
    b1 = a1->getStartInYear(2010, -3*HOUR, 0, d1);
    b2 = a3->getFinalStart(-3*HOUR, 0, d2);
    if (!b1 || !b2 || d1 != d2) {
        errln("FAIL: Bad date is returned by getFinalStart");
    }

    // AnnualTimeZone::getNextStart / getPreviousStart
    b1 = a1->getNextStart(time1, -3*HOUR, 0, FALSE, d1);
    if (!b1) {
        errln("FAIL: getNextStart returned FALSE for ai");
    } else {
        b2 = a1->getPreviousStart(d1, -3*HOUR, 0, TRUE, d2);
        if (!b2 || d1 != d2) {
            errln("FAIL: Bad Date is returned by getPreviousStart");
        }
    }
    b1 = a3->getNextStart(time2, -3*HOUR, 0, FALSE, d1);
    if (b1) {
        dataerrln("FAIL: getNextStart must return FALSE when no start time is available after the base time");
    }
    b1 = a3->getFinalStart(-3*HOUR, 0, d1);
    b2 = a3->getPreviousStart(time2, -3*HOUR, 0, FALSE, d2);
    if (!b1 || !b2 || d1 != d2) {
        dataerrln("FAIL: getPreviousStart does not match with getFinalStart after the end year");
    }

    // AnnualTimeZone::isEquavalentTo
    if (!a1->isEquivalentTo(*a2)) {
        errln("FAIL: AnnualTimeZoneRule a1 is equivalent to a2, but returned FALSE");
    }
    if (a1->isEquivalentTo(*a3)) {
        errln("FAIL: AnnualTimeZoneRule a1 is not equivalent to a3, but returned TRUE");
    }
    if (!a1->isEquivalentTo(*a1)) {
        errln("FAIL: AnnualTimeZoneRule a1 is equivalent to itself, but returned FALSE");
    }
    if (a1->isEquivalentTo(*t1)) {
        errln("FAIL: AnnualTimeZoneRule is not equivalent to TimeArrayTimeZoneRule, but returned TRUE");
    }

    // InitialTimezoneRule::operator=/clone
    InitialTimeZoneRule i0("i0", 10*HOUR, 0);
    if (i0 == *i1 || !(i0 != *i1)) {
        errln("FAIL: InitialTimeZoneRule i0 is not equal to i1, but got wrong result");
    }
    i0 = *i1;
    if (i0 != *i1 || !(i0 == *i1)) {
        errln("FAIL: InitialTimeZoneRule i0 is equal to i1, but got wrong result");
    }
    InitialTimeZoneRule *i0c = i0.clone();
    if (*i0c != *i1 || !(*i0c == *i1)) {
        errln("FAIL: InitialTimeZoneRule i0c is equal to i1, but got wrong result");
    }
    delete i0c;

    // InitialTimeZoneRule::isEquivalentRule
    if (!i1->isEquivalentTo(*i2)) {
        errln("FAIL: InitialTimeZoneRule i1 is equivalent to i2, but returned FALSE");
    }
    if (i1->isEquivalentTo(*i3)) {
        errln("FAIL: InitialTimeZoneRule i1 is not equivalent to i3, but returned TRUE");
    }
    if (i1->isEquivalentTo(*a1)) {
        errln("FAIL: An InitialTimeZoneRule is not equivalent to an AnnualTimeZoneRule, but returned TRUE");
    }

    // InitialTimeZoneRule::getFirstStart/getFinalStart/getNextStart/getPreviousStart
    b1 = i1->getFirstStart(0, 0, d1);
    if (b1) {
        errln("FAIL: InitialTimeZone::getFirstStart returned TRUE");
    }
    b1 = i1->getFinalStart(0, 0, d1);
    if (b1) {
        errln("FAIL: InitialTimeZone::getFinalStart returned TRUE");
    }
    b1 = i1->getNextStart(time1, 0, 0, FALSE, d1);
    if (b1) {
        errln("FAIL: InitialTimeZone::getNextStart returned TRUE");
    }
    b1 = i1->getPreviousStart(time1, 0, 0, FALSE, d1);
    if (b1) {
        errln("FAIL: InitialTimeZone::getPreviousStart returned TRUE");
    }

    // TimeArrayTimeZoneRule::operator=/clone
    TimeArrayTimeZoneRule t0("t0", 4*HOUR, 0, trtimes1, 1, DateTimeRule::UTC_TIME);
    if (t0 == *t1 || !(t0 != *t1)) {
        errln("FAIL: TimeArrayTimeZoneRule t0 is not equal to t1, but got wrong result");
    }
    t0 = *t1;
    if (t0 != *t1 || !(t0 == *t1)) {
        errln("FAIL: TimeArrayTimeZoneRule t0 is equal to t1, but got wrong result");
    }
    TimeArrayTimeZoneRule *t0c = t0.clone();
    if (*t0c != *t1 || !(*t0c == *t1)) {
        errln("FAIL: TimeArrayTimeZoneRule t0c is equal to t1, but got wrong result");
    }
    delete t0c;

    // TimeArrayTimeZoneRule::countStartTimes
    if (t1->countStartTimes() != 1) {
        errln("FAIL: Bad start time count is returned by TimeArrayTimeZoneRule::countStartTimes");
    }

    // TimeArrayTimeZoneRule::getStartTimeAt
    b1 = t1->getStartTimeAt(-1, d1);
    if (b1) {
        errln("FAIL: TimeArrayTimeZoneRule::getStartTimeAt returned TRUE for index -1");
    }
    b1 = t1->getStartTimeAt(0, d1);
    if (!b1 || d1 != trtimes1[0]) {
        errln("FAIL: TimeArrayTimeZoneRule::getStartTimeAt returned incorrect result for index 0");
    }
    b1 = t1->getStartTimeAt(1, d1);
    if (b1) {
        errln("FAIL: TimeArrayTimeZoneRule::getStartTimeAt returned TRUE for index 1");
    }

    // TimeArrayTimeZoneRule::getTimeType
    if (t1->getTimeType() != DateTimeRule::UTC_TIME) {
        errln("FAIL: TimeArrayTimeZoneRule t1 uses UTC_TIME, but different type is returned");
    }
    if (t4->getTimeType() != DateTimeRule::STANDARD_TIME) {
        errln("FAIL: TimeArrayTimeZoneRule t4 uses STANDARD_TIME, but different type is returned");
    }
    if (t5->getTimeType() != DateTimeRule::WALL_TIME) {
        errln("FAIL: TimeArrayTimeZoneRule t5 uses WALL_TIME, but different type is returned");
    }

    // TimeArrayTimeZoneRule::getFirstStart/getFinalStart
    b1 = t1->getFirstStart(0, 0, d1);
    if (!b1 || d1 != trtimes1[0]) {
        errln("FAIL: Bad first start time returned from TimeArrayTimeZoneRule t1");
    }
    b1 = t1->getFinalStart(0, 0, d1);
    if (!b1 || d1 != trtimes1[0]) {
        errln("FAIL: Bad final start time returned from TimeArrayTimeZoneRule t1");
    }
    b1 = t4->getFirstStart(-4*HOUR, 1*HOUR, d1);
    if (!b1 || d1 != (trtimes1[0] + 4*HOUR)) {
        errln("FAIL: Bad first start time returned from TimeArrayTimeZoneRule t4");
    }
    b1 = t5->getFirstStart(-4*HOUR, 1*HOUR, d1);
    if (!b1 || d1 != (trtimes1[0] + 3*HOUR)) {
        errln("FAIL: Bad first start time returned from TimeArrayTimeZoneRule t5");
    }

    // TimeArrayTimeZoneRule::getNextStart/getPreviousStart
    b1 = t3->getNextStart(time1, -3*HOUR, 1*HOUR, FALSE, d1);
    if (b1) {
        dataerrln("FAIL: getNextStart returned TRUE after the final transition for t3");
    }
    b1 = t3->getPreviousStart(time1, -3*HOUR, 1*HOUR, FALSE, d1);
    if (!b1 || d1 != trtimes2[1]) {
        dataerrln("FAIL: Bad start time returned by getPreviousStart for t3");
    } else {
        b2 = t3->getPreviousStart(d1, -3*HOUR, 1*HOUR, FALSE, d2);
        if (!b2 || d2 != trtimes2[0]) {
            errln("FAIL: Bad start time returned by getPreviousStart for t3");
        }
    }
    b1 = t3->getPreviousStart(time3, -3*HOUR, 1*HOUR, FALSE, d1); //time3 - year 1950, no result expected
    if (b1) {
        errln("FAIL: getPreviousStart returned TRUE before the first transition for t3");
    }

    // TimeArrayTimeZoneRule::isEquivalentTo
    if (!t1->isEquivalentTo(*t2)) {
        errln("FAIL: TimeArrayTimeZoneRule t1 is equivalent to t2, but returned FALSE");
    }
    if (t1->isEquivalentTo(*t3)) {
        errln("FAIL: TimeArrayTimeZoneRule t1 is not equivalent to t3, but returned TRUE");
    }
    if (t1->isEquivalentTo(*t4)) {
        errln("FAIL: TimeArrayTimeZoneRule t1 is not equivalent to t4, but returned TRUE");
    }
    if (t1->isEquivalentTo(*a1)) {
        errln("FAIL: TimeArrayTimeZoneRule is not equivalent to AnnualTimeZoneRule, but returned TRUE");
    }

    delete dtr1;
    delete dtr2;
    delete dtr3;
    delete dtr4;
    delete a1;
    delete a2;
    delete a3;
    delete i1;
    delete i2;
    delete i3;
    delete t1;
    delete t2;
    delete t3;
    delete t4;
    delete t5;
}

/*
 * API coverage test for BasicTimeZone APIs in SimpleTimeZone
 */
void
TimeZoneRuleTest::TestSimpleTimeZoneCoverage(void) {
    UDate time1 = getUTCMillis(1990, UCAL_JUNE, 1);
    UDate time2 = getUTCMillis(2000, UCAL_JUNE, 1);

    TimeZoneTransition tzt1, tzt2;
    UBool avail1, avail2;
    UErrorCode status = U_ZERO_ERROR;
    const TimeZoneRule *trrules[2];
    const InitialTimeZoneRule *ir = NULL;
    int32_t numTzRules;

    // BasicTimeZone API implementation in SimpleTimeZone
    SimpleTimeZone *stz1 = new SimpleTimeZone(-5*HOUR, "GMT-5");

    avail1 = stz1->getNextTransition(time1, FALSE, tzt1);
    if (avail1) {
        errln("FAIL: No transition must be returned by getNextTranstion for SimpleTimeZone with no DST rule");
    }
    avail1 = stz1->getPreviousTransition(time1, FALSE, tzt1);
    if (avail1) {
        errln("FAIL: No transition must be returned by getPreviousTransition  for SimpleTimeZone with no DST rule");
    }

    numTzRules = stz1->countTransitionRules(status);
    if (U_FAILURE(status)) {
        errln("FAIL: countTransitionRules failed");
    }
    if (numTzRules != 0) {
        errln((UnicodeString)"FAIL: countTransitionRules returned " + numTzRules);
    }
    numTzRules = 2;
    stz1->getTimeZoneRules(ir, trrules, numTzRules, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getTimeZoneRules failed");
    }
    if (numTzRules != 0) {
        errln("FAIL: Incorrect transition rule count");
    }
    if (ir == NULL || ir->getRawOffset() != stz1->getRawOffset()) {
        errln("FAIL: Bad initial time zone rule");
    }

    // Set DST rule
    stz1->setStartRule(UCAL_MARCH, 11, 2*HOUR, status); // March 11
    stz1->setEndRule(UCAL_NOVEMBER, 1, UCAL_SUNDAY, 2*HOUR, status); // First Sunday in November
    if (U_FAILURE(status)) {
        errln("FAIL: Failed to set DST rules in a SimpleTimeZone");
    }

    avail1 = stz1->getNextTransition(time1, FALSE, tzt1);
    if (!avail1) {
        errln("FAIL: Non-null transition must be returned by getNextTranstion for SimpleTimeZone with a DST rule");
    }
    avail1 = stz1->getPreviousTransition(time1, FALSE, tzt1);
    if (!avail1) {
        errln("FAIL: Non-null transition must be returned by getPreviousTransition  for SimpleTimeZone with a DST rule");
    }

    numTzRules = stz1->countTransitionRules(status);
    if (U_FAILURE(status)) {
        errln("FAIL: countTransitionRules failed");
    }
    if (numTzRules != 2) {
        errln((UnicodeString)"FAIL: countTransitionRules returned " + numTzRules);
    }

    numTzRules = 2;
    trrules[0] = NULL;
    trrules[1] = NULL;
    stz1->getTimeZoneRules(ir, trrules, numTzRules, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getTimeZoneRules failed");
    }
    if (numTzRules != 2) {
        errln("FAIL: Incorrect transition rule count");
    }
    if (ir == NULL || ir->getRawOffset() != stz1->getRawOffset()) {
        errln("FAIL: Bad initial time zone rule");
    }
    if (trrules[0] == NULL || trrules[0]->getRawOffset() != stz1->getRawOffset()) {
        errln("FAIL: Bad transition rule 0");
    }
    if (trrules[1] == NULL || trrules[1]->getRawOffset() != stz1->getRawOffset()) {
        errln("FAIL: Bad transition rule 1");
    }

    // Set DST start year
    stz1->setStartYear(2007);
    avail1 = stz1->getPreviousTransition(time1, FALSE, tzt1);
    if (avail1) {
        errln("FAIL: No transition must be returned before 1990");
    }
    avail1 = stz1->getNextTransition(time1, FALSE, tzt1); // transition after 1990-06-01
    avail2 = stz1->getNextTransition(time2, FALSE, tzt2); // transition after 2000-06-01
    if (!avail1 || !avail2 || tzt1 != tzt2) {
        errln("FAIL: Bad transition returned by SimpleTimeZone::getNextTransition");
    }
    delete stz1;
}

/*
 * API coverage test for VTimeZone
 */
void
TimeZoneRuleTest::TestVTimeZoneCoverage(void) {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString TZID("Europe/Moscow");

    BasicTimeZone *otz = (BasicTimeZone*)TimeZone::createTimeZone(TZID);
    VTimeZone *vtz = VTimeZone::createVTimeZoneByID(TZID);

    // getOffset(era, year, month, day, dayOfWeek, milliseconds, ec)
    int32_t offset1 = otz->getOffset(GregorianCalendar::AD, 2007, UCAL_JULY, 1, UCAL_SUNDAY, 0, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(7 args) failed for otz");
    }
    int32_t offset2 = vtz->getOffset(GregorianCalendar::AD, 2007, UCAL_JULY, 1, UCAL_SUNDAY, 0, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(7 args) failed for vtz");
    }
    if (offset1 != offset2) {
        errln("FAIL: getOffset(7 args) returned different results in VTimeZone and OlsonTimeZone");
    }

    // getOffset(era, year, month, day, dayOfWeek, milliseconds, monthLength, ec)
    offset1 = otz->getOffset(GregorianCalendar::AD, 2007, UCAL_JULY, 1, UCAL_SUNDAY, 0, 31, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(8 args) failed for otz");
    }
    offset2 = vtz->getOffset(GregorianCalendar::AD, 2007, UCAL_JULY, 1, UCAL_SUNDAY, 0, 31, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(8 args) failed for vtz");
    }
    if (offset1 != offset2) {
        errln("FAIL: getOffset(8 args) returned different results in VTimeZone and OlsonTimeZone");
    }


    // getOffset(date, local, rawOffset, dstOffset, ec)
    UDate t = Calendar::getNow();
    int32_t rawOffset1, dstSavings1;
    int32_t rawOffset2, dstSavings2;

    otz->getOffset(t, FALSE, rawOffset1, dstSavings1, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(5 args) failed for otz");
    }
    vtz->getOffset(t, FALSE, rawOffset2, dstSavings2, status);
    if (U_FAILURE(status)) {
        errln("FAIL: getOffset(5 args) failed for vtz");
    }
    if (rawOffset1 != rawOffset2 || dstSavings1 != dstSavings2) {
        errln("FAIL: getOffset(long,boolean,int[]) returned different results in VTimeZone and OlsonTimeZone");
    }

    // getRawOffset
    if (otz->getRawOffset() != vtz->getRawOffset()) {
        errln("FAIL: getRawOffset returned different results in VTimeZone and OlsonTimeZone");
    }

    // inDaylightTime
    UBool inDst1, inDst2;
    inDst1 = otz->inDaylightTime(t, status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: inDaylightTime failed for otz: %s", u_errorName(status));
    }
    inDst2 = vtz->inDaylightTime(t, status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: inDaylightTime failed for vtz: %s", u_errorName(status));
    }
    if (inDst1 != inDst2) {
        errln("FAIL: inDaylightTime returned different results in VTimeZone and OlsonTimeZone");
    }

    // useDaylightTime
    if (otz->useDaylightTime() != vtz->useDaylightTime()) {
        errln("FAIL: useDaylightTime returned different results in VTimeZone and OlsonTimeZone");
    }

    // setRawOffset
    const int32_t RAW = -10*HOUR;
    VTimeZone *tmpvtz = (VTimeZone*)vtz->clone();
    tmpvtz->setRawOffset(RAW);
    if (tmpvtz->getRawOffset() != RAW) {
        logln("setRawOffset is implemented in VTimeZone");
    }

    // hasSameRules
    UBool bSame = otz->hasSameRules(*vtz);
    logln((UnicodeString)"OlsonTimeZone::hasSameRules(VTimeZone) should return FALSE always for now - actual: " + bSame);

    // getTZURL/setTZURL
    UnicodeString TZURL("http://icu-project.org/timezone");
    UnicodeString url;
    if (vtz->getTZURL(url)) {
        errln("FAIL: getTZURL returned TRUE");
    }
    vtz->setTZURL(TZURL);
    if (!vtz->getTZURL(url) || url != TZURL) {
        errln("FAIL: URL returned by getTZURL does not match the one set by setTZURL");
    }

    // getLastModified/setLastModified
    UDate lastmod;
    if (vtz->getLastModified(lastmod)) {
        errln("FAIL: getLastModified returned TRUE");
    }
    vtz->setLastModified(t);
    if (!vtz->getLastModified(lastmod) || lastmod != t) {
        errln("FAIL: Date returned by getLastModified does not match the one set by setLastModified");
    }

    // getNextTransition/getPreviousTransition
    UDate base = getUTCMillis(2007, UCAL_JULY, 1);
    TimeZoneTransition tzt1, tzt2;
    UBool btr1 = otz->getNextTransition(base, TRUE, tzt1);
    UBool btr2 = vtz->getNextTransition(base, TRUE, tzt2);
    if (!btr1 || !btr2 || tzt1 != tzt2) {
        dataerrln("FAIL: getNextTransition returned different results in VTimeZone and OlsonTimeZone");
    }
    btr1 = otz->getPreviousTransition(base, FALSE, tzt1);
    btr2 = vtz->getPreviousTransition(base, FALSE, tzt2);
    if (!btr1 || !btr2 || tzt1 != tzt2) {
        dataerrln("FAIL: getPreviousTransition returned different results in VTimeZone and OlsonTimeZone");
    }

    // TimeZoneTransition constructor/clone
    TimeZoneTransition *tzt1c = tzt1.clone();
    if (*tzt1c != tzt1 || !(*tzt1c == tzt1)) {
        errln("FAIL: TimeZoneTransition tzt1c is equal to tzt1, but got wrong result");
    }
    delete tzt1c;
    TimeZoneTransition tzt3(tzt1);
    if (tzt3 != tzt1 || !(tzt3 == tzt1)) {
        errln("FAIL: TimeZoneTransition tzt3 is equal to tzt1, but got wrong result");
    }

    // hasEquivalentTransitions
    UDate time1 = getUTCMillis(1950, UCAL_JANUARY, 1);
    UDate time2 = getUTCMillis(2020, UCAL_JANUARY, 1);
    UBool equiv = vtz->hasEquivalentTransitions(*otz, time1, time2, FALSE, status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: hasEquivalentTransitions failed for vtz/otz: %s", u_errorName(status));
    }
    if (!equiv) {
        dataerrln("FAIL: hasEquivalentTransitons returned false for the same time zone");
    }

    // operator=/operator==/operator!=
    VTimeZone *vtz1 = VTimeZone::createVTimeZoneByID("America/Los_Angeles");
    if (*vtz1 == *vtz || !(*vtz1 != *vtz)) {
        errln("FAIL: VTimeZone vtz1 is not equal to vtz, but got wrong result");
    }
    *vtz1 = *vtz;
    if (*vtz1 != *vtz || !(*vtz1 == *vtz)) {
        errln("FAIL: VTimeZone vtz1 is equal to vtz, but got wrong result");
    }

    // Creation from BasicTimeZone
    //
    status = U_ZERO_ERROR;
    VTimeZone *vtzFromBasic = NULL;
    SimpleTimeZone *simpleTZ = new SimpleTimeZone(28800000, "Asia/Singapore");
    simpleTZ->setStartYear(1970);
    simpleTZ->setStartRule(0,  // month
                          1,  // day of week
                          0,  // time
                          status);
    simpleTZ->setEndRule(1, 1, 0, status);
    if (U_FAILURE(status)) {
        errln("File %s, line %d, failed with status = %s", __FILE__, __LINE__, u_errorName(status));
        goto end_basic_tz_test;
    }
    vtzFromBasic = VTimeZone::createVTimeZoneFromBasicTimeZone(*simpleTZ, status);
    if (U_FAILURE(status) || vtzFromBasic == NULL) {
        dataerrln("File %s, line %d, failed with status = %s", __FILE__, __LINE__, u_errorName(status));
        goto end_basic_tz_test;
    }

    // delete the source time zone, to make sure there are no dependencies on it.
    delete simpleTZ;

    // Create another simple time zone w the same rules, and check that it is the
    // same as the test VTimeZone created above.
    {
        SimpleTimeZone simpleTZ2(28800000, "Asia/Singapore");
        simpleTZ2.setStartYear(1970);
        simpleTZ2.setStartRule(0,  // month
                              1,  // day of week
                              0,  // time
                              status);
        simpleTZ2.setEndRule(1, 1, 0, status);
        if (U_FAILURE(status)) {
            errln("File %s, line %d, failed with status = %s", __FILE__, __LINE__, u_errorName(status));
            goto end_basic_tz_test;
        }
        if (vtzFromBasic->hasSameRules(simpleTZ2) == FALSE) {
            errln("File %s, line %d, failed hasSameRules() ", __FILE__, __LINE__);
            goto end_basic_tz_test;
        }
    }
end_basic_tz_test:
    delete vtzFromBasic;

    delete otz;
    delete vtz;
    delete tmpvtz;
    delete vtz1;
}


void
TimeZoneRuleTest::TestVTimeZoneParse(void) {
    UErrorCode status = U_ZERO_ERROR;

    // Trying to create VTimeZone from empty data
    UnicodeString emptyData;
    VTimeZone *empty = VTimeZone::createVTimeZone(emptyData, status);
    if (U_SUCCESS(status) || empty != NULL) {
        delete empty;
        errln("FAIL: Non-null VTimeZone is returned for empty VTIMEZONE data");
    }
    status = U_ZERO_ERROR;

    // Create VTimeZone for Asia/Tokyo
    UnicodeString asiaTokyoID("Asia/Tokyo");
    static const UChar asiaTokyo[] = {
        /* "BEGIN:VTIMEZONE\x0D\x0A" */
        0x42,0x45,0x47,0x49,0x4E,0x3A,0x56,0x54,0x49,0x4D,0x45,0x5A,0x4F,0x4E,0x45,0x0D,0x0A,
        /* "TZID:Asia\x0D\x0A" */
        0x54,0x5A,0x49,0x44,0x3A,0x41,0x73,0x69,0x61,0x0D,0x0A,
        /* "\x09/Tokyo\x0D\x0A" */
        0x09,0x2F,0x54,0x6F,0x6B,0x79,0x6F,0x0D,0x0A,
        /* "BEGIN:STANDARD\x0D\x0A" */
        0x42,0x45,0x47,0x49,0x4E,0x3A,0x53,0x54,0x41,0x4E,0x44,0x41,0x52,0x44,0x0D,0x0A,
        /* "TZOFFSETFROM:+0900\x0D\x0A" */
        0x54,0x5A,0x4F,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4F,0x4D,0x3A,0x2B,0x30,0x39,0x30,0x30,0x0D,0x0A,
        /* "TZOFFSETTO:+0900\x0D\x0A" */
        0x54,0x5A,0x4F,0x46,0x46,0x53,0x45,0x54,0x54,0x4F,0x3A,0x2B,0x30,0x39,0x30,0x30,0x0D,0x0A,
        /* "TZNAME:JST\x0D\x0A" */
        0x54,0x5A,0x4E,0x41,0x4D,0x45,0x3A,0x4A,0x53,0x54,0x0D,0x0A,
        /* "DTSTART:19700101\x0D\x0A" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3A,0x31,0x39,0x37,0x30,0x30,0x31,0x30,0x31,0x0D,0x0A,
        /* " T000000\x0D\x0A" */
        0x20,0x54,0x30,0x30,0x30,0x30,0x30,0x30,0x0D,0x0A,
        /* "END:STANDARD\x0D\x0A" */
        0x45,0x4E,0x44,0x3A,0x53,0x54,0x41,0x4E,0x44,0x41,0x52,0x44,0x0D,0x0A,
        /* "END:VTIMEZONE" */
        0x45,0x4E,0x44,0x3A,0x56,0x54,0x49,0x4D,0x45,0x5A,0x4F,0x4E,0x45,
        0
    };
    VTimeZone *tokyo = VTimeZone::createVTimeZone(asiaTokyo, status);
    if (U_FAILURE(status) || tokyo == NULL) {
        errln("FAIL: Failed to create a VTimeZone tokyo");
    } else {
        // Check ID
        UnicodeString tzid;
        tokyo->getID(tzid);
        if (tzid != asiaTokyoID) {
            errln((UnicodeString)"FAIL: Invalid TZID: " + tzid);
        }
        // Make sure offsets are correct
        int32_t rawOffset, dstSavings;
        tokyo->getOffset(Calendar::getNow(), FALSE, rawOffset, dstSavings, status);
        if (U_FAILURE(status)) {
            errln("FAIL: getOffset failed for tokyo");
        }
        if (rawOffset != 9*HOUR || dstSavings != 0) {
            errln("FAIL: Bad offsets returned by a VTimeZone created for Tokyo");
        }
    }
    delete tokyo;

        // Create VTimeZone from VTIMEZONE data
    static const UChar fooData[] = {
        /* "BEGIN:VCALENDAR\x0D\x0A" */
        0x42,0x45,0x47,0x49,0x4E,0x3A,0x56,0x43,0x41,0x4C,0x45,0x4E,0x44,0x41,0x52,0x0D,0x0A,
        /* "BEGIN:VTIMEZONE\x0D\x0A" */
        0x42,0x45,0x47,0x49,0x4E,0x3A,0x56,0x54,0x49,0x4D,0x45,0x5A,0x4F,0x4E,0x45,0x0D,0x0A,
        /* "TZID:FOO\x0D\x0A" */
        0x54,0x5A,0x49,0x44,0x3A,0x46,0x4F,0x4F,0x0D,0x0A,
        /* "BEGIN:STANDARD\x0D\x0A" */
        0x42,0x45,0x47,0x49,0x4E,0x3A,0x53,0x54,0x41,0x4E,0x44,0x41,0x52,0x44,0x0D,0x0A,
        /* "TZOFFSETFROM:-0700\x0D\x0A" */
        0x54,0x5A,0x4F,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4F,0x4D,0x3A,0x2D,0x30,0x37,0x30,0x30,0x0D,0x0A,
        /* "TZOFFSETTO:-0800\x0D\x0A" */
        0x54,0x5A,0x4F,0x46,0x46,0x53,0x45,0x54,0x54,0x4F,0x3A,0x2D,0x30,0x38,0x30,0x30,0x0D,0x0A,
        /* "TZNAME:FST\x0D\x0A" */
        0x54,0x5A,0x4E,0x41,0x4D,0x45,0x3A,0x46,0x53,0x54,0x0D,0x0A,
        /* "DTSTART:20071010T010000\x0D\x0A" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3A,0x32,0x30,0x30,0x37,0x31,0x30,0x31,0x30,0x54,0x30,0x31,0x30,0x30,0x30,0x30,0x0D,0x0A,
        /* "RRULE:FREQ=YEARLY;BYDAY=WE;BYMONTHDAY=10,11,12,13,14,15,16;BYMONTH=10\x0D\x0A" */
        0x52,0x52,0x55,0x4C,0x45,0x3A,0x46,0x52,0x45,0x51,0x3D,0x59,0x45,0x41,0x52,0x4C,0x59,0x3B,0x42,0x59,0x44,0x41,0x59,0x3D,0x57,0x45,0x3B,0x42,0x59,0x4D,0x4F,0x4E,0x54,0x48,0x44,0x41,0x59,0x3D,0x31,0x30,0x2C,0x31,0x31,0x2C,0x31,0x32,0x2C,0x31,0x33,0x2C,0x31,0x34,0x2C,0x31,0x35,0x2C,0x31,0x36,0x3B,0x42,0x59,0x4D,0x4F,0x4E,0x54,0x48,0x3D,0x31,0x30,0x0D,0x0A,
        /* "END:STANDARD\x0D\x0A" */
        0x45,0x4E,0x44,0x3A,0x53,0x54,0x41,0x4E,0x44,0x41,0x52,0x44,0x0D,0x0A,
        /* "BEGIN:DAYLIGHT\x0D\x0A" */
        0x42,0x45,0x47,0x49,0x4E,0x3A,0x44,0x41,0x59,0x4C,0x49,0x47,0x48,0x54,0x0D,0x0A,
        /* "TZOFFSETFROM:-0800\x0D\x0A" */
        0x54,0x5A,0x4F,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4F,0x4D,0x3A,0x2D,0x30,0x38,0x30,0x30,0x0D,0x0A,
        /* "TZOFFSETTO:-0700\x0D\x0A" */
        0x54,0x5A,0x4F,0x46,0x46,0x53,0x45,0x54,0x54,0x4F,0x3A,0x2D,0x30,0x37,0x30,0x30,0x0D,0x0A,
        /* "TZNAME:FDT\x0D\x0A" */
        0x54,0x5A,0x4E,0x41,0x4D,0x45,0x3A,0x46,0x44,0x54,0x0D,0x0A,
        /* "DTSTART:20070415T010000\x0D\x0A" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3A,0x32,0x30,0x30,0x37,0x30,0x34,0x31,0x35,0x54,0x30,0x31,0x30,0x30,0x30,0x30,0x0D,0x0A,
        /* "RRULE:FREQ=YEARLY;BYMONTHDAY=15;BYMONTH=4\x0D\x0A" */
        0x52,0x52,0x55,0x4C,0x45,0x3A,0x46,0x52,0x45,0x51,0x3D,0x59,0x45,0x41,0x52,0x4C,0x59,0x3B,0x42,0x59,0x4D,0x4F,0x4E,0x54,0x48,0x44,0x41,0x59,0x3D,0x31,0x35,0x3B,0x42,0x59,0x4D,0x4F,0x4E,0x54,0x48,0x3D,0x34,0x0D,0x0A,
        /* "END:DAYLIGHT\x0D\x0A" */
        0x45,0x4E,0x44,0x3A,0x44,0x41,0x59,0x4C,0x49,0x47,0x48,0x54,0x0D,0x0A,
        /* "END:VTIMEZONE\x0D\x0A" */
        0x45,0x4E,0x44,0x3A,0x56,0x54,0x49,0x4D,0x45,0x5A,0x4F,0x4E,0x45,0x0D,0x0A,
        /* "END:VCALENDAR" */
        0x45,0x4E,0x44,0x3A,0x56,0x43,0x41,0x4C,0x45,0x4E,0x44,0x41,0x52,
        0
    };

    VTimeZone *foo = VTimeZone::createVTimeZone(fooData, status);
    if (U_FAILURE(status) || foo == NULL) {
        errln("FAIL: Failed to create a VTimeZone foo");
    } else {
        // Write VTIMEZONE data
        UnicodeString fooData2;
        foo->write(getUTCMillis(2005, UCAL_JANUARY, 1), fooData2, status);
        if (U_FAILURE(status)) {
            errln("FAIL: Failed to write VTIMEZONE data for foo");
        }
        logln(fooData2);
    }
    delete foo;
}

void
TimeZoneRuleTest::TestT6216(void) {
    // Test case in #6216
    static const UChar tokyoTZ[] = {
        /* "BEGIN:VCALENDAR\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x56,0x43,0x41,0x4c,0x45,0x4e,0x44,0x41,0x52,0x0d,0x0a,
        /* "VERSION:2.0\r\n" */
        0x56,0x45,0x52,0x53,0x49,0x4f,0x4e,0x3a,0x32,0x2e,0x30,0x0d,0x0a,
        /* "PRODID:-//PYVOBJECT//NONSGML Version 1//EN\r\n" */
        0x50,0x52,0x4f,0x44,0x49,0x44,0x3a,0x2d,0x2f,0x2f,0x50,0x59,0x56,0x4f,0x42,0x4a,0x45,0x43,0x54,0x2f,0x2f,0x4e,0x4f,0x4e,0x53,0x47,0x4d,0x4c,0x20,0x56,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x31,0x2f,0x2f,0x45,0x4e,0x0d,0x0a,
        /* "BEGIN:VTIMEZONE\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x56,0x54,0x49,0x4d,0x45,0x5a,0x4f,0x4e,0x45,0x0d,0x0a,
        /* "TZID:Asia/Tokyo\r\n" */
        0x54,0x5a,0x49,0x44,0x3a,0x41,0x73,0x69,0x61,0x2f,0x54,0x6f,0x6b,0x79,0x6f,0x0d,0x0a,
        /* "BEGIN:STANDARD\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "DTSTART:20000101T000000\r\n" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3a,0x32,0x30,0x30,0x30,0x30,0x31,0x30,0x31,0x54,0x30,0x30,0x30,0x30,0x30,0x30,0x0d,0x0a,
        /* "RRULE:FREQ=YEARLY;BYMONTH=1\r\n" */
        0x52,0x52,0x55,0x4c,0x45,0x3a,0x46,0x52,0x45,0x51,0x3d,0x59,0x45,0x41,0x52,0x4c,0x59,0x3b,0x42,0x59,0x4d,0x4f,0x4e,0x54,0x48,0x3d,0x31,0x0d,0x0a,
        /* "TZNAME:Asia/Tokyo\r\n" */
        0x54,0x5a,0x4e,0x41,0x4d,0x45,0x3a,0x41,0x73,0x69,0x61,0x2f,0x54,0x6f,0x6b,0x79,0x6f,0x0d,0x0a,
        /* "TZOFFSETFROM:+0900\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4f,0x4d,0x3a,0x2b,0x30,0x39,0x30,0x30,0x0d,0x0a,
        /* "TZOFFSETTO:+0900\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x54,0x4f,0x3a,0x2b,0x30,0x39,0x30,0x30,0x0d,0x0a,
        /* "END:STANDARD\r\n" */
        0x45,0x4e,0x44,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "END:VTIMEZONE\r\n" */
        0x45,0x4e,0x44,0x3a,0x56,0x54,0x49,0x4d,0x45,0x5a,0x4f,0x4e,0x45,0x0d,0x0a,
        /* "END:VCALENDAR" */
        0x45,0x4e,0x44,0x3a,0x56,0x43,0x41,0x4c,0x45,0x4e,0x44,0x41,0x52,0x0d,0x0a,
        0
    };
    // Single final rule, overlapping with another
    static const UChar finalOverlap[] = {
        /* "BEGIN:VCALENDAR\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x56,0x43,0x41,0x4c,0x45,0x4e,0x44,0x41,0x52,0x0d,0x0a,
        /* "BEGIN:VTIMEZONE\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x56,0x54,0x49,0x4d,0x45,0x5a,0x4f,0x4e,0x45,0x0d,0x0a,
        /* "TZID:FinalOverlap\r\n" */
        0x54,0x5a,0x49,0x44,0x3a,0x46,0x69,0x6e,0x61,0x6c,0x4f,0x76,0x65,0x72,0x6c,0x61,0x70,0x0d,0x0a,
        /* "BEGIN:STANDARD\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "TZOFFSETFROM:-0200\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4f,0x4d,0x3a,0x2d,0x30,0x32,0x30,0x30,0x0d,0x0a,
        /* "TZOFFSETTO:-0300\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x54,0x4f,0x3a,0x2d,0x30,0x33,0x30,0x30,0x0d,0x0a,
        /* "TZNAME:STD\r\n" */
        0x54,0x5a,0x4e,0x41,0x4d,0x45,0x3a,0x53,0x54,0x44,0x0d,0x0a,
        /* "DTSTART:20001029T020000\r\n" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3a,0x32,0x30,0x30,0x30,0x31,0x30,0x32,0x39,0x54,0x30,0x32,0x30,0x30,0x30,0x30,0x0d,0x0a,
        /* "RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=10\r\n" */
        0x52,0x52,0x55,0x4c,0x45,0x3a,0x46,0x52,0x45,0x51,0x3d,0x59,0x45,0x41,0x52,0x4c,0x59,0x3b,0x42,0x59,0x44,0x41,0x59,0x3d,0x2d,0x31,0x53,0x55,0x3b,0x42,0x59,0x4d,0x4f,0x4e,0x54,0x48,0x3d,0x31,0x30,0x0d,0x0a,
        /* "END:STANDARD\r\n" */
        0x45,0x4e,0x44,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "BEGIN:DAYLIGHT\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x44,0x41,0x59,0x4c,0x49,0x47,0x48,0x54,0x0d,0x0a,
        /* "TZOFFSETFROM:-0300\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4f,0x4d,0x3a,0x2d,0x30,0x33,0x30,0x30,0x0d,0x0a,
        /* "TZOFFSETTO:-0200\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x54,0x4f,0x3a,0x2d,0x30,0x32,0x30,0x30,0x0d,0x0a,
        /* "TZNAME:DST\r\n" */
        0x54,0x5a,0x4e,0x41,0x4d,0x45,0x3a,0x44,0x53,0x54,0x0d,0x0a,
        /* "DTSTART:19990404T020000\r\n" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3a,0x31,0x39,0x39,0x39,0x30,0x34,0x30,0x34,0x54,0x30,0x32,0x30,0x30,0x30,0x30,0x0d,0x0a,
        /* "RRULE:FREQ=YEARLY;BYDAY=1SU;BYMONTH=4;UNTIL=20050403T040000Z\r\n" */
        0x52,0x52,0x55,0x4c,0x45,0x3a,0x46,0x52,0x45,0x51,0x3d,0x59,0x45,0x41,0x52,0x4c,0x59,0x3b,0x42,0x59,0x44,0x41,0x59,0x3d,0x31,0x53,0x55,0x3b,0x42,0x59,0x4d,0x4f,0x4e,0x54,0x48,0x3d,0x34,0x3b,0x55,0x4e,0x54,0x49,0x4c,0x3d,0x32,0x30,0x30,0x35,0x30,0x34,0x30,0x33,0x54,0x30,0x34,0x30,0x30,0x30,0x30,0x5a,0x0d,0x0a,
        /* "END:DAYLIGHT\r\n" */
        0x45,0x4e,0x44,0x3a,0x44,0x41,0x59,0x4c,0x49,0x47,0x48,0x54,0x0d,0x0a,
        /* "END:VTIMEZONE\r\n" */
        0x45,0x4e,0x44,0x3a,0x56,0x54,0x49,0x4d,0x45,0x5a,0x4f,0x4e,0x45,0x0d,0x0a,
        /* "END:VCALENDAR" */
        0x45,0x4e,0x44,0x3a,0x56,0x43,0x41,0x4c,0x45,0x4e,0x44,0x41,0x52,0x0d,0x0a,
        0
    };
    // Single final rule, no overlapping with another
    static const UChar finalNonOverlap[] = {
        /* "BEGIN:VCALENDAR\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x56,0x43,0x41,0x4c,0x45,0x4e,0x44,0x41,0x52,0x0d,0x0a,
        /* "BEGIN:VTIMEZONE\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x56,0x54,0x49,0x4d,0x45,0x5a,0x4f,0x4e,0x45,0x0d,0x0a,
        /* "TZID:FinalNonOverlap\r\n" */
        0x54,0x5a,0x49,0x44,0x3a,0x46,0x69,0x6e,0x61,0x6c,0x4e,0x6f,0x6e,0x4f,0x76,0x65,0x72,0x6c,0x61,0x70,0x0d,0x0a,
        /* "BEGIN:STANDARD\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "TZOFFSETFROM:-0200\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4f,0x4d,0x3a,0x2d,0x30,0x32,0x30,0x30,0x0d,0x0a,
        /* "TZOFFSETTO:-0300\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x54,0x4f,0x3a,0x2d,0x30,0x33,0x30,0x30,0x0d,0x0a,
        /* "TZNAME:STD\r\n" */
        0x54,0x5a,0x4e,0x41,0x4d,0x45,0x3a,0x53,0x54,0x44,0x0d,0x0a,
        /* "DTSTART:20001029T020000\r\n" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3a,0x32,0x30,0x30,0x30,0x31,0x30,0x32,0x39,0x54,0x30,0x32,0x30,0x30,0x30,0x30,0x0d,0x0a,
        /* "RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=10;UNTIL=20041031T040000Z\r\n" */
        0x52,0x52,0x55,0x4c,0x45,0x3a,0x46,0x52,0x45,0x51,0x3d,0x59,0x45,0x41,0x52,0x4c,0x59,0x3b,0x42,0x59,0x44,0x41,0x59,0x3d,0x2d,0x31,0x53,0x55,0x3b,0x42,0x59,0x4d,0x4f,0x4e,0x54,0x48,0x3d,0x31,0x30,0x3b,0x55,0x4e,0x54,0x49,0x4c,0x3d,0x32,0x30,0x30,0x34,0x31,0x30,0x33,0x31,0x54,0x30,0x34,0x30,0x30,0x30,0x30,0x5a,0x0d,0x0a,
        /* "END:STANDARD\r\n" */
        0x45,0x4e,0x44,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "BEGIN:DAYLIGHT\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x44,0x41,0x59,0x4c,0x49,0x47,0x48,0x54,0x0d,0x0a,
        /* "TZOFFSETFROM:-0300\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4f,0x4d,0x3a,0x2d,0x30,0x33,0x30,0x30,0x0d,0x0a,
        /* "TZOFFSETTO:-0200\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x54,0x4f,0x3a,0x2d,0x30,0x32,0x30,0x30,0x0d,0x0a,
        /* "TZNAME:DST\r\n" */
        0x54,0x5a,0x4e,0x41,0x4d,0x45,0x3a,0x44,0x53,0x54,0x0d,0x0a,
        /* "DTSTART:19990404T020000\r\n" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3a,0x31,0x39,0x39,0x39,0x30,0x34,0x30,0x34,0x54,0x30,0x32,0x30,0x30,0x30,0x30,0x0d,0x0a,
        /* "RRULE:FREQ=YEARLY;BYDAY=1SU;BYMONTH=4;UNTIL=20050403T040000Z\r\n" */
        0x52,0x52,0x55,0x4c,0x45,0x3a,0x46,0x52,0x45,0x51,0x3d,0x59,0x45,0x41,0x52,0x4c,0x59,0x3b,0x42,0x59,0x44,0x41,0x59,0x3d,0x31,0x53,0x55,0x3b,0x42,0x59,0x4d,0x4f,0x4e,0x54,0x48,0x3d,0x34,0x3b,0x55,0x4e,0x54,0x49,0x4c,0x3d,0x32,0x30,0x30,0x35,0x30,0x34,0x30,0x33,0x54,0x30,0x34,0x30,0x30,0x30,0x30,0x5a,0x0d,0x0a,
        /* "END:DAYLIGHT\r\n" */
        0x45,0x4e,0x44,0x3a,0x44,0x41,0x59,0x4c,0x49,0x47,0x48,0x54,0x0d,0x0a,
        /* "BEGIN:STANDARD\r\n" */
        0x42,0x45,0x47,0x49,0x4e,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "TZOFFSETFROM:-0200\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x46,0x52,0x4f,0x4d,0x3a,0x2d,0x30,0x32,0x30,0x30,0x0d,0x0a,
        /* "TZOFFSETTO:-0300\r\n" */
        0x54,0x5a,0x4f,0x46,0x46,0x53,0x45,0x54,0x54,0x4f,0x3a,0x2d,0x30,0x33,0x30,0x30,0x0d,0x0a,
        /* "TZNAME:STDFINAL\r\n" */
        0x54,0x5a,0x4e,0x41,0x4d,0x45,0x3a,0x53,0x54,0x44,0x46,0x49,0x4e,0x41,0x4c,0x0d,0x0a,
        /* "DTSTART:20071028T020000\r\n" */
        0x44,0x54,0x53,0x54,0x41,0x52,0x54,0x3a,0x32,0x30,0x30,0x37,0x31,0x30,0x32,0x38,0x54,0x30,0x32,0x30,0x30,0x30,0x30,0x0d,0x0a,
        /* "RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=10\r\n" */
        0x52,0x52,0x55,0x4c,0x45,0x3a,0x46,0x52,0x45,0x51,0x3d,0x59,0x45,0x41,0x52,0x4c,0x59,0x3b,0x42,0x59,0x44,0x41,0x59,0x3d,0x2d,0x31,0x53,0x55,0x3b,0x42,0x59,0x4d,0x4f,0x4e,0x54,0x48,0x3d,0x31,0x30,0x0d,0x0a,
        /* "END:STANDARD\r\n" */
        0x45,0x4e,0x44,0x3a,0x53,0x54,0x41,0x4e,0x44,0x41,0x52,0x44,0x0d,0x0a,
        /* "END:VTIMEZONE\r\n" */
        0x45,0x4e,0x44,0x3a,0x56,0x54,0x49,0x4d,0x45,0x5a,0x4f,0x4e,0x45,0x0d,0x0a,
        /* "END:VCALENDAR" */
        0x45,0x4e,0x44,0x3a,0x56,0x43,0x41,0x4c,0x45,0x4e,0x44,0x41,0x52,0x0d,0x0a,
        0
    };

    static const int32_t TestDates[][3] = {
        {1995, UCAL_JANUARY, 1},
        {1995, UCAL_JULY, 1},
        {2000, UCAL_JANUARY, 1},
        {2000, UCAL_JULY, 1},
        {2005, UCAL_JANUARY, 1},
        {2005, UCAL_JULY, 1},
        {2010, UCAL_JANUARY, 1},
        {2010, UCAL_JULY, 1},
        {0, 0, 0}
    };

    /*static*/ const UnicodeString TestZones[] = {
        UnicodeString(tokyoTZ),
        UnicodeString(finalOverlap),
        UnicodeString(finalNonOverlap),
        UnicodeString()
    };

    int32_t Expected[][8] = {
      //  JAN90      JUL90      JAN00      JUL00      JAN05      JUL05      JAN10      JUL10
        { 32400000,  32400000,  32400000,  32400000,  32400000,  32400000,  32400000,  32400000},
        {-10800000, -10800000,  -7200000,  -7200000, -10800000,  -7200000, -10800000, -10800000},
        {-10800000, -10800000,  -7200000,  -7200000, -10800000,  -7200000, -10800000, -10800000}
    };

    int32_t i, j;

    // Get test times
    UDate times[sizeof(TestDates) / (3 * sizeof(int32_t))];
    int32_t numTimes;

    UErrorCode status = U_ZERO_ERROR;
    TimeZone *utc = TimeZone::createTimeZone("Etc/GMT");
    GregorianCalendar cal(utc, status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: Failed to creat a GregorianCalendar: %s", u_errorName(status));
        return;
    }
    for (i = 0; TestDates[i][2] != 0; i++) {
        cal.clear();
        cal.set(TestDates[i][0], TestDates[i][1], TestDates[i][2]);
        times[i] = cal.getTime(status);
        if (U_FAILURE(status)) {
            errln("FAIL: getTime failed");
            return;
        }
    }
    numTimes = i;

    // Test offset
    for (i = 0; !TestZones[i].isEmpty(); i++) {
        VTimeZone *vtz = VTimeZone::createVTimeZone(TestZones[i], status);
        if (U_FAILURE(status)) {
            errln("FAIL: failed to create VTimeZone");
            continue;
        }
        for (j = 0; j < numTimes; j++) {
            int32_t raw, dst;
            status = U_ZERO_ERROR;
            vtz->getOffset(times[j], FALSE, raw, dst, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"FAIL: getOffset failed for time zone " + i + " at " + times[j]);
            }
            int32_t offset = raw + dst;
            if (offset != Expected[i][j]) {
                errln((UnicodeString)"FAIL: Invalid offset at time(" + times[j] + "):" + offset + " Expected:" + Expected[i][j]);
            }
        }
        delete vtz;
    }
}

void
TimeZoneRuleTest::TestT6669(void) {
    UErrorCode status = U_ZERO_ERROR;
    SimpleTimeZone stz(0, "CustomID", UCAL_JANUARY, 1, UCAL_SUNDAY, 0, UCAL_JULY, 1, UCAL_SUNDAY, 0, status);
    if (U_FAILURE(status)) {
        errln("FAIL: Failed to creat a SimpleTimeZone");
        return;
    }

    UDate t = 1230681600000.0; //2008-12-31T00:00:00
    UDate expectedNext = 1231027200000.0; //2009-01-04T00:00:00
    UDate expectedPrev = 1215298800000.0; //2008-07-06T00:00:00

    TimeZoneTransition tzt;
    UBool avail = stz.getNextTransition(t, FALSE, tzt);
    if (!avail) {
        errln("FAIL: No transition returned by getNextTransition.");
    } else if (tzt.getTime() != expectedNext) {
        errln((UnicodeString)"FAIL: Wrong transition time returned by getNextTransition - "
            + tzt.getTime() + " Expected: " + expectedNext);
    }

    avail = stz.getPreviousTransition(t, TRUE, tzt);
    if (!avail) {
        errln("FAIL: No transition returned by getPreviousTransition.");
    } else if (tzt.getTime() != expectedPrev) {
        errln((UnicodeString)"FAIL: Wrong transition time returned by getPreviousTransition - "
            + tzt.getTime() + " Expected: " + expectedPrev);
    }
}

void
TimeZoneRuleTest::TestVTimeZoneWrapper(void) {
#if 0
    // local variables
    UBool b;
    UChar * data = NULL;
    int32_t length = 0;
    int32_t i;
    UDate result;
    UDate base = 1231027200000.0; //2009-01-04T00:00:00
    UErrorCode status;

    const char *name = "Test Initial";
    UChar uname[20];

    UClassID cid1;
    UClassID cid2;

    ZRule * r;
    IZRule* ir1;
    IZRule* ir2;
    ZTrans* zt1;
    ZTrans* zt2;
    VZone*  v1;
    VZone*  v2;

    uprv_memset(uname, 0, sizeof(uname));
    u_uastrcpy(uname, name);

    // create rules
    ir1 = izrule_open(uname, 13, 2*HOUR, 0);
    ir2 = izrule_clone(ir1);

    // test equality
    b = izrule_equals(ir1, ir2);
    b = izrule_isEquivalentTo(ir1, ir2);

    // test accessors
    izrule_getName(ir1, data, length);
    i = izrule_getRawOffset(ir1);
    i = izrule_getDSTSavings(ir1);

    b = izrule_getFirstStart(ir1, 2*HOUR, 0, result);
    b = izrule_getFinalStart(ir1, 2*HOUR, 0, result);
    b = izrule_getNextStart(ir1, base , 2*HOUR, 0, true, result);
    b = izrule_getPreviousStart(ir1, base, 2*HOUR, 0, true, result);

    // test class ids
    cid1 = izrule_getStaticClassID(ir1);
    cid2 = izrule_getDynamicClassID(ir1);

    // test transitions
    zt1 = ztrans_open(base, ir1, ir2);
    zt2 = ztrans_clone(zt1);
    zt2 = ztrans_openEmpty();

    // test equality
    b = ztrans_equals(zt1, zt2);

    // test accessors
    result = ztrans_getTime(zt1);
    ztrans_setTime(zt1, result);

    r = (ZRule*)ztrans_getFrom(zt1);
    ztrans_setFrom(zt1, (void*)ir1);
    ztrans_adoptFrom(zt1, (void*)ir1);

    r = (ZRule*)ztrans_getTo(zt1);
    ztrans_setTo(zt1, (void*)ir2);
    ztrans_adoptTo(zt1, (void*)ir2);

    // test class ids
    cid1 = ztrans_getStaticClassID(zt1);
    cid2 = ztrans_getDynamicClassID(zt2);

    // test vzone
    v1 = vzone_openID((UChar*)"America/Chicago", sizeof("America/Chicago"));
    v2 = vzone_clone(v1);
    //v2 = vzone_openData(const UChar* vtzdata, int32_t vtzdataLength, UErrorCode& status);

    // test equality
    b = vzone_equals(v1, v2);
    b = vzone_hasSameRules(v1, v2);

    // test accessors
    b = vzone_getTZURL(v1, data, length);
    vzone_setTZURL(v1, data, length);
    
    b = vzone_getLastModified(v1, result);
    vzone_setLastModified(v1, result);
    
    // test writers
    vzone_write(v1, data, length, status);
    vzone_writeFromStart(v1, result, data, length, status);
    vzone_writeSimple(v1, result, data, length, status);

    // test more accessors
    i = vzone_getRawOffset(v1);
    vzone_setRawOffset(v1, i);

    b = vzone_useDaylightTime(v1);
    b = vzone_inDaylightTime(v1, result, status);

    b = vzone_getNextTransition(v1, result, false, zt1);
    b = vzone_getPreviousTransition(v1, result, false, zt1);
    i = vzone_countTransitionRules(v1, status);

    cid1 = vzone_getStaticClassID(v1);
    cid2 = vzone_getDynamicClassID(v1);

    // cleanup
    vzone_close(v1);
    vzone_close(v2);
    ztrans_close(zt1);
    ztrans_close(zt2);
#endif
}

//----------- private test helpers -------------------------------------------------

UDate
TimeZoneRuleTest::getUTCMillis(int32_t y, int32_t m, int32_t d,
                               int32_t hr, int32_t min, int32_t sec, int32_t msec) {
    UErrorCode status = U_ZERO_ERROR;
    const TimeZone *tz = TimeZone::getGMT();
    Calendar *cal = Calendar::createInstance(*tz, status);
    if (U_FAILURE(status)) {
        delete cal;
        dataerrln("FAIL: Calendar::createInstance failed: %s", u_errorName(status));
        return 0.0;
    }
    cal->set(y, m, d, hr, min, sec);
    cal->set(UCAL_MILLISECOND, msec);
    UDate utc = cal->getTime(status);
    if (U_FAILURE(status)) {
        delete cal;
        errln("FAIL: Calendar::getTime failed");
        return 0.0;
    }
    delete cal;
    return utc;
}

/*
 * Check if a time shift really happens on each transition returned by getNextTransition or
 * getPreviousTransition in the specified time range
 */
void
TimeZoneRuleTest::verifyTransitions(BasicTimeZone& icutz, UDate start, UDate end) {
    UErrorCode status = U_ZERO_ERROR;
    UDate time;
    int32_t raw, dst, raw0, dst0;
    TimeZoneTransition tzt, tzt0;
    UBool avail;
    UBool first = TRUE;
    UnicodeString tzid;

    // Ascending
    time = start;
    while (TRUE) {
        avail = icutz.getNextTransition(time, FALSE, tzt);
        if (!avail) {
            break;
        }
        time = tzt.getTime();
        if (time >= end) {
            break;
        }
        icutz.getOffset(time, FALSE, raw, dst, status);
        icutz.getOffset(time - 1, FALSE, raw0, dst0, status);
        if (U_FAILURE(status)) {
            errln("FAIL: Error in getOffset");
            break;
        }

        if (raw == raw0 && dst == dst0) {
            errln((UnicodeString)"FAIL: False transition returned by getNextTransition for "
                + icutz.getID(tzid) + " at " + dateToString(time));
        }
        if (!first &&
                (tzt0.getTo()->getRawOffset() != tzt.getFrom()->getRawOffset()
                || tzt0.getTo()->getDSTSavings() != tzt.getFrom()->getDSTSavings())) {
            errln((UnicodeString)"FAIL: TO rule of the previous transition does not match FROM rule of this transtion at "
                    + dateToString(time) + " for " + icutz.getID(tzid));                
        }
        tzt0 = tzt;
        first = FALSE;
    }

    // Descending
    first = TRUE;
    time = end;
    while(true) {
        avail = icutz.getPreviousTransition(time, FALSE, tzt);
        if (!avail) {
            break;
        }
        time = tzt.getTime();
        if (time <= start) {
            break;
        }
        icutz.getOffset(time, FALSE, raw, dst, status);
        icutz.getOffset(time - 1, FALSE, raw0, dst0, status);
        if (U_FAILURE(status)) {
            errln("FAIL: Error in getOffset");
            break;
        }

        if (raw == raw0 && dst == dst0) {
            errln((UnicodeString)"FAIL: False transition returned by getPreviousTransition for "
                + icutz.getID(tzid) + " at " + dateToString(time));
        }

        if (!first &&
                (tzt0.getFrom()->getRawOffset() != tzt.getTo()->getRawOffset()
                || tzt0.getFrom()->getDSTSavings() != tzt.getTo()->getDSTSavings())) {
            errln((UnicodeString)"FAIL: TO rule of the next transition does not match FROM rule in this transtion at "
                    + dateToString(time) + " for " + icutz.getID(tzid));                
        }
        tzt0 = tzt;
        first = FALSE;
    }
}

/*
 * Compare all time transitions in 2 time zones in the specified time range in ascending order
 */
void
TimeZoneRuleTest::compareTransitionsAscending(BasicTimeZone& z1, BasicTimeZone& z2,
                                              UDate start, UDate end, UBool inclusive) {
    UnicodeString zid1, zid2;
    TimeZoneTransition tzt1, tzt2;
    UBool avail1, avail2;
    UBool inRange1, inRange2;

    z1.getID(zid1);
    z2.getID(zid2);

    UDate time = start;
    while (TRUE) {
        avail1 = z1.getNextTransition(time, inclusive, tzt1);
        avail2 = z2.getNextTransition(time, inclusive, tzt2);

        inRange1 = inRange2 = FALSE;
        if (avail1) {
            if (tzt1.getTime() < end || (inclusive && tzt1.getTime() == end)) {
                inRange1 = TRUE;
            }
        }
        if (avail2) {
            if (tzt2.getTime() < end || (inclusive && tzt2.getTime() == end)) {
                inRange2 = TRUE;
            }
        }
        if (!inRange1 && !inRange2) {
            // No more transition in the range
            break;
        }
        if (!inRange1) {
            errln((UnicodeString)"FAIL: " + zid1 + " does not have any transitions after "
                + dateToString(time) + " before " + dateToString(end));
            break;
        }
        if (!inRange2) {
            errln((UnicodeString)"FAIL: " + zid2 + " does not have any transitions after "
                + dateToString(time) + " before " + dateToString(end));
            break;
        }
        if (tzt1.getTime() != tzt2.getTime()) {
            errln((UnicodeString)"FAIL: First transition after " + dateToString(time) + " "
                    + zid1 + "[" + dateToString(tzt1.getTime()) + "] "
                    + zid2 + "[" + dateToString(tzt2.getTime()) + "]");
            break;
        }
        time = tzt1.getTime();
        if (inclusive) {
            time += 1;
        }
    }
}

/*
 * Compare all time transitions in 2 time zones in the specified time range in descending order
 */
void
TimeZoneRuleTest::compareTransitionsDescending(BasicTimeZone& z1, BasicTimeZone& z2,
                                               UDate start, UDate end, UBool inclusive) {
    UnicodeString zid1, zid2;
    TimeZoneTransition tzt1, tzt2;
    UBool avail1, avail2;
    UBool inRange1, inRange2;

    z1.getID(zid1);
    z2.getID(zid2);

    UDate time = end;
    while (TRUE) {
        avail1 = z1.getPreviousTransition(time, inclusive, tzt1);
        avail2 = z2.getPreviousTransition(time, inclusive, tzt2);

        inRange1 = inRange2 = FALSE;
        if (avail1) {
            if (tzt1.getTime() > start || (inclusive && tzt1.getTime() == start)) {
                inRange1 = TRUE;
            }
        }
        if (avail2) {
            if (tzt2.getTime() > start || (inclusive && tzt2.getTime() == start)) {
                inRange2 = TRUE;
            }
        }
        if (!inRange1 && !inRange2) {
            // No more transition in the range
            break;
        }
        if (!inRange1) {
            errln((UnicodeString)"FAIL: " + zid1 + " does not have any transitions before "
                + dateToString(time) + " after " + dateToString(start));
            break;
        }
        if (!inRange2) {
            errln((UnicodeString)"FAIL: " + zid2 + " does not have any transitions before "
                + dateToString(time) + " after " + dateToString(start));
            break;
        }
        if (tzt1.getTime() != tzt2.getTime()) {
            errln((UnicodeString)"FAIL: Last transition before " + dateToString(time) + " "
                    + zid1 + "[" + dateToString(tzt1.getTime()) + "] "
                    + zid2 + "[" + dateToString(tzt2.getTime()) + "]");
            break;
        }
        time = tzt1.getTime();
        if (inclusive) {
            time -= 1;
        }
    }
}

// Slightly modified version of BasicTimeZone::hasEquivalentTransitions.
// This version returns TRUE if transition time delta is within the given
// delta range.
static UBool hasEquivalentTransitions(/*const*/ BasicTimeZone& tz1, /*const*/BasicTimeZone& tz2,
                                        UDate start, UDate end,
                                        UBool ignoreDstAmount, int32_t maxTransitionTimeDelta,
                                        UErrorCode& status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (tz1.hasSameRules(tz2)) {
        return TRUE;
    }
    // Check the offsets at the start time
    int32_t raw1, raw2, dst1, dst2;
    tz1.getOffset(start, FALSE, raw1, dst1, status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    tz2.getOffset(start, FALSE, raw2, dst2, status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (ignoreDstAmount) {
        if ((raw1 + dst1 != raw2 + dst2)
            || (dst1 != 0 && dst2 == 0)
            || (dst1 == 0 && dst2 != 0)) {
            return FALSE;
        }
    } else {
        if (raw1 != raw2 || dst1 != dst2) {
            return FALSE;
        }            
    }
    // Check transitions in the range
    UDate time = start;
    TimeZoneTransition tr1, tr2;
    while (TRUE) {
        UBool avail1 = tz1.getNextTransition(time, FALSE, tr1);
        UBool avail2 = tz2.getNextTransition(time, FALSE, tr2);

        if (ignoreDstAmount) {
            // Skip a transition which only differ the amount of DST savings
            while (TRUE) {
                if (avail1
                        && tr1.getTime() <= end
                        && (tr1.getFrom()->getRawOffset() + tr1.getFrom()->getDSTSavings()
                                == tr1.getTo()->getRawOffset() + tr1.getTo()->getDSTSavings())
                        && (tr1.getFrom()->getDSTSavings() != 0 && tr1.getTo()->getDSTSavings() != 0)) {
                    tz1.getNextTransition(tr1.getTime(), FALSE, tr1);
                } else {
                    break;
                }
            }
            while (TRUE) {
                if (avail2
                        && tr2.getTime() <= end
                        && (tr2.getFrom()->getRawOffset() + tr2.getFrom()->getDSTSavings()
                                == tr2.getTo()->getRawOffset() + tr2.getTo()->getDSTSavings())
                        && (tr2.getFrom()->getDSTSavings() != 0 && tr2.getTo()->getDSTSavings() != 0)) {
                    tz2.getNextTransition(tr2.getTime(), FALSE, tr2);
                } else {
                    break;
                }
            }
        }

        UBool inRange1 = (avail1 && tr1.getTime() <= end);
        UBool inRange2 = (avail2 && tr2.getTime() <= end);
        if (!inRange1 && !inRange2) {
            // No more transition in the range
            break;
        }
        if (!inRange1 || !inRange2) {
            return FALSE;
        }
        double delta = tr1.getTime() >= tr2.getTime() ? tr1.getTime() - tr2.getTime() : tr2.getTime() - tr1.getTime();
        if (delta > (double)maxTransitionTimeDelta) {
            return FALSE;
        }
        if (ignoreDstAmount) {
            if (tr1.getTo()->getRawOffset() + tr1.getTo()->getDSTSavings()
                        != tr2.getTo()->getRawOffset() + tr2.getTo()->getDSTSavings()
                    || (tr1.getTo()->getDSTSavings() != 0 &&  tr2.getTo()->getDSTSavings() == 0)
                    || (tr1.getTo()->getDSTSavings() == 0 &&  tr2.getTo()->getDSTSavings() != 0)) {
                return FALSE;
            }
        } else {
            if (tr1.getTo()->getRawOffset() != tr2.getTo()->getRawOffset() ||
                tr1.getTo()->getDSTSavings() != tr2.getTo()->getDSTSavings()) {
                return FALSE;
            }
        }
        time = tr1.getTime() > tr2.getTime() ? tr1.getTime() : tr2.getTime();
    }
    return TRUE;
}


#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
