/*
 ******************************************************************************
 * Copyright (C) 2003-2008, International Business Machines Corporation
 * and others. All Rights Reserved.
 ******************************************************************************
 *
 * File PERSNCAL.CPP
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   9/23/2003 mehran        posted to icu-design
 *****************************************************************************
 */

#include "persncal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>

static const int8_t monthDays[] = { 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29 };

static int32_t
jalali_to_julian(int year, int month, int day) 
{
    int32_t daysNo=0;
    int i;

    year = year -475+2820;
    month -= 1;

    daysNo=(year/2820)*1029983;
    year=year % 2820;  

    daysNo+=(year/128)* 46751;
    if((year/128)>21)
    {
        daysNo-=46751;
        year=(year%128)+128;
    }
    else
        year=year%128;

    if(year>=29)
    {
        year-=29;
        daysNo+=10592;
    }

    if(year>=66)
    {
        year-=66;
        daysNo+=24106;
    }
    else if( year>=33)
    {
        daysNo+=(year/33)* 12053;
        year=year%33;
    }

    if (year >= 5)
    {
        daysNo += 1826;
        year -=5;
    }
    else if (year == 4)
    {
        daysNo += 1460;
        year -=4;
    }

    daysNo += 1461 * (year/4);
    year %= 4;
    daysNo += 365 * year;

    for (i = 0; i < month; i++) {
        daysNo += monthDays[i];
    }

    daysNo += day;

    return daysNo-856493;
}

static void julian_to_jalali (int32_t daysNo, int *h_y, int *h_m, int *h_d) 
{
    int year=0, month=0, day=0,scalarDays=0;
    int i;

    daysNo+=856493;
    scalarDays=daysNo;
    year=(daysNo/1029983)*2820;
    daysNo=daysNo%1029983;

    if((daysNo/46751)<=21)
    {
        year+=(daysNo/46751)* 128;
        daysNo=daysNo%46751;
    }
    else
    {
        year+=(daysNo/46751)* 128;
        daysNo=daysNo%46751;
        year-=128;
        daysNo+=46751;
    }

    if (daysNo >= 10592)
    {
        year+= 29;
        daysNo -= 10592;
    }

    if(daysNo>=24106)
    {
        daysNo-=24106;
        year+=66;
    }

    if(daysNo>=12053)
    {
        daysNo-=12053;
        year+=33;
    }


    if (daysNo >= 1826)
    {
        year+= 5;
        daysNo -= 1826;
    }
    else if (daysNo > 1095)
    {
        year+= 3;
        daysNo -= 1095;

    }

    year +=(4 * (daysNo/1461));
    daysNo %= 1461;

    if (daysNo == 0)
    {
        year -= 1;
        daysNo = 366;
    }
    else
    {
        year += daysNo/365;
        daysNo = daysNo % 365;
        if (daysNo == 0)
        {
            year -= 1;
            daysNo = 365;
        }

    }

    for (i = 0; i < 11 && daysNo > monthDays[i]; ++i) {
        daysNo -= monthDays[i];
    }

    month = i + 1;

    day = daysNo;

    *h_d = day;
    *h_m = month;
    *h_y = year-2345;
}

U_NAMESPACE_BEGIN

// Implementation of the PersianCalendar class

//-------------------------------------------------------------------------
// Constructors...
//-------------------------------------------------------------------------

const char *PersianCalendar::getType() const { 
    return "persian";
}

Calendar* PersianCalendar::clone() const {
    return new PersianCalendar(*this);
}

PersianCalendar::PersianCalendar(const Locale& aLocale, UErrorCode& success)
  :   Calendar(TimeZone::createDefault(), aLocale, success)
{
    setTimeInMillis(getNow(), success); // Call this again now that the vtable is set up properly.
}

PersianCalendar::PersianCalendar(const PersianCalendar& other) : Calendar(other) {
}

PersianCalendar::~PersianCalendar()
{
}

//-------------------------------------------------------------------------
// Minimum / Maximum access functions
//-------------------------------------------------------------------------

static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    // Minimum  Greatest     Least   Maximum
    //           Minimum   Maximum
    {        0,        0,        0,        0}, // ERA
    { -5000000, -5000000,  5000000,  5000000}, // YEAR
    {        0,        0,       11,       11}, // MONTH
    {        1,        1,       52,       53}, // WEEK_OF_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // WEEK_OF_MONTH
    {        1,       1,        29,       31}, // DAY_OF_MONTH
    {        1,       1,       365,      366}, // DAY_OF_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DAY_OF_WEEK
    {        1,       1,         5,        5}, // DAY_OF_WEEK_IN_MONTH
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // AM_PM
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // HOUR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // HOUR_OF_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MINUTE
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // SECOND
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MILLISECOND
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // ZONE_OFFSET
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DST_OFFSET
    { -5000000, -5000000,  5000000,  5000000}, // YEAR_WOY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DOW_LOCAL
    { -5000000, -5000000,  5000000,  5000000}, // EXTENDED_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // JULIAN_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MILLISECONDS_IN_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // IS_LEAP_MONTH
};
static const int32_t MONTH_COUNT[12][4]  = {
    //len len2   st  st2
    {  31,  31,   0,   0 }, // Farvardin
    {  31,  31,  31,  31 }, // Ordibehesht
    {  31,  31,  62,  62 }, // Khordad
    {  31,  31,  93,  93 }, // Tir
    {  31,  31, 124, 124 }, // Mordad
    {  31,  31, 155, 155 }, // Shahrivar
    {  30,  30, 186, 186 }, // Mehr
    {  30,  30, 216, 216 }, // Aban
    {  30,  30, 246, 246 }, // Azar
    {  30,  30, 276, 276 }, // Dey
    {  30,  30, 306, 306 }, // Bahman
    {  29,  30, 336, 336 }  // Esfand
    // len  length of month
    // len2 length of month in a leap year
    // st   days in year before start of month
    // st2  days in year before month in leap year
};

int32_t PersianCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return LIMITS[field][limitType];
}

//-------------------------------------------------------------------------
// Assorted calculation utilities
//

/**
 * Determine whether a year is a leap year in the Persian calendar
 */
UBool PersianCalendar::isLeapYear(int32_t year)
{
    return jalali_to_julian(year+1,1,1)-jalali_to_julian(year,1,1) == 366;
}
    
/**
 * Return the day # on which the given year starts.  Days are counted
 * from the Hijri epoch, origin 0.
 */
int32_t PersianCalendar::yearStart(int32_t year) {
    return handleComputeMonthStart(year,1,FALSE);
}
    
/**
 * Return the day # on which the given month starts.  Days are counted
 * from the Hijri epoch, origin 0.
 *
 * @param year  The hijri shamsi year
 * @param year  The hijri shamsi month, 0-based
 */
int32_t PersianCalendar::monthStart(int32_t year, int32_t month) const {
    return handleComputeMonthStart(year,month,FALSE);
}
    
//----------------------------------------------------------------------
// Calendar framework
//----------------------------------------------------------------------

/**
 * Return the length (in days) of the given month.
 *
 * @param year  The hijri shamsi year
 * @param year  The hijri shamsi month, 0-based
 */
int32_t PersianCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {
    return MONTH_COUNT[month][PersianCalendar::isLeapYear(extendedYear)?1:0];
}

/**
 * Return the number of days in the given Persian year
 */
int32_t PersianCalendar::handleGetYearLength(int32_t extendedYear) const {
    return 365 + (PersianCalendar::isLeapYear(extendedYear) ? 1 : 0);
}
    
//-------------------------------------------------------------------------
// Functions for converting from field values to milliseconds....
//-------------------------------------------------------------------------

// Return JD of start of given month/year
int32_t PersianCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool useMonth) const {
    // If the month is out of range, adjust it into range, and
    // modify the extended year value accordingly.
    if (month < 0 || month > 11) {
        eyear += month / 12;
        month = month % 12;
    }
    return jalali_to_julian(eyear,(useMonth?month+1:1),1)-1+1947955; 
}

//-------------------------------------------------------------------------
// Functions for converting from milliseconds to field values
//-------------------------------------------------------------------------

int32_t PersianCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); // Default to year 1
    } else {
        year = internalGet(UCAL_YEAR, 1); // Default to year 1
    }
    return year;
}

/**
 * Override Calendar to compute several fields specific to the Persian
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
 */
void PersianCalendar::handleComputeFields(int32_t julianDay, UErrorCode &/*status*/) {
    int jy,jm,jd;        
    julian_to_jalali(julianDay-1947955,&jy,&jm,&jd);
    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, jy);
    internalSet(UCAL_EXTENDED_YEAR, jy);
    internalSet(UCAL_MONTH, jm-1);
    internalSet(UCAL_DAY_OF_MONTH, jd);
    internalSet(UCAL_DAY_OF_YEAR, jd + MONTH_COUNT[jm-1][2]);
}    

UBool
PersianCalendar::inDaylightTime(UErrorCode& status) const
{
    // copied from GregorianCalendar
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
        return FALSE;

    // Force an update of the state of the Calendar.
    ((PersianCalendar*)this)->complete(status); // cast away const

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}

// default century
const UDate     PersianCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   PersianCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           PersianCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         PersianCalendar::fgSystemDefaultCenturyStartYear   = -1;

UBool PersianCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate PersianCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t PersianCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
PersianCalendar::internalGetDefaultCenturyStart() const
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
PersianCalendar::internalGetDefaultCenturyStartYear() const
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
PersianCalendar::initializeSystemDefaultCentury()
{
    // initialize systemDefaultCentury and systemDefaultCenturyYear based
    // on the current time.  They'll be set to 80 years before
    // the current time.
    UErrorCode status = U_ZERO_ERROR;
    PersianCalendar calendar(Locale("@calendar=persian"),status);
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

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(PersianCalendar)

U_NAMESPACE_END

#endif

