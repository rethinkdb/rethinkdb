// Copyright 2014 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
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

#ifndef V8_PERF_JIT_H_
#define V8_PERF_JIT_H_

#include "src/v8.h"

namespace v8 {
namespace internal {

// TODO(jarin) For now, we disable perf integration on Android because of a
// build problem - when building the snapshot with AOSP, librt is not
// available, so we cannot use the clock_gettime function. To fix this, we
// should thread through the V8_LIBRT_NOT_AVAILABLE flag here and only disable
// the perf integration when this flag is present (the perf integration is not
// needed when generating snapshot, so it is fine to ifdef it away).

#if V8_OS_LINUX

// Linux perf tool logging support
class PerfJitLogger : public CodeEventLogger {
 public:
  PerfJitLogger();
  virtual ~PerfJitLogger();

  virtual void CodeMoveEvent(Address from, Address to);
  virtual void CodeDeleteEvent(Address from);
  virtual void CodeDisableOptEvent(Code* code, SharedFunctionInfo* shared) {}
  virtual void SnapshotPositionEvent(Address addr, int pos);

 private:
  uint64_t GetTimestamp();
  virtual void LogRecordedBuffer(Code* code, SharedFunctionInfo* shared,
                                 const char* name, int length);

  // Extension added to V8 log file name to get the low-level log name.
  static const char kFilenameFormatString[];
  static const int kFilenameBufferPadding;

  // File buffer size of the low-level log. We don't use the default to
  // minimize the associated overhead.
  static const int kLogBufferSize = 2 * MB;

  void LogWriteBytes(const char* bytes, int size);
  void LogWriteHeader();

  static const uint32_t kElfMachIA32 = 3;
  static const uint32_t kElfMachX64 = 62;
  static const uint32_t kElfMachARM = 40;
  static const uint32_t kElfMachMIPS = 10;

  uint32_t GetElfMach() {
#if V8_TARGET_ARCH_IA32
    return kElfMachIA32;
#elif V8_TARGET_ARCH_X64
    return kElfMachX64;
#elif V8_TARGET_ARCH_ARM
    return kElfMachARM;
#elif V8_TARGET_ARCH_MIPS
    return kElfMachMIPS;
#else
    UNIMPLEMENTED();
    return 0;
#endif
  }

  FILE* perf_output_handle_;
  uint64_t code_index_;
};

#else

// PerfJitLogger is only implemented on Linux
class PerfJitLogger : public CodeEventLogger {
 public:
  virtual void CodeMoveEvent(Address from, Address to) { UNIMPLEMENTED(); }

  virtual void CodeDeleteEvent(Address from) { UNIMPLEMENTED(); }

  virtual void CodeDisableOptEvent(Code* code, SharedFunctionInfo* shared) {
    UNIMPLEMENTED();
  }

  virtual void SnapshotPositionEvent(Address addr, int pos) { UNIMPLEMENTED(); }

  virtual void LogRecordedBuffer(Code* code, SharedFunctionInfo* shared,
                                 const char* name, int length) {
    UNIMPLEMENTED();
  }
};

#endif  // V8_OS_LINUX
}
}  // namespace v8::internal
#endif
