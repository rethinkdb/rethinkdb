// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --allow-natives-syntax --block-concurrent-recompilation
// Flags: --no-concurrent-osr

function Ctor() {
  this.a = 1;
}

function get_closure() {
  return function add_field(obj, osr) {
    obj.c = 3;
    var x = 0;
    if (osr) {
      %OptimizeFunctionOnNextCall(add_field, "osr");
    }
    for (var i = 0; i < 10; i++) {
      x = i + 1;
    }
    return x;
  }
}

var f1 = get_closure();
f1(new Ctor(), false);
f1(new Ctor(), false);

%OptimizeFunctionOnNextCall(f1, "concurrent");

// Kick off concurrent recompilation and OSR.
var o = new Ctor();
f1(o, true);
assertOptimized(f1, "no sync");

// Flush the optimizing compiler's queue.
%NotifyContextDisposed();
assertUnoptimized(f1, "no sync");

// Trigger deopt.
o.c = 2.2;

var f2 = get_closure();
f2(new Ctor(), true);
