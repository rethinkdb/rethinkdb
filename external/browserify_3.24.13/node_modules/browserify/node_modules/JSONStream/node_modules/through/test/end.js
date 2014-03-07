var test = require('tape')
var through = require('../')

// must emit end before close.

test('end before close', function (assert) {
  var ts = through()
  var ended = false, closed = false

  ts.on('end', function () {
    assert.ok(!closed)
    ended = true
  })
  ts.on('close', function () {
    assert.ok(ended)
    closed = true
  })

  ts.write(1)
  ts.write(2)
  ts.write(3)
  ts.end()
  assert.ok(ended)
  assert.ok(closed)
  assert.end()
})
