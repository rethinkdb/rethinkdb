/*
*******************************************************************************
* Copyright (C) 2008-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
*
* File PLURRULE.H
*
* Modification History:*
*   Date        Name        Description
*
********************************************************************************
*/

#ifndef PLURRULE
#define PLURRULE

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: PluralRules object
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"

U_NAMESPACE_BEGIN

class Hashtable;
class RuleChain;
class RuleParser;

/**
 * Defines rules for mapping positive long values onto a small set of
 * keywords. Rules are constructed from a text description, consisting
 * of a series of keywords and conditions.  The {@link #select} method
 * examines each condition in order and returns the keyword for the
 * first condition that matches the number.  If none match,
 * default rule(other) is returned.
 *
 * Examples:<pre>
 *   "one: n is 1; few: n in 2..4"</pre>
 *  This defines two rules, for 'one' and 'few'.  The condition for
 *  'one' is "n is 1" which means that the number must be equal to
 *  1 for this condition to pass.  The condition for 'few' is
 *  "n in 2..4" which means that the number must be between 2 and
 *  4 inclusive for this condition to pass.  All other numbers
 *  are assigned the keyword "other" by the default rule.
 *  </p><pre>
 *    "zero: n is 0; one: n is 1; zero: n mod 100 in 1..19"</pre>
 *  This illustrates that the same keyword can be defined multiple times.
 *  Each rule is examined in order, and the first keyword whose condition
 *  passes is the one returned.  Also notes that a modulus is applied
 *  to n in the last rule.  Thus its condition holds for 119, 219, 319...
 *  </p><pre>
 *    "one: n is 1; few: n mod 10 in 2..4 and n mod 100 not in 12..14"</pre>
 *  This illustrates conjunction and negation.  The condition for 'few'
 *  has two parts, both of which must be met: "n mod 10 in 2..4" and
 *  "n mod 100 not in 12..14".  The first part applies a modulus to n
 *  before the test as in the previous example.  The second part applies
 *  a different modulus and also uses negation, thus it matches all
 *  numbers _not_ in 12, 13, 14, 112, 113, 114, 212, 213, 214...
 *  </p>
 *  <p>
 * Syntax:<pre>
 * \code
 * rules         = rule (';' rule)*
 * rule          = keyword ':' condition
 * keyword       = <identifier>
 * condition     = and_condition ('or' and_condition)*
 * and_condition = relation ('and' relation)*
 * relation      = is_relation | in_relation | within_relation | 'n' <EOL>
 * is_relation   = expr 'is' ('not')? value
 * in_relation   = expr ('not')? 'in' range
 * within_relation = expr ('not')? 'within' range
 * expr          = 'n' ('mod' value)?
 * value         = digit+
 * digit         = 0|1|2|3|4|5|6|7|8|9
 * range         = value'..'value
 * \endcode
 * </pre></p>
 * <p>
 *  The difference between 'in' and 'within' is that 'in' only includes
 *  integers in the specified range, while 'within' includes all values.</p>
 *  <p>
 *  Keywords
 *  could be defined by users or from ICU locale data. There are 6
 *  predefined values in ICU - 'zero', 'one', 'two', 'few', 'many' and
 *  'other'. Callers need to check the value of keyword returned by
 *  {@link #select} method.
 *  </p>
 *
 * Examples:<pre>
 * UnicodeString keyword = pl->select(number);
 * if (keyword== UnicodeString("one") {
 *     ...
 * }
 * else if ( ... )
 * </pre>
 * <strong>Note:</strong><br>
 *  <p>
 *   ICU defines plural rules for many locales based on CLDR <i>Language Plural Rules</i>.
 *   For these predefined rules, see CLDR page at 
 *    http://unicode.org/repos/cldr-tmp/trunk/diff/supplemental/language_plural_rules.html
 * </p>
 */
class U_I18N_API PluralRules : public UObject {
public:

    /**
     * Constructor.
     * @param status  Output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     *
     * @stable ICU 4.0
     */
    PluralRules(UErrorCode& status);

    /**
     * Copy constructor.
     * @stable ICU 4.0
     */
    PluralRules(const PluralRules& other);

    /**
     * Destructor.
     * @stable ICU 4.0
     */
    virtual ~PluralRules();

    /**
     * Clone
     * @stable ICU 4.0
     */
    PluralRules* clone() const;

    /**
      * Assignment operator.
      * @stable ICU 4.0
      */
    PluralRules& operator=(const PluralRules&);

    /**
     * Creates a PluralRules from a description if it is parsable, otherwise
     * returns null.
     *
     * @param description rule description
     * @param status      Output param set to success/failure code on exit, which
     *                    must not indicate a failure before the function call.
     * @return            new PluralRules pointer. NULL if there is an error.
     * @stable ICU 4.0
     */
    static PluralRules* U_EXPORT2 createRules(const UnicodeString& description,
                                              UErrorCode& status);

    /**
     * The default rules that accept any number.
     *
     * @param status  Output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @return        new PluralRules pointer. NULL if there is an error.
     * @stable ICU 4.0
     */
    static PluralRules* U_EXPORT2 createDefaultRules(UErrorCode& status);

    /**
     * Provides access to the predefined <code>PluralRules</code> for a given
     * locale.
     *
     * @param locale  The locale for which a <code>PluralRules</code> object is
     *                returned.
     * @param status  Output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @return        The predefined <code>PluralRules</code> object pointer for
     *                this locale. If there's no predefined rules for this locale,
     *                the rules for the closest parent in the locale hierarchy
     *                that has one will  be returned.  The final fallback always
     *                returns the default 'other' rules.
     * @stable ICU 4.0
     */
    static PluralRules* U_EXPORT2 forLocale(const Locale& locale, UErrorCode& status);
    
    /**
     * Given a number, returns the keyword of the first rule that applies to
     * the number.  This function can be used with isKeyword* functions to
     * determine the keyword for default plural rules.
     *
     * @param number  The number for which the rule has to be determined.
     * @return        The keyword of the selected rule.
     * @stable ICU 4.0
     */
    UnicodeString select(int32_t number) const;
    
    /**
     * Given a number, returns the keyword of the first rule that applies to
     * the number.  This function can be used with isKeyword* functions to
     * determine the keyword for default plural rules.
     *
     * @param number  The number for which the rule has to be determined.
     * @return        The keyword of the selected rule.
     * @stable ICU 4.0
     */
    UnicodeString select(double number) const;

    /**
     * Returns a list of all rule keywords used in this <code>PluralRules</code>
     * object.  The rule 'other' is always present by default.
     *
     * @param status Output param set to success/failure code on exit, which
     *               must not indicate a failure before the function call.
     * @return       StringEnumeration with the keywords.
     *               The caller must delete the object.
     * @stable ICU 4.0
     */
    StringEnumeration* getKeywords(UErrorCode& status) const;

    /**
     * Returns TRUE if the given keyword is defined in this
     * <code>PluralRules</code> object.
     *
     * @param keyword  the input keyword.
     * @return         TRUE if the input keyword is defined.
     *                 Otherwise, return FALSE.
     * @stable ICU 4.0
     */
    UBool isKeyword(const UnicodeString& keyword) const;


    /**
     * Returns keyword for default plural form.
     *
     * @return         keyword for default plural form.
     * @internal 4.0
     * @stable ICU 4.0
     */
    UnicodeString getKeywordOther() const;

    /**
     * Compares the equality of two PluralRules objects.
     *
     * @param other The other PluralRules object to be compared with.
     * @return      True if the given PluralRules is the same as this
     *              PluralRules; false otherwise.
     * @stable ICU 4.0
     */
    virtual UBool operator==(const PluralRules& other) const;

    /**
     * Compares the inequality of two PluralRules objects.
     *
     * @param other The PluralRules object to be compared with.
     * @return      True if the given PluralRules is not the same as this
     *              PluralRules; false otherwise.
     * @stable ICU 4.0
     */
    UBool operator!=(const PluralRules& other) const  {return !operator==(other);}


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
    Hashtable       *fLocaleStringsHash;
    UnicodeString   mLocaleName;
    RuleChain       *mRules;
    RuleParser      *mParser;

    PluralRules();   // default constructor not implemented
    int32_t getRepeatLimit() const;
    void parseDescription(UnicodeString& ruleData, RuleChain& rules, UErrorCode &status);
    void getNextLocale(const UnicodeString& localeData, int32_t* curIndex, UnicodeString& localeName);
    void addRules(RuleChain& rules);
    int32_t getNumberValue(const UnicodeString& token) const;
    UnicodeString getRuleFromResource(const Locale& locale, UErrorCode& status);

};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _PLURRULE
//eof
