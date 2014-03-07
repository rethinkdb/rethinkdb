/*
******************************************************************************
* Copyright (C) 2003-2008, International Business Machines Corporation
* and others. All Rights Reserved.
******************************************************************************
*
* File HEBRWCAL.CPP
*
* Modification History:
*
*   Date        Name        Description
*   12/03/2003  srl         ported from java HebrewCalendar
*****************************************************************************
*/

#include "hebrwcal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>
#include "gregoimp.h" // Math
#include "astro.h" // CalendarAstronomer
#include "uhash.h"
#include "ucln_in.h"

// Hebrew Calendar implementation

/**
* The absolute date, in milliseconds since 1/1/1970 AD, Gregorian,
* of the start of the Hebrew calendar.  In order to keep this calendar's
* time of day in sync with that of the Gregorian calendar, we use
* midnight, rather than sunset the day before.
*/
//static const double EPOCH_MILLIS = -180799862400000.; // 1/1/1 HY

static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    // Minimum  Greatest    Least  Maximum
    //           Minimum  Maximum
    {        0,        0,        0,        0}, // ERA
    { -5000000, -5000000,  5000000,  5000000}, // YEAR
    {        0,        0,       12,       12}, // MONTH
    {        1,        1,       51,       56}, // WEEK_OF_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // WEEK_OF_MONTH
    {        1,        1,       29,       30}, // DAY_OF_MONTH
    {        1,        1,      353,      385}, // DAY_OF_YEAR
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
    { -5000000, -5000000,  5000000,  5000000}, // YEAR_WOY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // DOW_LOCAL
    { -5000000, -5000000,  5000000,  5000000}, // EXTENDED_YEAR
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // JULIAN_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // MILLISECONDS_IN_DAY
    {/*N/A*/-1,/*N/A*/-1,/*N/A*/-1,/*N/A*/-1}, // IS_LEAP_MONTH
};

/**
* The lengths of the Hebrew months.  This is complicated, because there
* are three different types of years, or six if you count leap years.
* Due to the rules for postponing the start of the year to avoid having
* certain holidays fall on the sabbath, the year can end up being three
* different lengths, called "deficient", "normal", and "complete".
*/
static const int8_t MONTH_LENGTH[][3] = {
    // Deficient  Normal     Complete
    {   30,         30,         30     },           //Tishri
    {   29,         29,         30     },           //Heshvan
    {   29,         30,         30     },           //Kislev
    {   29,         29,         29     },           //Tevet
    {   30,         30,         30     },           //Shevat
    {   30,         30,         30     },           //Adar I (leap years only)
    {   29,         29,         29     },           //Adar
    {   30,         30,         30     },           //Nisan
    {   29,         29,         29     },           //Iyar
    {   30,         30,         30     },           //Sivan
    {   29,         29,         29     },           //Tammuz
    {   30,         30,         30     },           //Av
    {   29,         29,         29     },           //Elul
};

/**
* The cumulative # of days to the end of each month in a non-leap year
* Although this can be calculated from the MONTH_LENGTH table,
* keeping it around separately makes some calculations a lot faster
*/

static const int16_t MONTH_START[][3] = {
    // Deficient  Normal     Complete
    {    0,          0,          0  },          // (placeholder)
    {   30,         30,         30  },          // Tishri
    {   59,         59,         60  },          // Heshvan
    {   88,         89,         90  },          // Kislev
    {  117,        118,        119  },          // Tevet
    {  147,        148,        149  },          // Shevat
    {  147,        148,        149  },          // (Adar I placeholder)
    {  176,        177,        178  },          // Adar
    {  206,        207,        208  },          // Nisan
    {  235,        236,        237  },          // Iyar
    {  265,        266,        267  },          // Sivan
    {  294,        295,        296  },          // Tammuz
    {  324,        325,        326  },          // Av
    {  353,        354,        355  },          // Elul
};

/**
* The cumulative # of days to the end of each month in a leap year
*/
static const int16_t  LEAP_MONTH_START[][3] = {
    // Deficient  Normal     Complete
    {    0,          0,          0  },          // (placeholder)
    {   30,         30,         30  },          // Tishri
    {   59,         59,         60  },          // Heshvan
    {   88,         89,         90  },          // Kislev
    {  117,        118,        119  },          // Tevet
    {  147,        148,        149  },          // Shevat
    {  177,        178,        179  },          // Adar I
    {  206,        207,        208  },          // Adar II
    {  236,        237,        238  },          // Nisan
    {  265,        266,        267  },          // Iyar
    {  295,        296,        297  },          // Sivan
    {  324,        325,        326  },          // Tammuz
    {  354,        355,        356  },          // Av
    {  383,        384,        385  },          // Elul
};

static U_NAMESPACE_QUALIFIER CalendarCache *gCache =  NULL;

U_CDECL_BEGIN
static UBool calendar_hebrew_cleanup(void) {
    delete gCache;
    gCache = NULL;
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
// Constructors...
//-------------------------------------------------------------------------

/**
* Constructs a default <code>HebrewCalendar</code> using the current time
* in the default time zone with the default locale.
* @internal
*/
HebrewCalendar::HebrewCalendar(const Locale& aLocale, UErrorCode& success)
:   Calendar(TimeZone::createDefault(), aLocale, success)

{
    setTimeInMillis(getNow(), success); // Call this again now that the vtable is set up properly.
}


HebrewCalendar::~HebrewCalendar() {
}

const char *HebrewCalendar::getType() const {
    return "hebrew";
}

Calendar* HebrewCalendar::clone() const {
    return new HebrewCalendar(*this);
}

HebrewCalendar::HebrewCalendar(const HebrewCalendar& other) : Calendar(other) {
}


//-------------------------------------------------------------------------
// Rolling and adding functions overridden from Calendar
//
// These methods call through to the default implementation in IBMCalendar
// for most of the fields and only handle the unusual ones themselves.
//-------------------------------------------------------------------------

/**
* Add a signed amount to a specified field, using this calendar's rules.
* For example, to add three days to the current date, you can call
* <code>add(Calendar.DATE, 3)</code>. 
* <p>
* When adding to certain fields, the values of other fields may conflict and
* need to be changed.  For example, when adding one to the {@link #MONTH MONTH} field
* for the date "30 Av 5758", the {@link #DAY_OF_MONTH DAY_OF_MONTH} field
* must be adjusted so that the result is "29 Elul 5758" rather than the invalid
* "30 Elul 5758".
* <p>
* This method is able to add to
* all fields except for {@link #ERA ERA}, {@link #DST_OFFSET DST_OFFSET},
* and {@link #ZONE_OFFSET ZONE_OFFSET}.
* <p>
* <b>Note:</b> You should always use {@link #roll roll} and add rather
* than attempting to perform arithmetic operations directly on the fields
* of a <tt>HebrewCalendar</tt>.  Since the {@link #MONTH MONTH} field behaves
* discontinuously in non-leap years, simple arithmetic can give invalid results.
* <p>
* @param field     the time field.
* @param amount    the amount to add to the field.
*
* @exception   IllegalArgumentException if the field is invalid or refers
*              to a field that cannot be handled by this method.
* @internal
*/
void HebrewCalendar::add(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if(U_FAILURE(status)) {
        return;
    }
    switch (field) {
  case UCAL_MONTH: 
      {
          // We can't just do a set(MONTH, get(MONTH) + amount).  The
          // reason is ADAR_1.  Suppose amount is +2 and we land in
          // ADAR_1 -- then we have to bump to ADAR_2 aka ADAR.  But
          // if amount is -2 and we land in ADAR_1, then we have to
          // bump the other way -- down to SHEVAT.  - Alan 11/00
          int32_t month = get(UCAL_MONTH, status);
          int32_t year = get(UCAL_YEAR, status);
          UBool acrossAdar1;
          if (amount > 0) {
              acrossAdar1 = (month < ADAR_1); // started before ADAR_1?
              month += amount;
              for (;;) {
                  if (acrossAdar1 && month>=ADAR_1 && !isLeapYear(year)) {
                      ++month;
                  }
                  if (month <= ELUL) {
                      break;
                  }
                  month -= ELUL+1;
                  ++year;
                  acrossAdar1 = TRUE;
              }
          } else {
              acrossAdar1 = (month > ADAR_1); // started after ADAR_1?
              month += amount;
              for (;;) {
                  if (acrossAdar1 && month<=ADAR_1 && !isLeapYear(year)) {
                      --month;
                  }
                  if (month >= 0) {
                      break;
                  }
                  month += ELUL+1;
                  --year;
                  acrossAdar1 = TRUE;
              }
          }
          set(UCAL_MONTH, month);
          set(UCAL_YEAR, year);
          pinField(UCAL_DAY_OF_MONTH, status);
          break;
      }

  default:
      Calendar::add(field, amount, status);
      break;
    }
}

/**
* @deprecated ICU 2.6 use UCalendarDateFields instead of EDateFields
*/
void HebrewCalendar::add(EDateFields field, int32_t amount, UErrorCode& status)
{
    add((UCalendarDateFields)field, amount, status);
}

/**
* Rolls (up/down) a specified amount time on the given field.  For
* example, to roll the current date up by three days, you can call
* <code>roll(Calendar.DATE, 3)</code>.  If the
* field is rolled past its maximum allowable value, it will "wrap" back
* to its minimum and continue rolling.  
* For example, calling <code>roll(Calendar.DATE, 10)</code>
* on a Hebrew calendar set to "25 Av 5758" will result in the date "5 Av 5758".
* <p>
* When rolling certain fields, the values of other fields may conflict and
* need to be changed.  For example, when rolling the {@link #MONTH MONTH} field
* upward by one for the date "30 Av 5758", the {@link #DAY_OF_MONTH DAY_OF_MONTH} field
* must be adjusted so that the result is "29 Elul 5758" rather than the invalid
* "30 Elul".
* <p>
* This method is able to roll
* all fields except for {@link #ERA ERA}, {@link #DST_OFFSET DST_OFFSET},
* and {@link #ZONE_OFFSET ZONE_OFFSET}.  Subclasses may, of course, add support for
* additional fields in their overrides of <code>roll</code>.
* <p>
* <b>Note:</b> You should always use roll and {@link #add add} rather
* than attempting to perform arithmetic operations directly on the fields
* of a <tt>HebrewCalendar</tt>.  Since the {@link #MONTH MONTH} field behaves
* discontinuously in non-leap years, simple arithmetic can give invalid results.
* <p>
* @param field     the time field.
* @param amount    the amount by which the field should be rolled.
*
* @exception   IllegalArgumentException if the field is invalid or refers
*              to a field that cannot be handled by this method.
* @internal
*/
void HebrewCalendar::roll(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if(U_FAILURE(status)) {
        return;
    }
    switch (field) {
  case UCAL_MONTH:
      {
          int32_t month = get(UCAL_MONTH, status);
          int32_t year = get(UCAL_YEAR, status);

          UBool leapYear = isLeapYear(year);
          int32_t yearLength = monthsInYear(year);
          int32_t newMonth = month + (amount % yearLength);
          //
          // If it's not a leap year and we're rolling past the missing month
          // of ADAR_1, we need to roll an extra month to make up for it.
          //
          if (!leapYear) {
              if (amount > 0 && month < ADAR_1 && newMonth >= ADAR_1) {
                  newMonth++;
              } else if (amount < 0 && month > ADAR_1 && newMonth <= ADAR_1) {
                  newMonth--;
              }
          }
          set(UCAL_MONTH, (newMonth + 13) % 13);
          pinField(UCAL_DAY_OF_MONTH, status);
          return;
      }
  default:
      Calendar::roll(field, amount, status);
    }
}

void HebrewCalendar::roll(EDateFields field, int32_t amount, UErrorCode& status) {
    roll((UCalendarDateFields)field, amount, status);
}

//-------------------------------------------------------------------------
// Support methods
//-------------------------------------------------------------------------

// Hebrew date calculations are performed in terms of days, hours, and
// "parts" (or halakim), which are 1/1080 of an hour, or 3 1/3 seconds.
static const int32_t HOUR_PARTS = 1080;
static const int32_t DAY_PARTS  = 24*HOUR_PARTS;

// An approximate value for the length of a lunar month.
// It is used to calculate the approximate year and month of a given
// absolute date.
static const int32_t  MONTH_DAYS = 29;
static const int32_t MONTH_FRACT = 12*HOUR_PARTS + 793;
static const int32_t MONTH_PARTS = MONTH_DAYS*DAY_PARTS + MONTH_FRACT;

// The time of the new moon (in parts) on 1 Tishri, year 1 (the epoch)
// counting from noon on the day before.  BAHARAD is an abbreviation of
// Bet (Monday), Hey (5 hours from sunset), Resh-Daled (204).
static const int32_t BAHARAD = 11*HOUR_PARTS + 204;

/**
* Finds the day # of the first day in the given Hebrew year.
* To do this, we want to calculate the time of the Tishri 1 new moon
* in that year.
* <p>
* The algorithm here is similar to ones described in a number of
* references, including:
* <ul>
* <li>"Calendrical Calculations", by Nachum Dershowitz & Edward Reingold,
*     Cambridge University Press, 1997, pages 85-91.
*
* <li>Hebrew Calendar Science and Myths,
*     <a href="http://www.geocities.com/Athens/1584/">
*     http://www.geocities.com/Athens/1584/</a>
*
* <li>The Calendar FAQ,
*      <a href="http://www.faqs.org/faqs/calendars/faq/">
*      http://www.faqs.org/faqs/calendars/faq/</a>
* </ul>
*/
int32_t HebrewCalendar::startOfYear(int32_t year, UErrorCode &status)
{
    ucln_i18n_registerCleanup(UCLN_I18N_HEBREW_CALENDAR, calendar_hebrew_cleanup);
    int32_t day = CalendarCache::get(&gCache, year, status);

    if (day == 0) {
        int32_t months = (235 * year - 234) / 19;           // # of months before year

        int64_t frac = (int64_t)months * MONTH_FRACT + BAHARAD;  // Fractional part of day #
        day  = months * 29 + (int32_t)(frac / DAY_PARTS);        // Whole # part of calculation
        frac = frac % DAY_PARTS;                        // Time of day

        int32_t wd = (day % 7);                        // Day of week (0 == Monday)

        if (wd == 2 || wd == 4 || wd == 6) {
            // If the 1st is on Sun, Wed, or Fri, postpone to the next day
            day += 1;
            wd = (day % 7);
        }
        if (wd == 1 && frac > 15*HOUR_PARTS+204 && !isLeapYear(year) ) {
            // If the new moon falls after 3:11:20am (15h204p from the previous noon)
            // on a Tuesday and it is not a leap year, postpone by 2 days.
            // This prevents 356-day years.
            day += 2;
        }
        else if (wd == 0 && frac > 21*HOUR_PARTS+589 && isLeapYear(year-1) ) {
            // If the new moon falls after 9:32:43 1/3am (21h589p from yesterday noon)
            // on a Monday and *last* year was a leap year, postpone by 1 day.
            // Prevents 382-day years.
            day += 1;
        }
        CalendarCache::put(&gCache, year, day, status);
    }
    return day;
}

/**
* Find the day of the week for a given day
*
* @param day   The # of days since the start of the Hebrew calendar,
*              1-based (i.e. 1/1/1 AM is day 1).
*/
int32_t HebrewCalendar::absoluteDayToDayOfWeek(int32_t day)
{
    // We know that 1/1/1 AM is a Monday, which makes the math easy...
    return (day % 7) + 1;
}

/**
* Returns the the type of a given year.
*  0   "Deficient" year with 353 or 383 days
*  1   "Normal"    year with 354 or 384 days
*  2   "Complete"  year with 355 or 385 days
*/
int32_t HebrewCalendar::yearType(int32_t year) const
{
    int32_t yearLength = handleGetYearLength(year);

    if (yearLength > 380) {
        yearLength -= 30;        // Subtract length of leap month.
    }

    int type = 0;

    switch (yearLength) {
  case 353:
      type = 0; break;
  case 354:
      type = 1; break;
  case 355:
      type = 2; break;
  default:
      //throw new RuntimeException("Illegal year length " + yearLength + " in year " + year);
      type = 1;
    }
    return type;
}

/**
* Determine whether a given Hebrew year is a leap year
*
* The rule here is that if (year % 19) == 0, 3, 6, 8, 11, 14, or 17.
* The formula below performs the same test, believe it or not.
*/
UBool HebrewCalendar::isLeapYear(int32_t year) {
    //return (year * 12 + 17) % 19 >= 12;
    int32_t x = (year*12 + 17) % 19;
    return x >= ((x < 0) ? -7 : 12);
}

int32_t HebrewCalendar::monthsInYear(int32_t year) {
    return isLeapYear(year) ? 13 : 12;
}

//-------------------------------------------------------------------------
// Calendar framework
//-------------------------------------------------------------------------

/**
* @internal
*/
int32_t HebrewCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return LIMITS[field][limitType];
}

/**
* Returns the length of the given month in the given year
* @internal
*/
int32_t HebrewCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {
    // Resolve out-of-range months.  This is necessary in order to
    // obtain the correct year.  We correct to
    // a 12- or 13-month year (add/subtract 12 or 13, depending
    // on the year) but since we _always_ number from 0..12, and
    // the leap year determines whether or not month 5 (Adar 1)
    // is present, we allow 0..12 in any given year.
    while (month < 0) {
        month += monthsInYear(--extendedYear);
    }
    // Careful: allow 0..12 in all years
    while (month > 12) {
        month -= monthsInYear(extendedYear++);
    }

    switch (month) {
    case HESHVAN:
    case KISLEV:
      // These two month lengths can vary
      return MONTH_LENGTH[month][yearType(extendedYear)];

    default:
      // The rest are a fixed length
      return MONTH_LENGTH[month][0];
    }
}

/**
* Returns the number of days in the given Hebrew year
* @internal
*/
int32_t HebrewCalendar::handleGetYearLength(int32_t eyear) const {
    UErrorCode status = U_ZERO_ERROR;
    return startOfYear(eyear+1, status) - startOfYear(eyear, status);
}

//-------------------------------------------------------------------------
// Functions for converting from milliseconds to field values
//-------------------------------------------------------------------------

/**
* Subclasses may override this method to compute several fields
* specific to each calendar system.  These are:
*
* <ul><li>ERA
* <li>YEAR
* <li>MONTH
* <li>DAY_OF_MONTH
* <li>DAY_OF_YEAR
* <li>EXTENDED_YEAR</ul>
* 
* Subclasses can refer to the DAY_OF_WEEK and DOW_LOCAL fields,
* which will be set when this method is called.  Subclasses can
* also call the getGregorianXxx() methods to obtain Gregorian
* calendar equivalents for the given Julian day.
*
* <p>In addition, subclasses should compute any subclass-specific
* fields, that is, fields from BASE_FIELD_COUNT to
* getFieldCount() - 1.
* @internal
*/
void HebrewCalendar::handleComputeFields(int32_t julianDay, UErrorCode &status) {
    int32_t d = julianDay - 347997;
    double m = ((d * (double)DAY_PARTS)/ (double) MONTH_PARTS);         // Months (approx)
    int32_t year = (int32_t)( ((19. * m + 234.) / 235.) + 1.);     // Years (approx)
    int32_t ys  = startOfYear(year, status);                   // 1st day of year
    int32_t dayOfYear = (d - ys);

    // Because of the postponement rules, it's possible to guess wrong.  Fix it.
    while (dayOfYear < 1) {
        year--;
        ys  = startOfYear(year, status);
        dayOfYear = (d - ys);
    }

    // Now figure out which month we're in, and the date within that month
    int32_t type = yearType(year);
    UBool isLeap = isLeapYear(year);

    int32_t month = 0;
    int32_t momax = sizeof(MONTH_START) / (3 * sizeof(MONTH_START[0][0]));
    while (month < momax && dayOfYear > (  isLeap ? LEAP_MONTH_START[month][type] : MONTH_START[month][type] ) ) {
        month++;
    }
    if (month >= momax || month<=0) {
        // TODO: I found dayOfYear could be out of range when
        // a large value is set to julianDay.  I patched startOfYear
        // to reduce the chace, but it could be still reproduced either
        // by startOfYear or other places.  For now, we check
        // the month is in valid range to avoid out of array index
        // access problem here.  However, we need to carefully review
        // the calendar implementation to check the extreme limit of
        // each calendar field and the code works well for any values
        // in the valid value range.  -yoshito
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    month--;
    int dayOfMonth = dayOfYear - (isLeap ? LEAP_MONTH_START[month][type] : MONTH_START[month][type]);

    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_EXTENDED_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);       
}

//-------------------------------------------------------------------------
// Functions for converting from field values to milliseconds
//-------------------------------------------------------------------------

/**
* @internal
*/
int32_t HebrewCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); // Default to year 1
    } else {
        year = internalGet(UCAL_YEAR, 1); // Default to year 1
    }
    return year;
}

/**
* Return JD of start of given month/year.
* @internal
*/
int32_t HebrewCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool /*useMonth*/) const {
    UErrorCode status = U_ZERO_ERROR;
    // Resolve out-of-range months.  This is necessary in order to
    // obtain the correct year.  We correct to
    // a 12- or 13-month year (add/subtract 12 or 13, depending
    // on the year) but since we _always_ number from 0..12, and
    // the leap year determines whether or not month 5 (Adar 1)
    // is present, we allow 0..12 in any given year.
    while (month < 0) {
        month += monthsInYear(--eyear);
    }
    // Careful: allow 0..12 in all years
    while (month > 12) {
        month -= monthsInYear(eyear++);
    }

    int32_t day = startOfYear(eyear, status);

    if(U_FAILURE(status)) {
        return 0;
    }

    if (month != 0) {
        if (isLeapYear(eyear)) {
            day += LEAP_MONTH_START[month][yearType(eyear)];
        } else {
            day += MONTH_START[month][yearType(eyear)];
        }
    }

    return (int) (day + 347997);
}

UBool
HebrewCalendar::inDaylightTime(UErrorCode& status) const
{
    // copied from GregorianCalendar
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
        return FALSE;

    // Force an update of the state of the Calendar.
    ((HebrewCalendar*)this)->complete(status); // cast away const

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}

// default century
const UDate     HebrewCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   HebrewCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           HebrewCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         HebrewCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool HebrewCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate HebrewCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t HebrewCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
HebrewCalendar::internalGetDefaultCenturyStart() const
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
HebrewCalendar::internalGetDefaultCenturyStartYear() const
{
    // lazy-evaluate systemDefaultCenturyStartYear
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStartYear

    return fgSystemDefaultCenturyStartYear;
}

void
HebrewCalendar::initializeSystemDefaultCentury()
{
    // initialize systemDefaultCentury and systemDefaultCenturyYear based
    // on the current time.  They'll be set to 80 years before
    // the current time.
    UErrorCode status = U_ZERO_ERROR;
    HebrewCalendar calendar(Locale("@calendar=hebrew"),status);
    if (U_SUCCESS(status))
    {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury) {
            fgSystemDefaultCenturyStartYear = newYear;
            fgSystemDefaultCenturyStart = newStart;
        }
        umtx_unlock(NULL);
    }
    // We have no recourse upon failure unless we want to propagate the failure
    // out.
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(HebrewCalendar)

U_NAMESPACE_END

#endif // UCONFIG_NO_FORMATTING

