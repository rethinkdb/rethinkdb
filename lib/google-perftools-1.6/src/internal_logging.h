// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Sanjay Ghemawat <opensource@google.com>
//
// Internal logging and related utility routines.

#ifndef TCMALLOC_INTERNAL_LOGGING_H_
#define TCMALLOC_INTERNAL_LOGGING_H_

#include <config.h>
#include <stdlib.h>   // for abort()
#ifdef HAVE_UNISTD_H
#include <unistd.h>   // for write()
#endif

//-------------------------------------------------------------------
// Utility routines
//-------------------------------------------------------------------

// Safe debugging routine: we write directly to the stderr file
// descriptor and avoid FILE buffering because that may invoke
// malloc()
extern void TCMalloc_MESSAGE(const char* filename,
                             int line_number,
                             const char* format, ...)
#ifdef HAVE___ATTRIBUTE__
  __attribute__ ((__format__ (__printf__, 3, 4)))
#endif
;

// Right now, the only non-fatal messages we want to report are when
// an allocation fails (we'll return NULL eventually, but sometimes
// want more prominent notice to help debug).  message should be
// a literal string with no %<whatever> format directives.
#ifdef TCMALLOC_WARNINGS
#define MESSAGE(message, num_bytes)                                     \
   TCMalloc_MESSAGE(__FILE__, __LINE__, message " (%"PRIuS" bytes)\n",  \
                    static_cast<size_t>(num_bytes))
#else
#define MESSAGE(message, num_bytes)
#endif

// Dumps the specified message and then calls abort().  If
// "dump_stats" is specified, the first call will also dump the
// tcmalloc stats.
extern void TCMalloc_CRASH(bool dump_stats,
                           const char* filename,
                           int line_number,
                           const char* format, ...)
#ifdef HAVE___ATTRIBUTE__
  __attribute__ ((__format__ (__printf__, 4, 5)))
#endif
;

// This is a class that makes using the macro easier.  With this class,
// CRASH("%d", i) expands to TCMalloc_CrashReporter.PrintfAndDie("%d", i).
class PERFTOOLS_DLL_DECL TCMalloc_CrashReporter {
 public:
  TCMalloc_CrashReporter(bool dump_stats, const char* file, int line)
      : dump_stats_(dump_stats), file_(file), line_(line) {
  }
  void PrintfAndDie(const char* format, ...)
#ifdef HAVE___ATTRIBUTE__
      __attribute__ ((__format__ (__printf__, 2, 3)))  // 2,3 due to "this"
#endif
;

 private:
  bool dump_stats_;
  const char* file_;
  int line_;
};

#define CRASH \
  TCMalloc_CrashReporter(false, __FILE__, __LINE__).PrintfAndDie

#define CRASH_WITH_STATS \
  TCMalloc_CrashReporter(true, __FILE__, __LINE__).PrintfAndDie

// Like assert(), but executed even in NDEBUG mode
#undef CHECK_CONDITION
#define CHECK_CONDITION(cond)                                            \
do {                                                                     \
  if (!(cond)) {                                                         \
    CRASH("assertion failed: %s\n", #cond);   \
  }                                                                      \
} while (0)

// Our own version of assert() so we can avoid hanging by trying to do
// all kinds of goofy printing while holding the malloc lock.
#ifndef NDEBUG
#define ASSERT(cond) CHECK_CONDITION(cond)
#else
#define ASSERT(cond) ((void) 0)
#endif

// Print into buffer
class TCMalloc_Printer {
 private:
  char* buf_;           // Where should we write next
  int   left_;          // Space left in buffer (including space for \0)

 public:
  // REQUIRES: "length > 0"
  TCMalloc_Printer(char* buf, int length) : buf_(buf), left_(length) {
    buf[0] = '\0';
  }

  void printf(const char* format, ...)
#ifdef HAVE___ATTRIBUTE__
    __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
;
};

#endif  // TCMALLOC_INTERNAL_LOGGING_H_
