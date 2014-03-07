// Copyright 2008 the V8 project authors. All rights reserved.
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

// Flags: --expose-gc

function for_in_null() {
  try {
    for (var x in null) {
      return false;
    }
  } catch(e) {
    return false;
  }
  return true;
}

function for_in_undefined() {
  try {
    for (var x in undefined) {
      return false;
    }
  } catch(e) {
    return false;
  }
  return true;
}

for (var i = 0; i < 10; ++i) {
  assertTrue(for_in_null());
  gc();
}

for (var j = 0; j < 10; ++j) {
  assertTrue(for_in_undefined());
  gc();
}

assertEquals(10, i);
assertEquals(10, j);


function Accumulate(x) {
  var accumulator = "";
  for (var i in x) {
    accumulator += i;
  }
  return accumulator;
}

for (var i = 0; i < 3; ++i) {
  var elements = Accumulate("abcd");
  // We do not assume that for-in enumerates elements in order.
  assertTrue(-1 != elements.indexOf("0"));
  assertTrue(-1 != elements.indexOf("1"));
  assertTrue(-1 != elements.indexOf("2"));
  assertTrue(-1 != elements.indexOf("3"));
  assertEquals(4, elements.length);
}

function for_in_string_prototype() {

  var x = new String("abc");
  x.foo = 19;
  function B() {
    this.bar = 5;
    this[7] = 4;
  }
  B.prototype = x;

  var y = new B();
  y.gub = 13;

  var elements = Accumulate(y);
  var elements1 = Accumulate(y);
  // If for-in returns elements in a different order on multiple calls, this
  // assert will fail.  If that happens, consider if that behavior is OK.
  assertEquals(elements, elements1, "For-in elements not the same both times.");
  // We do not assume that for-in enumerates elements in order.
  assertTrue(-1 != elements.indexOf("0"));
  assertTrue(-1 != elements.indexOf("1"));
  assertTrue(-1 != elements.indexOf("2"));
  assertTrue(-1 != elements.indexOf("7"));
  assertTrue(-1 != elements.indexOf("foo"));
  assertTrue(-1 != elements.indexOf("bar"));
  assertTrue(-1 != elements.indexOf("gub"));
  assertEquals(13, elements.length);

  elements = Accumulate(x);
  assertTrue(-1 != elements.indexOf("0"));
  assertTrue(-1 != elements.indexOf("1"));
  assertTrue(-1 != elements.indexOf("2"));
  assertTrue(-1 != elements.indexOf("foo"));
  assertEquals(6, elements.length);
}

for_in_string_prototype();
for_in_string_prototype();
