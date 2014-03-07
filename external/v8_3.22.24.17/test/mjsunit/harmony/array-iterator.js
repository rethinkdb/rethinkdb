// Copyright 2013 the V8 project authors. All rights reserved.
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

// Flags: --harmony-iteration --allow-natives-syntax

function TestArrayPrototype() {
  assertTrue(Array.prototype.hasOwnProperty('entries'));
  assertTrue(Array.prototype.hasOwnProperty('values'));
  assertTrue(Array.prototype.hasOwnProperty('keys'));

  assertFalse(Array.prototype.propertyIsEnumerable('entries'));
  assertFalse(Array.prototype.propertyIsEnumerable('values'));
  assertFalse(Array.prototype.propertyIsEnumerable('keys'));
}
TestArrayPrototype();

function assertIteratorResult(value, done, result) {
  assertEquals({value: value, done: done}, result);
}

function TestValues() {
  var array = ['a', 'b', 'c'];
  var iterator = array.values();
  assertIteratorResult('a', false, iterator.next());
  assertIteratorResult('b', false, iterator.next());
  assertIteratorResult('c', false, iterator.next());
  assertIteratorResult(void 0, true, iterator.next());

  array.push('d');
  assertIteratorResult(void 0, true, iterator.next());
}
TestValues();

function TestValuesMutate() {
  var array = ['a', 'b', 'c'];
  var iterator = array.values();
  assertIteratorResult('a', false, iterator.next());
  assertIteratorResult('b', false, iterator.next());
  assertIteratorResult('c', false, iterator.next());
  array.push('d');
  assertIteratorResult('d', false, iterator.next());
  assertIteratorResult(void 0, true, iterator.next());
}
TestValuesMutate();

function TestKeys() {
  var array = ['a', 'b', 'c'];
  var iterator = array.keys();
  assertIteratorResult(0, false, iterator.next());
  assertIteratorResult(1, false, iterator.next());
  assertIteratorResult(2, false, iterator.next());
  assertIteratorResult(void 0, true, iterator.next());

  array.push('d');
  assertIteratorResult(void 0, true, iterator.next());
}
TestKeys();

function TestKeysMutate() {
  var array = ['a', 'b', 'c'];
  var iterator = array.keys();
  assertIteratorResult(0, false, iterator.next());
  assertIteratorResult(1, false, iterator.next());
  assertIteratorResult(2, false, iterator.next());
  array.push('d');
  assertIteratorResult(3, false, iterator.next());
  assertIteratorResult(void 0, true, iterator.next());
}
TestKeysMutate();

function TestEntries() {
  var array = ['a', 'b', 'c'];
  var iterator = array.entries();
  assertIteratorResult([0, 'a'], false, iterator.next());
  assertIteratorResult([1, 'b'], false, iterator.next());
  assertIteratorResult([2, 'c'], false, iterator.next());
  assertIteratorResult(void 0, true, iterator.next());

  array.push('d');
  assertIteratorResult(void 0, true, iterator.next());
}
TestEntries();

function TestEntriesMutate() {
  var array = ['a', 'b', 'c'];
  var iterator = array.entries();
  assertIteratorResult([0, 'a'], false, iterator.next());
  assertIteratorResult([1, 'b'], false, iterator.next());
  assertIteratorResult([2, 'c'], false, iterator.next());
  array.push('d');
  assertIteratorResult([3, 'd'], false, iterator.next());
  assertIteratorResult(void 0, true, iterator.next());
}
TestEntriesMutate();

function TestArrayIteratorPrototype() {
  var array = [];
  var iterator = array.values();

  var ArrayIterator = iterator.constructor;
  assertEquals(ArrayIterator.prototype, array.values().__proto__);
  assertEquals(ArrayIterator.prototype, array.keys().__proto__);
  assertEquals(ArrayIterator.prototype, array.entries().__proto__);

  assertEquals(Object.prototype, ArrayIterator.prototype.__proto__);

  assertEquals('Array Iterator', %_ClassOf(array.values()));
  assertEquals('Array Iterator', %_ClassOf(array.keys()));
  assertEquals('Array Iterator', %_ClassOf(array.entries()));

  var prototypeDescriptor =
      Object.getOwnPropertyDescriptor(ArrayIterator, 'prototype');
  assertFalse(prototypeDescriptor.configurable);
  assertFalse(prototypeDescriptor.enumerable);
  assertFalse(prototypeDescriptor.writable);
}
TestArrayIteratorPrototype();

function TestForArrayValues() {
  var buffer = [];
  var array = [0, 'a', true, false, null, /* hole */, undefined, NaN];
  var i = 0;
  for (var value of array.values()) {
    buffer[i++] = value;
  }

  assertEquals(8, buffer.length);

  for (var i = 0; i < buffer.length - 1; i++) {
    assertEquals(array[i], buffer[i]);
  }
  assertTrue(isNaN(buffer[buffer.length - 1]));
}
TestForArrayValues();

function TestForArrayKeys() {
  var buffer = [];
  var array = [0, 'a', true, false, null, /* hole */, undefined, NaN];
  var i = 0;
  for (var key of array.keys()) {
    buffer[i++] = key;
  }

  assertEquals(8, buffer.length);

  for (var i = 0; i < buffer.length; i++) {
    assertEquals(i, buffer[i]);
  }
}
TestForArrayKeys();

function TestForArrayEntries() {
  var buffer = [];
  var array = [0, 'a', true, false, null, /* hole */, undefined, NaN];
  var i = 0;
  for (var entry of array.entries()) {
    buffer[i++] = entry;
  }

  assertEquals(8, buffer.length);

  for (var i = 0; i < buffer.length - 1; i++) {
    assertEquals(array[i], buffer[i][1]);
  }
  assertTrue(isNaN(buffer[buffer.length - 1][1]));

  for (var i = 0; i < buffer.length; i++) {
    assertEquals(i, buffer[i][0]);
  }
}
TestForArrayEntries();
