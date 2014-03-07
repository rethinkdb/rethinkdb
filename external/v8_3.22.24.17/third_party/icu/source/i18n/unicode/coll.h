/*
******************************************************************************
*   Copyright (C) 1996-2010, International Business Machines                 *
*   Corporation and others.  All Rights Reserved.                            *
******************************************************************************
*/

/**
 * \file 
 * \brief C++ API: Collation Service.
 */
 
/**
* File coll.h
*
* Created by: Helena Shih
*
* Modification History:
*
*  Date        Name        Description
* 02/5/97      aliu        Modified createDefault to load collation data from
*                          binary files when possible.  Added related methods
*                          createCollationFromFile, chopLocale, createPathName.
* 02/11/97     aliu        Added members addToCache, findInCache, and fgCache.
* 02/12/97     aliu        Modified to create objects from RuleBasedCollator cache.
*                          Moved cache out of Collation class.
* 02/13/97     aliu        Moved several methods out of this class and into
*                          RuleBasedCollator, with modifications.  Modified
*                          createDefault() to call new RuleBasedCollator(Locale&)
*                          constructor.  General clean up and documentation.
* 02/20/97     helena      Added clone, operator==, operator!=, operator=, copy
*                          constructor and getDynamicClassID.
* 03/25/97     helena      Updated with platform independent data types.
* 05/06/97     helena      Added memory allocation error detection.
* 06/20/97     helena      Java class name change.
* 09/03/97     helena      Added createCollationKeyValues().
* 02/10/98     damiba      Added compare() with length as parameter.
* 04/23/99     stephen     Removed EDecompositionMode, merged with
*                          Normalizer::EMode.
* 11/02/99     helena      Collator performance enhancements.  Eliminates the
*                          UnicodeString construction and special case for NO_OP.
* 11/23/99     srl         More performance enhancements. Inlining of
*                          critical accessors.
* 05/15/00     helena      Added version information API.
* 01/29/01     synwee      Modified into a C++ wrapper which calls C apis
*                          (ucoll.h).
*/

#ifndef COLL_H
#define COLL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"
#include "unicode/normlzr.h"
#include "unicode/locid.h"
#include "unicode/uniset.h"
#include "unicode/umisc.h"
#include "unicode/uiter.h"
#include "unicode/stringpiece.h"

U_NAMESPACE_BEGIN

class StringEnumeration;

#if !UCONFIG_NO_SERVICE
/**
 * @stable ICU 2.6
 */
class CollatorFactory;
#endif

/**
* @stable ICU 2.0
*/
class CollationKey;

/**
* The <code>Collator</code> class performs locale-sensitive string
* comparison.<br>
* You use this class to build searching and sorting routines for natural
* language text.<br>
* <em>Important: </em>The ICU collation service has been reimplemented
* in order to achieve better performance and UCA compliance.
* For details, see the
* <a href="http://source.icu-project.org/repos/icu/icuhtml/trunk/design/collation/ICU_collation_design.htm">
* collation design document</a>.
* <p>
* <code>Collator</code> is an abstract base class. Subclasses implement
* specific collation strategies. One subclass,
* <code>RuleBasedCollator</code>, is currently provided and is applicable
* to a wide set of languages. Other subclasses may be created to handle more
* specialized needs.
* <p>
* Like other locale-sensitive classes, you can use the static factory method,
* <code>createInstance</code>, to obtain the appropriate
* <code>Collator</code> object for a given locale. You will only need to
* look at the subclasses of <code>Collator</code> if you need to
* understand the details of a particular collation strategy or if you need to
* modify that strategy.
* <p>
* The following example shows how to compare two strings using the
* <code>Collator</code> for the default locale.
* \htmlonly<blockquote>\endhtmlonly
* <pre>
* \code
* // Compare two strings in the default locale
* UErrorCode success = U_ZERO_ERROR;
* Collator* myCollator = Collator::createInstance(success);
* if (myCollator->compare("abc", "ABC") < 0)
*   cout << "abc is less than ABC" << endl;
* else
*   cout << "abc is greater than or equal to ABC" << endl;
* \endcode
* </pre>
* \htmlonly</blockquote>\endhtmlonly
* <p>
* You can set a <code>Collator</code>'s <em>strength</em> property to
* determine the level of difference considered significant in comparisons.
* Five strengths are provided: <code>PRIMARY</code>, <code>SECONDARY</code>,
* <code>TERTIARY</code>, <code>QUATERNARY</code> and <code>IDENTICAL</code>.
* The exact assignment of strengths to language features is locale dependant.
* For example, in Czech, "e" and "f" are considered primary differences,
* while "e" and "\u00EA" are secondary differences, "e" and "E" are tertiary
* differences and "e" and "e" are identical. The following shows how both case
* and accents could be ignored for US English.
* \htmlonly<blockquote>\endhtmlonly
* <pre>
* \code
* //Get the Collator for US English and set its strength to PRIMARY
* UErrorCode success = U_ZERO_ERROR;
* Collator* usCollator = Collator::createInstance(Locale::US, success);
* usCollator->setStrength(Collator::PRIMARY);
* if (usCollator->compare("abc", "ABC") == 0)
*     cout << "'abc' and 'ABC' strings are equivalent with strength PRIMARY" << endl;
* \endcode
* </pre>
* \htmlonly</blockquote>\endhtmlonly
* <p>
* For comparing strings exactly once, the <code>compare</code> method
* provides the best performance. When sorting a list of strings however, it
* is generally necessary to compare each string multiple times. In this case,
* sort keys provide better performance. The <code>getSortKey</code> methods
* convert a string to a series of bytes that can be compared bitwise against
* other sort keys using <code>strcmp()</code>. Sort keys are written as
* zero-terminated byte strings. They consist of several substrings, one for
* each collation strength level, that are delimited by 0x01 bytes.
* If the string code points are appended for UCOL_IDENTICAL, then they are
* processed for correct code point order comparison and may contain 0x01
* bytes but not zero bytes.
* </p>
* <p>
* An older set of APIs returns a <code>CollationKey</code> object that wraps
* the sort key bytes instead of returning the bytes themselves.
* Its use is deprecated, but it is still available for compatibility with
* Java.
* </p>
* <p>
* <strong>Note:</strong> <code>Collator</code>s with different Locale,
* and CollationStrength settings will return different sort
* orders for the same set of strings. Locales have specific collation rules,
* and the way in which secondary and tertiary differences are taken into
* account, for example, will result in a different sorting order for same
* strings.
* </p>
* @see         RuleBasedCollator
* @see         CollationKey
* @see         CollationElementIterator
* @see         Locale
* @see         Normalizer
* @version     2.0 11/15/01
*/

class U_I18N_API Collator : public UObject {
public:

    // Collator public enums -----------------------------------------------

    /**
     * Base letter represents a primary difference. Set comparison level to
     * PRIMARY to ignore secondary and tertiary differences.<br>
     * Use this to set the strength of a Collator object.<br>
     * Example of primary difference, "abc" &lt; "abd"
     *
     * Diacritical differences on the same base letter represent a secondary
     * difference. Set comparison level to SECONDARY to ignore tertiary
     * differences. Use this to set the strength of a Collator object.<br>
     * Example of secondary difference, "&auml;" >> "a".
     *
     * Uppercase and lowercase versions of the same character represents a
     * tertiary difference.  Set comparison level to TERTIARY to include all
     * comparison differences. Use this to set the strength of a Collator
     * object.<br>
     * Example of tertiary difference, "abc" &lt;&lt;&lt; "ABC".
     *
     * Two characters are considered "identical" when they have the same unicode
     * spellings.<br>
     * For example, "&auml;" == "&auml;".
     *
     * UCollationStrength is also used to determine the strength of sort keys
     * generated from Collator objects.
     * @stable ICU 2.0
     */
    enum ECollationStrength
    {
        PRIMARY    = 0,
        SECONDARY  = 1,
        TERTIARY   = 2,
        QUATERNARY = 3,
        IDENTICAL  = 15
    };

    /**
     * LESS is returned if source string is compared to be less than target
     * string in the compare() method.
     * EQUAL is returned if source string is compared to be equal to target
     * string in the compare() method.
     * GREATER is returned if source string is compared to be greater than
     * target string in the compare() method.
     * @see Collator#compare
     * @deprecated ICU 2.6. Use C enum UCollationResult defined in ucol.h
     */
    enum EComparisonResult
    {
        LESS = -1,
        EQUAL = 0,
        GREATER = 1
    };

    // Collator public destructor -----------------------------------------

    /**
     * Destructor
     * @stable ICU 2.0
     */
    virtual ~Collator();

    // Collator public methods --------------------------------------------

    /**
     * Returns true if "other" is the same as "this"
     * @param other Collator object to be compared
     * @return true if other is the same as this.
     * @stable ICU 2.0
     */
    virtual UBool operator==(const Collator& other) const;

    /**
     * Returns true if "other" is not the same as "this".
     * @param other Collator object to be compared
     * @return true if other is not the same as this.
     * @stable ICU 2.0
     */
    virtual UBool operator!=(const Collator& other) const;

    /**
     * Makes a shallow copy of the current object.
     * @return a copy of this object
     * @stable ICU 2.0
     */
    virtual Collator* clone(void) const = 0;

    /**
     * Creates the Collator object for the current default locale.
     * The default locale is determined by Locale::getDefault.
     * The UErrorCode& err parameter is used to return status information to the user.
     * To check whether the construction succeeded or not, you should check the
     * value of U_SUCCESS(err).  If you wish more detailed information, you can
     * check for informational error results which still indicate success.
     * U_USING_FALLBACK_ERROR indicates that a fall back locale was used. For
     * example, 'de_CH' was requested, but nothing was found there, so 'de' was
     * used. U_USING_DEFAULT_ERROR indicates that the default locale data was
     * used; neither the requested locale nor any of its fall back locales
     * could be found.
     * The caller owns the returned object and is responsible for deleting it.
     *
     * @param err    the error code status.
     * @return       the collation object of the default locale.(for example, en_US)
     * @see Locale#getDefault
     * @stable ICU 2.0
     */
    static Collator* U_EXPORT2 createInstance(UErrorCode&  err);

    /**
     * Gets the table-based collation object for the desired locale. The
     * resource of the desired locale will be loaded by ResourceLoader.
     * Locale::ENGLISH is the base collation table and all other languages are
     * built on top of it with additional language-specific modifications.
     * The UErrorCode& err parameter is used to return status information to the user.
     * To check whether the construction succeeded or not, you should check
     * the value of U_SUCCESS(err).  If you wish more detailed information, you
     * can check for informational error results which still indicate success.
     * U_USING_FALLBACK_ERROR indicates that a fall back locale was used.  For
     * example, 'de_CH' was requested, but nothing was found there, so 'de' was
     * used.  U_USING_DEFAULT_ERROR indicates that the default locale data was
     * used; neither the requested locale nor any of its fall back locales
     * could be found.
     * The caller owns the returned object and is responsible for deleting it.
     * @param loc    The locale ID for which to open a collator.
     * @param err    the error code status.
     * @return       the created table-based collation object based on the desired
     *               locale.
     * @see Locale
     * @see ResourceLoader
     * @stable ICU 2.2
     */
    static Collator* U_EXPORT2 createInstance(const Locale& loc, UErrorCode& err);

#ifdef U_USE_COLLATION_OBSOLETE_2_6
    /**
     * Create a Collator with a specific version.
     * This is the same as createInstance(loc, err) except that getVersion() of
     * the returned object is guaranteed to be the same as the version
     * parameter.
     * This is designed to be used to open the same collator for a given
     * locale even when ICU is updated.
     * The same locale and version guarantees the same sort keys and
     * comparison results.
     * <p>
     * Note: this API will be removed in a future release.  Use
     * <tt>createInstance(const Locale&, UErrorCode&) instead.</tt></p>
     *
     * @param loc The locale ID for which to open a collator.
     * @param version The requested collator version.
     * @param err A reference to a UErrorCode,
     *            must not indicate a failure before calling this function.
     * @return A pointer to a Collator, or 0 if an error occurred
     *         or a collator with the requested version is not available.
     *
     * @see getVersion
     * @obsolete ICU 2.6
     */
    static Collator *createInstance(const Locale &loc, UVersionInfo version, UErrorCode &err);
#endif

    /**
     * The comparison function compares the character data stored in two
     * different strings. Returns information about whether a string is less
     * than, greater than or equal to another string.
     * @param source the source string to be compared with.
     * @param target the string that is to be compared with the source string.
     * @return Returns a byte value. GREATER if source is greater
     * than target; EQUAL if source is equal to target; LESS if source is less
     * than target
     * @deprecated ICU 2.6 use the overload with UErrorCode &
     */
    virtual EComparisonResult compare(const UnicodeString& source,
                                      const UnicodeString& target) const;

    /**
     * The comparison function compares the character data stored in two
     * different strings. Returns information about whether a string is less
     * than, greater than or equal to another string.
     * @param source the source string to be compared with.
     * @param target the string that is to be compared with the source string.
     * @param status possible error code
     * @return Returns an enum value. UCOL_GREATER if source is greater
     * than target; UCOL_EQUAL if source is equal to target; UCOL_LESS if source is less
     * than target
     * @stable ICU 2.6
     */
    virtual UCollationResult compare(const UnicodeString& source,
                                      const UnicodeString& target,
                                      UErrorCode &status) const = 0;

    /**
     * Does the same thing as compare but limits the comparison to a specified
     * length
     * @param source the source string to be compared with.
     * @param target the string that is to be compared with the source string.
     * @param length the length the comparison is limited to
     * @return Returns a byte value. GREATER if source (up to the specified
     *         length) is greater than target; EQUAL if source (up to specified
     *         length) is equal to target; LESS if source (up to the specified
     *         length) is less  than target.
     * @deprecated ICU 2.6 use the overload with UErrorCode &
     */
    virtual EComparisonResult compare(const UnicodeString& source,
                                      const UnicodeString& target,
                                      int32_t length) const;

    /**
     * Does the same thing as compare but limits the comparison to a specified
     * length
     * @param source the source string to be compared with.
     * @param target the string that is to be compared with the source string.
     * @param length the length the comparison is limited to
     * @param status possible error code
     * @return Returns an enum value. UCOL_GREATER if source (up to the specified
     *         length) is greater than target; UCOL_EQUAL if source (up to specified
     *         length) is equal to target; UCOL_LESS if source (up to the specified
     *         length) is less  than target.
     * @stable ICU 2.6
     */
    virtual UCollationResult compare(const UnicodeString& source,
                                      const UnicodeString& target,
                                      int32_t length,
                                      UErrorCode &status) const = 0;

    /**
     * The comparison function compares the character data stored in two
     * different string arrays. Returns information about whether a string array
     * is less than, greater than or equal to another string array.
     * @param source the source string array to be compared with.
     * @param sourceLength the length of the source string array.  If this value
     *        is equal to -1, the string array is null-terminated.
     * @param target the string that is to be compared with the source string.
     * @param targetLength the length of the target string array.  If this value
     *        is equal to -1, the string array is null-terminated.
     * @return Returns a byte value. GREATER if source is greater than target;
     *         EQUAL if source is equal to target; LESS if source is less than
     *         target
     * @deprecated ICU 2.6 use the overload with UErrorCode &
     */
    virtual EComparisonResult compare(const UChar* source, int32_t sourceLength,
                                      const UChar* target, int32_t targetLength)
                                      const;

    /**
     * The comparison function compares the character data stored in two
     * different string arrays. Returns information about whether a string array
     * is less than, greater than or equal to another string array.
     * @param source the source string array to be compared with.
     * @param sourceLength the length of the source string array.  If this value
     *        is equal to -1, the string array is null-terminated.
     * @param target the string that is to be compared with the source string.
     * @param targetLength the length of the target string array.  If this value
     *        is equal to -1, the string array is null-terminated.
     * @param status possible error code
     * @return Returns an enum value. UCOL_GREATER if source is greater
     * than target; UCOL_EQUAL if source is equal to target; UCOL_LESS if source is less
     * than target
     * @stable ICU 2.6
     */
    virtual UCollationResult compare(const UChar* source, int32_t sourceLength,
                                      const UChar* target, int32_t targetLength,
                                      UErrorCode &status) const = 0;

    /**
     * Compares two strings using the Collator.
     * Returns whether the first one compares less than/equal to/greater than
     * the second one.
     * This version takes UCharIterator input.
     * @param sIter the first ("source") string iterator
     * @param tIter the second ("target") string iterator
     * @param status ICU status
     * @return UCOL_LESS, UCOL_EQUAL or UCOL_GREATER
     * @stable ICU 4.2
     */
    virtual UCollationResult compare(UCharIterator &sIter,
                                     UCharIterator &tIter,
                                     UErrorCode &status) const;

    /**
     * Compares two UTF-8 strings using the Collator.
     * Returns whether the first one compares less than/equal to/greater than
     * the second one.
     * This version takes UTF-8 input.
     * Note that a StringPiece can be implicitly constructed
     * from a std::string or a NUL-terminated const char * string.
     * @param source the first UTF-8 string
     * @param target the second UTF-8 string
     * @param status ICU status
     * @return UCOL_LESS, UCOL_EQUAL or UCOL_GREATER
     * @stable ICU 4.2
     */
    virtual UCollationResult compareUTF8(const StringPiece &source,
                                         const StringPiece &target,
                                         UErrorCode &status) const;

    /**
     * Transforms the string into a series of characters that can be compared
     * with CollationKey::compareTo. It is not possible to restore the original
     * string from the chars in the sort key.  The generated sort key handles
     * only a limited number of ignorable characters.
     * <p>Use CollationKey::equals or CollationKey::compare to compare the
     * generated sort keys.
     * If the source string is null, a null collation key will be returned.
     * @param source the source string to be transformed into a sort key.
     * @param key the collation key to be filled in
     * @param status the error code status.
     * @return the collation key of the string based on the collation rules.
     * @see CollationKey#compare
     * @deprecated ICU 2.8 Use getSortKey(...) instead
     */
    virtual CollationKey& getCollationKey(const UnicodeString&  source,
                                          CollationKey& key,
                                          UErrorCode& status) const = 0;

    /**
     * Transforms the string into a series of characters that can be compared
     * with CollationKey::compareTo. It is not possible to restore the original
     * string from the chars in the sort key.  The generated sort key handles
     * only a limited number of ignorable characters.
     * <p>Use CollationKey::equals or CollationKey::compare to compare the
     * generated sort keys.
     * <p>If the source string is null, a null collation key will be returned.
     * @param source the source string to be transformed into a sort key.
     * @param sourceLength length of the collation key
     * @param key the collation key to be filled in
     * @param status the error code status.
     * @return the collation key of the string based on the collation rules.
     * @see CollationKey#compare
     * @deprecated ICU 2.8 Use getSortKey(...) instead
     */
    virtual CollationKey& getCollationKey(const UChar*source,
                                          int32_t sourceLength,
                                          CollationKey& key,
                                          UErrorCode& status) const = 0;
    /**
     * Generates the hash code for the collation object
     * @stable ICU 2.0
     */
    virtual int32_t hashCode(void) const = 0;

    /**
     * Gets the locale of the Collator
     *
     * @param type can be either requested, valid or actual locale. For more
     *             information see the definition of ULocDataLocaleType in
     *             uloc.h
     * @param status the error code status.
     * @return locale where the collation data lives. If the collator
     *         was instantiated from rules, locale is empty.
     * @deprecated ICU 2.8 This API is under consideration for revision
     * in ICU 3.0.
     */
    virtual const Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const = 0;

    /**
     * Convenience method for comparing two strings based on the collation rules.
     * @param source the source string to be compared with.
     * @param target the target string to be compared with.
     * @return true if the first string is greater than the second one,
     *         according to the collation rules. false, otherwise.
     * @see Collator#compare
     * @stable ICU 2.0
     */
    UBool greater(const UnicodeString& source, const UnicodeString& target)
                  const;

    /**
     * Convenience method for comparing two strings based on the collation rules.
     * @param source the source string to be compared with.
     * @param target the target string to be compared with.
     * @return true if the first string is greater than or equal to the second
     *         one, according to the collation rules. false, otherwise.
     * @see Collator#compare
     * @stable ICU 2.0
     */
    UBool greaterOrEqual(const UnicodeString& source,
                         const UnicodeString& target) const;

    /**
     * Convenience method for comparing two strings based on the collation rules.
     * @param source the source string to be compared with.
     * @param target the target string to be compared with.
     * @return true if the strings are equal according to the collation rules.
     *         false, otherwise.
     * @see Collator#compare
     * @stable ICU 2.0
     */
    UBool equals(const UnicodeString& source, const UnicodeString& target) const;

    /**
     * Determines the minimum strength that will be use in comparison or
     * transformation.
     * <p>E.g. with strength == SECONDARY, the tertiary difference is ignored
     * <p>E.g. with strength == PRIMARY, the secondary and tertiary difference
     * are ignored.
     * @return the current comparison level.
     * @see Collator#setStrength
     * @deprecated ICU 2.6 Use getAttribute(UCOL_STRENGTH...) instead
     */
    virtual ECollationStrength getStrength(void) const = 0;

    /**
     * Sets the minimum strength to be used in comparison or transformation.
     * <p>Example of use:
     * <pre>
     *  \code
     *  UErrorCode status = U_ZERO_ERROR;
     *  Collator*myCollation = Collator::createInstance(Locale::US, status);
     *  if (U_FAILURE(status)) return;
     *  myCollation->setStrength(Collator::PRIMARY);
     *  // result will be "abc" == "ABC"
     *  // tertiary differences will be ignored
     *  Collator::ComparisonResult result = myCollation->compare("abc", "ABC");
     * \endcode
     * </pre>
     * @see Collator#getStrength
     * @param newStrength the new comparison level.
     * @deprecated ICU 2.6 Use setAttribute(UCOL_STRENGTH...) instead
     */
    virtual void setStrength(ECollationStrength newStrength) = 0;

    /**
     * Get the current reordering of scripts (if one has been set).
     * @param dest The array to fill with the script ordering.
     * @param destCapacity The length of dest. If it is 0, then dest may be NULL and the function will only return the length of the result without writing any of the result string (pre-flighting).
     * @param pErrorCode Must be a valid pointer to an error code value, which must not indicate a failure before the function call.
     * @return The length of the array of the script ordering.
     * @see ucol_getReorderCodes
     * @internal 
     */
    virtual int32_t getReorderCodes(int32_t *dest,
                                    int32_t destCapacity,
                                    UErrorCode& status) const;

    /**
     * Set the ordering of scripts for this collator.
     * @param reorderCodes An array of reorder codes in the new order.
     * @param reorderCodesLength The length of reorderCodes.
     * @see ucol_setReorderCodes
     * @internal 
     */
    virtual void setReorderCodes(const int32_t* reorderCodes,
                                int32_t reorderCodesLength,
                                UErrorCode& status) ;

    /**
     * Get name of the object for the desired Locale, in the desired langauge
     * @param objectLocale must be from getAvailableLocales
     * @param displayLocale specifies the desired locale for output
     * @param name the fill-in parameter of the return value
     * @return display-able name of the object for the object locale in the
     *         desired language
     * @stable ICU 2.0
     */
    static UnicodeString& U_EXPORT2 getDisplayName(const Locale& objectLocale,
                                         const Locale& displayLocale,
                                         UnicodeString& name);

    /**
    * Get name of the object for the desired Locale, in the langauge of the
    * default locale.
    * @param objectLocale must be from getAvailableLocales
    * @param name the fill-in parameter of the return value
    * @return name of the object for the desired locale in the default language
    * @stable ICU 2.0
    */
    static UnicodeString& U_EXPORT2 getDisplayName(const Locale& objectLocale,
                                         UnicodeString& name);

    /**
     * Get the set of Locales for which Collations are installed.
     *
     * <p>Note this does not include locales supported by registered collators.
     * If collators might have been registered, use the overload of getAvailableLocales
     * that returns a StringEnumeration.</p>
     *
     * @param count the output parameter of number of elements in the locale list
     * @return the list of available locales for which collations are installed
     * @stable ICU 2.0
     */
    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);

    /**
     * Return a StringEnumeration over the locales available at the time of the call,
     * including registered locales.  If a severe error occurs (such as out of memory
     * condition) this will return null. If there is no locale data, an empty enumeration
     * will be returned.
     * @return a StringEnumeration over the locales available at the time of the call
     * @stable ICU 2.6
     */
    static StringEnumeration* U_EXPORT2 getAvailableLocales(void);

    /**
     * Create a string enumerator of all possible keywords that are relevant to
     * collation. At this point, the only recognized keyword for this
     * service is "collation".
     * @param status input-output error code
     * @return a string enumeration over locale strings. The caller is
     * responsible for closing the result.
     * @stable ICU 3.0
     */
    static StringEnumeration* U_EXPORT2 getKeywords(UErrorCode& status);

    /**
     * Given a keyword, create a string enumeration of all values
     * for that keyword that are currently in use.
     * @param keyword a particular keyword as enumerated by
     * ucol_getKeywords. If any other keyword is passed in, status is set
     * to U_ILLEGAL_ARGUMENT_ERROR.
     * @param status input-output error code
     * @return a string enumeration over collation keyword values, or NULL
     * upon error. The caller is responsible for deleting the result.
     * @stable ICU 3.0
     */
    static StringEnumeration* U_EXPORT2 getKeywordValues(const char *keyword, UErrorCode& status);

    /**
     * Given a key and a locale, returns an array of string values in a preferred
     * order that would make a difference. These are all and only those values where
     * the open (creation) of the service with the locale formed from the input locale
     * plus input keyword and that value has different behavior than creation with the
     * input locale alone.
     * @param keyword        one of the keys supported by this service.  For now, only
     *                      "collation" is supported.
     * @param locale        the locale
     * @param commonlyUsed  if set to true it will return only commonly used values
     *                      with the given locale in preferred order.  Otherwise,
     *                      it will return all the available values for the locale.
     * @param status ICU status
     * @return a string enumeration over keyword values for the given key and the locale.
     * @stable ICU 4.2
     */
    static StringEnumeration* U_EXPORT2 getKeywordValuesForLocale(const char* keyword, const Locale& locale,
                                                                    UBool commonlyUsed, UErrorCode& status);

    /**
     * Return the functionally equivalent locale for the given
     * requested locale, with respect to given keyword, for the
     * collation service.  If two locales return the same result, then
     * collators instantiated for these locales will behave
     * equivalently.  The converse is not always true; two collators
     * may in fact be equivalent, but return different results, due to
     * internal details.  The return result has no other meaning than
     * that stated above, and implies nothing as to the relationship
     * between the two locales.  This is intended for use by
     * applications who wish to cache collators, or otherwise reuse
     * collators when possible.  The functional equivalent may change
     * over time.  For more information, please see the <a
     * href="http://icu-project.org/userguide/locale.html#services">
     * Locales and Services</a> section of the ICU User Guide.
     * @param keyword a particular keyword as enumerated by
     * ucol_getKeywords.
     * @param locale the requested locale
     * @param isAvailable reference to a fillin parameter that
     * indicates whether the requested locale was 'available' to the
     * collation service. A locale is defined as 'available' if it
     * physically exists within the collation locale data.
     * @param status reference to input-output error code
     * @return the functionally equivalent collation locale, or the root
     * locale upon error.
     * @stable ICU 3.0
     */
    static Locale U_EXPORT2 getFunctionalEquivalent(const char* keyword, const Locale& locale,
                                          UBool& isAvailable, UErrorCode& status);

#if !UCONFIG_NO_SERVICE
    /**
     * Register a new Collator.  The collator will be adopted.
     * @param toAdopt the Collator instance to be adopted
     * @param locale the locale with which the collator will be associated
     * @param status the in/out status code, no special meanings are assigned
     * @return a registry key that can be used to unregister this collator
     * @stable ICU 2.6
     */
    static URegistryKey U_EXPORT2 registerInstance(Collator* toAdopt, const Locale& locale, UErrorCode& status);

    /**
     * Register a new CollatorFactory.  The factory will be adopted.
     * @param toAdopt the CollatorFactory instance to be adopted
     * @param status the in/out status code, no special meanings are assigned
     * @return a registry key that can be used to unregister this collator
     * @stable ICU 2.6
     */
    static URegistryKey U_EXPORT2 registerFactory(CollatorFactory* toAdopt, UErrorCode& status);

    /**
     * Unregister a previously-registered Collator or CollatorFactory
     * using the key returned from the register call.  Key becomes
     * invalid after a successful call and should not be used again.
     * The object corresponding to the key will be deleted.
     * @param key the registry key returned by a previous call to registerInstance
     * @param status the in/out status code, no special meanings are assigned
     * @return TRUE if the collator for the key was successfully unregistered
     * @stable ICU 2.6
     */
    static UBool U_EXPORT2 unregister(URegistryKey key, UErrorCode& status);
#endif /* UCONFIG_NO_SERVICE */

    /**
     * Gets the version information for a Collator.
     * @param info the version # information, the result will be filled in
     * @stable ICU 2.0
     */
    virtual void getVersion(UVersionInfo info) const = 0;

    /**
     * Returns a unique class ID POLYMORPHICALLY. Pure virtual method.
     * This method is to implement a simple version of RTTI, since not all C++
     * compilers support genuine RTTI. Polymorphic operator==() and clone()
     * methods call this method.
     * @return The class ID for this object. All objects of a given class have
     *         the same class ID.  Objects of other classes have different class
     *         IDs.
     * @stable ICU 2.0
     */
    virtual UClassID getDynamicClassID(void) const = 0;

    /**
     * Universal attribute setter
     * @param attr attribute type
     * @param value attribute value
     * @param status to indicate whether the operation went on smoothly or
     *        there were errors
     * @stable ICU 2.2
     */
    virtual void setAttribute(UColAttribute attr, UColAttributeValue value,
                              UErrorCode &status) = 0;

    /**
     * Universal attribute getter
     * @param attr attribute type
     * @param status to indicate whether the operation went on smoothly or
     *        there were errors
     * @return attribute value
     * @stable ICU 2.2
     */
    virtual UColAttributeValue getAttribute(UColAttribute attr,
                                            UErrorCode &status) = 0;

    /**
     * Sets the variable top to a collation element value of a string supplied.
     * @param varTop one or more (if contraction) UChars to which the variable top should be set
     * @param len length of variable top string. If -1 it is considered to be zero terminated.
     * @param status error code. If error code is set, the return value is undefined. Errors set by this function are: <br>
     *    U_CE_NOT_FOUND_ERROR if more than one character was passed and there is no such a contraction<br>
     *    U_PRIMARY_TOO_LONG_ERROR if the primary for the variable top has more than two bytes
     * @return a 32 bit value containing the value of the variable top in upper 16 bits. Lower 16 bits are undefined
     * @stable ICU 2.0
     */
    virtual uint32_t setVariableTop(const UChar *varTop, int32_t len, UErrorCode &status) = 0;

    /**
     * Sets the variable top to a collation element value of a string supplied.
     * @param varTop an UnicodeString size 1 or more (if contraction) of UChars to which the variable top should be set
     * @param status error code. If error code is set, the return value is undefined. Errors set by this function are: <br>
     *    U_CE_NOT_FOUND_ERROR if more than one character was passed and there is no such a contraction<br>
     *    U_PRIMARY_TOO_LONG_ERROR if the primary for the variable top has more than two bytes
     * @return a 32 bit value containing the value of the variable top in upper 16 bits. Lower 16 bits are undefined
     * @stable ICU 2.0
     */
    virtual uint32_t setVariableTop(const UnicodeString varTop, UErrorCode &status) = 0;

    /**
     * Sets the variable top to a collation element value supplied. Variable top is set to the upper 16 bits.
     * Lower 16 bits are ignored.
     * @param varTop CE value, as returned by setVariableTop or ucol)getVariableTop
     * @param status error code (not changed by function)
     * @stable ICU 2.0
     */
    virtual void setVariableTop(const uint32_t varTop, UErrorCode &status) = 0;

    /**
     * Gets the variable top value of a Collator.
     * Lower 16 bits are undefined and should be ignored.
     * @param status error code (not changed by function). If error code is set, the return value is undefined.
     * @stable ICU 2.0
     */
    virtual uint32_t getVariableTop(UErrorCode &status) const = 0;

    /**
     * Get an UnicodeSet that contains all the characters and sequences
     * tailored in this collator.
     * @param status      error code of the operation
     * @return a pointer to a UnicodeSet object containing all the
     *         code points and sequences that may sort differently than
     *         in the UCA. The object must be disposed of by using delete
     * @stable ICU 2.4
     */
    virtual UnicodeSet *getTailoredSet(UErrorCode &status) const;


    /**
     * Thread safe cloning operation
     * @return pointer to the new clone, user should remove it.
     * @stable ICU 2.2
     */
    virtual Collator* safeClone(void) = 0;

    /**
     * Get the sort key as an array of bytes from an UnicodeString.
     * Sort key byte arrays are zero-terminated and can be compared using
     * strcmp().
     * @param source string to be processed.
     * @param result buffer to store result in. If NULL, number of bytes needed
     *        will be returned.
     * @param resultLength length of the result buffer. If if not enough the
     *        buffer will be filled to capacity.
     * @return Number of bytes needed for storing the sort key
     * @stable ICU 2.2
     */
    virtual int32_t getSortKey(const UnicodeString& source,
                              uint8_t* result,
                              int32_t resultLength) const = 0;

    /**
     * Get the sort key as an array of bytes from an UChar buffer.
     * Sort key byte arrays are zero-terminated and can be compared using
     * strcmp().
     * @param source string to be processed.
     * @param sourceLength length of string to be processed.
     *        If -1, the string is 0 terminated and length will be decided by the
     *        function.
     * @param result buffer to store result in. If NULL, number of bytes needed
     *        will be returned.
     * @param resultLength length of the result buffer. If if not enough the
     *        buffer will be filled to capacity.
     * @return Number of bytes needed for storing the sort key
     * @stable ICU 2.2
     */
    virtual int32_t getSortKey(const UChar*source, int32_t sourceLength,
                               uint8_t*result, int32_t resultLength) const = 0;

    /**
     * Produce a bound for a given sortkey and a number of levels.
     * Return value is always the number of bytes needed, regardless of
     * whether the result buffer was big enough or even valid.<br>
     * Resulting bounds can be used to produce a range of strings that are
     * between upper and lower bounds. For example, if bounds are produced
     * for a sortkey of string "smith", strings between upper and lower
     * bounds with one level would include "Smith", "SMITH", "sMiTh".<br>
     * There are two upper bounds that can be produced. If UCOL_BOUND_UPPER
     * is produced, strings matched would be as above. However, if bound
     * produced using UCOL_BOUND_UPPER_LONG is used, the above example will
     * also match "Smithsonian" and similar.<br>
     * For more on usage, see example in cintltst/capitst.c in procedure
     * TestBounds.
     * Sort keys may be compared using <TT>strcmp</TT>.
     * @param source The source sortkey.
     * @param sourceLength The length of source, or -1 if null-terminated.
     *                     (If an unmodified sortkey is passed, it is always null
     *                      terminated).
     * @param boundType Type of bound required. It can be UCOL_BOUND_LOWER, which
     *                  produces a lower inclusive bound, UCOL_BOUND_UPPER, that
     *                  produces upper bound that matches strings of the same length
     *                  or UCOL_BOUND_UPPER_LONG that matches strings that have the
     *                  same starting substring as the source string.
     * @param noOfLevels  Number of levels required in the resulting bound (for most
     *                    uses, the recommended value is 1). See users guide for
     *                    explanation on number of levels a sortkey can have.
     * @param result A pointer to a buffer to receive the resulting sortkey.
     * @param resultLength The maximum size of result.
     * @param status Used for returning error code if something went wrong. If the
     *               number of levels requested is higher than the number of levels
     *               in the source key, a warning (U_SORT_KEY_TOO_SHORT_WARNING) is
     *               issued.
     * @return The size needed to fully store the bound.
     * @see ucol_keyHashCode
     * @stable ICU 2.1
     */
    static int32_t U_EXPORT2 getBound(const uint8_t       *source,
            int32_t             sourceLength,
            UColBoundMode       boundType,
            uint32_t            noOfLevels,
            uint8_t             *result,
            int32_t             resultLength,
            UErrorCode          &status);


protected:

    // Collator protected constructors -------------------------------------

    /**
    * Default constructor.
    * Constructor is different from the old default Collator constructor.
    * The task for determing the default collation strength and normalization
    * mode is left to the child class.
    * @stable ICU 2.0
    */
    Collator();

    /**
    * Constructor.
    * Empty constructor, does not handle the arguments.
    * This constructor is done for backward compatibility with 1.7 and 1.8.
    * The task for handling the argument collation strength and normalization
    * mode is left to the child class.
    * @param collationStrength collation strength
    * @param decompositionMode
    * @deprecated ICU 2.4. Subclasses should use the default constructor
    * instead and handle the strength and normalization mode themselves.
    */
    Collator(UCollationStrength collationStrength,
             UNormalizationMode decompositionMode);

    /**
    * Copy constructor.
    * @param other Collator object to be copied from
    * @stable ICU 2.0
    */
    Collator(const Collator& other);

    // Collator protected methods -----------------------------------------


   /**
    * Used internally by registraton to define the requested and valid locales.
    * @param requestedLocale the requested locale
    * @param validLocale the valid locale
    * @param actualLocale the actual locale
    * @internal
    */
    virtual void setLocales(const Locale& requestedLocale, const Locale& validLocale, const Locale& actualLocale);

public:
#if !UCONFIG_NO_SERVICE
    /**
     * used only by ucol_open, not for public use
     * @internal
     */
    static UCollator* createUCollator(const char* loc, UErrorCode* status);
#endif
private:
    /**
     * Assignment operator. Private for now.
     * @internal
     */
    Collator& operator=(const Collator& other);

    friend class CFactory;
    friend class SimpleCFactory;
    friend class ICUCollatorFactory;
    friend class ICUCollatorService;
    static Collator* makeInstance(const Locale& desiredLocale,
                                  UErrorCode& status);

    // Collator private data members ---------------------------------------

    /*
    synwee : removed as attributes to be handled by child class
    UCollationStrength  strength;
    Normalizer::EMode  decmp;
    */
    /* This is useless information */
/*  static const UVersionInfo fVersion;*/
};

#if !UCONFIG_NO_SERVICE
/**
 * A factory, used with registerFactory, the creates multiple collators and provides
 * display names for them.  A factory supports some number of locales-- these are the
 * locales for which it can create collators.  The factory can be visible, in which
 * case the supported locales will be enumerated by getAvailableLocales, or invisible,
 * in which they are not.  Invisible locales are still supported, they are just not
 * listed by getAvailableLocales.
 * <p>
 * If standard locale display names are sufficient, Collator instances can
 * be registered using registerInstance instead.</p>
 * <p>
 * Note: if the collators are to be used from C APIs, they must be instances
 * of RuleBasedCollator.</p>
 *
 * @stable ICU 2.6
 */
class U_I18N_API CollatorFactory : public UObject {
public:

    /**
     * Destructor
     * @stable ICU 3.0
     */
    virtual ~CollatorFactory();

    /**
     * Return true if this factory is visible.  Default is true.
     * If not visible, the locales supported by this factory will not
     * be listed by getAvailableLocales.
     * @return true if the factory is visible.
     * @stable ICU 2.6
     */
    virtual UBool visible(void) const;

    /**
     * Return a collator for the provided locale.  If the locale
     * is not supported, return NULL.
     * @param loc the locale identifying the collator to be created.
     * @return a new collator if the locale is supported, otherwise NULL.
     * @stable ICU 2.6
     */
    virtual Collator* createCollator(const Locale& loc) = 0;

    /**
     * Return the name of the collator for the objectLocale, localized for the displayLocale.
     * If objectLocale is not supported, or the factory is not visible, set the result string
     * to bogus.
     * @param objectLocale the locale identifying the collator
     * @param displayLocale the locale for which the display name of the collator should be localized
     * @param result an output parameter for the display name, set to bogus if not supported.
     * @return the display name
     * @stable ICU 2.6
     */
    virtual  UnicodeString& getDisplayName(const Locale& objectLocale,
                                           const Locale& displayLocale,
                                           UnicodeString& result);

    /**
     * Return an array of all the locale names directly supported by this factory.
     * The number of names is returned in count.  This array is owned by the factory.
     * Its contents must never change.
     * @param count output parameter for the number of locales supported by the factory
     * @param status the in/out error code
     * @return a pointer to an array of count UnicodeStrings.
     * @stable ICU 2.6
     */
    virtual const UnicodeString * getSupportedIDs(int32_t &count, UErrorCode& status) = 0;
};
#endif /* UCONFIG_NO_SERVICE */

// Collator inline methods -----------------------------------------------

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
