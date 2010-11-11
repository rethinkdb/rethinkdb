// Copyright (c) 2007, Google Inc.
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

#include "config_for_unittests.h"
#include <stdio.h>
#if defined HAVE_STDINT_H
#include <stdint.h>             // to get uintptr_t
#elif defined HAVE_INTTYPES_H
#include <inttypes.h>           // another place uintptr_t might be defined
#endif
#include <sys/types.h>
#include "base/logging.h"
#include "system-alloc.h"

class ArraySysAllocator : public SysAllocator {
public:
  // Was this allocator invoked at least once?
  bool invoked_;

  ArraySysAllocator() : SysAllocator() {
    ptr_ = 0;
    invoked_ = false;
  }

  void* Alloc(size_t size, size_t *actual_size, size_t alignment) {
    invoked_ = true;
    void *result = &array_[ptr_];
    uintptr_t ptr = reinterpret_cast<uintptr_t>(result);

    if (actual_size) {
      *actual_size = size;
    }

    // Try to get more memory for alignment
    size_t extra = alignment - (ptr & (alignment-1));
    size += extra;
    CHECK_LT(ptr_ + size, kArraySize);

    if ((ptr & (alignment-1)) != 0) {
      ptr += alignment - (ptr & (alignment-1));
    }

    ptr_ += size;
    return reinterpret_cast<void *>(ptr);
  }

  void DumpStats(TCMalloc_Printer* printer) {
  }

private:
  static const int kArraySize = 8 * 1024 * 1024;
  char array_[kArraySize];
  // We allocate the next chunk from here
  int ptr_;

};
const int ArraySysAllocator::kArraySize;
ArraySysAllocator a;

static void TestBasicInvoked() {
  RegisterSystemAllocator(&a, 0);

  // An allocation size that is likely to trigger the system allocator.
  // XXX: this is implementation specific.
  char *p = new char[1024 * 1024];
  delete [] p;

  // Make sure that our allocator was invoked.
  CHECK(a.invoked_);
}

int main(int argc, char** argv) {
  TestBasicInvoked();

  printf("PASS\n");
  return 0;
}
