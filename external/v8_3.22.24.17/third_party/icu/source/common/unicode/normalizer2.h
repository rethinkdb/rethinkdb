/*
*******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  normalizer2.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009nov22
*   created by: Markus W. Scherer
*/

#ifndef __NORMALIZER2_H__
#define __NORMALIZER2_H__

/**
 * \file
 * \brief C++ API: New API for Unicode Normalization.
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/unorm2.h"

U_NAMESPACE_BEGIN

/**
 * Unicode normalization functionality for standard Unicode normalization or
 * for using custom mapping tables.
 * All instances of this class are unmodifiable/immutable.
 * Instances returned by getInstance() are singletons that must not be deleted by the caller.
 * The Normalizer2 class is not intended for public subclassing.
 *
 * The primary functions are to produce a normalized string and to detect whether
 * a string is already normalized.
 * The most commonly used normalization forms are those defined in
 * http://www.unicode.org/unicode/reports/tr15/
 * However, this API supports additional normalization forms for specialized purposes.
 * For example, NFKC_Casefold is provided via getInstance("nfkc_cf", COMPOSE)
 * and can be used in implementations of UTS #46.
 *
 * Not only are the standard compose and decompose modes supplied,
 * but additional modes are provided as documented in the Mode enum.
 *
 * Some of the functions in this class identify normalization boundaries.
 * At a normalization boundary, the portions of the string
 * before it and starting from it do not interact and can be handled independently.
 *
 * The spanQuickCheckYes() stops at a normalization boundary.
 * When the goal is a normalized string, then the text before the boundary
 * can be copied, and the remainder can be processed with normalizeSecondAndAppend().
 *
 * The hasBoundaryBefore(), hasBoundaryAfter() and isInert() functions test whether
 * a character is guaranteed to be at a normalization boundary,
 * regardless of context.
 * This is used for moving from one normalization boundary to the next
 * or preceding boundary, and for performing iterative normalization.
 *
 * Iterative normalization is useful when only a small portion of a
 * longer string needs to be processed.
 * For example, in ICU, iterative normalization is used by the NormalizationTransliterator
 * (to avoid replacing already-normalized text) and ucol_nextSortKeyPart()
 * (to process only the substring for which sort key bytes are computed).
 *
 * The set of normalization boundaries returned by these functions may not be
 * complete: There may be more boundaries that could be returned.
 * Different functions may return different boundaries.
 * @stable ICU 4.4
 */
class U_COMMON_API Normalizer2 : public UObject {
public:
    /**
     * Returns a Normalizer2 instance which uses the specified data file
     * (packageName/name similar to ucnv_openPackage() and ures_open()/ResourceBundle)
     * and which composes or decomposes text according to the specified mode.
     * Returns an unmodifiable singleton instance. Do not delete it.
     *
     * Use packageName=NULL for data files that are part of ICU's own data.
     * Use name="nfc" and UNORM2_COMPOSE/UNORM2_DECOMPOSE for Unicode standard NFC/NFD.
     * Use name="nfkc" and UNORM2_COMPOSE/UNORM2_DECOMPOSE for Unicode standard NFKC/NFKD.
     * Use name="nfkc_cf" and UNORM2_COMPOSE for Unicode standard NFKC_CF=NFKC_Casefold.
     *
     * @param packageName NULL for ICU built-in data, otherwise application data package name
     * @param name "nfc" or "nfkc" or "nfkc_cf" or name of custom data file
     * @param mode normalization mode (compose or decompose etc.)
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return the requested Normalizer2, if successful
     * @stable ICU 4.4
     */
    static const Normalizer2 *
    getInstance(const char *packageName,
                const char *name,
                UNormalization2Mode mode,
                UErrorCode &errorCode);

    /**
     * Returns the normalized form of the source string.
     * @param src source string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return normalized src
     * @stable ICU 4.4
     */
    UnicodeString
    normalize(const UnicodeString &src, UErrorCode &errorCode) const {
        UnicodeString result;
        normalize(src, result, errorCode);
        return result;
    }
    /**
     * Writes the normalized form of the source string to the destination string
     * (replacing its contents) and returns the destination string.
     * The source and destination strings must be different objects.
     * @param src source string
     * @param dest destination string; its contents is replaced with normalized src
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return dest
     * @stable ICU 4.4
     */
    virtual UnicodeString &
    normalize(const UnicodeString &src,
              UnicodeString &dest,
              UErrorCode &errorCode) const = 0;
    /**
     * Appends the normalized form of the second string to the first string
     * (merging them at the boundary) and returns the first string.
     * The result is normalized if the first string was normalized.
     * The first and second strings must be different objects.
     * @param first string, should be normalized
     * @param second string, will be normalized
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return first
     * @stable ICU 4.4
     */
    virtual UnicodeString &
    normalizeSecondAndAppend(UnicodeString &first,
                             const UnicodeString &second,
                             UErrorCode &errorCode) const = 0;
    /**
     * Appends the second string to the first string
     * (merging them at the boundary) and returns the first string.
     * The result is normalized if both the strings were normalized.
     * The first and second strings must be different objects.
     * @param first string, should be normalized
     * @param second string, should be normalized
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return first
     * @stable ICU 4.4
     */
    virtual UnicodeString &
    append(UnicodeString &first,
           const UnicodeString &second,
           UErrorCode &errorCode) const = 0;

    /**
     * Gets the decomposition mapping of c. Equivalent to normalize(UnicodeString(c))
     * on a UNORM2_DECOMPOSE Normalizer2 instance, but much faster.
     * This function is independent of the mode of the Normalizer2.
     * @param c code point
     * @param decomposition String object which will be set to c's
     *                      decomposition mapping, if there is one.
     * @return TRUE if c has a decomposition, otherwise FALSE
     * @draft ICU 4.6
     */
    virtual UBool
    getDecomposition(UChar32 c, UnicodeString &decomposition) const = 0;

    /**
     * Tests if the string is normalized.
     * Internally, in cases where the quickCheck() method would return "maybe"
     * (which is only possible for the two COMPOSE modes) this method
     * resolves to "yes" or "no" to provide a definitive result,
     * at the cost of doing more work in those cases.
     * @param s input string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return TRUE if s is normalized
     * @stable ICU 4.4
     */
    virtual UBool
    isNormalized(const UnicodeString &s, UErrorCode &errorCode) const = 0;

    /**
     * Tests if the string is normalized.
     * For the two COMPOSE modes, the result could be "maybe" in cases that
     * would take a little more work to resolve definitively.
     * Use spanQuickCheckYes() and normalizeSecondAndAppend() for a faster
     * combination of quick check + normalization, to avoid
     * re-checking the "yes" prefix.
     * @param s input string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return UNormalizationCheckResult
     * @stable ICU 4.4
     */
    virtual UNormalizationCheckResult
    quickCheck(const UnicodeString &s, UErrorCode &errorCode) const = 0;

    /**
     * Returns the end of the normalized substring of the input string.
     * In other words, with <code>end=spanQuickCheckYes(s, ec);</code>
     * the substring <code>UnicodeString(s, 0, end)</code>
     * will pass the quick check with a "yes" result.
     *
     * The returned end index is usually one or more characters before the
     * "no" or "maybe" character: The end index is at a normalization boundary.
     * (See the class documentation for more about normalization boundaries.)
     *
     * When the goal is a normalized string and most input strings are expected
     * to be normalized already, then call this method,
     * and if it returns a prefix shorter than the input string,
     * copy that prefix and use normalizeSecondAndAppend() for the remainder.
     * @param s input string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return "yes" span end index
     * @stable ICU 4.4
     */
    virtual int32_t
    spanQuickCheckYes(const UnicodeString &s, UErrorCode &errorCode) const = 0;

    /**
     * Tests if the character always has a normalization boundary before it,
     * regardless of context.
     * If true, then the character does not normalization-interact with
     * preceding characters.
     * In other words, a string containing this character can be normalized
     * by processing portions before this character and starting from this
     * character independently.
     * This is used for iterative normalization. See the class documentation for details.
     * @param c character to test
     * @return TRUE if c has a normalization boundary before it
     * @stable ICU 4.4
     */
    virtual UBool hasBoundaryBefore(UChar32 c) const = 0;

    /**
     * Tests if the character always has a normalization boundary after it,
     * regardless of context.
     * If true, then the character does not normalization-interact with
     * following characters.
     * In other words, a string containing this character can be normalized
     * by processing portions up to this character and after this
     * character independently.
     * This is used for iterative normalization. See the class documentation for details.
     * Note that this operation may be significantly slower than hasBoundaryBefore().
     * @param c character to test
     * @return TRUE if c has a normalization boundary after it
     * @stable ICU 4.4
     */
    virtual UBool hasBoundaryAfter(UChar32 c) const = 0;

    /**
     * Tests if the character is normalization-inert.
     * If true, then the character does not change, nor normalization-interact with
     * preceding or following characters.
     * In other words, a string containing this character can be normalized
     * by processing portions before this character and after this
     * character independently.
     * This is used for iterative normalization. See the class documentation for details.
     * Note that this operation may be significantly slower than hasBoundaryBefore().
     * @param c character to test
     * @return TRUE if c is normalization-inert
     * @stable ICU 4.4
     */
    virtual UBool isInert(UChar32 c) const = 0;

private:
    // No ICU "poor man's RTTI" for this class nor its subclasses.
    virtual UClassID getDynamicClassID() const;
};

/**
 * Normalization filtered by a UnicodeSet.
 * Normalizes portions of the text contained in the filter set and leaves
 * portions not contained in the filter set unchanged.
 * Filtering is done via UnicodeSet::span(..., USET_SPAN_SIMPLE).
 * Not-in-the-filter text is treated as "is normalized" and "quick check yes".
 * This class implements all of (and only) the Normalizer2 API.
 * An instance of this class is unmodifiable/immutable but is constructed and
 * must be destructed by the owner.
 * @stable ICU 4.4
 */
class U_COMMON_API FilteredNormalizer2 : public Normalizer2 {
public:
    /**
     * Constructs a filtered normalizer wrapping any Normalizer2 instance
     * and a filter set.
     * Both are aliased and must not be modified or deleted while this object
     * is used.
     * The filter set should be frozen; otherwise the performance will suffer greatly.
     * @param n2 wrapped Normalizer2 instance
     * @param filterSet UnicodeSet which determines the characters to be normalized
     * @stable ICU 4.4
     */
    FilteredNormalizer2(const Normalizer2 &n2, const UnicodeSet &filterSet) :
            norm2(n2), set(filterSet) {}

    /**
     * Writes the normalized form of the source string to the destination string
     * (replacing its contents) and returns the destination string.
     * The source and destination strings must be different objects.
     * @param src source string
     * @param dest destination string; its contents is replaced with normalized src
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return dest
     * @stable ICU 4.4
     */
    virtual UnicodeString &
    normalize(const UnicodeString &src,
              UnicodeString &dest,
              UErrorCode &errorCode) const;
    /**
     * Appends the normalized form of the second string to the first string
     * (merging them at the boundary) and returns the first string.
     * The result is normalized if the first string was normalized.
     * The first and second strings must be different objects.
     * @param first string, should be normalized
     * @param second string, will be normalized
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return first
     * @stable ICU 4.4
     */
    virtual UnicodeString &
    normalizeSecondAndAppend(UnicodeString &first,
                             const UnicodeString &second,
                             UErrorCode &errorCode) const;
    /**
     * Appends the second string to the first string
     * (merging them at the boundary) and returns the first string.
     * The result is normalized if both the strings were normalized.
     * The first and second strings must be different objects.
     * @param first string, should be normalized
     * @param second string, should be normalized
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return first
     * @stable ICU 4.4
     */
    virtual UnicodeString &
    append(UnicodeString &first,
           const UnicodeString &second,
           UErrorCode &errorCode) const;

    /**
     * Gets the decomposition mapping of c. Equivalent to normalize(UnicodeString(c))
     * on a UNORM2_DECOMPOSE Normalizer2 instance, but much faster.
     * This function is independent of the mode of the Normalizer2.
     * @param c code point
     * @param decomposition String object which will be set to c's
     *                      decomposition mapping, if there is one.
     * @return TRUE if c has a decomposition, otherwise FALSE
     * @draft ICU 4.6
     */
    virtual UBool
    getDecomposition(UChar32 c, UnicodeString &decomposition) const;

    /**
     * Tests if the string is normalized.
     * For details see the Normalizer2 base class documentation.
     * @param s input string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return TRUE if s is normalized
     * @stable ICU 4.4
     */
    virtual UBool
    isNormalized(const UnicodeString &s, UErrorCode &errorCode) const;
    /**
     * Tests if the string is normalized.
     * For details see the Normalizer2 base class documentation.
     * @param s input string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return UNormalizationCheckResult
     * @stable ICU 4.4
     */
    virtual UNormalizationCheckResult
    quickCheck(const UnicodeString &s, UErrorCode &errorCode) const;
    /**
     * Returns the end of the normalized substring of the input string.
     * For details see the Normalizer2 base class documentation.
     * @param s input string
     * @param errorCode Standard ICU error code. Its input value must
     *                  pass the U_SUCCESS() test, or else the function returns
     *                  immediately. Check for U_FAILURE() on output or use with
     *                  function chaining. (See User Guide for details.)
     * @return "yes" span end index
     * @stable ICU 4.4
     */
    virtual int32_t
    spanQuickCheckYes(const UnicodeString &s, UErrorCode &errorCode) const;

    /**
     * Tests if the character always has a normalization boundary before it,
     * regardless of context.
     * For details see the Normalizer2 base class documentation.
     * @param c character to test
     * @return TRUE if c has a normalization boundary before it
     * @stable ICU 4.4
     */
    virtual UBool hasBoundaryBefore(UChar32 c) const;

    /**
     * Tests if the character always has a normalization boundary after it,
     * regardless of context.
     * For details see the Normalizer2 base class documentation.
     * @param c character to test
     * @return TRUE if c has a normalization boundary after it
     * @stable ICU 4.4
     */
    virtual UBool hasBoundaryAfter(UChar32 c) const;

    /**
     * Tests if the character is normalization-inert.
     * For details see the Normalizer2 base class documentation.
     * @param c character to test
     * @return TRUE if c is normalization-inert
     * @stable ICU 4.4
     */
    virtual UBool isInert(UChar32 c) const;
private:
    UnicodeString &
    normalize(const UnicodeString &src,
              UnicodeString &dest,
              USetSpanCondition spanCondition,
              UErrorCode &errorCode) const;

    UnicodeString &
    normalizeSecondAndAppend(UnicodeString &first,
                             const UnicodeString &second,
                             UBool doNormalize,
                             UErrorCode &errorCode) const;

    const Normalizer2 &norm2;
    const UnicodeSet &set;
};

U_NAMESPACE_END

#endif  // !UCONFIG_NO_NORMALIZATION
#endif  // __NORMALIZER2_H__
