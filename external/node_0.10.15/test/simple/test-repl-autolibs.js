// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

var assert = require('assert');
var util = require('util');
var repl = require('repl');

// A stream to push an array into a REPL
function ArrayStream() {
  this.run = function (data) {
    var self = this;
    data.forEach(function (line) {
      self.emit('data', line + '\n');
    });
  }
}
util.inherits(ArrayStream, require('stream').Stream);
ArrayStream.prototype.readable = true;
ArrayStream.prototype.writable = true;
ArrayStream.prototype.resume = function () {};
ArrayStream.prototype.write = function () {};

var putIn = new ArrayStream;
var testMe = repl.start('', putIn, null, true);

test1();

function test1(){
  var gotWrite = false;
  putIn.write = function (data) {
    gotWrite = true;
    if (data.length) {

      // inspect output matches repl output
      assert.equal(data, util.inspect(require('fs'), null, 2, false) + '\n');
      // globally added lib matches required lib
      assert.equal(global.fs, require('fs'));
      test2();
    }
  };
  assert(!gotWrite);
  putIn.run(['fs']);
  assert(gotWrite);
}

function test2(){
  var gotWrite = false;
  putIn.write = function(data) {
    gotWrite = true;
    if (data.length) {
      // repl response error message
      assert.equal(data, '{}\n');
      // original value wasn't overwritten
      assert.equal(val, global.url);
    }
  };
  var val = {};
  global.url = val;
  assert(!gotWrite);
  putIn.run(['url']);
  assert(gotWrite);
}
