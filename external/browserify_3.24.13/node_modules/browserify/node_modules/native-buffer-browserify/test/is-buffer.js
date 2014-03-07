var B = require('../index.js').Buffer
var test = require('tape')

test('Buffer.isBuffer', function (t) {
  t.plan(3)
  t.equal(B.isBuffer(new B('hey', 'utf8')), true)
  t.equal(B.isBuffer(new B([1, 2, 3], 'utf8')), true)
  t.equal(B.isBuffer('hey'), false)
  t.end()
})
