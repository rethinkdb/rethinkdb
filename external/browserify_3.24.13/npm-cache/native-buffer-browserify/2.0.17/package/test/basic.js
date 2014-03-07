var B = require('../index.js').Buffer
var test = require('tape')

test('new buffer from array', function (t) {
  t.equal(
    new B([1, 2, 3]).toString(),
    '\u0001\u0002\u0003'
  )
  t.end()
})

test('new buffer from string', function (t) {
  t.plan(1)
  t.equal(
    new B('hey', 'utf8').toString(),
    'hey'
  )
  t.end()
})

test('buffer toArrayBuffer()', function (t) {
  var data = [1, 2, 3, 4, 5, 6, 7, 8]
  if (typeof Uint8Array === 'function') {
    var result = new B(data).toArrayBuffer()
    var expected = new Uint8Array(data).buffer
    for (var i = 0; i < expected.byteLength; i++) {
      t.equal(result[i], expected[i])
    }
  } else {
    t.pass('No toArrayBuffer() method provided in old browsers')
  }
  t.end()
})

test('buffer toJSON()', function (t) {
  t.plan(1)
  var data = [1, 2, 3, 4]
  t.deepEqual(
    new B(data).toJSON(),
    { type: 'Buffer', data: [1,2,3,4] }
  )
  t.end()
})

test('buffer copy example', function (t) {
  t.plan(1)

  buf1 = new B(26)
  buf2 = new B(26)

  for (var i = 0 ; i < 26 ; i++) {
    buf1[i] = i + 97; // 97 is ASCII a
    buf2[i] = 33; // ASCII !
  }

  buf1.copy(buf2, 8, 16, 20)

  t.equal(
    buf2.toString('ascii', 0, 25),
    '!!!!!!!!qrst!!!!!!!!!!!!!'
  )
  t.end()
})
