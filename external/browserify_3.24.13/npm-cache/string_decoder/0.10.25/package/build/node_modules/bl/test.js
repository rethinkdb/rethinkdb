const tape       = require('tape')
    , crypto     = require('crypto')
    , fs         = require('fs')
    , hash       = require('hash_file')
    , BufferList = require('./')

tape('single bytes from single buffer', function (t) {
  var bl = new BufferList()
  bl.append(new Buffer('abcd'))

  t.equal(bl.length, 4)

  t.equal(bl.get(0), 97)
  t.equal(bl.get(1), 98)
  t.equal(bl.get(2), 99)
  t.equal(bl.get(3), 100)

  t.end()
})

tape('single bytes from multiple buffers', function (t) {
  var bl = new BufferList()
  bl.append(new Buffer('abcd'))
  bl.append(new Buffer('efg'))
  bl.append(new Buffer('hi'))
  bl.append(new Buffer('j'))

  t.equal(bl.length, 10)

  t.equal(bl.get(0), 97)
  t.equal(bl.get(1), 98)
  t.equal(bl.get(2), 99)
  t.equal(bl.get(3), 100)
  t.equal(bl.get(4), 101)
  t.equal(bl.get(5), 102)
  t.equal(bl.get(6), 103)
  t.equal(bl.get(7), 104)
  t.equal(bl.get(8), 105)
  t.equal(bl.get(9), 106)
  t.end()
})

tape('multi bytes from single buffer', function (t) {
  var bl = new BufferList()
  bl.append(new Buffer('abcd'))

  t.equal(bl.length, 4)

  t.equal(bl.slice(0, 4).toString('ascii'), 'abcd')
  t.equal(bl.slice(0, 3).toString('ascii'), 'abc')
  t.equal(bl.slice(1, 4).toString('ascii'), 'bcd')

  t.end()
})

tape('multiple bytes from multiple buffers', function (t) {
  var bl = new BufferList()

  bl.append(new Buffer('abcd'))
  bl.append(new Buffer('efg'))
  bl.append(new Buffer('hi'))
  bl.append(new Buffer('j'))

  t.equal(bl.length, 10)

  t.equal(bl.slice(0, 10).toString('ascii'), 'abcdefghij')
  t.equal(bl.slice(3, 10).toString('ascii'), 'defghij')
  t.equal(bl.slice(3, 6).toString('ascii'), 'def')
  t.equal(bl.slice(3, 8).toString('ascii'), 'defgh')
  t.equal(bl.slice(5, 10).toString('ascii'), 'fghij')

  t.end()
})

tape('consuming from multiple buffers', function (t) {
  var bl = new BufferList()

  bl.append(new Buffer('abcd'))
  bl.append(new Buffer('efg'))
  bl.append(new Buffer('hi'))
  bl.append(new Buffer('j'))

  t.equal(bl.length, 10)

  t.equal(bl.slice(0, 10).toString('ascii'), 'abcdefghij')

  bl.consume(3)
  t.equal(bl.length, 7)
  t.equal(bl.slice(0, 7).toString('ascii'), 'defghij')

  bl.consume(2)
  t.equal(bl.length, 5)
  t.equal(bl.slice(0, 5).toString('ascii'), 'fghij')

  bl.consume(1)
  t.equal(bl.length, 4)
  t.equal(bl.slice(0, 4).toString('ascii'), 'ghij')

  bl.consume(1)
  t.equal(bl.length, 3)
  t.equal(bl.slice(0, 3).toString('ascii'), 'hij')

  bl.consume(2)
  t.equal(bl.length, 1)
  t.equal(bl.slice(0, 1).toString('ascii'), 'j')

  t.end()
})

tape('test readUInt8 / readInt8', function (t) {
  var buf1 = new Buffer(1)
    , buf2 = new Buffer(3)
    , buf3 = new Buffer(3)
    , bl  = new BufferList()

  buf2[1] = 0x3
  buf2[2] = 0x4
  buf3[0] = 0x23
  buf3[1] = 0x42

  bl.append(buf1)
  bl.append(buf2)
  bl.append(buf3)

  t.equal(bl.readUInt8(2), 0x3)
  t.equal(bl.readInt8(2), 0x3)
  t.equal(bl.readUInt8(3), 0x4)
  t.equal(bl.readInt8(3), 0x4)
  t.equal(bl.readUInt8(4), 0x23)
  t.equal(bl.readInt8(4), 0x23)
  t.equal(bl.readUInt8(5), 0x42)
  t.equal(bl.readInt8(5), 0x42)
  t.end()
})

tape('test readUInt16LE / readUInt16BE / readInt16LE / readInt16BE', function (t) {
  var buf1 = new Buffer(1)
    , buf2 = new Buffer(3)
    , buf3 = new Buffer(3)
    , bl   = new BufferList()

  buf2[1] = 0x3
  buf2[2] = 0x4
  buf3[0] = 0x23
  buf3[1] = 0x42

  bl.append(buf1)
  bl.append(buf2)
  bl.append(buf3)

  t.equal(bl.readUInt16BE(2), 0x0304)
  t.equal(bl.readUInt16LE(2), 0x0403)
  t.equal(bl.readInt16BE(2), 0x0304)
  t.equal(bl.readInt16LE(2), 0x0403)
  t.equal(bl.readUInt16BE(3), 0x0423)
  t.equal(bl.readUInt16LE(3), 0x2304)
  t.equal(bl.readInt16BE(3), 0x0423)
  t.equal(bl.readInt16LE(3), 0x2304)
  t.equal(bl.readUInt16BE(4), 0x2342)
  t.equal(bl.readUInt16LE(4), 0x4223)
  t.equal(bl.readInt16BE(4), 0x2342)
  t.equal(bl.readInt16LE(4), 0x4223)
  t.end()
})

tape('test readUInt32LE / readUInt32BE / readInt32LE / readInt32BE', function (t) {
  var buf1 = new Buffer(1)
    , buf2 = new Buffer(3)
    , buf3 = new Buffer(3)
    , bl   = new BufferList()

  buf2[1] = 0x3
  buf2[2] = 0x4
  buf3[0] = 0x23
  buf3[1] = 0x42

  bl.append(buf1)
  bl.append(buf2)
  bl.append(buf3)

  t.equal(bl.readUInt32BE(2), 0x03042342)
  t.equal(bl.readUInt32LE(2), 0x42230403)
  t.equal(bl.readInt32BE(2), 0x03042342)
  t.equal(bl.readInt32LE(2), 0x42230403)
  t.end()
})

tape('test readFloatLE / readFloatBE', function (t) {
  var buf1 = new Buffer(1)
    , buf2 = new Buffer(3)
    , buf3 = new Buffer(3)
    , bl   = new BufferList()

  buf2[1] = 0x00
  buf2[2] = 0x00
  buf3[0] = 0x80
  buf3[1] = 0x3f

  bl.append(buf1)
  bl.append(buf2)
  bl.append(buf3)

  t.equal(bl.readFloatLE(2), 0x01)
  t.end()
})

tape('test readDoubleLE / readDoubleBE', function (t) {
  var buf1 = new Buffer(1)
    , buf2 = new Buffer(3)
    , buf3 = new Buffer(10)
    , bl   = new BufferList()

  buf2[1] = 0x55
  buf2[2] = 0x55
  buf3[0] = 0x55
  buf3[1] = 0x55
  buf3[2] = 0x55
  buf3[3] = 0x55
  buf3[4] = 0xd5
  buf3[5] = 0x3f

  bl.append(buf1)
  bl.append(buf2)
  bl.append(buf3)

  t.equal(bl.readDoubleLE(2), 0.3333333333333333)
  t.end()
})

tape('test toString', function (t) {
  var bl = new BufferList()

  bl.append(new Buffer('abcd'))
  bl.append(new Buffer('efg'))
  bl.append(new Buffer('hi'))
  bl.append(new Buffer('j'))

  t.equal(bl.toString('ascii', 0, 10), 'abcdefghij')
  t.equal(bl.toString('ascii', 3, 10), 'defghij')
  t.equal(bl.toString('ascii', 3, 6), 'def')
  t.equal(bl.toString('ascii', 3, 8), 'defgh')
  t.equal(bl.toString('ascii', 5, 10), 'fghij')

  t.end()
})

tape('test toString encoding', function (t) {
  var bl = new BufferList()
    , b  = new Buffer('abcdefghij\xff\x00')

  bl.append(new Buffer('abcd'))
  bl.append(new Buffer('efg'))
  bl.append(new Buffer('hi'))
  bl.append(new Buffer('j'))
  bl.append(new Buffer('\xff\x00'))

  'hex utf8 utf-8 ascii binary base64 ucs2 ucs-2 utf16le utf-16le'
    .split(' ')
    .forEach(function (enc) {
      t.equal(bl.toString(enc), b.toString(enc))
    })

  t.end()
})

tape('test stream', function (t) {
  var random = crypto.randomBytes(1024 * 1024)
    , rndhash = hash(random, 'md5')
    , md5sum = crypto.createHash('md5')
    , bl     = new BufferList(function (err, buf) {
        t.ok(Buffer.isBuffer(buf))
        t.ok(err === null)
        t.equal(rndhash, hash(bl.slice(), 'md5'))
        t.equal(rndhash, hash(buf, 'md5'))

        bl.pipe(fs.createWriteStream('/tmp/bl_test_rnd_out.dat'))
          .on('close', function () {
            var s = fs.createReadStream('/tmp/bl_test_rnd_out.dat')
            s.on('data', md5sum.update.bind(md5sum))
            s.on('end', function() {
              t.equal(rndhash, md5sum.digest('hex'), 'woohoo! correct hash!')
              t.end()
            })
          })

      })

  fs.writeFileSync('/tmp/bl_test_rnd.dat', random)
  fs.createReadStream('/tmp/bl_test_rnd.dat').pipe(bl)
})

tape('instantiation with Buffer', function (t) {
  var buf  = crypto.randomBytes(1024)
    , buf2 = crypto.randomBytes(1024)
    , b    = BufferList(buf)

  t.equal(hash(buf, 'md5'), hash(b.slice(), 'md5'), 'same hash!')
  b = BufferList([ buf, buf2 ])
  t.equal(hash(b.slice(), 'md5'), hash(Buffer.concat([ buf, buf2 ]), 'md5'), 'same hash!')
  t.end()
})

tape('test String appendage', function (t) {
  var bl = new BufferList()
    , b  = new Buffer('abcdefghij\xff\x00')

  bl.append('abcd')
  bl.append('efg')
  bl.append('hi')
  bl.append('j')
  bl.append('\xff\x00')

  'hex utf8 utf-8 ascii binary base64 ucs2 ucs-2 utf16le utf-16le'
    .split(' ')
    .forEach(function (enc) {
      t.equal(bl.toString(enc), b.toString(enc))
    })

  t.end()
})

tape('write nothing, should get empty buffer', function (t) {
  t.plan(3)
  BufferList(function (err, data) {
    t.notOk(err, 'no error')
    t.ok(Buffer.isBuffer(data), 'got a buffer')
    t.equal(0, data.length, 'got a zero-length buffer')
    t.end()
  }).end()
})

tape('unicode string', function (t) {
  t.plan(2)
  var inp1 = '\u2600'
    , inp2 = '\u2603'
    , exp = inp1 + ' and ' + inp2
    , bl = BufferList()
  bl.write(inp1)
  bl.write(' and ')
  bl.write(inp2)
  t.equal(exp, bl.toString())
  t.equal(new Buffer(exp).toString('hex'), bl.toString('hex'))
})

tape('should emit finish', function (t) {
  var source = BufferList()
    , dest = BufferList()
 
  source.write('hello')
  source.pipe(dest)

  dest.on('finish', function () {
    t.equal(dest.toString('utf8'), 'hello')
    t.end()
  })
})

tape('basic copy', function (t) {
  var buf  = crypto.randomBytes(1024)
    , buf2 = new Buffer(1024)
    , b    = BufferList(buf)

  b.copy(buf2)
  t.equal(hash(b.slice(), 'md5'), hash(buf2, 'md5'), 'same hash!')
  t.end()
})

tape('copy after many appends', function (t) {
  var buf  = crypto.randomBytes(512)
    , buf2 = new Buffer(1024)
    , b    = BufferList(buf)

  b.append(buf)
  b.copy(buf2)
  t.equal(hash(b.slice(), 'md5'), hash(buf2, 'md5'), 'same hash!')
  t.end()
})

tape('copy at a precise position', function (t) {
  var buf  = crypto.randomBytes(1004)
    , buf2 = new Buffer(1024)
    , b    = BufferList(buf)

  b.copy(buf2, 20)
  t.equal(hash(b.slice(), 'md5'), hash(buf2.slice(20), 'md5'), 'same hash!')
  t.end()
})

tape('copy starting from a precise location', function (t) {
  var buf  = crypto.randomBytes(10)
    , buf2 = new Buffer(5)
    , b    = BufferList(buf)

  b.copy(buf2, 0, 5)
  t.equal(hash(b.slice(5), 'md5'), hash(buf2, 'md5'), 'same hash!')
  t.end()
})

tape('copy in an interval', function (t) {
  var buf      = crypto.randomBytes(10)
    , buf2     = new Buffer(3)
    , b        = BufferList(buf)
    , expected = new Buffer(3)

  // put the same old data there
  buf2.copy(expected)
  buf.copy(expected, 0, 5, 7)

  b.copy(buf2, 0, 5, 7)
  t.equal(hash(expected, 'md5'), hash(buf2, 'md5'), 'same hash!')
  t.end()
})

tape('copy an interval between two buffers', function (t) {
  var buf      = crypto.randomBytes(10)
    , buf2     = new Buffer(10)
    , b        = BufferList(buf)

  b.append(buf)
  b.copy(buf2, 0, 5, 15)

  t.equal(hash(b.slice(5, 15), 'md5'), hash(buf2, 'md5'), 'same hash!')
  t.end()
})

tape('duplicate', function (t) {
  t.plan(2)

  var bl = new BufferList('abcdefghij\xff\x00')
    , dup = bl.duplicate()

  t.equal(bl.prototype, dup.prototype)
  t.equal(bl.toString('hex'), dup.toString('hex'))
})

tape('handle error', function (t) {
  t.plan(2)
  fs.createReadStream('/does/not/exist').pipe(BufferList(function (err, data) {
    t.ok(err instanceof Error, 'has error')
    t.notOk(data, 'no data')
  }))
})