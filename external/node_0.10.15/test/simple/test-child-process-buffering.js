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

var common = require('../common');
var assert = require('assert');

var spawn = require('child_process').spawn;

var pwd_called = false;
var childClosed = false;
var childExited = false;

function pwd(callback) {
  var output = '';
  var child = common.spawnPwd();

  child.stdout.setEncoding('utf8');
  child.stdout.on('data', function(s) {
    console.log('stdout: ' + JSON.stringify(s));
    output += s;
  });

  child.on('exit', function(c) {
    console.log('exit: ' + c);
    assert.equal(0, c);
    childExited = true;
  });

  child.on('close', function () {
    callback(output);
    pwd_called = true;
    childClosed = true;
  });
}


pwd(function(result) {
  console.dir(result);
  assert.equal(true, result.length > 1);
  assert.equal('\n', result[result.length - 1]);
});

process.on('exit', function() {
  assert.equal(true, pwd_called);
  assert.equal(true, childExited);
  assert.equal(true, childClosed);
});
