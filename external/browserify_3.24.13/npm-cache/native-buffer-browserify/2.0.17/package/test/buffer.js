var B = require('../index.js').Buffer
var test = require('tape')

test('utf8 buffer to base64', function (t) {
  t.equal(
    new B("Ձאab", "utf8").toString("base64"),
    '1YHXkGFi'
  )
  t.end()
})

test('utf8 buffer to hex', function (t) {
  t.equal(
    new B("Ձאab", "utf8").toString("hex"),
    'd581d7906162'
  )
  t.end()
})

test('utf8 to utf8', function (t) {
  t.equal(
    new B("öäüõÖÄÜÕ", "utf8").toString("utf8"),
    'öäüõÖÄÜÕ'
  )
  t.end()
})

test('ascii buffer to base64', function (t) {
  t.equal(
    new B("123456!@#$%^", "ascii").toString("base64"),
    'MTIzNDU2IUAjJCVe'
  )
  t.end()
})

test('ascii buffer to hex', function (t) {
  t.equal(
    new B("123456!@#$%^", "ascii").toString("hex"),
    '31323334353621402324255e'
  )
  t.end()
})

test('base64 buffer to utf8', function (t) {
  t.equal(
    new B("1YHXkGFi", "base64").toString("utf8"),
    'Ձאab'
  )
  t.end()
})

test('hex buffer to utf8', function (t) {
  t.equal(
    new B("d581d7906162", "hex").toString("utf8"),
    'Ձאab'
  )
  t.end()
})

test('base64 buffer to ascii', function (t) {
  t.equal(
    new B("MTIzNDU2IUAjJCVe", "base64").toString("ascii"),
    '123456!@#$%^'
  )
  t.end()
})

test('hex buffer to ascii', function (t) {
  t.equal(
    new B("31323334353621402324255e", "hex").toString("ascii"),
    '123456!@#$%^'
  )
  t.end()
})

test('base64 buffer to binary', function (t) {
  t.equal(
    new B("MTIzNDU2IUAjJCVe", "base64").toString("binary"),
    '123456!@#$%^'
  )
  t.end()
})

test('hex buffer to binary', function (t) {
  t.equal(
    new B("31323334353621402324255e", "hex").toString("binary"),
    '123456!@#$%^'
  )
  t.end()
})

test('utf8 to binary', function (t) {
  t.equal(
    new B("öäüõÖÄÜÕ", "utf8").toString("binary"),
    "Ã¶Ã¤Ã¼ÃµÃÃÃÃ"
  )
  t.end()
})

test("hex of write{Uint,Int}{8,16,32}{LE,BE}", function (t) {
  t.plan(2*(2*2*2+2))
  var hex = [
    "03", "0300", "0003", "03000000", "00000003",
    "fd", "fdff", "fffd", "fdffffff", "fffffffd"
  ]
  var reads = [ 3, 3, 3, 3, 3, -3, -3, -3, -3, -3 ]
  var xs = ["UInt","Int"]
  var ys = [8,16,32]
  for (var i = 0; i < xs.length; i++) {
    var x = xs[i]
    for (var j = 0; j < ys.length; j++) {
      var y = ys[j]
      var endianesses = (y === 8) ? [""] : ["LE","BE"]
      for (var k = 0; k < endianesses.length; k++) {
        var z = endianesses[k]

        var v1  = new B(y / 8)
        var writefn  = "write" + x + y + z
        var val = (x === "Int") ? -3 : 3
        v1[writefn](val, 0)
        t.equal(
          v1.toString("hex"),
          hex.shift()
        )
        var readfn = "read" + x + y + z
        t.equal(
          v1[readfn](0),
          reads.shift()
        )
      }
    }
  }
  t.end()
})

test("hex of write{Uint,Int}{8,16,32}{LE,BE} with overflow", function (t) {
    t.plan(3*(2*2*2+2))
    var hex = [
      "", "03", "00", "030000", "000000",
      "", "fd", "ff", "fdffff", "ffffff"
    ]
    var reads = [
      undefined, 3, 0, 3, 0,
      undefined, 253, -256, 16777213, -256
    ]
    var xs = ["UInt","Int"]
    var ys = [8,16,32]
    for (var i = 0; i < xs.length; i++) {
      var x = xs[i]
      for (var j = 0; j < ys.length; j++) {
        var y = ys[j]
        var endianesses = (y === 8) ? [""] : ["LE","BE"]
        for (var k = 0; k < endianesses.length; k++) {
          var z = endianesses[k]

          var v1  = new B(y / 8 - 1)
          var next = new B(4)
          next.writeUInt32BE(0, 0)
          var writefn  = "write" + x + y + z
          var val = (x === "Int") ? -3 : 3
          v1[writefn](val, 0, true)
          t.equal(
            v1.toString("hex"),
            hex.shift()
          )
          // check that nothing leaked to next buffer.
          t.equal(next.readUInt32BE(0), 0)
          // check that no bytes are read from next buffer.
          next.writeInt32BE(~0, 0)
          var readfn = "read" + x + y + z
          t.equal(
            v1[readfn](0, true),
            reads.shift()
          )
        }
      }
    }
    t.end()
})

test("concat() a varying number of buffers", function (t) {
  t.plan(5)
  var zero = []
  var one  = [ new B('asdf') ]
  var long = []
  for (var i = 0; i < 10; i++) long.push(new B('asdf'))

  var flatZero = B.concat(zero)
  var flatOne = B.concat(one)
  var flatLong = B.concat(long)
  var flatLongLen = B.concat(long, 40)

  t.equal(flatZero.length, 0)
  t.equal(flatOne.toString(), 'asdf')
  t.equal(flatOne, one[0])
  t.equal(flatLong.toString(), (new Array(10+1).join('asdf')))
  t.equal(flatLongLen.toString(), (new Array(10+1).join('asdf')))
  t.end()
})

test("new buffer from buffer", function (t) {
  var b1 = new B('asdf')
  var b2 = new B(b1)
  t.equal(b1.toString('hex'), b2.toString('hex'))
  t.end()
})

test("new buffer from uint8array", function (t) {
  if (typeof Uint8Array === 'function') {
    var b1 = new Uint8Array([0, 1, 2, 3])
    var b2 = new B(b1)
    t.equal(b1[0], 0)
    t.equal(b1[1], 1)
    t.equal(b1[2], 2)
    t.equal(b1[3], 3)
    t.equal(b1[4], undefined)
  }
  t.end()
})

test("fill", function(t) {
  t.plan(1)
  var b = new B(10)
  b.fill(2)
  t.equal(b.toString('hex'), '02020202020202020202')
  t.end()
})

test('copy() empty buffer with sourceEnd=0', function (t) {
  t.plan(1)
  var source = new B([42])
  var destination = new B([43])
  source.copy(destination, 0, 0, 0)
  t.equal(destination.readUInt8(0), 43)
  t.end()
})

test('base64 ignore whitespace', function(t) {
  t.plan(1)
  var text = "\n   YW9ldQ==  "
  var buf = new B(text, 'base64')
  t.equal(buf.toString(), 'aoeu')
  t.end()
})

test('buffer.slice sets indexes', function (t) {
  t.plan(1)
  t.equal((new B('hallo')).slice(0, 5).toString(), 'hallo')
  t.end()
})

test('buffer.slice out of range', function (t) {
  t.plan(2)
  t.equal((new B('hallo')).slice(0, 10).toString(), 'hallo')
  t.equal((new B('hallo')).slice(10, 2).toString(), '')
  t.end()
})

test('base64 strings without padding', function (t) {
  t.plan(1)
  t.equal((new B('YW9ldQ', 'base64').toString()), 'aoeu')
  t.end()
})
