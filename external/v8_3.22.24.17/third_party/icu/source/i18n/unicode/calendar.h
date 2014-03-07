/*
********************************************************************************
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File CALENDAR.H
*
* Modification History:
*
*   Date        Name        Description
*   04/22/97    aliu        Expanded and corrected comments and other header
*                           contents.
*   05/01/97    aliu        Made equals(), before(), after() arguments const.
*   05/20/97    aliu        Replaced fAreFieldsSet with fAreFieldsInSync and
*                           fAreAllFieldsSet.
*   07/27/98    stephen     Sync up with JDK 1.2
*   11/15/99    weiv        added YEAR_WOY and DOW_LOCAL
*                           to EDateFields
*    8/19/2002  srl         Removed Javaisms
*   11/07/2003  srl         Update, clean up documentation.
********************************************************************************
*/

#ifndef CALENDAR_H
#define CALENDAR_H

#include "unicode/utypes.h"

/**
 * \file 
 * \brief C++ API: Calendar object
 */
#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/locid.h"
#include "unicode/timezone.h"
#include "unicode/ucal.h"
#include "unicode/umisc.h"

U_NAMESPACE_BEGIN

class ICUServiceFactory;

/**
 * @internal
 */
typedef int32_t UFieldResolutionTable[12][8];

/**
 * <code>Calendar</code> is an abstract base class for converting between
 * a <code>UDate</code> object and a set of integer fields such as
 * <code>YEAR</code>, <code>MONTH</code>, <code>DAY</code>, <code>HOUR</code>,
 * and so on. (A <code>UDate</code> object represents a specific instant in
 * time with millisecond precision. See UDate
 * for information about the <code>UDate</code> class.)
 *
 * <p>
 * Subclasses of <code>Calendar</code> interpret a <code>UDate</code>
 * according to the rules of a specific calendar system.
 * The most commonly used subclass of <code>Calendar</code> is
 * <code>GregorianCalendar</code>. Other subclasses could represent
 * the various types of lunar calendars in use in many parts of the world.
 *
 * <p>
 * <b>NOTE</b>: (ICU 2.6) The subclass interface should be considered unstable
 * - it WILL change.
 *
 * <p>
 * Like other locale-sensitive classes, <code>Calendar</code> provides a
 * static method, <code>createInstance</code>, for getting a generally useful
 * object of this type. <code>Calendar</code>'s <code>createInstance</code> method
 * returns the appropriate <code>Calendar</code> subclass whose
 * time fields have been initialized with the current date and time:
 * \htmlonly<blockquote>\endhtmlonly
 * <pre>
 * Calendar *rightNow = Calendar::createInstance(errCode);
 * </pre>
 * \htmlonly</blockquote>\endhtmlonly
 *
 * <p>
 * A <code>Calendar</code> object can produce all the time field values
 * needed to implement the date-time formatting for a particular language
 * and calendar style (for example, Japanese-Gregorian, Japanese-Traditional).
 *
 * <p>
 * When computing a <code>UDate</code> from time fields, two special circumstances
 * may arise: there may be insufficient information to compute the
 * <code>UDate</code> (such as only year and month but no day in the month),
 * or there may be inconsistent information (such as "Tuesday, July 15, 1996"
 * -- July 15, 1996 is actually a Monday).
 *
 * <p>
 * <strong>Insufficient information.</strong> The calendar will use default
 * information to specify the missing fields. This may vary by calendar; for
 * the Gregorian calendar, the default for a field is the same as that of the
 * start of the epoch: i.e., YEAR = 1970, MONTH = JANUARY, DATE = 1, etc.
 *
 * <p>
 * <strong>Inconsistent information.</strong> If fields conflict, the calendar
 * will give preference to fields set more recently. For example, when
 * determining the day, the calendar will look for one of the following
 * combinations of fields.  The most recent combination, as determined by the
 * most recently set single field, will be used.
 *
 * \htmlonly<blockquote>\endhtmlonly
 * <pre>
 * MONTH + DAY_OF_MONTH
 * MONTH + WEEK_OF_MONTH + DAY_OF_WEEK
 * MONTH + DAY_OF_WEEK_IN_MONTH + DAY_OF_WEEK
 * DAY_OF_YEAR
 * DAY_OF_WEEK + WEEK_OF_YEAR
 * </pre>
 * \htmlonly</blockquote>\endhtmlonly
 *
 * For the time of day:
 *
 * \htmlonly<blockquote>\endhtmlonly
 * <pre>
 * HOUR_OF_DAY
 * AM_PM + HOUR
 * </pre>
 * \htmlonly</blockquote>\endhtmlonly
 *
 * <p>
 * <strong>Note:</strong> for some non-Gregorian calendars, different
 * fields may be necessary for complete disambiguation. For example, a full
 * specification of the historial Arabic astronomical calendar requires year,
 * month, day-of-month <em>and</em> day-of-week in some cases.
 *
 * <p>
 * <strong>Note:</strong> There are certain possible ambiguities in
 * interpretation of certain singular times, which are resolved in the
 * following ways:
 * <ol>
 *     <li> 24:00:00 "belongs" to the following day. That is,
 *          23:59 on Dec 31, 1969 &lt; 24:00 on Jan 1, 1970 &lt; 24:01:00 on Jan 1, 1970
 *
 *     <li> Although historically not precise, midnight also belongs to "am",
 *          and noon belongs to "pm", so on the same day,
 *          12:00 am (midnight) &lt; 12:01 am, and 12:00 pm (noon) &lt; 12:01 pm
 * </ol>
 *
 * <p>
 * The date or time format strings are not part of the definition of a
 * calendar, as those must be modifiable or overridable by the user at
 * runtime. Use {@link DateFormat}
 * to format dates.
 *
 * <p>
 * <code>Calendar</code> provides an API for field "rolling", where fields
 * can be incremented or decremented, but wrap around. For example, rolling the
 * month up in the date <code>December 12, <b>1996</b></code> results in
 * <code>January 12, <b>1996</b></code>.
 *
 * <p>
 * <code>Calendar</code> also provides a date arithmetic function for
 * adding the specified (signed) amount of time to a particular time field.
 * For example, subtracting 5 days from the date <code>September 12, 1996</code>
 * results in <code>September 7, 1996</code>.
 *
 * @stable ICU 2.0
 */
class U_I18N_API Calendar : public UObject {
public:

    /**
     * Field IDs for date and time. Used to specify date/time fields. ERA is calendar
     * specific. Example ranges given are for illustration only; see specific Calendar
     * subclasses for actual ranges.
     * @deprecated ICU 2.6. Use C enum UCalendarDateFields defined in ucal.h
     */
    enum EDateFields {
#ifndef U_HIDE_DEPRECATED_API
/*
 * ERA may be defined on other platforms. To avoid any potential problems undefined it here.
 */
#ifdef ERA
#undef ERA
#endif
        ERA,                  // Example: 0..1
        YEAR,                 // Example: 1..big number
        MONTH,                // Example: 0..11
        WEEK_OF_YEAR,         // Example: 1..53
        WEEK_OF_MONTH,        // Example: 1..4
        DATE,                 // Example: 1..31
        DAY_OF_YEAR,          // Example: 1..365
        DAY_OF_WEEK,          // Example: 1..7
        DAY_OF_WEEK_IN_MONTH, // Example: 1..4, may be specified as -1
        AM_PM,                // Example: 0..1
        HOUR,                 // Example: 0..11
        HOUR_OF_DAY,          // Example: 0..23
        MINUTE,               // Example: 0..59
        SECOND,               // Example: 0..59
        MILLISECOND,          // Example: 0..999
        ZONE_OFFSET,          // Example: -12*U_MILLIS_PER_HOUR..12*U_MILLIS_PER_HOUR
        DST_OFFSET,           // Example: 0 or U_MILLIS_PER_HOUR
        YEAR_WOY,             // 'Y' Example: 1..big number - Year of Week of Year
        DOW_LOCAL,            // 'e' Example: 1..7 - Day of Week / Localized
        
        EXTENDED_YEAR,
        JULIAN_DAY,
        MILLISECONDS_IN_DAY,
        IS_LEAP_MONTH,

        FIELD_COUNT = UCAL_FIELD_COUNT // See ucal.h for other fields.
#endif /* U_HIDE_DEPRECATED_API */
    };

    /**
     * Useful constant for days of week. Note: Calendar day-of-week is 1-based. Clients
     * who create locale resources for the field of first-day-of-week should be aware of
     * this. For instance, in US locale, first-day-of-week is set to 1, i.e., SUNDAY.
     * @deprecated ICU 2.6. Use C enum UCalendarDaysOfWeek defined in ucal.h
     */
    enum EDaysOfWeek {
#ifndef U_HIDE_DEPRECATED_API
        SUNDAY = 1,
        MONDAY,
        TUESDAY,
        WEDNESDAY,
        THURSDAY,
        FRIDAY,
        SATURDAY
#endif /* U_HIDE_DEPRECATED_API */
    };

    /**
     * Useful constants for month. Note: Calendar month is 0-based.
     * @deprecated ICU 2.6. Use C enum UCalendarMonths defined in ucal.h
     */
    enum EMonths {
#ifndef U_HIDE_DEPRECATED_API
        JANUARY,
        FEBRUARY,
        MARCH,
        APRIL,
        MAY,
        JUNE,
        JULY,
        AUGUST,
        SEPTEMBER,
        OCTOBER,
        NOVEMBER,
        DECEMBER,
        UNDECIMBER
#endif /* U_HIDE_DEPRECATED_API */
    };

    /**
     * Useful constants for hour in 12-hour clock. Used in GregorianCalendar.
     * @deprecated ICU 2.6. Use C enum UCalendarAMPMs defined in ucal.h
     */
    enum EAmpm {
#ifndef U_HIDE_DEPRECATED_API
        AM,
        PM
#endif /* U_HIDE_DEPRECATED_API */
    };

    /**
     * destructor
     * @stable ICU 2.0
     */
    virtual ~Calendar();

    /**
     * Create and return a polymorphic copy of this calendar.
     *
     * @return    a polymorphic copy of this calendar.
     * @stable ICU 2.0
     */
    virtual Calendar* clone(void) const = 0;

    /**
     * Creates a Calendar using the default timezone and locale. Clients are responsible
     * for deleting the object returned.
     *
     * @param success  Indicates the success/failure of Calendar creation. Filled in
     *                 with U_ZERO_ERROR if created successfully, set to a failure result
     *                 otherwise. U_MISSING_RESOURCE_ERROR will be returned if the resource data
     *                 requests a calendar type which has not been installed.
     * @return         A Calendar if created successfully. NULL otherwise.
     * @stable ICU 2.0
     */
    static Calendar* U_EXPORT2 createInstance(UErrorCode& success);

    /**
     * Creates a Calendar using the given timezone and the default locale.
     * The Calendar takes ownership of zoneToAdopt; the
     * client must not delete it.
     *
     * @param zoneToAdopt  The given timezone to be adopted.
     * @param success      Indicates the success/failure of Calendar creation. Filled in
     *                     with U_ZERO_ERROR if created successfully, set to a failure result
     *                     otherwise.
     * @return             A Calendar if created successfully. NULL otherwise.
     * @stable ICU 2.0
     */
    static Calendar* U_EXPORT2 createInstance(TimeZone* zoneToAdopt, UErrorCode& success);

    /**
     * Creates a Calendar using the given timezone and the default locale.  The TimeZone
     * is _not_ adopted; the client is still responsible for deleting it.
     *
     * @param zone  The timezone.
     * @param success      Indicates the success/failure of Calendar creation. Filled in
     *                     with U_ZERO_ERROR if created successfully, set to a failure result
     *                     otherwise.
     * @return             A Calendar if created successfully. NULL otherwise.
     * @stable ICU 2.0
     */
    static Calendar* U_EXPORT2 createInstance(const TimeZone& zone, UErrorCode& success);

    /**
     * Creates a Calendar using the default timezone and the given locale.
     *
     * @param aLocale  The given locale.
     * @param success  Indicates the success/failure of Calendar creation. Filled in
     *                 with U_ZERO_ERROR if created successfully, set to a failure result
     *                 otherwise.
     * @return         A Calendar if created successfully. NULL otherwise.
     * @stable ICU 2.0
     */
    static Calendar* U_EXPORT2 createInstance(const Locale& aLocale, UErrorCode& success);

    /**
     * Creates a Calendar using the given timezone and given locale.
     * The Calendar takes ownership of zoneToAdopt; the
     * client must not delete it.
     *
     * @param zoneToAdopt  The given timezone to be adopted.
     * @param aLocale      The given locale.
     * @param success      Indicates the success/failure of Calendar creation. Filled in
     *                     with U_ZERO_ERROR if created successfully, set to a failure result
     *                     otherwise.
     * @return             A Calendar if created successfully. NULL otherwise.
     * @stable ICU 2.0
     */
    static Calendar* U_EXPORT2 createInstance(TimeZone* zoneToAdopt, const Locale& aLocale, UErrorCode& success);

    /**
     * Gets a Calendar using the given timezone and given locale.  The TimeZone
     * is _not_ adopted; the client is still responsible for deleting it.
     *
     * @param zoneToAdopt  The given timezone to be adopted.
     * @param aLocale      The given locale.
     * @param success      Indicates the success/failure of Calendar creation. Filled in
     *                     with U_ZERO_ERROR if created successfully, set to a failure result
     *                     otherwise.
     * @return             A Calendar if created successfully. NULL otherwise.
     * @stable ICU 2.0
     */
    static Calendar* U_EXPORT2 createInstance(const TimeZone& zoneToAdopt, const Locale& aLocale, UErrorCode& success);

    /**
     * Returns a list of the locales for which Calendars are installed.
     *
     * @param count  Number of locales returned.
     * @return       An array of Locale objects representing the set of locales for which
     *               Calendars are installed.  The system retains ownership of this list;
     *               the caller must NOT delete it. Does not include user-registered Calendars.
     * @stable ICU 2.0
     */
    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);


    /**
     * Given a key and a locale, returns an array of string values in a preferred
     * order that would make a difference. These are all and only those values where
     * the open (creation) of the service with the locale formed from the input locale
     * plus input keyword and that value has different behavior than creation with the
     * input locale alone.
     * @param key           one of the keys supported by this service.  For now, only
     *                      "calendar" is supported.
     * @param locale        the locale
     * @param commonlyUsed  if set to true it will return only commonly used values
     *                      with the given locale in preferred order.  Otherwise,
     *                      it will return all the available values for the locale.
     * @param status        ICU Error Code
     * @return a string enumeration over keyword values for the given key and the locale.
     * @stable ICU 4.2
     */
    static StringEnumeration* U_EXPORT2 getKeywordValuesForLocale(const char* key,
                    const Locale& locale, UBool commonlyUsed, UErrorCode& status);

    /**
     * Returns the current UTC (GMT) time measured in milliseconds since 0:00:00 on 1/1/70
     * (derived from the system time).
     *
     * @return   The current UTC time in milliseconds.
     * @stable ICU 2.0
     */
    static UDate U_EXPORT2 getNow(void);

    /**
     * Gets this Calendar's time as milliseconds. May involve recalculation of time due
     * to previous calls to set time field values. The time specified is non-local UTC
     * (GMT) time. Although this method is const, this object may actually be changed
     * (semantically const).
     *
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @return        The current time in UTC (GMT) time, or zero if the operation
     *                failed.
     * @stable ICU 2.0
     */
    inline UDate getTime(UErrorCode& status) const { return getTimeInMillis(status); }

    /**
     * Sets this Calendar's current time with the given UDate. The time specified should
     * be in non-local UTC (GMT) time.
     *
     * @param date  The given UDate in UTC (GMT) time.
     * @param status  Output param set to success/failure code on exit. If any value
     *                set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.0
     */
    inline void setTime(UDate date, UErrorCode& status) { setTimeInMillis(date, status); }

    /**
     * Compares the equality of two Calendar objects. Objects of different subclasses
     * are considered unequal. This comparison is very exacting; two Calendar objects
     * must be in exactly the same state to be considered equal. To compare based on the
     * represented time, use equals() instead.
     *
     * @param that  The Calendar object to be compared with.
     * @return      True if the given Calendar is the same as this Calendar; false
     *              otherwise.
     * @stable ICU 2.0
     */
    virtual UBool operator==(const Calendar& that) const;

    /**
     * Compares the inequality of two Calendar objects.
     *
     * @param that  The Calendar object to be compared with.
     * @return      True if the given Calendar is not the same as this Calendar; false
     *              otherwise.
     * @stable ICU 2.0
     */
    UBool operator!=(const Calendar& that) const {return !operator==(that);}

    /**
     * Returns TRUE if the given Calendar object is equivalent to this
     * one.  An equivalent Calendar will behave exactly as this one
     * does, but it may be set to a different time.  By contrast, for
     * the operator==() method to return TRUE, the other Calendar must
     * be set to the same time.
     *
     * @param other the Calendar to be compared with this Calendar
     * @stable ICU 2.4
     */
    virtual UBool isEquivalentTo(const Calendar& other) const;

    /**
     * Compares the Calendar time, whereas Calendar::operator== compares the equality of
     * Calendar objects.
     *
     * @param when    The Calendar to be compared with this Calendar. Although this is a
     *                const parameter, the object may be modified physically
     *                (semantically const).
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @return        True if the current time of this Calendar is equal to the time of
     *                Calendar when; false otherwise.
     * @stable ICU 2.0
     */
    UBool equals(const Calendar& when, UErrorCode& status) const;

    /**
     * Returns true if this Calendar's current time is before "when"'s current time.
     *
     * @param when    The Calendar to be compared with this Calendar. Although this is a
     *                const parameter, the object may be modified physically
     *                (semantically const).
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @return        True if the current time of this Calendar is before the time of
     *                Calendar when; false otherwise.
     * @stable ICU 2.0
     */
    UBool before(const Calendar& when, UErrorCode& status) const;

    /**
     * Returns true if this Calendar's current time is after "when"'s current time.
     *
     * @param when    The Calendar to be compared with this Calendar. Although this is a
     *                const parameter, the object may be modified physically
     *                (semantically const).
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @return        True if the current time of this Calendar is after the time of
     *                Calendar when; false otherwise.
     * @stable ICU 2.0
     */
    UBool after(const Calendar& when, UErrorCode& status) const;

    /**
     * UDate Arithmetic function. Adds the specified (signed) amount of time to the given
     * time field, based on the calendar's rules. For example, to subtract 5 days from
     * the current time of the calendar, call add(Calendar::DATE, -5). When adding on
     * the month or Calendar::MONTH field, other fields like date might conflict and
     * need to be changed. For instance, adding 1 month on the date 01/31/96 will result
     * in 02/29/96.
     *
     * @param field   Specifies which date field to modify.
     * @param amount  The amount of time to be added to the field, in the natural unit
     *                for that field (e.g., days for the day fields, hours for the hour
     *                field.)
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @deprecated ICU 2.6. use add(UCalendarDateFields field, int32_t amount, UErrorCode& status) instead.
     */
    virtual void add(EDateFields field, int32_t amount, UErrorCode& status);

    /**
     * UDate Arithmetic function. Adds the specified (signed) amount of time to the given
     * time field, based on the calendar's rules. For example, to subtract 5 days from
     * the current time of the calendar, call add(Calendar::DATE, -5). When adding on
     * the month or Calendar::MONTH field, other fields like date might conflict and
     * need to be changed. For instance, adding 1 month on the date 01/31/96 will result
     * in 02/29/96.
     *
     * @param field   Specifies which date field to modify.
     * @param amount  The amount of time to be added to the field, in the natural unit
     *                for that field (e.g., days for the day fields, hours for the hour
     *                field.)
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.6.
     */
    virtual void add(UCalendarDateFields field, int32_t amount, UErrorCode& status);

    /**
     * Time Field Rolling function. Rolls (up/down) a single unit of time on the given
     * time field. For example, to roll the current date up by one day, call
     * roll(Calendar::DATE, true). When rolling on the year or Calendar::YEAR field, it
     * will roll the year value in the range between getMinimum(Calendar::YEAR) and the
     * value returned by getMaximum(Calendar::YEAR). When rolling on the month or
     * Calendar::MONTH field, other fields like date might conflict and, need to be
     * changed. For instance, rolling the month up on the date 01/31/96 will result in
     * 02/29/96. Rolling up always means rolling forward in time; e.g., rolling the year
     * up on "100 BC" will result in "99 BC", for Gregorian calendar. When rolling on the
     * hour-in-day or Calendar::HOUR_OF_DAY field, it will roll the hour value in the range
     * between 0 and 23, which is zero-based.
     * <P>
     * NOTE: Do not use this method -- use roll(EDateFields, int, UErrorCode&) instead.
     *
     * @param field   The time field.
     * @param up      Indicates if the value of the specified time field is to be rolled
     *                up or rolled down. Use true if rolling up, false otherwise.
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @deprecated ICU 2.6. Use roll(UCalendarDateFields field, UBool up, UErrorCode& status) instead.
     */
    inline void roll(EDateFields field, UBool up, UErrorCode& status);

    /**
     * Time Field Rolling function. Rolls (up/down) a single unit of time on the given
     * time field. For example, to roll the current date up by one day, call
     * roll(Calendar::DATE, true). When rolling on the year or Calendar::YEAR field, it
     * will roll the year value in the range between getMinimum(Calendar::YEAR) and the
     * value returned by getMaximum(Calendar::YEAR). When rolling on the month or
     * Calendar::MONTH field, other fields like date might conflict and, need to be
     * changed. For instance, rolling the month up on the date 01/31/96 will result in
     * 02/29/96. Rolling up always means rolling forward in time; e.g., rolling the year
     * up on "100 BC" will result in "99 BC", for Gregorian calendar. When rolling on the
     * hour-in-day or Calendar::HOUR_OF_DAY field, it will roll the hour value in the range
     * between 0 and 23, which is zero-based.
     * <P>
     * NOTE: Do not use this method -- use roll(UCalendarDateFields, int, UErrorCode&) instead.
     *
     * @param field   The time field.
     * @param up      Indicates if the value of the specified time field is to be rolled
     *                up or rolled down. Use true if rolling up, false otherwise.
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.6.
     */
    inline void roll(UCalendarDateFields field, UBool up, UErrorCode& status);

    /**
     * Time Field Rolling function. Rolls by the given amount on the given
     * time field. For example, to roll the current date up by one day, call
     * roll(Calendar::DATE, +1, status). When rolling on the month or
     * Calendar::MONTH field, other fields like date might conflict and, need to be
     * changed. For instance, rolling the month up on the date 01/31/96 will result in
     * 02/29/96.  Rolling by a positive value always means rolling forward in time;
     * e.g., rolling the year by +1 on "100 BC" will result in "99 BC", for Gregorian
     * calendar. When rolling on the hour-in-day or Calendar::HOUR_OF_DAY field, it will
     * roll the hour value in the range between 0 and 23, which is zero-based.
     * <P>
     * The only difference between roll() and add() is that roll() does not change
     * the value of more significant fields when it reaches the minimum or maximum
     * of its range, whereas add() does.
     *
     * @param field   The time field.
     * @param amount  Indicates amount to roll.
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid, this will be set to
     *                an error status.
     * @deprecated ICU 2.6. Use roll(UCalendarDateFields field, int32_t amount, UErrorCode& status) instead.
     */
    virtual void roll(EDateFields field, int32_t amount, UErrorCode& status);

    /**
     * Time Field Rolling function. Rolls by the given amount on the given
     * time field. For example, to roll the current date up by one day, call
     * roll(Calendar::DATE, +1, status). When rolling on the month or
     * Calendar::MONTH field, other fields like date might conflict and, need to be
     * changed. For instance, rolling the month up on the date 01/31/96 will result in
     * 02/29/96.  Rolling by a positive value always means rolling forward in time;
     * e.g., rolling the year by +1 on "100 BC" will result in "99 BC", for Gregorian
     * calendar. When rolling on the hour-in-day or Calendar::HOUR_OF_DAY field, it will
     * roll the hour value in the range between 0 and 23, which is zero-based.
     * <P>
     * The only difference between roll() and add() is that roll() does not change
     * the value of more significant fields when it reaches the minimum or maximum
     * of its range, whereas add() does.
     *
     * @param field   The time field.
     * @param amount  Indicates amount to roll.
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid, this will be set to
     *                an error status.
     * @stable ICU 2.6.
     */
    virtual void roll(UCalendarDateFields field, int32_t amount, UErrorCode& status);

    /**
     * Return the difference between the given time and the time this
     * calendar object is set to.  If this calendar is set
     * <em>before</em> the given time, the returned value will be
     * positive.  If this calendar is set <em>after</em> the given
     * time, the returned value will be negative.  The
     * <code>field</code> parameter specifies the units of the return
     * value.  For example, if <code>fieldDifference(when,
     * Calendar::MONTH)</code> returns 3, then this calendar is set to
     * 3 months before <code>when</code>, and possibly some addition
     * time less than one month.
     *
     * <p>As a side effect of this call, this calendar is advanced
     * toward <code>when</code> by the given amount.  That is, calling
     * this method has the side effect of calling <code>add(field,
     * n)</code>, where <code>n</code> is the return value.
     *
     * <p>Usage: To use this method, call it first with the largest
     * field of interest, then with progressively smaller fields.  For
     * example:
     *
     * <pre>
     * int y = cal->fieldDifference(when, Calendar::YEAR, err);
     * int m = cal->fieldDifference(when, Calendar::MONTH, err);
     * int d = cal->fieldDifference(when, Calendar::DATE, err);</pre>
     *
     * computes the difference between <code>cal</code> and
     * <code>when</code> in years, months, and days.
     *
     * <p>Note: <code>fieldDifference()</code> is
     * <em>asymmetrical</em>.  That is, in the following code:
     *
     * <pre>
     * cal->setTime(date1, err);
     * int m1 = cal->fieldDifference(date2, Calendar::MONTH, err);
     * int d1 = cal->fieldDifference(date2, Calendar::DATE, err);
     * cal->setTime(date2, err);
     * int m2 = cal->fieldDifference(date1, Calendar::MONTH, err);
     * int d2 = cal->fieldDifference(date1, Calendar::DATE, err);</pre>
     *
     * one might expect that <code>m1 == -m2 && d1 == -d2</code>.
     * However, this is not generally the case, because of
     * irregularities in the underlying calendar system (e.g., the
     * Gregorian calendar has a varying number of days per month).
     *
     * @param when the date to compare this calendar's time to
     * @param field the field in which to compute the result
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid, this will be set to
     *                an error status.
     * @return the difference, either positive or negative, between
     * this calendar's time and <code>when</code>, in terms of
     * <code>field</code>.
     * @deprecated ICU 2.6. Use fieldDifference(UDate when, UCalendarDateFields field, UErrorCode& status).
     */
    virtual int32_t fieldDifference(UDate when, EDateFields field, UErrorCode& status);

    /**
     * Return the difference between the given time and the time this
     * calendar object is set to.  If this calendar is set
     * <em>before</em> the given time, the returned value will be
     * positive.  If this calendar is set <em>after</em> the given
     * time, the returned value will be negative.  The
     * <code>field</code> parameter specifies the units of the return
     * value.  For example, if <code>fieldDifference(when,
     * Calendar::MONTH)</code> returns 3, then this calendar is set to
     * 3 months before <code>when</code>, and possibly some addition
     * time less than one month.
     *
     * <p>As a side effect of this call, this calendar is advanced
     * toward <code>when</code> by the given amount.  That is, calling
     * this method has the side effect of calling <code>add(field,
     * n)</code>, where <code>n</code> is the return value.
     *
     * <p>Usage: To use this method, call it first with the largest
     * field of interest, then with progressively smaller fields.  For
     * example:
     *
     * <pre>
     * int y = cal->fieldDifference(when, Calendar::YEAR, err);
     * int m = cal->fieldDifference(when, Calendar::MONTH, err);
     * int d = cal->fieldDifference(when, Calendar::DATE, err);</pre>
     *
     * computes the difference between <code>cal</code> and
     * <code>when</code> in years, months, and days.
     *
     * <p>Note: <code>fieldDifference()</code> is
     * <em>asymmetrical</em>.  That is, in the following code:
     *
     * <pre>
     * cal->setTime(date1, err);
     * int m1 = cal->fieldDifference(date2, Calendar::MONTH, err);
     * int d1 = cal->fieldDifference(date2, Calendar::DATE, err);
     * cal->setTime(date2, err);
     * int m2 = cal->fieldDifference(date1, Calendar::MONTH, err);
     * int d2 = cal->fieldDifference(date1, Calendar::DATE, err);</pre>
     *
     * one might expect that <code>m1 == -m2 && d1 == -d2</code>.
     * However, this is not generally the case, because of
     * irregularities in the underlying calendar system (e.g., the
     * Gregorian calendar has a varying number of days per month).
     *
     * @param when the date to compare this calendar's time to
     * @param field the field in which to compute the result
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid, this will be set to
     *                an error status.
     * @return the difference, either positive or negative, between
     * this calendar's time and <code>when</code>, in terms of
     * <code>field</code>.
     * @stable ICU 2.6.
     */
    virtual int32_t fieldDifference(UDate when, UCalendarDateFields field, UErrorCode& status);

    /**
     * Sets the calendar's time zone to be the one passed in. The Calendar takes ownership
     * of the TimeZone; the caller is no longer responsible for deleting it.  If the
     * given time zone is NULL, this function has no effect.
     *
     * @param value  The given time zone.
     * @stable ICU 2.0
     */
    void adoptTimeZone(TimeZone* value);

    /**
     * Sets the calendar's time zone to be the same as the one passed in. The TimeZone
     * passed in is _not_ adopted; the client is still responsible for deleting it.
     *
     * @param zone  The given time zone.
     * @stable ICU 2.0
     */
    void setTimeZone(const TimeZone& zone);

    /**
     * Returns a reference to the time zone owned by this calendar. The returned reference
     * is only valid until clients make another call to adoptTimeZone or setTimeZone,
     * or this Calendar is destroyed.
     *
     * @return   The time zone object associated with this calendar.
     * @stable ICU 2.0
     */
    const TimeZone& getTimeZone(void) const;

    /**
     * Returns the time zone owned by this calendar. The caller owns the returned object
     * and must delete it when done.  After this call, the new time zone associated
     * with this Calendar is the default TimeZone as returned by TimeZone::createDefault().
     *
     * @return   The time zone object which was associated with this calendar.
     * @stable ICU 2.0
     */
    TimeZone* orphanTimeZone(void);

    /**
     * Queries if the current date for this Calendar is in Daylight Savings Time.
     *
     * @param status Fill-in parameter which receives the status of this operation.
     * @return   True if the current date for this Calendar is in Daylight Savings Time,
     *           false, otherwise.
     * @stable ICU 2.0
     */
    virtual UBool inDaylightTime(UErrorCode& status) const = 0;

    /**
     * Specifies whether or not date/time interpretation is to be lenient. With lenient
     * interpretation, a date such as "February 942, 1996" will be treated as being
     * equivalent to the 941st day after February 1, 1996. With strict interpretation,
     * such dates will cause an error when computing time from the time field values
     * representing the dates.
     *
     * @param lenient  True specifies date/time interpretation to be lenient.
     *
     * @see            DateFormat#setLenient
     * @stable ICU 2.0
     */
    void setLenient(UBool lenient);

    /**
     * Tells whether date/time interpretation is to be lenient.
     *
     * @return   True tells that date/time interpretation is to be lenient.
     * @stable ICU 2.0
     */
    UBool isLenient(void) const;

    /**
     * Sets what the first day of the week is; e.g., Sunday in US, Monday in France.
     *
     * @param value  The given first day of the week.
     * @deprecated ICU 2.6. Use setFirstDayOfWeek(UCalendarDaysOfWeek value) instead.
     */
    void setFirstDayOfWeek(EDaysOfWeek value);

    /**
     * Sets what the first day of the week is; e.g., Sunday in US, Monday in France.
     *
     * @param value  The given first day of the week.
     * @stable ICU 2.6.
     */
    void setFirstDayOfWeek(UCalendarDaysOfWeek value);

    /**
     * Gets what the first day of the week is; e.g., Sunday in US, Monday in France.
     *
     * @return   The first day of the week.
     * @deprecated ICU 2.6 use the overload with error code
     */
    EDaysOfWeek getFirstDayOfWeek(void) const;

    /**
     * Gets what the first day of the week is; e.g., Sunday in US, Monday in France.
     *
     * @param status error code
     * @return   The first day of the week.
     * @stable ICU 2.6
     */
    UCalendarDaysOfWeek getFirstDayOfWeek(UErrorCode &status) const;

    /**
     * Sets what the minimal days required in the first week of the year are; For
     * example, if the first week is defined as one that contains the first day of the
     * first month of a year, call the method with value 1. If it must be a full week,
     * use value 7.
     *
     * @param value  The given minimal days required in the first week of the year.
     * @stable ICU 2.0
     */
    void setMinimalDaysInFirstWeek(uint8_t value);

    /**
     * Gets what the minimal days required in the first week of the year are; e.g., if
     * the first week is defined as one that contains the first day of the first month
     * of a year, getMinimalDaysInFirstWeek returns 1. If the minimal days required must
     * be a full week, getMinimalDaysInFirstWeek returns 7.
     *
     * @return   The minimal days required in the first week of the year.
     * @stable ICU 2.0
     */
    uint8_t getMinimalDaysInFirstWeek(void) const;

    /**
     * Gets the minimum value for the given time field. e.g., for Gregorian
     * DAY_OF_MONTH, 1.
     *
     * @param field  The given time field.
     * @return       The minimum value for the given time field.
     * @deprecated ICU 2.6. Use getMinimum(UCalendarDateFields field) instead.
     */
    virtual int32_t getMinimum(EDateFields field) const;

    /**
     * Gets the minimum value for the given time field. e.g., for Gregorian
     * DAY_OF_MONTH, 1.
     *
     * @param field  The given time field.
     * @return       The minimum value for the given time field.
     * @stable ICU 2.6.
     */
    virtual int32_t getMinimum(UCalendarDateFields field) const;

    /**
     * Gets the maximum value for the given time field. e.g. for Gregorian DAY_OF_MONTH,
     * 31.
     *
     * @param field  The given time field.
     * @return       The maximum value for the given time field.
     * @deprecated ICU 2.6. Use getMaximum(UCalendarDateFields field) instead.
     */
    virtual int32_t getMaximum(EDateFields field) const;

    /**
     * Gets the maximum value for the given time field. e.g. for Gregorian DAY_OF_MONTH,
     * 31.
     *
     * @param field  The given time field.
     * @return       The maximum value for the given time field.
     * @stable ICU 2.6.
     */
    virtual int32_t getMaximum(UCalendarDateFields field) const;

    /**
     * Gets the highest minimum value for the given field if varies. Otherwise same as
     * getMinimum(). For Gregorian, no difference.
     *
     * @param field  The given time field.
     * @return       The highest minimum value for the given time field.
     * @deprecated ICU 2.6. Use getGreatestMinimum(UCalendarDateFields field) instead.
     */
    virtual int32_t getGreatestMinimum(EDateFields field) const;

    /**
     * Gets the highest minimum value for the given field if varies. Otherwise same as
     * getMinimum(). For Gregorian, no difference.
     *
     * @param field  The given time field.
     * @return       The highest minimum value for the given time field.
     * @stable ICU 2.6.
     */
    virtual int32_t getGreatestMinimum(UCalendarDateFields field) const;

    /**
     * Gets the lowest maximum value for the given field if varies. Otherwise same as
     * getMaximum(). e.g., for Gregorian DAY_OF_MONTH, 28.
     *
     * @param field  The given time field.
     * @return       The lowest maximum value for the given time field.
     * @deprecated ICU 2.6. Use getLeastMaximum(UCalendarDateFields field) instead.
     */
    virtual int32_t getLeastMaximum(EDateFields field) const;

    /**
     * Gets the lowest maximum value for the given field if varies. Otherwise same as
     * getMaximum(). e.g., for Gregorian DAY_OF_MONTH, 28.
     *
     * @param field  The given time field.
     * @return       The lowest maximum value for the given time field.
     * @stable ICU 2.6.
     */
    virtual int32_t getLeastMaximum(UCalendarDateFields field) const;

    /**
     * Return the minimum value that this field could have, given the current date.
     * For the Gregorian calendar, this is the same as getMinimum() and getGreatestMinimum().
     *
     * The version of this function on Calendar uses an iterative algorithm to determine the
     * actual minimum value for the field.  There is almost always a more efficient way to
     * accomplish this (in most cases, you can simply return getMinimum()).  GregorianCalendar
     * overrides this function with a more efficient implementation.
     *
     * @param field    the field to determine the minimum of
     * @param status   Fill-in parameter which receives the status of this operation.
     * @return         the minimum of the given field for the current date of this Calendar
     * @deprecated ICU 2.6. Use getActualMinimum(UCalendarDateFields field, UErrorCode& status) instead.
     */
    int32_t getActualMinimum(EDateFields field, UErrorCode& status) const;

    /**
     * Return the minimum value that this field could have, given the current date.
     * For the Gregorian calendar, this is the same as getMinimum() and getGreatestMinimum().
     *
     * The version of this function on Calendar uses an iterative algorithm to determine the
     * actual minimum value for the field.  There is almost always a more efficient way to
     * accomplish this (in most cases, you can simply return getMinimum()).  GregorianCalendar
     * overrides this function with a more efficient implementation.
     *
     * @param field    the field to determine the minimum of
     * @param status   Fill-in parameter which receives the status of this operation.
     * @return         the minimum of the given field for the current date of this Calendar
     * @stable ICU 2.6.
     */
    virtual int32_t getActualMinimum(UCalendarDateFields field, UErrorCode& status) const;

    /**
     * Return the maximum value that this field could have, given the current date.
     * For example, with the date "Feb 3, 1997" and the DAY_OF_MONTH field, the actual
     * maximum would be 28; for "Feb 3, 1996" it s 29.  Similarly for a Hebrew calendar,
     * for some years the actual maximum for MONTH is 12, and for others 13.
     *
     * The version of this function on Calendar uses an iterative algorithm to determine the
     * actual maximum value for the field.  There is almost always a more efficient way to
     * accomplish this (in most cases, you can simply return getMaximum()).  GregorianCalendar
     * overrides this function with a more efficient implementation.
     *
     * @param field    the field to determine the maximum of
     * @param status   Fill-in parameter which receives the status of this operation.
     * @return         the maximum of the given field for the current date of this Calendar
     * @deprecated ICU 2.6. Use getActualMaximum(UCalendarDateFields field, UErrorCode& status) instead.
     */
    int32_t getActualMaximum(EDateFields field, UErrorCode& status) const;

    /**
     * Return the maximum value that this field could have, given the current date.
     * For example, with the date "Feb 3, 1997" and the DAY_OF_MONTH field, the actual
     * maximum would be 28; for "Feb 3, 1996" it s 29.  Similarly for a Hebrew calendar,
     * for some years the actual maximum for MONTH is 12, and for others 13.
     *
     * The version of this function on Calendar uses an iterative algorithm to determine the
     * actual maximum value for the field.  There is almost always a more efficient way to
     * accomplish this (in most cases, you can simply return getMaximum()).  GregorianCalendar
     * overrides this function with a more efficient implementation.
     *
     * @param field    the field to determine the maximum of
     * @param status   Fill-in parameter which receives the status of this operation.
     * @return         the maximum of the given field for the current date of this Calendar
     * @stable ICU 2.6.
     */
    virtual int32_t getActualMaximum(UCalendarDateFields field, UErrorCode& status) const;

    /**
     * Gets the value for a given time field. Recalculate the current time field values
     * if the time value has been changed by a call to setTime(). Return zero for unset
     * fields if any fields have been explicitly set by a call to set(). To force a
     * recomputation of all fields regardless of the previous state, call complete().
     * This method is semantically const, but may alter the object in memory.
     *
     * @param field  The given time field.
     * @param status Fill-in parameter which receives the status of the operation.
     * @return       The value for the given time field, or zero if the field is unset,
     *               and set() has been called for any other field.
     * @deprecated ICU 2.6. Use get(UCalendarDateFields field, UErrorCode& status) instead.
     */
    int32_t get(EDateFields field, UErrorCode& status) const;

    /**
     * Gets the value for a given time field. Recalculate the current time field values
     * if the time value has been changed by a call to setTime(). Return zero for unset
     * fields if any fields have been explicitly set by a call to set(). To force a
     * recomputation of all fields regardless of the previous state, call complete().
     * This method is semantically const, but may alter the object in memory.
     *
     * @param field  The given time field.
     * @param status Fill-in parameter which receives the status of the operation.
     * @return       The value for the given time field, or zero if the field is unset,
     *               and set() has been called for any other field.
     * @stable ICU 2.6.
     */
    int32_t get(UCalendarDateFields field, UErrorCode& status) const;

    /**
     * Determines if the given time field has a value set. This can affect in the
     * resolving of time in Calendar. Unset fields have a value of zero, by definition.
     *
     * @param field  The given time field.
     * @return   True if the given time field has a value set; false otherwise.
     * @deprecated ICU 2.6. Use isSet(UCalendarDateFields field) instead.
     */
    UBool isSet(EDateFields field) const;

    /**
     * Determines if the given time field has a value set. This can affect in the
     * resolving of time in Calendar. Unset fields have a value of zero, by definition.
     *
     * @param field  The given time field.
     * @return   True if the given time field has a value set; false otherwise.
     * @stable ICU 2.6.
     */
    UBool isSet(UCalendarDateFields field) const;

    /**
     * Sets the given time field with the given value.
     *
     * @param field  The given time field.
     * @param value  The value to be set for the given time field.
     * @deprecated ICU 2.6. Use set(UCalendarDateFields field, int32_t value) instead.
     */
    void set(EDateFields field, int32_t value);

    /**
     * Sets the given time field with the given value.
     *
     * @param field  The given time field.
     * @param value  The value to be set for the given time field.
     * @stable ICU 2.6.
     */
    void set(UCalendarDateFields field, int32_t value);

    /**
     * Sets the values for the fields YEAR, MONTH, and DATE. Other field values are
     * retained; call clear() first if this is not desired.
     *
     * @param year   The value used to set the YEAR time field.
     * @param month  The value used to set the MONTH time field. Month value is 0-based.
     *               e.g., 0 for January.
     * @param date   The value used to set the DATE time field.
     * @stable ICU 2.0
     */
    void set(int32_t year, int32_t month, int32_t date);

    /**
     * Sets the values for the fields YEAR, MONTH, DATE, HOUR_OF_DAY, and MINUTE. Other
     * field values are retained; call clear() first if this is not desired.
     *
     * @param year    The value used to set the YEAR time field.
     * @param month   The value used to set the MONTH time field. Month value is
     *                0-based. E.g., 0 for January.
     * @param date    The value used to set the DATE time field.
     * @param hour    The value used to set the HOUR_OF_DAY time field.
     * @param minute  The value used to set the MINUTE time field.
     * @stable ICU 2.0
     */
    void set(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute);

    /**
     * Sets the values for the fields YEAR, MONTH, DATE, HOUR_OF_DAY, MINUTE, and SECOND.
     * Other field values are retained; call clear() first if this is not desired.
     *
     * @param year    The value used to set the YEAR time field.
     * @param month   The value used to set the MONTH time field. Month value is
     *                0-based. E.g., 0 for January.
     * @param date    The value used to set the DATE time field.
     * @param hour    The value used to set the HOUR_OF_DAY time field.
     * @param minute  The value used to set the MINUTE time field.
     * @param second  The value used to set the SECOND time field.
     * @stable ICU 2.0
     */
    void set(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second);

    /**
     * Clears the values of all the time fields, making them both unset and assigning
     * them a value of zero. The field values will be determined during the next
     * resolving of time into time fields.
     * @stable ICU 2.0
     */
    void clear(void);

    /**
     * Clears the value in the given time field, both making it unset and assigning it a
     * value of zero. This field value will be determined during the next resolving of
     * time into time fields.
     *
     * @param field  The time field to be cleared.
     * @deprecated ICU 2.6. Use clear(UCalendarDateFields field) instead.
     */
    void clear(EDateFields field);

    /**
     * Clears the value in the given time field, both making it unset and assigning it a
     * value of zero. This field value will be determined during the next resolving of
     * time into time fields.
     *
     * @param field  The time field to be cleared.
     * @stable ICU 2.6.
     */
    void clear(UCalendarDateFields field);

    /**
     * Returns a unique class ID POLYMORPHICALLY. Pure virtual method. This method is to
     * implement a simple version of RTTI, since not all C++ compilers support genuine
     * RTTI. Polymorphic operator==() and clone() methods call this method.
     * <P>
     * Concrete subclasses of Calendar must implement getDynamicClassID() and also a
     * static method and data member:
     *
     *      static UClassID getStaticClassID() { return (UClassID)&amp;fgClassID; }
     *      static char fgClassID;
     *
     * @return   The class ID for this object. All objects of a given class have the
     *           same class ID. Objects of other classes have different class IDs.
     * @stable ICU 2.0
     */
    virtual UClassID getDynamicClassID(void) const = 0;

    /**
     * Returns the resource key string used for this calendar type.
     * For example, prepending "Eras_" to this string could return "Eras_japanese"
     * or "Eras_gregorian".
     *
     * @returns static string, for example, "gregorian" or "japanese"
     * @internal
     */
    virtual const char * getType() const = 0;

    /**
     * Returns whether the given day of the week is a weekday, a
     * weekend day, or a day that transitions from one to the other,
     * in this calendar system. If a transition occurs at midnight,
     * then the days before and after the transition will have the
     * type UCAL_WEEKDAY or UCAL_WEEKEND. If a transition occurs at a time
     * other than midnight, then the day of the transition will have
     * the type UCAL_WEEKEND_ONSET or UCAL_WEEKEND_CEASE. In this case, the
     * method getWeekendTransition() will return the point of
     * transition.
     * @param dayOfWeek The day of the week whose type is desired (UCAL_SUNDAY..UCAL_SATURDAY).
     * @param status The error code for the operation.
     * @return The UCalendarWeekdayType for the day of the week.
     * @stable ICU 4.4
     */
    virtual UCalendarWeekdayType getDayOfWeekType(UCalendarDaysOfWeek dayOfWeek, UErrorCode &status) const;

    /**
     * Returns the time during the day at which the weekend begins or ends in
     * this calendar system.  If getDayOfWeekType() rerturns UCAL_WEEKEND_ONSET
     * for the specified dayOfWeek, return the time at which the weekend begins.
     * If getDayOfWeekType() returns UCAL_WEEKEND_CEASE for the specified dayOfWeek,
     * return the time at which the weekend ends. If getDayOfWeekType() returns
     * some other UCalendarWeekdayType for the specified dayOfWeek, is it an error condition
     * (U_ILLEGAL_ARGUMENT_ERROR).
     * @param dayOfWeek The day of the week for which the weekend transition time is
     * desired (UCAL_SUNDAY..UCAL_SATURDAY).
     * @param status The error code for the operation.
     * @return The milliseconds after midnight at which the weekend begins or ends.
     * @stable ICU 4.4
     */
    virtual int32_t getWeekendTransition(UCalendarDaysOfWeek dayOfWeek, UErrorCode &status) const;

    /**
     * Returns TRUE if the given UDate is in the weekend in
     * this calendar system.
     * @param date The UDate in question.
     * @param status The error code for the operation.
     * @return TRUE if the given UDate is in the weekend in
     * this calendar system, FALSE otherwise.
     * @stable ICU 4.4
     */
    virtual UBool isWeekend(UDate date, UErrorCode &status) const;

    /**
     * Returns TRUE if this Calendar's current date-time is in the weekend in
     * this calendar system.
     * @return TRUE if this Calendar's current date-time is in the weekend in
     * this calendar system, FALSE otherwise.
     * @stable ICU 4.4
     */
    virtual UBool isWeekend(void) const;

protected:

     /**
      * Constructs a Calendar with the default time zone as returned by
      * TimeZone::createInstance(), and the default locale.
      *
      * @param success  Indicates the status of Calendar object construction. Returns
      *                 U_ZERO_ERROR if constructed successfully.
     * @stable ICU 2.0
      */
    Calendar(UErrorCode& success);

    /**
     * Copy constructor
     *
     * @param source    Calendar object to be copied from
     * @stable ICU 2.0
     */
    Calendar(const Calendar& source);

    /**
     * Default assignment operator
     *
     * @param right    Calendar object to be copied
     * @stable ICU 2.0
     */
    Calendar& operator=(const Calendar& right);

    /**
     * Constructs a Calendar with the given time zone and locale. Clients are no longer
     * responsible for deleting the given time zone object after it's adopted.
     *
     * @param zone     The given time zone.
     * @param aLocale  The given locale.
     * @param success  Indicates the status of Calendar object construction. Returns
     *                 U_ZERO_ERROR if constructed successfully.
     * @stable ICU 2.0
     */
    Calendar(TimeZone* zone, const Locale& aLocale, UErrorCode& success);

    /**
     * Constructs a Calendar with the given time zone and locale.
     *
     * @param zone     The given time zone.
     * @param aLocale  The given locale.
     * @param success  Indicates the status of Calendar object construction. Returns
     *                 U_ZERO_ERROR if constructed successfully.
     * @stable ICU 2.0
     */
    Calendar(const TimeZone& zone, const Locale& aLocale, UErrorCode& success);

    /**
     * Converts Calendar's time field values to GMT as milliseconds.
     *
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.0
     */
    virtual void computeTime(UErrorCode& status);

    /**
     * Converts GMT as milliseconds to time field values. This allows you to sync up the
     * time field values with a new time that is set for the calendar.  This method
     * does NOT recompute the time first; to recompute the time, then the fields, use
     * the method complete().
     *
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.0
     */
    virtual void computeFields(UErrorCode& status);

    /**
     * Gets this Calendar's current time as a long.
     *
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @return the current time as UTC milliseconds from the epoch.
     * @stable ICU 2.0
     */
    double getTimeInMillis(UErrorCode& status) const;

    /**
     * Sets this Calendar's current time from the given long value.
     * @param millis  the new time in UTC milliseconds from the epoch.
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.0
     */
    void setTimeInMillis( double millis, UErrorCode& status );

    /**
     * Recomputes the current time from currently set fields, and then fills in any
     * unset fields in the time field list.
     *
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     * @stable ICU 2.0
     */
    void complete(UErrorCode& status);

    /**
     * Gets the value for a given time field. Subclasses can use this function to get
     * field values without forcing recomputation of time.
     *
     * @param field  The given time field.
     * @return       The value for the given time field.
     * @deprecated ICU 2.6. Use internalGet(UCalendarDateFields field) instead.
     */
    inline int32_t internalGet(EDateFields field) const {return fFields[field];}

    /**
     * Gets the value for a given time field. Subclasses can use this function to get
     * field values without forcing recomputation of time. If the field's stamp is UNSET,
     * the defaultValue is used.
     *
     * @param field  The given time field.
     * @param defaultValue a default value used if the field is unset.
     * @return       The value for the given time field.
     * @internal
     */
    inline int32_t internalGet(UCalendarDateFields field, int32_t defaultValue) const {return fStamp[field]>kUnset ? fFields[field] : defaultValue;}

    /**
     * Gets the value for a given time field. Subclasses can use this function to get
     * field values without forcing recomputation of time.
     *
     * @param field  The given time field.
     * @return       The value for the given time field.
     * @internal
     */
    inline int32_t internalGet(UCalendarDateFields field) const {return fFields[field];}

    /**
     * Sets the value for a given time field.  This is a fast internal method for
     * subclasses.  It does not affect the areFieldsInSync, isTimeSet, or areAllFieldsSet
     * flags.
     *
     * @param field    The given time field.
     * @param value    The value for the given time field.
     * @deprecated ICU 2.6. Use internalSet(UCalendarDateFields field, int32_t value) instead.
     */
    void internalSet(EDateFields field, int32_t value);

    /**
     * Sets the value for a given time field.  This is a fast internal method for
     * subclasses.  It does not affect the areFieldsInSync, isTimeSet, or areAllFieldsSet
     * flags.
     *
     * @param field    The given time field.
     * @param value    The value for the given time field.
     * @stable ICU 2.6.
     */
    inline void internalSet(UCalendarDateFields field, int32_t value);

    /**
     * Prepare this calendar for computing the actual minimum or maximum.
     * This method modifies this calendar's fields; it is called on a
     * temporary calendar.
     * @internal
     */
    virtual void prepareGetActual(UCalendarDateFields field, UBool isMinimum, UErrorCode &status);

    /**
     * Limit enums. Not in sync with UCalendarLimitType (refers to internal fields).
     * @internal
     */
    enum ELimitType {
      UCAL_LIMIT_MINIMUM = 0,
      UCAL_LIMIT_GREATEST_MINIMUM,
      UCAL_LIMIT_LEAST_MAXIMUM,
      UCAL_LIMIT_MAXIMUM,
      UCAL_LIMIT_COUNT
    };

    /**
     * Subclass API for defining limits of different types.
     * Subclasses must implement this method to return limits for the
     * following fields:
     *
     * <pre>UCAL_ERA
     * UCAL_YEAR
     * UCAL_MONTH
     * UCAL_WEEK_OF_YEAR
     * UCAL_WEEK_OF_MONTH
     * UCAL_DATE (DAY_OF_MONTH on Java)
     * UCAL_DAY_OF_YEAR
     * UCAL_DAY_OF_WEEK_IN_MONTH
     * UCAL_YEAR_WOY
     * UCAL_EXTENDED_YEAR</pre>
     *
     * @param field one of the above field numbers
     * @param limitType one of <code>MINIMUM</code>, <code>GREATEST_MINIMUM</code>,
     * <code>LEAST_MAXIMUM</code>, or <code>MAXIMUM</code>
     * @internal
     */
    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const = 0;

    /**
     * Return a limit for a field.
     * @param field the field, from <code>0..UCAL_MAX_FIELD</code>
     * @param limitType the type specifier for the limit
     * @see #ELimitType
     * @internal
     */
    virtual int32_t getLimit(UCalendarDateFields field, ELimitType limitType) const;


    /**
     * Return the Julian day number of day before the first day of the
     * given month in the given extended year.  Subclasses should override
     * this method to implement their calendar system.
     * @param eyear the extended year
     * @param month the zero-based month, or 0 if useMonth is false
     * @param useMonth if false, compute the day before the first day of
     * the given year, otherwise, compute the day before the first day of
     * the given month
     * @return the Julian day number of the day before the first
     * day of the given month and year
     * @internal
     */
    virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month,
                                                   UBool useMonth) const  = 0;

    /**
     * Return the number of days in the given month of the given extended
     * year of this calendar system.  Subclasses should override this
     * method if they can provide a more correct or more efficient
     * implementation than the default implementation in Calendar.
     * @internal
     */
    virtual int32_t handleGetMonthLength(int32_t extendedYear, int32_t month) const ;

    /**
     * Return the number of days in the given extended year of this
     * calendar system.  Subclasses should override this method if they can
     * provide a more correct or more efficient implementation than the
     * default implementation in Calendar.
     * @stable ICU 2.0
     */
    virtual int32_t handleGetYearLength(int32_t eyear) const;


    /**
     * Return the extended year defined by the current fields.  This will
     * use the UCAL_EXTENDED_YEAR field or the UCAL_YEAR and supra-year fields (such
     * as UCAL_ERA) specific to the calendar system, depending on which set of
     * fields is newer.
     * @return the extended year
     * @internal
     */
    virtual int32_t handleGetExtendedYear() = 0;

    /**
     * Subclasses may override this.  This method calls
     * handleGetMonthLength() to obtain the calendar-specific month
     * length.
     * @param bestField which field to use to calculate the date
     * @return julian day specified by calendar fields.
     * @internal
     */
    virtual int32_t handleComputeJulianDay(UCalendarDateFields bestField);

    /**
     * Subclasses must override this to convert from week fields
     * (YEAR_WOY and WEEK_OF_YEAR) to an extended year in the case
     * where YEAR, EXTENDED_YEAR are not set.
     * The Calendar implementation assumes yearWoy is in extended gregorian form
     * @internal
     * @return the extended year, UCAL_EXTENDED_YEAR
     */
    virtual int32_t handleGetExtendedYearFromWeekFields(int32_t yearWoy, int32_t woy);

    /**
     * Compute the Julian day from fields.  Will determine whether to use
     * the JULIAN_DAY field directly, or other fields.
     * @return the julian day
     * @internal
     */
    int32_t computeJulianDay();

    /**
     * Compute the milliseconds in the day from the fields.  This is a
     * value from 0 to 23:59:59.999 inclusive, unless fields are out of
     * range, in which case it can be an arbitrary value.  This value
     * reflects local zone wall time.
     * @internal
     */
    int32_t computeMillisInDay();

    /**
     * This method can assume EXTENDED_YEAR has been set.
     * @param millis milliseconds of the date fields
     * @param millisInDay milliseconds of the time fields; may be out
     * or range.
     * @param ec Output param set to failure code on function return
     *          when this function fails.
     * @internal
     */
    int32_t computeZoneOffset(double millis, int32_t millisInDay, UErrorCode &ec);


    /**
     * Determine the best stamp in a range.
     * @param start first enum to look at
     * @param end last enum to look at
     * @param bestSoFar stamp prior to function call
     * @return the stamp value of the best stamp
     * @internal
     */
    int32_t newestStamp(UCalendarDateFields start, UCalendarDateFields end, int32_t bestSoFar) const;

    /**
     * Values for field resolution tables
     * @see #resolveFields
     * @internal
     */
    enum {
      /** Marker for end of resolve set (row or group). */
      kResolveSTOP = -1,
      /** Value to be bitwised "ORed" against resolve table field values for remapping.  Example: (UCAL_DATE | kResolveRemap) in 1st column will cause 'UCAL_DATE' to be returned, but will not examine the value of UCAL_DATE.  */
      kResolveRemap = 32
    };

    /**
     * Precedence table for Dates
     * @see #resolveFields
     * @internal
     */
    static const UFieldResolutionTable kDatePrecedence[];

    /**
     * Precedence table for Year
     * @see #resolveFields
     * @internal
     */
    static const UFieldResolutionTable kYearPrecedence[];

    /**
     * Precedence table for Day of Week
     * @see #resolveFields
     * @internal
     */
    static const UFieldResolutionTable kDOWPrecedence[];

    /**
     * Given a precedence table, return the newest field combination in
     * the table, or UCAL_FIELD_COUNT if none is found.
     *
     * <p>The precedence table is a 3-dimensional array of integers.  It
     * may be thought of as an array of groups.  Each group is an array of
     * lines.  Each line is an array of field numbers.  Within a line, if
     * all fields are set, then the time stamp of the line is taken to be
     * the stamp of the most recently set field.  If any field of a line is
     * unset, then the line fails to match.  Within a group, the line with
     * the newest time stamp is selected.  The first field of the line is
     * returned to indicate which line matched.
     *
     * <p>In some cases, it may be desirable to map a line to field that
     * whose stamp is NOT examined.  For example, if the best field is
     * DAY_OF_WEEK then the DAY_OF_WEEK_IN_MONTH algorithm may be used.  In
     * order to do this, insert the value <code>kResolveRemap | F</code> at
     * the start of the line, where <code>F</code> is the desired return
     * field value.  This field will NOT be examined; it only determines
     * the return value if the other fields in the line are the newest.
     *
     * <p>If all lines of a group contain at least one unset field, then no
     * line will match, and the group as a whole will fail to match.  In
     * that case, the next group will be processed.  If all groups fail to
     * match, then UCAL_FIELD_COUNT is returned.
     * @internal
     */
    UCalendarDateFields resolveFields(const UFieldResolutionTable *precedenceTable);


    /**
     * @internal
     */
    virtual const UFieldResolutionTable* getFieldResolutionTable() const;

    /**
     * Return the field that is newer, either defaultField, or
     * alternateField.  If neither is newer or neither is set, return defaultField.
     * @internal
     */
    UCalendarDateFields newerField(UCalendarDateFields defaultField, UCalendarDateFields alternateField) const;


private:
    /**
     * Helper function for calculating limits by trial and error
     * @param field The field being investigated
     * @param startValue starting (least max) value of field
     * @param endValue ending (greatest max) value of field
     * @param status return type
     * @internal
     */
    int32_t getActualHelper(UCalendarDateFields field, int32_t startValue, int32_t endValue, UErrorCode &status) const;


protected:
    /**
     * The flag which indicates if the current time is set in the calendar.
     * @stable ICU 2.0
     */
    UBool      fIsTimeSet;

    /**
     * True if the fields are in sync with the currently set time of this Calendar.
     * If false, then the next attempt to get the value of a field will
     * force a recomputation of all fields from the current value of the time
     * field.
     * <P>
     * This should really be named areFieldsInSync, but the old name is retained
     * for backward compatibility.
     * @stable ICU 2.0
     */
    UBool      fAreFieldsSet;

    /**
     * True if all of the fields have been set.  This is initially false, and set to
     * true by computeFields().
     * @stable ICU 2.0
     */
    UBool      fAreAllFieldsSet;

    /**
     * True if all fields have been virtually set, but have not yet been
     * computed.  This occurs only in setTimeInMillis().  A calendar set
     * to this state will compute all fields from the time if it becomes
     * necessary, but otherwise will delay such computation.
     * @stable ICU 3.0
     */
    UBool fAreFieldsVirtuallySet;

    /**
     * Get the current time without recomputing.
     *
     * @return     the current time without recomputing.
     * @stable ICU 2.0
     */
    UDate        internalGetTime(void) const     { return fTime; }

    /**
     * Set the current time without affecting flags or fields.
     *
     * @param time    The time to be set
     * @return        the current time without recomputing.
     * @stable ICU 2.0
     */
    void        internalSetTime(UDate time)     { fTime = time; }

    /**
     * The time fields containing values into which the millis is computed.
     * @stable ICU 2.0
     */
    int32_t     fFields[UCAL_FIELD_COUNT];

    /**
     * The flags which tell if a specified time field for the calendar is set.
     * @deprecated ICU 2.8 use (fStamp[n]!=kUnset)
     */
    UBool      fIsSet[UCAL_FIELD_COUNT];

    /** Special values of stamp[]
     * @stable ICU 2.0
     */
    enum {
        kUnset                 = 0,
        kInternallySet,
        kMinimumUserStamp
    };

    /**
     * Pseudo-time-stamps which specify when each field was set. There
     * are two special values, UNSET and INTERNALLY_SET. Values from
     * MINIMUM_USER_SET to Integer.MAX_VALUE are legal user set values.
     * @stable ICU 2.0
     */
    int32_t        fStamp[UCAL_FIELD_COUNT];

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
     * Subclasses can refer to the DAY_OF_WEEK and DOW_LOCAL fields, which
     * will be set when this method is called.  Subclasses can also call
     * the getGregorianXxx() methods to obtain Gregorian calendar
     * equivalents for the given Julian day.
     *
     * <p>In addition, subclasses should compute any subclass-specific
     * fields, that is, fields from BASE_FIELD_COUNT to
     * getFieldCount() - 1.
     *
     * <p>The default implementation in <code>Calendar</code> implements
     * a pure proleptic Gregorian calendar.
     * @internal
     */
    virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);

    /**
     * Return the extended year on the Gregorian calendar as computed by
     * <code>computeGregorianFields()</code>.
     * @internal
     */
    int32_t getGregorianYear() const {
        return fGregorianYear;
    }

    /**
     * Return the month (0-based) on the Gregorian calendar as computed by
     * <code>computeGregorianFields()</code>.
     * @internal
     */
    int32_t getGregorianMonth() const {
        return fGregorianMonth;
    }

    /**
     * Return the day of year (1-based) on the Gregorian calendar as
     * computed by <code>computeGregorianFields()</code>.
     * @internal
     */
    int32_t getGregorianDayOfYear() const {
        return fGregorianDayOfYear;
    }

    /**
     * Return the day of month (1-based) on the Gregorian calendar as
     * computed by <code>computeGregorianFields()</code>.
     * @internal
     */
    int32_t getGregorianDayOfMonth() const {
      return fGregorianDayOfMonth;
    }

    /**
     * Called by computeJulianDay.  Returns the default month (0-based) for the year,
     * taking year and era into account.  Defaults to 0 for Gregorian, which doesn't care.
     * @param eyear The extended year
     * @internal
     */
    virtual int32_t getDefaultMonthInYear(int32_t eyear) ;


    /**
     * Called by computeJulianDay.  Returns the default day (1-based) for the month,
     * taking currently-set year and era into account.  Defaults to 1 for Gregorian.
     * @param eyear the extended year
     * @param month the month in the year
     * @internal
     */
    virtual int32_t getDefaultDayInMonth(int32_t eyear, int32_t month);

    //-------------------------------------------------------------------------
    // Protected utility methods for use by subclasses.  These are very handy
    // for implementing add, roll, and computeFields.
    //-------------------------------------------------------------------------

    /**
     * Adjust the specified field so that it is within
     * the allowable range for the date to which this calendar is set.
     * For example, in a Gregorian calendar pinning the {@link #UCalendarDateFields DAY_OF_MONTH}
     * field for a calendar set to April 31 would cause it to be set
     * to April 30.
     * <p>
     * <b>Subclassing:</b>
     * <br>
     * This utility method is intended for use by subclasses that need to implement
     * their own overrides of {@link #roll roll} and {@link #add add}.
     * <p>
     * <b>Note:</b>
     * <code>pinField</code> is implemented in terms of
     * {@link #getActualMinimum getActualMinimum}
     * and {@link #getActualMaximum getActualMaximum}.  If either of those methods uses
     * a slow, iterative algorithm for a particular field, it would be
     * unwise to attempt to call <code>pinField</code> for that field.  If you
     * really do need to do so, you should override this method to do
     * something more efficient for that field.
     * <p>
     * @param field The calendar field whose value should be pinned.
     * @param status Output param set to failure code on function return
     *          when this function fails.
     *
     * @see #getActualMinimum
     * @see #getActualMaximum
     * @stable ICU 2.0
     */
    virtual void pinField(UCalendarDateFields field, UErrorCode& status);

    /**
     * Return the week number of a day, within a period. This may be the week number in
     * a year or the week number in a month. Usually this will be a value >= 1, but if
     * some initial days of the period are excluded from week 1, because
     * {@link #getMinimalDaysInFirstWeek getMinimalDaysInFirstWeek} is > 1, then
     * the week number will be zero for those
     * initial days. This method requires the day number and day of week for some
     * known date in the period in order to determine the day of week
     * on the desired day.
     * <p>
     * <b>Subclassing:</b>
     * <br>
     * This method is intended for use by subclasses in implementing their
     * {@link #computeTime computeTime} and/or {@link #computeFields computeFields} methods.
     * It is often useful in {@link #getActualMinimum getActualMinimum} and
     * {@link #getActualMaximum getActualMaximum} as well.
     * <p>
     * This variant is handy for computing the week number of some other
     * day of a period (often the first or last day of the period) when its day
     * of the week is not known but the day number and day of week for some other
     * day in the period (e.g. the current date) <em>is</em> known.
     * <p>
     * @param desiredDay    The {@link #UCalendarDateFields DAY_OF_YEAR} or
     *              {@link #UCalendarDateFields DAY_OF_MONTH} whose week number is desired.
     *              Should be 1 for the first day of the period.
     *
     * @param dayOfPeriod   The {@link #UCalendarDateFields DAY_OF_YEAR}
     *              or {@link #UCalendarDateFields DAY_OF_MONTH} for a day in the period whose
     *              {@link #UCalendarDateFields DAY_OF_WEEK} is specified by the
     *              <code>knownDayOfWeek</code> parameter.
     *              Should be 1 for first day of period.
     *
     * @param dayOfWeek  The {@link #UCalendarDateFields DAY_OF_WEEK} for the day
     *              corresponding to the <code>knownDayOfPeriod</code> parameter.
     *              1-based with 1=Sunday.
     *
     * @return      The week number (one-based), or zero if the day falls before
     *              the first week because
     *              {@link #getMinimalDaysInFirstWeek getMinimalDaysInFirstWeek}
     *              is more than one.
     *
     * @stable ICU 2.8
     */
    int32_t weekNumber(int32_t desiredDay, int32_t dayOfPeriod, int32_t dayOfWeek);


    /**
     * Return the week number of a day, within a period. This may be the week number in
     * a year, or the week number in a month. Usually this will be a value >= 1, but if
     * some initial days of the period are excluded from week 1, because
     * {@link #getMinimalDaysInFirstWeek getMinimalDaysInFirstWeek} is > 1,
     * then the week number will be zero for those
     * initial days. This method requires the day of week for the given date in order to
     * determine the result.
     * <p>
     * <b>Subclassing:</b>
     * <br>
     * This method is intended for use by subclasses in implementing their
     * {@link #computeTime computeTime} and/or {@link #computeFields computeFields} methods.
     * It is often useful in {@link #getActualMinimum getActualMinimum} and
     * {@link #getActualMaximum getActualMaximum} as well.
     * <p>
     * @param dayOfPeriod   The {@link #UCalendarDateFields DAY_OF_YEAR} or
     *                      {@link #UCalendarDateFields DAY_OF_MONTH} whose week number is desired.
     *                      Should be 1 for the first day of the period.
     *
     * @param dayOfWeek     The {@link #UCalendarDateFields DAY_OF_WEEK} for the day
     *                      corresponding to the <code>dayOfPeriod</code> parameter.
     *                      1-based with 1=Sunday.
     *
     * @return      The week number (one-based), or zero if the day falls before
     *              the first week because
     *              {@link #getMinimalDaysInFirstWeek getMinimalDaysInFirstWeek}
     *              is more than one.
     * @internal
     */
    inline int32_t weekNumber(int32_t dayOfPeriod, int32_t dayOfWeek);

    /**
     * returns the local DOW, valid range 0..6
     * @internal
     */
    int32_t getLocalDOW();

private:

    /**
     * The next available value for fStamp[]
     */
    int32_t fNextStamp;// = MINIMUM_USER_STAMP;

    /**
     * The current time set for the calendar.
     */
    UDate        fTime;

    /**
     * @see   #setLenient
     */
    UBool      fLenient;

    /**
     * Time zone affects the time calculation done by Calendar. Calendar subclasses use
     * the time zone data to produce the local time.
     */
    TimeZone*   fZone;

    /**
     * Both firstDayOfWeek and minimalDaysInFirstWeek are locale-dependent. They are
     * used to figure out the week count for a specific date for a given locale. These
     * must be set when a Calendar is constructed. For example, in US locale,
     * firstDayOfWeek is SUNDAY; minimalDaysInFirstWeek is 1. They are used to figure
     * out the week count for a specific date for a given locale. These must be set when
     * a Calendar is constructed.
     */
    UCalendarDaysOfWeek fFirstDayOfWeek;
    uint8_t     fMinimalDaysInFirstWeek;
    UCalendarDaysOfWeek fWeekendOnset;
    int32_t fWeekendOnsetMillis;
    UCalendarDaysOfWeek fWeekendCease;
    int32_t fWeekendCeaseMillis;

    /**
     * Sets firstDayOfWeek and minimalDaysInFirstWeek. Called at Calendar construction
     * time.
     *
     * @param desiredLocale  The given locale.
     * @param type           The calendar type identifier, e.g: gregorian, buddhist, etc.
     * @param success        Indicates the status of setting the week count data from
     *                       the resource for the given locale. Returns U_ZERO_ERROR if
     *                       constructed successfully.
     */
    void        setWeekData(const Locale& desiredLocale, const char *type, UErrorCode& success);

    /**
     * Recompute the time and update the status fields isTimeSet
     * and areFieldsSet.  Callers should check isTimeSet and only
     * call this method if isTimeSet is false.
     *
     * @param status  Output param set to success/failure code on exit. If any value
     *                previously set in the time field is invalid or restricted by
     *                leniency, this will be set to an error status.
     */
    void updateTime(UErrorCode& status);

    /**
     * The Gregorian year, as computed by computeGregorianFields() and
     * returned by getGregorianYear().
     * @see #computeGregorianFields
     */
    int32_t fGregorianYear;

    /**
     * The Gregorian month, as computed by computeGregorianFields() and
     * returned by getGregorianMonth().
     * @see #computeGregorianFields
     */
    int32_t fGregorianMonth;

    /**
     * The Gregorian day of the year, as computed by
     * computeGregorianFields() and returned by getGregorianDayOfYear().
     * @see #computeGregorianFields
     */
    int32_t fGregorianDayOfYear;

    /**
     * The Gregorian day of the month, as computed by
     * computeGregorianFields() and returned by getGregorianDayOfMonth().
     * @see #computeGregorianFields
     */
    int32_t fGregorianDayOfMonth;

    /* calculations */

    /**
     * Compute the Gregorian calendar year, month, and day of month from
     * the given Julian day.  These values are not stored in fields, but in
     * member variables gregorianXxx.  Also compute the DAY_OF_WEEK and
     * DOW_LOCAL fields.
     */
    void computeGregorianAndDOWFields(int32_t julianDay, UErrorCode &ec);

protected:

    /**
     * Compute the Gregorian calendar year, month, and day of month from the
     * Julian day.  These values are not stored in fields, but in member
     * variables gregorianXxx.  They are used for time zone computations and by
     * subclasses that are Gregorian derivatives.  Subclasses may call this
     * method to perform a Gregorian calendar millis->fields computation.
     */
    void computeGregorianFields(int32_t julianDay, UErrorCode &ec);

private:

    /**
     * Compute the fields WEEK_OF_YEAR, YEAR_WOY, WEEK_OF_MONTH,
     * DAY_OF_WEEK_IN_MONTH, and DOW_LOCAL from EXTENDED_YEAR, YEAR,
     * DAY_OF_WEEK, and DAY_OF_YEAR.  The latter fields are computed by the
     * subclass based on the calendar system.
     *
     * <p>The YEAR_WOY field is computed simplistically.  It is equal to YEAR
     * most of the time, but at the year boundary it may be adjusted to YEAR-1
     * or YEAR+1 to reflect the overlap of a week into an adjacent year.  In
     * this case, a simple increment or decrement is performed on YEAR, even
     * though this may yield an invalid YEAR value.  For instance, if the YEAR
     * is part of a calendar system with an N-year cycle field CYCLE, then
     * incrementing the YEAR may involve incrementing CYCLE and setting YEAR
     * back to 0 or 1.  This is not handled by this code, and in fact cannot be
     * simply handled without having subclasses define an entire parallel set of
     * fields for fields larger than or equal to a year.  This additional
     * complexity is not warranted, since the intention of the YEAR_WOY field is
     * to support ISO 8601 notation, so it will typically be used with a
     * proleptic Gregorian calendar, which has no field larger than a year.
     */
    void computeWeekFields(UErrorCode &ec);


    /**
     * Ensure that each field is within its valid range by calling {@link
     * #validateField(int, int&)} on each field that has been set.  This method
     * should only be called if this calendar is not lenient.
     * @see #isLenient
     * @see #validateField(int, int&)
     * @internal
     */
    void validateFields(UErrorCode &status);

    /**
     * Validate a single field of this calendar.  Subclasses should
     * override this method to validate any calendar-specific fields.
     * Generic fields can be handled by
     * <code>Calendar.validateField()</code>.
     * @see #validateField(int, int, int, int&)
     * @internal
     */
    virtual void validateField(UCalendarDateFields field, UErrorCode &status);

    /**
     * Validate a single field of this calendar given its minimum and
     * maximum allowed value.  If the field is out of range,
     * <code>U_ILLEGAL_ARGUMENT_ERROR</code> will be set.  Subclasses may
     * use this method in their implementation of {@link
     * #validateField(int, int&)}.
     * @internal
     */
    void validateField(UCalendarDateFields field, int32_t min, int32_t max, UErrorCode& status);

 protected:
    /**
     * Convert a quasi Julian date to the day of the week. The Julian date used here is
     * not a true Julian date, since it is measured from midnight, not noon. Return
     * value is one-based.
     *
     * @param julian  The given Julian date number.
     * @return   Day number from 1..7 (SUN..SAT).
     * @internal
     */
    static uint8_t julianDayToDayOfWeek(double julian);

 private:
    char validLocale[ULOC_FULLNAME_CAPACITY];
    char actualLocale[ULOC_FULLNAME_CAPACITY];

 public:
#if !UCONFIG_NO_SERVICE
    /**
     * INTERNAL FOR 2.6 --  Registration.
     */

    /**
     * Return a StringEnumeration over the locales available at the time of the call,
     * including registered locales.
     * @return a StringEnumeration over the locales available at the time of the call
     * @internal
     */
    static StringEnumeration* getAvailableLocales(void);

    /**
     * Register a new Calendar factory.  The factory will be adopted.
     * INTERNAL in 2.6
     * @param toAdopt the factory instance to be adopted
     * @param status the in/out status code, no special meanings are assigned
     * @return a registry key that can be used to unregister this factory
     * @internal
     */
    static URegistryKey registerFactory(ICUServiceFactory* toAdopt, UErrorCode& status);

    /**
     * Unregister a previously-registered CalendarFactory using the key returned from the
     * register call.  Key becomes invalid after a successful call and should not be used again.
     * The CalendarFactory corresponding to the key will be deleted.
     * INTERNAL in 2.6
     * @param key the registry key returned by a previous call to registerFactory
     * @param status the in/out status code, no special meanings are assigned
     * @return TRUE if the factory for the key was successfully unregistered
     * @internal
     */
    static UBool unregister(URegistryKey key, UErrorCode& status);

    /**
     * Multiple Calendar Implementation
     * @internal
     */
    friend class CalendarFactory;

    /**
     * Multiple Calendar Implementation
     * @internal
     */
    friend class CalendarService;

    /**
     * Multiple Calendar Implementation
     * @internal
     */
    friend class DefaultCalendarFactory;
#endif /* !UCONFIG_NO_SERVICE */

    /**
     * @internal
     * @return TRUE if this calendar has a default century (i.e. 03 -> 2003)
     */
    virtual UBool haveDefaultCentury() const = 0;

    /**
     * @internal
     * @return the start of the default century, as a UDate
     */
    virtual UDate defaultCenturyStart() const = 0;
    /**
     * @internal
     * @return the beginning year of the default century, as a year
     */
    virtual int32_t defaultCenturyStartYear() const = 0;

    /** Get the locale for this calendar object. You can choose between valid and actual locale.
     *  @param type type of the locale we're looking for (valid or actual)
     *  @param status error code for the operation
     *  @return the locale
     *  @stable ICU 2.8
     */
    Locale getLocale(ULocDataLocaleType type, UErrorCode &status) const;

    /** Get the locale for this calendar object. You can choose between valid and actual locale.
     *  @param type type of the locale we're looking for (valid or actual)
     *  @param status error code for the operation
     *  @return the locale
     *  @internal
     */
    const char* getLocaleID(ULocDataLocaleType type, UErrorCode &status) const;

};

// -------------------------------------

inline Calendar*
Calendar::createInstance(TimeZone* zone, UErrorCode& errorCode)
{
    // since the Locale isn't specified, use the default locale
    return createInstance(zone, Locale::getDefault(), errorCode);
}

// -------------------------------------

inline void
Calendar::roll(UCalendarDateFields field, UBool up, UErrorCode& status)
{
    roll(field, (int32_t)(up ? +1 : -1), status);
}

inline void
Calendar::roll(EDateFields field, UBool up, UErrorCode& status)
{
    roll((UCalendarDateFields) field, up, status);
}


// -------------------------------------

/**
 * Fast method for subclasses.  The caller must maintain fUserSetDSTOffset and
 * fUserSetZoneOffset, as well as the isSet[] array.
 */

inline void
Calendar::internalSet(UCalendarDateFields field, int32_t value)
{
    fFields[field] = value;
    fStamp[field] = kInternallySet;
    fIsSet[field]     = TRUE; // Remove later
}

inline int32_t  Calendar::weekNumber(int32_t dayOfPeriod, int32_t dayOfWeek)
{
  return weekNumber(dayOfPeriod, dayOfPeriod, dayOfWeek);
}


U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _CALENDAR
