/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*

* File PLURFMT.H
*
* Modification History:*
*   Date        Name        Description
*
********************************************************************************
*/

#ifndef PLURFMT
#define PLURFMT

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: PluralFormat object
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/numfmt.h"
#include "unicode/plurrule.h"

U_NAMESPACE_BEGIN

class Hashtable;

/**
 * <p>
 * <code>PluralFormat</code> supports the creation of internationalized
 * messages with plural inflection. It is based on <i>plural
 * selection</i>, i.e. the caller specifies messages for each
 * plural case that can appear in the users language and the
 * <code>PluralFormat</code> selects the appropriate message based on
 * the number.
 * </p>
 * <h4>The Problem of Plural Forms in Internationalized Messages</h4>
 * <p>
 * Different languages have different ways to inflect
 * plurals. Creating internationalized messages that include plural
 * forms is only feasible when the framework is able to handle plural
 * forms of <i>all</i> languages correctly. <code>ChoiceFormat</code>
 * doesn't handle this well, because it attaches a number interval to
 * each message and selects the message whose interval contains a
 * given number. This can only handle a finite number of
 * intervals. But in some languages, like Polish, one plural case
 * applies to infinitely many intervals (e.g., paucal applies to
 * numbers ending with 2, 3, or 4 except those ending with 12, 13, or
 * 14). Thus <code>ChoiceFormat</code> is not adequate.
 * </p><p>
 * <code>PluralFormat</code> deals with this by breaking the problem
 * into two parts:
 * <ul>
 * <li>It uses <code>PluralRules</code> that can define more complex
 *     conditions for a plural case than just a single interval. These plural
 *     rules define both what plural cases exist in a language, and to
 *     which numbers these cases apply.
 * <li>It provides predefined plural rules for many locales. Thus, the programmer
 *     need not worry about the plural cases of a language. On the flip side,
 *     the localizer does not have to specify the plural cases; he can simply
 *     use the predefined keywords. The whole plural formatting of messages can
 *     be done using localized patterns from resource bundles. For predefined plural
 *     rules, see CLDR <i>Language Plural Rules</i> page at 
 *    http://unicode.org/repos/cldr-tmp/trunk/diff/supplemental/language_plural_rules.html
 * </ul>
 * </p>
 * <h4>Usage of <code>PluralFormat</code></h4>
 * <p>
 * This discussion assumes that you use <code>PluralFormat</code> with
 * a predefined set of plural rules. You can create one using one of
 * the constructors that takes a <code>locale</code> object. To
 * specify the message pattern, you can either pass it to the
 * constructor or set it explicitly using the
 * <code>applyPattern()</code> method. The <code>format()</code>
 * method takes a number object and selects the message of the
 * matching plural case. This message will be returned.
 * </p>
 * <h5>Patterns and Their Interpretation</h5>
 * <p>
 * The pattern text defines the message output for each plural case of the
 * used locale. The pattern is a sequence of
 * <code><i>caseKeyword</i>{<i>message</i>}</code> clauses, separated by white
 * space characters. Each clause assigns the message <code><i>message</i></code>
 * to the plural case identified by <code><i>caseKeyword</i></code>.
 * </p><p>
 * There are 6 predefined casekeyword in ICU - 'zero', 'one', 'two', 'few', 'many' and
 * 'other'. You always have to define a message text for the default plural case
 * "<code>other</code>" which is contained in every rule set. If the plural
 * rules of the <code>PluralFormat</code> object do not contain a plural case
 * identified by <code><i>caseKeyword</i></code>, U_DEFAULT_KEYWORD_MISSING
 * will be set to status.
 * If you do not specify a message text for a particular plural case, the
 * message text of the plural case "<code>other</code>" gets assigned to this
 * plural case. If you specify more than one message for the same plural case,
 * U_DUPLICATE_KEYWORD will be set to status.
 * <br>
 * Spaces between <code><i>caseKeyword</i></code> and
 * <code><i>message</i></code>  will be ignored; spaces within
 * <code><i>message</i></code> will be preserved.
 * </p><p>
 * The message text for a particular plural case may contain other message
 * format patterns. <code>PluralFormat</code> preserves these so that you
 * can use the strings produced by <code>PluralFormat</code> with other
 * formatters. If you are using <code>PluralFormat</code> inside a
 * <code>MessageFormat</code> pattern, <code>MessageFormat</code> will
 * automatically evaluate the resulting format pattern.<br>
 * Thus, curly braces (<code>{</code>, <code>}</code>) are <i>only</i> allowed
 * in message texts to define a nested format pattern.<br>
 * The pound sign (<code>#</code>) will be interpreted as the number placeholder
 * in the message text, if it is not contained in curly braces (to preserve
 * <code>NumberFormat</code> patterns). <code>PluralFormat</code> will
 * replace each of those pound signs by the number passed to the
 * <code>format()</code> method. It will be formatted using a
 * <code>NumberFormat</code> for the <code>PluralFormat</code>'s locale. If you
 * need special number formatting, you have to explicitly specify a
 * <code>NumberFormat</code> for the <code>PluralFormat</code> to use.
 * </p>
 * Example
 * <pre>
 * \code
 * UErrorCode status = U_ZERO_ERROR;
 * MessageFormat* msgFmt = new MessageFormat(UnicodeString("{0, plural,
 *   one{{0, number, C''est #,##0.0#  fichier}} other {Ce sont # fichiers}} dans la liste."),
 *   Locale("fr"), status);
 * if (U_FAILURE(status)) {
 *     return;
 * }
 * Formattable args1[] = {(int32_t)0};
 * Formattable args2[] = {(int32_t)3};
 * FieldPosition ignore(FieldPosition::DONT_CARE);
 * UnicodeString result;
 * msgFmt->format(args1, 1, result, ignore, status);
 * cout << result << endl;
 * result.remove();
 * msgFmt->format(args2, 1, result, ignore, status);
 * cout << result << endl;
 * \endcode
 * </pre>
 * Produces the output:<br>
 * <code>C'est 0,0 fichier dans la liste.</code><br>
 * <code>Ce sont 3 fichiers dans la liste.</code>
 * <p>
 * <strong>Note:</strong><br>
 *   Currently <code>PluralFormat</code>
 *   does not make use of quotes like <code>MessageFormat</code>.
 *   If you use plural format strings with <code>MessageFormat</code> and want
 *   to use a quote sign <code>'</code>, you have to write <code>''</code>.
 *   <code>MessageFormat</code> unquotes this pattern and  passes the unquoted
 *   pattern to <code>PluralFormat</code>. It's a bit trickier if you use
 *   nested formats that do quoting. In the example above, we wanted to insert
 *   <code>'</code> in the number format pattern. Since
 *   <code>NumberFormat</code> supports quotes, we had to insert
 *   <code>''</code>. But since <code>MessageFormat</code> unquotes the
 *   pattern before it gets passed to <code>PluralFormat</code>, we have to
 *   double these quotes, i.e. write <code>''''</code>.
 * </p>
 * <h4>Defining Custom Plural Rules</h4>
 * <p>If you need to use <code>PluralFormat</code> with custom rules, you can
 * create a <code>PluralRules</code> object and pass it to
 * <code>PluralFormat</code>'s constructor. If you also specify a locale in this
 * constructor, this locale will be used to format the number in the message
 * texts.
 * </p><p>
 * For more information about <code>PluralRules</code>, see
 * {@link PluralRules}.
 * </p>
 *
 * ported from Java
 * @stable ICU 4.0
 */

class U_I18N_API PluralFormat : public Format {
public:

    /**
     * Creates a new <code>PluralFormat</code> for the default locale.
     * This locale will be used to get the set of plural rules and for standard
     * number formatting.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given locale.
     * @param locale the <code>PluralFormat</code> will be configured with
     *               rules for this locale. This locale will also be used for
     *               standard number formatting.
     * @param status output param set to success/failure code on exit, which
     *               must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const Locale& locale, UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given set of rules.
     * The standard number formatting will be done using the default locale.
     * @param rules   defines the behavior of the <code>PluralFormat</code>
     *                object.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const PluralRules& rules, UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given set of rules.
     * The standard number formatting will be done using the given locale.
     * @param locale  the default number formatting will be done using this
     *                locale.
     * @param rules   defines the behavior of the <code>PluralFormat</code>
     *                object.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const Locale& locale, const PluralRules& rules, UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given pattern string.
     * The default locale will be used to get the set of plural rules and for
     * standard number formatting.
     * @param  pattern the pattern for this <code>PluralFormat</code>.
     *                 errors are returned to status if the pattern is invalid.
     * @param status   output param set to success/failure code on exit, which
     *                 must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const UnicodeString& pattern, UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given pattern string and
     * locale.
     * The locale will be used to get the set of plural rules and for
     * standard number formatting.
     * @param locale   the <code>PluralFormat</code> will be configured with
     *                 rules for this locale. This locale will also be used for
     *                 standard number formatting.
     * @param pattern  the pattern for this <code>PluralFormat</code>.
     *                 errors are returned to status if the pattern is invalid.
     * @param status   output param set to success/failure code on exit, which
     *                 must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const Locale& locale, const UnicodeString& pattern, UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given set of rules, a
     * pattern and a locale.
     * @param rules    defines the behavior of the <code>PluralFormat</code>
     *                 object.
     * @param pattern  the pattern for this <code>PluralFormat</code>.
     *                 errors are returned to status if the pattern is invalid.
     * @param status   output param set to success/failure code on exit, which
     *                 must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const PluralRules& rules,
                 const UnicodeString& pattern,
                 UErrorCode& status);

    /**
     * Creates a new <code>PluralFormat</code> for a given set of rules, a
     * pattern and a locale.
     * @param locale  the <code>PluralFormat</code> will be configured with
     *                rules for this locale. This locale will also be used for
     *                standard number formatting.
     * @param rules   defines the behavior of the <code>PluralFormat</code>
     *                object.
     * @param pattern the pattern for this <code>PluralFormat</code>.
     *                errors are returned to status if the pattern is invalid.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    PluralFormat(const Locale& locale,
                 const PluralRules& rules,
                 const UnicodeString& pattern,
                 UErrorCode& status);

    /**
      * copy constructor.
      * @stable ICU 4.0
      */
    PluralFormat(const PluralFormat& other);

    /**
     * Destructor.
     * @stable ICU 4.0
     */
    virtual ~PluralFormat();

    /**
     * Sets the pattern used by this plural format.
     * The method parses the pattern and creates a map of format strings
     * for the plural rules.
     * Patterns and their interpretation are specified in the class description.
     *
     * @param pattern the pattern for this plural format
     *                errors are returned to status if the pattern is invalid.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    void applyPattern(const UnicodeString& pattern, UErrorCode& status);


    using Format::format;

    /**
     * Formats a plural message for a given number.
     *
     * @param number  a number for which the plural message should be formatted
     *                for. If no pattern has been applied to this
     *                <code>PluralFormat</code> object yet, the formatted number
     *                will be returned.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @return        the string containing the formatted plural message.
     * @stable ICU 4.0
     */
    UnicodeString format(int32_t number, UErrorCode& status) const;

    /**
     * Formats a plural message for a given number.
     *
     * @param number  a number for which the plural message should be formatted
     *                for. If no pattern has been applied to this
     *                PluralFormat object yet, the formatted number
     *                will be returned.
     * @param status  output param set to success or failure code on exit, which
     *                must not indicate a failure before the function call.
     * @return        the string containing the formatted plural message.
     * @stable ICU 4.0
     */
    UnicodeString format(double number, UErrorCode& status) const;

    /**
     * Formats a plural message for a given number.
     *
     * @param number   a number for which the plural message should be formatted
     *                 for. If no pattern has been applied to this
     *                 <code>PluralFormat</code> object yet, the formatted number
     *                 will be returned.
     * @param appendTo output parameter to receive result.
     *                 result is appended to existing contents.
     * @param pos      On input: an alignment field, if desired.
     *                 On output: the offsets of the alignment field.
     * @param status   output param set to success/failure code on exit, which
     *                 must not indicate a failure before the function call.
     * @return         the string containing the formatted plural message.
     * @stable ICU 4.0
     */
    UnicodeString& format(int32_t number,
                          UnicodeString& appendTo,
                          FieldPosition& pos,
                          UErrorCode& status) const;

    /**
     * Formats a plural message for a given number.
     *
     * @param number   a number for which the plural message should be formatted
     *                 for. If no pattern has been applied to this
     *                 PluralFormat object yet, the formatted number
     *                 will be returned.
     * @param appendTo output parameter to receive result.
     *                 result is appended to existing contents.
     * @param pos      On input: an alignment field, if desired.
     *                 On output: the offsets of the alignment field.
     * @param status   output param set to success/failure code on exit, which
     *                 must not indicate a failure before the function call.
     * @return         the string containing the formatted plural message.
     * @stable ICU 4.0
     */
    UnicodeString& format(double number,
                          UnicodeString& appendTo,
                          FieldPosition& pos,
                          UErrorCode& status) const;

    /**
     * Sets the locale used by this <code>PluraFormat</code> object.
     * Note: Calling this method resets this <code>PluraFormat</code> object,
     *     i.e., a pattern that was applied previously will be removed,
     *     and the NumberFormat is set to the default number format for
     *     the locale.  The resulting format behaves the same as one
     *     constructed from {@link #PluralFormat(const Locale& locale, UErrorCode& status)}.
     * @param locale  the <code>locale</code> to use to configure the formatter.
     * @param status  output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @stable ICU 4.0
     */
    void setLocale(const Locale& locale, UErrorCode& status);

    /**
      * Sets the number format used by this formatter.  You only need to
      * call this if you want a different number format than the default
      * formatter for the locale.
      * @param format  the number format to use.
      * @param status  output param set to success/failure code on exit, which
      *                must not indicate a failure before the function call.
      * @stable ICU 4.0
      */
    void setNumberFormat(const NumberFormat* format, UErrorCode& status);

    /**
       * Assignment operator
       *
       * @param other    the PluralFormat object to copy from.
       * @stable ICU 4.0
       */
    PluralFormat& operator=(const PluralFormat& other);

    /**
      * Return true if another object is semantically equal to this one.
      *
      * @param other    the PluralFormat object to be compared with.
      * @return         true if other is semantically equal to this.
      * @stable ICU 4.0
      */
    virtual UBool operator==(const Format& other) const;

    /**
     * Return true if another object is semantically unequal to this one.
     *
     * @param other    the PluralFormat object to be compared with.
     * @return         true if other is semantically unequal to this.
     * @stable ICU 4.0
     */
    virtual UBool operator!=(const Format& other) const;

    /**
     * Clones this Format object polymorphically.  The caller owns the
     * result and should delete it when done.
     * @stable ICU 4.0
     */
    virtual Format* clone(void) const;

    /**
    * Redeclared Format method.
    *
    * @param obj       The object to be formatted into a string.
    * @param appendTo  output parameter to receive result.
    *                  Result is appended to existing contents.
    * @param pos       On input: an alignment field, if desired.
    *                  On output: the offsets of the alignment field.
    * @param status    output param filled with success/failure status.
    * @return          Reference to 'appendTo' parameter.
    * @stable ICU 4.0
    */
   UnicodeString& format(const Formattable& obj,
                         UnicodeString& appendTo,
                         FieldPosition& pos,
                         UErrorCode& status) const;

   /**
    * Returns the pattern from applyPattern() or constructor().
    *
    * @param  appendTo  output parameter to receive result.
     *                  Result is appended to existing contents.
    * @return the UnicodeString with inserted pattern.
    * @stable ICU 4.0
    */
   UnicodeString& toPattern(UnicodeString& appendTo);

   /**
    * This method is not yet supported by <code>PluralFormat</code>.
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
    * @param parse_pos The position to start parsing at. Upon return
    *                  this param is set to the position after the
    *                  last character successfully parsed. If the
    *                  source is not parsed successfully, this param
    *                  will remain unchanged.
    * @stable ICU 4.0
    */
   virtual void parseObject(const UnicodeString& source,
                            Formattable& result,
                            ParsePosition& parse_pos) const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 4.0
     *
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 4.0
     */
     virtual UClassID getDynamicClassID() const;

private:
    typedef enum fmtToken {
        none,
        tLetter,
        tNumber,
        tSpace,
        tNumberSign,
        tLeftBrace,
        tRightBrace
    }fmtToken;

    Locale  locale;
    PluralRules* pluralRules;
    UnicodeString pattern;
    Hashtable  *fParsedValuesHash;
    NumberFormat*  numberFormat;
    NumberFormat*  replacedNumberFormat;

    PluralFormat();   // default constructor not implemented
    void init(const PluralRules* rules, const Locale& curlocale, UErrorCode& status);
    UBool inRange(UChar ch, fmtToken& type);
    UBool checkSufficientDefinition();
    void parsingFailure();
    UnicodeString insertFormattedNumber(double number,
                                        UnicodeString& message,
                                        UnicodeString& appendTo,
                                        FieldPosition& pos) const;
    void copyHashtable(Hashtable *other, UErrorCode& status);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _PLURFMT
//eof
