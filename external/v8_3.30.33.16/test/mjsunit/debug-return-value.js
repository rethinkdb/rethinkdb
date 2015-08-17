// Copyright 2010 the V8 project authors. All rights reserved.
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

// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug


function ParsedResponse(json) {
  this.response_ = eval('(' + json + ')');
  this.refs_ = [];
  if (this.response_.refs) {
    for (var i = 0; i < this.response_.refs.length; i++) {
      this.refs_[this.response_.refs[i].handle] = this.response_.refs[i];
    }
  }
}


ParsedResponse.prototype.response = function() {
  return this.response_;
}


ParsedResponse.prototype.body = function() {
  return this.response_.body;
}


ParsedResponse.prototype.running = function() {
  return this.response_.running;
}


ParsedResponse.prototype.lookup = function(handle) {
  return this.refs_[handle];
}


listener_complete = false;
exception = false;
break_count = 0;
expected_return_value = 0;
debugger_source_position = 0;

// Listener which expects to do four steps to reach returning from the function.
function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break)
    {
      break_count++;
      if (break_count < 4) {
        assertFalse(exec_state.frame(0).isAtReturn())
        switch (break_count) {
          case 1:
            // Collect the position of the debugger statement.
            debugger_source_position = exec_state.frame(0).sourcePosition();
            break;
          case 2:
            // Position now at the if statement.
            assertEquals(debugger_source_position + 10,
                         exec_state.frame(0).sourcePosition());
            break;
          case 3:
            // Position now at either of the returns.
            if (expected_return_value == 1) {
              assertEquals(debugger_source_position + 19,
                           exec_state.frame(0).sourcePosition());
            } else {
              assertEquals(debugger_source_position + 38,
                           exec_state.frame(0).sourcePosition());
            }
            break;
          default:
            fail("Unexpected");
        }
        exec_state.prepareStep(Debug.StepAction.StepIn, 1);
      } else {
        // Position at the end of the function.
        assertEquals(debugger_source_position + 50,
        exec_state.frame(0).sourcePosition());

        // Just about to return from the function.
        assertTrue(exec_state.frame(0).isAtReturn())
        assertEquals(expected_return_value,
                     exec_state.frame(0).returnValue().value());

        // Check the same using the JSON commands.
        var dcp = exec_state.debugCommandProcessor(false);
        var request = '{"seq":0,"type":"request","command":"backtrace"}';
        var resp = dcp.processDebugJSONRequest(request);
        response = new ParsedResponse(resp);
        frames = response.body().frames;
        assertTrue(frames[0].atReturn);
        assertEquals(expected_return_value,
                     response.lookup(frames[0].returnValue.ref).value);

        listener_complete = true;
      }
    }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.setListener(listener);

// Four steps from the debugger statement in this function will position us at
// the function return.
//             0         1         2         3         4         5
//             0123456789012345678901234567890123456789012345678901

function f(x) {debugger; if (x) { return 1; } else { return 2; } };

// Call f expecting different return values.
break_count = 0;
expected_return_value = 2;
listener_complete = false;
f();
assertFalse(exception, "exception in listener")
assertTrue(listener_complete);
assertEquals(4, break_count);

break_count = 0;
expected_return_value = 1;
listener_complete = false;
f(true);
assertFalse(exception, "exception in listener")
assertTrue(listener_complete);
assertEquals(4, break_count);

break_count = 0;
expected_return_value = 2;
listener_complete = false;
f(false);
assertFalse(exception, "exception in listener")
assertTrue(listener_complete);
assertEquals(4, break_count);
