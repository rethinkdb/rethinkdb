// Copyright 2012 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Unit tests for goog.proto2.TextFormatSerializer
 *
 */

goog.provide('goog.proto2.TextFormatSerializerTest');

goog.require('goog.proto2.TextFormatSerializer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.recordFunction');
goog.require('proto2.TestAllTypes');

goog.setTestOnly('goog.proto2.TextFormatSerializerTest');

function testSerialization() {
  var message = new proto2.TestAllTypes();

  // Set the fields.
  // Singular.
  message.setOptionalInt32(101);
  message.setOptionalUint32(103);
  message.setOptionalSint32(105);
  message.setOptionalFixed32(107);
  message.setOptionalSfixed32(109);
  message.setOptionalInt64('102');
  message.setOptionalFloat(111.5);
  message.setOptionalDouble(112.5);
  message.setOptionalBool(true);
  message.setOptionalString('test');
  message.setOptionalBytes('abcd');

  var group = new proto2.TestAllTypes.OptionalGroup();
  group.setA(111);

  message.setOptionalgroup(group);

  var nestedMessage = new proto2.TestAllTypes.NestedMessage();
  nestedMessage.setB(112);

  message.setOptionalNestedMessage(nestedMessage);

  message.setOptionalNestedEnum(proto2.TestAllTypes.NestedEnum.FOO);

  // Repeated.
  message.addRepeatedInt32(201);
  message.addRepeatedInt32(202);

  // Serialize to a simplified text format.
  var simplified = new goog.proto2.TextFormatSerializer().serialize(message);
  var expected = 'optional_int32: 101\n' +
      'optional_int64: 102\n' +
      'optional_uint32: 103\n' +
      'optional_sint32: 105\n' +
      'optional_fixed32: 107\n' +
      'optional_sfixed32: 109\n' +
      'optional_float: 111.5\n' +
      'optional_double: 112.5\n' +
      'optional_bool: true\n' +
      'optional_string: "test"\n' +
      'optional_bytes: "abcd"\n' +
      'optionalgroup {\n' +
      '  a: 111\n' +
      '}\n' +
      'optional_nested_message {\n' +
      '  b: 112\n' +
      '}\n' +
      'optional_nested_enum: FOO\n' +
      'repeated_int32: 201\n' +
      'repeated_int32: 202\n';

  assertEquals(expected, simplified);
}

function testSerializationOfUnknown() {
  var nestedUnknown = new proto2.TestAllTypes();
  var message = new proto2.TestAllTypes();

  // Set the fields.
  // Known.
  message.setOptionalInt32(101);
  message.addRepeatedInt32(201);
  message.addRepeatedInt32(202);

  nestedUnknown.addRepeatedInt32(301);
  nestedUnknown.addRepeatedInt32(302);

  // Unknown.
  message.setUnknown(1000, 301);
  message.setUnknown(1001, 302);
  message.setUnknown(1002, 'hello world');
  message.setUnknown(1002, nestedUnknown);

  nestedUnknown.setUnknown(2000, 401);

  // Serialize.
  var simplified = new goog.proto2.TextFormatSerializer().serialize(message);
  var expected = 'optional_int32: 101\n' +
      'repeated_int32: 201\n' +
      'repeated_int32: 202\n' +
      '1000: 301\n' +
      '1001: 302\n' +
      '1002 {\n' +
      '  repeated_int32: 301\n' +
      '  repeated_int32: 302\n' +
      '  2000: 401\n' +
      '}';

  assertEquals(expected, simplified);
}

/**
 * Asserts that the given string value parses into the given set of tokens.
 * @param {string} value The string value to parse.
 * @param {Array.<Object> | Object} tokens The tokens to check against. If not
 *     an array, a single token is expected.
 * @param {boolean=} opt_ignoreWhitespace Whether whitespace tokens should be
 *     skipped by the tokenizer.
 */
function assertTokens(value, tokens, opt_ignoreWhitespace) {
  var tokenizer = new goog.proto2.TextFormatSerializer.Tokenizer_(
      value, opt_ignoreWhitespace);
  var tokensFound = [];

  while (tokenizer.next()) {
    tokensFound.push(tokenizer.getCurrent());
  }

  if (goog.typeOf(tokens) != 'array') {
    tokens = [tokens];
  }

  assertEquals(tokens.length, tokensFound.length);
  for (var i = 0; i < tokens.length; ++i) {
    assertToken(tokens[i], tokensFound[i]);
  }
}

function assertToken(expected, found) {
  assertEquals(expected.type, found.type);
  if (expected.value) {
    assertEquals(expected.value, found.value);
  }
}

function testTokenizer() {
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens('{ 123 }', [
      { type: types.OPEN_BRACE },
      { type: types.WHITESPACE, value: ' ' },
      { type: types.NUMBER, value: '123' },
      { type: types.WHITESPACE, value: ' '},
      { type: types.CLOSE_BRACE }
  ]);
}

function testTokenizerNoWhitespace() {
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens('{ "hello world" }', [
      { type: types.OPEN_BRACE },
      { type: types.STRING, value: '"hello world"' },
      { type: types.CLOSE_BRACE }
  ], true);
}


function assertIdentifier(identifier) {
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens(identifier, { type: types.IDENTIFIER, value: identifier });
}

function assertComment(comment) {
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens(comment, { type: types.COMMENT, value: comment });
}

function assertString(str) {
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens(str, { type: types.STRING, value: str });
}

function assertNumber(num) {
  num = num.toString();
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens(num, { type: types.NUMBER, value: num });
}

function testTokenizerSingleTokens() {
  var types = goog.proto2.TextFormatSerializer.Tokenizer_.TokenTypes;
  assertTokens('{', { type: types.OPEN_BRACE });
  assertTokens('}', { type: types.CLOSE_BRACE });
  assertTokens('<', { type: types.OPEN_TAG });
  assertTokens('>', { type: types.CLOSE_TAG });
  assertTokens(':', { type: types.COLON });
  assertTokens(',', { type: types.COMMA });
  assertTokens(';', { type: types.SEMI });

  assertIdentifier('abcd');
  assertIdentifier('Abcd');
  assertIdentifier('ABcd');
  assertIdentifier('ABcD');
  assertIdentifier('a123nc');
  assertIdentifier('a45_bC');
  assertIdentifier('A45_bC');

  assertIdentifier('inf');
  assertIdentifier('infinity');
  assertIdentifier('nan');

  assertNumber(0);
  assertNumber(10);
  assertNumber(123);
  assertNumber(1234);
  assertNumber(123.56);
  assertNumber(-124);
  assertNumber(-1234);
  assertNumber(-123.56);
  assertNumber('123f');
  assertNumber('123.6f');
  assertNumber('-123f');
  assertNumber('-123.8f');
  assertNumber('0x1234');
  assertNumber('0x12ac34');
  assertNumber('0x49e281db686fb');

  assertString('""');
  assertString('"hello world"');
  assertString('"hello # world"');
  assertString('"hello #\\" world"');
  assertString('"|"');
  assertString('"\\"\\""');
  assertString('"\\"foo\\""');
  assertString('"\\"foo\\" and \\"bar\\""');
  assertString('"foo \\"and\\" bar"');

  assertComment('# foo bar baz');
  assertComment('# foo ## bar baz');
  assertComment('# foo "bar" baz');
}

function testSerializationOfStringWithQuotes() {
  var nestedUnknown = new proto2.TestAllTypes();
  var message = new proto2.TestAllTypes();
  message.setOptionalString('hello "world"');

  // Serialize.
  var simplified = new goog.proto2.TextFormatSerializer().serialize(message);
  var expected = 'optional_string: "hello \\"world\\""\n';
  assertEquals(expected, simplified);
}

function testDeserialization() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
     'repeated_int32: 201\n' +
     'repeated_int32: 202\n' +
     'optional_float: 123.4';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(101, message.getOptionalInt32());
  assertEquals(201, message.getRepeatedInt32(0));
  assertEquals(202, message.getRepeatedInt32(1));
  assertEquals(123.4, message.getOptionalFloat());
}

function testDeserializationOfList() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'repeated_int32: [201, 202]\n' +
      'optional_float: 123.4';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(101, message.getOptionalInt32());
  assertEquals(201, message.getRepeatedInt32(0));
  assertEquals(123.4, message.getOptionalFloat());
}

function testDeserializationOfIntegerAsHexadecimalString() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 0x1\n' +
      'optional_sint32: 0xf\n' +
      'optional_uint32: 0xffffffff\n' +
      'repeated_int32: [0x0, 0xff]\n';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(1, message.getOptionalInt32());
  assertEquals(15, message.getOptionalSint32());
  assertEquals(4294967295, message.getOptionalUint32());
  assertEquals(0, message.getRepeatedInt32(0));
  assertEquals(255, message.getRepeatedInt32(1));
}

function testDeserializationOfInt64AsHexadecimalString() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int64: 0xf';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals('0xf', message.getOptionalInt64());
}

function testDeserializationOfZeroFalseAndEmptyString() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 0\n' +
      'optional_bool: false\n' +
      'optional_string: ""';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(0, message.getOptionalInt32());
  assertEquals(false, message.getOptionalBool());
  assertEquals('', message.getOptionalString());
}

function testDeserializationSkipUnknown() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'repeated_int32: 201\n' +
      'some_unknown: true\n' +
      'repeated_int32: 202\n' +
      'optional_float: 123.4';

  var parser = new goog.proto2.TextFormatSerializer.Parser();
  assertTrue(parser.parse(message, value, true));

  assertEquals(101, message.getOptionalInt32());
  assertEquals(201, message.getRepeatedInt32(0));
  assertEquals(202, message.getRepeatedInt32(1));
  assertEquals(123.4, message.getOptionalFloat());
}

function testDeserializationSkipUnknownList() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'repeated_int32: 201\n' +
      'some_unknown: [true, 1, 201, "hello"]\n' +
      'repeated_int32: 202\n' +
      'optional_float: 123.4';

  var parser = new goog.proto2.TextFormatSerializer.Parser();
  assertTrue(parser.parse(message, value, true));

  assertEquals(101, message.getOptionalInt32());
  assertEquals(201, message.getRepeatedInt32(0));
  assertEquals(202, message.getRepeatedInt32(1));
  assertEquals(123.4, message.getOptionalFloat());
}

function testDeserializationSkipUnknownNested() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'repeated_int32: 201\n' +
      'some_unknown: <\n' +
      '  a: 1\n' +
      '  b: 2\n' +
      '>\n' +
      'repeated_int32: 202\n' +
      'optional_float: 123.4';

  var parser = new goog.proto2.TextFormatSerializer.Parser();
  assertTrue(parser.parse(message, value, true));

  assertEquals(101, message.getOptionalInt32());
  assertEquals(201, message.getRepeatedInt32(0));
  assertEquals(202, message.getRepeatedInt32(1));
  assertEquals(123.4, message.getOptionalFloat());
}

function testDeserializationSkipUnknownNestedInvalid() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'repeated_int32: 201\n' +
      'some_unknown: <\n' +
      '  a: \n' + // Missing value.
      '  b: 2\n' +
      '>\n' +
      'repeated_int32: 202\n' +
      'optional_float: 123.4';

  var parser = new goog.proto2.TextFormatSerializer.Parser();
  assertFalse(parser.parse(message, value, true));
}

function testDeserializationSkipUnknownNestedInvalid2() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'repeated_int32: 201\n' +
      'some_unknown: <\n' +
      '  a: 2\n' +
      '  b: 2\n' +
      '}\n' + // Delimiter mismatch
      'repeated_int32: 202\n' +
      'optional_float: 123.4';

  var parser = new goog.proto2.TextFormatSerializer.Parser();
  assertFalse(parser.parse(message, value, true));
}


function testDeserializationLegacyFormat() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101,\n' +
      'repeated_int32: 201,\n' +
      'repeated_int32: 202;\n' +
      'optional_float: 123.4';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(101, message.getOptionalInt32());
  assertEquals(201, message.getRepeatedInt32(0));
  assertEquals(202, message.getRepeatedInt32(1));
  assertEquals(123.4, message.getOptionalFloat());
}

function testDeserializationVariedNumbers() {
  var message = new proto2.TestAllTypes();
  var value = (
      'repeated_int32: 23\n' +
      'repeated_int32: -3\n' +
      'repeated_int32: 0xdeadbeef\n' +
      'repeated_float: 123.0\n' +
      'repeated_float: -3.27\n' +
      'repeated_float: -35.5f\n'
      );

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(23, message.getRepeatedInt32(0));
  assertEquals(-3, message.getRepeatedInt32(1));
  assertEquals(3735928559, message.getRepeatedInt32(2));
  assertEquals(123.0, message.getRepeatedFloat(0));
  assertEquals(-3.27, message.getRepeatedFloat(1));
  assertEquals(-35.5, message.getRepeatedFloat(2));
}

function testParseNumericalConstant() {
  var parseNumericalConstant =
      goog.proto2.TextFormatSerializer.Parser.parseNumericalConstant_;

  assertEquals(Infinity, parseNumericalConstant('inf'));
  assertEquals(Infinity, parseNumericalConstant('inff'));
  assertEquals(Infinity, parseNumericalConstant('infinity'));
  assertEquals(Infinity, parseNumericalConstant('infinityf'));
  assertEquals(Infinity, parseNumericalConstant('Infinityf'));

  assertEquals(-Infinity, parseNumericalConstant('-inf'));
  assertEquals(-Infinity, parseNumericalConstant('-inff'));
  assertEquals(-Infinity, parseNumericalConstant('-infinity'));
  assertEquals(-Infinity, parseNumericalConstant('-infinityf'));
  assertEquals(-Infinity, parseNumericalConstant('-Infinity'));

  assertNull(parseNumericalConstant('-infin'));
  assertNull(parseNumericalConstant('infin'));
  assertNull(parseNumericalConstant('-infinite'));

  assertNull(parseNumericalConstant('-infin'));
  assertNull(parseNumericalConstant('infin'));
  assertNull(parseNumericalConstant('-infinite'));

  assertTrue(isNaN(parseNumericalConstant('Nan')));
  assertTrue(isNaN(parseNumericalConstant('NaN')));
  assertTrue(isNaN(parseNumericalConstant('NAN')));
  assertTrue(isNaN(parseNumericalConstant('nan')));
  assertTrue(isNaN(parseNumericalConstant('nanf')));
  assertTrue(isNaN(parseNumericalConstant('NaNf')));

  assertEquals(Number.POSITIVE_INFINITY, parseNumericalConstant('infinity'));
  assertEquals(Number.NEGATIVE_INFINITY, parseNumericalConstant('-inf'));
  assertEquals(Number.NEGATIVE_INFINITY, parseNumericalConstant('-infinity'));

  assertNull(parseNumericalConstant('na'));
  assertNull(parseNumericalConstant('-nan'));
  assertNull(parseNumericalConstant('none'));
}

function testDeserializationOfNumericalConstants() {

  var message = new proto2.TestAllTypes();
  var value = (
      'repeated_float: inf\n' +
      'repeated_float: -inf\n' +
      'repeated_float: nan\n' +
      'repeated_float: 300.2\n'
      );

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(Infinity, message.getRepeatedFloat(0));
  assertEquals(-Infinity, message.getRepeatedFloat(1));
  assertTrue(isNaN(message.getRepeatedFloat(2)));
  assertEquals(300.2, message.getRepeatedFloat(3));
}

function testGetNumberFromString() {
  var getNumberFromString =
      goog.proto2.TextFormatSerializer.Parser.getNumberFromString_;

  assertEquals(3735928559, getNumberFromString('0xdeadbeef'));
  assertEquals(4276215469, getNumberFromString('0xFEE1DEAD'));
  assertEquals(123.1, getNumberFromString('123.1'));
  assertEquals(123.0, getNumberFromString('123.0'));
  assertEquals(-29.3, getNumberFromString('-29.3f'));
  assertEquals(23, getNumberFromString('23'));
  assertEquals(-3, getNumberFromString('-3'));
  assertEquals(-3.27, getNumberFromString('-3.27'));

  assertThrows(goog.partial(getNumberFromString, 'cat'));
  assertThrows(goog.partial(getNumberFromString, 'NaN'));
  assertThrows(goog.partial(getNumberFromString, 'inf'));
}

function testDeserializationError() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int33: 101\n';
  var result =
      new goog.proto2.TextFormatSerializer().deserializeTo(message, value);
  assertEquals(result, 'Unknown field: optional_int33');
}

function testNestedDeserialization() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'optional_nested_message: {\n' +
      '  b: 301\n' +
      '}';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(101, message.getOptionalInt32());
  assertEquals(301, message.getOptionalNestedMessage().getB());
}

function testNestedDeserializationLegacyFormat() {
  var message = new proto2.TestAllTypes();
  var value = 'optional_int32: 101\n' +
      'optional_nested_message: <\n' +
      '  b: 301\n' +
      '>';

  new goog.proto2.TextFormatSerializer().deserializeTo(message, value);

  assertEquals(101, message.getOptionalInt32());
  assertEquals(301, message.getOptionalNestedMessage().getB());
}

function testBidirectional() {
  var message = new proto2.TestAllTypes();

  // Set the fields.
  // Singular.
  message.setOptionalInt32(101);
  message.setOptionalInt64('102');
  message.setOptionalUint32(103);
  message.setOptionalUint64('104');
  message.setOptionalSint32(105);
  message.setOptionalSint64('106');
  message.setOptionalFixed32(107);
  message.setOptionalFixed64('108');
  message.setOptionalSfixed32(109);
  message.setOptionalSfixed64('110');
  message.setOptionalFloat(111.5);
  message.setOptionalDouble(112.5);
  message.setOptionalBool(true);
  message.setOptionalString('test');
  message.setOptionalBytes('abcd');

  var group = new proto2.TestAllTypes.OptionalGroup();
  group.setA(111);

  message.setOptionalgroup(group);

  var nestedMessage = new proto2.TestAllTypes.NestedMessage();
  nestedMessage.setB(112);

  message.setOptionalNestedMessage(nestedMessage);

  message.setOptionalNestedEnum(proto2.TestAllTypes.NestedEnum.FOO);

  // Repeated.
  message.addRepeatedInt32(201);
  message.addRepeatedInt32(202);
  message.addRepeatedString('hello "world"');

  // Serialize the message to text form.
  var serializer = new goog.proto2.TextFormatSerializer();
  var textform = serializer.serialize(message);

  // Create a copy and deserialize into the copy.
  var copy = new proto2.TestAllTypes();
  serializer.deserializeTo(copy, textform);

  // Assert that the messages are structurally equivalent.
  assertTrue(copy.equals(message));
}


function testBidirectional64BitNumber() {
  var message = new proto2.TestAllTypes();
  message.setOptionalInt64Number(10000000);
  message.setOptionalInt64String('200000000000000000');

  // Serialize the message to text form.
  var serializer = new goog.proto2.TextFormatSerializer();
  var textform = serializer.serialize(message);

  // Create a copy and deserialize into the copy.
  var copy = new proto2.TestAllTypes();
  serializer.deserializeTo(copy, textform);

  // Assert that the messages are structurally equivalent.
  assertTrue(copy.equals(message));
}
