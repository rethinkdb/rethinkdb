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
// Sanjay Ghemawat <opensource@google.com>

#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>    // for write()
#endif
#include <string.h>
#include <google/malloc_extension.h>
#include "internal_logging.h"

static const int kLogBufSize = 800;

void TCMalloc_MESSAGE(const char* filename,
                      int line_number,
                      const char* format, ...) {
  char buf[kLogBufSize];
  const int n = snprintf(buf, sizeof(buf), "%s:%d] ", filename, line_number);
  if (n < kLogBufSize) {
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf + n, kLogBufSize - n, format, ap);
    va_end(ap);
  }
  write(STDERR_FILENO, buf, strlen(buf));
}

static const int kStatsBufferSize = 16 << 10;
static char stats_buffer[kStatsBufferSize] = { 0 };

static void TCMalloc_CRASH_internal(bool dump_stats,
                                    const char* filename,
                                    int line_number,
                                    const char* format, va_list ap) {
  char buf[kLogBufSize];
  const int n = snprintf(buf, sizeof(buf), "%s:%d] ", filename, line_number);
  if (n < kLogBufSize) {
    vsnprintf(buf + n, kLogBufSize - n, format, ap);
  }
  write(STDERR_FILENO, buf, strlen(buf));
  if (dump_stats) {
    MallocExtension::instance()->GetStats(stats_buffer, kStatsBufferSize);
    write(STDERR_FILENO, stats_buffer, strlen(stats_buffer));
  }

  abort();
}

void TCMalloc_CRASH(bool dump_stats,
                    const char* filename,
                    int line_number,
                    const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  TCMalloc_CRASH_internal(dump_stats, filename, line_number, format, ap);
  va_end(ap);
}


void TCMalloc_CrashReporter::PrintfAndDie(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  TCMalloc_CRASH_internal(dump_stats_, file_, line_, format, ap);
  va_end(ap);
}

void TCMalloc_Printer::printf(const char* format, ...) {
  if (left_ > 0) {
    va_list ap;
    va_start(ap, format);
    const int r = vsnprintf(buf_, left_, format, ap);
    va_end(ap);
    if (r < 0) {
      // Perhaps an old glibc that returns -1 on truncation?
      left_ = 0;
    } else if (r > left_) {
      // Truncation
      left_ = 0;
    } else {
      left_ -= r;
      buf_ += r;
    }
  }
}
