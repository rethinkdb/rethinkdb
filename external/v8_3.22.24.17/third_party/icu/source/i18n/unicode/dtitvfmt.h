/********************************************************************************
* Copyright (C) 2008-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File DTITVFMT.H
*
*******************************************************************************
*/

#ifndef __DTITVFMT_H__
#define __DTITVFMT_H__


#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: Format and parse date interval in a language-independent manner.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/ucal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtintrv.h"
#include "unicode/dtitvinf.h"
#include "unicode/dtptngen.h"

U_NAMESPACE_BEGIN



/**
 * DateIntervalFormat is a class for formatting and parsing date
 * intervals in a language-independent manner.
 * Only formatting is supported, parsing is not supported.
 *
 * <P>
 * Date interval means from one date to another date,
 * for example, from "Jan 11, 2008" to "Jan 18, 2008".
 * We introduced class DateInterval to represent it.
 * DateInterval is a pair of UDate, which is
 * the standard milliseconds since 24:00 GMT, Jan 1, 1970.
 *
 * <P>
 * DateIntervalFormat formats a DateInterval into
 * text as compactly as possible.
 * For example, the date interval format from "Jan 11, 2008" to "Jan 18,. 2008"
 * is "Jan 11-18, 2008" for English.
 * And it parses text into DateInterval,
 * although initially, parsing is not supported.
 *
 * <P>
 * There is no structural information in date time patterns.
 * For any punctuations and string literals inside a date time pattern,
 * we do not know whether it is just a separator, or a prefix, or a suffix.
 * Without such information, so, it is difficult to generate a sub-pattern
 * (or super-pattern) by algorithm.
 * So, formatting a DateInterval is pattern-driven. It is very
 * similar to formatting in SimpleDateFormat.
 * We introduce class DateIntervalInfo to save date interval
 * patterns, similar to date time pattern in SimpleDateFormat.
 *
 * <P>
 * Logically, the interval patterns are mappings
 * from (skeleton, the_largest_different_calendar_field)
 * to (date_interval_pattern).
 *
 * <P>
 * A skeleton
 * <ol>
 * <li>
 * only keeps the field pattern letter and ignores all other parts
 * in a pattern, such as space, punctuations, and string literals.
 * </li>
 * <li>
 * hides the order of fields.
 * </li>
 * <li>
 * might hide a field's pattern letter length.
 * </li>
 * </ol>
 *
 * For those non-digit calendar fields, the pattern letter length is
 * important, such as MMM, MMMM, and MMMMM; EEE and EEEE,
 * and the field's pattern letter length is honored.
 *
 * For the digit calendar fields,  such as M or MM, d or dd, yy or yyyy,
 * the field pattern length is ignored and the best match, which is defined
 * in date time patterns, will be returned without honor the field pattern
 * letter length in skeleton.
 *
 * <P>
 * The calendar fields we support for interval formatting are:
 * year, month, date, day-of-week, am-pm, hour, hour-of-day, and minute.
 * Those calendar fields can be defined in the following order:
 * year >  month > date > hour (in day) >  minute
 *
 * The largest different calendar fields between 2 calendars is the
 * first different calendar field in above order.
 *
 * For example: the largest different calendar fields between "Jan 10, 2007"
 * and "Feb 20, 2008" is year.
 *
 * <P>
 * For other calendar fields, the compact interval formatting is not
 * supported. And the interval format will be fall back to fall-back
 * patterns, which is mostly "{date0} - {date1}".
 *
 * <P>
 * There is a set of pre-defined static skeleton strings.
 * There are pre-defined interval patterns for those pre-defined skeletons
 * in locales' resource files.
 * For example, for a skeleton UDAT_YEAR_ABBR_MONTH_DAY, which is  &quot;yMMMd&quot;,
 * in  en_US, if the largest different calendar field between date1 and date2
 * is &quot;year&quot;, the date interval pattern  is &quot;MMM d, yyyy - MMM d, yyyy&quot;,
 * such as &quot;Jan 10, 2007 - Jan 10, 2008&quot;.
 * If the largest different calendar field between date1 and date2 is &quot;month&quot;,
 * the date interval pattern is &quot;MMM d - MMM d, yyyy&quot;,
 * such as &quot;Jan 10 - Feb 10, 2007&quot;.
 * If the largest different calendar field between date1 and date2 is &quot;day&quot;,
 * the date interval pattern is &quot;MMM d-d, yyyy&quot;, such as &quot;Jan 10-20, 2007&quot;.
 *
 * For date skeleton, the interval patterns when year, or month, or date is
 * different are defined in resource files.
 * For time skeleton, the interval patterns when am/pm, or hour, or minute is
 * different are defined in resource files.
 *
 * <P>
 * If a skeleton is not found in a locale's DateIntervalInfo, which means
 * the interval patterns for the skeleton is not defined in resource file,
 * the interval pattern will falls back to the interval "fallback" pattern
 * defined in resource file.
 * If the interval "fallback" pattern is not defined, the default fall-back
 * is "{date0} - {data1}".
 *
 * <P>
 * For the combination of date and time,
 * The rule to generate interval patterns are:
 * <ol>
 * <li>
 *    when the year, month, or day differs, falls back to fall-back
 *    interval pattern, which mostly is the concatenate the two original
 *    expressions with a separator between,
 *    For example, interval pattern from "Jan 10, 2007 10:10 am"
 *    to "Jan 11, 2007 10:10am" is
 *    "Jan 10, 2007 10:10 am - Jan 11, 2007 10:10am"
 * </li>
 * <li>
 *    otherwise, present the date followed by the range expression
 *    for the time.
 *    For example, interval pattern from "Jan 10, 2007 10:10 am"
 *    to "Jan 10, 2007 11:10am" is "Jan 10, 2007 10:10 am - 11:10am"
 * </li>
 * </ol>
 *
 *
 * <P>
 * If two dates are the same, the interval pattern is the single date pattern.
 * For example, interval pattern from "Jan 10, 2007" to "Jan 10, 2007" is
 * "Jan 10, 2007".
 *
 * Or if the presenting fields between 2 dates have the exact same values,
 * the interval pattern is the  single date pattern.
 * For example, if user only requests year and month,
 * the interval pattern from "Jan 10, 2007" to "Jan 20, 2007" is "Jan 2007".
 *
 * <P>
 * DateIntervalFormat needs the following information for correct
 * formatting: time zone, calendar type, pattern, date format symbols,
 * and date interval patterns.
 * It can be instantiated in 2 ways:
 * <ol>
 * <li>
 *    create an instance using default or given locale plus given skeleton.
 *    Users are encouraged to created date interval formatter this way and
 *    to use the pre-defined skeleton macros, such as
 *    UDAT_YEAR_NUM_MONTH, which consists the calendar fields and
 *    the format style.
 * </li>
 * <li>
 *    create an instance using default or given locale plus given skeleton
 *    plus a given DateIntervalInfo.
 *    This factory method is for powerful users who want to provide their own
 *    interval patterns.
 *    Locale provides the timezone, calendar, and format symbols information.
 *    Local plus skeleton provides full pattern information.
 *    DateIntervalInfo provides the date interval patterns.
 * </li>
 * </ol>
 *
 * <P>
 * For the calendar field pattern letter, such as G, y, M, d, a, h, H, m, s etc.
 * DateIntervalFormat uses the same syntax as that of
 * DateTime format.
 *
 * <P>
 * Code Sample: general usage
 * <pre>
 * \code
 *   // the date interval object which the DateIntervalFormat formats on
 *   // and parses into
 *   DateInterval*  dtInterval = new DateInterval(1000*3600*24, 1000*3600*24*2);
 *   UErrorCode status = U_ZERO_ERROR;
 *   DateIntervalFormat* dtIntervalFmt = DateIntervalFormat::createInstance(
 *                           UDAT_YEAR_MONTH_DAY,
 *                           Locale("en", "GB", ""), status);
 *   UnicodeUnicodeString dateIntervalString;
 *   FieldPosition pos = 0;
 *   // formatting
 *   dtIntervalFmt->format(dtInterval, dateIntervalUnicodeString, pos, status);
 *   delete dtIntervalFmt;
 * \endcode
 * </pre>
 */

class U_I18N_API DateIntervalFormat : public Format {
public:

    /**
     * Construct a DateIntervalFormat from skeleton and  the default locale.
     *
     * This is a convenient override of
     * createInstance(const UnicodeString& skeleton, const Locale& locale,
     *                UErrorCode&)
     * with the value of locale as default locale.
     *
     * @param skeleton  the skeleton on which interval format based.
     * @param status    output param set to success/failure code on exit
     * @return          a date time interval formatter which the caller owns.
     * @stable ICU 4.0
     */
    static DateIntervalFormat* U_EXPORT2 createInstance(
                                               const UnicodeString& skeleton,
                                               UErrorCode& status);

    /**
     * Construct a DateIntervalFormat from skeleton and a given locale.
     * <P>
     * In this factory method,
     * the date interval pattern information is load from resource files.
     * Users are encouraged to created date interval formatter this way and
     * to use the pre-defined skeleton macros.
     *
     * <P>
     * There are pre-defined skeletons (defined in udate.h) having predefined
     * interval patterns in resource files.
     * Users are encouraged to use those macros.
     * For example:
     * DateIntervalFormat::createInstance(UDAT_MONTH_DAY, status)
     *
     * The given Locale provides the interval patterns.
     * For example, for en_GB, if skeleton is UDAT_YEAR_ABBR_MONTH_WEEKDAY_DAY,
     * which is "yMMMEEEd",
     * the interval patterns defined in resource file to above skeleton are:
     * "EEE, d MMM, yyyy - EEE, d MMM, yyyy" for year differs,
     * "EEE, d MMM - EEE, d MMM, yyyy" for month differs,
     * "EEE, d - EEE, d MMM, yyyy" for day differs,
     * @param skeleton  the skeleton on which interval format based.
     * @param locale    the given locale
     * @param status    output param set to success/failure code on exit
     * @return          a date time interval formatter which the caller owns.
     * @stable ICU 4.0
     */

    static DateIntervalFormat* U_EXPORT2 createInstance(
                                               const UnicodeString& skeleton,
                                               const Locale& locale,
                                               UErrorCode& status);

    /**
     * Construct a DateIntervalFormat from skeleton
     *  DateIntervalInfo, and default locale.
     *
     * This is a convenient override of
     * createInstance(const UnicodeString& skeleton, const Locale& locale,
     *                const DateIntervalInfo& dtitvinf, UErrorCode&)
     * with the locale value as default locale.
     *
     * @param skeleton  the skeleton on which interval format based.
     * @param dtitvinf  the DateIntervalInfo object.
     * @param status    output param set to success/failure code on exit
     * @return          a date time interval formatter which the caller owns.
     * @stable ICU 4.0
     */
    static DateIntervalFormat* U_EXPORT2 createInstance(
                                              const UnicodeString& skeleton,
                                              const DateIntervalInfo& dtitvinf,
                                              UErrorCode& status);

    /**
     * Construct a DateIntervalFormat from skeleton
     * a DateIntervalInfo, and the given locale.
     *
     * <P>
     * In this factory method, user provides its own date interval pattern
     * information, instead of using those pre-defined data in resource file.
     * This factory method is for powerful users who want to provide their own
     * interval patterns.
     * <P>
     * There are pre-defined skeletons (defined in udate.h) having predefined
     * interval patterns in resource files.
     * Users are encouraged to use those macros.
     * For example:
     * DateIntervalFormat::createInstance(UDAT_MONTH_DAY, status)
     *
     * The DateIntervalInfo provides the interval patterns.
     * and the DateIntervalInfo ownership remains to the caller.
     *
     * User are encouraged to set default interval pattern in DateIntervalInfo
     * as well, if they want to set other interval patterns ( instead of
     * reading the interval patterns from resource files).
     * When the corresponding interval pattern for a largest calendar different
     * field is not found ( if user not set it ), interval format fallback to
     * the default interval pattern.
     * If user does not provide default interval pattern, it fallback to
     * "{date0} - {date1}"
     *
     * @param skeleton  the skeleton on which interval format based.
     * @param locale    the given locale
     * @param dtitvinf  the DateIntervalInfo object.
     * @param status    output param set to success/failure code on exit
     * @return          a date time interval formatter which the caller owns.
     * @stable ICU 4.0
     */
    static DateIntervalFormat* U_EXPORT2 createInstance(
                                              const UnicodeString& skeleton,
                                              const Locale& locale,
                                              const DateIntervalInfo& dtitvinf,
                                              UErrorCode& status);

    /**
     * Destructor.
     * @stable ICU 4.0
     */
    virtual ~DateIntervalFormat();

    /**
     * Clone this Format object polymorphically. The caller owns the result and
     * should delete it when done.
     * @return    A copy of the object.
     * @stable ICU 4.0
     */
    virtual Format* clone(void) const;

    /**
     * Return true if the given Format objects are semantically equal. Objects
     * of different subclasses are considered unequal.
     * @param other    the object to be compared with.
     * @return         true if the given Format objects are semantically equal.
     * @stable ICU 4.0
     */
    virtual UBool operator==(const Format& other) const;

    /**
     * Return true if the given Format objects are not semantically equal.
     * Objects of different subclasses are considered unequal.
     * @param other the object to be compared with.
     * @return      true if the given Format objects are not semantically equal.
     * @stable ICU 4.0
     */
    UBool operator!=(const Format& other) const;


    using Format::format;

    /**
     * Format an object to produce a string. This method handles Formattable
     * objects with a DateInterval type.
     * If a the Formattable object type is not a DateInterval,
     * then it returns a failing UErrorCode.
     *
     * @param obj               The object to format.
     *                          Must be a DateInterval.
     * @param appendTo          Output parameter to receive result.
     *                          Result is appended to existing contents.
     * @param fieldPosition     On input: an alignment field, if desired.
     *                          On output: the offsets of the alignment field.
     * @param status            Output param filled with success/failure status.
     * @return                  Reference to 'appendTo' parameter.
     * @stable ICU 4.0
     */
    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& fieldPosition,
                                  UErrorCode& status) const ;



    /**
     * Format a DateInterval to produce a string.
     *
     * @param dtInterval        DateInterval to be formatted.
     * @param appendTo          Output parameter to receive result.
     *                          Result is appended to existing contents.
     * @param fieldPosition     On input: an alignment field, if desired.
     *                          On output: the offsets of the alignment field.
     * @param status            Output param filled with success/failure status.
     * @return                  Reference to 'appendTo' parameter.
     * @stable ICU 4.0
     */
    UnicodeString& format(const DateInterval* dtInterval,
                          UnicodeString& appendTo,
                          FieldPosition& fieldPosition,
                          UErrorCode& status) const ;


    /**
     * Format 2 Calendars to produce a string.
     *
     * Note: "fromCalendar" and "toCalendar" are not const,
     * since calendar is not const in  SimpleDateFormat::format(Calendar&),
     *
     * @param fromCalendar      calendar set to the from date in date interval
     *                          to be formatted into date interval string
     * @param toCalendar        calendar set to the to date in date interval
     *                          to be formatted into date interval string
     * @param appendTo          Output parameter to receive result.
     *                          Result is appended to existing contents.
     * @param fieldPosition     On input: an alignment field, if desired.
     *                          On output: the offsets of the alignment field.
     * @param status            Output param filled with success/failure status.
     *                          Caller needs to make sure it is SUCCESS
     *                          at the function entrance
     * @return                  Reference to 'appendTo' parameter.
     * @stable ICU 4.0
     */
    UnicodeString& format(Calendar& fromCalendar,
                          Calendar& toCalendar,
                          UnicodeString& appendTo,
                          FieldPosition& fieldPosition,
                          UErrorCode& status) const ;

    /**
     * Date interval parsing is not supported. Please do not use.
     * <P>
     * This method should handle parsing of
     * date time interval strings into Formattable objects with
     * DateInterval type, which is a pair of UDate.
     * <P>
     * Before calling, set parse_pos.index to the offset you want to start
     * parsing at in the source. After calling, parse_pos.index is the end of
     * the text you parsed. If error occurs, index is unchanged.
     * <P>
     * When parsing, leading whitespace is discarded (with a successful parse),
     * while trailing whitespace is left as is.
     * <P>
     * See Format::parseObject() for more.
     *
     * @param source    The string to be parsed into an object.
     * @param result    Formattable to be set to the parse result.
     *                  If parse fails, return contents are undefined.
     * @param parse_pos The position to start parsing at. Since no parsing
     *                  is supported, upon return this param is unchanged.
     * @return          A newly created Formattable* object, or NULL
     *                  on failure.  The caller owns this and should
     *                  delete it when done.
     * @internal ICU 4.0
     */
    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const;


    /**
     * Gets the date time interval patterns.
     * @return the date time interval patterns associated with
     * this date interval formatter.
     * @stable ICU 4.0
     */
    const DateIntervalInfo* getDateIntervalInfo(void) const;


    /**
     * Set the date time interval patterns.
     * @param newIntervalPatterns   the given interval patterns to copy.
     * @param status          output param set to success/failure code on exit
     * @stable ICU 4.0
     */
    void setDateIntervalInfo(const DateIntervalInfo& newIntervalPatterns,
                             UErrorCode& status);


    /**
     * Gets the date formatter
     * @return the date formatter associated with this date interval formatter.
     * @stable ICU 4.0
     */
    const DateFormat* getDateFormat(void) const;

    /**
     * Return the class ID for this class. This is useful only for comparing to
     * a return value from getDynamicClassID(). For example:
     * <pre>
     * .   Base* polymorphic_pointer = createPolymorphicObject();
     * .   if (polymorphic_pointer->getDynamicClassID() ==
     * .       erived::getStaticClassID()) ...
     * </pre>
     * @return          The class ID for all objects of this class.
     * @stable ICU 4.0
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * Returns a unique class ID POLYMORPHICALLY. Pure virtual override. This
     * method is to implement a simple version of RTTI, since not all C++
     * compilers support genuine RTTI. Polymorphic operator==() and clone()
     * methods call this method.
     *
     * @return          The class ID for this object. All objects of a
     *                  given class have the same class ID.  Objects of
     *                  other classes have different class IDs.
     * @stable ICU 4.0
     */
    virtual UClassID getDynamicClassID(void) const;

protected:

    /**
     * Copy constructor.
     * @stable ICU 4.0
     */
    DateIntervalFormat(const DateIntervalFormat&);

    /**
     * Assignment operator.
     * @stable ICU 4.0
     */
    DateIntervalFormat& operator=(const DateIntervalFormat&);

private:

    /*
     * This is for ICU internal use only. Please do not use.
     * Save the interval pattern information.
     * Interval pattern consists of 2 single date patterns and the separator.
     * For example, interval pattern "MMM d - MMM d, yyyy" consists
     * a single date pattern "MMM d", another single date pattern "MMM d, yyyy",
     * and a separator "-".
     * The pattern is divided into 2 parts. For above example,
     * the first part is "MMM d - ", and the second part is "MMM d, yyyy".
     * Also, the first date appears in an interval pattern could be
     * the earlier date or the later date.
     * And such information is saved in the interval pattern as well.
     * @internal ICU 4.0
     */
    struct PatternInfo {
        UnicodeString firstPart;
        UnicodeString secondPart;
        /**
         * Whether the first date in interval pattern is later date or not.
         * Fallback format set the default ordering.
         * And for a particular interval pattern, the order can be
         * overriden by prefixing the interval pattern with "latestFirst:" or
         * "earliestFirst:"
         * For example, given 2 date, Jan 10, 2007 to Feb 10, 2007.
         * if the fallback format is "{0} - {1}",
         * and the pattern is "d MMM - d MMM yyyy", the interval format is
         * "10 Jan - 10 Feb, 2007".
         * If the pattern is "latestFirst:d MMM - d MMM yyyy",
         * the interval format is "10 Feb - 10 Jan, 2007"
         */
        UBool         laterDateFirst;
    };


    /**
     * default constructor
     * @internal ICU 4.0
     */
    DateIntervalFormat();

    /**
     * Construct a DateIntervalFormat from DateFormat,
     * a DateIntervalInfo, and skeleton.
     * DateFormat provides the timezone, calendar,
     * full pattern, and date format symbols information.
     * It should be a SimpleDateFormat object which
     * has a pattern in it.
     * the DateIntervalInfo provides the interval patterns.
     *
     * Note: the DateIntervalFormat takes ownership of both
     * DateFormat and DateIntervalInfo objects.
     * Caller should not delete them.
     *
     * @param locale    the locale of this date interval formatter.
     * @param dtitvinf  the DateIntervalInfo object to be adopted.
     * @param skeleton  the skeleton of the date formatter
     * @param status    output param set to success/failure code on exit
     * @internal ICU 4.0
     */
    DateIntervalFormat(const Locale& locale, DateIntervalInfo* dtItvInfo,
                       const UnicodeString* skeleton, UErrorCode& status);


    /**
     * Construct a DateIntervalFormat from DateFormat
     * and a DateIntervalInfo.
     *
     * It is a wrapper of the constructor.
     *
     * @param locale    the locale of this date interval formatter.
     * @param dtitvinf  the DateIntervalInfo object to be adopted.
     * @param skeleton  the skeleton of this formatter.
     * @param status    Output param set to success/failure code.
     * @return          a date time interval formatter which the caller owns.
     * @internal ICU 4.0
     */
    static DateIntervalFormat* U_EXPORT2 create(const Locale& locale,
                                                DateIntervalInfo* dtitvinf,
                                                const UnicodeString* skeleton,
                                                UErrorCode& status);

    /**
     * Create a simple date/time formatter from skeleton, given locale,
     * and date time pattern generator.
     *
     * @param skeleton  the skeleton on which date format based.
     * @param locale    the given locale.
     * @param dtpng     the date time pattern generator.
     * @param status    Output param to be set to success/failure code.
     *                  If it is failure, the returned date formatter will
     *                  be NULL.
     * @return          a simple date formatter which the caller owns.
     * @internal ICU 4.0
     */
    static SimpleDateFormat* U_EXPORT2 createSDFPatternInstance(
                                        const UnicodeString& skeleton,
                                        const Locale& locale,
                                        DateTimePatternGenerator* dtpng,
                                        UErrorCode& status);


    /**
     *  Below are for generating interval patterns local to the formatter
     */


    /**
     * Format 2 Calendars using fall-back interval pattern
     *
     * The full pattern used in this fall-back format is the
     * full pattern of the date formatter.
     *
     * @param fromCalendar      calendar set to the from date in date interval
     *                          to be formatted into date interval string
     * @param toCalendar        calendar set to the to date in date interval
     *                          to be formatted into date interval string
     * @param appendTo          Output parameter to receive result.
     *                          Result is appended to existing contents.
     * @param pos               On input: an alignment field, if desired.
     *                          On output: the offsets of the alignment field.
     * @param status            output param set to success/failure code on exit
     * @return                  Reference to 'appendTo' parameter.
     * @internal ICU 4.0
     */
    UnicodeString& fallbackFormat(Calendar& fromCalendar,
                                  Calendar& toCalendar,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;



    /**
     * Initialize interval patterns locale to this formatter
     *
     * This code is a bit complicated since
     * 1. the interval patterns saved in resource bundle files are interval
     *    patterns based on date or time only.
     *    It does not have interval patterns based on both date and time.
     *    Interval patterns on both date and time are algorithm generated.
     *
     *    For example, it has interval patterns on skeleton "dMy" and "hm",
     *    but it does not have interval patterns on skeleton "dMyhm".
     *
     *    The rule to generate interval patterns for both date and time skeleton are
     *    1) when the year, month, or day differs, concatenate the two original
     *    expressions with a separator between,
     *    For example, interval pattern from "Jan 10, 2007 10:10 am"
     *    to "Jan 11, 2007 10:10am" is
     *    "Jan 10, 2007 10:10 am - Jan 11, 2007 10:10am"
     *
     *    2) otherwise, present the date followed by the range expression
     *    for the time.
     *    For example, interval pattern from "Jan 10, 2007 10:10 am"
     *    to "Jan 10, 2007 11:10am" is
     *    "Jan 10, 2007 10:10 am - 11:10am"
     *
     * 2. even a pattern does not request a certain calendar field,
     *    the interval pattern needs to include such field if such fields are
     *    different between 2 dates.
     *    For example, a pattern/skeleton is "hm", but the interval pattern
     *    includes year, month, and date when year, month, and date differs.
     *
     *
     * @param status    output param set to success/failure code on exit
     * @internal ICU 4.0
     */
    void initializePattern(UErrorCode& status);



    /**
     * Set fall back interval pattern given a calendar field,
     * a skeleton, and a date time pattern generator.
     * @param field      the largest different calendar field
     * @param skeleton   a skeleton
     * @param status     output param set to success/failure code on exit
     * @internal ICU 4.0
     */
    void setFallbackPattern(UCalendarDateFields field,
                            const UnicodeString& skeleton,
                            UErrorCode& status);



    /**
     * get separated date and time skeleton from a combined skeleton.
     *
     * The difference between date skeleton and normalizedDateSkeleton are:
     * 1. both 'y' and 'd' are appeared only once in normalizeDateSkeleton
     * 2. 'E' and 'EE' are normalized into 'EEE'
     * 3. 'MM' is normalized into 'M'
     *
     ** the difference between time skeleton and normalizedTimeSkeleton are:
     * 1. both 'H' and 'h' are normalized as 'h' in normalized time skeleton,
     * 2. 'a' is omitted in normalized time skeleton.
     * 3. there is only one appearance for 'h', 'm','v', 'z' in normalized time
     *    skeleton
     *
     *
     *  @param skeleton               given combined skeleton.
     *  @param date                   Output parameter for date only skeleton.
     *  @param normalizedDate         Output parameter for normalized date only
     *
     *  @param time                   Output parameter for time only skeleton.
     *  @param normalizedTime         Output parameter for normalized time only
     *                                skeleton.
     *
     * @internal ICU 4.0
     */
    static void  U_EXPORT2 getDateTimeSkeleton(const UnicodeString& skeleton,
                                    UnicodeString& date,
                                    UnicodeString& normalizedDate,
                                    UnicodeString& time,
                                    UnicodeString& normalizedTime);



    /**
     * Generate date or time interval pattern from resource,
     * and set them into the interval pattern locale to this formatter.
     *
     * It needs to handle the following:
     * 1. need to adjust field width.
     *    For example, the interval patterns saved in DateIntervalInfo
     *    includes "dMMMy", but not "dMMMMy".
     *    Need to get interval patterns for dMMMMy from dMMMy.
     *    Another example, the interval patterns saved in DateIntervalInfo
     *    includes "hmv", but not "hmz".
     *    Need to get interval patterns for "hmz' from 'hmv'
     *
     * 2. there might be no pattern for 'y' differ for skeleton "Md",
     *    in order to get interval patterns for 'y' differ,
     *    need to look for it from skeleton 'yMd'
     *
     * @param dateSkeleton   normalized date skeleton
     * @param timeSkeleton   normalized time skeleton
     * @return               whether the resource is found for the skeleton.
     *                       TRUE if interval pattern found for the skeleton,
     *                       FALSE otherwise.
     * @internal ICU 4.0
     */
    UBool setSeparateDateTimePtn(const UnicodeString& dateSkeleton,
                                 const UnicodeString& timeSkeleton);




    /**
     * Generate interval pattern from existing resource
     *
     * It not only save the interval patterns,
     * but also return the extended skeleton and its best match skeleton.
     *
     * @param field           largest different calendar field
     * @param skeleton        skeleton
     * @param bestSkeleton    the best match skeleton which has interval pattern
     *                        defined in resource
     * @param differenceInfo  the difference between skeleton and best skeleton
     *         0 means the best matched skeleton is the same as input skeleton
     *         1 means the fields are the same, but field width are different
     *         2 means the only difference between fields are v/z,
     *        -1 means there are other fields difference
     *
     * @param extendedSkeleton      extended skeleton
     * @param extendedBestSkeleton  extended best match skeleton
     * @return                      whether the interval pattern is found
     *                              through extending skeleton or not.
     *                              TRUE if interval pattern is found by
     *                              extending skeleton, FALSE otherwise.
     * @internal ICU 4.0
     */
    UBool setIntervalPattern(UCalendarDateFields field,
                             const UnicodeString* skeleton,
                             const UnicodeString* bestSkeleton,
                             int8_t differenceInfo,
                             UnicodeString* extendedSkeleton = NULL,
                             UnicodeString* extendedBestSkeleton = NULL);

    /**
     * Adjust field width in best match interval pattern to match
     * the field width in input skeleton.
     *
     * TODO (xji) make a general solution
     * The adjusting rule can be:
     * 1. always adjust
     * 2. never adjust
     * 3. default adjust, which means adjust according to the following rules
     * 3.1 always adjust string, such as MMM and MMMM
     * 3.2 never adjust between string and numeric, such as MM and MMM
     * 3.3 always adjust year
     * 3.4 do not adjust 'd', 'h', or 'm' if h presents
     * 3.5 do not adjust 'M' if it is numeric(?)
     *
     * Since date interval format is well-formed format,
     * date and time skeletons are normalized previously,
     * till this stage, the adjust here is only "adjust strings, such as MMM
     * and MMMM, EEE and EEEE.
     *
     * @param inputSkeleton            the input skeleton
     * @param bestMatchSkeleton        the best match skeleton
     * @param bestMatchIntervalpattern the best match interval pattern
     * @param differenceInfo           the difference between 2 skeletons
     *                                 1 means only field width differs
     *                                 2 means v/z exchange
     * @param adjustedIntervalPattern  adjusted interval pattern
     * @internal ICU 4.0
     */
    static void U_EXPORT2 adjustFieldWidth(
                            const UnicodeString& inputSkeleton,
                            const UnicodeString& bestMatchSkeleton,
                            const UnicodeString& bestMatchIntervalPattern,
                            int8_t differenceInfo,
                            UnicodeString& adjustedIntervalPattern);

    /**
     * Concat a single date pattern with a time interval pattern,
     * set it into the intervalPatterns, while field is time field.
     * This is used to handle time interval patterns on skeleton with
     * both time and date. Present the date followed by
     * the range expression for the time.
     * @param format         date and time format
     * @param formatLen      format string length
     * @param datePattern    date pattern
     * @param field          time calendar field: AM_PM, HOUR, MINUTE
     * @param status         output param set to success/failure code on exit
     * @internal ICU 4.0
     */
    void concatSingleDate2TimeInterval(const UChar* format,
                                       int32_t formatLen,
                                       const UnicodeString& datePattern,
                                       UCalendarDateFields field,
                                       UErrorCode& status);

    /**
     * check whether a calendar field present in a skeleton.
     * @param field      calendar field need to check
     * @param skeleton   given skeleton on which to check the calendar field
     * @return           true if field present in a skeleton.
     * @internal ICU 4.0
     */
    static UBool U_EXPORT2 fieldExistsInSkeleton(UCalendarDateFields field,
                                                 const UnicodeString& skeleton);


    /**
     * Split interval patterns into 2 part.
     * @param intervalPattern  interval pattern
     * @return the index in interval pattern which split the pattern into 2 part
     * @internal ICU 4.0
     */
    static int32_t  U_EXPORT2 splitPatternInto2Part(const UnicodeString& intervalPattern);


    /**
     * Break interval patterns as 2 part and save them into pattern info.
     * @param field            calendar field
     * @param intervalPattern  interval pattern
     * @internal ICU 4.0
     */
    void setIntervalPattern(UCalendarDateFields field,
                            const UnicodeString& intervalPattern);


    /**
     * Break interval patterns as 2 part and save them into pattern info.
     * @param field            calendar field
     * @param intervalPattern  interval pattern
     * @param laterDateFirst   whether later date appear first in interval pattern
     * @internal ICU 4.0
     */
    void setIntervalPattern(UCalendarDateFields field,
                            const UnicodeString& intervalPattern,
                            UBool laterDateFirst);


    /**
     * Set pattern information.
     *
     * @param field            calendar field
     * @param firstPart        the first part in interval pattern
     * @param secondPart       the second part in interval pattern
     * @param laterDateFirst   whether the first date in intervalPattern
     *                         is earlier date or later date
     * @internal ICU 4.0
     */
    void setPatternInfo(UCalendarDateFields field,
                        const UnicodeString* firstPart,
                        const UnicodeString* secondpart,
                        UBool laterDateFirst);


    // from calendar field to pattern letter
    static const UChar fgCalendarFieldToPatternLetter[];


    /**
     * The interval patterns for this locale.
     */
    DateIntervalInfo*     fInfo;

    /**
     * The DateFormat object used to format single pattern
     */
    SimpleDateFormat*     fDateFormat;

    /**
     * The 2 calendars with the from and to date.
     * could re-use the calendar in fDateFormat,
     * but keeping 2 calendars make it clear and clean.
     */
    Calendar* fFromCalendar;
    Calendar* fToCalendar;

    /**
     * Date time pattern generator
     */
    DateTimePatternGenerator* fDtpng;

    /**
     * Following are interval information relavent (locale) to this formatter.
     */
    UnicodeString fSkeleton;
    PatternInfo fIntervalPatterns[DateIntervalInfo::kIPI_MAX_INDEX];
};

inline UBool
DateIntervalFormat::operator!=(const Format& other) const  {
    return !operator==(other);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _DTITVFMT_H__
//eof
