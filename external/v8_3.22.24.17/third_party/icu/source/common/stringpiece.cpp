// Copyright (C) 2009-2010, International Business Machines
// Corporation and others. All Rights Reserved.
//
// Copyright 2004 and onwards Google Inc.
//
// Author: wilsonh@google.com (Wilson Hsieh)
//

#include "unicode/utypes.h"
#include "unicode/stringpiece.h"
#include "cstring.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

StringPiece::StringPiece(const char* str)
    : ptr_(str), length_((str == NULL) ? 0 : static_cast<int32_t>(uprv_strlen(str))) { }

StringPiece::StringPiece(const StringPiece& x, int32_t pos) {
  if (pos < 0) {
    pos = 0;
  } else if (pos > x.length_) {
    pos = x.length_;
  }
  ptr_ = x.ptr_ + pos;
  length_ = x.length_ - pos;
}

StringPiece::StringPiece(const StringPiece& x, int32_t pos, int32_t len) {
  if (pos < 0) {
    pos = 0;
  } else if (pos > x.length_) {
    pos = x.length_;
  }
  if (len < 0) {
    len = 0;
  } else if (len > x.length_ - pos) {
    len = x.length_ - pos;
  }
  ptr_ = x.ptr_ + pos;
  length_ = len;
}

void StringPiece::set(const char* str) {
  ptr_ = str;
  if (str != NULL)
    length_ = static_cast<int32_t>(uprv_strlen(str));
  else
    length_ = 0;
}

U_EXPORT UBool U_EXPORT2
operator==(const StringPiece& x, const StringPiece& y) {
  int32_t len = x.size();
  if (len != y.size()) {
    return false;
  }
  if (len == 0) {
    return true;
  }
  const char* p = x.data();
  const char* p2 = y.data();
  // Test last byte in case strings share large common prefix
  --len;
  if (p[len] != p2[len]) return false;
  // At this point we can, but don't have to, ignore the last byte.
  return uprv_memcmp(p, p2, len) == 0;
}


/* Microsft Visual Studios <= 8.0 complains about redefinition of this
 * static const class variable. However, the C++ standard states that this 
 * definition is correct. Perhaps there is a bug in the Microsoft compiler. 
 * This is not an issue on any other compilers (that we know of) including 
 * Visual Studios 9.0.
 * Cygwin with MSVC 9.0 also complains here about redefinition.
 */
#if (!defined(_MSC_VER) || (_MSC_VER > 1700)) && !defined(CYGWINMSVC)
const int32_t StringPiece::npos;
#endif

U_NAMESPACE_END
