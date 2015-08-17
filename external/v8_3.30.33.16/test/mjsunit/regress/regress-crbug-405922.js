// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --allow-natives-syntax --expose-debug-as debug

Debug = debug.Debug

function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break) {
      exec_state.prepareStep(Debug.StepAction.StepIn, 3);
    }
  } catch (e) {
  }
}

Debug.setListener(listener);

function f(x) {
  if (x > 0) %_CallFunction(null, x-1, f);
}

debugger;
f(2);

Debug.setListener(null);
