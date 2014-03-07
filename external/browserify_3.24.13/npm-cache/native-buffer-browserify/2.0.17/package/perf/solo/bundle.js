(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);throw new Error("Cannot find module '"+o+"'")}var f=n[o]={exports:{}};t[o][0].call(f.exports,function(e){var n=t[o][1][e];return s(n?n:e)},f,f.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
var base64 = require('base64-js')
var ieee754 = require('ieee754')

exports.Buffer = Buffer
exports.SlowBuffer = Buffer
exports.INSPECT_MAX_BYTES = 50
Buffer.poolSize = 8192

/**
 * If `browserSupport`:
 *   === true    Use Uint8Array implementation (fastest)
 *   === false   Use Object implementation (compatible down to IE6)
 */
var browserSupport = (function () {
   // Detect if browser supports Typed Arrays. Supported browsers are IE 10+,
   // Firefox 4+, Chrome 7+, Safari 5.1+, Opera 11.6+, iOS 4.2+.
   if (typeof Uint8Array === 'undefined' || typeof ArrayBuffer === 'undefined')
      return false

  // Does the browser support adding properties to `Uint8Array` instances? If
  // not, then that's the same as no `Uint8Array` support. We need to be able to
  // add all the node Buffer API methods.
  // Relevant Firefox bug: https://bugzilla.mozilla.org/show_bug.cgi?id=695438
  try {
    var arr = new Uint8Array(0)
    arr.foo = function () { return 42 }
    return 42 === arr.foo() &&
        typeof arr.subarray === 'function' // Chrome 9-10 lack `subarray`
  } catch (e) {
    return false
  }
})()


/**
 * Class: Buffer
 * =============
 *
 * The Buffer constructor returns instances of `Uint8Array` that are augmented
 * with function properties for all the node `Buffer` API functions. We use
 * `Uint8Array` so that square bracket notation works as expected -- it returns
 * a single octet.
 *
 * By augmenting the instances, we can avoid modifying the `Uint8Array`
 * prototype.
 */
function Buffer (subject, encoding, noZero) {
  if (!(this instanceof Buffer))
    return new Buffer(subject, encoding, noZero)

  var type = typeof subject

  // Workaround: node's base64 implementation allows for non-padded strings
  // while base64-js does not.
  if (encoding === 'base64' && type === 'string') {
    subject = stringtrim(subject)
    while (subject.length % 4 !== 0) {
      subject = subject + '='
    }
  }

  // Find the length
  var length
  if (type === 'number')
    length = coerce(subject)
  else if (type === 'string')
    length = Buffer.byteLength(subject, encoding)
  else if (type === 'object')
    length = coerce(subject.length) // Assume object is an array
  else
    throw new Error('First argument needs to be a number, array or string.')

  var buf
  if (browserSupport) {
    // Preferred: Return an augmented `Uint8Array` instance for best performance
    buf = augment(new Uint8Array(length))
  } else {
    // Fallback: Return this instance of Buffer
    buf = this
    buf.length = length
  }

  var i
  if (Buffer.isBuffer(subject)) {
    // Speed optimization -- use set if we're copying from a Uint8Array
    buf.set(subject)
  } else if (isArrayish(subject)) {
    // Treat array-ish objects as a byte array
    for (i = 0; i < length; i++) {
      if (Buffer.isBuffer(subject))
        buf[i] = subject.readUInt8(i)
      else
        buf[i] = subject[i]
    }
  } else if (type === 'string') {
    buf.write(subject, 0, encoding)
  } else if (type === 'number' && !browserSupport && !noZero) {
    for (i = 0; i < length; i++) {
      buf[i] = 0
    }
  }

  return buf
}

// STATIC METHODS
// ==============

Buffer.isEncoding = function (encoding) {
  switch (String(encoding).toLowerCase()) {
    case 'hex':
    case 'utf8':
    case 'utf-8':
    case 'ascii':
    case 'binary':
    case 'base64':
    case 'raw':
      return true

    default:
      return false
  }
}

Buffer.isBuffer = function (b) {
  return b && b._isBuffer
}

Buffer.byteLength = function (str, encoding) {
  switch (encoding || 'utf8') {
    case 'hex':
      return str.length / 2

    case 'utf8':
    case 'utf-8':
      return utf8ToBytes(str).length

    case 'ascii':
    case 'binary':
      return str.length

    case 'base64':
      return base64ToBytes(str).length

    default:
      throw new Error('Unknown encoding')
  }
}

Buffer.concat = function (list, totalLength) {
  if (!isArray(list)) {
    throw new Error('Usage: Buffer.concat(list, [totalLength])\n' +
        'list should be an Array.')
  }

  if (list.length === 0) {
    return new Buffer(0)
  } else if (list.length === 1) {
    return list[0]
  }

  var i
  if (typeof totalLength !== 'number') {
    totalLength = 0
    for (i = 0; i < list.length; i++) {
      totalLength += list[i].length
    }
  }

  var buf = new Buffer(totalLength)
  var pos = 0
  for (i = 0; i < list.length; i++) {
    var item = list[i]
    item.copy(buf, pos)
    pos += item.length
  }
  return buf
}

// BUFFER INSTANCE METHODS
// =======================

function _hexWrite (buf, string, offset, length) {
  offset = Number(offset) || 0
  var remaining = buf.length - offset
  if (!length) {
    length = remaining
  } else {
    length = Number(length)
    if (length > remaining) {
      length = remaining
    }
  }

  // must be an even number of digits
  var strLen = string.length
  if (strLen % 2 !== 0) {
    throw new Error('Invalid hex string')
  }
  if (length > strLen / 2) {
    length = strLen / 2
  }
  for (var i = 0; i < length; i++) {
    var byte = parseInt(string.substr(i * 2, 2), 16)
    if (isNaN(byte)) throw new Error('Invalid hex string')
    buf[offset + i] = byte
  }
  Buffer._charsWritten = i * 2
  return i
}

function _utf8Write (buf, string, offset, length) {
  var bytes, pos
  return Buffer._charsWritten = blitBuffer(utf8ToBytes(string), buf, offset, length)
}

function _asciiWrite (buf, string, offset, length) {
  var bytes, pos
  return Buffer._charsWritten = blitBuffer(asciiToBytes(string), buf, offset, length)
}

function _binaryWrite (buf, string, offset, length) {
  return _asciiWrite(buf, string, offset, length)
}

function _base64Write (buf, string, offset, length) {
  var bytes, pos
  return Buffer._charsWritten = blitBuffer(base64ToBytes(string), buf, offset, length)
}

Buffer.prototype.write = function (string, offset, length, encoding) {
  // Support both (string, offset, length, encoding)
  // and the legacy (string, encoding, offset, length)
  if (isFinite(offset)) {
    if (!isFinite(length)) {
      encoding = length
      length = undefined
    }
  } else {  // legacy
    var swap = encoding
    encoding = offset
    offset = length
    length = swap
  }

  offset = Number(offset) || 0
  var remaining = this.length - offset
  if (!length) {
    length = remaining
  } else {
    length = Number(length)
    if (length > remaining) {
      length = remaining
    }
  }
  encoding = String(encoding || 'utf8').toLowerCase()

  switch (encoding) {
    case 'hex':
      return _hexWrite(this, string, offset, length)

    case 'utf8':
    case 'utf-8':
      return _utf8Write(this, string, offset, length)

    case 'ascii':
      return _asciiWrite(this, string, offset, length)

    case 'binary':
      return _binaryWrite(this, string, offset, length)

    case 'base64':
      return _base64Write(this, string, offset, length)

    default:
      throw new Error('Unknown encoding')
  }
}

Buffer.prototype.toString = function (encoding, start, end) {
  var self = this

  encoding = String(encoding || 'utf8').toLowerCase()
  start = Number(start) || 0
  end = (end !== undefined)
    ? Number(end)
    : end = self.length

  // Fastpath empty strings
  if (end === start)
    return ''

  switch (encoding) {
    case 'hex':
      return _hexSlice(self, start, end)

    case 'utf8':
    case 'utf-8':
      return _utf8Slice(self, start, end)

    case 'ascii':
      return _asciiSlice(self, start, end)

    case 'binary':
      return _binarySlice(self, start, end)

    case 'base64':
      return _base64Slice(self, start, end)

    default:
      throw new Error('Unknown encoding')
  }
}

Buffer.prototype.toJSON = function () {
  return {
    type: 'Buffer',
    data: Array.prototype.slice.call(this._arr || this, 0)
  }
}

// copy(targetBuffer, targetStart=0, sourceStart=0, sourceEnd=buffer.length)
Buffer.prototype.copy = function (target, target_start, start, end) {
  var source = this

  if (!start) start = 0
  if (!end && end !== 0) end = this.length
  if (!target_start) target_start = 0

  // Copy 0 bytes; we're done
  if (end === start) return
  if (target.length === 0 || source.length === 0) return

  // Fatal error conditions
  if (end < start)
    throw new Error('sourceEnd < sourceStart')
  if (target_start < 0 || target_start >= target.length)
    throw new Error('targetStart out of bounds')
  if (start < 0 || start >= source.length)
    throw new Error('sourceStart out of bounds')
  if (end < 0 || end > source.length)
    throw new Error('sourceEnd out of bounds')

  // Are we oob?
  if (end > this.length)
    end = this.length
  if (target.length - target_start < end - start)
    end = target.length - target_start + start

  // copy!
  for (var i = 0; i < end - start; i++)
    target[i + target_start] = this[i + start]
}

function _base64Slice (buf, start, end) {
  if (start === 0 && end === buf.length) {
    return base64.fromByteArray(buf)
  } else {
    return base64.fromByteArray(buf.slice(start, end))
  }
}

function _utf8Slice (buf, start, end) {
  var res = ''
  var tmp = ''
  end = Math.min(buf.length, end)

  for (var i = start; i < end; i++) {
    if (buf[i] <= 0x7F) {
      res += decodeUtf8Char(tmp) + String.fromCharCode(buf[i])
      tmp = ''
    } else {
      tmp += '%' + buf[i].toString(16)
    }
  }

  return res + decodeUtf8Char(tmp)
}

function _asciiSlice (buf, start, end) {
  var ret = ''
  end = Math.min(buf.length, end)

  for (var i = start; i < end; i++)
    ret += String.fromCharCode(buf[i])
  return ret
}

function _binarySlice (buf, start, end) {
  return _asciiSlice(buf, start, end)
}

function _hexSlice (buf, start, end) {
  var len = buf.length

  if (!start || start < 0) start = 0
  if (!end || end < 0 || end > len) end = len

  var out = ''
  for (var i = start; i < end; i++) {
    out += toHex(buf[i])
  }
  return out
}

// TODO: add test that modifying the new buffer slice will modify memory in the
// original buffer! Use code from:
// http://nodejs.org/api/buffer.html#buffer_buf_slice_start_end
Buffer.prototype.slice = function (start, end) {
  var len = this.length
  start = clamp(start, len, 0)
  end = clamp(end, len, len)

  if (browserSupport) {
    return augment(this.subarray(start, end))
  } else {
    // TODO: slicing works, with limitations (no parent tracking/update)
    // https://github.com/feross/native-buffer-browserify/issues/9
    var sliceLen = end - start
    var newBuf = new Buffer(sliceLen, undefined, true)
    for (var i = 0; i < sliceLen; i++) {
      newBuf[i] = this[i + start]
    }
    return newBuf
  }
}

Buffer.prototype.readUInt8 = function (offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < buf.length, 'Trying to read beyond buffer length')
  }

  if (offset >= buf.length)
    return

  return buf[offset]
}

function _readUInt16 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len)
    return

  var val
  if (littleEndian) {
    val = buf[offset]
    if (offset + 1 < len)
      val |= buf[offset + 1] << 8
  } else {
    val = buf[offset] << 8
    if (offset + 1 < len)
      val |= buf[offset + 1]
  }
  return val
}

Buffer.prototype.readUInt16LE = function (offset, noAssert) {
  return _readUInt16(this, offset, true, noAssert)
}

Buffer.prototype.readUInt16BE = function (offset, noAssert) {
  return _readUInt16(this, offset, false, noAssert)
}

function _readUInt32 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len)
    return

  var val
  if (littleEndian) {
    if (offset + 2 < len)
      val = buf[offset + 2] << 16
    if (offset + 1 < len)
      val |= buf[offset + 1] << 8
    val |= buf[offset]
    if (offset + 3 < len)
      val = val + (buf[offset + 3] << 24 >>> 0)
  } else {
    if (offset + 1 < len)
      val = buf[offset + 1] << 16
    if (offset + 2 < len)
      val |= buf[offset + 2] << 8
    if (offset + 3 < len)
      val |= buf[offset + 3]
    val = val + (buf[offset] << 24 >>> 0)
  }
  return val
}

Buffer.prototype.readUInt32LE = function (offset, noAssert) {
  return _readUInt32(this, offset, true, noAssert)
}

Buffer.prototype.readUInt32BE = function (offset, noAssert) {
  return _readUInt32(this, offset, false, noAssert)
}

Buffer.prototype.readInt8 = function (offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(offset !== undefined && offset !== null,
        'missing offset')
    assert(offset < buf.length, 'Trying to read beyond buffer length')
  }

  if (offset >= buf.length)
    return

  var neg = buf[offset] & 0x80
  if (neg)
    return (0xff - buf[offset] + 1) * -1
  else
    return buf[offset]
}

function _readInt16 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len)
    return

  var val = _readUInt16(buf, offset, littleEndian, true)
  var neg = val & 0x8000
  if (neg)
    return (0xffff - val + 1) * -1
  else
    return val
}

Buffer.prototype.readInt16LE = function (offset, noAssert) {
  return _readInt16(this, offset, true, noAssert)
}

Buffer.prototype.readInt16BE = function (offset, noAssert) {
  return _readInt16(this, offset, false, noAssert)
}

function _readInt32 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len)
    return

  var val = _readUInt32(buf, offset, littleEndian, true)
  var neg = val & 0x80000000
  if (neg)
    return (0xffffffff - val + 1) * -1
  else
    return val
}

Buffer.prototype.readInt32LE = function (offset, noAssert) {
  return _readInt32(this, offset, true, noAssert)
}

Buffer.prototype.readInt32BE = function (offset, noAssert) {
  return _readInt32(this, offset, false, noAssert)
}

function _readFloat (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  return ieee754.read(buf, offset, littleEndian, 23, 4)
}

Buffer.prototype.readFloatLE = function (offset, noAssert) {
  return _readFloat(this, offset, true, noAssert)
}

Buffer.prototype.readFloatBE = function (offset, noAssert) {
  return _readFloat(this, offset, false, noAssert)
}

function _readDouble (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset + 7 < buf.length, 'Trying to read beyond buffer length')
  }

  return ieee754.read(buf, offset, littleEndian, 52, 8)
}

Buffer.prototype.readDoubleLE = function (offset, noAssert) {
  return _readDouble(this, offset, true, noAssert)
}

Buffer.prototype.readDoubleBE = function (offset, noAssert) {
  return _readDouble(this, offset, false, noAssert)
}

Buffer.prototype.writeUInt8 = function (value, offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xff)
  }

  if (offset >= buf.length) return

  buf[offset] = value
}

function _writeUInt16 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xffff)
  }

  var len = buf.length
  if (offset >= len)
    return

  for (var i = 0, j = Math.min(len - offset, 2); i < j; i++) {
    buf[offset + i] =
        (value & (0xff << (8 * (littleEndian ? i : 1 - i)))) >>>
            (littleEndian ? i : 1 - i) * 8
  }
}

Buffer.prototype.writeUInt16LE = function (value, offset, noAssert) {
  _writeUInt16(this, value, offset, true, noAssert)
}

Buffer.prototype.writeUInt16BE = function (value, offset, noAssert) {
  _writeUInt16(this, value, offset, false, noAssert)
}

function _writeUInt32 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xffffffff)
  }

  var len = buf.length
  if (offset >= len)
    return

  for (var i = 0, j = Math.min(len - offset, 4); i < j; i++) {
    buf[offset + i] =
        (value >>> (littleEndian ? i : 3 - i) * 8) & 0xff
  }
}

Buffer.prototype.writeUInt32LE = function (value, offset, noAssert) {
  _writeUInt32(this, value, offset, true, noAssert)
}

Buffer.prototype.writeUInt32BE = function (value, offset, noAssert) {
  _writeUInt32(this, value, offset, false, noAssert)
}

Buffer.prototype.writeInt8 = function (value, offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7f, -0x80)
  }

  if (offset >= buf.length)
    return

  if (value >= 0)
    buf.writeUInt8(value, offset, noAssert)
  else
    buf.writeUInt8(0xff + value + 1, offset, noAssert)
}

function _writeInt16 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7fff, -0x8000)
  }

  var len = buf.length
  if (offset >= len)
    return

  if (value >= 0)
    _writeUInt16(buf, value, offset, littleEndian, noAssert)
  else
    _writeUInt16(buf, 0xffff + value + 1, offset, littleEndian, noAssert)
}

Buffer.prototype.writeInt16LE = function (value, offset, noAssert) {
  _writeInt16(this, value, offset, true, noAssert)
}

Buffer.prototype.writeInt16BE = function (value, offset, noAssert) {
  _writeInt16(this, value, offset, false, noAssert)
}

function _writeInt32 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7fffffff, -0x80000000)
  }

  var len = buf.length
  if (offset >= len)
    return

  if (value >= 0)
    _writeUInt32(buf, value, offset, littleEndian, noAssert)
  else
    _writeUInt32(buf, 0xffffffff + value + 1, offset, littleEndian, noAssert)
}

Buffer.prototype.writeInt32LE = function (value, offset, noAssert) {
  _writeInt32(this, value, offset, true, noAssert)
}

Buffer.prototype.writeInt32BE = function (value, offset, noAssert) {
  _writeInt32(this, value, offset, false, noAssert)
}

function _writeFloat (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to write beyond buffer length')
    verifIEEE754(value, 3.4028234663852886e+38, -3.4028234663852886e+38)
  }

  var len = buf.length
  if (offset >= len)
    return

  ieee754.write(buf, value, offset, littleEndian, 23, 4)
}

Buffer.prototype.writeFloatLE = function (value, offset, noAssert) {
  _writeFloat(this, value, offset, true, noAssert)
}

Buffer.prototype.writeFloatBE = function (value, offset, noAssert) {
  _writeFloat(this, value, offset, false, noAssert)
}

function _writeDouble (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof littleEndian === 'boolean', 'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 7 < buf.length,
        'Trying to write beyond buffer length')
    verifIEEE754(value, 1.7976931348623157E+308, -1.7976931348623157E+308)
  }

  var len = buf.length
  if (offset >= len)
    return

  ieee754.write(buf, value, offset, littleEndian, 52, 8)
}

Buffer.prototype.writeDoubleLE = function (value, offset, noAssert) {
  _writeDouble(this, value, offset, true, noAssert)
}

Buffer.prototype.writeDoubleBE = function (value, offset, noAssert) {
  _writeDouble(this, value, offset, false, noAssert)
}

// fill(value, start=0, end=buffer.length)
Buffer.prototype.fill = function (value, start, end) {
  if (!value) value = 0
  if (!start) start = 0
  if (!end) end = this.length

  if (typeof value === 'string') {
    value = value.charCodeAt(0)
  }

  if (typeof value !== 'number' || isNaN(value)) {
    throw new Error('value is not a number')
  }

  if (end < start) throw new Error('end < start')

  // Fill 0 bytes; we're done
  if (end === start) return
  if (this.length === 0) return

  if (start < 0 || start >= this.length) {
    throw new Error('start out of bounds')
  }

  if (end < 0 || end > this.length) {
    throw new Error('end out of bounds')
  }

  for (var i = start; i < end; i++) {
    this[i] = value
  }
}

Buffer.prototype.inspect = function () {
  var out = []
  var len = this.length
  for (var i = 0; i < len; i++) {
    out[i] = toHex(this[i])
    if (i === exports.INSPECT_MAX_BYTES) {
      out[i + 1] = '...'
      break
    }
  }
  return '<Buffer ' + out.join(' ') + '>'
}

/**
 * Creates a new `ArrayBuffer` with the *copied* memory of the buffer instance.
 * Added in Node 0.12. Not added to Buffer.prototype since it should only
 * be available in browsers that support ArrayBuffer.
 */
function BufferToArrayBuffer () {
  return (new Buffer(this)).buffer
}

// HELPER FUNCTIONS
// ================

function stringtrim (str) {
  if (str.trim) return str.trim()
  return str.replace(/^\s+|\s+$/g, '')
}

var BP = Buffer.prototype

function augment (arr) {
  arr._isBuffer = true

  // Augment the Uint8Array *instance* (not the class!) with Buffer methods
  arr.write = BP.write
  arr.toString = BP.toString
  arr.toLocaleString = BP.toString
  arr.toJSON = BP.toJSON
  arr.copy = BP.copy
  arr.slice = BP.slice
  arr.readUInt8 = BP.readUInt8
  arr.readUInt16LE = BP.readUInt16LE
  arr.readUInt16BE = BP.readUInt16BE
  arr.readUInt32LE = BP.readUInt32LE
  arr.readUInt32BE = BP.readUInt32BE
  arr.readInt8 = BP.readInt8
  arr.readInt16LE = BP.readInt16LE
  arr.readInt16BE = BP.readInt16BE
  arr.readInt32LE = BP.readInt32LE
  arr.readInt32BE = BP.readInt32BE
  arr.readFloatLE = BP.readFloatLE
  arr.readFloatBE = BP.readFloatBE
  arr.readDoubleLE = BP.readDoubleLE
  arr.readDoubleBE = BP.readDoubleBE
  arr.writeUInt8 = BP.writeUInt8
  arr.writeUInt16LE = BP.writeUInt16LE
  arr.writeUInt16BE = BP.writeUInt16BE
  arr.writeUInt32LE = BP.writeUInt32LE
  arr.writeUInt32BE = BP.writeUInt32BE
  arr.writeInt8 = BP.writeInt8
  arr.writeInt16LE = BP.writeInt16LE
  arr.writeInt16BE = BP.writeInt16BE
  arr.writeInt32LE = BP.writeInt32LE
  arr.writeInt32BE = BP.writeInt32BE
  arr.writeFloatLE = BP.writeFloatLE
  arr.writeFloatBE = BP.writeFloatBE
  arr.writeDoubleLE = BP.writeDoubleLE
  arr.writeDoubleBE = BP.writeDoubleBE
  arr.fill = BP.fill
  arr.inspect = BP.inspect
  arr.toArrayBuffer = BufferToArrayBuffer

  return arr
}

// slice(start, end)
function clamp (index, len, defaultValue) {
  if (typeof index !== 'number') return defaultValue
  index = ~~index;  // Coerce to integer.
  if (index >= len) return len
  if (index >= 0) return index
  index += len
  if (index >= 0) return index
  return 0
}

function coerce (length) {
  // Coerce length to a number (possibly NaN), round up
  // in case it's fractional (e.g. 123.456) then do a
  // double negate to coerce a NaN to 0. Easy, right?
  length = ~~Math.ceil(+length)
  return length < 0 ? 0 : length
}

function isArray (subject) {
  return (Array.isArray || function (subject) {
    return Object.prototype.toString.call(subject) === '[object Array]'
  })(subject)
}

function isArrayish (subject) {
  return isArray(subject) || Buffer.isBuffer(subject) ||
      subject && typeof subject === 'object' &&
      typeof subject.length === 'number'
}

function toHex (n) {
  if (n < 16) return '0' + n.toString(16)
  return n.toString(16)
}

function utf8ToBytes (str) {
  var byteArray = []
  for (var i = 0; i < str.length; i++)
    if (str.charCodeAt(i) <= 0x7F)
      byteArray.push(str.charCodeAt(i))
    else {
      var h = encodeURIComponent(str.charAt(i)).substr(1).split('%')
      for (var j = 0; j < h.length; j++)
        byteArray.push(parseInt(h[j], 16))
    }
  return byteArray
}

function asciiToBytes (str) {
  var byteArray = []
  for (var i = 0; i < str.length; i++) {
    // Node's code seems to be doing this and not & 0x7F..
    byteArray.push(str.charCodeAt(i) & 0xFF)
  }
  return byteArray
}

function base64ToBytes (str) {
  return base64.toByteArray(str)
}

function blitBuffer (src, dst, offset, length) {
  var pos
  for (var i = 0; i < length; i++) {
    if ((i + offset >= dst.length) || (i >= src.length))
      break
    dst[i + offset] = src[i]
  }
  return i
}

function decodeUtf8Char (str) {
  try {
    return decodeURIComponent(str)
  } catch (err) {
    return String.fromCharCode(0xFFFD) // UTF 8 invalid char
  }
}

/*
 * We have to make sure that the value is a valid integer. This means that it
 * is non-negative. It has no fractional component and that it does not
 * exceed the maximum allowed value.
 */
function verifuint (value, max) {
  assert(typeof value == 'number', 'cannot write a non-number as a number')
  assert(value >= 0,
      'specified a negative value for writing an unsigned value')
  assert(value <= max, 'value is larger than maximum value for type')
  assert(Math.floor(value) === value, 'value has a fractional component')
}

function verifsint(value, max, min) {
  assert(typeof value == 'number', 'cannot write a non-number as a number')
  assert(value <= max, 'value larger than maximum allowed value')
  assert(value >= min, 'value smaller than minimum allowed value')
  assert(Math.floor(value) === value, 'value has a fractional component')
}

function verifIEEE754(value, max, min) {
  assert(typeof value == 'number', 'cannot write a non-number as a number')
  assert(value <= max, 'value larger than maximum allowed value')
  assert(value >= min, 'value smaller than minimum allowed value')
}

function assert (test, message) {
  if (!test) throw new Error(message || 'Failed assertion')
}

},{"base64-js":2,"ieee754":5}],2:[function(require,module,exports){
(function (exports) {
	'use strict';

	var lookup = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

	function b64ToByteArray(b64) {
		var i, j, l, tmp, placeHolders, arr;
	
		if (b64.length % 4 > 0) {
			throw 'Invalid string. Length must be a multiple of 4';
		}

		// the number of equal signs (place holders)
		// if there are two placeholders, than the two characters before it
		// represent one byte
		// if there is only one, then the three characters before it represent 2 bytes
		// this is just a cheap hack to not do indexOf twice
		placeHolders = indexOf(b64, '=');
		placeHolders = placeHolders > 0 ? b64.length - placeHolders : 0;

		// base64 is 4/3 + up to two characters of the original data
		arr = [];//new Uint8Array(b64.length * 3 / 4 - placeHolders);

		// if there are placeholders, only get up to the last complete 4 chars
		l = placeHolders > 0 ? b64.length - 4 : b64.length;

		for (i = 0, j = 0; i < l; i += 4, j += 3) {
			tmp = (indexOf(lookup, b64.charAt(i)) << 18) | (indexOf(lookup, b64.charAt(i + 1)) << 12) | (indexOf(lookup, b64.charAt(i + 2)) << 6) | indexOf(lookup, b64.charAt(i + 3));
			arr.push((tmp & 0xFF0000) >> 16);
			arr.push((tmp & 0xFF00) >> 8);
			arr.push(tmp & 0xFF);
		}

		if (placeHolders === 2) {
			tmp = (indexOf(lookup, b64.charAt(i)) << 2) | (indexOf(lookup, b64.charAt(i + 1)) >> 4);
			arr.push(tmp & 0xFF);
		} else if (placeHolders === 1) {
			tmp = (indexOf(lookup, b64.charAt(i)) << 10) | (indexOf(lookup, b64.charAt(i + 1)) << 4) | (indexOf(lookup, b64.charAt(i + 2)) >> 2);
			arr.push((tmp >> 8) & 0xFF);
			arr.push(tmp & 0xFF);
		}

		return arr;
	}

	function uint8ToBase64(uint8) {
		var i,
			extraBytes = uint8.length % 3, // if we have 1 byte left, pad 2 bytes
			output = "",
			temp, length;

		function tripletToBase64 (num) {
			return lookup.charAt(num >> 18 & 0x3F) + lookup.charAt(num >> 12 & 0x3F) + lookup.charAt(num >> 6 & 0x3F) + lookup.charAt(num & 0x3F);
		};

		// go through the array every three bytes, we'll deal with trailing stuff later
		for (i = 0, length = uint8.length - extraBytes; i < length; i += 3) {
			temp = (uint8[i] << 16) + (uint8[i + 1] << 8) + (uint8[i + 2]);
			output += tripletToBase64(temp);
		}

		// pad the end with zeros, but make sure to not forget the extra bytes
		switch (extraBytes) {
			case 1:
				temp = uint8[uint8.length - 1];
				output += lookup.charAt(temp >> 2);
				output += lookup.charAt((temp << 4) & 0x3F);
				output += '==';
				break;
			case 2:
				temp = (uint8[uint8.length - 2] << 8) + (uint8[uint8.length - 1]);
				output += lookup.charAt(temp >> 10);
				output += lookup.charAt((temp >> 4) & 0x3F);
				output += lookup.charAt((temp << 2) & 0x3F);
				output += '=';
				break;
		}

		return output;
	}

	module.exports.toByteArray = b64ToByteArray;
	module.exports.fromByteArray = uint8ToBase64;
}());

function indexOf (arr, elt /*, from*/) {
	var len = arr.length;

	var from = Number(arguments[1]) || 0;
	from = (from < 0)
		? Math.ceil(from)
		: Math.floor(from);
	if (from < 0)
		from += len;

	for (; from < len; from++) {
		if ((typeof arr === 'string' && arr.charAt(from) === elt) ||
				(typeof arr !== 'string' && arr[from] === elt)) {
			return from;
		}
	}
	return -1;
}

},{}],3:[function(require,module,exports){
var process=require("__browserify_process"),global=typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {};/*!
 * Benchmark.js v1.0.0 <http://benchmarkjs.com/>
 * Copyright 2010-2012 Mathias Bynens <http://mths.be/>
 * Based on JSLitmus.js, copyright Robert Kieffer <http://broofa.com/>
 * Modified by John-David Dalton <http://allyoucanleet.com/>
 * Available under MIT license <http://mths.be/mit>
 */
;(function(window, undefined) {
  'use strict';

  /** Used to assign each benchmark an incrimented id */
  var counter = 0;

  /** Detect DOM document object */
  var doc = isHostType(window, 'document') && document;

  /** Detect free variable `define` */
  var freeDefine = typeof define == 'function' &&
    typeof define.amd == 'object' && define.amd && define;

  /** Detect free variable `exports` */
  var freeExports = typeof exports == 'object' && exports &&
    (typeof global == 'object' && global && global == global.global && (window = global), exports);

  /** Detect free variable `require` */
  var freeRequire = typeof require == 'function' && require;

  /** Used to crawl all properties regardless of enumerability */
  var getAllKeys = Object.getOwnPropertyNames;

  /** Used to get property descriptors */
  var getDescriptor = Object.getOwnPropertyDescriptor;

  /** Used in case an object doesn't have its own method */
  var hasOwnProperty = {}.hasOwnProperty;

  /** Used to check if an object is extensible */
  var isExtensible = Object.isExtensible || function() { return true; };

  /** Used to access Wade Simmons' Node microtime module */
  var microtimeObject = req('microtime');

  /** Used to access the browser's high resolution timer */
  var perfObject = isHostType(window, 'performance') && performance;

  /** Used to call the browser's high resolution timer */
  var perfName = perfObject && (
    perfObject.now && 'now' ||
    perfObject.webkitNow && 'webkitNow'
  );

  /** Used to access Node's high resolution timer */
  var processObject = isHostType(window, 'process') && process;

  /** Used to check if an own property is enumerable */
  var propertyIsEnumerable = {}.propertyIsEnumerable;

  /** Used to set property descriptors */
  var setDescriptor = Object.defineProperty;

  /** Used to resolve a value's internal [[Class]] */
  var toString = {}.toString;

  /** Used to prevent a `removeChild` memory leak in IE < 9 */
  var trash = doc && doc.createElement('div');

  /** Used to integrity check compiled tests */
  var uid = 'uid' + (+new Date);

  /** Used to avoid infinite recursion when methods call each other */
  var calledBy = {};

  /** Used to avoid hz of Infinity */
  var divisors = {
    '1': 4096,
    '2': 512,
    '3': 64,
    '4': 8,
    '5': 0
  };

  /**
   * T-Distribution two-tailed critical values for 95% confidence
   * http://www.itl.nist.gov/div898/handbook/eda/section3/eda3672.htm
   */
  var tTable = {
    '1':  12.706,'2':  4.303, '3':  3.182, '4':  2.776, '5':  2.571, '6':  2.447,
    '7':  2.365, '8':  2.306, '9':  2.262, '10': 2.228, '11': 2.201, '12': 2.179,
    '13': 2.16,  '14': 2.145, '15': 2.131, '16': 2.12,  '17': 2.11,  '18': 2.101,
    '19': 2.093, '20': 2.086, '21': 2.08,  '22': 2.074, '23': 2.069, '24': 2.064,
    '25': 2.06,  '26': 2.056, '27': 2.052, '28': 2.048, '29': 2.045, '30': 2.042,
    'infinity': 1.96
  };

  /**
   * Critical Mann-Whitney U-values for 95% confidence
   * http://www.saburchill.com/IBbiology/stats/003.html
   */
  var uTable = {
    '5':  [0, 1, 2],
    '6':  [1, 2, 3, 5],
    '7':  [1, 3, 5, 6, 8],
    '8':  [2, 4, 6, 8, 10, 13],
    '9':  [2, 4, 7, 10, 12, 15, 17],
    '10': [3, 5, 8, 11, 14, 17, 20, 23],
    '11': [3, 6, 9, 13, 16, 19, 23, 26, 30],
    '12': [4, 7, 11, 14, 18, 22, 26, 29, 33, 37],
    '13': [4, 8, 12, 16, 20, 24, 28, 33, 37, 41, 45],
    '14': [5, 9, 13, 17, 22, 26, 31, 36, 40, 45, 50, 55],
    '15': [5, 10, 14, 19, 24, 29, 34, 39, 44, 49, 54, 59, 64],
    '16': [6, 11, 15, 21, 26, 31, 37, 42, 47, 53, 59, 64, 70, 75],
    '17': [6, 11, 17, 22, 28, 34, 39, 45, 51, 57, 63, 67, 75, 81, 87],
    '18': [7, 12, 18, 24, 30, 36, 42, 48, 55, 61, 67, 74, 80, 86, 93, 99],
    '19': [7, 13, 19, 25, 32, 38, 45, 52, 58, 65, 72, 78, 85, 92, 99, 106, 113],
    '20': [8, 14, 20, 27, 34, 41, 48, 55, 62, 69, 76, 83, 90, 98, 105, 112, 119, 127],
    '21': [8, 15, 22, 29, 36, 43, 50, 58, 65, 73, 80, 88, 96, 103, 111, 119, 126, 134, 142],
    '22': [9, 16, 23, 30, 38, 45, 53, 61, 69, 77, 85, 93, 101, 109, 117, 125, 133, 141, 150, 158],
    '23': [9, 17, 24, 32, 40, 48, 56, 64, 73, 81, 89, 98, 106, 115, 123, 132, 140, 149, 157, 166, 175],
    '24': [10, 17, 25, 33, 42, 50, 59, 67, 76, 85, 94, 102, 111, 120, 129, 138, 147, 156, 165, 174, 183, 192],
    '25': [10, 18, 27, 35, 44, 53, 62, 71, 80, 89, 98, 107, 117, 126, 135, 145, 154, 163, 173, 182, 192, 201, 211],
    '26': [11, 19, 28, 37, 46, 55, 64, 74, 83, 93, 102, 112, 122, 132, 141, 151, 161, 171, 181, 191, 200, 210, 220, 230],
    '27': [11, 20, 29, 38, 48, 57, 67, 77, 87, 97, 107, 118, 125, 138, 147, 158, 168, 178, 188, 199, 209, 219, 230, 240, 250],
    '28': [12, 21, 30, 40, 50, 60, 70, 80, 90, 101, 111, 122, 132, 143, 154, 164, 175, 186, 196, 207, 218, 228, 239, 250, 261, 272],
    '29': [13, 22, 32, 42, 52, 62, 73, 83, 94, 105, 116, 127, 138, 149, 160, 171, 182, 193, 204, 215, 226, 238, 249, 260, 271, 282, 294],
    '30': [13, 23, 33, 43, 54, 65, 76, 87, 98, 109, 120, 131, 143, 154, 166, 177, 189, 200, 212, 223, 235, 247, 258, 270, 282, 293, 305, 317]
  };

  /**
   * An object used to flag environments/features.
   *
   * @static
   * @memberOf Benchmark
   * @type Object
   */
  var support = {};

  (function() {

    /**
     * Detect Adobe AIR.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.air = isClassOf(window.runtime, 'ScriptBridgingProxyObject');

    /**
     * Detect if `arguments` objects have the correct internal [[Class]] value.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.argumentsClass = isClassOf(arguments, 'Arguments');

    /**
     * Detect if in a browser environment.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.browser = doc && isHostType(window, 'navigator');

    /**
     * Detect if strings support accessing characters by index.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.charByIndex =
      // IE 8 supports indexes on string literals but not string objects
      ('x'[0] + Object('x')[0]) == 'xx';

    /**
     * Detect if strings have indexes as own properties.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.charByOwnIndex =
      // Narwhal, Rhino, RingoJS, IE 8, and Opera < 10.52 support indexes on
      // strings but don't detect them as own properties
      support.charByIndex && hasKey('x', '0');

    /**
     * Detect if Java is enabled/exposed.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.java = isClassOf(window.java, 'JavaPackage');

    /**
     * Detect if the Timers API exists.
     *
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.timeout = isHostType(window, 'setTimeout') && isHostType(window, 'clearTimeout');

    /**
     * Detect if functions support decompilation.
     *
     * @name decompilation
     * @memberOf Benchmark.support
     * @type Boolean
     */
    try {
      // Safari 2.x removes commas in object literals
      // from Function#toString results
      // http://webk.it/11609
      // Firefox 3.6 and Opera 9.25 strip grouping
      // parentheses from Function#toString results
      // http://bugzil.la/559438
      support.decompilation = Function(
        'return (' + (function(x) { return { 'x': '' + (1 + x) + '', 'y': 0 }; }) + ')'
      )()(0).x === '1';
    } catch(e) {
      support.decompilation = false;
    }

    /**
     * Detect ES5+ property descriptor API.
     *
     * @name descriptors
     * @memberOf Benchmark.support
     * @type Boolean
     */
    try {
      var o = {};
      support.descriptors = (setDescriptor(o, o, o), 'value' in getDescriptor(o, o));
    } catch(e) {
      support.descriptors = false;
    }

    /**
     * Detect ES5+ Object.getOwnPropertyNames().
     *
     * @name getAllKeys
     * @memberOf Benchmark.support
     * @type Boolean
     */
    try {
      support.getAllKeys = /\bvalueOf\b/.test(getAllKeys(Object.prototype));
    } catch(e) {
      support.getAllKeys = false;
    }

    /**
     * Detect if own properties are iterated before inherited properties (all but IE < 9).
     *
     * @name iteratesOwnLast
     * @memberOf Benchmark.support
     * @type Boolean
     */
    support.iteratesOwnFirst = (function() {
      var props = [];
      function ctor() { this.x = 1; }
      ctor.prototype = { 'y': 1 };
      for (var prop in new ctor) { props.push(prop); }
      return props[0] == 'x';
    }());

    /**
     * Detect if a node's [[Class]] is resolvable (all but IE < 9)
     * and that the JS engine errors when attempting to coerce an object to a
     * string without a `toString` property value of `typeof` "function".
     *
     * @name nodeClass
     * @memberOf Benchmark.support
     * @type Boolean
     */
    try {
      support.nodeClass = ({ 'toString': 0 } + '', toString.call(doc || 0) != '[object Object]');
    } catch(e) {
      support.nodeClass = true;
    }
  }());

  /**
   * Timer object used by `clock()` and `Deferred#resolve`.
   *
   * @private
   * @type Object
   */
  var timer = {

   /**
    * The timer namespace object or constructor.
    *
    * @private
    * @memberOf timer
    * @type Function|Object
    */
    'ns': Date,

   /**
    * Starts the deferred timer.
    *
    * @private
    * @memberOf timer
    * @param {Object} deferred The deferred instance.
    */
    'start': null, // lazy defined in `clock()`

   /**
    * Stops the deferred timer.
    *
    * @private
    * @memberOf timer
    * @param {Object} deferred The deferred instance.
    */
    'stop': null // lazy defined in `clock()`
  };

  /** Shortcut for inverse results */
  var noArgumentsClass = !support.argumentsClass,
      noCharByIndex = !support.charByIndex,
      noCharByOwnIndex = !support.charByOwnIndex;

  /** Math shortcuts */
  var abs   = Math.abs,
      floor = Math.floor,
      max   = Math.max,
      min   = Math.min,
      pow   = Math.pow,
      sqrt  = Math.sqrt;

  /*--------------------------------------------------------------------------*/

  /**
   * The Benchmark constructor.
   *
   * @constructor
   * @param {String} name A name to identify the benchmark.
   * @param {Function|String} fn The test to benchmark.
   * @param {Object} [options={}] Options object.
   * @example
   *
   * // basic usage (the `new` operator is optional)
   * var bench = new Benchmark(fn);
   *
   * // or using a name first
   * var bench = new Benchmark('foo', fn);
   *
   * // or with options
   * var bench = new Benchmark('foo', fn, {
   *
   *   // displayed by Benchmark#toString if `name` is not available
   *   'id': 'xyz',
   *
   *   // called when the benchmark starts running
   *   'onStart': onStart,
   *
   *   // called after each run cycle
   *   'onCycle': onCycle,
   *
   *   // called when aborted
   *   'onAbort': onAbort,
   *
   *   // called when a test errors
   *   'onError': onError,
   *
   *   // called when reset
   *   'onReset': onReset,
   *
   *   // called when the benchmark completes running
   *   'onComplete': onComplete,
   *
   *   // compiled/called before the test loop
   *   'setup': setup,
   *
   *   // compiled/called after the test loop
   *   'teardown': teardown
   * });
   *
   * // or name and options
   * var bench = new Benchmark('foo', {
   *
   *   // a flag to indicate the benchmark is deferred
   *   'defer': true,
   *
   *   // benchmark test function
   *   'fn': function(deferred) {
   *     // call resolve() when the deferred test is finished
   *     deferred.resolve();
   *   }
   * });
   *
   * // or options only
   * var bench = new Benchmark({
   *
   *   // benchmark name
   *   'name': 'foo',
   *
   *   // benchmark test as a string
   *   'fn': '[1,2,3,4].sort()'
   * });
   *
   * // a test's `this` binding is set to the benchmark instance
   * var bench = new Benchmark('foo', function() {
   *   'My name is '.concat(this.name); // My name is foo
   * });
   */
  function Benchmark(name, fn, options) {
    var me = this;

    // allow instance creation without the `new` operator
    if (me == null || me.constructor != Benchmark) {
      return new Benchmark(name, fn, options);
    }
    // juggle arguments
    if (isClassOf(name, 'Object')) {
      // 1 argument (options)
      options = name;
    }
    else if (isClassOf(name, 'Function')) {
      // 2 arguments (fn, options)
      options = fn;
      fn = name;
    }
    else if (isClassOf(fn, 'Object')) {
      // 2 arguments (name, options)
      options = fn;
      fn = null;
      me.name = name;
    }
    else {
      // 3 arguments (name, fn [, options])
      me.name = name;
    }
    setOptions(me, options);
    me.id || (me.id = ++counter);
    me.fn == null && (me.fn = fn);
    me.stats = deepClone(me.stats);
    me.times = deepClone(me.times);
  }

  /**
   * The Deferred constructor.
   *
   * @constructor
   * @memberOf Benchmark
   * @param {Object} clone The cloned benchmark instance.
   */
  function Deferred(clone) {
    var me = this;
    if (me == null || me.constructor != Deferred) {
      return new Deferred(clone);
    }
    me.benchmark = clone;
    clock(me);
  }

  /**
   * The Event constructor.
   *
   * @constructor
   * @memberOf Benchmark
   * @param {String|Object} type The event type.
   */
  function Event(type) {
    var me = this;
    return (me == null || me.constructor != Event)
      ? new Event(type)
      : (type instanceof Event)
          ? type
          : extend(me, { 'timeStamp': +new Date }, typeof type == 'string' ? { 'type': type } : type);
  }

  /**
   * The Suite constructor.
   *
   * @constructor
   * @memberOf Benchmark
   * @param {String} name A name to identify the suite.
   * @param {Object} [options={}] Options object.
   * @example
   *
   * // basic usage (the `new` operator is optional)
   * var suite = new Benchmark.Suite;
   *
   * // or using a name first
   * var suite = new Benchmark.Suite('foo');
   *
   * // or with options
   * var suite = new Benchmark.Suite('foo', {
   *
   *   // called when the suite starts running
   *   'onStart': onStart,
   *
   *   // called between running benchmarks
   *   'onCycle': onCycle,
   *
   *   // called when aborted
   *   'onAbort': onAbort,
   *
   *   // called when a test errors
   *   'onError': onError,
   *
   *   // called when reset
   *   'onReset': onReset,
   *
   *   // called when the suite completes running
   *   'onComplete': onComplete
   * });
   */
  function Suite(name, options) {
    var me = this;

    // allow instance creation without the `new` operator
    if (me == null || me.constructor != Suite) {
      return new Suite(name, options);
    }
    // juggle arguments
    if (isClassOf(name, 'Object')) {
      // 1 argument (options)
      options = name;
    } else {
      // 2 arguments (name [, options])
      me.name = name;
    }
    setOptions(me, options);
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Note: Some array methods have been implemented in plain JavaScript to avoid
   * bugs in IE, Opera, Rhino, and Mobile Safari.
   *
   * IE compatibility mode and IE < 9 have buggy Array `shift()` and `splice()`
   * functions that fail to remove the last element, `object[0]`, of
   * array-like-objects even though the `length` property is set to `0`.
   * The `shift()` method is buggy in IE 8 compatibility mode, while `splice()`
   * is buggy regardless of mode in IE < 9 and buggy in compatibility mode in IE 9.
   *
   * In Opera < 9.50 and some older/beta Mobile Safari versions using `unshift()`
   * generically to augment the `arguments` object will pave the value at index 0
   * without incrimenting the other values's indexes.
   * https://github.com/documentcloud/underscore/issues/9
   *
   * Rhino and environments it powers, like Narwhal and RingoJS, may have
   * buggy Array `concat()`, `reverse()`, `shift()`, `slice()`, `splice()` and
   * `unshift()` functions that make sparse arrays non-sparse by assigning the
   * undefined indexes a value of undefined.
   * https://github.com/mozilla/rhino/commit/702abfed3f8ca043b2636efd31c14ba7552603dd
   */

  /**
   * Creates an array containing the elements of the host array followed by the
   * elements of each argument in order.
   *
   * @memberOf Benchmark.Suite
   * @returns {Array} The new array.
   */
  function concat() {
    var value,
        j = -1,
        length = arguments.length,
        result = slice.call(this),
        index = result.length;

    while (++j < length) {
      value = arguments[j];
      if (isClassOf(value, 'Array')) {
        for (var k = 0, l = value.length; k < l; k++, index++) {
          if (k in value) {
            result[index] = value[k];
          }
        }
      } else {
        result[index++] = value;
      }
    }
    return result;
  }

  /**
   * Utility function used by `shift()`, `splice()`, and `unshift()`.
   *
   * @private
   * @param {Number} start The index to start inserting elements.
   * @param {Number} deleteCount The number of elements to delete from the insert point.
   * @param {Array} elements The elements to insert.
   * @returns {Array} An array of deleted elements.
   */
  function insert(start, deleteCount, elements) {
    // `result` should have its length set to the `deleteCount`
    // see https://bugs.ecmascript.org/show_bug.cgi?id=332
    var deleteEnd = start + deleteCount,
        elementCount = elements ? elements.length : 0,
        index = start - 1,
        length = start + elementCount,
        object = this,
        result = Array(deleteCount),
        tail = slice.call(object, deleteEnd);

    // delete elements from the array
    while (++index < deleteEnd) {
      if (index in object) {
        result[index - start] = object[index];
        delete object[index];
      }
    }
    // insert elements
    index = start - 1;
    while (++index < length) {
      object[index] = elements[index - start];
    }
    // append tail elements
    start = index--;
    length = max(0, (object.length >>> 0) - deleteCount + elementCount);
    while (++index < length) {
      if ((index - start) in tail) {
        object[index] = tail[index - start];
      } else if (index in object) {
        delete object[index];
      }
    }
    // delete excess elements
    deleteCount = deleteCount > elementCount ? deleteCount - elementCount : 0;
    while (deleteCount--) {
      index = length + deleteCount;
      if (index in object) {
        delete object[index];
      }
    }
    object.length = length;
    return result;
  }

  /**
   * Rearrange the host array's elements in reverse order.
   *
   * @memberOf Benchmark.Suite
   * @returns {Array} The reversed array.
   */
  function reverse() {
    var upperIndex,
        value,
        index = -1,
        object = Object(this),
        length = object.length >>> 0,
        middle = floor(length / 2);

    if (length > 1) {
      while (++index < middle) {
        upperIndex = length - index - 1;
        value = upperIndex in object ? object[upperIndex] : uid;
        if (index in object) {
          object[upperIndex] = object[index];
        } else {
          delete object[upperIndex];
        }
        if (value != uid) {
          object[index] = value;
        } else {
          delete object[index];
        }
      }
    }
    return object;
  }

  /**
   * Removes the first element of the host array and returns it.
   *
   * @memberOf Benchmark.Suite
   * @returns {Mixed} The first element of the array.
   */
  function shift() {
    return insert.call(this, 0, 1)[0];
  }

  /**
   * Creates an array of the host array's elements from the start index up to,
   * but not including, the end index.
   *
   * @memberOf Benchmark.Suite
   * @param {Number} start The starting index.
   * @param {Number} end The end index.
   * @returns {Array} The new array.
   */
  function slice(start, end) {
    var index = -1,
        object = Object(this),
        length = object.length >>> 0,
        result = [];

    start = toInteger(start);
    start = start < 0 ? max(length + start, 0) : min(start, length);
    start--;
    end = end == null ? length : toInteger(end);
    end = end < 0 ? max(length + end, 0) : min(end, length);

    while ((++index, ++start) < end) {
      if (start in object) {
        result[index] = object[start];
      }
    }
    return result;
  }

  /**
   * Allows removing a range of elements and/or inserting elements into the
   * host array.
   *
   * @memberOf Benchmark.Suite
   * @param {Number} start The start index.
   * @param {Number} deleteCount The number of elements to delete.
   * @param {Mixed} [val1, val2, ...] values to insert at the `start` index.
   * @returns {Array} An array of removed elements.
   */
  function splice(start, deleteCount) {
    var object = Object(this),
        length = object.length >>> 0;

    start = toInteger(start);
    start = start < 0 ? max(length + start, 0) : min(start, length);

    // support the de-facto SpiderMonkey extension
    // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Array/splice#Parameters
    // https://bugs.ecmascript.org/show_bug.cgi?id=429
    deleteCount = arguments.length == 1
      ? length - start
      : min(max(toInteger(deleteCount), 0), length - start);

    return insert.call(object, start, deleteCount, slice.call(arguments, 2));
  }

  /**
   * Converts the specified `value` to an integer.
   *
   * @private
   * @param {Mixed} value The value to convert.
   * @returns {Number} The resulting integer.
   */
  function toInteger(value) {
    value = +value;
    return value === 0 || !isFinite(value) ? value || 0 : value - (value % 1);
  }

  /**
   * Appends arguments to the host array.
   *
   * @memberOf Benchmark.Suite
   * @returns {Number} The new length.
   */
  function unshift() {
    var object = Object(this);
    insert.call(object, 0, 0, arguments);
    return object.length;
  }

  /*--------------------------------------------------------------------------*/

  /**
   * A generic `Function#bind` like method.
   *
   * @private
   * @param {Function} fn The function to be bound to `thisArg`.
   * @param {Mixed} thisArg The `this` binding for the given function.
   * @returns {Function} The bound function.
   */
  function bind(fn, thisArg) {
    return function() { fn.apply(thisArg, arguments); };
  }

  /**
   * Creates a function from the given arguments string and body.
   *
   * @private
   * @param {String} args The comma separated function arguments.
   * @param {String} body The function body.
   * @returns {Function} The new function.
   */
  function createFunction() {
    // lazy define
    createFunction = function(args, body) {
      var result,
          anchor = freeDefine ? define.amd : Benchmark,
          prop = uid + 'createFunction';

      runScript((freeDefine ? 'define.amd.' : 'Benchmark.') + prop + '=function(' + args + '){' + body + '}');
      result = anchor[prop];
      delete anchor[prop];
      return result;
    };
    // fix JaegerMonkey bug
    // http://bugzil.la/639720
    createFunction = support.browser && (createFunction('', 'return"' + uid + '"') || noop)() == uid ? createFunction : Function;
    return createFunction.apply(null, arguments);
  }

  /**
   * Delay the execution of a function based on the benchmark's `delay` property.
   *
   * @private
   * @param {Object} bench The benchmark instance.
   * @param {Object} fn The function to execute.
   */
  function delay(bench, fn) {
    bench._timerId = setTimeout(fn, bench.delay * 1e3);
  }

  /**
   * Destroys the given element.
   *
   * @private
   * @param {Element} element The element to destroy.
   */
  function destroyElement(element) {
    trash.appendChild(element);
    trash.innerHTML = '';
  }

  /**
   * Iterates over an object's properties, executing the `callback` for each.
   * Callbacks may terminate the loop by explicitly returning `false`.
   *
   * @private
   * @param {Object} object The object to iterate over.
   * @param {Function} callback The function executed per own property.
   * @param {Object} options The options object.
   * @returns {Object} Returns the object iterated over.
   */
  function forProps() {
    var forShadowed,
        skipSeen,
        forArgs = true,
        shadowed = ['constructor', 'hasOwnProperty', 'isPrototypeOf', 'propertyIsEnumerable', 'toLocaleString', 'toString', 'valueOf'];

    (function(enumFlag, key) {
      // must use a non-native constructor to catch the Safari 2 issue
      function Klass() { this.valueOf = 0; };
      Klass.prototype.valueOf = 0;
      // check various for-in bugs
      for (key in new Klass) {
        enumFlag += key == 'valueOf' ? 1 : 0;
      }
      // check if `arguments` objects have non-enumerable indexes
      for (key in arguments) {
        key == '0' && (forArgs = false);
      }
      // Safari 2 iterates over shadowed properties twice
      // http://replay.waybackmachine.org/20090428222941/http://tobielangel.com/2007/1/29/for-in-loop-broken-in-safari/
      skipSeen = enumFlag == 2;
      // IE < 9 incorrectly makes an object's properties non-enumerable if they have
      // the same name as other non-enumerable properties in its prototype chain.
      forShadowed = !enumFlag;
    }(0));

    // lazy define
    forProps = function(object, callback, options) {
      options || (options = {});

      var result = object;
      object = Object(object);

      var ctor,
          key,
          keys,
          skipCtor,
          done = !result,
          which = options.which,
          allFlag = which == 'all',
          index = -1,
          iteratee = object,
          length = object.length,
          ownFlag = allFlag || which == 'own',
          seen = {},
          skipProto = isClassOf(object, 'Function'),
          thisArg = options.bind;

      if (thisArg !== undefined) {
        callback = bind(callback, thisArg);
      }
      // iterate all properties
      if (allFlag && support.getAllKeys) {
        for (index = 0, keys = getAllKeys(object), length = keys.length; index < length; index++) {
          key = keys[index];
          if (callback(object[key], key, object) === false) {
            break;
          }
        }
      }
      // else iterate only enumerable properties
      else {
        for (key in object) {
          // Firefox < 3.6, Opera > 9.50 - Opera < 11.60, and Safari < 5.1
          // (if the prototype or a property on the prototype has been set)
          // incorrectly set a function's `prototype` property [[Enumerable]] value
          // to `true`. Because of this we standardize on skipping the `prototype`
          // property of functions regardless of their [[Enumerable]] value.
          if ((done =
              !(skipProto && key == 'prototype') &&
              !(skipSeen && (hasKey(seen, key) || !(seen[key] = true))) &&
              (!ownFlag || ownFlag && hasKey(object, key)) &&
              callback(object[key], key, object) === false)) {
            break;
          }
        }
        // in IE < 9 strings don't support accessing characters by index
        if (!done && (forArgs && isArguments(object) ||
            ((noCharByIndex || noCharByOwnIndex) && isClassOf(object, 'String') &&
              (iteratee = noCharByIndex ? object.split('') : object)))) {
          while (++index < length) {
            if ((done =
                callback(iteratee[index], String(index), object) === false)) {
              break;
            }
          }
        }
        if (!done && forShadowed) {
          // Because IE < 9 can't set the `[[Enumerable]]` attribute of an existing
          // property and the `constructor` property of a prototype defaults to
          // non-enumerable, we manually skip the `constructor` property when we
          // think we are iterating over a `prototype` object.
          ctor = object.constructor;
          skipCtor = ctor && ctor.prototype && ctor.prototype.constructor === ctor;
          for (index = 0; index < 7; index++) {
            key = shadowed[index];
            if (!(skipCtor && key == 'constructor') &&
                hasKey(object, key) &&
                callback(object[key], key, object) === false) {
              break;
            }
          }
        }
      }
      return result;
    };
    return forProps.apply(null, arguments);
  }

  /**
   * Gets the name of the first argument from a function's source.
   *
   * @private
   * @param {Function} fn The function.
   * @returns {String} The argument name.
   */
  function getFirstArgument(fn) {
    return (!hasKey(fn, 'toString') &&
      (/^[\s(]*function[^(]*\(([^\s,)]+)/.exec(fn) || 0)[1]) || '';
  }

  /**
   * Computes the arithmetic mean of a sample.
   *
   * @private
   * @param {Array} sample The sample.
   * @returns {Number} The mean.
   */
  function getMean(sample) {
    return reduce(sample, function(sum, x) {
      return sum + x;
    }) / sample.length || 0;
  }

  /**
   * Gets the source code of a function.
   *
   * @private
   * @param {Function} fn The function.
   * @param {String} altSource A string used when a function's source code is unretrievable.
   * @returns {String} The function's source code.
   */
  function getSource(fn, altSource) {
    var result = altSource;
    if (isStringable(fn)) {
      result = String(fn);
    } else if (support.decompilation) {
      // escape the `{` for Firefox 1
      result = (/^[^{]+\{([\s\S]*)}\s*$/.exec(fn) || 0)[1];
    }
    // trim string
    result = (result || '').replace(/^\s+|\s+$/g, '');

    // detect strings containing only the "use strict" directive
    return /^(?:\/\*+[\w|\W]*?\*\/|\/\/.*?[\n\r\u2028\u2029]|\s)*(["'])use strict\1;?$/.test(result)
      ? ''
      : result;
  }

  /**
   * Checks if a value is an `arguments` object.
   *
   * @private
   * @param {Mixed} value The value to check.
   * @returns {Boolean} Returns `true` if the value is an `arguments` object, else `false`.
   */
  function isArguments() {
    // lazy define
    isArguments = function(value) {
      return toString.call(value) == '[object Arguments]';
    };
    if (noArgumentsClass) {
      isArguments = function(value) {
        return hasKey(value, 'callee') &&
          !(propertyIsEnumerable && propertyIsEnumerable.call(value, 'callee'));
      };
    }
    return isArguments(arguments[0]);
  }

  /**
   * Checks if an object is of the specified class.
   *
   * @private
   * @param {Mixed} value The value to check.
   * @param {String} name The name of the class.
   * @returns {Boolean} Returns `true` if the value is of the specified class, else `false`.
   */
  function isClassOf(value, name) {
    return value != null && toString.call(value) == '[object ' + name + ']';
  }

  /**
   * Host objects can return type values that are different from their actual
   * data type. The objects we are concerned with usually return non-primitive
   * types of object, function, or unknown.
   *
   * @private
   * @param {Mixed} object The owner of the property.
   * @param {String} property The property to check.
   * @returns {Boolean} Returns `true` if the property value is a non-primitive, else `false`.
   */
  function isHostType(object, property) {
    var type = object != null ? typeof object[property] : 'number';
    return !/^(?:boolean|number|string|undefined)$/.test(type) &&
      (type == 'object' ? !!object[property] : true);
  }

  /**
   * Checks if a given `value` is an object created by the `Object` constructor
   * assuming objects created by the `Object` constructor have no inherited
   * enumerable properties and that there are no `Object.prototype` extensions.
   *
   * @private
   * @param {Mixed} value The value to check.
   * @returns {Boolean} Returns `true` if the `value` is a plain `Object` object, else `false`.
   */
  function isPlainObject(value) {
    // avoid non-objects and false positives for `arguments` objects in IE < 9
    var result = false;
    if (!(value && typeof value == 'object') || (noArgumentsClass && isArguments(value))) {
      return result;
    }
    // IE < 9 presents DOM nodes as `Object` objects except they have `toString`
    // methods that are `typeof` "string" and still can coerce nodes to strings.
    // Also check that the constructor is `Object` (i.e. `Object instanceof Object`)
    var ctor = value.constructor;
    if ((support.nodeClass || !(typeof value.toString != 'function' && typeof (value + '') == 'string')) &&
        (!isClassOf(ctor, 'Function') || ctor instanceof ctor)) {
      // In most environments an object's own properties are iterated before
      // its inherited properties. If the last iterated property is an object's
      // own property then there are no inherited enumerable properties.
      if (support.iteratesOwnFirst) {
        forProps(value, function(subValue, subKey) {
          result = subKey;
        });
        return result === false || hasKey(value, result);
      }
      // IE < 9 iterates inherited properties before own properties. If the first
      // iterated property is an object's own property then there are no inherited
      // enumerable properties.
      forProps(value, function(subValue, subKey) {
        result = !hasKey(value, subKey);
        return false;
      });
      return result === false;
    }
    return result;
  }

  /**
   * Checks if a value can be safely coerced to a string.
   *
   * @private
   * @param {Mixed} value The value to check.
   * @returns {Boolean} Returns `true` if the value can be coerced, else `false`.
   */
  function isStringable(value) {
    return hasKey(value, 'toString') || isClassOf(value, 'String');
  }

  /**
   * Wraps a function and passes `this` to the original function as the
   * first argument.
   *
   * @private
   * @param {Function} fn The function to be wrapped.
   * @returns {Function} The new function.
   */
  function methodize(fn) {
    return function() {
      var args = [this];
      args.push.apply(args, arguments);
      return fn.apply(null, args);
    };
  }

  /**
   * A no-operation function.
   *
   * @private
   */
  function noop() {
    // no operation performed
  }

  /**
   * A wrapper around require() to suppress `module missing` errors.
   *
   * @private
   * @param {String} id The module id.
   * @returns {Mixed} The exported module or `null`.
   */
  function req(id) {
    try {
      var result = freeExports && freeRequire(id);
    } catch(e) { }
    return result || null;
  }

  /**
   * Runs a snippet of JavaScript via script injection.
   *
   * @private
   * @param {String} code The code to run.
   */
  function runScript(code) {
    var anchor = freeDefine ? define.amd : Benchmark,
        script = doc.createElement('script'),
        sibling = doc.getElementsByTagName('script')[0],
        parent = sibling.parentNode,
        prop = uid + 'runScript',
        prefix = '(' + (freeDefine ? 'define.amd.' : 'Benchmark.') + prop + '||function(){})();';

    // Firefox 2.0.0.2 cannot use script injection as intended because it executes
    // asynchronously, but that's OK because script injection is only used to avoid
    // the previously commented JaegerMonkey bug.
    try {
      // remove the inserted script *before* running the code to avoid differences
      // in the expected script element count/order of the document.
      script.appendChild(doc.createTextNode(prefix + code));
      anchor[prop] = function() { destroyElement(script); };
    } catch(e) {
      parent = parent.cloneNode(false);
      sibling = null;
      script.text = code;
    }
    parent.insertBefore(script, sibling);
    delete anchor[prop];
  }

  /**
   * A helper function for setting options/event handlers.
   *
   * @private
   * @param {Object} bench The benchmark instance.
   * @param {Object} [options={}] Options object.
   */
  function setOptions(bench, options) {
    options = extend({}, bench.constructor.options, options);
    bench.options = forOwn(options, function(value, key) {
      if (value != null) {
        // add event listeners
        if (/^on[A-Z]/.test(key)) {
          forEach(key.split(' '), function(key) {
            bench.on(key.slice(2).toLowerCase(), value);
          });
        } else if (!hasKey(bench, key)) {
          bench[key] = deepClone(value);
        }
      }
    });
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Handles cycling/completing the deferred benchmark.
   *
   * @memberOf Benchmark.Deferred
   */
  function resolve() {
    var me = this,
        clone = me.benchmark,
        bench = clone._original;

    if (bench.aborted) {
      // cycle() -> clone cycle/complete event -> compute()'s invoked bench.run() cycle/complete
      me.teardown();
      clone.running = false;
      cycle(me);
    }
    else if (++me.cycles < clone.count) {
      // continue the test loop
      if (support.timeout) {
        // use setTimeout to avoid a call stack overflow if called recursively
        setTimeout(function() { clone.compiled.call(me, timer); }, 0);
      } else {
        clone.compiled.call(me, timer);
      }
    }
    else {
      timer.stop(me);
      me.teardown();
      delay(clone, function() { cycle(me); });
    }
  }

  /*--------------------------------------------------------------------------*/

  /**
   * A deep clone utility.
   *
   * @static
   * @memberOf Benchmark
   * @param {Mixed} value The value to clone.
   * @returns {Mixed} The cloned value.
   */
  function deepClone(value) {
    var accessor,
        circular,
        clone,
        ctor,
        descriptor,
        extensible,
        key,
        length,
        markerKey,
        parent,
        result,
        source,
        subIndex,
        data = { 'value': value },
        index = 0,
        marked = [],
        queue = { 'length': 0 },
        unmarked = [];

    /**
     * An easily detectable decorator for cloned values.
     */
    function Marker(object) {
      this.raw = object;
    }

    /**
     * The callback used by `forProps()`.
     */
    function forPropsCallback(subValue, subKey) {
      // exit early to avoid cloning the marker
      if (subValue && subValue.constructor == Marker) {
        return;
      }
      // add objects to the queue
      if (subValue === Object(subValue)) {
        queue[queue.length++] = { 'key': subKey, 'parent': clone, 'source': value };
      }
      // assign non-objects
      else {
        try {
          // will throw an error in strict mode if the property is read-only
          clone[subKey] = subValue;
        } catch(e) { }
      }
    }

    /**
     * Gets an available marker key for the given object.
     */
    function getMarkerKey(object) {
      // avoid collisions with existing keys
      var result = uid;
      while (object[result] && object[result].constructor != Marker) {
        result += 1;
      }
      return result;
    }

    do {
      key = data.key;
      parent = data.parent;
      source = data.source;
      clone = value = source ? source[key] : data.value;
      accessor = circular = descriptor = false;

      // create a basic clone to filter out functions, DOM elements, and
      // other non `Object` objects
      if (value === Object(value)) {
        // use custom deep clone function if available
        if (isClassOf(value.deepClone, 'Function')) {
          clone = value.deepClone();
        } else {
          ctor = value.constructor;
          switch (toString.call(value)) {
            case '[object Array]':
              clone = new ctor(value.length);
              break;

            case '[object Boolean]':
              clone = new ctor(value == true);
              break;

            case '[object Date]':
              clone = new ctor(+value);
              break;

            case '[object Object]':
              isPlainObject(value) && (clone = {});
              break;

            case '[object Number]':
            case '[object String]':
              clone = new ctor(value);
              break;

            case '[object RegExp]':
              clone = ctor(value.source,
                (value.global     ? 'g' : '') +
                (value.ignoreCase ? 'i' : '') +
                (value.multiline  ? 'm' : ''));
          }
        }
        // continue clone if `value` doesn't have an accessor descriptor
        // http://es5.github.com/#x8.10.1
        if (clone && clone != value &&
            !(descriptor = source && support.descriptors && getDescriptor(source, key),
              accessor = descriptor && (descriptor.get || descriptor.set))) {
          // use an existing clone (circular reference)
          if ((extensible = isExtensible(value))) {
            markerKey = getMarkerKey(value);
            if (value[markerKey]) {
              circular = clone = value[markerKey].raw;
            }
          } else {
            // for frozen/sealed objects
            for (subIndex = 0, length = unmarked.length; subIndex < length; subIndex++) {
              data = unmarked[subIndex];
              if (data.object === value) {
                circular = clone = data.clone;
                break;
              }
            }
          }
          if (!circular) {
            // mark object to allow quickly detecting circular references and tie it to its clone
            if (extensible) {
              value[markerKey] = new Marker(clone);
              marked.push({ 'key': markerKey, 'object': value });
            } else {
              // for frozen/sealed objects
              unmarked.push({ 'clone': clone, 'object': value });
            }
            // iterate over object properties
            forProps(value, forPropsCallback, { 'which': 'all' });
          }
        }
      }
      if (parent) {
        // for custom property descriptors
        if (accessor || (descriptor && !(descriptor.configurable && descriptor.enumerable && descriptor.writable))) {
          if ('value' in descriptor) {
            descriptor.value = clone;
          }
          setDescriptor(parent, key, descriptor);
        }
        // for default property descriptors
        else {
          parent[key] = clone;
        }
      } else {
        result = clone;
      }
    } while ((data = queue[index++]));

    // remove markers
    for (index = 0, length = marked.length; index < length; index++) {
      data = marked[index];
      delete data.object[data.key];
    }
    return result;
  }

  /**
   * An iteration utility for arrays and objects.
   * Callbacks may terminate the loop by explicitly returning `false`.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array|Object} object The object to iterate over.
   * @param {Function} callback The function called per iteration.
   * @param {Mixed} thisArg The `this` binding for the callback.
   * @returns {Array|Object} Returns the object iterated over.
   */
  function each(object, callback, thisArg) {
    var result = object;
    object = Object(object);

    var fn = callback,
        index = -1,
        length = object.length,
        isSnapshot = !!(object.snapshotItem && (length = object.snapshotLength)),
        isSplittable = (noCharByIndex || noCharByOwnIndex) && isClassOf(object, 'String'),
        isConvertable = isSnapshot || isSplittable || 'item' in object,
        origObject = object;

    // in Opera < 10.5 `hasKey(object, 'length')` returns `false` for NodeLists
    if (length === length >>> 0) {
      if (isConvertable) {
        // the third argument of the callback is the original non-array object
        callback = function(value, index) {
          return fn.call(this, value, index, origObject);
        };
        // in IE < 9 strings don't support accessing characters by index
        if (isSplittable) {
          object = object.split('');
        } else {
          object = [];
          while (++index < length) {
            // in Safari 2 `index in object` is always `false` for NodeLists
            object[index] = isSnapshot ? result.snapshotItem(index) : result[index];
          }
        }
      }
      forEach(object, callback, thisArg);
    } else {
      forOwn(object, callback, thisArg);
    }
    return result;
  }

  /**
   * Copies enumerable properties from the source(s) object to the destination object.
   *
   * @static
   * @memberOf Benchmark
   * @param {Object} destination The destination object.
   * @param {Object} [source={}] The source object.
   * @returns {Object} The destination object.
   */
  function extend(destination, source) {
    // Chrome < 14 incorrectly sets `destination` to `undefined` when we `delete arguments[0]`
    // http://code.google.com/p/v8/issues/detail?id=839
    var result = destination;
    delete arguments[0];

    forEach(arguments, function(source) {
      forProps(source, function(value, key) {
        result[key] = value;
      });
    });
    return result;
  }

  /**
   * A generic `Array#filter` like method.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} array The array to iterate over.
   * @param {Function|String} callback The function/alias called per iteration.
   * @param {Mixed} thisArg The `this` binding for the callback.
   * @returns {Array} A new array of values that passed callback filter.
   * @example
   *
   * // get odd numbers
   * Benchmark.filter([1, 2, 3, 4, 5], function(n) {
   *   return n % 2;
   * }); // -> [1, 3, 5];
   *
   * // get fastest benchmarks
   * Benchmark.filter(benches, 'fastest');
   *
   * // get slowest benchmarks
   * Benchmark.filter(benches, 'slowest');
   *
   * // get benchmarks that completed without erroring
   * Benchmark.filter(benches, 'successful');
   */
  function filter(array, callback, thisArg) {
    var result;

    if (callback == 'successful') {
      // callback to exclude those that are errored, unrun, or have hz of Infinity
      callback = function(bench) { return bench.cycles && isFinite(bench.hz); };
    }
    else if (callback == 'fastest' || callback == 'slowest') {
      // get successful, sort by period + margin of error, and filter fastest/slowest
      result = filter(array, 'successful').sort(function(a, b) {
        a = a.stats; b = b.stats;
        return (a.mean + a.moe > b.mean + b.moe ? 1 : -1) * (callback == 'fastest' ? 1 : -1);
      });
      result = filter(result, function(bench) {
        return result[0].compare(bench) == 0;
      });
    }
    return result || reduce(array, function(result, value, index) {
      return callback.call(thisArg, value, index, array) ? (result.push(value), result) : result;
    }, []);
  }

  /**
   * A generic `Array#forEach` like method.
   * Callbacks may terminate the loop by explicitly returning `false`.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} array The array to iterate over.
   * @param {Function} callback The function called per iteration.
   * @param {Mixed} thisArg The `this` binding for the callback.
   * @returns {Array} Returns the array iterated over.
   */
  function forEach(array, callback, thisArg) {
    var index = -1,
        length = (array = Object(array)).length >>> 0;

    if (thisArg !== undefined) {
      callback = bind(callback, thisArg);
    }
    while (++index < length) {
      if (index in array &&
          callback(array[index], index, array) === false) {
        break;
      }
    }
    return array;
  }

  /**
   * Iterates over an object's own properties, executing the `callback` for each.
   * Callbacks may terminate the loop by explicitly returning `false`.
   *
   * @static
   * @memberOf Benchmark
   * @param {Object} object The object to iterate over.
   * @param {Function} callback The function executed per own property.
   * @param {Mixed} thisArg The `this` binding for the callback.
   * @returns {Object} Returns the object iterated over.
   */
  function forOwn(object, callback, thisArg) {
    return forProps(object, callback, { 'bind': thisArg, 'which': 'own' });
  }

  /**
   * Converts a number to a more readable comma-separated string representation.
   *
   * @static
   * @memberOf Benchmark
   * @param {Number} number The number to convert.
   * @returns {String} The more readable string representation.
   */
  function formatNumber(number) {
    number = String(number).split('.');
    return number[0].replace(/(?=(?:\d{3})+$)(?!\b)/g, ',') +
      (number[1] ? '.' + number[1] : '');
  }

  /**
   * Checks if an object has the specified key as a direct property.
   *
   * @static
   * @memberOf Benchmark
   * @param {Object} object The object to check.
   * @param {String} key The key to check for.
   * @returns {Boolean} Returns `true` if key is a direct property, else `false`.
   */
  function hasKey() {
    // lazy define for worst case fallback (not as accurate)
    hasKey = function(object, key) {
      var parent = object != null && (object.constructor || Object).prototype;
      return !!parent && key in Object(object) && !(key in parent && object[key] === parent[key]);
    };
    // for modern browsers
    if (isClassOf(hasOwnProperty, 'Function')) {
      hasKey = function(object, key) {
        return object != null && hasOwnProperty.call(object, key);
      };
    }
    // for Safari 2
    else if ({}.__proto__ == Object.prototype) {
      hasKey = function(object, key) {
        var result = false;
        if (object != null) {
          object = Object(object);
          object.__proto__ = [object.__proto__, object.__proto__ = null, result = key in object][0];
        }
        return result;
      };
    }
    return hasKey.apply(this, arguments);
  }

  /**
   * A generic `Array#indexOf` like method.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} array The array to iterate over.
   * @param {Mixed} value The value to search for.
   * @param {Number} [fromIndex=0] The index to start searching from.
   * @returns {Number} The index of the matched value or `-1`.
   */
  function indexOf(array, value, fromIndex) {
    var index = toInteger(fromIndex),
        length = (array = Object(array)).length >>> 0;

    index = (index < 0 ? max(0, length + index) : index) - 1;
    while (++index < length) {
      if (index in array && value === array[index]) {
        return index;
      }
    }
    return -1;
  }

  /**
   * Modify a string by replacing named tokens with matching object property values.
   *
   * @static
   * @memberOf Benchmark
   * @param {String} string The string to modify.
   * @param {Object} object The template object.
   * @returns {String} The modified string.
   */
  function interpolate(string, object) {
    forOwn(object, function(value, key) {
      // escape regexp special characters in `key`
      string = string.replace(RegExp('#\\{' + key.replace(/([.*+?^=!:${}()|[\]\/\\])/g, '\\$1') + '\\}', 'g'), value);
    });
    return string;
  }

  /**
   * Invokes a method on all items in an array.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} benches Array of benchmarks to iterate over.
   * @param {String|Object} name The name of the method to invoke OR options object.
   * @param {Mixed} [arg1, arg2, ...] Arguments to invoke the method with.
   * @returns {Array} A new array of values returned from each method invoked.
   * @example
   *
   * // invoke `reset` on all benchmarks
   * Benchmark.invoke(benches, 'reset');
   *
   * // invoke `emit` with arguments
   * Benchmark.invoke(benches, 'emit', 'complete', listener);
   *
   * // invoke `run(true)`, treat benchmarks as a queue, and register invoke callbacks
   * Benchmark.invoke(benches, {
   *
   *   // invoke the `run` method
   *   'name': 'run',
   *
   *   // pass a single argument
   *   'args': true,
   *
   *   // treat as queue, removing benchmarks from front of `benches` until empty
   *   'queued': true,
   *
   *   // called before any benchmarks have been invoked.
   *   'onStart': onStart,
   *
   *   // called between invoking benchmarks
   *   'onCycle': onCycle,
   *
   *   // called after all benchmarks have been invoked.
   *   'onComplete': onComplete
   * });
   */
  function invoke(benches, name) {
    var args,
        bench,
        queued,
        index = -1,
        eventProps = { 'currentTarget': benches },
        options = { 'onStart': noop, 'onCycle': noop, 'onComplete': noop },
        result = map(benches, function(bench) { return bench; });

    /**
     * Invokes the method of the current object and if synchronous, fetches the next.
     */
    function execute() {
      var listeners,
          async = isAsync(bench);

      if (async) {
        // use `getNext` as the first listener
        bench.on('complete', getNext);
        listeners = bench.events.complete;
        listeners.splice(0, 0, listeners.pop());
      }
      // execute method
      result[index] = isClassOf(bench && bench[name], 'Function') ? bench[name].apply(bench, args) : undefined;
      // if synchronous return true until finished
      return !async && getNext();
    }

    /**
     * Fetches the next bench or executes `onComplete` callback.
     */
    function getNext(event) {
      var cycleEvent,
          last = bench,
          async = isAsync(last);

      if (async) {
        last.off('complete', getNext);
        last.emit('complete');
      }
      // emit "cycle" event
      eventProps.type = 'cycle';
      eventProps.target = last;
      cycleEvent = Event(eventProps);
      options.onCycle.call(benches, cycleEvent);

      // choose next benchmark if not exiting early
      if (!cycleEvent.aborted && raiseIndex() !== false) {
        bench = queued ? benches[0] : result[index];
        if (isAsync(bench)) {
          delay(bench, execute);
        }
        else if (async) {
          // resume execution if previously asynchronous but now synchronous
          while (execute()) { }
        }
        else {
          // continue synchronous execution
          return true;
        }
      } else {
        // emit "complete" event
        eventProps.type = 'complete';
        options.onComplete.call(benches, Event(eventProps));
      }
      // When used as a listener `event.aborted = true` will cancel the rest of
      // the "complete" listeners because they were already called above and when
      // used as part of `getNext` the `return false` will exit the execution while-loop.
      if (event) {
        event.aborted = true;
      } else {
        return false;
      }
    }

    /**
     * Checks if invoking `Benchmark#run` with asynchronous cycles.
     */
    function isAsync(object) {
      // avoid using `instanceof` here because of IE memory leak issues with host objects
      var async = args[0] && args[0].async;
      return Object(object).constructor == Benchmark && name == 'run' &&
        ((async == null ? object.options.async : async) && support.timeout || object.defer);
    }

    /**
     * Raises `index` to the next defined index or returns `false`.
     */
    function raiseIndex() {
      var length = result.length;
      if (queued) {
        // if queued remove the previous bench and subsequent skipped non-entries
        do {
          ++index > 0 && shift.call(benches);
        } while ((length = benches.length) && !('0' in benches));
      }
      else {
        while (++index < length && !(index in result)) { }
      }
      // if we reached the last index then return `false`
      return (queued ? length : index < length) ? index : (index = false);
    }

    // juggle arguments
    if (isClassOf(name, 'String')) {
      // 2 arguments (array, name)
      args = slice.call(arguments, 2);
    } else {
      // 2 arguments (array, options)
      options = extend(options, name);
      name = options.name;
      args = isClassOf(args = 'args' in options ? options.args : [], 'Array') ? args : [args];
      queued = options.queued;
    }

    // start iterating over the array
    if (raiseIndex() !== false) {
      // emit "start" event
      bench = result[index];
      eventProps.type = 'start';
      eventProps.target = bench;
      options.onStart.call(benches, Event(eventProps));

      // end early if the suite was aborted in an "onStart" listener
      if (benches.aborted && benches.constructor == Suite && name == 'run') {
        // emit "cycle" event
        eventProps.type = 'cycle';
        options.onCycle.call(benches, Event(eventProps));
        // emit "complete" event
        eventProps.type = 'complete';
        options.onComplete.call(benches, Event(eventProps));
      }
      // else start
      else {
        if (isAsync(bench)) {
          delay(bench, execute);
        } else {
          while (execute()) { }
        }
      }
    }
    return result;
  }

  /**
   * Creates a string of joined array values or object key-value pairs.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array|Object} object The object to operate on.
   * @param {String} [separator1=','] The separator used between key-value pairs.
   * @param {String} [separator2=': '] The separator used between keys and values.
   * @returns {String} The joined result.
   */
  function join(object, separator1, separator2) {
    var result = [],
        length = (object = Object(object)).length,
        arrayLike = length === length >>> 0;

    separator2 || (separator2 = ': ');
    each(object, function(value, key) {
      result.push(arrayLike ? value : key + separator2 + value);
    });
    return result.join(separator1 || ',');
  }

  /**
   * A generic `Array#map` like method.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} array The array to iterate over.
   * @param {Function} callback The function called per iteration.
   * @param {Mixed} thisArg The `this` binding for the callback.
   * @returns {Array} A new array of values returned by the callback.
   */
  function map(array, callback, thisArg) {
    return reduce(array, function(result, value, index) {
      result[index] = callback.call(thisArg, value, index, array);
      return result;
    }, Array(Object(array).length >>> 0));
  }

  /**
   * Retrieves the value of a specified property from all items in an array.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} array The array to iterate over.
   * @param {String} property The property to pluck.
   * @returns {Array} A new array of property values.
   */
  function pluck(array, property) {
    return map(array, function(object) {
      return object == null ? undefined : object[property];
    });
  }

  /**
   * A generic `Array#reduce` like method.
   *
   * @static
   * @memberOf Benchmark
   * @param {Array} array The array to iterate over.
   * @param {Function} callback The function called per iteration.
   * @param {Mixed} accumulator Initial value of the accumulator.
   * @returns {Mixed} The accumulator.
   */
  function reduce(array, callback, accumulator) {
    var noaccum = arguments.length < 3;
    forEach(array, function(value, index) {
      accumulator = noaccum ? (noaccum = false, value) : callback(accumulator, value, index, array);
    });
    return accumulator;
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Aborts all benchmarks in the suite.
   *
   * @name abort
   * @memberOf Benchmark.Suite
   * @returns {Object} The suite instance.
   */
  function abortSuite() {
    var event,
        me = this,
        resetting = calledBy.resetSuite;

    if (me.running) {
      event = Event('abort');
      me.emit(event);
      if (!event.cancelled || resetting) {
        // avoid infinite recursion
        calledBy.abortSuite = true;
        me.reset();
        delete calledBy.abortSuite;

        if (!resetting) {
          me.aborted = true;
          invoke(me, 'abort');
        }
      }
    }
    return me;
  }

  /**
   * Adds a test to the benchmark suite.
   *
   * @memberOf Benchmark.Suite
   * @param {String} name A name to identify the benchmark.
   * @param {Function|String} fn The test to benchmark.
   * @param {Object} [options={}] Options object.
   * @returns {Object} The benchmark instance.
   * @example
   *
   * // basic usage
   * suite.add(fn);
   *
   * // or using a name first
   * suite.add('foo', fn);
   *
   * // or with options
   * suite.add('foo', fn, {
   *   'onCycle': onCycle,
   *   'onComplete': onComplete
   * });
   *
   * // or name and options
   * suite.add('foo', {
   *   'fn': fn,
   *   'onCycle': onCycle,
   *   'onComplete': onComplete
   * });
   *
   * // or options only
   * suite.add({
   *   'name': 'foo',
   *   'fn': fn,
   *   'onCycle': onCycle,
   *   'onComplete': onComplete
   * });
   */
  function add(name, fn, options) {
    var me = this,
        bench = Benchmark(name, fn, options),
        event = Event({ 'type': 'add', 'target': bench });

    if (me.emit(event), !event.cancelled) {
      me.push(bench);
    }
    return me;
  }

  /**
   * Creates a new suite with cloned benchmarks.
   *
   * @name clone
   * @memberOf Benchmark.Suite
   * @param {Object} options Options object to overwrite cloned options.
   * @returns {Object} The new suite instance.
   */
  function cloneSuite(options) {
    var me = this,
        result = new me.constructor(extend({}, me.options, options));

    // copy own properties
    forOwn(me, function(value, key) {
      if (!hasKey(result, key)) {
        result[key] = value && isClassOf(value.clone, 'Function')
          ? value.clone()
          : deepClone(value);
      }
    });
    return result;
  }

  /**
   * An `Array#filter` like method.
   *
   * @name filter
   * @memberOf Benchmark.Suite
   * @param {Function|String} callback The function/alias called per iteration.
   * @returns {Object} A new suite of benchmarks that passed callback filter.
   */
  function filterSuite(callback) {
    var me = this,
        result = new me.constructor;

    result.push.apply(result, filter(me, callback));
    return result;
  }

  /**
   * Resets all benchmarks in the suite.
   *
   * @name reset
   * @memberOf Benchmark.Suite
   * @returns {Object} The suite instance.
   */
  function resetSuite() {
    var event,
        me = this,
        aborting = calledBy.abortSuite;

    if (me.running && !aborting) {
      // no worries, `resetSuite()` is called within `abortSuite()`
      calledBy.resetSuite = true;
      me.abort();
      delete calledBy.resetSuite;
    }
    // reset if the state has changed
    else if ((me.aborted || me.running) &&
        (me.emit(event = Event('reset')), !event.cancelled)) {
      me.running = false;
      if (!aborting) {
        invoke(me, 'reset');
      }
    }
    return me;
  }

  /**
   * Runs the suite.
   *
   * @name run
   * @memberOf Benchmark.Suite
   * @param {Object} [options={}] Options object.
   * @returns {Object} The suite instance.
   * @example
   *
   * // basic usage
   * suite.run();
   *
   * // or with options
   * suite.run({ 'async': true, 'queued': true });
   */
  function runSuite(options) {
    var me = this;

    me.reset();
    me.running = true;
    options || (options = {});

    invoke(me, {
      'name': 'run',
      'args': options,
      'queued': options.queued,
      'onStart': function(event) {
        me.emit(event);
      },
      'onCycle': function(event) {
        var bench = event.target;
        if (bench.error) {
          me.emit({ 'type': 'error', 'target': bench });
        }
        me.emit(event);
        event.aborted = me.aborted;
      },
      'onComplete': function(event) {
        me.running = false;
        me.emit(event);
      }
    });
    return me;
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Executes all registered listeners of the specified event type.
   *
   * @memberOf Benchmark, Benchmark.Suite
   * @param {String|Object} type The event type or object.
   * @returns {Mixed} Returns the return value of the last listener executed.
   */
  function emit(type) {
    var listeners,
        me = this,
        event = Event(type),
        events = me.events,
        args = (arguments[0] = event, arguments);

    event.currentTarget || (event.currentTarget = me);
    event.target || (event.target = me);
    delete event.result;

    if (events && (listeners = hasKey(events, event.type) && events[event.type])) {
      forEach(listeners.slice(), function(listener) {
        if ((event.result = listener.apply(me, args)) === false) {
          event.cancelled = true;
        }
        return !event.aborted;
      });
    }
    return event.result;
  }

  /**
   * Returns an array of event listeners for a given type that can be manipulated
   * to add or remove listeners.
   *
   * @memberOf Benchmark, Benchmark.Suite
   * @param {String} type The event type.
   * @returns {Array} The listeners array.
   */
  function listeners(type) {
    var me = this,
        events = me.events || (me.events = {});

    return hasKey(events, type) ? events[type] : (events[type] = []);
  }

  /**
   * Unregisters a listener for the specified event type(s),
   * or unregisters all listeners for the specified event type(s),
   * or unregisters all listeners for all event types.
   *
   * @memberOf Benchmark, Benchmark.Suite
   * @param {String} [type] The event type.
   * @param {Function} [listener] The function to unregister.
   * @returns {Object} The benchmark instance.
   * @example
   *
   * // unregister a listener for an event type
   * bench.off('cycle', listener);
   *
   * // unregister a listener for multiple event types
   * bench.off('start cycle', listener);
   *
   * // unregister all listeners for an event type
   * bench.off('cycle');
   *
   * // unregister all listeners for multiple event types
   * bench.off('start cycle complete');
   *
   * // unregister all listeners for all event types
   * bench.off();
   */
  function off(type, listener) {
    var me = this,
        events = me.events;

    events && each(type ? type.split(' ') : events, function(listeners, type) {
      var index;
      if (typeof listeners == 'string') {
        type = listeners;
        listeners = hasKey(events, type) && events[type];
      }
      if (listeners) {
        if (listener) {
          index = indexOf(listeners, listener);
          if (index > -1) {
            listeners.splice(index, 1);
          }
        } else {
          listeners.length = 0;
        }
      }
    });
    return me;
  }

  /**
   * Registers a listener for the specified event type(s).
   *
   * @memberOf Benchmark, Benchmark.Suite
   * @param {String} type The event type.
   * @param {Function} listener The function to register.
   * @returns {Object} The benchmark instance.
   * @example
   *
   * // register a listener for an event type
   * bench.on('cycle', listener);
   *
   * // register a listener for multiple event types
   * bench.on('start cycle', listener);
   */
  function on(type, listener) {
    var me = this,
        events = me.events || (me.events = {});

    forEach(type.split(' '), function(type) {
      (hasKey(events, type)
        ? events[type]
        : (events[type] = [])
      ).push(listener);
    });
    return me;
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Aborts the benchmark without recording times.
   *
   * @memberOf Benchmark
   * @returns {Object} The benchmark instance.
   */
  function abort() {
    var event,
        me = this,
        resetting = calledBy.reset;

    if (me.running) {
      event = Event('abort');
      me.emit(event);
      if (!event.cancelled || resetting) {
        // avoid infinite recursion
        calledBy.abort = true;
        me.reset();
        delete calledBy.abort;

        if (support.timeout) {
          clearTimeout(me._timerId);
          delete me._timerId;
        }
        if (!resetting) {
          me.aborted = true;
          me.running = false;
        }
      }
    }
    return me;
  }

  /**
   * Creates a new benchmark using the same test and options.
   *
   * @memberOf Benchmark
   * @param {Object} options Options object to overwrite cloned options.
   * @returns {Object} The new benchmark instance.
   * @example
   *
   * var bizarro = bench.clone({
   *   'name': 'doppelganger'
   * });
   */
  function clone(options) {
    var me = this,
        result = new me.constructor(extend({}, me, options));

    // correct the `options` object
    result.options = extend({}, me.options, options);

    // copy own custom properties
    forOwn(me, function(value, key) {
      if (!hasKey(result, key)) {
        result[key] = deepClone(value);
      }
    });
    return result;
  }

  /**
   * Determines if a benchmark is faster than another.
   *
   * @memberOf Benchmark
   * @param {Object} other The benchmark to compare.
   * @returns {Number} Returns `-1` if slower, `1` if faster, and `0` if indeterminate.
   */
  function compare(other) {
    var critical,
        zStat,
        me = this,
        sample1 = me.stats.sample,
        sample2 = other.stats.sample,
        size1 = sample1.length,
        size2 = sample2.length,
        maxSize = max(size1, size2),
        minSize = min(size1, size2),
        u1 = getU(sample1, sample2),
        u2 = getU(sample2, sample1),
        u = min(u1, u2);

    function getScore(xA, sampleB) {
      return reduce(sampleB, function(total, xB) {
        return total + (xB > xA ? 0 : xB < xA ? 1 : 0.5);
      }, 0);
    }

    function getU(sampleA, sampleB) {
      return reduce(sampleA, function(total, xA) {
        return total + getScore(xA, sampleB);
      }, 0);
    }

    function getZ(u) {
      return (u - ((size1 * size2) / 2)) / sqrt((size1 * size2 * (size1 + size2 + 1)) / 12);
    }

    // exit early if comparing the same benchmark
    if (me == other) {
      return 0;
    }
    // reject the null hyphothesis the two samples come from the
    // same population (i.e. have the same median) if...
    if (size1 + size2 > 30) {
      // ...the z-stat is greater than 1.96 or less than -1.96
      // http://www.statisticslectures.com/topics/mannwhitneyu/
      zStat = getZ(u);
      return abs(zStat) > 1.96 ? (zStat > 0 ? -1 : 1) : 0;
    }
    // ...the U value is less than or equal the critical U value
    // http://www.geoib.com/mann-whitney-u-test.html
    critical = maxSize < 5 || minSize < 3 ? 0 : uTable[maxSize][minSize - 3];
    return u <= critical ? (u == u1 ? 1 : -1) : 0;
  }

  /**
   * Reset properties and abort if running.
   *
   * @memberOf Benchmark
   * @returns {Object} The benchmark instance.
   */
  function reset() {
    var data,
        event,
        me = this,
        index = 0,
        changes = { 'length': 0 },
        queue = { 'length': 0 };

    if (me.running && !calledBy.abort) {
      // no worries, `reset()` is called within `abort()`
      calledBy.reset = true;
      me.abort();
      delete calledBy.reset;
    }
    else {
      // a non-recursive solution to check if properties have changed
      // http://www.jslab.dk/articles/non.recursive.preorder.traversal.part4
      data = { 'destination': me, 'source': extend({}, me.constructor.prototype, me.options) };
      do {
        forOwn(data.source, function(value, key) {
          var changed,
              destination = data.destination,
              currValue = destination[key];

          if (value && typeof value == 'object') {
            if (isClassOf(value, 'Array')) {
              // check if an array value has changed to a non-array value
              if (!isClassOf(currValue, 'Array')) {
                changed = currValue = [];
              }
              // or has changed its length
              if (currValue.length != value.length) {
                changed = currValue = currValue.slice(0, value.length);
                currValue.length = value.length;
              }
            }
            // check if an object has changed to a non-object value
            else if (!currValue || typeof currValue != 'object') {
              changed = currValue = {};
            }
            // register a changed object
            if (changed) {
              changes[changes.length++] = { 'destination': destination, 'key': key, 'value': currValue };
            }
            queue[queue.length++] = { 'destination': currValue, 'source': value };
          }
          // register a changed primitive
          else if (value !== currValue && !(value == null || isClassOf(value, 'Function'))) {
            changes[changes.length++] = { 'destination': destination, 'key': key, 'value': value };
          }
        });
      }
      while ((data = queue[index++]));

      // if changed emit the `reset` event and if it isn't cancelled reset the benchmark
      if (changes.length && (me.emit(event = Event('reset')), !event.cancelled)) {
        forEach(changes, function(data) {
          data.destination[data.key] = data.value;
        });
      }
    }
    return me;
  }

  /**
   * Displays relevant benchmark information when coerced to a string.
   *
   * @name toString
   * @memberOf Benchmark
   * @returns {String} A string representation of the benchmark instance.
   */
  function toStringBench() {
    var me = this,
        error = me.error,
        hz = me.hz,
        id = me.id,
        stats = me.stats,
        size = stats.sample.length,
        pm = support.java ? '+/-' : '\xb1',
        result = me.name || (isNaN(id) ? id : '<Test #' + id + '>');

    if (error) {
      result += ': ' + join(error);
    } else {
      result += ' x ' + formatNumber(hz.toFixed(hz < 100 ? 2 : 0)) + ' ops/sec ' + pm +
        stats.rme.toFixed(2) + '% (' + size + ' run' + (size == 1 ? '' : 's') + ' sampled)';
    }
    return result;
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Clocks the time taken to execute a test per cycle (secs).
   *
   * @private
   * @param {Object} bench The benchmark instance.
   * @returns {Number} The time taken.
   */
  function clock() {
    var applet,
        options = Benchmark.options,
        template = { 'begin': 's$=new n$', 'end': 'r$=(new n$-s$)/1e3', 'uid': uid },
        timers = [{ 'ns': timer.ns, 'res': max(0.0015, getRes('ms')), 'unit': 'ms' }];

    // lazy define for hi-res timers
    clock = function(clone) {
      var deferred;
      if (clone instanceof Deferred) {
        deferred = clone;
        clone = deferred.benchmark;
      }

      var bench = clone._original,
          fn = bench.fn,
          fnArg = deferred ? getFirstArgument(fn) || 'deferred' : '',
          stringable = isStringable(fn);

      var source = {
        'setup': getSource(bench.setup, preprocess('m$.setup()')),
        'fn': getSource(fn, preprocess('m$.fn(' + fnArg + ')')),
        'fnArg': fnArg,
        'teardown': getSource(bench.teardown, preprocess('m$.teardown()'))
      };

      var count = bench.count = clone.count,
          decompilable = support.decompilation || stringable,
          id = bench.id,
          isEmpty = !(source.fn || stringable),
          name = bench.name || (typeof id == 'number' ? '<Test #' + id + '>' : id),
          ns = timer.ns,
          result = 0;

      // init `minTime` if needed
      clone.minTime = bench.minTime || (bench.minTime = bench.options.minTime = options.minTime);

      // repair nanosecond timer
      // (some Chrome builds erase the `ns` variable after millions of executions)
      if (applet) {
        try {
          ns.nanoTime();
        } catch(e) {
          // use non-element to avoid issues with libs that augment them
          ns = timer.ns = new applet.Packages.nano;
        }
      }

      // Compile in setup/teardown functions and the test loop.
      // Create a new compiled test, instead of using the cached `bench.compiled`,
      // to avoid potential engine optimizations enabled over the life of the test.
      var compiled = bench.compiled = createFunction(preprocess('t$'), interpolate(
        preprocess(deferred
          ? 'var d$=this,#{fnArg}=d$,m$=d$.benchmark._original,f$=m$.fn,su$=m$.setup,td$=m$.teardown;' +
            // when `deferred.cycles` is `0` then...
            'if(!d$.cycles){' +
            // set `deferred.fn`
            'd$.fn=function(){var #{fnArg}=d$;if(typeof f$=="function"){try{#{fn}\n}catch(e$){f$(d$)}}else{#{fn}\n}};' +
            // set `deferred.teardown`
            'd$.teardown=function(){d$.cycles=0;if(typeof td$=="function"){try{#{teardown}\n}catch(e$){td$()}}else{#{teardown}\n}};' +
            // execute the benchmark's `setup`
            'if(typeof su$=="function"){try{#{setup}\n}catch(e$){su$()}}else{#{setup}\n};' +
            // start timer
            't$.start(d$);' +
            // execute `deferred.fn` and return a dummy object
            '}d$.fn();return{}'

          : 'var r$,s$,m$=this,f$=m$.fn,i$=m$.count,n$=t$.ns;#{setup}\n#{begin};' +
            'while(i$--){#{fn}\n}#{end};#{teardown}\nreturn{elapsed:r$,uid:"#{uid}"}'),
        source
      ));

      try {
        if (isEmpty) {
          // Firefox may remove dead code from Function#toString results
          // http://bugzil.la/536085
          throw new Error('The test "' + name + '" is empty. This may be the result of dead code removal.');
        }
        else if (!deferred) {
          // pretest to determine if compiled code is exits early, usually by a
          // rogue `return` statement, by checking for a return object with the uid
          bench.count = 1;
          compiled = (compiled.call(bench, timer) || {}).uid == uid && compiled;
          bench.count = count;
        }
      } catch(e) {
        compiled = null;
        clone.error = e || new Error(String(e));
        bench.count = count;
      }
      // fallback when a test exits early or errors during pretest
      if (decompilable && !compiled && !deferred && !isEmpty) {
        compiled = createFunction(preprocess('t$'), interpolate(
          preprocess(
            (clone.error && !stringable
              ? 'var r$,s$,m$=this,f$=m$.fn,i$=m$.count'
              : 'function f$(){#{fn}\n}var r$,s$,m$=this,i$=m$.count'
            ) +
            ',n$=t$.ns;#{setup}\n#{begin};m$.f$=f$;while(i$--){m$.f$()}#{end};' +
            'delete m$.f$;#{teardown}\nreturn{elapsed:r$}'
          ),
          source
        ));

        try {
          // pretest one more time to check for errors
          bench.count = 1;
          compiled.call(bench, timer);
          bench.compiled = compiled;
          bench.count = count;
          delete clone.error;
        }
        catch(e) {
          bench.count = count;
          if (clone.error) {
            compiled = null;
          } else {
            bench.compiled = compiled;
            clone.error = e || new Error(String(e));
          }
        }
      }
      // assign `compiled` to `clone` before calling in case a deferred benchmark
      // immediately calls `deferred.resolve()`
      clone.compiled = compiled;
      // if no errors run the full test loop
      if (!clone.error) {
        result = compiled.call(deferred || bench, timer).elapsed;
      }
      return result;
    };

    /*------------------------------------------------------------------------*/

    /**
     * Gets the current timer's minimum resolution (secs).
     */
    function getRes(unit) {
      var measured,
          begin,
          count = 30,
          divisor = 1e3,
          ns = timer.ns,
          sample = [];

      // get average smallest measurable time
      while (count--) {
        if (unit == 'us') {
          divisor = 1e6;
          if (ns.stop) {
            ns.start();
            while (!(measured = ns.microseconds())) { }
          } else if (ns[perfName]) {
            divisor = 1e3;
            measured = Function('n', 'var r,s=n.' + perfName + '();while(!(r=n.' + perfName + '()-s)){};return r')(ns);
          } else {
            begin = ns();
            while (!(measured = ns() - begin)) { }
          }
        }
        else if (unit == 'ns') {
          divisor = 1e9;
          if (ns.nanoTime) {
            begin = ns.nanoTime();
            while (!(measured = ns.nanoTime() - begin)) { }
          } else {
            begin = (begin = ns())[0] + (begin[1] / divisor);
            while (!(measured = ((measured = ns())[0] + (measured[1] / divisor)) - begin)) { }
            divisor = 1;
          }
        }
        else {
          begin = new ns;
          while (!(measured = new ns - begin)) { }
        }
        // check for broken timers (nanoTime may have issues)
        // http://alivebutsleepy.srnet.cz/unreliable-system-nanotime/
        if (measured > 0) {
          sample.push(measured);
        } else {
          sample.push(Infinity);
          break;
        }
      }
      // convert to seconds
      return getMean(sample) / divisor;
    }

    /**
     * Replaces all occurrences of `$` with a unique number and
     * template tokens with content.
     */
    function preprocess(code) {
      return interpolate(code, template).replace(/\$/g, /\d+/.exec(uid));
    }

    /*------------------------------------------------------------------------*/

    // detect nanosecond support from a Java applet
    each(doc && doc.applets || [], function(element) {
      return !(timer.ns = applet = 'nanoTime' in element && element);
    });

    // check type in case Safari returns an object instead of a number
    try {
      if (typeof timer.ns.nanoTime() == 'number') {
        timers.push({ 'ns': timer.ns, 'res': getRes('ns'), 'unit': 'ns' });
      }
    } catch(e) { }

    // detect Chrome's microsecond timer:
    // enable benchmarking via the --enable-benchmarking command
    // line switch in at least Chrome 7 to use chrome.Interval
    try {
      if ((timer.ns = new (window.chrome || window.chromium).Interval)) {
        timers.push({ 'ns': timer.ns, 'res': getRes('us'), 'unit': 'us' });
      }
    } catch(e) { }

    // detect `performance.now` microsecond resolution timer
    if ((timer.ns = perfName && perfObject)) {
      timers.push({ 'ns': timer.ns, 'res': getRes('us'), 'unit': 'us' });
    }

    // detect Node's nanosecond resolution timer available in Node >= 0.8
    if (processObject && typeof (timer.ns = processObject.hrtime) == 'function') {
      timers.push({ 'ns': timer.ns, 'res': getRes('ns'), 'unit': 'ns' });
    }

    // detect Wade Simmons' Node microtime module
    if (microtimeObject && typeof (timer.ns = microtimeObject.now) == 'function') {
      timers.push({ 'ns': timer.ns,  'res': getRes('us'), 'unit': 'us' });
    }

    // pick timer with highest resolution
    timer = reduce(timers, function(timer, other) {
      return other.res < timer.res ? other : timer;
    });

    // remove unused applet
    if (timer.unit != 'ns' && applet) {
      applet = destroyElement(applet);
    }
    // error if there are no working timers
    if (timer.res == Infinity) {
      throw new Error('Benchmark.js was unable to find a working timer.');
    }
    // use API of chosen timer
    if (timer.unit == 'ns') {
      if (timer.ns.nanoTime) {
        extend(template, {
          'begin': 's$=n$.nanoTime()',
          'end': 'r$=(n$.nanoTime()-s$)/1e9'
        });
      } else {
        extend(template, {
          'begin': 's$=n$()',
          'end': 'r$=n$(s$);r$=r$[0]+(r$[1]/1e9)'
        });
      }
    }
    else if (timer.unit == 'us') {
      if (timer.ns.stop) {
        extend(template, {
          'begin': 's$=n$.start()',
          'end': 'r$=n$.microseconds()/1e6'
        });
      } else if (perfName) {
        extend(template, {
          'begin': 's$=n$.' + perfName + '()',
          'end': 'r$=(n$.' + perfName + '()-s$)/1e3'
        });
      } else {
        extend(template, {
          'begin': 's$=n$()',
          'end': 'r$=(n$()-s$)/1e6'
        });
      }
    }

    // define `timer` methods
    timer.start = createFunction(preprocess('o$'),
      preprocess('var n$=this.ns,#{begin};o$.elapsed=0;o$.timeStamp=s$'));

    timer.stop = createFunction(preprocess('o$'),
      preprocess('var n$=this.ns,s$=o$.timeStamp,#{end};o$.elapsed=r$'));

    // resolve time span required to achieve a percent uncertainty of at most 1%
    // http://spiff.rit.edu/classes/phys273/uncert/uncert.html
    options.minTime || (options.minTime = max(timer.res / 2 / 0.01, 0.05));
    return clock.apply(null, arguments);
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Computes stats on benchmark results.
   *
   * @private
   * @param {Object} bench The benchmark instance.
   * @param {Object} options The options object.
   */
  function compute(bench, options) {
    options || (options = {});

    var async = options.async,
        elapsed = 0,
        initCount = bench.initCount,
        minSamples = bench.minSamples,
        queue = [],
        sample = bench.stats.sample;

    /**
     * Adds a clone to the queue.
     */
    function enqueue() {
      queue.push(bench.clone({
        '_original': bench,
        'events': {
          'abort': [update],
          'cycle': [update],
          'error': [update],
          'start': [update]
        }
      }));
    }

    /**
     * Updates the clone/original benchmarks to keep their data in sync.
     */
    function update(event) {
      var clone = this,
          type = event.type;

      if (bench.running) {
        if (type == 'start') {
          // Note: `clone.minTime` prop is inited in `clock()`
          clone.count = bench.initCount;
        }
        else {
          if (type == 'error') {
            bench.error = clone.error;
          }
          if (type == 'abort') {
            bench.abort();
            bench.emit('cycle');
          } else {
            event.currentTarget = event.target = bench;
            bench.emit(event);
          }
        }
      } else if (bench.aborted) {
        // clear abort listeners to avoid triggering bench's abort/cycle again
        clone.events.abort.length = 0;
        clone.abort();
      }
    }

    /**
     * Determines if more clones should be queued or if cycling should stop.
     */
    function evaluate(event) {
      var critical,
          df,
          mean,
          moe,
          rme,
          sd,
          sem,
          variance,
          clone = event.target,
          done = bench.aborted,
          now = +new Date,
          size = sample.push(clone.times.period),
          maxedOut = size >= minSamples && (elapsed += now - clone.times.timeStamp) / 1e3 > bench.maxTime,
          times = bench.times,
          varOf = function(sum, x) { return sum + pow(x - mean, 2); };

      // exit early for aborted or unclockable tests
      if (done || clone.hz == Infinity) {
        maxedOut = !(size = sample.length = queue.length = 0);
      }

      if (!done) {
        // sample mean (estimate of the population mean)
        mean = getMean(sample);
        // sample variance (estimate of the population variance)
        variance = reduce(sample, varOf, 0) / (size - 1) || 0;
        // sample standard deviation (estimate of the population standard deviation)
        sd = sqrt(variance);
        // standard error of the mean (a.k.a. the standard deviation of the sampling distribution of the sample mean)
        sem = sd / sqrt(size);
        // degrees of freedom
        df = size - 1;
        // critical value
        critical = tTable[Math.round(df) || 1] || tTable.infinity;
        // margin of error
        moe = sem * critical;
        // relative margin of error
        rme = (moe / mean) * 100 || 0;

        extend(bench.stats, {
          'deviation': sd,
          'mean': mean,
          'moe': moe,
          'rme': rme,
          'sem': sem,
          'variance': variance
        });

        // Abort the cycle loop when the minimum sample size has been collected
        // and the elapsed time exceeds the maximum time allowed per benchmark.
        // We don't count cycle delays toward the max time because delays may be
        // increased by browsers that clamp timeouts for inactive tabs.
        // https://developer.mozilla.org/en/window.setTimeout#Inactive_tabs
        if (maxedOut) {
          // reset the `initCount` in case the benchmark is rerun
          bench.initCount = initCount;
          bench.running = false;
          done = true;
          times.elapsed = (now - times.timeStamp) / 1e3;
        }
        if (bench.hz != Infinity) {
          bench.hz = 1 / mean;
          times.cycle = mean * bench.count;
          times.period = mean;
        }
      }
      // if time permits, increase sample size to reduce the margin of error
      if (queue.length < 2 && !maxedOut) {
        enqueue();
      }
      // abort the invoke cycle when done
      event.aborted = done;
    }

    // init queue and begin
    enqueue();
    invoke(queue, {
      'name': 'run',
      'args': { 'async': async },
      'queued': true,
      'onCycle': evaluate,
      'onComplete': function() { bench.emit('complete'); }
    });
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Cycles a benchmark until a run `count` can be established.
   *
   * @private
   * @param {Object} clone The cloned benchmark instance.
   * @param {Object} options The options object.
   */
  function cycle(clone, options) {
    options || (options = {});

    var deferred;
    if (clone instanceof Deferred) {
      deferred = clone;
      clone = clone.benchmark;
    }

    var clocked,
        cycles,
        divisor,
        event,
        minTime,
        period,
        async = options.async,
        bench = clone._original,
        count = clone.count,
        times = clone.times;

    // continue, if not aborted between cycles
    if (clone.running) {
      // `minTime` is set to `Benchmark.options.minTime` in `clock()`
      cycles = ++clone.cycles;
      clocked = deferred ? deferred.elapsed : clock(clone);
      minTime = clone.minTime;

      if (cycles > bench.cycles) {
        bench.cycles = cycles;
      }
      if (clone.error) {
        event = Event('error');
        event.message = clone.error;
        clone.emit(event);
        if (!event.cancelled) {
          clone.abort();
        }
      }
    }

    // continue, if not errored
    if (clone.running) {
      // time taken to complete last test cycle
      bench.times.cycle = times.cycle = clocked;
      // seconds per operation
      period = bench.times.period = times.period = clocked / count;
      // ops per second
      bench.hz = clone.hz = 1 / period;
      // avoid working our way up to this next time
      bench.initCount = clone.initCount = count;
      // do we need to do another cycle?
      clone.running = clocked < minTime;

      if (clone.running) {
        // tests may clock at `0` when `initCount` is a small number,
        // to avoid that we set its count to something a bit higher
        if (!clocked && (divisor = divisors[clone.cycles]) != null) {
          count = floor(4e6 / divisor);
        }
        // calculate how many more iterations it will take to achive the `minTime`
        if (count <= clone.count) {
          count += Math.ceil((minTime - clocked) / period);
        }
        clone.running = count != Infinity;
      }
    }
    // should we exit early?
    event = Event('cycle');
    clone.emit(event);
    if (event.aborted) {
      clone.abort();
    }
    // figure out what to do next
    if (clone.running) {
      // start a new cycle
      clone.count = count;
      if (deferred) {
        clone.compiled.call(deferred, timer);
      } else if (async) {
        delay(clone, function() { cycle(clone, options); });
      } else {
        cycle(clone);
      }
    }
    else {
      // fix TraceMonkey bug associated with clock fallbacks
      // http://bugzil.la/509069
      if (support.browser) {
        runScript(uid + '=1;delete ' + uid);
      }
      // done
      clone.emit('complete');
    }
  }

  /*--------------------------------------------------------------------------*/

  /**
   * Runs the benchmark.
   *
   * @memberOf Benchmark
   * @param {Object} [options={}] Options object.
   * @returns {Object} The benchmark instance.
   * @example
   *
   * // basic usage
   * bench.run();
   *
   * // or with options
   * bench.run({ 'async': true });
   */
  function run(options) {
    var me = this,
        event = Event('start');

    // set `running` to `false` so `reset()` won't call `abort()`
    me.running = false;
    me.reset();
    me.running = true;

    me.count = me.initCount;
    me.times.timeStamp = +new Date;
    me.emit(event);

    if (!event.cancelled) {
      options = { 'async': ((options = options && options.async) == null ? me.async : options) && support.timeout };

      // for clones created within `compute()`
      if (me._original) {
        if (me.defer) {
          Deferred(me);
        } else {
          cycle(me, options);
        }
      }
      // for original benchmarks
      else {
        compute(me, options);
      }
    }
    return me;
  }

  /*--------------------------------------------------------------------------*/

  // Firefox 1 erroneously defines variable and argument names of functions on
  // the function itself as non-configurable properties with `undefined` values.
  // The bugginess continues as the `Benchmark` constructor has an argument
  // named `options` and Firefox 1 will not assign a value to `Benchmark.options`,
  // making it non-writable in the process, unless it is the first property
  // assigned by for-in loop of `extend()`.
  extend(Benchmark, {

    /**
     * The default options copied by benchmark instances.
     *
     * @static
     * @memberOf Benchmark
     * @type Object
     */
    'options': {

      /**
       * A flag to indicate that benchmark cycles will execute asynchronously
       * by default.
       *
       * @memberOf Benchmark.options
       * @type Boolean
       */
      'async': false,

      /**
       * A flag to indicate that the benchmark clock is deferred.
       *
       * @memberOf Benchmark.options
       * @type Boolean
       */
      'defer': false,

      /**
       * The delay between test cycles (secs).
       * @memberOf Benchmark.options
       * @type Number
       */
      'delay': 0.005,

      /**
       * Displayed by Benchmark#toString when a `name` is not available
       * (auto-generated if absent).
       *
       * @memberOf Benchmark.options
       * @type String
       */
      'id': undefined,

      /**
       * The default number of times to execute a test on a benchmark's first cycle.
       *
       * @memberOf Benchmark.options
       * @type Number
       */
      'initCount': 1,

      /**
       * The maximum time a benchmark is allowed to run before finishing (secs).
       * Note: Cycle delays aren't counted toward the maximum time.
       *
       * @memberOf Benchmark.options
       * @type Number
       */
      'maxTime': 5,

      /**
       * The minimum sample size required to perform statistical analysis.
       *
       * @memberOf Benchmark.options
       * @type Number
       */
      'minSamples': 5,

      /**
       * The time needed to reduce the percent uncertainty of measurement to 1% (secs).
       *
       * @memberOf Benchmark.options
       * @type Number
       */
      'minTime': 0,

      /**
       * The name of the benchmark.
       *
       * @memberOf Benchmark.options
       * @type String
       */
      'name': undefined,

      /**
       * An event listener called when the benchmark is aborted.
       *
       * @memberOf Benchmark.options
       * @type Function
       */
      'onAbort': undefined,

      /**
       * An event listener called when the benchmark completes running.
       *
       * @memberOf Benchmark.options
       * @type Function
       */
      'onComplete': undefined,

      /**
       * An event listener called after each run cycle.
       *
       * @memberOf Benchmark.options
       * @type Function
       */
      'onCycle': undefined,

      /**
       * An event listener called when a test errors.
       *
       * @memberOf Benchmark.options
       * @type Function
       */
      'onError': undefined,

      /**
       * An event listener called when the benchmark is reset.
       *
       * @memberOf Benchmark.options
       * @type Function
       */
      'onReset': undefined,

      /**
       * An event listener called when the benchmark starts running.
       *
       * @memberOf Benchmark.options
       * @type Function
       */
      'onStart': undefined
    },

    /**
     * Platform object with properties describing things like browser name,
     * version, and operating system.
     *
     * @static
     * @memberOf Benchmark
     * @type Object
     */
    'platform': req('platform') || window.platform || {

      /**
       * The platform description.
       *
       * @memberOf Benchmark.platform
       * @type String
       */
      'description': window.navigator && navigator.userAgent || null,

      /**
       * The name of the browser layout engine.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'layout': null,

      /**
       * The name of the product hosting the browser.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'product': null,

      /**
       * The name of the browser/environment.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'name': null,

      /**
       * The name of the product's manufacturer.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'manufacturer': null,

      /**
       * The name of the operating system.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'os': null,

      /**
       * The alpha/beta release indicator.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'prerelease': null,

      /**
       * The browser/environment version.
       *
       * @memberOf Benchmark.platform
       * @type String|Null
       */
      'version': null,

      /**
       * Return platform description when the platform object is coerced to a string.
       *
       * @memberOf Benchmark.platform
       * @type Function
       * @returns {String} The platform description.
       */
      'toString': function() {
        return this.description || '';
      }
    },

    /**
     * The semantic version number.
     *
     * @static
     * @memberOf Benchmark
     * @type String
     */
    'version': '1.0.0',

    // an object of environment/feature detection flags
    'support': support,

    // clone objects
    'deepClone': deepClone,

    // iteration utility
    'each': each,

    // augment objects
    'extend': extend,

    // generic Array#filter
    'filter': filter,

    // generic Array#forEach
    'forEach': forEach,

    // generic own property iteration utility
    'forOwn': forOwn,

    // converts a number to a comma-separated string
    'formatNumber': formatNumber,

    // generic Object#hasOwnProperty
    // (trigger hasKey's lazy define before assigning it to Benchmark)
    'hasKey': (hasKey(Benchmark, ''), hasKey),

    // generic Array#indexOf
    'indexOf': indexOf,

    // template utility
    'interpolate': interpolate,

    // invokes a method on each item in an array
    'invoke': invoke,

    // generic Array#join for arrays and objects
    'join': join,

    // generic Array#map
    'map': map,

    // retrieves a property value from each item in an array
    'pluck': pluck,

    // generic Array#reduce
    'reduce': reduce
  });

  /*--------------------------------------------------------------------------*/

  extend(Benchmark.prototype, {

    /**
     * The number of times a test was executed.
     *
     * @memberOf Benchmark
     * @type Number
     */
    'count': 0,

    /**
     * The number of cycles performed while benchmarking.
     *
     * @memberOf Benchmark
     * @type Number
     */
    'cycles': 0,

    /**
     * The number of executions per second.
     *
     * @memberOf Benchmark
     * @type Number
     */
    'hz': 0,

    /**
     * The compiled test function.
     *
     * @memberOf Benchmark
     * @type Function|String
     */
    'compiled': undefined,

    /**
     * The error object if the test failed.
     *
     * @memberOf Benchmark
     * @type Object
     */
    'error': undefined,

    /**
     * The test to benchmark.
     *
     * @memberOf Benchmark
     * @type Function|String
     */
    'fn': undefined,

    /**
     * A flag to indicate if the benchmark is aborted.
     *
     * @memberOf Benchmark
     * @type Boolean
     */
    'aborted': false,

    /**
     * A flag to indicate if the benchmark is running.
     *
     * @memberOf Benchmark
     * @type Boolean
     */
    'running': false,

    /**
     * Compiled into the test and executed immediately **before** the test loop.
     *
     * @memberOf Benchmark
     * @type Function|String
     * @example
     *
     * // basic usage
     * var bench = Benchmark({
     *   'setup': function() {
     *     var c = this.count,
     *         element = document.getElementById('container');
     *     while (c--) {
     *       element.appendChild(document.createElement('div'));
     *     }
     *   },
     *   'fn': function() {
     *     element.removeChild(element.lastChild);
     *   }
     * });
     *
     * // compiles to something like:
     * var c = this.count,
     *     element = document.getElementById('container');
     * while (c--) {
     *   element.appendChild(document.createElement('div'));
     * }
     * var start = new Date;
     * while (count--) {
     *   element.removeChild(element.lastChild);
     * }
     * var end = new Date - start;
     *
     * // or using strings
     * var bench = Benchmark({
     *   'setup': '\
     *     var a = 0;\n\
     *     (function() {\n\
     *       (function() {\n\
     *         (function() {',
     *   'fn': 'a += 1;',
     *   'teardown': '\
     *          }())\n\
     *        }())\n\
     *      }())'
     * });
     *
     * // compiles to something like:
     * var a = 0;
     * (function() {
     *   (function() {
     *     (function() {
     *       var start = new Date;
     *       while (count--) {
     *         a += 1;
     *       }
     *       var end = new Date - start;
     *     }())
     *   }())
     * }())
     */
    'setup': noop,

    /**
     * Compiled into the test and executed immediately **after** the test loop.
     *
     * @memberOf Benchmark
     * @type Function|String
     */
    'teardown': noop,

    /**
     * An object of stats including mean, margin or error, and standard deviation.
     *
     * @memberOf Benchmark
     * @type Object
     */
    'stats': {

      /**
       * The margin of error.
       *
       * @memberOf Benchmark#stats
       * @type Number
       */
      'moe': 0,

      /**
       * The relative margin of error (expressed as a percentage of the mean).
       *
       * @memberOf Benchmark#stats
       * @type Number
       */
      'rme': 0,

      /**
       * The standard error of the mean.
       *
       * @memberOf Benchmark#stats
       * @type Number
       */
      'sem': 0,

      /**
       * The sample standard deviation.
       *
       * @memberOf Benchmark#stats
       * @type Number
       */
      'deviation': 0,

      /**
       * The sample arithmetic mean.
       *
       * @memberOf Benchmark#stats
       * @type Number
       */
      'mean': 0,

      /**
       * The array of sampled periods.
       *
       * @memberOf Benchmark#stats
       * @type Array
       */
      'sample': [],

      /**
       * The sample variance.
       *
       * @memberOf Benchmark#stats
       * @type Number
       */
      'variance': 0
    },

    /**
     * An object of timing data including cycle, elapsed, period, start, and stop.
     *
     * @memberOf Benchmark
     * @type Object
     */
    'times': {

      /**
       * The time taken to complete the last cycle (secs).
       *
       * @memberOf Benchmark#times
       * @type Number
       */
      'cycle': 0,

      /**
       * The time taken to complete the benchmark (secs).
       *
       * @memberOf Benchmark#times
       * @type Number
       */
      'elapsed': 0,

      /**
       * The time taken to execute the test once (secs).
       *
       * @memberOf Benchmark#times
       * @type Number
       */
      'period': 0,

      /**
       * A timestamp of when the benchmark started (ms).
       *
       * @memberOf Benchmark#times
       * @type Number
       */
      'timeStamp': 0
    },

    // aborts benchmark (does not record times)
    'abort': abort,

    // creates a new benchmark using the same test and options
    'clone': clone,

    // compares benchmark's hertz with another
    'compare': compare,

    // executes listeners
    'emit': emit,

    // get listeners
    'listeners': listeners,

    // unregister listeners
    'off': off,

    // register listeners
    'on': on,

    // reset benchmark properties
    'reset': reset,

    // runs the benchmark
    'run': run,

    // pretty print benchmark info
    'toString': toStringBench
  });

  /*--------------------------------------------------------------------------*/

  extend(Deferred.prototype, {

    /**
     * The deferred benchmark instance.
     *
     * @memberOf Benchmark.Deferred
     * @type Object
     */
    'benchmark': null,

    /**
     * The number of deferred cycles performed while benchmarking.
     *
     * @memberOf Benchmark.Deferred
     * @type Number
     */
    'cycles': 0,

    /**
     * The time taken to complete the deferred benchmark (secs).
     *
     * @memberOf Benchmark.Deferred
     * @type Number
     */
    'elapsed': 0,

    /**
     * A timestamp of when the deferred benchmark started (ms).
     *
     * @memberOf Benchmark.Deferred
     * @type Number
     */
    'timeStamp': 0,

    // cycles/completes the deferred benchmark
    'resolve': resolve
  });

  /*--------------------------------------------------------------------------*/

  extend(Event.prototype, {

    /**
     * A flag to indicate if the emitters listener iteration is aborted.
     *
     * @memberOf Benchmark.Event
     * @type Boolean
     */
    'aborted': false,

    /**
     * A flag to indicate if the default action is cancelled.
     *
     * @memberOf Benchmark.Event
     * @type Boolean
     */
    'cancelled': false,

    /**
     * The object whose listeners are currently being processed.
     *
     * @memberOf Benchmark.Event
     * @type Object
     */
    'currentTarget': undefined,

    /**
     * The return value of the last executed listener.
     *
     * @memberOf Benchmark.Event
     * @type Mixed
     */
    'result': undefined,

    /**
     * The object to which the event was originally emitted.
     *
     * @memberOf Benchmark.Event
     * @type Object
     */
    'target': undefined,

    /**
     * A timestamp of when the event was created (ms).
     *
     * @memberOf Benchmark.Event
     * @type Number
     */
    'timeStamp': 0,

    /**
     * The event type.
     *
     * @memberOf Benchmark.Event
     * @type String
     */
    'type': ''
  });

  /*--------------------------------------------------------------------------*/

  /**
   * The default options copied by suite instances.
   *
   * @static
   * @memberOf Benchmark.Suite
   * @type Object
   */
  Suite.options = {

    /**
     * The name of the suite.
     *
     * @memberOf Benchmark.Suite.options
     * @type String
     */
    'name': undefined
  };

  /*--------------------------------------------------------------------------*/

  extend(Suite.prototype, {

    /**
     * The number of benchmarks in the suite.
     *
     * @memberOf Benchmark.Suite
     * @type Number
     */
    'length': 0,

    /**
     * A flag to indicate if the suite is aborted.
     *
     * @memberOf Benchmark.Suite
     * @type Boolean
     */
    'aborted': false,

    /**
     * A flag to indicate if the suite is running.
     *
     * @memberOf Benchmark.Suite
     * @type Boolean
     */
    'running': false,

    /**
     * An `Array#forEach` like method.
     * Callbacks may terminate the loop by explicitly returning `false`.
     *
     * @memberOf Benchmark.Suite
     * @param {Function} callback The function called per iteration.
     * @returns {Object} The suite iterated over.
     */
    'forEach': methodize(forEach),

    /**
     * An `Array#indexOf` like method.
     *
     * @memberOf Benchmark.Suite
     * @param {Mixed} value The value to search for.
     * @returns {Number} The index of the matched value or `-1`.
     */
    'indexOf': methodize(indexOf),

    /**
     * Invokes a method on all benchmarks in the suite.
     *
     * @memberOf Benchmark.Suite
     * @param {String|Object} name The name of the method to invoke OR options object.
     * @param {Mixed} [arg1, arg2, ...] Arguments to invoke the method with.
     * @returns {Array} A new array of values returned from each method invoked.
     */
    'invoke': methodize(invoke),

    /**
     * Converts the suite of benchmarks to a string.
     *
     * @memberOf Benchmark.Suite
     * @param {String} [separator=','] A string to separate each element of the array.
     * @returns {String} The string.
     */
    'join': [].join,

    /**
     * An `Array#map` like method.
     *
     * @memberOf Benchmark.Suite
     * @param {Function} callback The function called per iteration.
     * @returns {Array} A new array of values returned by the callback.
     */
    'map': methodize(map),

    /**
     * Retrieves the value of a specified property from all benchmarks in the suite.
     *
     * @memberOf Benchmark.Suite
     * @param {String} property The property to pluck.
     * @returns {Array} A new array of property values.
     */
    'pluck': methodize(pluck),

    /**
     * Removes the last benchmark from the suite and returns it.
     *
     * @memberOf Benchmark.Suite
     * @returns {Mixed} The removed benchmark.
     */
    'pop': [].pop,

    /**
     * Appends benchmarks to the suite.
     *
     * @memberOf Benchmark.Suite
     * @returns {Number} The suite's new length.
     */
    'push': [].push,

    /**
     * Sorts the benchmarks of the suite.
     *
     * @memberOf Benchmark.Suite
     * @param {Function} [compareFn=null] A function that defines the sort order.
     * @returns {Object} The sorted suite.
     */
    'sort': [].sort,

    /**
     * An `Array#reduce` like method.
     *
     * @memberOf Benchmark.Suite
     * @param {Function} callback The function called per iteration.
     * @param {Mixed} accumulator Initial value of the accumulator.
     * @returns {Mixed} The accumulator.
     */
    'reduce': methodize(reduce),

    // aborts all benchmarks in the suite
    'abort': abortSuite,

    // adds a benchmark to the suite
    'add': add,

    // creates a new suite with cloned benchmarks
    'clone': cloneSuite,

    // executes listeners of a specified type
    'emit': emit,

    // creates a new suite of filtered benchmarks
    'filter': filterSuite,

    // get listeners
    'listeners': listeners,

    // unregister listeners
    'off': off,

   // register listeners
    'on': on,

    // resets all benchmarks in the suite
    'reset': resetSuite,

    // runs all benchmarks in the suite
    'run': runSuite,

    // array methods
    'concat': concat,

    'reverse': reverse,

    'shift': shift,

    'slice': slice,

    'splice': splice,

    'unshift': unshift
  });

  /*--------------------------------------------------------------------------*/

  // expose Deferred, Event and Suite
  extend(Benchmark, {
    'Deferred': Deferred,
    'Event': Event,
    'Suite': Suite
  });

  // expose Benchmark
  // some AMD build optimizers, like r.js, check for specific condition patterns like the following:
  if (typeof define == 'function' && typeof define.amd == 'object' && define.amd) {
    // define as an anonymous module so, through path mapping, it can be aliased
    define(function() {
      return Benchmark;
    });
  }
  // check for `exports` after `define` in case a build optimizer adds an `exports` object
  else if (freeExports) {
    // in Node.js or RingoJS v0.8.0+
    if (typeof module == 'object' && module && module.exports == freeExports) {
      (module.exports = Benchmark).Benchmark = Benchmark;
    }
    // in Narwhal or RingoJS v0.7.0-
    else {
      freeExports.Benchmark = Benchmark;
    }
  }
  // in a browser or Rhino
  else {
    // use square bracket notation so Closure Compiler won't munge `Benchmark`
    // http://code.google.com/closure/compiler/docs/api-tutorial3.html#export
    window['Benchmark'] = Benchmark;
  }

  // trigger clock's lazy define early to avoid a security error
  if (support.air) {
    clock({ '_original': { 'fn': noop, 'count': 1, 'options': {} } });
  }
}(this));

},{"__browserify_process":4}],4:[function(require,module,exports){
// shim for using process in browser

var process = module.exports = {};

process.nextTick = (function () {
    var canSetImmediate = typeof window !== 'undefined'
    && window.setImmediate;
    var canPost = typeof window !== 'undefined'
    && window.postMessage && window.addEventListener
    ;

    if (canSetImmediate) {
        return function (f) { return window.setImmediate(f) };
    }

    if (canPost) {
        var queue = [];
        window.addEventListener('message', function (ev) {
            if (ev.source === window && ev.data === 'process-tick') {
                ev.stopPropagation();
                if (queue.length > 0) {
                    var fn = queue.shift();
                    fn();
                }
            }
        }, true);

        return function nextTick(fn) {
            queue.push(fn);
            window.postMessage('process-tick', '*');
        };
    }

    return function nextTick(fn) {
        setTimeout(fn, 0);
    };
})();

process.title = 'browser';
process.browser = true;
process.env = {};
process.argv = [];

process.binding = function (name) {
    throw new Error('process.binding is not supported');
}

// TODO(shtylman)
process.cwd = function () { return '/' };
process.chdir = function (dir) {
    throw new Error('process.chdir is not supported');
};

},{}],5:[function(require,module,exports){
exports.read = function(buffer, offset, isLE, mLen, nBytes) {
  var e, m,
      eLen = nBytes * 8 - mLen - 1,
      eMax = (1 << eLen) - 1,
      eBias = eMax >> 1,
      nBits = -7,
      i = isLE ? (nBytes - 1) : 0,
      d = isLE ? -1 : 1,
      s = buffer[offset + i];

  i += d;

  e = s & ((1 << (-nBits)) - 1);
  s >>= (-nBits);
  nBits += eLen;
  for (; nBits > 0; e = e * 256 + buffer[offset + i], i += d, nBits -= 8);

  m = e & ((1 << (-nBits)) - 1);
  e >>= (-nBits);
  nBits += mLen;
  for (; nBits > 0; m = m * 256 + buffer[offset + i], i += d, nBits -= 8);

  if (e === 0) {
    e = 1 - eBias;
  } else if (e === eMax) {
    return m ? NaN : ((s ? -1 : 1) * Infinity);
  } else {
    m = m + Math.pow(2, mLen);
    e = e - eBias;
  }
  return (s ? -1 : 1) * m * Math.pow(2, e - mLen);
};

exports.write = function(buffer, value, offset, isLE, mLen, nBytes) {
  var e, m, c,
      eLen = nBytes * 8 - mLen - 1,
      eMax = (1 << eLen) - 1,
      eBias = eMax >> 1,
      rt = (mLen === 23 ? Math.pow(2, -24) - Math.pow(2, -77) : 0),
      i = isLE ? 0 : (nBytes - 1),
      d = isLE ? 1 : -1,
      s = value < 0 || (value === 0 && 1 / value < 0) ? 1 : 0;

  value = Math.abs(value);

  if (isNaN(value) || value === Infinity) {
    m = isNaN(value) ? 1 : 0;
    e = eMax;
  } else {
    e = Math.floor(Math.log(value) / Math.LN2);
    if (value * (c = Math.pow(2, -e)) < 1) {
      e--;
      c *= 2;
    }
    if (e + eBias >= 1) {
      value += rt / c;
    } else {
      value += rt * Math.pow(2, 1 - eBias);
    }
    if (value * c >= 2) {
      e++;
      c /= 2;
    }

    if (e + eBias >= eMax) {
      m = 0;
      e = eMax;
    } else if (e + eBias >= 1) {
      m = (value * c - 1) * Math.pow(2, mLen);
      e = e + eBias;
    } else {
      m = value * Math.pow(2, eBias - 1) * Math.pow(2, mLen);
      e = 0;
    }
  }

  for (; mLen >= 8; buffer[offset + i] = m & 0xff, i += d, m /= 256, mLen -= 8);

  e = (e << mLen) | m;
  eLen += mLen;
  for (; eLen > 0; buffer[offset + i] = e & 0xff, i += d, e /= 256, eLen -= 8);

  buffer[offset + i - d] |= s * 128;
};

},{}],6:[function(require,module,exports){
var global=typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {};var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 20

var newTarget = NewBuffer(LENGTH * 4)

for (var i = 0; i < LENGTH; i++) {
  newTarget.writeUInt32LE(7000 + i, i * 4)
}

suite.add('NewBuffer#readUInt32LE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = newTarget.readUInt32LE(i * 4)
  }
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })

},{"../../":1,"benchmark":3}]},{},[6])
//@ sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiZ2VuZXJhdGVkLmpzIiwic291cmNlcyI6WyIvVXNlcnMvZmVyb3NzL2NvZGUvbmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5L2luZGV4LmpzIiwiL1VzZXJzL2Zlcm9zcy9jb2RlL25hdGl2ZS1idWZmZXItYnJvd3NlcmlmeS9ub2RlX21vZHVsZXMvYmFzZTY0LWpzL2xpYi9iNjQuanMiLCIvVXNlcnMvZmVyb3NzL2NvZGUvbmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5L25vZGVfbW9kdWxlcy9iZW5jaG1hcmsvYmVuY2htYXJrLmpzIiwiL1VzZXJzL2Zlcm9zcy9jb2RlL25hdGl2ZS1idWZmZXItYnJvd3NlcmlmeS9ub2RlX21vZHVsZXMvYnJvd3NlcmlmeS9ub2RlX21vZHVsZXMvaW5zZXJ0LW1vZHVsZS1nbG9iYWxzL25vZGVfbW9kdWxlcy9wcm9jZXNzL2Jyb3dzZXIuanMiLCIvVXNlcnMvZmVyb3NzL2NvZGUvbmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5L25vZGVfbW9kdWxlcy9pZWVlNzU0L2luZGV4LmpzIiwiL1VzZXJzL2Zlcm9zcy9jb2RlL25hdGl2ZS1idWZmZXItYnJvd3NlcmlmeS9wZXJmL3NvbG8vcmVhZFVJbnQzMkxFLmpzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiI7QUFBQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ25nQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUN2R0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDOTBIQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ3BEQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNwRkE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBIiwic291cmNlc0NvbnRlbnQiOlsidmFyIGJhc2U2NCA9IHJlcXVpcmUoJ2Jhc2U2NC1qcycpXG52YXIgaWVlZTc1NCA9IHJlcXVpcmUoJ2llZWU3NTQnKVxuXG5leHBvcnRzLkJ1ZmZlciA9IEJ1ZmZlclxuZXhwb3J0cy5TbG93QnVmZmVyID0gQnVmZmVyXG5leHBvcnRzLklOU1BFQ1RfTUFYX0JZVEVTID0gNTBcbkJ1ZmZlci5wb29sU2l6ZSA9IDgxOTJcblxuLyoqXG4gKiBJZiBgYnJvd3NlclN1cHBvcnRgOlxuICogICA9PT0gdHJ1ZSAgICBVc2UgVWludDhBcnJheSBpbXBsZW1lbnRhdGlvbiAoZmFzdGVzdClcbiAqICAgPT09IGZhbHNlICAgVXNlIE9iamVjdCBpbXBsZW1lbnRhdGlvbiAoY29tcGF0aWJsZSBkb3duIHRvIElFNilcbiAqL1xudmFyIGJyb3dzZXJTdXBwb3J0ID0gKGZ1bmN0aW9uICgpIHtcbiAgIC8vIERldGVjdCBpZiBicm93c2VyIHN1cHBvcnRzIFR5cGVkIEFycmF5cy4gU3VwcG9ydGVkIGJyb3dzZXJzIGFyZSBJRSAxMCssXG4gICAvLyBGaXJlZm94IDQrLCBDaHJvbWUgNyssIFNhZmFyaSA1LjErLCBPcGVyYSAxMS42KywgaU9TIDQuMisuXG4gICBpZiAodHlwZW9mIFVpbnQ4QXJyYXkgPT09ICd1bmRlZmluZWQnIHx8IHR5cGVvZiBBcnJheUJ1ZmZlciA9PT0gJ3VuZGVmaW5lZCcpXG4gICAgICByZXR1cm4gZmFsc2VcblxuICAvLyBEb2VzIHRoZSBicm93c2VyIHN1cHBvcnQgYWRkaW5nIHByb3BlcnRpZXMgdG8gYFVpbnQ4QXJyYXlgIGluc3RhbmNlcz8gSWZcbiAgLy8gbm90LCB0aGVuIHRoYXQncyB0aGUgc2FtZSBhcyBubyBgVWludDhBcnJheWAgc3VwcG9ydC4gV2UgbmVlZCB0byBiZSBhYmxlIHRvXG4gIC8vIGFkZCBhbGwgdGhlIG5vZGUgQnVmZmVyIEFQSSBtZXRob2RzLlxuICAvLyBSZWxldmFudCBGaXJlZm94IGJ1ZzogaHR0cHM6Ly9idWd6aWxsYS5tb3ppbGxhLm9yZy9zaG93X2J1Zy5jZ2k/aWQ9Njk1NDM4XG4gIHRyeSB7XG4gICAgdmFyIGFyciA9IG5ldyBVaW50OEFycmF5KDApXG4gICAgYXJyLmZvbyA9IGZ1bmN0aW9uICgpIHsgcmV0dXJuIDQyIH1cbiAgICByZXR1cm4gNDIgPT09IGFyci5mb28oKSAmJlxuICAgICAgICB0eXBlb2YgYXJyLnN1YmFycmF5ID09PSAnZnVuY3Rpb24nIC8vIENocm9tZSA5LTEwIGxhY2sgYHN1YmFycmF5YFxuICB9IGNhdGNoIChlKSB7XG4gICAgcmV0dXJuIGZhbHNlXG4gIH1cbn0pKClcblxuXG4vKipcbiAqIENsYXNzOiBCdWZmZXJcbiAqID09PT09PT09PT09PT1cbiAqXG4gKiBUaGUgQnVmZmVyIGNvbnN0cnVjdG9yIHJldHVybnMgaW5zdGFuY2VzIG9mIGBVaW50OEFycmF5YCB0aGF0IGFyZSBhdWdtZW50ZWRcbiAqIHdpdGggZnVuY3Rpb24gcHJvcGVydGllcyBmb3IgYWxsIHRoZSBub2RlIGBCdWZmZXJgIEFQSSBmdW5jdGlvbnMuIFdlIHVzZVxuICogYFVpbnQ4QXJyYXlgIHNvIHRoYXQgc3F1YXJlIGJyYWNrZXQgbm90YXRpb24gd29ya3MgYXMgZXhwZWN0ZWQgLS0gaXQgcmV0dXJuc1xuICogYSBzaW5nbGUgb2N0ZXQuXG4gKlxuICogQnkgYXVnbWVudGluZyB0aGUgaW5zdGFuY2VzLCB3ZSBjYW4gYXZvaWQgbW9kaWZ5aW5nIHRoZSBgVWludDhBcnJheWBcbiAqIHByb3RvdHlwZS5cbiAqL1xuZnVuY3Rpb24gQnVmZmVyIChzdWJqZWN0LCBlbmNvZGluZywgbm9aZXJvKSB7XG4gIGlmICghKHRoaXMgaW5zdGFuY2VvZiBCdWZmZXIpKVxuICAgIHJldHVybiBuZXcgQnVmZmVyKHN1YmplY3QsIGVuY29kaW5nLCBub1plcm8pXG5cbiAgdmFyIHR5cGUgPSB0eXBlb2Ygc3ViamVjdFxuXG4gIC8vIFdvcmthcm91bmQ6IG5vZGUncyBiYXNlNjQgaW1wbGVtZW50YXRpb24gYWxsb3dzIGZvciBub24tcGFkZGVkIHN0cmluZ3NcbiAgLy8gd2hpbGUgYmFzZTY0LWpzIGRvZXMgbm90LlxuICBpZiAoZW5jb2RpbmcgPT09ICdiYXNlNjQnICYmIHR5cGUgPT09ICdzdHJpbmcnKSB7XG4gICAgc3ViamVjdCA9IHN0cmluZ3RyaW0oc3ViamVjdClcbiAgICB3aGlsZSAoc3ViamVjdC5sZW5ndGggJSA0ICE9PSAwKSB7XG4gICAgICBzdWJqZWN0ID0gc3ViamVjdCArICc9J1xuICAgIH1cbiAgfVxuXG4gIC8vIEZpbmQgdGhlIGxlbmd0aFxuICB2YXIgbGVuZ3RoXG4gIGlmICh0eXBlID09PSAnbnVtYmVyJylcbiAgICBsZW5ndGggPSBjb2VyY2Uoc3ViamVjdClcbiAgZWxzZSBpZiAodHlwZSA9PT0gJ3N0cmluZycpXG4gICAgbGVuZ3RoID0gQnVmZmVyLmJ5dGVMZW5ndGgoc3ViamVjdCwgZW5jb2RpbmcpXG4gIGVsc2UgaWYgKHR5cGUgPT09ICdvYmplY3QnKVxuICAgIGxlbmd0aCA9IGNvZXJjZShzdWJqZWN0Lmxlbmd0aCkgLy8gQXNzdW1lIG9iamVjdCBpcyBhbiBhcnJheVxuICBlbHNlXG4gICAgdGhyb3cgbmV3IEVycm9yKCdGaXJzdCBhcmd1bWVudCBuZWVkcyB0byBiZSBhIG51bWJlciwgYXJyYXkgb3Igc3RyaW5nLicpXG5cbiAgdmFyIGJ1ZlxuICBpZiAoYnJvd3NlclN1cHBvcnQpIHtcbiAgICAvLyBQcmVmZXJyZWQ6IFJldHVybiBhbiBhdWdtZW50ZWQgYFVpbnQ4QXJyYXlgIGluc3RhbmNlIGZvciBiZXN0IHBlcmZvcm1hbmNlXG4gICAgYnVmID0gYXVnbWVudChuZXcgVWludDhBcnJheShsZW5ndGgpKVxuICB9IGVsc2Uge1xuICAgIC8vIEZhbGxiYWNrOiBSZXR1cm4gdGhpcyBpbnN0YW5jZSBvZiBCdWZmZXJcbiAgICBidWYgPSB0aGlzXG4gICAgYnVmLmxlbmd0aCA9IGxlbmd0aFxuICB9XG5cbiAgdmFyIGlcbiAgaWYgKEJ1ZmZlci5pc0J1ZmZlcihzdWJqZWN0KSkge1xuICAgIC8vIFNwZWVkIG9wdGltaXphdGlvbiAtLSB1c2Ugc2V0IGlmIHdlJ3JlIGNvcHlpbmcgZnJvbSBhIFVpbnQ4QXJyYXlcbiAgICBidWYuc2V0KHN1YmplY3QpXG4gIH0gZWxzZSBpZiAoaXNBcnJheWlzaChzdWJqZWN0KSkge1xuICAgIC8vIFRyZWF0IGFycmF5LWlzaCBvYmplY3RzIGFzIGEgYnl0ZSBhcnJheVxuICAgIGZvciAoaSA9IDA7IGkgPCBsZW5ndGg7IGkrKykge1xuICAgICAgaWYgKEJ1ZmZlci5pc0J1ZmZlcihzdWJqZWN0KSlcbiAgICAgICAgYnVmW2ldID0gc3ViamVjdC5yZWFkVUludDgoaSlcbiAgICAgIGVsc2VcbiAgICAgICAgYnVmW2ldID0gc3ViamVjdFtpXVxuICAgIH1cbiAgfSBlbHNlIGlmICh0eXBlID09PSAnc3RyaW5nJykge1xuICAgIGJ1Zi53cml0ZShzdWJqZWN0LCAwLCBlbmNvZGluZylcbiAgfSBlbHNlIGlmICh0eXBlID09PSAnbnVtYmVyJyAmJiAhYnJvd3NlclN1cHBvcnQgJiYgIW5vWmVybykge1xuICAgIGZvciAoaSA9IDA7IGkgPCBsZW5ndGg7IGkrKykge1xuICAgICAgYnVmW2ldID0gMFxuICAgIH1cbiAgfVxuXG4gIHJldHVybiBidWZcbn1cblxuLy8gU1RBVElDIE1FVEhPRFNcbi8vID09PT09PT09PT09PT09XG5cbkJ1ZmZlci5pc0VuY29kaW5nID0gZnVuY3Rpb24gKGVuY29kaW5nKSB7XG4gIHN3aXRjaCAoU3RyaW5nKGVuY29kaW5nKS50b0xvd2VyQ2FzZSgpKSB7XG4gICAgY2FzZSAnaGV4JzpcbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgY2FzZSAnYXNjaWknOlxuICAgIGNhc2UgJ2JpbmFyeSc6XG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICBjYXNlICdyYXcnOlxuICAgICAgcmV0dXJuIHRydWVcblxuICAgIGRlZmF1bHQ6XG4gICAgICByZXR1cm4gZmFsc2VcbiAgfVxufVxuXG5CdWZmZXIuaXNCdWZmZXIgPSBmdW5jdGlvbiAoYikge1xuICByZXR1cm4gYiAmJiBiLl9pc0J1ZmZlclxufVxuXG5CdWZmZXIuYnl0ZUxlbmd0aCA9IGZ1bmN0aW9uIChzdHIsIGVuY29kaW5nKSB7XG4gIHN3aXRjaCAoZW5jb2RpbmcgfHwgJ3V0ZjgnKSB7XG4gICAgY2FzZSAnaGV4JzpcbiAgICAgIHJldHVybiBzdHIubGVuZ3RoIC8gMlxuXG4gICAgY2FzZSAndXRmOCc6XG4gICAgY2FzZSAndXRmLTgnOlxuICAgICAgcmV0dXJuIHV0ZjhUb0J5dGVzKHN0cikubGVuZ3RoXG5cbiAgICBjYXNlICdhc2NpaSc6XG4gICAgY2FzZSAnYmluYXJ5JzpcbiAgICAgIHJldHVybiBzdHIubGVuZ3RoXG5cbiAgICBjYXNlICdiYXNlNjQnOlxuICAgICAgcmV0dXJuIGJhc2U2NFRvQnl0ZXMoc3RyKS5sZW5ndGhcblxuICAgIGRlZmF1bHQ6XG4gICAgICB0aHJvdyBuZXcgRXJyb3IoJ1Vua25vd24gZW5jb2RpbmcnKVxuICB9XG59XG5cbkJ1ZmZlci5jb25jYXQgPSBmdW5jdGlvbiAobGlzdCwgdG90YWxMZW5ndGgpIHtcbiAgaWYgKCFpc0FycmF5KGxpc3QpKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdVc2FnZTogQnVmZmVyLmNvbmNhdChsaXN0LCBbdG90YWxMZW5ndGhdKVxcbicgK1xuICAgICAgICAnbGlzdCBzaG91bGQgYmUgYW4gQXJyYXkuJylcbiAgfVxuXG4gIGlmIChsaXN0Lmxlbmd0aCA9PT0gMCkge1xuICAgIHJldHVybiBuZXcgQnVmZmVyKDApXG4gIH0gZWxzZSBpZiAobGlzdC5sZW5ndGggPT09IDEpIHtcbiAgICByZXR1cm4gbGlzdFswXVxuICB9XG5cbiAgdmFyIGlcbiAgaWYgKHR5cGVvZiB0b3RhbExlbmd0aCAhPT0gJ251bWJlcicpIHtcbiAgICB0b3RhbExlbmd0aCA9IDBcbiAgICBmb3IgKGkgPSAwOyBpIDwgbGlzdC5sZW5ndGg7IGkrKykge1xuICAgICAgdG90YWxMZW5ndGggKz0gbGlzdFtpXS5sZW5ndGhcbiAgICB9XG4gIH1cblxuICB2YXIgYnVmID0gbmV3IEJ1ZmZlcih0b3RhbExlbmd0aClcbiAgdmFyIHBvcyA9IDBcbiAgZm9yIChpID0gMDsgaSA8IGxpc3QubGVuZ3RoOyBpKyspIHtcbiAgICB2YXIgaXRlbSA9IGxpc3RbaV1cbiAgICBpdGVtLmNvcHkoYnVmLCBwb3MpXG4gICAgcG9zICs9IGl0ZW0ubGVuZ3RoXG4gIH1cbiAgcmV0dXJuIGJ1ZlxufVxuXG4vLyBCVUZGRVIgSU5TVEFOQ0UgTUVUSE9EU1xuLy8gPT09PT09PT09PT09PT09PT09PT09PT1cblxuZnVuY3Rpb24gX2hleFdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgb2Zmc2V0ID0gTnVtYmVyKG9mZnNldCkgfHwgMFxuICB2YXIgcmVtYWluaW5nID0gYnVmLmxlbmd0aCAtIG9mZnNldFxuICBpZiAoIWxlbmd0aCkge1xuICAgIGxlbmd0aCA9IHJlbWFpbmluZ1xuICB9IGVsc2Uge1xuICAgIGxlbmd0aCA9IE51bWJlcihsZW5ndGgpXG4gICAgaWYgKGxlbmd0aCA+IHJlbWFpbmluZykge1xuICAgICAgbGVuZ3RoID0gcmVtYWluaW5nXG4gICAgfVxuICB9XG5cbiAgLy8gbXVzdCBiZSBhbiBldmVuIG51bWJlciBvZiBkaWdpdHNcbiAgdmFyIHN0ckxlbiA9IHN0cmluZy5sZW5ndGhcbiAgaWYgKHN0ckxlbiAlIDIgIT09IDApIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ0ludmFsaWQgaGV4IHN0cmluZycpXG4gIH1cbiAgaWYgKGxlbmd0aCA+IHN0ckxlbiAvIDIpIHtcbiAgICBsZW5ndGggPSBzdHJMZW4gLyAyXG4gIH1cbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBsZW5ndGg7IGkrKykge1xuICAgIHZhciBieXRlID0gcGFyc2VJbnQoc3RyaW5nLnN1YnN0cihpICogMiwgMiksIDE2KVxuICAgIGlmIChpc05hTihieXRlKSkgdGhyb3cgbmV3IEVycm9yKCdJbnZhbGlkIGhleCBzdHJpbmcnKVxuICAgIGJ1ZltvZmZzZXQgKyBpXSA9IGJ5dGVcbiAgfVxuICBCdWZmZXIuX2NoYXJzV3JpdHRlbiA9IGkgKiAyXG4gIHJldHVybiBpXG59XG5cbmZ1bmN0aW9uIF91dGY4V3JpdGUgKGJ1Ziwgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCkge1xuICB2YXIgYnl0ZXMsIHBvc1xuICByZXR1cm4gQnVmZmVyLl9jaGFyc1dyaXR0ZW4gPSBibGl0QnVmZmVyKHV0ZjhUb0J5dGVzKHN0cmluZyksIGJ1Ziwgb2Zmc2V0LCBsZW5ndGgpXG59XG5cbmZ1bmN0aW9uIF9hc2NpaVdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIGJ5dGVzLCBwb3NcbiAgcmV0dXJuIEJ1ZmZlci5fY2hhcnNXcml0dGVuID0gYmxpdEJ1ZmZlcihhc2NpaVRvQnl0ZXMoc3RyaW5nKSwgYnVmLCBvZmZzZXQsIGxlbmd0aClcbn1cblxuZnVuY3Rpb24gX2JpbmFyeVdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgcmV0dXJuIF9hc2NpaVdyaXRlKGJ1Ziwgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aClcbn1cblxuZnVuY3Rpb24gX2Jhc2U2NFdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIGJ5dGVzLCBwb3NcbiAgcmV0dXJuIEJ1ZmZlci5fY2hhcnNXcml0dGVuID0gYmxpdEJ1ZmZlcihiYXNlNjRUb0J5dGVzKHN0cmluZyksIGJ1Ziwgb2Zmc2V0LCBsZW5ndGgpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGUgPSBmdW5jdGlvbiAoc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCwgZW5jb2RpbmcpIHtcbiAgLy8gU3VwcG9ydCBib3RoIChzdHJpbmcsIG9mZnNldCwgbGVuZ3RoLCBlbmNvZGluZylcbiAgLy8gYW5kIHRoZSBsZWdhY3kgKHN0cmluZywgZW5jb2RpbmcsIG9mZnNldCwgbGVuZ3RoKVxuICBpZiAoaXNGaW5pdGUob2Zmc2V0KSkge1xuICAgIGlmICghaXNGaW5pdGUobGVuZ3RoKSkge1xuICAgICAgZW5jb2RpbmcgPSBsZW5ndGhcbiAgICAgIGxlbmd0aCA9IHVuZGVmaW5lZFxuICAgIH1cbiAgfSBlbHNlIHsgIC8vIGxlZ2FjeVxuICAgIHZhciBzd2FwID0gZW5jb2RpbmdcbiAgICBlbmNvZGluZyA9IG9mZnNldFxuICAgIG9mZnNldCA9IGxlbmd0aFxuICAgIGxlbmd0aCA9IHN3YXBcbiAgfVxuXG4gIG9mZnNldCA9IE51bWJlcihvZmZzZXQpIHx8IDBcbiAgdmFyIHJlbWFpbmluZyA9IHRoaXMubGVuZ3RoIC0gb2Zmc2V0XG4gIGlmICghbGVuZ3RoKSB7XG4gICAgbGVuZ3RoID0gcmVtYWluaW5nXG4gIH0gZWxzZSB7XG4gICAgbGVuZ3RoID0gTnVtYmVyKGxlbmd0aClcbiAgICBpZiAobGVuZ3RoID4gcmVtYWluaW5nKSB7XG4gICAgICBsZW5ndGggPSByZW1haW5pbmdcbiAgICB9XG4gIH1cbiAgZW5jb2RpbmcgPSBTdHJpbmcoZW5jb2RpbmcgfHwgJ3V0ZjgnKS50b0xvd2VyQ2FzZSgpXG5cbiAgc3dpdGNoIChlbmNvZGluZykge1xuICAgIGNhc2UgJ2hleCc6XG4gICAgICByZXR1cm4gX2hleFdyaXRlKHRoaXMsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG5cbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgICByZXR1cm4gX3V0ZjhXcml0ZSh0aGlzLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKVxuXG4gICAgY2FzZSAnYXNjaWknOlxuICAgICAgcmV0dXJuIF9hc2NpaVdyaXRlKHRoaXMsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG5cbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIF9iaW5hcnlXcml0ZSh0aGlzLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKVxuXG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICAgIHJldHVybiBfYmFzZTY0V3JpdGUodGhpcywgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aClcblxuICAgIGRlZmF1bHQ6XG4gICAgICB0aHJvdyBuZXcgRXJyb3IoJ1Vua25vd24gZW5jb2RpbmcnKVxuICB9XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUudG9TdHJpbmcgPSBmdW5jdGlvbiAoZW5jb2RpbmcsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIHNlbGYgPSB0aGlzXG5cbiAgZW5jb2RpbmcgPSBTdHJpbmcoZW5jb2RpbmcgfHwgJ3V0ZjgnKS50b0xvd2VyQ2FzZSgpXG4gIHN0YXJ0ID0gTnVtYmVyKHN0YXJ0KSB8fCAwXG4gIGVuZCA9IChlbmQgIT09IHVuZGVmaW5lZClcbiAgICA/IE51bWJlcihlbmQpXG4gICAgOiBlbmQgPSBzZWxmLmxlbmd0aFxuXG4gIC8vIEZhc3RwYXRoIGVtcHR5IHN0cmluZ3NcbiAgaWYgKGVuZCA9PT0gc3RhcnQpXG4gICAgcmV0dXJuICcnXG5cbiAgc3dpdGNoIChlbmNvZGluZykge1xuICAgIGNhc2UgJ2hleCc6XG4gICAgICByZXR1cm4gX2hleFNsaWNlKHNlbGYsIHN0YXJ0LCBlbmQpXG5cbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgICByZXR1cm4gX3V0ZjhTbGljZShzZWxmLCBzdGFydCwgZW5kKVxuXG4gICAgY2FzZSAnYXNjaWknOlxuICAgICAgcmV0dXJuIF9hc2NpaVNsaWNlKHNlbGYsIHN0YXJ0LCBlbmQpXG5cbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIF9iaW5hcnlTbGljZShzZWxmLCBzdGFydCwgZW5kKVxuXG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICAgIHJldHVybiBfYmFzZTY0U2xpY2Uoc2VsZiwgc3RhcnQsIGVuZClcblxuICAgIGRlZmF1bHQ6XG4gICAgICB0aHJvdyBuZXcgRXJyb3IoJ1Vua25vd24gZW5jb2RpbmcnKVxuICB9XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUudG9KU09OID0gZnVuY3Rpb24gKCkge1xuICByZXR1cm4ge1xuICAgIHR5cGU6ICdCdWZmZXInLFxuICAgIGRhdGE6IEFycmF5LnByb3RvdHlwZS5zbGljZS5jYWxsKHRoaXMuX2FyciB8fCB0aGlzLCAwKVxuICB9XG59XG5cbi8vIGNvcHkodGFyZ2V0QnVmZmVyLCB0YXJnZXRTdGFydD0wLCBzb3VyY2VTdGFydD0wLCBzb3VyY2VFbmQ9YnVmZmVyLmxlbmd0aClcbkJ1ZmZlci5wcm90b3R5cGUuY29weSA9IGZ1bmN0aW9uICh0YXJnZXQsIHRhcmdldF9zdGFydCwgc3RhcnQsIGVuZCkge1xuICB2YXIgc291cmNlID0gdGhpc1xuXG4gIGlmICghc3RhcnQpIHN0YXJ0ID0gMFxuICBpZiAoIWVuZCAmJiBlbmQgIT09IDApIGVuZCA9IHRoaXMubGVuZ3RoXG4gIGlmICghdGFyZ2V0X3N0YXJ0KSB0YXJnZXRfc3RhcnQgPSAwXG5cbiAgLy8gQ29weSAwIGJ5dGVzOyB3ZSdyZSBkb25lXG4gIGlmIChlbmQgPT09IHN0YXJ0KSByZXR1cm5cbiAgaWYgKHRhcmdldC5sZW5ndGggPT09IDAgfHwgc291cmNlLmxlbmd0aCA9PT0gMCkgcmV0dXJuXG5cbiAgLy8gRmF0YWwgZXJyb3IgY29uZGl0aW9uc1xuICBpZiAoZW5kIDwgc3RhcnQpXG4gICAgdGhyb3cgbmV3IEVycm9yKCdzb3VyY2VFbmQgPCBzb3VyY2VTdGFydCcpXG4gIGlmICh0YXJnZXRfc3RhcnQgPCAwIHx8IHRhcmdldF9zdGFydCA+PSB0YXJnZXQubGVuZ3RoKVxuICAgIHRocm93IG5ldyBFcnJvcigndGFyZ2V0U3RhcnQgb3V0IG9mIGJvdW5kcycpXG4gIGlmIChzdGFydCA8IDAgfHwgc3RhcnQgPj0gc291cmNlLmxlbmd0aClcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3NvdXJjZVN0YXJ0IG91dCBvZiBib3VuZHMnKVxuICBpZiAoZW5kIDwgMCB8fCBlbmQgPiBzb3VyY2UubGVuZ3RoKVxuICAgIHRocm93IG5ldyBFcnJvcignc291cmNlRW5kIG91dCBvZiBib3VuZHMnKVxuXG4gIC8vIEFyZSB3ZSBvb2I/XG4gIGlmIChlbmQgPiB0aGlzLmxlbmd0aClcbiAgICBlbmQgPSB0aGlzLmxlbmd0aFxuICBpZiAodGFyZ2V0Lmxlbmd0aCAtIHRhcmdldF9zdGFydCA8IGVuZCAtIHN0YXJ0KVxuICAgIGVuZCA9IHRhcmdldC5sZW5ndGggLSB0YXJnZXRfc3RhcnQgKyBzdGFydFxuXG4gIC8vIGNvcHkhXG4gIGZvciAodmFyIGkgPSAwOyBpIDwgZW5kIC0gc3RhcnQ7IGkrKylcbiAgICB0YXJnZXRbaSArIHRhcmdldF9zdGFydF0gPSB0aGlzW2kgKyBzdGFydF1cbn1cblxuZnVuY3Rpb24gX2Jhc2U2NFNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgaWYgKHN0YXJ0ID09PSAwICYmIGVuZCA9PT0gYnVmLmxlbmd0aCkge1xuICAgIHJldHVybiBiYXNlNjQuZnJvbUJ5dGVBcnJheShidWYpXG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIGJhc2U2NC5mcm9tQnl0ZUFycmF5KGJ1Zi5zbGljZShzdGFydCwgZW5kKSlcbiAgfVxufVxuXG5mdW5jdGlvbiBfdXRmOFNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIHJlcyA9ICcnXG4gIHZhciB0bXAgPSAnJ1xuICBlbmQgPSBNYXRoLm1pbihidWYubGVuZ3RoLCBlbmQpXG5cbiAgZm9yICh2YXIgaSA9IHN0YXJ0OyBpIDwgZW5kOyBpKyspIHtcbiAgICBpZiAoYnVmW2ldIDw9IDB4N0YpIHtcbiAgICAgIHJlcyArPSBkZWNvZGVVdGY4Q2hhcih0bXApICsgU3RyaW5nLmZyb21DaGFyQ29kZShidWZbaV0pXG4gICAgICB0bXAgPSAnJ1xuICAgIH0gZWxzZSB7XG4gICAgICB0bXAgKz0gJyUnICsgYnVmW2ldLnRvU3RyaW5nKDE2KVxuICAgIH1cbiAgfVxuXG4gIHJldHVybiByZXMgKyBkZWNvZGVVdGY4Q2hhcih0bXApXG59XG5cbmZ1bmN0aW9uIF9hc2NpaVNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIHJldCA9ICcnXG4gIGVuZCA9IE1hdGgubWluKGJ1Zi5sZW5ndGgsIGVuZClcblxuICBmb3IgKHZhciBpID0gc3RhcnQ7IGkgPCBlbmQ7IGkrKylcbiAgICByZXQgKz0gU3RyaW5nLmZyb21DaGFyQ29kZShidWZbaV0pXG4gIHJldHVybiByZXRcbn1cblxuZnVuY3Rpb24gX2JpbmFyeVNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgcmV0dXJuIF9hc2NpaVNsaWNlKGJ1Ziwgc3RhcnQsIGVuZClcbn1cblxuZnVuY3Rpb24gX2hleFNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcblxuICBpZiAoIXN0YXJ0IHx8IHN0YXJ0IDwgMCkgc3RhcnQgPSAwXG4gIGlmICghZW5kIHx8IGVuZCA8IDAgfHwgZW5kID4gbGVuKSBlbmQgPSBsZW5cblxuICB2YXIgb3V0ID0gJydcbiAgZm9yICh2YXIgaSA9IHN0YXJ0OyBpIDwgZW5kOyBpKyspIHtcbiAgICBvdXQgKz0gdG9IZXgoYnVmW2ldKVxuICB9XG4gIHJldHVybiBvdXRcbn1cblxuLy8gVE9ETzogYWRkIHRlc3QgdGhhdCBtb2RpZnlpbmcgdGhlIG5ldyBidWZmZXIgc2xpY2Ugd2lsbCBtb2RpZnkgbWVtb3J5IGluIHRoZVxuLy8gb3JpZ2luYWwgYnVmZmVyISBVc2UgY29kZSBmcm9tOlxuLy8gaHR0cDovL25vZGVqcy5vcmcvYXBpL2J1ZmZlci5odG1sI2J1ZmZlcl9idWZfc2xpY2Vfc3RhcnRfZW5kXG5CdWZmZXIucHJvdG90eXBlLnNsaWNlID0gZnVuY3Rpb24gKHN0YXJ0LCBlbmQpIHtcbiAgdmFyIGxlbiA9IHRoaXMubGVuZ3RoXG4gIHN0YXJ0ID0gY2xhbXAoc3RhcnQsIGxlbiwgMClcbiAgZW5kID0gY2xhbXAoZW5kLCBsZW4sIGxlbilcblxuICBpZiAoYnJvd3NlclN1cHBvcnQpIHtcbiAgICByZXR1cm4gYXVnbWVudCh0aGlzLnN1YmFycmF5KHN0YXJ0LCBlbmQpKVxuICB9IGVsc2Uge1xuICAgIC8vIFRPRE86IHNsaWNpbmcgd29ya3MsIHdpdGggbGltaXRhdGlvbnMgKG5vIHBhcmVudCB0cmFja2luZy91cGRhdGUpXG4gICAgLy8gaHR0cHM6Ly9naXRodWIuY29tL2Zlcm9zcy9uYXRpdmUtYnVmZmVyLWJyb3dzZXJpZnkvaXNzdWVzLzlcbiAgICB2YXIgc2xpY2VMZW4gPSBlbmQgLSBzdGFydFxuICAgIHZhciBuZXdCdWYgPSBuZXcgQnVmZmVyKHNsaWNlTGVuLCB1bmRlZmluZWQsIHRydWUpXG4gICAgZm9yICh2YXIgaSA9IDA7IGkgPCBzbGljZUxlbjsgaSsrKSB7XG4gICAgICBuZXdCdWZbaV0gPSB0aGlzW2kgKyBzdGFydF1cbiAgICB9XG4gICAgcmV0dXJuIG5ld0J1ZlxuICB9XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQ4ID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1Zi5sZW5ndGgpXG4gICAgcmV0dXJuXG5cbiAgcmV0dXJuIGJ1ZltvZmZzZXRdXG59XG5cbmZ1bmN0aW9uIF9yZWFkVUludDE2IChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiBsaXR0bGVFbmRpYW4gPT09ICdib29sZWFuJywgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDEgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pXG4gICAgcmV0dXJuXG5cbiAgdmFyIHZhbFxuICBpZiAobGl0dGxlRW5kaWFuKSB7XG4gICAgdmFsID0gYnVmW29mZnNldF1cbiAgICBpZiAob2Zmc2V0ICsgMSA8IGxlbilcbiAgICAgIHZhbCB8PSBidWZbb2Zmc2V0ICsgMV0gPDwgOFxuICB9IGVsc2Uge1xuICAgIHZhbCA9IGJ1ZltvZmZzZXRdIDw8IDhcbiAgICBpZiAob2Zmc2V0ICsgMSA8IGxlbilcbiAgICAgIHZhbCB8PSBidWZbb2Zmc2V0ICsgMV1cbiAgfVxuICByZXR1cm4gdmFsXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQxNkxFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkVUludDE2KHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQxNkJFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkVUludDE2KHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfcmVhZFVJbnQzMiAoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh0eXBlb2YgbGl0dGxlRW5kaWFuID09PSAnYm9vbGVhbicsICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKVxuICAgIHJldHVyblxuXG4gIHZhciB2YWxcbiAgaWYgKGxpdHRsZUVuZGlhbikge1xuICAgIGlmIChvZmZzZXQgKyAyIDwgbGVuKVxuICAgICAgdmFsID0gYnVmW29mZnNldCArIDJdIDw8IDE2XG4gICAgaWYgKG9mZnNldCArIDEgPCBsZW4pXG4gICAgICB2YWwgfD0gYnVmW29mZnNldCArIDFdIDw8IDhcbiAgICB2YWwgfD0gYnVmW29mZnNldF1cbiAgICBpZiAob2Zmc2V0ICsgMyA8IGxlbilcbiAgICAgIHZhbCA9IHZhbCArIChidWZbb2Zmc2V0ICsgM10gPDwgMjQgPj4+IDApXG4gIH0gZWxzZSB7XG4gICAgaWYgKG9mZnNldCArIDEgPCBsZW4pXG4gICAgICB2YWwgPSBidWZbb2Zmc2V0ICsgMV0gPDwgMTZcbiAgICBpZiAob2Zmc2V0ICsgMiA8IGxlbilcbiAgICAgIHZhbCB8PSBidWZbb2Zmc2V0ICsgMl0gPDwgOFxuICAgIGlmIChvZmZzZXQgKyAzIDwgbGVuKVxuICAgICAgdmFsIHw9IGJ1ZltvZmZzZXQgKyAzXVxuICAgIHZhbCA9IHZhbCArIChidWZbb2Zmc2V0XSA8PCAyNCA+Pj4gMClcbiAgfVxuICByZXR1cm4gdmFsXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQzMkxFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkVUludDMyKHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQzMkJFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkVUludDMyKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLnJlYWRJbnQ4ID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1Zi5sZW5ndGgpXG4gICAgcmV0dXJuXG5cbiAgdmFyIG5lZyA9IGJ1ZltvZmZzZXRdICYgMHg4MFxuICBpZiAobmVnKVxuICAgIHJldHVybiAoMHhmZiAtIGJ1ZltvZmZzZXRdICsgMSkgKiAtMVxuICBlbHNlXG4gICAgcmV0dXJuIGJ1ZltvZmZzZXRdXG59XG5cbmZ1bmN0aW9uIF9yZWFkSW50MTYgKGJ1Ziwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodHlwZW9mIGxpdHRsZUVuZGlhbiA9PT0gJ2Jvb2xlYW4nLCAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMSA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbilcbiAgICByZXR1cm5cblxuICB2YXIgdmFsID0gX3JlYWRVSW50MTYoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgdHJ1ZSlcbiAgdmFyIG5lZyA9IHZhbCAmIDB4ODAwMFxuICBpZiAobmVnKVxuICAgIHJldHVybiAoMHhmZmZmIC0gdmFsICsgMSkgKiAtMVxuICBlbHNlXG4gICAgcmV0dXJuIHZhbFxufVxuXG5CdWZmZXIucHJvdG90eXBlLnJlYWRJbnQxNkxFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkSW50MTYodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkSW50MTZCRSA9IGZ1bmN0aW9uIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEludDE2KHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfcmVhZEludDMyIChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiBsaXR0bGVFbmRpYW4gPT09ICdib29sZWFuJywgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDMgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pXG4gICAgcmV0dXJuXG5cbiAgdmFyIHZhbCA9IF9yZWFkVUludDMyKGJ1Ziwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIHRydWUpXG4gIHZhciBuZWcgPSB2YWwgJiAweDgwMDAwMDAwXG4gIGlmIChuZWcpXG4gICAgcmV0dXJuICgweGZmZmZmZmZmIC0gdmFsICsgMSkgKiAtMVxuICBlbHNlXG4gICAgcmV0dXJuIHZhbFxufVxuXG5CdWZmZXIucHJvdG90eXBlLnJlYWRJbnQzMkxFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkSW50MzIodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkSW50MzJCRSA9IGZ1bmN0aW9uIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEludDMyKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfcmVhZEZsb2F0IChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiBsaXR0bGVFbmRpYW4gPT09ICdib29sZWFuJywgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHJldHVybiBpZWVlNzU0LnJlYWQoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgMjMsIDQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZEZsb2F0TEUgPSBmdW5jdGlvbiAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWRGbG9hdCh0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLnJlYWRGbG9hdEJFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkRmxvYXQodGhpcywgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF9yZWFkRG91YmxlIChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiBsaXR0bGVFbmRpYW4gPT09ICdib29sZWFuJywgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgKyA3IDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHJldHVybiBpZWVlNzU0LnJlYWQoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgNTIsIDgpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZERvdWJsZUxFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkRG91YmxlKHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZERvdWJsZUJFID0gZnVuY3Rpb24gKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkRG91YmxlKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlVUludDggPSBmdW5jdGlvbiAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0IDwgYnVmLmxlbmd0aCwgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmKVxuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWYubGVuZ3RoKSByZXR1cm5cblxuICBidWZbb2Zmc2V0XSA9IHZhbHVlXG59XG5cbmZ1bmN0aW9uIF93cml0ZVVJbnQxNiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgbGl0dGxlRW5kaWFuID09PSAnYm9vbGVhbicsICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAxIDwgYnVmLmxlbmd0aCwgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmZmYpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbilcbiAgICByZXR1cm5cblxuICBmb3IgKHZhciBpID0gMCwgaiA9IE1hdGgubWluKGxlbiAtIG9mZnNldCwgMik7IGkgPCBqOyBpKyspIHtcbiAgICBidWZbb2Zmc2V0ICsgaV0gPVxuICAgICAgICAodmFsdWUgJiAoMHhmZiA8PCAoOCAqIChsaXR0bGVFbmRpYW4gPyBpIDogMSAtIGkpKSkpID4+PlxuICAgICAgICAgICAgKGxpdHRsZUVuZGlhbiA/IGkgOiAxIC0gaSkgKiA4XG4gIH1cbn1cblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZVVJbnQxNkxFID0gZnVuY3Rpb24gKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZVVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZVVJbnQxNkJFID0gZnVuY3Rpb24gKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZVVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF93cml0ZVVJbnQzMiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgbGl0dGxlRW5kaWFuID09PSAnYm9vbGVhbicsICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmZmZmZmZmKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pXG4gICAgcmV0dXJuXG5cbiAgZm9yICh2YXIgaSA9IDAsIGogPSBNYXRoLm1pbihsZW4gLSBvZmZzZXQsIDQpOyBpIDwgajsgaSsrKSB7XG4gICAgYnVmW29mZnNldCArIGldID1cbiAgICAgICAgKHZhbHVlID4+PiAobGl0dGxlRW5kaWFuID8gaSA6IDMgLSBpKSAqIDgpICYgMHhmZlxuICB9XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVVSW50MzJMRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVVSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVVSW50MzJCRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVVSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlSW50OCA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB2YXIgYnVmID0gdGhpc1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsICdtaXNzaW5nIHZhbHVlJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZnNpbnQodmFsdWUsIDB4N2YsIC0weDgwKVxuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWYubGVuZ3RoKVxuICAgIHJldHVyblxuXG4gIGlmICh2YWx1ZSA+PSAwKVxuICAgIGJ1Zi53cml0ZVVJbnQ4KHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KVxuICBlbHNlXG4gICAgYnVmLndyaXRlVUludDgoMHhmZiArIHZhbHVlICsgMSwgb2Zmc2V0LCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gX3dyaXRlSW50MTYgKGJ1ZiwgdmFsdWUsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsICdtaXNzaW5nIHZhbHVlJylcbiAgICBhc3NlcnQodHlwZW9mIGxpdHRsZUVuZGlhbiA9PT0gJ2Jvb2xlYW4nLCAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMSA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmc2ludCh2YWx1ZSwgMHg3ZmZmLCAtMHg4MDAwKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pXG4gICAgcmV0dXJuXG5cbiAgaWYgKHZhbHVlID49IDApXG4gICAgX3dyaXRlVUludDE2KGJ1ZiwgdmFsdWUsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydClcbiAgZWxzZVxuICAgIF93cml0ZVVJbnQxNihidWYsIDB4ZmZmZiArIHZhbHVlICsgMSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlSW50MTZMRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZUludDE2QkUgPSBmdW5jdGlvbiAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfd3JpdGVJbnQzMiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgbGl0dGxlRW5kaWFuID09PSAnYm9vbGVhbicsICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZzaW50KHZhbHVlLCAweDdmZmZmZmZmLCAtMHg4MDAwMDAwMClcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKVxuICAgIHJldHVyblxuXG4gIGlmICh2YWx1ZSA+PSAwKVxuICAgIF93cml0ZVVJbnQzMihidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpXG4gIGVsc2VcbiAgICBfd3JpdGVVSW50MzIoYnVmLCAweGZmZmZmZmZmICsgdmFsdWUgKyAxLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVJbnQzMkxFID0gZnVuY3Rpb24gKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZUludDMyKHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlSW50MzJCRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVJbnQzMih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF93cml0ZUZsb2F0IChidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KHR5cGVvZiBsaXR0bGVFbmRpYW4gPT09ICdib29sZWFuJywgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDMgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZklFRUU3NTQodmFsdWUsIDMuNDAyODIzNDY2Mzg1Mjg4NmUrMzgsIC0zLjQwMjgyMzQ2NjM4NTI4ODZlKzM4KVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pXG4gICAgcmV0dXJuXG5cbiAgaWVlZTc1NC53cml0ZShidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgMjMsIDQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVGbG9hdExFID0gZnVuY3Rpb24gKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZUZsb2F0KHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlRmxvYXRCRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVGbG9hdCh0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF93cml0ZURvdWJsZSAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgbGl0dGxlRW5kaWFuID09PSAnYm9vbGVhbicsICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyA3IDwgYnVmLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZJRUVFNzU0KHZhbHVlLCAxLjc5NzY5MzEzNDg2MjMxNTdFKzMwOCwgLTEuNzk3NjkzMTM0ODYyMzE1N0UrMzA4KVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pXG4gICAgcmV0dXJuXG5cbiAgaWVlZTc1NC53cml0ZShidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgNTIsIDgpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVEb3VibGVMRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVEb3VibGUodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVEb3VibGVCRSA9IGZ1bmN0aW9uICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVEb3VibGUodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG4vLyBmaWxsKHZhbHVlLCBzdGFydD0wLCBlbmQ9YnVmZmVyLmxlbmd0aClcbkJ1ZmZlci5wcm90b3R5cGUuZmlsbCA9IGZ1bmN0aW9uICh2YWx1ZSwgc3RhcnQsIGVuZCkge1xuICBpZiAoIXZhbHVlKSB2YWx1ZSA9IDBcbiAgaWYgKCFzdGFydCkgc3RhcnQgPSAwXG4gIGlmICghZW5kKSBlbmQgPSB0aGlzLmxlbmd0aFxuXG4gIGlmICh0eXBlb2YgdmFsdWUgPT09ICdzdHJpbmcnKSB7XG4gICAgdmFsdWUgPSB2YWx1ZS5jaGFyQ29kZUF0KDApXG4gIH1cblxuICBpZiAodHlwZW9mIHZhbHVlICE9PSAnbnVtYmVyJyB8fCBpc05hTih2YWx1ZSkpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3ZhbHVlIGlzIG5vdCBhIG51bWJlcicpXG4gIH1cblxuICBpZiAoZW5kIDwgc3RhcnQpIHRocm93IG5ldyBFcnJvcignZW5kIDwgc3RhcnQnKVxuXG4gIC8vIEZpbGwgMCBieXRlczsgd2UncmUgZG9uZVxuICBpZiAoZW5kID09PSBzdGFydCkgcmV0dXJuXG4gIGlmICh0aGlzLmxlbmd0aCA9PT0gMCkgcmV0dXJuXG5cbiAgaWYgKHN0YXJ0IDwgMCB8fCBzdGFydCA+PSB0aGlzLmxlbmd0aCkge1xuICAgIHRocm93IG5ldyBFcnJvcignc3RhcnQgb3V0IG9mIGJvdW5kcycpXG4gIH1cblxuICBpZiAoZW5kIDwgMCB8fCBlbmQgPiB0aGlzLmxlbmd0aCkge1xuICAgIHRocm93IG5ldyBFcnJvcignZW5kIG91dCBvZiBib3VuZHMnKVxuICB9XG5cbiAgZm9yICh2YXIgaSA9IHN0YXJ0OyBpIDwgZW5kOyBpKyspIHtcbiAgICB0aGlzW2ldID0gdmFsdWVcbiAgfVxufVxuXG5CdWZmZXIucHJvdG90eXBlLmluc3BlY3QgPSBmdW5jdGlvbiAoKSB7XG4gIHZhciBvdXQgPSBbXVxuICB2YXIgbGVuID0gdGhpcy5sZW5ndGhcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBsZW47IGkrKykge1xuICAgIG91dFtpXSA9IHRvSGV4KHRoaXNbaV0pXG4gICAgaWYgKGkgPT09IGV4cG9ydHMuSU5TUEVDVF9NQVhfQllURVMpIHtcbiAgICAgIG91dFtpICsgMV0gPSAnLi4uJ1xuICAgICAgYnJlYWtcbiAgICB9XG4gIH1cbiAgcmV0dXJuICc8QnVmZmVyICcgKyBvdXQuam9pbignICcpICsgJz4nXG59XG5cbi8qKlxuICogQ3JlYXRlcyBhIG5ldyBgQXJyYXlCdWZmZXJgIHdpdGggdGhlICpjb3BpZWQqIG1lbW9yeSBvZiB0aGUgYnVmZmVyIGluc3RhbmNlLlxuICogQWRkZWQgaW4gTm9kZSAwLjEyLiBOb3QgYWRkZWQgdG8gQnVmZmVyLnByb3RvdHlwZSBzaW5jZSBpdCBzaG91bGQgb25seVxuICogYmUgYXZhaWxhYmxlIGluIGJyb3dzZXJzIHRoYXQgc3VwcG9ydCBBcnJheUJ1ZmZlci5cbiAqL1xuZnVuY3Rpb24gQnVmZmVyVG9BcnJheUJ1ZmZlciAoKSB7XG4gIHJldHVybiAobmV3IEJ1ZmZlcih0aGlzKSkuYnVmZmVyXG59XG5cbi8vIEhFTFBFUiBGVU5DVElPTlNcbi8vID09PT09PT09PT09PT09PT1cblxuZnVuY3Rpb24gc3RyaW5ndHJpbSAoc3RyKSB7XG4gIGlmIChzdHIudHJpbSkgcmV0dXJuIHN0ci50cmltKClcbiAgcmV0dXJuIHN0ci5yZXBsYWNlKC9eXFxzK3xcXHMrJC9nLCAnJylcbn1cblxudmFyIEJQID0gQnVmZmVyLnByb3RvdHlwZVxuXG5mdW5jdGlvbiBhdWdtZW50IChhcnIpIHtcbiAgYXJyLl9pc0J1ZmZlciA9IHRydWVcblxuICAvLyBBdWdtZW50IHRoZSBVaW50OEFycmF5ICppbnN0YW5jZSogKG5vdCB0aGUgY2xhc3MhKSB3aXRoIEJ1ZmZlciBtZXRob2RzXG4gIGFyci53cml0ZSA9IEJQLndyaXRlXG4gIGFyci50b1N0cmluZyA9IEJQLnRvU3RyaW5nXG4gIGFyci50b0xvY2FsZVN0cmluZyA9IEJQLnRvU3RyaW5nXG4gIGFyci50b0pTT04gPSBCUC50b0pTT05cbiAgYXJyLmNvcHkgPSBCUC5jb3B5XG4gIGFyci5zbGljZSA9IEJQLnNsaWNlXG4gIGFyci5yZWFkVUludDggPSBCUC5yZWFkVUludDhcbiAgYXJyLnJlYWRVSW50MTZMRSA9IEJQLnJlYWRVSW50MTZMRVxuICBhcnIucmVhZFVJbnQxNkJFID0gQlAucmVhZFVJbnQxNkJFXG4gIGFyci5yZWFkVUludDMyTEUgPSBCUC5yZWFkVUludDMyTEVcbiAgYXJyLnJlYWRVSW50MzJCRSA9IEJQLnJlYWRVSW50MzJCRVxuICBhcnIucmVhZEludDggPSBCUC5yZWFkSW50OFxuICBhcnIucmVhZEludDE2TEUgPSBCUC5yZWFkSW50MTZMRVxuICBhcnIucmVhZEludDE2QkUgPSBCUC5yZWFkSW50MTZCRVxuICBhcnIucmVhZEludDMyTEUgPSBCUC5yZWFkSW50MzJMRVxuICBhcnIucmVhZEludDMyQkUgPSBCUC5yZWFkSW50MzJCRVxuICBhcnIucmVhZEZsb2F0TEUgPSBCUC5yZWFkRmxvYXRMRVxuICBhcnIucmVhZEZsb2F0QkUgPSBCUC5yZWFkRmxvYXRCRVxuICBhcnIucmVhZERvdWJsZUxFID0gQlAucmVhZERvdWJsZUxFXG4gIGFyci5yZWFkRG91YmxlQkUgPSBCUC5yZWFkRG91YmxlQkVcbiAgYXJyLndyaXRlVUludDggPSBCUC53cml0ZVVJbnQ4XG4gIGFyci53cml0ZVVJbnQxNkxFID0gQlAud3JpdGVVSW50MTZMRVxuICBhcnIud3JpdGVVSW50MTZCRSA9IEJQLndyaXRlVUludDE2QkVcbiAgYXJyLndyaXRlVUludDMyTEUgPSBCUC53cml0ZVVJbnQzMkxFXG4gIGFyci53cml0ZVVJbnQzMkJFID0gQlAud3JpdGVVSW50MzJCRVxuICBhcnIud3JpdGVJbnQ4ID0gQlAud3JpdGVJbnQ4XG4gIGFyci53cml0ZUludDE2TEUgPSBCUC53cml0ZUludDE2TEVcbiAgYXJyLndyaXRlSW50MTZCRSA9IEJQLndyaXRlSW50MTZCRVxuICBhcnIud3JpdGVJbnQzMkxFID0gQlAud3JpdGVJbnQzMkxFXG4gIGFyci53cml0ZUludDMyQkUgPSBCUC53cml0ZUludDMyQkVcbiAgYXJyLndyaXRlRmxvYXRMRSA9IEJQLndyaXRlRmxvYXRMRVxuICBhcnIud3JpdGVGbG9hdEJFID0gQlAud3JpdGVGbG9hdEJFXG4gIGFyci53cml0ZURvdWJsZUxFID0gQlAud3JpdGVEb3VibGVMRVxuICBhcnIud3JpdGVEb3VibGVCRSA9IEJQLndyaXRlRG91YmxlQkVcbiAgYXJyLmZpbGwgPSBCUC5maWxsXG4gIGFyci5pbnNwZWN0ID0gQlAuaW5zcGVjdFxuICBhcnIudG9BcnJheUJ1ZmZlciA9IEJ1ZmZlclRvQXJyYXlCdWZmZXJcblxuICByZXR1cm4gYXJyXG59XG5cbi8vIHNsaWNlKHN0YXJ0LCBlbmQpXG5mdW5jdGlvbiBjbGFtcCAoaW5kZXgsIGxlbiwgZGVmYXVsdFZhbHVlKSB7XG4gIGlmICh0eXBlb2YgaW5kZXggIT09ICdudW1iZXInKSByZXR1cm4gZGVmYXVsdFZhbHVlXG4gIGluZGV4ID0gfn5pbmRleDsgIC8vIENvZXJjZSB0byBpbnRlZ2VyLlxuICBpZiAoaW5kZXggPj0gbGVuKSByZXR1cm4gbGVuXG4gIGlmIChpbmRleCA+PSAwKSByZXR1cm4gaW5kZXhcbiAgaW5kZXggKz0gbGVuXG4gIGlmIChpbmRleCA+PSAwKSByZXR1cm4gaW5kZXhcbiAgcmV0dXJuIDBcbn1cblxuZnVuY3Rpb24gY29lcmNlIChsZW5ndGgpIHtcbiAgLy8gQ29lcmNlIGxlbmd0aCB0byBhIG51bWJlciAocG9zc2libHkgTmFOKSwgcm91bmQgdXBcbiAgLy8gaW4gY2FzZSBpdCdzIGZyYWN0aW9uYWwgKGUuZy4gMTIzLjQ1NikgdGhlbiBkbyBhXG4gIC8vIGRvdWJsZSBuZWdhdGUgdG8gY29lcmNlIGEgTmFOIHRvIDAuIEVhc3ksIHJpZ2h0P1xuICBsZW5ndGggPSB+fk1hdGguY2VpbCgrbGVuZ3RoKVxuICByZXR1cm4gbGVuZ3RoIDwgMCA/IDAgOiBsZW5ndGhcbn1cblxuZnVuY3Rpb24gaXNBcnJheSAoc3ViamVjdCkge1xuICByZXR1cm4gKEFycmF5LmlzQXJyYXkgfHwgZnVuY3Rpb24gKHN1YmplY3QpIHtcbiAgICByZXR1cm4gT2JqZWN0LnByb3RvdHlwZS50b1N0cmluZy5jYWxsKHN1YmplY3QpID09PSAnW29iamVjdCBBcnJheV0nXG4gIH0pKHN1YmplY3QpXG59XG5cbmZ1bmN0aW9uIGlzQXJyYXlpc2ggKHN1YmplY3QpIHtcbiAgcmV0dXJuIGlzQXJyYXkoc3ViamVjdCkgfHwgQnVmZmVyLmlzQnVmZmVyKHN1YmplY3QpIHx8XG4gICAgICBzdWJqZWN0ICYmIHR5cGVvZiBzdWJqZWN0ID09PSAnb2JqZWN0JyAmJlxuICAgICAgdHlwZW9mIHN1YmplY3QubGVuZ3RoID09PSAnbnVtYmVyJ1xufVxuXG5mdW5jdGlvbiB0b0hleCAobikge1xuICBpZiAobiA8IDE2KSByZXR1cm4gJzAnICsgbi50b1N0cmluZygxNilcbiAgcmV0dXJuIG4udG9TdHJpbmcoMTYpXG59XG5cbmZ1bmN0aW9uIHV0ZjhUb0J5dGVzIChzdHIpIHtcbiAgdmFyIGJ5dGVBcnJheSA9IFtdXG4gIGZvciAodmFyIGkgPSAwOyBpIDwgc3RyLmxlbmd0aDsgaSsrKVxuICAgIGlmIChzdHIuY2hhckNvZGVBdChpKSA8PSAweDdGKVxuICAgICAgYnl0ZUFycmF5LnB1c2goc3RyLmNoYXJDb2RlQXQoaSkpXG4gICAgZWxzZSB7XG4gICAgICB2YXIgaCA9IGVuY29kZVVSSUNvbXBvbmVudChzdHIuY2hhckF0KGkpKS5zdWJzdHIoMSkuc3BsaXQoJyUnKVxuICAgICAgZm9yICh2YXIgaiA9IDA7IGogPCBoLmxlbmd0aDsgaisrKVxuICAgICAgICBieXRlQXJyYXkucHVzaChwYXJzZUludChoW2pdLCAxNikpXG4gICAgfVxuICByZXR1cm4gYnl0ZUFycmF5XG59XG5cbmZ1bmN0aW9uIGFzY2lpVG9CeXRlcyAoc3RyKSB7XG4gIHZhciBieXRlQXJyYXkgPSBbXVxuICBmb3IgKHZhciBpID0gMDsgaSA8IHN0ci5sZW5ndGg7IGkrKykge1xuICAgIC8vIE5vZGUncyBjb2RlIHNlZW1zIHRvIGJlIGRvaW5nIHRoaXMgYW5kIG5vdCAmIDB4N0YuLlxuICAgIGJ5dGVBcnJheS5wdXNoKHN0ci5jaGFyQ29kZUF0KGkpICYgMHhGRilcbiAgfVxuICByZXR1cm4gYnl0ZUFycmF5XG59XG5cbmZ1bmN0aW9uIGJhc2U2NFRvQnl0ZXMgKHN0cikge1xuICByZXR1cm4gYmFzZTY0LnRvQnl0ZUFycmF5KHN0cilcbn1cblxuZnVuY3Rpb24gYmxpdEJ1ZmZlciAoc3JjLCBkc3QsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIHZhciBwb3NcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBsZW5ndGg7IGkrKykge1xuICAgIGlmICgoaSArIG9mZnNldCA+PSBkc3QubGVuZ3RoKSB8fCAoaSA+PSBzcmMubGVuZ3RoKSlcbiAgICAgIGJyZWFrXG4gICAgZHN0W2kgKyBvZmZzZXRdID0gc3JjW2ldXG4gIH1cbiAgcmV0dXJuIGlcbn1cblxuZnVuY3Rpb24gZGVjb2RlVXRmOENoYXIgKHN0cikge1xuICB0cnkge1xuICAgIHJldHVybiBkZWNvZGVVUklDb21wb25lbnQoc3RyKVxuICB9IGNhdGNoIChlcnIpIHtcbiAgICByZXR1cm4gU3RyaW5nLmZyb21DaGFyQ29kZSgweEZGRkQpIC8vIFVURiA4IGludmFsaWQgY2hhclxuICB9XG59XG5cbi8qXG4gKiBXZSBoYXZlIHRvIG1ha2Ugc3VyZSB0aGF0IHRoZSB2YWx1ZSBpcyBhIHZhbGlkIGludGVnZXIuIFRoaXMgbWVhbnMgdGhhdCBpdFxuICogaXMgbm9uLW5lZ2F0aXZlLiBJdCBoYXMgbm8gZnJhY3Rpb25hbCBjb21wb25lbnQgYW5kIHRoYXQgaXQgZG9lcyBub3RcbiAqIGV4Y2VlZCB0aGUgbWF4aW11bSBhbGxvd2VkIHZhbHVlLlxuICovXG5mdW5jdGlvbiB2ZXJpZnVpbnQgKHZhbHVlLCBtYXgpIHtcbiAgYXNzZXJ0KHR5cGVvZiB2YWx1ZSA9PSAnbnVtYmVyJywgJ2Nhbm5vdCB3cml0ZSBhIG5vbi1udW1iZXIgYXMgYSBudW1iZXInKVxuICBhc3NlcnQodmFsdWUgPj0gMCxcbiAgICAgICdzcGVjaWZpZWQgYSBuZWdhdGl2ZSB2YWx1ZSBmb3Igd3JpdGluZyBhbiB1bnNpZ25lZCB2YWx1ZScpXG4gIGFzc2VydCh2YWx1ZSA8PSBtYXgsICd2YWx1ZSBpcyBsYXJnZXIgdGhhbiBtYXhpbXVtIHZhbHVlIGZvciB0eXBlJylcbiAgYXNzZXJ0KE1hdGguZmxvb3IodmFsdWUpID09PSB2YWx1ZSwgJ3ZhbHVlIGhhcyBhIGZyYWN0aW9uYWwgY29tcG9uZW50Jylcbn1cblxuZnVuY3Rpb24gdmVyaWZzaW50KHZhbHVlLCBtYXgsIG1pbikge1xuICBhc3NlcnQodHlwZW9mIHZhbHVlID09ICdudW1iZXInLCAnY2Fubm90IHdyaXRlIGEgbm9uLW51bWJlciBhcyBhIG51bWJlcicpXG4gIGFzc2VydCh2YWx1ZSA8PSBtYXgsICd2YWx1ZSBsYXJnZXIgdGhhbiBtYXhpbXVtIGFsbG93ZWQgdmFsdWUnKVxuICBhc3NlcnQodmFsdWUgPj0gbWluLCAndmFsdWUgc21hbGxlciB0aGFuIG1pbmltdW0gYWxsb3dlZCB2YWx1ZScpXG4gIGFzc2VydChNYXRoLmZsb29yKHZhbHVlKSA9PT0gdmFsdWUsICd2YWx1ZSBoYXMgYSBmcmFjdGlvbmFsIGNvbXBvbmVudCcpXG59XG5cbmZ1bmN0aW9uIHZlcmlmSUVFRTc1NCh2YWx1ZSwgbWF4LCBtaW4pIHtcbiAgYXNzZXJ0KHR5cGVvZiB2YWx1ZSA9PSAnbnVtYmVyJywgJ2Nhbm5vdCB3cml0ZSBhIG5vbi1udW1iZXIgYXMgYSBudW1iZXInKVxuICBhc3NlcnQodmFsdWUgPD0gbWF4LCAndmFsdWUgbGFyZ2VyIHRoYW4gbWF4aW11bSBhbGxvd2VkIHZhbHVlJylcbiAgYXNzZXJ0KHZhbHVlID49IG1pbiwgJ3ZhbHVlIHNtYWxsZXIgdGhhbiBtaW5pbXVtIGFsbG93ZWQgdmFsdWUnKVxufVxuXG5mdW5jdGlvbiBhc3NlcnQgKHRlc3QsIG1lc3NhZ2UpIHtcbiAgaWYgKCF0ZXN0KSB0aHJvdyBuZXcgRXJyb3IobWVzc2FnZSB8fCAnRmFpbGVkIGFzc2VydGlvbicpXG59XG4iLCIoZnVuY3Rpb24gKGV4cG9ydHMpIHtcblx0J3VzZSBzdHJpY3QnO1xuXG5cdHZhciBsb29rdXAgPSAnQUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5ejAxMjM0NTY3ODkrLyc7XG5cblx0ZnVuY3Rpb24gYjY0VG9CeXRlQXJyYXkoYjY0KSB7XG5cdFx0dmFyIGksIGosIGwsIHRtcCwgcGxhY2VIb2xkZXJzLCBhcnI7XG5cdFxuXHRcdGlmIChiNjQubGVuZ3RoICUgNCA+IDApIHtcblx0XHRcdHRocm93ICdJbnZhbGlkIHN0cmluZy4gTGVuZ3RoIG11c3QgYmUgYSBtdWx0aXBsZSBvZiA0Jztcblx0XHR9XG5cblx0XHQvLyB0aGUgbnVtYmVyIG9mIGVxdWFsIHNpZ25zIChwbGFjZSBob2xkZXJzKVxuXHRcdC8vIGlmIHRoZXJlIGFyZSB0d28gcGxhY2Vob2xkZXJzLCB0aGFuIHRoZSB0d28gY2hhcmFjdGVycyBiZWZvcmUgaXRcblx0XHQvLyByZXByZXNlbnQgb25lIGJ5dGVcblx0XHQvLyBpZiB0aGVyZSBpcyBvbmx5IG9uZSwgdGhlbiB0aGUgdGhyZWUgY2hhcmFjdGVycyBiZWZvcmUgaXQgcmVwcmVzZW50IDIgYnl0ZXNcblx0XHQvLyB0aGlzIGlzIGp1c3QgYSBjaGVhcCBoYWNrIHRvIG5vdCBkbyBpbmRleE9mIHR3aWNlXG5cdFx0cGxhY2VIb2xkZXJzID0gaW5kZXhPZihiNjQsICc9Jyk7XG5cdFx0cGxhY2VIb2xkZXJzID0gcGxhY2VIb2xkZXJzID4gMCA/IGI2NC5sZW5ndGggLSBwbGFjZUhvbGRlcnMgOiAwO1xuXG5cdFx0Ly8gYmFzZTY0IGlzIDQvMyArIHVwIHRvIHR3byBjaGFyYWN0ZXJzIG9mIHRoZSBvcmlnaW5hbCBkYXRhXG5cdFx0YXJyID0gW107Ly9uZXcgVWludDhBcnJheShiNjQubGVuZ3RoICogMyAvIDQgLSBwbGFjZUhvbGRlcnMpO1xuXG5cdFx0Ly8gaWYgdGhlcmUgYXJlIHBsYWNlaG9sZGVycywgb25seSBnZXQgdXAgdG8gdGhlIGxhc3QgY29tcGxldGUgNCBjaGFyc1xuXHRcdGwgPSBwbGFjZUhvbGRlcnMgPiAwID8gYjY0Lmxlbmd0aCAtIDQgOiBiNjQubGVuZ3RoO1xuXG5cdFx0Zm9yIChpID0gMCwgaiA9IDA7IGkgPCBsOyBpICs9IDQsIGogKz0gMykge1xuXHRcdFx0dG1wID0gKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkpKSA8PCAxOCkgfCAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSArIDEpKSA8PCAxMikgfCAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSArIDIpKSA8PCA2KSB8IGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkgKyAzKSk7XG5cdFx0XHRhcnIucHVzaCgodG1wICYgMHhGRjAwMDApID4+IDE2KTtcblx0XHRcdGFyci5wdXNoKCh0bXAgJiAweEZGMDApID4+IDgpO1xuXHRcdFx0YXJyLnB1c2godG1wICYgMHhGRik7XG5cdFx0fVxuXG5cdFx0aWYgKHBsYWNlSG9sZGVycyA9PT0gMikge1xuXHRcdFx0dG1wID0gKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkpKSA8PCAyKSB8IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpICsgMSkpID4+IDQpO1xuXHRcdFx0YXJyLnB1c2godG1wICYgMHhGRik7XG5cdFx0fSBlbHNlIGlmIChwbGFjZUhvbGRlcnMgPT09IDEpIHtcblx0XHRcdHRtcCA9IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpKSkgPDwgMTApIHwgKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkgKyAxKSkgPDwgNCkgfCAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSArIDIpKSA+PiAyKTtcblx0XHRcdGFyci5wdXNoKCh0bXAgPj4gOCkgJiAweEZGKTtcblx0XHRcdGFyci5wdXNoKHRtcCAmIDB4RkYpO1xuXHRcdH1cblxuXHRcdHJldHVybiBhcnI7XG5cdH1cblxuXHRmdW5jdGlvbiB1aW50OFRvQmFzZTY0KHVpbnQ4KSB7XG5cdFx0dmFyIGksXG5cdFx0XHRleHRyYUJ5dGVzID0gdWludDgubGVuZ3RoICUgMywgLy8gaWYgd2UgaGF2ZSAxIGJ5dGUgbGVmdCwgcGFkIDIgYnl0ZXNcblx0XHRcdG91dHB1dCA9IFwiXCIsXG5cdFx0XHR0ZW1wLCBsZW5ndGg7XG5cblx0XHRmdW5jdGlvbiB0cmlwbGV0VG9CYXNlNjQgKG51bSkge1xuXHRcdFx0cmV0dXJuIGxvb2t1cC5jaGFyQXQobnVtID4+IDE4ICYgMHgzRikgKyBsb29rdXAuY2hhckF0KG51bSA+PiAxMiAmIDB4M0YpICsgbG9va3VwLmNoYXJBdChudW0gPj4gNiAmIDB4M0YpICsgbG9va3VwLmNoYXJBdChudW0gJiAweDNGKTtcblx0XHR9O1xuXG5cdFx0Ly8gZ28gdGhyb3VnaCB0aGUgYXJyYXkgZXZlcnkgdGhyZWUgYnl0ZXMsIHdlJ2xsIGRlYWwgd2l0aCB0cmFpbGluZyBzdHVmZiBsYXRlclxuXHRcdGZvciAoaSA9IDAsIGxlbmd0aCA9IHVpbnQ4Lmxlbmd0aCAtIGV4dHJhQnl0ZXM7IGkgPCBsZW5ndGg7IGkgKz0gMykge1xuXHRcdFx0dGVtcCA9ICh1aW50OFtpXSA8PCAxNikgKyAodWludDhbaSArIDFdIDw8IDgpICsgKHVpbnQ4W2kgKyAyXSk7XG5cdFx0XHRvdXRwdXQgKz0gdHJpcGxldFRvQmFzZTY0KHRlbXApO1xuXHRcdH1cblxuXHRcdC8vIHBhZCB0aGUgZW5kIHdpdGggemVyb3MsIGJ1dCBtYWtlIHN1cmUgdG8gbm90IGZvcmdldCB0aGUgZXh0cmEgYnl0ZXNcblx0XHRzd2l0Y2ggKGV4dHJhQnl0ZXMpIHtcblx0XHRcdGNhc2UgMTpcblx0XHRcdFx0dGVtcCA9IHVpbnQ4W3VpbnQ4Lmxlbmd0aCAtIDFdO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwLmNoYXJBdCh0ZW1wID4+IDIpO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwLmNoYXJBdCgodGVtcCA8PCA0KSAmIDB4M0YpO1xuXHRcdFx0XHRvdXRwdXQgKz0gJz09Jztcblx0XHRcdFx0YnJlYWs7XG5cdFx0XHRjYXNlIDI6XG5cdFx0XHRcdHRlbXAgPSAodWludDhbdWludDgubGVuZ3RoIC0gMl0gPDwgOCkgKyAodWludDhbdWludDgubGVuZ3RoIC0gMV0pO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwLmNoYXJBdCh0ZW1wID4+IDEwKTtcblx0XHRcdFx0b3V0cHV0ICs9IGxvb2t1cC5jaGFyQXQoKHRlbXAgPj4gNCkgJiAweDNGKTtcblx0XHRcdFx0b3V0cHV0ICs9IGxvb2t1cC5jaGFyQXQoKHRlbXAgPDwgMikgJiAweDNGKTtcblx0XHRcdFx0b3V0cHV0ICs9ICc9Jztcblx0XHRcdFx0YnJlYWs7XG5cdFx0fVxuXG5cdFx0cmV0dXJuIG91dHB1dDtcblx0fVxuXG5cdG1vZHVsZS5leHBvcnRzLnRvQnl0ZUFycmF5ID0gYjY0VG9CeXRlQXJyYXk7XG5cdG1vZHVsZS5leHBvcnRzLmZyb21CeXRlQXJyYXkgPSB1aW50OFRvQmFzZTY0O1xufSgpKTtcblxuZnVuY3Rpb24gaW5kZXhPZiAoYXJyLCBlbHQgLyosIGZyb20qLykge1xuXHR2YXIgbGVuID0gYXJyLmxlbmd0aDtcblxuXHR2YXIgZnJvbSA9IE51bWJlcihhcmd1bWVudHNbMV0pIHx8IDA7XG5cdGZyb20gPSAoZnJvbSA8IDApXG5cdFx0PyBNYXRoLmNlaWwoZnJvbSlcblx0XHQ6IE1hdGguZmxvb3IoZnJvbSk7XG5cdGlmIChmcm9tIDwgMClcblx0XHRmcm9tICs9IGxlbjtcblxuXHRmb3IgKDsgZnJvbSA8IGxlbjsgZnJvbSsrKSB7XG5cdFx0aWYgKCh0eXBlb2YgYXJyID09PSAnc3RyaW5nJyAmJiBhcnIuY2hhckF0KGZyb20pID09PSBlbHQpIHx8XG5cdFx0XHRcdCh0eXBlb2YgYXJyICE9PSAnc3RyaW5nJyAmJiBhcnJbZnJvbV0gPT09IGVsdCkpIHtcblx0XHRcdHJldHVybiBmcm9tO1xuXHRcdH1cblx0fVxuXHRyZXR1cm4gLTE7XG59XG4iLCJ2YXIgcHJvY2Vzcz1yZXF1aXJlKFwiX19icm93c2VyaWZ5X3Byb2Nlc3NcIiksZ2xvYmFsPXR5cGVvZiBzZWxmICE9PSBcInVuZGVmaW5lZFwiID8gc2VsZiA6IHR5cGVvZiB3aW5kb3cgIT09IFwidW5kZWZpbmVkXCIgPyB3aW5kb3cgOiB7fTsvKiFcbiAqIEJlbmNobWFyay5qcyB2MS4wLjAgPGh0dHA6Ly9iZW5jaG1hcmtqcy5jb20vPlxuICogQ29weXJpZ2h0IDIwMTAtMjAxMiBNYXRoaWFzIEJ5bmVucyA8aHR0cDovL210aHMuYmUvPlxuICogQmFzZWQgb24gSlNMaXRtdXMuanMsIGNvcHlyaWdodCBSb2JlcnQgS2llZmZlciA8aHR0cDovL2Jyb29mYS5jb20vPlxuICogTW9kaWZpZWQgYnkgSm9obi1EYXZpZCBEYWx0b24gPGh0dHA6Ly9hbGx5b3VjYW5sZWV0LmNvbS8+XG4gKiBBdmFpbGFibGUgdW5kZXIgTUlUIGxpY2Vuc2UgPGh0dHA6Ly9tdGhzLmJlL21pdD5cbiAqL1xuOyhmdW5jdGlvbih3aW5kb3csIHVuZGVmaW5lZCkge1xuICAndXNlIHN0cmljdCc7XG5cbiAgLyoqIFVzZWQgdG8gYXNzaWduIGVhY2ggYmVuY2htYXJrIGFuIGluY3JpbWVudGVkIGlkICovXG4gIHZhciBjb3VudGVyID0gMDtcblxuICAvKiogRGV0ZWN0IERPTSBkb2N1bWVudCBvYmplY3QgKi9cbiAgdmFyIGRvYyA9IGlzSG9zdFR5cGUod2luZG93LCAnZG9jdW1lbnQnKSAmJiBkb2N1bWVudDtcblxuICAvKiogRGV0ZWN0IGZyZWUgdmFyaWFibGUgYGRlZmluZWAgKi9cbiAgdmFyIGZyZWVEZWZpbmUgPSB0eXBlb2YgZGVmaW5lID09ICdmdW5jdGlvbicgJiZcbiAgICB0eXBlb2YgZGVmaW5lLmFtZCA9PSAnb2JqZWN0JyAmJiBkZWZpbmUuYW1kICYmIGRlZmluZTtcblxuICAvKiogRGV0ZWN0IGZyZWUgdmFyaWFibGUgYGV4cG9ydHNgICovXG4gIHZhciBmcmVlRXhwb3J0cyA9IHR5cGVvZiBleHBvcnRzID09ICdvYmplY3QnICYmIGV4cG9ydHMgJiZcbiAgICAodHlwZW9mIGdsb2JhbCA9PSAnb2JqZWN0JyAmJiBnbG9iYWwgJiYgZ2xvYmFsID09IGdsb2JhbC5nbG9iYWwgJiYgKHdpbmRvdyA9IGdsb2JhbCksIGV4cG9ydHMpO1xuXG4gIC8qKiBEZXRlY3QgZnJlZSB2YXJpYWJsZSBgcmVxdWlyZWAgKi9cbiAgdmFyIGZyZWVSZXF1aXJlID0gdHlwZW9mIHJlcXVpcmUgPT0gJ2Z1bmN0aW9uJyAmJiByZXF1aXJlO1xuXG4gIC8qKiBVc2VkIHRvIGNyYXdsIGFsbCBwcm9wZXJ0aWVzIHJlZ2FyZGxlc3Mgb2YgZW51bWVyYWJpbGl0eSAqL1xuICB2YXIgZ2V0QWxsS2V5cyA9IE9iamVjdC5nZXRPd25Qcm9wZXJ0eU5hbWVzO1xuXG4gIC8qKiBVc2VkIHRvIGdldCBwcm9wZXJ0eSBkZXNjcmlwdG9ycyAqL1xuICB2YXIgZ2V0RGVzY3JpcHRvciA9IE9iamVjdC5nZXRPd25Qcm9wZXJ0eURlc2NyaXB0b3I7XG5cbiAgLyoqIFVzZWQgaW4gY2FzZSBhbiBvYmplY3QgZG9lc24ndCBoYXZlIGl0cyBvd24gbWV0aG9kICovXG4gIHZhciBoYXNPd25Qcm9wZXJ0eSA9IHt9Lmhhc093blByb3BlcnR5O1xuXG4gIC8qKiBVc2VkIHRvIGNoZWNrIGlmIGFuIG9iamVjdCBpcyBleHRlbnNpYmxlICovXG4gIHZhciBpc0V4dGVuc2libGUgPSBPYmplY3QuaXNFeHRlbnNpYmxlIHx8IGZ1bmN0aW9uKCkgeyByZXR1cm4gdHJ1ZTsgfTtcblxuICAvKiogVXNlZCB0byBhY2Nlc3MgV2FkZSBTaW1tb25zJyBOb2RlIG1pY3JvdGltZSBtb2R1bGUgKi9cbiAgdmFyIG1pY3JvdGltZU9iamVjdCA9IHJlcSgnbWljcm90aW1lJyk7XG5cbiAgLyoqIFVzZWQgdG8gYWNjZXNzIHRoZSBicm93c2VyJ3MgaGlnaCByZXNvbHV0aW9uIHRpbWVyICovXG4gIHZhciBwZXJmT2JqZWN0ID0gaXNIb3N0VHlwZSh3aW5kb3csICdwZXJmb3JtYW5jZScpICYmIHBlcmZvcm1hbmNlO1xuXG4gIC8qKiBVc2VkIHRvIGNhbGwgdGhlIGJyb3dzZXIncyBoaWdoIHJlc29sdXRpb24gdGltZXIgKi9cbiAgdmFyIHBlcmZOYW1lID0gcGVyZk9iamVjdCAmJiAoXG4gICAgcGVyZk9iamVjdC5ub3cgJiYgJ25vdycgfHxcbiAgICBwZXJmT2JqZWN0LndlYmtpdE5vdyAmJiAnd2Via2l0Tm93J1xuICApO1xuXG4gIC8qKiBVc2VkIHRvIGFjY2VzcyBOb2RlJ3MgaGlnaCByZXNvbHV0aW9uIHRpbWVyICovXG4gIHZhciBwcm9jZXNzT2JqZWN0ID0gaXNIb3N0VHlwZSh3aW5kb3csICdwcm9jZXNzJykgJiYgcHJvY2VzcztcblxuICAvKiogVXNlZCB0byBjaGVjayBpZiBhbiBvd24gcHJvcGVydHkgaXMgZW51bWVyYWJsZSAqL1xuICB2YXIgcHJvcGVydHlJc0VudW1lcmFibGUgPSB7fS5wcm9wZXJ0eUlzRW51bWVyYWJsZTtcblxuICAvKiogVXNlZCB0byBzZXQgcHJvcGVydHkgZGVzY3JpcHRvcnMgKi9cbiAgdmFyIHNldERlc2NyaXB0b3IgPSBPYmplY3QuZGVmaW5lUHJvcGVydHk7XG5cbiAgLyoqIFVzZWQgdG8gcmVzb2x2ZSBhIHZhbHVlJ3MgaW50ZXJuYWwgW1tDbGFzc11dICovXG4gIHZhciB0b1N0cmluZyA9IHt9LnRvU3RyaW5nO1xuXG4gIC8qKiBVc2VkIHRvIHByZXZlbnQgYSBgcmVtb3ZlQ2hpbGRgIG1lbW9yeSBsZWFrIGluIElFIDwgOSAqL1xuICB2YXIgdHJhc2ggPSBkb2MgJiYgZG9jLmNyZWF0ZUVsZW1lbnQoJ2RpdicpO1xuXG4gIC8qKiBVc2VkIHRvIGludGVncml0eSBjaGVjayBjb21waWxlZCB0ZXN0cyAqL1xuICB2YXIgdWlkID0gJ3VpZCcgKyAoK25ldyBEYXRlKTtcblxuICAvKiogVXNlZCB0byBhdm9pZCBpbmZpbml0ZSByZWN1cnNpb24gd2hlbiBtZXRob2RzIGNhbGwgZWFjaCBvdGhlciAqL1xuICB2YXIgY2FsbGVkQnkgPSB7fTtcblxuICAvKiogVXNlZCB0byBhdm9pZCBoeiBvZiBJbmZpbml0eSAqL1xuICB2YXIgZGl2aXNvcnMgPSB7XG4gICAgJzEnOiA0MDk2LFxuICAgICcyJzogNTEyLFxuICAgICczJzogNjQsXG4gICAgJzQnOiA4LFxuICAgICc1JzogMFxuICB9O1xuXG4gIC8qKlxuICAgKiBULURpc3RyaWJ1dGlvbiB0d28tdGFpbGVkIGNyaXRpY2FsIHZhbHVlcyBmb3IgOTUlIGNvbmZpZGVuY2VcbiAgICogaHR0cDovL3d3dy5pdGwubmlzdC5nb3YvZGl2ODk4L2hhbmRib29rL2VkYS9zZWN0aW9uMy9lZGEzNjcyLmh0bVxuICAgKi9cbiAgdmFyIHRUYWJsZSA9IHtcbiAgICAnMSc6ICAxMi43MDYsJzInOiAgNC4zMDMsICczJzogIDMuMTgyLCAnNCc6ICAyLjc3NiwgJzUnOiAgMi41NzEsICc2JzogIDIuNDQ3LFxuICAgICc3JzogIDIuMzY1LCAnOCc6ICAyLjMwNiwgJzknOiAgMi4yNjIsICcxMCc6IDIuMjI4LCAnMTEnOiAyLjIwMSwgJzEyJzogMi4xNzksXG4gICAgJzEzJzogMi4xNiwgICcxNCc6IDIuMTQ1LCAnMTUnOiAyLjEzMSwgJzE2JzogMi4xMiwgICcxNyc6IDIuMTEsICAnMTgnOiAyLjEwMSxcbiAgICAnMTknOiAyLjA5MywgJzIwJzogMi4wODYsICcyMSc6IDIuMDgsICAnMjInOiAyLjA3NCwgJzIzJzogMi4wNjksICcyNCc6IDIuMDY0LFxuICAgICcyNSc6IDIuMDYsICAnMjYnOiAyLjA1NiwgJzI3JzogMi4wNTIsICcyOCc6IDIuMDQ4LCAnMjknOiAyLjA0NSwgJzMwJzogMi4wNDIsXG4gICAgJ2luZmluaXR5JzogMS45NlxuICB9O1xuXG4gIC8qKlxuICAgKiBDcml0aWNhbCBNYW5uLVdoaXRuZXkgVS12YWx1ZXMgZm9yIDk1JSBjb25maWRlbmNlXG4gICAqIGh0dHA6Ly93d3cuc2FidXJjaGlsbC5jb20vSUJiaW9sb2d5L3N0YXRzLzAwMy5odG1sXG4gICAqL1xuICB2YXIgdVRhYmxlID0ge1xuICAgICc1JzogIFswLCAxLCAyXSxcbiAgICAnNic6ICBbMSwgMiwgMywgNV0sXG4gICAgJzcnOiAgWzEsIDMsIDUsIDYsIDhdLFxuICAgICc4JzogIFsyLCA0LCA2LCA4LCAxMCwgMTNdLFxuICAgICc5JzogIFsyLCA0LCA3LCAxMCwgMTIsIDE1LCAxN10sXG4gICAgJzEwJzogWzMsIDUsIDgsIDExLCAxNCwgMTcsIDIwLCAyM10sXG4gICAgJzExJzogWzMsIDYsIDksIDEzLCAxNiwgMTksIDIzLCAyNiwgMzBdLFxuICAgICcxMic6IFs0LCA3LCAxMSwgMTQsIDE4LCAyMiwgMjYsIDI5LCAzMywgMzddLFxuICAgICcxMyc6IFs0LCA4LCAxMiwgMTYsIDIwLCAyNCwgMjgsIDMzLCAzNywgNDEsIDQ1XSxcbiAgICAnMTQnOiBbNSwgOSwgMTMsIDE3LCAyMiwgMjYsIDMxLCAzNiwgNDAsIDQ1LCA1MCwgNTVdLFxuICAgICcxNSc6IFs1LCAxMCwgMTQsIDE5LCAyNCwgMjksIDM0LCAzOSwgNDQsIDQ5LCA1NCwgNTksIDY0XSxcbiAgICAnMTYnOiBbNiwgMTEsIDE1LCAyMSwgMjYsIDMxLCAzNywgNDIsIDQ3LCA1MywgNTksIDY0LCA3MCwgNzVdLFxuICAgICcxNyc6IFs2LCAxMSwgMTcsIDIyLCAyOCwgMzQsIDM5LCA0NSwgNTEsIDU3LCA2MywgNjcsIDc1LCA4MSwgODddLFxuICAgICcxOCc6IFs3LCAxMiwgMTgsIDI0LCAzMCwgMzYsIDQyLCA0OCwgNTUsIDYxLCA2NywgNzQsIDgwLCA4NiwgOTMsIDk5XSxcbiAgICAnMTknOiBbNywgMTMsIDE5LCAyNSwgMzIsIDM4LCA0NSwgNTIsIDU4LCA2NSwgNzIsIDc4LCA4NSwgOTIsIDk5LCAxMDYsIDExM10sXG4gICAgJzIwJzogWzgsIDE0LCAyMCwgMjcsIDM0LCA0MSwgNDgsIDU1LCA2MiwgNjksIDc2LCA4MywgOTAsIDk4LCAxMDUsIDExMiwgMTE5LCAxMjddLFxuICAgICcyMSc6IFs4LCAxNSwgMjIsIDI5LCAzNiwgNDMsIDUwLCA1OCwgNjUsIDczLCA4MCwgODgsIDk2LCAxMDMsIDExMSwgMTE5LCAxMjYsIDEzNCwgMTQyXSxcbiAgICAnMjInOiBbOSwgMTYsIDIzLCAzMCwgMzgsIDQ1LCA1MywgNjEsIDY5LCA3NywgODUsIDkzLCAxMDEsIDEwOSwgMTE3LCAxMjUsIDEzMywgMTQxLCAxNTAsIDE1OF0sXG4gICAgJzIzJzogWzksIDE3LCAyNCwgMzIsIDQwLCA0OCwgNTYsIDY0LCA3MywgODEsIDg5LCA5OCwgMTA2LCAxMTUsIDEyMywgMTMyLCAxNDAsIDE0OSwgMTU3LCAxNjYsIDE3NV0sXG4gICAgJzI0JzogWzEwLCAxNywgMjUsIDMzLCA0MiwgNTAsIDU5LCA2NywgNzYsIDg1LCA5NCwgMTAyLCAxMTEsIDEyMCwgMTI5LCAxMzgsIDE0NywgMTU2LCAxNjUsIDE3NCwgMTgzLCAxOTJdLFxuICAgICcyNSc6IFsxMCwgMTgsIDI3LCAzNSwgNDQsIDUzLCA2MiwgNzEsIDgwLCA4OSwgOTgsIDEwNywgMTE3LCAxMjYsIDEzNSwgMTQ1LCAxNTQsIDE2MywgMTczLCAxODIsIDE5MiwgMjAxLCAyMTFdLFxuICAgICcyNic6IFsxMSwgMTksIDI4LCAzNywgNDYsIDU1LCA2NCwgNzQsIDgzLCA5MywgMTAyLCAxMTIsIDEyMiwgMTMyLCAxNDEsIDE1MSwgMTYxLCAxNzEsIDE4MSwgMTkxLCAyMDAsIDIxMCwgMjIwLCAyMzBdLFxuICAgICcyNyc6IFsxMSwgMjAsIDI5LCAzOCwgNDgsIDU3LCA2NywgNzcsIDg3LCA5NywgMTA3LCAxMTgsIDEyNSwgMTM4LCAxNDcsIDE1OCwgMTY4LCAxNzgsIDE4OCwgMTk5LCAyMDksIDIxOSwgMjMwLCAyNDAsIDI1MF0sXG4gICAgJzI4JzogWzEyLCAyMSwgMzAsIDQwLCA1MCwgNjAsIDcwLCA4MCwgOTAsIDEwMSwgMTExLCAxMjIsIDEzMiwgMTQzLCAxNTQsIDE2NCwgMTc1LCAxODYsIDE5NiwgMjA3LCAyMTgsIDIyOCwgMjM5LCAyNTAsIDI2MSwgMjcyXSxcbiAgICAnMjknOiBbMTMsIDIyLCAzMiwgNDIsIDUyLCA2MiwgNzMsIDgzLCA5NCwgMTA1LCAxMTYsIDEyNywgMTM4LCAxNDksIDE2MCwgMTcxLCAxODIsIDE5MywgMjA0LCAyMTUsIDIyNiwgMjM4LCAyNDksIDI2MCwgMjcxLCAyODIsIDI5NF0sXG4gICAgJzMwJzogWzEzLCAyMywgMzMsIDQzLCA1NCwgNjUsIDc2LCA4NywgOTgsIDEwOSwgMTIwLCAxMzEsIDE0MywgMTU0LCAxNjYsIDE3NywgMTg5LCAyMDAsIDIxMiwgMjIzLCAyMzUsIDI0NywgMjU4LCAyNzAsIDI4MiwgMjkzLCAzMDUsIDMxN11cbiAgfTtcblxuICAvKipcbiAgICogQW4gb2JqZWN0IHVzZWQgdG8gZmxhZyBlbnZpcm9ubWVudHMvZmVhdHVyZXMuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAdHlwZSBPYmplY3RcbiAgICovXG4gIHZhciBzdXBwb3J0ID0ge307XG5cbiAgKGZ1bmN0aW9uKCkge1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IEFkb2JlIEFJUi5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LmFpciA9IGlzQ2xhc3NPZih3aW5kb3cucnVudGltZSwgJ1NjcmlwdEJyaWRnaW5nUHJveHlPYmplY3QnKTtcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBpZiBgYXJndW1lbnRzYCBvYmplY3RzIGhhdmUgdGhlIGNvcnJlY3QgaW50ZXJuYWwgW1tDbGFzc11dIHZhbHVlLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5zdXBwb3J0XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgIHN1cHBvcnQuYXJndW1lbnRzQ2xhc3MgPSBpc0NsYXNzT2YoYXJndW1lbnRzLCAnQXJndW1lbnRzJyk7XG5cbiAgICAvKipcbiAgICAgKiBEZXRlY3QgaWYgaW4gYSBicm93c2VyIGVudmlyb25tZW50LlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5zdXBwb3J0XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgIHN1cHBvcnQuYnJvd3NlciA9IGRvYyAmJiBpc0hvc3RUeXBlKHdpbmRvdywgJ25hdmlnYXRvcicpO1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIHN0cmluZ3Mgc3VwcG9ydCBhY2Nlc3NpbmcgY2hhcmFjdGVycyBieSBpbmRleC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LmNoYXJCeUluZGV4ID1cbiAgICAgIC8vIElFIDggc3VwcG9ydHMgaW5kZXhlcyBvbiBzdHJpbmcgbGl0ZXJhbHMgYnV0IG5vdCBzdHJpbmcgb2JqZWN0c1xuICAgICAgKCd4J1swXSArIE9iamVjdCgneCcpWzBdKSA9PSAneHgnO1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIHN0cmluZ3MgaGF2ZSBpbmRleGVzIGFzIG93biBwcm9wZXJ0aWVzLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5zdXBwb3J0XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgIHN1cHBvcnQuY2hhckJ5T3duSW5kZXggPVxuICAgICAgLy8gTmFyd2hhbCwgUmhpbm8sIFJpbmdvSlMsIElFIDgsIGFuZCBPcGVyYSA8IDEwLjUyIHN1cHBvcnQgaW5kZXhlcyBvblxuICAgICAgLy8gc3RyaW5ncyBidXQgZG9uJ3QgZGV0ZWN0IHRoZW0gYXMgb3duIHByb3BlcnRpZXNcbiAgICAgIHN1cHBvcnQuY2hhckJ5SW5kZXggJiYgaGFzS2V5KCd4JywgJzAnKTtcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBpZiBKYXZhIGlzIGVuYWJsZWQvZXhwb3NlZC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LmphdmEgPSBpc0NsYXNzT2Yod2luZG93LmphdmEsICdKYXZhUGFja2FnZScpO1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIHRoZSBUaW1lcnMgQVBJIGV4aXN0cy5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LnRpbWVvdXQgPSBpc0hvc3RUeXBlKHdpbmRvdywgJ3NldFRpbWVvdXQnKSAmJiBpc0hvc3RUeXBlKHdpbmRvdywgJ2NsZWFyVGltZW91dCcpO1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIGZ1bmN0aW9ucyBzdXBwb3J0IGRlY29tcGlsYXRpb24uXG4gICAgICpcbiAgICAgKiBAbmFtZSBkZWNvbXBpbGF0aW9uXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5zdXBwb3J0XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgIHRyeSB7XG4gICAgICAvLyBTYWZhcmkgMi54IHJlbW92ZXMgY29tbWFzIGluIG9iamVjdCBsaXRlcmFsc1xuICAgICAgLy8gZnJvbSBGdW5jdGlvbiN0b1N0cmluZyByZXN1bHRzXG4gICAgICAvLyBodHRwOi8vd2Viay5pdC8xMTYwOVxuICAgICAgLy8gRmlyZWZveCAzLjYgYW5kIE9wZXJhIDkuMjUgc3RyaXAgZ3JvdXBpbmdcbiAgICAgIC8vIHBhcmVudGhlc2VzIGZyb20gRnVuY3Rpb24jdG9TdHJpbmcgcmVzdWx0c1xuICAgICAgLy8gaHR0cDovL2J1Z3ppbC5sYS81NTk0MzhcbiAgICAgIHN1cHBvcnQuZGVjb21waWxhdGlvbiA9IEZ1bmN0aW9uKFxuICAgICAgICAncmV0dXJuICgnICsgKGZ1bmN0aW9uKHgpIHsgcmV0dXJuIHsgJ3gnOiAnJyArICgxICsgeCkgKyAnJywgJ3knOiAwIH07IH0pICsgJyknXG4gICAgICApKCkoMCkueCA9PT0gJzEnO1xuICAgIH0gY2F0Y2goZSkge1xuICAgICAgc3VwcG9ydC5kZWNvbXBpbGF0aW9uID0gZmFsc2U7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IEVTNSsgcHJvcGVydHkgZGVzY3JpcHRvciBBUEkuXG4gICAgICpcbiAgICAgKiBAbmFtZSBkZXNjcmlwdG9yc1xuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICB0cnkge1xuICAgICAgdmFyIG8gPSB7fTtcbiAgICAgIHN1cHBvcnQuZGVzY3JpcHRvcnMgPSAoc2V0RGVzY3JpcHRvcihvLCBvLCBvKSwgJ3ZhbHVlJyBpbiBnZXREZXNjcmlwdG9yKG8sIG8pKTtcbiAgICB9IGNhdGNoKGUpIHtcbiAgICAgIHN1cHBvcnQuZGVzY3JpcHRvcnMgPSBmYWxzZTtcbiAgICB9XG5cbiAgICAvKipcbiAgICAgKiBEZXRlY3QgRVM1KyBPYmplY3QuZ2V0T3duUHJvcGVydHlOYW1lcygpLlxuICAgICAqXG4gICAgICogQG5hbWUgZ2V0QWxsS2V5c1xuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICB0cnkge1xuICAgICAgc3VwcG9ydC5nZXRBbGxLZXlzID0gL1xcYnZhbHVlT2ZcXGIvLnRlc3QoZ2V0QWxsS2V5cyhPYmplY3QucHJvdG90eXBlKSk7XG4gICAgfSBjYXRjaChlKSB7XG4gICAgICBzdXBwb3J0LmdldEFsbEtleXMgPSBmYWxzZTtcbiAgICB9XG5cbiAgICAvKipcbiAgICAgKiBEZXRlY3QgaWYgb3duIHByb3BlcnRpZXMgYXJlIGl0ZXJhdGVkIGJlZm9yZSBpbmhlcml0ZWQgcHJvcGVydGllcyAoYWxsIGJ1dCBJRSA8IDkpLlxuICAgICAqXG4gICAgICogQG5hbWUgaXRlcmF0ZXNPd25MYXN0XG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5zdXBwb3J0XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgIHN1cHBvcnQuaXRlcmF0ZXNPd25GaXJzdCA9IChmdW5jdGlvbigpIHtcbiAgICAgIHZhciBwcm9wcyA9IFtdO1xuICAgICAgZnVuY3Rpb24gY3RvcigpIHsgdGhpcy54ID0gMTsgfVxuICAgICAgY3Rvci5wcm90b3R5cGUgPSB7ICd5JzogMSB9O1xuICAgICAgZm9yICh2YXIgcHJvcCBpbiBuZXcgY3RvcikgeyBwcm9wcy5wdXNoKHByb3ApOyB9XG4gICAgICByZXR1cm4gcHJvcHNbMF0gPT0gJ3gnO1xuICAgIH0oKSk7XG5cbiAgICAvKipcbiAgICAgKiBEZXRlY3QgaWYgYSBub2RlJ3MgW1tDbGFzc11dIGlzIHJlc29sdmFibGUgKGFsbCBidXQgSUUgPCA5KVxuICAgICAqIGFuZCB0aGF0IHRoZSBKUyBlbmdpbmUgZXJyb3JzIHdoZW4gYXR0ZW1wdGluZyB0byBjb2VyY2UgYW4gb2JqZWN0IHRvIGFcbiAgICAgKiBzdHJpbmcgd2l0aG91dCBhIGB0b1N0cmluZ2AgcHJvcGVydHkgdmFsdWUgb2YgYHR5cGVvZmAgXCJmdW5jdGlvblwiLlxuICAgICAqXG4gICAgICogQG5hbWUgbm9kZUNsYXNzXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5zdXBwb3J0XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgIHRyeSB7XG4gICAgICBzdXBwb3J0Lm5vZGVDbGFzcyA9ICh7ICd0b1N0cmluZyc6IDAgfSArICcnLCB0b1N0cmluZy5jYWxsKGRvYyB8fCAwKSAhPSAnW29iamVjdCBPYmplY3RdJyk7XG4gICAgfSBjYXRjaChlKSB7XG4gICAgICBzdXBwb3J0Lm5vZGVDbGFzcyA9IHRydWU7XG4gICAgfVxuICB9KCkpO1xuXG4gIC8qKlxuICAgKiBUaW1lciBvYmplY3QgdXNlZCBieSBgY2xvY2soKWAgYW5kIGBEZWZlcnJlZCNyZXNvbHZlYC5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHR5cGUgT2JqZWN0XG4gICAqL1xuICB2YXIgdGltZXIgPSB7XG5cbiAgIC8qKlxuICAgICogVGhlIHRpbWVyIG5hbWVzcGFjZSBvYmplY3Qgb3IgY29uc3RydWN0b3IuXG4gICAgKlxuICAgICogQHByaXZhdGVcbiAgICAqIEBtZW1iZXJPZiB0aW1lclxuICAgICogQHR5cGUgRnVuY3Rpb258T2JqZWN0XG4gICAgKi9cbiAgICAnbnMnOiBEYXRlLFxuXG4gICAvKipcbiAgICAqIFN0YXJ0cyB0aGUgZGVmZXJyZWQgdGltZXIuXG4gICAgKlxuICAgICogQHByaXZhdGVcbiAgICAqIEBtZW1iZXJPZiB0aW1lclxuICAgICogQHBhcmFtIHtPYmplY3R9IGRlZmVycmVkIFRoZSBkZWZlcnJlZCBpbnN0YW5jZS5cbiAgICAqL1xuICAgICdzdGFydCc6IG51bGwsIC8vIGxhenkgZGVmaW5lZCBpbiBgY2xvY2soKWBcblxuICAgLyoqXG4gICAgKiBTdG9wcyB0aGUgZGVmZXJyZWQgdGltZXIuXG4gICAgKlxuICAgICogQHByaXZhdGVcbiAgICAqIEBtZW1iZXJPZiB0aW1lclxuICAgICogQHBhcmFtIHtPYmplY3R9IGRlZmVycmVkIFRoZSBkZWZlcnJlZCBpbnN0YW5jZS5cbiAgICAqL1xuICAgICdzdG9wJzogbnVsbCAvLyBsYXp5IGRlZmluZWQgaW4gYGNsb2NrKClgXG4gIH07XG5cbiAgLyoqIFNob3J0Y3V0IGZvciBpbnZlcnNlIHJlc3VsdHMgKi9cbiAgdmFyIG5vQXJndW1lbnRzQ2xhc3MgPSAhc3VwcG9ydC5hcmd1bWVudHNDbGFzcyxcbiAgICAgIG5vQ2hhckJ5SW5kZXggPSAhc3VwcG9ydC5jaGFyQnlJbmRleCxcbiAgICAgIG5vQ2hhckJ5T3duSW5kZXggPSAhc3VwcG9ydC5jaGFyQnlPd25JbmRleDtcblxuICAvKiogTWF0aCBzaG9ydGN1dHMgKi9cbiAgdmFyIGFicyAgID0gTWF0aC5hYnMsXG4gICAgICBmbG9vciA9IE1hdGguZmxvb3IsXG4gICAgICBtYXggICA9IE1hdGgubWF4LFxuICAgICAgbWluICAgPSBNYXRoLm1pbixcbiAgICAgIHBvdyAgID0gTWF0aC5wb3csXG4gICAgICBzcXJ0ICA9IE1hdGguc3FydDtcblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogVGhlIEJlbmNobWFyayBjb25zdHJ1Y3Rvci5cbiAgICpcbiAgICogQGNvbnN0cnVjdG9yXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBuYW1lIEEgbmFtZSB0byBpZGVudGlmeSB0aGUgYmVuY2htYXJrLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufFN0cmluZ30gZm4gVGhlIHRlc3QgdG8gYmVuY2htYXJrLlxuICAgKiBAcGFyYW0ge09iamVjdH0gW29wdGlvbnM9e31dIE9wdGlvbnMgb2JqZWN0LlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyBiYXNpYyB1c2FnZSAodGhlIGBuZXdgIG9wZXJhdG9yIGlzIG9wdGlvbmFsKVxuICAgKiB2YXIgYmVuY2ggPSBuZXcgQmVuY2htYXJrKGZuKTtcbiAgICpcbiAgICogLy8gb3IgdXNpbmcgYSBuYW1lIGZpcnN0XG4gICAqIHZhciBiZW5jaCA9IG5ldyBCZW5jaG1hcmsoJ2ZvbycsIGZuKTtcbiAgICpcbiAgICogLy8gb3Igd2l0aCBvcHRpb25zXG4gICAqIHZhciBiZW5jaCA9IG5ldyBCZW5jaG1hcmsoJ2ZvbycsIGZuLCB7XG4gICAqXG4gICAqICAgLy8gZGlzcGxheWVkIGJ5IEJlbmNobWFyayN0b1N0cmluZyBpZiBgbmFtZWAgaXMgbm90IGF2YWlsYWJsZVxuICAgKiAgICdpZCc6ICd4eXonLFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIHRoZSBiZW5jaG1hcmsgc3RhcnRzIHJ1bm5pbmdcbiAgICogICAnb25TdGFydCc6IG9uU3RhcnQsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIGFmdGVyIGVhY2ggcnVuIGN5Y2xlXG4gICAqICAgJ29uQ3ljbGUnOiBvbkN5Y2xlLFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIGFib3J0ZWRcbiAgICogICAnb25BYm9ydCc6IG9uQWJvcnQsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIHdoZW4gYSB0ZXN0IGVycm9yc1xuICAgKiAgICdvbkVycm9yJzogb25FcnJvcixcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiByZXNldFxuICAgKiAgICdvblJlc2V0Jzogb25SZXNldCxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiB0aGUgYmVuY2htYXJrIGNvbXBsZXRlcyBydW5uaW5nXG4gICAqICAgJ29uQ29tcGxldGUnOiBvbkNvbXBsZXRlLFxuICAgKlxuICAgKiAgIC8vIGNvbXBpbGVkL2NhbGxlZCBiZWZvcmUgdGhlIHRlc3QgbG9vcFxuICAgKiAgICdzZXR1cCc6IHNldHVwLFxuICAgKlxuICAgKiAgIC8vIGNvbXBpbGVkL2NhbGxlZCBhZnRlciB0aGUgdGVzdCBsb29wXG4gICAqICAgJ3RlYXJkb3duJzogdGVhcmRvd25cbiAgICogfSk7XG4gICAqXG4gICAqIC8vIG9yIG5hbWUgYW5kIG9wdGlvbnNcbiAgICogdmFyIGJlbmNoID0gbmV3IEJlbmNobWFyaygnZm9vJywge1xuICAgKlxuICAgKiAgIC8vIGEgZmxhZyB0byBpbmRpY2F0ZSB0aGUgYmVuY2htYXJrIGlzIGRlZmVycmVkXG4gICAqICAgJ2RlZmVyJzogdHJ1ZSxcbiAgICpcbiAgICogICAvLyBiZW5jaG1hcmsgdGVzdCBmdW5jdGlvblxuICAgKiAgICdmbic6IGZ1bmN0aW9uKGRlZmVycmVkKSB7XG4gICAqICAgICAvLyBjYWxsIHJlc29sdmUoKSB3aGVuIHRoZSBkZWZlcnJlZCB0ZXN0IGlzIGZpbmlzaGVkXG4gICAqICAgICBkZWZlcnJlZC5yZXNvbHZlKCk7XG4gICAqICAgfVxuICAgKiB9KTtcbiAgICpcbiAgICogLy8gb3Igb3B0aW9ucyBvbmx5XG4gICAqIHZhciBiZW5jaCA9IG5ldyBCZW5jaG1hcmsoe1xuICAgKlxuICAgKiAgIC8vIGJlbmNobWFyayBuYW1lXG4gICAqICAgJ25hbWUnOiAnZm9vJyxcbiAgICpcbiAgICogICAvLyBiZW5jaG1hcmsgdGVzdCBhcyBhIHN0cmluZ1xuICAgKiAgICdmbic6ICdbMSwyLDMsNF0uc29ydCgpJ1xuICAgKiB9KTtcbiAgICpcbiAgICogLy8gYSB0ZXN0J3MgYHRoaXNgIGJpbmRpbmcgaXMgc2V0IHRvIHRoZSBiZW5jaG1hcmsgaW5zdGFuY2VcbiAgICogdmFyIGJlbmNoID0gbmV3IEJlbmNobWFyaygnZm9vJywgZnVuY3Rpb24oKSB7XG4gICAqICAgJ015IG5hbWUgaXMgJy5jb25jYXQodGhpcy5uYW1lKTsgLy8gTXkgbmFtZSBpcyBmb29cbiAgICogfSk7XG4gICAqL1xuICBmdW5jdGlvbiBCZW5jaG1hcmsobmFtZSwgZm4sIG9wdGlvbnMpIHtcbiAgICB2YXIgbWUgPSB0aGlzO1xuXG4gICAgLy8gYWxsb3cgaW5zdGFuY2UgY3JlYXRpb24gd2l0aG91dCB0aGUgYG5ld2Agb3BlcmF0b3JcbiAgICBpZiAobWUgPT0gbnVsbCB8fCBtZS5jb25zdHJ1Y3RvciAhPSBCZW5jaG1hcmspIHtcbiAgICAgIHJldHVybiBuZXcgQmVuY2htYXJrKG5hbWUsIGZuLCBvcHRpb25zKTtcbiAgICB9XG4gICAgLy8ganVnZ2xlIGFyZ3VtZW50c1xuICAgIGlmIChpc0NsYXNzT2YobmFtZSwgJ09iamVjdCcpKSB7XG4gICAgICAvLyAxIGFyZ3VtZW50IChvcHRpb25zKVxuICAgICAgb3B0aW9ucyA9IG5hbWU7XG4gICAgfVxuICAgIGVsc2UgaWYgKGlzQ2xhc3NPZihuYW1lLCAnRnVuY3Rpb24nKSkge1xuICAgICAgLy8gMiBhcmd1bWVudHMgKGZuLCBvcHRpb25zKVxuICAgICAgb3B0aW9ucyA9IGZuO1xuICAgICAgZm4gPSBuYW1lO1xuICAgIH1cbiAgICBlbHNlIGlmIChpc0NsYXNzT2YoZm4sICdPYmplY3QnKSkge1xuICAgICAgLy8gMiBhcmd1bWVudHMgKG5hbWUsIG9wdGlvbnMpXG4gICAgICBvcHRpb25zID0gZm47XG4gICAgICBmbiA9IG51bGw7XG4gICAgICBtZS5uYW1lID0gbmFtZTtcbiAgICB9XG4gICAgZWxzZSB7XG4gICAgICAvLyAzIGFyZ3VtZW50cyAobmFtZSwgZm4gWywgb3B0aW9uc10pXG4gICAgICBtZS5uYW1lID0gbmFtZTtcbiAgICB9XG4gICAgc2V0T3B0aW9ucyhtZSwgb3B0aW9ucyk7XG4gICAgbWUuaWQgfHwgKG1lLmlkID0gKytjb3VudGVyKTtcbiAgICBtZS5mbiA9PSBudWxsICYmIChtZS5mbiA9IGZuKTtcbiAgICBtZS5zdGF0cyA9IGRlZXBDbG9uZShtZS5zdGF0cyk7XG4gICAgbWUudGltZXMgPSBkZWVwQ2xvbmUobWUudGltZXMpO1xuICB9XG5cbiAgLyoqXG4gICAqIFRoZSBEZWZlcnJlZCBjb25zdHJ1Y3Rvci5cbiAgICpcbiAgICogQGNvbnN0cnVjdG9yXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtPYmplY3R9IGNsb25lIFRoZSBjbG9uZWQgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKi9cbiAgZnVuY3Rpb24gRGVmZXJyZWQoY2xvbmUpIHtcbiAgICB2YXIgbWUgPSB0aGlzO1xuICAgIGlmIChtZSA9PSBudWxsIHx8IG1lLmNvbnN0cnVjdG9yICE9IERlZmVycmVkKSB7XG4gICAgICByZXR1cm4gbmV3IERlZmVycmVkKGNsb25lKTtcbiAgICB9XG4gICAgbWUuYmVuY2htYXJrID0gY2xvbmU7XG4gICAgY2xvY2sobWUpO1xuICB9XG5cbiAgLyoqXG4gICAqIFRoZSBFdmVudCBjb25zdHJ1Y3Rvci5cbiAgICpcbiAgICogQGNvbnN0cnVjdG9yXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtTdHJpbmd8T2JqZWN0fSB0eXBlIFRoZSBldmVudCB0eXBlLlxuICAgKi9cbiAgZnVuY3Rpb24gRXZlbnQodHlwZSkge1xuICAgIHZhciBtZSA9IHRoaXM7XG4gICAgcmV0dXJuIChtZSA9PSBudWxsIHx8IG1lLmNvbnN0cnVjdG9yICE9IEV2ZW50KVxuICAgICAgPyBuZXcgRXZlbnQodHlwZSlcbiAgICAgIDogKHR5cGUgaW5zdGFuY2VvZiBFdmVudClcbiAgICAgICAgICA/IHR5cGVcbiAgICAgICAgICA6IGV4dGVuZChtZSwgeyAndGltZVN0YW1wJzogK25ldyBEYXRlIH0sIHR5cGVvZiB0eXBlID09ICdzdHJpbmcnID8geyAndHlwZSc6IHR5cGUgfSA6IHR5cGUpO1xuICB9XG5cbiAgLyoqXG4gICAqIFRoZSBTdWl0ZSBjb25zdHJ1Y3Rvci5cbiAgICpcbiAgICogQGNvbnN0cnVjdG9yXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtTdHJpbmd9IG5hbWUgQSBuYW1lIHRvIGlkZW50aWZ5IHRoZSBzdWl0ZS5cbiAgICogQHBhcmFtIHtPYmplY3R9IFtvcHRpb25zPXt9XSBPcHRpb25zIG9iamVjdC5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gYmFzaWMgdXNhZ2UgKHRoZSBgbmV3YCBvcGVyYXRvciBpcyBvcHRpb25hbClcbiAgICogdmFyIHN1aXRlID0gbmV3IEJlbmNobWFyay5TdWl0ZTtcbiAgICpcbiAgICogLy8gb3IgdXNpbmcgYSBuYW1lIGZpcnN0XG4gICAqIHZhciBzdWl0ZSA9IG5ldyBCZW5jaG1hcmsuU3VpdGUoJ2ZvbycpO1xuICAgKlxuICAgKiAvLyBvciB3aXRoIG9wdGlvbnNcbiAgICogdmFyIHN1aXRlID0gbmV3IEJlbmNobWFyay5TdWl0ZSgnZm9vJywge1xuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIHRoZSBzdWl0ZSBzdGFydHMgcnVubmluZ1xuICAgKiAgICdvblN0YXJ0Jzogb25TdGFydCxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgYmV0d2VlbiBydW5uaW5nIGJlbmNobWFya3NcbiAgICogICAnb25DeWNsZSc6IG9uQ3ljbGUsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIHdoZW4gYWJvcnRlZFxuICAgKiAgICdvbkFib3J0Jzogb25BYm9ydCxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiBhIHRlc3QgZXJyb3JzXG4gICAqICAgJ29uRXJyb3InOiBvbkVycm9yLFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIHJlc2V0XG4gICAqICAgJ29uUmVzZXQnOiBvblJlc2V0LFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIHRoZSBzdWl0ZSBjb21wbGV0ZXMgcnVubmluZ1xuICAgKiAgICdvbkNvbXBsZXRlJzogb25Db21wbGV0ZVxuICAgKiB9KTtcbiAgICovXG4gIGZ1bmN0aW9uIFN1aXRlKG5hbWUsIG9wdGlvbnMpIHtcbiAgICB2YXIgbWUgPSB0aGlzO1xuXG4gICAgLy8gYWxsb3cgaW5zdGFuY2UgY3JlYXRpb24gd2l0aG91dCB0aGUgYG5ld2Agb3BlcmF0b3JcbiAgICBpZiAobWUgPT0gbnVsbCB8fCBtZS5jb25zdHJ1Y3RvciAhPSBTdWl0ZSkge1xuICAgICAgcmV0dXJuIG5ldyBTdWl0ZShuYW1lLCBvcHRpb25zKTtcbiAgICB9XG4gICAgLy8ganVnZ2xlIGFyZ3VtZW50c1xuICAgIGlmIChpc0NsYXNzT2YobmFtZSwgJ09iamVjdCcpKSB7XG4gICAgICAvLyAxIGFyZ3VtZW50IChvcHRpb25zKVxuICAgICAgb3B0aW9ucyA9IG5hbWU7XG4gICAgfSBlbHNlIHtcbiAgICAgIC8vIDIgYXJndW1lbnRzIChuYW1lIFssIG9wdGlvbnNdKVxuICAgICAgbWUubmFtZSA9IG5hbWU7XG4gICAgfVxuICAgIHNldE9wdGlvbnMobWUsIG9wdGlvbnMpO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIE5vdGU6IFNvbWUgYXJyYXkgbWV0aG9kcyBoYXZlIGJlZW4gaW1wbGVtZW50ZWQgaW4gcGxhaW4gSmF2YVNjcmlwdCB0byBhdm9pZFxuICAgKiBidWdzIGluIElFLCBPcGVyYSwgUmhpbm8sIGFuZCBNb2JpbGUgU2FmYXJpLlxuICAgKlxuICAgKiBJRSBjb21wYXRpYmlsaXR5IG1vZGUgYW5kIElFIDwgOSBoYXZlIGJ1Z2d5IEFycmF5IGBzaGlmdCgpYCBhbmQgYHNwbGljZSgpYFxuICAgKiBmdW5jdGlvbnMgdGhhdCBmYWlsIHRvIHJlbW92ZSB0aGUgbGFzdCBlbGVtZW50LCBgb2JqZWN0WzBdYCwgb2ZcbiAgICogYXJyYXktbGlrZS1vYmplY3RzIGV2ZW4gdGhvdWdoIHRoZSBgbGVuZ3RoYCBwcm9wZXJ0eSBpcyBzZXQgdG8gYDBgLlxuICAgKiBUaGUgYHNoaWZ0KClgIG1ldGhvZCBpcyBidWdneSBpbiBJRSA4IGNvbXBhdGliaWxpdHkgbW9kZSwgd2hpbGUgYHNwbGljZSgpYFxuICAgKiBpcyBidWdneSByZWdhcmRsZXNzIG9mIG1vZGUgaW4gSUUgPCA5IGFuZCBidWdneSBpbiBjb21wYXRpYmlsaXR5IG1vZGUgaW4gSUUgOS5cbiAgICpcbiAgICogSW4gT3BlcmEgPCA5LjUwIGFuZCBzb21lIG9sZGVyL2JldGEgTW9iaWxlIFNhZmFyaSB2ZXJzaW9ucyB1c2luZyBgdW5zaGlmdCgpYFxuICAgKiBnZW5lcmljYWxseSB0byBhdWdtZW50IHRoZSBgYXJndW1lbnRzYCBvYmplY3Qgd2lsbCBwYXZlIHRoZSB2YWx1ZSBhdCBpbmRleCAwXG4gICAqIHdpdGhvdXQgaW5jcmltZW50aW5nIHRoZSBvdGhlciB2YWx1ZXMncyBpbmRleGVzLlxuICAgKiBodHRwczovL2dpdGh1Yi5jb20vZG9jdW1lbnRjbG91ZC91bmRlcnNjb3JlL2lzc3Vlcy85XG4gICAqXG4gICAqIFJoaW5vIGFuZCBlbnZpcm9ubWVudHMgaXQgcG93ZXJzLCBsaWtlIE5hcndoYWwgYW5kIFJpbmdvSlMsIG1heSBoYXZlXG4gICAqIGJ1Z2d5IEFycmF5IGBjb25jYXQoKWAsIGByZXZlcnNlKClgLCBgc2hpZnQoKWAsIGBzbGljZSgpYCwgYHNwbGljZSgpYCBhbmRcbiAgICogYHVuc2hpZnQoKWAgZnVuY3Rpb25zIHRoYXQgbWFrZSBzcGFyc2UgYXJyYXlzIG5vbi1zcGFyc2UgYnkgYXNzaWduaW5nIHRoZVxuICAgKiB1bmRlZmluZWQgaW5kZXhlcyBhIHZhbHVlIG9mIHVuZGVmaW5lZC5cbiAgICogaHR0cHM6Ly9naXRodWIuY29tL21vemlsbGEvcmhpbm8vY29tbWl0LzcwMmFiZmVkM2Y4Y2EwNDNiMjYzNmVmZDMxYzE0YmE3NTUyNjAzZGRcbiAgICovXG5cbiAgLyoqXG4gICAqIENyZWF0ZXMgYW4gYXJyYXkgY29udGFpbmluZyB0aGUgZWxlbWVudHMgb2YgdGhlIGhvc3QgYXJyYXkgZm9sbG93ZWQgYnkgdGhlXG4gICAqIGVsZW1lbnRzIG9mIGVhY2ggYXJndW1lbnQgaW4gb3JkZXIuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHJldHVybnMge0FycmF5fSBUaGUgbmV3IGFycmF5LlxuICAgKi9cbiAgZnVuY3Rpb24gY29uY2F0KCkge1xuICAgIHZhciB2YWx1ZSxcbiAgICAgICAgaiA9IC0xLFxuICAgICAgICBsZW5ndGggPSBhcmd1bWVudHMubGVuZ3RoLFxuICAgICAgICByZXN1bHQgPSBzbGljZS5jYWxsKHRoaXMpLFxuICAgICAgICBpbmRleCA9IHJlc3VsdC5sZW5ndGg7XG5cbiAgICB3aGlsZSAoKytqIDwgbGVuZ3RoKSB7XG4gICAgICB2YWx1ZSA9IGFyZ3VtZW50c1tqXTtcbiAgICAgIGlmIChpc0NsYXNzT2YodmFsdWUsICdBcnJheScpKSB7XG4gICAgICAgIGZvciAodmFyIGsgPSAwLCBsID0gdmFsdWUubGVuZ3RoOyBrIDwgbDsgaysrLCBpbmRleCsrKSB7XG4gICAgICAgICAgaWYgKGsgaW4gdmFsdWUpIHtcbiAgICAgICAgICAgIHJlc3VsdFtpbmRleF0gPSB2YWx1ZVtrXTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIHJlc3VsdFtpbmRleCsrXSA9IHZhbHVlO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIFV0aWxpdHkgZnVuY3Rpb24gdXNlZCBieSBgc2hpZnQoKWAsIGBzcGxpY2UoKWAsIGFuZCBgdW5zaGlmdCgpYC5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtOdW1iZXJ9IHN0YXJ0IFRoZSBpbmRleCB0byBzdGFydCBpbnNlcnRpbmcgZWxlbWVudHMuXG4gICAqIEBwYXJhbSB7TnVtYmVyfSBkZWxldGVDb3VudCBUaGUgbnVtYmVyIG9mIGVsZW1lbnRzIHRvIGRlbGV0ZSBmcm9tIHRoZSBpbnNlcnQgcG9pbnQuXG4gICAqIEBwYXJhbSB7QXJyYXl9IGVsZW1lbnRzIFRoZSBlbGVtZW50cyB0byBpbnNlcnQuXG4gICAqIEByZXR1cm5zIHtBcnJheX0gQW4gYXJyYXkgb2YgZGVsZXRlZCBlbGVtZW50cy5cbiAgICovXG4gIGZ1bmN0aW9uIGluc2VydChzdGFydCwgZGVsZXRlQ291bnQsIGVsZW1lbnRzKSB7XG4gICAgLy8gYHJlc3VsdGAgc2hvdWxkIGhhdmUgaXRzIGxlbmd0aCBzZXQgdG8gdGhlIGBkZWxldGVDb3VudGBcbiAgICAvLyBzZWUgaHR0cHM6Ly9idWdzLmVjbWFzY3JpcHQub3JnL3Nob3dfYnVnLmNnaT9pZD0zMzJcbiAgICB2YXIgZGVsZXRlRW5kID0gc3RhcnQgKyBkZWxldGVDb3VudCxcbiAgICAgICAgZWxlbWVudENvdW50ID0gZWxlbWVudHMgPyBlbGVtZW50cy5sZW5ndGggOiAwLFxuICAgICAgICBpbmRleCA9IHN0YXJ0IC0gMSxcbiAgICAgICAgbGVuZ3RoID0gc3RhcnQgKyBlbGVtZW50Q291bnQsXG4gICAgICAgIG9iamVjdCA9IHRoaXMsXG4gICAgICAgIHJlc3VsdCA9IEFycmF5KGRlbGV0ZUNvdW50KSxcbiAgICAgICAgdGFpbCA9IHNsaWNlLmNhbGwob2JqZWN0LCBkZWxldGVFbmQpO1xuXG4gICAgLy8gZGVsZXRlIGVsZW1lbnRzIGZyb20gdGhlIGFycmF5XG4gICAgd2hpbGUgKCsraW5kZXggPCBkZWxldGVFbmQpIHtcbiAgICAgIGlmIChpbmRleCBpbiBvYmplY3QpIHtcbiAgICAgICAgcmVzdWx0W2luZGV4IC0gc3RhcnRdID0gb2JqZWN0W2luZGV4XTtcbiAgICAgICAgZGVsZXRlIG9iamVjdFtpbmRleF07XG4gICAgICB9XG4gICAgfVxuICAgIC8vIGluc2VydCBlbGVtZW50c1xuICAgIGluZGV4ID0gc3RhcnQgLSAxO1xuICAgIHdoaWxlICgrK2luZGV4IDwgbGVuZ3RoKSB7XG4gICAgICBvYmplY3RbaW5kZXhdID0gZWxlbWVudHNbaW5kZXggLSBzdGFydF07XG4gICAgfVxuICAgIC8vIGFwcGVuZCB0YWlsIGVsZW1lbnRzXG4gICAgc3RhcnQgPSBpbmRleC0tO1xuICAgIGxlbmd0aCA9IG1heCgwLCAob2JqZWN0Lmxlbmd0aCA+Pj4gMCkgLSBkZWxldGVDb3VudCArIGVsZW1lbnRDb3VudCk7XG4gICAgd2hpbGUgKCsraW5kZXggPCBsZW5ndGgpIHtcbiAgICAgIGlmICgoaW5kZXggLSBzdGFydCkgaW4gdGFpbCkge1xuICAgICAgICBvYmplY3RbaW5kZXhdID0gdGFpbFtpbmRleCAtIHN0YXJ0XTtcbiAgICAgIH0gZWxzZSBpZiAoaW5kZXggaW4gb2JqZWN0KSB7XG4gICAgICAgIGRlbGV0ZSBvYmplY3RbaW5kZXhdO1xuICAgICAgfVxuICAgIH1cbiAgICAvLyBkZWxldGUgZXhjZXNzIGVsZW1lbnRzXG4gICAgZGVsZXRlQ291bnQgPSBkZWxldGVDb3VudCA+IGVsZW1lbnRDb3VudCA/IGRlbGV0ZUNvdW50IC0gZWxlbWVudENvdW50IDogMDtcbiAgICB3aGlsZSAoZGVsZXRlQ291bnQtLSkge1xuICAgICAgaW5kZXggPSBsZW5ndGggKyBkZWxldGVDb3VudDtcbiAgICAgIGlmIChpbmRleCBpbiBvYmplY3QpIHtcbiAgICAgICAgZGVsZXRlIG9iamVjdFtpbmRleF07XG4gICAgICB9XG4gICAgfVxuICAgIG9iamVjdC5sZW5ndGggPSBsZW5ndGg7XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBSZWFycmFuZ2UgdGhlIGhvc3QgYXJyYXkncyBlbGVtZW50cyBpbiByZXZlcnNlIG9yZGVyLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEByZXR1cm5zIHtBcnJheX0gVGhlIHJldmVyc2VkIGFycmF5LlxuICAgKi9cbiAgZnVuY3Rpb24gcmV2ZXJzZSgpIHtcbiAgICB2YXIgdXBwZXJJbmRleCxcbiAgICAgICAgdmFsdWUsXG4gICAgICAgIGluZGV4ID0gLTEsXG4gICAgICAgIG9iamVjdCA9IE9iamVjdCh0aGlzKSxcbiAgICAgICAgbGVuZ3RoID0gb2JqZWN0Lmxlbmd0aCA+Pj4gMCxcbiAgICAgICAgbWlkZGxlID0gZmxvb3IobGVuZ3RoIC8gMik7XG5cbiAgICBpZiAobGVuZ3RoID4gMSkge1xuICAgICAgd2hpbGUgKCsraW5kZXggPCBtaWRkbGUpIHtcbiAgICAgICAgdXBwZXJJbmRleCA9IGxlbmd0aCAtIGluZGV4IC0gMTtcbiAgICAgICAgdmFsdWUgPSB1cHBlckluZGV4IGluIG9iamVjdCA/IG9iamVjdFt1cHBlckluZGV4XSA6IHVpZDtcbiAgICAgICAgaWYgKGluZGV4IGluIG9iamVjdCkge1xuICAgICAgICAgIG9iamVjdFt1cHBlckluZGV4XSA9IG9iamVjdFtpbmRleF07XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgZGVsZXRlIG9iamVjdFt1cHBlckluZGV4XTtcbiAgICAgICAgfVxuICAgICAgICBpZiAodmFsdWUgIT0gdWlkKSB7XG4gICAgICAgICAgb2JqZWN0W2luZGV4XSA9IHZhbHVlO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIGRlbGV0ZSBvYmplY3RbaW5kZXhdO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiBvYmplY3Q7XG4gIH1cblxuICAvKipcbiAgICogUmVtb3ZlcyB0aGUgZmlyc3QgZWxlbWVudCBvZiB0aGUgaG9zdCBhcnJheSBhbmQgcmV0dXJucyBpdC5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcmV0dXJucyB7TWl4ZWR9IFRoZSBmaXJzdCBlbGVtZW50IG9mIHRoZSBhcnJheS5cbiAgICovXG4gIGZ1bmN0aW9uIHNoaWZ0KCkge1xuICAgIHJldHVybiBpbnNlcnQuY2FsbCh0aGlzLCAwLCAxKVswXTtcbiAgfVxuXG4gIC8qKlxuICAgKiBDcmVhdGVzIGFuIGFycmF5IG9mIHRoZSBob3N0IGFycmF5J3MgZWxlbWVudHMgZnJvbSB0aGUgc3RhcnQgaW5kZXggdXAgdG8sXG4gICAqIGJ1dCBub3QgaW5jbHVkaW5nLCB0aGUgZW5kIGluZGV4LlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7TnVtYmVyfSBzdGFydCBUaGUgc3RhcnRpbmcgaW5kZXguXG4gICAqIEBwYXJhbSB7TnVtYmVyfSBlbmQgVGhlIGVuZCBpbmRleC5cbiAgICogQHJldHVybnMge0FycmF5fSBUaGUgbmV3IGFycmF5LlxuICAgKi9cbiAgZnVuY3Rpb24gc2xpY2Uoc3RhcnQsIGVuZCkge1xuICAgIHZhciBpbmRleCA9IC0xLFxuICAgICAgICBvYmplY3QgPSBPYmplY3QodGhpcyksXG4gICAgICAgIGxlbmd0aCA9IG9iamVjdC5sZW5ndGggPj4+IDAsXG4gICAgICAgIHJlc3VsdCA9IFtdO1xuXG4gICAgc3RhcnQgPSB0b0ludGVnZXIoc3RhcnQpO1xuICAgIHN0YXJ0ID0gc3RhcnQgPCAwID8gbWF4KGxlbmd0aCArIHN0YXJ0LCAwKSA6IG1pbihzdGFydCwgbGVuZ3RoKTtcbiAgICBzdGFydC0tO1xuICAgIGVuZCA9IGVuZCA9PSBudWxsID8gbGVuZ3RoIDogdG9JbnRlZ2VyKGVuZCk7XG4gICAgZW5kID0gZW5kIDwgMCA/IG1heChsZW5ndGggKyBlbmQsIDApIDogbWluKGVuZCwgbGVuZ3RoKTtcblxuICAgIHdoaWxlICgoKytpbmRleCwgKytzdGFydCkgPCBlbmQpIHtcbiAgICAgIGlmIChzdGFydCBpbiBvYmplY3QpIHtcbiAgICAgICAgcmVzdWx0W2luZGV4XSA9IG9iamVjdFtzdGFydF07XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogQWxsb3dzIHJlbW92aW5nIGEgcmFuZ2Ugb2YgZWxlbWVudHMgYW5kL29yIGluc2VydGluZyBlbGVtZW50cyBpbnRvIHRoZVxuICAgKiBob3N0IGFycmF5LlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7TnVtYmVyfSBzdGFydCBUaGUgc3RhcnQgaW5kZXguXG4gICAqIEBwYXJhbSB7TnVtYmVyfSBkZWxldGVDb3VudCBUaGUgbnVtYmVyIG9mIGVsZW1lbnRzIHRvIGRlbGV0ZS5cbiAgICogQHBhcmFtIHtNaXhlZH0gW3ZhbDEsIHZhbDIsIC4uLl0gdmFsdWVzIHRvIGluc2VydCBhdCB0aGUgYHN0YXJ0YCBpbmRleC5cbiAgICogQHJldHVybnMge0FycmF5fSBBbiBhcnJheSBvZiByZW1vdmVkIGVsZW1lbnRzLlxuICAgKi9cbiAgZnVuY3Rpb24gc3BsaWNlKHN0YXJ0LCBkZWxldGVDb3VudCkge1xuICAgIHZhciBvYmplY3QgPSBPYmplY3QodGhpcyksXG4gICAgICAgIGxlbmd0aCA9IG9iamVjdC5sZW5ndGggPj4+IDA7XG5cbiAgICBzdGFydCA9IHRvSW50ZWdlcihzdGFydCk7XG4gICAgc3RhcnQgPSBzdGFydCA8IDAgPyBtYXgobGVuZ3RoICsgc3RhcnQsIDApIDogbWluKHN0YXJ0LCBsZW5ndGgpO1xuXG4gICAgLy8gc3VwcG9ydCB0aGUgZGUtZmFjdG8gU3BpZGVyTW9ua2V5IGV4dGVuc2lvblxuICAgIC8vIGh0dHBzOi8vZGV2ZWxvcGVyLm1vemlsbGEub3JnL2VuL0phdmFTY3JpcHQvUmVmZXJlbmNlL0dsb2JhbF9PYmplY3RzL0FycmF5L3NwbGljZSNQYXJhbWV0ZXJzXG4gICAgLy8gaHR0cHM6Ly9idWdzLmVjbWFzY3JpcHQub3JnL3Nob3dfYnVnLmNnaT9pZD00MjlcbiAgICBkZWxldGVDb3VudCA9IGFyZ3VtZW50cy5sZW5ndGggPT0gMVxuICAgICAgPyBsZW5ndGggLSBzdGFydFxuICAgICAgOiBtaW4obWF4KHRvSW50ZWdlcihkZWxldGVDb3VudCksIDApLCBsZW5ndGggLSBzdGFydCk7XG5cbiAgICByZXR1cm4gaW5zZXJ0LmNhbGwob2JqZWN0LCBzdGFydCwgZGVsZXRlQ291bnQsIHNsaWNlLmNhbGwoYXJndW1lbnRzLCAyKSk7XG4gIH1cblxuICAvKipcbiAgICogQ29udmVydHMgdGhlIHNwZWNpZmllZCBgdmFsdWVgIHRvIGFuIGludGVnZXIuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHZhbHVlIFRoZSB2YWx1ZSB0byBjb252ZXJ0LlxuICAgKiBAcmV0dXJucyB7TnVtYmVyfSBUaGUgcmVzdWx0aW5nIGludGVnZXIuXG4gICAqL1xuICBmdW5jdGlvbiB0b0ludGVnZXIodmFsdWUpIHtcbiAgICB2YWx1ZSA9ICt2YWx1ZTtcbiAgICByZXR1cm4gdmFsdWUgPT09IDAgfHwgIWlzRmluaXRlKHZhbHVlKSA/IHZhbHVlIHx8IDAgOiB2YWx1ZSAtICh2YWx1ZSAlIDEpO1xuICB9XG5cbiAgLyoqXG4gICAqIEFwcGVuZHMgYXJndW1lbnRzIHRvIHRoZSBob3N0IGFycmF5LlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEByZXR1cm5zIHtOdW1iZXJ9IFRoZSBuZXcgbGVuZ3RoLlxuICAgKi9cbiAgZnVuY3Rpb24gdW5zaGlmdCgpIHtcbiAgICB2YXIgb2JqZWN0ID0gT2JqZWN0KHRoaXMpO1xuICAgIGluc2VydC5jYWxsKG9iamVjdCwgMCwgMCwgYXJndW1lbnRzKTtcbiAgICByZXR1cm4gb2JqZWN0Lmxlbmd0aDtcbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBBIGdlbmVyaWMgYEZ1bmN0aW9uI2JpbmRgIGxpa2UgbWV0aG9kLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBmbiBUaGUgZnVuY3Rpb24gdG8gYmUgYm91bmQgdG8gYHRoaXNBcmdgLlxuICAgKiBAcGFyYW0ge01peGVkfSB0aGlzQXJnIFRoZSBgdGhpc2AgYmluZGluZyBmb3IgdGhlIGdpdmVuIGZ1bmN0aW9uLlxuICAgKiBAcmV0dXJucyB7RnVuY3Rpb259IFRoZSBib3VuZCBmdW5jdGlvbi5cbiAgICovXG4gIGZ1bmN0aW9uIGJpbmQoZm4sIHRoaXNBcmcpIHtcbiAgICByZXR1cm4gZnVuY3Rpb24oKSB7IGZuLmFwcGx5KHRoaXNBcmcsIGFyZ3VtZW50cyk7IH07XG4gIH1cblxuICAvKipcbiAgICogQ3JlYXRlcyBhIGZ1bmN0aW9uIGZyb20gdGhlIGdpdmVuIGFyZ3VtZW50cyBzdHJpbmcgYW5kIGJvZHkuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBhcmdzIFRoZSBjb21tYSBzZXBhcmF0ZWQgZnVuY3Rpb24gYXJndW1lbnRzLlxuICAgKiBAcGFyYW0ge1N0cmluZ30gYm9keSBUaGUgZnVuY3Rpb24gYm9keS5cbiAgICogQHJldHVybnMge0Z1bmN0aW9ufSBUaGUgbmV3IGZ1bmN0aW9uLlxuICAgKi9cbiAgZnVuY3Rpb24gY3JlYXRlRnVuY3Rpb24oKSB7XG4gICAgLy8gbGF6eSBkZWZpbmVcbiAgICBjcmVhdGVGdW5jdGlvbiA9IGZ1bmN0aW9uKGFyZ3MsIGJvZHkpIHtcbiAgICAgIHZhciByZXN1bHQsXG4gICAgICAgICAgYW5jaG9yID0gZnJlZURlZmluZSA/IGRlZmluZS5hbWQgOiBCZW5jaG1hcmssXG4gICAgICAgICAgcHJvcCA9IHVpZCArICdjcmVhdGVGdW5jdGlvbic7XG5cbiAgICAgIHJ1blNjcmlwdCgoZnJlZURlZmluZSA/ICdkZWZpbmUuYW1kLicgOiAnQmVuY2htYXJrLicpICsgcHJvcCArICc9ZnVuY3Rpb24oJyArIGFyZ3MgKyAnKXsnICsgYm9keSArICd9Jyk7XG4gICAgICByZXN1bHQgPSBhbmNob3JbcHJvcF07XG4gICAgICBkZWxldGUgYW5jaG9yW3Byb3BdO1xuICAgICAgcmV0dXJuIHJlc3VsdDtcbiAgICB9O1xuICAgIC8vIGZpeCBKYWVnZXJNb25rZXkgYnVnXG4gICAgLy8gaHR0cDovL2J1Z3ppbC5sYS82Mzk3MjBcbiAgICBjcmVhdGVGdW5jdGlvbiA9IHN1cHBvcnQuYnJvd3NlciAmJiAoY3JlYXRlRnVuY3Rpb24oJycsICdyZXR1cm5cIicgKyB1aWQgKyAnXCInKSB8fCBub29wKSgpID09IHVpZCA/IGNyZWF0ZUZ1bmN0aW9uIDogRnVuY3Rpb247XG4gICAgcmV0dXJuIGNyZWF0ZUZ1bmN0aW9uLmFwcGx5KG51bGwsIGFyZ3VtZW50cyk7XG4gIH1cblxuICAvKipcbiAgICogRGVsYXkgdGhlIGV4ZWN1dGlvbiBvZiBhIGZ1bmN0aW9uIGJhc2VkIG9uIHRoZSBiZW5jaG1hcmsncyBgZGVsYXlgIHByb3BlcnR5LlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge09iamVjdH0gYmVuY2ggVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQHBhcmFtIHtPYmplY3R9IGZuIFRoZSBmdW5jdGlvbiB0byBleGVjdXRlLlxuICAgKi9cbiAgZnVuY3Rpb24gZGVsYXkoYmVuY2gsIGZuKSB7XG4gICAgYmVuY2guX3RpbWVySWQgPSBzZXRUaW1lb3V0KGZuLCBiZW5jaC5kZWxheSAqIDFlMyk7XG4gIH1cblxuICAvKipcbiAgICogRGVzdHJveXMgdGhlIGdpdmVuIGVsZW1lbnQuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7RWxlbWVudH0gZWxlbWVudCBUaGUgZWxlbWVudCB0byBkZXN0cm95LlxuICAgKi9cbiAgZnVuY3Rpb24gZGVzdHJveUVsZW1lbnQoZWxlbWVudCkge1xuICAgIHRyYXNoLmFwcGVuZENoaWxkKGVsZW1lbnQpO1xuICAgIHRyYXNoLmlubmVySFRNTCA9ICcnO1xuICB9XG5cbiAgLyoqXG4gICAqIEl0ZXJhdGVzIG92ZXIgYW4gb2JqZWN0J3MgcHJvcGVydGllcywgZXhlY3V0aW5nIHRoZSBgY2FsbGJhY2tgIGZvciBlYWNoLlxuICAgKiBDYWxsYmFja3MgbWF5IHRlcm1pbmF0ZSB0aGUgbG9vcCBieSBleHBsaWNpdGx5IHJldHVybmluZyBgZmFsc2VgLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge09iamVjdH0gb2JqZWN0IFRoZSBvYmplY3QgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gZXhlY3V0ZWQgcGVyIG93biBwcm9wZXJ0eS5cbiAgICogQHBhcmFtIHtPYmplY3R9IG9wdGlvbnMgVGhlIG9wdGlvbnMgb2JqZWN0LlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBSZXR1cm5zIHRoZSBvYmplY3QgaXRlcmF0ZWQgb3Zlci5cbiAgICovXG4gIGZ1bmN0aW9uIGZvclByb3BzKCkge1xuICAgIHZhciBmb3JTaGFkb3dlZCxcbiAgICAgICAgc2tpcFNlZW4sXG4gICAgICAgIGZvckFyZ3MgPSB0cnVlLFxuICAgICAgICBzaGFkb3dlZCA9IFsnY29uc3RydWN0b3InLCAnaGFzT3duUHJvcGVydHknLCAnaXNQcm90b3R5cGVPZicsICdwcm9wZXJ0eUlzRW51bWVyYWJsZScsICd0b0xvY2FsZVN0cmluZycsICd0b1N0cmluZycsICd2YWx1ZU9mJ107XG5cbiAgICAoZnVuY3Rpb24oZW51bUZsYWcsIGtleSkge1xuICAgICAgLy8gbXVzdCB1c2UgYSBub24tbmF0aXZlIGNvbnN0cnVjdG9yIHRvIGNhdGNoIHRoZSBTYWZhcmkgMiBpc3N1ZVxuICAgICAgZnVuY3Rpb24gS2xhc3MoKSB7IHRoaXMudmFsdWVPZiA9IDA7IH07XG4gICAgICBLbGFzcy5wcm90b3R5cGUudmFsdWVPZiA9IDA7XG4gICAgICAvLyBjaGVjayB2YXJpb3VzIGZvci1pbiBidWdzXG4gICAgICBmb3IgKGtleSBpbiBuZXcgS2xhc3MpIHtcbiAgICAgICAgZW51bUZsYWcgKz0ga2V5ID09ICd2YWx1ZU9mJyA/IDEgOiAwO1xuICAgICAgfVxuICAgICAgLy8gY2hlY2sgaWYgYGFyZ3VtZW50c2Agb2JqZWN0cyBoYXZlIG5vbi1lbnVtZXJhYmxlIGluZGV4ZXNcbiAgICAgIGZvciAoa2V5IGluIGFyZ3VtZW50cykge1xuICAgICAgICBrZXkgPT0gJzAnICYmIChmb3JBcmdzID0gZmFsc2UpO1xuICAgICAgfVxuICAgICAgLy8gU2FmYXJpIDIgaXRlcmF0ZXMgb3ZlciBzaGFkb3dlZCBwcm9wZXJ0aWVzIHR3aWNlXG4gICAgICAvLyBodHRwOi8vcmVwbGF5LndheWJhY2ttYWNoaW5lLm9yZy8yMDA5MDQyODIyMjk0MS9odHRwOi8vdG9iaWVsYW5nZWwuY29tLzIwMDcvMS8yOS9mb3ItaW4tbG9vcC1icm9rZW4taW4tc2FmYXJpL1xuICAgICAgc2tpcFNlZW4gPSBlbnVtRmxhZyA9PSAyO1xuICAgICAgLy8gSUUgPCA5IGluY29ycmVjdGx5IG1ha2VzIGFuIG9iamVjdCdzIHByb3BlcnRpZXMgbm9uLWVudW1lcmFibGUgaWYgdGhleSBoYXZlXG4gICAgICAvLyB0aGUgc2FtZSBuYW1lIGFzIG90aGVyIG5vbi1lbnVtZXJhYmxlIHByb3BlcnRpZXMgaW4gaXRzIHByb3RvdHlwZSBjaGFpbi5cbiAgICAgIGZvclNoYWRvd2VkID0gIWVudW1GbGFnO1xuICAgIH0oMCkpO1xuXG4gICAgLy8gbGF6eSBkZWZpbmVcbiAgICBmb3JQcm9wcyA9IGZ1bmN0aW9uKG9iamVjdCwgY2FsbGJhY2ssIG9wdGlvbnMpIHtcbiAgICAgIG9wdGlvbnMgfHwgKG9wdGlvbnMgPSB7fSk7XG5cbiAgICAgIHZhciByZXN1bHQgPSBvYmplY3Q7XG4gICAgICBvYmplY3QgPSBPYmplY3Qob2JqZWN0KTtcblxuICAgICAgdmFyIGN0b3IsXG4gICAgICAgICAga2V5LFxuICAgICAgICAgIGtleXMsXG4gICAgICAgICAgc2tpcEN0b3IsXG4gICAgICAgICAgZG9uZSA9ICFyZXN1bHQsXG4gICAgICAgICAgd2hpY2ggPSBvcHRpb25zLndoaWNoLFxuICAgICAgICAgIGFsbEZsYWcgPSB3aGljaCA9PSAnYWxsJyxcbiAgICAgICAgICBpbmRleCA9IC0xLFxuICAgICAgICAgIGl0ZXJhdGVlID0gb2JqZWN0LFxuICAgICAgICAgIGxlbmd0aCA9IG9iamVjdC5sZW5ndGgsXG4gICAgICAgICAgb3duRmxhZyA9IGFsbEZsYWcgfHwgd2hpY2ggPT0gJ293bicsXG4gICAgICAgICAgc2VlbiA9IHt9LFxuICAgICAgICAgIHNraXBQcm90byA9IGlzQ2xhc3NPZihvYmplY3QsICdGdW5jdGlvbicpLFxuICAgICAgICAgIHRoaXNBcmcgPSBvcHRpb25zLmJpbmQ7XG5cbiAgICAgIGlmICh0aGlzQXJnICE9PSB1bmRlZmluZWQpIHtcbiAgICAgICAgY2FsbGJhY2sgPSBiaW5kKGNhbGxiYWNrLCB0aGlzQXJnKTtcbiAgICAgIH1cbiAgICAgIC8vIGl0ZXJhdGUgYWxsIHByb3BlcnRpZXNcbiAgICAgIGlmIChhbGxGbGFnICYmIHN1cHBvcnQuZ2V0QWxsS2V5cykge1xuICAgICAgICBmb3IgKGluZGV4ID0gMCwga2V5cyA9IGdldEFsbEtleXMob2JqZWN0KSwgbGVuZ3RoID0ga2V5cy5sZW5ndGg7IGluZGV4IDwgbGVuZ3RoOyBpbmRleCsrKSB7XG4gICAgICAgICAga2V5ID0ga2V5c1tpbmRleF07XG4gICAgICAgICAgaWYgKGNhbGxiYWNrKG9iamVjdFtrZXldLCBrZXksIG9iamVjdCkgPT09IGZhbHNlKSB7XG4gICAgICAgICAgICBicmVhaztcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIC8vIGVsc2UgaXRlcmF0ZSBvbmx5IGVudW1lcmFibGUgcHJvcGVydGllc1xuICAgICAgZWxzZSB7XG4gICAgICAgIGZvciAoa2V5IGluIG9iamVjdCkge1xuICAgICAgICAgIC8vIEZpcmVmb3ggPCAzLjYsIE9wZXJhID4gOS41MCAtIE9wZXJhIDwgMTEuNjAsIGFuZCBTYWZhcmkgPCA1LjFcbiAgICAgICAgICAvLyAoaWYgdGhlIHByb3RvdHlwZSBvciBhIHByb3BlcnR5IG9uIHRoZSBwcm90b3R5cGUgaGFzIGJlZW4gc2V0KVxuICAgICAgICAgIC8vIGluY29ycmVjdGx5IHNldCBhIGZ1bmN0aW9uJ3MgYHByb3RvdHlwZWAgcHJvcGVydHkgW1tFbnVtZXJhYmxlXV0gdmFsdWVcbiAgICAgICAgICAvLyB0byBgdHJ1ZWAuIEJlY2F1c2Ugb2YgdGhpcyB3ZSBzdGFuZGFyZGl6ZSBvbiBza2lwcGluZyB0aGUgYHByb3RvdHlwZWBcbiAgICAgICAgICAvLyBwcm9wZXJ0eSBvZiBmdW5jdGlvbnMgcmVnYXJkbGVzcyBvZiB0aGVpciBbW0VudW1lcmFibGVdXSB2YWx1ZS5cbiAgICAgICAgICBpZiAoKGRvbmUgPVxuICAgICAgICAgICAgICAhKHNraXBQcm90byAmJiBrZXkgPT0gJ3Byb3RvdHlwZScpICYmXG4gICAgICAgICAgICAgICEoc2tpcFNlZW4gJiYgKGhhc0tleShzZWVuLCBrZXkpIHx8ICEoc2VlbltrZXldID0gdHJ1ZSkpKSAmJlxuICAgICAgICAgICAgICAoIW93bkZsYWcgfHwgb3duRmxhZyAmJiBoYXNLZXkob2JqZWN0LCBrZXkpKSAmJlxuICAgICAgICAgICAgICBjYWxsYmFjayhvYmplY3Rba2V5XSwga2V5LCBvYmplY3QpID09PSBmYWxzZSkpIHtcbiAgICAgICAgICAgIGJyZWFrO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgICAvLyBpbiBJRSA8IDkgc3RyaW5ncyBkb24ndCBzdXBwb3J0IGFjY2Vzc2luZyBjaGFyYWN0ZXJzIGJ5IGluZGV4XG4gICAgICAgIGlmICghZG9uZSAmJiAoZm9yQXJncyAmJiBpc0FyZ3VtZW50cyhvYmplY3QpIHx8XG4gICAgICAgICAgICAoKG5vQ2hhckJ5SW5kZXggfHwgbm9DaGFyQnlPd25JbmRleCkgJiYgaXNDbGFzc09mKG9iamVjdCwgJ1N0cmluZycpICYmXG4gICAgICAgICAgICAgIChpdGVyYXRlZSA9IG5vQ2hhckJ5SW5kZXggPyBvYmplY3Quc3BsaXQoJycpIDogb2JqZWN0KSkpKSB7XG4gICAgICAgICAgd2hpbGUgKCsraW5kZXggPCBsZW5ndGgpIHtcbiAgICAgICAgICAgIGlmICgoZG9uZSA9XG4gICAgICAgICAgICAgICAgY2FsbGJhY2soaXRlcmF0ZWVbaW5kZXhdLCBTdHJpbmcoaW5kZXgpLCBvYmplY3QpID09PSBmYWxzZSkpIHtcbiAgICAgICAgICAgICAgYnJlYWs7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICAgIGlmICghZG9uZSAmJiBmb3JTaGFkb3dlZCkge1xuICAgICAgICAgIC8vIEJlY2F1c2UgSUUgPCA5IGNhbid0IHNldCB0aGUgYFtbRW51bWVyYWJsZV1dYCBhdHRyaWJ1dGUgb2YgYW4gZXhpc3RpbmdcbiAgICAgICAgICAvLyBwcm9wZXJ0eSBhbmQgdGhlIGBjb25zdHJ1Y3RvcmAgcHJvcGVydHkgb2YgYSBwcm90b3R5cGUgZGVmYXVsdHMgdG9cbiAgICAgICAgICAvLyBub24tZW51bWVyYWJsZSwgd2UgbWFudWFsbHkgc2tpcCB0aGUgYGNvbnN0cnVjdG9yYCBwcm9wZXJ0eSB3aGVuIHdlXG4gICAgICAgICAgLy8gdGhpbmsgd2UgYXJlIGl0ZXJhdGluZyBvdmVyIGEgYHByb3RvdHlwZWAgb2JqZWN0LlxuICAgICAgICAgIGN0b3IgPSBvYmplY3QuY29uc3RydWN0b3I7XG4gICAgICAgICAgc2tpcEN0b3IgPSBjdG9yICYmIGN0b3IucHJvdG90eXBlICYmIGN0b3IucHJvdG90eXBlLmNvbnN0cnVjdG9yID09PSBjdG9yO1xuICAgICAgICAgIGZvciAoaW5kZXggPSAwOyBpbmRleCA8IDc7IGluZGV4KyspIHtcbiAgICAgICAgICAgIGtleSA9IHNoYWRvd2VkW2luZGV4XTtcbiAgICAgICAgICAgIGlmICghKHNraXBDdG9yICYmIGtleSA9PSAnY29uc3RydWN0b3InKSAmJlxuICAgICAgICAgICAgICAgIGhhc0tleShvYmplY3QsIGtleSkgJiZcbiAgICAgICAgICAgICAgICBjYWxsYmFjayhvYmplY3Rba2V5XSwga2V5LCBvYmplY3QpID09PSBmYWxzZSkge1xuICAgICAgICAgICAgICBicmVhaztcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIHJldHVybiByZXN1bHQ7XG4gICAgfTtcbiAgICByZXR1cm4gZm9yUHJvcHMuYXBwbHkobnVsbCwgYXJndW1lbnRzKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBHZXRzIHRoZSBuYW1lIG9mIHRoZSBmaXJzdCBhcmd1bWVudCBmcm9tIGEgZnVuY3Rpb24ncyBzb3VyY2UuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7RnVuY3Rpb259IGZuIFRoZSBmdW5jdGlvbi5cbiAgICogQHJldHVybnMge1N0cmluZ30gVGhlIGFyZ3VtZW50IG5hbWUuXG4gICAqL1xuICBmdW5jdGlvbiBnZXRGaXJzdEFyZ3VtZW50KGZuKSB7XG4gICAgcmV0dXJuICghaGFzS2V5KGZuLCAndG9TdHJpbmcnKSAmJlxuICAgICAgKC9eW1xccyhdKmZ1bmN0aW9uW14oXSpcXCgoW15cXHMsKV0rKS8uZXhlYyhmbikgfHwgMClbMV0pIHx8ICcnO1xuICB9XG5cbiAgLyoqXG4gICAqIENvbXB1dGVzIHRoZSBhcml0aG1ldGljIG1lYW4gb2YgYSBzYW1wbGUuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7QXJyYXl9IHNhbXBsZSBUaGUgc2FtcGxlLlxuICAgKiBAcmV0dXJucyB7TnVtYmVyfSBUaGUgbWVhbi5cbiAgICovXG4gIGZ1bmN0aW9uIGdldE1lYW4oc2FtcGxlKSB7XG4gICAgcmV0dXJuIHJlZHVjZShzYW1wbGUsIGZ1bmN0aW9uKHN1bSwgeCkge1xuICAgICAgcmV0dXJuIHN1bSArIHg7XG4gICAgfSkgLyBzYW1wbGUubGVuZ3RoIHx8IDA7XG4gIH1cblxuICAvKipcbiAgICogR2V0cyB0aGUgc291cmNlIGNvZGUgb2YgYSBmdW5jdGlvbi5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gZm4gVGhlIGZ1bmN0aW9uLlxuICAgKiBAcGFyYW0ge1N0cmluZ30gYWx0U291cmNlIEEgc3RyaW5nIHVzZWQgd2hlbiBhIGZ1bmN0aW9uJ3Mgc291cmNlIGNvZGUgaXMgdW5yZXRyaWV2YWJsZS5cbiAgICogQHJldHVybnMge1N0cmluZ30gVGhlIGZ1bmN0aW9uJ3Mgc291cmNlIGNvZGUuXG4gICAqL1xuICBmdW5jdGlvbiBnZXRTb3VyY2UoZm4sIGFsdFNvdXJjZSkge1xuICAgIHZhciByZXN1bHQgPSBhbHRTb3VyY2U7XG4gICAgaWYgKGlzU3RyaW5nYWJsZShmbikpIHtcbiAgICAgIHJlc3VsdCA9IFN0cmluZyhmbik7XG4gICAgfSBlbHNlIGlmIChzdXBwb3J0LmRlY29tcGlsYXRpb24pIHtcbiAgICAgIC8vIGVzY2FwZSB0aGUgYHtgIGZvciBGaXJlZm94IDFcbiAgICAgIHJlc3VsdCA9ICgvXltee10rXFx7KFtcXHNcXFNdKil9XFxzKiQvLmV4ZWMoZm4pIHx8IDApWzFdO1xuICAgIH1cbiAgICAvLyB0cmltIHN0cmluZ1xuICAgIHJlc3VsdCA9IChyZXN1bHQgfHwgJycpLnJlcGxhY2UoL15cXHMrfFxccyskL2csICcnKTtcblxuICAgIC8vIGRldGVjdCBzdHJpbmdzIGNvbnRhaW5pbmcgb25seSB0aGUgXCJ1c2Ugc3RyaWN0XCIgZGlyZWN0aXZlXG4gICAgcmV0dXJuIC9eKD86XFwvXFwqK1tcXHd8XFxXXSo/XFwqXFwvfFxcL1xcLy4qP1tcXG5cXHJcXHUyMDI4XFx1MjAyOV18XFxzKSooW1wiJ10pdXNlIHN0cmljdFxcMTs/JC8udGVzdChyZXN1bHQpXG4gICAgICA/ICcnXG4gICAgICA6IHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBDaGVja3MgaWYgYSB2YWx1ZSBpcyBhbiBgYXJndW1lbnRzYCBvYmplY3QuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHZhbHVlIFRoZSB2YWx1ZSB0byBjaGVjay5cbiAgICogQHJldHVybnMge0Jvb2xlYW59IFJldHVybnMgYHRydWVgIGlmIHRoZSB2YWx1ZSBpcyBhbiBgYXJndW1lbnRzYCBvYmplY3QsIGVsc2UgYGZhbHNlYC5cbiAgICovXG4gIGZ1bmN0aW9uIGlzQXJndW1lbnRzKCkge1xuICAgIC8vIGxhenkgZGVmaW5lXG4gICAgaXNBcmd1bWVudHMgPSBmdW5jdGlvbih2YWx1ZSkge1xuICAgICAgcmV0dXJuIHRvU3RyaW5nLmNhbGwodmFsdWUpID09ICdbb2JqZWN0IEFyZ3VtZW50c10nO1xuICAgIH07XG4gICAgaWYgKG5vQXJndW1lbnRzQ2xhc3MpIHtcbiAgICAgIGlzQXJndW1lbnRzID0gZnVuY3Rpb24odmFsdWUpIHtcbiAgICAgICAgcmV0dXJuIGhhc0tleSh2YWx1ZSwgJ2NhbGxlZScpICYmXG4gICAgICAgICAgIShwcm9wZXJ0eUlzRW51bWVyYWJsZSAmJiBwcm9wZXJ0eUlzRW51bWVyYWJsZS5jYWxsKHZhbHVlLCAnY2FsbGVlJykpO1xuICAgICAgfTtcbiAgICB9XG4gICAgcmV0dXJuIGlzQXJndW1lbnRzKGFyZ3VtZW50c1swXSk7XG4gIH1cblxuICAvKipcbiAgICogQ2hlY2tzIGlmIGFuIG9iamVjdCBpcyBvZiB0aGUgc3BlY2lmaWVkIGNsYXNzLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge01peGVkfSB2YWx1ZSBUaGUgdmFsdWUgdG8gY2hlY2suXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBuYW1lIFRoZSBuYW1lIG9mIHRoZSBjbGFzcy5cbiAgICogQHJldHVybnMge0Jvb2xlYW59IFJldHVybnMgYHRydWVgIGlmIHRoZSB2YWx1ZSBpcyBvZiB0aGUgc3BlY2lmaWVkIGNsYXNzLCBlbHNlIGBmYWxzZWAuXG4gICAqL1xuICBmdW5jdGlvbiBpc0NsYXNzT2YodmFsdWUsIG5hbWUpIHtcbiAgICByZXR1cm4gdmFsdWUgIT0gbnVsbCAmJiB0b1N0cmluZy5jYWxsKHZhbHVlKSA9PSAnW29iamVjdCAnICsgbmFtZSArICddJztcbiAgfVxuXG4gIC8qKlxuICAgKiBIb3N0IG9iamVjdHMgY2FuIHJldHVybiB0eXBlIHZhbHVlcyB0aGF0IGFyZSBkaWZmZXJlbnQgZnJvbSB0aGVpciBhY3R1YWxcbiAgICogZGF0YSB0eXBlLiBUaGUgb2JqZWN0cyB3ZSBhcmUgY29uY2VybmVkIHdpdGggdXN1YWxseSByZXR1cm4gbm9uLXByaW1pdGl2ZVxuICAgKiB0eXBlcyBvZiBvYmplY3QsIGZ1bmN0aW9uLCBvciB1bmtub3duLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge01peGVkfSBvYmplY3QgVGhlIG93bmVyIG9mIHRoZSBwcm9wZXJ0eS5cbiAgICogQHBhcmFtIHtTdHJpbmd9IHByb3BlcnR5IFRoZSBwcm9wZXJ0eSB0byBjaGVjay5cbiAgICogQHJldHVybnMge0Jvb2xlYW59IFJldHVybnMgYHRydWVgIGlmIHRoZSBwcm9wZXJ0eSB2YWx1ZSBpcyBhIG5vbi1wcmltaXRpdmUsIGVsc2UgYGZhbHNlYC5cbiAgICovXG4gIGZ1bmN0aW9uIGlzSG9zdFR5cGUob2JqZWN0LCBwcm9wZXJ0eSkge1xuICAgIHZhciB0eXBlID0gb2JqZWN0ICE9IG51bGwgPyB0eXBlb2Ygb2JqZWN0W3Byb3BlcnR5XSA6ICdudW1iZXInO1xuICAgIHJldHVybiAhL14oPzpib29sZWFufG51bWJlcnxzdHJpbmd8dW5kZWZpbmVkKSQvLnRlc3QodHlwZSkgJiZcbiAgICAgICh0eXBlID09ICdvYmplY3QnID8gISFvYmplY3RbcHJvcGVydHldIDogdHJ1ZSk7XG4gIH1cblxuICAvKipcbiAgICogQ2hlY2tzIGlmIGEgZ2l2ZW4gYHZhbHVlYCBpcyBhbiBvYmplY3QgY3JlYXRlZCBieSB0aGUgYE9iamVjdGAgY29uc3RydWN0b3JcbiAgICogYXNzdW1pbmcgb2JqZWN0cyBjcmVhdGVkIGJ5IHRoZSBgT2JqZWN0YCBjb25zdHJ1Y3RvciBoYXZlIG5vIGluaGVyaXRlZFxuICAgKiBlbnVtZXJhYmxlIHByb3BlcnRpZXMgYW5kIHRoYXQgdGhlcmUgYXJlIG5vIGBPYmplY3QucHJvdG90eXBlYCBleHRlbnNpb25zLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge01peGVkfSB2YWx1ZSBUaGUgdmFsdWUgdG8gY2hlY2suXG4gICAqIEByZXR1cm5zIHtCb29sZWFufSBSZXR1cm5zIGB0cnVlYCBpZiB0aGUgYHZhbHVlYCBpcyBhIHBsYWluIGBPYmplY3RgIG9iamVjdCwgZWxzZSBgZmFsc2VgLlxuICAgKi9cbiAgZnVuY3Rpb24gaXNQbGFpbk9iamVjdCh2YWx1ZSkge1xuICAgIC8vIGF2b2lkIG5vbi1vYmplY3RzIGFuZCBmYWxzZSBwb3NpdGl2ZXMgZm9yIGBhcmd1bWVudHNgIG9iamVjdHMgaW4gSUUgPCA5XG4gICAgdmFyIHJlc3VsdCA9IGZhbHNlO1xuICAgIGlmICghKHZhbHVlICYmIHR5cGVvZiB2YWx1ZSA9PSAnb2JqZWN0JykgfHwgKG5vQXJndW1lbnRzQ2xhc3MgJiYgaXNBcmd1bWVudHModmFsdWUpKSkge1xuICAgICAgcmV0dXJuIHJlc3VsdDtcbiAgICB9XG4gICAgLy8gSUUgPCA5IHByZXNlbnRzIERPTSBub2RlcyBhcyBgT2JqZWN0YCBvYmplY3RzIGV4Y2VwdCB0aGV5IGhhdmUgYHRvU3RyaW5nYFxuICAgIC8vIG1ldGhvZHMgdGhhdCBhcmUgYHR5cGVvZmAgXCJzdHJpbmdcIiBhbmQgc3RpbGwgY2FuIGNvZXJjZSBub2RlcyB0byBzdHJpbmdzLlxuICAgIC8vIEFsc28gY2hlY2sgdGhhdCB0aGUgY29uc3RydWN0b3IgaXMgYE9iamVjdGAgKGkuZS4gYE9iamVjdCBpbnN0YW5jZW9mIE9iamVjdGApXG4gICAgdmFyIGN0b3IgPSB2YWx1ZS5jb25zdHJ1Y3RvcjtcbiAgICBpZiAoKHN1cHBvcnQubm9kZUNsYXNzIHx8ICEodHlwZW9mIHZhbHVlLnRvU3RyaW5nICE9ICdmdW5jdGlvbicgJiYgdHlwZW9mICh2YWx1ZSArICcnKSA9PSAnc3RyaW5nJykpICYmXG4gICAgICAgICghaXNDbGFzc09mKGN0b3IsICdGdW5jdGlvbicpIHx8IGN0b3IgaW5zdGFuY2VvZiBjdG9yKSkge1xuICAgICAgLy8gSW4gbW9zdCBlbnZpcm9ubWVudHMgYW4gb2JqZWN0J3Mgb3duIHByb3BlcnRpZXMgYXJlIGl0ZXJhdGVkIGJlZm9yZVxuICAgICAgLy8gaXRzIGluaGVyaXRlZCBwcm9wZXJ0aWVzLiBJZiB0aGUgbGFzdCBpdGVyYXRlZCBwcm9wZXJ0eSBpcyBhbiBvYmplY3Qnc1xuICAgICAgLy8gb3duIHByb3BlcnR5IHRoZW4gdGhlcmUgYXJlIG5vIGluaGVyaXRlZCBlbnVtZXJhYmxlIHByb3BlcnRpZXMuXG4gICAgICBpZiAoc3VwcG9ydC5pdGVyYXRlc093bkZpcnN0KSB7XG4gICAgICAgIGZvclByb3BzKHZhbHVlLCBmdW5jdGlvbihzdWJWYWx1ZSwgc3ViS2V5KSB7XG4gICAgICAgICAgcmVzdWx0ID0gc3ViS2V5O1xuICAgICAgICB9KTtcbiAgICAgICAgcmV0dXJuIHJlc3VsdCA9PT0gZmFsc2UgfHwgaGFzS2V5KHZhbHVlLCByZXN1bHQpO1xuICAgICAgfVxuICAgICAgLy8gSUUgPCA5IGl0ZXJhdGVzIGluaGVyaXRlZCBwcm9wZXJ0aWVzIGJlZm9yZSBvd24gcHJvcGVydGllcy4gSWYgdGhlIGZpcnN0XG4gICAgICAvLyBpdGVyYXRlZCBwcm9wZXJ0eSBpcyBhbiBvYmplY3QncyBvd24gcHJvcGVydHkgdGhlbiB0aGVyZSBhcmUgbm8gaW5oZXJpdGVkXG4gICAgICAvLyBlbnVtZXJhYmxlIHByb3BlcnRpZXMuXG4gICAgICBmb3JQcm9wcyh2YWx1ZSwgZnVuY3Rpb24oc3ViVmFsdWUsIHN1YktleSkge1xuICAgICAgICByZXN1bHQgPSAhaGFzS2V5KHZhbHVlLCBzdWJLZXkpO1xuICAgICAgICByZXR1cm4gZmFsc2U7XG4gICAgICB9KTtcbiAgICAgIHJldHVybiByZXN1bHQgPT09IGZhbHNlO1xuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIENoZWNrcyBpZiBhIHZhbHVlIGNhbiBiZSBzYWZlbHkgY29lcmNlZCB0byBhIHN0cmluZy5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtNaXhlZH0gdmFsdWUgVGhlIHZhbHVlIHRvIGNoZWNrLlxuICAgKiBAcmV0dXJucyB7Qm9vbGVhbn0gUmV0dXJucyBgdHJ1ZWAgaWYgdGhlIHZhbHVlIGNhbiBiZSBjb2VyY2VkLCBlbHNlIGBmYWxzZWAuXG4gICAqL1xuICBmdW5jdGlvbiBpc1N0cmluZ2FibGUodmFsdWUpIHtcbiAgICByZXR1cm4gaGFzS2V5KHZhbHVlLCAndG9TdHJpbmcnKSB8fCBpc0NsYXNzT2YodmFsdWUsICdTdHJpbmcnKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBXcmFwcyBhIGZ1bmN0aW9uIGFuZCBwYXNzZXMgYHRoaXNgIHRvIHRoZSBvcmlnaW5hbCBmdW5jdGlvbiBhcyB0aGVcbiAgICogZmlyc3QgYXJndW1lbnQuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7RnVuY3Rpb259IGZuIFRoZSBmdW5jdGlvbiB0byBiZSB3cmFwcGVkLlxuICAgKiBAcmV0dXJucyB7RnVuY3Rpb259IFRoZSBuZXcgZnVuY3Rpb24uXG4gICAqL1xuICBmdW5jdGlvbiBtZXRob2RpemUoZm4pIHtcbiAgICByZXR1cm4gZnVuY3Rpb24oKSB7XG4gICAgICB2YXIgYXJncyA9IFt0aGlzXTtcbiAgICAgIGFyZ3MucHVzaC5hcHBseShhcmdzLCBhcmd1bWVudHMpO1xuICAgICAgcmV0dXJuIGZuLmFwcGx5KG51bGwsIGFyZ3MpO1xuICAgIH07XG4gIH1cblxuICAvKipcbiAgICogQSBuby1vcGVyYXRpb24gZnVuY3Rpb24uXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqL1xuICBmdW5jdGlvbiBub29wKCkge1xuICAgIC8vIG5vIG9wZXJhdGlvbiBwZXJmb3JtZWRcbiAgfVxuXG4gIC8qKlxuICAgKiBBIHdyYXBwZXIgYXJvdW5kIHJlcXVpcmUoKSB0byBzdXBwcmVzcyBgbW9kdWxlIG1pc3NpbmdgIGVycm9ycy5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtTdHJpbmd9IGlkIFRoZSBtb2R1bGUgaWQuXG4gICAqIEByZXR1cm5zIHtNaXhlZH0gVGhlIGV4cG9ydGVkIG1vZHVsZSBvciBgbnVsbGAuXG4gICAqL1xuICBmdW5jdGlvbiByZXEoaWQpIHtcbiAgICB0cnkge1xuICAgICAgdmFyIHJlc3VsdCA9IGZyZWVFeHBvcnRzICYmIGZyZWVSZXF1aXJlKGlkKTtcbiAgICB9IGNhdGNoKGUpIHsgfVxuICAgIHJldHVybiByZXN1bHQgfHwgbnVsbDtcbiAgfVxuXG4gIC8qKlxuICAgKiBSdW5zIGEgc25pcHBldCBvZiBKYXZhU2NyaXB0IHZpYSBzY3JpcHQgaW5qZWN0aW9uLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge1N0cmluZ30gY29kZSBUaGUgY29kZSB0byBydW4uXG4gICAqL1xuICBmdW5jdGlvbiBydW5TY3JpcHQoY29kZSkge1xuICAgIHZhciBhbmNob3IgPSBmcmVlRGVmaW5lID8gZGVmaW5lLmFtZCA6IEJlbmNobWFyayxcbiAgICAgICAgc2NyaXB0ID0gZG9jLmNyZWF0ZUVsZW1lbnQoJ3NjcmlwdCcpLFxuICAgICAgICBzaWJsaW5nID0gZG9jLmdldEVsZW1lbnRzQnlUYWdOYW1lKCdzY3JpcHQnKVswXSxcbiAgICAgICAgcGFyZW50ID0gc2libGluZy5wYXJlbnROb2RlLFxuICAgICAgICBwcm9wID0gdWlkICsgJ3J1blNjcmlwdCcsXG4gICAgICAgIHByZWZpeCA9ICcoJyArIChmcmVlRGVmaW5lID8gJ2RlZmluZS5hbWQuJyA6ICdCZW5jaG1hcmsuJykgKyBwcm9wICsgJ3x8ZnVuY3Rpb24oKXt9KSgpOyc7XG5cbiAgICAvLyBGaXJlZm94IDIuMC4wLjIgY2Fubm90IHVzZSBzY3JpcHQgaW5qZWN0aW9uIGFzIGludGVuZGVkIGJlY2F1c2UgaXQgZXhlY3V0ZXNcbiAgICAvLyBhc3luY2hyb25vdXNseSwgYnV0IHRoYXQncyBPSyBiZWNhdXNlIHNjcmlwdCBpbmplY3Rpb24gaXMgb25seSB1c2VkIHRvIGF2b2lkXG4gICAgLy8gdGhlIHByZXZpb3VzbHkgY29tbWVudGVkIEphZWdlck1vbmtleSBidWcuXG4gICAgdHJ5IHtcbiAgICAgIC8vIHJlbW92ZSB0aGUgaW5zZXJ0ZWQgc2NyaXB0ICpiZWZvcmUqIHJ1bm5pbmcgdGhlIGNvZGUgdG8gYXZvaWQgZGlmZmVyZW5jZXNcbiAgICAgIC8vIGluIHRoZSBleHBlY3RlZCBzY3JpcHQgZWxlbWVudCBjb3VudC9vcmRlciBvZiB0aGUgZG9jdW1lbnQuXG4gICAgICBzY3JpcHQuYXBwZW5kQ2hpbGQoZG9jLmNyZWF0ZVRleHROb2RlKHByZWZpeCArIGNvZGUpKTtcbiAgICAgIGFuY2hvcltwcm9wXSA9IGZ1bmN0aW9uKCkgeyBkZXN0cm95RWxlbWVudChzY3JpcHQpOyB9O1xuICAgIH0gY2F0Y2goZSkge1xuICAgICAgcGFyZW50ID0gcGFyZW50LmNsb25lTm9kZShmYWxzZSk7XG4gICAgICBzaWJsaW5nID0gbnVsbDtcbiAgICAgIHNjcmlwdC50ZXh0ID0gY29kZTtcbiAgICB9XG4gICAgcGFyZW50Lmluc2VydEJlZm9yZShzY3JpcHQsIHNpYmxpbmcpO1xuICAgIGRlbGV0ZSBhbmNob3JbcHJvcF07XG4gIH1cblxuICAvKipcbiAgICogQSBoZWxwZXIgZnVuY3Rpb24gZm9yIHNldHRpbmcgb3B0aW9ucy9ldmVudCBoYW5kbGVycy5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IGJlbmNoIFRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBbb3B0aW9ucz17fV0gT3B0aW9ucyBvYmplY3QuXG4gICAqL1xuICBmdW5jdGlvbiBzZXRPcHRpb25zKGJlbmNoLCBvcHRpb25zKSB7XG4gICAgb3B0aW9ucyA9IGV4dGVuZCh7fSwgYmVuY2guY29uc3RydWN0b3Iub3B0aW9ucywgb3B0aW9ucyk7XG4gICAgYmVuY2gub3B0aW9ucyA9IGZvck93bihvcHRpb25zLCBmdW5jdGlvbih2YWx1ZSwga2V5KSB7XG4gICAgICBpZiAodmFsdWUgIT0gbnVsbCkge1xuICAgICAgICAvLyBhZGQgZXZlbnQgbGlzdGVuZXJzXG4gICAgICAgIGlmICgvXm9uW0EtWl0vLnRlc3Qoa2V5KSkge1xuICAgICAgICAgIGZvckVhY2goa2V5LnNwbGl0KCcgJyksIGZ1bmN0aW9uKGtleSkge1xuICAgICAgICAgICAgYmVuY2gub24oa2V5LnNsaWNlKDIpLnRvTG93ZXJDYXNlKCksIHZhbHVlKTtcbiAgICAgICAgICB9KTtcbiAgICAgICAgfSBlbHNlIGlmICghaGFzS2V5KGJlbmNoLCBrZXkpKSB7XG4gICAgICAgICAgYmVuY2hba2V5XSA9IGRlZXBDbG9uZSh2YWx1ZSk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9KTtcbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBIYW5kbGVzIGN5Y2xpbmcvY29tcGxldGluZyB0aGUgZGVmZXJyZWQgYmVuY2htYXJrLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkRlZmVycmVkXG4gICAqL1xuICBmdW5jdGlvbiByZXNvbHZlKCkge1xuICAgIHZhciBtZSA9IHRoaXMsXG4gICAgICAgIGNsb25lID0gbWUuYmVuY2htYXJrLFxuICAgICAgICBiZW5jaCA9IGNsb25lLl9vcmlnaW5hbDtcblxuICAgIGlmIChiZW5jaC5hYm9ydGVkKSB7XG4gICAgICAvLyBjeWNsZSgpIC0+IGNsb25lIGN5Y2xlL2NvbXBsZXRlIGV2ZW50IC0+IGNvbXB1dGUoKSdzIGludm9rZWQgYmVuY2gucnVuKCkgY3ljbGUvY29tcGxldGVcbiAgICAgIG1lLnRlYXJkb3duKCk7XG4gICAgICBjbG9uZS5ydW5uaW5nID0gZmFsc2U7XG4gICAgICBjeWNsZShtZSk7XG4gICAgfVxuICAgIGVsc2UgaWYgKCsrbWUuY3ljbGVzIDwgY2xvbmUuY291bnQpIHtcbiAgICAgIC8vIGNvbnRpbnVlIHRoZSB0ZXN0IGxvb3BcbiAgICAgIGlmIChzdXBwb3J0LnRpbWVvdXQpIHtcbiAgICAgICAgLy8gdXNlIHNldFRpbWVvdXQgdG8gYXZvaWQgYSBjYWxsIHN0YWNrIG92ZXJmbG93IGlmIGNhbGxlZCByZWN1cnNpdmVseVxuICAgICAgICBzZXRUaW1lb3V0KGZ1bmN0aW9uKCkgeyBjbG9uZS5jb21waWxlZC5jYWxsKG1lLCB0aW1lcik7IH0sIDApO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgY2xvbmUuY29tcGlsZWQuY2FsbChtZSwgdGltZXIpO1xuICAgICAgfVxuICAgIH1cbiAgICBlbHNlIHtcbiAgICAgIHRpbWVyLnN0b3AobWUpO1xuICAgICAgbWUudGVhcmRvd24oKTtcbiAgICAgIGRlbGF5KGNsb25lLCBmdW5jdGlvbigpIHsgY3ljbGUobWUpOyB9KTtcbiAgICB9XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogQSBkZWVwIGNsb25lIHV0aWxpdHkuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge01peGVkfSB2YWx1ZSBUaGUgdmFsdWUgdG8gY2xvbmUuXG4gICAqIEByZXR1cm5zIHtNaXhlZH0gVGhlIGNsb25lZCB2YWx1ZS5cbiAgICovXG4gIGZ1bmN0aW9uIGRlZXBDbG9uZSh2YWx1ZSkge1xuICAgIHZhciBhY2Nlc3NvcixcbiAgICAgICAgY2lyY3VsYXIsXG4gICAgICAgIGNsb25lLFxuICAgICAgICBjdG9yLFxuICAgICAgICBkZXNjcmlwdG9yLFxuICAgICAgICBleHRlbnNpYmxlLFxuICAgICAgICBrZXksXG4gICAgICAgIGxlbmd0aCxcbiAgICAgICAgbWFya2VyS2V5LFxuICAgICAgICBwYXJlbnQsXG4gICAgICAgIHJlc3VsdCxcbiAgICAgICAgc291cmNlLFxuICAgICAgICBzdWJJbmRleCxcbiAgICAgICAgZGF0YSA9IHsgJ3ZhbHVlJzogdmFsdWUgfSxcbiAgICAgICAgaW5kZXggPSAwLFxuICAgICAgICBtYXJrZWQgPSBbXSxcbiAgICAgICAgcXVldWUgPSB7ICdsZW5ndGgnOiAwIH0sXG4gICAgICAgIHVubWFya2VkID0gW107XG5cbiAgICAvKipcbiAgICAgKiBBbiBlYXNpbHkgZGV0ZWN0YWJsZSBkZWNvcmF0b3IgZm9yIGNsb25lZCB2YWx1ZXMuXG4gICAgICovXG4gICAgZnVuY3Rpb24gTWFya2VyKG9iamVjdCkge1xuICAgICAgdGhpcy5yYXcgPSBvYmplY3Q7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogVGhlIGNhbGxiYWNrIHVzZWQgYnkgYGZvclByb3BzKClgLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIGZvclByb3BzQ2FsbGJhY2soc3ViVmFsdWUsIHN1YktleSkge1xuICAgICAgLy8gZXhpdCBlYXJseSB0byBhdm9pZCBjbG9uaW5nIHRoZSBtYXJrZXJcbiAgICAgIGlmIChzdWJWYWx1ZSAmJiBzdWJWYWx1ZS5jb25zdHJ1Y3RvciA9PSBNYXJrZXIpIHtcbiAgICAgICAgcmV0dXJuO1xuICAgICAgfVxuICAgICAgLy8gYWRkIG9iamVjdHMgdG8gdGhlIHF1ZXVlXG4gICAgICBpZiAoc3ViVmFsdWUgPT09IE9iamVjdChzdWJWYWx1ZSkpIHtcbiAgICAgICAgcXVldWVbcXVldWUubGVuZ3RoKytdID0geyAna2V5Jzogc3ViS2V5LCAncGFyZW50JzogY2xvbmUsICdzb3VyY2UnOiB2YWx1ZSB9O1xuICAgICAgfVxuICAgICAgLy8gYXNzaWduIG5vbi1vYmplY3RzXG4gICAgICBlbHNlIHtcbiAgICAgICAgdHJ5IHtcbiAgICAgICAgICAvLyB3aWxsIHRocm93IGFuIGVycm9yIGluIHN0cmljdCBtb2RlIGlmIHRoZSBwcm9wZXJ0eSBpcyByZWFkLW9ubHlcbiAgICAgICAgICBjbG9uZVtzdWJLZXldID0gc3ViVmFsdWU7XG4gICAgICAgIH0gY2F0Y2goZSkgeyB9XG4gICAgICB9XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogR2V0cyBhbiBhdmFpbGFibGUgbWFya2VyIGtleSBmb3IgdGhlIGdpdmVuIG9iamVjdC5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiBnZXRNYXJrZXJLZXkob2JqZWN0KSB7XG4gICAgICAvLyBhdm9pZCBjb2xsaXNpb25zIHdpdGggZXhpc3Rpbmcga2V5c1xuICAgICAgdmFyIHJlc3VsdCA9IHVpZDtcbiAgICAgIHdoaWxlIChvYmplY3RbcmVzdWx0XSAmJiBvYmplY3RbcmVzdWx0XS5jb25zdHJ1Y3RvciAhPSBNYXJrZXIpIHtcbiAgICAgICAgcmVzdWx0ICs9IDE7XG4gICAgICB9XG4gICAgICByZXR1cm4gcmVzdWx0O1xuICAgIH1cblxuICAgIGRvIHtcbiAgICAgIGtleSA9IGRhdGEua2V5O1xuICAgICAgcGFyZW50ID0gZGF0YS5wYXJlbnQ7XG4gICAgICBzb3VyY2UgPSBkYXRhLnNvdXJjZTtcbiAgICAgIGNsb25lID0gdmFsdWUgPSBzb3VyY2UgPyBzb3VyY2Vba2V5XSA6IGRhdGEudmFsdWU7XG4gICAgICBhY2Nlc3NvciA9IGNpcmN1bGFyID0gZGVzY3JpcHRvciA9IGZhbHNlO1xuXG4gICAgICAvLyBjcmVhdGUgYSBiYXNpYyBjbG9uZSB0byBmaWx0ZXIgb3V0IGZ1bmN0aW9ucywgRE9NIGVsZW1lbnRzLCBhbmRcbiAgICAgIC8vIG90aGVyIG5vbiBgT2JqZWN0YCBvYmplY3RzXG4gICAgICBpZiAodmFsdWUgPT09IE9iamVjdCh2YWx1ZSkpIHtcbiAgICAgICAgLy8gdXNlIGN1c3RvbSBkZWVwIGNsb25lIGZ1bmN0aW9uIGlmIGF2YWlsYWJsZVxuICAgICAgICBpZiAoaXNDbGFzc09mKHZhbHVlLmRlZXBDbG9uZSwgJ0Z1bmN0aW9uJykpIHtcbiAgICAgICAgICBjbG9uZSA9IHZhbHVlLmRlZXBDbG9uZSgpO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIGN0b3IgPSB2YWx1ZS5jb25zdHJ1Y3RvcjtcbiAgICAgICAgICBzd2l0Y2ggKHRvU3RyaW5nLmNhbGwodmFsdWUpKSB7XG4gICAgICAgICAgICBjYXNlICdbb2JqZWN0IEFycmF5XSc6XG4gICAgICAgICAgICAgIGNsb25lID0gbmV3IGN0b3IodmFsdWUubGVuZ3RoKTtcbiAgICAgICAgICAgICAgYnJlYWs7XG5cbiAgICAgICAgICAgIGNhc2UgJ1tvYmplY3QgQm9vbGVhbl0nOlxuICAgICAgICAgICAgICBjbG9uZSA9IG5ldyBjdG9yKHZhbHVlID09IHRydWUpO1xuICAgICAgICAgICAgICBicmVhaztcblxuICAgICAgICAgICAgY2FzZSAnW29iamVjdCBEYXRlXSc6XG4gICAgICAgICAgICAgIGNsb25lID0gbmV3IGN0b3IoK3ZhbHVlKTtcbiAgICAgICAgICAgICAgYnJlYWs7XG5cbiAgICAgICAgICAgIGNhc2UgJ1tvYmplY3QgT2JqZWN0XSc6XG4gICAgICAgICAgICAgIGlzUGxhaW5PYmplY3QodmFsdWUpICYmIChjbG9uZSA9IHt9KTtcbiAgICAgICAgICAgICAgYnJlYWs7XG5cbiAgICAgICAgICAgIGNhc2UgJ1tvYmplY3QgTnVtYmVyXSc6XG4gICAgICAgICAgICBjYXNlICdbb2JqZWN0IFN0cmluZ10nOlxuICAgICAgICAgICAgICBjbG9uZSA9IG5ldyBjdG9yKHZhbHVlKTtcbiAgICAgICAgICAgICAgYnJlYWs7XG5cbiAgICAgICAgICAgIGNhc2UgJ1tvYmplY3QgUmVnRXhwXSc6XG4gICAgICAgICAgICAgIGNsb25lID0gY3Rvcih2YWx1ZS5zb3VyY2UsXG4gICAgICAgICAgICAgICAgKHZhbHVlLmdsb2JhbCAgICAgPyAnZycgOiAnJykgK1xuICAgICAgICAgICAgICAgICh2YWx1ZS5pZ25vcmVDYXNlID8gJ2knIDogJycpICtcbiAgICAgICAgICAgICAgICAodmFsdWUubXVsdGlsaW5lICA/ICdtJyA6ICcnKSk7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICAgIC8vIGNvbnRpbnVlIGNsb25lIGlmIGB2YWx1ZWAgZG9lc24ndCBoYXZlIGFuIGFjY2Vzc29yIGRlc2NyaXB0b3JcbiAgICAgICAgLy8gaHR0cDovL2VzNS5naXRodWIuY29tLyN4OC4xMC4xXG4gICAgICAgIGlmIChjbG9uZSAmJiBjbG9uZSAhPSB2YWx1ZSAmJlxuICAgICAgICAgICAgIShkZXNjcmlwdG9yID0gc291cmNlICYmIHN1cHBvcnQuZGVzY3JpcHRvcnMgJiYgZ2V0RGVzY3JpcHRvcihzb3VyY2UsIGtleSksXG4gICAgICAgICAgICAgIGFjY2Vzc29yID0gZGVzY3JpcHRvciAmJiAoZGVzY3JpcHRvci5nZXQgfHwgZGVzY3JpcHRvci5zZXQpKSkge1xuICAgICAgICAgIC8vIHVzZSBhbiBleGlzdGluZyBjbG9uZSAoY2lyY3VsYXIgcmVmZXJlbmNlKVxuICAgICAgICAgIGlmICgoZXh0ZW5zaWJsZSA9IGlzRXh0ZW5zaWJsZSh2YWx1ZSkpKSB7XG4gICAgICAgICAgICBtYXJrZXJLZXkgPSBnZXRNYXJrZXJLZXkodmFsdWUpO1xuICAgICAgICAgICAgaWYgKHZhbHVlW21hcmtlcktleV0pIHtcbiAgICAgICAgICAgICAgY2lyY3VsYXIgPSBjbG9uZSA9IHZhbHVlW21hcmtlcktleV0ucmF3O1xuICAgICAgICAgICAgfVxuICAgICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgICAvLyBmb3IgZnJvemVuL3NlYWxlZCBvYmplY3RzXG4gICAgICAgICAgICBmb3IgKHN1YkluZGV4ID0gMCwgbGVuZ3RoID0gdW5tYXJrZWQubGVuZ3RoOyBzdWJJbmRleCA8IGxlbmd0aDsgc3ViSW5kZXgrKykge1xuICAgICAgICAgICAgICBkYXRhID0gdW5tYXJrZWRbc3ViSW5kZXhdO1xuICAgICAgICAgICAgICBpZiAoZGF0YS5vYmplY3QgPT09IHZhbHVlKSB7XG4gICAgICAgICAgICAgICAgY2lyY3VsYXIgPSBjbG9uZSA9IGRhdGEuY2xvbmU7XG4gICAgICAgICAgICAgICAgYnJlYWs7XG4gICAgICAgICAgICAgIH1cbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG4gICAgICAgICAgaWYgKCFjaXJjdWxhcikge1xuICAgICAgICAgICAgLy8gbWFyayBvYmplY3QgdG8gYWxsb3cgcXVpY2tseSBkZXRlY3RpbmcgY2lyY3VsYXIgcmVmZXJlbmNlcyBhbmQgdGllIGl0IHRvIGl0cyBjbG9uZVxuICAgICAgICAgICAgaWYgKGV4dGVuc2libGUpIHtcbiAgICAgICAgICAgICAgdmFsdWVbbWFya2VyS2V5XSA9IG5ldyBNYXJrZXIoY2xvbmUpO1xuICAgICAgICAgICAgICBtYXJrZWQucHVzaCh7ICdrZXknOiBtYXJrZXJLZXksICdvYmplY3QnOiB2YWx1ZSB9KTtcbiAgICAgICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgICAgIC8vIGZvciBmcm96ZW4vc2VhbGVkIG9iamVjdHNcbiAgICAgICAgICAgICAgdW5tYXJrZWQucHVzaCh7ICdjbG9uZSc6IGNsb25lLCAnb2JqZWN0JzogdmFsdWUgfSk7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgICAvLyBpdGVyYXRlIG92ZXIgb2JqZWN0IHByb3BlcnRpZXNcbiAgICAgICAgICAgIGZvclByb3BzKHZhbHVlLCBmb3JQcm9wc0NhbGxiYWNrLCB7ICd3aGljaCc6ICdhbGwnIH0pO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgaWYgKHBhcmVudCkge1xuICAgICAgICAvLyBmb3IgY3VzdG9tIHByb3BlcnR5IGRlc2NyaXB0b3JzXG4gICAgICAgIGlmIChhY2Nlc3NvciB8fCAoZGVzY3JpcHRvciAmJiAhKGRlc2NyaXB0b3IuY29uZmlndXJhYmxlICYmIGRlc2NyaXB0b3IuZW51bWVyYWJsZSAmJiBkZXNjcmlwdG9yLndyaXRhYmxlKSkpIHtcbiAgICAgICAgICBpZiAoJ3ZhbHVlJyBpbiBkZXNjcmlwdG9yKSB7XG4gICAgICAgICAgICBkZXNjcmlwdG9yLnZhbHVlID0gY2xvbmU7XG4gICAgICAgICAgfVxuICAgICAgICAgIHNldERlc2NyaXB0b3IocGFyZW50LCBrZXksIGRlc2NyaXB0b3IpO1xuICAgICAgICB9XG4gICAgICAgIC8vIGZvciBkZWZhdWx0IHByb3BlcnR5IGRlc2NyaXB0b3JzXG4gICAgICAgIGVsc2Uge1xuICAgICAgICAgIHBhcmVudFtrZXldID0gY2xvbmU7XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIHJlc3VsdCA9IGNsb25lO1xuICAgICAgfVxuICAgIH0gd2hpbGUgKChkYXRhID0gcXVldWVbaW5kZXgrK10pKTtcblxuICAgIC8vIHJlbW92ZSBtYXJrZXJzXG4gICAgZm9yIChpbmRleCA9IDAsIGxlbmd0aCA9IG1hcmtlZC5sZW5ndGg7IGluZGV4IDwgbGVuZ3RoOyBpbmRleCsrKSB7XG4gICAgICBkYXRhID0gbWFya2VkW2luZGV4XTtcbiAgICAgIGRlbGV0ZSBkYXRhLm9iamVjdFtkYXRhLmtleV07XG4gICAgfVxuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogQW4gaXRlcmF0aW9uIHV0aWxpdHkgZm9yIGFycmF5cyBhbmQgb2JqZWN0cy5cbiAgICogQ2FsbGJhY2tzIG1heSB0ZXJtaW5hdGUgdGhlIGxvb3AgYnkgZXhwbGljaXRseSByZXR1cm5pbmcgYGZhbHNlYC5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl8T2JqZWN0fSBvYmplY3QgVGhlIG9iamVjdCB0byBpdGVyYXRlIG92ZXIuXG4gICAqIEBwYXJhbSB7RnVuY3Rpb259IGNhbGxiYWNrIFRoZSBmdW5jdGlvbiBjYWxsZWQgcGVyIGl0ZXJhdGlvbi5cbiAgICogQHBhcmFtIHtNaXhlZH0gdGhpc0FyZyBUaGUgYHRoaXNgIGJpbmRpbmcgZm9yIHRoZSBjYWxsYmFjay5cbiAgICogQHJldHVybnMge0FycmF5fE9iamVjdH0gUmV0dXJucyB0aGUgb2JqZWN0IGl0ZXJhdGVkIG92ZXIuXG4gICAqL1xuICBmdW5jdGlvbiBlYWNoKG9iamVjdCwgY2FsbGJhY2ssIHRoaXNBcmcpIHtcbiAgICB2YXIgcmVzdWx0ID0gb2JqZWN0O1xuICAgIG9iamVjdCA9IE9iamVjdChvYmplY3QpO1xuXG4gICAgdmFyIGZuID0gY2FsbGJhY2ssXG4gICAgICAgIGluZGV4ID0gLTEsXG4gICAgICAgIGxlbmd0aCA9IG9iamVjdC5sZW5ndGgsXG4gICAgICAgIGlzU25hcHNob3QgPSAhIShvYmplY3Quc25hcHNob3RJdGVtICYmIChsZW5ndGggPSBvYmplY3Quc25hcHNob3RMZW5ndGgpKSxcbiAgICAgICAgaXNTcGxpdHRhYmxlID0gKG5vQ2hhckJ5SW5kZXggfHwgbm9DaGFyQnlPd25JbmRleCkgJiYgaXNDbGFzc09mKG9iamVjdCwgJ1N0cmluZycpLFxuICAgICAgICBpc0NvbnZlcnRhYmxlID0gaXNTbmFwc2hvdCB8fCBpc1NwbGl0dGFibGUgfHwgJ2l0ZW0nIGluIG9iamVjdCxcbiAgICAgICAgb3JpZ09iamVjdCA9IG9iamVjdDtcblxuICAgIC8vIGluIE9wZXJhIDwgMTAuNSBgaGFzS2V5KG9iamVjdCwgJ2xlbmd0aCcpYCByZXR1cm5zIGBmYWxzZWAgZm9yIE5vZGVMaXN0c1xuICAgIGlmIChsZW5ndGggPT09IGxlbmd0aCA+Pj4gMCkge1xuICAgICAgaWYgKGlzQ29udmVydGFibGUpIHtcbiAgICAgICAgLy8gdGhlIHRoaXJkIGFyZ3VtZW50IG9mIHRoZSBjYWxsYmFjayBpcyB0aGUgb3JpZ2luYWwgbm9uLWFycmF5IG9iamVjdFxuICAgICAgICBjYWxsYmFjayA9IGZ1bmN0aW9uKHZhbHVlLCBpbmRleCkge1xuICAgICAgICAgIHJldHVybiBmbi5jYWxsKHRoaXMsIHZhbHVlLCBpbmRleCwgb3JpZ09iamVjdCk7XG4gICAgICAgIH07XG4gICAgICAgIC8vIGluIElFIDwgOSBzdHJpbmdzIGRvbid0IHN1cHBvcnQgYWNjZXNzaW5nIGNoYXJhY3RlcnMgYnkgaW5kZXhcbiAgICAgICAgaWYgKGlzU3BsaXR0YWJsZSkge1xuICAgICAgICAgIG9iamVjdCA9IG9iamVjdC5zcGxpdCgnJyk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgb2JqZWN0ID0gW107XG4gICAgICAgICAgd2hpbGUgKCsraW5kZXggPCBsZW5ndGgpIHtcbiAgICAgICAgICAgIC8vIGluIFNhZmFyaSAyIGBpbmRleCBpbiBvYmplY3RgIGlzIGFsd2F5cyBgZmFsc2VgIGZvciBOb2RlTGlzdHNcbiAgICAgICAgICAgIG9iamVjdFtpbmRleF0gPSBpc1NuYXBzaG90ID8gcmVzdWx0LnNuYXBzaG90SXRlbShpbmRleCkgOiByZXN1bHRbaW5kZXhdO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgZm9yRWFjaChvYmplY3QsIGNhbGxiYWNrLCB0aGlzQXJnKTtcbiAgICB9IGVsc2Uge1xuICAgICAgZm9yT3duKG9iamVjdCwgY2FsbGJhY2ssIHRoaXNBcmcpO1xuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIENvcGllcyBlbnVtZXJhYmxlIHByb3BlcnRpZXMgZnJvbSB0aGUgc291cmNlKHMpIG9iamVjdCB0byB0aGUgZGVzdGluYXRpb24gb2JqZWN0LlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtPYmplY3R9IGRlc3RpbmF0aW9uIFRoZSBkZXN0aW5hdGlvbiBvYmplY3QuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBbc291cmNlPXt9XSBUaGUgc291cmNlIG9iamVjdC5cbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIGRlc3RpbmF0aW9uIG9iamVjdC5cbiAgICovXG4gIGZ1bmN0aW9uIGV4dGVuZChkZXN0aW5hdGlvbiwgc291cmNlKSB7XG4gICAgLy8gQ2hyb21lIDwgMTQgaW5jb3JyZWN0bHkgc2V0cyBgZGVzdGluYXRpb25gIHRvIGB1bmRlZmluZWRgIHdoZW4gd2UgYGRlbGV0ZSBhcmd1bWVudHNbMF1gXG4gICAgLy8gaHR0cDovL2NvZGUuZ29vZ2xlLmNvbS9wL3Y4L2lzc3Vlcy9kZXRhaWw/aWQ9ODM5XG4gICAgdmFyIHJlc3VsdCA9IGRlc3RpbmF0aW9uO1xuICAgIGRlbGV0ZSBhcmd1bWVudHNbMF07XG5cbiAgICBmb3JFYWNoKGFyZ3VtZW50cywgZnVuY3Rpb24oc291cmNlKSB7XG4gICAgICBmb3JQcm9wcyhzb3VyY2UsIGZ1bmN0aW9uKHZhbHVlLCBrZXkpIHtcbiAgICAgICAgcmVzdWx0W2tleV0gPSB2YWx1ZTtcbiAgICAgIH0pO1xuICAgIH0pO1xuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogQSBnZW5lcmljIGBBcnJheSNmaWx0ZXJgIGxpa2UgbWV0aG9kLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtBcnJheX0gYXJyYXkgVGhlIGFycmF5IHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtGdW5jdGlvbnxTdHJpbmd9IGNhbGxiYWNrIFRoZSBmdW5jdGlvbi9hbGlhcyBjYWxsZWQgcGVyIGl0ZXJhdGlvbi5cbiAgICogQHBhcmFtIHtNaXhlZH0gdGhpc0FyZyBUaGUgYHRoaXNgIGJpbmRpbmcgZm9yIHRoZSBjYWxsYmFjay5cbiAgICogQHJldHVybnMge0FycmF5fSBBIG5ldyBhcnJheSBvZiB2YWx1ZXMgdGhhdCBwYXNzZWQgY2FsbGJhY2sgZmlsdGVyLlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyBnZXQgb2RkIG51bWJlcnNcbiAgICogQmVuY2htYXJrLmZpbHRlcihbMSwgMiwgMywgNCwgNV0sIGZ1bmN0aW9uKG4pIHtcbiAgICogICByZXR1cm4gbiAlIDI7XG4gICAqIH0pOyAvLyAtPiBbMSwgMywgNV07XG4gICAqXG4gICAqIC8vIGdldCBmYXN0ZXN0IGJlbmNobWFya3NcbiAgICogQmVuY2htYXJrLmZpbHRlcihiZW5jaGVzLCAnZmFzdGVzdCcpO1xuICAgKlxuICAgKiAvLyBnZXQgc2xvd2VzdCBiZW5jaG1hcmtzXG4gICAqIEJlbmNobWFyay5maWx0ZXIoYmVuY2hlcywgJ3Nsb3dlc3QnKTtcbiAgICpcbiAgICogLy8gZ2V0IGJlbmNobWFya3MgdGhhdCBjb21wbGV0ZWQgd2l0aG91dCBlcnJvcmluZ1xuICAgKiBCZW5jaG1hcmsuZmlsdGVyKGJlbmNoZXMsICdzdWNjZXNzZnVsJyk7XG4gICAqL1xuICBmdW5jdGlvbiBmaWx0ZXIoYXJyYXksIGNhbGxiYWNrLCB0aGlzQXJnKSB7XG4gICAgdmFyIHJlc3VsdDtcblxuICAgIGlmIChjYWxsYmFjayA9PSAnc3VjY2Vzc2Z1bCcpIHtcbiAgICAgIC8vIGNhbGxiYWNrIHRvIGV4Y2x1ZGUgdGhvc2UgdGhhdCBhcmUgZXJyb3JlZCwgdW5ydW4sIG9yIGhhdmUgaHogb2YgSW5maW5pdHlcbiAgICAgIGNhbGxiYWNrID0gZnVuY3Rpb24oYmVuY2gpIHsgcmV0dXJuIGJlbmNoLmN5Y2xlcyAmJiBpc0Zpbml0ZShiZW5jaC5oeik7IH07XG4gICAgfVxuICAgIGVsc2UgaWYgKGNhbGxiYWNrID09ICdmYXN0ZXN0JyB8fCBjYWxsYmFjayA9PSAnc2xvd2VzdCcpIHtcbiAgICAgIC8vIGdldCBzdWNjZXNzZnVsLCBzb3J0IGJ5IHBlcmlvZCArIG1hcmdpbiBvZiBlcnJvciwgYW5kIGZpbHRlciBmYXN0ZXN0L3Nsb3dlc3RcbiAgICAgIHJlc3VsdCA9IGZpbHRlcihhcnJheSwgJ3N1Y2Nlc3NmdWwnKS5zb3J0KGZ1bmN0aW9uKGEsIGIpIHtcbiAgICAgICAgYSA9IGEuc3RhdHM7IGIgPSBiLnN0YXRzO1xuICAgICAgICByZXR1cm4gKGEubWVhbiArIGEubW9lID4gYi5tZWFuICsgYi5tb2UgPyAxIDogLTEpICogKGNhbGxiYWNrID09ICdmYXN0ZXN0JyA/IDEgOiAtMSk7XG4gICAgICB9KTtcbiAgICAgIHJlc3VsdCA9IGZpbHRlcihyZXN1bHQsIGZ1bmN0aW9uKGJlbmNoKSB7XG4gICAgICAgIHJldHVybiByZXN1bHRbMF0uY29tcGFyZShiZW5jaCkgPT0gMDtcbiAgICAgIH0pO1xuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0IHx8IHJlZHVjZShhcnJheSwgZnVuY3Rpb24ocmVzdWx0LCB2YWx1ZSwgaW5kZXgpIHtcbiAgICAgIHJldHVybiBjYWxsYmFjay5jYWxsKHRoaXNBcmcsIHZhbHVlLCBpbmRleCwgYXJyYXkpID8gKHJlc3VsdC5wdXNoKHZhbHVlKSwgcmVzdWx0KSA6IHJlc3VsdDtcbiAgICB9LCBbXSk7XG4gIH1cblxuICAvKipcbiAgICogQSBnZW5lcmljIGBBcnJheSNmb3JFYWNoYCBsaWtlIG1ldGhvZC5cbiAgICogQ2FsbGJhY2tzIG1heSB0ZXJtaW5hdGUgdGhlIGxvb3AgYnkgZXhwbGljaXRseSByZXR1cm5pbmcgYGZhbHNlYC5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl9IGFycmF5IFRoZSBhcnJheSB0byBpdGVyYXRlIG92ZXIuXG4gICAqIEBwYXJhbSB7RnVuY3Rpb259IGNhbGxiYWNrIFRoZSBmdW5jdGlvbiBjYWxsZWQgcGVyIGl0ZXJhdGlvbi5cbiAgICogQHBhcmFtIHtNaXhlZH0gdGhpc0FyZyBUaGUgYHRoaXNgIGJpbmRpbmcgZm9yIHRoZSBjYWxsYmFjay5cbiAgICogQHJldHVybnMge0FycmF5fSBSZXR1cm5zIHRoZSBhcnJheSBpdGVyYXRlZCBvdmVyLlxuICAgKi9cbiAgZnVuY3Rpb24gZm9yRWFjaChhcnJheSwgY2FsbGJhY2ssIHRoaXNBcmcpIHtcbiAgICB2YXIgaW5kZXggPSAtMSxcbiAgICAgICAgbGVuZ3RoID0gKGFycmF5ID0gT2JqZWN0KGFycmF5KSkubGVuZ3RoID4+PiAwO1xuXG4gICAgaWYgKHRoaXNBcmcgIT09IHVuZGVmaW5lZCkge1xuICAgICAgY2FsbGJhY2sgPSBiaW5kKGNhbGxiYWNrLCB0aGlzQXJnKTtcbiAgICB9XG4gICAgd2hpbGUgKCsraW5kZXggPCBsZW5ndGgpIHtcbiAgICAgIGlmIChpbmRleCBpbiBhcnJheSAmJlxuICAgICAgICAgIGNhbGxiYWNrKGFycmF5W2luZGV4XSwgaW5kZXgsIGFycmF5KSA9PT0gZmFsc2UpIHtcbiAgICAgICAgYnJlYWs7XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiBhcnJheTtcbiAgfVxuXG4gIC8qKlxuICAgKiBJdGVyYXRlcyBvdmVyIGFuIG9iamVjdCdzIG93biBwcm9wZXJ0aWVzLCBleGVjdXRpbmcgdGhlIGBjYWxsYmFja2AgZm9yIGVhY2guXG4gICAqIENhbGxiYWNrcyBtYXkgdGVybWluYXRlIHRoZSBsb29wIGJ5IGV4cGxpY2l0bHkgcmV0dXJuaW5nIGBmYWxzZWAuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge09iamVjdH0gb2JqZWN0IFRoZSBvYmplY3QgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gZXhlY3V0ZWQgcGVyIG93biBwcm9wZXJ0eS5cbiAgICogQHBhcmFtIHtNaXhlZH0gdGhpc0FyZyBUaGUgYHRoaXNgIGJpbmRpbmcgZm9yIHRoZSBjYWxsYmFjay5cbiAgICogQHJldHVybnMge09iamVjdH0gUmV0dXJucyB0aGUgb2JqZWN0IGl0ZXJhdGVkIG92ZXIuXG4gICAqL1xuICBmdW5jdGlvbiBmb3JPd24ob2JqZWN0LCBjYWxsYmFjaywgdGhpc0FyZykge1xuICAgIHJldHVybiBmb3JQcm9wcyhvYmplY3QsIGNhbGxiYWNrLCB7ICdiaW5kJzogdGhpc0FyZywgJ3doaWNoJzogJ293bicgfSk7XG4gIH1cblxuICAvKipcbiAgICogQ29udmVydHMgYSBudW1iZXIgdG8gYSBtb3JlIHJlYWRhYmxlIGNvbW1hLXNlcGFyYXRlZCBzdHJpbmcgcmVwcmVzZW50YXRpb24uXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge051bWJlcn0gbnVtYmVyIFRoZSBudW1iZXIgdG8gY29udmVydC5cbiAgICogQHJldHVybnMge1N0cmluZ30gVGhlIG1vcmUgcmVhZGFibGUgc3RyaW5nIHJlcHJlc2VudGF0aW9uLlxuICAgKi9cbiAgZnVuY3Rpb24gZm9ybWF0TnVtYmVyKG51bWJlcikge1xuICAgIG51bWJlciA9IFN0cmluZyhudW1iZXIpLnNwbGl0KCcuJyk7XG4gICAgcmV0dXJuIG51bWJlclswXS5yZXBsYWNlKC8oPz0oPzpcXGR7M30pKyQpKD8hXFxiKS9nLCAnLCcpICtcbiAgICAgIChudW1iZXJbMV0gPyAnLicgKyBudW1iZXJbMV0gOiAnJyk7XG4gIH1cblxuICAvKipcbiAgICogQ2hlY2tzIGlmIGFuIG9iamVjdCBoYXMgdGhlIHNwZWNpZmllZCBrZXkgYXMgYSBkaXJlY3QgcHJvcGVydHkuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge09iamVjdH0gb2JqZWN0IFRoZSBvYmplY3QgdG8gY2hlY2suXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBrZXkgVGhlIGtleSB0byBjaGVjayBmb3IuXG4gICAqIEByZXR1cm5zIHtCb29sZWFufSBSZXR1cm5zIGB0cnVlYCBpZiBrZXkgaXMgYSBkaXJlY3QgcHJvcGVydHksIGVsc2UgYGZhbHNlYC5cbiAgICovXG4gIGZ1bmN0aW9uIGhhc0tleSgpIHtcbiAgICAvLyBsYXp5IGRlZmluZSBmb3Igd29yc3QgY2FzZSBmYWxsYmFjayAobm90IGFzIGFjY3VyYXRlKVxuICAgIGhhc0tleSA9IGZ1bmN0aW9uKG9iamVjdCwga2V5KSB7XG4gICAgICB2YXIgcGFyZW50ID0gb2JqZWN0ICE9IG51bGwgJiYgKG9iamVjdC5jb25zdHJ1Y3RvciB8fCBPYmplY3QpLnByb3RvdHlwZTtcbiAgICAgIHJldHVybiAhIXBhcmVudCAmJiBrZXkgaW4gT2JqZWN0KG9iamVjdCkgJiYgIShrZXkgaW4gcGFyZW50ICYmIG9iamVjdFtrZXldID09PSBwYXJlbnRba2V5XSk7XG4gICAgfTtcbiAgICAvLyBmb3IgbW9kZXJuIGJyb3dzZXJzXG4gICAgaWYgKGlzQ2xhc3NPZihoYXNPd25Qcm9wZXJ0eSwgJ0Z1bmN0aW9uJykpIHtcbiAgICAgIGhhc0tleSA9IGZ1bmN0aW9uKG9iamVjdCwga2V5KSB7XG4gICAgICAgIHJldHVybiBvYmplY3QgIT0gbnVsbCAmJiBoYXNPd25Qcm9wZXJ0eS5jYWxsKG9iamVjdCwga2V5KTtcbiAgICAgIH07XG4gICAgfVxuICAgIC8vIGZvciBTYWZhcmkgMlxuICAgIGVsc2UgaWYgKHt9Ll9fcHJvdG9fXyA9PSBPYmplY3QucHJvdG90eXBlKSB7XG4gICAgICBoYXNLZXkgPSBmdW5jdGlvbihvYmplY3QsIGtleSkge1xuICAgICAgICB2YXIgcmVzdWx0ID0gZmFsc2U7XG4gICAgICAgIGlmIChvYmplY3QgIT0gbnVsbCkge1xuICAgICAgICAgIG9iamVjdCA9IE9iamVjdChvYmplY3QpO1xuICAgICAgICAgIG9iamVjdC5fX3Byb3RvX18gPSBbb2JqZWN0Ll9fcHJvdG9fXywgb2JqZWN0Ll9fcHJvdG9fXyA9IG51bGwsIHJlc3VsdCA9IGtleSBpbiBvYmplY3RdWzBdO1xuICAgICAgICB9XG4gICAgICAgIHJldHVybiByZXN1bHQ7XG4gICAgICB9O1xuICAgIH1cbiAgICByZXR1cm4gaGFzS2V5LmFwcGx5KHRoaXMsIGFyZ3VtZW50cyk7XG4gIH1cblxuICAvKipcbiAgICogQSBnZW5lcmljIGBBcnJheSNpbmRleE9mYCBsaWtlIG1ldGhvZC5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl9IGFycmF5IFRoZSBhcnJheSB0byBpdGVyYXRlIG92ZXIuXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHZhbHVlIFRoZSB2YWx1ZSB0byBzZWFyY2ggZm9yLlxuICAgKiBAcGFyYW0ge051bWJlcn0gW2Zyb21JbmRleD0wXSBUaGUgaW5kZXggdG8gc3RhcnQgc2VhcmNoaW5nIGZyb20uXG4gICAqIEByZXR1cm5zIHtOdW1iZXJ9IFRoZSBpbmRleCBvZiB0aGUgbWF0Y2hlZCB2YWx1ZSBvciBgLTFgLlxuICAgKi9cbiAgZnVuY3Rpb24gaW5kZXhPZihhcnJheSwgdmFsdWUsIGZyb21JbmRleCkge1xuICAgIHZhciBpbmRleCA9IHRvSW50ZWdlcihmcm9tSW5kZXgpLFxuICAgICAgICBsZW5ndGggPSAoYXJyYXkgPSBPYmplY3QoYXJyYXkpKS5sZW5ndGggPj4+IDA7XG5cbiAgICBpbmRleCA9IChpbmRleCA8IDAgPyBtYXgoMCwgbGVuZ3RoICsgaW5kZXgpIDogaW5kZXgpIC0gMTtcbiAgICB3aGlsZSAoKytpbmRleCA8IGxlbmd0aCkge1xuICAgICAgaWYgKGluZGV4IGluIGFycmF5ICYmIHZhbHVlID09PSBhcnJheVtpbmRleF0pIHtcbiAgICAgICAgcmV0dXJuIGluZGV4O1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gLTE7XG4gIH1cblxuICAvKipcbiAgICogTW9kaWZ5IGEgc3RyaW5nIGJ5IHJlcGxhY2luZyBuYW1lZCB0b2tlbnMgd2l0aCBtYXRjaGluZyBvYmplY3QgcHJvcGVydHkgdmFsdWVzLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtTdHJpbmd9IHN0cmluZyBUaGUgc3RyaW5nIHRvIG1vZGlmeS5cbiAgICogQHBhcmFtIHtPYmplY3R9IG9iamVjdCBUaGUgdGVtcGxhdGUgb2JqZWN0LlxuICAgKiBAcmV0dXJucyB7U3RyaW5nfSBUaGUgbW9kaWZpZWQgc3RyaW5nLlxuICAgKi9cbiAgZnVuY3Rpb24gaW50ZXJwb2xhdGUoc3RyaW5nLCBvYmplY3QpIHtcbiAgICBmb3JPd24ob2JqZWN0LCBmdW5jdGlvbih2YWx1ZSwga2V5KSB7XG4gICAgICAvLyBlc2NhcGUgcmVnZXhwIHNwZWNpYWwgY2hhcmFjdGVycyBpbiBga2V5YFxuICAgICAgc3RyaW5nID0gc3RyaW5nLnJlcGxhY2UoUmVnRXhwKCcjXFxcXHsnICsga2V5LnJlcGxhY2UoLyhbLiorP149IToke30oKXxbXFxdXFwvXFxcXF0pL2csICdcXFxcJDEnKSArICdcXFxcfScsICdnJyksIHZhbHVlKTtcbiAgICB9KTtcbiAgICByZXR1cm4gc3RyaW5nO1xuICB9XG5cbiAgLyoqXG4gICAqIEludm9rZXMgYSBtZXRob2Qgb24gYWxsIGl0ZW1zIGluIGFuIGFycmF5LlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtBcnJheX0gYmVuY2hlcyBBcnJheSBvZiBiZW5jaG1hcmtzIHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtTdHJpbmd8T2JqZWN0fSBuYW1lIFRoZSBuYW1lIG9mIHRoZSBtZXRob2QgdG8gaW52b2tlIE9SIG9wdGlvbnMgb2JqZWN0LlxuICAgKiBAcGFyYW0ge01peGVkfSBbYXJnMSwgYXJnMiwgLi4uXSBBcmd1bWVudHMgdG8gaW52b2tlIHRoZSBtZXRob2Qgd2l0aC5cbiAgICogQHJldHVybnMge0FycmF5fSBBIG5ldyBhcnJheSBvZiB2YWx1ZXMgcmV0dXJuZWQgZnJvbSBlYWNoIG1ldGhvZCBpbnZva2VkLlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyBpbnZva2UgYHJlc2V0YCBvbiBhbGwgYmVuY2htYXJrc1xuICAgKiBCZW5jaG1hcmsuaW52b2tlKGJlbmNoZXMsICdyZXNldCcpO1xuICAgKlxuICAgKiAvLyBpbnZva2UgYGVtaXRgIHdpdGggYXJndW1lbnRzXG4gICAqIEJlbmNobWFyay5pbnZva2UoYmVuY2hlcywgJ2VtaXQnLCAnY29tcGxldGUnLCBsaXN0ZW5lcik7XG4gICAqXG4gICAqIC8vIGludm9rZSBgcnVuKHRydWUpYCwgdHJlYXQgYmVuY2htYXJrcyBhcyBhIHF1ZXVlLCBhbmQgcmVnaXN0ZXIgaW52b2tlIGNhbGxiYWNrc1xuICAgKiBCZW5jaG1hcmsuaW52b2tlKGJlbmNoZXMsIHtcbiAgICpcbiAgICogICAvLyBpbnZva2UgdGhlIGBydW5gIG1ldGhvZFxuICAgKiAgICduYW1lJzogJ3J1bicsXG4gICAqXG4gICAqICAgLy8gcGFzcyBhIHNpbmdsZSBhcmd1bWVudFxuICAgKiAgICdhcmdzJzogdHJ1ZSxcbiAgICpcbiAgICogICAvLyB0cmVhdCBhcyBxdWV1ZSwgcmVtb3ZpbmcgYmVuY2htYXJrcyBmcm9tIGZyb250IG9mIGBiZW5jaGVzYCB1bnRpbCBlbXB0eVxuICAgKiAgICdxdWV1ZWQnOiB0cnVlLFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCBiZWZvcmUgYW55IGJlbmNobWFya3MgaGF2ZSBiZWVuIGludm9rZWQuXG4gICAqICAgJ29uU3RhcnQnOiBvblN0YXJ0LFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCBiZXR3ZWVuIGludm9raW5nIGJlbmNobWFya3NcbiAgICogICAnb25DeWNsZSc6IG9uQ3ljbGUsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIGFmdGVyIGFsbCBiZW5jaG1hcmtzIGhhdmUgYmVlbiBpbnZva2VkLlxuICAgKiAgICdvbkNvbXBsZXRlJzogb25Db21wbGV0ZVxuICAgKiB9KTtcbiAgICovXG4gIGZ1bmN0aW9uIGludm9rZShiZW5jaGVzLCBuYW1lKSB7XG4gICAgdmFyIGFyZ3MsXG4gICAgICAgIGJlbmNoLFxuICAgICAgICBxdWV1ZWQsXG4gICAgICAgIGluZGV4ID0gLTEsXG4gICAgICAgIGV2ZW50UHJvcHMgPSB7ICdjdXJyZW50VGFyZ2V0JzogYmVuY2hlcyB9LFxuICAgICAgICBvcHRpb25zID0geyAnb25TdGFydCc6IG5vb3AsICdvbkN5Y2xlJzogbm9vcCwgJ29uQ29tcGxldGUnOiBub29wIH0sXG4gICAgICAgIHJlc3VsdCA9IG1hcChiZW5jaGVzLCBmdW5jdGlvbihiZW5jaCkgeyByZXR1cm4gYmVuY2g7IH0pO1xuXG4gICAgLyoqXG4gICAgICogSW52b2tlcyB0aGUgbWV0aG9kIG9mIHRoZSBjdXJyZW50IG9iamVjdCBhbmQgaWYgc3luY2hyb25vdXMsIGZldGNoZXMgdGhlIG5leHQuXG4gICAgICovXG4gICAgZnVuY3Rpb24gZXhlY3V0ZSgpIHtcbiAgICAgIHZhciBsaXN0ZW5lcnMsXG4gICAgICAgICAgYXN5bmMgPSBpc0FzeW5jKGJlbmNoKTtcblxuICAgICAgaWYgKGFzeW5jKSB7XG4gICAgICAgIC8vIHVzZSBgZ2V0TmV4dGAgYXMgdGhlIGZpcnN0IGxpc3RlbmVyXG4gICAgICAgIGJlbmNoLm9uKCdjb21wbGV0ZScsIGdldE5leHQpO1xuICAgICAgICBsaXN0ZW5lcnMgPSBiZW5jaC5ldmVudHMuY29tcGxldGU7XG4gICAgICAgIGxpc3RlbmVycy5zcGxpY2UoMCwgMCwgbGlzdGVuZXJzLnBvcCgpKTtcbiAgICAgIH1cbiAgICAgIC8vIGV4ZWN1dGUgbWV0aG9kXG4gICAgICByZXN1bHRbaW5kZXhdID0gaXNDbGFzc09mKGJlbmNoICYmIGJlbmNoW25hbWVdLCAnRnVuY3Rpb24nKSA/IGJlbmNoW25hbWVdLmFwcGx5KGJlbmNoLCBhcmdzKSA6IHVuZGVmaW5lZDtcbiAgICAgIC8vIGlmIHN5bmNocm9ub3VzIHJldHVybiB0cnVlIHVudGlsIGZpbmlzaGVkXG4gICAgICByZXR1cm4gIWFzeW5jICYmIGdldE5leHQoKTtcbiAgICB9XG5cbiAgICAvKipcbiAgICAgKiBGZXRjaGVzIHRoZSBuZXh0IGJlbmNoIG9yIGV4ZWN1dGVzIGBvbkNvbXBsZXRlYCBjYWxsYmFjay5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiBnZXROZXh0KGV2ZW50KSB7XG4gICAgICB2YXIgY3ljbGVFdmVudCxcbiAgICAgICAgICBsYXN0ID0gYmVuY2gsXG4gICAgICAgICAgYXN5bmMgPSBpc0FzeW5jKGxhc3QpO1xuXG4gICAgICBpZiAoYXN5bmMpIHtcbiAgICAgICAgbGFzdC5vZmYoJ2NvbXBsZXRlJywgZ2V0TmV4dCk7XG4gICAgICAgIGxhc3QuZW1pdCgnY29tcGxldGUnKTtcbiAgICAgIH1cbiAgICAgIC8vIGVtaXQgXCJjeWNsZVwiIGV2ZW50XG4gICAgICBldmVudFByb3BzLnR5cGUgPSAnY3ljbGUnO1xuICAgICAgZXZlbnRQcm9wcy50YXJnZXQgPSBsYXN0O1xuICAgICAgY3ljbGVFdmVudCA9IEV2ZW50KGV2ZW50UHJvcHMpO1xuICAgICAgb3B0aW9ucy5vbkN5Y2xlLmNhbGwoYmVuY2hlcywgY3ljbGVFdmVudCk7XG5cbiAgICAgIC8vIGNob29zZSBuZXh0IGJlbmNobWFyayBpZiBub3QgZXhpdGluZyBlYXJseVxuICAgICAgaWYgKCFjeWNsZUV2ZW50LmFib3J0ZWQgJiYgcmFpc2VJbmRleCgpICE9PSBmYWxzZSkge1xuICAgICAgICBiZW5jaCA9IHF1ZXVlZCA/IGJlbmNoZXNbMF0gOiByZXN1bHRbaW5kZXhdO1xuICAgICAgICBpZiAoaXNBc3luYyhiZW5jaCkpIHtcbiAgICAgICAgICBkZWxheShiZW5jaCwgZXhlY3V0ZSk7XG4gICAgICAgIH1cbiAgICAgICAgZWxzZSBpZiAoYXN5bmMpIHtcbiAgICAgICAgICAvLyByZXN1bWUgZXhlY3V0aW9uIGlmIHByZXZpb3VzbHkgYXN5bmNocm9ub3VzIGJ1dCBub3cgc3luY2hyb25vdXNcbiAgICAgICAgICB3aGlsZSAoZXhlY3V0ZSgpKSB7IH1cbiAgICAgICAgfVxuICAgICAgICBlbHNlIHtcbiAgICAgICAgICAvLyBjb250aW51ZSBzeW5jaHJvbm91cyBleGVjdXRpb25cbiAgICAgICAgICByZXR1cm4gdHJ1ZTtcbiAgICAgICAgfVxuICAgICAgfSBlbHNlIHtcbiAgICAgICAgLy8gZW1pdCBcImNvbXBsZXRlXCIgZXZlbnRcbiAgICAgICAgZXZlbnRQcm9wcy50eXBlID0gJ2NvbXBsZXRlJztcbiAgICAgICAgb3B0aW9ucy5vbkNvbXBsZXRlLmNhbGwoYmVuY2hlcywgRXZlbnQoZXZlbnRQcm9wcykpO1xuICAgICAgfVxuICAgICAgLy8gV2hlbiB1c2VkIGFzIGEgbGlzdGVuZXIgYGV2ZW50LmFib3J0ZWQgPSB0cnVlYCB3aWxsIGNhbmNlbCB0aGUgcmVzdCBvZlxuICAgICAgLy8gdGhlIFwiY29tcGxldGVcIiBsaXN0ZW5lcnMgYmVjYXVzZSB0aGV5IHdlcmUgYWxyZWFkeSBjYWxsZWQgYWJvdmUgYW5kIHdoZW5cbiAgICAgIC8vIHVzZWQgYXMgcGFydCBvZiBgZ2V0TmV4dGAgdGhlIGByZXR1cm4gZmFsc2VgIHdpbGwgZXhpdCB0aGUgZXhlY3V0aW9uIHdoaWxlLWxvb3AuXG4gICAgICBpZiAoZXZlbnQpIHtcbiAgICAgICAgZXZlbnQuYWJvcnRlZCA9IHRydWU7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICByZXR1cm4gZmFsc2U7XG4gICAgICB9XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogQ2hlY2tzIGlmIGludm9raW5nIGBCZW5jaG1hcmsjcnVuYCB3aXRoIGFzeW5jaHJvbm91cyBjeWNsZXMuXG4gICAgICovXG4gICAgZnVuY3Rpb24gaXNBc3luYyhvYmplY3QpIHtcbiAgICAgIC8vIGF2b2lkIHVzaW5nIGBpbnN0YW5jZW9mYCBoZXJlIGJlY2F1c2Ugb2YgSUUgbWVtb3J5IGxlYWsgaXNzdWVzIHdpdGggaG9zdCBvYmplY3RzXG4gICAgICB2YXIgYXN5bmMgPSBhcmdzWzBdICYmIGFyZ3NbMF0uYXN5bmM7XG4gICAgICByZXR1cm4gT2JqZWN0KG9iamVjdCkuY29uc3RydWN0b3IgPT0gQmVuY2htYXJrICYmIG5hbWUgPT0gJ3J1bicgJiZcbiAgICAgICAgKChhc3luYyA9PSBudWxsID8gb2JqZWN0Lm9wdGlvbnMuYXN5bmMgOiBhc3luYykgJiYgc3VwcG9ydC50aW1lb3V0IHx8IG9iamVjdC5kZWZlcik7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogUmFpc2VzIGBpbmRleGAgdG8gdGhlIG5leHQgZGVmaW5lZCBpbmRleCBvciByZXR1cm5zIGBmYWxzZWAuXG4gICAgICovXG4gICAgZnVuY3Rpb24gcmFpc2VJbmRleCgpIHtcbiAgICAgIHZhciBsZW5ndGggPSByZXN1bHQubGVuZ3RoO1xuICAgICAgaWYgKHF1ZXVlZCkge1xuICAgICAgICAvLyBpZiBxdWV1ZWQgcmVtb3ZlIHRoZSBwcmV2aW91cyBiZW5jaCBhbmQgc3Vic2VxdWVudCBza2lwcGVkIG5vbi1lbnRyaWVzXG4gICAgICAgIGRvIHtcbiAgICAgICAgICArK2luZGV4ID4gMCAmJiBzaGlmdC5jYWxsKGJlbmNoZXMpO1xuICAgICAgICB9IHdoaWxlICgobGVuZ3RoID0gYmVuY2hlcy5sZW5ndGgpICYmICEoJzAnIGluIGJlbmNoZXMpKTtcbiAgICAgIH1cbiAgICAgIGVsc2Uge1xuICAgICAgICB3aGlsZSAoKytpbmRleCA8IGxlbmd0aCAmJiAhKGluZGV4IGluIHJlc3VsdCkpIHsgfVxuICAgICAgfVxuICAgICAgLy8gaWYgd2UgcmVhY2hlZCB0aGUgbGFzdCBpbmRleCB0aGVuIHJldHVybiBgZmFsc2VgXG4gICAgICByZXR1cm4gKHF1ZXVlZCA/IGxlbmd0aCA6IGluZGV4IDwgbGVuZ3RoKSA/IGluZGV4IDogKGluZGV4ID0gZmFsc2UpO1xuICAgIH1cblxuICAgIC8vIGp1Z2dsZSBhcmd1bWVudHNcbiAgICBpZiAoaXNDbGFzc09mKG5hbWUsICdTdHJpbmcnKSkge1xuICAgICAgLy8gMiBhcmd1bWVudHMgKGFycmF5LCBuYW1lKVxuICAgICAgYXJncyA9IHNsaWNlLmNhbGwoYXJndW1lbnRzLCAyKTtcbiAgICB9IGVsc2Uge1xuICAgICAgLy8gMiBhcmd1bWVudHMgKGFycmF5LCBvcHRpb25zKVxuICAgICAgb3B0aW9ucyA9IGV4dGVuZChvcHRpb25zLCBuYW1lKTtcbiAgICAgIG5hbWUgPSBvcHRpb25zLm5hbWU7XG4gICAgICBhcmdzID0gaXNDbGFzc09mKGFyZ3MgPSAnYXJncycgaW4gb3B0aW9ucyA/IG9wdGlvbnMuYXJncyA6IFtdLCAnQXJyYXknKSA/IGFyZ3MgOiBbYXJnc107XG4gICAgICBxdWV1ZWQgPSBvcHRpb25zLnF1ZXVlZDtcbiAgICB9XG5cbiAgICAvLyBzdGFydCBpdGVyYXRpbmcgb3ZlciB0aGUgYXJyYXlcbiAgICBpZiAocmFpc2VJbmRleCgpICE9PSBmYWxzZSkge1xuICAgICAgLy8gZW1pdCBcInN0YXJ0XCIgZXZlbnRcbiAgICAgIGJlbmNoID0gcmVzdWx0W2luZGV4XTtcbiAgICAgIGV2ZW50UHJvcHMudHlwZSA9ICdzdGFydCc7XG4gICAgICBldmVudFByb3BzLnRhcmdldCA9IGJlbmNoO1xuICAgICAgb3B0aW9ucy5vblN0YXJ0LmNhbGwoYmVuY2hlcywgRXZlbnQoZXZlbnRQcm9wcykpO1xuXG4gICAgICAvLyBlbmQgZWFybHkgaWYgdGhlIHN1aXRlIHdhcyBhYm9ydGVkIGluIGFuIFwib25TdGFydFwiIGxpc3RlbmVyXG4gICAgICBpZiAoYmVuY2hlcy5hYm9ydGVkICYmIGJlbmNoZXMuY29uc3RydWN0b3IgPT0gU3VpdGUgJiYgbmFtZSA9PSAncnVuJykge1xuICAgICAgICAvLyBlbWl0IFwiY3ljbGVcIiBldmVudFxuICAgICAgICBldmVudFByb3BzLnR5cGUgPSAnY3ljbGUnO1xuICAgICAgICBvcHRpb25zLm9uQ3ljbGUuY2FsbChiZW5jaGVzLCBFdmVudChldmVudFByb3BzKSk7XG4gICAgICAgIC8vIGVtaXQgXCJjb21wbGV0ZVwiIGV2ZW50XG4gICAgICAgIGV2ZW50UHJvcHMudHlwZSA9ICdjb21wbGV0ZSc7XG4gICAgICAgIG9wdGlvbnMub25Db21wbGV0ZS5jYWxsKGJlbmNoZXMsIEV2ZW50KGV2ZW50UHJvcHMpKTtcbiAgICAgIH1cbiAgICAgIC8vIGVsc2Ugc3RhcnRcbiAgICAgIGVsc2Uge1xuICAgICAgICBpZiAoaXNBc3luYyhiZW5jaCkpIHtcbiAgICAgICAgICBkZWxheShiZW5jaCwgZXhlY3V0ZSk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgd2hpbGUgKGV4ZWN1dGUoKSkgeyB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBDcmVhdGVzIGEgc3RyaW5nIG9mIGpvaW5lZCBhcnJheSB2YWx1ZXMgb3Igb2JqZWN0IGtleS12YWx1ZSBwYWlycy5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl8T2JqZWN0fSBvYmplY3QgVGhlIG9iamVjdCB0byBvcGVyYXRlIG9uLlxuICAgKiBAcGFyYW0ge1N0cmluZ30gW3NlcGFyYXRvcjE9JywnXSBUaGUgc2VwYXJhdG9yIHVzZWQgYmV0d2VlbiBrZXktdmFsdWUgcGFpcnMuXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBbc2VwYXJhdG9yMj0nOiAnXSBUaGUgc2VwYXJhdG9yIHVzZWQgYmV0d2VlbiBrZXlzIGFuZCB2YWx1ZXMuXG4gICAqIEByZXR1cm5zIHtTdHJpbmd9IFRoZSBqb2luZWQgcmVzdWx0LlxuICAgKi9cbiAgZnVuY3Rpb24gam9pbihvYmplY3QsIHNlcGFyYXRvcjEsIHNlcGFyYXRvcjIpIHtcbiAgICB2YXIgcmVzdWx0ID0gW10sXG4gICAgICAgIGxlbmd0aCA9IChvYmplY3QgPSBPYmplY3Qob2JqZWN0KSkubGVuZ3RoLFxuICAgICAgICBhcnJheUxpa2UgPSBsZW5ndGggPT09IGxlbmd0aCA+Pj4gMDtcblxuICAgIHNlcGFyYXRvcjIgfHwgKHNlcGFyYXRvcjIgPSAnOiAnKTtcbiAgICBlYWNoKG9iamVjdCwgZnVuY3Rpb24odmFsdWUsIGtleSkge1xuICAgICAgcmVzdWx0LnB1c2goYXJyYXlMaWtlID8gdmFsdWUgOiBrZXkgKyBzZXBhcmF0b3IyICsgdmFsdWUpO1xuICAgIH0pO1xuICAgIHJldHVybiByZXN1bHQuam9pbihzZXBhcmF0b3IxIHx8ICcsJyk7XG4gIH1cblxuICAvKipcbiAgICogQSBnZW5lcmljIGBBcnJheSNtYXBgIGxpa2UgbWV0aG9kLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtBcnJheX0gYXJyYXkgVGhlIGFycmF5IHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uIGNhbGxlZCBwZXIgaXRlcmF0aW9uLlxuICAgKiBAcGFyYW0ge01peGVkfSB0aGlzQXJnIFRoZSBgdGhpc2AgYmluZGluZyBmb3IgdGhlIGNhbGxiYWNrLlxuICAgKiBAcmV0dXJucyB7QXJyYXl9IEEgbmV3IGFycmF5IG9mIHZhbHVlcyByZXR1cm5lZCBieSB0aGUgY2FsbGJhY2suXG4gICAqL1xuICBmdW5jdGlvbiBtYXAoYXJyYXksIGNhbGxiYWNrLCB0aGlzQXJnKSB7XG4gICAgcmV0dXJuIHJlZHVjZShhcnJheSwgZnVuY3Rpb24ocmVzdWx0LCB2YWx1ZSwgaW5kZXgpIHtcbiAgICAgIHJlc3VsdFtpbmRleF0gPSBjYWxsYmFjay5jYWxsKHRoaXNBcmcsIHZhbHVlLCBpbmRleCwgYXJyYXkpO1xuICAgICAgcmV0dXJuIHJlc3VsdDtcbiAgICB9LCBBcnJheShPYmplY3QoYXJyYXkpLmxlbmd0aCA+Pj4gMCkpO1xuICB9XG5cbiAgLyoqXG4gICAqIFJldHJpZXZlcyB0aGUgdmFsdWUgb2YgYSBzcGVjaWZpZWQgcHJvcGVydHkgZnJvbSBhbGwgaXRlbXMgaW4gYW4gYXJyYXkuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge0FycmF5fSBhcnJheSBUaGUgYXJyYXkgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge1N0cmluZ30gcHJvcGVydHkgVGhlIHByb3BlcnR5IHRvIHBsdWNrLlxuICAgKiBAcmV0dXJucyB7QXJyYXl9IEEgbmV3IGFycmF5IG9mIHByb3BlcnR5IHZhbHVlcy5cbiAgICovXG4gIGZ1bmN0aW9uIHBsdWNrKGFycmF5LCBwcm9wZXJ0eSkge1xuICAgIHJldHVybiBtYXAoYXJyYXksIGZ1bmN0aW9uKG9iamVjdCkge1xuICAgICAgcmV0dXJuIG9iamVjdCA9PSBudWxsID8gdW5kZWZpbmVkIDogb2JqZWN0W3Byb3BlcnR5XTtcbiAgICB9KTtcbiAgfVxuXG4gIC8qKlxuICAgKiBBIGdlbmVyaWMgYEFycmF5I3JlZHVjZWAgbGlrZSBtZXRob2QuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge0FycmF5fSBhcnJheSBUaGUgYXJyYXkgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAqIEBwYXJhbSB7TWl4ZWR9IGFjY3VtdWxhdG9yIEluaXRpYWwgdmFsdWUgb2YgdGhlIGFjY3VtdWxhdG9yLlxuICAgKiBAcmV0dXJucyB7TWl4ZWR9IFRoZSBhY2N1bXVsYXRvci5cbiAgICovXG4gIGZ1bmN0aW9uIHJlZHVjZShhcnJheSwgY2FsbGJhY2ssIGFjY3VtdWxhdG9yKSB7XG4gICAgdmFyIG5vYWNjdW0gPSBhcmd1bWVudHMubGVuZ3RoIDwgMztcbiAgICBmb3JFYWNoKGFycmF5LCBmdW5jdGlvbih2YWx1ZSwgaW5kZXgpIHtcbiAgICAgIGFjY3VtdWxhdG9yID0gbm9hY2N1bSA/IChub2FjY3VtID0gZmFsc2UsIHZhbHVlKSA6IGNhbGxiYWNrKGFjY3VtdWxhdG9yLCB2YWx1ZSwgaW5kZXgsIGFycmF5KTtcbiAgICB9KTtcbiAgICByZXR1cm4gYWNjdW11bGF0b3I7XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogQWJvcnRzIGFsbCBiZW5jaG1hcmtzIGluIHRoZSBzdWl0ZS5cbiAgICpcbiAgICogQG5hbWUgYWJvcnRcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgc3VpdGUgaW5zdGFuY2UuXG4gICAqL1xuICBmdW5jdGlvbiBhYm9ydFN1aXRlKCkge1xuICAgIHZhciBldmVudCxcbiAgICAgICAgbWUgPSB0aGlzLFxuICAgICAgICByZXNldHRpbmcgPSBjYWxsZWRCeS5yZXNldFN1aXRlO1xuXG4gICAgaWYgKG1lLnJ1bm5pbmcpIHtcbiAgICAgIGV2ZW50ID0gRXZlbnQoJ2Fib3J0Jyk7XG4gICAgICBtZS5lbWl0KGV2ZW50KTtcbiAgICAgIGlmICghZXZlbnQuY2FuY2VsbGVkIHx8IHJlc2V0dGluZykge1xuICAgICAgICAvLyBhdm9pZCBpbmZpbml0ZSByZWN1cnNpb25cbiAgICAgICAgY2FsbGVkQnkuYWJvcnRTdWl0ZSA9IHRydWU7XG4gICAgICAgIG1lLnJlc2V0KCk7XG4gICAgICAgIGRlbGV0ZSBjYWxsZWRCeS5hYm9ydFN1aXRlO1xuXG4gICAgICAgIGlmICghcmVzZXR0aW5nKSB7XG4gICAgICAgICAgbWUuYWJvcnRlZCA9IHRydWU7XG4gICAgICAgICAgaW52b2tlKG1lLCAnYWJvcnQnKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gbWU7XG4gIH1cblxuICAvKipcbiAgICogQWRkcyBhIHRlc3QgdG8gdGhlIGJlbmNobWFyayBzdWl0ZS5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge1N0cmluZ30gbmFtZSBBIG5hbWUgdG8gaWRlbnRpZnkgdGhlIGJlbmNobWFyay5cbiAgICogQHBhcmFtIHtGdW5jdGlvbnxTdHJpbmd9IGZuIFRoZSB0ZXN0IHRvIGJlbmNobWFyay5cbiAgICogQHBhcmFtIHtPYmplY3R9IFtvcHRpb25zPXt9XSBPcHRpb25zIG9iamVjdC5cbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gYmFzaWMgdXNhZ2VcbiAgICogc3VpdGUuYWRkKGZuKTtcbiAgICpcbiAgICogLy8gb3IgdXNpbmcgYSBuYW1lIGZpcnN0XG4gICAqIHN1aXRlLmFkZCgnZm9vJywgZm4pO1xuICAgKlxuICAgKiAvLyBvciB3aXRoIG9wdGlvbnNcbiAgICogc3VpdGUuYWRkKCdmb28nLCBmbiwge1xuICAgKiAgICdvbkN5Y2xlJzogb25DeWNsZSxcbiAgICogICAnb25Db21wbGV0ZSc6IG9uQ29tcGxldGVcbiAgICogfSk7XG4gICAqXG4gICAqIC8vIG9yIG5hbWUgYW5kIG9wdGlvbnNcbiAgICogc3VpdGUuYWRkKCdmb28nLCB7XG4gICAqICAgJ2ZuJzogZm4sXG4gICAqICAgJ29uQ3ljbGUnOiBvbkN5Y2xlLFxuICAgKiAgICdvbkNvbXBsZXRlJzogb25Db21wbGV0ZVxuICAgKiB9KTtcbiAgICpcbiAgICogLy8gb3Igb3B0aW9ucyBvbmx5XG4gICAqIHN1aXRlLmFkZCh7XG4gICAqICAgJ25hbWUnOiAnZm9vJyxcbiAgICogICAnZm4nOiBmbixcbiAgICogICAnb25DeWNsZSc6IG9uQ3ljbGUsXG4gICAqICAgJ29uQ29tcGxldGUnOiBvbkNvbXBsZXRlXG4gICAqIH0pO1xuICAgKi9cbiAgZnVuY3Rpb24gYWRkKG5hbWUsIGZuLCBvcHRpb25zKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgYmVuY2ggPSBCZW5jaG1hcmsobmFtZSwgZm4sIG9wdGlvbnMpLFxuICAgICAgICBldmVudCA9IEV2ZW50KHsgJ3R5cGUnOiAnYWRkJywgJ3RhcmdldCc6IGJlbmNoIH0pO1xuXG4gICAgaWYgKG1lLmVtaXQoZXZlbnQpLCAhZXZlbnQuY2FuY2VsbGVkKSB7XG4gICAgICBtZS5wdXNoKGJlbmNoKTtcbiAgICB9XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyoqXG4gICAqIENyZWF0ZXMgYSBuZXcgc3VpdGUgd2l0aCBjbG9uZWQgYmVuY2htYXJrcy5cbiAgICpcbiAgICogQG5hbWUgY2xvbmVcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge09iamVjdH0gb3B0aW9ucyBPcHRpb25zIG9iamVjdCB0byBvdmVyd3JpdGUgY2xvbmVkIG9wdGlvbnMuXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBuZXcgc3VpdGUgaW5zdGFuY2UuXG4gICAqL1xuICBmdW5jdGlvbiBjbG9uZVN1aXRlKG9wdGlvbnMpIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICByZXN1bHQgPSBuZXcgbWUuY29uc3RydWN0b3IoZXh0ZW5kKHt9LCBtZS5vcHRpb25zLCBvcHRpb25zKSk7XG5cbiAgICAvLyBjb3B5IG93biBwcm9wZXJ0aWVzXG4gICAgZm9yT3duKG1lLCBmdW5jdGlvbih2YWx1ZSwga2V5KSB7XG4gICAgICBpZiAoIWhhc0tleShyZXN1bHQsIGtleSkpIHtcbiAgICAgICAgcmVzdWx0W2tleV0gPSB2YWx1ZSAmJiBpc0NsYXNzT2YodmFsdWUuY2xvbmUsICdGdW5jdGlvbicpXG4gICAgICAgICAgPyB2YWx1ZS5jbG9uZSgpXG4gICAgICAgICAgOiBkZWVwQ2xvbmUodmFsdWUpO1xuICAgICAgfVxuICAgIH0pO1xuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogQW4gYEFycmF5I2ZpbHRlcmAgbGlrZSBtZXRob2QuXG4gICAqXG4gICAqIEBuYW1lIGZpbHRlclxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7RnVuY3Rpb258U3RyaW5nfSBjYWxsYmFjayBUaGUgZnVuY3Rpb24vYWxpYXMgY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IEEgbmV3IHN1aXRlIG9mIGJlbmNobWFya3MgdGhhdCBwYXNzZWQgY2FsbGJhY2sgZmlsdGVyLlxuICAgKi9cbiAgZnVuY3Rpb24gZmlsdGVyU3VpdGUoY2FsbGJhY2spIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICByZXN1bHQgPSBuZXcgbWUuY29uc3RydWN0b3I7XG5cbiAgICByZXN1bHQucHVzaC5hcHBseShyZXN1bHQsIGZpbHRlcihtZSwgY2FsbGJhY2spKTtcbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIFJlc2V0cyBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGUuXG4gICAqXG4gICAqIEBuYW1lIHJlc2V0XG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIHN1aXRlIGluc3RhbmNlLlxuICAgKi9cbiAgZnVuY3Rpb24gcmVzZXRTdWl0ZSgpIHtcbiAgICB2YXIgZXZlbnQsXG4gICAgICAgIG1lID0gdGhpcyxcbiAgICAgICAgYWJvcnRpbmcgPSBjYWxsZWRCeS5hYm9ydFN1aXRlO1xuXG4gICAgaWYgKG1lLnJ1bm5pbmcgJiYgIWFib3J0aW5nKSB7XG4gICAgICAvLyBubyB3b3JyaWVzLCBgcmVzZXRTdWl0ZSgpYCBpcyBjYWxsZWQgd2l0aGluIGBhYm9ydFN1aXRlKClgXG4gICAgICBjYWxsZWRCeS5yZXNldFN1aXRlID0gdHJ1ZTtcbiAgICAgIG1lLmFib3J0KCk7XG4gICAgICBkZWxldGUgY2FsbGVkQnkucmVzZXRTdWl0ZTtcbiAgICB9XG4gICAgLy8gcmVzZXQgaWYgdGhlIHN0YXRlIGhhcyBjaGFuZ2VkXG4gICAgZWxzZSBpZiAoKG1lLmFib3J0ZWQgfHwgbWUucnVubmluZykgJiZcbiAgICAgICAgKG1lLmVtaXQoZXZlbnQgPSBFdmVudCgncmVzZXQnKSksICFldmVudC5jYW5jZWxsZWQpKSB7XG4gICAgICBtZS5ydW5uaW5nID0gZmFsc2U7XG4gICAgICBpZiAoIWFib3J0aW5nKSB7XG4gICAgICAgIGludm9rZShtZSwgJ3Jlc2V0Jyk7XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiBtZTtcbiAgfVxuXG4gIC8qKlxuICAgKiBSdW5zIHRoZSBzdWl0ZS5cbiAgICpcbiAgICogQG5hbWUgcnVuXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IFtvcHRpb25zPXt9XSBPcHRpb25zIG9iamVjdC5cbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIHN1aXRlIGluc3RhbmNlLlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyBiYXNpYyB1c2FnZVxuICAgKiBzdWl0ZS5ydW4oKTtcbiAgICpcbiAgICogLy8gb3Igd2l0aCBvcHRpb25zXG4gICAqIHN1aXRlLnJ1bih7ICdhc3luYyc6IHRydWUsICdxdWV1ZWQnOiB0cnVlIH0pO1xuICAgKi9cbiAgZnVuY3Rpb24gcnVuU3VpdGUob3B0aW9ucykge1xuICAgIHZhciBtZSA9IHRoaXM7XG5cbiAgICBtZS5yZXNldCgpO1xuICAgIG1lLnJ1bm5pbmcgPSB0cnVlO1xuICAgIG9wdGlvbnMgfHwgKG9wdGlvbnMgPSB7fSk7XG5cbiAgICBpbnZva2UobWUsIHtcbiAgICAgICduYW1lJzogJ3J1bicsXG4gICAgICAnYXJncyc6IG9wdGlvbnMsXG4gICAgICAncXVldWVkJzogb3B0aW9ucy5xdWV1ZWQsXG4gICAgICAnb25TdGFydCc6IGZ1bmN0aW9uKGV2ZW50KSB7XG4gICAgICAgIG1lLmVtaXQoZXZlbnQpO1xuICAgICAgfSxcbiAgICAgICdvbkN5Y2xlJzogZnVuY3Rpb24oZXZlbnQpIHtcbiAgICAgICAgdmFyIGJlbmNoID0gZXZlbnQudGFyZ2V0O1xuICAgICAgICBpZiAoYmVuY2guZXJyb3IpIHtcbiAgICAgICAgICBtZS5lbWl0KHsgJ3R5cGUnOiAnZXJyb3InLCAndGFyZ2V0JzogYmVuY2ggfSk7XG4gICAgICAgIH1cbiAgICAgICAgbWUuZW1pdChldmVudCk7XG4gICAgICAgIGV2ZW50LmFib3J0ZWQgPSBtZS5hYm9ydGVkO1xuICAgICAgfSxcbiAgICAgICdvbkNvbXBsZXRlJzogZnVuY3Rpb24oZXZlbnQpIHtcbiAgICAgICAgbWUucnVubmluZyA9IGZhbHNlO1xuICAgICAgICBtZS5lbWl0KGV2ZW50KTtcbiAgICAgIH1cbiAgICB9KTtcbiAgICByZXR1cm4gbWU7XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogRXhlY3V0ZXMgYWxsIHJlZ2lzdGVyZWQgbGlzdGVuZXJzIG9mIHRoZSBzcGVjaWZpZWQgZXZlbnQgdHlwZS5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyaywgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7U3RyaW5nfE9iamVjdH0gdHlwZSBUaGUgZXZlbnQgdHlwZSBvciBvYmplY3QuXG4gICAqIEByZXR1cm5zIHtNaXhlZH0gUmV0dXJucyB0aGUgcmV0dXJuIHZhbHVlIG9mIHRoZSBsYXN0IGxpc3RlbmVyIGV4ZWN1dGVkLlxuICAgKi9cbiAgZnVuY3Rpb24gZW1pdCh0eXBlKSB7XG4gICAgdmFyIGxpc3RlbmVycyxcbiAgICAgICAgbWUgPSB0aGlzLFxuICAgICAgICBldmVudCA9IEV2ZW50KHR5cGUpLFxuICAgICAgICBldmVudHMgPSBtZS5ldmVudHMsXG4gICAgICAgIGFyZ3MgPSAoYXJndW1lbnRzWzBdID0gZXZlbnQsIGFyZ3VtZW50cyk7XG5cbiAgICBldmVudC5jdXJyZW50VGFyZ2V0IHx8IChldmVudC5jdXJyZW50VGFyZ2V0ID0gbWUpO1xuICAgIGV2ZW50LnRhcmdldCB8fCAoZXZlbnQudGFyZ2V0ID0gbWUpO1xuICAgIGRlbGV0ZSBldmVudC5yZXN1bHQ7XG5cbiAgICBpZiAoZXZlbnRzICYmIChsaXN0ZW5lcnMgPSBoYXNLZXkoZXZlbnRzLCBldmVudC50eXBlKSAmJiBldmVudHNbZXZlbnQudHlwZV0pKSB7XG4gICAgICBmb3JFYWNoKGxpc3RlbmVycy5zbGljZSgpLCBmdW5jdGlvbihsaXN0ZW5lcikge1xuICAgICAgICBpZiAoKGV2ZW50LnJlc3VsdCA9IGxpc3RlbmVyLmFwcGx5KG1lLCBhcmdzKSkgPT09IGZhbHNlKSB7XG4gICAgICAgICAgZXZlbnQuY2FuY2VsbGVkID0gdHJ1ZTtcbiAgICAgICAgfVxuICAgICAgICByZXR1cm4gIWV2ZW50LmFib3J0ZWQ7XG4gICAgICB9KTtcbiAgICB9XG4gICAgcmV0dXJuIGV2ZW50LnJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBSZXR1cm5zIGFuIGFycmF5IG9mIGV2ZW50IGxpc3RlbmVycyBmb3IgYSBnaXZlbiB0eXBlIHRoYXQgY2FuIGJlIG1hbmlwdWxhdGVkXG4gICAqIHRvIGFkZCBvciByZW1vdmUgbGlzdGVuZXJzLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLCBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHBhcmFtIHtTdHJpbmd9IHR5cGUgVGhlIGV2ZW50IHR5cGUuXG4gICAqIEByZXR1cm5zIHtBcnJheX0gVGhlIGxpc3RlbmVycyBhcnJheS5cbiAgICovXG4gIGZ1bmN0aW9uIGxpc3RlbmVycyh0eXBlKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgZXZlbnRzID0gbWUuZXZlbnRzIHx8IChtZS5ldmVudHMgPSB7fSk7XG5cbiAgICByZXR1cm4gaGFzS2V5KGV2ZW50cywgdHlwZSkgPyBldmVudHNbdHlwZV0gOiAoZXZlbnRzW3R5cGVdID0gW10pO1xuICB9XG5cbiAgLyoqXG4gICAqIFVucmVnaXN0ZXJzIGEgbGlzdGVuZXIgZm9yIHRoZSBzcGVjaWZpZWQgZXZlbnQgdHlwZShzKSxcbiAgICogb3IgdW5yZWdpc3RlcnMgYWxsIGxpc3RlbmVycyBmb3IgdGhlIHNwZWNpZmllZCBldmVudCB0eXBlKHMpLFxuICAgKiBvciB1bnJlZ2lzdGVycyBhbGwgbGlzdGVuZXJzIGZvciBhbGwgZXZlbnQgdHlwZXMuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmssIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge1N0cmluZ30gW3R5cGVdIFRoZSBldmVudCB0eXBlLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBbbGlzdGVuZXJdIFRoZSBmdW5jdGlvbiB0byB1bnJlZ2lzdGVyLlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyB1bnJlZ2lzdGVyIGEgbGlzdGVuZXIgZm9yIGFuIGV2ZW50IHR5cGVcbiAgICogYmVuY2gub2ZmKCdjeWNsZScsIGxpc3RlbmVyKTtcbiAgICpcbiAgICogLy8gdW5yZWdpc3RlciBhIGxpc3RlbmVyIGZvciBtdWx0aXBsZSBldmVudCB0eXBlc1xuICAgKiBiZW5jaC5vZmYoJ3N0YXJ0IGN5Y2xlJywgbGlzdGVuZXIpO1xuICAgKlxuICAgKiAvLyB1bnJlZ2lzdGVyIGFsbCBsaXN0ZW5lcnMgZm9yIGFuIGV2ZW50IHR5cGVcbiAgICogYmVuY2gub2ZmKCdjeWNsZScpO1xuICAgKlxuICAgKiAvLyB1bnJlZ2lzdGVyIGFsbCBsaXN0ZW5lcnMgZm9yIG11bHRpcGxlIGV2ZW50IHR5cGVzXG4gICAqIGJlbmNoLm9mZignc3RhcnQgY3ljbGUgY29tcGxldGUnKTtcbiAgICpcbiAgICogLy8gdW5yZWdpc3RlciBhbGwgbGlzdGVuZXJzIGZvciBhbGwgZXZlbnQgdHlwZXNcbiAgICogYmVuY2gub2ZmKCk7XG4gICAqL1xuICBmdW5jdGlvbiBvZmYodHlwZSwgbGlzdGVuZXIpIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICBldmVudHMgPSBtZS5ldmVudHM7XG5cbiAgICBldmVudHMgJiYgZWFjaCh0eXBlID8gdHlwZS5zcGxpdCgnICcpIDogZXZlbnRzLCBmdW5jdGlvbihsaXN0ZW5lcnMsIHR5cGUpIHtcbiAgICAgIHZhciBpbmRleDtcbiAgICAgIGlmICh0eXBlb2YgbGlzdGVuZXJzID09ICdzdHJpbmcnKSB7XG4gICAgICAgIHR5cGUgPSBsaXN0ZW5lcnM7XG4gICAgICAgIGxpc3RlbmVycyA9IGhhc0tleShldmVudHMsIHR5cGUpICYmIGV2ZW50c1t0eXBlXTtcbiAgICAgIH1cbiAgICAgIGlmIChsaXN0ZW5lcnMpIHtcbiAgICAgICAgaWYgKGxpc3RlbmVyKSB7XG4gICAgICAgICAgaW5kZXggPSBpbmRleE9mKGxpc3RlbmVycywgbGlzdGVuZXIpO1xuICAgICAgICAgIGlmIChpbmRleCA+IC0xKSB7XG4gICAgICAgICAgICBsaXN0ZW5lcnMuc3BsaWNlKGluZGV4LCAxKTtcbiAgICAgICAgICB9XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgbGlzdGVuZXJzLmxlbmd0aCA9IDA7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9KTtcbiAgICByZXR1cm4gbWU7XG4gIH1cblxuICAvKipcbiAgICogUmVnaXN0ZXJzIGEgbGlzdGVuZXIgZm9yIHRoZSBzcGVjaWZpZWQgZXZlbnQgdHlwZShzKS5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyaywgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7U3RyaW5nfSB0eXBlIFRoZSBldmVudCB0eXBlLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBsaXN0ZW5lciBUaGUgZnVuY3Rpb24gdG8gcmVnaXN0ZXIuXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqIEBleGFtcGxlXG4gICAqXG4gICAqIC8vIHJlZ2lzdGVyIGEgbGlzdGVuZXIgZm9yIGFuIGV2ZW50IHR5cGVcbiAgICogYmVuY2gub24oJ2N5Y2xlJywgbGlzdGVuZXIpO1xuICAgKlxuICAgKiAvLyByZWdpc3RlciBhIGxpc3RlbmVyIGZvciBtdWx0aXBsZSBldmVudCB0eXBlc1xuICAgKiBiZW5jaC5vbignc3RhcnQgY3ljbGUnLCBsaXN0ZW5lcik7XG4gICAqL1xuICBmdW5jdGlvbiBvbih0eXBlLCBsaXN0ZW5lcikge1xuICAgIHZhciBtZSA9IHRoaXMsXG4gICAgICAgIGV2ZW50cyA9IG1lLmV2ZW50cyB8fCAobWUuZXZlbnRzID0ge30pO1xuXG4gICAgZm9yRWFjaCh0eXBlLnNwbGl0KCcgJyksIGZ1bmN0aW9uKHR5cGUpIHtcbiAgICAgIChoYXNLZXkoZXZlbnRzLCB0eXBlKVxuICAgICAgICA/IGV2ZW50c1t0eXBlXVxuICAgICAgICA6IChldmVudHNbdHlwZV0gPSBbXSlcbiAgICAgICkucHVzaChsaXN0ZW5lcik7XG4gICAgfSk7XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIEFib3J0cyB0aGUgYmVuY2htYXJrIHdpdGhvdXQgcmVjb3JkaW5nIHRpbWVzLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqL1xuICBmdW5jdGlvbiBhYm9ydCgpIHtcbiAgICB2YXIgZXZlbnQsXG4gICAgICAgIG1lID0gdGhpcyxcbiAgICAgICAgcmVzZXR0aW5nID0gY2FsbGVkQnkucmVzZXQ7XG5cbiAgICBpZiAobWUucnVubmluZykge1xuICAgICAgZXZlbnQgPSBFdmVudCgnYWJvcnQnKTtcbiAgICAgIG1lLmVtaXQoZXZlbnQpO1xuICAgICAgaWYgKCFldmVudC5jYW5jZWxsZWQgfHwgcmVzZXR0aW5nKSB7XG4gICAgICAgIC8vIGF2b2lkIGluZmluaXRlIHJlY3Vyc2lvblxuICAgICAgICBjYWxsZWRCeS5hYm9ydCA9IHRydWU7XG4gICAgICAgIG1lLnJlc2V0KCk7XG4gICAgICAgIGRlbGV0ZSBjYWxsZWRCeS5hYm9ydDtcblxuICAgICAgICBpZiAoc3VwcG9ydC50aW1lb3V0KSB7XG4gICAgICAgICAgY2xlYXJUaW1lb3V0KG1lLl90aW1lcklkKTtcbiAgICAgICAgICBkZWxldGUgbWUuX3RpbWVySWQ7XG4gICAgICAgIH1cbiAgICAgICAgaWYgKCFyZXNldHRpbmcpIHtcbiAgICAgICAgICBtZS5hYm9ydGVkID0gdHJ1ZTtcbiAgICAgICAgICBtZS5ydW5uaW5nID0gZmFsc2U7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyoqXG4gICAqIENyZWF0ZXMgYSBuZXcgYmVuY2htYXJrIHVzaW5nIHRoZSBzYW1lIHRlc3QgYW5kIG9wdGlvbnMuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtPYmplY3R9IG9wdGlvbnMgT3B0aW9ucyBvYmplY3QgdG8gb3ZlcndyaXRlIGNsb25lZCBvcHRpb25zLlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgbmV3IGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogdmFyIGJpemFycm8gPSBiZW5jaC5jbG9uZSh7XG4gICAqICAgJ25hbWUnOiAnZG9wcGVsZ2FuZ2VyJ1xuICAgKiB9KTtcbiAgICovXG4gIGZ1bmN0aW9uIGNsb25lKG9wdGlvbnMpIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICByZXN1bHQgPSBuZXcgbWUuY29uc3RydWN0b3IoZXh0ZW5kKHt9LCBtZSwgb3B0aW9ucykpO1xuXG4gICAgLy8gY29ycmVjdCB0aGUgYG9wdGlvbnNgIG9iamVjdFxuICAgIHJlc3VsdC5vcHRpb25zID0gZXh0ZW5kKHt9LCBtZS5vcHRpb25zLCBvcHRpb25zKTtcblxuICAgIC8vIGNvcHkgb3duIGN1c3RvbSBwcm9wZXJ0aWVzXG4gICAgZm9yT3duKG1lLCBmdW5jdGlvbih2YWx1ZSwga2V5KSB7XG4gICAgICBpZiAoIWhhc0tleShyZXN1bHQsIGtleSkpIHtcbiAgICAgICAgcmVzdWx0W2tleV0gPSBkZWVwQ2xvbmUodmFsdWUpO1xuICAgICAgfVxuICAgIH0pO1xuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogRGV0ZXJtaW5lcyBpZiBhIGJlbmNobWFyayBpcyBmYXN0ZXIgdGhhbiBhbm90aGVyLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBvdGhlciBUaGUgYmVuY2htYXJrIHRvIGNvbXBhcmUuXG4gICAqIEByZXR1cm5zIHtOdW1iZXJ9IFJldHVybnMgYC0xYCBpZiBzbG93ZXIsIGAxYCBpZiBmYXN0ZXIsIGFuZCBgMGAgaWYgaW5kZXRlcm1pbmF0ZS5cbiAgICovXG4gIGZ1bmN0aW9uIGNvbXBhcmUob3RoZXIpIHtcbiAgICB2YXIgY3JpdGljYWwsXG4gICAgICAgIHpTdGF0LFxuICAgICAgICBtZSA9IHRoaXMsXG4gICAgICAgIHNhbXBsZTEgPSBtZS5zdGF0cy5zYW1wbGUsXG4gICAgICAgIHNhbXBsZTIgPSBvdGhlci5zdGF0cy5zYW1wbGUsXG4gICAgICAgIHNpemUxID0gc2FtcGxlMS5sZW5ndGgsXG4gICAgICAgIHNpemUyID0gc2FtcGxlMi5sZW5ndGgsXG4gICAgICAgIG1heFNpemUgPSBtYXgoc2l6ZTEsIHNpemUyKSxcbiAgICAgICAgbWluU2l6ZSA9IG1pbihzaXplMSwgc2l6ZTIpLFxuICAgICAgICB1MSA9IGdldFUoc2FtcGxlMSwgc2FtcGxlMiksXG4gICAgICAgIHUyID0gZ2V0VShzYW1wbGUyLCBzYW1wbGUxKSxcbiAgICAgICAgdSA9IG1pbih1MSwgdTIpO1xuXG4gICAgZnVuY3Rpb24gZ2V0U2NvcmUoeEEsIHNhbXBsZUIpIHtcbiAgICAgIHJldHVybiByZWR1Y2Uoc2FtcGxlQiwgZnVuY3Rpb24odG90YWwsIHhCKSB7XG4gICAgICAgIHJldHVybiB0b3RhbCArICh4QiA+IHhBID8gMCA6IHhCIDwgeEEgPyAxIDogMC41KTtcbiAgICAgIH0sIDApO1xuICAgIH1cblxuICAgIGZ1bmN0aW9uIGdldFUoc2FtcGxlQSwgc2FtcGxlQikge1xuICAgICAgcmV0dXJuIHJlZHVjZShzYW1wbGVBLCBmdW5jdGlvbih0b3RhbCwgeEEpIHtcbiAgICAgICAgcmV0dXJuIHRvdGFsICsgZ2V0U2NvcmUoeEEsIHNhbXBsZUIpO1xuICAgICAgfSwgMCk7XG4gICAgfVxuXG4gICAgZnVuY3Rpb24gZ2V0Wih1KSB7XG4gICAgICByZXR1cm4gKHUgLSAoKHNpemUxICogc2l6ZTIpIC8gMikpIC8gc3FydCgoc2l6ZTEgKiBzaXplMiAqIChzaXplMSArIHNpemUyICsgMSkpIC8gMTIpO1xuICAgIH1cblxuICAgIC8vIGV4aXQgZWFybHkgaWYgY29tcGFyaW5nIHRoZSBzYW1lIGJlbmNobWFya1xuICAgIGlmIChtZSA9PSBvdGhlcikge1xuICAgICAgcmV0dXJuIDA7XG4gICAgfVxuICAgIC8vIHJlamVjdCB0aGUgbnVsbCBoeXBob3RoZXNpcyB0aGUgdHdvIHNhbXBsZXMgY29tZSBmcm9tIHRoZVxuICAgIC8vIHNhbWUgcG9wdWxhdGlvbiAoaS5lLiBoYXZlIHRoZSBzYW1lIG1lZGlhbikgaWYuLi5cbiAgICBpZiAoc2l6ZTEgKyBzaXplMiA+IDMwKSB7XG4gICAgICAvLyAuLi50aGUgei1zdGF0IGlzIGdyZWF0ZXIgdGhhbiAxLjk2IG9yIGxlc3MgdGhhbiAtMS45NlxuICAgICAgLy8gaHR0cDovL3d3dy5zdGF0aXN0aWNzbGVjdHVyZXMuY29tL3RvcGljcy9tYW5ud2hpdG5leXUvXG4gICAgICB6U3RhdCA9IGdldFoodSk7XG4gICAgICByZXR1cm4gYWJzKHpTdGF0KSA+IDEuOTYgPyAoelN0YXQgPiAwID8gLTEgOiAxKSA6IDA7XG4gICAgfVxuICAgIC8vIC4uLnRoZSBVIHZhbHVlIGlzIGxlc3MgdGhhbiBvciBlcXVhbCB0aGUgY3JpdGljYWwgVSB2YWx1ZVxuICAgIC8vIGh0dHA6Ly93d3cuZ2VvaWIuY29tL21hbm4td2hpdG5leS11LXRlc3QuaHRtbFxuICAgIGNyaXRpY2FsID0gbWF4U2l6ZSA8IDUgfHwgbWluU2l6ZSA8IDMgPyAwIDogdVRhYmxlW21heFNpemVdW21pblNpemUgLSAzXTtcbiAgICByZXR1cm4gdSA8PSBjcml0aWNhbCA/ICh1ID09IHUxID8gMSA6IC0xKSA6IDA7XG4gIH1cblxuICAvKipcbiAgICogUmVzZXQgcHJvcGVydGllcyBhbmQgYWJvcnQgaWYgcnVubmluZy5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKi9cbiAgZnVuY3Rpb24gcmVzZXQoKSB7XG4gICAgdmFyIGRhdGEsXG4gICAgICAgIGV2ZW50LFxuICAgICAgICBtZSA9IHRoaXMsXG4gICAgICAgIGluZGV4ID0gMCxcbiAgICAgICAgY2hhbmdlcyA9IHsgJ2xlbmd0aCc6IDAgfSxcbiAgICAgICAgcXVldWUgPSB7ICdsZW5ndGgnOiAwIH07XG5cbiAgICBpZiAobWUucnVubmluZyAmJiAhY2FsbGVkQnkuYWJvcnQpIHtcbiAgICAgIC8vIG5vIHdvcnJpZXMsIGByZXNldCgpYCBpcyBjYWxsZWQgd2l0aGluIGBhYm9ydCgpYFxuICAgICAgY2FsbGVkQnkucmVzZXQgPSB0cnVlO1xuICAgICAgbWUuYWJvcnQoKTtcbiAgICAgIGRlbGV0ZSBjYWxsZWRCeS5yZXNldDtcbiAgICB9XG4gICAgZWxzZSB7XG4gICAgICAvLyBhIG5vbi1yZWN1cnNpdmUgc29sdXRpb24gdG8gY2hlY2sgaWYgcHJvcGVydGllcyBoYXZlIGNoYW5nZWRcbiAgICAgIC8vIGh0dHA6Ly93d3cuanNsYWIuZGsvYXJ0aWNsZXMvbm9uLnJlY3Vyc2l2ZS5wcmVvcmRlci50cmF2ZXJzYWwucGFydDRcbiAgICAgIGRhdGEgPSB7ICdkZXN0aW5hdGlvbic6IG1lLCAnc291cmNlJzogZXh0ZW5kKHt9LCBtZS5jb25zdHJ1Y3Rvci5wcm90b3R5cGUsIG1lLm9wdGlvbnMpIH07XG4gICAgICBkbyB7XG4gICAgICAgIGZvck93bihkYXRhLnNvdXJjZSwgZnVuY3Rpb24odmFsdWUsIGtleSkge1xuICAgICAgICAgIHZhciBjaGFuZ2VkLFxuICAgICAgICAgICAgICBkZXN0aW5hdGlvbiA9IGRhdGEuZGVzdGluYXRpb24sXG4gICAgICAgICAgICAgIGN1cnJWYWx1ZSA9IGRlc3RpbmF0aW9uW2tleV07XG5cbiAgICAgICAgICBpZiAodmFsdWUgJiYgdHlwZW9mIHZhbHVlID09ICdvYmplY3QnKSB7XG4gICAgICAgICAgICBpZiAoaXNDbGFzc09mKHZhbHVlLCAnQXJyYXknKSkge1xuICAgICAgICAgICAgICAvLyBjaGVjayBpZiBhbiBhcnJheSB2YWx1ZSBoYXMgY2hhbmdlZCB0byBhIG5vbi1hcnJheSB2YWx1ZVxuICAgICAgICAgICAgICBpZiAoIWlzQ2xhc3NPZihjdXJyVmFsdWUsICdBcnJheScpKSB7XG4gICAgICAgICAgICAgICAgY2hhbmdlZCA9IGN1cnJWYWx1ZSA9IFtdO1xuICAgICAgICAgICAgICB9XG4gICAgICAgICAgICAgIC8vIG9yIGhhcyBjaGFuZ2VkIGl0cyBsZW5ndGhcbiAgICAgICAgICAgICAgaWYgKGN1cnJWYWx1ZS5sZW5ndGggIT0gdmFsdWUubGVuZ3RoKSB7XG4gICAgICAgICAgICAgICAgY2hhbmdlZCA9IGN1cnJWYWx1ZSA9IGN1cnJWYWx1ZS5zbGljZSgwLCB2YWx1ZS5sZW5ndGgpO1xuICAgICAgICAgICAgICAgIGN1cnJWYWx1ZS5sZW5ndGggPSB2YWx1ZS5sZW5ndGg7XG4gICAgICAgICAgICAgIH1cbiAgICAgICAgICAgIH1cbiAgICAgICAgICAgIC8vIGNoZWNrIGlmIGFuIG9iamVjdCBoYXMgY2hhbmdlZCB0byBhIG5vbi1vYmplY3QgdmFsdWVcbiAgICAgICAgICAgIGVsc2UgaWYgKCFjdXJyVmFsdWUgfHwgdHlwZW9mIGN1cnJWYWx1ZSAhPSAnb2JqZWN0Jykge1xuICAgICAgICAgICAgICBjaGFuZ2VkID0gY3VyclZhbHVlID0ge307XG4gICAgICAgICAgICB9XG4gICAgICAgICAgICAvLyByZWdpc3RlciBhIGNoYW5nZWQgb2JqZWN0XG4gICAgICAgICAgICBpZiAoY2hhbmdlZCkge1xuICAgICAgICAgICAgICBjaGFuZ2VzW2NoYW5nZXMubGVuZ3RoKytdID0geyAnZGVzdGluYXRpb24nOiBkZXN0aW5hdGlvbiwgJ2tleSc6IGtleSwgJ3ZhbHVlJzogY3VyclZhbHVlIH07XG4gICAgICAgICAgICB9XG4gICAgICAgICAgICBxdWV1ZVtxdWV1ZS5sZW5ndGgrK10gPSB7ICdkZXN0aW5hdGlvbic6IGN1cnJWYWx1ZSwgJ3NvdXJjZSc6IHZhbHVlIH07XG4gICAgICAgICAgfVxuICAgICAgICAgIC8vIHJlZ2lzdGVyIGEgY2hhbmdlZCBwcmltaXRpdmVcbiAgICAgICAgICBlbHNlIGlmICh2YWx1ZSAhPT0gY3VyclZhbHVlICYmICEodmFsdWUgPT0gbnVsbCB8fCBpc0NsYXNzT2YodmFsdWUsICdGdW5jdGlvbicpKSkge1xuICAgICAgICAgICAgY2hhbmdlc1tjaGFuZ2VzLmxlbmd0aCsrXSA9IHsgJ2Rlc3RpbmF0aW9uJzogZGVzdGluYXRpb24sICdrZXknOiBrZXksICd2YWx1ZSc6IHZhbHVlIH07XG4gICAgICAgICAgfVxuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICAgIHdoaWxlICgoZGF0YSA9IHF1ZXVlW2luZGV4KytdKSk7XG5cbiAgICAgIC8vIGlmIGNoYW5nZWQgZW1pdCB0aGUgYHJlc2V0YCBldmVudCBhbmQgaWYgaXQgaXNuJ3QgY2FuY2VsbGVkIHJlc2V0IHRoZSBiZW5jaG1hcmtcbiAgICAgIGlmIChjaGFuZ2VzLmxlbmd0aCAmJiAobWUuZW1pdChldmVudCA9IEV2ZW50KCdyZXNldCcpKSwgIWV2ZW50LmNhbmNlbGxlZCkpIHtcbiAgICAgICAgZm9yRWFjaChjaGFuZ2VzLCBmdW5jdGlvbihkYXRhKSB7XG4gICAgICAgICAgZGF0YS5kZXN0aW5hdGlvbltkYXRhLmtleV0gPSBkYXRhLnZhbHVlO1xuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyoqXG4gICAqIERpc3BsYXlzIHJlbGV2YW50IGJlbmNobWFyayBpbmZvcm1hdGlvbiB3aGVuIGNvZXJjZWQgdG8gYSBzdHJpbmcuXG4gICAqXG4gICAqIEBuYW1lIHRvU3RyaW5nXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHJldHVybnMge1N0cmluZ30gQSBzdHJpbmcgcmVwcmVzZW50YXRpb24gb2YgdGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICovXG4gIGZ1bmN0aW9uIHRvU3RyaW5nQmVuY2goKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgZXJyb3IgPSBtZS5lcnJvcixcbiAgICAgICAgaHogPSBtZS5oeixcbiAgICAgICAgaWQgPSBtZS5pZCxcbiAgICAgICAgc3RhdHMgPSBtZS5zdGF0cyxcbiAgICAgICAgc2l6ZSA9IHN0YXRzLnNhbXBsZS5sZW5ndGgsXG4gICAgICAgIHBtID0gc3VwcG9ydC5qYXZhID8gJysvLScgOiAnXFx4YjEnLFxuICAgICAgICByZXN1bHQgPSBtZS5uYW1lIHx8IChpc05hTihpZCkgPyBpZCA6ICc8VGVzdCAjJyArIGlkICsgJz4nKTtcblxuICAgIGlmIChlcnJvcikge1xuICAgICAgcmVzdWx0ICs9ICc6ICcgKyBqb2luKGVycm9yKTtcbiAgICB9IGVsc2Uge1xuICAgICAgcmVzdWx0ICs9ICcgeCAnICsgZm9ybWF0TnVtYmVyKGh6LnRvRml4ZWQoaHogPCAxMDAgPyAyIDogMCkpICsgJyBvcHMvc2VjICcgKyBwbSArXG4gICAgICAgIHN0YXRzLnJtZS50b0ZpeGVkKDIpICsgJyUgKCcgKyBzaXplICsgJyBydW4nICsgKHNpemUgPT0gMSA/ICcnIDogJ3MnKSArICcgc2FtcGxlZCknO1xuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIENsb2NrcyB0aGUgdGltZSB0YWtlbiB0byBleGVjdXRlIGEgdGVzdCBwZXIgY3ljbGUgKHNlY3MpLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge09iamVjdH0gYmVuY2ggVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQHJldHVybnMge051bWJlcn0gVGhlIHRpbWUgdGFrZW4uXG4gICAqL1xuICBmdW5jdGlvbiBjbG9jaygpIHtcbiAgICB2YXIgYXBwbGV0LFxuICAgICAgICBvcHRpb25zID0gQmVuY2htYXJrLm9wdGlvbnMsXG4gICAgICAgIHRlbXBsYXRlID0geyAnYmVnaW4nOiAncyQ9bmV3IG4kJywgJ2VuZCc6ICdyJD0obmV3IG4kLXMkKS8xZTMnLCAndWlkJzogdWlkIH0sXG4gICAgICAgIHRpbWVycyA9IFt7ICducyc6IHRpbWVyLm5zLCAncmVzJzogbWF4KDAuMDAxNSwgZ2V0UmVzKCdtcycpKSwgJ3VuaXQnOiAnbXMnIH1dO1xuXG4gICAgLy8gbGF6eSBkZWZpbmUgZm9yIGhpLXJlcyB0aW1lcnNcbiAgICBjbG9jayA9IGZ1bmN0aW9uKGNsb25lKSB7XG4gICAgICB2YXIgZGVmZXJyZWQ7XG4gICAgICBpZiAoY2xvbmUgaW5zdGFuY2VvZiBEZWZlcnJlZCkge1xuICAgICAgICBkZWZlcnJlZCA9IGNsb25lO1xuICAgICAgICBjbG9uZSA9IGRlZmVycmVkLmJlbmNobWFyaztcbiAgICAgIH1cblxuICAgICAgdmFyIGJlbmNoID0gY2xvbmUuX29yaWdpbmFsLFxuICAgICAgICAgIGZuID0gYmVuY2guZm4sXG4gICAgICAgICAgZm5BcmcgPSBkZWZlcnJlZCA/IGdldEZpcnN0QXJndW1lbnQoZm4pIHx8ICdkZWZlcnJlZCcgOiAnJyxcbiAgICAgICAgICBzdHJpbmdhYmxlID0gaXNTdHJpbmdhYmxlKGZuKTtcblxuICAgICAgdmFyIHNvdXJjZSA9IHtcbiAgICAgICAgJ3NldHVwJzogZ2V0U291cmNlKGJlbmNoLnNldHVwLCBwcmVwcm9jZXNzKCdtJC5zZXR1cCgpJykpLFxuICAgICAgICAnZm4nOiBnZXRTb3VyY2UoZm4sIHByZXByb2Nlc3MoJ20kLmZuKCcgKyBmbkFyZyArICcpJykpLFxuICAgICAgICAnZm5BcmcnOiBmbkFyZyxcbiAgICAgICAgJ3RlYXJkb3duJzogZ2V0U291cmNlKGJlbmNoLnRlYXJkb3duLCBwcmVwcm9jZXNzKCdtJC50ZWFyZG93bigpJykpXG4gICAgICB9O1xuXG4gICAgICB2YXIgY291bnQgPSBiZW5jaC5jb3VudCA9IGNsb25lLmNvdW50LFxuICAgICAgICAgIGRlY29tcGlsYWJsZSA9IHN1cHBvcnQuZGVjb21waWxhdGlvbiB8fCBzdHJpbmdhYmxlLFxuICAgICAgICAgIGlkID0gYmVuY2guaWQsXG4gICAgICAgICAgaXNFbXB0eSA9ICEoc291cmNlLmZuIHx8IHN0cmluZ2FibGUpLFxuICAgICAgICAgIG5hbWUgPSBiZW5jaC5uYW1lIHx8ICh0eXBlb2YgaWQgPT0gJ251bWJlcicgPyAnPFRlc3QgIycgKyBpZCArICc+JyA6IGlkKSxcbiAgICAgICAgICBucyA9IHRpbWVyLm5zLFxuICAgICAgICAgIHJlc3VsdCA9IDA7XG5cbiAgICAgIC8vIGluaXQgYG1pblRpbWVgIGlmIG5lZWRlZFxuICAgICAgY2xvbmUubWluVGltZSA9IGJlbmNoLm1pblRpbWUgfHwgKGJlbmNoLm1pblRpbWUgPSBiZW5jaC5vcHRpb25zLm1pblRpbWUgPSBvcHRpb25zLm1pblRpbWUpO1xuXG4gICAgICAvLyByZXBhaXIgbmFub3NlY29uZCB0aW1lclxuICAgICAgLy8gKHNvbWUgQ2hyb21lIGJ1aWxkcyBlcmFzZSB0aGUgYG5zYCB2YXJpYWJsZSBhZnRlciBtaWxsaW9ucyBvZiBleGVjdXRpb25zKVxuICAgICAgaWYgKGFwcGxldCkge1xuICAgICAgICB0cnkge1xuICAgICAgICAgIG5zLm5hbm9UaW1lKCk7XG4gICAgICAgIH0gY2F0Y2goZSkge1xuICAgICAgICAgIC8vIHVzZSBub24tZWxlbWVudCB0byBhdm9pZCBpc3N1ZXMgd2l0aCBsaWJzIHRoYXQgYXVnbWVudCB0aGVtXG4gICAgICAgICAgbnMgPSB0aW1lci5ucyA9IG5ldyBhcHBsZXQuUGFja2FnZXMubmFubztcbiAgICAgICAgfVxuICAgICAgfVxuXG4gICAgICAvLyBDb21waWxlIGluIHNldHVwL3RlYXJkb3duIGZ1bmN0aW9ucyBhbmQgdGhlIHRlc3QgbG9vcC5cbiAgICAgIC8vIENyZWF0ZSBhIG5ldyBjb21waWxlZCB0ZXN0LCBpbnN0ZWFkIG9mIHVzaW5nIHRoZSBjYWNoZWQgYGJlbmNoLmNvbXBpbGVkYCxcbiAgICAgIC8vIHRvIGF2b2lkIHBvdGVudGlhbCBlbmdpbmUgb3B0aW1pemF0aW9ucyBlbmFibGVkIG92ZXIgdGhlIGxpZmUgb2YgdGhlIHRlc3QuXG4gICAgICB2YXIgY29tcGlsZWQgPSBiZW5jaC5jb21waWxlZCA9IGNyZWF0ZUZ1bmN0aW9uKHByZXByb2Nlc3MoJ3QkJyksIGludGVycG9sYXRlKFxuICAgICAgICBwcmVwcm9jZXNzKGRlZmVycmVkXG4gICAgICAgICAgPyAndmFyIGQkPXRoaXMsI3tmbkFyZ309ZCQsbSQ9ZCQuYmVuY2htYXJrLl9vcmlnaW5hbCxmJD1tJC5mbixzdSQ9bSQuc2V0dXAsdGQkPW0kLnRlYXJkb3duOycgK1xuICAgICAgICAgICAgLy8gd2hlbiBgZGVmZXJyZWQuY3ljbGVzYCBpcyBgMGAgdGhlbi4uLlxuICAgICAgICAgICAgJ2lmKCFkJC5jeWNsZXMpeycgK1xuICAgICAgICAgICAgLy8gc2V0IGBkZWZlcnJlZC5mbmBcbiAgICAgICAgICAgICdkJC5mbj1mdW5jdGlvbigpe3ZhciAje2ZuQXJnfT1kJDtpZih0eXBlb2YgZiQ9PVwiZnVuY3Rpb25cIil7dHJ5eyN7Zm59XFxufWNhdGNoKGUkKXtmJChkJCl9fWVsc2V7I3tmbn1cXG59fTsnICtcbiAgICAgICAgICAgIC8vIHNldCBgZGVmZXJyZWQudGVhcmRvd25gXG4gICAgICAgICAgICAnZCQudGVhcmRvd249ZnVuY3Rpb24oKXtkJC5jeWNsZXM9MDtpZih0eXBlb2YgdGQkPT1cImZ1bmN0aW9uXCIpe3RyeXsje3RlYXJkb3dufVxcbn1jYXRjaChlJCl7dGQkKCl9fWVsc2V7I3t0ZWFyZG93bn1cXG59fTsnICtcbiAgICAgICAgICAgIC8vIGV4ZWN1dGUgdGhlIGJlbmNobWFyaydzIGBzZXR1cGBcbiAgICAgICAgICAgICdpZih0eXBlb2Ygc3UkPT1cImZ1bmN0aW9uXCIpe3RyeXsje3NldHVwfVxcbn1jYXRjaChlJCl7c3UkKCl9fWVsc2V7I3tzZXR1cH1cXG59OycgK1xuICAgICAgICAgICAgLy8gc3RhcnQgdGltZXJcbiAgICAgICAgICAgICd0JC5zdGFydChkJCk7JyArXG4gICAgICAgICAgICAvLyBleGVjdXRlIGBkZWZlcnJlZC5mbmAgYW5kIHJldHVybiBhIGR1bW15IG9iamVjdFxuICAgICAgICAgICAgJ31kJC5mbigpO3JldHVybnt9J1xuXG4gICAgICAgICAgOiAndmFyIHIkLHMkLG0kPXRoaXMsZiQ9bSQuZm4saSQ9bSQuY291bnQsbiQ9dCQubnM7I3tzZXR1cH1cXG4je2JlZ2lufTsnICtcbiAgICAgICAgICAgICd3aGlsZShpJC0tKXsje2ZufVxcbn0je2VuZH07I3t0ZWFyZG93bn1cXG5yZXR1cm57ZWxhcHNlZDpyJCx1aWQ6XCIje3VpZH1cIn0nKSxcbiAgICAgICAgc291cmNlXG4gICAgICApKTtcblxuICAgICAgdHJ5IHtcbiAgICAgICAgaWYgKGlzRW1wdHkpIHtcbiAgICAgICAgICAvLyBGaXJlZm94IG1heSByZW1vdmUgZGVhZCBjb2RlIGZyb20gRnVuY3Rpb24jdG9TdHJpbmcgcmVzdWx0c1xuICAgICAgICAgIC8vIGh0dHA6Ly9idWd6aWwubGEvNTM2MDg1XG4gICAgICAgICAgdGhyb3cgbmV3IEVycm9yKCdUaGUgdGVzdCBcIicgKyBuYW1lICsgJ1wiIGlzIGVtcHR5LiBUaGlzIG1heSBiZSB0aGUgcmVzdWx0IG9mIGRlYWQgY29kZSByZW1vdmFsLicpO1xuICAgICAgICB9XG4gICAgICAgIGVsc2UgaWYgKCFkZWZlcnJlZCkge1xuICAgICAgICAgIC8vIHByZXRlc3QgdG8gZGV0ZXJtaW5lIGlmIGNvbXBpbGVkIGNvZGUgaXMgZXhpdHMgZWFybHksIHVzdWFsbHkgYnkgYVxuICAgICAgICAgIC8vIHJvZ3VlIGByZXR1cm5gIHN0YXRlbWVudCwgYnkgY2hlY2tpbmcgZm9yIGEgcmV0dXJuIG9iamVjdCB3aXRoIHRoZSB1aWRcbiAgICAgICAgICBiZW5jaC5jb3VudCA9IDE7XG4gICAgICAgICAgY29tcGlsZWQgPSAoY29tcGlsZWQuY2FsbChiZW5jaCwgdGltZXIpIHx8IHt9KS51aWQgPT0gdWlkICYmIGNvbXBpbGVkO1xuICAgICAgICAgIGJlbmNoLmNvdW50ID0gY291bnQ7XG4gICAgICAgIH1cbiAgICAgIH0gY2F0Y2goZSkge1xuICAgICAgICBjb21waWxlZCA9IG51bGw7XG4gICAgICAgIGNsb25lLmVycm9yID0gZSB8fCBuZXcgRXJyb3IoU3RyaW5nKGUpKTtcbiAgICAgICAgYmVuY2guY291bnQgPSBjb3VudDtcbiAgICAgIH1cbiAgICAgIC8vIGZhbGxiYWNrIHdoZW4gYSB0ZXN0IGV4aXRzIGVhcmx5IG9yIGVycm9ycyBkdXJpbmcgcHJldGVzdFxuICAgICAgaWYgKGRlY29tcGlsYWJsZSAmJiAhY29tcGlsZWQgJiYgIWRlZmVycmVkICYmICFpc0VtcHR5KSB7XG4gICAgICAgIGNvbXBpbGVkID0gY3JlYXRlRnVuY3Rpb24ocHJlcHJvY2VzcygndCQnKSwgaW50ZXJwb2xhdGUoXG4gICAgICAgICAgcHJlcHJvY2VzcyhcbiAgICAgICAgICAgIChjbG9uZS5lcnJvciAmJiAhc3RyaW5nYWJsZVxuICAgICAgICAgICAgICA/ICd2YXIgciQscyQsbSQ9dGhpcyxmJD1tJC5mbixpJD1tJC5jb3VudCdcbiAgICAgICAgICAgICAgOiAnZnVuY3Rpb24gZiQoKXsje2ZufVxcbn12YXIgciQscyQsbSQ9dGhpcyxpJD1tJC5jb3VudCdcbiAgICAgICAgICAgICkgK1xuICAgICAgICAgICAgJyxuJD10JC5uczsje3NldHVwfVxcbiN7YmVnaW59O20kLmYkPWYkO3doaWxlKGkkLS0pe20kLmYkKCl9I3tlbmR9OycgK1xuICAgICAgICAgICAgJ2RlbGV0ZSBtJC5mJDsje3RlYXJkb3dufVxcbnJldHVybntlbGFwc2VkOnIkfSdcbiAgICAgICAgICApLFxuICAgICAgICAgIHNvdXJjZVxuICAgICAgICApKTtcblxuICAgICAgICB0cnkge1xuICAgICAgICAgIC8vIHByZXRlc3Qgb25lIG1vcmUgdGltZSB0byBjaGVjayBmb3IgZXJyb3JzXG4gICAgICAgICAgYmVuY2guY291bnQgPSAxO1xuICAgICAgICAgIGNvbXBpbGVkLmNhbGwoYmVuY2gsIHRpbWVyKTtcbiAgICAgICAgICBiZW5jaC5jb21waWxlZCA9IGNvbXBpbGVkO1xuICAgICAgICAgIGJlbmNoLmNvdW50ID0gY291bnQ7XG4gICAgICAgICAgZGVsZXRlIGNsb25lLmVycm9yO1xuICAgICAgICB9XG4gICAgICAgIGNhdGNoKGUpIHtcbiAgICAgICAgICBiZW5jaC5jb3VudCA9IGNvdW50O1xuICAgICAgICAgIGlmIChjbG9uZS5lcnJvcikge1xuICAgICAgICAgICAgY29tcGlsZWQgPSBudWxsO1xuICAgICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgICBiZW5jaC5jb21waWxlZCA9IGNvbXBpbGVkO1xuICAgICAgICAgICAgY2xvbmUuZXJyb3IgPSBlIHx8IG5ldyBFcnJvcihTdHJpbmcoZSkpO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgLy8gYXNzaWduIGBjb21waWxlZGAgdG8gYGNsb25lYCBiZWZvcmUgY2FsbGluZyBpbiBjYXNlIGEgZGVmZXJyZWQgYmVuY2htYXJrXG4gICAgICAvLyBpbW1lZGlhdGVseSBjYWxscyBgZGVmZXJyZWQucmVzb2x2ZSgpYFxuICAgICAgY2xvbmUuY29tcGlsZWQgPSBjb21waWxlZDtcbiAgICAgIC8vIGlmIG5vIGVycm9ycyBydW4gdGhlIGZ1bGwgdGVzdCBsb29wXG4gICAgICBpZiAoIWNsb25lLmVycm9yKSB7XG4gICAgICAgIHJlc3VsdCA9IGNvbXBpbGVkLmNhbGwoZGVmZXJyZWQgfHwgYmVuY2gsIHRpbWVyKS5lbGFwc2VkO1xuICAgICAgfVxuICAgICAgcmV0dXJuIHJlc3VsdDtcbiAgICB9O1xuXG4gICAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gICAgLyoqXG4gICAgICogR2V0cyB0aGUgY3VycmVudCB0aW1lcidzIG1pbmltdW0gcmVzb2x1dGlvbiAoc2VjcykuXG4gICAgICovXG4gICAgZnVuY3Rpb24gZ2V0UmVzKHVuaXQpIHtcbiAgICAgIHZhciBtZWFzdXJlZCxcbiAgICAgICAgICBiZWdpbixcbiAgICAgICAgICBjb3VudCA9IDMwLFxuICAgICAgICAgIGRpdmlzb3IgPSAxZTMsXG4gICAgICAgICAgbnMgPSB0aW1lci5ucyxcbiAgICAgICAgICBzYW1wbGUgPSBbXTtcblxuICAgICAgLy8gZ2V0IGF2ZXJhZ2Ugc21hbGxlc3QgbWVhc3VyYWJsZSB0aW1lXG4gICAgICB3aGlsZSAoY291bnQtLSkge1xuICAgICAgICBpZiAodW5pdCA9PSAndXMnKSB7XG4gICAgICAgICAgZGl2aXNvciA9IDFlNjtcbiAgICAgICAgICBpZiAobnMuc3RvcCkge1xuICAgICAgICAgICAgbnMuc3RhcnQoKTtcbiAgICAgICAgICAgIHdoaWxlICghKG1lYXN1cmVkID0gbnMubWljcm9zZWNvbmRzKCkpKSB7IH1cbiAgICAgICAgICB9IGVsc2UgaWYgKG5zW3BlcmZOYW1lXSkge1xuICAgICAgICAgICAgZGl2aXNvciA9IDFlMztcbiAgICAgICAgICAgIG1lYXN1cmVkID0gRnVuY3Rpb24oJ24nLCAndmFyIHIscz1uLicgKyBwZXJmTmFtZSArICcoKTt3aGlsZSghKHI9bi4nICsgcGVyZk5hbWUgKyAnKCktcykpe307cmV0dXJuIHInKShucyk7XG4gICAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICAgIGJlZ2luID0gbnMoKTtcbiAgICAgICAgICAgIHdoaWxlICghKG1lYXN1cmVkID0gbnMoKSAtIGJlZ2luKSkgeyB9XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICAgIGVsc2UgaWYgKHVuaXQgPT0gJ25zJykge1xuICAgICAgICAgIGRpdmlzb3IgPSAxZTk7XG4gICAgICAgICAgaWYgKG5zLm5hbm9UaW1lKSB7XG4gICAgICAgICAgICBiZWdpbiA9IG5zLm5hbm9UaW1lKCk7XG4gICAgICAgICAgICB3aGlsZSAoIShtZWFzdXJlZCA9IG5zLm5hbm9UaW1lKCkgLSBiZWdpbikpIHsgfVxuICAgICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgICBiZWdpbiA9IChiZWdpbiA9IG5zKCkpWzBdICsgKGJlZ2luWzFdIC8gZGl2aXNvcik7XG4gICAgICAgICAgICB3aGlsZSAoIShtZWFzdXJlZCA9ICgobWVhc3VyZWQgPSBucygpKVswXSArIChtZWFzdXJlZFsxXSAvIGRpdmlzb3IpKSAtIGJlZ2luKSkgeyB9XG4gICAgICAgICAgICBkaXZpc29yID0gMTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgICAgZWxzZSB7XG4gICAgICAgICAgYmVnaW4gPSBuZXcgbnM7XG4gICAgICAgICAgd2hpbGUgKCEobWVhc3VyZWQgPSBuZXcgbnMgLSBiZWdpbikpIHsgfVxuICAgICAgICB9XG4gICAgICAgIC8vIGNoZWNrIGZvciBicm9rZW4gdGltZXJzIChuYW5vVGltZSBtYXkgaGF2ZSBpc3N1ZXMpXG4gICAgICAgIC8vIGh0dHA6Ly9hbGl2ZWJ1dHNsZWVweS5zcm5ldC5jei91bnJlbGlhYmxlLXN5c3RlbS1uYW5vdGltZS9cbiAgICAgICAgaWYgKG1lYXN1cmVkID4gMCkge1xuICAgICAgICAgIHNhbXBsZS5wdXNoKG1lYXN1cmVkKTtcbiAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICBzYW1wbGUucHVzaChJbmZpbml0eSk7XG4gICAgICAgICAgYnJlYWs7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIC8vIGNvbnZlcnQgdG8gc2Vjb25kc1xuICAgICAgcmV0dXJuIGdldE1lYW4oc2FtcGxlKSAvIGRpdmlzb3I7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogUmVwbGFjZXMgYWxsIG9jY3VycmVuY2VzIG9mIGAkYCB3aXRoIGEgdW5pcXVlIG51bWJlciBhbmRcbiAgICAgKiB0ZW1wbGF0ZSB0b2tlbnMgd2l0aCBjb250ZW50LlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIHByZXByb2Nlc3MoY29kZSkge1xuICAgICAgcmV0dXJuIGludGVycG9sYXRlKGNvZGUsIHRlbXBsYXRlKS5yZXBsYWNlKC9cXCQvZywgL1xcZCsvLmV4ZWModWlkKSk7XG4gICAgfVxuXG4gICAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gICAgLy8gZGV0ZWN0IG5hbm9zZWNvbmQgc3VwcG9ydCBmcm9tIGEgSmF2YSBhcHBsZXRcbiAgICBlYWNoKGRvYyAmJiBkb2MuYXBwbGV0cyB8fCBbXSwgZnVuY3Rpb24oZWxlbWVudCkge1xuICAgICAgcmV0dXJuICEodGltZXIubnMgPSBhcHBsZXQgPSAnbmFub1RpbWUnIGluIGVsZW1lbnQgJiYgZWxlbWVudCk7XG4gICAgfSk7XG5cbiAgICAvLyBjaGVjayB0eXBlIGluIGNhc2UgU2FmYXJpIHJldHVybnMgYW4gb2JqZWN0IGluc3RlYWQgb2YgYSBudW1iZXJcbiAgICB0cnkge1xuICAgICAgaWYgKHR5cGVvZiB0aW1lci5ucy5uYW5vVGltZSgpID09ICdudW1iZXInKSB7XG4gICAgICAgIHRpbWVycy5wdXNoKHsgJ25zJzogdGltZXIubnMsICdyZXMnOiBnZXRSZXMoJ25zJyksICd1bml0JzogJ25zJyB9KTtcbiAgICAgIH1cbiAgICB9IGNhdGNoKGUpIHsgfVxuXG4gICAgLy8gZGV0ZWN0IENocm9tZSdzIG1pY3Jvc2Vjb25kIHRpbWVyOlxuICAgIC8vIGVuYWJsZSBiZW5jaG1hcmtpbmcgdmlhIHRoZSAtLWVuYWJsZS1iZW5jaG1hcmtpbmcgY29tbWFuZFxuICAgIC8vIGxpbmUgc3dpdGNoIGluIGF0IGxlYXN0IENocm9tZSA3IHRvIHVzZSBjaHJvbWUuSW50ZXJ2YWxcbiAgICB0cnkge1xuICAgICAgaWYgKCh0aW1lci5ucyA9IG5ldyAod2luZG93LmNocm9tZSB8fCB3aW5kb3cuY2hyb21pdW0pLkludGVydmFsKSkge1xuICAgICAgICB0aW1lcnMucHVzaCh7ICducyc6IHRpbWVyLm5zLCAncmVzJzogZ2V0UmVzKCd1cycpLCAndW5pdCc6ICd1cycgfSk7XG4gICAgICB9XG4gICAgfSBjYXRjaChlKSB7IH1cblxuICAgIC8vIGRldGVjdCBgcGVyZm9ybWFuY2Uubm93YCBtaWNyb3NlY29uZCByZXNvbHV0aW9uIHRpbWVyXG4gICAgaWYgKCh0aW1lci5ucyA9IHBlcmZOYW1lICYmIHBlcmZPYmplY3QpKSB7XG4gICAgICB0aW1lcnMucHVzaCh7ICducyc6IHRpbWVyLm5zLCAncmVzJzogZ2V0UmVzKCd1cycpLCAndW5pdCc6ICd1cycgfSk7XG4gICAgfVxuXG4gICAgLy8gZGV0ZWN0IE5vZGUncyBuYW5vc2Vjb25kIHJlc29sdXRpb24gdGltZXIgYXZhaWxhYmxlIGluIE5vZGUgPj0gMC44XG4gICAgaWYgKHByb2Nlc3NPYmplY3QgJiYgdHlwZW9mICh0aW1lci5ucyA9IHByb2Nlc3NPYmplY3QuaHJ0aW1lKSA9PSAnZnVuY3Rpb24nKSB7XG4gICAgICB0aW1lcnMucHVzaCh7ICducyc6IHRpbWVyLm5zLCAncmVzJzogZ2V0UmVzKCducycpLCAndW5pdCc6ICducycgfSk7XG4gICAgfVxuXG4gICAgLy8gZGV0ZWN0IFdhZGUgU2ltbW9ucycgTm9kZSBtaWNyb3RpbWUgbW9kdWxlXG4gICAgaWYgKG1pY3JvdGltZU9iamVjdCAmJiB0eXBlb2YgKHRpbWVyLm5zID0gbWljcm90aW1lT2JqZWN0Lm5vdykgPT0gJ2Z1bmN0aW9uJykge1xuICAgICAgdGltZXJzLnB1c2goeyAnbnMnOiB0aW1lci5ucywgICdyZXMnOiBnZXRSZXMoJ3VzJyksICd1bml0JzogJ3VzJyB9KTtcbiAgICB9XG5cbiAgICAvLyBwaWNrIHRpbWVyIHdpdGggaGlnaGVzdCByZXNvbHV0aW9uXG4gICAgdGltZXIgPSByZWR1Y2UodGltZXJzLCBmdW5jdGlvbih0aW1lciwgb3RoZXIpIHtcbiAgICAgIHJldHVybiBvdGhlci5yZXMgPCB0aW1lci5yZXMgPyBvdGhlciA6IHRpbWVyO1xuICAgIH0pO1xuXG4gICAgLy8gcmVtb3ZlIHVudXNlZCBhcHBsZXRcbiAgICBpZiAodGltZXIudW5pdCAhPSAnbnMnICYmIGFwcGxldCkge1xuICAgICAgYXBwbGV0ID0gZGVzdHJveUVsZW1lbnQoYXBwbGV0KTtcbiAgICB9XG4gICAgLy8gZXJyb3IgaWYgdGhlcmUgYXJlIG5vIHdvcmtpbmcgdGltZXJzXG4gICAgaWYgKHRpbWVyLnJlcyA9PSBJbmZpbml0eSkge1xuICAgICAgdGhyb3cgbmV3IEVycm9yKCdCZW5jaG1hcmsuanMgd2FzIHVuYWJsZSB0byBmaW5kIGEgd29ya2luZyB0aW1lci4nKTtcbiAgICB9XG4gICAgLy8gdXNlIEFQSSBvZiBjaG9zZW4gdGltZXJcbiAgICBpZiAodGltZXIudW5pdCA9PSAnbnMnKSB7XG4gICAgICBpZiAodGltZXIubnMubmFub1RpbWUpIHtcbiAgICAgICAgZXh0ZW5kKHRlbXBsYXRlLCB7XG4gICAgICAgICAgJ2JlZ2luJzogJ3MkPW4kLm5hbm9UaW1lKCknLFxuICAgICAgICAgICdlbmQnOiAnciQ9KG4kLm5hbm9UaW1lKCktcyQpLzFlOSdcbiAgICAgICAgfSk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBleHRlbmQodGVtcGxhdGUsIHtcbiAgICAgICAgICAnYmVnaW4nOiAncyQ9biQoKScsXG4gICAgICAgICAgJ2VuZCc6ICdyJD1uJChzJCk7ciQ9ciRbMF0rKHIkWzFdLzFlOSknXG4gICAgICAgIH0pO1xuICAgICAgfVxuICAgIH1cbiAgICBlbHNlIGlmICh0aW1lci51bml0ID09ICd1cycpIHtcbiAgICAgIGlmICh0aW1lci5ucy5zdG9wKSB7XG4gICAgICAgIGV4dGVuZCh0ZW1wbGF0ZSwge1xuICAgICAgICAgICdiZWdpbic6ICdzJD1uJC5zdGFydCgpJyxcbiAgICAgICAgICAnZW5kJzogJ3IkPW4kLm1pY3Jvc2Vjb25kcygpLzFlNidcbiAgICAgICAgfSk7XG4gICAgICB9IGVsc2UgaWYgKHBlcmZOYW1lKSB7XG4gICAgICAgIGV4dGVuZCh0ZW1wbGF0ZSwge1xuICAgICAgICAgICdiZWdpbic6ICdzJD1uJC4nICsgcGVyZk5hbWUgKyAnKCknLFxuICAgICAgICAgICdlbmQnOiAnciQ9KG4kLicgKyBwZXJmTmFtZSArICcoKS1zJCkvMWUzJ1xuICAgICAgICB9KTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIGV4dGVuZCh0ZW1wbGF0ZSwge1xuICAgICAgICAgICdiZWdpbic6ICdzJD1uJCgpJyxcbiAgICAgICAgICAnZW5kJzogJ3IkPShuJCgpLXMkKS8xZTYnXG4gICAgICAgIH0pO1xuICAgICAgfVxuICAgIH1cblxuICAgIC8vIGRlZmluZSBgdGltZXJgIG1ldGhvZHNcbiAgICB0aW1lci5zdGFydCA9IGNyZWF0ZUZ1bmN0aW9uKHByZXByb2Nlc3MoJ28kJyksXG4gICAgICBwcmVwcm9jZXNzKCd2YXIgbiQ9dGhpcy5ucywje2JlZ2lufTtvJC5lbGFwc2VkPTA7byQudGltZVN0YW1wPXMkJykpO1xuXG4gICAgdGltZXIuc3RvcCA9IGNyZWF0ZUZ1bmN0aW9uKHByZXByb2Nlc3MoJ28kJyksXG4gICAgICBwcmVwcm9jZXNzKCd2YXIgbiQ9dGhpcy5ucyxzJD1vJC50aW1lU3RhbXAsI3tlbmR9O28kLmVsYXBzZWQ9ciQnKSk7XG5cbiAgICAvLyByZXNvbHZlIHRpbWUgc3BhbiByZXF1aXJlZCB0byBhY2hpZXZlIGEgcGVyY2VudCB1bmNlcnRhaW50eSBvZiBhdCBtb3N0IDElXG4gICAgLy8gaHR0cDovL3NwaWZmLnJpdC5lZHUvY2xhc3Nlcy9waHlzMjczL3VuY2VydC91bmNlcnQuaHRtbFxuICAgIG9wdGlvbnMubWluVGltZSB8fCAob3B0aW9ucy5taW5UaW1lID0gbWF4KHRpbWVyLnJlcyAvIDIgLyAwLjAxLCAwLjA1KSk7XG4gICAgcmV0dXJuIGNsb2NrLmFwcGx5KG51bGwsIGFyZ3VtZW50cyk7XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogQ29tcHV0ZXMgc3RhdHMgb24gYmVuY2htYXJrIHJlc3VsdHMuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBiZW5jaCBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKiBAcGFyYW0ge09iamVjdH0gb3B0aW9ucyBUaGUgb3B0aW9ucyBvYmplY3QuXG4gICAqL1xuICBmdW5jdGlvbiBjb21wdXRlKGJlbmNoLCBvcHRpb25zKSB7XG4gICAgb3B0aW9ucyB8fCAob3B0aW9ucyA9IHt9KTtcblxuICAgIHZhciBhc3luYyA9IG9wdGlvbnMuYXN5bmMsXG4gICAgICAgIGVsYXBzZWQgPSAwLFxuICAgICAgICBpbml0Q291bnQgPSBiZW5jaC5pbml0Q291bnQsXG4gICAgICAgIG1pblNhbXBsZXMgPSBiZW5jaC5taW5TYW1wbGVzLFxuICAgICAgICBxdWV1ZSA9IFtdLFxuICAgICAgICBzYW1wbGUgPSBiZW5jaC5zdGF0cy5zYW1wbGU7XG5cbiAgICAvKipcbiAgICAgKiBBZGRzIGEgY2xvbmUgdG8gdGhlIHF1ZXVlLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIGVucXVldWUoKSB7XG4gICAgICBxdWV1ZS5wdXNoKGJlbmNoLmNsb25lKHtcbiAgICAgICAgJ19vcmlnaW5hbCc6IGJlbmNoLFxuICAgICAgICAnZXZlbnRzJzoge1xuICAgICAgICAgICdhYm9ydCc6IFt1cGRhdGVdLFxuICAgICAgICAgICdjeWNsZSc6IFt1cGRhdGVdLFxuICAgICAgICAgICdlcnJvcic6IFt1cGRhdGVdLFxuICAgICAgICAgICdzdGFydCc6IFt1cGRhdGVdXG4gICAgICAgIH1cbiAgICAgIH0pKTtcbiAgICB9XG5cbiAgICAvKipcbiAgICAgKiBVcGRhdGVzIHRoZSBjbG9uZS9vcmlnaW5hbCBiZW5jaG1hcmtzIHRvIGtlZXAgdGhlaXIgZGF0YSBpbiBzeW5jLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIHVwZGF0ZShldmVudCkge1xuICAgICAgdmFyIGNsb25lID0gdGhpcyxcbiAgICAgICAgICB0eXBlID0gZXZlbnQudHlwZTtcblxuICAgICAgaWYgKGJlbmNoLnJ1bm5pbmcpIHtcbiAgICAgICAgaWYgKHR5cGUgPT0gJ3N0YXJ0Jykge1xuICAgICAgICAgIC8vIE5vdGU6IGBjbG9uZS5taW5UaW1lYCBwcm9wIGlzIGluaXRlZCBpbiBgY2xvY2soKWBcbiAgICAgICAgICBjbG9uZS5jb3VudCA9IGJlbmNoLmluaXRDb3VudDtcbiAgICAgICAgfVxuICAgICAgICBlbHNlIHtcbiAgICAgICAgICBpZiAodHlwZSA9PSAnZXJyb3InKSB7XG4gICAgICAgICAgICBiZW5jaC5lcnJvciA9IGNsb25lLmVycm9yO1xuICAgICAgICAgIH1cbiAgICAgICAgICBpZiAodHlwZSA9PSAnYWJvcnQnKSB7XG4gICAgICAgICAgICBiZW5jaC5hYm9ydCgpO1xuICAgICAgICAgICAgYmVuY2guZW1pdCgnY3ljbGUnKTtcbiAgICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgICAgZXZlbnQuY3VycmVudFRhcmdldCA9IGV2ZW50LnRhcmdldCA9IGJlbmNoO1xuICAgICAgICAgICAgYmVuY2guZW1pdChldmVudCk7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9IGVsc2UgaWYgKGJlbmNoLmFib3J0ZWQpIHtcbiAgICAgICAgLy8gY2xlYXIgYWJvcnQgbGlzdGVuZXJzIHRvIGF2b2lkIHRyaWdnZXJpbmcgYmVuY2gncyBhYm9ydC9jeWNsZSBhZ2FpblxuICAgICAgICBjbG9uZS5ldmVudHMuYWJvcnQubGVuZ3RoID0gMDtcbiAgICAgICAgY2xvbmUuYWJvcnQoKTtcbiAgICAgIH1cbiAgICB9XG5cbiAgICAvKipcbiAgICAgKiBEZXRlcm1pbmVzIGlmIG1vcmUgY2xvbmVzIHNob3VsZCBiZSBxdWV1ZWQgb3IgaWYgY3ljbGluZyBzaG91bGQgc3RvcC5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiBldmFsdWF0ZShldmVudCkge1xuICAgICAgdmFyIGNyaXRpY2FsLFxuICAgICAgICAgIGRmLFxuICAgICAgICAgIG1lYW4sXG4gICAgICAgICAgbW9lLFxuICAgICAgICAgIHJtZSxcbiAgICAgICAgICBzZCxcbiAgICAgICAgICBzZW0sXG4gICAgICAgICAgdmFyaWFuY2UsXG4gICAgICAgICAgY2xvbmUgPSBldmVudC50YXJnZXQsXG4gICAgICAgICAgZG9uZSA9IGJlbmNoLmFib3J0ZWQsXG4gICAgICAgICAgbm93ID0gK25ldyBEYXRlLFxuICAgICAgICAgIHNpemUgPSBzYW1wbGUucHVzaChjbG9uZS50aW1lcy5wZXJpb2QpLFxuICAgICAgICAgIG1heGVkT3V0ID0gc2l6ZSA+PSBtaW5TYW1wbGVzICYmIChlbGFwc2VkICs9IG5vdyAtIGNsb25lLnRpbWVzLnRpbWVTdGFtcCkgLyAxZTMgPiBiZW5jaC5tYXhUaW1lLFxuICAgICAgICAgIHRpbWVzID0gYmVuY2gudGltZXMsXG4gICAgICAgICAgdmFyT2YgPSBmdW5jdGlvbihzdW0sIHgpIHsgcmV0dXJuIHN1bSArIHBvdyh4IC0gbWVhbiwgMik7IH07XG5cbiAgICAgIC8vIGV4aXQgZWFybHkgZm9yIGFib3J0ZWQgb3IgdW5jbG9ja2FibGUgdGVzdHNcbiAgICAgIGlmIChkb25lIHx8IGNsb25lLmh6ID09IEluZmluaXR5KSB7XG4gICAgICAgIG1heGVkT3V0ID0gIShzaXplID0gc2FtcGxlLmxlbmd0aCA9IHF1ZXVlLmxlbmd0aCA9IDApO1xuICAgICAgfVxuXG4gICAgICBpZiAoIWRvbmUpIHtcbiAgICAgICAgLy8gc2FtcGxlIG1lYW4gKGVzdGltYXRlIG9mIHRoZSBwb3B1bGF0aW9uIG1lYW4pXG4gICAgICAgIG1lYW4gPSBnZXRNZWFuKHNhbXBsZSk7XG4gICAgICAgIC8vIHNhbXBsZSB2YXJpYW5jZSAoZXN0aW1hdGUgb2YgdGhlIHBvcHVsYXRpb24gdmFyaWFuY2UpXG4gICAgICAgIHZhcmlhbmNlID0gcmVkdWNlKHNhbXBsZSwgdmFyT2YsIDApIC8gKHNpemUgLSAxKSB8fCAwO1xuICAgICAgICAvLyBzYW1wbGUgc3RhbmRhcmQgZGV2aWF0aW9uIChlc3RpbWF0ZSBvZiB0aGUgcG9wdWxhdGlvbiBzdGFuZGFyZCBkZXZpYXRpb24pXG4gICAgICAgIHNkID0gc3FydCh2YXJpYW5jZSk7XG4gICAgICAgIC8vIHN0YW5kYXJkIGVycm9yIG9mIHRoZSBtZWFuIChhLmsuYS4gdGhlIHN0YW5kYXJkIGRldmlhdGlvbiBvZiB0aGUgc2FtcGxpbmcgZGlzdHJpYnV0aW9uIG9mIHRoZSBzYW1wbGUgbWVhbilcbiAgICAgICAgc2VtID0gc2QgLyBzcXJ0KHNpemUpO1xuICAgICAgICAvLyBkZWdyZWVzIG9mIGZyZWVkb21cbiAgICAgICAgZGYgPSBzaXplIC0gMTtcbiAgICAgICAgLy8gY3JpdGljYWwgdmFsdWVcbiAgICAgICAgY3JpdGljYWwgPSB0VGFibGVbTWF0aC5yb3VuZChkZikgfHwgMV0gfHwgdFRhYmxlLmluZmluaXR5O1xuICAgICAgICAvLyBtYXJnaW4gb2YgZXJyb3JcbiAgICAgICAgbW9lID0gc2VtICogY3JpdGljYWw7XG4gICAgICAgIC8vIHJlbGF0aXZlIG1hcmdpbiBvZiBlcnJvclxuICAgICAgICBybWUgPSAobW9lIC8gbWVhbikgKiAxMDAgfHwgMDtcblxuICAgICAgICBleHRlbmQoYmVuY2guc3RhdHMsIHtcbiAgICAgICAgICAnZGV2aWF0aW9uJzogc2QsXG4gICAgICAgICAgJ21lYW4nOiBtZWFuLFxuICAgICAgICAgICdtb2UnOiBtb2UsXG4gICAgICAgICAgJ3JtZSc6IHJtZSxcbiAgICAgICAgICAnc2VtJzogc2VtLFxuICAgICAgICAgICd2YXJpYW5jZSc6IHZhcmlhbmNlXG4gICAgICAgIH0pO1xuXG4gICAgICAgIC8vIEFib3J0IHRoZSBjeWNsZSBsb29wIHdoZW4gdGhlIG1pbmltdW0gc2FtcGxlIHNpemUgaGFzIGJlZW4gY29sbGVjdGVkXG4gICAgICAgIC8vIGFuZCB0aGUgZWxhcHNlZCB0aW1lIGV4Y2VlZHMgdGhlIG1heGltdW0gdGltZSBhbGxvd2VkIHBlciBiZW5jaG1hcmsuXG4gICAgICAgIC8vIFdlIGRvbid0IGNvdW50IGN5Y2xlIGRlbGF5cyB0b3dhcmQgdGhlIG1heCB0aW1lIGJlY2F1c2UgZGVsYXlzIG1heSBiZVxuICAgICAgICAvLyBpbmNyZWFzZWQgYnkgYnJvd3NlcnMgdGhhdCBjbGFtcCB0aW1lb3V0cyBmb3IgaW5hY3RpdmUgdGFicy5cbiAgICAgICAgLy8gaHR0cHM6Ly9kZXZlbG9wZXIubW96aWxsYS5vcmcvZW4vd2luZG93LnNldFRpbWVvdXQjSW5hY3RpdmVfdGFic1xuICAgICAgICBpZiAobWF4ZWRPdXQpIHtcbiAgICAgICAgICAvLyByZXNldCB0aGUgYGluaXRDb3VudGAgaW4gY2FzZSB0aGUgYmVuY2htYXJrIGlzIHJlcnVuXG4gICAgICAgICAgYmVuY2guaW5pdENvdW50ID0gaW5pdENvdW50O1xuICAgICAgICAgIGJlbmNoLnJ1bm5pbmcgPSBmYWxzZTtcbiAgICAgICAgICBkb25lID0gdHJ1ZTtcbiAgICAgICAgICB0aW1lcy5lbGFwc2VkID0gKG5vdyAtIHRpbWVzLnRpbWVTdGFtcCkgLyAxZTM7XG4gICAgICAgIH1cbiAgICAgICAgaWYgKGJlbmNoLmh6ICE9IEluZmluaXR5KSB7XG4gICAgICAgICAgYmVuY2guaHogPSAxIC8gbWVhbjtcbiAgICAgICAgICB0aW1lcy5jeWNsZSA9IG1lYW4gKiBiZW5jaC5jb3VudDtcbiAgICAgICAgICB0aW1lcy5wZXJpb2QgPSBtZWFuO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgICAvLyBpZiB0aW1lIHBlcm1pdHMsIGluY3JlYXNlIHNhbXBsZSBzaXplIHRvIHJlZHVjZSB0aGUgbWFyZ2luIG9mIGVycm9yXG4gICAgICBpZiAocXVldWUubGVuZ3RoIDwgMiAmJiAhbWF4ZWRPdXQpIHtcbiAgICAgICAgZW5xdWV1ZSgpO1xuICAgICAgfVxuICAgICAgLy8gYWJvcnQgdGhlIGludm9rZSBjeWNsZSB3aGVuIGRvbmVcbiAgICAgIGV2ZW50LmFib3J0ZWQgPSBkb25lO1xuICAgIH1cblxuICAgIC8vIGluaXQgcXVldWUgYW5kIGJlZ2luXG4gICAgZW5xdWV1ZSgpO1xuICAgIGludm9rZShxdWV1ZSwge1xuICAgICAgJ25hbWUnOiAncnVuJyxcbiAgICAgICdhcmdzJzogeyAnYXN5bmMnOiBhc3luYyB9LFxuICAgICAgJ3F1ZXVlZCc6IHRydWUsXG4gICAgICAnb25DeWNsZSc6IGV2YWx1YXRlLFxuICAgICAgJ29uQ29tcGxldGUnOiBmdW5jdGlvbigpIHsgYmVuY2guZW1pdCgnY29tcGxldGUnKTsgfVxuICAgIH0pO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIEN5Y2xlcyBhIGJlbmNobWFyayB1bnRpbCBhIHJ1biBgY291bnRgIGNhbiBiZSBlc3RhYmxpc2hlZC5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IGNsb25lIFRoZSBjbG9uZWQgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKiBAcGFyYW0ge09iamVjdH0gb3B0aW9ucyBUaGUgb3B0aW9ucyBvYmplY3QuXG4gICAqL1xuICBmdW5jdGlvbiBjeWNsZShjbG9uZSwgb3B0aW9ucykge1xuICAgIG9wdGlvbnMgfHwgKG9wdGlvbnMgPSB7fSk7XG5cbiAgICB2YXIgZGVmZXJyZWQ7XG4gICAgaWYgKGNsb25lIGluc3RhbmNlb2YgRGVmZXJyZWQpIHtcbiAgICAgIGRlZmVycmVkID0gY2xvbmU7XG4gICAgICBjbG9uZSA9IGNsb25lLmJlbmNobWFyaztcbiAgICB9XG5cbiAgICB2YXIgY2xvY2tlZCxcbiAgICAgICAgY3ljbGVzLFxuICAgICAgICBkaXZpc29yLFxuICAgICAgICBldmVudCxcbiAgICAgICAgbWluVGltZSxcbiAgICAgICAgcGVyaW9kLFxuICAgICAgICBhc3luYyA9IG9wdGlvbnMuYXN5bmMsXG4gICAgICAgIGJlbmNoID0gY2xvbmUuX29yaWdpbmFsLFxuICAgICAgICBjb3VudCA9IGNsb25lLmNvdW50LFxuICAgICAgICB0aW1lcyA9IGNsb25lLnRpbWVzO1xuXG4gICAgLy8gY29udGludWUsIGlmIG5vdCBhYm9ydGVkIGJldHdlZW4gY3ljbGVzXG4gICAgaWYgKGNsb25lLnJ1bm5pbmcpIHtcbiAgICAgIC8vIGBtaW5UaW1lYCBpcyBzZXQgdG8gYEJlbmNobWFyay5vcHRpb25zLm1pblRpbWVgIGluIGBjbG9jaygpYFxuICAgICAgY3ljbGVzID0gKytjbG9uZS5jeWNsZXM7XG4gICAgICBjbG9ja2VkID0gZGVmZXJyZWQgPyBkZWZlcnJlZC5lbGFwc2VkIDogY2xvY2soY2xvbmUpO1xuICAgICAgbWluVGltZSA9IGNsb25lLm1pblRpbWU7XG5cbiAgICAgIGlmIChjeWNsZXMgPiBiZW5jaC5jeWNsZXMpIHtcbiAgICAgICAgYmVuY2guY3ljbGVzID0gY3ljbGVzO1xuICAgICAgfVxuICAgICAgaWYgKGNsb25lLmVycm9yKSB7XG4gICAgICAgIGV2ZW50ID0gRXZlbnQoJ2Vycm9yJyk7XG4gICAgICAgIGV2ZW50Lm1lc3NhZ2UgPSBjbG9uZS5lcnJvcjtcbiAgICAgICAgY2xvbmUuZW1pdChldmVudCk7XG4gICAgICAgIGlmICghZXZlbnQuY2FuY2VsbGVkKSB7XG4gICAgICAgICAgY2xvbmUuYWJvcnQoKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cblxuICAgIC8vIGNvbnRpbnVlLCBpZiBub3QgZXJyb3JlZFxuICAgIGlmIChjbG9uZS5ydW5uaW5nKSB7XG4gICAgICAvLyB0aW1lIHRha2VuIHRvIGNvbXBsZXRlIGxhc3QgdGVzdCBjeWNsZVxuICAgICAgYmVuY2gudGltZXMuY3ljbGUgPSB0aW1lcy5jeWNsZSA9IGNsb2NrZWQ7XG4gICAgICAvLyBzZWNvbmRzIHBlciBvcGVyYXRpb25cbiAgICAgIHBlcmlvZCA9IGJlbmNoLnRpbWVzLnBlcmlvZCA9IHRpbWVzLnBlcmlvZCA9IGNsb2NrZWQgLyBjb3VudDtcbiAgICAgIC8vIG9wcyBwZXIgc2Vjb25kXG4gICAgICBiZW5jaC5oeiA9IGNsb25lLmh6ID0gMSAvIHBlcmlvZDtcbiAgICAgIC8vIGF2b2lkIHdvcmtpbmcgb3VyIHdheSB1cCB0byB0aGlzIG5leHQgdGltZVxuICAgICAgYmVuY2guaW5pdENvdW50ID0gY2xvbmUuaW5pdENvdW50ID0gY291bnQ7XG4gICAgICAvLyBkbyB3ZSBuZWVkIHRvIGRvIGFub3RoZXIgY3ljbGU/XG4gICAgICBjbG9uZS5ydW5uaW5nID0gY2xvY2tlZCA8IG1pblRpbWU7XG5cbiAgICAgIGlmIChjbG9uZS5ydW5uaW5nKSB7XG4gICAgICAgIC8vIHRlc3RzIG1heSBjbG9jayBhdCBgMGAgd2hlbiBgaW5pdENvdW50YCBpcyBhIHNtYWxsIG51bWJlcixcbiAgICAgICAgLy8gdG8gYXZvaWQgdGhhdCB3ZSBzZXQgaXRzIGNvdW50IHRvIHNvbWV0aGluZyBhIGJpdCBoaWdoZXJcbiAgICAgICAgaWYgKCFjbG9ja2VkICYmIChkaXZpc29yID0gZGl2aXNvcnNbY2xvbmUuY3ljbGVzXSkgIT0gbnVsbCkge1xuICAgICAgICAgIGNvdW50ID0gZmxvb3IoNGU2IC8gZGl2aXNvcik7XG4gICAgICAgIH1cbiAgICAgICAgLy8gY2FsY3VsYXRlIGhvdyBtYW55IG1vcmUgaXRlcmF0aW9ucyBpdCB3aWxsIHRha2UgdG8gYWNoaXZlIHRoZSBgbWluVGltZWBcbiAgICAgICAgaWYgKGNvdW50IDw9IGNsb25lLmNvdW50KSB7XG4gICAgICAgICAgY291bnQgKz0gTWF0aC5jZWlsKChtaW5UaW1lIC0gY2xvY2tlZCkgLyBwZXJpb2QpO1xuICAgICAgICB9XG4gICAgICAgIGNsb25lLnJ1bm5pbmcgPSBjb3VudCAhPSBJbmZpbml0eTtcbiAgICAgIH1cbiAgICB9XG4gICAgLy8gc2hvdWxkIHdlIGV4aXQgZWFybHk/XG4gICAgZXZlbnQgPSBFdmVudCgnY3ljbGUnKTtcbiAgICBjbG9uZS5lbWl0KGV2ZW50KTtcbiAgICBpZiAoZXZlbnQuYWJvcnRlZCkge1xuICAgICAgY2xvbmUuYWJvcnQoKTtcbiAgICB9XG4gICAgLy8gZmlndXJlIG91dCB3aGF0IHRvIGRvIG5leHRcbiAgICBpZiAoY2xvbmUucnVubmluZykge1xuICAgICAgLy8gc3RhcnQgYSBuZXcgY3ljbGVcbiAgICAgIGNsb25lLmNvdW50ID0gY291bnQ7XG4gICAgICBpZiAoZGVmZXJyZWQpIHtcbiAgICAgICAgY2xvbmUuY29tcGlsZWQuY2FsbChkZWZlcnJlZCwgdGltZXIpO1xuICAgICAgfSBlbHNlIGlmIChhc3luYykge1xuICAgICAgICBkZWxheShjbG9uZSwgZnVuY3Rpb24oKSB7IGN5Y2xlKGNsb25lLCBvcHRpb25zKTsgfSk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBjeWNsZShjbG9uZSk7XG4gICAgICB9XG4gICAgfVxuICAgIGVsc2Uge1xuICAgICAgLy8gZml4IFRyYWNlTW9ua2V5IGJ1ZyBhc3NvY2lhdGVkIHdpdGggY2xvY2sgZmFsbGJhY2tzXG4gICAgICAvLyBodHRwOi8vYnVnemlsLmxhLzUwOTA2OVxuICAgICAgaWYgKHN1cHBvcnQuYnJvd3Nlcikge1xuICAgICAgICBydW5TY3JpcHQodWlkICsgJz0xO2RlbGV0ZSAnICsgdWlkKTtcbiAgICAgIH1cbiAgICAgIC8vIGRvbmVcbiAgICAgIGNsb25lLmVtaXQoJ2NvbXBsZXRlJyk7XG4gICAgfVxuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIFJ1bnMgdGhlIGJlbmNobWFyay5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge09iamVjdH0gW29wdGlvbnM9e31dIE9wdGlvbnMgb2JqZWN0LlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyBiYXNpYyB1c2FnZVxuICAgKiBiZW5jaC5ydW4oKTtcbiAgICpcbiAgICogLy8gb3Igd2l0aCBvcHRpb25zXG4gICAqIGJlbmNoLnJ1bih7ICdhc3luYyc6IHRydWUgfSk7XG4gICAqL1xuICBmdW5jdGlvbiBydW4ob3B0aW9ucykge1xuICAgIHZhciBtZSA9IHRoaXMsXG4gICAgICAgIGV2ZW50ID0gRXZlbnQoJ3N0YXJ0Jyk7XG5cbiAgICAvLyBzZXQgYHJ1bm5pbmdgIHRvIGBmYWxzZWAgc28gYHJlc2V0KClgIHdvbid0IGNhbGwgYGFib3J0KClgXG4gICAgbWUucnVubmluZyA9IGZhbHNlO1xuICAgIG1lLnJlc2V0KCk7XG4gICAgbWUucnVubmluZyA9IHRydWU7XG5cbiAgICBtZS5jb3VudCA9IG1lLmluaXRDb3VudDtcbiAgICBtZS50aW1lcy50aW1lU3RhbXAgPSArbmV3IERhdGU7XG4gICAgbWUuZW1pdChldmVudCk7XG5cbiAgICBpZiAoIWV2ZW50LmNhbmNlbGxlZCkge1xuICAgICAgb3B0aW9ucyA9IHsgJ2FzeW5jJzogKChvcHRpb25zID0gb3B0aW9ucyAmJiBvcHRpb25zLmFzeW5jKSA9PSBudWxsID8gbWUuYXN5bmMgOiBvcHRpb25zKSAmJiBzdXBwb3J0LnRpbWVvdXQgfTtcblxuICAgICAgLy8gZm9yIGNsb25lcyBjcmVhdGVkIHdpdGhpbiBgY29tcHV0ZSgpYFxuICAgICAgaWYgKG1lLl9vcmlnaW5hbCkge1xuICAgICAgICBpZiAobWUuZGVmZXIpIHtcbiAgICAgICAgICBEZWZlcnJlZChtZSk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgY3ljbGUobWUsIG9wdGlvbnMpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgICAvLyBmb3Igb3JpZ2luYWwgYmVuY2htYXJrc1xuICAgICAgZWxzZSB7XG4gICAgICAgIGNvbXB1dGUobWUsIG9wdGlvbnMpO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gbWU7XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvLyBGaXJlZm94IDEgZXJyb25lb3VzbHkgZGVmaW5lcyB2YXJpYWJsZSBhbmQgYXJndW1lbnQgbmFtZXMgb2YgZnVuY3Rpb25zIG9uXG4gIC8vIHRoZSBmdW5jdGlvbiBpdHNlbGYgYXMgbm9uLWNvbmZpZ3VyYWJsZSBwcm9wZXJ0aWVzIHdpdGggYHVuZGVmaW5lZGAgdmFsdWVzLlxuICAvLyBUaGUgYnVnZ2luZXNzIGNvbnRpbnVlcyBhcyB0aGUgYEJlbmNobWFya2AgY29uc3RydWN0b3IgaGFzIGFuIGFyZ3VtZW50XG4gIC8vIG5hbWVkIGBvcHRpb25zYCBhbmQgRmlyZWZveCAxIHdpbGwgbm90IGFzc2lnbiBhIHZhbHVlIHRvIGBCZW5jaG1hcmsub3B0aW9uc2AsXG4gIC8vIG1ha2luZyBpdCBub24td3JpdGFibGUgaW4gdGhlIHByb2Nlc3MsIHVubGVzcyBpdCBpcyB0aGUgZmlyc3QgcHJvcGVydHlcbiAgLy8gYXNzaWduZWQgYnkgZm9yLWluIGxvb3Agb2YgYGV4dGVuZCgpYC5cbiAgZXh0ZW5kKEJlbmNobWFyaywge1xuXG4gICAgLyoqXG4gICAgICogVGhlIGRlZmF1bHQgb3B0aW9ucyBjb3BpZWQgYnkgYmVuY2htYXJrIGluc3RhbmNlcy5cbiAgICAgKlxuICAgICAqIEBzdGF0aWNcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ29wdGlvbnMnOiB7XG5cbiAgICAgIC8qKlxuICAgICAgICogQSBmbGFnIHRvIGluZGljYXRlIHRoYXQgYmVuY2htYXJrIGN5Y2xlcyB3aWxsIGV4ZWN1dGUgYXN5bmNocm9ub3VzbHlcbiAgICAgICAqIGJ5IGRlZmF1bHQuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICAgKi9cbiAgICAgICdhc3luYyc6IGZhbHNlLFxuXG4gICAgICAvKipcbiAgICAgICAqIEEgZmxhZyB0byBpbmRpY2F0ZSB0aGF0IHRoZSBiZW5jaG1hcmsgY2xvY2sgaXMgZGVmZXJyZWQuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICAgKi9cbiAgICAgICdkZWZlcic6IGZhbHNlLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBkZWxheSBiZXR3ZWVuIHRlc3QgY3ljbGVzIChzZWNzKS5cbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdkZWxheSc6IDAuMDA1LFxuXG4gICAgICAvKipcbiAgICAgICAqIERpc3BsYXllZCBieSBCZW5jaG1hcmsjdG9TdHJpbmcgd2hlbiBhIGBuYW1lYCBpcyBub3QgYXZhaWxhYmxlXG4gICAgICAgKiAoYXV0by1nZW5lcmF0ZWQgaWYgYWJzZW50KS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIFN0cmluZ1xuICAgICAgICovXG4gICAgICAnaWQnOiB1bmRlZmluZWQsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIGRlZmF1bHQgbnVtYmVyIG9mIHRpbWVzIHRvIGV4ZWN1dGUgYSB0ZXN0IG9uIGEgYmVuY2htYXJrJ3MgZmlyc3QgY3ljbGUuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ2luaXRDb3VudCc6IDEsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG1heGltdW0gdGltZSBhIGJlbmNobWFyayBpcyBhbGxvd2VkIHRvIHJ1biBiZWZvcmUgZmluaXNoaW5nIChzZWNzKS5cbiAgICAgICAqIE5vdGU6IEN5Y2xlIGRlbGF5cyBhcmVuJ3QgY291bnRlZCB0b3dhcmQgdGhlIG1heGltdW0gdGltZS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAnbWF4VGltZSc6IDUsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG1pbmltdW0gc2FtcGxlIHNpemUgcmVxdWlyZWQgdG8gcGVyZm9ybSBzdGF0aXN0aWNhbCBhbmFseXNpcy5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAnbWluU2FtcGxlcyc6IDUsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIHRpbWUgbmVlZGVkIHRvIHJlZHVjZSB0aGUgcGVyY2VudCB1bmNlcnRhaW50eSBvZiBtZWFzdXJlbWVudCB0byAxJSAoc2VjcykuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ21pblRpbWUnOiAwLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBuYW1lIG9mIHRoZSBiZW5jaG1hcmsuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBTdHJpbmdcbiAgICAgICAqL1xuICAgICAgJ25hbWUnOiB1bmRlZmluZWQsXG5cbiAgICAgIC8qKlxuICAgICAgICogQW4gZXZlbnQgbGlzdGVuZXIgY2FsbGVkIHdoZW4gdGhlIGJlbmNobWFyayBpcyBhYm9ydGVkLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgRnVuY3Rpb25cbiAgICAgICAqL1xuICAgICAgJ29uQWJvcnQnOiB1bmRlZmluZWQsXG5cbiAgICAgIC8qKlxuICAgICAgICogQW4gZXZlbnQgbGlzdGVuZXIgY2FsbGVkIHdoZW4gdGhlIGJlbmNobWFyayBjb21wbGV0ZXMgcnVubmluZy5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIEZ1bmN0aW9uXG4gICAgICAgKi9cbiAgICAgICdvbkNvbXBsZXRlJzogdW5kZWZpbmVkLFxuXG4gICAgICAvKipcbiAgICAgICAqIEFuIGV2ZW50IGxpc3RlbmVyIGNhbGxlZCBhZnRlciBlYWNoIHJ1biBjeWNsZS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIEZ1bmN0aW9uXG4gICAgICAgKi9cbiAgICAgICdvbkN5Y2xlJzogdW5kZWZpbmVkLFxuXG4gICAgICAvKipcbiAgICAgICAqIEFuIGV2ZW50IGxpc3RlbmVyIGNhbGxlZCB3aGVuIGEgdGVzdCBlcnJvcnMuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBGdW5jdGlvblxuICAgICAgICovXG4gICAgICAnb25FcnJvcic6IHVuZGVmaW5lZCxcblxuICAgICAgLyoqXG4gICAgICAgKiBBbiBldmVudCBsaXN0ZW5lciBjYWxsZWQgd2hlbiB0aGUgYmVuY2htYXJrIGlzIHJlc2V0LlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgRnVuY3Rpb25cbiAgICAgICAqL1xuICAgICAgJ29uUmVzZXQnOiB1bmRlZmluZWQsXG5cbiAgICAgIC8qKlxuICAgICAgICogQW4gZXZlbnQgbGlzdGVuZXIgY2FsbGVkIHdoZW4gdGhlIGJlbmNobWFyayBzdGFydHMgcnVubmluZy5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIEZ1bmN0aW9uXG4gICAgICAgKi9cbiAgICAgICdvblN0YXJ0JzogdW5kZWZpbmVkXG4gICAgfSxcblxuICAgIC8qKlxuICAgICAqIFBsYXRmb3JtIG9iamVjdCB3aXRoIHByb3BlcnRpZXMgZGVzY3JpYmluZyB0aGluZ3MgbGlrZSBicm93c2VyIG5hbWUsXG4gICAgICogdmVyc2lvbiwgYW5kIG9wZXJhdGluZyBzeXN0ZW0uXG4gICAgICpcbiAgICAgKiBAc3RhdGljXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICdwbGF0Zm9ybSc6IHJlcSgncGxhdGZvcm0nKSB8fCB3aW5kb3cucGxhdGZvcm0gfHwge1xuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBwbGF0Zm9ybSBkZXNjcmlwdGlvbi5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBTdHJpbmdcbiAgICAgICAqL1xuICAgICAgJ2Rlc2NyaXB0aW9uJzogd2luZG93Lm5hdmlnYXRvciAmJiBuYXZpZ2F0b3IudXNlckFnZW50IHx8IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG5hbWUgb2YgdGhlIGJyb3dzZXIgbGF5b3V0IGVuZ2luZS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBTdHJpbmd8TnVsbFxuICAgICAgICovXG4gICAgICAnbGF5b3V0JzogbnVsbCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgbmFtZSBvZiB0aGUgcHJvZHVjdCBob3N0aW5nIHRoZSBicm93c2VyLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsucGxhdGZvcm1cbiAgICAgICAqIEB0eXBlIFN0cmluZ3xOdWxsXG4gICAgICAgKi9cbiAgICAgICdwcm9kdWN0JzogbnVsbCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgbmFtZSBvZiB0aGUgYnJvd3Nlci9lbnZpcm9ubWVudC5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBTdHJpbmd8TnVsbFxuICAgICAgICovXG4gICAgICAnbmFtZSc6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG5hbWUgb2YgdGhlIHByb2R1Y3QncyBtYW51ZmFjdHVyZXIuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nfE51bGxcbiAgICAgICAqL1xuICAgICAgJ21hbnVmYWN0dXJlcic6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG5hbWUgb2YgdGhlIG9wZXJhdGluZyBzeXN0ZW0uXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nfE51bGxcbiAgICAgICAqL1xuICAgICAgJ29zJzogbnVsbCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgYWxwaGEvYmV0YSByZWxlYXNlIGluZGljYXRvci5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBTdHJpbmd8TnVsbFxuICAgICAgICovXG4gICAgICAncHJlcmVsZWFzZSc6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIGJyb3dzZXIvZW52aXJvbm1lbnQgdmVyc2lvbi5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBTdHJpbmd8TnVsbFxuICAgICAgICovXG4gICAgICAndmVyc2lvbic6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogUmV0dXJuIHBsYXRmb3JtIGRlc2NyaXB0aW9uIHdoZW4gdGhlIHBsYXRmb3JtIG9iamVjdCBpcyBjb2VyY2VkIHRvIGEgc3RyaW5nLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsucGxhdGZvcm1cbiAgICAgICAqIEB0eXBlIEZ1bmN0aW9uXG4gICAgICAgKiBAcmV0dXJucyB7U3RyaW5nfSBUaGUgcGxhdGZvcm0gZGVzY3JpcHRpb24uXG4gICAgICAgKi9cbiAgICAgICd0b1N0cmluZyc6IGZ1bmN0aW9uKCkge1xuICAgICAgICByZXR1cm4gdGhpcy5kZXNjcmlwdGlvbiB8fCAnJztcbiAgICAgIH1cbiAgICB9LFxuXG4gICAgLyoqXG4gICAgICogVGhlIHNlbWFudGljIHZlcnNpb24gbnVtYmVyLlxuICAgICAqXG4gICAgICogQHN0YXRpY1xuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBTdHJpbmdcbiAgICAgKi9cbiAgICAndmVyc2lvbic6ICcxLjAuMCcsXG5cbiAgICAvLyBhbiBvYmplY3Qgb2YgZW52aXJvbm1lbnQvZmVhdHVyZSBkZXRlY3Rpb24gZmxhZ3NcbiAgICAnc3VwcG9ydCc6IHN1cHBvcnQsXG5cbiAgICAvLyBjbG9uZSBvYmplY3RzXG4gICAgJ2RlZXBDbG9uZSc6IGRlZXBDbG9uZSxcblxuICAgIC8vIGl0ZXJhdGlvbiB1dGlsaXR5XG4gICAgJ2VhY2gnOiBlYWNoLFxuXG4gICAgLy8gYXVnbWVudCBvYmplY3RzXG4gICAgJ2V4dGVuZCc6IGV4dGVuZCxcblxuICAgIC8vIGdlbmVyaWMgQXJyYXkjZmlsdGVyXG4gICAgJ2ZpbHRlcic6IGZpbHRlcixcblxuICAgIC8vIGdlbmVyaWMgQXJyYXkjZm9yRWFjaFxuICAgICdmb3JFYWNoJzogZm9yRWFjaCxcblxuICAgIC8vIGdlbmVyaWMgb3duIHByb3BlcnR5IGl0ZXJhdGlvbiB1dGlsaXR5XG4gICAgJ2Zvck93bic6IGZvck93bixcblxuICAgIC8vIGNvbnZlcnRzIGEgbnVtYmVyIHRvIGEgY29tbWEtc2VwYXJhdGVkIHN0cmluZ1xuICAgICdmb3JtYXROdW1iZXInOiBmb3JtYXROdW1iZXIsXG5cbiAgICAvLyBnZW5lcmljIE9iamVjdCNoYXNPd25Qcm9wZXJ0eVxuICAgIC8vICh0cmlnZ2VyIGhhc0tleSdzIGxhenkgZGVmaW5lIGJlZm9yZSBhc3NpZ25pbmcgaXQgdG8gQmVuY2htYXJrKVxuICAgICdoYXNLZXknOiAoaGFzS2V5KEJlbmNobWFyaywgJycpLCBoYXNLZXkpLFxuXG4gICAgLy8gZ2VuZXJpYyBBcnJheSNpbmRleE9mXG4gICAgJ2luZGV4T2YnOiBpbmRleE9mLFxuXG4gICAgLy8gdGVtcGxhdGUgdXRpbGl0eVxuICAgICdpbnRlcnBvbGF0ZSc6IGludGVycG9sYXRlLFxuXG4gICAgLy8gaW52b2tlcyBhIG1ldGhvZCBvbiBlYWNoIGl0ZW0gaW4gYW4gYXJyYXlcbiAgICAnaW52b2tlJzogaW52b2tlLFxuXG4gICAgLy8gZ2VuZXJpYyBBcnJheSNqb2luIGZvciBhcnJheXMgYW5kIG9iamVjdHNcbiAgICAnam9pbic6IGpvaW4sXG5cbiAgICAvLyBnZW5lcmljIEFycmF5I21hcFxuICAgICdtYXAnOiBtYXAsXG5cbiAgICAvLyByZXRyaWV2ZXMgYSBwcm9wZXJ0eSB2YWx1ZSBmcm9tIGVhY2ggaXRlbSBpbiBhbiBhcnJheVxuICAgICdwbHVjayc6IHBsdWNrLFxuXG4gICAgLy8gZ2VuZXJpYyBBcnJheSNyZWR1Y2VcbiAgICAncmVkdWNlJzogcmVkdWNlXG4gIH0pO1xuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIGV4dGVuZChCZW5jaG1hcmsucHJvdG90eXBlLCB7XG5cbiAgICAvKipcbiAgICAgKiBUaGUgbnVtYmVyIG9mIHRpbWVzIGEgdGVzdCB3YXMgZXhlY3V0ZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ2NvdW50JzogMCxcblxuICAgIC8qKlxuICAgICAqIFRoZSBudW1iZXIgb2YgY3ljbGVzIHBlcmZvcm1lZCB3aGlsZSBiZW5jaG1hcmtpbmcuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ2N5Y2xlcyc6IDAsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgbnVtYmVyIG9mIGV4ZWN1dGlvbnMgcGVyIHNlY29uZC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgKi9cbiAgICAnaHonOiAwLFxuXG4gICAgLyoqXG4gICAgICogVGhlIGNvbXBpbGVkIHRlc3QgZnVuY3Rpb24uXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgRnVuY3Rpb258U3RyaW5nXG4gICAgICovXG4gICAgJ2NvbXBpbGVkJzogdW5kZWZpbmVkLFxuXG4gICAgLyoqXG4gICAgICogVGhlIGVycm9yIG9iamVjdCBpZiB0aGUgdGVzdCBmYWlsZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ2Vycm9yJzogdW5kZWZpbmVkLFxuXG4gICAgLyoqXG4gICAgICogVGhlIHRlc3QgdG8gYmVuY2htYXJrLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIEZ1bmN0aW9ufFN0cmluZ1xuICAgICAqL1xuICAgICdmbic6IHVuZGVmaW5lZCxcblxuICAgIC8qKlxuICAgICAqIEEgZmxhZyB0byBpbmRpY2F0ZSBpZiB0aGUgYmVuY2htYXJrIGlzIGFib3J0ZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgICdhYm9ydGVkJzogZmFsc2UsXG5cbiAgICAvKipcbiAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgaWYgdGhlIGJlbmNobWFyayBpcyBydW5uaW5nLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICAncnVubmluZyc6IGZhbHNlLFxuXG4gICAgLyoqXG4gICAgICogQ29tcGlsZWQgaW50byB0aGUgdGVzdCBhbmQgZXhlY3V0ZWQgaW1tZWRpYXRlbHkgKipiZWZvcmUqKiB0aGUgdGVzdCBsb29wLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIEZ1bmN0aW9ufFN0cmluZ1xuICAgICAqIEBleGFtcGxlXG4gICAgICpcbiAgICAgKiAvLyBiYXNpYyB1c2FnZVxuICAgICAqIHZhciBiZW5jaCA9IEJlbmNobWFyayh7XG4gICAgICogICAnc2V0dXAnOiBmdW5jdGlvbigpIHtcbiAgICAgKiAgICAgdmFyIGMgPSB0aGlzLmNvdW50LFxuICAgICAqICAgICAgICAgZWxlbWVudCA9IGRvY3VtZW50LmdldEVsZW1lbnRCeUlkKCdjb250YWluZXInKTtcbiAgICAgKiAgICAgd2hpbGUgKGMtLSkge1xuICAgICAqICAgICAgIGVsZW1lbnQuYXBwZW5kQ2hpbGQoZG9jdW1lbnQuY3JlYXRlRWxlbWVudCgnZGl2JykpO1xuICAgICAqICAgICB9XG4gICAgICogICB9LFxuICAgICAqICAgJ2ZuJzogZnVuY3Rpb24oKSB7XG4gICAgICogICAgIGVsZW1lbnQucmVtb3ZlQ2hpbGQoZWxlbWVudC5sYXN0Q2hpbGQpO1xuICAgICAqICAgfVxuICAgICAqIH0pO1xuICAgICAqXG4gICAgICogLy8gY29tcGlsZXMgdG8gc29tZXRoaW5nIGxpa2U6XG4gICAgICogdmFyIGMgPSB0aGlzLmNvdW50LFxuICAgICAqICAgICBlbGVtZW50ID0gZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ2NvbnRhaW5lcicpO1xuICAgICAqIHdoaWxlIChjLS0pIHtcbiAgICAgKiAgIGVsZW1lbnQuYXBwZW5kQ2hpbGQoZG9jdW1lbnQuY3JlYXRlRWxlbWVudCgnZGl2JykpO1xuICAgICAqIH1cbiAgICAgKiB2YXIgc3RhcnQgPSBuZXcgRGF0ZTtcbiAgICAgKiB3aGlsZSAoY291bnQtLSkge1xuICAgICAqICAgZWxlbWVudC5yZW1vdmVDaGlsZChlbGVtZW50Lmxhc3RDaGlsZCk7XG4gICAgICogfVxuICAgICAqIHZhciBlbmQgPSBuZXcgRGF0ZSAtIHN0YXJ0O1xuICAgICAqXG4gICAgICogLy8gb3IgdXNpbmcgc3RyaW5nc1xuICAgICAqIHZhciBiZW5jaCA9IEJlbmNobWFyayh7XG4gICAgICogICAnc2V0dXAnOiAnXFxcbiAgICAgKiAgICAgdmFyIGEgPSAwO1xcblxcXG4gICAgICogICAgIChmdW5jdGlvbigpIHtcXG5cXFxuICAgICAqICAgICAgIChmdW5jdGlvbigpIHtcXG5cXFxuICAgICAqICAgICAgICAgKGZ1bmN0aW9uKCkgeycsXG4gICAgICogICAnZm4nOiAnYSArPSAxOycsXG4gICAgICogICAndGVhcmRvd24nOiAnXFxcbiAgICAgKiAgICAgICAgICB9KCkpXFxuXFxcbiAgICAgKiAgICAgICAgfSgpKVxcblxcXG4gICAgICogICAgICB9KCkpJ1xuICAgICAqIH0pO1xuICAgICAqXG4gICAgICogLy8gY29tcGlsZXMgdG8gc29tZXRoaW5nIGxpa2U6XG4gICAgICogdmFyIGEgPSAwO1xuICAgICAqIChmdW5jdGlvbigpIHtcbiAgICAgKiAgIChmdW5jdGlvbigpIHtcbiAgICAgKiAgICAgKGZ1bmN0aW9uKCkge1xuICAgICAqICAgICAgIHZhciBzdGFydCA9IG5ldyBEYXRlO1xuICAgICAqICAgICAgIHdoaWxlIChjb3VudC0tKSB7XG4gICAgICogICAgICAgICBhICs9IDE7XG4gICAgICogICAgICAgfVxuICAgICAqICAgICAgIHZhciBlbmQgPSBuZXcgRGF0ZSAtIHN0YXJ0O1xuICAgICAqICAgICB9KCkpXG4gICAgICogICB9KCkpXG4gICAgICogfSgpKVxuICAgICAqL1xuICAgICdzZXR1cCc6IG5vb3AsXG5cbiAgICAvKipcbiAgICAgKiBDb21waWxlZCBpbnRvIHRoZSB0ZXN0IGFuZCBleGVjdXRlZCBpbW1lZGlhdGVseSAqKmFmdGVyKiogdGhlIHRlc3QgbG9vcC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBGdW5jdGlvbnxTdHJpbmdcbiAgICAgKi9cbiAgICAndGVhcmRvd24nOiBub29wLFxuXG4gICAgLyoqXG4gICAgICogQW4gb2JqZWN0IG9mIHN0YXRzIGluY2x1ZGluZyBtZWFuLCBtYXJnaW4gb3IgZXJyb3IsIGFuZCBzdGFuZGFyZCBkZXZpYXRpb24uXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ3N0YXRzJzoge1xuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBtYXJnaW4gb2YgZXJyb3IuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayNzdGF0c1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdtb2UnOiAwLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSByZWxhdGl2ZSBtYXJnaW4gb2YgZXJyb3IgKGV4cHJlc3NlZCBhcyBhIHBlcmNlbnRhZ2Ugb2YgdGhlIG1lYW4pLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjc3RhdHNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAncm1lJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgc3RhbmRhcmQgZXJyb3Igb2YgdGhlIG1lYW4uXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayNzdGF0c1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdzZW0nOiAwLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBzYW1wbGUgc3RhbmRhcmQgZGV2aWF0aW9uLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjc3RhdHNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAnZGV2aWF0aW9uJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgc2FtcGxlIGFyaXRobWV0aWMgbWVhbi5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrI3N0YXRzXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ21lYW4nOiAwLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBhcnJheSBvZiBzYW1wbGVkIHBlcmlvZHMuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayNzdGF0c1xuICAgICAgICogQHR5cGUgQXJyYXlcbiAgICAgICAqL1xuICAgICAgJ3NhbXBsZSc6IFtdLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBzYW1wbGUgdmFyaWFuY2UuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayNzdGF0c1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICd2YXJpYW5jZSc6IDBcbiAgICB9LFxuXG4gICAgLyoqXG4gICAgICogQW4gb2JqZWN0IG9mIHRpbWluZyBkYXRhIGluY2x1ZGluZyBjeWNsZSwgZWxhcHNlZCwgcGVyaW9kLCBzdGFydCwgYW5kIHN0b3AuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ3RpbWVzJzoge1xuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSB0aW1lIHRha2VuIHRvIGNvbXBsZXRlIHRoZSBsYXN0IGN5Y2xlIChzZWNzKS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrI3RpbWVzXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ2N5Y2xlJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgdGltZSB0YWtlbiB0byBjb21wbGV0ZSB0aGUgYmVuY2htYXJrIChzZWNzKS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrI3RpbWVzXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ2VsYXBzZWQnOiAwLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSB0aW1lIHRha2VuIHRvIGV4ZWN1dGUgdGhlIHRlc3Qgb25jZSAoc2VjcykuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayN0aW1lc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdwZXJpb2QnOiAwLFxuXG4gICAgICAvKipcbiAgICAgICAqIEEgdGltZXN0YW1wIG9mIHdoZW4gdGhlIGJlbmNobWFyayBzdGFydGVkIChtcykuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayN0aW1lc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICd0aW1lU3RhbXAnOiAwXG4gICAgfSxcblxuICAgIC8vIGFib3J0cyBiZW5jaG1hcmsgKGRvZXMgbm90IHJlY29yZCB0aW1lcylcbiAgICAnYWJvcnQnOiBhYm9ydCxcblxuICAgIC8vIGNyZWF0ZXMgYSBuZXcgYmVuY2htYXJrIHVzaW5nIHRoZSBzYW1lIHRlc3QgYW5kIG9wdGlvbnNcbiAgICAnY2xvbmUnOiBjbG9uZSxcblxuICAgIC8vIGNvbXBhcmVzIGJlbmNobWFyaydzIGhlcnR6IHdpdGggYW5vdGhlclxuICAgICdjb21wYXJlJzogY29tcGFyZSxcblxuICAgIC8vIGV4ZWN1dGVzIGxpc3RlbmVyc1xuICAgICdlbWl0JzogZW1pdCxcblxuICAgIC8vIGdldCBsaXN0ZW5lcnNcbiAgICAnbGlzdGVuZXJzJzogbGlzdGVuZXJzLFxuXG4gICAgLy8gdW5yZWdpc3RlciBsaXN0ZW5lcnNcbiAgICAnb2ZmJzogb2ZmLFxuXG4gICAgLy8gcmVnaXN0ZXIgbGlzdGVuZXJzXG4gICAgJ29uJzogb24sXG5cbiAgICAvLyByZXNldCBiZW5jaG1hcmsgcHJvcGVydGllc1xuICAgICdyZXNldCc6IHJlc2V0LFxuXG4gICAgLy8gcnVucyB0aGUgYmVuY2htYXJrXG4gICAgJ3J1bic6IHJ1bixcblxuICAgIC8vIHByZXR0eSBwcmludCBiZW5jaG1hcmsgaW5mb1xuICAgICd0b1N0cmluZyc6IHRvU3RyaW5nQmVuY2hcbiAgfSk7XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgZXh0ZW5kKERlZmVycmVkLnByb3RvdHlwZSwge1xuXG4gICAgLyoqXG4gICAgICogVGhlIGRlZmVycmVkIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuRGVmZXJyZWRcbiAgICAgKiBAdHlwZSBPYmplY3RcbiAgICAgKi9cbiAgICAnYmVuY2htYXJrJzogbnVsbCxcblxuICAgIC8qKlxuICAgICAqIFRoZSBudW1iZXIgb2YgZGVmZXJyZWQgY3ljbGVzIHBlcmZvcm1lZCB3aGlsZSBiZW5jaG1hcmtpbmcuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkRlZmVycmVkXG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ2N5Y2xlcyc6IDAsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgdGltZSB0YWtlbiB0byBjb21wbGV0ZSB0aGUgZGVmZXJyZWQgYmVuY2htYXJrIChzZWNzKS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuRGVmZXJyZWRcbiAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgKi9cbiAgICAnZWxhcHNlZCc6IDAsXG5cbiAgICAvKipcbiAgICAgKiBBIHRpbWVzdGFtcCBvZiB3aGVuIHRoZSBkZWZlcnJlZCBiZW5jaG1hcmsgc3RhcnRlZCAobXMpLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5EZWZlcnJlZFxuICAgICAqIEB0eXBlIE51bWJlclxuICAgICAqL1xuICAgICd0aW1lU3RhbXAnOiAwLFxuXG4gICAgLy8gY3ljbGVzL2NvbXBsZXRlcyB0aGUgZGVmZXJyZWQgYmVuY2htYXJrXG4gICAgJ3Jlc29sdmUnOiByZXNvbHZlXG4gIH0pO1xuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIGV4dGVuZChFdmVudC5wcm90b3R5cGUsIHtcblxuICAgIC8qKlxuICAgICAqIEEgZmxhZyB0byBpbmRpY2F0ZSBpZiB0aGUgZW1pdHRlcnMgbGlzdGVuZXIgaXRlcmF0aW9uIGlzIGFib3J0ZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkV2ZW50XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgICdhYm9ydGVkJzogZmFsc2UsXG5cbiAgICAvKipcbiAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgaWYgdGhlIGRlZmF1bHQgYWN0aW9uIGlzIGNhbmNlbGxlZC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuRXZlbnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgJ2NhbmNlbGxlZCc6IGZhbHNlLFxuXG4gICAgLyoqXG4gICAgICogVGhlIG9iamVjdCB3aG9zZSBsaXN0ZW5lcnMgYXJlIGN1cnJlbnRseSBiZWluZyBwcm9jZXNzZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkV2ZW50XG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ2N1cnJlbnRUYXJnZXQnOiB1bmRlZmluZWQsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgcmV0dXJuIHZhbHVlIG9mIHRoZSBsYXN0IGV4ZWN1dGVkIGxpc3RlbmVyLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5FdmVudFxuICAgICAqIEB0eXBlIE1peGVkXG4gICAgICovXG4gICAgJ3Jlc3VsdCc6IHVuZGVmaW5lZCxcblxuICAgIC8qKlxuICAgICAqIFRoZSBvYmplY3QgdG8gd2hpY2ggdGhlIGV2ZW50IHdhcyBvcmlnaW5hbGx5IGVtaXR0ZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkV2ZW50XG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ3RhcmdldCc6IHVuZGVmaW5lZCxcblxuICAgIC8qKlxuICAgICAqIEEgdGltZXN0YW1wIG9mIHdoZW4gdGhlIGV2ZW50IHdhcyBjcmVhdGVkIChtcykuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkV2ZW50XG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ3RpbWVTdGFtcCc6IDAsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgZXZlbnQgdHlwZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuRXZlbnRcbiAgICAgKiBAdHlwZSBTdHJpbmdcbiAgICAgKi9cbiAgICAndHlwZSc6ICcnXG4gIH0pO1xuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBUaGUgZGVmYXVsdCBvcHRpb25zIGNvcGllZCBieSBzdWl0ZSBpbnN0YW5jZXMuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAdHlwZSBPYmplY3RcbiAgICovXG4gIFN1aXRlLm9wdGlvbnMgPSB7XG5cbiAgICAvKipcbiAgICAgKiBUaGUgbmFtZSBvZiB0aGUgc3VpdGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlLm9wdGlvbnNcbiAgICAgKiBAdHlwZSBTdHJpbmdcbiAgICAgKi9cbiAgICAnbmFtZSc6IHVuZGVmaW5lZFxuICB9O1xuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIGV4dGVuZChTdWl0ZS5wcm90b3R5cGUsIHtcblxuICAgIC8qKlxuICAgICAqIFRoZSBudW1iZXIgb2YgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ2xlbmd0aCc6IDAsXG5cbiAgICAvKipcbiAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgaWYgdGhlIHN1aXRlIGlzIGFib3J0ZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgICdhYm9ydGVkJzogZmFsc2UsXG5cbiAgICAvKipcbiAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgaWYgdGhlIHN1aXRlIGlzIHJ1bm5pbmcuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgICdydW5uaW5nJzogZmFsc2UsXG5cbiAgICAvKipcbiAgICAgKiBBbiBgQXJyYXkjZm9yRWFjaGAgbGlrZSBtZXRob2QuXG4gICAgICogQ2FsbGJhY2tzIG1heSB0ZXJtaW5hdGUgdGhlIGxvb3AgYnkgZXhwbGljaXRseSByZXR1cm5pbmcgYGZhbHNlYC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAgICogQHJldHVybnMge09iamVjdH0gVGhlIHN1aXRlIGl0ZXJhdGVkIG92ZXIuXG4gICAgICovXG4gICAgJ2ZvckVhY2gnOiBtZXRob2RpemUoZm9yRWFjaCksXG5cbiAgICAvKipcbiAgICAgKiBBbiBgQXJyYXkjaW5kZXhPZmAgbGlrZSBtZXRob2QuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHBhcmFtIHtNaXhlZH0gdmFsdWUgVGhlIHZhbHVlIHRvIHNlYXJjaCBmb3IuXG4gICAgICogQHJldHVybnMge051bWJlcn0gVGhlIGluZGV4IG9mIHRoZSBtYXRjaGVkIHZhbHVlIG9yIGAtMWAuXG4gICAgICovXG4gICAgJ2luZGV4T2YnOiBtZXRob2RpemUoaW5kZXhPZiksXG5cbiAgICAvKipcbiAgICAgKiBJbnZva2VzIGEgbWV0aG9kIG9uIGFsbCBiZW5jaG1hcmtzIGluIHRoZSBzdWl0ZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcGFyYW0ge1N0cmluZ3xPYmplY3R9IG5hbWUgVGhlIG5hbWUgb2YgdGhlIG1ldGhvZCB0byBpbnZva2UgT1Igb3B0aW9ucyBvYmplY3QuXG4gICAgICogQHBhcmFtIHtNaXhlZH0gW2FyZzEsIGFyZzIsIC4uLl0gQXJndW1lbnRzIHRvIGludm9rZSB0aGUgbWV0aG9kIHdpdGguXG4gICAgICogQHJldHVybnMge0FycmF5fSBBIG5ldyBhcnJheSBvZiB2YWx1ZXMgcmV0dXJuZWQgZnJvbSBlYWNoIG1ldGhvZCBpbnZva2VkLlxuICAgICAqL1xuICAgICdpbnZva2UnOiBtZXRob2RpemUoaW52b2tlKSxcblxuICAgIC8qKlxuICAgICAqIENvbnZlcnRzIHRoZSBzdWl0ZSBvZiBiZW5jaG1hcmtzIHRvIGEgc3RyaW5nLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEBwYXJhbSB7U3RyaW5nfSBbc2VwYXJhdG9yPScsJ10gQSBzdHJpbmcgdG8gc2VwYXJhdGUgZWFjaCBlbGVtZW50IG9mIHRoZSBhcnJheS5cbiAgICAgKiBAcmV0dXJucyB7U3RyaW5nfSBUaGUgc3RyaW5nLlxuICAgICAqL1xuICAgICdqb2luJzogW10uam9pbixcblxuICAgIC8qKlxuICAgICAqIEFuIGBBcnJheSNtYXBgIGxpa2UgbWV0aG9kLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEBwYXJhbSB7RnVuY3Rpb259IGNhbGxiYWNrIFRoZSBmdW5jdGlvbiBjYWxsZWQgcGVyIGl0ZXJhdGlvbi5cbiAgICAgKiBAcmV0dXJucyB7QXJyYXl9IEEgbmV3IGFycmF5IG9mIHZhbHVlcyByZXR1cm5lZCBieSB0aGUgY2FsbGJhY2suXG4gICAgICovXG4gICAgJ21hcCc6IG1ldGhvZGl6ZShtYXApLFxuXG4gICAgLyoqXG4gICAgICogUmV0cmlldmVzIHRoZSB2YWx1ZSBvZiBhIHNwZWNpZmllZCBwcm9wZXJ0eSBmcm9tIGFsbCBiZW5jaG1hcmtzIGluIHRoZSBzdWl0ZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcGFyYW0ge1N0cmluZ30gcHJvcGVydHkgVGhlIHByb3BlcnR5IHRvIHBsdWNrLlxuICAgICAqIEByZXR1cm5zIHtBcnJheX0gQSBuZXcgYXJyYXkgb2YgcHJvcGVydHkgdmFsdWVzLlxuICAgICAqL1xuICAgICdwbHVjayc6IG1ldGhvZGl6ZShwbHVjayksXG5cbiAgICAvKipcbiAgICAgKiBSZW1vdmVzIHRoZSBsYXN0IGJlbmNobWFyayBmcm9tIHRoZSBzdWl0ZSBhbmQgcmV0dXJucyBpdC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcmV0dXJucyB7TWl4ZWR9IFRoZSByZW1vdmVkIGJlbmNobWFyay5cbiAgICAgKi9cbiAgICAncG9wJzogW10ucG9wLFxuXG4gICAgLyoqXG4gICAgICogQXBwZW5kcyBiZW5jaG1hcmtzIHRvIHRoZSBzdWl0ZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcmV0dXJucyB7TnVtYmVyfSBUaGUgc3VpdGUncyBuZXcgbGVuZ3RoLlxuICAgICAqL1xuICAgICdwdXNoJzogW10ucHVzaCxcblxuICAgIC8qKlxuICAgICAqIFNvcnRzIHRoZSBiZW5jaG1hcmtzIG9mIHRoZSBzdWl0ZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBbY29tcGFyZUZuPW51bGxdIEEgZnVuY3Rpb24gdGhhdCBkZWZpbmVzIHRoZSBzb3J0IG9yZGVyLlxuICAgICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBzb3J0ZWQgc3VpdGUuXG4gICAgICovXG4gICAgJ3NvcnQnOiBbXS5zb3J0LFxuXG4gICAgLyoqXG4gICAgICogQW4gYEFycmF5I3JlZHVjZWAgbGlrZSBtZXRob2QuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHBhcmFtIHtGdW5jdGlvbn0gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uIGNhbGxlZCBwZXIgaXRlcmF0aW9uLlxuICAgICAqIEBwYXJhbSB7TWl4ZWR9IGFjY3VtdWxhdG9yIEluaXRpYWwgdmFsdWUgb2YgdGhlIGFjY3VtdWxhdG9yLlxuICAgICAqIEByZXR1cm5zIHtNaXhlZH0gVGhlIGFjY3VtdWxhdG9yLlxuICAgICAqL1xuICAgICdyZWR1Y2UnOiBtZXRob2RpemUocmVkdWNlKSxcblxuICAgIC8vIGFib3J0cyBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGVcbiAgICAnYWJvcnQnOiBhYm9ydFN1aXRlLFxuXG4gICAgLy8gYWRkcyBhIGJlbmNobWFyayB0byB0aGUgc3VpdGVcbiAgICAnYWRkJzogYWRkLFxuXG4gICAgLy8gY3JlYXRlcyBhIG5ldyBzdWl0ZSB3aXRoIGNsb25lZCBiZW5jaG1hcmtzXG4gICAgJ2Nsb25lJzogY2xvbmVTdWl0ZSxcblxuICAgIC8vIGV4ZWN1dGVzIGxpc3RlbmVycyBvZiBhIHNwZWNpZmllZCB0eXBlXG4gICAgJ2VtaXQnOiBlbWl0LFxuXG4gICAgLy8gY3JlYXRlcyBhIG5ldyBzdWl0ZSBvZiBmaWx0ZXJlZCBiZW5jaG1hcmtzXG4gICAgJ2ZpbHRlcic6IGZpbHRlclN1aXRlLFxuXG4gICAgLy8gZ2V0IGxpc3RlbmVyc1xuICAgICdsaXN0ZW5lcnMnOiBsaXN0ZW5lcnMsXG5cbiAgICAvLyB1bnJlZ2lzdGVyIGxpc3RlbmVyc1xuICAgICdvZmYnOiBvZmYsXG5cbiAgIC8vIHJlZ2lzdGVyIGxpc3RlbmVyc1xuICAgICdvbic6IG9uLFxuXG4gICAgLy8gcmVzZXRzIGFsbCBiZW5jaG1hcmtzIGluIHRoZSBzdWl0ZVxuICAgICdyZXNldCc6IHJlc2V0U3VpdGUsXG5cbiAgICAvLyBydW5zIGFsbCBiZW5jaG1hcmtzIGluIHRoZSBzdWl0ZVxuICAgICdydW4nOiBydW5TdWl0ZSxcblxuICAgIC8vIGFycmF5IG1ldGhvZHNcbiAgICAnY29uY2F0JzogY29uY2F0LFxuXG4gICAgJ3JldmVyc2UnOiByZXZlcnNlLFxuXG4gICAgJ3NoaWZ0Jzogc2hpZnQsXG5cbiAgICAnc2xpY2UnOiBzbGljZSxcblxuICAgICdzcGxpY2UnOiBzcGxpY2UsXG5cbiAgICAndW5zaGlmdCc6IHVuc2hpZnRcbiAgfSk7XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLy8gZXhwb3NlIERlZmVycmVkLCBFdmVudCBhbmQgU3VpdGVcbiAgZXh0ZW5kKEJlbmNobWFyaywge1xuICAgICdEZWZlcnJlZCc6IERlZmVycmVkLFxuICAgICdFdmVudCc6IEV2ZW50LFxuICAgICdTdWl0ZSc6IFN1aXRlXG4gIH0pO1xuXG4gIC8vIGV4cG9zZSBCZW5jaG1hcmtcbiAgLy8gc29tZSBBTUQgYnVpbGQgb3B0aW1pemVycywgbGlrZSByLmpzLCBjaGVjayBmb3Igc3BlY2lmaWMgY29uZGl0aW9uIHBhdHRlcm5zIGxpa2UgdGhlIGZvbGxvd2luZzpcbiAgaWYgKHR5cGVvZiBkZWZpbmUgPT0gJ2Z1bmN0aW9uJyAmJiB0eXBlb2YgZGVmaW5lLmFtZCA9PSAnb2JqZWN0JyAmJiBkZWZpbmUuYW1kKSB7XG4gICAgLy8gZGVmaW5lIGFzIGFuIGFub255bW91cyBtb2R1bGUgc28sIHRocm91Z2ggcGF0aCBtYXBwaW5nLCBpdCBjYW4gYmUgYWxpYXNlZFxuICAgIGRlZmluZShmdW5jdGlvbigpIHtcbiAgICAgIHJldHVybiBCZW5jaG1hcms7XG4gICAgfSk7XG4gIH1cbiAgLy8gY2hlY2sgZm9yIGBleHBvcnRzYCBhZnRlciBgZGVmaW5lYCBpbiBjYXNlIGEgYnVpbGQgb3B0aW1pemVyIGFkZHMgYW4gYGV4cG9ydHNgIG9iamVjdFxuICBlbHNlIGlmIChmcmVlRXhwb3J0cykge1xuICAgIC8vIGluIE5vZGUuanMgb3IgUmluZ29KUyB2MC44LjArXG4gICAgaWYgKHR5cGVvZiBtb2R1bGUgPT0gJ29iamVjdCcgJiYgbW9kdWxlICYmIG1vZHVsZS5leHBvcnRzID09IGZyZWVFeHBvcnRzKSB7XG4gICAgICAobW9kdWxlLmV4cG9ydHMgPSBCZW5jaG1hcmspLkJlbmNobWFyayA9IEJlbmNobWFyaztcbiAgICB9XG4gICAgLy8gaW4gTmFyd2hhbCBvciBSaW5nb0pTIHYwLjcuMC1cbiAgICBlbHNlIHtcbiAgICAgIGZyZWVFeHBvcnRzLkJlbmNobWFyayA9IEJlbmNobWFyaztcbiAgICB9XG4gIH1cbiAgLy8gaW4gYSBicm93c2VyIG9yIFJoaW5vXG4gIGVsc2Uge1xuICAgIC8vIHVzZSBzcXVhcmUgYnJhY2tldCBub3RhdGlvbiBzbyBDbG9zdXJlIENvbXBpbGVyIHdvbid0IG11bmdlIGBCZW5jaG1hcmtgXG4gICAgLy8gaHR0cDovL2NvZGUuZ29vZ2xlLmNvbS9jbG9zdXJlL2NvbXBpbGVyL2RvY3MvYXBpLXR1dG9yaWFsMy5odG1sI2V4cG9ydFxuICAgIHdpbmRvd1snQmVuY2htYXJrJ10gPSBCZW5jaG1hcms7XG4gIH1cblxuICAvLyB0cmlnZ2VyIGNsb2NrJ3MgbGF6eSBkZWZpbmUgZWFybHkgdG8gYXZvaWQgYSBzZWN1cml0eSBlcnJvclxuICBpZiAoc3VwcG9ydC5haXIpIHtcbiAgICBjbG9jayh7ICdfb3JpZ2luYWwnOiB7ICdmbic6IG5vb3AsICdjb3VudCc6IDEsICdvcHRpb25zJzoge30gfSB9KTtcbiAgfVxufSh0aGlzKSk7XG4iLCIvLyBzaGltIGZvciB1c2luZyBwcm9jZXNzIGluIGJyb3dzZXJcblxudmFyIHByb2Nlc3MgPSBtb2R1bGUuZXhwb3J0cyA9IHt9O1xuXG5wcm9jZXNzLm5leHRUaWNrID0gKGZ1bmN0aW9uICgpIHtcbiAgICB2YXIgY2FuU2V0SW1tZWRpYXRlID0gdHlwZW9mIHdpbmRvdyAhPT0gJ3VuZGVmaW5lZCdcbiAgICAmJiB3aW5kb3cuc2V0SW1tZWRpYXRlO1xuICAgIHZhciBjYW5Qb3N0ID0gdHlwZW9mIHdpbmRvdyAhPT0gJ3VuZGVmaW5lZCdcbiAgICAmJiB3aW5kb3cucG9zdE1lc3NhZ2UgJiYgd2luZG93LmFkZEV2ZW50TGlzdGVuZXJcbiAgICA7XG5cbiAgICBpZiAoY2FuU2V0SW1tZWRpYXRlKSB7XG4gICAgICAgIHJldHVybiBmdW5jdGlvbiAoZikgeyByZXR1cm4gd2luZG93LnNldEltbWVkaWF0ZShmKSB9O1xuICAgIH1cblxuICAgIGlmIChjYW5Qb3N0KSB7XG4gICAgICAgIHZhciBxdWV1ZSA9IFtdO1xuICAgICAgICB3aW5kb3cuYWRkRXZlbnRMaXN0ZW5lcignbWVzc2FnZScsIGZ1bmN0aW9uIChldikge1xuICAgICAgICAgICAgaWYgKGV2LnNvdXJjZSA9PT0gd2luZG93ICYmIGV2LmRhdGEgPT09ICdwcm9jZXNzLXRpY2snKSB7XG4gICAgICAgICAgICAgICAgZXYuc3RvcFByb3BhZ2F0aW9uKCk7XG4gICAgICAgICAgICAgICAgaWYgKHF1ZXVlLmxlbmd0aCA+IDApIHtcbiAgICAgICAgICAgICAgICAgICAgdmFyIGZuID0gcXVldWUuc2hpZnQoKTtcbiAgICAgICAgICAgICAgICAgICAgZm4oKTtcbiAgICAgICAgICAgICAgICB9XG4gICAgICAgICAgICB9XG4gICAgICAgIH0sIHRydWUpO1xuXG4gICAgICAgIHJldHVybiBmdW5jdGlvbiBuZXh0VGljayhmbikge1xuICAgICAgICAgICAgcXVldWUucHVzaChmbik7XG4gICAgICAgICAgICB3aW5kb3cucG9zdE1lc3NhZ2UoJ3Byb2Nlc3MtdGljaycsICcqJyk7XG4gICAgICAgIH07XG4gICAgfVxuXG4gICAgcmV0dXJuIGZ1bmN0aW9uIG5leHRUaWNrKGZuKSB7XG4gICAgICAgIHNldFRpbWVvdXQoZm4sIDApO1xuICAgIH07XG59KSgpO1xuXG5wcm9jZXNzLnRpdGxlID0gJ2Jyb3dzZXInO1xucHJvY2Vzcy5icm93c2VyID0gdHJ1ZTtcbnByb2Nlc3MuZW52ID0ge307XG5wcm9jZXNzLmFyZ3YgPSBbXTtcblxucHJvY2Vzcy5iaW5kaW5nID0gZnVuY3Rpb24gKG5hbWUpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3Byb2Nlc3MuYmluZGluZyBpcyBub3Qgc3VwcG9ydGVkJyk7XG59XG5cbi8vIFRPRE8oc2h0eWxtYW4pXG5wcm9jZXNzLmN3ZCA9IGZ1bmN0aW9uICgpIHsgcmV0dXJuICcvJyB9O1xucHJvY2Vzcy5jaGRpciA9IGZ1bmN0aW9uIChkaXIpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3Byb2Nlc3MuY2hkaXIgaXMgbm90IHN1cHBvcnRlZCcpO1xufTtcbiIsImV4cG9ydHMucmVhZCA9IGZ1bmN0aW9uKGJ1ZmZlciwgb2Zmc2V0LCBpc0xFLCBtTGVuLCBuQnl0ZXMpIHtcbiAgdmFyIGUsIG0sXG4gICAgICBlTGVuID0gbkJ5dGVzICogOCAtIG1MZW4gLSAxLFxuICAgICAgZU1heCA9ICgxIDw8IGVMZW4pIC0gMSxcbiAgICAgIGVCaWFzID0gZU1heCA+PiAxLFxuICAgICAgbkJpdHMgPSAtNyxcbiAgICAgIGkgPSBpc0xFID8gKG5CeXRlcyAtIDEpIDogMCxcbiAgICAgIGQgPSBpc0xFID8gLTEgOiAxLFxuICAgICAgcyA9IGJ1ZmZlcltvZmZzZXQgKyBpXTtcblxuICBpICs9IGQ7XG5cbiAgZSA9IHMgJiAoKDEgPDwgKC1uQml0cykpIC0gMSk7XG4gIHMgPj49ICgtbkJpdHMpO1xuICBuQml0cyArPSBlTGVuO1xuICBmb3IgKDsgbkJpdHMgPiAwOyBlID0gZSAqIDI1NiArIGJ1ZmZlcltvZmZzZXQgKyBpXSwgaSArPSBkLCBuQml0cyAtPSA4KTtcblxuICBtID0gZSAmICgoMSA8PCAoLW5CaXRzKSkgLSAxKTtcbiAgZSA+Pj0gKC1uQml0cyk7XG4gIG5CaXRzICs9IG1MZW47XG4gIGZvciAoOyBuQml0cyA+IDA7IG0gPSBtICogMjU2ICsgYnVmZmVyW29mZnNldCArIGldLCBpICs9IGQsIG5CaXRzIC09IDgpO1xuXG4gIGlmIChlID09PSAwKSB7XG4gICAgZSA9IDEgLSBlQmlhcztcbiAgfSBlbHNlIGlmIChlID09PSBlTWF4KSB7XG4gICAgcmV0dXJuIG0gPyBOYU4gOiAoKHMgPyAtMSA6IDEpICogSW5maW5pdHkpO1xuICB9IGVsc2Uge1xuICAgIG0gPSBtICsgTWF0aC5wb3coMiwgbUxlbik7XG4gICAgZSA9IGUgLSBlQmlhcztcbiAgfVxuICByZXR1cm4gKHMgPyAtMSA6IDEpICogbSAqIE1hdGgucG93KDIsIGUgLSBtTGVuKTtcbn07XG5cbmV4cG9ydHMud3JpdGUgPSBmdW5jdGlvbihidWZmZXIsIHZhbHVlLCBvZmZzZXQsIGlzTEUsIG1MZW4sIG5CeXRlcykge1xuICB2YXIgZSwgbSwgYyxcbiAgICAgIGVMZW4gPSBuQnl0ZXMgKiA4IC0gbUxlbiAtIDEsXG4gICAgICBlTWF4ID0gKDEgPDwgZUxlbikgLSAxLFxuICAgICAgZUJpYXMgPSBlTWF4ID4+IDEsXG4gICAgICBydCA9IChtTGVuID09PSAyMyA/IE1hdGgucG93KDIsIC0yNCkgLSBNYXRoLnBvdygyLCAtNzcpIDogMCksXG4gICAgICBpID0gaXNMRSA/IDAgOiAobkJ5dGVzIC0gMSksXG4gICAgICBkID0gaXNMRSA/IDEgOiAtMSxcbiAgICAgIHMgPSB2YWx1ZSA8IDAgfHwgKHZhbHVlID09PSAwICYmIDEgLyB2YWx1ZSA8IDApID8gMSA6IDA7XG5cbiAgdmFsdWUgPSBNYXRoLmFicyh2YWx1ZSk7XG5cbiAgaWYgKGlzTmFOKHZhbHVlKSB8fCB2YWx1ZSA9PT0gSW5maW5pdHkpIHtcbiAgICBtID0gaXNOYU4odmFsdWUpID8gMSA6IDA7XG4gICAgZSA9IGVNYXg7XG4gIH0gZWxzZSB7XG4gICAgZSA9IE1hdGguZmxvb3IoTWF0aC5sb2codmFsdWUpIC8gTWF0aC5MTjIpO1xuICAgIGlmICh2YWx1ZSAqIChjID0gTWF0aC5wb3coMiwgLWUpKSA8IDEpIHtcbiAgICAgIGUtLTtcbiAgICAgIGMgKj0gMjtcbiAgICB9XG4gICAgaWYgKGUgKyBlQmlhcyA+PSAxKSB7XG4gICAgICB2YWx1ZSArPSBydCAvIGM7XG4gICAgfSBlbHNlIHtcbiAgICAgIHZhbHVlICs9IHJ0ICogTWF0aC5wb3coMiwgMSAtIGVCaWFzKTtcbiAgICB9XG4gICAgaWYgKHZhbHVlICogYyA+PSAyKSB7XG4gICAgICBlKys7XG4gICAgICBjIC89IDI7XG4gICAgfVxuXG4gICAgaWYgKGUgKyBlQmlhcyA+PSBlTWF4KSB7XG4gICAgICBtID0gMDtcbiAgICAgIGUgPSBlTWF4O1xuICAgIH0gZWxzZSBpZiAoZSArIGVCaWFzID49IDEpIHtcbiAgICAgIG0gPSAodmFsdWUgKiBjIC0gMSkgKiBNYXRoLnBvdygyLCBtTGVuKTtcbiAgICAgIGUgPSBlICsgZUJpYXM7XG4gICAgfSBlbHNlIHtcbiAgICAgIG0gPSB2YWx1ZSAqIE1hdGgucG93KDIsIGVCaWFzIC0gMSkgKiBNYXRoLnBvdygyLCBtTGVuKTtcbiAgICAgIGUgPSAwO1xuICAgIH1cbiAgfVxuXG4gIGZvciAoOyBtTGVuID49IDg7IGJ1ZmZlcltvZmZzZXQgKyBpXSA9IG0gJiAweGZmLCBpICs9IGQsIG0gLz0gMjU2LCBtTGVuIC09IDgpO1xuXG4gIGUgPSAoZSA8PCBtTGVuKSB8IG07XG4gIGVMZW4gKz0gbUxlbjtcbiAgZm9yICg7IGVMZW4gPiAwOyBidWZmZXJbb2Zmc2V0ICsgaV0gPSBlICYgMHhmZiwgaSArPSBkLCBlIC89IDI1NiwgZUxlbiAtPSA4KTtcblxuICBidWZmZXJbb2Zmc2V0ICsgaSAtIGRdIHw9IHMgKiAxMjg7XG59O1xuIiwidmFyIGdsb2JhbD10eXBlb2Ygc2VsZiAhPT0gXCJ1bmRlZmluZWRcIiA/IHNlbGYgOiB0eXBlb2Ygd2luZG93ICE9PSBcInVuZGVmaW5lZFwiID8gd2luZG93IDoge307dmFyIGJlbmNobWFyayA9IHJlcXVpcmUoJ2JlbmNobWFyaycpXG52YXIgc3VpdGUgPSBuZXcgYmVuY2htYXJrLlN1aXRlKClcblxuZ2xvYmFsLk5ld0J1ZmZlciA9IHJlcXVpcmUoJy4uLy4uLycpLkJ1ZmZlciAvLyBuYXRpdmUtYnVmZmVyLWJyb3dzZXJpZnlcblxudmFyIExFTkdUSCA9IDIwXG5cbnZhciBuZXdUYXJnZXQgPSBOZXdCdWZmZXIoTEVOR1RIICogNClcblxuZm9yICh2YXIgaSA9IDA7IGkgPCBMRU5HVEg7IGkrKykge1xuICBuZXdUYXJnZXQud3JpdGVVSW50MzJMRSg3MDAwICsgaSwgaSAqIDQpXG59XG5cbnN1aXRlLmFkZCgnTmV3QnVmZmVyI3JlYWRVSW50MzJMRScsIGZ1bmN0aW9uICgpIHtcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBMRU5HVEg7IGkrKykge1xuICAgIHZhciB4ID0gbmV3VGFyZ2V0LnJlYWRVSW50MzJMRShpICogNClcbiAgfVxufSlcbi5vbignZXJyb3InLCBmdW5jdGlvbiAoZXZlbnQpIHtcbiAgY29uc29sZS5lcnJvcihldmVudC50YXJnZXQuZXJyb3Iuc3RhY2spXG59KVxuLm9uKCdjeWNsZScsIGZ1bmN0aW9uIChldmVudCkge1xuICBjb25zb2xlLmxvZyhTdHJpbmcoZXZlbnQudGFyZ2V0KSlcbn0pXG5cbi5ydW4oeyAnYXN5bmMnOiB0cnVlIH0pXG4iXX0=
