// Copyright 2013 the V8 project authors. All rights reserved.
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

#include <stdlib.h>

#include "v8.h"
#include "macro-assembler.h"
#include "arm/macro-assembler-arm.h"
#include "arm/simulator-arm.h"
#include "cctest.h"


using namespace v8::internal;

typedef void* (*F)(int x, int y, int p2, int p3, int p4);

#define __ masm->


static byte to_non_zero(int n) {
  return static_cast<unsigned>(n) % 255 + 1;
}


static bool all_zeroes(const byte* beg, const byte* end) {
  CHECK(beg);
  CHECK(beg <= end);
  while (beg < end) {
    if (*beg++ != 0)
      return false;
  }
  return true;
}


TEST(CopyBytes) {
  CcTest::InitializeVM();
  Isolate* isolate = Isolate::Current();
  HandleScope handles(isolate);

  const int data_size = 1 * KB;
  size_t act_size;

  // Allocate two blocks to copy data between.
  byte* src_buffer = static_cast<byte*>(OS::Allocate(data_size, &act_size, 0));
  CHECK(src_buffer);
  CHECK(act_size >= static_cast<size_t>(data_size));
  byte* dest_buffer = static_cast<byte*>(OS::Allocate(data_size, &act_size, 0));
  CHECK(dest_buffer);
  CHECK(act_size >= static_cast<size_t>(data_size));

  // Storage for R0 and R1.
  byte* r0_;
  byte* r1_;

  MacroAssembler assembler(isolate, NULL, 0);
  MacroAssembler* masm = &assembler;

  // Code to be generated: The stuff in CopyBytes followed by a store of R0 and
  // R1, respectively.
  __ CopyBytes(r0, r1, r2, r3);
  __ mov(r2, Operand(reinterpret_cast<int>(&r0_)));
  __ mov(r3, Operand(reinterpret_cast<int>(&r1_)));
  __ str(r0, MemOperand(r2));
  __ str(r1, MemOperand(r3));
  __ bx(lr);

  CodeDesc desc;
  masm->GetCode(&desc);
  Object* code = isolate->heap()->CreateCode(
      desc,
      Code::ComputeFlags(Code::STUB),
      Handle<Code>())->ToObjectChecked();
  CHECK(code->IsCode());

  F f = FUNCTION_CAST<F>(Code::cast(code)->entry());

  // Initialise source data with non-zero bytes.
  for (int i = 0; i < data_size; i++) {
    src_buffer[i] = to_non_zero(i);
  }

  const int fuzz = 11;

  for (int size = 0; size < 600; size++) {
    for (const byte* src = src_buffer; src < src_buffer + fuzz; src++) {
      for (byte* dest = dest_buffer; dest < dest_buffer + fuzz; dest++) {
        memset(dest_buffer, 0, data_size);
        CHECK(dest + size < dest_buffer + data_size);
        (void) CALL_GENERATED_CODE(f, reinterpret_cast<int>(src),
                                      reinterpret_cast<int>(dest), size, 0, 0);
        // R0 and R1 should point at the first byte after the copied data.
        CHECK_EQ(src + size, r0_);
        CHECK_EQ(dest + size, r1_);
        // Check that we haven't written outside the target area.
        CHECK(all_zeroes(dest_buffer, dest));
        CHECK(all_zeroes(dest + size, dest_buffer + data_size));
        // Check the target area.
        CHECK_EQ(0, memcmp(src, dest, size));
      }
    }
  }

  // Check that the source data hasn't been clobbered.
  for (int i = 0; i < data_size; i++) {
    CHECK(src_buffer[i] == to_non_zero(i));
  }
}



#undef __
