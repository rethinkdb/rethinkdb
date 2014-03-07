var B = require('../index.js').Buffer
var test = require('tape')

test('detect utf16 surrogate pairs', function(t) {
  t.plan(1)
  var text = '\uD83D\uDE38' + '\uD83D\uDCAD' + '\uD83D\uDC4D'
  var buf = new B(text)
  t.equal(text, buf.toString())
  t.end()
})

test('throw on orphaned utf16 surrogate lead code point', function(t) {
  t.plan(1)
  var text = '\uD83D\uDE38' + '\uD83D' + '\uD83D\uDC4D'
  var err
  try {
    var buf = new B(text)
  } catch (e) {
    err = e
  }
  t.equal(err instanceof URIError, true)
  t.end()
})

test('throw on orphaned utf16 surrogate trail code point', function(t) {
  t.plan(1)
  var text = '\uD83D\uDE38' + '\uDCAD' + '\uD83D\uDC4D'
  var err
  try {
    var buf = new B(text)
  } catch (e) {
    err = e
  }
  t.equal(err instanceof URIError, true)
  t.end()
})