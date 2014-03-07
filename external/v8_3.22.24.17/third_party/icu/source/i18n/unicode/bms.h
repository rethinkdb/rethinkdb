/*
 * Copyright (C) 1996-2010, International Business Machines Corporation and Others.
 * All rights reserved.
 */

/**
 * \file 
 * \brief C API: Boyer-Moore StringSearch prototype.
 * \internal
 */

#ifndef _BMS_H
#define _BMS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/ucol.h"

/**
 * A <code>UCD</code> object holds the Collator-specific data needed to
 * compute the length of the shortest string that can
 * generate a partcular list of CEs.
 *
 * <code>UCD</code> objects are quite expensive to compute. Because
 * of this, they are cached. When you call <code>ucd_open</code> it
 * returns a reference counted cached object. When you call <code>ucd_close</code>
 * the reference count on the object is decremented but the object is not deleted.
 *
 * If you do not need to reuse any unreferenced objects in the cache, you can call
 * <code>ucd_flushCCache</code>. If you no longer need any <code>UCD</code>
 * objects, you can call <code>ucd_freeCache</code>
 */
typedef void UCD;

/**
 * Open a <code>UCD</code> object.
 *
 * @param coll - the collator
 * @param status - will be set if any errors occur. 
 *
 * @return the <code>UCD</code> object. You must call
 *         <code>ucd_close</code> when you are done using the object.
 *
 * Note: if on return status is set to an error, the only safe
 * thing to do with the returned object is to call <code>ucd_close</code>.
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI UCD * U_EXPORT2
ucd_open(UCollator *coll, UErrorCode *status);

/**
 * Release a <code>UCD</code> object.
 *
 * @param ucd - the object
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI void U_EXPORT2
ucd_close(UCD *ucd);

/**
 * Get the <code>UCollator</code> object used to create a <code>UCD</code> object.
 * The <code>UCollator</code> object returned may not be the exact
 * object that was used to create this object, but it will have the
 * same behavior.
 *
 * @param ucd - the <code>UCD</code> object
 *
 * @return the <code>UCollator</code> used to create the given
 *         <code>UCD</code> object.
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI UCollator * U_EXPORT2
ucd_getCollator(UCD *ucd);

/**
 * <code>UCD</code> objects are expensive to compute, and so
 * may be cached. This routine will free the cached objects and delete
 * the cache.
 *
 * WARNING: Don't call this until you are have called <code>close</code>
 * for each <code>UCD</code> object that you have used. also,
 * DO NOT call this if another thread may be calling <code>ucd_flushCache</code>
 * at the same time.
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI void U_EXPORT2
ucd_freeCache();

/**
 * <code>UCD</code> objects are expensive to compute, and so
 * may be cached. This routine will remove any unused <code>UCD</code>
 * objects from the cache.
 *
 * @internal 4.0.1 technology preview
 */
U_CAPI void U_EXPORT2
ucd_flushCache();

/**
 * BMS
 *
 * This object holds the information needed to do a Collation sensitive Boyer-Moore search. It encapulates
 * the pattern, the "bad character" and "good suffix" tables, the Collator-based data needed to compute them,
 * and a reference to the text being searched.
 *
 * To do a search, you first need to get a <code>UCD</code> object by calling <code>ucd_open</code>.
 * Then you construct a <code>BMS</code> object from the <code>UCD</code> object, the pattern
 * string and the target string. Then you call the <code>search</code> method. Here's a code sample:
 *
 * <pre>
 * void boyerMooreExample(UCollator *collator, UChar *pattern, int32_t patternLen, UChar *target, int32_t targetLength)
 * {
 *     UErrorCode status = U_ZERO_ERROR;
 *     int32_t offset = 0, start = -1, end = -1;
 *     UCD *ucd = NULL);
 *     BMS *bms = NULL;
 *
 *     ucd = ucd_open(collator, &status);
 *     if (U_FAILURE(status)) {
 *         // could not create a UCD object
 *         return;
 *     }
 *
 *     BMS *bms = bms_open(ucd, pattern, patternLength, target, targetlength, &status);
 *     if (U_FAILURE(status)) {
 *         // could not create a BMS object
 *         ucd_close(ucd);
 *         return;
 *     }
 *
 *
 *     // Find all matches
 *     while (bms_search(bms, offset, &start, &end)) {
 *         // process the match between start and end
 *         ...
 *
 *         // advance past the match
 *         offset = end; 
 *     }
 *
 *     // at this point, if offset == 0, there were no matches
 *     if (offset == 0) {
 *         // handle the case of no matches
 *     }
 *
 *     bms_close(bms);
 *     ucd_close(ucd);
 *
 *     // UCD objects are cached, so the call to
 *     // ucd_close doesn't delete the object.
 *     // Call this if you don't need the object any more.
 *     ucd_flushCache();
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
 * NOTE: This is a technology preview. The final version of this API may not bear any resenblence to this API.
 *
 * @internal ICU 4.0.1 technology preview
 */
struct BMS;
typedef struct BMS BMS; /**< @see BMS */

/**
 * Construct a <code>MBS</code> object.
 *
 * @param ucd - A <code>UCD</code> object holding the Collator-sensitive data
 * @param pattern - the string for which to search
 * @param patternLength - the length of the string for which to search
 * @param target - the string in which to search
 * @param targetLength - the length of the string in which to search
 * @param status - will be set if any errors occur. 
 *
 * @return the <code>BMS</code> object.
 *
 * Note: if on return status is set to an error, the only safe
 * thing to do with the returned object is to call
 * <code>bms_close</code>.
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI BMS * U_EXPORT2
bms_open(UCD *ucd,
         const UChar *pattern, int32_t patternLength,
         const UChar *target,  int32_t targetLength,
         UErrorCode  *status);

/**
 * Close a <code>BMS</code> object and release all the
 * storage associated with it.
 *
 * @param bms - the <code>BMS</code> object to close.
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI void U_EXPORT2
bms_close(BMS *bms);

/**
 * Test the pattern to see if it generates any CEs.
 *
 * @param bms - the <code>BMS</code> object
 * @return <code>TRUE</code> if the pattern string did not generate any CEs
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI UBool U_EXPORT2
bms_empty(BMS *bms);

/**
 * Get the <code>UCD</code> object used to create
 * a given <code>BMS</code> object.
 *
 * @param bms - the <code>BMS</code> object
 *
 * @return - the <code>UCD</code> object used to create
 *           the given <code>BMS</code> object.
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI UCD * U_EXPORT2
bms_getData(BMS *bms);

/**
 * Search for the pattern string in the target string.
 *
 * @param bms - the <code>BMS</code> object
 * @param offset - the offset in the target string at which to begin the search
 * @param start - will be set to the starting offset of the match, or -1 if there's no match
 * @param end - will be set to the ending offset of the match, or -1 if there's no match
 *
 * @return <code>TRUE</code> if the match succeeds, <code>FALSE</code> otherwise.
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI UBool U_EXPORT2
bms_search(BMS *bms, int32_t offset, int32_t *start, int32_t *end);

/**
 * Set the target string for the match.
 *
 * @param bms - the <code>BMS</code> object
 * @param target - the new target string
 * @param targetLength - the length of the new target string
 * @param status - will be set if any errors occur. 
 *
 * @internal ICU 4.0.1 technology preview
 */
U_CAPI void U_EXPORT2
bms_setTargetString(BMS *bms, const UChar *target, int32_t targetLength, UErrorCode *status);

#endif

#endif /* _BMS_H */
