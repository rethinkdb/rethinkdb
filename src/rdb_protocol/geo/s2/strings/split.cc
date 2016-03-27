// Copyright 2008 and onwards Google Inc.  All rights reserved.

#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <string>
#include <iterator>

#include "rdb_protocol/geo/s2/base/integral_types.h"
#include "rdb_protocol/geo/s2/base/logging.h"
#include "rdb_protocol/geo/s2/base/macros.h"
#include "rdb_protocol/geo/s2/base/strtoint.h"
#include "rdb_protocol/geo/s2/strings/split.h"
#include "rdb_protocol/geo/s2/strings/strutil.h"
#include "rdb_protocol/geo/s2/util/hash/hash_jenkins_lookup2.h"
#include "utils.hpp"

namespace geo {
using std::numeric_limits;

namespace {
// NOTE(user): we have to implement our own interator because
// insert_iterator<set<string> > does not instantiate without
// errors, perhaps since string != std::string.
// This is not a fully functional iterator, but is
// sufficient for SplitStringToIteratorUsing().
template <typename T>
struct simple_insert_iterator {
  T* t_;
  simple_insert_iterator(T* t) : t_(t) { }

  simple_insert_iterator<T>& operator=(const typename T::value_type& value) {
    t_->insert(value);
    return *this;
  }

  simple_insert_iterator<T>& operator*()     { return *this; }
  simple_insert_iterator<T>& operator++()    { return *this; }
  simple_insert_iterator<T>& operator++(int) { return *this; }
};

// Used to populate a hash_map out of pairs of consecutive strings in
// SplitStringToIterator{Using|AllowEmpty}().
template <typename T>
struct simple_hash_map_iterator {
  typedef unordered_map<T, T> hashmap;
  hashmap* t;
  bool even;
  typename hashmap::iterator curr;

  simple_hash_map_iterator(hashmap* t_init) : t(t_init), even(true) {
    curr = t->begin();
  }

  simple_hash_map_iterator<T>& operator=(const T& value) {
    if (even) {
      curr = t->insert(make_pair(value, T())).first;
    } else {
      curr->second = value;
    }
    even = !even;
    return *this;
  }

  simple_hash_map_iterator<T>& operator*()       { return *this; }
  simple_hash_map_iterator<T>& operator++()      { return *this; }
  simple_hash_map_iterator<T>& operator++(UNUSED int i) { return *this; }
};

}  // anonymous namespace

// ----------------------------------------------------------------------
// SplitStringIntoNPiecesAllowEmpty()
// SplitStringToIteratorAllowEmpty()
//    Split a string using a character delimiter. Append the components
//    to 'result'.  If there are consecutive delimiters, this function
//    will return corresponding empty strings. The string is split into
//    at most the specified number of pieces greedily. This means that the
//    last piece may possibly be split further. To split into as many pieces
//    as possible, specify 0 as the number of pieces.
//
//    If "full" is the empty string, yields an empty string as the only value.
//
//    If "pieces" is negative for some reason, it returns the whole string
// ----------------------------------------------------------------------
template <typename StringType, typename ITR>
static inline
void SplitStringToIteratorAllowEmpty(const StringType& full,
                                     const char* delim,
                                     int pieces,
                                     ITR& result) {
  std::string::size_type begin_index, end_index;
  begin_index = 0;

  for (int i=0; (i < pieces-1) || (pieces == 0); i++) {
    end_index = full.find_first_of(delim, begin_index);
    if (end_index == std::string::npos) {
      *result++ = full.substr(begin_index);
      return;
    }
    *result++ = full.substr(begin_index, (end_index - begin_index));
    begin_index = end_index + 1;
  }
  *result++ = full.substr(begin_index);
}

void SplitStringIntoNPiecesAllowEmpty(const std::string& full,
                                      const char* delim,
                                      int pieces,
                                      vector<std::string>* result) {
  std::back_insert_iterator<std::vector<std::string> > it(*result);
  SplitStringToIteratorAllowEmpty(full, delim, pieces, it);
}

// ----------------------------------------------------------------------
// SplitStringAllowEmpty
// SplitStringToHashsetAllowEmpty
// SplitStringToSetAllowEmpty
// SplitStringToHashmapAllowEmpty
//    Split a string using a character delimiter. Append the components
//    to 'result'.  If there are consecutive delimiters, this function
//    will return corresponding empty strings.
// ----------------------------------------------------------------------
void SplitStringAllowEmpty(const std::string& full, const char* delim,
                           vector<std::string>* result) {
  std::back_insert_iterator<std::vector<std::string> > it((*result));
  SplitStringToIteratorAllowEmpty(full, delim, 0, it);
}

void SplitStringToHashsetAllowEmpty(const std::string& full, const char* delim,
                                    unordered_set<std::string>* result) {
  simple_insert_iterator<unordered_set<std::string> > it((result));
  SplitStringToIteratorAllowEmpty(full, delim, 0, it);
}

void SplitStringToSetAllowEmpty(const std::string& full, const char* delim,
                                set<std::string>* result) {
  simple_insert_iterator<set<std::string> > it((result));
  SplitStringToIteratorAllowEmpty(full, delim, 0, it);
}

void SplitStringToHashmapAllowEmpty(const std::string& full, const char* delim,
                                    unordered_map<std::string, std::string>* result) {
  simple_hash_map_iterator<std::string> it((result));
  SplitStringToIteratorAllowEmpty(full, delim, 0, it);
}

// If we know how much to allocate for a vector of strings, we can
// allocate the vector<string> only once and directly to the right size.
// This saves in between 33-66 % of memory space needed for the result,
// and runs faster in the microbenchmarks.
//
// The reserve is only implemented for the single character delim.
//
// The implementation for counting is cut-and-pasted from
// SplitStringToIteratorUsing. I could have written my own counting iterator,
// and use the existing template function, but probably this is more clear
// and more sure to get optimized to reasonable code.
static int CalculateReserveForVector(const std::string& full, const char* delim) {
  int count = 0;
  if (delim[0] != '\0' && delim[1] == '\0') {
    // Optimize the common case where delim is a single character.
    char c = delim[0];
    const char* p = full.data();
    const char* end = p + full.size();
    while (p != end) {
      if (*p == c) {  // This could be optimized with hasless(v,1) trick.
        ++p;
      } else {
        while (++p != end && *p != c) {
          // Skip to the next occurence of the delimiter.
        }
        ++count;
      }
    }
  }
  return count;
}

// ----------------------------------------------------------------------
// SplitStringUsing()
// SplitStringToHashsetUsing()
// SplitStringToSetUsing()
// SplitStringToHashmapUsing()
//    Split a string using a character delimiter. Append the components
//    to 'result'.
//
// Note: For multi-character delimiters, this routine will split on *ANY* of
// the characters in the string, not the entire string as a single delimiter.
// ----------------------------------------------------------------------
template <typename StringType, typename ITR>
static inline
void SplitStringToIteratorUsing(const StringType& full,
                                const char* delim,
                                ITR& result) {
  // Optimize the common case where delim is a single character.
  if (delim[0] != '\0' && delim[1] == '\0') {
    char c = delim[0];
    const char* p = full.data();
    const char* end = p + full.size();
    while (p != end) {
      if (*p == c) {
        ++p;
      } else {
        const char* start = p;
        while (++p != end && *p != c) {
          // Skip to the next occurence of the delimiter.
        }
        *result++ = StringType(start, p - start);
      }
    }
    return;
  }

  std::string::size_type begin_index, end_index;
  begin_index = full.find_first_not_of(delim);
  while (begin_index != std::string::npos) {
    end_index = full.find_first_of(delim, begin_index);
    if (end_index == std::string::npos) {
      *result++ = full.substr(begin_index);
      return;
    }
    *result++ = full.substr(begin_index, (end_index - begin_index));
    begin_index = full.find_first_not_of(delim, end_index);
  }
}

void SplitStringUsing(const std::string& full,
                      const char* delim,
                      vector<std::string>* result) {
  result->reserve(result->size() + CalculateReserveForVector(full, delim));
  std::back_insert_iterator<std::vector<std::string> > it((*result));
  SplitStringToIteratorUsing(full, delim, it);
}

void SplitStringToHashsetUsing(const std::string& full, const char* delim,
                               unordered_set<std::string>* result) {
  simple_insert_iterator<unordered_set<std::string> > it((result));
  SplitStringToIteratorUsing(full, delim, it);
}

void SplitStringToSetUsing(const std::string& full, const char* delim,
                           set<std::string>* result) {
  simple_insert_iterator<set<std::string> > it((result));
  SplitStringToIteratorUsing(full, delim, it);
}

void SplitStringToHashmapUsing(const std::string& full, const char* delim,
                               unordered_map<std::string, std::string>* result) {
  simple_hash_map_iterator<std::string> it((result));
  SplitStringToIteratorUsing(full, delim, it);
}

// ----------------------------------------------------------------------
// SplitOneIntToken()
// SplitOneInt32Token()
// SplitOneUint32Token()
// SplitOneInt64Token()
// SplitOneUint64Token()
// SplitOneDoubleToken()
// SplitOneFloatToken()
// SplitOneDecimalIntToken()
// SplitOneDecimalInt32Token()
// SplitOneDecimalUint32Token()
// SplitOneDecimalInt64Token()
// SplitOneDecimalUint64Token()
// SplitOneHexUint32Token()
// SplitOneHexUint64Token()
//   Mainly a stringified wrapper around strtol/strtoul/strtod
// ----------------------------------------------------------------------
// Curried functions for the macro below
static inline long strto32_0(const char * source, char ** end) {
  return strto32(source, end, 0); }
static inline unsigned long strtou32_0(const char * source, char ** end) {
  return strtou32(source, end, 0); }
static inline int64 strto64_0(const char * source, char ** end) {
  return strto64(source, end, 0); }
static inline uint64 strtou64_0(const char * source, char ** end) {
  return strtou64(source, end, 0); }
static inline long strto32_10(const char * source, char ** end) {
  return strto32(source, end, 10); }
static inline unsigned long strtou32_10(const char * source, char ** end) {
  return strtou32(source, end, 10); }
static inline int64 strto64_10(const char * source, char ** end) {
  return strto64(source, end, 10); }
static inline uint64 strtou64_10(const char * source, char ** end) {
  return strtou64(source, end, 10); }
static inline uint32 strtou32_16(const char * source, char ** end) {
  return strtou32(source, end, 16); }
static inline uint64 strtou64_16(const char * source, char ** end) {
  return strtou64(source, end, 16); }

#define DEFINE_SPLIT_ONE_NUMBER_TOKEN(name, type, function) \
bool SplitOne##name##Token(const char ** source, const char * delim, \
                           type * value) {                      \
  assert(source);                                               \
  assert(delim);                                                \
  assert(value);                                                \
  if (!*source)                                                 \
    return false;                                               \
  /* Parse int */                                               \
  char * end;                                                   \
  *value = function(*source, &end);                             \
  if (end == *source)                                           \
    return false; /* number not present at start of std::string */   \
  if (end[0] && !strchr(delim, end[0]))                         \
    return false; /* Garbage characters after int */            \
  /* Advance past token */                                      \
  if (*end != '\0')                                             \
    *source = const_cast<const char *>(end+1);                  \
   else                                                         \
    *source = NULL;                                             \
  return true;                                                  \
}

DEFINE_SPLIT_ONE_NUMBER_TOKEN(Int, int, strto32_0)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Int32, int32, strto32_0)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Uint32, uint32, strtou32_0)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Int64, int64, strto64_0)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Uint64, uint64, strtou64_0)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Double, double, strtod)
#ifdef COMPILER_MSVC  // has no strtof()
// Note: does an implicit cast to float.
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Float, float, strtod)
#else
DEFINE_SPLIT_ONE_NUMBER_TOKEN(Float, float, strtof)
#endif
DEFINE_SPLIT_ONE_NUMBER_TOKEN(DecimalInt, int, strto32_10)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(DecimalInt32, int32, strto32_10)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(DecimalUint32, uint32, strtou32_10)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(DecimalInt64, int64, strto64_10)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(DecimalUint64, uint64, strtou64_10)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(HexUint32, uint32, strtou32_16)
DEFINE_SPLIT_ONE_NUMBER_TOKEN(HexUint64, uint64, strtou64_16)

}  // namespace geo
