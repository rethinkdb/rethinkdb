// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --expose-debug-as debug --allow-natives-syntax --crankshaft

Debug = debug.Debug;

Debug.setListener(function() {});

function f() {}
f();
f();
%OptimizeFunctionOnNextCall(f);
f();
assertOptimized(f);

Debug.setListener(null);
