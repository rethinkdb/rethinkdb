/*
* Copyright (C) 1997-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File SMPDTFMT.H
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   07/09/97    helena      Make ParsePosition into a class.
*   07/21/98    stephen     Added GMT_PLUS, GMT_MINUS
*                            Changed setTwoDigitStartDate to set2DigitYearStart
*                            Changed getTwoDigitStartDate to get2DigitYearStart
*                            Removed subParseLong
*                            Removed getZoneIndex (added in DateFormatSymbols)
*   06/14/99    stephen     Removed fgTimeZoneDataSuffix
*   10/14/99    aliu        Updated class doc to describe 2-digit year parsing
*                           {j28 4182066}.
*******************************************************************************
*/

#ifndef SMPDTFMT_H
#define SMPDTFMT_H

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: Format and parse dates in a language-independent manner.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"

U_NAMESPACE_BEGIN

class DateFormatSymbols;
class DateFormat;
class MessageFormat;
class FieldPositionHandler;

/**
 *
 * SimpleDateFormat is a concrete class for formatting and parsing dates in a
 * language-independent manner. It allows for formatting (millis -> text),
 * parsing (text -> millis), and normalization. Formats/Parses a date or time,
 * which is the standard milliseconds since 24:00 GMT, Jan 1, 1970.
 * <P>
 * Clients are encouraged to create a date-time formatter using DateFormat::getInstance(),
 * getDateInstance(), getDateInstance(), or getDateTimeInstance() rather than
 * explicitly constructing an instance of SimpleDateFormat.  This way, the client
 * is guaranteed to get an appropriate formatting pattern for whatever locale the
 * program is running in.  However, if the client needs something more unusual than
 * the default patterns in the locales, he can construct a SimpleDateFormat directly
 * and give it an appropriate pattern (or use one of the factory methods on DateFormat
 * and modify the pattern after the fact with toPattern() and applyPattern().
 * <P>
 * Date/Time format syntax:
 * <P>
 * The date/time format is specified by means of a string time pattern. In this
 * pattern, all ASCII letters are reserved as pattern letters, which are defined
 * as the following:
 * <pre>
 * \code
 * Symbol   Meaning                 Presentation        Example
 * ------   -------                 ------------        -------
 * G        era designator          (Text)              AD
 * y        year                    (Number)            1996
 * Y        year (week of year)     (Number)            1997
 * u        extended year           (Number)            4601
 * Q        Quarter                 (Text & Number)     Q2 & 02
 * M        month in year           (Text & Number)     July & 07
 * d        day in month            (Number)            10
 * h        hour in am/pm (1~12)    (Number)            12
 * H        hour in day (0~23)      (Number)            0
 * m        minute in hour          (Number)            30
 * s        second in minute        (Number)            55
 * S        fractional second       (Number)            978
 * E        day of week             (Text)              Tuesday
 * e        day of week (local 1~7) (Text & Number)     Tues & 2
 * D        day in year             (Number)            189
 * F        day of week in month    (Number)            2 (2nd Wed in July)
 * w        week in year            (Number)            27
 * W        week in month           (Number)            2
 * a        am/pm marker            (Text)              PM
 * k        hour in day (1~24)      (Number)            24
 * K        hour in am/pm (0~11)    (Number)            0
 * z        time zone               (Time)              Pacific Standard Time
 * Z        time zone (RFC 822)     (Number)            -0800
 * v        time zone (generic)     (Text)              Pacific Time
 * V        time zone (abreviation) (Text)              PT
 * VVVV     time zone (location)    (Text)              United States (Los Angeles)
 * g        Julian day              (Number)            2451334
 * A        milliseconds in day     (Number)            69540000
 * q        stand alone quarter     (Text & Number)     Q2 & 02
 * L        stand alone month       (Text & Number)     July & 07
 * c        stand alone day of week (Text & Number)     Tuesday & 2
 * '        escape for text         (Delimiter)         'Date='
 * ''       single quote            (Literal)           'o''clock'
 * \endcode
 * </pre>
 * The count of pattern letters determine the format.
 * <P>
 * (Text): 4 or more, use full form, &lt;4, use short or abbreviated form if it
 * exists. (e.g., "EEEE" produces "Monday", "EEE" produces "Mon")
 * <P>
 * (Number): the minimum number of digits. Shorter numbers are zero-padded to
 * this amount (e.g. if "m" produces "6", "mm" produces "06"). Year is handled
 * specially; that is, if the count of 'y' is 2, the Year will be truncated to 2 digits.
 * (e.g., if "yyyy" produces "1997", "yy" produces "97".)
 * Unlike other fields, fractional seconds are padded on the right with zero.
 * <P>
 * (Text & Number): 3 or over, use text, otherwise use number.  (e.g., "M" produces "1",
 * "MM" produces "01", "MMM" produces "Jan", and "MMMM" produces "January".)
 * <P>
 * Any characters in the pattern that are not in the ranges of ['a'..'z'] and
 * ['A'..'Z'] will be treated as quoted text. For instance, characters
 * like ':', '.', ' ', '#' and '@' will appear in the resulting time text
 * even they are not embraced within single quotes.
 * <P>
 * A pattern containing any invalid pattern letter will result in a failing
 * UErrorCode result during formatting or parsing.
 * <P>
 * Examples using the US locale:
 * <pre>
 * \code
 *    Format Pattern                         Result
 *    --------------                         -------
 *    "yyyy.MM.dd G 'at' HH:mm:ss vvvv" ->>  1996.07.10 AD at 15:08:56 Pacific Time
 *    "EEE, MMM d, ''yy"                ->>  Wed, July 10, '96
 *    "h:mm a"                          ->>  12:08 PM
 *    "hh 'o''clock' a, zzzz"           ->>  12 o'clock PM, Pacific Daylight Time
 *    "K:mm a, vvv"                     ->>  0:00 PM, PT
 *    "yyyyy.MMMMM.dd GGG hh:mm aaa"    ->>  1996.July.10 AD 12:08 PM
 * \endcode
 * </pre>
 * Code Sample:
 * <pre>
 * \code
 *     UErrorCode success = U_ZERO_ERROR;
 *     SimpleTimeZone* pdt = new SimpleTimeZone(-8 * 60 * 60 * 1000, "PST");
 *     pdt->setStartRule( Calendar::APRIL, 1, Calendar::SUNDAY, 2*60*60*1000);
 *     pdt->setEndRule( Calendar::OCTOBER, -1, Calendar::SUNDAY, 2*60*60*1000);
 *
 *     // Format the current time.
 *     SimpleDateFormat* formatter
 *         = new SimpleDateFormat ("yyyy.MM.dd G 'at' hh:mm:ss a zzz", success );
 *     GregorianCalendar cal(success);
 *     UDate currentTime_1 = cal.getTime(success);
 *     FieldPosition fp(0);
 *     UnicodeString dateString;
 *     formatter->format( currentTime_1, dateString, fp );
 *     cout << "result: " << dateString << endl;
 *
 *     // Parse the previous string back into a Date.
 *     ParsePosition pp(0);
 *     UDate currentTime_2 = formatter->parse(dateString, pp );
 * \endcode
 * </pre>
 * In the above example, the time value "currentTime_2" obtained from parsing
 * will be equal to currentTime_1. However, they may not be equal if the am/pm
 * marker 'a' is left out from the format pattern while the "hour in am/pm"
 * pattern symbol is used. This information loss can happen when formatting the
 * time in PM.
 *
 * <p>
 * When parsing a date string using the abbreviated year pattern ("y" or "yy"),
 * SimpleDateFormat must interpret the abbreviated year
 * relative to some century.  It does this by adjusting dates to be
 * within 80 years before and 20 years after the time the SimpleDateFormat
 * instance is created. For example, using a pattern of "MM/dd/yy" and a
 * SimpleDateFormat instance created on Jan 1, 1997,  the string
 * "01/11/12" would be interpreted as Jan 11, 2012 while the string "05/04/64"
 * would be interpreted as May 4, 1964.
 * During parsing, only strings consisting of exactly two digits, as defined by
 * <code>Unicode::isDigit()</code>, will be parsed into the default century.
 * Any other numeric string, such as a one digit string, a three or more digit
 * string, or a two digit string that isn't all digits (for example, "-1"), is
 * interpreted literally.  So "01/02/3" or "01/02/003" are parsed, using the
 * same pattern, as Jan 2, 3 AD.  Likewise, "01/02/-3" is parsed as Jan 2, 4 BC.
 *
 * <p>
 * If the year pattern has more than two 'y' characters, the year is
 * interpreted literally, regardless of the number of digits.  So using the
 * pattern "MM/dd/yyyy", "01/11/12" parses to Jan 11, 12 A.D.
 *
 * <p>
 * When numeric fields abut one another directly, with no intervening delimiter
 * characters, they constitute a run of abutting numeric fields.  Such runs are
 * parsed specially.  For example, the format "HHmmss" parses the input text
 * "123456" to 12:34:56, parses the input text "12345" to 1:23:45, and fails to
 * parse "1234".  In other words, the leftmost field of the run is flexible,
 * while the others keep a fixed width.  If the parse fails anywhere in the run,
 * then the leftmost field is shortened by one character, and the entire run is
 * parsed again. This is repeated until either the parse succeeds or the
 * leftmost field is one character in length.  If the parse still fails at that
 * point, the parse of the run fails.
 *
 * <P>
 * For time zones that have no names, SimpleDateFormat uses strings GMT+hours:minutes or
 * GMT-hours:minutes.
 * <P>
 * The calendar defines what is the first day of the week, the first week of the
 * year, whether hours are zero based or not (0 vs 12 or 24), and the timezone.
 * There is one common number format to handle all the numbers; the digit count
 * is handled programmatically according to the pattern.
 *
 * <p><em>User subclasses are not supported.</em> While clients may write
 * subclasses, such code will not necessarily work and will not be
 * guaranteed to work stably from release to release.
 */
class U_I18N_API SimpleDateFormat: public DateFormat {
public:
    /**
     * Construct a SimpleDateFormat using the default pattern for the default
     * locale.
     * <P>
     * [Note:] Not all locales support SimpleDateFormat; for full generality,
     * use the factory methods in the DateFormat class.
     * @param status    Output param set to success/failure code.
     * @stable ICU 2.0
     */
    SimpleDateFormat(UErrorCode& status);

    /**
     * Construct a SimpleDateFormat using the given pattern and the default locale.
     * The locale is used to obtain the symbols used in formatting (e.g., the
     * names of the months), but not to provide the pattern.
     * <P>
     * [Note:] Not all locales support SimpleDateFormat; for full generality,
     * use the factory methods in the DateFormat class.
     * @param pattern    the pattern for the format.
     * @param status     Output param set to success/failure code.
     * @stable ICU 2.0
     */
    SimpleDateFormat(const UnicodeString& pattern,
                     UErrorCode& status);

    /**
     * Construct a SimpleDateFormat using the given pattern, numbering system override, and the default locale.
     * The locale is used to obtain the symbols used in formatting (e.g., the
     * names of the months), but not to provide the pattern.
     * <P>
     * A numbering system override is a string containing either the name of a known numbering system,
     * or a set of field and numbering system pairs that specify which fields are to be formattied with
     * the alternate numbering system.  For example, to specify that all numeric fields in the specified
     * date or time pattern are to be rendered using Thai digits, simply specify the numbering system override
     * as "thai".  To specify that just the year portion of the date be formatted using Hebrew numbering,
     * use the override string "y=hebrew".  Numbering system overrides can be combined using a semi-colon
     * character in the override string, such as "d=decimal;M=arabic;y=hebrew", etc.
     *
     * <P>
     * [Note:] Not all locales support SimpleDateFormat; for full generality,
     * use the factory methods in the DateFormat class.
     * @param pattern    the pattern for the format.
     * @param override   the override string.
     * @param status     Output param set to success/failure code.
     * @stable ICU 4.2
     */
    SimpleDateFormat(const UnicodeString& pattern,
                     const UnicodeString& override,
                     UErrorCode& status);

    /**
     * Construct a SimpleDateFormat using the given pattern and locale.
     * The locale is used to obtain the symbols used in formatting (e.g., the
     * names of the months), but not to provide the pattern.
     * <P>
     * [Note:] Not all locales support SimpleDateFormat; for full generality,
     * use the factory methods in the DateFormat class.
     * @param pattern    the pattern for the format.
     * @param locale     the given locale.
     * @param status     Output param set to success/failure code.
     * @stable ICU 2.0
     */
    SimpleDateFormat(const UnicodeString& pattern,
                     const Locale& locale,
                     UErrorCode& status);

    /**
     * Construct a SimpleDateFormat using the given pattern, numbering system override, and locale.
     * The locale is used to obtain the symbols used in formatting (e.g., the
     * names of the months), but not to provide the pattern.
     * <P>
     * A numbering system override is a string containing either the name of a known numbering system,
     * or a set of field and numbering system pairs that specify which fields are to be formattied with
     * the alternate numbering system.  For example, to specify that all numeric fields in the specified
     * date or time pattern are to be rendered using Thai digits, simply specify the numbering system override
     * as "thai".  To specify that just the year portion of the date be formatted using Hebrew numbering,
     * use the override string "y=hebrew".  Numbering system overrides can be combined using a semi-colon
     * character in the override string, such as "d=decimal;M=arabic;y=hebrew", etc.
     * <P>
     * [Note:] Not all locales support SimpleDateFormat; for full generality,
     * use the factory methods in the DateFormat class.
     * @param pattern    the pattern for the format.
     * @param override   the numbering system override.
     * @param locale     the given locale.
     * @param status     Output param set to success/failure code.
     * @stable ICU 4.2
     */
    SimpleDateFormat(const UnicodeString& pattern,
                     const UnicodeString& override,
                     const Locale& locale,
                     UErrorCode& status);

    /**
     * Construct a SimpleDateFormat using the given pattern and locale-specific
     * symbol data.  The formatter takes ownership of the DateFormatSymbols object;
     * the caller is no longer responsible for deleting it.
     * @param pattern           the given pattern for the format.
     * @param formatDataToAdopt the symbols to be adopted.
     * @param status            Output param set to success/faulure code.
     * @stable ICU 2.0
     */
    SimpleDateFormat(const UnicodeString& pattern,
                     DateFormatSymbols* formatDataToAdopt,
                     UErrorCode& status);

    /**
     * Construct a SimpleDateFormat using the given pattern and locale-specific
     * symbol data.  The DateFormatSymbols object is NOT adopted; the caller
     * remains responsible for deleting it.
     * @param pattern           the given pattern for the format.
     * @param formatData        the formatting symbols to be use.
     * @param status            Output param set to success/faulure code.
     * @stable ICU 2.0
     */
    SimpleDateFormat(const UnicodeString& pattern,
                     const DateFormatSymbols& formatData,
                     UErrorCode& status);

    /**
     * Copy constructor.
     * @stable ICU 2.0
     */
    SimpleDateFormat(const SimpleDateFormat&);

    /**
     * Assignment operator.
     * @stable ICU 2.0
     */
    SimpleDateFormat& operator=(const SimpleDateFormat&);

    /**
     * Destructor.
     * @stable ICU 2.0
     */
    virtual ~SimpleDateFormat();

    /**
     * Clone this Format object polymorphically. The caller owns the result and
     * should delete it when done.
     * @return    A copy of the object.
     * @stable ICU 2.0
     */
    virtual Format* clone(void) const;

    /**
     * Return true if the given Format objects are semantically equal. Objects
     * of different subclasses are considered unequal.
     * @param other    the object to be compared with.
     * @return         true if the given Format objects are semantically equal.
     * @stable ICU 2.0
     */
    virtual UBool operator==(const Format& other) const;


    using DateFormat::format;

    /**
     * Format a date or time, which is the standard millis since 24:00 GMT, Jan
     * 1, 1970. Overrides DateFormat pure virtual method.
     * <P>
     * Example: using the US locale: "yyyy.MM.dd e 'at' HH:mm:ss zzz" ->>
     * 1996.07.10 AD at 15:08:56 PDT
     *
     * @param cal       Calendar set to the date and time to be formatted
     *                  into a date/time string.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param pos       The formatting position. On input: an alignment field,
     *                  if desired. On output: the offsets of the alignment field.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.1
     */
    virtual UnicodeString& format(  Calendar& cal,
                                    UnicodeString& appendTo,
                                    FieldPosition& pos) const;

    /**
     * Format a date or time, which is the standard millis since 24:00 GMT, Jan
     * 1, 1970. Overrides DateFormat pure virtual method.
     * <P>
     * Example: using the US locale: "yyyy.MM.dd e 'at' HH:mm:ss zzz" ->>
     * 1996.07.10 AD at 15:08:56 PDT
     *
     * @param cal       Calendar set to the date and time to be formatted
     *                  into a date/time string.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param posIter   On return, can be used to iterate over positions
     *                  of fields generated by this format call.  Field values
     *                  are defined in UDateFormatField.
     * @param status    Input/output param set to success/failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 4.4
     */
    virtual UnicodeString& format(  Calendar& cal,
                                    UnicodeString& appendTo,
                                    FieldPositionIterator* posIter,
                                    UErrorCode& status) const;

    /**
     * Format a date or time, which is the standard millis since 24:00 GMT, Jan
     * 1, 1970. Overrides DateFormat pure virtual method.
     * <P>
     * Example: using the US locale: "yyyy.MM.dd e 'at' HH:mm:ss zzz" ->>
     * 1996.07.10 AD at 15:08:56 PDT
     *
     * @param obj       A Formattable containing the date-time value to be formatted
     *                  into a date-time string.  If the type of the Formattable
     *                  is a numeric type, it is treated as if it were an
     *                  instance of Date.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param pos       The formatting position. On input: an alignment field,
     *                  if desired. On output: the offsets of the alignment field.
     * @param status    Input/output param set to success/failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    virtual UnicodeString& format(  const Formattable& obj,
                                    UnicodeString& appendTo,
                                    FieldPosition& pos,
                                    UErrorCode& status) const;

    /**
     * Format a date or time, which is the standard millis since 24:00 GMT, Jan
     * 1, 1970. Overrides DateFormat pure virtual method.
     * <P>
     * Example: using the US locale: "yyyy.MM.dd e 'at' HH:mm:ss zzz" ->>
     * 1996.07.10 AD at 15:08:56 PDT
     *
     * @param obj       A Formattable containing the date-time value to be formatted
     *                  into a date-time string.  If the type of the Formattable
     *                  is a numeric type, it is treated as if it were an
     *                  instance of Date.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param posIter   On return, can be used to iterate over positions
     *                  of fields generated by this format call.  Field values
     *                  are defined in UDateFormatField.
     * @param status    Input/output param set to success/failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 4.4
     */
    virtual UnicodeString& format(  const Formattable& obj,
                                    UnicodeString& appendTo,
                                    FieldPositionIterator* posIter,
                                    UErrorCode& status) const;

    /**
     * Redeclared DateFormat method.
     * @param date          the Date value to be formatted.
     * @param appendTo      Output parameter to receive result.
     *                      Result is appended to existing contents.
     * @param fieldPosition The formatting position. On input: an alignment field,
     *                      if desired. On output: the offsets of the alignment field.
     * @return              Reference to 'appendTo' parameter.
     * @stable ICU 2.1
     */
    UnicodeString& format(UDate date,
                          UnicodeString& appendTo,
                          FieldPosition& fieldPosition) const;

    /**
     * Redeclared DateFormat method.
     * @param date          the Date value to be formatted.
     * @param appendTo      Output parameter to receive result.
     *                      Result is appended to existing contents.
     * @param posIter       On return, can be used to iterate over positions
     *                      of fields generated by this format call.  Field values
     *                      are defined in UDateFormatField.
     * @param status        Input/output param set to success/failure code.
     * @return              Reference to 'appendTo' parameter.
     * @stable ICU 4.4
     */
    UnicodeString& format(UDate date,
                          UnicodeString& appendTo,
                          FieldPositionIterator* posIter,
                          UErrorCode& status) const;

    /**
     * Redeclared DateFormat method.
     * @param obj       Object to be formatted.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Input/output success/failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    /**
     * Redeclared DateFormat method.
     * @param date      Date value to be formatted.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    UnicodeString& format(UDate date, UnicodeString& appendTo) const;

    /**
     * Parse a date/time string beginning at the given parse position. For
     * example, a time text "07/10/96 4:5 PM, PDT" will be parsed into a Date
     * that is equivalent to Date(837039928046).
     * <P>
     * By default, parsing is lenient: If the input is not in the form used by
     * this object's format method but can still be parsed as a date, then the
     * parse succeeds. Clients may insist on strict adherence to the format by
     * calling setLenient(false).
     *
     * @param text  The date/time string to be parsed
     * @param cal   a Calendar set to the date and time to be formatted
     *              into a date/time string.
     * @param pos   On input, the position at which to start parsing; on
     *              output, the position at which parsing terminated, or the
     *              start position if the parse failed.
     * @return      A valid UDate if the input could be parsed.
     * @stable ICU 2.1
     */
    virtual void parse( const UnicodeString& text,
                        Calendar& cal,
                        ParsePosition& pos) const;

    /**
     * Parse a date/time string starting at the given parse position. For
     * example, a time text "07/10/96 4:5 PM, PDT" will be parsed into a Date
     * that is equivalent to Date(837039928046).
     * <P>
     * By default, parsing is lenient: If the input is not in the form used by
     * this object's format method but can still be parsed as a date, then the
     * parse succeeds. Clients may insist on strict adherence to the format by
     * calling setLenient(false).
     *
     * @see DateFormat::setLenient(boolean)
     *
     * @param text  The date/time string to be parsed
     * @param pos   On input, the position at which to start parsing; on
     *              output, the position at which parsing terminated, or the
     *              start position if the parse failed.
     * @return      A valid UDate if the input could be parsed.
     * @stable ICU 2.0
     */
    UDate parse( const UnicodeString& text,
                 ParsePosition& pos) const;


    /**
     * Parse a date/time string. For example, a time text "07/10/96 4:5 PM, PDT"
     * will be parsed into a UDate that is equivalent to Date(837039928046).
     * Parsing begins at the beginning of the string and proceeds as far as
     * possible.  Assuming no parse errors were encountered, this function
     * doesn't return any information about how much of the string was consumed
     * by the parsing.  If you need that information, use the version of
     * parse() that takes a ParsePosition.
     *
     * @param text  The date/time string to be parsed
     * @param status Filled in with U_ZERO_ERROR if the parse was successful, and with
     *              an error value if there was a parse error.
     * @return      A valid UDate if the input could be parsed.
     * @stable ICU 2.0
     */
    virtual UDate parse( const UnicodeString& text,
                        UErrorCode& status) const;

    /**
     * Set the start UDate used to interpret two-digit year strings.
     * When dates are parsed having 2-digit year strings, they are placed within
     * a assumed range of 100 years starting on the two digit start date.  For
     * example, the string "24-Jan-17" may be in the year 1817, 1917, 2017, or
     * some other year.  SimpleDateFormat chooses a year so that the resultant
     * date is on or after the two digit start date and within 100 years of the
     * two digit start date.
     * <P>
     * By default, the two digit start date is set to 80 years before the current
     * time at which a SimpleDateFormat object is created.
     * @param d      start UDate used to interpret two-digit year strings.
     * @param status Filled in with U_ZERO_ERROR if the parse was successful, and with
     *               an error value if there was a parse error.
     * @stable ICU 2.0
     */
    virtual void set2DigitYearStart(UDate d, UErrorCode& status);

    /**
     * Get the start UDate used to interpret two-digit year strings.
     * When dates are parsed having 2-digit year strings, they are placed within
     * a assumed range of 100 years starting on the two digit start date.  For
     * example, the string "24-Jan-17" may be in the year 1817, 1917, 2017, or
     * some other year.  SimpleDateFormat chooses a year so that the resultant
     * date is on or after the two digit start date and within 100 years of the
     * two digit start date.
     * <P>
     * By default, the two digit start date is set to 80 years before the current
     * time at which a SimpleDateFormat object is created.
     * @param status Filled in with U_ZERO_ERROR if the parse was successful, and with
     *               an error value if there was a parse error.
     * @stable ICU 2.0
     */
    UDate get2DigitYearStart(UErrorCode& status) const;

    /**
     * Return a pattern string describing this date format.
     * @param result Output param to receive the pattern.
     * @return       A reference to 'result'.
     * @stable ICU 2.0
     */
    virtual UnicodeString& toPattern(UnicodeString& result) const;

    /**
     * Return a localized pattern string describing this date format.
     * In most cases, this will return the same thing as toPattern(),
     * but a locale can specify characters to use in pattern descriptions
     * in place of the ones described in this class's class documentation.
     * (Presumably, letters that would be more mnemonic in that locale's
     * language.)  This function would produce a pattern using those
     * letters.
     *
     * @param result    Receives the localized pattern.
     * @param status    Output param set to success/failure code on
     *                  exit. If the pattern is invalid, this will be
     *                  set to a failure result.
     * @return          A reference to 'result'.
     * @stable ICU 2.0
     */
    virtual UnicodeString& toLocalizedPattern(UnicodeString& result,
                                              UErrorCode& status) const;

    /**
     * Apply the given unlocalized pattern string to this date format.
     * (i.e., after this call, this formatter will format dates according to
     * the new pattern)
     *
     * @param pattern   The pattern to be applied.
     * @stable ICU 2.0
     */
    virtual void applyPattern(const UnicodeString& pattern);

    /**
     * Apply the given localized pattern string to this date format.
     * (see toLocalizedPattern() for more information on localized patterns.)
     *
     * @param pattern   The localized pattern to be applied.
     * @param status    Output param set to success/failure code on
     *                  exit. If the pattern is invalid, this will be
     *                  set to a failure result.
     * @stable ICU 2.0
     */
    virtual void applyLocalizedPattern(const UnicodeString& pattern,
                                       UErrorCode& status);

    /**
     * Gets the date/time formatting symbols (this is an object carrying
     * the various strings and other symbols used in formatting: e.g., month
     * names and abbreviations, time zone names, AM/PM strings, etc.)
     * @return a copy of the date-time formatting data associated
     * with this date-time formatter.
     * @stable ICU 2.0
     */
    virtual const DateFormatSymbols* getDateFormatSymbols(void) const;

    /**
     * Set the date/time formatting symbols.  The caller no longer owns the
     * DateFormatSymbols object and should not delete it after making this call.
     * @param newFormatSymbols the given date-time formatting symbols to copy.
     * @stable ICU 2.0
     */
    virtual void adoptDateFormatSymbols(DateFormatSymbols* newFormatSymbols);

    /**
     * Set the date/time formatting data.
     * @param newFormatSymbols the given date-time formatting symbols to copy.
     * @stable ICU 2.0
     */
    virtual void setDateFormatSymbols(const DateFormatSymbols& newFormatSymbols);

    /**
     * Return the class ID for this class. This is useful only for comparing to
     * a return value from getDynamicClassID(). For example:
     * <pre>
     * .   Base* polymorphic_pointer = createPolymorphicObject();
     * .   if (polymorphic_pointer->getDynamicClassID() ==
     * .       erived::getStaticClassID()) ...
     * </pre>
     * @return          The class ID for all objects of this class.
     * @stable ICU 2.0
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
     * @stable ICU 2.0
     */
    virtual UClassID getDynamicClassID(void) const;

    /**
     * Set the calendar to be used by this date format. Initially, the default
     * calendar for the specified or default locale is used.  The caller should
     * not delete the Calendar object after it is adopted by this call.
     * Adopting a new calendar will change to the default symbols.
     *
     * @param calendarToAdopt    Calendar object to be adopted.
     * @stable ICU 2.0
     */
    virtual void adoptCalendar(Calendar* calendarToAdopt);

    /**
     * This is for ICU internal use only. Please do not use.
     * Check whether the 'field' is smaller than all the fields covered in
     * pattern, return TRUE if it is. The sequence of calendar field,
     * from large to small is: ERA, YEAR, MONTH, DATE, AM_PM, HOUR, MINUTE,...
     * @param field    the calendar field need to check against
     * @return         TRUE if the 'field' is smaller than all the fields
     *                 covered in pattern. FALSE otherwise.
     * @internal ICU 4.0
     */
    UBool isFieldUnitIgnored(UCalendarDateFields field) const;


    /**
     * This is for ICU internal use only. Please do not use.
     * Check whether the 'field' is smaller than all the fields covered in
     * pattern, return TRUE if it is. The sequence of calendar field,
     * from large to small is: ERA, YEAR, MONTH, DATE, AM_PM, HOUR, MINUTE,...
     * @param pattern  the pattern to check against
     * @param field    the calendar field need to check against
     * @return         TRUE if the 'field' is smaller than all the fields
     *                 covered in pattern. FALSE otherwise.
     * @internal ICU 4.0
     */
    static UBool isFieldUnitIgnored(const UnicodeString& pattern,
                                    UCalendarDateFields field);



    /**
     * This is for ICU internal use only. Please do not use.
     * Get the locale of this simple date formatter.
     * It is used in DateIntervalFormat.
     *
     * @return   locale in this simple date formatter
     * @internal ICU 4.0
     */
    const Locale& getSmpFmtLocale(void) const;


private:
    friend class DateFormat;

    void initializeDefaultCentury(void);

    SimpleDateFormat(); // default constructor not implemented

    /**
     * Used by the DateFormat factory methods to construct a SimpleDateFormat.
     * @param timeStyle the time style.
     * @param dateStyle the date style.
     * @param locale    the given locale.
     * @param status    Output param set to success/failure code on
     *                  exit.
     */
    SimpleDateFormat(EStyle timeStyle, EStyle dateStyle, const Locale& locale, UErrorCode& status);

    /**
     * Construct a SimpleDateFormat for the given locale.  If no resource data
     * is available, create an object of last resort, using hard-coded strings.
     * This is an internal method, called by DateFormat.  It should never fail.
     * @param locale    the given locale.
     * @param status    Output param set to success/failure code on
     *                  exit.
     */
    SimpleDateFormat(const Locale& locale, UErrorCode& status); // Use default pattern

    /**
     * Hook called by format(... FieldPosition& ...) and format(...FieldPositionIterator&...)
     */
    UnicodeString& _format(Calendar& cal, UnicodeString& appendTo, FieldPositionHandler& handler,
           UErrorCode& status) const;

    /**
     * Called by format() to format a single field.
     *
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param ch        The format character we encountered in the pattern.
     * @param count     Number of characters in the current pattern symbol (e.g.,
     *                  "yyyy" in the pattern would result in a call to this function
     *                  with ch equal to 'y' and count equal to 4)
     * @param handler   Records information about field positions.
     * @param status    Receives a status code, which will be U_ZERO_ERROR if the operation
     *                  succeeds.
     */
    void subFormat(UnicodeString &appendTo,
                   UChar ch,
                   int32_t count,
                   FieldPositionHandler& handler,
                   Calendar& cal,
                   UErrorCode& status) const; // in case of illegal argument

    /**
     * Used by subFormat() to format a numeric value.
     * Appends to toAppendTo a string representation of "value"
     * having a number of digits between "minDigits" and
     * "maxDigits".  Uses the DateFormat's NumberFormat.
     *
     * @param appendTo  Output parameter to receive result.
     *                  Formatted number is appended to existing contents.
     * @param value     Value to format.
     * @param minDigits Minimum number of digits the result should have
     * @param maxDigits Maximum number of digits the result should have
     */
    void zeroPaddingNumber(NumberFormat *currentNumberFormat,
                           UnicodeString &appendTo,
                           int32_t value,
                           int32_t minDigits,
                           int32_t maxDigits) const;

    /**
     * Return true if the given format character, occuring count
     * times, represents a numeric field.
     */
    static UBool isNumeric(UChar formatChar, int32_t count);

    /**
     * initializes fCalendar from parameters.  Returns fCalendar as a convenience.
     * @param adoptZone  Zone to be adopted, or NULL for TimeZone::createDefault().
     * @param locale Locale of the calendar
     * @param status Error code
     * @return the newly constructed fCalendar
     */
    Calendar *initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status);

    /**
     * initializes fSymbols from parameters.
     * @param locale Locale of the symbols
     * @param calendar Alias to Calendar that will be used.
     * @param status Error code
     */
    void initializeSymbols(const Locale& locale, Calendar* calendar, UErrorCode& status);

    /**
     * Called by several of the constructors to load pattern data and formatting symbols
     * out of a resource bundle and initialize the locale based on it.
     * @param timeStyle     The time style, as passed to DateFormat::createDateInstance().
     * @param dateStyle     The date style, as passed to DateFormat::createTimeInstance().
     * @param locale        The locale to load the patterns from.
     * @param status        Filled in with an error code if loading the data from the
     *                      resources fails.
     */
    void construct(EStyle timeStyle, EStyle dateStyle, const Locale& locale, UErrorCode& status);

    /**
     * Called by construct() and the various constructors to set up the SimpleDateFormat's
     * Calendar and NumberFormat objects.
     * @param locale    The locale for which we want a Calendar and a NumberFormat.
     * @param statuc    Filled in with an error code if creating either subobject fails.
     */
    void initialize(const Locale& locale, UErrorCode& status);

    /**
     * Private code-size reduction function used by subParse.
     * @param text the time text being parsed.
     * @param start where to start parsing.
     * @param field the date field being parsed.
     * @param stringArray the string array to parsed.
     * @param stringArrayCount the size of the array.
     * @param cal a Calendar set to the date and time to be formatted
     *            into a date/time string.
     * @return the new start position if matching succeeded; a negative number
     * indicating matching failure, otherwise.
     */
    int32_t matchString(const UnicodeString& text, int32_t start, UCalendarDateFields field,
                        const UnicodeString* stringArray, int32_t stringArrayCount, Calendar& cal) const;

    /**
     * Private code-size reduction function used by subParse.
     * @param text the time text being parsed.
     * @param start where to start parsing.
     * @param field the date field being parsed.
     * @param stringArray the string array to parsed.
     * @param stringArrayCount the size of the array.
     * @param cal a Calendar set to the date and time to be formatted
     *            into a date/time string.
     * @return the new start position if matching succeeded; a negative number
     * indicating matching failure, otherwise.
     */
    int32_t matchQuarterString(const UnicodeString& text, int32_t start, UCalendarDateFields field,
                               const UnicodeString* stringArray, int32_t stringArrayCount, Calendar& cal) const;

    /**
     * Private member function that converts the parsed date strings into
     * timeFields. Returns -start (for ParsePosition) if failed.
     * @param text the time text to be parsed.
     * @param start where to start parsing.
     * @param ch the pattern character for the date field text to be parsed.
     * @param count the count of a pattern character.
     * @param obeyCount if true then the count is strictly obeyed.
     * @param ambiguousYear If true then the two-digit year == the default start year.
     * @param saveHebrewMonth Used to hang onto month until year is known.
     * @param cal a Calendar set to the date and time to be formatted
     *            into a date/time string.
     * @return the new start position if matching succeeded; a negative number
     * indicating matching failure, otherwise.
     */
    int32_t subParse(const UnicodeString& text, int32_t& start, UChar ch, int32_t count,
                     UBool obeyCount, UBool allowNegative, UBool ambiguousYear[], int32_t& saveHebrewMonth, Calendar& cal,
                     int32_t patLoc) const;

    void parseInt(const UnicodeString& text,
                  Formattable& number,
                  ParsePosition& pos,
                  UBool allowNegative,
                  NumberFormat *fmt) const;

    void parseInt(const UnicodeString& text,
                  Formattable& number,
                  int32_t maxDigits,
                  ParsePosition& pos,
                  UBool allowNegative,
                  NumberFormat *fmt) const;

    int32_t checkIntSuffix(const UnicodeString& text, int32_t start,
                           int32_t patLoc, UBool isNegative) const;

    /**
     * Translate a pattern, mapping each character in the from string to the
     * corresponding character in the to string. Return an error if the original
     * pattern contains an unmapped character, or if a quote is unmatched.
     * Quoted (single quotes only) material is not translated.
     * @param originalPattern   the original pattern.
     * @param translatedPattern Output param to receive the translited pattern.
     * @param from              the characters to be translited from.
     * @param to                the characters to be translited to.
     * @param status            Receives a status code, which will be U_ZERO_ERROR
     *                          if the operation succeeds.
     */
    static void translatePattern(const UnicodeString& originalPattern,
                                UnicodeString& translatedPattern,
                                const UnicodeString& from,
                                const UnicodeString& to,
                                UErrorCode& status);

    /**
     * Sets the starting date of the 100-year window that dates with 2-digit years
     * are considered to fall within.
     * @param startDate the start date
     * @param status    Receives a status code, which will be U_ZERO_ERROR
     *                  if the operation succeeds.
     */
    void         parseAmbiguousDatesAsAfter(UDate startDate, UErrorCode& status);

    /**
     * Return the length matched by the given affix, or -1 if none.
     * Runs of white space in the affix, match runs of white space in
     * the input.
     * @param affix pattern string, taken as a literal
     * @param input input text
     * @param pos offset into input at which to begin matching
     * @return length of input that matches, or -1 if match failure
     */
    int32_t compareSimpleAffix(const UnicodeString& affix,
                   const UnicodeString& input,
                   int32_t pos) const;

    /**
     * Skip over a run of zero or more isRuleWhiteSpace() characters at
     * pos in text.
     */
    int32_t skipRuleWhiteSpace(const UnicodeString& text, int32_t pos) const;

    /**
     * Skip over a run of zero or more isUWhiteSpace() characters at pos
     * in text.
     */
    int32_t skipUWhiteSpace(const UnicodeString& text, int32_t pos) const;

    /**
     * Private methods for formatting/parsing GMT string
     */
    void appendGMT(NumberFormat *currentNumberFormat,UnicodeString &appendTo, Calendar& cal, UErrorCode& status) const;
    void formatGMTDefault(NumberFormat *currentNumberFormat,UnicodeString &appendTo, int32_t offset) const;
    int32_t parseGMT(const UnicodeString &text, ParsePosition &pos) const;
    int32_t parseGMTDefault(const UnicodeString &text, ParsePosition &pos) const;
    UBool isDefaultGMTFormat() const;

    void formatRFC822TZ(UnicodeString &appendTo, int32_t offset) const;

    /**
     * Initialize MessageFormat instances used for GMT formatting/parsing
     */
    void initGMTFormatters(UErrorCode &status);

    /**
     * Initialize NumberFormat instances used for numbering system overrides.
     */
    void initNumberFormatters(const Locale &locale,UErrorCode &status);

    /**
     * Get the numbering system to be used for a particular field.
     */
     NumberFormat * getNumberFormatByIndex(UDateFormatField index) const;

    /**
     * Parse the given override string and set up structures for number formats
     */
    void processOverrideString(const Locale &locale, const UnicodeString &str, int8_t type, UErrorCode &status);

    /**
     * Used to map pattern characters to Calendar field identifiers.
     */
    static const UCalendarDateFields fgPatternIndexToCalendarField[];

    /**
     * Map index into pattern character string to DateFormat field number
     */
    static const UDateFormatField fgPatternIndexToDateFormatField[];

    /**
     * Used to map Calendar field to field level.
     * The larger the level, the smaller the field unit.
     * For example, UCAL_ERA level is 0, UCAL_YEAR level is 10,
     * UCAL_MONTH level is 20.
     */
    static const int32_t fgCalendarFieldToLevel[];
    static const int32_t fgPatternCharToLevel[];

    /**
     * The formatting pattern for this formatter.
     */
    UnicodeString       fPattern;

    /**
     * The numbering system override for dates.
     */
    UnicodeString       fDateOverride;

    /**
     * The numbering system override for times.
     */
    UnicodeString       fTimeOverride;


    /**
     * The original locale used (for reloading symbols)
     */
    Locale              fLocale;

    /**
     * A pointer to an object containing the strings to use in formatting (e.g.,
     * month and day names, AM and PM strings, time zone names, etc.)
     */
    DateFormatSymbols*  fSymbols;   // Owned

    /**
     * If dates have ambiguous years, we map them into the century starting
     * at defaultCenturyStart, which may be any date.  If defaultCenturyStart is
     * set to SYSTEM_DEFAULT_CENTURY, which it is by default, then the system
     * values are used.  The instance values defaultCenturyStart and
     * defaultCenturyStartYear are only used if explicitly set by the user
     * through the API method parseAmbiguousDatesAsAfter().
     */
    UDate                fDefaultCenturyStart;

    /**
     * See documentation for defaultCenturyStart.
     */
    /*transient*/ int32_t   fDefaultCenturyStartYear;

    enum ParsedTZType {
        TZTYPE_UNK,
        TZTYPE_STD,
        TZTYPE_DST
    };

    ParsedTZType tztype; // here to avoid api change

    typedef struct NSOverride {
        NumberFormat *nf;
        int32_t hash;
        NSOverride *next;
    } NSOverride;

    /*
     * MessageFormat instances used for localized GMT format
     */
    enum {
        kGMTNegativeHMS = 0,
        kGMTNegativeHM,
        kGMTPositiveHMS,
        kGMTPositiveHM,

        kNumGMTFormatters
    };
    enum {
        kGMTNegativeHMSMinLenIdx = 0,
        kGMTPositiveHMSMinLenIdx,

        kNumGMTFormatMinLengths
    };

    MessageFormat   **fGMTFormatters;
    // If a GMT hour format has a second field, we need to make sure
    // the length of input localized GMT string must match the expected
    // length.  Otherwise, sub DateForamt handling offset format may
    // unexpectedly success parsing input GMT string without second field.
    // See #6880 about this issue.
    // TODO: SimpleDateFormat should provide an option to invalidate
    //
    int32_t         fGMTFormatHmsMinLen[kNumGMTFormatMinLengths];

    NumberFormat    **fNumberFormatters;

    NSOverride      *fOverrideList;

    UBool fHaveDefaultCentury;
};

inline UDate
SimpleDateFormat::get2DigitYearStart(UErrorCode& /*status*/) const
{
    return fDefaultCenturyStart;
}

inline UnicodeString&
SimpleDateFormat::format(const Formattable& obj,
                         UnicodeString& appendTo,
                         UErrorCode& status) const {
    // Don't use Format:: - use immediate base class only,
    // in case immediate base modifies behavior later.
    return DateFormat::format(obj, appendTo, status);
}

inline UnicodeString&
SimpleDateFormat::format(const Formattable& obj,
                         UnicodeString& appendTo,
                         FieldPosition& pos,
                         UErrorCode& status) const
{
    // Don't use Format:: - use immediate base class only,
    // in case immediate base modifies behavior later.
    return DateFormat::format(obj, appendTo, pos, status);
}

inline UnicodeString&
SimpleDateFormat::format(const Formattable& obj,
                         UnicodeString& appendTo,
                         FieldPositionIterator* posIter,
                         UErrorCode& status) const
{
    // Don't use Format:: - use immediate base class only,
    // in case immediate base modifies behavior later.
    return DateFormat::format(obj, appendTo, posIter, status);
}

inline UnicodeString&
SimpleDateFormat::format(UDate date,
                         UnicodeString& appendTo,
                         FieldPosition& fieldPosition) const {
    // Don't use Format:: - use immediate base class only,
    // in case immediate base modifies behavior later.
    return DateFormat::format(date, appendTo, fieldPosition);
}

inline UnicodeString&
SimpleDateFormat::format(UDate date,
                         UnicodeString& appendTo,
                         FieldPositionIterator* posIter,
                         UErrorCode& status) const {
    // Don't use Format:: - use immediate base class only,
    // in case immediate base modifies behavior later.
    return DateFormat::format(date, appendTo, posIter, status);
}

inline UnicodeString&
SimpleDateFormat::format(UDate date, UnicodeString& appendTo) const {
    return DateFormat::format(date, appendTo);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _SMPDTFMT
//eof
