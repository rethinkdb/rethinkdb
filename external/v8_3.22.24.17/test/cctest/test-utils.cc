// Copyright 2011 the V8 project authors. All rights reserved.
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

#include "cctest.h"
#include "platform.h"
#include "utils-inl.h"

using namespace v8::internal;


TEST(Utils1) {
  CHECK_EQ(-1000000, FastD2I(-1000000.0));
  CHECK_EQ(-1, FastD2I(-1.0));
  CHECK_EQ(0, FastD2I(0.0));
  CHECK_EQ(1, FastD2I(1.0));
  CHECK_EQ(1000000, FastD2I(1000000.0));

  CHECK_EQ(-1000000, FastD2I(-1000000.123));
  CHECK_EQ(-1, FastD2I(-1.234));
  CHECK_EQ(0, FastD2I(0.345));
  CHECK_EQ(1, FastD2I(1.234));
  CHECK_EQ(1000000, FastD2I(1000000.123));
  // Check that >> is implemented as arithmetic shift right.
  // If this is not true, then ArithmeticShiftRight() must be changed,
  // There are also documented right shifts in assembler.cc of
  // int8_t and intptr_t signed integers.
  CHECK_EQ(-2, -8 >> 2);
  CHECK_EQ(-2, static_cast<int8_t>(-8) >> 2);
  CHECK_EQ(-2, static_cast<int>(static_cast<intptr_t>(-8) >> 2));

  CHECK_EQ(-1000000, FastD2IChecked(-1000000.0));
  CHECK_EQ(-1, FastD2IChecked(-1.0));
  CHECK_EQ(0, FastD2IChecked(0.0));
  CHECK_EQ(1, FastD2IChecked(1.0));
  CHECK_EQ(1000000, FastD2IChecked(1000000.0));

  CHECK_EQ(-1000000, FastD2IChecked(-1000000.123));
  CHECK_EQ(-1, FastD2IChecked(-1.234));
  CHECK_EQ(0, FastD2IChecked(0.345));
  CHECK_EQ(1, FastD2IChecked(1.234));
  CHECK_EQ(1000000, FastD2IChecked(1000000.123));

  CHECK_EQ(INT_MAX, FastD2IChecked(1.0e100));
  CHECK_EQ(INT_MIN, FastD2IChecked(-1.0e100));
  CHECK_EQ(INT_MIN, FastD2IChecked(OS::nan_value()));
}


TEST(SNPrintF) {
  // Make sure that strings that are truncated because of too small
  // buffers are zero-terminated anyway.
  const char* s = "the quick lazy .... oh forget it!";
  int length = StrLength(s);
  for (int i = 1; i < length * 2; i++) {
    static const char kMarker = static_cast<char>(42);
    Vector<char> buffer = Vector<char>::New(i + 1);
    buffer[i] = kMarker;
    int n = OS::SNPrintF(Vector<char>(buffer.start(), i), "%s", s);
    CHECK(n <= i);
    CHECK(n == length || n == -1);
    CHECK_EQ(0, strncmp(buffer.start(), s, i - 1));
    CHECK_EQ(kMarker, buffer[i]);
    if (i <= length) {
      CHECK_EQ(i - 1, StrLength(buffer.start()));
    } else {
      CHECK_EQ(length, StrLength(buffer.start()));
    }
    buffer.Dispose();
  }
}


static const int kAreaSize = 512;


void TestMemMove(byte* area1,
                 byte* area2,
                 int src_offset,
                 int dest_offset,
                 int length) {
  for (int i = 0; i < kAreaSize; i++) {
    area1[i] = i & 0xFF;
    area2[i] = i & 0xFF;
  }
  OS::MemMove(area1 + dest_offset, area1 + src_offset, length);
  memmove(area2 + dest_offset, area2 + src_offset, length);
  if (memcmp(area1, area2, kAreaSize) != 0) {
    printf("OS::MemMove(): src_offset: %d, dest_offset: %d, length: %d\n",
           src_offset, dest_offset, length);
    for (int i = 0; i < kAreaSize; i++) {
      if (area1[i] == area2[i]) continue;
      printf("diff at offset %d (%p): is %d, should be %d\n",
             i, reinterpret_cast<void*>(area1 + i), area1[i], area2[i]);
    }
    CHECK(false);
  }
}


TEST(MemMove) {
  v8::V8::Initialize();
  byte* area1 = new byte[kAreaSize];
  byte* area2 = new byte[kAreaSize];

  static const int kMinOffset = 32;
  static const int kMaxOffset = 64;
  static const int kMaxLength = 128;
  STATIC_ASSERT(kMaxOffset + kMaxLength < kAreaSize);

  for (int src_offset = kMinOffset; src_offset <= kMaxOffset; src_offset++) {
    for (int dst_offset = kMinOffset; dst_offset <= kMaxOffset; dst_offset++) {
      for (int length = 0; length <= kMaxLength; length++) {
        TestMemMove(area1, area2, src_offset, dst_offset, length);
      }
    }
  }
  delete[] area1;
  delete[] area2;
}


TEST(Collector) {
  Collector<int> collector(8);
  const int kLoops = 5;
  const int kSequentialSize = 1000;
  const int kBlockSize = 7;
  for (int loop = 0; loop < kLoops; loop++) {
    Vector<int> block = collector.AddBlock(7, 0xbadcafe);
    for (int i = 0; i < kSequentialSize; i++) {
      collector.Add(i);
    }
    for (int i = 0; i < kBlockSize - 1; i++) {
      block[i] = i * 7;
    }
  }
  Vector<int> result = collector.ToVector();
  CHECK_EQ(kLoops * (kBlockSize + kSequentialSize), result.length());
  for (int i = 0; i < kLoops; i++) {
    int offset = i * (kSequentialSize + kBlockSize);
    for (int j = 0; j < kBlockSize - 1; j++) {
      CHECK_EQ(j * 7, result[offset + j]);
    }
    CHECK_EQ(0xbadcafe, result[offset + kBlockSize - 1]);
    for (int j = 0; j < kSequentialSize; j++) {
      CHECK_EQ(j, result[offset + kBlockSize + j]);
    }
  }
  result.Dispose();
}


TEST(SequenceCollector) {
  SequenceCollector<int> collector(8);
  const int kLoops = 5000;
  const int kMaxSequenceSize = 13;
  int total_length = 0;
  for (int loop = 0; loop < kLoops; loop++) {
    int seq_length = loop % kMaxSequenceSize;
    collector.StartSequence();
    for (int j = 0; j < seq_length; j++) {
      collector.Add(j);
    }
    Vector<int> sequence = collector.EndSequence();
    for (int j = 0; j < seq_length; j++) {
      CHECK_EQ(j, sequence[j]);
    }
    total_length += seq_length;
  }
  Vector<int> result = collector.ToVector();
  CHECK_EQ(total_length, result.length());
  int offset = 0;
  for (int loop = 0; loop < kLoops; loop++) {
    int seq_length = loop % kMaxSequenceSize;
    for (int j = 0; j < seq_length; j++) {
      CHECK_EQ(j, result[offset]);
      offset++;
    }
  }
  result.Dispose();
}


TEST(SequenceCollectorRegression) {
  SequenceCollector<char> collector(16);
  collector.StartSequence();
  collector.Add('0');
  collector.AddBlock(
      i::Vector<const char>("12345678901234567890123456789012", 32));
  i::Vector<char> seq = collector.EndSequence();
  CHECK_EQ(0, strncmp("0123456789012345678901234567890123",
                      seq.start(), seq.length()));
}
