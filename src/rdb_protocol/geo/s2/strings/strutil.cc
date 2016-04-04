//
// Copyright (C) 1999-2005 Google, Inc.
//

#include "rdb_protocol/geo/s2/strings/strutil.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>          // for DBL_DIG and FLT_DIG
#include <math.h>           // for HUGE_VAL
#include <pthread.h>        // for gmtime_r (on Windows)
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <limits>
#include <set>
#include <string>
#include <vector>


#include "errors.hpp"
#include "rdb_protocol/geo/s2/base/logging.h"
#include "rdb_protocol/geo/s2/base/scoped_ptr.h"
//#include "rdb_protocol/geo/s2/strutil-inl.h"
//#include "rdb_protocol/geo/s2/third_party/utf/utf.h"  // for runetochar
//#include "rdb_protocol/geo/s2/util/gtl/stl_util-inl.h"  // for string_as_array
//#include "rdb_protocol/geo/s2/util/hash/hash.h"
#include "rdb_protocol/geo/s2/strings/split.h"

#ifdef OS_WINDOWS
#include <pthread.h>        // for gmtime_r
#ifdef min  // windows.h defines this to something silly
#undef min
#endif
#endif

namespace geo {
using std::min;
using std::max;
using std::swap;
using std::reverse;
using std::unordered_map;
using std::unordered_set;
using std::numeric_limits;
using std::set;
using std::multiset;
using std::vector;


// ----------------------------------------------------------------------
// FpToString()
// FloatToString()
// IntToString()
//    Convert various types to their string representation.  These
//    all do the obvious, trivial thing.
// ----------------------------------------------------------------------

std::string FpToString(Fprint fp) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%016llx", fp);
  return std::string(buf);
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
std::string FloatToString(float f, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, f);
  return std::string(buf);
}

std::string IntToString(int i, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, i);
  return std::string(buf);
}

std::string Int64ToString(int64 i64, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, i64);
  return std::string(buf);
}

std::string UInt64ToString(uint64 ui64, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, ui64);
  return std::string(buf);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// Default arguments
std::string FloatToString(float f)   { return FloatToString(f, "%7f"); }
std::string IntToString(int i)       { return IntToString(i, "%7d"); }
std::string Int64ToString(int64 i64) {
  return Int64ToString(i64, "%7" GG_LL_FORMAT "d");
}
std::string UInt64ToString(uint64 ui64) {
  return UInt64ToString(ui64, "%7" GG_LL_FORMAT "u");
}

// ----------------------------------------------------------------------
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
//    These are intended for speed.  FastHexToBuffer() assumes the
//    integer is non-negative.  FastHexToBuffer() puts output in
//    hex rather than decimal.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//       All functions take the output buffer as an arg.  FastInt()
//    uses at most 22 bytes.
//    They all return a pointer to the beginning of the output,
//    which may not be the beginning of the input buffer.
// ----------------------------------------------------------------------

char *FastHexToBuffer(int i, char* buffer) {
  CHECK(i >= 0) << "FastHexToBuffer() wants non-negative integers, not " << i;

  static const char *hexdigits = "0123456789abcdef";
  char *p = buffer + 21;
  *p-- = '\0';
  do {
    *p-- = hexdigits[i & 15];   // mod by 16
    i >>= 4;                    // divide by 16
  } while (i > 0);
  return p + 1;
}

char *InternalFastHexToBuffer(uint64 value, char* buffer, int num_byte) {
  static const char *hexdigits = "0123456789abcdef";
  buffer[num_byte] = '\0';
  for (int i = num_byte - 1; i >= 0; i--) {
    buffer[i] = hexdigits[uint32(value) & 0xf];
    value >>= 4;
  }
  return buffer;
}

char *FastHex64ToBuffer(uint64 value, char* buffer) {
  return InternalFastHexToBuffer(value, buffer, 16);
}

char *FastHex32ToBuffer(uint32 value, char* buffer) {
  return InternalFastHexToBuffer(value, buffer, 8);
}

// ----------------------------------------------------------------------
// ParseLeadingUInt64Value
// ParseLeadingInt64Value
// ParseLeadingHex64Value
//    A simple parser for long long values. Returns the parsed value if a
//    valid integer is found; else returns deflt
//    UInt64 and Int64 cannot handle decimal numbers with leading 0s.
// --------------------------------------------------------------------
uint64 ParseLeadingUInt64Value(const char *str, uint64 deflt) {
  char *error = NULL;
  const uint64 value = strtoull(str, &error, 0);
  return (error == str) ? deflt : value;
}

int64 ParseLeadingInt64Value(const char *str, int64 deflt) {
  char *error = NULL;
  const int64 value = strtoll(str, &error, 0);
  return (error == str) ? deflt : value;
}

uint64 ParseLeadingHex64Value(const char *str, uint64 deflt) {
  char *error = NULL;
  const uint64 value = strtoull(str, &error, 16);
  return (error == str) ? deflt : value;
}

// ----------------------------------------------------------------------
// ParseLeadingDec64Value
// ParseLeadingUDec64Value
//    A simple parser for [u]int64 values. Returns the parsed value
//    if a valid value is found; else returns deflt
//    The string passed in is treated as *10 based*.
//    This can handle strings with leading 0s.
// --------------------------------------------------------------------

int64 ParseLeadingDec64Value(const char *str, int64 deflt) {
  char *error = NULL;
  const int64 value = strtoll(str, &error, 10);
  return (error == str) ? deflt : value;
}

uint64 ParseLeadingUDec64Value(const char *str, uint64 deflt) {
  char *error = NULL;
  const uint64 value = strtoull(str, &error, 10);
  return (error == str) ? deflt : value;
}

bool DictionaryParse(const std::string& encoded_str,
                      vector<pair<std::string, std::string> >* items) {
  vector<std::string> entries;
  SplitStringUsing(encoded_str, ",", &entries);
  for (size_t i = 0; i < entries.size(); ++i) {
    vector<std::string> fields;
    SplitStringAllowEmpty(entries[i], ":", &fields);
    if (fields.size() != 2) // parsing error
      return false;
    items->push_back(make_pair(fields[0], fields[1]));
  }
  return true;
}

}  // namespace geo
