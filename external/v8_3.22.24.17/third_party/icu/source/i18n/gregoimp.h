/*
 **********************************************************************
 * Copyright (c) 2003-2008, International Business Machines
 * Corporation and others.  All Rights Reserved.
 **********************************************************************
 * Author: Alan Liu
 * Created: September 2 2003
 * Since: ICU 2.8
 **********************************************************************
 */

#ifndef GREGOIMP_H
#define GREGOIMP_H
#include "unicode/utypes.h"
#if !UCONFIG_NO_FORMATTING

#include "unicode/ures.h"
#include "unicode/locid.h"
#include "putilimp.h"

U_NAMESPACE_BEGIN

/**
 * A utility class providing mathematical functions used by time zone
 * and calendar code.  Do not instantiate.  Formerly just named 'Math'.
 * @internal
 */
class ClockMath {
 public:
    /**
     * Divide two integers, returning the floor of the quotient.
     * Unlike the built-in division, this is mathematically
     * well-behaved.  E.g., <code>-1/4</code> => 0 but
     * <code>floorDivide(-1,4)</code> => -1.
     * @param numerator the numerator
     * @param denominator a divisor which must be != 0
     * @return the floor of the quotient
     */
    static int32_t floorDivide(int32_t numerator, int32_t denominator);

    /**
     * Divide two numbers, returning the floor of the quotient.
     * Unlike the built-in division, this is mathematically
     * well-behaved.  E.g., <code>-1/4</code> => 0 but
     * <code>floorDivide(-1,4)</code> => -1.
     * @param numerator the numerator
     * @param denominator a divisor which must be != 0
     * @return the floor of the quotient
     */
    static inline double floorDivide(double numerator, double denominator);

    /**
     * Divide two numbers, returning the floor of the quotient and
     * the modulus remainder.  Unlike the built-in division, this is
     * mathematically well-behaved.  E.g., <code>-1/4</code> => 0 and
     * <code>-1%4</code> => -1, but <code>floorDivide(-1,4)</code> =>
     * -1 with <code>remainder</code> => 3.  NOTE: If numerator is
     * too large, the returned quotient may overflow.
     * @param numerator the numerator
     * @param denominator a divisor which must be != 0
     * @param remainder output parameter to receive the
     * remainder. Unlike <code>numerator % denominator</code>, this
     * will always be non-negative, in the half-open range <code>[0,
     * |denominator|)</code>.
     * @return the floor of the quotient
     */
    static int32_t floorDivide(double numerator, int32_t denominator,
                               int32_t& remainder);

    /**
     * For a positive divisor, return the quotient and remainder
     * such that dividend = quotient*divisor + remainder and
     * 0 <= remainder < divisor.
     *
     * Works around edge-case bugs.  Handles pathological input
     * (divident >> divisor) reasonably.
     *
     * Calling with a divisor <= 0 is disallowed.
     */
    static double floorDivide(double dividend, double divisor,
                              double& remainder);
};

// Useful millisecond constants
#define kOneDay    (1.0 * U_MILLIS_PER_DAY)       //  86,400,000
#define kOneHour   (60*60*1000)
#define kOneMinute 60000
#define kOneSecond 1000
#define kOneMillisecond  1
#define kOneWeek   (7.0 * kOneDay) // 604,800,000

// Epoch constants
#define kJan1_1JulianDay  1721426 // January 1, year 1 (Gregorian)

#define kEpochStartAsJulianDay  2440588 // January 1, 1970 (Gregorian)

#define kEpochYear              1970


#define kEarliestViableMillis  -185331720384000000.0  // minimum representable by julian day  -1e17

#define kLatestViableMillis     185753453990400000.0  // max representable by julian day      +1e17

/**
 * The minimum supported Julian day.  This value is equivalent to
 * MIN_MILLIS.
 */
#define MIN_JULIAN (-0x7F000000)

/**
 * The minimum supported epoch milliseconds.  This value is equivalent
 * to MIN_JULIAN.
 */
#define MIN_MILLIS ((MIN_JULIAN - kEpochStartAsJulianDay) * kOneDay)

/**
 * The maximum supported Julian day.  This value is equivalent to
 * MAX_MILLIS.
 */
#define MAX_JULIAN (+0x7F000000)

/**
 * The maximum supported epoch milliseconds.  This value is equivalent
 * to MAX_JULIAN.
 */
#define MAX_MILLIS ((MAX_JULIAN - kEpochStartAsJulianDay) * kOneDay)

/**
 * A utility class providing proleptic Gregorian calendar functions
 * used by time zone and calendar code.  Do not instantiate.
 *
 * Note:  Unlike GregorianCalendar, all computations performed by this
 * class occur in the pure proleptic GregorianCalendar.
 */
class Grego {
 public:
    /**
     * Return TRUE if the given year is a leap year.
     * @param year Gregorian year, with 0 == 1 BCE, -1 == 2 BCE, etc.
     * @return TRUE if the year is a leap year
     */
    static inline UBool isLeapYear(int32_t year);

    /**
     * Return the number of days in the given month.
     * @param year Gregorian year, with 0 == 1 BCE, -1 == 2 BCE, etc.
     * @param month 0-based month, with 0==Jan
     * @return the number of days in the given month
     */
    static inline int8_t monthLength(int32_t year, int32_t month);

    /**
     * Return the length of a previous month of the Gregorian calendar.
     * @param y the extended year
     * @param m the 0-based month number
     * @return the number of days in the month previous to the given month
     */
    static inline int8_t previousMonthLength(int y, int m);

    /**
     * Convert a year, month, and day-of-month, given in the proleptic
     * Gregorian calendar, to 1970 epoch days.
     * @param year Gregorian year, with 0 == 1 BCE, -1 == 2 BCE, etc.
     * @param month 0-based month, with 0==Jan
     * @param dom 1-based day of month
     * @return the day number, with day 0 == Jan 1 1970
     */
    static double fieldsToDay(int32_t year, int32_t month, int32_t dom);
    
    /**
     * Convert a 1970-epoch day number to proleptic Gregorian year,
     * month, day-of-month, and day-of-week.
     * @param day 1970-epoch day (integral value)
     * @param year output parameter to receive year
     * @param month output parameter to receive month (0-based, 0==Jan)
     * @param dom output parameter to receive day-of-month (1-based)
     * @param dow output parameter to receive day-of-week (1-based, 1==Sun)
     * @param doy output parameter to receive day-of-year (1-based)
     */
    static void dayToFields(double day, int32_t& year, int32_t& month,
                            int32_t& dom, int32_t& dow, int32_t& doy);

    /**
     * Convert a 1970-epoch day number to proleptic Gregorian year,
     * month, day-of-month, and day-of-week.
     * @param day 1970-epoch day (integral value)
     * @param year output parameter to receive year
     * @param month output parameter to receive month (0-based, 0==Jan)
     * @param dom output parameter to receive day-of-month (1-based)
     * @param dow output parameter to receive day-of-week (1-based, 1==Sun)
     */
    static inline void dayToFields(double day, int32_t& year, int32_t& month,
                                   int32_t& dom, int32_t& dow);

    /**
     * Convert a 1970-epoch milliseconds to proleptic Gregorian year,
     * month, day-of-month, and day-of-week, day of year and millis-in-day.
     * @param time 1970-epoch milliseconds
     * @param year output parameter to receive year
     * @param month output parameter to receive month (0-based, 0==Jan)
     * @param dom output parameter to receive day-of-month (1-based)
     * @param dow output parameter to receive day-of-week (1-based, 1==Sun)
     * @param doy output parameter to receive day-of-year (1-based)
     * @param mid output parameter to recieve millis-in-day
     */
    static void timeToFields(UDate time, int32_t& year, int32_t& month,
                            int32_t& dom, int32_t& dow, int32_t& doy, int32_t& mid);

    /**
     * Return the day of week on the 1970-epoch day
     * @param day the 1970-epoch day (integral value)
     * @return the day of week
     */
    static int32_t dayOfWeek(double day);

    /**
     * Returns the ordinal number for the specified day of week within the month.
     * The valid return value is 1, 2, 3, 4 or -1.
     * @param year Gregorian year, with 0 == 1 BCE, -1 == 2 BCE, etc.
     * @param month 0-based month, with 0==Jan
     * @param dom 1-based day of month
     * @return The ordinal number for the specified day of week within the month
     */
    static int32_t dayOfWeekInMonth(int32_t year, int32_t month, int32_t dom);

    /**
     * Converts Julian day to time as milliseconds.
     * @param julian the given Julian day number.
     * @return time as milliseconds.
     * @internal
     */
    static inline double julianDayToMillis(int32_t julian);

    /**
     * Converts time as milliseconds to Julian day.
     * @param millis the given milliseconds.
     * @return the Julian day number.
     * @internal
     */
    static inline int32_t millisToJulianDay(double millis);

    /** 
     * Calculates the Gregorian day shift value for an extended year.
     * @param eyear Extended year 
     * @returns number of days to ADD to Julian in order to convert from J->G
     */
    static inline int32_t gregorianShift(int32_t eyear);

 private:
    static const int16_t DAYS_BEFORE[24];
    static const int8_t MONTH_LENGTH[24];
};

inline double ClockMath::floorDivide(double numerator, double denominator) {
    return uprv_floor(numerator / denominator);
}

inline UBool Grego::isLeapYear(int32_t year) {
    // year&0x3 == year%4
    return ((year&0x3) == 0) && ((year%100 != 0) || (year%400 == 0));
}

inline int8_t
Grego::monthLength(int32_t year, int32_t month) {
    return MONTH_LENGTH[month + (isLeapYear(year) ? 12 : 0)];
}

inline int8_t
Grego::previousMonthLength(int y, int m) {
  return (m > 0) ? monthLength(y, m-1) : 31;
}

inline void Grego::dayToFields(double day, int32_t& year, int32_t& month,
                               int32_t& dom, int32_t& dow) {
  int32_t doy_unused;
  dayToFields(day,year,month,dom,dow,doy_unused);
}

inline double Grego::julianDayToMillis(int32_t julian)
{
  return (julian - kEpochStartAsJulianDay) * kOneDay;
}

inline int32_t Grego::millisToJulianDay(double millis) {
  return (int32_t) (kEpochStartAsJulianDay + ClockMath::floorDivide(millis, (double)kOneDay));
}

inline int32_t Grego::gregorianShift(int32_t eyear) {
  int32_t y = eyear-1;
  int32_t gregShift = ClockMath::floorDivide(y, 400) - ClockMath::floorDivide(y, 100) + 2;
  return gregShift;
}

/**
 * This utility class provides convenient access to the data needed for a calendar. 
 * @internal ICU 3.0
 */
class CalendarData : public UMemory {
public: 
    /**
     * Construct a CalendarData from the given locale.
     * @param loc locale to use. The 'calendar' keyword will be ignored.
     * @param type calendar type. NULL indicates the gregorian calendar. 
     * No default lookup is done.
     * @param status error code
     */
    CalendarData(const Locale& loc, const char *type, UErrorCode& status);

    /**
     * Load data for calendar. Note, this object owns the resources, do NOT call ures_close()!
     * The ResourceBundle C++ API should NOT be used because it is too slow for a low level API.
     *
     * @param key Resource key to data
     * @param status Error Status
     * @internal
     */
    UResourceBundle* getByKey(const char *key, UErrorCode& status);

    /**
     * Load data for calendar. Note, this object owns the resources, do NOT call ures_close()!
     * There is an implicit key of 'format'
     * data is located in:   "calendar/key/format/subKey"
     * for example,  calendar/dayNames/format/abbreviated
     * The ResourceBundle C++ API should NOT be used because it is too slow for a low level API.
     *
     * @param key Resource key to data
     * @param subKey Resource key to data
     * @param status Error Status
     * @internal
     */
    UResourceBundle* getByKey2(const char *key, const char *subKey, UErrorCode& status);

    /**
     * Load data for calendar. Note, this object owns the resources, do NOT call ures_close()!
     * data is located in:   "calendar/key/contextKey/subKey"
     * for example,  calendar/dayNames/standalone/narrow
     * The ResourceBundle C++ API should NOT be used because it is too slow for a low level API.
     *
     * @param key Resource key to data
     * @param contextKey Resource key to data
     * @param subKey Resource key to data
     * @param status Error Status
     * @internal
     */
    UResourceBundle* getByKey3(const char *key, const char *contextKey, const char *subKey, UErrorCode& status);

    ~CalendarData();

private:
    void initData(const char *locale, const char *type, UErrorCode& status);

    UResourceBundle *fFillin;
    UResourceBundle *fOtherFillin;
    UResourceBundle *fBundle;
    UResourceBundle *fFallback;
    CalendarData(); // Not implemented.
};

U_NAMESPACE_END

#endif // !UCONFIG_NO_FORMATTING
#endif // GREGOIMP_H

//eof
