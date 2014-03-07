/*
******************************************************************************
* Copyright (C) 1999-2010, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*
* File unistr.cpp
*
* Modification History:
*
*   Date        Name        Description
*   09/25/98    stephen     Creation.
*   04/20/99    stephen     Overhauled per 4/16 code review.
*   07/09/99    stephen     Renamed {hi,lo},{byte,word} to icu_X for HP/UX
*   11/18/99    aliu        Added handleReplaceBetween() to make inherit from
*                           Replaceable.
*   06/25/01    grhoten     Removed the dependency on iostream
******************************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "uhash.h"
#include "ustr_imp.h"
#include "umutex.h"

#if 0

#if U_IOSTREAM_SOURCE >= 199711
#include <iostream>
using namespace std;
#elif U_IOSTREAM_SOURCE >= 198506
#include <iostream.h>
#endif

//DEBUGGING
void
print(const UnicodeString& s,
      const char *name)
{
  UChar c;
  cout << name << ":|";
  for(int i = 0; i < s.length(); ++i) {
    c = s[i];
    if(c>= 0x007E || c < 0x0020)
      cout << "[0x" << hex << s[i] << "]";
    else
      cout << (char) s[i];
  }
  cout << '|' << endl;
}

void
print(const UChar *s,
      int32_t len,
      const char *name)
{
  UChar c;
  cout << name << ":|";
  for(int i = 0; i < len; ++i) {
    c = s[i];
    if(c>= 0x007E || c < 0x0020)
      cout << "[0x" << hex << s[i] << "]";
    else
      cout << (char) s[i];
  }
  cout << '|' << endl;
}
// END DEBUGGING
#endif

// Local function definitions for now

// need to copy areas that may overlap
static
inline void
us_arrayCopy(const UChar *src, int32_t srcStart,
         UChar *dst, int32_t dstStart, int32_t count)
{
  if(count>0) {
    uprv_memmove(dst+dstStart, src+srcStart, (size_t)(count*sizeof(*src)));
  }
}

// u_unescapeAt() callback to get a UChar from a UnicodeString
U_CDECL_BEGIN
static UChar U_CALLCONV
UnicodeString_charAt(int32_t offset, void *context) {
    return ((U_NAMESPACE_QUALIFIER UnicodeString*) context)->charAt(offset);
}
U_CDECL_END

U_NAMESPACE_BEGIN

/* The Replaceable virtual destructor can't be defined in the header
   due to how AIX works with multiple definitions of virtual functions.
*/
Replaceable::~Replaceable() {}
Replaceable::Replaceable() {}
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UnicodeString)

UnicodeString U_EXPORT2
operator+ (const UnicodeString &s1, const UnicodeString &s2) {
    return
        UnicodeString(s1.length()+s2.length()+1, (UChar32)0, 0).
            append(s1).
                append(s2);
}

//========================================
// Reference Counting functions, put at top of file so that optimizing compilers
//                               have a chance to automatically inline.
//========================================

void
UnicodeString::addRef()
{  umtx_atomic_inc((int32_t *)fUnion.fFields.fArray - 1);}

int32_t
UnicodeString::removeRef()
{ return umtx_atomic_dec((int32_t *)fUnion.fFields.fArray - 1);}

int32_t
UnicodeString::refCount() const 
{ 
    umtx_lock(NULL);
    // Note: without the lock to force a memory barrier, we might see a very
    //       stale value on some multi-processor systems.
    int32_t  count = *((int32_t *)fUnion.fFields.fArray - 1);
    umtx_unlock(NULL);
    return count;
 }

void
UnicodeString::releaseArray() {
  if((fFlags & kRefCounted) && removeRef() == 0) {
    uprv_free((int32_t *)fUnion.fFields.fArray - 1);
  }
}



//========================================
// Constructors
//========================================
UnicodeString::UnicodeString()
  : fShortLength(0),
    fFlags(kShortString)
{}

UnicodeString::UnicodeString(int32_t capacity, UChar32 c, int32_t count)
  : fShortLength(0),
    fFlags(0)
{
  if(count <= 0 || (uint32_t)c > 0x10ffff) {
    // just allocate and do not do anything else
    allocate(capacity);
  } else {
    // count > 0, allocate and fill the new string with count c's
    int32_t unitCount = UTF_CHAR_LENGTH(c), length = count * unitCount;
    if(capacity < length) {
      capacity = length;
    }
    if(allocate(capacity)) {
      UChar *array = getArrayStart();
      int32_t i = 0;

      // fill the new string with c
      if(unitCount == 1) {
        // fill with length UChars
        while(i < length) {
          array[i++] = (UChar)c;
        }
      } else {
        // get the code units for c
        UChar units[UTF_MAX_CHAR_LENGTH];
        UTF_APPEND_CHAR_UNSAFE(units, i, c);

        // now it must be i==unitCount
        i = 0;

        // for Unicode, unitCount can only be 1, 2, 3, or 4
        // 1 is handled above
        while(i < length) {
          int32_t unitIdx = 0;
          while(unitIdx < unitCount) {
            array[i++]=units[unitIdx++];
          }
        }
      }
    }
    setLength(length);
  }
}

UnicodeString::UnicodeString(UChar ch)
  : fShortLength(1),
    fFlags(kShortString)
{
  fUnion.fStackBuffer[0] = ch;
}

UnicodeString::UnicodeString(UChar32 ch)
  : fShortLength(0),
    fFlags(kShortString)
{
  int32_t i = 0;
  UBool isError = FALSE;
  U16_APPEND(fUnion.fStackBuffer, i, US_STACKBUF_SIZE, ch, isError);
  fShortLength = (int8_t)i;
}

UnicodeString::UnicodeString(const UChar *text)
  : fShortLength(0),
    fFlags(kShortString)
{
  doReplace(0, 0, text, 0, -1);
}

UnicodeString::UnicodeString(const UChar *text,
                             int32_t textLength)
  : fShortLength(0),
    fFlags(kShortString)
{
  doReplace(0, 0, text, 0, textLength);
}

UnicodeString::UnicodeString(UBool isTerminated,
                             const UChar *text,
                             int32_t textLength)
  : fShortLength(0),
    fFlags(kReadonlyAlias)
{
  if(text == NULL) {
    // treat as an empty string, do not alias
    setToEmpty();
  } else if(textLength < -1 ||
            (textLength == -1 && !isTerminated) ||
            (textLength >= 0 && isTerminated && text[textLength] != 0)
  ) {
    setToBogus();
  } else {
    if(textLength == -1) {
      // text is terminated, or else it would have failed the above test
      textLength = u_strlen(text);
    }
    setArray((UChar *)text, textLength, isTerminated ? textLength + 1 : textLength);
  }
}

UnicodeString::UnicodeString(UChar *buff,
                             int32_t buffLength,
                             int32_t buffCapacity)
  : fShortLength(0),
    fFlags(kWritableAlias)
{
  if(buff == NULL) {
    // treat as an empty string, do not alias
    setToEmpty();
  } else if(buffLength < -1 || buffCapacity < 0 || buffLength > buffCapacity) {
    setToBogus();
  } else {
    if(buffLength == -1) {
      // fLength = u_strlen(buff); but do not look beyond buffCapacity
      const UChar *p = buff, *limit = buff + buffCapacity;
      while(p != limit && *p != 0) {
        ++p;
      }
      buffLength = (int32_t)(p - buff);
    }
    setArray(buff, buffLength, buffCapacity);
  }
}

UnicodeString::UnicodeString(const char *src, int32_t length, EInvariant)
  : fShortLength(0),
    fFlags(kShortString)
{
  if(src==NULL) {
    // treat as an empty string
  } else {
    if(length<0) {
      length=(int32_t)uprv_strlen(src);
    }
    if(cloneArrayIfNeeded(length, length, FALSE)) {
      u_charsToUChars(src, getArrayStart(), length);
      setLength(length);
    } else {
      setToBogus();
    }
  }
}

#if U_CHARSET_IS_UTF8

UnicodeString::UnicodeString(const char *codepageData)
  : fShortLength(0),
    fFlags(kShortString) {
  if(codepageData != 0) {
    setToUTF8(codepageData);
  }
}

UnicodeString::UnicodeString(const char *codepageData, int32_t dataLength)
  : fShortLength(0),
    fFlags(kShortString) {
  // if there's nothing to convert, do nothing
  if(codepageData == 0 || dataLength == 0 || dataLength < -1) {
    return;
  }
  if(dataLength == -1) {
    dataLength = (int32_t)uprv_strlen(codepageData);
  }
  setToUTF8(StringPiece(codepageData, dataLength));
}

// else see unistr_cnv.cpp
#endif

UnicodeString::UnicodeString(const UnicodeString& that)
  : Replaceable(),
    fShortLength(0),
    fFlags(kShortString)
{
  copyFrom(that);
}

UnicodeString::UnicodeString(const UnicodeString& that,
                             int32_t srcStart)
  : Replaceable(),
    fShortLength(0),
    fFlags(kShortString)
{
  setTo(that, srcStart);
}

UnicodeString::UnicodeString(const UnicodeString& that,
                             int32_t srcStart,
                             int32_t srcLength)
  : Replaceable(),
    fShortLength(0),
    fFlags(kShortString)
{
  setTo(that, srcStart, srcLength);
}

// Replaceable base class clone() default implementation, does not clone
Replaceable *
Replaceable::clone() const {
  return NULL;
}

// UnicodeString overrides clone() with a real implementation
Replaceable *
UnicodeString::clone() const {
  return new UnicodeString(*this);
}

//========================================
// array allocation
//========================================

UBool
UnicodeString::allocate(int32_t capacity) {
  if(capacity <= US_STACKBUF_SIZE) {
    fFlags = kShortString;
  } else {
    // count bytes for the refCounter and the string capacity, and
    // round up to a multiple of 16; then divide by 4 and allocate int32_t's
    // to be safely aligned for the refCount
    // the +1 is for the NUL terminator, to avoid reallocation in getTerminatedBuffer()
    int32_t words = (int32_t)(((sizeof(int32_t) + (capacity + 1) * U_SIZEOF_UCHAR + 15) & ~15) >> 2);
    int32_t *array = (int32_t*) uprv_malloc( sizeof(int32_t) * words );
    if(array != 0) {
      // set initial refCount and point behind the refCount
      *array++ = 1;

      // have fArray point to the first UChar
      fUnion.fFields.fArray = (UChar *)array;
      fUnion.fFields.fCapacity = (int32_t)((words - 1) * (sizeof(int32_t) / U_SIZEOF_UCHAR));
      fFlags = kLongString;
    } else {
      fShortLength = 0;
      fUnion.fFields.fArray = 0;
      fUnion.fFields.fCapacity = 0;
      fFlags = kIsBogus;
      return FALSE;
    }
  }
  return TRUE;
}

//========================================
// Destructor
//========================================
UnicodeString::~UnicodeString()
{
  releaseArray();
}

//========================================
// Factory methods
//========================================

UnicodeString UnicodeString::fromUTF8(const StringPiece &utf8) {
  UnicodeString result;
  result.setToUTF8(utf8);
  return result;
}

UnicodeString UnicodeString::fromUTF32(const UChar32 *utf32, int32_t length) {
  UnicodeString result;
  int32_t capacity;
  // Most UTF-32 strings will be BMP-only and result in a same-length
  // UTF-16 string. We overestimate the capacity just slightly,
  // just in case there are a few supplementary characters.
  if(length <= US_STACKBUF_SIZE) {
    capacity = US_STACKBUF_SIZE;
  } else {
    capacity = length + (length >> 4) + 4;
  }
  do {
    UChar *utf16 = result.getBuffer(capacity);
    int32_t length16;
    UErrorCode errorCode = U_ZERO_ERROR;
    u_strFromUTF32WithSub(utf16, result.getCapacity(), &length16,
        utf32, length,
        0xfffd,  // Substitution character.
        NULL,    // Don't care about number of substitutions.
        &errorCode);
    result.releaseBuffer(length16);
    if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
      capacity = length16 + 1;  // +1 for the terminating NUL.
      continue;
    } else if(U_FAILURE(errorCode)) {
      result.setToBogus();
    }
    break;
  } while(TRUE);
  return result;
}

//========================================
// Assignment
//========================================

UnicodeString &
UnicodeString::operator=(const UnicodeString &src) {
  return copyFrom(src);
}

UnicodeString &
UnicodeString::fastCopyFrom(const UnicodeString &src) {
  return copyFrom(src, TRUE);
}

UnicodeString &
UnicodeString::copyFrom(const UnicodeString &src, UBool fastCopy) {
  // if assigning to ourselves, do nothing
  if(this == 0 || this == &src) {
    return *this;
  }

  // is the right side bogus?
  if(&src == 0 || src.isBogus()) {
    setToBogus();
    return *this;
  }

  // delete the current contents
  releaseArray();

  if(src.isEmpty()) {
    // empty string - use the stack buffer
    setToEmpty();
    return *this;
  }

  // we always copy the length
  int32_t srcLength = src.length();
  setLength(srcLength);

  // fLength>0 and not an "open" src.getBuffer(minCapacity)
  switch(src.fFlags) {
  case kShortString:
    // short string using the stack buffer, do the same
    fFlags = kShortString;
    uprv_memcpy(fUnion.fStackBuffer, src.fUnion.fStackBuffer, srcLength * U_SIZEOF_UCHAR);
    break;
  case kLongString:
    // src uses a refCounted string buffer, use that buffer with refCount
    // src is const, use a cast - we don't really change it
    ((UnicodeString &)src).addRef();
    // copy all fields, share the reference-counted buffer
    fUnion.fFields.fArray = src.fUnion.fFields.fArray;
    fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
    fFlags = src.fFlags;
    break;
  case kReadonlyAlias:
    if(fastCopy) {
      // src is a readonly alias, do the same
      // -> maintain the readonly alias as such
      fUnion.fFields.fArray = src.fUnion.fFields.fArray;
      fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
      fFlags = src.fFlags;
      break;
    }
    // else if(!fastCopy) fall through to case kWritableAlias
    // -> allocate a new buffer and copy the contents
  case kWritableAlias:
    // src is a writable alias; we make a copy of that instead
    if(allocate(srcLength)) {
      uprv_memcpy(getArrayStart(), src.getArrayStart(), srcLength * U_SIZEOF_UCHAR);
      break;
    }
    // if there is not enough memory, then fall through to setting to bogus
  default:
    // if src is bogus, set ourselves to bogus
    // do not call setToBogus() here because fArray and fFlags are not consistent here
    fShortLength = 0;
    fUnion.fFields.fArray = 0;
    fUnion.fFields.fCapacity = 0;
    fFlags = kIsBogus;
    break;
  }

  return *this;
}

//========================================
// Miscellaneous operations
//========================================

UnicodeString UnicodeString::unescape() const {
    UnicodeString result(length(), (UChar32)0, (int32_t)0); // construct with capacity
    const UChar *array = getBuffer();
    int32_t len = length();
    int32_t prev = 0;
    for (int32_t i=0;;) {
        if (i == len) {
            result.append(array, prev, len - prev);
            break;
        }
        if (array[i++] == 0x5C /*'\\'*/) {
            result.append(array, prev, (i - 1) - prev);
            UChar32 c = unescapeAt(i); // advances i
            if (c < 0) {
                result.remove(); // return empty string
                break; // invalid escape sequence
            }
            result.append(c);
            prev = i;
        }
    }
    return result;
}

UChar32 UnicodeString::unescapeAt(int32_t &offset) const {
    return u_unescapeAt(UnicodeString_charAt, &offset, length(), (void*)this);
}

//========================================
// Read-only implementation
//========================================
int8_t
UnicodeString::doCompare( int32_t start,
              int32_t length,
              const UChar *srcChars,
              int32_t srcStart,
              int32_t srcLength) const
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

  int32_t minLength;
  int8_t lengthResult;

  // get the srcLength if necessary
  if(srcLength < 0) {
    srcLength = u_strlen(srcChars + srcStart);
  }

  // are we comparing different lengths?
  if(length != srcLength) {
    if(length < srcLength) {
      minLength = length;
      lengthResult = -1;
    } else {
      minLength = srcLength;
      lengthResult = 1;
    }
  } else {
    minLength = length;
    lengthResult = 0;
  }

  /*
   * note that uprv_memcmp() returns an int but we return an int8_t;
   * we need to take care not to truncate the result -
   * one way to do this is to right-shift the value to
   * move the sign bit into the lower 8 bits and making sure that this
   * does not become 0 itself
   */

  if(minLength > 0 && chars != srcChars) {
    int32_t result;

#   if U_IS_BIG_ENDIAN 
      // big-endian: byte comparison works
      result = uprv_memcmp(chars, srcChars, minLength * sizeof(UChar));
      if(result != 0) {
        return (int8_t)(result >> 15 | 1);
      }
#   else
      // little-endian: compare UChar units
      do {
        result = ((int32_t)*(chars++) - (int32_t)*(srcChars++));
        if(result != 0) {
          return (int8_t)(result >> 15 | 1);
        }
      } while(--minLength > 0);
#   endif
  }
  return lengthResult;
}

/* String compare in code point order - doCompare() compares in code unit order. */
int8_t
UnicodeString::doCompareCodePointOrder(int32_t start,
                                       int32_t length,
                                       const UChar *srcChars,
                                       int32_t srcStart,
                                       int32_t srcLength) const
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

  int32_t diff = uprv_strCompare(getArrayStart() + start, length, srcChars + srcStart, srcLength, FALSE, TRUE);
  /* translate the 32-bit result into an 8-bit one */
  if(diff!=0) {
    return (int8_t)(diff >> 15 | 1);
  } else {
    return 0;
  }
}

int32_t
UnicodeString::getLength() const {
    return length();
}

UChar
UnicodeString::getCharAt(int32_t offset) const {
  return charAt(offset);
}

UChar32
UnicodeString::getChar32At(int32_t offset) const {
  return char32At(offset);
}

int32_t
UnicodeString::countChar32(int32_t start, int32_t length) const {
  pinIndices(start, length);
  // if(isBogus()) then fArray==0 and start==0 - u_countChar32() checks for NULL
  return u_countChar32(getArrayStart()+start, length);
}

UBool
UnicodeString::hasMoreChar32Than(int32_t start, int32_t length, int32_t number) const {
  pinIndices(start, length);
  // if(isBogus()) then fArray==0 and start==0 - u_strHasMoreChar32Than() checks for NULL
  return u_strHasMoreChar32Than(getArrayStart()+start, length, number);
}

int32_t
UnicodeString::moveIndex32(int32_t index, int32_t delta) const {
  // pin index
  int32_t len = length();
  if(index<0) {
    index=0;
  } else if(index>len) {
    index=len;
  }

  const UChar *array = getArrayStart();
  if(delta>0) {
    UTF_FWD_N(array, index, len, delta);
  } else {
    UTF_BACK_N(array, 0, index, -delta);
  }

  return index;
}

void
UnicodeString::doExtract(int32_t start,
             int32_t length,
             UChar *dst,
             int32_t dstStart) const
{
  // pin indices to legal values
  pinIndices(start, length);

  // do not copy anything if we alias dst itself
  const UChar *array = getArrayStart();
  if(array + start != dst + dstStart) {
    us_arrayCopy(array, start, dst, dstStart, length);
  }
}

int32_t
UnicodeString::extract(UChar *dest, int32_t destCapacity,
                       UErrorCode &errorCode) const {
  int32_t len = length();
  if(U_SUCCESS(errorCode)) {
    if(isBogus() || destCapacity<0 || (destCapacity>0 && dest==0)) {
      errorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else {
      const UChar *array = getArrayStart();
      if(len>0 && len<=destCapacity && array!=dest) {
        uprv_memcpy(dest, array, len*U_SIZEOF_UCHAR);
      }
      return u_terminateUChars(dest, destCapacity, len, &errorCode);
    }
  }

  return len;
}

int32_t
UnicodeString::extract(int32_t start,
                       int32_t length,
                       char *target,
                       int32_t targetCapacity,
                       enum EInvariant) const
{
  // if the arguments are illegal, then do nothing
  if(targetCapacity < 0 || (targetCapacity > 0 && target == NULL)) {
    return 0;
  }

  // pin the indices to legal values
  pinIndices(start, length);

  if(length <= targetCapacity) {
    u_UCharsToChars(getArrayStart() + start, target, length);
  }
  UErrorCode status = U_ZERO_ERROR;
  return u_terminateChars(target, targetCapacity, length, &status);
}

UnicodeString
UnicodeString::tempSubString(int32_t start, int32_t len) const {
  pinIndices(start, len);
  const UChar *array = getBuffer();  // not getArrayStart() to check kIsBogus & kOpenGetBuffer
  if(array==NULL) {
    array=fUnion.fStackBuffer;  // anything not NULL because that would make an empty string
    len=-2;  // bogus result string
  }
  return UnicodeString(FALSE, array + start, len);
}

int32_t
UnicodeString::toUTF8(int32_t start, int32_t len,
                      char *target, int32_t capacity) const {
  pinIndices(start, len);
  int32_t length8;
  UErrorCode errorCode = U_ZERO_ERROR;
  u_strToUTF8WithSub(target, capacity, &length8,
                     getBuffer() + start, len,
                     0xFFFD,  // Standard substitution character.
                     NULL,    // Don't care about number of substitutions.
                     &errorCode);
  return length8;
}

#if U_CHARSET_IS_UTF8

int32_t
UnicodeString::extract(int32_t start, int32_t len,
                       char *target, uint32_t dstSize) const {
  // if the arguments are illegal, then do nothing
  if(/*dstSize < 0 || */(dstSize > 0 && target == 0)) {
    return 0;
  }
  return toUTF8(start, len, target, dstSize <= 0x7fffffff ? (int32_t)dstSize : 0x7fffffff);
}

// else see unistr_cnv.cpp
#endif

void 
UnicodeString::extractBetween(int32_t start,
                  int32_t limit,
                  UnicodeString& target) const {
  pinIndex(start);
  pinIndex(limit);
  doExtract(start, limit - start, target);
}

// When converting from UTF-16 to UTF-8, the result will have at most 3 times
// as many bytes as the source has UChars.
// The "worst cases" are writing systems like Indic, Thai and CJK with
// 3:1 bytes:UChars.
void
UnicodeString::toUTF8(ByteSink &sink) const {
  int32_t length16 = length();
  if(length16 != 0) {
    char stackBuffer[1024];
    int32_t capacity = (int32_t)sizeof(stackBuffer);
    UBool utf8IsOwned = FALSE;
    char *utf8 = sink.GetAppendBuffer(length16 < capacity ? length16 : capacity,
                                      3*length16,
                                      stackBuffer, capacity,
                                      &capacity);
    int32_t length8 = 0;
    UErrorCode errorCode = U_ZERO_ERROR;
    u_strToUTF8WithSub(utf8, capacity, &length8,
                       getBuffer(), length16,
                       0xFFFD,  // Standard substitution character.
                       NULL,    // Don't care about number of substitutions.
                       &errorCode);
    if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
      utf8 = (char *)uprv_malloc(length8);
      if(utf8 != NULL) {
        utf8IsOwned = TRUE;
        errorCode = U_ZERO_ERROR;
        u_strToUTF8WithSub(utf8, length8, &length8,
                           getBuffer(), length16,
                           0xFFFD,  // Standard substitution character.
                           NULL,    // Don't care about number of substitutions.
                           &errorCode);
      } else {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
      }
    }
    if(U_SUCCESS(errorCode)) {
      sink.Append(utf8, length8);
      sink.Flush();
    }
    if(utf8IsOwned) {
      uprv_free(utf8);
    }
  }
}

int32_t
UnicodeString::toUTF32(UChar32 *utf32, int32_t capacity, UErrorCode &errorCode) const {
  int32_t length32=0;
  if(U_SUCCESS(errorCode)) {
    // getBuffer() and u_strToUTF32WithSub() check for illegal arguments.
    u_strToUTF32WithSub(utf32, capacity, &length32,
        getBuffer(), length(),
        0xfffd,  // Substitution character.
        NULL,    // Don't care about number of substitutions.
        &errorCode);
  }
  return length32;
}

int32_t 
UnicodeString::indexOf(const UChar *srcChars,
               int32_t srcStart,
               int32_t srcLength,
               int32_t start,
               int32_t length) const
{
  if(isBogus() || srcChars == 0 || srcStart < 0 || srcLength == 0) {
    return -1;
  }

  // UnicodeString does not find empty substrings
  if(srcLength < 0 && srcChars[srcStart] == 0) {
    return -1;
  }

  // get the indices within bounds
  pinIndices(start, length);

  // find the first occurrence of the substring
  const UChar *array = getArrayStart();
  const UChar *match = u_strFindFirst(array + start, length, srcChars + srcStart, srcLength);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doIndexOf(UChar c,
             int32_t start,
             int32_t length) const
{
  // pin indices
  pinIndices(start, length);

  // find the first occurrence of c
  const UChar *array = getArrayStart();
  const UChar *match = u_memchr(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doIndexOf(UChar32 c,
                         int32_t start,
                         int32_t length) const {
  // pin indices
  pinIndices(start, length);

  // find the first occurrence of c
  const UChar *array = getArrayStart();
  const UChar *match = u_memchr32(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t 
UnicodeString::lastIndexOf(const UChar *srcChars,
               int32_t srcStart,
               int32_t srcLength,
               int32_t start,
               int32_t length) const
{
  if(isBogus() || srcChars == 0 || srcStart < 0 || srcLength == 0) {
    return -1;
  }

  // UnicodeString does not find empty substrings
  if(srcLength < 0 && srcChars[srcStart] == 0) {
    return -1;
  }

  // get the indices within bounds
  pinIndices(start, length);

  // find the last occurrence of the substring
  const UChar *array = getArrayStart();
  const UChar *match = u_strFindLast(array + start, length, srcChars + srcStart, srcLength);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doLastIndexOf(UChar c,
                 int32_t start,
                 int32_t length) const
{
  if(isBogus()) {
    return -1;
  }

  // pin indices
  pinIndices(start, length);

  // find the last occurrence of c
  const UChar *array = getArrayStart();
  const UChar *match = u_memrchr(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doLastIndexOf(UChar32 c,
                             int32_t start,
                             int32_t length) const {
  // pin indices
  pinIndices(start, length);

  // find the last occurrence of c
  const UChar *array = getArrayStart();
  const UChar *match = u_memrchr32(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

//========================================
// Write implementation
//========================================

UnicodeString& 
UnicodeString::findAndReplace(int32_t start,
                  int32_t length,
                  const UnicodeString& oldText,
                  int32_t oldStart,
                  int32_t oldLength,
                  const UnicodeString& newText,
                  int32_t newStart,
                  int32_t newLength)
{
  if(isBogus() || oldText.isBogus() || newText.isBogus()) {
    return *this;
  }

  pinIndices(start, length);
  oldText.pinIndices(oldStart, oldLength);
  newText.pinIndices(newStart, newLength);

  if(oldLength == 0) {
    return *this;
  }

  while(length > 0 && length >= oldLength) {
    int32_t pos = indexOf(oldText, oldStart, oldLength, start, length);
    if(pos < 0) {
      // no more oldText's here: done
      break;
    } else {
      // we found oldText, replace it by newText and go beyond it
      replace(pos, oldLength, newText, newStart, newLength);
      length -= pos + oldLength - start;
      start = pos + newLength;
    }
  }

  return *this;
}


void
UnicodeString::setToBogus()
{
  releaseArray();

  fShortLength = 0;
  fUnion.fFields.fArray = 0;
  fUnion.fFields.fCapacity = 0;
  fFlags = kIsBogus;
}

// turn a bogus string into an empty one
void
UnicodeString::unBogus() {
  if(fFlags & kIsBogus) {
    setToEmpty();
  }
}

// setTo() analogous to the readonly-aliasing constructor with the same signature
UnicodeString &
UnicodeString::setTo(UBool isTerminated,
                     const UChar *text,
                     int32_t textLength)
{
  if(fFlags & kOpenGetBuffer) {
    // do not modify a string that has an "open" getBuffer(minCapacity)
    return *this;
  }

  if(text == NULL) {
    // treat as an empty string, do not alias
    releaseArray();
    setToEmpty();
    return *this;
  }

  if( textLength < -1 ||
      (textLength == -1 && !isTerminated) ||
      (textLength >= 0 && isTerminated && text[textLength] != 0)
  ) {
    setToBogus();
    return *this;
  }

  releaseArray();

  if(textLength == -1) {
    // text is terminated, or else it would have failed the above test
    textLength = u_strlen(text);
  }
  setArray((UChar *)text, textLength, isTerminated ? textLength + 1 : textLength);

  fFlags = kReadonlyAlias;
  return *this;
}

// setTo() analogous to the writable-aliasing constructor with the same signature
UnicodeString &
UnicodeString::setTo(UChar *buffer,
                     int32_t buffLength,
                     int32_t buffCapacity) {
  if(fFlags & kOpenGetBuffer) {
    // do not modify a string that has an "open" getBuffer(minCapacity)
    return *this;
  }

  if(buffer == NULL) {
    // treat as an empty string, do not alias
    releaseArray();
    setToEmpty();
    return *this;
  }

  if(buffLength < -1 || buffCapacity < 0 || buffLength > buffCapacity) {
    setToBogus();
    return *this;
  } else if(buffLength == -1) {
    // buffLength = u_strlen(buff); but do not look beyond buffCapacity
    const UChar *p = buffer, *limit = buffer + buffCapacity;
    while(p != limit && *p != 0) {
      ++p;
    }
    buffLength = (int32_t)(p - buffer);
  }

  releaseArray();

  setArray(buffer, buffLength, buffCapacity);
  fFlags = kWritableAlias;
  return *this;
}

UnicodeString &UnicodeString::setToUTF8(const StringPiece &utf8) {
  unBogus();
  int32_t length = utf8.length();
  int32_t capacity;
  // The UTF-16 string will be at most as long as the UTF-8 string.
  if(length <= US_STACKBUF_SIZE) {
    capacity = US_STACKBUF_SIZE;
  } else {
    capacity = length + 1;  // +1 for the terminating NUL.
  }
  UChar *utf16 = getBuffer(capacity);
  int32_t length16;
  UErrorCode errorCode = U_ZERO_ERROR;
  u_strFromUTF8WithSub(utf16, getCapacity(), &length16,
      utf8.data(), length,
      0xfffd,  // Substitution character.
      NULL,    // Don't care about number of substitutions.
      &errorCode);
  releaseBuffer(length16);
  if(U_FAILURE(errorCode)) {
    setToBogus();
  }
  return *this;
}

UnicodeString&
UnicodeString::setCharAt(int32_t offset,
             UChar c)
{
  int32_t len = length();
  if(cloneArrayIfNeeded() && len > 0) {
    if(offset < 0) {
      offset = 0;
    } else if(offset >= len) {
      offset = len - 1;
    }

    getArrayStart()[offset] = c;
  }
  return *this;
}

UnicodeString&
UnicodeString::doReplace( int32_t start,
              int32_t length,
              const UnicodeString& src,
              int32_t srcStart,
              int32_t srcLength)
{
  if(!src.isBogus()) {
    // pin the indices to legal values
    src.pinIndices(srcStart, srcLength);

    // get the characters from src
    // and replace the range in ourselves with them
    return doReplace(start, length, src.getArrayStart(), srcStart, srcLength);
  } else {
    // remove the range
    return doReplace(start, length, 0, 0, 0);
  }
}

UnicodeString&
UnicodeString::doReplace(int32_t start,
             int32_t length,
             const UChar *srcChars,
             int32_t srcStart,
             int32_t srcLength)
{
  if(!isWritable()) {
    return *this;
  }

  int32_t oldLength = this->length();

  // optimize (read-only alias).remove(0, start) and .remove(start, end)
  if((fFlags&kBufferIsReadonly) && srcLength == 0) {
    if(start == 0) {
      // remove prefix by adjusting the array pointer
      pinIndex(length);
      fUnion.fFields.fArray += length;
      fUnion.fFields.fCapacity -= length;
      setLength(oldLength - length);
      return *this;
    } else {
      pinIndex(start);
      if(length >= (oldLength - start)) {
        // remove suffix by reducing the length (like truncate())
        setLength(start);
        fUnion.fFields.fCapacity = start;  // not NUL-terminated any more
        return *this;
      }
    }
  }

  if(srcChars == 0) {
    srcStart = srcLength = 0;
  } else if(srcLength < 0) {
    // get the srcLength if necessary
    srcLength = u_strlen(srcChars + srcStart);
  }

  // calculate the size of the string after the replace
  int32_t newSize;

  // optimize append() onto a large-enough, owned string
  if(start >= oldLength) {
    newSize = oldLength + srcLength;
    if(newSize <= getCapacity() && isBufferWritable()) {
      us_arrayCopy(srcChars, srcStart, getArrayStart(), oldLength, srcLength);
      setLength(newSize);
      return *this;
    } else {
      // pin the indices to legal values
      start = oldLength;
      length = 0;
    }
  } else {
    // pin the indices to legal values
    pinIndices(start, length);

    newSize = oldLength - length + srcLength;
  }

  // the following may change fArray but will not copy the current contents;
  // therefore we need to keep the current fArray
  UChar oldStackBuffer[US_STACKBUF_SIZE];
  UChar *oldArray;
  if((fFlags&kUsingStackBuffer) && (newSize > US_STACKBUF_SIZE)) {
    // copy the stack buffer contents because it will be overwritten with
    // fUnion.fFields values
    u_memcpy(oldStackBuffer, fUnion.fStackBuffer, oldLength);
    oldArray = oldStackBuffer;
  } else {
    oldArray = getArrayStart();
  }

  // clone our array and allocate a bigger array if needed
  int32_t *bufferToDelete = 0;
  if(!cloneArrayIfNeeded(newSize, newSize + (newSize >> 2) + kGrowSize,
                         FALSE, &bufferToDelete)
  ) {
    return *this;
  }

  // now do the replace

  UChar *newArray = getArrayStart();
  if(newArray != oldArray) {
    // if fArray changed, then we need to copy everything except what will change
    us_arrayCopy(oldArray, 0, newArray, 0, start);
    us_arrayCopy(oldArray, start + length,
                 newArray, start + srcLength,
                 oldLength - (start + length));
  } else if(length != srcLength) {
    // fArray did not change; copy only the portion that isn't changing, leaving a hole
    us_arrayCopy(oldArray, start + length,
                 newArray, start + srcLength,
                 oldLength - (start + length));
  }

  // now fill in the hole with the new string
  us_arrayCopy(srcChars, srcStart, newArray, start, srcLength);

  setLength(newSize);

  // delayed delete in case srcChars == fArray when we started, and
  // to keep oldArray alive for the above operations
  if (bufferToDelete) {
    uprv_free(bufferToDelete);
  }

  return *this;
}

/**
 * Replaceable API
 */
void
UnicodeString::handleReplaceBetween(int32_t start,
                                    int32_t limit,
                                    const UnicodeString& text) {
    replaceBetween(start, limit, text);
}

/**
 * Replaceable API
 */
void 
UnicodeString::copy(int32_t start, int32_t limit, int32_t dest) {
    if (limit <= start) {
        return; // Nothing to do; avoid bogus malloc call
    }
    UChar* text = (UChar*) uprv_malloc( sizeof(UChar) * (limit - start) );
    // Check to make sure text is not null.
    if (text != NULL) {
	    extractBetween(start, limit, text, 0);
	    insert(dest, text, 0, limit - start);    
	    uprv_free(text);
    }
}

/**
 * Replaceable API
 *
 * NOTE: This is for the Replaceable class.  There is no rep.cpp,
 * so we implement this function here.
 */
UBool Replaceable::hasMetaData() const {
    return TRUE;
}

/**
 * Replaceable API
 */
UBool UnicodeString::hasMetaData() const {
    return FALSE;
}

UnicodeString&
UnicodeString::doReverse(int32_t start, int32_t length) {
  if(length <= 1 || !cloneArrayIfNeeded()) {
    return *this;
  }

  // pin the indices to legal values
  pinIndices(start, length);
  if(length <= 1) {  // pinIndices() might have shrunk the length
    return *this;
  }

  UChar *left = getArrayStart() + start;
  UChar *right = left + length - 1;  // -1 for inclusive boundary (length>=2)
  UChar swap;
  UBool hasSupplementary = FALSE;

  // Before the loop we know left<right because length>=2.
  do {
    hasSupplementary |= (UBool)U16_IS_LEAD(swap = *left);
    hasSupplementary |= (UBool)U16_IS_LEAD(*left++ = *right);
    *right-- = swap;
  } while(left < right);
  // Make sure to test the middle code unit of an odd-length string.
  // Redundant if the length is even.
  hasSupplementary |= (UBool)U16_IS_LEAD(*left);

  /* if there are supplementary code points in the reversed range, then re-swap their surrogates */
  if(hasSupplementary) {
    UChar swap2;

    left = getArrayStart() + start;
    right = left + length - 1; // -1 so that we can look at *(left+1) if left<right
    while(left < right) {
      if(U16_IS_TRAIL(swap = *left) && U16_IS_LEAD(swap2 = *(left + 1))) {
        *left++ = swap2;
        *left++ = swap;
      } else {
        ++left;
      }
    }
  }

  return *this;
}

UBool 
UnicodeString::padLeading(int32_t targetLength,
                          UChar padChar)
{
  int32_t oldLength = length();
  if(oldLength >= targetLength || !cloneArrayIfNeeded(targetLength)) {
    return FALSE;
  } else {
    // move contents up by padding width
    UChar *array = getArrayStart();
    int32_t start = targetLength - oldLength;
    us_arrayCopy(array, 0, array, start, oldLength);

    // fill in padding character
    while(--start >= 0) {
      array[start] = padChar;
    }
    setLength(targetLength);
    return TRUE;
  }
}

UBool 
UnicodeString::padTrailing(int32_t targetLength,
                           UChar padChar)
{
  int32_t oldLength = length();
  if(oldLength >= targetLength || !cloneArrayIfNeeded(targetLength)) {
    return FALSE;
  } else {
    // fill in padding character
    UChar *array = getArrayStart();
    int32_t length = targetLength;
    while(--length >= oldLength) {
      array[length] = padChar;
    }
    setLength(targetLength);
    return TRUE;
  }
}

//========================================
// Hashing
//========================================
int32_t
UnicodeString::doHashCode() const
{
    /* Delegate hash computation to uhash.  This makes UnicodeString
     * hashing consistent with UChar* hashing.  */
    int32_t hashCode = uhash_hashUCharsN(getArrayStart(), length());
    if (hashCode == kInvalidHashCode) {
        hashCode = kEmptyHashCode;
    }
    return hashCode;
}

//========================================
// External Buffer
//========================================

UChar *
UnicodeString::getBuffer(int32_t minCapacity) {
  if(minCapacity>=-1 && cloneArrayIfNeeded(minCapacity)) {
    fFlags|=kOpenGetBuffer;
    fShortLength=0;
    return getArrayStart();
  } else {
    return 0;
  }
}

void
UnicodeString::releaseBuffer(int32_t newLength) {
  if(fFlags&kOpenGetBuffer && newLength>=-1) {
    // set the new fLength
    int32_t capacity=getCapacity();
    if(newLength==-1) {
      // the new length is the string length, capped by fCapacity
      const UChar *array=getArrayStart(), *p=array, *limit=array+capacity;
      while(p<limit && *p!=0) {
        ++p;
      }
      newLength=(int32_t)(p-array);
    } else if(newLength>capacity) {
      newLength=capacity;
    }
    setLength(newLength);
    fFlags&=~kOpenGetBuffer;
  }
}

//========================================
// Miscellaneous
//========================================
UBool
UnicodeString::cloneArrayIfNeeded(int32_t newCapacity,
                                  int32_t growCapacity,
                                  UBool doCopyArray,
                                  int32_t **pBufferToDelete,
                                  UBool forceClone) {
  // default parameters need to be static, therefore
  // the defaults are -1 to have convenience defaults
  if(newCapacity == -1) {
    newCapacity = getCapacity();
  }

  // while a getBuffer(minCapacity) is "open",
  // prevent any modifications of the string by returning FALSE here
  // if the string is bogus, then only an assignment or similar can revive it
  if(!isWritable()) {
    return FALSE;
  }

  /*
   * We need to make a copy of the array if
   * the buffer is read-only, or
   * the buffer is refCounted (shared), and refCount>1, or
   * the buffer is too small.
   * Return FALSE if memory could not be allocated.
   */
  if(forceClone ||
     fFlags & kBufferIsReadonly ||
     (fFlags & kRefCounted && refCount() > 1) ||
     newCapacity > getCapacity()
  ) {
    // check growCapacity for default value and use of the stack buffer
    if(growCapacity == -1) {
      growCapacity = newCapacity;
    } else if(newCapacity <= US_STACKBUF_SIZE && growCapacity > US_STACKBUF_SIZE) {
      growCapacity = US_STACKBUF_SIZE;
    }

    // save old values
    UChar oldStackBuffer[US_STACKBUF_SIZE];
    UChar *oldArray;
    uint8_t flags = fFlags;

    if(flags&kUsingStackBuffer) {
      if(doCopyArray && growCapacity > US_STACKBUF_SIZE) {
        // copy the stack buffer contents because it will be overwritten with
        // fUnion.fFields values
        us_arrayCopy(fUnion.fStackBuffer, 0, oldStackBuffer, 0, fShortLength);
        oldArray = oldStackBuffer;
      } else {
        oldArray = 0; // no need to copy from stack buffer to itself
      }
    } else {
      oldArray = fUnion.fFields.fArray;
    }

    // allocate a new array
    if(allocate(growCapacity) ||
       (newCapacity < growCapacity && allocate(newCapacity))
    ) {
      if(doCopyArray && oldArray != 0) {
        // copy the contents
        // do not copy more than what fits - it may be smaller than before
        int32_t minLength = length();
        newCapacity = getCapacity();
        if(newCapacity < minLength) {
          minLength = newCapacity;
          setLength(minLength);
        }
        us_arrayCopy(oldArray, 0, getArrayStart(), 0, minLength);
      } else {
        fShortLength = 0;
      }

      // release the old array
      if(flags & kRefCounted) {
        // the array is refCounted; decrement and release if 0
        int32_t *pRefCount = ((int32_t *)oldArray - 1);
        if(umtx_atomic_dec(pRefCount) == 0) {
          if(pBufferToDelete == 0) {
            uprv_free(pRefCount);
          } else {
            // the caller requested to delete it himself
            *pBufferToDelete = pRefCount;
          }
        }
      }
    } else {
      // not enough memory for growCapacity and not even for the smaller newCapacity
      // reset the old values for setToBogus() to release the array
      if(!(flags&kUsingStackBuffer)) {
        fUnion.fFields.fArray = oldArray;
      }
      fFlags = flags;
      setToBogus();
      return FALSE;
    }
  }
  return TRUE;
}
U_NAMESPACE_END

#ifdef U_STATIC_IMPLEMENTATION
/*
This should never be called. It is defined here to make sure that the
virtual vector deleting destructor is defined within unistr.cpp.
The vector deleting destructor is already a part of UObject,
but defining it here makes sure that it is included with this object file.
This makes sure that static library dependencies are kept to a minimum.
*/
static void uprv_UnicodeStringDummy(void) {
    U_NAMESPACE_USE
    delete [] (new UnicodeString[2]);
}
#endif
