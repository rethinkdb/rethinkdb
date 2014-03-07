/*
 ******************************************************************************
 *   Copyright (C) 1996-2010, International Business Machines                 *
 *   Corporation and others.  All Rights Reserved.                            *
 ******************************************************************************
 */

/**
 * \file 
 * \brief C++ API: Boyer-Moore StringSearch technology preview
 * \internal ICU 4.0.1 technology preview
 */
 
#ifndef B_M_SEARCH_H
#define B_M_SEARCH_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"

#include "unicode/colldata.h"

U_NAMESPACE_BEGIN

class BadCharacterTable;
class GoodSuffixTable;
class Target;

/**
 * BoyerMooreSearch
 *
 * This object holds the information needed to do a Collation sensitive Boyer-Moore search. It encapulates
 * the pattern, the "bad character" and "good suffix" tables, the Collator-based data needed to compute them,
 * and a reference to the text being searched.
 *
 * To do a search, you fist need to get a <code>CollData</code> object by calling <code>CollData::open</code>.
 * Then you construct a <code>BoyerMooreSearch</code> object from the <code>CollData</code> object, the pattern
 * string and the target string. Then you call the <code>search</code> method. Here's a code sample:
 *
 * <pre>
 * void boyerMooreExample(UCollator *collator, UnicodeString *pattern, UnicodeString *target)
 * {
 *     UErrorCode status = U_ZERO_ERROR;
 *     CollData *collData = CollData::open(collator, status);
 *
 *     if (U_FAILURE(status)) {
 *         // could not create a CollData object
 *         return;
 *     }
 *
 *     BoyerMooreSearch *search = new BoyerMooreSearch(collData, *patternString, target, status);
 *
 *     if (U_FAILURE(status)) {
 *         // could not create a BoyerMooreSearch object
 *         CollData::close(collData);
 *         return;
 *     }
 *
 *     int32_t offset = 0, start = -1, end = -1;
 *
 *     // Find all matches
 *     while (search->search(offset, start, end)) {
 *         // process the match between start and end
 *         ...
 *         // advance past the match
 *         offset = end; 
 *     }
 *
 *     // at this point, if offset == 0, there were no matches
 *     if (offset == 0) {
 *         // handle the case of no matches
 *     }
 *
 *     delete search;
 *     CollData::close(collData);
 *
 *     // CollData objects are cached, so the call to
 *     // CollData::close doesn't delete the object.
 *     // Call this if you don't need the object any more.
 *     CollData::flushCollDataCache();
 * }
 * </pre>
 *
 * NOTE: This is a technology preview. The final version of this API may not bear any resenblence to this API.
 *
 * Knows linitations:
 *   1) Backwards searching has not been implemented.
 *
 *   2) For Han and Hangul characters, this code ignores any Collation tailorings. In general,
 *      this isn't a problem, but in Korean locals, at strength 1, Hangul characters are tailored
 *      to be equal to Han characters with the same pronounciation. Because this code ignroes
 *      tailorings, searching for a Hangul character will not find a Han character and visa-versa.
 *
 *   3) In some cases, searching for a pattern that needs to be normalized and ends
 *      in a discontiguous contraction may fail. The only known cases of this are with
 *      the Tibetan script. For example searching for the pattern
 *      "\u0F7F\u0F80\u0F81\u0F82\u0F83\u0F84\u0F85" will fail. (This case is artificial. We've
 *      been unable to find a pratical, real-world example of this failure.)  
 *
 * @internal ICU 4.0.1 technology preview
 *
 * @see CollData
 */
class U_I18N_API BoyerMooreSearch : public UObject
{
public:
    /**
     * Construct a <code>BoyerMooreSearch</code> object.
     *
     * @param theData - A <code>CollData</code> object holding the Collator-sensitive data
     * @param patternString - the string for which to search
     * @param targetString - the string in which to search or <code>NULL</code> if youu will
     *                       set it later by calling <code>setTargetString</code>.
     * @param status - will be set if any errors occur. 
     *
     * Note: if on return, status is set to an error code,
     * the only safe thing to do with this object is to call
     * the destructor.
     *
     * @internal ICU 4.0.1 technology preview
     */
    BoyerMooreSearch(CollData *theData, const UnicodeString &patternString, const UnicodeString *targetString, UErrorCode &status);

    /**
     * The desstructor
     *
     * @internal ICU 4.0.1 technology preview
     */
    ~BoyerMooreSearch();

    /**
     * Test the pattern to see if it generates any CEs.
     *
     * @return <code>TRUE</code> if the pattern string did not generate any CEs
     *
     * @internal ICU 4.0.1 technology preview
     */
    UBool empty();

    /**
     * Search for the pattern string in the target string.
     *
     * @param offset - the offset in the target string at which to begin the search
     * @param start - will be set to the starting offset of the match, or -1 if there's no match
     * @param end - will be set to the ending offset of the match, or -1 if there's no match
     *
     * @return <code>TRUE</code> if the match succeeds, <code>FALSE</code> otherwise.
     *
     * @internal ICU 4.0.1 technology preview
     */
    UBool search(int32_t offset, int32_t &start, int32_t &end);

    /**
     * Set the target string for the match.
     *
     * @param targetString - the new target string
     * @param status - will be set if any errors occur. 
     *
     * @internal ICU 4.0.1 technology preview
     */
    void setTargetString(const UnicodeString *targetString, UErrorCode &status);

    // **** no longer need these? ****
    /**
     * Return the <code>CollData</code> object used for searching
     *
     * @return the <code>CollData</code> object used for searching
     *
     * @internal ICU 4.0.1 technology preview
     */
    CollData *getData();

    /**
     * Return the CEs generated by the pattern string.
     *
     * @return a <code>CEList</code> object holding the CEs generated by the pattern string.
     *
     * @internal ICU 4.0.1 technology preview
     */
    CEList   *getPatternCEs();

    /**
     * Return the <code>BadCharacterTable</code> object computed for the pattern string.
     *
     * @return the <code>BadCharacterTable</code> object.
     *
     * @internal ICU 4.0.1 technology preview
     */
    BadCharacterTable *getBadCharacterTable();

    /**
     * Return the <code>GoodSuffixTable</code> object computed for the pattern string.
     *
     * @return the <code>GoodSuffixTable</code> object computed for the pattern string.
     *
     * @internal ICU 4.0.1 technology preview
     */
    GoodSuffixTable   *getGoodSuffixTable();

    /**
     * UObject glue...
     * @internal ICU 4.0.1 technology preview
     */
    virtual UClassID getDynamicClassID() const;
    /**
     * UObject glue...
     * @internal ICU 4.0.1 technology preview
     */
    static UClassID getStaticClassID();
    
private:
    CollData *data;
    CEList *patCEs;
    BadCharacterTable *badCharacterTable;
    GoodSuffixTable   *goodSuffixTable;
    UnicodeString pattern;
    Target *target;
};

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_COLLATION
#endif // #ifndef B_M_SEARCH_H
