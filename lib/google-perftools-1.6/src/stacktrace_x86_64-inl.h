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
// Author: Arun Sharma
//
// Produce stack trace using libgcc

extern "C" {
#include <stdlib.h> // for NULL
#include <unwind.h> // ABI defined unwinder
#include <string.h> // for memset
}
#include "google/stacktrace.h"

typedef struct {
  void **result;
  int max_depth;
  int skip_count;
  int count;
} trace_arg_t;


// Workaround for the malloc() in _Unwind_Backtrace() issue.
static _Unwind_Reason_Code nop_backtrace(struct _Unwind_Context *uc, void *opq) {
  return _URC_NO_REASON;
}


// This code is not considered ready to run until
// static initializers run so that we are guaranteed
// that any malloc-related initialization is done.
static bool ready_to_run = false;
class StackTraceInit {
 public:
   StackTraceInit() {
     // Extra call to force initialization
     _Unwind_Backtrace(nop_backtrace, NULL);
     ready_to_run = true;
   }
};

static StackTraceInit module_initializer;  // Force initialization

static _Unwind_Reason_Code GetOneFrame(struct _Unwind_Context *uc, void *opq) {
  trace_arg_t *targ = (trace_arg_t *) opq;

  if (targ->skip_count > 0) {
    targ->skip_count--;
  } else {
    targ->result[targ->count++] = (void *) _Unwind_GetIP(uc);
  }

  if (targ->count == targ->max_depth)
    return _URC_END_OF_STACK;

  return _URC_NO_REASON;
}

// If you change this function, also change GetStackFrames below.
int GetStackTrace(void** result, int max_depth, int skip_count) {
  if (!ready_to_run)
    return 0;

  trace_arg_t targ;

  skip_count += 1;         // Do not include the "GetStackTrace" frame

  targ.result = result;
  targ.max_depth = max_depth;
  targ.skip_count = skip_count;
  targ.count = 0;

  _Unwind_Backtrace(GetOneFrame, &targ);

  return targ.count;
}

// If you change this function, also change GetStackTrace above:
//
// This GetStackFrames routine shares a lot of code with GetStackTrace
// above. This code could have been refactored into a common routine,
// and then both GetStackTrace/GetStackFrames could call that routine.
// There are two problems with that:
//
// (1) The performance of the refactored-code suffers substantially - the
//     refactored needs to be able to record the stack trace when called
//     from GetStackTrace, and both the stack trace and stack frame sizes,
//     when called from GetStackFrames - this introduces enough new
//     conditionals that GetStackTrace performance can degrade by as much
//     as 50%.
//
// (2) Whether the refactored routine gets inlined into GetStackTrace and
//     GetStackFrames depends on the compiler, and we can't guarantee the
//     behavior either-way, even with "__attribute__ ((always_inline))"
//     or "__attribute__ ((noinline))". But we need this guarantee or the
//     frame counts may be off by one.
//
// Both (1) and (2) can be addressed without this code duplication, by
// clever use of template functions, and by defining GetStackTrace and
// GetStackFrames as macros that expand to these template functions.
// However, this approach comes with its own set of problems - namely,
// macros and  preprocessor trouble - for example,  if GetStackTrace
// and/or GetStackFrames is ever defined as a member functions in some
// class, we are in trouble.
int GetStackFrames(void** pcs, int* sizes, int max_depth, int skip_count) {
  if (!ready_to_run)
    return 0;

  trace_arg_t targ;

  skip_count += 1;         // Do not include the "GetStackFrames" frame

  targ.result = pcs;
  targ.max_depth = max_depth;
  targ.skip_count = skip_count;
  targ.count = 0;

  _Unwind_Backtrace(GetOneFrame, &targ);

  // No implementation for finding out the stack frame sizes yet.
  memset(sizes, 0, sizeof(*sizes) * targ.count);

  return targ.count;
}
