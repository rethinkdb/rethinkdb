
var B = require('../index.js').Buffer
var test = require('tape')

test('Buffer.isEncoding', function (t) {
  t.plan(3)
  t.equal(B.isEncoding('HEX'), true)
  t.equal(B.isEncoding('hex'), true)
  t.equal(B.isEncoding('bad'), false)
  t.end()
})
