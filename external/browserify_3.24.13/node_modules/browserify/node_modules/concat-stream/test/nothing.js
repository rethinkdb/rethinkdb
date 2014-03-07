var concat = require('../')
var test = require('tape')

test('no callback stream', function (t) {
  var stream = concat()
  stream.write('space')
  stream.end(' cats')
  t.end()
})
