//
// Copyright (C) 1999-2005 Google, Inc.
//

#include "geo/s2/strings/strutil.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>          // for DBL_DIG and FLT_DIG
#include <math.h>           // for HUGE_VAL
#include <pthread.h>        // for gmtime_r (on Windows)
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>           // for FastTimeToBuffer()

#include <algorithm>
using std::min;
using std::max;
using std::swap;
using std::reverse;

#include <unordered_map>
using std::unordered_map;

#include <unordered_set>
using std::unordered_set;

#include <iterator>
#include <limits>
using std::numeric_limits;

#include <set>
using std::set;
using std::multiset;

#include <string>

#include <vector>
using std::vector;


#include "errors.hpp"
#include "geo/s2/base/logging.h"
#include "geo/s2/base/scoped_ptr.h"
//#include "geo/s2/strutil-inl.h"
//#include "geo/s2/third_party/utf/utf.h"  // for runetochar
//#include "geo/s2/util/gtl/stl_util-inl.h"  // for string_as_array
//#include "geo/s2/util/hash/hash.h"
#include "geo/s2/strings/split.h"

#ifdef OS_WINDOWS
#include <pthread.h>        // for gmtime_r
#ifdef min  // windows.h defines this to something silly
#undef min
#endif
#endif

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
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
#pragma GCC diagnostic pop

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
// FastIntToBuffer()
// FastInt64ToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// FastTimeToBuffer()
//    These are intended for speed.  FastHexToBuffer() assumes the
//    integer is non-negative.  FastHexToBuffer() puts output in
//    hex rather than decimal.  FastTimeToBuffer() puts the output
//    into RFC822 format.  If time is 0, uses the current time.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//       All functions take the output buffer as an arg.  FastInt()
//    uses at most 22 bytes, FastTime() uses exactly 30 bytes.
//    They all return a pointer to the beginning of the output,
//    which may not be the beginning of the input buffer.  (Though
//    for FastTimeToBuffer(), we guarantee that it is.)
// ----------------------------------------------------------------------

char *FastInt64ToBuffer(int64 i, char* buffer) {
  FastInt64ToBufferLeft(i, buffer);
  return buffer;
}

// Offset into buffer where FastInt32ToBuffer places the end of string
// null character.  Also used by FastInt32ToBufferLeft
static const int kFastInt32ToBufferOffset = 11;

char *FastInt32ToBuffer(int32 i, char* buffer) {
  FastInt32ToBufferLeft(i, buffer);
  return buffer;
}

// These are not the original implementations. The original ones can be found at
// http://protobuf-mercury.googlecode.com/svn-history/r49/trunk/src/google/protobuf/compiler/mercury/mercury_strutil.cc
// under an unclear license, but are missing in the S2 repository.
// These ones are slower.
// TODO! And need testing
char* FastInt32ToBufferLeft(int32 i, char* buffer) {
    return FastInt64ToBufferLeft(i, buffer);
}
char* FastInt64ToBufferLeft(int64 i, char* buffer) {
  std::string s = Int64ToString(i);
  guarantee(s.length() + 1 < kFastToBufferSize);
  memcpy(buffer, s.c_str(), s.length() + 1);
  return buffer + s.length();
}

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

// Several converters use this table to reduce
// division and modulo operations.
static const char two_ASCII_digits[100][2] = {
  {'0','0'}, {'0','1'}, {'0','2'}, {'0','3'}, {'0','4'},
  {'0','5'}, {'0','6'}, {'0','7'}, {'0','8'}, {'0','9'},
  {'1','0'}, {'1','1'}, {'1','2'}, {'1','3'}, {'1','4'},
  {'1','5'}, {'1','6'}, {'1','7'}, {'1','8'}, {'1','9'},
  {'2','0'}, {'2','1'}, {'2','2'}, {'2','3'}, {'2','4'},
  {'2','5'}, {'2','6'}, {'2','7'}, {'2','8'}, {'2','9'},
  {'3','0'}, {'3','1'}, {'3','2'}, {'3','3'}, {'3','4'},
  {'3','5'}, {'3','6'}, {'3','7'}, {'3','8'}, {'3','9'},
  {'4','0'}, {'4','1'}, {'4','2'}, {'4','3'}, {'4','4'},
  {'4','5'}, {'4','6'}, {'4','7'}, {'4','8'}, {'4','9'},
  {'5','0'}, {'5','1'}, {'5','2'}, {'5','3'}, {'5','4'},
  {'5','5'}, {'5','6'}, {'5','7'}, {'5','8'}, {'5','9'},
  {'6','0'}, {'6','1'}, {'6','2'}, {'6','3'}, {'6','4'},
  {'6','5'}, {'6','6'}, {'6','7'}, {'6','8'}, {'6','9'},
  {'7','0'}, {'7','1'}, {'7','2'}, {'7','3'}, {'7','4'},
  {'7','5'}, {'7','6'}, {'7','7'}, {'7','8'}, {'7','9'},
  {'8','0'}, {'8','1'}, {'8','2'}, {'8','3'}, {'8','4'},
  {'8','5'}, {'8','6'}, {'8','7'}, {'8','8'}, {'8','9'},
  {'9','0'}, {'9','1'}, {'9','2'}, {'9','3'}, {'9','4'},
  {'9','5'}, {'9','6'}, {'9','7'}, {'9','8'}, {'9','9'}
};

static inline void PutTwoDigits(int i, char* p) {
  DCHECK_GE(i, 0);
  DCHECK_LT(i, 100);
  p[0] = two_ASCII_digits[i][0];
  p[1] = two_ASCII_digits[i][1];
}

char* FastTimeToBuffer(time_t s, char* buffer) {
  if (s == 0) {
    time(&s);
  }

  struct tm tm;
  if (gmtime_r(&s, &tm) == NULL) {
    // Error message must fit in 30-char buffer.
    memcpy(buffer, "Invalid:", sizeof("Invalid:"));
    FastInt64ToBufferLeft(s, buffer+strlen(buffer));
    return buffer;
  }

  // strftime format: "%a, %d %b %Y %H:%M:%S GMT",
  // but strftime does locale stuff which we do not want
  // plus strftime takes > 10x the time of hard code

  const char* weekday_name = "Xxx";
  switch (tm.tm_wday) {
    default: { DCHECK(false); } break;
    case 0:  weekday_name = "Sun"; break;
    case 1:  weekday_name = "Mon"; break;
    case 2:  weekday_name = "Tue"; break;
    case 3:  weekday_name = "Wed"; break;
    case 4:  weekday_name = "Thu"; break;
    case 5:  weekday_name = "Fri"; break;
    case 6:  weekday_name = "Sat"; break;
  }

  const char* month_name = "Xxx";
  switch (tm.tm_mon) {
    default:  { DCHECK(false); } break;
    case 0:   month_name = "Jan"; break;
    case 1:   month_name = "Feb"; break;
    case 2:   month_name = "Mar"; break;
    case 3:   month_name = "Apr"; break;
    case 4:   month_name = "May"; break;
    case 5:   month_name = "Jun"; break;
    case 6:   month_name = "Jul"; break;
    case 7:   month_name = "Aug"; break;
    case 8:   month_name = "Sep"; break;
    case 9:   month_name = "Oct"; break;
    case 10:  month_name = "Nov"; break;
    case 11:  month_name = "Dec"; break;
  }

  // Write out the buffer.

  memcpy(buffer+0, weekday_name, 3);
  buffer[3] = ',';
  buffer[4] = ' ';

  PutTwoDigits(tm.tm_mday, buffer+5);
  buffer[7] = ' ';

  memcpy(buffer+8, month_name, 3);
  buffer[11] = ' ';

  int32 year = tm.tm_year + 1900;
  PutTwoDigits(year/100, buffer+12);
  PutTwoDigits(year%100, buffer+14);
  buffer[16] = ' ';

  PutTwoDigits(tm.tm_hour, buffer+17);
  buffer[19] = ':';

  PutTwoDigits(tm.tm_min, buffer+20);
  buffer[22] = ':';

  PutTwoDigits(tm.tm_sec, buffer+23);

  // includes ending NUL
  memcpy(buffer+25, " GMT", 5);

  return buffer;
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
