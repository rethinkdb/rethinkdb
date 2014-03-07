/*
*******************************************************************************
* Copyright (C) 1997-2010, International Business Machines Corporation and others.
* All Rights Reserved.
* Modification History:
*
*   Date        Name        Description
*   06/24/99    helena      Integrated Alan's NF enhancements and Java2 bug fixes
*******************************************************************************
*/

#ifndef _UNUM
#define _UNUM

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"
#include "unicode/uloc.h"
#include "unicode/umisc.h"
#include "unicode/parseerr.h"
/**
 * \file
 * \brief C API: NumberFormat
 *
 * <h2> Number Format C API </h2>
 *
 * Number Format C API  Provides functions for
 * formatting and parsing a number.  Also provides methods for
 * determining which locales have number formats, and what their names
 * are.
 * <P>
 * UNumberFormat helps you to format and parse numbers for any locale.
 * Your code can be completely independent of the locale conventions
 * for decimal points, thousands-separators, or even the particular
 * decimal digits used, or whether the number format is even decimal.
 * There are different number format styles like decimal, currency,
 * percent and spellout.
 * <P>
 * To format a number for the current Locale, use one of the static
 * factory methods:
 * <pre>
 * \code
 *    UChar myString[20];
 *    double myNumber = 7.0;
 *    UErrorCode status = U_ZERO_ERROR;
 *    UNumberFormat* nf = unum_open(UNUM_DEFAULT, NULL, -1, NULL, NULL, &status);
 *    unum_formatDouble(nf, myNumber, myString, 20, NULL, &status);
 *    printf(" Example 1: %s\n", austrdup(myString) ); //austrdup( a function used to convert UChar* to char*)
 * \endcode
 * </pre>
 * If you are formatting multiple numbers, it is more efficient to get
 * the format and use it multiple times so that the system doesn't
 * have to fetch the information about the local language and country
 * conventions multiple times.
 * <pre>
 * \code
 * uint32_t i, resultlength, reslenneeded;
 * UErrorCode status = U_ZERO_ERROR;
 * UFieldPosition pos;
 * uint32_t a[] = { 123, 3333, -1234567 };
 * const uint32_t a_len = sizeof(a) / sizeof(a[0]);
 * UNumberFormat* nf;
 * UChar* result = NULL;
 *
 * nf = unum_open(UNUM_DEFAULT, NULL, -1, NULL, NULL, &status);
 * for (i = 0; i < a_len; i++) {
 *    resultlength=0;
 *    reslenneeded=unum_format(nf, a[i], NULL, resultlength, &pos, &status);
 *    result = NULL;
 *    if(status==U_BUFFER_OVERFLOW_ERROR){
 *       status=U_ZERO_ERROR;
 *       resultlength=reslenneeded+1;
 *       result=(UChar*)malloc(sizeof(UChar) * resultlength);
 *       unum_format(nf, a[i], result, resultlength, &pos, &status);
 *    }
 *    printf( " Example 2: %s\n", austrdup(result));
 *    free(result);
 * }
 * \endcode
 * </pre>
 * To format a number for a different Locale, specify it in the
 * call to unum_open().
 * <pre>
 * \code
 *     UNumberFormat* nf = unum_open(UNUM_DEFAULT, NULL, -1, "fr_FR", NULL, &success)
 * \endcode
 * </pre>
 * You can use a NumberFormat API unum_parse() to parse.
 * <pre>
 * \code
 *    UErrorCode status = U_ZERO_ERROR;
 *    int32_t pos=0;
 *    int32_t num;
 *    num = unum_parse(nf, str, u_strlen(str), &pos, &status);
 * \endcode
 * </pre>
 * Use UNUM_DECIMAL to get the normal number format for that country.
 * There are other static options available.  Use UNUM_CURRENCY
 * to get the currency number format for that country.  Use UNUM_PERCENT
 * to get a format for displaying percentages. With this format, a
 * fraction from 0.53 is displayed as 53%.
 * <P>
 * Use a pattern to create either a DecimalFormat or a RuleBasedNumberFormat
 * formatter.  The pattern must conform to the syntax defined for those
 * formatters.
 * <P>
 * You can also control the display of numbers with such function as
 * unum_getAttribues() and unum_setAtributes(), which let you set the
 * miminum fraction digits, grouping, etc.
 * @see UNumberFormatAttributes for more details
 * <P>
 * You can also use forms of the parse and format methods with
 * ParsePosition and UFieldPosition to allow you to:
 * <ul type=round>
 *   <li>(a) progressively parse through pieces of a string.
 *   <li>(b) align the decimal point and other areas.
 * </ul>
 * <p>
 * It is also possible to change or set the symbols used for a particular
 * locale like the currency symbol, the grouping seperator , monetary seperator
 * etc by making use of functions unum_setSymbols() and unum_getSymbols().
 */

/** A number formatter.
 *  For usage in C programs.
 *  @stable ICU 2.0
 */
typedef void* UNumberFormat;

/** The possible number format styles. 
 *  @stable ICU 2.0
 */
typedef enum UNumberFormatStyle {
    /**
     * Decimal format defined by pattern 
     * @stable ICU 3.0
     */
    UNUM_PATTERN_DECIMAL=0,
    /** Decimal format */
    UNUM_DECIMAL=1,
    /** Currency format */
    UNUM_CURRENCY,
    /** Percent format */
    UNUM_PERCENT,
    /** Scientific format */
    UNUM_SCIENTIFIC,
    /** Spellout rule-based format */
    UNUM_SPELLOUT,
    /** 
     * Ordinal rule-based format 
     * @stable ICU 3.0
     */
    UNUM_ORDINAL,
    /** 
     * Duration rule-based format 
     * @stable ICU 3.0
     */
    UNUM_DURATION,
    /** 
     * Numbering system rule-based format 
     * @stable ICU 4.2
     */
    UNUM_NUMBERING_SYSTEM,
    /** 
     * Rule-based format defined by pattern 
     * @stable ICU 3.0
     */
    UNUM_PATTERN_RULEBASED,
    /** Default format */
    UNUM_DEFAULT = UNUM_DECIMAL,
    /** (Alias for UNUM_PATTERN_DECIMAL) */
    UNUM_IGNORE = UNUM_PATTERN_DECIMAL
} UNumberFormatStyle;

/** The possible number format rounding modes. 
 *  @stable ICU 2.0
 */
typedef enum UNumberFormatRoundingMode {
    UNUM_ROUND_CEILING,
    UNUM_ROUND_FLOOR,
    UNUM_ROUND_DOWN,
    UNUM_ROUND_UP,
    /**
     * Half-even rounding, misspelled name
     * @deprecated, ICU 3.8
     */
    UNUM_FOUND_HALFEVEN,
    UNUM_ROUND_HALFDOWN,
    UNUM_ROUND_HALFUP,
    /**
     * Half-even rounding
     * @stable, ICU 3.8
     */
    UNUM_ROUND_HALFEVEN = UNUM_FOUND_HALFEVEN
} UNumberFormatRoundingMode;

/** The possible number format pad positions. 
 *  @stable ICU 2.0
 */
typedef enum UNumberFormatPadPosition {
    UNUM_PAD_BEFORE_PREFIX,
    UNUM_PAD_AFTER_PREFIX,
    UNUM_PAD_BEFORE_SUFFIX,
    UNUM_PAD_AFTER_SUFFIX
} UNumberFormatPadPosition;

/**
 * Create and return a new UNumberFormat for formatting and parsing
 * numbers.  A UNumberFormat may be used to format numbers by calling
 * {@link #unum_format }, and to parse numbers by calling {@link #unum_parse }.
 * The caller must call {@link #unum_close } when done to release resources
 * used by this object.
 * @param style The type of number format to open: one of
 * UNUM_DECIMAL, UNUM_CURRENCY, UNUM_PERCENT, UNUM_SCIENTIFIC, UNUM_SPELLOUT,
 * UNUM_PATTERN_DECIMAL, UNUM_PATTERN_RULEBASED, or UNUM_DEFAULT.
 * If UNUM_PATTERN_DECIMAL or UNUM_PATTERN_RULEBASED is passed then the
 * number format is opened using the given pattern, which must conform
 * to the syntax described in DecimalFormat or RuleBasedNumberFormat,
 * respectively.
 * @param pattern A pattern specifying the format to use. 
 * This parameter is ignored unless the style is
 * UNUM_PATTERN_DECIMAL or UNUM_PATTERN_RULEBASED.
 * @param patternLength The number of characters in the pattern, or -1
 * if null-terminated. This parameter is ignored unless the style is
 * UNUM_PATTERN.
 * @param locale A locale identifier to use to determine formatting
 * and parsing conventions, or NULL to use the default locale.
 * @param parseErr A pointer to a UParseError struct to receive the
 * details of any parsing errors, or NULL if no parsing error details
 * are desired.
 * @param status A pointer to an input-output UErrorCode.
 * @return A pointer to a newly created UNumberFormat, or NULL if an
 * error occurred.
 * @see unum_close
 * @see DecimalFormat
 * @stable ICU 2.0
 */
U_STABLE UNumberFormat* U_EXPORT2 
unum_open(  UNumberFormatStyle    style,
            const    UChar*    pattern,
            int32_t            patternLength,
            const    char*     locale,
            UParseError*       parseErr,
            UErrorCode*        status);


/**
* Close a UNumberFormat.
* Once closed, a UNumberFormat may no longer be used.
* @param fmt The formatter to close.
* @stable ICU 2.0
*/
U_STABLE void U_EXPORT2 
unum_close(UNumberFormat* fmt);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN

/**
 * \class LocalUNumberFormatPointer
 * "Smart pointer" class, closes a UNumberFormat via unum_close().
 * For most methods see the LocalPointerBase base class.
 *
 * @see LocalPointerBase
 * @see LocalPointer
 * @stable ICU 4.4
 */
U_DEFINE_LOCAL_OPEN_POINTER(LocalUNumberFormatPointer, UNumberFormat, unum_close);

U_NAMESPACE_END

#endif

/**
 * Open a copy of a UNumberFormat.
 * This function performs a deep copy.
 * @param fmt The format to copy
 * @param status A pointer to an UErrorCode to receive any errors.
 * @return A pointer to a UNumberFormat identical to fmt.
 * @stable ICU 2.0
 */
U_STABLE UNumberFormat* U_EXPORT2 
unum_clone(const UNumberFormat *fmt,
       UErrorCode *status);

/**
* Format an integer using a UNumberFormat.
* The integer will be formatted according to the UNumberFormat's locale.
* @param fmt The formatter to use.
* @param number The number to format.
* @param result A pointer to a buffer to receive the formatted number.
* @param resultLength The maximum size of result.
* @param pos    A pointer to a UFieldPosition.  On input, position->field
* is read.  On output, position->beginIndex and position->endIndex indicate
* the beginning and ending indices of field number position->field, if such
* a field exists.  This parameter may be NULL, in which case no field
* @param status A pointer to an UErrorCode to receive any errors
* @return The total buffer size needed; if greater than resultLength, the output was truncated.
* @see unum_formatInt64
* @see unum_formatDouble
* @see unum_parse
* @see unum_parseInt64
* @see unum_parseDouble
* @see UFieldPosition
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_format(    const    UNumberFormat*    fmt,
        int32_t            number,
        UChar*            result,
        int32_t            resultLength,
        UFieldPosition    *pos,
        UErrorCode*        status);

/**
* Format an int64 using a UNumberFormat.
* The int64 will be formatted according to the UNumberFormat's locale.
* @param fmt The formatter to use.
* @param number The number to format.
* @param result A pointer to a buffer to receive the formatted number.
* @param resultLength The maximum size of result.
* @param pos    A pointer to a UFieldPosition.  On input, position->field
* is read.  On output, position->beginIndex and position->endIndex indicate
* the beginning and ending indices of field number position->field, if such
* a field exists.  This parameter may be NULL, in which case no field
* @param status A pointer to an UErrorCode to receive any errors
* @return The total buffer size needed; if greater than resultLength, the output was truncated.
* @see unum_format
* @see unum_formatDouble
* @see unum_parse
* @see unum_parseInt64
* @see unum_parseDouble
* @see UFieldPosition
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_formatInt64(const UNumberFormat *fmt,
        int64_t         number,
        UChar*          result,
        int32_t         resultLength,
        UFieldPosition *pos,
        UErrorCode*     status);

/**
* Format a double using a UNumberFormat.
* The double will be formatted according to the UNumberFormat's locale.
* @param fmt The formatter to use.
* @param number The number to format.
* @param result A pointer to a buffer to receive the formatted number.
* @param resultLength The maximum size of result.
* @param pos    A pointer to a UFieldPosition.  On input, position->field
* is read.  On output, position->beginIndex and position->endIndex indicate
* the beginning and ending indices of field number position->field, if such
* a field exists.  This parameter may be NULL, in which case no field
* @param status A pointer to an UErrorCode to receive any errors
* @return The total buffer size needed; if greater than resultLength, the output was truncated.
* @see unum_format
* @see unum_formatInt64
* @see unum_parse
* @see unum_parseInt64
* @see unum_parseDouble
* @see UFieldPosition
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_formatDouble(    const    UNumberFormat*  fmt,
            double          number,
            UChar*          result,
            int32_t         resultLength,
            UFieldPosition  *pos, /* 0 if ignore */
            UErrorCode*     status);

/**
* Format a decimal number using a UNumberFormat.
* The number will be formatted according to the UNumberFormat's locale.
* The syntax of the input number is a "numeric string"
* as defined in the Decimal Arithmetic Specification, available at
* http://speleotrove.com/decimal
* @param fmt The formatter to use.
* @param number The number to format.
* @param length The length of the input number, or -1 if the input is nul-terminated.
* @param result A pointer to a buffer to receive the formatted number.
* @param resultLength The maximum size of result.
* @param pos    A pointer to a UFieldPosition.  On input, position->field
*               is read.  On output, position->beginIndex and position->endIndex indicate
*               the beginning and ending indices of field number position->field, if such
*               a field exists.  This parameter may be NULL, in which case it is ignored.
* @param status A pointer to an UErrorCode to receive any errors
* @return The total buffer size needed; if greater than resultLength, the output was truncated.
* @see unum_format
* @see unum_formatInt64
* @see unum_parse
* @see unum_parseInt64
* @see unum_parseDouble
* @see UFieldPosition
* @stable ICU 4.4 
*/
U_STABLE int32_t U_EXPORT2 
unum_formatDecimal(    const    UNumberFormat*  fmt,
            const char *    number,
            int32_t         length,
            UChar*          result,
            int32_t         resultLength,
            UFieldPosition  *pos, /* 0 if ignore */
            UErrorCode*     status);

/**
 * Format a double currency amount using a UNumberFormat.
 * The double will be formatted according to the UNumberFormat's locale.
 * @param fmt the formatter to use
 * @param number the number to format
 * @param currency the 3-letter null-terminated ISO 4217 currency code
 * @param result a pointer to the buffer to receive the formatted number
 * @param resultLength the maximum number of UChars to write to result
 * @param pos a pointer to a UFieldPosition.  On input,
 * position->field is read.  On output, position->beginIndex and
 * position->endIndex indicate the beginning and ending indices of
 * field number position->field, if such a field exists.  This
 * parameter may be NULL, in which case it is ignored.
 * @param status a pointer to an input-output UErrorCode
 * @return the total buffer size needed; if greater than resultLength,
 * the output was truncated.
 * @see unum_formatDouble
 * @see unum_parseDoubleCurrency
 * @see UFieldPosition
 * @stable ICU 3.0
 */
U_STABLE int32_t U_EXPORT2 
unum_formatDoubleCurrency(const UNumberFormat* fmt,
                          double number,
                          UChar* currency,
                          UChar* result,
                          int32_t resultLength,
                          UFieldPosition* pos, /* ignored if 0 */
                          UErrorCode* status);

/**
* Parse a string into an integer using a UNumberFormat.
* The string will be parsed according to the UNumberFormat's locale.
* @param fmt The formatter to use.
* @param text The text to parse.
* @param textLength The length of text, or -1 if null-terminated.
* @param parsePos If not 0, on input a pointer to an integer specifying the offset at which
* to begin parsing.  If not 0, on output the offset at which parsing ended.
* @param status A pointer to an UErrorCode to receive any errors
* @return The value of the parsed integer
* @see unum_parseInt64
* @see unum_parseDouble
* @see unum_format
* @see unum_formatInt64
* @see unum_formatDouble
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_parse(    const   UNumberFormat*  fmt,
        const   UChar*          text,
        int32_t         textLength,
        int32_t         *parsePos /* 0 = start */,
        UErrorCode      *status);

/**
* Parse a string into an int64 using a UNumberFormat.
* The string will be parsed according to the UNumberFormat's locale.
* @param fmt The formatter to use.
* @param text The text to parse.
* @param textLength The length of text, or -1 if null-terminated.
* @param parsePos If not 0, on input a pointer to an integer specifying the offset at which
* to begin parsing.  If not 0, on output the offset at which parsing ended.
* @param status A pointer to an UErrorCode to receive any errors
* @return The value of the parsed integer
* @see unum_parse
* @see unum_parseDouble
* @see unum_format
* @see unum_formatInt64
* @see unum_formatDouble
* @stable ICU 2.8
*/
U_STABLE int64_t U_EXPORT2 
unum_parseInt64(const UNumberFormat*  fmt,
        const UChar*  text,
        int32_t       textLength,
        int32_t       *parsePos /* 0 = start */,
        UErrorCode    *status);

/**
* Parse a string into a double using a UNumberFormat.
* The string will be parsed according to the UNumberFormat's locale.
* @param fmt The formatter to use.
* @param text The text to parse.
* @param textLength The length of text, or -1 if null-terminated.
* @param parsePos If not 0, on input a pointer to an integer specifying the offset at which
* to begin parsing.  If not 0, on output the offset at which parsing ended.
* @param status A pointer to an UErrorCode to receive any errors
* @return The value of the parsed double
* @see unum_parse
* @see unum_parseInt64
* @see unum_format
* @see unum_formatInt64
* @see unum_formatDouble
* @stable ICU 2.0
*/
U_STABLE double U_EXPORT2 
unum_parseDouble(    const   UNumberFormat*  fmt,
            const   UChar*          text,
            int32_t         textLength,
            int32_t         *parsePos /* 0 = start */,
            UErrorCode      *status);


/**
* Parse a number from a string into an unformatted numeric string using a UNumberFormat.
* The input string will be parsed according to the UNumberFormat's locale.
* The syntax of the output is a "numeric string"
* as defined in the Decimal Arithmetic Specification, available at
* http://speleotrove.com/decimal
* @param fmt The formatter to use.
* @param text The text to parse.
* @param textLength The length of text, or -1 if null-terminated.
* @param parsePos If not 0, on input a pointer to an integer specifying the offset at which
*                 to begin parsing.  If not 0, on output the offset at which parsing ended.
* @param outBuf A (char *) buffer to receive the parsed number as a string.  The output string
*               will be nul-terminated if there is sufficient space.
* @param outBufLength The size of the output buffer.  May be zero, in which case
*               the outBuf pointer may be NULL, and the function will return the
*               size of the output string.
* @param status A pointer to an UErrorCode to receive any errors
* @return the length of the output string, not including any terminating nul.
* @see unum_parse
* @see unum_parseInt64
* @see unum_format
* @see unum_formatInt64
* @see unum_formatDouble
* @stable ICU 4.4
*/
U_STABLE int32_t U_EXPORT2 
unum_parseDecimal(const   UNumberFormat*  fmt,
                 const   UChar*          text,
                         int32_t         textLength,
                         int32_t         *parsePos /* 0 = start */,
                         char            *outBuf,
                         int32_t         outBufLength,
                         UErrorCode      *status);

/**
 * Parse a string into a double and a currency using a UNumberFormat.
 * The string will be parsed according to the UNumberFormat's locale.
 * @param fmt the formatter to use
 * @param text the text to parse
 * @param textLength the length of text, or -1 if null-terminated
 * @param parsePos a pointer to an offset index into text at which to
 * begin parsing. On output, *parsePos will point after the last
 * parsed character.  This parameter may be 0, in which case parsing
 * begins at offset 0.
 * @param currency a pointer to the buffer to receive the parsed null-
 * terminated currency.  This buffer must have a capacity of at least
 * 4 UChars.
 * @param status a pointer to an input-output UErrorCode
 * @return the parsed double
 * @see unum_parseDouble
 * @see unum_formatDoubleCurrency
 * @stable ICU 3.0
 */
U_STABLE double U_EXPORT2
unum_parseDoubleCurrency(const UNumberFormat* fmt,
                         const UChar* text,
                         int32_t textLength,
                         int32_t* parsePos, /* 0 = start */
                         UChar* currency,
                         UErrorCode* status);

/**
 * Set the pattern used by a UNumberFormat.  This can only be used
 * on a DecimalFormat, other formats return U_ILLEGAL_ARGUMENT_ERROR
 * in the status.
 * @param format The formatter to set.
 * @param localized TRUE if the pattern is localized, FALSE otherwise.
 * @param pattern The new pattern
 * @param patternLength The length of pattern, or -1 if null-terminated.
 * @param parseError A pointer to UParseError to recieve information
 * about errors occurred during parsing, or NULL if no parse error
 * information is desired.
 * @param status A pointer to an input-output UErrorCode.
 * @see unum_toPattern
 * @see DecimalFormat
 * @stable ICU 2.0
 */
U_STABLE void U_EXPORT2 
unum_applyPattern(          UNumberFormat  *format,
                            UBool          localized,
                    const   UChar          *pattern,
                            int32_t         patternLength,
                            UParseError    *parseError,
                            UErrorCode     *status
                                    );

/**
* Get a locale for which decimal formatting patterns are available.
* A UNumberFormat in a locale returned by this function will perform the correct
* formatting and parsing for the locale.  The results of this call are not
* valid for rule-based number formats.
* @param localeIndex The index of the desired locale.
* @return A locale for which number formatting patterns are available, or 0 if none.
* @see unum_countAvailable
* @stable ICU 2.0
*/
U_STABLE const char* U_EXPORT2 
unum_getAvailable(int32_t localeIndex);

/**
* Determine how many locales have decimal formatting patterns available.  The
* results of this call are not valid for rule-based number formats.
* This function is useful for determining the loop ending condition for
* calls to {@link #unum_getAvailable }.
* @return The number of locales for which decimal formatting patterns are available.
* @see unum_getAvailable
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_countAvailable(void);

/** The possible UNumberFormat numeric attributes @stable ICU 2.0 */
typedef enum UNumberFormatAttribute {
  /** Parse integers only */
  UNUM_PARSE_INT_ONLY,
  /** Use grouping separator */
  UNUM_GROUPING_USED,
  /** Always show decimal point */
  UNUM_DECIMAL_ALWAYS_SHOWN,
  /** Maximum integer digits */
  UNUM_MAX_INTEGER_DIGITS,
  /** Minimum integer digits */
  UNUM_MIN_INTEGER_DIGITS,
  /** Integer digits */
  UNUM_INTEGER_DIGITS,
  /** Maximum fraction digits */
  UNUM_MAX_FRACTION_DIGITS,
  /** Minimum fraction digits */
  UNUM_MIN_FRACTION_DIGITS,
  /** Fraction digits */
  UNUM_FRACTION_DIGITS,
  /** Multiplier */
  UNUM_MULTIPLIER,
  /** Grouping size */
  UNUM_GROUPING_SIZE,
  /** Rounding Mode */
  UNUM_ROUNDING_MODE,
  /** Rounding increment */
  UNUM_ROUNDING_INCREMENT,
  /** The width to which the output of <code>format()</code> is padded. */
  UNUM_FORMAT_WIDTH,
  /** The position at which padding will take place. */
  UNUM_PADDING_POSITION,
  /** Secondary grouping size */
  UNUM_SECONDARY_GROUPING_SIZE,
  /** Use significant digits
   * @stable ICU 3.0 */
  UNUM_SIGNIFICANT_DIGITS_USED,
  /** Minimum significant digits
   * @stable ICU 3.0 */
  UNUM_MIN_SIGNIFICANT_DIGITS,
  /** Maximum significant digits
   * @stable ICU 3.0 */
  UNUM_MAX_SIGNIFICANT_DIGITS,
  /** Lenient parse mode used by rule-based formats.
   * @stable ICU 3.0
   */
  UNUM_LENIENT_PARSE
} UNumberFormatAttribute;

/**
* Get a numeric attribute associated with a UNumberFormat.
* An example of a numeric attribute is the number of integer digits a formatter will produce.
* @param fmt The formatter to query.
* @param attr The attribute to query; one of UNUM_PARSE_INT_ONLY, UNUM_GROUPING_USED,
* UNUM_DECIMAL_ALWAYS_SHOWN, UNUM_MAX_INTEGER_DIGITS, UNUM_MIN_INTEGER_DIGITS, UNUM_INTEGER_DIGITS,
* UNUM_MAX_FRACTION_DIGITS, UNUM_MIN_FRACTION_DIGITS, UNUM_FRACTION_DIGITS, UNUM_MULTIPLIER,
* UNUM_GROUPING_SIZE, UNUM_ROUNDING_MODE, UNUM_FORMAT_WIDTH, UNUM_PADDING_POSITION, UNUM_SECONDARY_GROUPING_SIZE.
* @return The value of attr.
* @see unum_setAttribute
* @see unum_getDoubleAttribute
* @see unum_setDoubleAttribute
* @see unum_getTextAttribute
* @see unum_setTextAttribute
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_getAttribute(const UNumberFormat*          fmt,
          UNumberFormatAttribute  attr);

/**
* Set a numeric attribute associated with a UNumberFormat.
* An example of a numeric attribute is the number of integer digits a formatter will produce.  If the
* formatter does not understand the attribute, the call is ignored.  Rule-based formatters only understand
* the lenient-parse attribute.
* @param fmt The formatter to set.
* @param attr The attribute to set; one of UNUM_PARSE_INT_ONLY, UNUM_GROUPING_USED,
* UNUM_DECIMAL_ALWAYS_SHOWN, UNUM_MAX_INTEGER_DIGITS, UNUM_MIN_INTEGER_DIGITS, UNUM_INTEGER_DIGITS,
* UNUM_MAX_FRACTION_DIGITS, UNUM_MIN_FRACTION_DIGITS, UNUM_FRACTION_DIGITS, UNUM_MULTIPLIER,
* UNUM_GROUPING_SIZE, UNUM_ROUNDING_MODE, UNUM_FORMAT_WIDTH, UNUM_PADDING_POSITION, UNUM_SECONDARY_GROUPING_SIZE,
* or UNUM_LENIENT_PARSE.
* @param newValue The new value of attr.
* @see unum_getAttribute
* @see unum_getDoubleAttribute
* @see unum_setDoubleAttribute
* @see unum_getTextAttribute
* @see unum_setTextAttribute
* @stable ICU 2.0
*/
U_STABLE void U_EXPORT2 
unum_setAttribute(    UNumberFormat*          fmt,
            UNumberFormatAttribute  attr,
            int32_t                 newValue);


/**
* Get a numeric attribute associated with a UNumberFormat.
* An example of a numeric attribute is the number of integer digits a formatter will produce.
* If the formatter does not understand the attribute, -1 is returned.
* @param fmt The formatter to query.
* @param attr The attribute to query; e.g. UNUM_ROUNDING_INCREMENT.
* @return The value of attr.
* @see unum_getAttribute
* @see unum_setAttribute
* @see unum_setDoubleAttribute
* @see unum_getTextAttribute
* @see unum_setTextAttribute
* @stable ICU 2.0
*/
U_STABLE double U_EXPORT2 
unum_getDoubleAttribute(const UNumberFormat*          fmt,
          UNumberFormatAttribute  attr);

/**
* Set a numeric attribute associated with a UNumberFormat.
* An example of a numeric attribute is the number of integer digits a formatter will produce.
* If the formatter does not understand the attribute, this call is ignored.
* @param fmt The formatter to set.
* @param attr The attribute to set; e.g. UNUM_ROUNDING_INCREMENT.
* @param newValue The new value of attr.
* @see unum_getAttribute
* @see unum_setAttribute
* @see unum_getDoubleAttribute
* @see unum_getTextAttribute
* @see unum_setTextAttribute
* @stable ICU 2.0
*/
U_STABLE void U_EXPORT2 
unum_setDoubleAttribute(    UNumberFormat*          fmt,
            UNumberFormatAttribute  attr,
            double                 newValue);

/** The possible UNumberFormat text attributes @stable ICU 2.0*/
typedef enum UNumberFormatTextAttribute {
  /** Positive prefix */
  UNUM_POSITIVE_PREFIX,
  /** Positive suffix */
  UNUM_POSITIVE_SUFFIX,
  /** Negative prefix */
  UNUM_NEGATIVE_PREFIX,
  /** Negative suffix */
  UNUM_NEGATIVE_SUFFIX,
  /** The character used to pad to the format width. */
  UNUM_PADDING_CHARACTER,
  /** The ISO currency code */
  UNUM_CURRENCY_CODE,
  /**
   * The default rule set.  This is only available with rule-based formatters.
   * @stable ICU 3.0
   */
  UNUM_DEFAULT_RULESET,
  /**
   * The public rule sets.  This is only available with rule-based formatters.
   * This is a read-only attribute.  The public rulesets are returned as a
   * single string, with each ruleset name delimited by ';' (semicolon).
   * @stable ICU 3.0
   */
  UNUM_PUBLIC_RULESETS
} UNumberFormatTextAttribute;

/**
* Get a text attribute associated with a UNumberFormat.
* An example of a text attribute is the suffix for positive numbers.  If the formatter
* does not understand the attributre, U_UNSUPPORTED_ERROR is returned as the status.
* Rule-based formatters only understand UNUM_DEFAULT_RULESET and UNUM_PUBLIC_RULESETS.
* @param fmt The formatter to query.
* @param tag The attribute to query; one of UNUM_POSITIVE_PREFIX, UNUM_POSITIVE_SUFFIX,
* UNUM_NEGATIVE_PREFIX, UNUM_NEGATIVE_SUFFIX, UNUM_PADDING_CHARACTER, UNUM_CURRENCY_CODE,
* UNUM_DEFAULT_RULESET, or UNUM_PUBLIC_RULESETS.
* @param result A pointer to a buffer to receive the attribute.
* @param resultLength The maximum size of result.
* @param status A pointer to an UErrorCode to receive any errors
* @return The total buffer size needed; if greater than resultLength, the output was truncated.
* @see unum_setTextAttribute
* @see unum_getAttribute
* @see unum_setAttribute
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2 
unum_getTextAttribute(    const    UNumberFormat*                    fmt,
            UNumberFormatTextAttribute      tag,
            UChar*                            result,
            int32_t                            resultLength,
            UErrorCode*                        status);

/**
* Set a text attribute associated with a UNumberFormat.
* An example of a text attribute is the suffix for positive numbers.  Rule-based formatters
* only understand UNUM_DEFAULT_RULESET.
* @param fmt The formatter to set.
* @param tag The attribute to set; one of UNUM_POSITIVE_PREFIX, UNUM_POSITIVE_SUFFIX,
* UNUM_NEGATIVE_PREFIX, UNUM_NEGATIVE_SUFFIX, UNUM_PADDING_CHARACTER, UNUM_CURRENCY_CODE,
* or UNUM_DEFAULT_RULESET.
* @param newValue The new value of attr.
* @param newValueLength The length of newValue, or -1 if null-terminated.
* @param status A pointer to an UErrorCode to receive any errors
* @see unum_getTextAttribute
* @see unum_getAttribute
* @see unum_setAttribute
* @stable ICU 2.0
*/
U_STABLE void U_EXPORT2 
unum_setTextAttribute(    UNumberFormat*                    fmt,
            UNumberFormatTextAttribute      tag,
            const    UChar*                            newValue,
            int32_t                            newValueLength,
            UErrorCode                        *status);

/**
 * Extract the pattern from a UNumberFormat.  The pattern will follow
 * the DecimalFormat pattern syntax.
 * @param fmt The formatter to query.
 * @param isPatternLocalized TRUE if the pattern should be localized,
 * FALSE otherwise.  This is ignored if the formatter is a rule-based
 * formatter.
 * @param result A pointer to a buffer to receive the pattern.
 * @param resultLength The maximum size of result.
 * @param status A pointer to an input-output UErrorCode.
 * @return The total buffer size needed; if greater than resultLength,
 * the output was truncated.
 * @see unum_applyPattern
 * @see DecimalFormat
 * @stable ICU 2.0
 */
U_STABLE int32_t U_EXPORT2 
unum_toPattern(    const    UNumberFormat*          fmt,
        UBool                  isPatternLocalized,
        UChar*                  result,
        int32_t                 resultLength,
        UErrorCode*             status);


/**
 * Constants for specifying a number format symbol.
 * @stable ICU 2.0
 */
typedef enum UNumberFormatSymbol {
  /** The decimal separator */
  UNUM_DECIMAL_SEPARATOR_SYMBOL = 0,
  /** The grouping separator */
  UNUM_GROUPING_SEPARATOR_SYMBOL = 1,
  /** The pattern separator */
  UNUM_PATTERN_SEPARATOR_SYMBOL = 2,
  /** The percent sign */
  UNUM_PERCENT_SYMBOL = 3,
  /** Zero*/
  UNUM_ZERO_DIGIT_SYMBOL = 4,
  /** Character representing a digit in the pattern */
  UNUM_DIGIT_SYMBOL = 5,
  /** The minus sign */
  UNUM_MINUS_SIGN_SYMBOL = 6,
  /** The plus sign */
  UNUM_PLUS_SIGN_SYMBOL = 7,
  /** The currency symbol */
  UNUM_CURRENCY_SYMBOL = 8,
  /** The international currency symbol */
  UNUM_INTL_CURRENCY_SYMBOL = 9,
  /** The monetary separator */
  UNUM_MONETARY_SEPARATOR_SYMBOL = 10,
  /** The exponential symbol */
  UNUM_EXPONENTIAL_SYMBOL = 11,
  /** Per mill symbol */
  UNUM_PERMILL_SYMBOL = 12,
  /** Escape padding character */
  UNUM_PAD_ESCAPE_SYMBOL = 13,
  /** Infinity symbol */
  UNUM_INFINITY_SYMBOL = 14,
  /** Nan symbol */
  UNUM_NAN_SYMBOL = 15,
  /** Significant digit symbol
   * @stable ICU 3.0 */
  UNUM_SIGNIFICANT_DIGIT_SYMBOL = 16,
  /** The monetary grouping separator 
   * @stable ICU 3.6
   */
  UNUM_MONETARY_GROUPING_SEPARATOR_SYMBOL = 17,  
  /** One
   * @draft ICU 4.6
   */
  UNUM_ONE_DIGIT_SYMBOL = 18,
  /** Two
   * @draft ICU 4.6
   */
  UNUM_TWO_DIGIT_SYMBOL = 19,
  /** Three
   * @draft ICU 4.6
   */
  UNUM_THREE_DIGIT_SYMBOL = 20,
  /** Four
   * @draft ICU 4.6
   */
  UNUM_FOUR_DIGIT_SYMBOL = 21,
  /** Five
   * @draft ICU 4.6
   */
  UNUM_FIVE_DIGIT_SYMBOL = 22,
  /** Six
   * @draft ICU 4.6
   */
  UNUM_SIX_DIGIT_SYMBOL = 23,
  /** Seven
    * @draft ICU 4.6
   */
  UNUM_SEVEN_DIGIT_SYMBOL = 24,
  /** Eight
   * @draft ICU 4.6
   */
  UNUM_EIGHT_DIGIT_SYMBOL = 25,
  /** Nine
   * @draft ICU 4.6
   */
  UNUM_NINE_DIGIT_SYMBOL = 26,
  /** count symbol constants */
  UNUM_FORMAT_SYMBOL_COUNT = 27
} UNumberFormatSymbol;

/**
* Get a symbol associated with a UNumberFormat.
* A UNumberFormat uses symbols to represent the special locale-dependent
* characters in a number, for example the percent sign. This API is not
* supported for rule-based formatters.
* @param fmt The formatter to query.
* @param symbol The UNumberFormatSymbol constant for the symbol to get
* @param buffer The string buffer that will receive the symbol string;
*               if it is NULL, then only the length of the symbol is returned
* @param size The size of the string buffer
* @param status A pointer to an UErrorCode to receive any errors
* @return The length of the symbol; the buffer is not modified if
*         <code>length&gt;=size</code>
* @see unum_setSymbol
* @stable ICU 2.0
*/
U_STABLE int32_t U_EXPORT2
unum_getSymbol(const UNumberFormat *fmt,
               UNumberFormatSymbol symbol,
               UChar *buffer,
               int32_t size,
               UErrorCode *status);

/**
* Set a symbol associated with a UNumberFormat.
* A UNumberFormat uses symbols to represent the special locale-dependent
* characters in a number, for example the percent sign.  This API is not
* supported for rule-based formatters.
* @param fmt The formatter to set.
* @param symbol The UNumberFormatSymbol constant for the symbol to set
* @param value The string to set the symbol to
* @param length The length of the string, or -1 for a zero-terminated string
* @param status A pointer to an UErrorCode to receive any errors.
* @see unum_getSymbol
* @stable ICU 2.0
*/
U_STABLE void U_EXPORT2
unum_setSymbol(UNumberFormat *fmt,
               UNumberFormatSymbol symbol,
               const UChar *value,
               int32_t length,
               UErrorCode *status);


/**
 * Get the locale for this number format object.
 * You can choose between valid and actual locale.
 * @param fmt The formatter to get the locale from
 * @param type type of the locale we're looking for (valid or actual) 
 * @param status error code for the operation
 * @return the locale name
 * @stable ICU 2.8
 */
U_STABLE const char* U_EXPORT2
unum_getLocaleByType(const UNumberFormat *fmt,
                     ULocDataLocaleType type,
                     UErrorCode* status); 

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
