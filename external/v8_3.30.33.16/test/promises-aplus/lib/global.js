// Copyright 2014 the V8 project authors. All rights reserved.
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

var global = this.global || {};
var setTimeout;
var clearTimeout;

(function() {
var timers = {};
var currentId = 0;

function PostMicrotask(fn) {
  var o = {};
  Object.observe(o, function() {
    fn();
  });
  // Change something to enqueue a microtask.
  o.x = 'hello';
}

setInterval = function(fn, delay) {
  var i = 0;
  var id = currentId++;
  function loop() {
    if (!timers[id]) {
      return;
    }
    if (i++ >= delay) {
      fn();
    }
    PostMicrotask(loop);
  }
  PostMicrotask(loop);
  timers[id] = true;
  return id;
}

clearTimeout = function(id) {
  delete timers[id];
}

clearInterval = clearTimeout;

setTimeout = function(fn, delay) {
  var id = setInterval(function() {
    fn();
    clearInterval(id);
  }, delay);
  return id;
}

}());
