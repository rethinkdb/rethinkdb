//
// Copyright 1999-2006 and onwards Google, Inc.
//
// Useful string functions and so forth.  This is a grab-bag file.
//
// You might also want to look at memutil.h, which holds mem*()
// equivalents of a lot of the str*() functions in string.h,
// eg memstr, mempbrk, etc.
//
// If you need to process UTF8 strings, take a look at files in i18n/utf8.

#ifndef STRINGS_STRUTIL_H_
#define STRINGS_STRUTIL_H_

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <string.h>
#include <stdlib.h>

// for strcasecmp (check SuSv3 -- this is the only header it's in!)
// MSVC doesn't have <strings.h>. Luckily, it defines equivalent
// functions (see port.h)
#ifndef COMPILER_MSVC
#include <strings.h>
#endif
#include <ctype.h>      // not needed, but removing it will break the build

#include "rdb_protocol/geo/s2/base/basictypes.h"
#include "rdb_protocol/geo/s2/base/logging.h"  // for CHECK
#include "rdb_protocol/geo/s2/base/strtoint.h"
#include "rdb_protocol/geo/s2/base/int128.h"
#include "rdb_protocol/geo/s2/strings/ascii_ctype.h"
//#include "rdb_protocol/geo/s2/charset.h"
//#include "rdb_protocol/geo/s2/escaping.h"
//#include "rdb_protocol/geo/s2/host_port.h"
#include "rdb_protocol/geo/s2/strings/stringprintf.h"
#include "rdb_protocol/geo/s2/base/stl_decl.h"
#include "rdb_protocol/geo/s2/base/port.h"
#include "rdb_protocol/geo/s2/util/endian/endian.h"

namespace geo {
using std::less;
using std::unordered_map;
using std::unordered_set;
using std::set;
using std::multiset;
using std::pair;
using std::make_pair;
using std::vector;

// A buffer size which is large enough for all the FastToBuffer functions, as
// well as DoubleToBuffer and FloatToBuffer.  We define this here in case other
// string headers depend on it.
static const unsigned int kFastToBufferSize = 32;

// ----------------------------------------------------------------------
// FpToString()
// FloatToString()
// IntToString()
// Int64ToString()
// UInt64ToString()
//    Convert various types to their string representation, possibly padded
//    with spaces, using snprintf format specifiers.
//    "Fp" here stands for fingerprint: a 64-bit entity
//    represented in 16 hex digits.
// ----------------------------------------------------------------------

std::string FpToString(Fprint fp);
std::string FloatToString(float f, const char* format);
std::string IntToString(int i, const char* format);
std::string Int64ToString(int64 i64, const char* format);
std::string UInt64ToString(uint64 ui64, const char* format);

// The default formats are %7f, %7d, and %7u respectively
std::string FloatToString(float f);
std::string IntToString(int i);
std::string Int64ToString(int64 i64);
std::string UInt64ToString(uint64 ui64);

// ----------------------------------------------------------------------
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
//    These are intended for speed.  FastHexToBuffer() puts output in
//    hex rather than decimal.
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
//    which may not be the beginning of the input buffer.
// ----------------------------------------------------------------------

// Previously documented minimums -- the buffers provided must be at least this
// long, though these numbers are subject to change:
//     Hex:                  22 bytes
//     Hex32:                 9 bytes
//     Hex64:                17 bytes
// Use kFastToBufferSize rather than hardcoding constants.

char* FastHexToBuffer(int i, char* buffer);
char* FastHex64ToBuffer(uint64 i, char* buffer);
char* FastHex32ToBuffer(uint32 i, char* buffer);

// ----------------------------------------------------------------------
// ConsumeStrayLeadingZeroes
//    Eliminates all leading zeroes (unless the string itself is composed
//    of nothing but zeroes, in which case one is kept: 0...0 becomes 0).
void ConsumeStrayLeadingZeroes(std::string* str);

// ----------------------------------------------------------------------
// ParseLeadingInt32Value
//    A simple parser for int32 values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    This cannot handle decimal numbers with leading 0s, since they will be
//    treated as octal.  If you know it's decimal, use ParseLeadingDec32Value.
// --------------------------------------------------------------------
int32 ParseLeadingInt32Value(const char* str, int32 deflt);
inline int32 ParseLeadingInt32Value(const std::string& str, int32 deflt) {
  return ParseLeadingInt32Value(str.c_str(), deflt);
}

// ParseLeadingUInt32Value
//    A simple parser for uint32 values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    This cannot handle decimal numbers with leading 0s, since they will be
//    treated as octal.  If you know it's decimal, use ParseLeadingUDec32Value.
// --------------------------------------------------------------------
uint32 ParseLeadingUInt32Value(const char* str, uint32 deflt);
inline uint32 ParseLeadingUInt32Value(const std::string& str, uint32 deflt) {
  return ParseLeadingUInt32Value(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingDec32Value
//    A simple parser for decimal int32 values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    The string passed in is treated as *10 based*.
//    This can handle strings with leading 0s.
//    See also: ParseLeadingDec64Value
// --------------------------------------------------------------------
int32 ParseLeadingDec32Value(const char* str, int32 deflt);
inline int32 ParseLeadingDec32Value(const std::string& str, int32 deflt) {
  return ParseLeadingDec32Value(str.c_str(), deflt);
}

// ParseLeadingUDec32Value
//    A simple parser for decimal uint32 values. Returns the parsed value
//    if a valid integer is found; else returns deflt. It does not
//    check if str is entirely consumed.
//    The string passed in is treated as *10 based*.
//    This can handle strings with leading 0s.
//    See also: ParseLeadingUDec64Value
// --------------------------------------------------------------------
uint32 ParseLeadingUDec32Value(const char* str, uint32 deflt);
inline uint32 ParseLeadingUDec32Value(const std::string& str, uint32 deflt) {
  return ParseLeadingUDec32Value(str.c_str(), deflt);
}

// ----------------------------------------------------------------------
// ParseLeadingUInt64Value
// ParseLeadingInt64Value
// ParseLeadingHex64Value
// ParseLeadingDec64Value
// ParseLeadingUDec64Value
//    A simple parser for long long values.
//    Returns the parsed value if a
//    valid integer is found; else returns deflt
// --------------------------------------------------------------------
uint64 ParseLeadingUInt64Value(const char* str, uint64 deflt);
inline uint64 ParseLeadingUInt64Value(const std::string& str, uint64 deflt) {
  return ParseLeadingUInt64Value(str.c_str(), deflt);
}
int64 ParseLeadingInt64Value(const char* str, int64 deflt);
inline int64 ParseLeadingInt64Value(const std::string& str, int64 deflt) {
  return ParseLeadingInt64Value(str.c_str(), deflt);
}
uint64 ParseLeadingHex64Value(const char* str, uint64 deflt);
inline uint64 ParseLeadingHex64Value(const std::string& str, uint64 deflt) {
  return ParseLeadingHex64Value(str.c_str(), deflt);
}
int64 ParseLeadingDec64Value(const char* str, int64 deflt);
inline int64 ParseLeadingDec64Value(const std::string& str, int64 deflt) {
  return ParseLeadingDec64Value(str.c_str(), deflt);
}
uint64 ParseLeadingUDec64Value(const char* str, uint64 deflt);
inline uint64 ParseLeadingUDec64Value(const std::string& str, uint64 deflt) {
  return ParseLeadingUDec64Value(str.c_str(), deflt);
}

// -------------------------------------------------------------------------
// DictionaryParse
//   This routine parses a common dictionary format (key and value separated
//   by ':', entries separated by commas). This format is used for many
//   complex commandline flags. It is also used to encode dictionaries for
//   exporting them or writing them to a checkpoint. Returns a vector of
//   <key, value> pairs. Returns true if there if no error in parsing, false
//    otherwise.
// -------------------------------------------------------------------------
bool DictionaryParse(const std::string& encoded_str,
                      vector<pair<std::string, std::string> >* items);

}  // namespace geo

#endif   /* #ifndef STRINGS_STRUTIL_H_ */
