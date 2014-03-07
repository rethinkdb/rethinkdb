/*
*******************************************************************************
* Copyright (C) 1996-2006, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
//===============================================================================
//
// File sortkey.cpp
//
//
//
// Created by: Helena Shih
//
// Modification History:
//
//  Date         Name          Description
//
//  6/20/97      helena        Java class name change.
//  6/23/97      helena        Added comments to make code more readable.
//  6/26/98      erm           Canged to use byte arrays instead of UnicodeString
//  7/31/98      erm           hashCode: minimum inc should be 2 not 1,
//                             Cleaned up operator=
// 07/12/99      helena        HPUX 11 CC port.
// 03/06/01      synwee        Modified compareTo, to handle the result of
//                             2 string similar in contents, but one is longer
//                             than the other
//===============================================================================

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/sortkey.h"
#include "cmemory.h"
#include "uhash.h"

U_NAMESPACE_BEGIN

// A hash code of kInvalidHashCode indicates that the has code needs
// to be computed. A hash code of kEmptyHashCode is used for empty keys
// and for any key whose computed hash code is kInvalidHashCode.
#define kInvalidHashCode ((int32_t)0)
#define kEmptyHashCode ((int32_t)1)

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CollationKey)

CollationKey::CollationKey()
    : UObject(), fBogus(FALSE), fCount(0), fCapacity(0),
      fHashCode(kEmptyHashCode), fBytes(NULL)
{
}

// Create a collation key from a bit array.
CollationKey::CollationKey(const uint8_t* newValues, int32_t count)
    : UObject(), fBogus(FALSE), fCount(count), fCapacity(count),
      fHashCode(kInvalidHashCode)
{
    fBytes = (uint8_t *)uprv_malloc(count);

    if (fBytes == NULL)
    {
        setToBogus();
        return;
    }

    uprv_memcpy(fBytes, newValues, fCount);
}

CollationKey::CollationKey(const CollationKey& other)
: UObject(other), fBogus(FALSE), fCount(other.fCount), fCapacity(other.fCapacity),
  fHashCode(other.fHashCode), fBytes(NULL)
{
    if (other.fBogus)
    {
        setToBogus();
        return;
    }

    fBytes = (uint8_t *)uprv_malloc(fCapacity);

    if (fBytes == NULL)
    {
        setToBogus();
        return;
    }

    uprv_memcpy(fBytes, other.fBytes, other.fCount);
    if(fCapacity>fCount) {
        uprv_memset(fBytes+fCount, 0, fCapacity-fCount);
    }
}

CollationKey::~CollationKey()
{
        uprv_free(fBytes);
}

void CollationKey::adopt(uint8_t *values, int32_t count) {
    if(fBytes != NULL) {
        uprv_free(fBytes);
    }
    fBogus = FALSE;
    fBytes = values;
    fCount = count;
    fCapacity = count;
    fHashCode = kInvalidHashCode;
}

// set the key to an empty state
CollationKey&
CollationKey::reset()
{
    fCount = 0;
    fBogus = FALSE;
    fHashCode = kEmptyHashCode;

    return *this;
}

// set the key to a "bogus" or invalid state
CollationKey&
CollationKey::setToBogus()
{
    uprv_free(fBytes);
    fBytes = NULL;

    fCapacity = 0;
    fCount = 0;
    fHashCode = kInvalidHashCode;

    return *this;
}

UBool
CollationKey::operator==(const CollationKey& source) const
{
    return (this->fCount == source.fCount &&
            (this->fBytes == source.fBytes ||
             uprv_memcmp(this->fBytes, source.fBytes, this->fCount) == 0));
}

const CollationKey&
CollationKey::operator=(const CollationKey& other)
{
    if (this != &other)
    {
        if (other.isBogus())
        {
            return setToBogus();
        }

        if (other.fBytes != NULL)
        {
            ensureCapacity(other.fCount);

            if (isBogus())
            {
                return *this;
            }

            fHashCode = other.fHashCode;
            uprv_memcpy(fBytes, other.fBytes, fCount);
        }
        else
        {
            fCount = 0;
            fBogus = FALSE;
            fHashCode = kEmptyHashCode;
        }
    }

    return *this;
}

// Bitwise comparison for the collation keys.
// NOTE: this is somewhat messy 'cause we can't count
// on memcmp returning the exact values which match
// Collator::EComparisonResult
Collator::EComparisonResult
CollationKey::compareTo(const CollationKey& target) const
{
    uint8_t *src = this->fBytes;
    uint8_t *tgt = target.fBytes;

    // are we comparing the same string
    if (src == tgt)
        return  Collator::EQUAL;

        /*
        int count = (this->fCount < target.fCount) ? this->fCount : target.fCount;
        if (count == 0)
        {
        // If count is 0, at least one of the keys is empty.
        // An empty key is always LESS than a non-empty one
        // and EQUAL to another empty
        if (this->fCount < target.fCount)
        {
        return Collator::LESS;
        }

          if (this->fCount > target.fCount)
          {
          return Collator::GREATER;
          }
          return Collator::EQUAL;
          }
    */

    int                         minLength;
    Collator::EComparisonResult result;

    // are we comparing different lengths?
    if (this->fCount != target.fCount) {
        if (this->fCount < target.fCount) {
            minLength = this->fCount;
            result    =  Collator::LESS;
        }
        else {
            minLength = target.fCount;
            result    =  Collator::GREATER;
        }
    }
    else {
        minLength = target.fCount;
        result    =  Collator::EQUAL;
    }

    if (minLength > 0) {
        int diff = uprv_memcmp(src, tgt, minLength);
        if (diff > 0) {
            return Collator::GREATER;
        }
        else
            if (diff < 0) {
                return Collator::LESS;
            }
    }

    return result;
    /*
    if (result < 0)
    {
    return Collator::LESS;
    }

      if (result > 0)
      {
      return Collator::GREATER;
      }
      return Collator::EQUAL;
    */
}

// Bitwise comparison for the collation keys.
UCollationResult
CollationKey::compareTo(const CollationKey& target, UErrorCode &status) const
{
  if(U_SUCCESS(status)) {
    uint8_t *src = this->fBytes;
    uint8_t *tgt = target.fBytes;

    // are we comparing the same string
    if (src == tgt)
        return  UCOL_EQUAL;

    int                         minLength;
    UCollationResult result;

    // are we comparing different lengths?
    if (this->fCount != target.fCount) {
        if (this->fCount < target.fCount) {
            minLength = this->fCount;
            result    =  UCOL_LESS;
        }
        else {
            minLength = target.fCount;
            result    =  UCOL_GREATER;
        }
    }
    else {
        minLength = target.fCount;
        result    =  UCOL_EQUAL;
    }

    if (minLength > 0) {
        int diff = uprv_memcmp(src, tgt, minLength);
        if (diff > 0) {
            return UCOL_GREATER;
        }
        else
            if (diff < 0) {
                return UCOL_LESS;
            }
    }

    return result;
  } else {
    return UCOL_EQUAL;
  }
}

CollationKey&
CollationKey::ensureCapacity(int32_t newSize)
{
    if (fCapacity < newSize)
    {
        uprv_free(fBytes);

        fBytes = (uint8_t *)uprv_malloc(newSize);

        if (fBytes == NULL)
        {
            return setToBogus();
        }

        uprv_memset(fBytes, 0, fCapacity);
        fCapacity = newSize;
    }

    fBogus = FALSE;
    fCount = newSize;
    fHashCode = kInvalidHashCode;

    return *this;
}

#ifdef U_USE_COLLATION_KEY_DEPRECATES
// Create a copy of the byte array.
uint8_t*
CollationKey::toByteArray(int32_t& count) const
{
    uint8_t *result = (uint8_t*) uprv_malloc( sizeof(uint8_t) * fCount );

    if (result == NULL)
    {
        count = 0;
    }
    else
    {
        count = fCount;
        uprv_memcpy(result, fBytes, fCount);
    }

    return result;
}
#endif

int32_t
CollationKey::hashCode() const
{
    // (Cribbed from UnicodeString)
    // We cache the hashCode; when it becomes invalid, due to any change to the
    // string, we note this by setting it to kInvalidHashCode. [LIU]

    // Note: This method is semantically const, but physically non-const.

    if (fHashCode == kInvalidHashCode)
    {
        UHashTok key;
        key.pointer = fBytes;
        ((CollationKey *)this)->fHashCode = uhash_hashChars(key);
#if 0
        // We compute the hash by iterating sparsely over 64 (at most) characters
        // spaced evenly through the string.  For each character, we multiply the
        // previous hash value by a prime number and add the new character in,
        // in the manner of a additive linear congruential random number generator,
        // thus producing a pseudorandom deterministic value which should be well
        // distributed over the output range. [LIU]
        const uint8_t   *p = fBytes, *limit = fBytes + fCount;
        int32_t         inc = (fCount >= 256) ? fCount/128 : 2; // inc = max(fSize/64, 1);
        int32_t         hash = 0;

        while (p < limit)
        {
            hash = ( hash * 37 ) + ((p[0] << 8) + p[1]);
            p += inc;
        }

        // If we happened to get kInvalidHashCode, replace it with kEmptyHashCode
        if (hash == kInvalidHashCode)
        {
            hash = kEmptyHashCode;
        }

        ((CollationKey *)this)->fHashCode = hash; // cast away const
#endif
    }

    return fHashCode;
}

U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
ucol_keyHashCode(const uint8_t *key, 
                       int32_t  length)
{
    U_NAMESPACE_QUALIFIER CollationKey newKey(key, length);
    return newKey.hashCode();
}

#endif /* #if !UCONFIG_NO_COLLATION */
