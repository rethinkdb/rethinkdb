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

// Copyright (C) 2011 by Ben Noordhuis <info@bnoordhuis.nl>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

var common = require('../common');
var punycode = require('punycode');
var assert = require('assert');

assert.equal(punycode.encode('ü'), 'tda');
assert.equal(punycode.encode('Goethe'), 'Goethe-');
assert.equal(punycode.encode('Bücher'), 'Bcher-kva');
assert.equal(punycode.encode(
    'Willst du die Blüthe des frühen, die Früchte des späteren Jahres'),
    'Willst du die Blthe des frhen, die Frchte des spteren Jahres-x9e96lkal');
assert.equal(punycode.encode('日本語'), 'wgv71a119e');

assert.equal(punycode.decode('tda'), 'ü');
assert.equal(punycode.decode('Goethe-'), 'Goethe');
assert.equal(punycode.decode('Bcher-kva'), 'Bücher');
assert.equal(punycode.decode(
    'Willst du die Blthe des frhen, die Frchte des spteren Jahres-x9e96lkal'),
    'Willst du die Blüthe des frühen, die Früchte des späteren Jahres');
assert.equal(punycode.decode('wgv71a119e'), '日本語');

// http://tools.ietf.org/html/rfc3492#section-7.1
var tests = {
  // (A) Arabic (Egyptian)
  'egbpdaj6bu4bxfgehfvwxn':
      '\u0644\u064A\u0647\u0645\u0627\u0628\u062A\u0643\u0644\u0645\u0648' +
      '\u0634\u0639\u0631\u0628\u064A\u061F',

  // (B) Chinese (simplified)
  'ihqwcrb4cv8a8dqg056pqjye':
      '\u4ED6\u4EEC\u4E3A\u4EC0\u4E48\u4E0D\u8BF4\u4E2D\u6587',

  // (C) Chinese (traditional)
  'ihqwctvzc91f659drss3x8bo0yb':
      '\u4ED6\u5011\u7232\u4EC0\u9EBD\u4E0D\u8AAA\u4E2D\u6587',

  // (D) Czech: Pro<ccaron>prost<ecaron>nemluv<iacute><ccaron>esky
  'Proprostnemluvesky-uyb24dma41a':
      '\u0050\u0072\u006F\u010D\u0070\u0072\u006F\u0073\u0074\u011B\u006E' +
      '\u0065\u006D\u006C\u0075\u0076\u00ED\u010D\u0065\u0073\u006B\u0079',

  // (E) Hebrew
  '4dbcagdahymbxekheh6e0a7fei0b':
      '\u05DC\u05DE\u05D4\u05D4\u05DD\u05E4\u05E9\u05D5\u05D8\u05DC\u05D0' +
      '\u05DE\u05D3\u05D1\u05E8\u05D9\u05DD\u05E2\u05D1\u05E8\u05D9\u05EA',

  // (F) Hindi (Devanagari)
  'i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd':
      '\u092F\u0939\u0932\u094B\u0917\u0939\u093F\u0928\u094D\u0926\u0940' +
      '\u0915\u094D\u092F\u094B\u0902\u0928\u0939\u0940\u0902\u092C\u094B' +
      '\u0932\u0938\u0915\u0924\u0947\u0939\u0948\u0902',

  // (G) Japanese (kanji and hiragana)
  'n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa':
      '\u306A\u305C\u307F\u3093\u306A\u65E5\u672C\u8A9E\u3092\u8A71\u3057' +
      '\u3066\u304F\u308C\u306A\u3044\u306E\u304B',

  // (H) Korean (Hangul syllables)
  '989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c':
      '\uC138\uACC4\uC758\uBAA8\uB4E0\uC0AC\uB78C\uB4E4\uC774\uD55C\uAD6D' +
      '\uC5B4\uB97C\uC774\uD574\uD55C\uB2E4\uBA74\uC5BC\uB9C8\uB098\uC88B' +
      '\uC744\uAE4C',

  // (I) Russian (Cyrillic)
  /* XXX disabled, fails - possibly a bug in the RFC
  'b1abfaaepdrnnbgefbaDotcwatmq2g4l':
      '\u043F\u043E\u0447\u0435\u043C\u0443\u0436\u0435\u043E\u043D\u0438' +
      '\u043D\u0435\u0433\u043E\u0432\u043E\u0440\u044F\u0442\u043F\u043E' +
      '\u0440\u0443\u0441\u0441\u043A\u0438',
  */

  // (J) Spanish: Porqu<eacute>nopuedensimplementehablarenEspa<ntilde>ol
  'PorqunopuedensimplementehablarenEspaol-fmd56a':
      '\u0050\u006F\u0072\u0071\u0075\u00E9\u006E\u006F\u0070\u0075\u0065' +
      '\u0064\u0065\u006E\u0073\u0069\u006D\u0070\u006C\u0065\u006D\u0065' +
      '\u006E\u0074\u0065\u0068\u0061\u0062\u006C\u0061\u0072\u0065\u006E' +
      '\u0045\u0073\u0070\u0061\u00F1\u006F\u006C',

  // (K) Vietnamese: T<adotbelow>isaoh<odotbelow>kh<ocirc>ngth
  // <ecirchookabove>ch<ihookabove>n<oacute>iti<ecircacute>ngVi<ecircdotbelow>t
  'TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g':
      '\u0054\u1EA1\u0069\u0073\u0061\u006F\u0068\u1ECD\u006B\u0068\u00F4' +
      '\u006E\u0067\u0074\u0068\u1EC3\u0063\u0068\u1EC9\u006E\u00F3\u0069' +
      '\u0074\u0069\u1EBF\u006E\u0067\u0056\u0069\u1EC7\u0074',

  // (L) 3<nen>B<gumi><kinpachi><sensei>
  '3B-ww4c5e180e575a65lsy2b':
      '\u0033\u5E74\u0042\u7D44\u91D1\u516B\u5148\u751F',

  // (M) <amuro><namie>-with-SUPER-MONKEYS
  '-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n':
      '\u5B89\u5BA4\u5948\u7F8E\u6075\u002D\u0077\u0069\u0074\u0068\u002D' +
      '\u0053\u0055\u0050\u0045\u0052\u002D\u004D\u004F\u004E\u004B\u0045' +
      '\u0059\u0053',

  // (N) Hello-Another-Way-<sorezore><no><basho>
  'Hello-Another-Way--fc4qua05auwb3674vfr0b':
      '\u0048\u0065\u006C\u006C\u006F\u002D\u0041\u006E\u006F\u0074\u0068' +
      '\u0065\u0072\u002D\u0057\u0061\u0079\u002D\u305D\u308C\u305E\u308C' +
      '\u306E\u5834\u6240',

  // (O) <hitotsu><yane><no><shita>2
  '2-u9tlzr9756bt3uc0v':
      '\u3072\u3068\u3064\u5C4B\u6839\u306E\u4E0B\u0032',

  // (P) Maji<de>Koi<suru>5<byou><mae>
  'MajiKoi5-783gue6qz075azm5e':
      '\u004D\u0061\u006A\u0069\u3067\u004B\u006F\u0069\u3059\u308B\u0035' +
      '\u79D2\u524D',

  // (Q) <pafii>de<runba>
  'de-jg4avhby1noc0d':
      '\u30D1\u30D5\u30A3\u30FC\u0064\u0065\u30EB\u30F3\u30D0',

  // (R) <sono><supiido><de>
  'd9juau41awczczp':
      '\u305D\u306E\u30B9\u30D4\u30FC\u30C9\u3067',

  // (S) -> $1.00 <-
  '-> $1.00 <--':
      '\u002D\u003E\u0020\u0024\u0031\u002E\u0030\u0030\u0020\u003C\u002D'
};

var errors = 0;

for (var encoded in tests) {
  var decoded = tests[encoded];
  try {
    assert.equal(punycode.encode(decoded), encoded);
  } catch (e) {
    console.error('FAIL: expected %j, got %j', e.expected, e.actual);
    errors++;
  }
  try {
    assert.equal(punycode.decode(encoded), decoded);
  } catch (e) {
    console.error('FAIL: expected %j, got %j', e.expected, e.actual);
    errors++;
  }
}

// BMP code point
assert.equal(punycode.ucs2.encode([0x61]), 'a');
// supplementary code point (surrogate pair)
assert.equal(punycode.ucs2.encode([0x1D306]), '\uD834\uDF06');
// high surrogate
assert.equal(punycode.ucs2.encode([0xD800]), '\uD800');
// high surrogate followed by non-surrogates
assert.equal(punycode.ucs2.encode([0xD800, 0x61, 0x62]), '\uD800ab');
// low surrogate
assert.equal(punycode.ucs2.encode([0xDC00]), '\uDC00');
// low surrogate followed by non-surrogates
assert.equal(punycode.ucs2.encode([0xDC00, 0x61, 0x62]), '\uDC00ab');

assert.equal(errors, 0);
