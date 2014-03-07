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
var Script = require('vm').Script;

common.globalCheck = false;

common.debug('run a string');
var script = new Script('\'passed\';');
common.debug('script created');
var result1 = script.runInNewContext();
var result2 = script.runInNewContext();
assert.equal('passed', result1);
assert.equal('passed', result2);

common.debug('thrown error');
script = new Script('throw new Error(\'test\');');
assert.throws(function() {
  script.runInNewContext();
});



common.debug('undefined reference');
var error;
script = new Script('foo.bar = 5;');
try {
  script.runInNewContext();
} catch (e) {
  error = e;
}
assert.ok(error);
assert.ok(error.message.indexOf('not defined') >= 0);

common.debug('error.message: ' + error.message);


hello = 5;
script = new Script('hello = 2');
script.runInNewContext();
assert.equal(5, hello);


common.debug('pass values in and out');
code = 'foo = 1;' +
       'bar = 2;' +
       'if (baz !== 3) throw new Error(\'test fail\');';
foo = 2;
obj = { foo: 0, baz: 3 };
script = new Script(code);
var baz = script.runInNewContext(obj);
assert.equal(1, obj.foo);
assert.equal(2, obj.bar);
assert.equal(2, foo);

common.debug('call a function by reference');
script = new Script('f()');
function changeFoo() { foo = 100 }
script.runInNewContext({ f: changeFoo });
assert.equal(foo, 100);

common.debug('modify an object by reference');
script = new Script('f.a = 2');
var f = { a: 1 };
script.runInNewContext({ f: f });
assert.equal(f.a, 2);

common.debug('invalid this');
assert.throws(function() {
  script.runInNewContext.call('\'hello\';');
});


