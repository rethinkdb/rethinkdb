/*  
**********************************************************************
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  ustr_imp.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001jan30
*   created by: Markus W. Scherer
*/

#ifndef __USTR_IMP_H__
#define __USTR_IMP_H__

#include "unicode/utypes.h"
#include "unicode/uiter.h"
#include "ucase.h"

/** Simple declaration for u_strToTitle() to avoid including unicode/ubrk.h. */
#ifndef UBRK_TYPEDEF_UBREAK_ITERATOR
#   define UBRK_TYPEDEF_UBREAK_ITERATOR
    typedef struct UBreakIterator UBreakIterator;
#endif

#ifndef U_COMPARE_IGNORE_CASE
/* see also unorm.h */
/**
 * Option bit for unorm_compare:
 * Perform case-insensitive comparison.
 * @draft ICU 2.2
 */
#define U_COMPARE_IGNORE_CASE       0x10000
#endif

/**
 * Internal option for unorm_cmpEquivFold() for strncmp style.
 * If set, checks for both string length and terminating NUL.
 * @internal
 */
#define _STRNCMP_STYLE 0x1000

/**
 * Compare two strings in code point order or code unit order.
 * Works in strcmp style (both lengths -1),
 * strncmp style (lengths equal and >=0, flag TRUE),
 * and memcmp/UnicodeString style (at least one length >=0).
 * @internal
 */
U_CFUNC int32_t U_EXPORT2
uprv_strCompare(const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                UBool strncmpStyle, UBool codePointOrder);

/**
 * Internal API, used by u_strcasecmp() etc.
 * Compare strings case-insensitively,
 * in code point order or code unit order.
 * @internal
 */
U_CFUNC int32_t
u_strcmpFold(const UChar *s1, int32_t length1,
             const UChar *s2, int32_t length2,
             uint32_t options,
             UErrorCode *pErrorCode);

/**
 * Are the Unicode properties loaded?
 * This must be used before internal functions are called that do
 * not perform this check.
 * Generate a debug assertion failure if data is not loaded.
 * @internal
 */
U_CFUNC UBool
uprv_haveProperties(UErrorCode *pErrorCode);

/**
  * Load the Unicode property data.
  * Intended primarily for use from u_init().
  * Has no effect if property data is already loaded.
  * NOT thread safe.
  * @internal
  */
/*U_CFUNC int8_t
uprv_loadPropsData(UErrorCode *errorCode);*/

/*
 * Internal string casing functions implementing
 * ustring.h/ustrcase.c and UnicodeString case mapping functions.
 */

/**
 * @internal
 */
struct UCaseMap {
    const UCaseProps *csp;
#if !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *iter;  /* We adopt the iterator, so we own it. */
#endif
    char locale[32];
    int32_t locCache;
    uint32_t options;
};

#ifndef __UCASEMAP_H__
typedef struct UCaseMap UCaseMap;
#endif

/**
 * @internal
 */
enum {
    TO_LOWER,
    TO_UPPER,
    TO_TITLE,
    FOLD_CASE
};

/**
 * @internal
 */
U_CFUNC int32_t
ustr_toLower(const UCaseProps *csp,
             UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode);

/**
 * @internal
 */
U_CFUNC int32_t
ustr_toUpper(const UCaseProps *csp,
             UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode);

#if !UCONFIG_NO_BREAK_ITERATION

/**
 * @internal
 */
U_CFUNC int32_t
ustr_toTitle(const UCaseProps *csp,
             UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             UBreakIterator *titleIter,
             const char *locale, uint32_t options,
             UErrorCode *pErrorCode);

#endif

/**
 * Internal case folding function.
 * @internal
 */
U_CFUNC int32_t
ustr_foldCase(const UCaseProps *csp,
              UChar *dest, int32_t destCapacity,
              const UChar *src, int32_t srcLength,
              uint32_t options,
              UErrorCode *pErrorCode);

/**
 * NUL-terminate a UChar * string if possible.
 * If length  < destCapacity then NUL-terminate.
 * If length == destCapacity then do not terminate but set U_STRING_NOT_TERMINATED_WARNING.
 * If length  > destCapacity then do not terminate but set U_BUFFER_OVERFLOW_ERROR.
 *
 * @param dest Destination buffer, can be NULL if destCapacity==0.
 * @param destCapacity Number of UChars available at dest.
 * @param length Number of UChars that were (to be) written to dest.
 * @param pErrorCode ICU error code.
 * @return length
 * @internal
 */
U_CAPI int32_t U_EXPORT2
u_terminateUChars(UChar *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);

/**
 * NUL-terminate a char * string if possible.
 * Same as u_terminateUChars() but for a different string type.
 */
U_CAPI int32_t U_EXPORT2
u_terminateChars(char *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);

/**
 * NUL-terminate a UChar32 * string if possible.
 * Same as u_terminateUChars() but for a different string type.
 */
U_CAPI int32_t U_EXPORT2
u_terminateUChar32s(UChar32 *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);

/**
 * NUL-terminate a wchar_t * string if possible.
 * Same as u_terminateUChars() but for a different string type.
 */
U_CAPI int32_t U_EXPORT2
u_terminateWChars(wchar_t *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);

#endif
