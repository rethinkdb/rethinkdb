/*
*******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  unistr_case.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:2
*
*   created on: 2004aug19
*   created by: Markus W. Scherer
*
*   Case-mapping functions moved here from unistr.cpp
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/locid.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/uchar.h"
#include "unicode/ubrk.h"
#include "ustr_imp.h"
#include "uhash.h"

U_NAMESPACE_BEGIN

//========================================
// Read-only implementation
//========================================

int8_t
UnicodeString::doCaseCompare(int32_t start,
                             int32_t length,
                             const UChar *srcChars,
                             int32_t srcStart,
                             int32_t srcLength,
                             uint32_t options) const
{
  // compare illegal string values
  // treat const UChar *srcChars==NULL as an empty string
  if(isBogus()) {
    return -1;
  }

  // pin indices to legal values
  pinIndices(start, length);

  if(srcChars == NULL) {
    srcStart = srcLength = 0;
  }

  // get the correct pointer
  const UChar *chars = getArrayStart();

  chars += start;
  srcChars += srcStart;

  if(chars != srcChars) {
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t result=u_strcmpFold(chars, length, srcChars, srcLength,
                                options|U_COMPARE_IGNORE_CASE, &errorCode);
    if(result!=0) {
      return (int8_t)(result >> 24 | 1);
    }
  } else {
    // get the srcLength if necessary
    if(srcLength < 0) {
      srcLength = u_strlen(srcChars + srcStart);
    }
    if(length != srcLength) {
      return (int8_t)((length - srcLength) >> 24 | 1);
    }
  }
  return 0;
}

//========================================
// Write implementation
//========================================

/*
 * Implement argument checking and buffer handling
 * for string case mapping as a common function.
 */

UnicodeString &
UnicodeString::caseMap(BreakIterator *titleIter,
                       const char *locale,
                       uint32_t options,
                       int32_t toWhichCase) {
  if(isEmpty() || !isWritable()) {
    // nothing to do
    return *this;
  }

  const UCaseProps *csp=ucase_getSingleton();

  // We need to allocate a new buffer for the internal string case mapping function.
  // This is very similar to how doReplace() keeps the old array pointer
  // and deletes the old array itself after it is done.
  // In addition, we are forcing cloneArrayIfNeeded() to always allocate a new array.
  UChar oldStackBuffer[US_STACKBUF_SIZE];
  UChar *oldArray;
  int32_t oldLength;

  if(fFlags&kUsingStackBuffer) {
    // copy the stack buffer contents because it will be overwritten
    u_memcpy(oldStackBuffer, fUnion.fStackBuffer, fShortLength);
    oldArray = oldStackBuffer;
    oldLength = fShortLength;
  } else {
    oldArray = getArrayStart();
    oldLength = length();
  }

  int32_t capacity;
  if(oldLength <= US_STACKBUF_SIZE) {
    capacity = US_STACKBUF_SIZE;
  } else {
    capacity = oldLength + 20;
  }
  int32_t *bufferToDelete = 0;
  if(!cloneArrayIfNeeded(capacity, capacity, FALSE, &bufferToDelete, TRUE)) {
    return *this;
  }

  // Case-map, and if the result is too long, then reallocate and repeat.
  UErrorCode errorCode;
  int32_t newLength;
  do {
    errorCode = U_ZERO_ERROR;
    if(toWhichCase==TO_LOWER) {
      newLength = ustr_toLower(csp, getArrayStart(), getCapacity(),
                               oldArray, oldLength,
                               locale, &errorCode);
    } else if(toWhichCase==TO_UPPER) {
      newLength = ustr_toUpper(csp, getArrayStart(), getCapacity(),
                               oldArray, oldLength,
                               locale, &errorCode);
    } else if(toWhichCase==TO_TITLE) {
#if UCONFIG_NO_BREAK_ITERATION
        errorCode=U_UNSUPPORTED_ERROR;
#else
      newLength = ustr_toTitle(csp, getArrayStart(), getCapacity(),
                               oldArray, oldLength,
                               (UBreakIterator *)titleIter, locale, options, &errorCode);
#endif
    } else {
      newLength = ustr_foldCase(csp, getArrayStart(), getCapacity(),
                                oldArray, oldLength,
                                options,
                                &errorCode);
    }
    setLength(newLength);
  } while(errorCode==U_BUFFER_OVERFLOW_ERROR && cloneArrayIfNeeded(newLength, newLength, FALSE));

  if (bufferToDelete) {
    uprv_free(bufferToDelete);
  }
  if(U_FAILURE(errorCode)) {
    setToBogus();
  }
  return *this;
}

UnicodeString &
UnicodeString::toLower() {
  return caseMap(0, Locale::getDefault().getName(), 0, TO_LOWER);
}

UnicodeString &
UnicodeString::toLower(const Locale &locale) {
  return caseMap(0, locale.getName(), 0, TO_LOWER);
}

UnicodeString &
UnicodeString::toUpper() {
  return caseMap(0, Locale::getDefault().getName(), 0, TO_UPPER);
}

UnicodeString &
UnicodeString::toUpper(const Locale &locale) {
  return caseMap(0, locale.getName(), 0, TO_UPPER);
}

#if !UCONFIG_NO_BREAK_ITERATION

UnicodeString &
UnicodeString::toTitle(BreakIterator *titleIter) {
  return caseMap(titleIter, Locale::getDefault().getName(), 0, TO_TITLE);
}

UnicodeString &
UnicodeString::toTitle(BreakIterator *titleIter, const Locale &locale) {
  return caseMap(titleIter, locale.getName(), 0, TO_TITLE);
}

UnicodeString &
UnicodeString::toTitle(BreakIterator *titleIter, const Locale &locale, uint32_t options) {
  return caseMap(titleIter, locale.getName(), options, TO_TITLE);
}

#endif

UnicodeString &
UnicodeString::foldCase(uint32_t options) {
    /* The Locale parameter isn't used. Use "" instead. */
    return caseMap(0, "", options, FOLD_CASE);
}

U_NAMESPACE_END

// Defined here to reduce dependencies on break iterator
U_CAPI int32_t U_EXPORT2
uhash_hashCaselessUnicodeString(const UHashTok key) {
    U_NAMESPACE_USE
    const UnicodeString *str = (const UnicodeString*) key.pointer;
    if (str == NULL) {
        return 0;
    }
    // Inefficient; a better way would be to have a hash function in
    // UnicodeString that does case folding on the fly.
    UnicodeString copy(*str);
    return copy.foldCase().hashCode();
}

// Defined here to reduce dependencies on break iterator
U_CAPI UBool U_EXPORT2
uhash_compareCaselessUnicodeString(const UHashTok key1, const UHashTok key2) {
    U_NAMESPACE_USE
    const UnicodeString *str1 = (const UnicodeString*) key1.pointer;
    const UnicodeString *str2 = (const UnicodeString*) key2.pointer;
    if (str1 == str2) {
        return TRUE;
    }
    if (str1 == NULL || str2 == NULL) {
        return FALSE;
    }
    return str1->caseCompare(*str2, U_FOLD_CASE_DEFAULT) == 0;
}

