/*
******************************************************************************
* Copyright (C) 2003-2010, International Business Machines Corporation
* and others. All Rights Reserved.
******************************************************************************
*
* File ISLAMCAL.H
*
* Modification History:
*
*   Date        Name        Description
*   10/14/2003  srl         ported from java IslamicCalendar
*****************************************************************************
*/

#include "islamcal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>
#include "gregoimp.h" // Math
#include "astro.h" // CalendarAstronomer
#include "uhash.h"
#include "ucln_in.h"

static const UDate HIJRA_MILLIS = -42521587200000.0;    // 7/16/622 AD 00:00

// Debugging
#ifdef U_DEBUG_ISLAMCAL
# include <stdio.h>
# include <stdarg.h>
static void debug_islamcal_loc(const char *f, int32_t l)
{
    fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_islamcal_msg(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    vfprintf(stderr, pat, ap);
    fflush(stderr);
}
// must use double parens, i.e.:  U_DEBUG_ISLAMCAL_MSG(("four is: %d",4));
#define U_DEBUG_ISLAMCAL_MSG(x) {debug_islamcal_loc(__FILE__,__LINE__);debug_islamcal_msg x;}
#else
#define U_DEBUG_ISLAMCAL_MSG(x)
#endif


// --- The cache --
// cache of months
static UMTX astroLock = 0;  // pod bay door lock
static U_NAMESPACE_QUALIFIER CalendarCache *gMonthCache = NULL;
static U_NAMESPACE_QUALIFIER CalendarAstronomer *gIslamicCalendarAstro = NULL;

U_CDECL_BEGIN
static UBool calendar_islamic_cleanup(void) {
    if (gMonthCache) {
        delete gMonthCache;
        gMonthCache = NULL;
    }
    if (gIslamicCalendarAstro) {
        delete gIslamicCalendarAstro;
        gIslamicCalendarAstro = NULL;
    }
    umtx_destroy(&astroLock);
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN

// Implementation of the IslamicCalendar class

//-------------------------------------------------------------------------
// Constructors...
//-------------------------------------------------------------------------

const char *IslamicCalendar::getType() const { 
    if(civil==CIVIL) {
        return "islamic-civil";
    } else {
        return "islamic";
    }
}

Calendar* IslamicCalendar::clone() const {
    return new IslamicCalendar(*this);
}

IslamicCalendar::IslamicCalendar(const Locale& aLocale, UErrorCode& success, ECivil beCivil)
:   Calendar(TimeZone::createDefault(), aLocale, success),
civil(beCivil)
{
    setTimeInMillis(getNow(), success); // Call this again now that the vtable is set up properly.
}

IslamicCalendar::IslamicCalendar(const IslamicCalendar& other) : Calendar(other), civil(other.civil) {
}

IslamicCalendar::~IslamicCalendar()
{
}

/**
* Determines whether this object uses the fixed-cycle Islamic civil calendar
* or an approximation of the religious, astronomical calendar.
*
* @param beCivil   <code>true</code> to use the civil calendar,
*                  <code>false</code> to use the astronomical calendar.
* @draft ICU 2.4
*/
void IslamicCalendar::setCivil(ECivil beCivil, UErrorCode &status)
{
    if (civil != beCivil) {
        // The fields of the calendar will become invalid, because the calendar
        // rules are different
        UDate m = getTimeInMillis(status);
        civil = beCivil;
        clear();
        setTimeInMillis(m, status);
    }
}

/**
* Returns <code>true</code> if this object is using the fixed-cycle civil
* calendar, or <code>false</code> if using the religious, astronomical
* calendar.
* @draft ICU 2.4
*/
UBool IslamicCalendar::isCivil() {
    return (civil == CIVIL);
}

//-------------------------------------------------------------------------
// Minimum / Maximum access functions
//-------------------------------------------------------------------------

// Note: Current IslamicCalendar implementation does not work
// well with negative years.

static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    // Minimum  Greatest    Least  Maximum
    //           Minimum  Maximum
    {        0,        0,        0,        0}, // ERA
    {        1,        1,  5000000,  5000000}, // YEAR
    {        0,        0,       11,       11}, // MONTH
    {        1,        1,       50,       51}, // WEEK_OF_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // WEEK_OF_MONTH
    {        1,        1,       29,       30}, // DAY_OF_MONTH
    {        1,        1,      354,      355}, // DAY_OF_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DAY_OF_WEEK
    {       -1,       -1,        5,        5}, // DAY_OF_WEEK_IN_MONTH
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // AM_PM
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // HOUR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // HOUR_OF_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MINUTE
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // SECOND
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MILLISECOND
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // ZONE_OFFSET
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DST_OFFSET
    {        1,        1,  5000000,  5000000}, // YEAR_WOY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DOW_LOCAL
    {        1,        1,  5000000,  5000000}, // EXTENDED_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // JULIAN_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MILLISECONDS_IN_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // IS_LEAP_MONTH
};

/**
* @draft ICU 2.4
*/
int32_t IslamicCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return LIMITS[field][limitType];
}

//-------------------------------------------------------------------------
// Assorted calculation utilities
//

/**
* Determine whether a year is a leap year in the Islamic civil calendar
*/
UBool IslamicCalendar::civilLeapYear(int32_t year)
{
    return (14 + 11 * year) % 30 < 11;
}

/**
* Return the day # on which the given year starts.  Days are counted
* from the Hijri epoch, origin 0.
*/
int32_t IslamicCalendar::yearStart(int32_t year) {
    if (civil == CIVIL) {
        return (year-1)*354 + ClockMath::floorDivide((3+11*year),30);
    } else {
        return trueMonthStart(12*(year-1));
    }
}

/**
* Return the day # on which the given month starts.  Days are counted
* from the Hijri epoch, origin 0.
*
* @param year  The hijri year
* @param year  The hijri month, 0-based
*/
int32_t IslamicCalendar::monthStart(int32_t year, int32_t month) const {
    if (civil == CIVIL) {
        return (int32_t)uprv_ceil(29.5*month)
            + (year-1)*354 + (int32_t)ClockMath::floorDivide((3+11*year),30);
    } else {
        return trueMonthStart(12*(year-1) + month);
    }
}

/**
* Find the day number on which a particular month of the true/lunar
* Islamic calendar starts.
*
* @param month The month in question, origin 0 from the Hijri epoch
*
* @return The day number on which the given month starts.
*/
int32_t IslamicCalendar::trueMonthStart(int32_t month) const
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t start = CalendarCache::get(&gMonthCache, month, status);

    if (start==0) {
        // Make a guess at when the month started, using the average length
        UDate origin = HIJRA_MILLIS 
            + uprv_floor(month * CalendarAstronomer::SYNODIC_MONTH) * kOneDay;

        // moonAge will fail due to memory allocation error
        double age = moonAge(origin, status);
        if (U_FAILURE(status)) {
            goto trueMonthStartEnd;
        }

        if (age >= 0) {
            // The month has already started
            do {
                origin -= kOneDay;
                age = moonAge(origin, status);
                if (U_FAILURE(status)) {
                    goto trueMonthStartEnd;
                }
            } while (age >= 0);
        }
        else {
            // Preceding month has not ended yet.
            do {
                origin += kOneDay;
                age = moonAge(origin, status);
                if (U_FAILURE(status)) {
                    goto trueMonthStartEnd;
                }
            } while (age < 0);
        }
        start = (int32_t)ClockMath::floorDivide((origin - HIJRA_MILLIS), (double)kOneDay) + 1;
        CalendarCache::put(&gMonthCache, month, start, status);
    }
trueMonthStartEnd :
    if(U_FAILURE(status)) {
        start = 0;
    }
    return start;
}

/**
* Return the "age" of the moon at the given time; this is the difference
* in ecliptic latitude between the moon and the sun.  This method simply
* calls CalendarAstronomer.moonAge, converts to degrees, 
* and adjusts the result to be in the range [-180, 180].
*
* @param time  The time at which the moon's age is desired,
*              in millis since 1/1/1970.
*/
double IslamicCalendar::moonAge(UDate time, UErrorCode &status)
{
    double age = 0;

    umtx_lock(&astroLock);
    if(gIslamicCalendarAstro == NULL) {
        gIslamicCalendarAstro = new CalendarAstronomer();
        if (gIslamicCalendarAstro == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return age;
        }
        ucln_i18n_registerCleanup(UCLN_I18N_ISLAMIC_CALENDAR, calendar_islamic_cleanup);
    }
    gIslamicCalendarAstro->setTime(time);
    age = gIslamicCalendarAstro->getMoonAge();
    umtx_unlock(&astroLock);

    // Convert to degrees and normalize...
    age = age * 180 / CalendarAstronomer::PI;
    if (age > 180) {
        age = age - 360;
    }

    return age;
}

//----------------------------------------------------------------------
// Calendar framework
//----------------------------------------------------------------------

/**
* Return the length (in days) of the given month.
*
* @param year  The hijri year
* @param year  The hijri month, 0-based
* @draft ICU 2.4
*/
int32_t IslamicCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {

    int32_t length = 0;

    if (civil == CIVIL) {
        length = 29 + (month+1) % 2;
        if (month == DHU_AL_HIJJAH && civilLeapYear(extendedYear)) {
            length++;
        }
    } else {
        month = 12*(extendedYear-1) + month;
        length =  trueMonthStart(month+1) - trueMonthStart(month) ;
    }
    return length;
}

/**
* Return the number of days in the given Islamic year
* @draft ICU 2.4
*/
int32_t IslamicCalendar::handleGetYearLength(int32_t extendedYear) const {
    if (civil == CIVIL) {
        return 354 + (civilLeapYear(extendedYear) ? 1 : 0);
    } else {
        int32_t month = 12*(extendedYear-1);
        return (trueMonthStart(month + 12) - trueMonthStart(month));
    }
}

//-------------------------------------------------------------------------
// Functions for converting from field values to milliseconds....
//-------------------------------------------------------------------------

// Return JD of start of given month/year
/**
* @draft ICU 2.4
*/
int32_t IslamicCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool /* useMonth */) const {
    return monthStart(eyear, month) + 1948439;
}    

//-------------------------------------------------------------------------
// Functions for converting from milliseconds to field values
//-------------------------------------------------------------------------

/**
* @draft ICU 2.4
*/
int32_t IslamicCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); // Default to year 1
    } else {
        year = internalGet(UCAL_YEAR, 1); // Default to year 1
    }
    return year;
}

/**
* Override Calendar to compute several fields specific to the Islamic
* calendar system.  These are:
*
* <ul><li>ERA
* <li>YEAR
* <li>MONTH
* <li>DAY_OF_MONTH
* <li>DAY_OF_YEAR
* <li>EXTENDED_YEAR</ul>
* 
* The DAY_OF_WEEK and DOW_LOCAL fields are already set when this
* method is called. The getGregorianXxx() methods return Gregorian
* calendar equivalents for the given Julian day.
* @draft ICU 2.4
*/
void IslamicCalendar::handleComputeFields(int32_t julianDay, UErrorCode &status) {
    int32_t year, month, dayOfMonth, dayOfYear;
    UDate startDate;
    int32_t days = julianDay - 1948440;

    if (civil == CIVIL) {
        // Use the civil calendar approximation, which is just arithmetic
        year  = (int)ClockMath::floorDivide( (double)(30 * days + 10646) , 10631.0 );
        month = (int32_t)uprv_ceil((days - 29 - yearStart(year)) / 29.5 );
        month = month<11?month:11;
        startDate = monthStart(year, month);
    } else {
        // Guess at the number of elapsed full months since the epoch
        int32_t months = (int32_t)uprv_floor((double)days / CalendarAstronomer::SYNODIC_MONTH);

        startDate = uprv_floor(months * CalendarAstronomer::SYNODIC_MONTH);

        double age = moonAge(internalGetTime(), status);
        if (U_FAILURE(status)) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return;
        }
        if ( days - startDate >= 25 && age > 0) {
            // If we're near the end of the month, assume next month and search backwards
            months++;
        }

        // Find out the last time that the new moon was actually visible at this longitude
        // This returns midnight the night that the moon was visible at sunset.
        while ((startDate = trueMonthStart(months)) > days) {
            // If it was after the date in question, back up a month and try again
            months--;
        }

        year = months / 12 + 1;
        month = months % 12;
    }

    dayOfMonth = (days - monthStart(year, month)) + 1;

    // Now figure out the day of the year.
    dayOfYear = (days - monthStart(year, 0) + 1);

    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_EXTENDED_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);       
}    

UBool
IslamicCalendar::inDaylightTime(UErrorCode& status) const
{
    // copied from GregorianCalendar
    if (U_FAILURE(status) || (&(getTimeZone()) == NULL && !getTimeZone().useDaylightTime())) 
        return FALSE;

    // Force an update of the state of the Calendar.
    ((IslamicCalendar*)this)->complete(status); // cast away const

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}

// default century
const UDate     IslamicCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   IslamicCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           IslamicCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         IslamicCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool IslamicCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate IslamicCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t IslamicCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
IslamicCalendar::internalGetDefaultCenturyStart() const
{
    // lazy-evaluate systemDefaultCenturyStart
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStart

    return fgSystemDefaultCenturyStart;
}

int32_t
IslamicCalendar::internalGetDefaultCenturyStartYear() const
{
    // lazy-evaluate systemDefaultCenturyStartYear
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStartYear

    return    fgSystemDefaultCenturyStartYear;
}

void
IslamicCalendar::initializeSystemDefaultCentury()
{
    // initialize systemDefaultCentury and systemDefaultCenturyYear based
    // on the current time.  They'll be set to 80 years before
    // the current time.
    UErrorCode status = U_ZERO_ERROR;
    IslamicCalendar calendar(Locale("@calendar=islamic-civil"),status);
    if (U_SUCCESS(status))
    {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
        {
            fgSystemDefaultCenturyStartYear = newYear;
            fgSystemDefaultCenturyStart = newStart;
        }
        umtx_unlock(NULL);
    }
    // We have no recourse upon failure unless we want to propagate the failure
    // out.
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IslamicCalendar)

U_NAMESPACE_END

#endif

