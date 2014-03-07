// Copyright 2012 the V8 project authors. All rights reserved.
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

// Flags: --allow-natives-syntax --smi-only-arrays --expose-gc
// Flags: --track-allocation-sites --noalways-opt

// Test element kind of objects.
// Since --smi-only-arrays affects builtins, its default setting at compile
// time sticks if built with snapshot.  If --smi-only-arrays is deactivated
// by default, only a no-snapshot build actually has smi-only arrays enabled
// in this test case.  Depending on whether smi-only arrays are actually
// enabled, this test takes the appropriate code path to check smi-only arrays.

// Reset the GC stress mode to be off. Needed because AllocationMementos only
// live for one gc, so a gc that happens in certain fragile areas of the test
// can break assumptions.
%SetFlags("--gc-interval=-1")

// support_smi_only_arrays = %HasFastSmiElements(new Array(1,2,3,4,5,6,7,8));
support_smi_only_arrays = true;

if (support_smi_only_arrays) {
  print("Tests include smi-only arrays.");
} else {
  print("Tests do NOT include smi-only arrays.");
}

var elements_kind = {
  fast_smi_only            :  'fast smi only elements',
  fast                     :  'fast elements',
  fast_double              :  'fast double elements',
  dictionary               :  'dictionary elements',
  external_byte            :  'external byte elements',
  external_unsigned_byte   :  'external unsigned byte elements',
  external_short           :  'external short elements',
  external_unsigned_short  :  'external unsigned short elements',
  external_int             :  'external int elements',
  external_unsigned_int    :  'external unsigned int elements',
  external_float           :  'external float elements',
  external_double          :  'external double elements',
  external_pixel           :  'external pixel elements'
}

function getKind(obj) {
  if (%HasFastSmiElements(obj)) return elements_kind.fast_smi_only;
  if (%HasFastObjectElements(obj)) return elements_kind.fast;
  if (%HasFastDoubleElements(obj)) return elements_kind.fast_double;
  if (%HasDictionaryElements(obj)) return elements_kind.dictionary;
}

function isHoley(obj) {
  if (%HasFastHoleyElements(obj)) return true;
  return false;
}

function assertKind(expected, obj, name_opt) {
  if (!support_smi_only_arrays &&
      expected == elements_kind.fast_smi_only) {
    expected = elements_kind.fast;
  }
  assertEquals(expected, getKind(obj), name_opt);
}

if (support_smi_only_arrays) {

  // Test: If a call site goes megamorphic, it loses the ability to
  // use allocation site feedback.
  (function() {
    function bar(t, len) {
      return new t(len);
    }

    a = bar(Array, 10);
    a[0] = 3.5;
    b = bar(Array, 1);
    assertKind(elements_kind.fast_double, b);
    c = bar(Object, 3);
    b = bar(Array, 10);
    assertKind(elements_kind.fast_smi_only, b);
    b[0] = 3.5;
    c = bar(Array, 10);
    assertKind(elements_kind.fast_smi_only, c);
  })();


  // Test: ensure that crankshafted array constructor sites are deopted
  // if another function is used.
  (function() {
    function bar0(t) {
      return new t();
    }
    a = bar0(Array);
    a[0] = 3.5;
    b = bar0(Array);
    assertKind(elements_kind.fast_double, b);
    %OptimizeFunctionOnNextCall(bar0);
    b = bar0(Array);
    assertKind(elements_kind.fast_double, b);
    assertOptimized(bar0);
    // bar0 should deopt
    b = bar0(Object);
    assertUnoptimized(bar0)
    // When it's re-optimized, we should call through the full stub
    bar0(Array);
    %OptimizeFunctionOnNextCall(bar0);
    b = bar0(Array);
    // We also lost our ability to record kind feedback, as the site
    // is megamorphic now.
    assertKind(elements_kind.fast_smi_only, b);
    assertOptimized(bar0);
    b[0] = 3.5;
    c = bar0(Array);
    assertKind(elements_kind.fast_smi_only, c);
  })();


  // Test: Ensure that bailouts from the stub don't deopt a crankshafted
  // method with a call to that stub.
  (function() {
    function bar(len) {
      return new Array(len);
    }
    a = bar(10);
    a[0] = "a string";
    a = bar(10);
    assertKind(elements_kind.fast, a);
    %OptimizeFunctionOnNextCall(bar);
    a = bar(10);
    assertKind(elements_kind.fast, a);
    assertOptimized(bar);
    // The stub bails out, but the method call should be fine.
    a = bar(100000);
    assertOptimized(bar);
    assertKind(elements_kind.dictionary, a);

    // If the argument isn't a smi, it bails out as well
    a = bar("oops");
    assertOptimized(bar);
    assertKind(elements_kind.fast, a);

    function barn(one, two, three) {
      return new Array(one, two, three);
    }

    barn(1, 2, 3);
    barn(1, 2, 3);
    %OptimizeFunctionOnNextCall(barn);
    barn(1, 2, 3);
    assertOptimized(barn);
    a = barn(1, "oops", 3);
    // The stub should bail out but the method should remain optimized.
    assertKind(elements_kind.fast, a);
    assertOptimized(barn);
  })();


  // Test: When a method with array constructor is crankshafted, the type
  // feedback for elements kind is baked in. Verify that transitions don't
  // change it anymore
  (function() {
    function bar() {
      return new Array();
    }
    a = bar();
    bar();
    %OptimizeFunctionOnNextCall(bar);
    b = bar();
    // This only makes sense to test if we allow crankshafting
    if (4 != %GetOptimizationStatus(bar)) {
      assertOptimized(bar);
      %DebugPrint(3);
      b[0] = 3.5;
      c = bar();
      assertKind(elements_kind.fast_smi_only, c);
      assertOptimized(bar);
    }
  })();


  // Test: create arrays in two contexts, verifying that the correct
  // map for Array in that context will be used.
  (function() {
    function bar() { return new Array(); }
    bar();
    bar();
    %OptimizeFunctionOnNextCall(bar);
    a = bar();
    assertTrue(a instanceof Array);

    var contextB = Realm.create();
    Realm.eval(contextB, "function bar2() { return new Array(); };");
    Realm.eval(contextB, "bar2(); bar2();");
    Realm.eval(contextB, "%OptimizeFunctionOnNextCall(bar2);");
    Realm.eval(contextB, "bar2();");
    assertFalse(Realm.eval(contextB, "bar2();") instanceof Array);
    assertTrue(Realm.eval(contextB, "bar2() instanceof Array"));
  })();
}
