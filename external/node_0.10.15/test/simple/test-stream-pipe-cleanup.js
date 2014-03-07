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

// This test asserts that Stream.prototype.pipe does not leave listeners
// hanging on the source or dest.

var common = require('../common');
var stream = require('stream');
var assert = require('assert');
var util = require('util');

function Writable() {
  this.writable = true;
  this.endCalls = 0;
  stream.Stream.call(this);
}
util.inherits(Writable, stream.Stream);
Writable.prototype.end = function() {
  this.endCalls++;
};

Writable.prototype.destroy = function() {
  this.endCalls++;
};

function Readable() {
  this.readable = true;
  stream.Stream.call(this);
}
util.inherits(Readable, stream.Stream);

function Duplex() {
  this.readable = true;
  Writable.call(this);
}
util.inherits(Duplex, Writable);

var i = 0;
var limit = 100;

var w = new Writable();

var r;

for (i = 0; i < limit; i++) {
  r = new Readable();
  r.pipe(w);
  r.emit('end');
}
assert.equal(0, r.listeners('end').length);
assert.equal(limit, w.endCalls);

w.endCalls = 0;

for (i = 0; i < limit; i++) {
  r = new Readable();
  r.pipe(w);
  r.emit('close');
}
assert.equal(0, r.listeners('close').length);
assert.equal(limit, w.endCalls);

w.endCalls = 0;

r = new Readable();

for (i = 0; i < limit; i++) {
  w = new Writable();
  r.pipe(w);
  w.emit('close');
}
assert.equal(0, w.listeners('close').length);

r = new Readable();
w = new Writable();
var d = new Duplex();
r.pipe(d); // pipeline A
d.pipe(w); // pipeline B
assert.equal(r.listeners('end').length, 2);   // A.onend, A.cleanup
assert.equal(r.listeners('close').length, 2); // A.onclose, A.cleanup
assert.equal(d.listeners('end').length, 2);   // B.onend, B.cleanup
assert.equal(d.listeners('close').length, 3); // A.cleanup, B.onclose, B.cleanup
assert.equal(w.listeners('end').length, 0);
assert.equal(w.listeners('close').length, 1); // B.cleanup

r.emit('end');
assert.equal(d.endCalls, 1);
assert.equal(w.endCalls, 0);
assert.equal(r.listeners('end').length, 0);
assert.equal(r.listeners('close').length, 0);
assert.equal(d.listeners('end').length, 2);   // B.onend, B.cleanup
assert.equal(d.listeners('close').length, 2); // B.onclose, B.cleanup
assert.equal(w.listeners('end').length, 0);
assert.equal(w.listeners('close').length, 1); // B.cleanup

d.emit('end');
assert.equal(d.endCalls, 1);
assert.equal(w.endCalls, 1);
assert.equal(r.listeners('end').length, 0);
assert.equal(r.listeners('close').length, 0);
assert.equal(d.listeners('end').length, 0);
assert.equal(d.listeners('close').length, 0);
assert.equal(w.listeners('end').length, 0);
assert.equal(w.listeners('close').length, 0);
