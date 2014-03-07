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

#include <limits>

#include "v8.h"

#include "cctest.h"
#include "code-stubs.h"
#include "test-code-stubs.h"
#include "factory.h"
#include "macro-assembler.h"
#include "platform.h"

using namespace v8::internal;


int STDCALL ConvertDToICVersion(double d) {
  union { double d; uint32_t u[2]; } dbl;
  dbl.d = d;
  uint32_t exponent_bits = dbl.u[1];
  int32_t shifted_mask = static_cast<int32_t>(Double::kExponentMask >> 32);
  int32_t exponent = (((exponent_bits & shifted_mask) >>
                       (Double::kPhysicalSignificandSize - 32)) -
                      HeapNumber::kExponentBias);
  uint32_t unsigned_exponent = static_cast<uint32_t>(exponent);
  int result = 0;
  uint32_t max_exponent =
    static_cast<uint32_t>(Double::kPhysicalSignificandSize);
  if (unsigned_exponent >= max_exponent) {
    if ((exponent - Double::kPhysicalSignificandSize) < 32) {
      result = dbl.u[0] << (exponent - Double::kPhysicalSignificandSize);
    }
  } else {
    uint64_t big_result =
        (BitCast<uint64_t>(d) & Double::kSignificandMask) | Double::kHiddenBit;
    big_result = big_result >> (Double::kPhysicalSignificandSize - exponent);
    result = static_cast<uint32_t>(big_result);
  }
  if (static_cast<int32_t>(exponent_bits) < 0) {
    return (0 - result);
  } else {
    return result;
  }
}


void RunOneTruncationTestWithTest(ConvertDToICallWrapper callWrapper,
                                  ConvertDToIFunc func,
                                  double from,
                                  double raw) {
  uint64_t to = static_cast<int64_t>(raw);
  int result = (*callWrapper)(func, from);
  CHECK_EQ(static_cast<int>(to), result);
}


int32_t DefaultCallWrapper(ConvertDToIFunc func,
                           double from) {
  return (*func)(from);
}


// #define NaN and Infinity so that it's possible to cut-and-paste these tests
// directly to a .js file and run them.
#define NaN (OS::nan_value())
#define Infinity (std::numeric_limits<double>::infinity())
#define RunOneTruncationTest(p1, p2) \
    RunOneTruncationTestWithTest(callWrapper, func, p1, p2)


void RunAllTruncationTests(ConvertDToIFunc func) {
  RunAllTruncationTests(DefaultCallWrapper, func);
}


void RunAllTruncationTests(ConvertDToICallWrapper callWrapper,
                           ConvertDToIFunc func) {
  RunOneTruncationTest(0, 0);
  RunOneTruncationTest(0.5, 0);
  RunOneTruncationTest(-0.5, 0);
  RunOneTruncationTest(1.5, 1);
  RunOneTruncationTest(-1.5, -1);
  RunOneTruncationTest(5.5, 5);
  RunOneTruncationTest(-5.0, -5);
  RunOneTruncationTest(NaN, 0);
  RunOneTruncationTest(Infinity, 0);
  RunOneTruncationTest(-NaN, 0);
  RunOneTruncationTest(-Infinity, 0);

  RunOneTruncationTest(4.5036e+15, 0x1635E000);
  RunOneTruncationTest(-4.5036e+15, -372629504);

  RunOneTruncationTest(4503603922337791.0, -1);
  RunOneTruncationTest(-4503603922337791.0, 1);
  RunOneTruncationTest(4503601774854143.0, 2147483647);
  RunOneTruncationTest(-4503601774854143.0, -2147483647);
  RunOneTruncationTest(9007207844675582.0, -2);
  RunOneTruncationTest(-9007207844675582.0, 2);

  RunOneTruncationTest(2.4178527921507624e+24, -536870912);
  RunOneTruncationTest(-2.4178527921507624e+24, 536870912);
  RunOneTruncationTest(2.417853945072267e+24, -536870912);
  RunOneTruncationTest(-2.417853945072267e+24, 536870912);

  RunOneTruncationTest(4.8357055843015248e+24, -1073741824);
  RunOneTruncationTest(-4.8357055843015248e+24, 1073741824);
  RunOneTruncationTest(4.8357078901445341e+24, -1073741824);
  RunOneTruncationTest(-4.8357078901445341e+24, 1073741824);

  RunOneTruncationTest(9.6714111686030497e+24, -2147483648.0);
  RunOneTruncationTest(-9.6714111686030497e+24, -2147483648.0);
  RunOneTruncationTest(9.6714157802890681e+24, -2147483648.0);
  RunOneTruncationTest(-9.6714157802890681e+24, -2147483648.0);
}

#undef NaN
#undef Infinity
#undef RunOneTruncationTest
