/*
**********************************************************************
* Copyright (c) 2003-2010, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: July 21 2003
* Since: ICU 2.8
**********************************************************************
*/

#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "olsontz.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ures.h"
#include "unicode/simpletz.h"
#include "unicode/gregocal.h"
#include "gregoimp.h"
#include "cmemory.h"
#include "uassert.h"
#include "uvector.h"
#include <float.h> // DBL_MAX
#include "uresimp.h" // struct UResourceBundle

#ifdef U_DEBUG_TZ
# include <stdio.h>
# include "uresimp.h" // for debugging

static void debug_tz_loc(const char *f, int32_t l)
{
  fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_tz_msg(const char *pat, ...)
{
  va_list ap;
  va_start(ap, pat);
  vfprintf(stderr, pat, ap);
  fflush(stderr);
}
// must use double parens, i.e.:  U_DEBUG_TZ_MSG(("four is: %d",4));
#define U_DEBUG_TZ_MSG(x) {debug_tz_loc(__FILE__,__LINE__);debug_tz_msg x;}
#else
#define U_DEBUG_TZ_MSG(x)
#endif

static UBool arrayEqual(const void *a1, const void *a2, int32_t size) {
    if (a1 == NULL && a2 == NULL) {
        return TRUE;
    }
    if ((a1 != NULL && a2 == NULL) || (a1 == NULL && a2 != NULL)) {
        return FALSE;
    }
    if (a1 == a2) {
        return TRUE;
    }

    return (uprv_memcmp(a1, a2, size) == 0);
}

U_NAMESPACE_BEGIN

#define kTRANS          "trans"
#define kTRANSPRE32     "transPre32"
#define kTRANSPOST32    "transPost32"
#define kTYPEOFFSETS    "typeOffsets"
#define kTYPEMAP        "typeMap"
#define kLINKS          "links"
#define kFINALRULE      "finalRule"
#define kFINALRAW       "finalRaw"
#define kFINALYEAR      "finalYear"

#define SECONDS_PER_DAY (24*60*60)

static const int32_t ZEROS[] = {0,0};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(OlsonTimeZone)

/**
 * Default constructor.  Creates a time zone with an empty ID and
 * a fixed GMT offset of zero.
 */
/*OlsonTimeZone::OlsonTimeZone() : finalYear(INT32_MAX), finalMillis(DBL_MAX), finalZone(0), transitionRulesInitialized(FALSE) {
    clearTransitionRules();
    constructEmpty();
}*/

/**
 * Construct a GMT+0 zone with no transitions.  This is done when a
 * constructor fails so the resultant object is well-behaved.
 */
void OlsonTimeZone::constructEmpty() {
    transitionCountPre32 = transitionCount32 = transitionCountPost32 = 0;
    transitionTimesPre32 = transitionTimes32 = transitionTimesPost32 = NULL;

    typeMapData = NULL;

    typeCount = 1;
    typeOffsets = ZEROS;

    finalZone = NULL;
}

/**
 * Construct from a resource bundle
 * @param top the top-level zoneinfo resource bundle.  This is used
 * to lookup the rule that `res' may refer to, if there is one.
 * @param res the resource bundle of the zone to be constructed
 * @param ec input-output error code
 */
OlsonTimeZone::OlsonTimeZone(const UResourceBundle* top,
                             const UResourceBundle* res,
                             UErrorCode& ec) :
  finalZone(NULL), transitionRulesInitialized(FALSE)
{
    clearTransitionRules();
    U_DEBUG_TZ_MSG(("OlsonTimeZone(%s)\n", ures_getKey((UResourceBundle*)res)));
    if ((top == NULL || res == NULL) && U_SUCCESS(ec)) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }
    if (U_SUCCESS(ec)) {
        // TODO -- clean up -- Doesn't work if res points to an alias
        //        // TODO remove nonconst casts below when ures_* API is fixed
        //        setID(ures_getKey((UResourceBundle*) res)); // cast away const

        int32_t len;
        UResourceBundle r;
        ures_initStackObject(&r);

        // Pre-32bit second transitions
        ures_getByKey(res, kTRANSPRE32, &r, &ec);
        transitionTimesPre32 = ures_getIntVector(&r, &len, &ec);
        transitionCountPre32 = len >> 1;
        if (ec == U_MISSING_RESOURCE_ERROR) {
            // No pre-32bit transitions
            transitionTimesPre32 = NULL;
            transitionCountPre32 = 0;
            ec = U_ZERO_ERROR;
        } else if (U_SUCCESS(ec) && (len < 0 || len > 0x7FFF || (len & 1) != 0) /* len must be even */) {
            ec = U_INVALID_FORMAT_ERROR;
        }

        // 32bit second transitions
        ures_getByKey(res, kTRANS, &r, &ec);
        transitionTimes32 = ures_getIntVector(&r, &len, &ec);
        transitionCount32 = len;
        if (ec == U_MISSING_RESOURCE_ERROR) {
            // No 32bit transitions
            transitionTimes32 = NULL;
            transitionCount32 = 0;
            ec = U_ZERO_ERROR;
        } else if (U_SUCCESS(ec) && (len < 0 || len > 0x7FFF)) {
            ec = U_INVALID_FORMAT_ERROR;
        }

        // Post-32bit second transitions
        ures_getByKey(res, kTRANSPOST32, &r, &ec);
        transitionTimesPost32 = ures_getIntVector(&r, &len, &ec);
        transitionCountPost32 = len >> 1;
        if (ec == U_MISSING_RESOURCE_ERROR) {
            // No pre-32bit transitions
            transitionTimesPost32 = NULL;
            transitionCountPost32 = 0;
            ec = U_ZERO_ERROR;
        } else if (U_SUCCESS(ec) && (len < 0 || len > 0x7FFF || (len & 1) != 0) /* len must be even */) {
            ec = U_INVALID_FORMAT_ERROR;
        }

        // Type offsets list must be of even size, with size >= 2
        ures_getByKey(res, kTYPEOFFSETS, &r, &ec);
        typeOffsets = ures_getIntVector(&r, &len, &ec);
        if (U_SUCCESS(ec) && (len < 2 || len > 0x7FFE || (len & 1) != 0)) {
            ec = U_INVALID_FORMAT_ERROR;
        }
        typeCount = (int16_t) len >> 1;

        // Type map data must be of the same size as the transition count
        typeMapData =  NULL;
        if (transitionCount() > 0) {
            ures_getByKey(res, kTYPEMAP, &r, &ec);
            typeMapData = ures_getBinary(&r, &len, &ec);
            if (ec == U_MISSING_RESOURCE_ERROR) {
                // no type mapping data
                ec = U_INVALID_FORMAT_ERROR;
            } else if (U_SUCCESS(ec) && len != transitionCount()) {
                ec = U_INVALID_FORMAT_ERROR;
            }
        }

        // Process final rule and data, if any
        const UChar *ruleIdUStr = ures_getStringByKey(res, kFINALRULE, &len, &ec);
        ures_getByKey(res, kFINALRAW, &r, &ec);
        int32_t ruleRaw = ures_getInt(&r, &ec);
        ures_getByKey(res, kFINALYEAR, &r, &ec);
        int32_t ruleYear = ures_getInt(&r, &ec);
        if (U_SUCCESS(ec)) {
            UnicodeString ruleID(TRUE, ruleIdUStr, len);
            UResourceBundle *rule = TimeZone::loadRule(top, ruleID, NULL, ec);
            const int32_t *ruleData = ures_getIntVector(rule, &len, &ec); 
            if (U_SUCCESS(ec) && len == 11) {
                UnicodeString emptyStr;
                finalZone = new SimpleTimeZone(
                    ruleRaw * U_MILLIS_PER_SECOND,
                    emptyStr,
                    (int8_t)ruleData[0], (int8_t)ruleData[1], (int8_t)ruleData[2],
                    ruleData[3] * U_MILLIS_PER_SECOND,
                    (SimpleTimeZone::TimeMode) ruleData[4],
                    (int8_t)ruleData[5], (int8_t)ruleData[6], (int8_t)ruleData[7],
                    ruleData[8] * U_MILLIS_PER_SECOND,
                    (SimpleTimeZone::TimeMode) ruleData[9],
                    ruleData[10] * U_MILLIS_PER_SECOND, ec);
                if (finalZone == NULL) {
                    ec = U_MEMORY_ALLOCATION_ERROR;
                } else {
                    finalStartYear = ruleYear;

                    // Note: Setting finalStartYear to the finalZone is problematic.  When a date is around
                    // year boundary, SimpleTimeZone may return false result when DST is observed at the 
                    // beginning of year.  We could apply safe margin (day or two), but when one of recurrent
                    // rules falls around year boundary, it could return false result.  Without setting the
                    // start year, finalZone works fine around the year boundary of the start year.

                    // finalZone->setStartYear(finalStartYear);


                    // Compute the millis for Jan 1, 0:00 GMT of the finalYear

                    // Note: finalStartMillis is used for detecting either if
                    // historic transition data or finalZone to be used.  In an
                    // extreme edge case - for example, two transitions fall into
                    // small windows of time around the year boundary, this may
                    // result incorrect offset computation.  But I think it will
                    // never happen practically.  Yoshito - Feb 20, 2010
                    finalStartMillis = Grego::fieldsToDay(finalStartYear, 0, 1) * U_MILLIS_PER_DAY;
                }
            } else {
                ec = U_INVALID_FORMAT_ERROR;
            }
            ures_close(rule);
        } else if (ec == U_MISSING_RESOURCE_ERROR) {
            // No final zone
            ec = U_ZERO_ERROR;
        }
        ures_close(&r);
    }

    if (U_FAILURE(ec)) {
        constructEmpty();
    }
}

/**
 * Copy constructor
 */
OlsonTimeZone::OlsonTimeZone(const OlsonTimeZone& other) :
    BasicTimeZone(other), finalZone(0) {
    *this = other;
}

/**
 * Assignment operator
 */
OlsonTimeZone& OlsonTimeZone::operator=(const OlsonTimeZone& other) {
    transitionTimesPre32 = other.transitionTimesPre32;
    transitionTimes32 = other.transitionTimes32;
    transitionTimesPost32 = other.transitionTimesPost32;

    transitionCountPre32 = other.transitionCountPre32;
    transitionCount32 = other.transitionCount32;
    transitionCountPost32 = other.transitionCountPost32;

    typeCount = other.typeCount;
    typeOffsets = other.typeOffsets;
    typeMapData = other.typeMapData;

    delete finalZone;
    finalZone = (other.finalZone != 0) ?
        (SimpleTimeZone*) other.finalZone->clone() : 0;

    finalStartYear = other.finalStartYear;
    finalStartMillis = other.finalStartMillis;

    clearTransitionRules();

    return *this;
}

/**
 * Destructor
 */
OlsonTimeZone::~OlsonTimeZone() {
    deleteTransitionRules();
    delete finalZone;
}

/**
 * Returns true if the two TimeZone objects are equal.
 */
UBool OlsonTimeZone::operator==(const TimeZone& other) const {
    return ((this == &other) ||
            (typeid(*this) == typeid(other) &&
            TimeZone::operator==(other) &&
            hasSameRules(other)));
}

/**
 * TimeZone API.
 */
TimeZone* OlsonTimeZone::clone() const {
    return new OlsonTimeZone(*this);
}

/**
 * TimeZone API.
 */
int32_t OlsonTimeZone::getOffset(uint8_t era, int32_t year, int32_t month,
                                 int32_t dom, uint8_t dow,
                                 int32_t millis, UErrorCode& ec) const {
    if (month < UCAL_JANUARY || month > UCAL_DECEMBER) {
        if (U_SUCCESS(ec)) {
            ec = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return 0;
    } else {
        return getOffset(era, year, month, dom, dow, millis,
                         Grego::monthLength(year, month),
                         ec);
    }
}

/**
 * TimeZone API.
 */
int32_t OlsonTimeZone::getOffset(uint8_t era, int32_t year, int32_t month,
                                 int32_t dom, uint8_t dow,
                                 int32_t millis, int32_t monthLength,
                                 UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return 0;
    }

    if ((era != GregorianCalendar::AD && era != GregorianCalendar::BC)
        || month < UCAL_JANUARY
        || month > UCAL_DECEMBER
        || dom < 1
        || dom > monthLength
        || dow < UCAL_SUNDAY
        || dow > UCAL_SATURDAY
        || millis < 0
        || millis >= U_MILLIS_PER_DAY
        || monthLength < 28
        || monthLength > 31) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if (era == GregorianCalendar::BC) {
        year = -year;
    }

    if (finalZone != NULL && year >= finalStartYear) {
        return finalZone->getOffset(era, year, month, dom, dow,
                                    millis, monthLength, ec);
    }

    // Compute local epoch millis from input fields
    UDate date = (UDate)(Grego::fieldsToDay(year, month, dom) * U_MILLIS_PER_DAY + millis);
    int32_t rawoff, dstoff;
    getHistoricalOffset(date, TRUE, kDaylight, kStandard, rawoff, dstoff);
    return rawoff + dstoff;
}

/**
 * TimeZone API.
 */
void OlsonTimeZone::getOffset(UDate date, UBool local, int32_t& rawoff,
                              int32_t& dstoff, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return;
    }
    if (finalZone != NULL && date >= finalStartMillis) {
        finalZone->getOffset(date, local, rawoff, dstoff, ec);
    } else {
        getHistoricalOffset(date, local, kFormer, kLatter, rawoff, dstoff);
    }
}

void
OlsonTimeZone::getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
                                  int32_t& rawoff, int32_t& dstoff, UErrorCode& ec) /*const*/ {
    if (U_FAILURE(ec)) {
        return;
    }
    if (finalZone != NULL && date >= finalStartMillis) {
        finalZone->getOffsetFromLocal(date, nonExistingTimeOpt, duplicatedTimeOpt, rawoff, dstoff, ec);
    } else {
        getHistoricalOffset(date, TRUE, nonExistingTimeOpt, duplicatedTimeOpt, rawoff, dstoff);
    }
}


/**
 * TimeZone API.
 */
void OlsonTimeZone::setRawOffset(int32_t /*offsetMillis*/) {
    // We don't support this operation, since OlsonTimeZones are
    // immutable (except for the ID, which is in the base class).

    // Nothing to do!
}

/**
 * TimeZone API.
 */
int32_t OlsonTimeZone::getRawOffset() const {
    UErrorCode ec = U_ZERO_ERROR;
    int32_t raw, dst;
    getOffset((double) uprv_getUTCtime() * U_MILLIS_PER_SECOND,
              FALSE, raw, dst, ec);
    return raw;
}

#if defined U_DEBUG_TZ
void printTime(double ms) {
            int32_t year, month, dom, dow;
            double millis=0;
            double days = ClockMath::floorDivide(((double)ms), (double)U_MILLIS_PER_DAY, millis);
            
            Grego::dayToFields(days, year, month, dom, dow);
            U_DEBUG_TZ_MSG(("   getHistoricalOffset:  time %.1f (%04d.%02d.%02d+%.1fh)\n", ms,
                            year, month+1, dom, (millis/kOneHour)));
    }
#endif

int64_t
OlsonTimeZone::transitionTimeInSeconds(int16_t transIdx) const {
    U_ASSERT(transIdx >= 0 && transIdx < transitionCount()); 

    if (transIdx < transitionCountPre32) {
        return (((int64_t)((uint32_t)transitionTimesPre32[transIdx << 1])) << 32)
            | ((int64_t)((uint32_t)transitionTimesPre32[(transIdx << 1) + 1]));
    }

    transIdx -= transitionCountPre32;
    if (transIdx < transitionCount32) {
        return (int64_t)transitionTimes32[transIdx];
    }

    transIdx -= transitionCount32;
    return (((int64_t)((uint32_t)transitionTimesPost32[transIdx << 1])) << 32)
        | ((int64_t)((uint32_t)transitionTimesPost32[(transIdx << 1) + 1]));
}

void
OlsonTimeZone::getHistoricalOffset(UDate date, UBool local,
                                   int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt,
                                   int32_t& rawoff, int32_t& dstoff) const {
    U_DEBUG_TZ_MSG(("getHistoricalOffset(%.1f, %s, %d, %d, raw, dst)\n",
        date, local?"T":"F", NonExistingTimeOpt, DuplicatedTimeOpt));
#if defined U_DEBUG_TZ
        printTime(date*1000.0);
#endif
    int16_t transCount = transitionCount();

    if (transCount > 0) {
        double sec = uprv_floor(date / U_MILLIS_PER_SECOND);
        if (!local && sec < transitionTimeInSeconds(0)) {
            // Before the first transition time
            rawoff = initialRawOffset() * U_MILLIS_PER_SECOND;
            dstoff = initialDstOffset() * U_MILLIS_PER_SECOND;
        } else {
            // Linear search from the end is the fastest approach, since
            // most lookups will happen at/near the end.
            int16_t transIdx;
            for (transIdx = transCount - 1; transIdx >= 0; transIdx--) {
                int64_t transition = transitionTimeInSeconds(transIdx);

                if (local) {
                    int32_t offsetBefore = zoneOffsetAt(transIdx - 1);
                    UBool dstBefore = dstOffsetAt(transIdx - 1) != 0;

                    int32_t offsetAfter = zoneOffsetAt(transIdx);
                    UBool dstAfter = dstOffsetAt(transIdx) != 0;

                    UBool dstToStd = dstBefore && !dstAfter;
                    UBool stdToDst = !dstBefore && dstAfter;
                    
                    if (offsetAfter - offsetBefore >= 0) {
                        // Positive transition, which makes a non-existing local time range
                        if (((NonExistingTimeOpt & kStdDstMask) == kStandard && dstToStd)
                                || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
                            transition += offsetBefore;
                        } else if (((NonExistingTimeOpt & kStdDstMask) == kStandard && stdToDst)
                                || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
                            transition += offsetAfter;
                        } else if ((NonExistingTimeOpt & kFormerLatterMask) == kLatter) {
                            transition += offsetBefore;
                        } else {
                            // Interprets the time with rule before the transition,
                            // default for non-existing time range
                            transition += offsetAfter;
                        }
                    } else {
                        // Negative transition, which makes a duplicated local time range
                        if (((DuplicatedTimeOpt & kStdDstMask) == kStandard && dstToStd)
                                || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
                            transition += offsetAfter;
                        } else if (((DuplicatedTimeOpt & kStdDstMask) == kStandard && stdToDst)
                                || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
                            transition += offsetBefore;
                        } else if ((DuplicatedTimeOpt & kFormerLatterMask) == kFormer) {
                            transition += offsetBefore;
                        } else {
                            // Interprets the time with rule after the transition,
                            // default for duplicated local time range
                            transition += offsetAfter;
                        }
                    }
                }
                if (sec >= transition) {
                    break;
                }
            }
            // transIdx could be -1 when local=true
            rawoff = rawOffsetAt(transIdx) * U_MILLIS_PER_SECOND;
            dstoff = dstOffsetAt(transIdx) * U_MILLIS_PER_SECOND;
        }
    } else {
        // No transitions, single pair of offsets only
        rawoff = initialRawOffset() * U_MILLIS_PER_SECOND;
        dstoff = initialDstOffset() * U_MILLIS_PER_SECOND;
    }
    U_DEBUG_TZ_MSG(("getHistoricalOffset(%.1f, %s, %d, %d, raw, dst) - raw=%d, dst=%d\n",
        date, local?"T":"F", NonExistingTimeOpt, DuplicatedTimeOpt, rawoff, dstoff));
}

/**
 * TimeZone API.
 */
UBool OlsonTimeZone::useDaylightTime() const {
    // If DST was observed in 1942 (for example) but has never been
    // observed from 1943 to the present, most clients will expect
    // this method to return FALSE.  This method determines whether
    // DST is in use in the current year (at any point in the year)
    // and returns TRUE if so.

    UDate current = uprv_getUTCtime();
    if (finalZone != NULL && current >= finalStartMillis) {
        return finalZone->useDaylightTime();
    }

    int32_t year, month, dom, dow, doy, mid;
    Grego::timeToFields(current, year, month, dom, dow, doy, mid);

    // Find start of this year, and start of next year
    double start = Grego::fieldsToDay(year, 0, 1) * SECONDS_PER_DAY;
    double limit = Grego::fieldsToDay(year+1, 0, 1) * SECONDS_PER_DAY;

    // Return TRUE if DST is observed at any time during the current
    // year.
    for (int16_t i = 0; i < transitionCount(); ++i) {
        double transition = transitionTime(i);
        if (transition >= limit) {
            break;
        }
        if ((transition >= start && dstOffsetAt(i) != 0)
                || (transition > start && dstOffsetAt(i - 1) != 0)) {
            return TRUE;
        }
    }
    return FALSE;
}
int32_t 
OlsonTimeZone::getDSTSavings() const{
    if (finalZone != NULL){
        return finalZone->getDSTSavings();
    }
    return TimeZone::getDSTSavings();
}
/**
 * TimeZone API.
 */
UBool OlsonTimeZone::inDaylightTime(UDate date, UErrorCode& ec) const {
    int32_t raw, dst;
    getOffset(date, FALSE, raw, dst, ec);
    return dst != 0;
}

UBool
OlsonTimeZone::hasSameRules(const TimeZone &other) const {
    if (this == &other) {
        return TRUE;
    }
    const OlsonTimeZone* z = dynamic_cast<const OlsonTimeZone*>(&other);
    if (z == NULL) {
        return FALSE;
    }

    // [sic] pointer comparison: typeMapData points into
    // memory-mapped or DLL space, so if two zones have the same
    // pointer, they are equal.
    if (typeMapData == z->typeMapData) {
        return TRUE;
    }
    
    // If the pointers are not equal, the zones may still
    // be equal if their rules and transitions are equal
    if ((finalZone == NULL && z->finalZone != NULL)
        || (finalZone != NULL && z->finalZone == NULL)
        || (finalZone != NULL && z->finalZone != NULL && *finalZone != *z->finalZone)) {
        return FALSE;
    }

    if (finalZone != NULL) {
        if (finalStartYear != z->finalStartYear || finalStartMillis != z->finalStartMillis) {
            return FALSE;
        }
    }
    if (typeCount != z->typeCount
        || transitionCountPre32 != z->transitionCountPre32
        || transitionCount32 != z->transitionCount32
        || transitionCountPost32 != z->transitionCountPost32) {
        return FALSE;
    }

    return
        arrayEqual(transitionTimesPre32, z->transitionTimesPre32, sizeof(transitionTimesPre32[0]) * transitionCountPre32 << 1)
        && arrayEqual(transitionTimes32, z->transitionTimes32, sizeof(transitionTimes32[0]) * transitionCount32)
        && arrayEqual(transitionTimesPost32, z->transitionTimesPost32, sizeof(transitionTimesPost32[0]) * transitionCountPost32 << 1)
        && arrayEqual(typeOffsets, z->typeOffsets, sizeof(typeOffsets[0]) * typeCount << 1)
        && arrayEqual(typeMapData, z->typeMapData, sizeof(typeMapData[0]) * transitionCount());
}

void
OlsonTimeZone::clearTransitionRules(void) {
    initialRule = NULL;
    firstTZTransition = NULL;
    firstFinalTZTransition = NULL;
    historicRules = NULL;
    historicRuleCount = 0;
    finalZoneWithStartYear = NULL;
    firstTZTransitionIdx = 0;
    transitionRulesInitialized = FALSE;
}

void
OlsonTimeZone::deleteTransitionRules(void) {
    if (initialRule != NULL) {
        delete initialRule;
    }
    if (firstTZTransition != NULL) {
        delete firstTZTransition;
    }
    if (firstFinalTZTransition != NULL) {
        delete firstFinalTZTransition;
    }
    if (finalZoneWithStartYear != NULL) {
        delete finalZoneWithStartYear;
    }
    if (historicRules != NULL) {
        for (int i = 0; i < historicRuleCount; i++) {
            if (historicRules[i] != NULL) {
                delete historicRules[i];
            }
        }
        uprv_free(historicRules);
    }
    clearTransitionRules();
}

void
OlsonTimeZone::initTransitionRules(UErrorCode& status) {
    if(U_FAILURE(status)) {
        return;
    }
    if (transitionRulesInitialized) {
        return;
    }
    deleteTransitionRules();
    UnicodeString tzid;
    getID(tzid);

    UnicodeString stdName = tzid + UNICODE_STRING_SIMPLE("(STD)");
    UnicodeString dstName = tzid + UNICODE_STRING_SIMPLE("(DST)");

    int32_t raw, dst;

    // Create initial rule
    raw = initialRawOffset() * U_MILLIS_PER_SECOND;
    dst = initialDstOffset() * U_MILLIS_PER_SECOND;
    initialRule = new InitialTimeZoneRule((dst == 0 ? stdName : dstName), raw, dst);
    // Check to make sure initialRule was created
    if (initialRule == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        deleteTransitionRules();
        return;
    }

    int32_t transCount = transitionCount();
    if (transCount > 0) {
        int16_t transitionIdx, typeIdx;

        // We probably no longer need to check the first "real" transition
        // here, because the new tzcode remove such transitions already.
        // For now, keeping this code for just in case. Feb 19, 2010 Yoshito
        firstTZTransitionIdx = 0;
        for (transitionIdx = 0; transitionIdx < transCount; transitionIdx++) {
            if (typeMapData[transitionIdx] != 0) { // type 0 is the initial type
                break;
            }
            firstTZTransitionIdx++;
        }
        if (transitionIdx == transCount) {
            // Actually no transitions...
        } else {
            // Build historic rule array
            UDate* times = (UDate*)uprv_malloc(sizeof(UDate)*transCount); /* large enough to store all transition times */
            if (times == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
            for (typeIdx = 0; typeIdx < typeCount; typeIdx++) {
                // Gather all start times for each pair of offsets
                int32_t nTimes = 0;
                for (transitionIdx = firstTZTransitionIdx; transitionIdx < transCount; transitionIdx++) {
                    if (typeIdx == (int16_t)typeMapData[transitionIdx]) {
                        UDate tt = (UDate)transitionTime(transitionIdx);
                        if (finalZone == NULL || tt <= finalStartMillis) {
                            // Exclude transitions after finalMillis
                            times[nTimes++] = tt;
                        }
                    }
                }
                if (nTimes > 0) {
                    // Create a TimeArrayTimeZoneRule
                    raw = typeOffsets[typeIdx << 1] * U_MILLIS_PER_SECOND;
                    dst = typeOffsets[(typeIdx << 1) + 1] * U_MILLIS_PER_SECOND;
                    if (historicRules == NULL) {
                        historicRuleCount = typeCount;
                        historicRules = (TimeArrayTimeZoneRule**)uprv_malloc(sizeof(TimeArrayTimeZoneRule*)*historicRuleCount);
                        if (historicRules == NULL) {
                            status = U_MEMORY_ALLOCATION_ERROR;
                            deleteTransitionRules();
                            uprv_free(times);
                            return;
                        }
                        for (int i = 0; i < historicRuleCount; i++) {
                            // Initialize TimeArrayTimeZoneRule pointers as NULL
                            historicRules[i] = NULL;
                        }
                    }
                    historicRules[typeIdx] = new TimeArrayTimeZoneRule((dst == 0 ? stdName : dstName),
                        raw, dst, times, nTimes, DateTimeRule::UTC_TIME);
                    // Check for memory allocation error
                    if (historicRules[typeIdx] == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        deleteTransitionRules();
                        return;
                    }
                }
            }
            uprv_free(times);

            // Create initial transition
            typeIdx = (int16_t)typeMapData[firstTZTransitionIdx];
            firstTZTransition = new TimeZoneTransition((UDate)transitionTime(firstTZTransitionIdx),
                    *initialRule, *historicRules[typeIdx]);
            // Check to make sure firstTZTransition was created.
            if (firstTZTransition == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
        }
    }
    if (finalZone != NULL) {
        // Get the first occurence of final rule starts
        UDate startTime = (UDate)finalStartMillis;
        TimeZoneRule *firstFinalRule = NULL;

        if (finalZone->useDaylightTime()) {
            /*
             * Note: When an OlsonTimeZone is constructed, we should set the final year
             * as the start year of finalZone.  However, the bounday condition used for
             * getting offset from finalZone has some problems.
             * For now, we do not set the valid start year when the construction time
             * and create a clone and set the start year when extracting rules.
             */
            finalZoneWithStartYear = (SimpleTimeZone*)finalZone->clone();
            // Check to make sure finalZone was actually cloned.
            if (finalZoneWithStartYear == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
            finalZoneWithStartYear->setStartYear(finalStartYear);

            TimeZoneTransition tzt;
            finalZoneWithStartYear->getNextTransition(startTime, false, tzt);
            firstFinalRule  = tzt.getTo()->clone();
            // Check to make sure firstFinalRule received proper clone.
            if (firstFinalRule == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
            startTime = tzt.getTime();
        } else {
            // final rule with no transitions
            finalZoneWithStartYear = (SimpleTimeZone*)finalZone->clone();
            // Check to make sure finalZone was actually cloned.
            if (finalZoneWithStartYear == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
            finalZone->getID(tzid);
            firstFinalRule = new TimeArrayTimeZoneRule(tzid,
                finalZone->getRawOffset(), 0, &startTime, 1, DateTimeRule::UTC_TIME);
            // Check firstFinalRule was properly created.
            if (firstFinalRule == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
        }
        TimeZoneRule *prevRule = NULL;
        if (transCount > 0) {
            prevRule = historicRules[typeMapData[transCount - 1]];
        }
        if (prevRule == NULL) {
            // No historic transitions, but only finalZone available
            prevRule = initialRule;
        }
        firstFinalTZTransition = new TimeZoneTransition();
        // Check to make sure firstFinalTZTransition was created before dereferencing
        if (firstFinalTZTransition == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            deleteTransitionRules();
            return;
        }
        firstFinalTZTransition->setTime(startTime);
        firstFinalTZTransition->adoptFrom(prevRule->clone());
        firstFinalTZTransition->adoptTo(firstFinalRule);
    }
    transitionRulesInitialized = TRUE;
}

UBool
OlsonTimeZone::getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) /*const*/ {
    UErrorCode status = U_ZERO_ERROR;
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    if (finalZone != NULL) {
        if (inclusive && base == firstFinalTZTransition->getTime()) {
            result = *firstFinalTZTransition;
            return TRUE;
        } else if (base >= firstFinalTZTransition->getTime()) {
            if (finalZone->useDaylightTime()) {
                //return finalZone->getNextTransition(base, inclusive, result);
                return finalZoneWithStartYear->getNextTransition(base, inclusive, result);
            } else {
                // No more transitions
                return FALSE;
            }
        }
    }
    if (historicRules != NULL) {
        // Find a historical transition
        int16_t transCount = transitionCount();
        int16_t ttidx = transCount - 1;
        for (; ttidx >= firstTZTransitionIdx; ttidx--) {
            UDate t = (UDate)transitionTime(ttidx);
            if (base > t || (!inclusive && base == t)) {
                break;
            }
        }
        if (ttidx == transCount - 1)  {
            if (firstFinalTZTransition != NULL) {
                result = *firstFinalTZTransition;
                return TRUE;
            } else {
                return FALSE;
            }
        } else if (ttidx < firstTZTransitionIdx) {
            result = *firstTZTransition;
            return TRUE;
        } else {
            // Create a TimeZoneTransition
            TimeZoneRule *to = historicRules[typeMapData[ttidx + 1]];
            TimeZoneRule *from = historicRules[typeMapData[ttidx]];
            UDate startTime = (UDate)transitionTime(ttidx+1);

            // The transitions loaded from zoneinfo.res may contain non-transition data
            UnicodeString fromName, toName;
            from->getName(fromName);
            to->getName(toName);
            if (fromName == toName && from->getRawOffset() == to->getRawOffset()
                    && from->getDSTSavings() == to->getDSTSavings()) {
                return getNextTransition(startTime, false, result);
            }
            result.setTime(startTime);
            result.adoptFrom(from->clone());
            result.adoptTo(to->clone());
            return TRUE;
        }
    }
    return FALSE;
}

UBool
OlsonTimeZone::getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) /*const*/ {
    UErrorCode status = U_ZERO_ERROR;
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    if (finalZone != NULL) {
        if (inclusive && base == firstFinalTZTransition->getTime()) {
            result = *firstFinalTZTransition;
            return TRUE;
        } else if (base > firstFinalTZTransition->getTime()) {
            if (finalZone->useDaylightTime()) {
                //return finalZone->getPreviousTransition(base, inclusive, result);
                return finalZoneWithStartYear->getPreviousTransition(base, inclusive, result);
            } else {
                result = *firstFinalTZTransition;
                return TRUE;
            }
        }
    }

    if (historicRules != NULL) {
        // Find a historical transition
        int16_t ttidx = transitionCount() - 1;
        for (; ttidx >= firstTZTransitionIdx; ttidx--) {
            UDate t = (UDate)transitionTime(ttidx);
            if (base > t || (inclusive && base == t)) {
                break;
            }
        }
        if (ttidx < firstTZTransitionIdx) {
            // No more transitions
            return FALSE;
        } else if (ttidx == firstTZTransitionIdx) {
            result = *firstTZTransition;
            return TRUE;
        } else {
            // Create a TimeZoneTransition
            TimeZoneRule *to = historicRules[typeMapData[ttidx]];
            TimeZoneRule *from = historicRules[typeMapData[ttidx-1]];
            UDate startTime = (UDate)transitionTime(ttidx);

            // The transitions loaded from zoneinfo.res may contain non-transition data
            UnicodeString fromName, toName;
            from->getName(fromName);
            to->getName(toName);
            if (fromName == toName && from->getRawOffset() == to->getRawOffset()
                    && from->getDSTSavings() == to->getDSTSavings()) {
                return getPreviousTransition(startTime, false, result);
            }
            result.setTime(startTime);
            result.adoptFrom(from->clone());
            result.adoptTo(to->clone());
            return TRUE;
        }
    }
    return FALSE;
}

int32_t
OlsonTimeZone::countTransitionRules(UErrorCode& status) /*const*/ {
    if (U_FAILURE(status)) {
        return 0;
    }
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return 0;
    }

    int32_t count = 0;
    if (historicRules != NULL) {
        // historicRules may contain null entries when original zoneinfo data
        // includes non transition data.
        for (int32_t i = 0; i < historicRuleCount; i++) {
            if (historicRules[i] != NULL) {
                count++;
            }
        }
    }
    if (finalZone != NULL) {
        if (finalZone->useDaylightTime()) {
            count += 2;
        } else {
            count++;
        }
    }
    return count;
}

void
OlsonTimeZone::getTimeZoneRules(const InitialTimeZoneRule*& initial,
                                const TimeZoneRule* trsrules[],
                                int32_t& trscount,
                                UErrorCode& status) /*const*/ {
    if (U_FAILURE(status)) {
        return;
    }
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return;
    }

    // Initial rule
    initial = initialRule;

    // Transition rules
    int32_t cnt = 0;
    if (historicRules != NULL && trscount > cnt) {
        // historicRules may contain null entries when original zoneinfo data
        // includes non transition data.
        for (int32_t i = 0; i < historicRuleCount; i++) {
            if (historicRules[i] != NULL) {
                trsrules[cnt++] = historicRules[i];
                if (cnt >= trscount) {
                    break;
                }
            }
        }
    }
    if (finalZoneWithStartYear != NULL && trscount > cnt) {
        const InitialTimeZoneRule *tmpini;
        int32_t tmpcnt = trscount - cnt;
        finalZoneWithStartYear->getTimeZoneRules(tmpini, &trsrules[cnt], tmpcnt, status);
        if (U_FAILURE(status)) {
            return;
        }
        cnt += tmpcnt;
    }
    // Set the result length
    trscount = cnt;
}

U_NAMESPACE_END

#endif // !UCONFIG_NO_FORMATTING

//eof
