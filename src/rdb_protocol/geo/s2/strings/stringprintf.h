// Copyright 2002 and onwards Google Inc.
//
// Printf variants that place their output in a C++ string.
//
// Usage:
//      std::string result = StringPrintf("%d %s\n", 10, "hello");
//      SStringPrintf(&result, "%d %s\n", 10, "hello");
//      StringAppendF(&result, "%d %s\n", 20, "there");

#ifndef _STRINGS_STRINGPRINTF_H
#define _STRINGS_STRINGPRINTF_H

#include <stdarg.h>

#include <string>
#include <vector>

#include "rdb_protocol/geo/s2/base/stl_decl.h"
#include "rdb_protocol/geo/s2/base/port.h"
#include "rdb_protocol/geo/s2/base/stringprintf.h"

namespace geo {
using std::vector;

// This file formerly contained
//   StringPrintf, SStringPrintf, StringAppendF, and StringAppendV.
// These routines have moved to base/stringprintf.{h,cc} to allow
// using them from files in base.  We include base/stringprintf.h
// in this file since so many clients were dependent on these
// routines being defined in stringprintf.h.


// The max arguments supported by StringPrintfVector
extern const size_t kStringPrintfVectorMaxArgs;

// You can use this version when all your arguments are strings, but
// you don't know how many arguments you'll have at compile time.
// StringPrintfVector will LOG(FATAL) if v.size() > kStringPrintfVectorMaxArgs
extern std::string StringPrintfVector(const char* format, const vector<std::string>& v);

}  // namespace geo


#endif /* _STRINGS_STRINGPRINTF_H */
