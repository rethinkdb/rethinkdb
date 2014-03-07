(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);throw new Error("Cannot find module '"+o+"'")}var f=n[o]={exports:{}};t[o][0].call(f.exports,function(e){var n=t[o][1][e];return s(n?n:e)},f,f.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
require=(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);throw new Error("Cannot find module '"+o+"'")}var f=n[o]={exports:{}};t[o][0].call(f.exports,function(e){var n=t[o][1][e];return s(n?n:e)},f,f.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({"26sEV7":[function(require,module,exports){
var base64 = require('base64-js')
var TA = require('typedarray')

exports.Buffer = Buffer
exports.SlowBuffer = Buffer
exports.INSPECT_MAX_BYTES = 50
Buffer.poolSize = 8192

/**
 * Use a shim for browsers that lack Typed Array support (< IE 9, < FF 3.6,
 * < Chrome 6, < Safari 5, < Opera 11.5, < iOS 4.1).
 */
var xDataView = typeof DataView === 'undefined'
  ? TA.DataView : DataView
var xArrayBuffer = typeof ArrayBuffer === 'undefined'
  ? TA.ArrayBuffer : ArrayBuffer
var xUint8Array = typeof Uint8Array === 'undefined'
  ? TA.Uint8Array : Uint8Array

/**
 * Does the browser support adding properties to `Uint8Array` instances? If
 * not, then use the shim even if the browser supports typed arrays; we need
 * to be able to add all the node Buffer API methods. We could use an ES6
 * Proxy to intercept method calls and simulate adding function properties in
 * Firefox, but that adds a huge performance penalty.
 *
 * Firefox bug report: https://bugzilla.mozilla.org/show_bug.cgi?id=695438
 */
var supportsAugment = (function () {
  try {
    var arr = new xUint8Array(0)
    arr.foo = function () { return 42 }
    return 42 === arr.foo()
  } catch (e) {
    return false
  }
})()
if (!supportsAugment) {
  xDataView = TA.DataView
  xArrayBuffer = TA.ArrayBuffer
  xUint8Array = TA.Uint8Array
}

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
function Buffer (subject, encoding) {
  var type = typeof subject

  // Work-around: node's base64 implementation
  // allows for non-padded strings while base64-js
  // does not..
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

  var buf = augment(new xUint8Array(length))
  if (Buffer.isBuffer(subject)) {
    // Speed optimization -- use set if we're copying from a Uint8Array
    buf.set(subject)
  } else if (isArrayIsh(subject)) {
    // Treat array-ish objects as a byte array.
    for (var i = 0; i < length; i++) {
      if (Buffer.isBuffer(subject))
        buf[i] = subject.readUInt8(i)
      else
        buf[i] = subject[i]
    }
  } else if (type === 'string') {
    buf.write(subject, 0, encoding)
  }

  return buf
}

// STATIC METHODS
// ==============

Buffer.isEncoding = function(encoding) {
  switch ((encoding + '').toLowerCase()) {
    case 'hex':
    case 'utf8':
    case 'utf-8':
    case 'ascii':
    case 'binary':
    case 'base64':
    case 'ucs2':
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
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

  var i
  var buf

  if (list.length === 0) {
    return new Buffer(0)
  } else if (list.length === 1) {
    return list[0]
  }

  if (typeof totalLength !== 'number') {
    totalLength = 0
    for (i = 0; i < list.length; i++) {
      buf = list[i]
      totalLength += buf.length
    }
  }

  var buffer = new Buffer(totalLength)
  var pos = 0
  for (i = 0; i < list.length; i++) {
    buf = list[i]
    buf.copy(buffer, pos)
    pos += buf.length
  }
  return buffer
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

function BufferWrite (string, offset, length, encoding) {
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

function BufferToString (encoding, start, end) {
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

function BufferToJSON () {
  return {
    type: 'Buffer',
    data: Array.prototype.slice.call(this._arr || this, 0)
  }
}

// copy(targetBuffer, targetStart=0, sourceStart=0, sourceEnd=buffer.length)
function BufferCopy (target, target_start, start, end) {
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
function BufferSlice (start, end) {
  var len = this.length
  start = clamp(start, len, 0)
  end = clamp(end, len, len)
  return augment(this.subarray(start, end)) // Uint8Array built-in method
}

function BufferReadUInt8 (offset, noAssert) {
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
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setUint8(0, buf[len - 1])
    return dv.getUint16(0, littleEndian)
  } else {
    return buf._dataview.getUint16(offset, littleEndian)
  }
}

function BufferReadUInt16LE (offset, noAssert) {
  return _readUInt16(this, offset, true, noAssert)
}

function BufferReadUInt16BE (offset, noAssert) {
  return _readUInt16(this, offset, false, noAssert)
}

function _readUInt32 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    for (var i = 0; i + offset < len; i++) {
      dv.setUint8(i, buf[i + offset])
    }
    return dv.getUint32(0, littleEndian)
  } else {
    return buf._dataview.getUint32(offset, littleEndian)
  }
}

function BufferReadUInt32LE (offset, noAssert) {
  return _readUInt32(this, offset, true, noAssert)
}

function BufferReadUInt32BE (offset, noAssert) {
  return _readUInt32(this, offset, false, noAssert)
}

function BufferReadInt8 (offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(offset !== undefined && offset !== null,
        'missing offset')
    assert(offset < buf.length, 'Trying to read beyond buffer length')
  }

  if (offset >= buf.length)
    return

  return buf._dataview.getInt8(offset)
}

function _readInt16 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null,
        'missing offset')
    assert(offset + 1 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setUint8(0, buf[len - 1])
    return dv.getInt16(0, littleEndian)
  } else {
    return buf._dataview.getInt16(offset, littleEndian)
  }
}

function BufferReadInt16LE (offset, noAssert) {
  return _readInt16(this, offset, true, noAssert)
}

function BufferReadInt16BE (offset, noAssert) {
  return _readInt16(this, offset, false, noAssert)
}

function _readInt32 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    for (var i = 0; i + offset < len; i++) {
      dv.setUint8(i, buf[i + offset])
    }
    return dv.getInt32(0, littleEndian)
  } else {
    return buf._dataview.getInt32(offset, littleEndian)
  }
}

function BufferReadInt32LE (offset, noAssert) {
  return _readInt32(this, offset, true, noAssert)
}

function BufferReadInt32BE (offset, noAssert) {
  return _readInt32(this, offset, false, noAssert)
}

function _readFloat (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  return buf._dataview.getFloat32(offset, littleEndian)
}

function BufferReadFloatLE (offset, noAssert) {
  return _readFloat(this, offset, true, noAssert)
}

function BufferReadFloatBE (offset, noAssert) {
  return _readFloat(this, offset, false, noAssert)
}

function _readDouble (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset + 7 < buf.length, 'Trying to read beyond buffer length')
  }

  return buf._dataview.getFloat64(offset, littleEndian)
}

function BufferReadDoubleLE (offset, noAssert) {
  return _readDouble(this, offset, true, noAssert)
}

function BufferReadDoubleBE (offset, noAssert) {
  return _readDouble(this, offset, false, noAssert)
}

function BufferWriteUInt8 (value, offset, noAssert) {
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
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xffff)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setUint16(0, value, littleEndian)
    buf[offset] = dv.getUint8(0)
  } else {
    buf._dataview.setUint16(offset, value, littleEndian)
  }
}

function BufferWriteUInt16LE (value, offset, noAssert) {
  _writeUInt16(this, value, offset, true, noAssert)
}

function BufferWriteUInt16BE (value, offset, noAssert) {
  _writeUInt16(this, value, offset, false, noAssert)
}

function _writeUInt32 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xffffffff)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    dv.setUint32(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setUint32(offset, value, littleEndian)
  }
}

function BufferWriteUInt32LE (value, offset, noAssert) {
  _writeUInt32(this, value, offset, true, noAssert)
}

function BufferWriteUInt32BE (value, offset, noAssert) {
  _writeUInt32(this, value, offset, false, noAssert)
}

function BufferWriteInt8 (value, offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7f, -0x80)
  }

  if (offset >= buf.length) return

  buf._dataview.setInt8(offset, value)
}

function _writeInt16 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7fff, -0x8000)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setInt16(0, value, littleEndian)
    buf[offset] = dv.getUint8(0)
  } else {
    buf._dataview.setInt16(offset, value, littleEndian)
  }
}

function BufferWriteInt16LE (value, offset, noAssert) {
  _writeInt16(this, value, offset, true, noAssert)
}

function BufferWriteInt16BE (value, offset, noAssert) {
  _writeInt16(this, value, offset, false, noAssert)
}

function _writeInt32 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7fffffff, -0x80000000)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    dv.setInt32(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setInt32(offset, value, littleEndian)
  }
}

function BufferWriteInt32LE (value, offset, noAssert) {
  _writeInt32(this, value, offset, true, noAssert)
}

function BufferWriteInt32BE (value, offset, noAssert) {
  _writeInt32(this, value, offset, false, noAssert)
}

function _writeFloat (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to write beyond buffer length')
    verifIEEE754(value, 3.4028234663852886e+38, -3.4028234663852886e+38)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    dv.setFloat32(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setFloat32(offset, value, littleEndian)
  }
}

function BufferWriteFloatLE (value, offset, noAssert) {
  _writeFloat(this, value, offset, true, noAssert)
}

function BufferWriteFloatBE (value, offset, noAssert) {
  _writeFloat(this, value, offset, false, noAssert)
}

function _writeDouble (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 7 < buf.length,
        'Trying to write beyond buffer length')
    verifIEEE754(value, 1.7976931348623157E+308, -1.7976931348623157E+308)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 7 >= len) {
    var dv = new xDataView(new xArrayBuffer(8))
    dv.setFloat64(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setFloat64(offset, value, littleEndian)
  }
}

function BufferWriteDoubleLE (value, offset, noAssert) {
  _writeDouble(this, value, offset, true, noAssert)
}

function BufferWriteDoubleBE (value, offset, noAssert) {
  _writeDouble(this, value, offset, false, noAssert)
}

// fill(value, start=0, end=buffer.length)
function BufferFill (value, start, end) {
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

function BufferInspect () {
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

// Creates a new `ArrayBuffer` with the *copied* memory of the buffer instance.
// Added in Node 0.12.
function BufferToArrayBuffer () {
  return (new Buffer(this)).buffer
}


// HELPER FUNCTIONS
// ================

function stringtrim (str) {
  if (str.trim) return str.trim()
  return str.replace(/^\s+|\s+$/g, '')
}

function augment (arr) {
  arr._isBuffer = true

  // Augment the Uint8Array *instance* (not the class!) with Buffer methods
  arr.write = BufferWrite
  arr.toString = BufferToString
  arr.toLocaleString = BufferToString
  arr.toJSON = BufferToJSON
  arr.copy = BufferCopy
  arr.slice = BufferSlice
  arr.readUInt8 = BufferReadUInt8
  arr.readUInt16LE = BufferReadUInt16LE
  arr.readUInt16BE = BufferReadUInt16BE
  arr.readUInt32LE = BufferReadUInt32LE
  arr.readUInt32BE = BufferReadUInt32BE
  arr.readInt8 = BufferReadInt8
  arr.readInt16LE = BufferReadInt16LE
  arr.readInt16BE = BufferReadInt16BE
  arr.readInt32LE = BufferReadInt32LE
  arr.readInt32BE = BufferReadInt32BE
  arr.readFloatLE = BufferReadFloatLE
  arr.readFloatBE = BufferReadFloatBE
  arr.readDoubleLE = BufferReadDoubleLE
  arr.readDoubleBE = BufferReadDoubleBE
  arr.writeUInt8 = BufferWriteUInt8
  arr.writeUInt16LE = BufferWriteUInt16LE
  arr.writeUInt16BE = BufferWriteUInt16BE
  arr.writeUInt32LE = BufferWriteUInt32LE
  arr.writeUInt32BE = BufferWriteUInt32BE
  arr.writeInt8 = BufferWriteInt8
  arr.writeInt16LE = BufferWriteInt16LE
  arr.writeInt16BE = BufferWriteInt16BE
  arr.writeInt32LE = BufferWriteInt32LE
  arr.writeInt32BE = BufferWriteInt32BE
  arr.writeFloatLE = BufferWriteFloatLE
  arr.writeFloatBE = BufferWriteFloatBE
  arr.writeDoubleLE = BufferWriteDoubleLE
  arr.writeDoubleBE = BufferWriteDoubleBE
  arr.fill = BufferFill
  arr.inspect = BufferInspect

  // Only add `toArrayBuffer` if the browser supports ArrayBuffer natively
  if (xUint8Array !== TA.Uint8Array)
    arr.toArrayBuffer = BufferToArrayBuffer

  if (arr.byteLength !== 0)
    arr._dataview = new xDataView(arr.buffer, arr.byteOffset, arr.byteLength)

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

function isArrayIsh (subject) {
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
 *
 *      value           The number to check for validity
 *
 *      max             The maximum value
 */
function verifuint (value, max) {
  assert(typeof (value) == 'number', 'cannot write a non-number as a number')
  assert(value >= 0,
      'specified a negative value for writing an unsigned value')
  assert(value <= max, 'value is larger than maximum value for type')
  assert(Math.floor(value) === value, 'value has a fractional component')
}

/*
 * A series of checks to make sure we actually have a signed 32-bit number
 */
function verifsint(value, max, min) {
  assert(typeof (value) == 'number', 'cannot write a non-number as a number')
  assert(value <= max, 'value larger than maximum allowed value')
  assert(value >= min, 'value smaller than minimum allowed value')
  assert(Math.floor(value) === value, 'value has a fractional component')
}

function verifIEEE754(value, max, min) {
  assert(typeof (value) == 'number', 'cannot write a non-number as a number')
  assert(value <= max, 'value larger than maximum allowed value')
  assert(value >= min, 'value smaller than minimum allowed value')
}

function assert (test, message) {
  if (!test) throw new Error(message || 'Failed assertion')
}

},{"base64-js":3,"typedarray":4}],"native-buffer-browserify":[function(require,module,exports){
module.exports=require('26sEV7');
},{}],3:[function(require,module,exports){
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

},{}],4:[function(require,module,exports){
var undefined = (void 0); // Paranoia

// Beyond this value, index getters/setters (i.e. array[0], array[1]) are so slow to
// create, and consume so much memory, that the browser appears frozen.
var MAX_ARRAY_LENGTH = 1e5;

// Approximations of internal ECMAScript conversion functions
var ECMAScript = (function() {
  // Stash a copy in case other scripts modify these
  var opts = Object.prototype.toString,
      ophop = Object.prototype.hasOwnProperty;

  return {
    // Class returns internal [[Class]] property, used to avoid cross-frame instanceof issues:
    Class: function(v) { return opts.call(v).replace(/^\[object *|\]$/g, ''); },
    HasProperty: function(o, p) { return p in o; },
    HasOwnProperty: function(o, p) { return ophop.call(o, p); },
    IsCallable: function(o) { return typeof o === 'function'; },
    ToInt32: function(v) { return v >> 0; },
    ToUint32: function(v) { return v >>> 0; }
  };
}());

// Snapshot intrinsics
var LN2 = Math.LN2,
    abs = Math.abs,
    floor = Math.floor,
    log = Math.log,
    min = Math.min,
    pow = Math.pow,
    round = Math.round;

// ES5: lock down object properties
function configureProperties(obj) {
  if (getOwnPropNames && defineProp) {
    var props = getOwnPropNames(obj), i;
    for (i = 0; i < props.length; i += 1) {
      defineProp(obj, props[i], {
        value: obj[props[i]],
        writable: false,
        enumerable: false,
        configurable: false
      });
    }
  }
}

// emulate ES5 getter/setter API using legacy APIs
// http://blogs.msdn.com/b/ie/archive/2010/09/07/transitioning-existing-code-to-the-es5-getter-setter-apis.aspx
// (second clause tests for Object.defineProperty() in IE<9 that only supports extending DOM prototypes, but
// note that IE<9 does not support __defineGetter__ or __defineSetter__ so it just renders the method harmless)
var defineProp
if (Object.defineProperty && (function() {
      try {
        Object.defineProperty({}, 'x', {});
        return true;
      } catch (e) {
        return false;
      }
    })()) {
  defineProp = Object.defineProperty;
} else {
  defineProp = function(o, p, desc) {
    if (!o === Object(o)) throw new TypeError("Object.defineProperty called on non-object");
    if (ECMAScript.HasProperty(desc, 'get') && Object.prototype.__defineGetter__) { Object.prototype.__defineGetter__.call(o, p, desc.get); }
    if (ECMAScript.HasProperty(desc, 'set') && Object.prototype.__defineSetter__) { Object.prototype.__defineSetter__.call(o, p, desc.set); }
    if (ECMAScript.HasProperty(desc, 'value')) { o[p] = desc.value; }
    return o;
  };
}

var getOwnPropNames = Object.getOwnPropertyNames || function (o) {
  if (o !== Object(o)) throw new TypeError("Object.getOwnPropertyNames called on non-object");
  var props = [], p;
  for (p in o) {
    if (ECMAScript.HasOwnProperty(o, p)) {
      props.push(p);
    }
  }
  return props;
};

// ES5: Make obj[index] an alias for obj._getter(index)/obj._setter(index, value)
// for index in 0 ... obj.length
function makeArrayAccessors(obj) {
  if (!defineProp) { return; }

  if (obj.length > MAX_ARRAY_LENGTH) throw new RangeError("Array too large for polyfill");

  function makeArrayAccessor(index) {
    defineProp(obj, index, {
      'get': function() { return obj._getter(index); },
      'set': function(v) { obj._setter(index, v); },
      enumerable: true,
      configurable: false
    });
  }

  var i;
  for (i = 0; i < obj.length; i += 1) {
    makeArrayAccessor(i);
  }
}

// Internal conversion functions:
//    pack<Type>()   - take a number (interpreted as Type), output a byte array
//    unpack<Type>() - take a byte array, output a Type-like number

function as_signed(value, bits) { var s = 32 - bits; return (value << s) >> s; }
function as_unsigned(value, bits) { var s = 32 - bits; return (value << s) >>> s; }

function packI8(n) { return [n & 0xff]; }
function unpackI8(bytes) { return as_signed(bytes[0], 8); }

function packU8(n) { return [n & 0xff]; }
function unpackU8(bytes) { return as_unsigned(bytes[0], 8); }

function packU8Clamped(n) { n = round(Number(n)); return [n < 0 ? 0 : n > 0xff ? 0xff : n & 0xff]; }

function packI16(n) { return [(n >> 8) & 0xff, n & 0xff]; }
function unpackI16(bytes) { return as_signed(bytes[0] << 8 | bytes[1], 16); }

function packU16(n) { return [(n >> 8) & 0xff, n & 0xff]; }
function unpackU16(bytes) { return as_unsigned(bytes[0] << 8 | bytes[1], 16); }

function packI32(n) { return [(n >> 24) & 0xff, (n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff]; }
function unpackI32(bytes) { return as_signed(bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3], 32); }

function packU32(n) { return [(n >> 24) & 0xff, (n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff]; }
function unpackU32(bytes) { return as_unsigned(bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3], 32); }

function packIEEE754(v, ebits, fbits) {

  var bias = (1 << (ebits - 1)) - 1,
      s, e, f, ln,
      i, bits, str, bytes;

  function roundToEven(n) {
    var w = floor(n), f = n - w;
    if (f < 0.5)
      return w;
    if (f > 0.5)
      return w + 1;
    return w % 2 ? w + 1 : w;
  }

  // Compute sign, exponent, fraction
  if (v !== v) {
    // NaN
    // http://dev.w3.org/2006/webapi/WebIDL/#es-type-mapping
    e = (1 << ebits) - 1; f = pow(2, fbits - 1); s = 0;
  } else if (v === Infinity || v === -Infinity) {
    e = (1 << ebits) - 1; f = 0; s = (v < 0) ? 1 : 0;
  } else if (v === 0) {
    e = 0; f = 0; s = (1 / v === -Infinity) ? 1 : 0;
  } else {
    s = v < 0;
    v = abs(v);

    if (v >= pow(2, 1 - bias)) {
      e = min(floor(log(v) / LN2), 1023);
      f = roundToEven(v / pow(2, e) * pow(2, fbits));
      if (f / pow(2, fbits) >= 2) {
        e = e + 1;
        f = 1;
      }
      if (e > bias) {
        // Overflow
        e = (1 << ebits) - 1;
        f = 0;
      } else {
        // Normalized
        e = e + bias;
        f = f - pow(2, fbits);
      }
    } else {
      // Denormalized
      e = 0;
      f = roundToEven(v / pow(2, 1 - bias - fbits));
    }
  }

  // Pack sign, exponent, fraction
  bits = [];
  for (i = fbits; i; i -= 1) { bits.push(f % 2 ? 1 : 0); f = floor(f / 2); }
  for (i = ebits; i; i -= 1) { bits.push(e % 2 ? 1 : 0); e = floor(e / 2); }
  bits.push(s ? 1 : 0);
  bits.reverse();
  str = bits.join('');

  // Bits to bytes
  bytes = [];
  while (str.length) {
    bytes.push(parseInt(str.substring(0, 8), 2));
    str = str.substring(8);
  }
  return bytes;
}

function unpackIEEE754(bytes, ebits, fbits) {

  // Bytes to bits
  var bits = [], i, j, b, str,
      bias, s, e, f;

  for (i = bytes.length; i; i -= 1) {
    b = bytes[i - 1];
    for (j = 8; j; j -= 1) {
      bits.push(b % 2 ? 1 : 0); b = b >> 1;
    }
  }
  bits.reverse();
  str = bits.join('');

  // Unpack sign, exponent, fraction
  bias = (1 << (ebits - 1)) - 1;
  s = parseInt(str.substring(0, 1), 2) ? -1 : 1;
  e = parseInt(str.substring(1, 1 + ebits), 2);
  f = parseInt(str.substring(1 + ebits), 2);

  // Produce number
  if (e === (1 << ebits) - 1) {
    return f !== 0 ? NaN : s * Infinity;
  } else if (e > 0) {
    // Normalized
    return s * pow(2, e - bias) * (1 + f / pow(2, fbits));
  } else if (f !== 0) {
    // Denormalized
    return s * pow(2, -(bias - 1)) * (f / pow(2, fbits));
  } else {
    return s < 0 ? -0 : 0;
  }
}

function unpackF64(b) { return unpackIEEE754(b, 11, 52); }
function packF64(v) { return packIEEE754(v, 11, 52); }
function unpackF32(b) { return unpackIEEE754(b, 8, 23); }
function packF32(v) { return packIEEE754(v, 8, 23); }


//
// 3 The ArrayBuffer Type
//

(function() {

  /** @constructor */
  var ArrayBuffer = function ArrayBuffer(length) {
    length = ECMAScript.ToInt32(length);
    if (length < 0) throw new RangeError('ArrayBuffer size is not a small enough positive integer');

    this.byteLength = length;
    this._bytes = [];
    this._bytes.length = length;

    var i;
    for (i = 0; i < this.byteLength; i += 1) {
      this._bytes[i] = 0;
    }

    configureProperties(this);
  };

  exports.ArrayBuffer = exports.ArrayBuffer || ArrayBuffer;

  //
  // 4 The ArrayBufferView Type
  //

  // NOTE: this constructor is not exported
  /** @constructor */
  var ArrayBufferView = function ArrayBufferView() {
    //this.buffer = null;
    //this.byteOffset = 0;
    //this.byteLength = 0;
  };

  //
  // 5 The Typed Array View Types
  //

  function makeConstructor(bytesPerElement, pack, unpack) {
    // Each TypedArray type requires a distinct constructor instance with
    // identical logic, which this produces.

    var ctor;
    ctor = function(buffer, byteOffset, length) {
      var array, sequence, i, s;

      if (!arguments.length || typeof arguments[0] === 'number') {
        // Constructor(unsigned long length)
        this.length = ECMAScript.ToInt32(arguments[0]);
        if (length < 0) throw new RangeError('ArrayBufferView size is not a small enough positive integer');

        this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        this.buffer = new ArrayBuffer(this.byteLength);
        this.byteOffset = 0;
      } else if (typeof arguments[0] === 'object' && arguments[0].constructor === ctor) {
        // Constructor(TypedArray array)
        array = arguments[0];

        this.length = array.length;
        this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        this.buffer = new ArrayBuffer(this.byteLength);
        this.byteOffset = 0;

        for (i = 0; i < this.length; i += 1) {
          this._setter(i, array._getter(i));
        }
      } else if (typeof arguments[0] === 'object' &&
                 !(arguments[0] instanceof ArrayBuffer || ECMAScript.Class(arguments[0]) === 'ArrayBuffer')) {
        // Constructor(sequence<type> array)
        sequence = arguments[0];

        this.length = ECMAScript.ToUint32(sequence.length);
        this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        this.buffer = new ArrayBuffer(this.byteLength);
        this.byteOffset = 0;

        for (i = 0; i < this.length; i += 1) {
          s = sequence[i];
          this._setter(i, Number(s));
        }
      } else if (typeof arguments[0] === 'object' &&
                 (arguments[0] instanceof ArrayBuffer || ECMAScript.Class(arguments[0]) === 'ArrayBuffer')) {
        // Constructor(ArrayBuffer buffer,
        //             optional unsigned long byteOffset, optional unsigned long length)
        this.buffer = buffer;

        this.byteOffset = ECMAScript.ToUint32(byteOffset);
        if (this.byteOffset > this.buffer.byteLength) {
          throw new RangeError("byteOffset out of range");
        }

        if (this.byteOffset % this.BYTES_PER_ELEMENT) {
          // The given byteOffset must be a multiple of the element
          // size of the specific type, otherwise an exception is raised.
          throw new RangeError("ArrayBuffer length minus the byteOffset is not a multiple of the element size.");
        }

        if (arguments.length < 3) {
          this.byteLength = this.buffer.byteLength - this.byteOffset;

          if (this.byteLength % this.BYTES_PER_ELEMENT) {
            throw new RangeError("length of buffer minus byteOffset not a multiple of the element size");
          }
          this.length = this.byteLength / this.BYTES_PER_ELEMENT;
        } else {
          this.length = ECMAScript.ToUint32(length);
          this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        }

        if ((this.byteOffset + this.byteLength) > this.buffer.byteLength) {
          throw new RangeError("byteOffset and length reference an area beyond the end of the buffer");
        }
      } else {
        throw new TypeError("Unexpected argument type(s)");
      }

      this.constructor = ctor;

      configureProperties(this);
      makeArrayAccessors(this);
    };

    ctor.prototype = new ArrayBufferView();
    ctor.prototype.BYTES_PER_ELEMENT = bytesPerElement;
    ctor.prototype._pack = pack;
    ctor.prototype._unpack = unpack;
    ctor.BYTES_PER_ELEMENT = bytesPerElement;

    // getter type (unsigned long index);
    ctor.prototype._getter = function(index) {
      if (arguments.length < 1) throw new SyntaxError("Not enough arguments");

      index = ECMAScript.ToUint32(index);
      if (index >= this.length) {
        return undefined;
      }

      var bytes = [], i, o;
      for (i = 0, o = this.byteOffset + index * this.BYTES_PER_ELEMENT;
           i < this.BYTES_PER_ELEMENT;
           i += 1, o += 1) {
        bytes.push(this.buffer._bytes[o]);
      }
      return this._unpack(bytes);
    };

    // NONSTANDARD: convenience alias for getter: type get(unsigned long index);
    ctor.prototype.get = ctor.prototype._getter;

    // setter void (unsigned long index, type value);
    ctor.prototype._setter = function(index, value) {
      if (arguments.length < 2) throw new SyntaxError("Not enough arguments");

      index = ECMAScript.ToUint32(index);
      if (index >= this.length) {
        return undefined;
      }

      var bytes = this._pack(value), i, o;
      for (i = 0, o = this.byteOffset + index * this.BYTES_PER_ELEMENT;
           i < this.BYTES_PER_ELEMENT;
           i += 1, o += 1) {
        this.buffer._bytes[o] = bytes[i];
      }
    };

    // void set(TypedArray array, optional unsigned long offset);
    // void set(sequence<type> array, optional unsigned long offset);
    ctor.prototype.set = function(index, value) {
      if (arguments.length < 1) throw new SyntaxError("Not enough arguments");
      var array, sequence, offset, len,
          i, s, d,
          byteOffset, byteLength, tmp;

      if (typeof arguments[0] === 'object' && arguments[0].constructor === this.constructor) {
        // void set(TypedArray array, optional unsigned long offset);
        array = arguments[0];
        offset = ECMAScript.ToUint32(arguments[1]);

        if (offset + array.length > this.length) {
          throw new RangeError("Offset plus length of array is out of range");
        }

        byteOffset = this.byteOffset + offset * this.BYTES_PER_ELEMENT;
        byteLength = array.length * this.BYTES_PER_ELEMENT;

        if (array.buffer === this.buffer) {
          tmp = [];
          for (i = 0, s = array.byteOffset; i < byteLength; i += 1, s += 1) {
            tmp[i] = array.buffer._bytes[s];
          }
          for (i = 0, d = byteOffset; i < byteLength; i += 1, d += 1) {
            this.buffer._bytes[d] = tmp[i];
          }
        } else {
          for (i = 0, s = array.byteOffset, d = byteOffset;
               i < byteLength; i += 1, s += 1, d += 1) {
            this.buffer._bytes[d] = array.buffer._bytes[s];
          }
        }
      } else if (typeof arguments[0] === 'object' && typeof arguments[0].length !== 'undefined') {
        // void set(sequence<type> array, optional unsigned long offset);
        sequence = arguments[0];
        len = ECMAScript.ToUint32(sequence.length);
        offset = ECMAScript.ToUint32(arguments[1]);

        if (offset + len > this.length) {
          throw new RangeError("Offset plus length of array is out of range");
        }

        for (i = 0; i < len; i += 1) {
          s = sequence[i];
          this._setter(offset + i, Number(s));
        }
      } else {
        throw new TypeError("Unexpected argument type(s)");
      }
    };

    // TypedArray subarray(long begin, optional long end);
    ctor.prototype.subarray = function(start, end) {
      function clamp(v, min, max) { return v < min ? min : v > max ? max : v; }

      start = ECMAScript.ToInt32(start);
      end = ECMAScript.ToInt32(end);

      if (arguments.length < 1) { start = 0; }
      if (arguments.length < 2) { end = this.length; }

      if (start < 0) { start = this.length + start; }
      if (end < 0) { end = this.length + end; }

      start = clamp(start, 0, this.length);
      end = clamp(end, 0, this.length);

      var len = end - start;
      if (len < 0) {
        len = 0;
      }

      return new this.constructor(
        this.buffer, this.byteOffset + start * this.BYTES_PER_ELEMENT, len);
    };

    return ctor;
  }

  var Int8Array = makeConstructor(1, packI8, unpackI8);
  var Uint8Array = makeConstructor(1, packU8, unpackU8);
  var Uint8ClampedArray = makeConstructor(1, packU8Clamped, unpackU8);
  var Int16Array = makeConstructor(2, packI16, unpackI16);
  var Uint16Array = makeConstructor(2, packU16, unpackU16);
  var Int32Array = makeConstructor(4, packI32, unpackI32);
  var Uint32Array = makeConstructor(4, packU32, unpackU32);
  var Float32Array = makeConstructor(4, packF32, unpackF32);
  var Float64Array = makeConstructor(8, packF64, unpackF64);

  exports.Int8Array = exports.Int8Array || Int8Array;
  exports.Uint8Array = exports.Uint8Array || Uint8Array;
  exports.Uint8ClampedArray = exports.Uint8ClampedArray || Uint8ClampedArray;
  exports.Int16Array = exports.Int16Array || Int16Array;
  exports.Uint16Array = exports.Uint16Array || Uint16Array;
  exports.Int32Array = exports.Int32Array || Int32Array;
  exports.Uint32Array = exports.Uint32Array || Uint32Array;
  exports.Float32Array = exports.Float32Array || Float32Array;
  exports.Float64Array = exports.Float64Array || Float64Array;
}());

//
// 6 The DataView View Type
//

(function() {
  function r(array, index) {
    return ECMAScript.IsCallable(array.get) ? array.get(index) : array[index];
  }

  var IS_BIG_ENDIAN = (function() {
    var u16array = new(exports.Uint16Array)([0x1234]),
        u8array = new(exports.Uint8Array)(u16array.buffer);
    return r(u8array, 0) === 0x12;
  }());

  // Constructor(ArrayBuffer buffer,
  //             optional unsigned long byteOffset,
  //             optional unsigned long byteLength)
  /** @constructor */
  var DataView = function DataView(buffer, byteOffset, byteLength) {
    if (arguments.length === 0) {
      buffer = new exports.ArrayBuffer(0);
    } else if (!(buffer instanceof exports.ArrayBuffer || ECMAScript.Class(buffer) === 'ArrayBuffer')) {
      throw new TypeError("TypeError");
    }

    this.buffer = buffer || new exports.ArrayBuffer(0);

    this.byteOffset = ECMAScript.ToUint32(byteOffset);
    if (this.byteOffset > this.buffer.byteLength) {
      throw new RangeError("byteOffset out of range");
    }

    if (arguments.length < 3) {
      this.byteLength = this.buffer.byteLength - this.byteOffset;
    } else {
      this.byteLength = ECMAScript.ToUint32(byteLength);
    }

    if ((this.byteOffset + this.byteLength) > this.buffer.byteLength) {
      throw new RangeError("byteOffset and length reference an area beyond the end of the buffer");
    }

    configureProperties(this);
  };

  function makeGetter(arrayType) {
    return function(byteOffset, littleEndian) {

      byteOffset = ECMAScript.ToUint32(byteOffset);

      if (byteOffset + arrayType.BYTES_PER_ELEMENT > this.byteLength) {
        throw new RangeError("Array index out of range");
      }
      byteOffset += this.byteOffset;

      var uint8Array = new exports.Uint8Array(this.buffer, byteOffset, arrayType.BYTES_PER_ELEMENT),
          bytes = [], i;
      for (i = 0; i < arrayType.BYTES_PER_ELEMENT; i += 1) {
        bytes.push(r(uint8Array, i));
      }

      if (Boolean(littleEndian) === Boolean(IS_BIG_ENDIAN)) {
        bytes.reverse();
      }

      return r(new arrayType(new exports.Uint8Array(bytes).buffer), 0);
    };
  }

  DataView.prototype.getUint8 = makeGetter(exports.Uint8Array);
  DataView.prototype.getInt8 = makeGetter(exports.Int8Array);
  DataView.prototype.getUint16 = makeGetter(exports.Uint16Array);
  DataView.prototype.getInt16 = makeGetter(exports.Int16Array);
  DataView.prototype.getUint32 = makeGetter(exports.Uint32Array);
  DataView.prototype.getInt32 = makeGetter(exports.Int32Array);
  DataView.prototype.getFloat32 = makeGetter(exports.Float32Array);
  DataView.prototype.getFloat64 = makeGetter(exports.Float64Array);

  function makeSetter(arrayType) {
    return function(byteOffset, value, littleEndian) {

      byteOffset = ECMAScript.ToUint32(byteOffset);
      if (byteOffset + arrayType.BYTES_PER_ELEMENT > this.byteLength) {
        throw new RangeError("Array index out of range");
      }

      // Get bytes
      var typeArray = new arrayType([value]),
          byteArray = new exports.Uint8Array(typeArray.buffer),
          bytes = [], i, byteView;

      for (i = 0; i < arrayType.BYTES_PER_ELEMENT; i += 1) {
        bytes.push(r(byteArray, i));
      }

      // Flip if necessary
      if (Boolean(littleEndian) === Boolean(IS_BIG_ENDIAN)) {
        bytes.reverse();
      }

      // Write them
      byteView = new exports.Uint8Array(this.buffer, byteOffset, arrayType.BYTES_PER_ELEMENT);
      byteView.set(bytes);
    };
  }

  DataView.prototype.setUint8 = makeSetter(exports.Uint8Array);
  DataView.prototype.setInt8 = makeSetter(exports.Int8Array);
  DataView.prototype.setUint16 = makeSetter(exports.Uint16Array);
  DataView.prototype.setInt16 = makeSetter(exports.Int16Array);
  DataView.prototype.setUint32 = makeSetter(exports.Uint32Array);
  DataView.prototype.setInt32 = makeSetter(exports.Int32Array);
  DataView.prototype.setFloat32 = makeSetter(exports.Float32Array);
  DataView.prototype.setFloat64 = makeSetter(exports.Float64Array);

  exports.DataView = exports.DataView || DataView;

}());

},{}]},{},[])
;;module.exports=require("native-buffer-browserify").Buffer

},{}],2:[function(require,module,exports){
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

},{}],3:[function(require,module,exports){
var base64 = require('base64-js')
var TA = require('typedarray')

exports.Buffer = Buffer
exports.SlowBuffer = Buffer
exports.INSPECT_MAX_BYTES = 50
Buffer.poolSize = 8192

/**
 * Use a shim for browsers that lack Typed Array support (< IE 9, < FF 3.6,
 * < Chrome 6, < Safari 5, < Opera 11.5, < iOS 4.1).
 */
var xDataView = typeof DataView === 'undefined'
  ? TA.DataView : DataView
var xArrayBuffer = typeof ArrayBuffer === 'undefined'
  ? TA.ArrayBuffer : ArrayBuffer
var xUint8Array = typeof Uint8Array === 'undefined'
  ? TA.Uint8Array : Uint8Array

/**
 * Check to see if the browser supports augmenting a `Uint8Array` instance.
 */
var browserSupport = (function () {
  try {
    var arr = new xUint8Array(0)
    arr.foo = function () { return 42 }
    return 42 === arr.foo()
  } catch (e) {
    return false
  }
})()

/**
 * Also use the shim in Firefox 4-17 (even though they have native Uint8Array),
 * since they don't support Proxy. Without that, it is not possible to augment
 * native Uint8Array instances in Firefox.
 */
if (xUint8Array !== TA.Uint8Array && !browserSupport) {
  xDataView = TA.DataView
  xArrayBuffer = TA.ArrayBuffer
  xUint8Array = TA.Uint8Array
  browserSupport = true
}

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
 *
 * Firefox is a special case because it doesn't allow augmenting "native" object
 * instances. See `ProxyBuffer` below for more details.
 */
function Buffer (subject, encoding) {
  var type = typeof subject

  // Work-around: node's base64 implementation
  // allows for non-padded strings while base64-js
  // does not..
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

  var buf = augment(new xUint8Array(length))
  if (Buffer.isBuffer(subject)) {
    // Speed optimization -- use set if we're copying from a Uint8Array
    buf.set(subject)
  } else if (isArrayIsh(subject)) {
    // Treat array-ish objects as a byte array.
    for (var i = 0; i < length; i++) {
      if (Buffer.isBuffer(subject))
        buf[i] = subject.readUInt8(i)
      else
        buf[i] = subject[i]
    }
  } else if (type === 'string') {
    buf.write(subject, 0, encoding)
  }

  return buf
}

// STATIC METHODS
// ==============

Buffer.isEncoding = function(encoding) {
  switch ((encoding + '').toLowerCase()) {
    case 'hex':
    case 'utf8':
    case 'utf-8':
    case 'ascii':
    case 'binary':
    case 'base64':
    case 'ucs2':
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
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

  var i
  var buf

  if (list.length === 0) {
    return new Buffer(0)
  } else if (list.length === 1) {
    return list[0]
  }

  if (typeof totalLength !== 'number') {
    totalLength = 0
    for (i = 0; i < list.length; i++) {
      buf = list[i]
      totalLength += buf.length
    }
  }

  var buffer = new Buffer(totalLength)
  var pos = 0
  for (i = 0; i < list.length; i++) {
    buf = list[i]
    buf.copy(buffer, pos)
    pos += buf.length
  }
  return buffer
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

function BufferWrite (string, offset, length, encoding) {
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

function BufferToString (encoding, start, end) {
  var self = (this instanceof ProxyBuffer)
    ? this._proxy
    : this

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

function BufferToJSON () {
  return {
    type: 'Buffer',
    data: Array.prototype.slice.call(this._arr || this, 0)
  }
}

// copy(targetBuffer, targetStart=0, sourceStart=0, sourceEnd=buffer.length)
function BufferCopy (target, target_start, start, end) {
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
function BufferSlice (start, end) {
  var len = this.length
  start = clamp(start, len, 0)
  end = clamp(end, len, len)
  return augment(this.subarray(start, end)) // Uint8Array built-in method
}

function BufferReadUInt8 (offset, noAssert) {
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
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setUint8(0, buf[len - 1])
    return dv.getUint16(0, littleEndian)
  } else {
    return buf._dataview.getUint16(offset, littleEndian)
  }
}

function BufferReadUInt16LE (offset, noAssert) {
  return _readUInt16(this, offset, true, noAssert)
}

function BufferReadUInt16BE (offset, noAssert) {
  return _readUInt16(this, offset, false, noAssert)
}

function _readUInt32 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    for (var i = 0; i + offset < len; i++) {
      dv.setUint8(i, buf[i + offset])
    }
    return dv.getUint32(0, littleEndian)
  } else {
    return buf._dataview.getUint32(offset, littleEndian)
  }
}

function BufferReadUInt32LE (offset, noAssert) {
  return _readUInt32(this, offset, true, noAssert)
}

function BufferReadUInt32BE (offset, noAssert) {
  return _readUInt32(this, offset, false, noAssert)
}

function BufferReadInt8 (offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(offset !== undefined && offset !== null,
        'missing offset')
    assert(offset < buf.length, 'Trying to read beyond buffer length')
  }

  if (offset >= buf.length)
    return

  return buf._dataview.getInt8(offset)
}

function _readInt16 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null,
        'missing offset')
    assert(offset + 1 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setUint8(0, buf[len - 1])
    return dv.getInt16(0, littleEndian)
  } else {
    return buf._dataview.getInt16(offset, littleEndian)
  }
}

function BufferReadInt16LE (offset, noAssert) {
  return _readInt16(this, offset, true, noAssert)
}

function BufferReadInt16BE (offset, noAssert) {
  return _readInt16(this, offset, false, noAssert)
}

function _readInt32 (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    for (var i = 0; i + offset < len; i++) {
      dv.setUint8(i, buf[i + offset])
    }
    return dv.getInt32(0, littleEndian)
  } else {
    return buf._dataview.getInt32(offset, littleEndian)
  }
}

function BufferReadInt32LE (offset, noAssert) {
  return _readInt32(this, offset, true, noAssert)
}

function BufferReadInt32BE (offset, noAssert) {
  return _readInt32(this, offset, false, noAssert)
}

function _readFloat (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset + 3 < buf.length, 'Trying to read beyond buffer length')
  }

  return buf._dataview.getFloat32(offset, littleEndian)
}

function BufferReadFloatLE (offset, noAssert) {
  return _readFloat(this, offset, true, noAssert)
}

function BufferReadFloatBE (offset, noAssert) {
  return _readFloat(this, offset, false, noAssert)
}

function _readDouble (buf, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset + 7 < buf.length, 'Trying to read beyond buffer length')
  }

  return buf._dataview.getFloat64(offset, littleEndian)
}

function BufferReadDoubleLE (offset, noAssert) {
  return _readDouble(this, offset, true, noAssert)
}

function BufferReadDoubleBE (offset, noAssert) {
  return _readDouble(this, offset, false, noAssert)
}

function BufferWriteUInt8 (value, offset, noAssert) {
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
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xffff)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setUint16(0, value, littleEndian)
    buf[offset] = dv.getUint8(0)
  } else {
    buf._dataview.setUint16(offset, value, littleEndian)
  }
}

function BufferWriteUInt16LE (value, offset, noAssert) {
  _writeUInt16(this, value, offset, true, noAssert)
}

function BufferWriteUInt16BE (value, offset, noAssert) {
  _writeUInt16(this, value, offset, false, noAssert)
}

function _writeUInt32 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'trying to write beyond buffer length')
    verifuint(value, 0xffffffff)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    dv.setUint32(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setUint32(offset, value, littleEndian)
  }
}

function BufferWriteUInt32LE (value, offset, noAssert) {
  _writeUInt32(this, value, offset, true, noAssert)
}

function BufferWriteUInt32BE (value, offset, noAssert) {
  _writeUInt32(this, value, offset, false, noAssert)
}

function BufferWriteInt8 (value, offset, noAssert) {
  var buf = this
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7f, -0x80)
  }

  if (offset >= buf.length) return

  buf._dataview.setInt8(offset, value)
}

function _writeInt16 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 1 < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7fff, -0x8000)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 1 === len) {
    var dv = new xDataView(new xArrayBuffer(2))
    dv.setInt16(0, value, littleEndian)
    buf[offset] = dv.getUint8(0)
  } else {
    buf._dataview.setInt16(offset, value, littleEndian)
  }
}

function BufferWriteInt16LE (value, offset, noAssert) {
  _writeInt16(this, value, offset, true, noAssert)
}

function BufferWriteInt16BE (value, offset, noAssert) {
  _writeInt16(this, value, offset, false, noAssert)
}

function _writeInt32 (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7fffffff, -0x80000000)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    dv.setInt32(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setInt32(offset, value, littleEndian)
  }
}

function BufferWriteInt32LE (value, offset, noAssert) {
  _writeInt32(this, value, offset, true, noAssert)
}

function BufferWriteInt32BE (value, offset, noAssert) {
  _writeInt32(this, value, offset, false, noAssert)
}

function _writeFloat (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 3 < buf.length, 'Trying to write beyond buffer length')
    verifIEEE754(value, 3.4028234663852886e+38, -3.4028234663852886e+38)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 3 >= len) {
    var dv = new xDataView(new xArrayBuffer(4))
    dv.setFloat32(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setFloat32(offset, value, littleEndian)
  }
}

function BufferWriteFloatLE (value, offset, noAssert) {
  _writeFloat(this, value, offset, true, noAssert)
}

function BufferWriteFloatBE (value, offset, noAssert) {
  _writeFloat(this, value, offset, false, noAssert)
}

function _writeDouble (buf, value, offset, littleEndian, noAssert) {
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(typeof (littleEndian) === 'boolean',
        'missing or invalid endian')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset + 7 < buf.length,
        'Trying to write beyond buffer length')
    verifIEEE754(value, 1.7976931348623157E+308, -1.7976931348623157E+308)
  }

  var len = buf.length
  if (offset >= len) {
    return
  } else if (offset + 7 >= len) {
    var dv = new xDataView(new xArrayBuffer(8))
    dv.setFloat64(0, value, littleEndian)
    for (var i = 0; i + offset < len; i++) {
      buf[i + offset] = dv.getUint8(i)
    }
  } else {
    buf._dataview.setFloat64(offset, value, littleEndian)
  }
}

function BufferWriteDoubleLE (value, offset, noAssert) {
  _writeDouble(this, value, offset, true, noAssert)
}

function BufferWriteDoubleBE (value, offset, noAssert) {
  _writeDouble(this, value, offset, false, noAssert)
}

// fill(value, start=0, end=buffer.length)
function BufferFill (value, start, end) {
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

function BufferInspect () {
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

// Creates a new `ArrayBuffer` with the *copied* memory of the buffer instance.
// Added in Node 0.12.
function BufferToArrayBuffer () {
  return (new Buffer(this)).buffer
}


// HELPER FUNCTIONS
// ================

function stringtrim (str) {
  if (str.trim) return str.trim()
  return str.replace(/^\s+|\s+$/g, '')
}

/**
 * Class: ProxyBuffer
 * ==================
 *
 * Only used in Firefox, since Firefox does not allow augmenting "native"
 * objects (like Uint8Array instances) with new properties for some unknown
 * (probably silly) reason. So we'll use an ES6 Proxy (supported since
 * Firefox 18) to wrap the Uint8Array instance without actually adding any
 * properties to it.
 *
 * Instances of this "fake" Buffer class are the "target" of the
 * ES6 Proxy (see `augment` function).
 *
 * We couldn't just use the `Uint8Array` as the target of the `Proxy` because
 * Proxies have an important limitation on trapping the `toString` method.
 * `Object.prototype.toString.call(proxy)` gets called whenever something is
 * implicitly cast to a String. Unfortunately, with a `Proxy` this
 * unconditionally returns `Object.prototype.toString.call(target)` which would
 * always return "[object Uint8Array]" if we used the `Uint8Array` instance as
 * the target. And, remember, in Firefox we cannot redefine the `Uint8Array`
 * instance's `toString` method.
 *
 * So, we use this `ProxyBuffer` class as the proxy's "target". Since this class
 * has its own custom `toString` method, it will get called whenever `toString`
 * gets called, implicitly or explicitly, on the `Proxy` instance.
 *
 * We also have to define the Uint8Array methods `subarray` and `set` on
 * `ProxyBuffer` because if we didn't then `proxy.subarray(0)` would have its
 * `this` set to `proxy` (a `Proxy` instance) which throws an exception in
 * Firefox which expects it to be a `TypedArray` instance.
 */
function ProxyBuffer (arr) {
  this._arr = arr

  if (arr.byteLength !== 0)
    this._dataview = new xDataView(arr.buffer, arr.byteOffset, arr.byteLength)
}

ProxyBuffer.prototype = {
  _isBuffer: true,
  write: BufferWrite,
  toString: BufferToString,
  toLocaleString: BufferToString,
  toJSON: BufferToJSON,
  copy: BufferCopy,
  slice: BufferSlice,
  readUInt8: BufferReadUInt8,
  readUInt16LE: BufferReadUInt16LE,
  readUInt16BE: BufferReadUInt16BE,
  readUInt32LE: BufferReadUInt32LE,
  readUInt32BE: BufferReadUInt32BE,
  readInt8: BufferReadInt8,
  readInt16LE: BufferReadInt16LE,
  readInt16BE: BufferReadInt16BE,
  readInt32LE: BufferReadInt32LE,
  readInt32BE: BufferReadInt32BE,
  readFloatLE: BufferReadFloatLE,
  readFloatBE: BufferReadFloatBE,
  readDoubleLE: BufferReadDoubleLE,
  readDoubleBE: BufferReadDoubleBE,
  writeUInt8: BufferWriteUInt8,
  writeUInt16LE: BufferWriteUInt16LE,
  writeUInt16BE: BufferWriteUInt16BE,
  writeUInt32LE: BufferWriteUInt32LE,
  writeUInt32BE: BufferWriteUInt32BE,
  writeInt8: BufferWriteInt8,
  writeInt16LE: BufferWriteInt16LE,
  writeInt16BE: BufferWriteInt16BE,
  writeInt32LE: BufferWriteInt32LE,
  writeInt32BE: BufferWriteInt32BE,
  writeFloatLE: BufferWriteFloatLE,
  writeFloatBE: BufferWriteFloatBE,
  writeDoubleLE: BufferWriteDoubleLE,
  writeDoubleBE: BufferWriteDoubleBE,
  fill: BufferFill,
  inspect: BufferInspect,
  toArrayBuffer: BufferToArrayBuffer,
  subarray: function () {
    return this._arr.subarray.apply(this._arr, arguments)
  },
  set: function () {
    return this._arr.set.apply(this._arr, arguments)
  }
}

var ProxyHandler = {
  get: function (target, name) {
    if (name in target) return target[name]
    else return target._arr[name]
  },
  set: function (target, name, value) {
    target._arr[name] = value
  }
}

function augment (arr) {
  if (browserSupport) {
    arr._isBuffer = true

    // Augment the Uint8Array *instance* (not the class!) with Buffer methods
    arr.write = BufferWrite
    arr.toString = BufferToString
    arr.toLocaleString = BufferToString
    arr.toJSON = BufferToJSON
    arr.copy = BufferCopy
    arr.slice = BufferSlice
    arr.readUInt8 = BufferReadUInt8
    arr.readUInt16LE = BufferReadUInt16LE
    arr.readUInt16BE = BufferReadUInt16BE
    arr.readUInt32LE = BufferReadUInt32LE
    arr.readUInt32BE = BufferReadUInt32BE
    arr.readInt8 = BufferReadInt8
    arr.readInt16LE = BufferReadInt16LE
    arr.readInt16BE = BufferReadInt16BE
    arr.readInt32LE = BufferReadInt32LE
    arr.readInt32BE = BufferReadInt32BE
    arr.readFloatLE = BufferReadFloatLE
    arr.readFloatBE = BufferReadFloatBE
    arr.readDoubleLE = BufferReadDoubleLE
    arr.readDoubleBE = BufferReadDoubleBE
    arr.writeUInt8 = BufferWriteUInt8
    arr.writeUInt16LE = BufferWriteUInt16LE
    arr.writeUInt16BE = BufferWriteUInt16BE
    arr.writeUInt32LE = BufferWriteUInt32LE
    arr.writeUInt32BE = BufferWriteUInt32BE
    arr.writeInt8 = BufferWriteInt8
    arr.writeInt16LE = BufferWriteInt16LE
    arr.writeInt16BE = BufferWriteInt16BE
    arr.writeInt32LE = BufferWriteInt32LE
    arr.writeInt32BE = BufferWriteInt32BE
    arr.writeFloatLE = BufferWriteFloatLE
    arr.writeFloatBE = BufferWriteFloatBE
    arr.writeDoubleLE = BufferWriteDoubleLE
    arr.writeDoubleBE = BufferWriteDoubleBE
    arr.fill = BufferFill
    arr.inspect = BufferInspect

    // Only add `toArrayBuffer` if the browser supports ArrayBuffer natively
    if (xUint8Array !== TA.Uint8Array)
      arr.toArrayBuffer = BufferToArrayBuffer

    if (arr.byteLength !== 0)
      arr._dataview = new xDataView(arr.buffer, arr.byteOffset, arr.byteLength)

    return arr

  } else {
    // This is a browser that doesn't support augmenting the `Uint8Array`
    // instance (*ahem* Firefox) so use an ES6 `Proxy`.
    var proxyBuffer = new ProxyBuffer(arr)
    var proxy = new Proxy(proxyBuffer, ProxyHandler)
    proxyBuffer._proxy = proxy
    return proxy
  }
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

function isArrayIsh (subject) {
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
 *
 *      value           The number to check for validity
 *
 *      max             The maximum value
 */
function verifuint (value, max) {
  assert(typeof (value) == 'number', 'cannot write a non-number as a number')
  assert(value >= 0,
      'specified a negative value for writing an unsigned value')
  assert(value <= max, 'value is larger than maximum value for type')
  assert(Math.floor(value) === value, 'value has a fractional component')
}

/*
 * A series of checks to make sure we actually have a signed 32-bit number
 */
function verifsint(value, max, min) {
  assert(typeof (value) == 'number', 'cannot write a non-number as a number')
  assert(value <= max, 'value larger than maximum allowed value')
  assert(value >= min, 'value smaller than minimum allowed value')
  assert(Math.floor(value) === value, 'value has a fractional component')
}

function verifIEEE754(value, max, min) {
  assert(typeof (value) == 'number', 'cannot write a non-number as a number')
  assert(value <= max, 'value larger than maximum allowed value')
  assert(value >= min, 'value smaller than minimum allowed value')
}

function assert (test, message) {
  if (!test) throw new Error(message || 'Failed assertion')
}

},{"base64-js":4,"typedarray":9}],4:[function(require,module,exports){
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

},{}],5:[function(require,module,exports){
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

},{"__browserify_process":2}],6:[function(require,module,exports){
exports.readIEEE754 = function(buffer, offset, isBE, mLen, nBytes) {
  var e, m,
      eLen = nBytes * 8 - mLen - 1,
      eMax = (1 << eLen) - 1,
      eBias = eMax >> 1,
      nBits = -7,
      i = isBE ? 0 : (nBytes - 1),
      d = isBE ? 1 : -1,
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

exports.writeIEEE754 = function(buffer, value, offset, isBE, mLen, nBytes) {
  var e, m, c,
      eLen = nBytes * 8 - mLen - 1,
      eMax = (1 << eLen) - 1,
      eBias = eMax >> 1,
      rt = (mLen === 23 ? Math.pow(2, -24) - Math.pow(2, -77) : 0),
      i = isBE ? (nBytes - 1) : 0,
      d = isBE ? -1 : 1,
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

},{}],7:[function(require,module,exports){
var assert;
exports.Buffer = Buffer;
exports.SlowBuffer = Buffer;
Buffer.poolSize = 8192;
exports.INSPECT_MAX_BYTES = 50;

function stringtrim(str) {
  if (str.trim) return str.trim();
  return str.replace(/^\s+|\s+$/g, '');
}

function Buffer(subject, encoding, offset) {
  if(!assert) assert= require('assert');
  if (!(this instanceof Buffer)) {
    return new Buffer(subject, encoding, offset);
  }
  this.parent = this;
  this.offset = 0;

  // Work-around: node's base64 implementation
  // allows for non-padded strings while base64-js
  // does not..
  if (encoding == "base64" && typeof subject == "string") {
    subject = stringtrim(subject);
    while (subject.length % 4 != 0) {
      subject = subject + "="; 
    }
  }

  var type;

  // Are we slicing?
  if (typeof offset === 'number') {
    this.length = coerce(encoding);
    // slicing works, with limitations (no parent tracking/update)
    // check https://github.com/toots/buffer-browserify/issues/19
    for (var i = 0; i < this.length; i++) {
        this[i] = subject.get(i+offset);
    }
  } else {
    // Find the length
    switch (type = typeof subject) {
      case 'number':
        this.length = coerce(subject);
        break;

      case 'string':
        this.length = Buffer.byteLength(subject, encoding);
        break;

      case 'object': // Assume object is an array
        this.length = coerce(subject.length);
        break;

      default:
        throw new Error('First argument needs to be a number, ' +
                        'array or string.');
    }

    // Treat array-ish objects as a byte array.
    if (isArrayIsh(subject)) {
      for (var i = 0; i < this.length; i++) {
        if (subject instanceof Buffer) {
          this[i] = subject.readUInt8(i);
        }
        else {
          this[i] = subject[i];
        }
      }
    } else if (type == 'string') {
      // We are a string
      this.length = this.write(subject, 0, encoding);
    } else if (type === 'number') {
      for (var i = 0; i < this.length; i++) {
        this[i] = 0;
      }
    }
  }
}

Buffer.prototype.get = function get(i) {
  if (i < 0 || i >= this.length) throw new Error('oob');
  return this[i];
};

Buffer.prototype.set = function set(i, v) {
  if (i < 0 || i >= this.length) throw new Error('oob');
  return this[i] = v;
};

Buffer.byteLength = function (str, encoding) {
  switch (encoding || "utf8") {
    case 'hex':
      return str.length / 2;

    case 'utf8':
    case 'utf-8':
      return utf8ToBytes(str).length;

    case 'ascii':
    case 'binary':
      return str.length;

    case 'base64':
      return base64ToBytes(str).length;

    default:
      throw new Error('Unknown encoding');
  }
};

Buffer.prototype.utf8Write = function (string, offset, length) {
  var bytes, pos;
  return Buffer._charsWritten =  blitBuffer(utf8ToBytes(string), this, offset, length);
};

Buffer.prototype.asciiWrite = function (string, offset, length) {
  var bytes, pos;
  return Buffer._charsWritten =  blitBuffer(asciiToBytes(string), this, offset, length);
};

Buffer.prototype.binaryWrite = Buffer.prototype.asciiWrite;

Buffer.prototype.base64Write = function (string, offset, length) {
  var bytes, pos;
  return Buffer._charsWritten = blitBuffer(base64ToBytes(string), this, offset, length);
};

Buffer.prototype.base64Slice = function (start, end) {
  var bytes = Array.prototype.slice.apply(this, arguments)
  return require("base64-js").fromByteArray(bytes);
};

Buffer.prototype.utf8Slice = function () {
  var bytes = Array.prototype.slice.apply(this, arguments);
  var res = "";
  var tmp = "";
  var i = 0;
  while (i < bytes.length) {
    if (bytes[i] <= 0x7F) {
      res += decodeUtf8Char(tmp) + String.fromCharCode(bytes[i]);
      tmp = "";
    } else
      tmp += "%" + bytes[i].toString(16);

    i++;
  }

  return res + decodeUtf8Char(tmp);
}

Buffer.prototype.asciiSlice = function () {
  var bytes = Array.prototype.slice.apply(this, arguments);
  var ret = "";
  for (var i = 0; i < bytes.length; i++)
    ret += String.fromCharCode(bytes[i]);
  return ret;
}

Buffer.prototype.binarySlice = Buffer.prototype.asciiSlice;

Buffer.prototype.inspect = function() {
  var out = [],
      len = this.length;
  for (var i = 0; i < len; i++) {
    out[i] = toHex(this[i]);
    if (i == exports.INSPECT_MAX_BYTES) {
      out[i + 1] = '...';
      break;
    }
  }
  return '<Buffer ' + out.join(' ') + '>';
};


Buffer.prototype.hexSlice = function(start, end) {
  var len = this.length;

  if (!start || start < 0) start = 0;
  if (!end || end < 0 || end > len) end = len;

  var out = '';
  for (var i = start; i < end; i++) {
    out += toHex(this[i]);
  }
  return out;
};


Buffer.prototype.toString = function(encoding, start, end) {
  encoding = String(encoding || 'utf8').toLowerCase();
  start = +start || 0;
  if (typeof end == 'undefined') end = this.length;

  // Fastpath empty strings
  if (+end == start) {
    return '';
  }

  switch (encoding) {
    case 'hex':
      return this.hexSlice(start, end);

    case 'utf8':
    case 'utf-8':
      return this.utf8Slice(start, end);

    case 'ascii':
      return this.asciiSlice(start, end);

    case 'binary':
      return this.binarySlice(start, end);

    case 'base64':
      return this.base64Slice(start, end);

    case 'ucs2':
    case 'ucs-2':
      return this.ucs2Slice(start, end);

    default:
      throw new Error('Unknown encoding');
  }
};


Buffer.prototype.hexWrite = function(string, offset, length) {
  offset = +offset || 0;
  var remaining = this.length - offset;
  if (!length) {
    length = remaining;
  } else {
    length = +length;
    if (length > remaining) {
      length = remaining;
    }
  }

  // must be an even number of digits
  var strLen = string.length;
  if (strLen % 2) {
    throw new Error('Invalid hex string');
  }
  if (length > strLen / 2) {
    length = strLen / 2;
  }
  for (var i = 0; i < length; i++) {
    var byte = parseInt(string.substr(i * 2, 2), 16);
    if (isNaN(byte)) throw new Error('Invalid hex string');
    this[offset + i] = byte;
  }
  Buffer._charsWritten = i * 2;
  return i;
};


Buffer.prototype.write = function(string, offset, length, encoding) {
  // Support both (string, offset, length, encoding)
  // and the legacy (string, encoding, offset, length)
  if (isFinite(offset)) {
    if (!isFinite(length)) {
      encoding = length;
      length = undefined;
    }
  } else {  // legacy
    var swap = encoding;
    encoding = offset;
    offset = length;
    length = swap;
  }

  offset = +offset || 0;
  var remaining = this.length - offset;
  if (!length) {
    length = remaining;
  } else {
    length = +length;
    if (length > remaining) {
      length = remaining;
    }
  }
  encoding = String(encoding || 'utf8').toLowerCase();

  switch (encoding) {
    case 'hex':
      return this.hexWrite(string, offset, length);

    case 'utf8':
    case 'utf-8':
      return this.utf8Write(string, offset, length);

    case 'ascii':
      return this.asciiWrite(string, offset, length);

    case 'binary':
      return this.binaryWrite(string, offset, length);

    case 'base64':
      return this.base64Write(string, offset, length);

    case 'ucs2':
    case 'ucs-2':
      return this.ucs2Write(string, offset, length);

    default:
      throw new Error('Unknown encoding');
  }
};

// slice(start, end)
function clamp(index, len, defaultValue) {
  if (typeof index !== 'number') return defaultValue;
  index = ~~index;  // Coerce to integer.
  if (index >= len) return len;
  if (index >= 0) return index;
  index += len;
  if (index >= 0) return index;
  return 0;
}

Buffer.prototype.slice = function(start, end) {
  var len = this.length;
  start = clamp(start, len, 0);
  end = clamp(end, len, len);
  return new Buffer(this, end - start, +start);
};

// copy(targetBuffer, targetStart=0, sourceStart=0, sourceEnd=buffer.length)
Buffer.prototype.copy = function(target, target_start, start, end) {
  var source = this;
  start || (start = 0);
  if (end === undefined || isNaN(end)) {
    end = this.length;
  }
  target_start || (target_start = 0);

  if (end < start) throw new Error('sourceEnd < sourceStart');

  // Copy 0 bytes; we're done
  if (end === start) return 0;
  if (target.length == 0 || source.length == 0) return 0;

  if (target_start < 0 || target_start >= target.length) {
    throw new Error('targetStart out of bounds');
  }

  if (start < 0 || start >= source.length) {
    throw new Error('sourceStart out of bounds');
  }

  if (end < 0 || end > source.length) {
    throw new Error('sourceEnd out of bounds');
  }

  // Are we oob?
  if (end > this.length) {
    end = this.length;
  }

  if (target.length - target_start < end - start) {
    end = target.length - target_start + start;
  }

  var temp = [];
  for (var i=start; i<end; i++) {
    assert.ok(typeof this[i] !== 'undefined', "copying undefined buffer bytes!");
    temp.push(this[i]);
  }

  for (var i=target_start; i<target_start+temp.length; i++) {
    target[i] = temp[i-target_start];
  }
};

// fill(value, start=0, end=buffer.length)
Buffer.prototype.fill = function fill(value, start, end) {
  value || (value = 0);
  start || (start = 0);
  end || (end = this.length);

  if (typeof value === 'string') {
    value = value.charCodeAt(0);
  }
  if (!(typeof value === 'number') || isNaN(value)) {
    throw new Error('value is not a number');
  }

  if (end < start) throw new Error('end < start');

  // Fill 0 bytes; we're done
  if (end === start) return 0;
  if (this.length == 0) return 0;

  if (start < 0 || start >= this.length) {
    throw new Error('start out of bounds');
  }

  if (end < 0 || end > this.length) {
    throw new Error('end out of bounds');
  }

  for (var i = start; i < end; i++) {
    this[i] = value;
  }
}

// Static methods
Buffer.isBuffer = function isBuffer(b) {
  return b instanceof Buffer || b instanceof Buffer;
};

Buffer.concat = function (list, totalLength) {
  if (!isArray(list)) {
    throw new Error("Usage: Buffer.concat(list, [totalLength])\n \
      list should be an Array.");
  }

  if (list.length === 0) {
    return new Buffer(0);
  } else if (list.length === 1) {
    return list[0];
  }

  if (typeof totalLength !== 'number') {
    totalLength = 0;
    for (var i = 0; i < list.length; i++) {
      var buf = list[i];
      totalLength += buf.length;
    }
  }

  var buffer = new Buffer(totalLength);
  var pos = 0;
  for (var i = 0; i < list.length; i++) {
    var buf = list[i];
    buf.copy(buffer, pos);
    pos += buf.length;
  }
  return buffer;
};

Buffer.isEncoding = function(encoding) {
  switch ((encoding + '').toLowerCase()) {
    case 'hex':
    case 'utf8':
    case 'utf-8':
    case 'ascii':
    case 'binary':
    case 'base64':
    case 'ucs2':
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
    case 'raw':
      return true;

    default:
      return false;
  }
};

// helpers

function coerce(length) {
  // Coerce length to a number (possibly NaN), round up
  // in case it's fractional (e.g. 123.456) then do a
  // double negate to coerce a NaN to 0. Easy, right?
  length = ~~Math.ceil(+length);
  return length < 0 ? 0 : length;
}

function isArray(subject) {
  return (Array.isArray ||
    function(subject){
      return {}.toString.apply(subject) == '[object Array]'
    })
    (subject)
}

function isArrayIsh(subject) {
  return isArray(subject) || Buffer.isBuffer(subject) ||
         subject && typeof subject === 'object' &&
         typeof subject.length === 'number';
}

function toHex(n) {
  if (n < 16) return '0' + n.toString(16);
  return n.toString(16);
}

function utf8ToBytes(str) {
  var byteArray = [];
  for (var i = 0; i < str.length; i++)
    if (str.charCodeAt(i) <= 0x7F)
      byteArray.push(str.charCodeAt(i));
    else {
      var h = encodeURIComponent(str.charAt(i)).substr(1).split('%');
      for (var j = 0; j < h.length; j++)
        byteArray.push(parseInt(h[j], 16));
    }

  return byteArray;
}

function asciiToBytes(str) {
  var byteArray = []
  for (var i = 0; i < str.length; i++ )
    // Node's code seems to be doing this and not & 0x7F..
    byteArray.push( str.charCodeAt(i) & 0xFF );

  return byteArray;
}

function base64ToBytes(str) {
  return require("base64-js").toByteArray(str);
}

function blitBuffer(src, dst, offset, length) {
  var pos, i = 0;
  while (i < length) {
    if ((i+offset >= dst.length) || (i >= src.length))
      break;

    dst[i + offset] = src[i];
    i++;
  }
  return i;
}

function decodeUtf8Char(str) {
  try {
    return decodeURIComponent(str);
  } catch (err) {
    return String.fromCharCode(0xFFFD); // UTF 8 invalid char
  }
}

// read/write bit-twiddling

Buffer.prototype.readUInt8 = function(offset, noAssert) {
  var buffer = this;

  if (!noAssert) {
    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset < buffer.length,
        'Trying to read beyond buffer length');
  }

  if (offset >= buffer.length) return;

  return buffer[offset];
};

function readUInt16(buffer, offset, isBigEndian, noAssert) {
  var val = 0;


  if (!noAssert) {
    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 1 < buffer.length,
        'Trying to read beyond buffer length');
  }

  if (offset >= buffer.length) return 0;

  if (isBigEndian) {
    val = buffer[offset] << 8;
    if (offset + 1 < buffer.length) {
      val |= buffer[offset + 1];
    }
  } else {
    val = buffer[offset];
    if (offset + 1 < buffer.length) {
      val |= buffer[offset + 1] << 8;
    }
  }

  return val;
}

Buffer.prototype.readUInt16LE = function(offset, noAssert) {
  return readUInt16(this, offset, false, noAssert);
};

Buffer.prototype.readUInt16BE = function(offset, noAssert) {
  return readUInt16(this, offset, true, noAssert);
};

function readUInt32(buffer, offset, isBigEndian, noAssert) {
  var val = 0;

  if (!noAssert) {
    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 3 < buffer.length,
        'Trying to read beyond buffer length');
  }

  if (offset >= buffer.length) return 0;

  if (isBigEndian) {
    if (offset + 1 < buffer.length)
      val = buffer[offset + 1] << 16;
    if (offset + 2 < buffer.length)
      val |= buffer[offset + 2] << 8;
    if (offset + 3 < buffer.length)
      val |= buffer[offset + 3];
    val = val + (buffer[offset] << 24 >>> 0);
  } else {
    if (offset + 2 < buffer.length)
      val = buffer[offset + 2] << 16;
    if (offset + 1 < buffer.length)
      val |= buffer[offset + 1] << 8;
    val |= buffer[offset];
    if (offset + 3 < buffer.length)
      val = val + (buffer[offset + 3] << 24 >>> 0);
  }

  return val;
}

Buffer.prototype.readUInt32LE = function(offset, noAssert) {
  return readUInt32(this, offset, false, noAssert);
};

Buffer.prototype.readUInt32BE = function(offset, noAssert) {
  return readUInt32(this, offset, true, noAssert);
};


/*
 * Signed integer types, yay team! A reminder on how two's complement actually
 * works. The first bit is the signed bit, i.e. tells us whether or not the
 * number should be positive or negative. If the two's complement value is
 * positive, then we're done, as it's equivalent to the unsigned representation.
 *
 * Now if the number is positive, you're pretty much done, you can just leverage
 * the unsigned translations and return those. Unfortunately, negative numbers
 * aren't quite that straightforward.
 *
 * At first glance, one might be inclined to use the traditional formula to
 * translate binary numbers between the positive and negative values in two's
 * complement. (Though it doesn't quite work for the most negative value)
 * Mainly:
 *  - invert all the bits
 *  - add one to the result
 *
 * Of course, this doesn't quite work in Javascript. Take for example the value
 * of -128. This could be represented in 16 bits (big-endian) as 0xff80. But of
 * course, Javascript will do the following:
 *
 * > ~0xff80
 * -65409
 *
 * Whoh there, Javascript, that's not quite right. But wait, according to
 * Javascript that's perfectly correct. When Javascript ends up seeing the
 * constant 0xff80, it has no notion that it is actually a signed number. It
 * assumes that we've input the unsigned value 0xff80. Thus, when it does the
 * binary negation, it casts it into a signed value, (positive 0xff80). Then
 * when you perform binary negation on that, it turns it into a negative number.
 *
 * Instead, we're going to have to use the following general formula, that works
 * in a rather Javascript friendly way. I'm glad we don't support this kind of
 * weird numbering scheme in the kernel.
 *
 * (BIT-MAX - (unsigned)val + 1) * -1
 *
 * The astute observer, may think that this doesn't make sense for 8-bit numbers
 * (really it isn't necessary for them). However, when you get 16-bit numbers,
 * you do. Let's go back to our prior example and see how this will look:
 *
 * (0xffff - 0xff80 + 1) * -1
 * (0x007f + 1) * -1
 * (0x0080) * -1
 */
Buffer.prototype.readInt8 = function(offset, noAssert) {
  var buffer = this;
  var neg;

  if (!noAssert) {
    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset < buffer.length,
        'Trying to read beyond buffer length');
  }

  if (offset >= buffer.length) return;

  neg = buffer[offset] & 0x80;
  if (!neg) {
    return (buffer[offset]);
  }

  return ((0xff - buffer[offset] + 1) * -1);
};

function readInt16(buffer, offset, isBigEndian, noAssert) {
  var neg, val;

  if (!noAssert) {
    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 1 < buffer.length,
        'Trying to read beyond buffer length');
  }

  val = readUInt16(buffer, offset, isBigEndian, noAssert);
  neg = val & 0x8000;
  if (!neg) {
    return val;
  }

  return (0xffff - val + 1) * -1;
}

Buffer.prototype.readInt16LE = function(offset, noAssert) {
  return readInt16(this, offset, false, noAssert);
};

Buffer.prototype.readInt16BE = function(offset, noAssert) {
  return readInt16(this, offset, true, noAssert);
};

function readInt32(buffer, offset, isBigEndian, noAssert) {
  var neg, val;

  if (!noAssert) {
    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 3 < buffer.length,
        'Trying to read beyond buffer length');
  }

  val = readUInt32(buffer, offset, isBigEndian, noAssert);
  neg = val & 0x80000000;
  if (!neg) {
    return (val);
  }

  return (0xffffffff - val + 1) * -1;
}

Buffer.prototype.readInt32LE = function(offset, noAssert) {
  return readInt32(this, offset, false, noAssert);
};

Buffer.prototype.readInt32BE = function(offset, noAssert) {
  return readInt32(this, offset, true, noAssert);
};

function readFloat(buffer, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset + 3 < buffer.length,
        'Trying to read beyond buffer length');
  }

  return require('./buffer_ieee754').readIEEE754(buffer, offset, isBigEndian,
      23, 4);
}

Buffer.prototype.readFloatLE = function(offset, noAssert) {
  return readFloat(this, offset, false, noAssert);
};

Buffer.prototype.readFloatBE = function(offset, noAssert) {
  return readFloat(this, offset, true, noAssert);
};

function readDouble(buffer, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset + 7 < buffer.length,
        'Trying to read beyond buffer length');
  }

  return require('./buffer_ieee754').readIEEE754(buffer, offset, isBigEndian,
      52, 8);
}

Buffer.prototype.readDoubleLE = function(offset, noAssert) {
  return readDouble(this, offset, false, noAssert);
};

Buffer.prototype.readDoubleBE = function(offset, noAssert) {
  return readDouble(this, offset, true, noAssert);
};


/*
 * We have to make sure that the value is a valid integer. This means that it is
 * non-negative. It has no fractional component and that it does not exceed the
 * maximum allowed value.
 *
 *      value           The number to check for validity
 *
 *      max             The maximum value
 */
function verifuint(value, max) {
  assert.ok(typeof (value) == 'number',
      'cannot write a non-number as a number');

  assert.ok(value >= 0,
      'specified a negative value for writing an unsigned value');

  assert.ok(value <= max, 'value is larger than maximum value for type');

  assert.ok(Math.floor(value) === value, 'value has a fractional component');
}

Buffer.prototype.writeUInt8 = function(value, offset, noAssert) {
  var buffer = this;

  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset < buffer.length,
        'trying to write beyond buffer length');

    verifuint(value, 0xff);
  }

  if (offset < buffer.length) {
    buffer[offset] = value;
  }
};

function writeUInt16(buffer, value, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 1 < buffer.length,
        'trying to write beyond buffer length');

    verifuint(value, 0xffff);
  }

  for (var i = 0; i < Math.min(buffer.length - offset, 2); i++) {
    buffer[offset + i] =
        (value & (0xff << (8 * (isBigEndian ? 1 - i : i)))) >>>
            (isBigEndian ? 1 - i : i) * 8;
  }

}

Buffer.prototype.writeUInt16LE = function(value, offset, noAssert) {
  writeUInt16(this, value, offset, false, noAssert);
};

Buffer.prototype.writeUInt16BE = function(value, offset, noAssert) {
  writeUInt16(this, value, offset, true, noAssert);
};

function writeUInt32(buffer, value, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 3 < buffer.length,
        'trying to write beyond buffer length');

    verifuint(value, 0xffffffff);
  }

  for (var i = 0; i < Math.min(buffer.length - offset, 4); i++) {
    buffer[offset + i] =
        (value >>> (isBigEndian ? 3 - i : i) * 8) & 0xff;
  }
}

Buffer.prototype.writeUInt32LE = function(value, offset, noAssert) {
  writeUInt32(this, value, offset, false, noAssert);
};

Buffer.prototype.writeUInt32BE = function(value, offset, noAssert) {
  writeUInt32(this, value, offset, true, noAssert);
};


/*
 * We now move onto our friends in the signed number category. Unlike unsigned
 * numbers, we're going to have to worry a bit more about how we put values into
 * arrays. Since we are only worrying about signed 32-bit values, we're in
 * slightly better shape. Unfortunately, we really can't do our favorite binary
 * & in this system. It really seems to do the wrong thing. For example:
 *
 * > -32 & 0xff
 * 224
 *
 * What's happening above is really: 0xe0 & 0xff = 0xe0. However, the results of
 * this aren't treated as a signed number. Ultimately a bad thing.
 *
 * What we're going to want to do is basically create the unsigned equivalent of
 * our representation and pass that off to the wuint* functions. To do that
 * we're going to do the following:
 *
 *  - if the value is positive
 *      we can pass it directly off to the equivalent wuint
 *  - if the value is negative
 *      we do the following computation:
 *         mb + val + 1, where
 *         mb   is the maximum unsigned value in that byte size
 *         val  is the Javascript negative integer
 *
 *
 * As a concrete value, take -128. In signed 16 bits this would be 0xff80. If
 * you do out the computations:
 *
 * 0xffff - 128 + 1
 * 0xffff - 127
 * 0xff80
 *
 * You can then encode this value as the signed version. This is really rather
 * hacky, but it should work and get the job done which is our goal here.
 */

/*
 * A series of checks to make sure we actually have a signed 32-bit number
 */
function verifsint(value, max, min) {
  assert.ok(typeof (value) == 'number',
      'cannot write a non-number as a number');

  assert.ok(value <= max, 'value larger than maximum allowed value');

  assert.ok(value >= min, 'value smaller than minimum allowed value');

  assert.ok(Math.floor(value) === value, 'value has a fractional component');
}

function verifIEEE754(value, max, min) {
  assert.ok(typeof (value) == 'number',
      'cannot write a non-number as a number');

  assert.ok(value <= max, 'value larger than maximum allowed value');

  assert.ok(value >= min, 'value smaller than minimum allowed value');
}

Buffer.prototype.writeInt8 = function(value, offset, noAssert) {
  var buffer = this;

  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset < buffer.length,
        'Trying to write beyond buffer length');

    verifsint(value, 0x7f, -0x80);
  }

  if (value >= 0) {
    buffer.writeUInt8(value, offset, noAssert);
  } else {
    buffer.writeUInt8(0xff + value + 1, offset, noAssert);
  }
};

function writeInt16(buffer, value, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 1 < buffer.length,
        'Trying to write beyond buffer length');

    verifsint(value, 0x7fff, -0x8000);
  }

  if (value >= 0) {
    writeUInt16(buffer, value, offset, isBigEndian, noAssert);
  } else {
    writeUInt16(buffer, 0xffff + value + 1, offset, isBigEndian, noAssert);
  }
}

Buffer.prototype.writeInt16LE = function(value, offset, noAssert) {
  writeInt16(this, value, offset, false, noAssert);
};

Buffer.prototype.writeInt16BE = function(value, offset, noAssert) {
  writeInt16(this, value, offset, true, noAssert);
};

function writeInt32(buffer, value, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 3 < buffer.length,
        'Trying to write beyond buffer length');

    verifsint(value, 0x7fffffff, -0x80000000);
  }

  if (value >= 0) {
    writeUInt32(buffer, value, offset, isBigEndian, noAssert);
  } else {
    writeUInt32(buffer, 0xffffffff + value + 1, offset, isBigEndian, noAssert);
  }
}

Buffer.prototype.writeInt32LE = function(value, offset, noAssert) {
  writeInt32(this, value, offset, false, noAssert);
};

Buffer.prototype.writeInt32BE = function(value, offset, noAssert) {
  writeInt32(this, value, offset, true, noAssert);
};

function writeFloat(buffer, value, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 3 < buffer.length,
        'Trying to write beyond buffer length');

    verifIEEE754(value, 3.4028234663852886e+38, -3.4028234663852886e+38);
  }

  require('./buffer_ieee754').writeIEEE754(buffer, value, offset, isBigEndian,
      23, 4);
}

Buffer.prototype.writeFloatLE = function(value, offset, noAssert) {
  writeFloat(this, value, offset, false, noAssert);
};

Buffer.prototype.writeFloatBE = function(value, offset, noAssert) {
  writeFloat(this, value, offset, true, noAssert);
};

function writeDouble(buffer, value, offset, isBigEndian, noAssert) {
  if (!noAssert) {
    assert.ok(value !== undefined && value !== null,
        'missing value');

    assert.ok(typeof (isBigEndian) === 'boolean',
        'missing or invalid endian');

    assert.ok(offset !== undefined && offset !== null,
        'missing offset');

    assert.ok(offset + 7 < buffer.length,
        'Trying to write beyond buffer length');

    verifIEEE754(value, 1.7976931348623157E+308, -1.7976931348623157E+308);
  }

  require('./buffer_ieee754').writeIEEE754(buffer, value, offset, isBigEndian,
      52, 8);
}

Buffer.prototype.writeDoubleLE = function(value, offset, noAssert) {
  writeDouble(this, value, offset, false, noAssert);
};

Buffer.prototype.writeDoubleBE = function(value, offset, noAssert) {
  writeDouble(this, value, offset, true, noAssert);
};

},{"./buffer_ieee754":6,"assert":11,"base64-js":8}],8:[function(require,module,exports){
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
		placeHolders = b64.indexOf('=');
		placeHolders = placeHolders > 0 ? b64.length - placeHolders : 0;

		// base64 is 4/3 + up to two characters of the original data
		arr = [];//new Uint8Array(b64.length * 3 / 4 - placeHolders);

		// if there are placeholders, only get up to the last complete 4 chars
		l = placeHolders > 0 ? b64.length - 4 : b64.length;

		for (i = 0, j = 0; i < l; i += 4, j += 3) {
			tmp = (lookup.indexOf(b64[i]) << 18) | (lookup.indexOf(b64[i + 1]) << 12) | (lookup.indexOf(b64[i + 2]) << 6) | lookup.indexOf(b64[i + 3]);
			arr.push((tmp & 0xFF0000) >> 16);
			arr.push((tmp & 0xFF00) >> 8);
			arr.push(tmp & 0xFF);
		}

		if (placeHolders === 2) {
			tmp = (lookup.indexOf(b64[i]) << 2) | (lookup.indexOf(b64[i + 1]) >> 4);
			arr.push(tmp & 0xFF);
		} else if (placeHolders === 1) {
			tmp = (lookup.indexOf(b64[i]) << 10) | (lookup.indexOf(b64[i + 1]) << 4) | (lookup.indexOf(b64[i + 2]) >> 2);
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
			return lookup[num >> 18 & 0x3F] + lookup[num >> 12 & 0x3F] + lookup[num >> 6 & 0x3F] + lookup[num & 0x3F];
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
				output += lookup[temp >> 2];
				output += lookup[(temp << 4) & 0x3F];
				output += '==';
				break;
			case 2:
				temp = (uint8[uint8.length - 2] << 8) + (uint8[uint8.length - 1]);
				output += lookup[temp >> 10];
				output += lookup[(temp >> 4) & 0x3F];
				output += lookup[(temp << 2) & 0x3F];
				output += '=';
				break;
		}

		return output;
	}

	module.exports.toByteArray = b64ToByteArray;
	module.exports.fromByteArray = uint8ToBase64;
}());

},{}],9:[function(require,module,exports){
var undefined = (void 0); // Paranoia

// Beyond this value, index getters/setters (i.e. array[0], array[1]) are so slow to
// create, and consume so much memory, that the browser appears frozen.
var MAX_ARRAY_LENGTH = 1e5;

// Approximations of internal ECMAScript conversion functions
var ECMAScript = (function() {
  // Stash a copy in case other scripts modify these
  var opts = Object.prototype.toString,
      ophop = Object.prototype.hasOwnProperty;

  return {
    // Class returns internal [[Class]] property, used to avoid cross-frame instanceof issues:
    Class: function(v) { return opts.call(v).replace(/^\[object *|\]$/g, ''); },
    HasProperty: function(o, p) { return p in o; },
    HasOwnProperty: function(o, p) { return ophop.call(o, p); },
    IsCallable: function(o) { return typeof o === 'function'; },
    ToInt32: function(v) { return v >> 0; },
    ToUint32: function(v) { return v >>> 0; }
  };
}());

// Snapshot intrinsics
var LN2 = Math.LN2,
    abs = Math.abs,
    floor = Math.floor,
    log = Math.log,
    min = Math.min,
    pow = Math.pow,
    round = Math.round;

// ES5: lock down object properties
function configureProperties(obj) {
  if (getOwnPropNames && defineProp) {
    var props = getOwnPropNames(obj), i;
    for (i = 0; i < props.length; i += 1) {
      defineProp(obj, props[i], {
        value: obj[props[i]],
        writable: false,
        enumerable: false,
        configurable: false
      });
    }
  }
}

// emulate ES5 getter/setter API using legacy APIs
// http://blogs.msdn.com/b/ie/archive/2010/09/07/transitioning-existing-code-to-the-es5-getter-setter-apis.aspx
// (second clause tests for Object.defineProperty() in IE<9 that only supports extending DOM prototypes, but
// note that IE<9 does not support __defineGetter__ or __defineSetter__ so it just renders the method harmless)
var defineProp
if (Object.defineProperty && (function() {
      try {
        Object.defineProperty({}, 'x', {});
        return true;
      } catch (e) {
        return false;
      }
    })()) {
  defineProp = Object.defineProperty;
} else {
  defineProp = function(o, p, desc) {
    if (!o === Object(o)) throw new TypeError("Object.defineProperty called on non-object");
    if (ECMAScript.HasProperty(desc, 'get') && Object.prototype.__defineGetter__) { Object.prototype.__defineGetter__.call(o, p, desc.get); }
    if (ECMAScript.HasProperty(desc, 'set') && Object.prototype.__defineSetter__) { Object.prototype.__defineSetter__.call(o, p, desc.set); }
    if (ECMAScript.HasProperty(desc, 'value')) { o[p] = desc.value; }
    return o;
  };
}

var getOwnPropNames = Object.getOwnPropertyNames || function (o) {
  if (o !== Object(o)) throw new TypeError("Object.getOwnPropertyNames called on non-object");
  var props = [], p;
  for (p in o) {
    if (ECMAScript.HasOwnProperty(o, p)) {
      props.push(p);
    }
  }
  return props;
};

// ES5: Make obj[index] an alias for obj._getter(index)/obj._setter(index, value)
// for index in 0 ... obj.length
function makeArrayAccessors(obj) {
  if (!defineProp) { return; }

  if (obj.length > MAX_ARRAY_LENGTH) throw new RangeError("Array too large for polyfill");

  function makeArrayAccessor(index) {
    defineProp(obj, index, {
      'get': function() { return obj._getter(index); },
      'set': function(v) { obj._setter(index, v); },
      enumerable: true,
      configurable: false
    });
  }

  var i;
  for (i = 0; i < obj.length; i += 1) {
    makeArrayAccessor(i);
  }
}

// Internal conversion functions:
//    pack<Type>()   - take a number (interpreted as Type), output a byte array
//    unpack<Type>() - take a byte array, output a Type-like number

function as_signed(value, bits) { var s = 32 - bits; return (value << s) >> s; }
function as_unsigned(value, bits) { var s = 32 - bits; return (value << s) >>> s; }

function packI8(n) { return [n & 0xff]; }
function unpackI8(bytes) { return as_signed(bytes[0], 8); }

function packU8(n) { return [n & 0xff]; }
function unpackU8(bytes) { return as_unsigned(bytes[0], 8); }

function packU8Clamped(n) { n = round(Number(n)); return [n < 0 ? 0 : n > 0xff ? 0xff : n & 0xff]; }

function packI16(n) { return [(n >> 8) & 0xff, n & 0xff]; }
function unpackI16(bytes) { return as_signed(bytes[0] << 8 | bytes[1], 16); }

function packU16(n) { return [(n >> 8) & 0xff, n & 0xff]; }
function unpackU16(bytes) { return as_unsigned(bytes[0] << 8 | bytes[1], 16); }

function packI32(n) { return [(n >> 24) & 0xff, (n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff]; }
function unpackI32(bytes) { return as_signed(bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3], 32); }

function packU32(n) { return [(n >> 24) & 0xff, (n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff]; }
function unpackU32(bytes) { return as_unsigned(bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3], 32); }

function packIEEE754(v, ebits, fbits) {

  var bias = (1 << (ebits - 1)) - 1,
      s, e, f, ln,
      i, bits, str, bytes;

  function roundToEven(n) {
    var w = floor(n), f = n - w;
    if (f < 0.5)
      return w;
    if (f > 0.5)
      return w + 1;
    return w % 2 ? w + 1 : w;
  }

  // Compute sign, exponent, fraction
  if (v !== v) {
    // NaN
    // http://dev.w3.org/2006/webapi/WebIDL/#es-type-mapping
    e = (1 << ebits) - 1; f = pow(2, fbits - 1); s = 0;
  } else if (v === Infinity || v === -Infinity) {
    e = (1 << ebits) - 1; f = 0; s = (v < 0) ? 1 : 0;
  } else if (v === 0) {
    e = 0; f = 0; s = (1 / v === -Infinity) ? 1 : 0;
  } else {
    s = v < 0;
    v = abs(v);

    if (v >= pow(2, 1 - bias)) {
      e = min(floor(log(v) / LN2), 1023);
      f = roundToEven(v / pow(2, e) * pow(2, fbits));
      if (f / pow(2, fbits) >= 2) {
        e = e + 1;
        f = 1;
      }
      if (e > bias) {
        // Overflow
        e = (1 << ebits) - 1;
        f = 0;
      } else {
        // Normalized
        e = e + bias;
        f = f - pow(2, fbits);
      }
    } else {
      // Denormalized
      e = 0;
      f = roundToEven(v / pow(2, 1 - bias - fbits));
    }
  }

  // Pack sign, exponent, fraction
  bits = [];
  for (i = fbits; i; i -= 1) { bits.push(f % 2 ? 1 : 0); f = floor(f / 2); }
  for (i = ebits; i; i -= 1) { bits.push(e % 2 ? 1 : 0); e = floor(e / 2); }
  bits.push(s ? 1 : 0);
  bits.reverse();
  str = bits.join('');

  // Bits to bytes
  bytes = [];
  while (str.length) {
    bytes.push(parseInt(str.substring(0, 8), 2));
    str = str.substring(8);
  }
  return bytes;
}

function unpackIEEE754(bytes, ebits, fbits) {

  // Bytes to bits
  var bits = [], i, j, b, str,
      bias, s, e, f;

  for (i = bytes.length; i; i -= 1) {
    b = bytes[i - 1];
    for (j = 8; j; j -= 1) {
      bits.push(b % 2 ? 1 : 0); b = b >> 1;
    }
  }
  bits.reverse();
  str = bits.join('');

  // Unpack sign, exponent, fraction
  bias = (1 << (ebits - 1)) - 1;
  s = parseInt(str.substring(0, 1), 2) ? -1 : 1;
  e = parseInt(str.substring(1, 1 + ebits), 2);
  f = parseInt(str.substring(1 + ebits), 2);

  // Produce number
  if (e === (1 << ebits) - 1) {
    return f !== 0 ? NaN : s * Infinity;
  } else if (e > 0) {
    // Normalized
    return s * pow(2, e - bias) * (1 + f / pow(2, fbits));
  } else if (f !== 0) {
    // Denormalized
    return s * pow(2, -(bias - 1)) * (f / pow(2, fbits));
  } else {
    return s < 0 ? -0 : 0;
  }
}

function unpackF64(b) { return unpackIEEE754(b, 11, 52); }
function packF64(v) { return packIEEE754(v, 11, 52); }
function unpackF32(b) { return unpackIEEE754(b, 8, 23); }
function packF32(v) { return packIEEE754(v, 8, 23); }


//
// 3 The ArrayBuffer Type
//

(function() {

  /** @constructor */
  var ArrayBuffer = function ArrayBuffer(length) {
    length = ECMAScript.ToInt32(length);
    if (length < 0) throw new RangeError('ArrayBuffer size is not a small enough positive integer');

    this.byteLength = length;
    this._bytes = [];
    this._bytes.length = length;

    var i;
    for (i = 0; i < this.byteLength; i += 1) {
      this._bytes[i] = 0;
    }

    configureProperties(this);
  };

  exports.ArrayBuffer = exports.ArrayBuffer || ArrayBuffer;

  //
  // 4 The ArrayBufferView Type
  //

  // NOTE: this constructor is not exported
  /** @constructor */
  var ArrayBufferView = function ArrayBufferView() {
    //this.buffer = null;
    //this.byteOffset = 0;
    //this.byteLength = 0;
  };

  //
  // 5 The Typed Array View Types
  //

  function makeConstructor(bytesPerElement, pack, unpack) {
    // Each TypedArray type requires a distinct constructor instance with
    // identical logic, which this produces.

    var ctor;
    ctor = function(buffer, byteOffset, length) {
      var array, sequence, i, s;

      if (!arguments.length || typeof arguments[0] === 'number') {
        // Constructor(unsigned long length)
        this.length = ECMAScript.ToInt32(arguments[0]);
        if (length < 0) throw new RangeError('ArrayBufferView size is not a small enough positive integer');

        this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        this.buffer = new ArrayBuffer(this.byteLength);
        this.byteOffset = 0;
      } else if (typeof arguments[0] === 'object' && arguments[0].constructor === ctor) {
        // Constructor(TypedArray array)
        array = arguments[0];

        this.length = array.length;
        this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        this.buffer = new ArrayBuffer(this.byteLength);
        this.byteOffset = 0;

        for (i = 0; i < this.length; i += 1) {
          this._setter(i, array._getter(i));
        }
      } else if (typeof arguments[0] === 'object' &&
                 !(arguments[0] instanceof ArrayBuffer || ECMAScript.Class(arguments[0]) === 'ArrayBuffer')) {
        // Constructor(sequence<type> array)
        sequence = arguments[0];

        this.length = ECMAScript.ToUint32(sequence.length);
        this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        this.buffer = new ArrayBuffer(this.byteLength);
        this.byteOffset = 0;

        for (i = 0; i < this.length; i += 1) {
          s = sequence[i];
          this._setter(i, Number(s));
        }
      } else if (typeof arguments[0] === 'object' &&
                 (arguments[0] instanceof ArrayBuffer || ECMAScript.Class(arguments[0]) === 'ArrayBuffer')) {
        // Constructor(ArrayBuffer buffer,
        //             optional unsigned long byteOffset, optional unsigned long length)
        this.buffer = buffer;

        this.byteOffset = ECMAScript.ToUint32(byteOffset);
        if (this.byteOffset > this.buffer.byteLength) {
          throw new RangeError("byteOffset out of range");
        }

        if (this.byteOffset % this.BYTES_PER_ELEMENT) {
          // The given byteOffset must be a multiple of the element
          // size of the specific type, otherwise an exception is raised.
          throw new RangeError("ArrayBuffer length minus the byteOffset is not a multiple of the element size.");
        }

        if (arguments.length < 3) {
          this.byteLength = this.buffer.byteLength - this.byteOffset;

          if (this.byteLength % this.BYTES_PER_ELEMENT) {
            throw new RangeError("length of buffer minus byteOffset not a multiple of the element size");
          }
          this.length = this.byteLength / this.BYTES_PER_ELEMENT;
        } else {
          this.length = ECMAScript.ToUint32(length);
          this.byteLength = this.length * this.BYTES_PER_ELEMENT;
        }

        if ((this.byteOffset + this.byteLength) > this.buffer.byteLength) {
          throw new RangeError("byteOffset and length reference an area beyond the end of the buffer");
        }
      } else {
        throw new TypeError("Unexpected argument type(s)");
      }

      this.constructor = ctor;

      configureProperties(this);
      makeArrayAccessors(this);
    };

    ctor.prototype = new ArrayBufferView();
    ctor.prototype.BYTES_PER_ELEMENT = bytesPerElement;
    ctor.prototype._pack = pack;
    ctor.prototype._unpack = unpack;
    ctor.BYTES_PER_ELEMENT = bytesPerElement;

    // getter type (unsigned long index);
    ctor.prototype._getter = function(index) {
      if (arguments.length < 1) throw new SyntaxError("Not enough arguments");

      index = ECMAScript.ToUint32(index);
      if (index >= this.length) {
        return undefined;
      }

      var bytes = [], i, o;
      for (i = 0, o = this.byteOffset + index * this.BYTES_PER_ELEMENT;
           i < this.BYTES_PER_ELEMENT;
           i += 1, o += 1) {
        bytes.push(this.buffer._bytes[o]);
      }
      return this._unpack(bytes);
    };

    // NONSTANDARD: convenience alias for getter: type get(unsigned long index);
    ctor.prototype.get = ctor.prototype._getter;

    // setter void (unsigned long index, type value);
    ctor.prototype._setter = function(index, value) {
      if (arguments.length < 2) throw new SyntaxError("Not enough arguments");

      index = ECMAScript.ToUint32(index);
      if (index >= this.length) {
        return undefined;
      }

      var bytes = this._pack(value), i, o;
      for (i = 0, o = this.byteOffset + index * this.BYTES_PER_ELEMENT;
           i < this.BYTES_PER_ELEMENT;
           i += 1, o += 1) {
        this.buffer._bytes[o] = bytes[i];
      }
    };

    // void set(TypedArray array, optional unsigned long offset);
    // void set(sequence<type> array, optional unsigned long offset);
    ctor.prototype.set = function(index, value) {
      if (arguments.length < 1) throw new SyntaxError("Not enough arguments");
      var array, sequence, offset, len,
          i, s, d,
          byteOffset, byteLength, tmp;

      if (typeof arguments[0] === 'object' && arguments[0].constructor === this.constructor) {
        // void set(TypedArray array, optional unsigned long offset);
        array = arguments[0];
        offset = ECMAScript.ToUint32(arguments[1]);

        if (offset + array.length > this.length) {
          throw new RangeError("Offset plus length of array is out of range");
        }

        byteOffset = this.byteOffset + offset * this.BYTES_PER_ELEMENT;
        byteLength = array.length * this.BYTES_PER_ELEMENT;

        if (array.buffer === this.buffer) {
          tmp = [];
          for (i = 0, s = array.byteOffset; i < byteLength; i += 1, s += 1) {
            tmp[i] = array.buffer._bytes[s];
          }
          for (i = 0, d = byteOffset; i < byteLength; i += 1, d += 1) {
            this.buffer._bytes[d] = tmp[i];
          }
        } else {
          for (i = 0, s = array.byteOffset, d = byteOffset;
               i < byteLength; i += 1, s += 1, d += 1) {
            this.buffer._bytes[d] = array.buffer._bytes[s];
          }
        }
      } else if (typeof arguments[0] === 'object' && typeof arguments[0].length !== 'undefined') {
        // void set(sequence<type> array, optional unsigned long offset);
        sequence = arguments[0];
        len = ECMAScript.ToUint32(sequence.length);
        offset = ECMAScript.ToUint32(arguments[1]);

        if (offset + len > this.length) {
          throw new RangeError("Offset plus length of array is out of range");
        }

        for (i = 0; i < len; i += 1) {
          s = sequence[i];
          this._setter(offset + i, Number(s));
        }
      } else {
        throw new TypeError("Unexpected argument type(s)");
      }
    };

    // TypedArray subarray(long begin, optional long end);
    ctor.prototype.subarray = function(start, end) {
      function clamp(v, min, max) { return v < min ? min : v > max ? max : v; }

      start = ECMAScript.ToInt32(start);
      end = ECMAScript.ToInt32(end);

      if (arguments.length < 1) { start = 0; }
      if (arguments.length < 2) { end = this.length; }

      if (start < 0) { start = this.length + start; }
      if (end < 0) { end = this.length + end; }

      start = clamp(start, 0, this.length);
      end = clamp(end, 0, this.length);

      var len = end - start;
      if (len < 0) {
        len = 0;
      }

      return new this.constructor(
        this.buffer, this.byteOffset + start * this.BYTES_PER_ELEMENT, len);
    };

    return ctor;
  }

  var Int8Array = makeConstructor(1, packI8, unpackI8);
  var Uint8Array = makeConstructor(1, packU8, unpackU8);
  var Uint8ClampedArray = makeConstructor(1, packU8Clamped, unpackU8);
  var Int16Array = makeConstructor(2, packI16, unpackI16);
  var Uint16Array = makeConstructor(2, packU16, unpackU16);
  var Int32Array = makeConstructor(4, packI32, unpackI32);
  var Uint32Array = makeConstructor(4, packU32, unpackU32);
  var Float32Array = makeConstructor(4, packF32, unpackF32);
  var Float64Array = makeConstructor(8, packF64, unpackF64);

  exports.Int8Array = exports.Int8Array || Int8Array;
  exports.Uint8Array = exports.Uint8Array || Uint8Array;
  exports.Uint8ClampedArray = exports.Uint8ClampedArray || Uint8ClampedArray;
  exports.Int16Array = exports.Int16Array || Int16Array;
  exports.Uint16Array = exports.Uint16Array || Uint16Array;
  exports.Int32Array = exports.Int32Array || Int32Array;
  exports.Uint32Array = exports.Uint32Array || Uint32Array;
  exports.Float32Array = exports.Float32Array || Float32Array;
  exports.Float64Array = exports.Float64Array || Float64Array;
}());

//
// 6 The DataView View Type
//

(function() {
  function r(array, index) {
    return ECMAScript.IsCallable(array.get) ? array.get(index) : array[index];
  }

  var IS_BIG_ENDIAN = (function() {
    var u16array = new(exports.Uint16Array)([0x1234]),
        u8array = new(exports.Uint8Array)(u16array.buffer);
    return r(u8array, 0) === 0x12;
  }());

  // Constructor(ArrayBuffer buffer,
  //             optional unsigned long byteOffset,
  //             optional unsigned long byteLength)
  /** @constructor */
  var DataView = function DataView(buffer, byteOffset, byteLength) {
    if (arguments.length === 0) {
      buffer = new exports.ArrayBuffer(0);
    } else if (!(buffer instanceof exports.ArrayBuffer || ECMAScript.Class(buffer) === 'ArrayBuffer')) {
      throw new TypeError("TypeError");
    }

    this.buffer = buffer || new exports.ArrayBuffer(0);

    this.byteOffset = ECMAScript.ToUint32(byteOffset);
    if (this.byteOffset > this.buffer.byteLength) {
      throw new RangeError("byteOffset out of range");
    }

    if (arguments.length < 3) {
      this.byteLength = this.buffer.byteLength - this.byteOffset;
    } else {
      this.byteLength = ECMAScript.ToUint32(byteLength);
    }

    if ((this.byteOffset + this.byteLength) > this.buffer.byteLength) {
      throw new RangeError("byteOffset and length reference an area beyond the end of the buffer");
    }

    configureProperties(this);
  };

  function makeGetter(arrayType) {
    return function(byteOffset, littleEndian) {

      byteOffset = ECMAScript.ToUint32(byteOffset);

      if (byteOffset + arrayType.BYTES_PER_ELEMENT > this.byteLength) {
        throw new RangeError("Array index out of range");
      }
      byteOffset += this.byteOffset;

      var uint8Array = new exports.Uint8Array(this.buffer, byteOffset, arrayType.BYTES_PER_ELEMENT),
          bytes = [], i;
      for (i = 0; i < arrayType.BYTES_PER_ELEMENT; i += 1) {
        bytes.push(r(uint8Array, i));
      }

      if (Boolean(littleEndian) === Boolean(IS_BIG_ENDIAN)) {
        bytes.reverse();
      }

      return r(new arrayType(new exports.Uint8Array(bytes).buffer), 0);
    };
  }

  DataView.prototype.getUint8 = makeGetter(exports.Uint8Array);
  DataView.prototype.getInt8 = makeGetter(exports.Int8Array);
  DataView.prototype.getUint16 = makeGetter(exports.Uint16Array);
  DataView.prototype.getInt16 = makeGetter(exports.Int16Array);
  DataView.prototype.getUint32 = makeGetter(exports.Uint32Array);
  DataView.prototype.getInt32 = makeGetter(exports.Int32Array);
  DataView.prototype.getFloat32 = makeGetter(exports.Float32Array);
  DataView.prototype.getFloat64 = makeGetter(exports.Float64Array);

  function makeSetter(arrayType) {
    return function(byteOffset, value, littleEndian) {

      byteOffset = ECMAScript.ToUint32(byteOffset);
      if (byteOffset + arrayType.BYTES_PER_ELEMENT > this.byteLength) {
        throw new RangeError("Array index out of range");
      }

      // Get bytes
      var typeArray = new arrayType([value]),
          byteArray = new exports.Uint8Array(typeArray.buffer),
          bytes = [], i, byteView;

      for (i = 0; i < arrayType.BYTES_PER_ELEMENT; i += 1) {
        bytes.push(r(byteArray, i));
      }

      // Flip if necessary
      if (Boolean(littleEndian) === Boolean(IS_BIG_ENDIAN)) {
        bytes.reverse();
      }

      // Write them
      byteView = new exports.Uint8Array(this.buffer, byteOffset, arrayType.BYTES_PER_ELEMENT);
      byteView.set(bytes);
    };
  }

  DataView.prototype.setUint8 = makeSetter(exports.Uint8Array);
  DataView.prototype.setInt8 = makeSetter(exports.Int8Array);
  DataView.prototype.setUint16 = makeSetter(exports.Uint16Array);
  DataView.prototype.setInt16 = makeSetter(exports.Int16Array);
  DataView.prototype.setUint32 = makeSetter(exports.Uint32Array);
  DataView.prototype.setInt32 = makeSetter(exports.Int32Array);
  DataView.prototype.setFloat32 = makeSetter(exports.Float32Array);
  DataView.prototype.setFloat64 = makeSetter(exports.Float64Array);

  exports.DataView = exports.DataView || DataView;

}());

},{}],10:[function(require,module,exports){
var global=typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {},Buffer=require("__browserify_Buffer");var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 50

var newTarget = NewBuffer(LENGTH)
var oldTarget = OldBuffer(LENGTH)
var nodeTarget = Buffer(LENGTH)

suite.add('NewBuffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    newTarget[i] = i + 97
  }
})
.add('OldBuffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    oldTarget[i] = i + 97
  }
})
.add('Buffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    nodeTarget[i] = i + 97
  }
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})
.on('complete', function () {
  console.log('Fastest is ' + this.filter('fastest').pluck('name'))
})
.run({ 'async': true })

},{"../../":3,"__browserify_Buffer":1,"benchmark":5,"buffer-browserify":7}],11:[function(require,module,exports){
// http://wiki.commonjs.org/wiki/Unit_Testing/1.0
//
// THIS IS NOT TESTED NOR LIKELY TO WORK OUTSIDE V8!
//
// Originally from narwhal.js (http://narwhaljs.org)
// Copyright (c) 2009 Thomas Robinson <280north.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the 'Software'), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// when used in node, this will actually load the util module we depend on
// versus loading the builtin util module as happens otherwise
// this is a bug in node module loading as far as I am concerned
var util = require('util/');

var pSlice = Array.prototype.slice;
var hasOwn = Object.prototype.hasOwnProperty;

// 1. The assert module provides functions that throw
// AssertionError's when particular conditions are not met. The
// assert module must conform to the following interface.

var assert = module.exports = ok;

// 2. The AssertionError is defined in assert.
// new assert.AssertionError({ message: message,
//                             actual: actual,
//                             expected: expected })

assert.AssertionError = function AssertionError(options) {
  this.name = 'AssertionError';
  this.actual = options.actual;
  this.expected = options.expected;
  this.operator = options.operator;
  if (options.message) {
    this.message = options.message;
    this.generatedMessage = false;
  } else {
    this.message = getMessage(this);
    this.generatedMessage = true;
  }
  var stackStartFunction = options.stackStartFunction || fail;

  if (Error.captureStackTrace) {
    Error.captureStackTrace(this, stackStartFunction);
  }
};

// assert.AssertionError instanceof Error
util.inherits(assert.AssertionError, Error);

function replacer(key, value) {
  if (util.isUndefined(value)) {
    return '' + value;
  }
  if (util.isNumber(value) && (isNaN(value) || !isFinite(value))) {
    return value.toString();
  }
  if (util.isFunction(value) || util.isRegExp(value)) {
    return value.toString();
  }
  return value;
}

function truncate(s, n) {
  if (util.isString(s)) {
    return s.length < n ? s : s.slice(0, n);
  } else {
    return s;
  }
}

function getMessage(self) {
  return truncate(JSON.stringify(self.actual, replacer), 128) + ' ' +
         self.operator + ' ' +
         truncate(JSON.stringify(self.expected, replacer), 128);
}

// At present only the three keys mentioned above are used and
// understood by the spec. Implementations or sub modules can pass
// other keys to the AssertionError's constructor - they will be
// ignored.

// 3. All of the following functions must throw an AssertionError
// when a corresponding condition is not met, with a message that
// may be undefined if not provided.  All assertion methods provide
// both the actual and expected values to the assertion error for
// display purposes.

function fail(actual, expected, message, operator, stackStartFunction) {
  throw new assert.AssertionError({
    message: message,
    actual: actual,
    expected: expected,
    operator: operator,
    stackStartFunction: stackStartFunction
  });
}

// EXTENSION! allows for well behaved errors defined elsewhere.
assert.fail = fail;

// 4. Pure assertion tests whether a value is truthy, as determined
// by !!guard.
// assert.ok(guard, message_opt);
// This statement is equivalent to assert.equal(true, !!guard,
// message_opt);. To test strictly for the value true, use
// assert.strictEqual(true, guard, message_opt);.

function ok(value, message) {
  if (!value) fail(value, true, message, '==', assert.ok);
}
assert.ok = ok;

// 5. The equality assertion tests shallow, coercive equality with
// ==.
// assert.equal(actual, expected, message_opt);

assert.equal = function equal(actual, expected, message) {
  if (actual != expected) fail(actual, expected, message, '==', assert.equal);
};

// 6. The non-equality assertion tests for whether two objects are not equal
// with != assert.notEqual(actual, expected, message_opt);

assert.notEqual = function notEqual(actual, expected, message) {
  if (actual == expected) {
    fail(actual, expected, message, '!=', assert.notEqual);
  }
};

// 7. The equivalence assertion tests a deep equality relation.
// assert.deepEqual(actual, expected, message_opt);

assert.deepEqual = function deepEqual(actual, expected, message) {
  if (!_deepEqual(actual, expected)) {
    fail(actual, expected, message, 'deepEqual', assert.deepEqual);
  }
};

function _deepEqual(actual, expected) {
  // 7.1. All identical values are equivalent, as determined by ===.
  if (actual === expected) {
    return true;

  } else if (util.isBuffer(actual) && util.isBuffer(expected)) {
    if (actual.length != expected.length) return false;

    for (var i = 0; i < actual.length; i++) {
      if (actual[i] !== expected[i]) return false;
    }

    return true;

  // 7.2. If the expected value is a Date object, the actual value is
  // equivalent if it is also a Date object that refers to the same time.
  } else if (util.isDate(actual) && util.isDate(expected)) {
    return actual.getTime() === expected.getTime();

  // 7.3 If the expected value is a RegExp object, the actual value is
  // equivalent if it is also a RegExp object with the same source and
  // properties (`global`, `multiline`, `lastIndex`, `ignoreCase`).
  } else if (util.isRegExp(actual) && util.isRegExp(expected)) {
    return actual.source === expected.source &&
           actual.global === expected.global &&
           actual.multiline === expected.multiline &&
           actual.lastIndex === expected.lastIndex &&
           actual.ignoreCase === expected.ignoreCase;

  // 7.4. Other pairs that do not both pass typeof value == 'object',
  // equivalence is determined by ==.
  } else if (!util.isObject(actual) && !util.isObject(expected)) {
    return actual == expected;

  // 7.5 For all other Object pairs, including Array objects, equivalence is
  // determined by having the same number of owned properties (as verified
  // with Object.prototype.hasOwnProperty.call), the same set of keys
  // (although not necessarily the same order), equivalent values for every
  // corresponding key, and an identical 'prototype' property. Note: this
  // accounts for both named and indexed properties on Arrays.
  } else {
    return objEquiv(actual, expected);
  }
}

function isArguments(object) {
  return Object.prototype.toString.call(object) == '[object Arguments]';
}

function objEquiv(a, b) {
  if (util.isNullOrUndefined(a) || util.isNullOrUndefined(b))
    return false;
  // an identical 'prototype' property.
  if (a.prototype !== b.prototype) return false;
  //~~~I've managed to break Object.keys through screwy arguments passing.
  //   Converting to array solves the problem.
  if (isArguments(a)) {
    if (!isArguments(b)) {
      return false;
    }
    a = pSlice.call(a);
    b = pSlice.call(b);
    return _deepEqual(a, b);
  }
  try {
    var ka = objectKeys(a),
        kb = objectKeys(b),
        key, i;
  } catch (e) {//happens when one is a string literal and the other isn't
    return false;
  }
  // having the same number of owned properties (keys incorporates
  // hasOwnProperty)
  if (ka.length != kb.length)
    return false;
  //the same set of keys (although not necessarily the same order),
  ka.sort();
  kb.sort();
  //~~~cheap key test
  for (i = ka.length - 1; i >= 0; i--) {
    if (ka[i] != kb[i])
      return false;
  }
  //equivalent values for every corresponding key, and
  //~~~possibly expensive deep test
  for (i = ka.length - 1; i >= 0; i--) {
    key = ka[i];
    if (!_deepEqual(a[key], b[key])) return false;
  }
  return true;
}

// 8. The non-equivalence assertion tests for any deep inequality.
// assert.notDeepEqual(actual, expected, message_opt);

assert.notDeepEqual = function notDeepEqual(actual, expected, message) {
  if (_deepEqual(actual, expected)) {
    fail(actual, expected, message, 'notDeepEqual', assert.notDeepEqual);
  }
};

// 9. The strict equality assertion tests strict equality, as determined by ===.
// assert.strictEqual(actual, expected, message_opt);

assert.strictEqual = function strictEqual(actual, expected, message) {
  if (actual !== expected) {
    fail(actual, expected, message, '===', assert.strictEqual);
  }
};

// 10. The strict non-equality assertion tests for strict inequality, as
// determined by !==.  assert.notStrictEqual(actual, expected, message_opt);

assert.notStrictEqual = function notStrictEqual(actual, expected, message) {
  if (actual === expected) {
    fail(actual, expected, message, '!==', assert.notStrictEqual);
  }
};

function expectedException(actual, expected) {
  if (!actual || !expected) {
    return false;
  }

  if (Object.prototype.toString.call(expected) == '[object RegExp]') {
    return expected.test(actual);
  } else if (actual instanceof expected) {
    return true;
  } else if (expected.call({}, actual) === true) {
    return true;
  }

  return false;
}

function _throws(shouldThrow, block, expected, message) {
  var actual;

  if (util.isString(expected)) {
    message = expected;
    expected = null;
  }

  try {
    block();
  } catch (e) {
    actual = e;
  }

  message = (expected && expected.name ? ' (' + expected.name + ').' : '.') +
            (message ? ' ' + message : '.');

  if (shouldThrow && !actual) {
    fail(actual, expected, 'Missing expected exception' + message);
  }

  if (!shouldThrow && expectedException(actual, expected)) {
    fail(actual, expected, 'Got unwanted exception' + message);
  }

  if ((shouldThrow && actual && expected &&
      !expectedException(actual, expected)) || (!shouldThrow && actual)) {
    throw actual;
  }
}

// 11. Expected to throw an error:
// assert.throws(block, Error_opt, message_opt);

assert.throws = function(block, /*optional*/error, /*optional*/message) {
  _throws.apply(this, [true].concat(pSlice.call(arguments)));
};

// EXTENSION! This is annoying to write outside this module.
assert.doesNotThrow = function(block, /*optional*/message) {
  _throws.apply(this, [false].concat(pSlice.call(arguments)));
};

assert.ifError = function(err) { if (err) {throw err;}};

var objectKeys = Object.keys || function (obj) {
  var keys = [];
  for (var key in obj) {
    if (hasOwn.call(obj, key)) keys.push(key);
  }
  return keys;
};

},{"util/":14}],12:[function(require,module,exports){
if (typeof Object.create === 'function') {
  // implementation from standard node.js 'util' module
  module.exports = function inherits(ctor, superCtor) {
    ctor.super_ = superCtor
    ctor.prototype = Object.create(superCtor.prototype, {
      constructor: {
        value: ctor,
        enumerable: false,
        writable: true,
        configurable: true
      }
    });
  };
} else {
  // old school shim for old browsers
  module.exports = function inherits(ctor, superCtor) {
    ctor.super_ = superCtor
    var TempCtor = function () {}
    TempCtor.prototype = superCtor.prototype
    ctor.prototype = new TempCtor()
    ctor.prototype.constructor = ctor
  }
}

},{}],13:[function(require,module,exports){
module.exports = function isBuffer(arg) {
  return arg && typeof arg === 'object'
    && typeof arg.copy === 'function'
    && typeof arg.fill === 'function'
    && typeof arg.readUInt8 === 'function';
}
},{}],14:[function(require,module,exports){
var process=require("__browserify_process"),global=typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {};// Copyright Joyent, Inc. and other Node contributors.
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

var formatRegExp = /%[sdj%]/g;
exports.format = function(f) {
  if (!isString(f)) {
    var objects = [];
    for (var i = 0; i < arguments.length; i++) {
      objects.push(inspect(arguments[i]));
    }
    return objects.join(' ');
  }

  var i = 1;
  var args = arguments;
  var len = args.length;
  var str = String(f).replace(formatRegExp, function(x) {
    if (x === '%%') return '%';
    if (i >= len) return x;
    switch (x) {
      case '%s': return String(args[i++]);
      case '%d': return Number(args[i++]);
      case '%j':
        try {
          return JSON.stringify(args[i++]);
        } catch (_) {
          return '[Circular]';
        }
      default:
        return x;
    }
  });
  for (var x = args[i]; i < len; x = args[++i]) {
    if (isNull(x) || !isObject(x)) {
      str += ' ' + x;
    } else {
      str += ' ' + inspect(x);
    }
  }
  return str;
};


// Mark that a method should not be used.
// Returns a modified function which warns once by default.
// If --no-deprecation is set, then it is a no-op.
exports.deprecate = function(fn, msg) {
  // Allow for deprecating things in the process of starting up.
  if (isUndefined(global.process)) {
    return function() {
      return exports.deprecate(fn, msg).apply(this, arguments);
    };
  }

  if (process.noDeprecation === true) {
    return fn;
  }

  var warned = false;
  function deprecated() {
    if (!warned) {
      if (process.throwDeprecation) {
        throw new Error(msg);
      } else if (process.traceDeprecation) {
        console.trace(msg);
      } else {
        console.error(msg);
      }
      warned = true;
    }
    return fn.apply(this, arguments);
  }

  return deprecated;
};


var debugs = {};
var debugEnviron;
exports.debuglog = function(set) {
  if (isUndefined(debugEnviron))
    debugEnviron = process.env.NODE_DEBUG || '';
  set = set.toUpperCase();
  if (!debugs[set]) {
    if (new RegExp('\\b' + set + '\\b', 'i').test(debugEnviron)) {
      var pid = process.pid;
      debugs[set] = function() {
        var msg = exports.format.apply(exports, arguments);
        console.error('%s %d: %s', set, pid, msg);
      };
    } else {
      debugs[set] = function() {};
    }
  }
  return debugs[set];
};


/**
 * Echos the value of a value. Trys to print the value out
 * in the best way possible given the different types.
 *
 * @param {Object} obj The object to print out.
 * @param {Object} opts Optional options object that alters the output.
 */
/* legacy: obj, showHidden, depth, colors*/
function inspect(obj, opts) {
  // default options
  var ctx = {
    seen: [],
    stylize: stylizeNoColor
  };
  // legacy...
  if (arguments.length >= 3) ctx.depth = arguments[2];
  if (arguments.length >= 4) ctx.colors = arguments[3];
  if (isBoolean(opts)) {
    // legacy...
    ctx.showHidden = opts;
  } else if (opts) {
    // got an "options" object
    exports._extend(ctx, opts);
  }
  // set default options
  if (isUndefined(ctx.showHidden)) ctx.showHidden = false;
  if (isUndefined(ctx.depth)) ctx.depth = 2;
  if (isUndefined(ctx.colors)) ctx.colors = false;
  if (isUndefined(ctx.customInspect)) ctx.customInspect = true;
  if (ctx.colors) ctx.stylize = stylizeWithColor;
  return formatValue(ctx, obj, ctx.depth);
}
exports.inspect = inspect;


// http://en.wikipedia.org/wiki/ANSI_escape_code#graphics
inspect.colors = {
  'bold' : [1, 22],
  'italic' : [3, 23],
  'underline' : [4, 24],
  'inverse' : [7, 27],
  'white' : [37, 39],
  'grey' : [90, 39],
  'black' : [30, 39],
  'blue' : [34, 39],
  'cyan' : [36, 39],
  'green' : [32, 39],
  'magenta' : [35, 39],
  'red' : [31, 39],
  'yellow' : [33, 39]
};

// Don't use 'blue' not visible on cmd.exe
inspect.styles = {
  'special': 'cyan',
  'number': 'yellow',
  'boolean': 'yellow',
  'undefined': 'grey',
  'null': 'bold',
  'string': 'green',
  'date': 'magenta',
  // "name": intentionally not styling
  'regexp': 'red'
};


function stylizeWithColor(str, styleType) {
  var style = inspect.styles[styleType];

  if (style) {
    return '\u001b[' + inspect.colors[style][0] + 'm' + str +
           '\u001b[' + inspect.colors[style][1] + 'm';
  } else {
    return str;
  }
}


function stylizeNoColor(str, styleType) {
  return str;
}


function arrayToHash(array) {
  var hash = {};

  array.forEach(function(val, idx) {
    hash[val] = true;
  });

  return hash;
}


function formatValue(ctx, value, recurseTimes) {
  // Provide a hook for user-specified inspect functions.
  // Check that value is an object with an inspect function on it
  if (ctx.customInspect &&
      value &&
      isFunction(value.inspect) &&
      // Filter out the util module, it's inspect function is special
      value.inspect !== exports.inspect &&
      // Also filter out any prototype objects using the circular check.
      !(value.constructor && value.constructor.prototype === value)) {
    var ret = value.inspect(recurseTimes, ctx);
    if (!isString(ret)) {
      ret = formatValue(ctx, ret, recurseTimes);
    }
    return ret;
  }

  // Primitive types cannot have properties
  var primitive = formatPrimitive(ctx, value);
  if (primitive) {
    return primitive;
  }

  // Look up the keys of the object.
  var keys = Object.keys(value);
  var visibleKeys = arrayToHash(keys);

  if (ctx.showHidden) {
    keys = Object.getOwnPropertyNames(value);
  }

  // IE doesn't make error fields non-enumerable
  // http://msdn.microsoft.com/en-us/library/ie/dww52sbt(v=vs.94).aspx
  if (isError(value)
      && (keys.indexOf('message') >= 0 || keys.indexOf('description') >= 0)) {
    return formatError(value);
  }

  // Some type of object without properties can be shortcutted.
  if (keys.length === 0) {
    if (isFunction(value)) {
      var name = value.name ? ': ' + value.name : '';
      return ctx.stylize('[Function' + name + ']', 'special');
    }
    if (isRegExp(value)) {
      return ctx.stylize(RegExp.prototype.toString.call(value), 'regexp');
    }
    if (isDate(value)) {
      return ctx.stylize(Date.prototype.toString.call(value), 'date');
    }
    if (isError(value)) {
      return formatError(value);
    }
  }

  var base = '', array = false, braces = ['{', '}'];

  // Make Array say that they are Array
  if (isArray(value)) {
    array = true;
    braces = ['[', ']'];
  }

  // Make functions say that they are functions
  if (isFunction(value)) {
    var n = value.name ? ': ' + value.name : '';
    base = ' [Function' + n + ']';
  }

  // Make RegExps say that they are RegExps
  if (isRegExp(value)) {
    base = ' ' + RegExp.prototype.toString.call(value);
  }

  // Make dates with properties first say the date
  if (isDate(value)) {
    base = ' ' + Date.prototype.toUTCString.call(value);
  }

  // Make error with message first say the error
  if (isError(value)) {
    base = ' ' + formatError(value);
  }

  if (keys.length === 0 && (!array || value.length == 0)) {
    return braces[0] + base + braces[1];
  }

  if (recurseTimes < 0) {
    if (isRegExp(value)) {
      return ctx.stylize(RegExp.prototype.toString.call(value), 'regexp');
    } else {
      return ctx.stylize('[Object]', 'special');
    }
  }

  ctx.seen.push(value);

  var output;
  if (array) {
    output = formatArray(ctx, value, recurseTimes, visibleKeys, keys);
  } else {
    output = keys.map(function(key) {
      return formatProperty(ctx, value, recurseTimes, visibleKeys, key, array);
    });
  }

  ctx.seen.pop();

  return reduceToSingleString(output, base, braces);
}


function formatPrimitive(ctx, value) {
  if (isUndefined(value))
    return ctx.stylize('undefined', 'undefined');
  if (isString(value)) {
    var simple = '\'' + JSON.stringify(value).replace(/^"|"$/g, '')
                                             .replace(/'/g, "\\'")
                                             .replace(/\\"/g, '"') + '\'';
    return ctx.stylize(simple, 'string');
  }
  if (isNumber(value))
    return ctx.stylize('' + value, 'number');
  if (isBoolean(value))
    return ctx.stylize('' + value, 'boolean');
  // For some reason typeof null is "object", so special case here.
  if (isNull(value))
    return ctx.stylize('null', 'null');
}


function formatError(value) {
  return '[' + Error.prototype.toString.call(value) + ']';
}


function formatArray(ctx, value, recurseTimes, visibleKeys, keys) {
  var output = [];
  for (var i = 0, l = value.length; i < l; ++i) {
    if (hasOwnProperty(value, String(i))) {
      output.push(formatProperty(ctx, value, recurseTimes, visibleKeys,
          String(i), true));
    } else {
      output.push('');
    }
  }
  keys.forEach(function(key) {
    if (!key.match(/^\d+$/)) {
      output.push(formatProperty(ctx, value, recurseTimes, visibleKeys,
          key, true));
    }
  });
  return output;
}


function formatProperty(ctx, value, recurseTimes, visibleKeys, key, array) {
  var name, str, desc;
  desc = Object.getOwnPropertyDescriptor(value, key) || { value: value[key] };
  if (desc.get) {
    if (desc.set) {
      str = ctx.stylize('[Getter/Setter]', 'special');
    } else {
      str = ctx.stylize('[Getter]', 'special');
    }
  } else {
    if (desc.set) {
      str = ctx.stylize('[Setter]', 'special');
    }
  }
  if (!hasOwnProperty(visibleKeys, key)) {
    name = '[' + key + ']';
  }
  if (!str) {
    if (ctx.seen.indexOf(desc.value) < 0) {
      if (isNull(recurseTimes)) {
        str = formatValue(ctx, desc.value, null);
      } else {
        str = formatValue(ctx, desc.value, recurseTimes - 1);
      }
      if (str.indexOf('\n') > -1) {
        if (array) {
          str = str.split('\n').map(function(line) {
            return '  ' + line;
          }).join('\n').substr(2);
        } else {
          str = '\n' + str.split('\n').map(function(line) {
            return '   ' + line;
          }).join('\n');
        }
      }
    } else {
      str = ctx.stylize('[Circular]', 'special');
    }
  }
  if (isUndefined(name)) {
    if (array && key.match(/^\d+$/)) {
      return str;
    }
    name = JSON.stringify('' + key);
    if (name.match(/^"([a-zA-Z_][a-zA-Z_0-9]*)"$/)) {
      name = name.substr(1, name.length - 2);
      name = ctx.stylize(name, 'name');
    } else {
      name = name.replace(/'/g, "\\'")
                 .replace(/\\"/g, '"')
                 .replace(/(^"|"$)/g, "'");
      name = ctx.stylize(name, 'string');
    }
  }

  return name + ': ' + str;
}


function reduceToSingleString(output, base, braces) {
  var numLinesEst = 0;
  var length = output.reduce(function(prev, cur) {
    numLinesEst++;
    if (cur.indexOf('\n') >= 0) numLinesEst++;
    return prev + cur.replace(/\u001b\[\d\d?m/g, '').length + 1;
  }, 0);

  if (length > 60) {
    return braces[0] +
           (base === '' ? '' : base + '\n ') +
           ' ' +
           output.join(',\n  ') +
           ' ' +
           braces[1];
  }

  return braces[0] + base + ' ' + output.join(', ') + ' ' + braces[1];
}


// NOTE: These type checking functions intentionally don't use `instanceof`
// because it is fragile and can be easily faked with `Object.create()`.
function isArray(ar) {
  return Array.isArray(ar);
}
exports.isArray = isArray;

function isBoolean(arg) {
  return typeof arg === 'boolean';
}
exports.isBoolean = isBoolean;

function isNull(arg) {
  return arg === null;
}
exports.isNull = isNull;

function isNullOrUndefined(arg) {
  return arg == null;
}
exports.isNullOrUndefined = isNullOrUndefined;

function isNumber(arg) {
  return typeof arg === 'number';
}
exports.isNumber = isNumber;

function isString(arg) {
  return typeof arg === 'string';
}
exports.isString = isString;

function isSymbol(arg) {
  return typeof arg === 'symbol';
}
exports.isSymbol = isSymbol;

function isUndefined(arg) {
  return arg === void 0;
}
exports.isUndefined = isUndefined;

function isRegExp(re) {
  return isObject(re) && objectToString(re) === '[object RegExp]';
}
exports.isRegExp = isRegExp;

function isObject(arg) {
  return typeof arg === 'object' && arg !== null;
}
exports.isObject = isObject;

function isDate(d) {
  return isObject(d) && objectToString(d) === '[object Date]';
}
exports.isDate = isDate;

function isError(e) {
  return isObject(e) &&
      (objectToString(e) === '[object Error]' || e instanceof Error);
}
exports.isError = isError;

function isFunction(arg) {
  return typeof arg === 'function';
}
exports.isFunction = isFunction;

function isPrimitive(arg) {
  return arg === null ||
         typeof arg === 'boolean' ||
         typeof arg === 'number' ||
         typeof arg === 'string' ||
         typeof arg === 'symbol' ||  // ES6 symbol
         typeof arg === 'undefined';
}
exports.isPrimitive = isPrimitive;

exports.isBuffer = require('./support/isBuffer');

function objectToString(o) {
  return Object.prototype.toString.call(o);
}


function pad(n) {
  return n < 10 ? '0' + n.toString(10) : n.toString(10);
}


var months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep',
              'Oct', 'Nov', 'Dec'];

// 26 Feb 16:19:34
function timestamp() {
  var d = new Date();
  var time = [pad(d.getHours()),
              pad(d.getMinutes()),
              pad(d.getSeconds())].join(':');
  return [d.getDate(), months[d.getMonth()], time].join(' ');
}


// log is just a thin wrapper to console.log that prepends a timestamp
exports.log = function() {
  console.log('%s - %s', timestamp(), exports.format.apply(exports, arguments));
};


/**
 * Inherit the prototype methods from one constructor into another.
 *
 * The Function.prototype.inherits from lang.js rewritten as a standalone
 * function (not on Function.prototype). NOTE: If this file is to be loaded
 * during bootstrapping this function needs to be rewritten using some native
 * functions as prototype setup using normal JavaScript does not work as
 * expected during bootstrapping (see mirror.js in r114903).
 *
 * @param {function} ctor Constructor function which needs to inherit the
 *     prototype.
 * @param {function} superCtor Constructor function to inherit prototype from.
 */
exports.inherits = require('inherits');

exports._extend = function(origin, add) {
  // Don't do anything if add isn't an object
  if (!add || !isObject(add)) return origin;

  var keys = Object.keys(add);
  var i = keys.length;
  while (i--) {
    origin[keys[i]] = add[keys[i]];
  }
  return origin;
};

function hasOwnProperty(obj, prop) {
  return Object.prototype.hasOwnProperty.call(obj, prop);
}

},{"./support/isBuffer":13,"__browserify_process":2,"inherits":12}]},{},[10])
//@ sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiZ2VuZXJhdGVkLmpzIiwic291cmNlcyI6WyIvVXNlcnMvZmVyb3NzL2NvZGUvaW5zZXJ0LW1vZHVsZS1nbG9iYWxzL2J1ZmZlci5qcyIsIi9Vc2Vycy9mZXJvc3MvY29kZS9pbnNlcnQtbW9kdWxlLWdsb2JhbHMvbm9kZV9tb2R1bGVzL3Byb2Nlc3MvYnJvd3Nlci5qcyIsIi9Vc2Vycy9mZXJvc3MvY29kZS9uYXRpdmUtYnVmZmVyLWJyb3dzZXJpZnkvaW5kZXguanMiLCIvVXNlcnMvZmVyb3NzL2NvZGUvbmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5L25vZGVfbW9kdWxlcy9iYXNlNjQtanMvbGliL2I2NC5qcyIsIi9Vc2Vycy9mZXJvc3MvY29kZS9uYXRpdmUtYnVmZmVyLWJyb3dzZXJpZnkvbm9kZV9tb2R1bGVzL2JlbmNobWFyay9iZW5jaG1hcmsuanMiLCIvVXNlcnMvZmVyb3NzL2NvZGUvbmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5L25vZGVfbW9kdWxlcy9idWZmZXItYnJvd3NlcmlmeS9idWZmZXJfaWVlZTc1NC5qcyIsIi9Vc2Vycy9mZXJvc3MvY29kZS9uYXRpdmUtYnVmZmVyLWJyb3dzZXJpZnkvbm9kZV9tb2R1bGVzL2J1ZmZlci1icm93c2VyaWZ5L2luZGV4LmpzIiwiL1VzZXJzL2Zlcm9zcy9jb2RlL25hdGl2ZS1idWZmZXItYnJvd3NlcmlmeS9ub2RlX21vZHVsZXMvYnVmZmVyLWJyb3dzZXJpZnkvbm9kZV9tb2R1bGVzL2Jhc2U2NC1qcy9saWIvYjY0LmpzIiwiL1VzZXJzL2Zlcm9zcy9jb2RlL25hdGl2ZS1idWZmZXItYnJvd3NlcmlmeS9ub2RlX21vZHVsZXMvdHlwZWRhcnJheS9pbmRleC5qcyIsIi9Vc2Vycy9mZXJvc3MvY29kZS9uYXRpdmUtYnVmZmVyLWJyb3dzZXJpZnkvcGVyZi9jb21wYXJpc29uL2JyYWNrZXQtbm90YXRpb24uanMiLCIvVXNlcnMvZmVyb3NzL2NvZGUvbm9kZS1icm93c2VyaWZ5L25vZGVfbW9kdWxlcy9hc3NlcnQvYXNzZXJ0LmpzIiwiL1VzZXJzL2Zlcm9zcy9jb2RlL25vZGUtYnJvd3NlcmlmeS9ub2RlX21vZHVsZXMvaW5oZXJpdHMvaW5oZXJpdHNfYnJvd3Nlci5qcyIsIi9Vc2Vycy9mZXJvc3MvY29kZS9ub2RlLWJyb3dzZXJpZnkvbm9kZV9tb2R1bGVzL3V0aWwvc3VwcG9ydC9pc0J1ZmZlckJyb3dzZXIuanMiLCIvVXNlcnMvZmVyb3NzL2NvZGUvbm9kZS1icm93c2VyaWZ5L25vZGVfbW9kdWxlcy91dGlsL3V0aWwuanMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IjtBQUFBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ3p2REE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNwREE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNqb0NBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDdkdBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQzkwSEE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDcEZBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNwbUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ3BGQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUN0bkJBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDckNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTs7QUNyVkE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBOztBQ3ZCQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7O0FDTEE7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQTtBQUNBO0FBQ0E7QUFDQSIsInNvdXJjZXNDb250ZW50IjpbInJlcXVpcmU9KGZ1bmN0aW9uIGUodCxuLHIpe2Z1bmN0aW9uIHMobyx1KXtpZighbltvXSl7aWYoIXRbb10pe3ZhciBhPXR5cGVvZiByZXF1aXJlPT1cImZ1bmN0aW9uXCImJnJlcXVpcmU7aWYoIXUmJmEpcmV0dXJuIGEobywhMCk7aWYoaSlyZXR1cm4gaShvLCEwKTt0aHJvdyBuZXcgRXJyb3IoXCJDYW5ub3QgZmluZCBtb2R1bGUgJ1wiK28rXCInXCIpfXZhciBmPW5bb109e2V4cG9ydHM6e319O3Rbb11bMF0uY2FsbChmLmV4cG9ydHMsZnVuY3Rpb24oZSl7dmFyIG49dFtvXVsxXVtlXTtyZXR1cm4gcyhuP246ZSl9LGYsZi5leHBvcnRzLGUsdCxuLHIpfXJldHVybiBuW29dLmV4cG9ydHN9dmFyIGk9dHlwZW9mIHJlcXVpcmU9PVwiZnVuY3Rpb25cIiYmcmVxdWlyZTtmb3IodmFyIG89MDtvPHIubGVuZ3RoO28rKylzKHJbb10pO3JldHVybiBzfSkoe1wiMjZzRVY3XCI6W2Z1bmN0aW9uKHJlcXVpcmUsbW9kdWxlLGV4cG9ydHMpe1xudmFyIGJhc2U2NCA9IHJlcXVpcmUoJ2Jhc2U2NC1qcycpXG52YXIgVEEgPSByZXF1aXJlKCd0eXBlZGFycmF5JylcblxuZXhwb3J0cy5CdWZmZXIgPSBCdWZmZXJcbmV4cG9ydHMuU2xvd0J1ZmZlciA9IEJ1ZmZlclxuZXhwb3J0cy5JTlNQRUNUX01BWF9CWVRFUyA9IDUwXG5CdWZmZXIucG9vbFNpemUgPSA4MTkyXG5cbi8qKlxuICogVXNlIGEgc2hpbSBmb3IgYnJvd3NlcnMgdGhhdCBsYWNrIFR5cGVkIEFycmF5IHN1cHBvcnQgKDwgSUUgOSwgPCBGRiAzLjYsXG4gKiA8IENocm9tZSA2LCA8IFNhZmFyaSA1LCA8IE9wZXJhIDExLjUsIDwgaU9TIDQuMSkuXG4gKi9cbnZhciB4RGF0YVZpZXcgPSB0eXBlb2YgRGF0YVZpZXcgPT09ICd1bmRlZmluZWQnXG4gID8gVEEuRGF0YVZpZXcgOiBEYXRhVmlld1xudmFyIHhBcnJheUJ1ZmZlciA9IHR5cGVvZiBBcnJheUJ1ZmZlciA9PT0gJ3VuZGVmaW5lZCdcbiAgPyBUQS5BcnJheUJ1ZmZlciA6IEFycmF5QnVmZmVyXG52YXIgeFVpbnQ4QXJyYXkgPSB0eXBlb2YgVWludDhBcnJheSA9PT0gJ3VuZGVmaW5lZCdcbiAgPyBUQS5VaW50OEFycmF5IDogVWludDhBcnJheVxuXG4vKipcbiAqIERvZXMgdGhlIGJyb3dzZXIgc3VwcG9ydCBhZGRpbmcgcHJvcGVydGllcyB0byBgVWludDhBcnJheWAgaW5zdGFuY2VzPyBJZlxuICogbm90LCB0aGVuIHVzZSB0aGUgc2hpbSBldmVuIGlmIHRoZSBicm93c2VyIHN1cHBvcnRzIHR5cGVkIGFycmF5czsgd2UgbmVlZFxuICogdG8gYmUgYWJsZSB0byBhZGQgYWxsIHRoZSBub2RlIEJ1ZmZlciBBUEkgbWV0aG9kcy4gV2UgY291bGQgdXNlIGFuIEVTNlxuICogUHJveHkgdG8gaW50ZXJjZXB0IG1ldGhvZCBjYWxscyBhbmQgc2ltdWxhdGUgYWRkaW5nIGZ1bmN0aW9uIHByb3BlcnRpZXMgaW5cbiAqIEZpcmVmb3gsIGJ1dCB0aGF0IGFkZHMgYSBodWdlIHBlcmZvcm1hbmNlIHBlbmFsdHkuXG4gKlxuICogRmlyZWZveCBidWcgcmVwb3J0OiBodHRwczovL2J1Z3ppbGxhLm1vemlsbGEub3JnL3Nob3dfYnVnLmNnaT9pZD02OTU0MzhcbiAqL1xudmFyIHN1cHBvcnRzQXVnbWVudCA9IChmdW5jdGlvbiAoKSB7XG4gIHRyeSB7XG4gICAgdmFyIGFyciA9IG5ldyB4VWludDhBcnJheSgwKVxuICAgIGFyci5mb28gPSBmdW5jdGlvbiAoKSB7IHJldHVybiA0MiB9XG4gICAgcmV0dXJuIDQyID09PSBhcnIuZm9vKClcbiAgfSBjYXRjaCAoZSkge1xuICAgIHJldHVybiBmYWxzZVxuICB9XG59KSgpXG5pZiAoIXN1cHBvcnRzQXVnbWVudCkge1xuICB4RGF0YVZpZXcgPSBUQS5EYXRhVmlld1xuICB4QXJyYXlCdWZmZXIgPSBUQS5BcnJheUJ1ZmZlclxuICB4VWludDhBcnJheSA9IFRBLlVpbnQ4QXJyYXlcbn1cblxuLyoqXG4gKiBDbGFzczogQnVmZmVyXG4gKiA9PT09PT09PT09PT09XG4gKlxuICogVGhlIEJ1ZmZlciBjb25zdHJ1Y3RvciByZXR1cm5zIGluc3RhbmNlcyBvZiBgVWludDhBcnJheWAgdGhhdCBhcmUgYXVnbWVudGVkXG4gKiB3aXRoIGZ1bmN0aW9uIHByb3BlcnRpZXMgZm9yIGFsbCB0aGUgbm9kZSBgQnVmZmVyYCBBUEkgZnVuY3Rpb25zLiBXZSB1c2VcbiAqIGBVaW50OEFycmF5YCBzbyB0aGF0IHNxdWFyZSBicmFja2V0IG5vdGF0aW9uIHdvcmtzIGFzIGV4cGVjdGVkIC0tIGl0IHJldHVybnNcbiAqIGEgc2luZ2xlIG9jdGV0LlxuICpcbiAqIEJ5IGF1Z21lbnRpbmcgdGhlIGluc3RhbmNlcywgd2UgY2FuIGF2b2lkIG1vZGlmeWluZyB0aGUgYFVpbnQ4QXJyYXlgXG4gKiBwcm90b3R5cGUuXG4gKi9cbmZ1bmN0aW9uIEJ1ZmZlciAoc3ViamVjdCwgZW5jb2RpbmcpIHtcbiAgdmFyIHR5cGUgPSB0eXBlb2Ygc3ViamVjdFxuXG4gIC8vIFdvcmstYXJvdW5kOiBub2RlJ3MgYmFzZTY0IGltcGxlbWVudGF0aW9uXG4gIC8vIGFsbG93cyBmb3Igbm9uLXBhZGRlZCBzdHJpbmdzIHdoaWxlIGJhc2U2NC1qc1xuICAvLyBkb2VzIG5vdC4uXG4gIGlmIChlbmNvZGluZyA9PT0gJ2Jhc2U2NCcgJiYgdHlwZSA9PT0gJ3N0cmluZycpIHtcbiAgICBzdWJqZWN0ID0gc3RyaW5ndHJpbShzdWJqZWN0KVxuICAgIHdoaWxlIChzdWJqZWN0Lmxlbmd0aCAlIDQgIT09IDApIHtcbiAgICAgIHN1YmplY3QgPSBzdWJqZWN0ICsgJz0nXG4gICAgfVxuICB9XG5cbiAgLy8gRmluZCB0aGUgbGVuZ3RoXG4gIHZhciBsZW5ndGhcbiAgaWYgKHR5cGUgPT09ICdudW1iZXInKVxuICAgIGxlbmd0aCA9IGNvZXJjZShzdWJqZWN0KVxuICBlbHNlIGlmICh0eXBlID09PSAnc3RyaW5nJylcbiAgICBsZW5ndGggPSBCdWZmZXIuYnl0ZUxlbmd0aChzdWJqZWN0LCBlbmNvZGluZylcbiAgZWxzZSBpZiAodHlwZSA9PT0gJ29iamVjdCcpXG4gICAgbGVuZ3RoID0gY29lcmNlKHN1YmplY3QubGVuZ3RoKSAvLyBBc3N1bWUgb2JqZWN0IGlzIGFuIGFycmF5XG4gIGVsc2VcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ0ZpcnN0IGFyZ3VtZW50IG5lZWRzIHRvIGJlIGEgbnVtYmVyLCBhcnJheSBvciBzdHJpbmcuJylcblxuICB2YXIgYnVmID0gYXVnbWVudChuZXcgeFVpbnQ4QXJyYXkobGVuZ3RoKSlcbiAgaWYgKEJ1ZmZlci5pc0J1ZmZlcihzdWJqZWN0KSkge1xuICAgIC8vIFNwZWVkIG9wdGltaXphdGlvbiAtLSB1c2Ugc2V0IGlmIHdlJ3JlIGNvcHlpbmcgZnJvbSBhIFVpbnQ4QXJyYXlcbiAgICBidWYuc2V0KHN1YmplY3QpXG4gIH0gZWxzZSBpZiAoaXNBcnJheUlzaChzdWJqZWN0KSkge1xuICAgIC8vIFRyZWF0IGFycmF5LWlzaCBvYmplY3RzIGFzIGEgYnl0ZSBhcnJheS5cbiAgICBmb3IgKHZhciBpID0gMDsgaSA8IGxlbmd0aDsgaSsrKSB7XG4gICAgICBpZiAoQnVmZmVyLmlzQnVmZmVyKHN1YmplY3QpKVxuICAgICAgICBidWZbaV0gPSBzdWJqZWN0LnJlYWRVSW50OChpKVxuICAgICAgZWxzZVxuICAgICAgICBidWZbaV0gPSBzdWJqZWN0W2ldXG4gICAgfVxuICB9IGVsc2UgaWYgKHR5cGUgPT09ICdzdHJpbmcnKSB7XG4gICAgYnVmLndyaXRlKHN1YmplY3QsIDAsIGVuY29kaW5nKVxuICB9XG5cbiAgcmV0dXJuIGJ1ZlxufVxuXG4vLyBTVEFUSUMgTUVUSE9EU1xuLy8gPT09PT09PT09PT09PT1cblxuQnVmZmVyLmlzRW5jb2RpbmcgPSBmdW5jdGlvbihlbmNvZGluZykge1xuICBzd2l0Y2ggKChlbmNvZGluZyArICcnKS50b0xvd2VyQ2FzZSgpKSB7XG4gICAgY2FzZSAnaGV4JzpcbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgY2FzZSAnYXNjaWknOlxuICAgIGNhc2UgJ2JpbmFyeSc6XG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICBjYXNlICd1Y3MyJzpcbiAgICBjYXNlICd1Y3MtMic6XG4gICAgY2FzZSAndXRmMTZsZSc6XG4gICAgY2FzZSAndXRmLTE2bGUnOlxuICAgIGNhc2UgJ3Jhdyc6XG4gICAgICByZXR1cm4gdHJ1ZVxuXG4gICAgZGVmYXVsdDpcbiAgICAgIHJldHVybiBmYWxzZVxuICB9XG59XG5cbkJ1ZmZlci5pc0J1ZmZlciA9IGZ1bmN0aW9uIChiKSB7XG4gIHJldHVybiBiICYmIGIuX2lzQnVmZmVyXG59XG5cbkJ1ZmZlci5ieXRlTGVuZ3RoID0gZnVuY3Rpb24gKHN0ciwgZW5jb2RpbmcpIHtcbiAgc3dpdGNoIChlbmNvZGluZyB8fCAndXRmOCcpIHtcbiAgICBjYXNlICdoZXgnOlxuICAgICAgcmV0dXJuIHN0ci5sZW5ndGggLyAyXG5cbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgICByZXR1cm4gdXRmOFRvQnl0ZXMoc3RyKS5sZW5ndGhcblxuICAgIGNhc2UgJ2FzY2lpJzpcbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIHN0ci5sZW5ndGhcblxuICAgIGNhc2UgJ2Jhc2U2NCc6XG4gICAgICByZXR1cm4gYmFzZTY0VG9CeXRlcyhzdHIpLmxlbmd0aFxuXG4gICAgZGVmYXVsdDpcbiAgICAgIHRocm93IG5ldyBFcnJvcignVW5rbm93biBlbmNvZGluZycpXG4gIH1cbn1cblxuQnVmZmVyLmNvbmNhdCA9IGZ1bmN0aW9uIChsaXN0LCB0b3RhbExlbmd0aCkge1xuICBpZiAoIWlzQXJyYXkobGlzdCkpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ1VzYWdlOiBCdWZmZXIuY29uY2F0KGxpc3QsIFt0b3RhbExlbmd0aF0pXFxuJyArXG4gICAgICAgICdsaXN0IHNob3VsZCBiZSBhbiBBcnJheS4nKVxuICB9XG5cbiAgdmFyIGlcbiAgdmFyIGJ1ZlxuXG4gIGlmIChsaXN0Lmxlbmd0aCA9PT0gMCkge1xuICAgIHJldHVybiBuZXcgQnVmZmVyKDApXG4gIH0gZWxzZSBpZiAobGlzdC5sZW5ndGggPT09IDEpIHtcbiAgICByZXR1cm4gbGlzdFswXVxuICB9XG5cbiAgaWYgKHR5cGVvZiB0b3RhbExlbmd0aCAhPT0gJ251bWJlcicpIHtcbiAgICB0b3RhbExlbmd0aCA9IDBcbiAgICBmb3IgKGkgPSAwOyBpIDwgbGlzdC5sZW5ndGg7IGkrKykge1xuICAgICAgYnVmID0gbGlzdFtpXVxuICAgICAgdG90YWxMZW5ndGggKz0gYnVmLmxlbmd0aFxuICAgIH1cbiAgfVxuXG4gIHZhciBidWZmZXIgPSBuZXcgQnVmZmVyKHRvdGFsTGVuZ3RoKVxuICB2YXIgcG9zID0gMFxuICBmb3IgKGkgPSAwOyBpIDwgbGlzdC5sZW5ndGg7IGkrKykge1xuICAgIGJ1ZiA9IGxpc3RbaV1cbiAgICBidWYuY29weShidWZmZXIsIHBvcylcbiAgICBwb3MgKz0gYnVmLmxlbmd0aFxuICB9XG4gIHJldHVybiBidWZmZXJcbn1cblxuLy8gQlVGRkVSIElOU1RBTkNFIE1FVEhPRFNcbi8vID09PT09PT09PT09PT09PT09PT09PT09XG5cbmZ1bmN0aW9uIF9oZXhXcml0ZSAoYnVmLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIG9mZnNldCA9IE51bWJlcihvZmZzZXQpIHx8IDBcbiAgdmFyIHJlbWFpbmluZyA9IGJ1Zi5sZW5ndGggLSBvZmZzZXRcbiAgaWYgKCFsZW5ndGgpIHtcbiAgICBsZW5ndGggPSByZW1haW5pbmdcbiAgfSBlbHNlIHtcbiAgICBsZW5ndGggPSBOdW1iZXIobGVuZ3RoKVxuICAgIGlmIChsZW5ndGggPiByZW1haW5pbmcpIHtcbiAgICAgIGxlbmd0aCA9IHJlbWFpbmluZ1xuICAgIH1cbiAgfVxuXG4gIC8vIG11c3QgYmUgYW4gZXZlbiBudW1iZXIgb2YgZGlnaXRzXG4gIHZhciBzdHJMZW4gPSBzdHJpbmcubGVuZ3RoXG4gIGlmIChzdHJMZW4gJSAyICE9PSAwKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdJbnZhbGlkIGhleCBzdHJpbmcnKVxuICB9XG4gIGlmIChsZW5ndGggPiBzdHJMZW4gLyAyKSB7XG4gICAgbGVuZ3RoID0gc3RyTGVuIC8gMlxuICB9XG4gIGZvciAodmFyIGkgPSAwOyBpIDwgbGVuZ3RoOyBpKyspIHtcbiAgICB2YXIgYnl0ZSA9IHBhcnNlSW50KHN0cmluZy5zdWJzdHIoaSAqIDIsIDIpLCAxNilcbiAgICBpZiAoaXNOYU4oYnl0ZSkpIHRocm93IG5ldyBFcnJvcignSW52YWxpZCBoZXggc3RyaW5nJylcbiAgICBidWZbb2Zmc2V0ICsgaV0gPSBieXRlXG4gIH1cbiAgQnVmZmVyLl9jaGFyc1dyaXR0ZW4gPSBpICogMlxuICByZXR1cm4gaVxufVxuXG5mdW5jdGlvbiBfdXRmOFdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIGJ5dGVzLCBwb3NcbiAgcmV0dXJuIEJ1ZmZlci5fY2hhcnNXcml0dGVuID0gYmxpdEJ1ZmZlcih1dGY4VG9CeXRlcyhzdHJpbmcpLCBidWYsIG9mZnNldCwgbGVuZ3RoKVxufVxuXG5mdW5jdGlvbiBfYXNjaWlXcml0ZSAoYnVmLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIHZhciBieXRlcywgcG9zXG4gIHJldHVybiBCdWZmZXIuX2NoYXJzV3JpdHRlbiA9IGJsaXRCdWZmZXIoYXNjaWlUb0J5dGVzKHN0cmluZyksIGJ1Ziwgb2Zmc2V0LCBsZW5ndGgpXG59XG5cbmZ1bmN0aW9uIF9iaW5hcnlXcml0ZSAoYnVmLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIHJldHVybiBfYXNjaWlXcml0ZShidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG59XG5cbmZ1bmN0aW9uIF9iYXNlNjRXcml0ZSAoYnVmLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIHZhciBieXRlcywgcG9zXG4gIHJldHVybiBCdWZmZXIuX2NoYXJzV3JpdHRlbiA9IGJsaXRCdWZmZXIoYmFzZTY0VG9CeXRlcyhzdHJpbmcpLCBidWYsIG9mZnNldCwgbGVuZ3RoKVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZSAoc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCwgZW5jb2RpbmcpIHtcbiAgLy8gU3VwcG9ydCBib3RoIChzdHJpbmcsIG9mZnNldCwgbGVuZ3RoLCBlbmNvZGluZylcbiAgLy8gYW5kIHRoZSBsZWdhY3kgKHN0cmluZywgZW5jb2RpbmcsIG9mZnNldCwgbGVuZ3RoKVxuICBpZiAoaXNGaW5pdGUob2Zmc2V0KSkge1xuICAgIGlmICghaXNGaW5pdGUobGVuZ3RoKSkge1xuICAgICAgZW5jb2RpbmcgPSBsZW5ndGhcbiAgICAgIGxlbmd0aCA9IHVuZGVmaW5lZFxuICAgIH1cbiAgfSBlbHNlIHsgIC8vIGxlZ2FjeVxuICAgIHZhciBzd2FwID0gZW5jb2RpbmdcbiAgICBlbmNvZGluZyA9IG9mZnNldFxuICAgIG9mZnNldCA9IGxlbmd0aFxuICAgIGxlbmd0aCA9IHN3YXBcbiAgfVxuXG4gIG9mZnNldCA9IE51bWJlcihvZmZzZXQpIHx8IDBcbiAgdmFyIHJlbWFpbmluZyA9IHRoaXMubGVuZ3RoIC0gb2Zmc2V0XG4gIGlmICghbGVuZ3RoKSB7XG4gICAgbGVuZ3RoID0gcmVtYWluaW5nXG4gIH0gZWxzZSB7XG4gICAgbGVuZ3RoID0gTnVtYmVyKGxlbmd0aClcbiAgICBpZiAobGVuZ3RoID4gcmVtYWluaW5nKSB7XG4gICAgICBsZW5ndGggPSByZW1haW5pbmdcbiAgICB9XG4gIH1cbiAgZW5jb2RpbmcgPSBTdHJpbmcoZW5jb2RpbmcgfHwgJ3V0ZjgnKS50b0xvd2VyQ2FzZSgpXG5cbiAgc3dpdGNoIChlbmNvZGluZykge1xuICAgIGNhc2UgJ2hleCc6XG4gICAgICByZXR1cm4gX2hleFdyaXRlKHRoaXMsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG5cbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgICByZXR1cm4gX3V0ZjhXcml0ZSh0aGlzLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKVxuXG4gICAgY2FzZSAnYXNjaWknOlxuICAgICAgcmV0dXJuIF9hc2NpaVdyaXRlKHRoaXMsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG5cbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIF9iaW5hcnlXcml0ZSh0aGlzLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKVxuXG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICAgIHJldHVybiBfYmFzZTY0V3JpdGUodGhpcywgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aClcblxuICAgIGRlZmF1bHQ6XG4gICAgICB0aHJvdyBuZXcgRXJyb3IoJ1Vua25vd24gZW5jb2RpbmcnKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclRvU3RyaW5nIChlbmNvZGluZywgc3RhcnQsIGVuZCkge1xuICB2YXIgc2VsZiA9IHRoaXNcblxuICBlbmNvZGluZyA9IFN0cmluZyhlbmNvZGluZyB8fCAndXRmOCcpLnRvTG93ZXJDYXNlKClcbiAgc3RhcnQgPSBOdW1iZXIoc3RhcnQpIHx8IDBcbiAgZW5kID0gKGVuZCAhPT0gdW5kZWZpbmVkKVxuICAgID8gTnVtYmVyKGVuZClcbiAgICA6IGVuZCA9IHNlbGYubGVuZ3RoXG5cbiAgLy8gRmFzdHBhdGggZW1wdHkgc3RyaW5nc1xuICBpZiAoZW5kID09PSBzdGFydClcbiAgICByZXR1cm4gJydcblxuICBzd2l0Y2ggKGVuY29kaW5nKSB7XG4gICAgY2FzZSAnaGV4JzpcbiAgICAgIHJldHVybiBfaGV4U2xpY2Uoc2VsZiwgc3RhcnQsIGVuZClcblxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICAgIHJldHVybiBfdXRmOFNsaWNlKHNlbGYsIHN0YXJ0LCBlbmQpXG5cbiAgICBjYXNlICdhc2NpaSc6XG4gICAgICByZXR1cm4gX2FzY2lpU2xpY2Uoc2VsZiwgc3RhcnQsIGVuZClcblxuICAgIGNhc2UgJ2JpbmFyeSc6XG4gICAgICByZXR1cm4gX2JpbmFyeVNsaWNlKHNlbGYsIHN0YXJ0LCBlbmQpXG5cbiAgICBjYXNlICdiYXNlNjQnOlxuICAgICAgcmV0dXJuIF9iYXNlNjRTbGljZShzZWxmLCBzdGFydCwgZW5kKVxuXG4gICAgZGVmYXVsdDpcbiAgICAgIHRocm93IG5ldyBFcnJvcignVW5rbm93biBlbmNvZGluZycpXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyVG9KU09OICgpIHtcbiAgcmV0dXJuIHtcbiAgICB0eXBlOiAnQnVmZmVyJyxcbiAgICBkYXRhOiBBcnJheS5wcm90b3R5cGUuc2xpY2UuY2FsbCh0aGlzLl9hcnIgfHwgdGhpcywgMClcbiAgfVxufVxuXG4vLyBjb3B5KHRhcmdldEJ1ZmZlciwgdGFyZ2V0U3RhcnQ9MCwgc291cmNlU3RhcnQ9MCwgc291cmNlRW5kPWJ1ZmZlci5sZW5ndGgpXG5mdW5jdGlvbiBCdWZmZXJDb3B5ICh0YXJnZXQsIHRhcmdldF9zdGFydCwgc3RhcnQsIGVuZCkge1xuICB2YXIgc291cmNlID0gdGhpc1xuXG4gIGlmICghc3RhcnQpIHN0YXJ0ID0gMFxuICBpZiAoIWVuZCAmJiBlbmQgIT09IDApIGVuZCA9IHRoaXMubGVuZ3RoXG4gIGlmICghdGFyZ2V0X3N0YXJ0KSB0YXJnZXRfc3RhcnQgPSAwXG5cbiAgLy8gQ29weSAwIGJ5dGVzOyB3ZSdyZSBkb25lXG4gIGlmIChlbmQgPT09IHN0YXJ0KSByZXR1cm5cbiAgaWYgKHRhcmdldC5sZW5ndGggPT09IDAgfHwgc291cmNlLmxlbmd0aCA9PT0gMCkgcmV0dXJuXG5cbiAgLy8gRmF0YWwgZXJyb3IgY29uZGl0aW9uc1xuICBpZiAoZW5kIDwgc3RhcnQpXG4gICAgdGhyb3cgbmV3IEVycm9yKCdzb3VyY2VFbmQgPCBzb3VyY2VTdGFydCcpXG4gIGlmICh0YXJnZXRfc3RhcnQgPCAwIHx8IHRhcmdldF9zdGFydCA+PSB0YXJnZXQubGVuZ3RoKVxuICAgIHRocm93IG5ldyBFcnJvcigndGFyZ2V0U3RhcnQgb3V0IG9mIGJvdW5kcycpXG4gIGlmIChzdGFydCA8IDAgfHwgc3RhcnQgPj0gc291cmNlLmxlbmd0aClcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3NvdXJjZVN0YXJ0IG91dCBvZiBib3VuZHMnKVxuICBpZiAoZW5kIDwgMCB8fCBlbmQgPiBzb3VyY2UubGVuZ3RoKVxuICAgIHRocm93IG5ldyBFcnJvcignc291cmNlRW5kIG91dCBvZiBib3VuZHMnKVxuXG4gIC8vIEFyZSB3ZSBvb2I/XG4gIGlmIChlbmQgPiB0aGlzLmxlbmd0aClcbiAgICBlbmQgPSB0aGlzLmxlbmd0aFxuICBpZiAodGFyZ2V0Lmxlbmd0aCAtIHRhcmdldF9zdGFydCA8IGVuZCAtIHN0YXJ0KVxuICAgIGVuZCA9IHRhcmdldC5sZW5ndGggLSB0YXJnZXRfc3RhcnQgKyBzdGFydFxuXG4gIC8vIGNvcHkhXG4gIGZvciAodmFyIGkgPSAwOyBpIDwgZW5kIC0gc3RhcnQ7IGkrKylcbiAgICB0YXJnZXRbaSArIHRhcmdldF9zdGFydF0gPSB0aGlzW2kgKyBzdGFydF1cbn1cblxuZnVuY3Rpb24gX2Jhc2U2NFNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgaWYgKHN0YXJ0ID09PSAwICYmIGVuZCA9PT0gYnVmLmxlbmd0aCkge1xuICAgIHJldHVybiBiYXNlNjQuZnJvbUJ5dGVBcnJheShidWYpXG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIGJhc2U2NC5mcm9tQnl0ZUFycmF5KGJ1Zi5zbGljZShzdGFydCwgZW5kKSlcbiAgfVxufVxuXG5mdW5jdGlvbiBfdXRmOFNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIHJlcyA9ICcnXG4gIHZhciB0bXAgPSAnJ1xuICBlbmQgPSBNYXRoLm1pbihidWYubGVuZ3RoLCBlbmQpXG5cbiAgZm9yICh2YXIgaSA9IHN0YXJ0OyBpIDwgZW5kOyBpKyspIHtcbiAgICBpZiAoYnVmW2ldIDw9IDB4N0YpIHtcbiAgICAgIHJlcyArPSBkZWNvZGVVdGY4Q2hhcih0bXApICsgU3RyaW5nLmZyb21DaGFyQ29kZShidWZbaV0pXG4gICAgICB0bXAgPSAnJ1xuICAgIH0gZWxzZSB7XG4gICAgICB0bXAgKz0gJyUnICsgYnVmW2ldLnRvU3RyaW5nKDE2KVxuICAgIH1cbiAgfVxuXG4gIHJldHVybiByZXMgKyBkZWNvZGVVdGY4Q2hhcih0bXApXG59XG5cbmZ1bmN0aW9uIF9hc2NpaVNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIHJldCA9ICcnXG4gIGVuZCA9IE1hdGgubWluKGJ1Zi5sZW5ndGgsIGVuZClcblxuICBmb3IgKHZhciBpID0gc3RhcnQ7IGkgPCBlbmQ7IGkrKylcbiAgICByZXQgKz0gU3RyaW5nLmZyb21DaGFyQ29kZShidWZbaV0pXG4gIHJldHVybiByZXRcbn1cblxuZnVuY3Rpb24gX2JpbmFyeVNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgcmV0dXJuIF9hc2NpaVNsaWNlKGJ1Ziwgc3RhcnQsIGVuZClcbn1cblxuZnVuY3Rpb24gX2hleFNsaWNlIChidWYsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcblxuICBpZiAoIXN0YXJ0IHx8IHN0YXJ0IDwgMCkgc3RhcnQgPSAwXG4gIGlmICghZW5kIHx8IGVuZCA8IDAgfHwgZW5kID4gbGVuKSBlbmQgPSBsZW5cblxuICB2YXIgb3V0ID0gJydcbiAgZm9yICh2YXIgaSA9IHN0YXJ0OyBpIDwgZW5kOyBpKyspIHtcbiAgICBvdXQgKz0gdG9IZXgoYnVmW2ldKVxuICB9XG4gIHJldHVybiBvdXRcbn1cblxuLy8gVE9ETzogYWRkIHRlc3QgdGhhdCBtb2RpZnlpbmcgdGhlIG5ldyBidWZmZXIgc2xpY2Ugd2lsbCBtb2RpZnkgbWVtb3J5IGluIHRoZVxuLy8gb3JpZ2luYWwgYnVmZmVyISBVc2UgY29kZSBmcm9tOlxuLy8gaHR0cDovL25vZGVqcy5vcmcvYXBpL2J1ZmZlci5odG1sI2J1ZmZlcl9idWZfc2xpY2Vfc3RhcnRfZW5kXG5mdW5jdGlvbiBCdWZmZXJTbGljZSAoc3RhcnQsIGVuZCkge1xuICB2YXIgbGVuID0gdGhpcy5sZW5ndGhcbiAgc3RhcnQgPSBjbGFtcChzdGFydCwgbGVuLCAwKVxuICBlbmQgPSBjbGFtcChlbmQsIGxlbiwgbGVuKVxuICByZXR1cm4gYXVnbWVudCh0aGlzLnN1YmFycmF5KHN0YXJ0LCBlbmQpKSAvLyBVaW50OEFycmF5IGJ1aWx0LWluIG1ldGhvZFxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkVUludDggKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1Zi5sZW5ndGgpXG4gICAgcmV0dXJuXG5cbiAgcmV0dXJuIGJ1ZltvZmZzZXRdXG59XG5cbmZ1bmN0aW9uIF9yZWFkVUludDE2IChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMSA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbikge1xuICAgIHJldHVyblxuICB9IGVsc2UgaWYgKG9mZnNldCArIDEgPT09IGxlbikge1xuICAgIHZhciBkdiA9IG5ldyB4RGF0YVZpZXcobmV3IHhBcnJheUJ1ZmZlcigyKSlcbiAgICBkdi5zZXRVaW50OCgwLCBidWZbbGVuIC0gMV0pXG4gICAgcmV0dXJuIGR2LmdldFVpbnQxNigwLCBsaXR0bGVFbmRpYW4pXG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIGJ1Zi5fZGF0YXZpZXcuZ2V0VWludDE2KG9mZnNldCwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRVSW50MTZMRSAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWRVSW50MTYodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZFVJbnQxNkJFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZFVJbnQxNih0aGlzLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gX3JlYWRVSW50MzIgKGJ1Ziwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodHlwZW9mIChsaXR0bGVFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKSB7XG4gICAgcmV0dXJuXG4gIH0gZWxzZSBpZiAob2Zmc2V0ICsgMyA+PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoNCkpXG4gICAgZm9yICh2YXIgaSA9IDA7IGkgKyBvZmZzZXQgPCBsZW47IGkrKykge1xuICAgICAgZHYuc2V0VWludDgoaSwgYnVmW2kgKyBvZmZzZXRdKVxuICAgIH1cbiAgICByZXR1cm4gZHYuZ2V0VWludDMyKDAsIGxpdHRsZUVuZGlhbilcbiAgfSBlbHNlIHtcbiAgICByZXR1cm4gYnVmLl9kYXRhdmlldy5nZXRVaW50MzIob2Zmc2V0LCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZFVJbnQzMkxFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZFVJbnQzMih0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkVUludDMyQkUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkVUludDMyKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkSW50OCAob2Zmc2V0LCBub0Fzc2VydCkge1xuICB2YXIgYnVmID0gdGhpc1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0IDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIGlmIChvZmZzZXQgPj0gYnVmLmxlbmd0aClcbiAgICByZXR1cm5cblxuICByZXR1cm4gYnVmLl9kYXRhdmlldy5nZXRJbnQ4KG9mZnNldClcbn1cblxuZnVuY3Rpb24gX3JlYWRJbnQxNiAoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDEgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAxID09PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoMikpXG4gICAgZHYuc2V0VWludDgoMCwgYnVmW2xlbiAtIDFdKVxuICAgIHJldHVybiBkdi5nZXRJbnQxNigwLCBsaXR0bGVFbmRpYW4pXG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIGJ1Zi5fZGF0YXZpZXcuZ2V0SW50MTYob2Zmc2V0LCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZEludDE2TEUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkSW50MTYodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZEludDE2QkUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkSW50MTYodGhpcywgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF9yZWFkSW50MzIgKGJ1Ziwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodHlwZW9mIChsaXR0bGVFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKSB7XG4gICAgcmV0dXJuXG4gIH0gZWxzZSBpZiAob2Zmc2V0ICsgMyA+PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoNCkpXG4gICAgZm9yICh2YXIgaSA9IDA7IGkgKyBvZmZzZXQgPCBsZW47IGkrKykge1xuICAgICAgZHYuc2V0VWludDgoaSwgYnVmW2kgKyBvZmZzZXRdKVxuICAgIH1cbiAgICByZXR1cm4gZHYuZ2V0SW50MzIoMCwgbGl0dGxlRW5kaWFuKVxuICB9IGVsc2Uge1xuICAgIHJldHVybiBidWYuX2RhdGF2aWV3LmdldEludDMyKG9mZnNldCwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRJbnQzMkxFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEludDMyKHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRJbnQzMkJFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEludDMyKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfcmVhZEZsb2F0IChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCArIDMgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgcmV0dXJuIGJ1Zi5fZGF0YXZpZXcuZ2V0RmxvYXQzMihvZmZzZXQsIGxpdHRsZUVuZGlhbilcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZEZsb2F0TEUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkRmxvYXQodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZEZsb2F0QkUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkRmxvYXQodGhpcywgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF9yZWFkRG91YmxlIChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCArIDcgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgcmV0dXJuIGJ1Zi5fZGF0YXZpZXcuZ2V0RmxvYXQ2NChvZmZzZXQsIGxpdHRsZUVuZGlhbilcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZERvdWJsZUxFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZERvdWJsZSh0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkRG91YmxlQkUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkRG91YmxlKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZVVJbnQ4ICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB2YXIgYnVmID0gdGhpc1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsICdtaXNzaW5nIHZhbHVlJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgPCBidWYubGVuZ3RoLCAndHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZnVpbnQodmFsdWUsIDB4ZmYpXG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1Zi5sZW5ndGgpIHJldHVyblxuXG4gIGJ1ZltvZmZzZXRdID0gdmFsdWVcbn1cblxuZnVuY3Rpb24gX3dyaXRlVUludDE2IChidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMSA8IGJ1Zi5sZW5ndGgsICd0cnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmdWludCh2YWx1ZSwgMHhmZmZmKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAxID09PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoMikpXG4gICAgZHYuc2V0VWludDE2KDAsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gICAgYnVmW29mZnNldF0gPSBkdi5nZXRVaW50OCgwKVxuICB9IGVsc2Uge1xuICAgIGJ1Zi5fZGF0YXZpZXcuc2V0VWludDE2KG9mZnNldCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgfVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZVVJbnQxNkxFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVVSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlVUludDE2QkUgKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZVVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF93cml0ZVVJbnQzMiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDMgPCBidWYubGVuZ3RoLCAndHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZnVpbnQodmFsdWUsIDB4ZmZmZmZmZmYpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbikge1xuICAgIHJldHVyblxuICB9IGVsc2UgaWYgKG9mZnNldCArIDMgPj0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDQpKVxuICAgIGR2LnNldFVpbnQzMigwLCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICAgIGZvciAodmFyIGkgPSAwOyBpICsgb2Zmc2V0IDwgbGVuOyBpKyspIHtcbiAgICAgIGJ1ZltpICsgb2Zmc2V0XSA9IGR2LmdldFVpbnQ4KGkpXG4gICAgfVxuICB9IGVsc2Uge1xuICAgIGJ1Zi5fZGF0YXZpZXcuc2V0VWludDMyKG9mZnNldCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgfVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZVVJbnQzMkxFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVVSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlVUludDMyQkUgKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZVVJbnQzMih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlSW50OCAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0IDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZzaW50KHZhbHVlLCAweDdmLCAtMHg4MClcbiAgfVxuXG4gIGlmIChvZmZzZXQgPj0gYnVmLmxlbmd0aCkgcmV0dXJuXG5cbiAgYnVmLl9kYXRhdmlldy5zZXRJbnQ4KG9mZnNldCwgdmFsdWUpXG59XG5cbmZ1bmN0aW9uIF93cml0ZUludDE2IChidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMSA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmc2ludCh2YWx1ZSwgMHg3ZmZmLCAtMHg4MDAwKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAxID09PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoMikpXG4gICAgZHYuc2V0SW50MTYoMCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgICBidWZbb2Zmc2V0XSA9IGR2LmdldFVpbnQ4KDApXG4gIH0gZWxzZSB7XG4gICAgYnVmLl9kYXRhdmlldy5zZXRJbnQxNihvZmZzZXQsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVJbnQxNkxFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVJbnQxNkJFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF93cml0ZUludDMyIChidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMyA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmc2ludCh2YWx1ZSwgMHg3ZmZmZmZmZiwgLTB4ODAwMDAwMDApXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbikge1xuICAgIHJldHVyblxuICB9IGVsc2UgaWYgKG9mZnNldCArIDMgPj0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDQpKVxuICAgIGR2LnNldEludDMyKDAsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gICAgZm9yICh2YXIgaSA9IDA7IGkgKyBvZmZzZXQgPCBsZW47IGkrKykge1xuICAgICAgYnVmW2kgKyBvZmZzZXRdID0gZHYuZ2V0VWludDgoaSlcbiAgICB9XG4gIH0gZWxzZSB7XG4gICAgYnVmLl9kYXRhdmlldy5zZXRJbnQzMihvZmZzZXQsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVJbnQzMkxFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVJbnQzMih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVJbnQzMkJFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVJbnQzMih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF93cml0ZUZsb2F0IChidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMyA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmSUVFRTc1NCh2YWx1ZSwgMy40MDI4MjM0NjYzODUyODg2ZSszOCwgLTMuNDAyODIzNDY2Mzg1Mjg4NmUrMzgpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbikge1xuICAgIHJldHVyblxuICB9IGVsc2UgaWYgKG9mZnNldCArIDMgPj0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDQpKVxuICAgIGR2LnNldEZsb2F0MzIoMCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgICBmb3IgKHZhciBpID0gMDsgaSArIG9mZnNldCA8IGxlbjsgaSsrKSB7XG4gICAgICBidWZbaSArIG9mZnNldF0gPSBkdi5nZXRVaW50OChpKVxuICAgIH1cbiAgfSBlbHNlIHtcbiAgICBidWYuX2RhdGF2aWV3LnNldEZsb2F0MzIob2Zmc2V0LCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlRmxvYXRMRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlRmxvYXQodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlRmxvYXRCRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlRmxvYXQodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfd3JpdGVEb3VibGUgKGJ1ZiwgdmFsdWUsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsICdtaXNzaW5nIHZhbHVlJylcbiAgICBhc3NlcnQodHlwZW9mIChsaXR0bGVFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyA3IDwgYnVmLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZJRUVFNzU0KHZhbHVlLCAxLjc5NzY5MzEzNDg2MjMxNTdFKzMwOCwgLTEuNzk3NjkzMTM0ODYyMzE1N0UrMzA4KVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyA3ID49IGxlbikge1xuICAgIHZhciBkdiA9IG5ldyB4RGF0YVZpZXcobmV3IHhBcnJheUJ1ZmZlcig4KSlcbiAgICBkdi5zZXRGbG9hdDY0KDAsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gICAgZm9yICh2YXIgaSA9IDA7IGkgKyBvZmZzZXQgPCBsZW47IGkrKykge1xuICAgICAgYnVmW2kgKyBvZmZzZXRdID0gZHYuZ2V0VWludDgoaSlcbiAgICB9XG4gIH0gZWxzZSB7XG4gICAgYnVmLl9kYXRhdmlldy5zZXRGbG9hdDY0KG9mZnNldCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgfVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZURvdWJsZUxFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVEb3VibGUodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlRG91YmxlQkUgKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZURvdWJsZSh0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbi8vIGZpbGwodmFsdWUsIHN0YXJ0PTAsIGVuZD1idWZmZXIubGVuZ3RoKVxuZnVuY3Rpb24gQnVmZmVyRmlsbCAodmFsdWUsIHN0YXJ0LCBlbmQpIHtcbiAgaWYgKCF2YWx1ZSkgdmFsdWUgPSAwXG4gIGlmICghc3RhcnQpIHN0YXJ0ID0gMFxuICBpZiAoIWVuZCkgZW5kID0gdGhpcy5sZW5ndGhcblxuICBpZiAodHlwZW9mIHZhbHVlID09PSAnc3RyaW5nJykge1xuICAgIHZhbHVlID0gdmFsdWUuY2hhckNvZGVBdCgwKVxuICB9XG5cbiAgaWYgKHR5cGVvZiB2YWx1ZSAhPT0gJ251bWJlcicgfHwgaXNOYU4odmFsdWUpKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCd2YWx1ZSBpcyBub3QgYSBudW1iZXInKVxuICB9XG5cbiAgaWYgKGVuZCA8IHN0YXJ0KSB0aHJvdyBuZXcgRXJyb3IoJ2VuZCA8IHN0YXJ0JylcblxuICAvLyBGaWxsIDAgYnl0ZXM7IHdlJ3JlIGRvbmVcbiAgaWYgKGVuZCA9PT0gc3RhcnQpIHJldHVyblxuICBpZiAodGhpcy5sZW5ndGggPT09IDApIHJldHVyblxuXG4gIGlmIChzdGFydCA8IDAgfHwgc3RhcnQgPj0gdGhpcy5sZW5ndGgpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3N0YXJ0IG91dCBvZiBib3VuZHMnKVxuICB9XG5cbiAgaWYgKGVuZCA8IDAgfHwgZW5kID4gdGhpcy5sZW5ndGgpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ2VuZCBvdXQgb2YgYm91bmRzJylcbiAgfVxuXG4gIGZvciAodmFyIGkgPSBzdGFydDsgaSA8IGVuZDsgaSsrKSB7XG4gICAgdGhpc1tpXSA9IHZhbHVlXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVySW5zcGVjdCAoKSB7XG4gIHZhciBvdXQgPSBbXVxuICB2YXIgbGVuID0gdGhpcy5sZW5ndGhcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBsZW47IGkrKykge1xuICAgIG91dFtpXSA9IHRvSGV4KHRoaXNbaV0pXG4gICAgaWYgKGkgPT09IGV4cG9ydHMuSU5TUEVDVF9NQVhfQllURVMpIHtcbiAgICAgIG91dFtpICsgMV0gPSAnLi4uJ1xuICAgICAgYnJlYWtcbiAgICB9XG4gIH1cbiAgcmV0dXJuICc8QnVmZmVyICcgKyBvdXQuam9pbignICcpICsgJz4nXG59XG5cbi8vIENyZWF0ZXMgYSBuZXcgYEFycmF5QnVmZmVyYCB3aXRoIHRoZSAqY29waWVkKiBtZW1vcnkgb2YgdGhlIGJ1ZmZlciBpbnN0YW5jZS5cbi8vIEFkZGVkIGluIE5vZGUgMC4xMi5cbmZ1bmN0aW9uIEJ1ZmZlclRvQXJyYXlCdWZmZXIgKCkge1xuICByZXR1cm4gKG5ldyBCdWZmZXIodGhpcykpLmJ1ZmZlclxufVxuXG5cbi8vIEhFTFBFUiBGVU5DVElPTlNcbi8vID09PT09PT09PT09PT09PT1cblxuZnVuY3Rpb24gc3RyaW5ndHJpbSAoc3RyKSB7XG4gIGlmIChzdHIudHJpbSkgcmV0dXJuIHN0ci50cmltKClcbiAgcmV0dXJuIHN0ci5yZXBsYWNlKC9eXFxzK3xcXHMrJC9nLCAnJylcbn1cblxuZnVuY3Rpb24gYXVnbWVudCAoYXJyKSB7XG4gIGFyci5faXNCdWZmZXIgPSB0cnVlXG5cbiAgLy8gQXVnbWVudCB0aGUgVWludDhBcnJheSAqaW5zdGFuY2UqIChub3QgdGhlIGNsYXNzISkgd2l0aCBCdWZmZXIgbWV0aG9kc1xuICBhcnIud3JpdGUgPSBCdWZmZXJXcml0ZVxuICBhcnIudG9TdHJpbmcgPSBCdWZmZXJUb1N0cmluZ1xuICBhcnIudG9Mb2NhbGVTdHJpbmcgPSBCdWZmZXJUb1N0cmluZ1xuICBhcnIudG9KU09OID0gQnVmZmVyVG9KU09OXG4gIGFyci5jb3B5ID0gQnVmZmVyQ29weVxuICBhcnIuc2xpY2UgPSBCdWZmZXJTbGljZVxuICBhcnIucmVhZFVJbnQ4ID0gQnVmZmVyUmVhZFVJbnQ4XG4gIGFyci5yZWFkVUludDE2TEUgPSBCdWZmZXJSZWFkVUludDE2TEVcbiAgYXJyLnJlYWRVSW50MTZCRSA9IEJ1ZmZlclJlYWRVSW50MTZCRVxuICBhcnIucmVhZFVJbnQzMkxFID0gQnVmZmVyUmVhZFVJbnQzMkxFXG4gIGFyci5yZWFkVUludDMyQkUgPSBCdWZmZXJSZWFkVUludDMyQkVcbiAgYXJyLnJlYWRJbnQ4ID0gQnVmZmVyUmVhZEludDhcbiAgYXJyLnJlYWRJbnQxNkxFID0gQnVmZmVyUmVhZEludDE2TEVcbiAgYXJyLnJlYWRJbnQxNkJFID0gQnVmZmVyUmVhZEludDE2QkVcbiAgYXJyLnJlYWRJbnQzMkxFID0gQnVmZmVyUmVhZEludDMyTEVcbiAgYXJyLnJlYWRJbnQzMkJFID0gQnVmZmVyUmVhZEludDMyQkVcbiAgYXJyLnJlYWRGbG9hdExFID0gQnVmZmVyUmVhZEZsb2F0TEVcbiAgYXJyLnJlYWRGbG9hdEJFID0gQnVmZmVyUmVhZEZsb2F0QkVcbiAgYXJyLnJlYWREb3VibGVMRSA9IEJ1ZmZlclJlYWREb3VibGVMRVxuICBhcnIucmVhZERvdWJsZUJFID0gQnVmZmVyUmVhZERvdWJsZUJFXG4gIGFyci53cml0ZVVJbnQ4ID0gQnVmZmVyV3JpdGVVSW50OFxuICBhcnIud3JpdGVVSW50MTZMRSA9IEJ1ZmZlcldyaXRlVUludDE2TEVcbiAgYXJyLndyaXRlVUludDE2QkUgPSBCdWZmZXJXcml0ZVVJbnQxNkJFXG4gIGFyci53cml0ZVVJbnQzMkxFID0gQnVmZmVyV3JpdGVVSW50MzJMRVxuICBhcnIud3JpdGVVSW50MzJCRSA9IEJ1ZmZlcldyaXRlVUludDMyQkVcbiAgYXJyLndyaXRlSW50OCA9IEJ1ZmZlcldyaXRlSW50OFxuICBhcnIud3JpdGVJbnQxNkxFID0gQnVmZmVyV3JpdGVJbnQxNkxFXG4gIGFyci53cml0ZUludDE2QkUgPSBCdWZmZXJXcml0ZUludDE2QkVcbiAgYXJyLndyaXRlSW50MzJMRSA9IEJ1ZmZlcldyaXRlSW50MzJMRVxuICBhcnIud3JpdGVJbnQzMkJFID0gQnVmZmVyV3JpdGVJbnQzMkJFXG4gIGFyci53cml0ZUZsb2F0TEUgPSBCdWZmZXJXcml0ZUZsb2F0TEVcbiAgYXJyLndyaXRlRmxvYXRCRSA9IEJ1ZmZlcldyaXRlRmxvYXRCRVxuICBhcnIud3JpdGVEb3VibGVMRSA9IEJ1ZmZlcldyaXRlRG91YmxlTEVcbiAgYXJyLndyaXRlRG91YmxlQkUgPSBCdWZmZXJXcml0ZURvdWJsZUJFXG4gIGFyci5maWxsID0gQnVmZmVyRmlsbFxuICBhcnIuaW5zcGVjdCA9IEJ1ZmZlckluc3BlY3RcblxuICAvLyBPbmx5IGFkZCBgdG9BcnJheUJ1ZmZlcmAgaWYgdGhlIGJyb3dzZXIgc3VwcG9ydHMgQXJyYXlCdWZmZXIgbmF0aXZlbHlcbiAgaWYgKHhVaW50OEFycmF5ICE9PSBUQS5VaW50OEFycmF5KVxuICAgIGFyci50b0FycmF5QnVmZmVyID0gQnVmZmVyVG9BcnJheUJ1ZmZlclxuXG4gIGlmIChhcnIuYnl0ZUxlbmd0aCAhPT0gMClcbiAgICBhcnIuX2RhdGF2aWV3ID0gbmV3IHhEYXRhVmlldyhhcnIuYnVmZmVyLCBhcnIuYnl0ZU9mZnNldCwgYXJyLmJ5dGVMZW5ndGgpXG5cbiAgcmV0dXJuIGFyclxufVxuXG4vLyBzbGljZShzdGFydCwgZW5kKVxuZnVuY3Rpb24gY2xhbXAgKGluZGV4LCBsZW4sIGRlZmF1bHRWYWx1ZSkge1xuICBpZiAodHlwZW9mIGluZGV4ICE9PSAnbnVtYmVyJykgcmV0dXJuIGRlZmF1bHRWYWx1ZVxuICBpbmRleCA9IH5+aW5kZXg7ICAvLyBDb2VyY2UgdG8gaW50ZWdlci5cbiAgaWYgKGluZGV4ID49IGxlbikgcmV0dXJuIGxlblxuICBpZiAoaW5kZXggPj0gMCkgcmV0dXJuIGluZGV4XG4gIGluZGV4ICs9IGxlblxuICBpZiAoaW5kZXggPj0gMCkgcmV0dXJuIGluZGV4XG4gIHJldHVybiAwXG59XG5cbmZ1bmN0aW9uIGNvZXJjZSAobGVuZ3RoKSB7XG4gIC8vIENvZXJjZSBsZW5ndGggdG8gYSBudW1iZXIgKHBvc3NpYmx5IE5hTiksIHJvdW5kIHVwXG4gIC8vIGluIGNhc2UgaXQncyBmcmFjdGlvbmFsIChlLmcuIDEyMy40NTYpIHRoZW4gZG8gYVxuICAvLyBkb3VibGUgbmVnYXRlIHRvIGNvZXJjZSBhIE5hTiB0byAwLiBFYXN5LCByaWdodD9cbiAgbGVuZ3RoID0gfn5NYXRoLmNlaWwoK2xlbmd0aClcbiAgcmV0dXJuIGxlbmd0aCA8IDAgPyAwIDogbGVuZ3RoXG59XG5cbmZ1bmN0aW9uIGlzQXJyYXkgKHN1YmplY3QpIHtcbiAgcmV0dXJuIChBcnJheS5pc0FycmF5IHx8IGZ1bmN0aW9uIChzdWJqZWN0KSB7XG4gICAgcmV0dXJuIE9iamVjdC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbChzdWJqZWN0KSA9PT0gJ1tvYmplY3QgQXJyYXldJ1xuICB9KShzdWJqZWN0KVxufVxuXG5mdW5jdGlvbiBpc0FycmF5SXNoIChzdWJqZWN0KSB7XG4gIHJldHVybiBpc0FycmF5KHN1YmplY3QpIHx8IEJ1ZmZlci5pc0J1ZmZlcihzdWJqZWN0KSB8fFxuICAgICAgc3ViamVjdCAmJiB0eXBlb2Ygc3ViamVjdCA9PT0gJ29iamVjdCcgJiZcbiAgICAgIHR5cGVvZiBzdWJqZWN0Lmxlbmd0aCA9PT0gJ251bWJlcidcbn1cblxuZnVuY3Rpb24gdG9IZXggKG4pIHtcbiAgaWYgKG4gPCAxNikgcmV0dXJuICcwJyArIG4udG9TdHJpbmcoMTYpXG4gIHJldHVybiBuLnRvU3RyaW5nKDE2KVxufVxuXG5mdW5jdGlvbiB1dGY4VG9CeXRlcyAoc3RyKSB7XG4gIHZhciBieXRlQXJyYXkgPSBbXVxuICBmb3IgKHZhciBpID0gMDsgaSA8IHN0ci5sZW5ndGg7IGkrKylcbiAgICBpZiAoc3RyLmNoYXJDb2RlQXQoaSkgPD0gMHg3RilcbiAgICAgIGJ5dGVBcnJheS5wdXNoKHN0ci5jaGFyQ29kZUF0KGkpKVxuICAgIGVsc2Uge1xuICAgICAgdmFyIGggPSBlbmNvZGVVUklDb21wb25lbnQoc3RyLmNoYXJBdChpKSkuc3Vic3RyKDEpLnNwbGl0KCclJylcbiAgICAgIGZvciAodmFyIGogPSAwOyBqIDwgaC5sZW5ndGg7IGorKylcbiAgICAgICAgYnl0ZUFycmF5LnB1c2gocGFyc2VJbnQoaFtqXSwgMTYpKVxuICAgIH1cblxuICByZXR1cm4gYnl0ZUFycmF5XG59XG5cbmZ1bmN0aW9uIGFzY2lpVG9CeXRlcyAoc3RyKSB7XG4gIHZhciBieXRlQXJyYXkgPSBbXVxuICBmb3IgKHZhciBpID0gMDsgaSA8IHN0ci5sZW5ndGg7IGkrKykge1xuICAgIC8vIE5vZGUncyBjb2RlIHNlZW1zIHRvIGJlIGRvaW5nIHRoaXMgYW5kIG5vdCAmIDB4N0YuLlxuICAgIGJ5dGVBcnJheS5wdXNoKHN0ci5jaGFyQ29kZUF0KGkpICYgMHhGRilcbiAgfVxuXG4gIHJldHVybiBieXRlQXJyYXlcbn1cblxuZnVuY3Rpb24gYmFzZTY0VG9CeXRlcyAoc3RyKSB7XG4gIHJldHVybiBiYXNlNjQudG9CeXRlQXJyYXkoc3RyKVxufVxuXG5mdW5jdGlvbiBibGl0QnVmZmVyIChzcmMsIGRzdCwgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIHBvc1xuICBmb3IgKHZhciBpID0gMDsgaSA8IGxlbmd0aDsgaSsrKSB7XG4gICAgaWYgKChpICsgb2Zmc2V0ID49IGRzdC5sZW5ndGgpIHx8IChpID49IHNyYy5sZW5ndGgpKVxuICAgICAgYnJlYWtcbiAgICBkc3RbaSArIG9mZnNldF0gPSBzcmNbaV1cbiAgfVxuICByZXR1cm4gaVxufVxuXG5mdW5jdGlvbiBkZWNvZGVVdGY4Q2hhciAoc3RyKSB7XG4gIHRyeSB7XG4gICAgcmV0dXJuIGRlY29kZVVSSUNvbXBvbmVudChzdHIpXG4gIH0gY2F0Y2ggKGVycikge1xuICAgIHJldHVybiBTdHJpbmcuZnJvbUNoYXJDb2RlKDB4RkZGRCkgLy8gVVRGIDggaW52YWxpZCBjaGFyXG4gIH1cbn1cblxuLypcbiAqIFdlIGhhdmUgdG8gbWFrZSBzdXJlIHRoYXQgdGhlIHZhbHVlIGlzIGEgdmFsaWQgaW50ZWdlci4gVGhpcyBtZWFucyB0aGF0IGl0XG4gKiBpcyBub24tbmVnYXRpdmUuIEl0IGhhcyBubyBmcmFjdGlvbmFsIGNvbXBvbmVudCBhbmQgdGhhdCBpdCBkb2VzIG5vdFxuICogZXhjZWVkIHRoZSBtYXhpbXVtIGFsbG93ZWQgdmFsdWUuXG4gKlxuICogICAgICB2YWx1ZSAgICAgICAgICAgVGhlIG51bWJlciB0byBjaGVjayBmb3IgdmFsaWRpdHlcbiAqXG4gKiAgICAgIG1heCAgICAgICAgICAgICBUaGUgbWF4aW11bSB2YWx1ZVxuICovXG5mdW5jdGlvbiB2ZXJpZnVpbnQgKHZhbHVlLCBtYXgpIHtcbiAgYXNzZXJ0KHR5cGVvZiAodmFsdWUpID09ICdudW1iZXInLCAnY2Fubm90IHdyaXRlIGEgbm9uLW51bWJlciBhcyBhIG51bWJlcicpXG4gIGFzc2VydCh2YWx1ZSA+PSAwLFxuICAgICAgJ3NwZWNpZmllZCBhIG5lZ2F0aXZlIHZhbHVlIGZvciB3cml0aW5nIGFuIHVuc2lnbmVkIHZhbHVlJylcbiAgYXNzZXJ0KHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGlzIGxhcmdlciB0aGFuIG1heGltdW0gdmFsdWUgZm9yIHR5cGUnKVxuICBhc3NlcnQoTWF0aC5mbG9vcih2YWx1ZSkgPT09IHZhbHVlLCAndmFsdWUgaGFzIGEgZnJhY3Rpb25hbCBjb21wb25lbnQnKVxufVxuXG4vKlxuICogQSBzZXJpZXMgb2YgY2hlY2tzIHRvIG1ha2Ugc3VyZSB3ZSBhY3R1YWxseSBoYXZlIGEgc2lnbmVkIDMyLWJpdCBudW1iZXJcbiAqL1xuZnVuY3Rpb24gdmVyaWZzaW50KHZhbHVlLCBtYXgsIG1pbikge1xuICBhc3NlcnQodHlwZW9mICh2YWx1ZSkgPT0gJ251bWJlcicsICdjYW5ub3Qgd3JpdGUgYSBub24tbnVtYmVyIGFzIGEgbnVtYmVyJylcbiAgYXNzZXJ0KHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGxhcmdlciB0aGFuIG1heGltdW0gYWxsb3dlZCB2YWx1ZScpXG4gIGFzc2VydCh2YWx1ZSA+PSBtaW4sICd2YWx1ZSBzbWFsbGVyIHRoYW4gbWluaW11bSBhbGxvd2VkIHZhbHVlJylcbiAgYXNzZXJ0KE1hdGguZmxvb3IodmFsdWUpID09PSB2YWx1ZSwgJ3ZhbHVlIGhhcyBhIGZyYWN0aW9uYWwgY29tcG9uZW50Jylcbn1cblxuZnVuY3Rpb24gdmVyaWZJRUVFNzU0KHZhbHVlLCBtYXgsIG1pbikge1xuICBhc3NlcnQodHlwZW9mICh2YWx1ZSkgPT0gJ251bWJlcicsICdjYW5ub3Qgd3JpdGUgYSBub24tbnVtYmVyIGFzIGEgbnVtYmVyJylcbiAgYXNzZXJ0KHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGxhcmdlciB0aGFuIG1heGltdW0gYWxsb3dlZCB2YWx1ZScpXG4gIGFzc2VydCh2YWx1ZSA+PSBtaW4sICd2YWx1ZSBzbWFsbGVyIHRoYW4gbWluaW11bSBhbGxvd2VkIHZhbHVlJylcbn1cblxuZnVuY3Rpb24gYXNzZXJ0ICh0ZXN0LCBtZXNzYWdlKSB7XG4gIGlmICghdGVzdCkgdGhyb3cgbmV3IEVycm9yKG1lc3NhZ2UgfHwgJ0ZhaWxlZCBhc3NlcnRpb24nKVxufVxuXG59LHtcImJhc2U2NC1qc1wiOjMsXCJ0eXBlZGFycmF5XCI6NH1dLFwibmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5XCI6W2Z1bmN0aW9uKHJlcXVpcmUsbW9kdWxlLGV4cG9ydHMpe1xubW9kdWxlLmV4cG9ydHM9cmVxdWlyZSgnMjZzRVY3Jyk7XG59LHt9XSwzOltmdW5jdGlvbihyZXF1aXJlLG1vZHVsZSxleHBvcnRzKXtcbihmdW5jdGlvbiAoZXhwb3J0cykge1xuXHQndXNlIHN0cmljdCc7XG5cblx0dmFyIGxvb2t1cCA9ICdBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWmFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6MDEyMzQ1Njc4OSsvJztcblxuXHRmdW5jdGlvbiBiNjRUb0J5dGVBcnJheShiNjQpIHtcblx0XHR2YXIgaSwgaiwgbCwgdG1wLCBwbGFjZUhvbGRlcnMsIGFycjtcblx0XG5cdFx0aWYgKGI2NC5sZW5ndGggJSA0ID4gMCkge1xuXHRcdFx0dGhyb3cgJ0ludmFsaWQgc3RyaW5nLiBMZW5ndGggbXVzdCBiZSBhIG11bHRpcGxlIG9mIDQnO1xuXHRcdH1cblxuXHRcdC8vIHRoZSBudW1iZXIgb2YgZXF1YWwgc2lnbnMgKHBsYWNlIGhvbGRlcnMpXG5cdFx0Ly8gaWYgdGhlcmUgYXJlIHR3byBwbGFjZWhvbGRlcnMsIHRoYW4gdGhlIHR3byBjaGFyYWN0ZXJzIGJlZm9yZSBpdFxuXHRcdC8vIHJlcHJlc2VudCBvbmUgYnl0ZVxuXHRcdC8vIGlmIHRoZXJlIGlzIG9ubHkgb25lLCB0aGVuIHRoZSB0aHJlZSBjaGFyYWN0ZXJzIGJlZm9yZSBpdCByZXByZXNlbnQgMiBieXRlc1xuXHRcdC8vIHRoaXMgaXMganVzdCBhIGNoZWFwIGhhY2sgdG8gbm90IGRvIGluZGV4T2YgdHdpY2Vcblx0XHRwbGFjZUhvbGRlcnMgPSBpbmRleE9mKGI2NCwgJz0nKTtcblx0XHRwbGFjZUhvbGRlcnMgPSBwbGFjZUhvbGRlcnMgPiAwID8gYjY0Lmxlbmd0aCAtIHBsYWNlSG9sZGVycyA6IDA7XG5cblx0XHQvLyBiYXNlNjQgaXMgNC8zICsgdXAgdG8gdHdvIGNoYXJhY3RlcnMgb2YgdGhlIG9yaWdpbmFsIGRhdGFcblx0XHRhcnIgPSBbXTsvL25ldyBVaW50OEFycmF5KGI2NC5sZW5ndGggKiAzIC8gNCAtIHBsYWNlSG9sZGVycyk7XG5cblx0XHQvLyBpZiB0aGVyZSBhcmUgcGxhY2Vob2xkZXJzLCBvbmx5IGdldCB1cCB0byB0aGUgbGFzdCBjb21wbGV0ZSA0IGNoYXJzXG5cdFx0bCA9IHBsYWNlSG9sZGVycyA+IDAgPyBiNjQubGVuZ3RoIC0gNCA6IGI2NC5sZW5ndGg7XG5cblx0XHRmb3IgKGkgPSAwLCBqID0gMDsgaSA8IGw7IGkgKz0gNCwgaiArPSAzKSB7XG5cdFx0XHR0bXAgPSAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSkpIDw8IDE4KSB8IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpICsgMSkpIDw8IDEyKSB8IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpICsgMikpIDw8IDYpIHwgaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSArIDMpKTtcblx0XHRcdGFyci5wdXNoKCh0bXAgJiAweEZGMDAwMCkgPj4gMTYpO1xuXHRcdFx0YXJyLnB1c2goKHRtcCAmIDB4RkYwMCkgPj4gOCk7XG5cdFx0XHRhcnIucHVzaCh0bXAgJiAweEZGKTtcblx0XHR9XG5cblx0XHRpZiAocGxhY2VIb2xkZXJzID09PSAyKSB7XG5cdFx0XHR0bXAgPSAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSkpIDw8IDIpIHwgKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkgKyAxKSkgPj4gNCk7XG5cdFx0XHRhcnIucHVzaCh0bXAgJiAweEZGKTtcblx0XHR9IGVsc2UgaWYgKHBsYWNlSG9sZGVycyA9PT0gMSkge1xuXHRcdFx0dG1wID0gKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkpKSA8PCAxMCkgfCAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSArIDEpKSA8PCA0KSB8IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpICsgMikpID4+IDIpO1xuXHRcdFx0YXJyLnB1c2goKHRtcCA+PiA4KSAmIDB4RkYpO1xuXHRcdFx0YXJyLnB1c2godG1wICYgMHhGRik7XG5cdFx0fVxuXG5cdFx0cmV0dXJuIGFycjtcblx0fVxuXG5cdGZ1bmN0aW9uIHVpbnQ4VG9CYXNlNjQodWludDgpIHtcblx0XHR2YXIgaSxcblx0XHRcdGV4dHJhQnl0ZXMgPSB1aW50OC5sZW5ndGggJSAzLCAvLyBpZiB3ZSBoYXZlIDEgYnl0ZSBsZWZ0LCBwYWQgMiBieXRlc1xuXHRcdFx0b3V0cHV0ID0gXCJcIixcblx0XHRcdHRlbXAsIGxlbmd0aDtcblxuXHRcdGZ1bmN0aW9uIHRyaXBsZXRUb0Jhc2U2NCAobnVtKSB7XG5cdFx0XHRyZXR1cm4gbG9va3VwLmNoYXJBdChudW0gPj4gMTggJiAweDNGKSArIGxvb2t1cC5jaGFyQXQobnVtID4+IDEyICYgMHgzRikgKyBsb29rdXAuY2hhckF0KG51bSA+PiA2ICYgMHgzRikgKyBsb29rdXAuY2hhckF0KG51bSAmIDB4M0YpO1xuXHRcdH07XG5cblx0XHQvLyBnbyB0aHJvdWdoIHRoZSBhcnJheSBldmVyeSB0aHJlZSBieXRlcywgd2UnbGwgZGVhbCB3aXRoIHRyYWlsaW5nIHN0dWZmIGxhdGVyXG5cdFx0Zm9yIChpID0gMCwgbGVuZ3RoID0gdWludDgubGVuZ3RoIC0gZXh0cmFCeXRlczsgaSA8IGxlbmd0aDsgaSArPSAzKSB7XG5cdFx0XHR0ZW1wID0gKHVpbnQ4W2ldIDw8IDE2KSArICh1aW50OFtpICsgMV0gPDwgOCkgKyAodWludDhbaSArIDJdKTtcblx0XHRcdG91dHB1dCArPSB0cmlwbGV0VG9CYXNlNjQodGVtcCk7XG5cdFx0fVxuXG5cdFx0Ly8gcGFkIHRoZSBlbmQgd2l0aCB6ZXJvcywgYnV0IG1ha2Ugc3VyZSB0byBub3QgZm9yZ2V0IHRoZSBleHRyYSBieXRlc1xuXHRcdHN3aXRjaCAoZXh0cmFCeXRlcykge1xuXHRcdFx0Y2FzZSAxOlxuXHRcdFx0XHR0ZW1wID0gdWludDhbdWludDgubGVuZ3RoIC0gMV07XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXAuY2hhckF0KHRlbXAgPj4gMik7XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXAuY2hhckF0KCh0ZW1wIDw8IDQpICYgMHgzRik7XG5cdFx0XHRcdG91dHB1dCArPSAnPT0nO1xuXHRcdFx0XHRicmVhaztcblx0XHRcdGNhc2UgMjpcblx0XHRcdFx0dGVtcCA9ICh1aW50OFt1aW50OC5sZW5ndGggLSAyXSA8PCA4KSArICh1aW50OFt1aW50OC5sZW5ndGggLSAxXSk7XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXAuY2hhckF0KHRlbXAgPj4gMTApO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwLmNoYXJBdCgodGVtcCA+PiA0KSAmIDB4M0YpO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwLmNoYXJBdCgodGVtcCA8PCAyKSAmIDB4M0YpO1xuXHRcdFx0XHRvdXRwdXQgKz0gJz0nO1xuXHRcdFx0XHRicmVhaztcblx0XHR9XG5cblx0XHRyZXR1cm4gb3V0cHV0O1xuXHR9XG5cblx0bW9kdWxlLmV4cG9ydHMudG9CeXRlQXJyYXkgPSBiNjRUb0J5dGVBcnJheTtcblx0bW9kdWxlLmV4cG9ydHMuZnJvbUJ5dGVBcnJheSA9IHVpbnQ4VG9CYXNlNjQ7XG59KCkpO1xuXG5mdW5jdGlvbiBpbmRleE9mIChhcnIsIGVsdCAvKiwgZnJvbSovKSB7XG5cdHZhciBsZW4gPSBhcnIubGVuZ3RoO1xuXG5cdHZhciBmcm9tID0gTnVtYmVyKGFyZ3VtZW50c1sxXSkgfHwgMDtcblx0ZnJvbSA9IChmcm9tIDwgMClcblx0XHQ/IE1hdGguY2VpbChmcm9tKVxuXHRcdDogTWF0aC5mbG9vcihmcm9tKTtcblx0aWYgKGZyb20gPCAwKVxuXHRcdGZyb20gKz0gbGVuO1xuXG5cdGZvciAoOyBmcm9tIDwgbGVuOyBmcm9tKyspIHtcblx0XHRpZiAoKHR5cGVvZiBhcnIgPT09ICdzdHJpbmcnICYmIGFyci5jaGFyQXQoZnJvbSkgPT09IGVsdCkgfHxcblx0XHRcdFx0KHR5cGVvZiBhcnIgIT09ICdzdHJpbmcnICYmIGFycltmcm9tXSA9PT0gZWx0KSkge1xuXHRcdFx0cmV0dXJuIGZyb207XG5cdFx0fVxuXHR9XG5cdHJldHVybiAtMTtcbn1cblxufSx7fV0sNDpbZnVuY3Rpb24ocmVxdWlyZSxtb2R1bGUsZXhwb3J0cyl7XG52YXIgdW5kZWZpbmVkID0gKHZvaWQgMCk7IC8vIFBhcmFub2lhXG5cbi8vIEJleW9uZCB0aGlzIHZhbHVlLCBpbmRleCBnZXR0ZXJzL3NldHRlcnMgKGkuZS4gYXJyYXlbMF0sIGFycmF5WzFdKSBhcmUgc28gc2xvdyB0b1xuLy8gY3JlYXRlLCBhbmQgY29uc3VtZSBzbyBtdWNoIG1lbW9yeSwgdGhhdCB0aGUgYnJvd3NlciBhcHBlYXJzIGZyb3plbi5cbnZhciBNQVhfQVJSQVlfTEVOR1RIID0gMWU1O1xuXG4vLyBBcHByb3hpbWF0aW9ucyBvZiBpbnRlcm5hbCBFQ01BU2NyaXB0IGNvbnZlcnNpb24gZnVuY3Rpb25zXG52YXIgRUNNQVNjcmlwdCA9IChmdW5jdGlvbigpIHtcbiAgLy8gU3Rhc2ggYSBjb3B5IGluIGNhc2Ugb3RoZXIgc2NyaXB0cyBtb2RpZnkgdGhlc2VcbiAgdmFyIG9wdHMgPSBPYmplY3QucHJvdG90eXBlLnRvU3RyaW5nLFxuICAgICAgb3Bob3AgPSBPYmplY3QucHJvdG90eXBlLmhhc093blByb3BlcnR5O1xuXG4gIHJldHVybiB7XG4gICAgLy8gQ2xhc3MgcmV0dXJucyBpbnRlcm5hbCBbW0NsYXNzXV0gcHJvcGVydHksIHVzZWQgdG8gYXZvaWQgY3Jvc3MtZnJhbWUgaW5zdGFuY2VvZiBpc3N1ZXM6XG4gICAgQ2xhc3M6IGZ1bmN0aW9uKHYpIHsgcmV0dXJuIG9wdHMuY2FsbCh2KS5yZXBsYWNlKC9eXFxbb2JqZWN0ICp8XFxdJC9nLCAnJyk7IH0sXG4gICAgSGFzUHJvcGVydHk6IGZ1bmN0aW9uKG8sIHApIHsgcmV0dXJuIHAgaW4gbzsgfSxcbiAgICBIYXNPd25Qcm9wZXJ0eTogZnVuY3Rpb24obywgcCkgeyByZXR1cm4gb3Bob3AuY2FsbChvLCBwKTsgfSxcbiAgICBJc0NhbGxhYmxlOiBmdW5jdGlvbihvKSB7IHJldHVybiB0eXBlb2YgbyA9PT0gJ2Z1bmN0aW9uJzsgfSxcbiAgICBUb0ludDMyOiBmdW5jdGlvbih2KSB7IHJldHVybiB2ID4+IDA7IH0sXG4gICAgVG9VaW50MzI6IGZ1bmN0aW9uKHYpIHsgcmV0dXJuIHYgPj4+IDA7IH1cbiAgfTtcbn0oKSk7XG5cbi8vIFNuYXBzaG90IGludHJpbnNpY3NcbnZhciBMTjIgPSBNYXRoLkxOMixcbiAgICBhYnMgPSBNYXRoLmFicyxcbiAgICBmbG9vciA9IE1hdGguZmxvb3IsXG4gICAgbG9nID0gTWF0aC5sb2csXG4gICAgbWluID0gTWF0aC5taW4sXG4gICAgcG93ID0gTWF0aC5wb3csXG4gICAgcm91bmQgPSBNYXRoLnJvdW5kO1xuXG4vLyBFUzU6IGxvY2sgZG93biBvYmplY3QgcHJvcGVydGllc1xuZnVuY3Rpb24gY29uZmlndXJlUHJvcGVydGllcyhvYmopIHtcbiAgaWYgKGdldE93blByb3BOYW1lcyAmJiBkZWZpbmVQcm9wKSB7XG4gICAgdmFyIHByb3BzID0gZ2V0T3duUHJvcE5hbWVzKG9iaiksIGk7XG4gICAgZm9yIChpID0gMDsgaSA8IHByb3BzLmxlbmd0aDsgaSArPSAxKSB7XG4gICAgICBkZWZpbmVQcm9wKG9iaiwgcHJvcHNbaV0sIHtcbiAgICAgICAgdmFsdWU6IG9ialtwcm9wc1tpXV0sXG4gICAgICAgIHdyaXRhYmxlOiBmYWxzZSxcbiAgICAgICAgZW51bWVyYWJsZTogZmFsc2UsXG4gICAgICAgIGNvbmZpZ3VyYWJsZTogZmFsc2VcbiAgICAgIH0pO1xuICAgIH1cbiAgfVxufVxuXG4vLyBlbXVsYXRlIEVTNSBnZXR0ZXIvc2V0dGVyIEFQSSB1c2luZyBsZWdhY3kgQVBJc1xuLy8gaHR0cDovL2Jsb2dzLm1zZG4uY29tL2IvaWUvYXJjaGl2ZS8yMDEwLzA5LzA3L3RyYW5zaXRpb25pbmctZXhpc3RpbmctY29kZS10by10aGUtZXM1LWdldHRlci1zZXR0ZXItYXBpcy5hc3B4XG4vLyAoc2Vjb25kIGNsYXVzZSB0ZXN0cyBmb3IgT2JqZWN0LmRlZmluZVByb3BlcnR5KCkgaW4gSUU8OSB0aGF0IG9ubHkgc3VwcG9ydHMgZXh0ZW5kaW5nIERPTSBwcm90b3R5cGVzLCBidXRcbi8vIG5vdGUgdGhhdCBJRTw5IGRvZXMgbm90IHN1cHBvcnQgX19kZWZpbmVHZXR0ZXJfXyBvciBfX2RlZmluZVNldHRlcl9fIHNvIGl0IGp1c3QgcmVuZGVycyB0aGUgbWV0aG9kIGhhcm1sZXNzKVxudmFyIGRlZmluZVByb3BcbmlmIChPYmplY3QuZGVmaW5lUHJvcGVydHkgJiYgKGZ1bmN0aW9uKCkge1xuICAgICAgdHJ5IHtcbiAgICAgICAgT2JqZWN0LmRlZmluZVByb3BlcnR5KHt9LCAneCcsIHt9KTtcbiAgICAgICAgcmV0dXJuIHRydWU7XG4gICAgICB9IGNhdGNoIChlKSB7XG4gICAgICAgIHJldHVybiBmYWxzZTtcbiAgICAgIH1cbiAgICB9KSgpKSB7XG4gIGRlZmluZVByb3AgPSBPYmplY3QuZGVmaW5lUHJvcGVydHk7XG59IGVsc2Uge1xuICBkZWZpbmVQcm9wID0gZnVuY3Rpb24obywgcCwgZGVzYykge1xuICAgIGlmICghbyA9PT0gT2JqZWN0KG8pKSB0aHJvdyBuZXcgVHlwZUVycm9yKFwiT2JqZWN0LmRlZmluZVByb3BlcnR5IGNhbGxlZCBvbiBub24tb2JqZWN0XCIpO1xuICAgIGlmIChFQ01BU2NyaXB0Lkhhc1Byb3BlcnR5KGRlc2MsICdnZXQnKSAmJiBPYmplY3QucHJvdG90eXBlLl9fZGVmaW5lR2V0dGVyX18pIHsgT2JqZWN0LnByb3RvdHlwZS5fX2RlZmluZUdldHRlcl9fLmNhbGwobywgcCwgZGVzYy5nZXQpOyB9XG4gICAgaWYgKEVDTUFTY3JpcHQuSGFzUHJvcGVydHkoZGVzYywgJ3NldCcpICYmIE9iamVjdC5wcm90b3R5cGUuX19kZWZpbmVTZXR0ZXJfXykgeyBPYmplY3QucHJvdG90eXBlLl9fZGVmaW5lU2V0dGVyX18uY2FsbChvLCBwLCBkZXNjLnNldCk7IH1cbiAgICBpZiAoRUNNQVNjcmlwdC5IYXNQcm9wZXJ0eShkZXNjLCAndmFsdWUnKSkgeyBvW3BdID0gZGVzYy52YWx1ZTsgfVxuICAgIHJldHVybiBvO1xuICB9O1xufVxuXG52YXIgZ2V0T3duUHJvcE5hbWVzID0gT2JqZWN0LmdldE93blByb3BlcnR5TmFtZXMgfHwgZnVuY3Rpb24gKG8pIHtcbiAgaWYgKG8gIT09IE9iamVjdChvKSkgdGhyb3cgbmV3IFR5cGVFcnJvcihcIk9iamVjdC5nZXRPd25Qcm9wZXJ0eU5hbWVzIGNhbGxlZCBvbiBub24tb2JqZWN0XCIpO1xuICB2YXIgcHJvcHMgPSBbXSwgcDtcbiAgZm9yIChwIGluIG8pIHtcbiAgICBpZiAoRUNNQVNjcmlwdC5IYXNPd25Qcm9wZXJ0eShvLCBwKSkge1xuICAgICAgcHJvcHMucHVzaChwKTtcbiAgICB9XG4gIH1cbiAgcmV0dXJuIHByb3BzO1xufTtcblxuLy8gRVM1OiBNYWtlIG9ialtpbmRleF0gYW4gYWxpYXMgZm9yIG9iai5fZ2V0dGVyKGluZGV4KS9vYmouX3NldHRlcihpbmRleCwgdmFsdWUpXG4vLyBmb3IgaW5kZXggaW4gMCAuLi4gb2JqLmxlbmd0aFxuZnVuY3Rpb24gbWFrZUFycmF5QWNjZXNzb3JzKG9iaikge1xuICBpZiAoIWRlZmluZVByb3ApIHsgcmV0dXJuOyB9XG5cbiAgaWYgKG9iai5sZW5ndGggPiBNQVhfQVJSQVlfTEVOR1RIKSB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcIkFycmF5IHRvbyBsYXJnZSBmb3IgcG9seWZpbGxcIik7XG5cbiAgZnVuY3Rpb24gbWFrZUFycmF5QWNjZXNzb3IoaW5kZXgpIHtcbiAgICBkZWZpbmVQcm9wKG9iaiwgaW5kZXgsIHtcbiAgICAgICdnZXQnOiBmdW5jdGlvbigpIHsgcmV0dXJuIG9iai5fZ2V0dGVyKGluZGV4KTsgfSxcbiAgICAgICdzZXQnOiBmdW5jdGlvbih2KSB7IG9iai5fc2V0dGVyKGluZGV4LCB2KTsgfSxcbiAgICAgIGVudW1lcmFibGU6IHRydWUsXG4gICAgICBjb25maWd1cmFibGU6IGZhbHNlXG4gICAgfSk7XG4gIH1cblxuICB2YXIgaTtcbiAgZm9yIChpID0gMDsgaSA8IG9iai5sZW5ndGg7IGkgKz0gMSkge1xuICAgIG1ha2VBcnJheUFjY2Vzc29yKGkpO1xuICB9XG59XG5cbi8vIEludGVybmFsIGNvbnZlcnNpb24gZnVuY3Rpb25zOlxuLy8gICAgcGFjazxUeXBlPigpICAgLSB0YWtlIGEgbnVtYmVyIChpbnRlcnByZXRlZCBhcyBUeXBlKSwgb3V0cHV0IGEgYnl0ZSBhcnJheVxuLy8gICAgdW5wYWNrPFR5cGU+KCkgLSB0YWtlIGEgYnl0ZSBhcnJheSwgb3V0cHV0IGEgVHlwZS1saWtlIG51bWJlclxuXG5mdW5jdGlvbiBhc19zaWduZWQodmFsdWUsIGJpdHMpIHsgdmFyIHMgPSAzMiAtIGJpdHM7IHJldHVybiAodmFsdWUgPDwgcykgPj4gczsgfVxuZnVuY3Rpb24gYXNfdW5zaWduZWQodmFsdWUsIGJpdHMpIHsgdmFyIHMgPSAzMiAtIGJpdHM7IHJldHVybiAodmFsdWUgPDwgcykgPj4+IHM7IH1cblxuZnVuY3Rpb24gcGFja0k4KG4pIHsgcmV0dXJuIFtuICYgMHhmZl07IH1cbmZ1bmN0aW9uIHVucGFja0k4KGJ5dGVzKSB7IHJldHVybiBhc19zaWduZWQoYnl0ZXNbMF0sIDgpOyB9XG5cbmZ1bmN0aW9uIHBhY2tVOChuKSB7IHJldHVybiBbbiAmIDB4ZmZdOyB9XG5mdW5jdGlvbiB1bnBhY2tVOChieXRlcykgeyByZXR1cm4gYXNfdW5zaWduZWQoYnl0ZXNbMF0sIDgpOyB9XG5cbmZ1bmN0aW9uIHBhY2tVOENsYW1wZWQobikgeyBuID0gcm91bmQoTnVtYmVyKG4pKTsgcmV0dXJuIFtuIDwgMCA/IDAgOiBuID4gMHhmZiA/IDB4ZmYgOiBuICYgMHhmZl07IH1cblxuZnVuY3Rpb24gcGFja0kxNihuKSB7IHJldHVybiBbKG4gPj4gOCkgJiAweGZmLCBuICYgMHhmZl07IH1cbmZ1bmN0aW9uIHVucGFja0kxNihieXRlcykgeyByZXR1cm4gYXNfc2lnbmVkKGJ5dGVzWzBdIDw8IDggfCBieXRlc1sxXSwgMTYpOyB9XG5cbmZ1bmN0aW9uIHBhY2tVMTYobikgeyByZXR1cm4gWyhuID4+IDgpICYgMHhmZiwgbiAmIDB4ZmZdOyB9XG5mdW5jdGlvbiB1bnBhY2tVMTYoYnl0ZXMpIHsgcmV0dXJuIGFzX3Vuc2lnbmVkKGJ5dGVzWzBdIDw8IDggfCBieXRlc1sxXSwgMTYpOyB9XG5cbmZ1bmN0aW9uIHBhY2tJMzIobikgeyByZXR1cm4gWyhuID4+IDI0KSAmIDB4ZmYsIChuID4+IDE2KSAmIDB4ZmYsIChuID4+IDgpICYgMHhmZiwgbiAmIDB4ZmZdOyB9XG5mdW5jdGlvbiB1bnBhY2tJMzIoYnl0ZXMpIHsgcmV0dXJuIGFzX3NpZ25lZChieXRlc1swXSA8PCAyNCB8IGJ5dGVzWzFdIDw8IDE2IHwgYnl0ZXNbMl0gPDwgOCB8IGJ5dGVzWzNdLCAzMik7IH1cblxuZnVuY3Rpb24gcGFja1UzMihuKSB7IHJldHVybiBbKG4gPj4gMjQpICYgMHhmZiwgKG4gPj4gMTYpICYgMHhmZiwgKG4gPj4gOCkgJiAweGZmLCBuICYgMHhmZl07IH1cbmZ1bmN0aW9uIHVucGFja1UzMihieXRlcykgeyByZXR1cm4gYXNfdW5zaWduZWQoYnl0ZXNbMF0gPDwgMjQgfCBieXRlc1sxXSA8PCAxNiB8IGJ5dGVzWzJdIDw8IDggfCBieXRlc1szXSwgMzIpOyB9XG5cbmZ1bmN0aW9uIHBhY2tJRUVFNzU0KHYsIGViaXRzLCBmYml0cykge1xuXG4gIHZhciBiaWFzID0gKDEgPDwgKGViaXRzIC0gMSkpIC0gMSxcbiAgICAgIHMsIGUsIGYsIGxuLFxuICAgICAgaSwgYml0cywgc3RyLCBieXRlcztcblxuICBmdW5jdGlvbiByb3VuZFRvRXZlbihuKSB7XG4gICAgdmFyIHcgPSBmbG9vcihuKSwgZiA9IG4gLSB3O1xuICAgIGlmIChmIDwgMC41KVxuICAgICAgcmV0dXJuIHc7XG4gICAgaWYgKGYgPiAwLjUpXG4gICAgICByZXR1cm4gdyArIDE7XG4gICAgcmV0dXJuIHcgJSAyID8gdyArIDEgOiB3O1xuICB9XG5cbiAgLy8gQ29tcHV0ZSBzaWduLCBleHBvbmVudCwgZnJhY3Rpb25cbiAgaWYgKHYgIT09IHYpIHtcbiAgICAvLyBOYU5cbiAgICAvLyBodHRwOi8vZGV2LnczLm9yZy8yMDA2L3dlYmFwaS9XZWJJREwvI2VzLXR5cGUtbWFwcGluZ1xuICAgIGUgPSAoMSA8PCBlYml0cykgLSAxOyBmID0gcG93KDIsIGZiaXRzIC0gMSk7IHMgPSAwO1xuICB9IGVsc2UgaWYgKHYgPT09IEluZmluaXR5IHx8IHYgPT09IC1JbmZpbml0eSkge1xuICAgIGUgPSAoMSA8PCBlYml0cykgLSAxOyBmID0gMDsgcyA9ICh2IDwgMCkgPyAxIDogMDtcbiAgfSBlbHNlIGlmICh2ID09PSAwKSB7XG4gICAgZSA9IDA7IGYgPSAwOyBzID0gKDEgLyB2ID09PSAtSW5maW5pdHkpID8gMSA6IDA7XG4gIH0gZWxzZSB7XG4gICAgcyA9IHYgPCAwO1xuICAgIHYgPSBhYnModik7XG5cbiAgICBpZiAodiA+PSBwb3coMiwgMSAtIGJpYXMpKSB7XG4gICAgICBlID0gbWluKGZsb29yKGxvZyh2KSAvIExOMiksIDEwMjMpO1xuICAgICAgZiA9IHJvdW5kVG9FdmVuKHYgLyBwb3coMiwgZSkgKiBwb3coMiwgZmJpdHMpKTtcbiAgICAgIGlmIChmIC8gcG93KDIsIGZiaXRzKSA+PSAyKSB7XG4gICAgICAgIGUgPSBlICsgMTtcbiAgICAgICAgZiA9IDE7XG4gICAgICB9XG4gICAgICBpZiAoZSA+IGJpYXMpIHtcbiAgICAgICAgLy8gT3ZlcmZsb3dcbiAgICAgICAgZSA9ICgxIDw8IGViaXRzKSAtIDE7XG4gICAgICAgIGYgPSAwO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgLy8gTm9ybWFsaXplZFxuICAgICAgICBlID0gZSArIGJpYXM7XG4gICAgICAgIGYgPSBmIC0gcG93KDIsIGZiaXRzKTtcbiAgICAgIH1cbiAgICB9IGVsc2Uge1xuICAgICAgLy8gRGVub3JtYWxpemVkXG4gICAgICBlID0gMDtcbiAgICAgIGYgPSByb3VuZFRvRXZlbih2IC8gcG93KDIsIDEgLSBiaWFzIC0gZmJpdHMpKTtcbiAgICB9XG4gIH1cblxuICAvLyBQYWNrIHNpZ24sIGV4cG9uZW50LCBmcmFjdGlvblxuICBiaXRzID0gW107XG4gIGZvciAoaSA9IGZiaXRzOyBpOyBpIC09IDEpIHsgYml0cy5wdXNoKGYgJSAyID8gMSA6IDApOyBmID0gZmxvb3IoZiAvIDIpOyB9XG4gIGZvciAoaSA9IGViaXRzOyBpOyBpIC09IDEpIHsgYml0cy5wdXNoKGUgJSAyID8gMSA6IDApOyBlID0gZmxvb3IoZSAvIDIpOyB9XG4gIGJpdHMucHVzaChzID8gMSA6IDApO1xuICBiaXRzLnJldmVyc2UoKTtcbiAgc3RyID0gYml0cy5qb2luKCcnKTtcblxuICAvLyBCaXRzIHRvIGJ5dGVzXG4gIGJ5dGVzID0gW107XG4gIHdoaWxlIChzdHIubGVuZ3RoKSB7XG4gICAgYnl0ZXMucHVzaChwYXJzZUludChzdHIuc3Vic3RyaW5nKDAsIDgpLCAyKSk7XG4gICAgc3RyID0gc3RyLnN1YnN0cmluZyg4KTtcbiAgfVxuICByZXR1cm4gYnl0ZXM7XG59XG5cbmZ1bmN0aW9uIHVucGFja0lFRUU3NTQoYnl0ZXMsIGViaXRzLCBmYml0cykge1xuXG4gIC8vIEJ5dGVzIHRvIGJpdHNcbiAgdmFyIGJpdHMgPSBbXSwgaSwgaiwgYiwgc3RyLFxuICAgICAgYmlhcywgcywgZSwgZjtcblxuICBmb3IgKGkgPSBieXRlcy5sZW5ndGg7IGk7IGkgLT0gMSkge1xuICAgIGIgPSBieXRlc1tpIC0gMV07XG4gICAgZm9yIChqID0gODsgajsgaiAtPSAxKSB7XG4gICAgICBiaXRzLnB1c2goYiAlIDIgPyAxIDogMCk7IGIgPSBiID4+IDE7XG4gICAgfVxuICB9XG4gIGJpdHMucmV2ZXJzZSgpO1xuICBzdHIgPSBiaXRzLmpvaW4oJycpO1xuXG4gIC8vIFVucGFjayBzaWduLCBleHBvbmVudCwgZnJhY3Rpb25cbiAgYmlhcyA9ICgxIDw8IChlYml0cyAtIDEpKSAtIDE7XG4gIHMgPSBwYXJzZUludChzdHIuc3Vic3RyaW5nKDAsIDEpLCAyKSA/IC0xIDogMTtcbiAgZSA9IHBhcnNlSW50KHN0ci5zdWJzdHJpbmcoMSwgMSArIGViaXRzKSwgMik7XG4gIGYgPSBwYXJzZUludChzdHIuc3Vic3RyaW5nKDEgKyBlYml0cyksIDIpO1xuXG4gIC8vIFByb2R1Y2UgbnVtYmVyXG4gIGlmIChlID09PSAoMSA8PCBlYml0cykgLSAxKSB7XG4gICAgcmV0dXJuIGYgIT09IDAgPyBOYU4gOiBzICogSW5maW5pdHk7XG4gIH0gZWxzZSBpZiAoZSA+IDApIHtcbiAgICAvLyBOb3JtYWxpemVkXG4gICAgcmV0dXJuIHMgKiBwb3coMiwgZSAtIGJpYXMpICogKDEgKyBmIC8gcG93KDIsIGZiaXRzKSk7XG4gIH0gZWxzZSBpZiAoZiAhPT0gMCkge1xuICAgIC8vIERlbm9ybWFsaXplZFxuICAgIHJldHVybiBzICogcG93KDIsIC0oYmlhcyAtIDEpKSAqIChmIC8gcG93KDIsIGZiaXRzKSk7XG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIHMgPCAwID8gLTAgOiAwO1xuICB9XG59XG5cbmZ1bmN0aW9uIHVucGFja0Y2NChiKSB7IHJldHVybiB1bnBhY2tJRUVFNzU0KGIsIDExLCA1Mik7IH1cbmZ1bmN0aW9uIHBhY2tGNjQodikgeyByZXR1cm4gcGFja0lFRUU3NTQodiwgMTEsIDUyKTsgfVxuZnVuY3Rpb24gdW5wYWNrRjMyKGIpIHsgcmV0dXJuIHVucGFja0lFRUU3NTQoYiwgOCwgMjMpOyB9XG5mdW5jdGlvbiBwYWNrRjMyKHYpIHsgcmV0dXJuIHBhY2tJRUVFNzU0KHYsIDgsIDIzKTsgfVxuXG5cbi8vXG4vLyAzIFRoZSBBcnJheUJ1ZmZlciBUeXBlXG4vL1xuXG4oZnVuY3Rpb24oKSB7XG5cbiAgLyoqIEBjb25zdHJ1Y3RvciAqL1xuICB2YXIgQXJyYXlCdWZmZXIgPSBmdW5jdGlvbiBBcnJheUJ1ZmZlcihsZW5ndGgpIHtcbiAgICBsZW5ndGggPSBFQ01BU2NyaXB0LlRvSW50MzIobGVuZ3RoKTtcbiAgICBpZiAobGVuZ3RoIDwgMCkgdGhyb3cgbmV3IFJhbmdlRXJyb3IoJ0FycmF5QnVmZmVyIHNpemUgaXMgbm90IGEgc21hbGwgZW5vdWdoIHBvc2l0aXZlIGludGVnZXInKTtcblxuICAgIHRoaXMuYnl0ZUxlbmd0aCA9IGxlbmd0aDtcbiAgICB0aGlzLl9ieXRlcyA9IFtdO1xuICAgIHRoaXMuX2J5dGVzLmxlbmd0aCA9IGxlbmd0aDtcblxuICAgIHZhciBpO1xuICAgIGZvciAoaSA9IDA7IGkgPCB0aGlzLmJ5dGVMZW5ndGg7IGkgKz0gMSkge1xuICAgICAgdGhpcy5fYnl0ZXNbaV0gPSAwO1xuICAgIH1cblxuICAgIGNvbmZpZ3VyZVByb3BlcnRpZXModGhpcyk7XG4gIH07XG5cbiAgZXhwb3J0cy5BcnJheUJ1ZmZlciA9IGV4cG9ydHMuQXJyYXlCdWZmZXIgfHwgQXJyYXlCdWZmZXI7XG5cbiAgLy9cbiAgLy8gNCBUaGUgQXJyYXlCdWZmZXJWaWV3IFR5cGVcbiAgLy9cblxuICAvLyBOT1RFOiB0aGlzIGNvbnN0cnVjdG9yIGlzIG5vdCBleHBvcnRlZFxuICAvKiogQGNvbnN0cnVjdG9yICovXG4gIHZhciBBcnJheUJ1ZmZlclZpZXcgPSBmdW5jdGlvbiBBcnJheUJ1ZmZlclZpZXcoKSB7XG4gICAgLy90aGlzLmJ1ZmZlciA9IG51bGw7XG4gICAgLy90aGlzLmJ5dGVPZmZzZXQgPSAwO1xuICAgIC8vdGhpcy5ieXRlTGVuZ3RoID0gMDtcbiAgfTtcblxuICAvL1xuICAvLyA1IFRoZSBUeXBlZCBBcnJheSBWaWV3IFR5cGVzXG4gIC8vXG5cbiAgZnVuY3Rpb24gbWFrZUNvbnN0cnVjdG9yKGJ5dGVzUGVyRWxlbWVudCwgcGFjaywgdW5wYWNrKSB7XG4gICAgLy8gRWFjaCBUeXBlZEFycmF5IHR5cGUgcmVxdWlyZXMgYSBkaXN0aW5jdCBjb25zdHJ1Y3RvciBpbnN0YW5jZSB3aXRoXG4gICAgLy8gaWRlbnRpY2FsIGxvZ2ljLCB3aGljaCB0aGlzIHByb2R1Y2VzLlxuXG4gICAgdmFyIGN0b3I7XG4gICAgY3RvciA9IGZ1bmN0aW9uKGJ1ZmZlciwgYnl0ZU9mZnNldCwgbGVuZ3RoKSB7XG4gICAgICB2YXIgYXJyYXksIHNlcXVlbmNlLCBpLCBzO1xuXG4gICAgICBpZiAoIWFyZ3VtZW50cy5sZW5ndGggfHwgdHlwZW9mIGFyZ3VtZW50c1swXSA9PT0gJ251bWJlcicpIHtcbiAgICAgICAgLy8gQ29uc3RydWN0b3IodW5zaWduZWQgbG9uZyBsZW5ndGgpXG4gICAgICAgIHRoaXMubGVuZ3RoID0gRUNNQVNjcmlwdC5Ub0ludDMyKGFyZ3VtZW50c1swXSk7XG4gICAgICAgIGlmIChsZW5ndGggPCAwKSB0aHJvdyBuZXcgUmFuZ2VFcnJvcignQXJyYXlCdWZmZXJWaWV3IHNpemUgaXMgbm90IGEgc21hbGwgZW5vdWdoIHBvc2l0aXZlIGludGVnZXInKTtcblxuICAgICAgICB0aGlzLmJ5dGVMZW5ndGggPSB0aGlzLmxlbmd0aCAqIHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG4gICAgICAgIHRoaXMuYnVmZmVyID0gbmV3IEFycmF5QnVmZmVyKHRoaXMuYnl0ZUxlbmd0aCk7XG4gICAgICAgIHRoaXMuYnl0ZU9mZnNldCA9IDA7XG4gICAgICB9IGVsc2UgaWYgKHR5cGVvZiBhcmd1bWVudHNbMF0gPT09ICdvYmplY3QnICYmIGFyZ3VtZW50c1swXS5jb25zdHJ1Y3RvciA9PT0gY3Rvcikge1xuICAgICAgICAvLyBDb25zdHJ1Y3RvcihUeXBlZEFycmF5IGFycmF5KVxuICAgICAgICBhcnJheSA9IGFyZ3VtZW50c1swXTtcblxuICAgICAgICB0aGlzLmxlbmd0aCA9IGFycmF5Lmxlbmd0aDtcbiAgICAgICAgdGhpcy5ieXRlTGVuZ3RoID0gdGhpcy5sZW5ndGggKiB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UO1xuICAgICAgICB0aGlzLmJ1ZmZlciA9IG5ldyBBcnJheUJ1ZmZlcih0aGlzLmJ5dGVMZW5ndGgpO1xuICAgICAgICB0aGlzLmJ5dGVPZmZzZXQgPSAwO1xuXG4gICAgICAgIGZvciAoaSA9IDA7IGkgPCB0aGlzLmxlbmd0aDsgaSArPSAxKSB7XG4gICAgICAgICAgdGhpcy5fc2V0dGVyKGksIGFycmF5Ll9nZXR0ZXIoaSkpO1xuICAgICAgICB9XG4gICAgICB9IGVsc2UgaWYgKHR5cGVvZiBhcmd1bWVudHNbMF0gPT09ICdvYmplY3QnICYmXG4gICAgICAgICAgICAgICAgICEoYXJndW1lbnRzWzBdIGluc3RhbmNlb2YgQXJyYXlCdWZmZXIgfHwgRUNNQVNjcmlwdC5DbGFzcyhhcmd1bWVudHNbMF0pID09PSAnQXJyYXlCdWZmZXInKSkge1xuICAgICAgICAvLyBDb25zdHJ1Y3RvcihzZXF1ZW5jZTx0eXBlPiBhcnJheSlcbiAgICAgICAgc2VxdWVuY2UgPSBhcmd1bWVudHNbMF07XG5cbiAgICAgICAgdGhpcy5sZW5ndGggPSBFQ01BU2NyaXB0LlRvVWludDMyKHNlcXVlbmNlLmxlbmd0aCk7XG4gICAgICAgIHRoaXMuYnl0ZUxlbmd0aCA9IHRoaXMubGVuZ3RoICogdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgdGhpcy5idWZmZXIgPSBuZXcgQXJyYXlCdWZmZXIodGhpcy5ieXRlTGVuZ3RoKTtcbiAgICAgICAgdGhpcy5ieXRlT2Zmc2V0ID0gMDtcblxuICAgICAgICBmb3IgKGkgPSAwOyBpIDwgdGhpcy5sZW5ndGg7IGkgKz0gMSkge1xuICAgICAgICAgIHMgPSBzZXF1ZW5jZVtpXTtcbiAgICAgICAgICB0aGlzLl9zZXR0ZXIoaSwgTnVtYmVyKHMpKTtcbiAgICAgICAgfVxuICAgICAgfSBlbHNlIGlmICh0eXBlb2YgYXJndW1lbnRzWzBdID09PSAnb2JqZWN0JyAmJlxuICAgICAgICAgICAgICAgICAoYXJndW1lbnRzWzBdIGluc3RhbmNlb2YgQXJyYXlCdWZmZXIgfHwgRUNNQVNjcmlwdC5DbGFzcyhhcmd1bWVudHNbMF0pID09PSAnQXJyYXlCdWZmZXInKSkge1xuICAgICAgICAvLyBDb25zdHJ1Y3RvcihBcnJheUJ1ZmZlciBidWZmZXIsXG4gICAgICAgIC8vICAgICAgICAgICAgIG9wdGlvbmFsIHVuc2lnbmVkIGxvbmcgYnl0ZU9mZnNldCwgb3B0aW9uYWwgdW5zaWduZWQgbG9uZyBsZW5ndGgpXG4gICAgICAgIHRoaXMuYnVmZmVyID0gYnVmZmVyO1xuXG4gICAgICAgIHRoaXMuYnl0ZU9mZnNldCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoYnl0ZU9mZnNldCk7XG4gICAgICAgIGlmICh0aGlzLmJ5dGVPZmZzZXQgPiB0aGlzLmJ1ZmZlci5ieXRlTGVuZ3RoKSB7XG4gICAgICAgICAgdGhyb3cgbmV3IFJhbmdlRXJyb3IoXCJieXRlT2Zmc2V0IG91dCBvZiByYW5nZVwiKTtcbiAgICAgICAgfVxuXG4gICAgICAgIGlmICh0aGlzLmJ5dGVPZmZzZXQgJSB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UKSB7XG4gICAgICAgICAgLy8gVGhlIGdpdmVuIGJ5dGVPZmZzZXQgbXVzdCBiZSBhIG11bHRpcGxlIG9mIHRoZSBlbGVtZW50XG4gICAgICAgICAgLy8gc2l6ZSBvZiB0aGUgc3BlY2lmaWMgdHlwZSwgb3RoZXJ3aXNlIGFuIGV4Y2VwdGlvbiBpcyByYWlzZWQuXG4gICAgICAgICAgdGhyb3cgbmV3IFJhbmdlRXJyb3IoXCJBcnJheUJ1ZmZlciBsZW5ndGggbWludXMgdGhlIGJ5dGVPZmZzZXQgaXMgbm90IGEgbXVsdGlwbGUgb2YgdGhlIGVsZW1lbnQgc2l6ZS5cIik7XG4gICAgICAgIH1cblxuICAgICAgICBpZiAoYXJndW1lbnRzLmxlbmd0aCA8IDMpIHtcbiAgICAgICAgICB0aGlzLmJ5dGVMZW5ndGggPSB0aGlzLmJ1ZmZlci5ieXRlTGVuZ3RoIC0gdGhpcy5ieXRlT2Zmc2V0O1xuXG4gICAgICAgICAgaWYgKHRoaXMuYnl0ZUxlbmd0aCAlIHRoaXMuQllURVNfUEVSX0VMRU1FTlQpIHtcbiAgICAgICAgICAgIHRocm93IG5ldyBSYW5nZUVycm9yKFwibGVuZ3RoIG9mIGJ1ZmZlciBtaW51cyBieXRlT2Zmc2V0IG5vdCBhIG11bHRpcGxlIG9mIHRoZSBlbGVtZW50IHNpemVcIik7XG4gICAgICAgICAgfVxuICAgICAgICAgIHRoaXMubGVuZ3RoID0gdGhpcy5ieXRlTGVuZ3RoIC8gdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICB0aGlzLmxlbmd0aCA9IEVDTUFTY3JpcHQuVG9VaW50MzIobGVuZ3RoKTtcbiAgICAgICAgICB0aGlzLmJ5dGVMZW5ndGggPSB0aGlzLmxlbmd0aCAqIHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG4gICAgICAgIH1cblxuICAgICAgICBpZiAoKHRoaXMuYnl0ZU9mZnNldCArIHRoaXMuYnl0ZUxlbmd0aCkgPiB0aGlzLmJ1ZmZlci5ieXRlTGVuZ3RoKSB7XG4gICAgICAgICAgdGhyb3cgbmV3IFJhbmdlRXJyb3IoXCJieXRlT2Zmc2V0IGFuZCBsZW5ndGggcmVmZXJlbmNlIGFuIGFyZWEgYmV5b25kIHRoZSBlbmQgb2YgdGhlIGJ1ZmZlclwiKTtcbiAgICAgICAgfVxuICAgICAgfSBlbHNlIHtcbiAgICAgICAgdGhyb3cgbmV3IFR5cGVFcnJvcihcIlVuZXhwZWN0ZWQgYXJndW1lbnQgdHlwZShzKVwiKTtcbiAgICAgIH1cblxuICAgICAgdGhpcy5jb25zdHJ1Y3RvciA9IGN0b3I7XG5cbiAgICAgIGNvbmZpZ3VyZVByb3BlcnRpZXModGhpcyk7XG4gICAgICBtYWtlQXJyYXlBY2Nlc3NvcnModGhpcyk7XG4gICAgfTtcblxuICAgIGN0b3IucHJvdG90eXBlID0gbmV3IEFycmF5QnVmZmVyVmlldygpO1xuICAgIGN0b3IucHJvdG90eXBlLkJZVEVTX1BFUl9FTEVNRU5UID0gYnl0ZXNQZXJFbGVtZW50O1xuICAgIGN0b3IucHJvdG90eXBlLl9wYWNrID0gcGFjaztcbiAgICBjdG9yLnByb3RvdHlwZS5fdW5wYWNrID0gdW5wYWNrO1xuICAgIGN0b3IuQllURVNfUEVSX0VMRU1FTlQgPSBieXRlc1BlckVsZW1lbnQ7XG5cbiAgICAvLyBnZXR0ZXIgdHlwZSAodW5zaWduZWQgbG9uZyBpbmRleCk7XG4gICAgY3Rvci5wcm90b3R5cGUuX2dldHRlciA9IGZ1bmN0aW9uKGluZGV4KSB7XG4gICAgICBpZiAoYXJndW1lbnRzLmxlbmd0aCA8IDEpIHRocm93IG5ldyBTeW50YXhFcnJvcihcIk5vdCBlbm91Z2ggYXJndW1lbnRzXCIpO1xuXG4gICAgICBpbmRleCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoaW5kZXgpO1xuICAgICAgaWYgKGluZGV4ID49IHRoaXMubGVuZ3RoKSB7XG4gICAgICAgIHJldHVybiB1bmRlZmluZWQ7XG4gICAgICB9XG5cbiAgICAgIHZhciBieXRlcyA9IFtdLCBpLCBvO1xuICAgICAgZm9yIChpID0gMCwgbyA9IHRoaXMuYnl0ZU9mZnNldCArIGluZGV4ICogdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgICAgaSA8IHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG4gICAgICAgICAgIGkgKz0gMSwgbyArPSAxKSB7XG4gICAgICAgIGJ5dGVzLnB1c2godGhpcy5idWZmZXIuX2J5dGVzW29dKTtcbiAgICAgIH1cbiAgICAgIHJldHVybiB0aGlzLl91bnBhY2soYnl0ZXMpO1xuICAgIH07XG5cbiAgICAvLyBOT05TVEFOREFSRDogY29udmVuaWVuY2UgYWxpYXMgZm9yIGdldHRlcjogdHlwZSBnZXQodW5zaWduZWQgbG9uZyBpbmRleCk7XG4gICAgY3Rvci5wcm90b3R5cGUuZ2V0ID0gY3Rvci5wcm90b3R5cGUuX2dldHRlcjtcblxuICAgIC8vIHNldHRlciB2b2lkICh1bnNpZ25lZCBsb25nIGluZGV4LCB0eXBlIHZhbHVlKTtcbiAgICBjdG9yLnByb3RvdHlwZS5fc2V0dGVyID0gZnVuY3Rpb24oaW5kZXgsIHZhbHVlKSB7XG4gICAgICBpZiAoYXJndW1lbnRzLmxlbmd0aCA8IDIpIHRocm93IG5ldyBTeW50YXhFcnJvcihcIk5vdCBlbm91Z2ggYXJndW1lbnRzXCIpO1xuXG4gICAgICBpbmRleCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoaW5kZXgpO1xuICAgICAgaWYgKGluZGV4ID49IHRoaXMubGVuZ3RoKSB7XG4gICAgICAgIHJldHVybiB1bmRlZmluZWQ7XG4gICAgICB9XG5cbiAgICAgIHZhciBieXRlcyA9IHRoaXMuX3BhY2sodmFsdWUpLCBpLCBvO1xuICAgICAgZm9yIChpID0gMCwgbyA9IHRoaXMuYnl0ZU9mZnNldCArIGluZGV4ICogdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgICAgaSA8IHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG4gICAgICAgICAgIGkgKz0gMSwgbyArPSAxKSB7XG4gICAgICAgIHRoaXMuYnVmZmVyLl9ieXRlc1tvXSA9IGJ5dGVzW2ldO1xuICAgICAgfVxuICAgIH07XG5cbiAgICAvLyB2b2lkIHNldChUeXBlZEFycmF5IGFycmF5LCBvcHRpb25hbCB1bnNpZ25lZCBsb25nIG9mZnNldCk7XG4gICAgLy8gdm9pZCBzZXQoc2VxdWVuY2U8dHlwZT4gYXJyYXksIG9wdGlvbmFsIHVuc2lnbmVkIGxvbmcgb2Zmc2V0KTtcbiAgICBjdG9yLnByb3RvdHlwZS5zZXQgPSBmdW5jdGlvbihpbmRleCwgdmFsdWUpIHtcbiAgICAgIGlmIChhcmd1bWVudHMubGVuZ3RoIDwgMSkgdGhyb3cgbmV3IFN5bnRheEVycm9yKFwiTm90IGVub3VnaCBhcmd1bWVudHNcIik7XG4gICAgICB2YXIgYXJyYXksIHNlcXVlbmNlLCBvZmZzZXQsIGxlbixcbiAgICAgICAgICBpLCBzLCBkLFxuICAgICAgICAgIGJ5dGVPZmZzZXQsIGJ5dGVMZW5ndGgsIHRtcDtcblxuICAgICAgaWYgKHR5cGVvZiBhcmd1bWVudHNbMF0gPT09ICdvYmplY3QnICYmIGFyZ3VtZW50c1swXS5jb25zdHJ1Y3RvciA9PT0gdGhpcy5jb25zdHJ1Y3Rvcikge1xuICAgICAgICAvLyB2b2lkIHNldChUeXBlZEFycmF5IGFycmF5LCBvcHRpb25hbCB1bnNpZ25lZCBsb25nIG9mZnNldCk7XG4gICAgICAgIGFycmF5ID0gYXJndW1lbnRzWzBdO1xuICAgICAgICBvZmZzZXQgPSBFQ01BU2NyaXB0LlRvVWludDMyKGFyZ3VtZW50c1sxXSk7XG5cbiAgICAgICAgaWYgKG9mZnNldCArIGFycmF5Lmxlbmd0aCA+IHRoaXMubGVuZ3RoKSB7XG4gICAgICAgICAgdGhyb3cgbmV3IFJhbmdlRXJyb3IoXCJPZmZzZXQgcGx1cyBsZW5ndGggb2YgYXJyYXkgaXMgb3V0IG9mIHJhbmdlXCIpO1xuICAgICAgICB9XG5cbiAgICAgICAgYnl0ZU9mZnNldCA9IHRoaXMuYnl0ZU9mZnNldCArIG9mZnNldCAqIHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG4gICAgICAgIGJ5dGVMZW5ndGggPSBhcnJheS5sZW5ndGggKiB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UO1xuXG4gICAgICAgIGlmIChhcnJheS5idWZmZXIgPT09IHRoaXMuYnVmZmVyKSB7XG4gICAgICAgICAgdG1wID0gW107XG4gICAgICAgICAgZm9yIChpID0gMCwgcyA9IGFycmF5LmJ5dGVPZmZzZXQ7IGkgPCBieXRlTGVuZ3RoOyBpICs9IDEsIHMgKz0gMSkge1xuICAgICAgICAgICAgdG1wW2ldID0gYXJyYXkuYnVmZmVyLl9ieXRlc1tzXTtcbiAgICAgICAgICB9XG4gICAgICAgICAgZm9yIChpID0gMCwgZCA9IGJ5dGVPZmZzZXQ7IGkgPCBieXRlTGVuZ3RoOyBpICs9IDEsIGQgKz0gMSkge1xuICAgICAgICAgICAgdGhpcy5idWZmZXIuX2J5dGVzW2RdID0gdG1wW2ldO1xuICAgICAgICAgIH1cbiAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICBmb3IgKGkgPSAwLCBzID0gYXJyYXkuYnl0ZU9mZnNldCwgZCA9IGJ5dGVPZmZzZXQ7XG4gICAgICAgICAgICAgICBpIDwgYnl0ZUxlbmd0aDsgaSArPSAxLCBzICs9IDEsIGQgKz0gMSkge1xuICAgICAgICAgICAgdGhpcy5idWZmZXIuX2J5dGVzW2RdID0gYXJyYXkuYnVmZmVyLl9ieXRlc1tzXTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSBpZiAodHlwZW9mIGFyZ3VtZW50c1swXSA9PT0gJ29iamVjdCcgJiYgdHlwZW9mIGFyZ3VtZW50c1swXS5sZW5ndGggIT09ICd1bmRlZmluZWQnKSB7XG4gICAgICAgIC8vIHZvaWQgc2V0KHNlcXVlbmNlPHR5cGU+IGFycmF5LCBvcHRpb25hbCB1bnNpZ25lZCBsb25nIG9mZnNldCk7XG4gICAgICAgIHNlcXVlbmNlID0gYXJndW1lbnRzWzBdO1xuICAgICAgICBsZW4gPSBFQ01BU2NyaXB0LlRvVWludDMyKHNlcXVlbmNlLmxlbmd0aCk7XG4gICAgICAgIG9mZnNldCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoYXJndW1lbnRzWzFdKTtcblxuICAgICAgICBpZiAob2Zmc2V0ICsgbGVuID4gdGhpcy5sZW5ndGgpIHtcbiAgICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcIk9mZnNldCBwbHVzIGxlbmd0aCBvZiBhcnJheSBpcyBvdXQgb2YgcmFuZ2VcIik7XG4gICAgICAgIH1cblxuICAgICAgICBmb3IgKGkgPSAwOyBpIDwgbGVuOyBpICs9IDEpIHtcbiAgICAgICAgICBzID0gc2VxdWVuY2VbaV07XG4gICAgICAgICAgdGhpcy5fc2V0dGVyKG9mZnNldCArIGksIE51bWJlcihzKSk7XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIHRocm93IG5ldyBUeXBlRXJyb3IoXCJVbmV4cGVjdGVkIGFyZ3VtZW50IHR5cGUocylcIik7XG4gICAgICB9XG4gICAgfTtcblxuICAgIC8vIFR5cGVkQXJyYXkgc3ViYXJyYXkobG9uZyBiZWdpbiwgb3B0aW9uYWwgbG9uZyBlbmQpO1xuICAgIGN0b3IucHJvdG90eXBlLnN1YmFycmF5ID0gZnVuY3Rpb24oc3RhcnQsIGVuZCkge1xuICAgICAgZnVuY3Rpb24gY2xhbXAodiwgbWluLCBtYXgpIHsgcmV0dXJuIHYgPCBtaW4gPyBtaW4gOiB2ID4gbWF4ID8gbWF4IDogdjsgfVxuXG4gICAgICBzdGFydCA9IEVDTUFTY3JpcHQuVG9JbnQzMihzdGFydCk7XG4gICAgICBlbmQgPSBFQ01BU2NyaXB0LlRvSW50MzIoZW5kKTtcblxuICAgICAgaWYgKGFyZ3VtZW50cy5sZW5ndGggPCAxKSB7IHN0YXJ0ID0gMDsgfVxuICAgICAgaWYgKGFyZ3VtZW50cy5sZW5ndGggPCAyKSB7IGVuZCA9IHRoaXMubGVuZ3RoOyB9XG5cbiAgICAgIGlmIChzdGFydCA8IDApIHsgc3RhcnQgPSB0aGlzLmxlbmd0aCArIHN0YXJ0OyB9XG4gICAgICBpZiAoZW5kIDwgMCkgeyBlbmQgPSB0aGlzLmxlbmd0aCArIGVuZDsgfVxuXG4gICAgICBzdGFydCA9IGNsYW1wKHN0YXJ0LCAwLCB0aGlzLmxlbmd0aCk7XG4gICAgICBlbmQgPSBjbGFtcChlbmQsIDAsIHRoaXMubGVuZ3RoKTtcblxuICAgICAgdmFyIGxlbiA9IGVuZCAtIHN0YXJ0O1xuICAgICAgaWYgKGxlbiA8IDApIHtcbiAgICAgICAgbGVuID0gMDtcbiAgICAgIH1cblxuICAgICAgcmV0dXJuIG5ldyB0aGlzLmNvbnN0cnVjdG9yKFxuICAgICAgICB0aGlzLmJ1ZmZlciwgdGhpcy5ieXRlT2Zmc2V0ICsgc3RhcnQgKiB0aGlzLkJZVEVTX1BFUl9FTEVNRU5ULCBsZW4pO1xuICAgIH07XG5cbiAgICByZXR1cm4gY3RvcjtcbiAgfVxuXG4gIHZhciBJbnQ4QXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoMSwgcGFja0k4LCB1bnBhY2tJOCk7XG4gIHZhciBVaW50OEFycmF5ID0gbWFrZUNvbnN0cnVjdG9yKDEsIHBhY2tVOCwgdW5wYWNrVTgpO1xuICB2YXIgVWludDhDbGFtcGVkQXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoMSwgcGFja1U4Q2xhbXBlZCwgdW5wYWNrVTgpO1xuICB2YXIgSW50MTZBcnJheSA9IG1ha2VDb25zdHJ1Y3RvcigyLCBwYWNrSTE2LCB1bnBhY2tJMTYpO1xuICB2YXIgVWludDE2QXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoMiwgcGFja1UxNiwgdW5wYWNrVTE2KTtcbiAgdmFyIEludDMyQXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoNCwgcGFja0kzMiwgdW5wYWNrSTMyKTtcbiAgdmFyIFVpbnQzMkFycmF5ID0gbWFrZUNvbnN0cnVjdG9yKDQsIHBhY2tVMzIsIHVucGFja1UzMik7XG4gIHZhciBGbG9hdDMyQXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoNCwgcGFja0YzMiwgdW5wYWNrRjMyKTtcbiAgdmFyIEZsb2F0NjRBcnJheSA9IG1ha2VDb25zdHJ1Y3Rvcig4LCBwYWNrRjY0LCB1bnBhY2tGNjQpO1xuXG4gIGV4cG9ydHMuSW50OEFycmF5ID0gZXhwb3J0cy5JbnQ4QXJyYXkgfHwgSW50OEFycmF5O1xuICBleHBvcnRzLlVpbnQ4QXJyYXkgPSBleHBvcnRzLlVpbnQ4QXJyYXkgfHwgVWludDhBcnJheTtcbiAgZXhwb3J0cy5VaW50OENsYW1wZWRBcnJheSA9IGV4cG9ydHMuVWludDhDbGFtcGVkQXJyYXkgfHwgVWludDhDbGFtcGVkQXJyYXk7XG4gIGV4cG9ydHMuSW50MTZBcnJheSA9IGV4cG9ydHMuSW50MTZBcnJheSB8fCBJbnQxNkFycmF5O1xuICBleHBvcnRzLlVpbnQxNkFycmF5ID0gZXhwb3J0cy5VaW50MTZBcnJheSB8fCBVaW50MTZBcnJheTtcbiAgZXhwb3J0cy5JbnQzMkFycmF5ID0gZXhwb3J0cy5JbnQzMkFycmF5IHx8IEludDMyQXJyYXk7XG4gIGV4cG9ydHMuVWludDMyQXJyYXkgPSBleHBvcnRzLlVpbnQzMkFycmF5IHx8IFVpbnQzMkFycmF5O1xuICBleHBvcnRzLkZsb2F0MzJBcnJheSA9IGV4cG9ydHMuRmxvYXQzMkFycmF5IHx8IEZsb2F0MzJBcnJheTtcbiAgZXhwb3J0cy5GbG9hdDY0QXJyYXkgPSBleHBvcnRzLkZsb2F0NjRBcnJheSB8fCBGbG9hdDY0QXJyYXk7XG59KCkpO1xuXG4vL1xuLy8gNiBUaGUgRGF0YVZpZXcgVmlldyBUeXBlXG4vL1xuXG4oZnVuY3Rpb24oKSB7XG4gIGZ1bmN0aW9uIHIoYXJyYXksIGluZGV4KSB7XG4gICAgcmV0dXJuIEVDTUFTY3JpcHQuSXNDYWxsYWJsZShhcnJheS5nZXQpID8gYXJyYXkuZ2V0KGluZGV4KSA6IGFycmF5W2luZGV4XTtcbiAgfVxuXG4gIHZhciBJU19CSUdfRU5ESUFOID0gKGZ1bmN0aW9uKCkge1xuICAgIHZhciB1MTZhcnJheSA9IG5ldyhleHBvcnRzLlVpbnQxNkFycmF5KShbMHgxMjM0XSksXG4gICAgICAgIHU4YXJyYXkgPSBuZXcoZXhwb3J0cy5VaW50OEFycmF5KSh1MTZhcnJheS5idWZmZXIpO1xuICAgIHJldHVybiByKHU4YXJyYXksIDApID09PSAweDEyO1xuICB9KCkpO1xuXG4gIC8vIENvbnN0cnVjdG9yKEFycmF5QnVmZmVyIGJ1ZmZlcixcbiAgLy8gICAgICAgICAgICAgb3B0aW9uYWwgdW5zaWduZWQgbG9uZyBieXRlT2Zmc2V0LFxuICAvLyAgICAgICAgICAgICBvcHRpb25hbCB1bnNpZ25lZCBsb25nIGJ5dGVMZW5ndGgpXG4gIC8qKiBAY29uc3RydWN0b3IgKi9cbiAgdmFyIERhdGFWaWV3ID0gZnVuY3Rpb24gRGF0YVZpZXcoYnVmZmVyLCBieXRlT2Zmc2V0LCBieXRlTGVuZ3RoKSB7XG4gICAgaWYgKGFyZ3VtZW50cy5sZW5ndGggPT09IDApIHtcbiAgICAgIGJ1ZmZlciA9IG5ldyBleHBvcnRzLkFycmF5QnVmZmVyKDApO1xuICAgIH0gZWxzZSBpZiAoIShidWZmZXIgaW5zdGFuY2VvZiBleHBvcnRzLkFycmF5QnVmZmVyIHx8IEVDTUFTY3JpcHQuQ2xhc3MoYnVmZmVyKSA9PT0gJ0FycmF5QnVmZmVyJykpIHtcbiAgICAgIHRocm93IG5ldyBUeXBlRXJyb3IoXCJUeXBlRXJyb3JcIik7XG4gICAgfVxuXG4gICAgdGhpcy5idWZmZXIgPSBidWZmZXIgfHwgbmV3IGV4cG9ydHMuQXJyYXlCdWZmZXIoMCk7XG5cbiAgICB0aGlzLmJ5dGVPZmZzZXQgPSBFQ01BU2NyaXB0LlRvVWludDMyKGJ5dGVPZmZzZXQpO1xuICAgIGlmICh0aGlzLmJ5dGVPZmZzZXQgPiB0aGlzLmJ1ZmZlci5ieXRlTGVuZ3RoKSB7XG4gICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcImJ5dGVPZmZzZXQgb3V0IG9mIHJhbmdlXCIpO1xuICAgIH1cblxuICAgIGlmIChhcmd1bWVudHMubGVuZ3RoIDwgMykge1xuICAgICAgdGhpcy5ieXRlTGVuZ3RoID0gdGhpcy5idWZmZXIuYnl0ZUxlbmd0aCAtIHRoaXMuYnl0ZU9mZnNldDtcbiAgICB9IGVsc2Uge1xuICAgICAgdGhpcy5ieXRlTGVuZ3RoID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihieXRlTGVuZ3RoKTtcbiAgICB9XG5cbiAgICBpZiAoKHRoaXMuYnl0ZU9mZnNldCArIHRoaXMuYnl0ZUxlbmd0aCkgPiB0aGlzLmJ1ZmZlci5ieXRlTGVuZ3RoKSB7XG4gICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcImJ5dGVPZmZzZXQgYW5kIGxlbmd0aCByZWZlcmVuY2UgYW4gYXJlYSBiZXlvbmQgdGhlIGVuZCBvZiB0aGUgYnVmZmVyXCIpO1xuICAgIH1cblxuICAgIGNvbmZpZ3VyZVByb3BlcnRpZXModGhpcyk7XG4gIH07XG5cbiAgZnVuY3Rpb24gbWFrZUdldHRlcihhcnJheVR5cGUpIHtcbiAgICByZXR1cm4gZnVuY3Rpb24oYnl0ZU9mZnNldCwgbGl0dGxlRW5kaWFuKSB7XG5cbiAgICAgIGJ5dGVPZmZzZXQgPSBFQ01BU2NyaXB0LlRvVWludDMyKGJ5dGVPZmZzZXQpO1xuXG4gICAgICBpZiAoYnl0ZU9mZnNldCArIGFycmF5VHlwZS5CWVRFU19QRVJfRUxFTUVOVCA+IHRoaXMuYnl0ZUxlbmd0aCkge1xuICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcIkFycmF5IGluZGV4IG91dCBvZiByYW5nZVwiKTtcbiAgICAgIH1cbiAgICAgIGJ5dGVPZmZzZXQgKz0gdGhpcy5ieXRlT2Zmc2V0O1xuXG4gICAgICB2YXIgdWludDhBcnJheSA9IG5ldyBleHBvcnRzLlVpbnQ4QXJyYXkodGhpcy5idWZmZXIsIGJ5dGVPZmZzZXQsIGFycmF5VHlwZS5CWVRFU19QRVJfRUxFTUVOVCksXG4gICAgICAgICAgYnl0ZXMgPSBbXSwgaTtcbiAgICAgIGZvciAoaSA9IDA7IGkgPCBhcnJheVR5cGUuQllURVNfUEVSX0VMRU1FTlQ7IGkgKz0gMSkge1xuICAgICAgICBieXRlcy5wdXNoKHIodWludDhBcnJheSwgaSkpO1xuICAgICAgfVxuXG4gICAgICBpZiAoQm9vbGVhbihsaXR0bGVFbmRpYW4pID09PSBCb29sZWFuKElTX0JJR19FTkRJQU4pKSB7XG4gICAgICAgIGJ5dGVzLnJldmVyc2UoKTtcbiAgICAgIH1cblxuICAgICAgcmV0dXJuIHIobmV3IGFycmF5VHlwZShuZXcgZXhwb3J0cy5VaW50OEFycmF5KGJ5dGVzKS5idWZmZXIpLCAwKTtcbiAgICB9O1xuICB9XG5cbiAgRGF0YVZpZXcucHJvdG90eXBlLmdldFVpbnQ4ID0gbWFrZUdldHRlcihleHBvcnRzLlVpbnQ4QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0SW50OCA9IG1ha2VHZXR0ZXIoZXhwb3J0cy5JbnQ4QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0VWludDE2ID0gbWFrZUdldHRlcihleHBvcnRzLlVpbnQxNkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLmdldEludDE2ID0gbWFrZUdldHRlcihleHBvcnRzLkludDE2QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0VWludDMyID0gbWFrZUdldHRlcihleHBvcnRzLlVpbnQzMkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLmdldEludDMyID0gbWFrZUdldHRlcihleHBvcnRzLkludDMyQXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0RmxvYXQzMiA9IG1ha2VHZXR0ZXIoZXhwb3J0cy5GbG9hdDMyQXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0RmxvYXQ2NCA9IG1ha2VHZXR0ZXIoZXhwb3J0cy5GbG9hdDY0QXJyYXkpO1xuXG4gIGZ1bmN0aW9uIG1ha2VTZXR0ZXIoYXJyYXlUeXBlKSB7XG4gICAgcmV0dXJuIGZ1bmN0aW9uKGJ5dGVPZmZzZXQsIHZhbHVlLCBsaXR0bGVFbmRpYW4pIHtcblxuICAgICAgYnl0ZU9mZnNldCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoYnl0ZU9mZnNldCk7XG4gICAgICBpZiAoYnl0ZU9mZnNldCArIGFycmF5VHlwZS5CWVRFU19QRVJfRUxFTUVOVCA+IHRoaXMuYnl0ZUxlbmd0aCkge1xuICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcIkFycmF5IGluZGV4IG91dCBvZiByYW5nZVwiKTtcbiAgICAgIH1cblxuICAgICAgLy8gR2V0IGJ5dGVzXG4gICAgICB2YXIgdHlwZUFycmF5ID0gbmV3IGFycmF5VHlwZShbdmFsdWVdKSxcbiAgICAgICAgICBieXRlQXJyYXkgPSBuZXcgZXhwb3J0cy5VaW50OEFycmF5KHR5cGVBcnJheS5idWZmZXIpLFxuICAgICAgICAgIGJ5dGVzID0gW10sIGksIGJ5dGVWaWV3O1xuXG4gICAgICBmb3IgKGkgPSAwOyBpIDwgYXJyYXlUeXBlLkJZVEVTX1BFUl9FTEVNRU5UOyBpICs9IDEpIHtcbiAgICAgICAgYnl0ZXMucHVzaChyKGJ5dGVBcnJheSwgaSkpO1xuICAgICAgfVxuXG4gICAgICAvLyBGbGlwIGlmIG5lY2Vzc2FyeVxuICAgICAgaWYgKEJvb2xlYW4obGl0dGxlRW5kaWFuKSA9PT0gQm9vbGVhbihJU19CSUdfRU5ESUFOKSkge1xuICAgICAgICBieXRlcy5yZXZlcnNlKCk7XG4gICAgICB9XG5cbiAgICAgIC8vIFdyaXRlIHRoZW1cbiAgICAgIGJ5dGVWaWV3ID0gbmV3IGV4cG9ydHMuVWludDhBcnJheSh0aGlzLmJ1ZmZlciwgYnl0ZU9mZnNldCwgYXJyYXlUeXBlLkJZVEVTX1BFUl9FTEVNRU5UKTtcbiAgICAgIGJ5dGVWaWV3LnNldChieXRlcyk7XG4gICAgfTtcbiAgfVxuXG4gIERhdGFWaWV3LnByb3RvdHlwZS5zZXRVaW50OCA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5VaW50OEFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldEludDggPSBtYWtlU2V0dGVyKGV4cG9ydHMuSW50OEFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldFVpbnQxNiA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5VaW50MTZBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5zZXRJbnQxNiA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5JbnQxNkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldFVpbnQzMiA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5VaW50MzJBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5zZXRJbnQzMiA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5JbnQzMkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldEZsb2F0MzIgPSBtYWtlU2V0dGVyKGV4cG9ydHMuRmxvYXQzMkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldEZsb2F0NjQgPSBtYWtlU2V0dGVyKGV4cG9ydHMuRmxvYXQ2NEFycmF5KTtcblxuICBleHBvcnRzLkRhdGFWaWV3ID0gZXhwb3J0cy5EYXRhVmlldyB8fCBEYXRhVmlldztcblxufSgpKTtcblxufSx7fV19LHt9LFtdKVxuOzttb2R1bGUuZXhwb3J0cz1yZXF1aXJlKFwibmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5XCIpLkJ1ZmZlclxuIiwiLy8gc2hpbSBmb3IgdXNpbmcgcHJvY2VzcyBpbiBicm93c2VyXG5cbnZhciBwcm9jZXNzID0gbW9kdWxlLmV4cG9ydHMgPSB7fTtcblxucHJvY2Vzcy5uZXh0VGljayA9IChmdW5jdGlvbiAoKSB7XG4gICAgdmFyIGNhblNldEltbWVkaWF0ZSA9IHR5cGVvZiB3aW5kb3cgIT09ICd1bmRlZmluZWQnXG4gICAgJiYgd2luZG93LnNldEltbWVkaWF0ZTtcbiAgICB2YXIgY2FuUG9zdCA9IHR5cGVvZiB3aW5kb3cgIT09ICd1bmRlZmluZWQnXG4gICAgJiYgd2luZG93LnBvc3RNZXNzYWdlICYmIHdpbmRvdy5hZGRFdmVudExpc3RlbmVyXG4gICAgO1xuXG4gICAgaWYgKGNhblNldEltbWVkaWF0ZSkge1xuICAgICAgICByZXR1cm4gZnVuY3Rpb24gKGYpIHsgcmV0dXJuIHdpbmRvdy5zZXRJbW1lZGlhdGUoZikgfTtcbiAgICB9XG5cbiAgICBpZiAoY2FuUG9zdCkge1xuICAgICAgICB2YXIgcXVldWUgPSBbXTtcbiAgICAgICAgd2luZG93LmFkZEV2ZW50TGlzdGVuZXIoJ21lc3NhZ2UnLCBmdW5jdGlvbiAoZXYpIHtcbiAgICAgICAgICAgIGlmIChldi5zb3VyY2UgPT09IHdpbmRvdyAmJiBldi5kYXRhID09PSAncHJvY2Vzcy10aWNrJykge1xuICAgICAgICAgICAgICAgIGV2LnN0b3BQcm9wYWdhdGlvbigpO1xuICAgICAgICAgICAgICAgIGlmIChxdWV1ZS5sZW5ndGggPiAwKSB7XG4gICAgICAgICAgICAgICAgICAgIHZhciBmbiA9IHF1ZXVlLnNoaWZ0KCk7XG4gICAgICAgICAgICAgICAgICAgIGZuKCk7XG4gICAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgfVxuICAgICAgICB9LCB0cnVlKTtcblxuICAgICAgICByZXR1cm4gZnVuY3Rpb24gbmV4dFRpY2soZm4pIHtcbiAgICAgICAgICAgIHF1ZXVlLnB1c2goZm4pO1xuICAgICAgICAgICAgd2luZG93LnBvc3RNZXNzYWdlKCdwcm9jZXNzLXRpY2snLCAnKicpO1xuICAgICAgICB9O1xuICAgIH1cblxuICAgIHJldHVybiBmdW5jdGlvbiBuZXh0VGljayhmbikge1xuICAgICAgICBzZXRUaW1lb3V0KGZuLCAwKTtcbiAgICB9O1xufSkoKTtcblxucHJvY2Vzcy50aXRsZSA9ICdicm93c2VyJztcbnByb2Nlc3MuYnJvd3NlciA9IHRydWU7XG5wcm9jZXNzLmVudiA9IHt9O1xucHJvY2Vzcy5hcmd2ID0gW107XG5cbnByb2Nlc3MuYmluZGluZyA9IGZ1bmN0aW9uIChuYW1lKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdwcm9jZXNzLmJpbmRpbmcgaXMgbm90IHN1cHBvcnRlZCcpO1xufVxuXG4vLyBUT0RPKHNodHlsbWFuKVxucHJvY2Vzcy5jd2QgPSBmdW5jdGlvbiAoKSB7IHJldHVybiAnLycgfTtcbnByb2Nlc3MuY2hkaXIgPSBmdW5jdGlvbiAoZGlyKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdwcm9jZXNzLmNoZGlyIGlzIG5vdCBzdXBwb3J0ZWQnKTtcbn07XG4iLCJ2YXIgYmFzZTY0ID0gcmVxdWlyZSgnYmFzZTY0LWpzJylcbnZhciBUQSA9IHJlcXVpcmUoJ3R5cGVkYXJyYXknKVxuXG5leHBvcnRzLkJ1ZmZlciA9IEJ1ZmZlclxuZXhwb3J0cy5TbG93QnVmZmVyID0gQnVmZmVyXG5leHBvcnRzLklOU1BFQ1RfTUFYX0JZVEVTID0gNTBcbkJ1ZmZlci5wb29sU2l6ZSA9IDgxOTJcblxuLyoqXG4gKiBVc2UgYSBzaGltIGZvciBicm93c2VycyB0aGF0IGxhY2sgVHlwZWQgQXJyYXkgc3VwcG9ydCAoPCBJRSA5LCA8IEZGIDMuNixcbiAqIDwgQ2hyb21lIDYsIDwgU2FmYXJpIDUsIDwgT3BlcmEgMTEuNSwgPCBpT1MgNC4xKS5cbiAqL1xudmFyIHhEYXRhVmlldyA9IHR5cGVvZiBEYXRhVmlldyA9PT0gJ3VuZGVmaW5lZCdcbiAgPyBUQS5EYXRhVmlldyA6IERhdGFWaWV3XG52YXIgeEFycmF5QnVmZmVyID0gdHlwZW9mIEFycmF5QnVmZmVyID09PSAndW5kZWZpbmVkJ1xuICA/IFRBLkFycmF5QnVmZmVyIDogQXJyYXlCdWZmZXJcbnZhciB4VWludDhBcnJheSA9IHR5cGVvZiBVaW50OEFycmF5ID09PSAndW5kZWZpbmVkJ1xuICA/IFRBLlVpbnQ4QXJyYXkgOiBVaW50OEFycmF5XG5cbi8qKlxuICogQ2hlY2sgdG8gc2VlIGlmIHRoZSBicm93c2VyIHN1cHBvcnRzIGF1Z21lbnRpbmcgYSBgVWludDhBcnJheWAgaW5zdGFuY2UuXG4gKi9cbnZhciBicm93c2VyU3VwcG9ydCA9IChmdW5jdGlvbiAoKSB7XG4gIHRyeSB7XG4gICAgdmFyIGFyciA9IG5ldyB4VWludDhBcnJheSgwKVxuICAgIGFyci5mb28gPSBmdW5jdGlvbiAoKSB7IHJldHVybiA0MiB9XG4gICAgcmV0dXJuIDQyID09PSBhcnIuZm9vKClcbiAgfSBjYXRjaCAoZSkge1xuICAgIHJldHVybiBmYWxzZVxuICB9XG59KSgpXG5cbi8qKlxuICogQWxzbyB1c2UgdGhlIHNoaW0gaW4gRmlyZWZveCA0LTE3IChldmVuIHRob3VnaCB0aGV5IGhhdmUgbmF0aXZlIFVpbnQ4QXJyYXkpLFxuICogc2luY2UgdGhleSBkb24ndCBzdXBwb3J0IFByb3h5LiBXaXRob3V0IHRoYXQsIGl0IGlzIG5vdCBwb3NzaWJsZSB0byBhdWdtZW50XG4gKiBuYXRpdmUgVWludDhBcnJheSBpbnN0YW5jZXMgaW4gRmlyZWZveC5cbiAqL1xuaWYgKHhVaW50OEFycmF5ICE9PSBUQS5VaW50OEFycmF5ICYmICFicm93c2VyU3VwcG9ydCkge1xuICB4RGF0YVZpZXcgPSBUQS5EYXRhVmlld1xuICB4QXJyYXlCdWZmZXIgPSBUQS5BcnJheUJ1ZmZlclxuICB4VWludDhBcnJheSA9IFRBLlVpbnQ4QXJyYXlcbiAgYnJvd3NlclN1cHBvcnQgPSB0cnVlXG59XG5cbi8qKlxuICogQ2xhc3M6IEJ1ZmZlclxuICogPT09PT09PT09PT09PVxuICpcbiAqIFRoZSBCdWZmZXIgY29uc3RydWN0b3IgcmV0dXJucyBpbnN0YW5jZXMgb2YgYFVpbnQ4QXJyYXlgIHRoYXQgYXJlIGF1Z21lbnRlZFxuICogd2l0aCBmdW5jdGlvbiBwcm9wZXJ0aWVzIGZvciBhbGwgdGhlIG5vZGUgYEJ1ZmZlcmAgQVBJIGZ1bmN0aW9ucy4gV2UgdXNlXG4gKiBgVWludDhBcnJheWAgc28gdGhhdCBzcXVhcmUgYnJhY2tldCBub3RhdGlvbiB3b3JrcyBhcyBleHBlY3RlZCAtLSBpdCByZXR1cm5zXG4gKiBhIHNpbmdsZSBvY3RldC5cbiAqXG4gKiBCeSBhdWdtZW50aW5nIHRoZSBpbnN0YW5jZXMsIHdlIGNhbiBhdm9pZCBtb2RpZnlpbmcgdGhlIGBVaW50OEFycmF5YFxuICogcHJvdG90eXBlLlxuICpcbiAqIEZpcmVmb3ggaXMgYSBzcGVjaWFsIGNhc2UgYmVjYXVzZSBpdCBkb2Vzbid0IGFsbG93IGF1Z21lbnRpbmcgXCJuYXRpdmVcIiBvYmplY3RcbiAqIGluc3RhbmNlcy4gU2VlIGBQcm94eUJ1ZmZlcmAgYmVsb3cgZm9yIG1vcmUgZGV0YWlscy5cbiAqL1xuZnVuY3Rpb24gQnVmZmVyIChzdWJqZWN0LCBlbmNvZGluZykge1xuICB2YXIgdHlwZSA9IHR5cGVvZiBzdWJqZWN0XG5cbiAgLy8gV29yay1hcm91bmQ6IG5vZGUncyBiYXNlNjQgaW1wbGVtZW50YXRpb25cbiAgLy8gYWxsb3dzIGZvciBub24tcGFkZGVkIHN0cmluZ3Mgd2hpbGUgYmFzZTY0LWpzXG4gIC8vIGRvZXMgbm90Li5cbiAgaWYgKGVuY29kaW5nID09PSAnYmFzZTY0JyAmJiB0eXBlID09PSAnc3RyaW5nJykge1xuICAgIHN1YmplY3QgPSBzdHJpbmd0cmltKHN1YmplY3QpXG4gICAgd2hpbGUgKHN1YmplY3QubGVuZ3RoICUgNCAhPT0gMCkge1xuICAgICAgc3ViamVjdCA9IHN1YmplY3QgKyAnPSdcbiAgICB9XG4gIH1cblxuICAvLyBGaW5kIHRoZSBsZW5ndGhcbiAgdmFyIGxlbmd0aFxuICBpZiAodHlwZSA9PT0gJ251bWJlcicpXG4gICAgbGVuZ3RoID0gY29lcmNlKHN1YmplY3QpXG4gIGVsc2UgaWYgKHR5cGUgPT09ICdzdHJpbmcnKVxuICAgIGxlbmd0aCA9IEJ1ZmZlci5ieXRlTGVuZ3RoKHN1YmplY3QsIGVuY29kaW5nKVxuICBlbHNlIGlmICh0eXBlID09PSAnb2JqZWN0JylcbiAgICBsZW5ndGggPSBjb2VyY2Uoc3ViamVjdC5sZW5ndGgpIC8vIEFzc3VtZSBvYmplY3QgaXMgYW4gYXJyYXlcbiAgZWxzZVxuICAgIHRocm93IG5ldyBFcnJvcignRmlyc3QgYXJndW1lbnQgbmVlZHMgdG8gYmUgYSBudW1iZXIsIGFycmF5IG9yIHN0cmluZy4nKVxuXG4gIHZhciBidWYgPSBhdWdtZW50KG5ldyB4VWludDhBcnJheShsZW5ndGgpKVxuICBpZiAoQnVmZmVyLmlzQnVmZmVyKHN1YmplY3QpKSB7XG4gICAgLy8gU3BlZWQgb3B0aW1pemF0aW9uIC0tIHVzZSBzZXQgaWYgd2UncmUgY29weWluZyBmcm9tIGEgVWludDhBcnJheVxuICAgIGJ1Zi5zZXQoc3ViamVjdClcbiAgfSBlbHNlIGlmIChpc0FycmF5SXNoKHN1YmplY3QpKSB7XG4gICAgLy8gVHJlYXQgYXJyYXktaXNoIG9iamVjdHMgYXMgYSBieXRlIGFycmF5LlxuICAgIGZvciAodmFyIGkgPSAwOyBpIDwgbGVuZ3RoOyBpKyspIHtcbiAgICAgIGlmIChCdWZmZXIuaXNCdWZmZXIoc3ViamVjdCkpXG4gICAgICAgIGJ1ZltpXSA9IHN1YmplY3QucmVhZFVJbnQ4KGkpXG4gICAgICBlbHNlXG4gICAgICAgIGJ1ZltpXSA9IHN1YmplY3RbaV1cbiAgICB9XG4gIH0gZWxzZSBpZiAodHlwZSA9PT0gJ3N0cmluZycpIHtcbiAgICBidWYud3JpdGUoc3ViamVjdCwgMCwgZW5jb2RpbmcpXG4gIH1cblxuICByZXR1cm4gYnVmXG59XG5cbi8vIFNUQVRJQyBNRVRIT0RTXG4vLyA9PT09PT09PT09PT09PVxuXG5CdWZmZXIuaXNFbmNvZGluZyA9IGZ1bmN0aW9uKGVuY29kaW5nKSB7XG4gIHN3aXRjaCAoKGVuY29kaW5nICsgJycpLnRvTG93ZXJDYXNlKCkpIHtcbiAgICBjYXNlICdoZXgnOlxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICBjYXNlICdhc2NpaSc6XG4gICAgY2FzZSAnYmluYXJ5JzpcbiAgICBjYXNlICdiYXNlNjQnOlxuICAgIGNhc2UgJ3VjczInOlxuICAgIGNhc2UgJ3Vjcy0yJzpcbiAgICBjYXNlICd1dGYxNmxlJzpcbiAgICBjYXNlICd1dGYtMTZsZSc6XG4gICAgY2FzZSAncmF3JzpcbiAgICAgIHJldHVybiB0cnVlXG5cbiAgICBkZWZhdWx0OlxuICAgICAgcmV0dXJuIGZhbHNlXG4gIH1cbn1cblxuQnVmZmVyLmlzQnVmZmVyID0gZnVuY3Rpb24gKGIpIHtcbiAgcmV0dXJuIGIgJiYgYi5faXNCdWZmZXJcbn1cblxuQnVmZmVyLmJ5dGVMZW5ndGggPSBmdW5jdGlvbiAoc3RyLCBlbmNvZGluZykge1xuICBzd2l0Y2ggKGVuY29kaW5nIHx8ICd1dGY4Jykge1xuICAgIGNhc2UgJ2hleCc6XG4gICAgICByZXR1cm4gc3RyLmxlbmd0aCAvIDJcblxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICAgIHJldHVybiB1dGY4VG9CeXRlcyhzdHIpLmxlbmd0aFxuXG4gICAgY2FzZSAnYXNjaWknOlxuICAgIGNhc2UgJ2JpbmFyeSc6XG4gICAgICByZXR1cm4gc3RyLmxlbmd0aFxuXG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICAgIHJldHVybiBiYXNlNjRUb0J5dGVzKHN0cikubGVuZ3RoXG5cbiAgICBkZWZhdWx0OlxuICAgICAgdGhyb3cgbmV3IEVycm9yKCdVbmtub3duIGVuY29kaW5nJylcbiAgfVxufVxuXG5CdWZmZXIuY29uY2F0ID0gZnVuY3Rpb24gKGxpc3QsIHRvdGFsTGVuZ3RoKSB7XG4gIGlmICghaXNBcnJheShsaXN0KSkge1xuICAgIHRocm93IG5ldyBFcnJvcignVXNhZ2U6IEJ1ZmZlci5jb25jYXQobGlzdCwgW3RvdGFsTGVuZ3RoXSlcXG4nICtcbiAgICAgICAgJ2xpc3Qgc2hvdWxkIGJlIGFuIEFycmF5LicpXG4gIH1cblxuICB2YXIgaVxuICB2YXIgYnVmXG5cbiAgaWYgKGxpc3QubGVuZ3RoID09PSAwKSB7XG4gICAgcmV0dXJuIG5ldyBCdWZmZXIoMClcbiAgfSBlbHNlIGlmIChsaXN0Lmxlbmd0aCA9PT0gMSkge1xuICAgIHJldHVybiBsaXN0WzBdXG4gIH1cblxuICBpZiAodHlwZW9mIHRvdGFsTGVuZ3RoICE9PSAnbnVtYmVyJykge1xuICAgIHRvdGFsTGVuZ3RoID0gMFxuICAgIGZvciAoaSA9IDA7IGkgPCBsaXN0Lmxlbmd0aDsgaSsrKSB7XG4gICAgICBidWYgPSBsaXN0W2ldXG4gICAgICB0b3RhbExlbmd0aCArPSBidWYubGVuZ3RoXG4gICAgfVxuICB9XG5cbiAgdmFyIGJ1ZmZlciA9IG5ldyBCdWZmZXIodG90YWxMZW5ndGgpXG4gIHZhciBwb3MgPSAwXG4gIGZvciAoaSA9IDA7IGkgPCBsaXN0Lmxlbmd0aDsgaSsrKSB7XG4gICAgYnVmID0gbGlzdFtpXVxuICAgIGJ1Zi5jb3B5KGJ1ZmZlciwgcG9zKVxuICAgIHBvcyArPSBidWYubGVuZ3RoXG4gIH1cbiAgcmV0dXJuIGJ1ZmZlclxufVxuXG4vLyBCVUZGRVIgSU5TVEFOQ0UgTUVUSE9EU1xuLy8gPT09PT09PT09PT09PT09PT09PT09PT1cblxuZnVuY3Rpb24gX2hleFdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgb2Zmc2V0ID0gTnVtYmVyKG9mZnNldCkgfHwgMFxuICB2YXIgcmVtYWluaW5nID0gYnVmLmxlbmd0aCAtIG9mZnNldFxuICBpZiAoIWxlbmd0aCkge1xuICAgIGxlbmd0aCA9IHJlbWFpbmluZ1xuICB9IGVsc2Uge1xuICAgIGxlbmd0aCA9IE51bWJlcihsZW5ndGgpXG4gICAgaWYgKGxlbmd0aCA+IHJlbWFpbmluZykge1xuICAgICAgbGVuZ3RoID0gcmVtYWluaW5nXG4gICAgfVxuICB9XG5cbiAgLy8gbXVzdCBiZSBhbiBldmVuIG51bWJlciBvZiBkaWdpdHNcbiAgdmFyIHN0ckxlbiA9IHN0cmluZy5sZW5ndGhcbiAgaWYgKHN0ckxlbiAlIDIgIT09IDApIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ0ludmFsaWQgaGV4IHN0cmluZycpXG4gIH1cbiAgaWYgKGxlbmd0aCA+IHN0ckxlbiAvIDIpIHtcbiAgICBsZW5ndGggPSBzdHJMZW4gLyAyXG4gIH1cbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBsZW5ndGg7IGkrKykge1xuICAgIHZhciBieXRlID0gcGFyc2VJbnQoc3RyaW5nLnN1YnN0cihpICogMiwgMiksIDE2KVxuICAgIGlmIChpc05hTihieXRlKSkgdGhyb3cgbmV3IEVycm9yKCdJbnZhbGlkIGhleCBzdHJpbmcnKVxuICAgIGJ1ZltvZmZzZXQgKyBpXSA9IGJ5dGVcbiAgfVxuICBCdWZmZXIuX2NoYXJzV3JpdHRlbiA9IGkgKiAyXG4gIHJldHVybiBpXG59XG5cbmZ1bmN0aW9uIF91dGY4V3JpdGUgKGJ1Ziwgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCkge1xuICB2YXIgYnl0ZXMsIHBvc1xuICByZXR1cm4gQnVmZmVyLl9jaGFyc1dyaXR0ZW4gPSBibGl0QnVmZmVyKHV0ZjhUb0J5dGVzKHN0cmluZyksIGJ1Ziwgb2Zmc2V0LCBsZW5ndGgpXG59XG5cbmZ1bmN0aW9uIF9hc2NpaVdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIGJ5dGVzLCBwb3NcbiAgcmV0dXJuIEJ1ZmZlci5fY2hhcnNXcml0dGVuID0gYmxpdEJ1ZmZlcihhc2NpaVRvQnl0ZXMoc3RyaW5nKSwgYnVmLCBvZmZzZXQsIGxlbmd0aClcbn1cblxuZnVuY3Rpb24gX2JpbmFyeVdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgcmV0dXJuIF9hc2NpaVdyaXRlKGJ1Ziwgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aClcbn1cblxuZnVuY3Rpb24gX2Jhc2U2NFdyaXRlIChidWYsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIGJ5dGVzLCBwb3NcbiAgcmV0dXJuIEJ1ZmZlci5fY2hhcnNXcml0dGVuID0gYmxpdEJ1ZmZlcihiYXNlNjRUb0J5dGVzKHN0cmluZyksIGJ1Ziwgb2Zmc2V0LCBsZW5ndGgpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlIChzdHJpbmcsIG9mZnNldCwgbGVuZ3RoLCBlbmNvZGluZykge1xuICAvLyBTdXBwb3J0IGJvdGggKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgsIGVuY29kaW5nKVxuICAvLyBhbmQgdGhlIGxlZ2FjeSAoc3RyaW5nLCBlbmNvZGluZywgb2Zmc2V0LCBsZW5ndGgpXG4gIGlmIChpc0Zpbml0ZShvZmZzZXQpKSB7XG4gICAgaWYgKCFpc0Zpbml0ZShsZW5ndGgpKSB7XG4gICAgICBlbmNvZGluZyA9IGxlbmd0aFxuICAgICAgbGVuZ3RoID0gdW5kZWZpbmVkXG4gICAgfVxuICB9IGVsc2UgeyAgLy8gbGVnYWN5XG4gICAgdmFyIHN3YXAgPSBlbmNvZGluZ1xuICAgIGVuY29kaW5nID0gb2Zmc2V0XG4gICAgb2Zmc2V0ID0gbGVuZ3RoXG4gICAgbGVuZ3RoID0gc3dhcFxuICB9XG5cbiAgb2Zmc2V0ID0gTnVtYmVyKG9mZnNldCkgfHwgMFxuICB2YXIgcmVtYWluaW5nID0gdGhpcy5sZW5ndGggLSBvZmZzZXRcbiAgaWYgKCFsZW5ndGgpIHtcbiAgICBsZW5ndGggPSByZW1haW5pbmdcbiAgfSBlbHNlIHtcbiAgICBsZW5ndGggPSBOdW1iZXIobGVuZ3RoKVxuICAgIGlmIChsZW5ndGggPiByZW1haW5pbmcpIHtcbiAgICAgIGxlbmd0aCA9IHJlbWFpbmluZ1xuICAgIH1cbiAgfVxuICBlbmNvZGluZyA9IFN0cmluZyhlbmNvZGluZyB8fCAndXRmOCcpLnRvTG93ZXJDYXNlKClcblxuICBzd2l0Y2ggKGVuY29kaW5nKSB7XG4gICAgY2FzZSAnaGV4JzpcbiAgICAgIHJldHVybiBfaGV4V3JpdGUodGhpcywgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aClcblxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICAgIHJldHVybiBfdXRmOFdyaXRlKHRoaXMsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG5cbiAgICBjYXNlICdhc2NpaSc6XG4gICAgICByZXR1cm4gX2FzY2lpV3JpdGUodGhpcywgc3RyaW5nLCBvZmZzZXQsIGxlbmd0aClcblxuICAgIGNhc2UgJ2JpbmFyeSc6XG4gICAgICByZXR1cm4gX2JpbmFyeVdyaXRlKHRoaXMsIHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpXG5cbiAgICBjYXNlICdiYXNlNjQnOlxuICAgICAgcmV0dXJuIF9iYXNlNjRXcml0ZSh0aGlzLCBzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKVxuXG4gICAgZGVmYXVsdDpcbiAgICAgIHRocm93IG5ldyBFcnJvcignVW5rbm93biBlbmNvZGluZycpXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyVG9TdHJpbmcgKGVuY29kaW5nLCBzdGFydCwgZW5kKSB7XG4gIHZhciBzZWxmID0gKHRoaXMgaW5zdGFuY2VvZiBQcm94eUJ1ZmZlcilcbiAgICA/IHRoaXMuX3Byb3h5XG4gICAgOiB0aGlzXG5cbiAgZW5jb2RpbmcgPSBTdHJpbmcoZW5jb2RpbmcgfHwgJ3V0ZjgnKS50b0xvd2VyQ2FzZSgpXG4gIHN0YXJ0ID0gTnVtYmVyKHN0YXJ0KSB8fCAwXG4gIGVuZCA9IChlbmQgIT09IHVuZGVmaW5lZClcbiAgICA/IE51bWJlcihlbmQpXG4gICAgOiBlbmQgPSBzZWxmLmxlbmd0aFxuXG4gIC8vIEZhc3RwYXRoIGVtcHR5IHN0cmluZ3NcbiAgaWYgKGVuZCA9PT0gc3RhcnQpXG4gICAgcmV0dXJuICcnXG5cbiAgc3dpdGNoIChlbmNvZGluZykge1xuICAgIGNhc2UgJ2hleCc6XG4gICAgICByZXR1cm4gX2hleFNsaWNlKHNlbGYsIHN0YXJ0LCBlbmQpXG5cbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgICByZXR1cm4gX3V0ZjhTbGljZShzZWxmLCBzdGFydCwgZW5kKVxuXG4gICAgY2FzZSAnYXNjaWknOlxuICAgICAgcmV0dXJuIF9hc2NpaVNsaWNlKHNlbGYsIHN0YXJ0LCBlbmQpXG5cbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIF9iaW5hcnlTbGljZShzZWxmLCBzdGFydCwgZW5kKVxuXG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICAgIHJldHVybiBfYmFzZTY0U2xpY2Uoc2VsZiwgc3RhcnQsIGVuZClcblxuICAgIGRlZmF1bHQ6XG4gICAgICB0aHJvdyBuZXcgRXJyb3IoJ1Vua25vd24gZW5jb2RpbmcnKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclRvSlNPTiAoKSB7XG4gIHJldHVybiB7XG4gICAgdHlwZTogJ0J1ZmZlcicsXG4gICAgZGF0YTogQXJyYXkucHJvdG90eXBlLnNsaWNlLmNhbGwodGhpcy5fYXJyIHx8IHRoaXMsIDApXG4gIH1cbn1cblxuLy8gY29weSh0YXJnZXRCdWZmZXIsIHRhcmdldFN0YXJ0PTAsIHNvdXJjZVN0YXJ0PTAsIHNvdXJjZUVuZD1idWZmZXIubGVuZ3RoKVxuZnVuY3Rpb24gQnVmZmVyQ29weSAodGFyZ2V0LCB0YXJnZXRfc3RhcnQsIHN0YXJ0LCBlbmQpIHtcbiAgdmFyIHNvdXJjZSA9IHRoaXNcblxuICBpZiAoIXN0YXJ0KSBzdGFydCA9IDBcbiAgaWYgKCFlbmQgJiYgZW5kICE9PSAwKSBlbmQgPSB0aGlzLmxlbmd0aFxuICBpZiAoIXRhcmdldF9zdGFydCkgdGFyZ2V0X3N0YXJ0ID0gMFxuXG4gIC8vIENvcHkgMCBieXRlczsgd2UncmUgZG9uZVxuICBpZiAoZW5kID09PSBzdGFydCkgcmV0dXJuXG4gIGlmICh0YXJnZXQubGVuZ3RoID09PSAwIHx8IHNvdXJjZS5sZW5ndGggPT09IDApIHJldHVyblxuXG4gIC8vIEZhdGFsIGVycm9yIGNvbmRpdGlvbnNcbiAgaWYgKGVuZCA8IHN0YXJ0KVxuICAgIHRocm93IG5ldyBFcnJvcignc291cmNlRW5kIDwgc291cmNlU3RhcnQnKVxuICBpZiAodGFyZ2V0X3N0YXJ0IDwgMCB8fCB0YXJnZXRfc3RhcnQgPj0gdGFyZ2V0Lmxlbmd0aClcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3RhcmdldFN0YXJ0IG91dCBvZiBib3VuZHMnKVxuICBpZiAoc3RhcnQgPCAwIHx8IHN0YXJ0ID49IHNvdXJjZS5sZW5ndGgpXG4gICAgdGhyb3cgbmV3IEVycm9yKCdzb3VyY2VTdGFydCBvdXQgb2YgYm91bmRzJylcbiAgaWYgKGVuZCA8IDAgfHwgZW5kID4gc291cmNlLmxlbmd0aClcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3NvdXJjZUVuZCBvdXQgb2YgYm91bmRzJylcblxuICAvLyBBcmUgd2Ugb29iP1xuICBpZiAoZW5kID4gdGhpcy5sZW5ndGgpXG4gICAgZW5kID0gdGhpcy5sZW5ndGhcbiAgaWYgKHRhcmdldC5sZW5ndGggLSB0YXJnZXRfc3RhcnQgPCBlbmQgLSBzdGFydClcbiAgICBlbmQgPSB0YXJnZXQubGVuZ3RoIC0gdGFyZ2V0X3N0YXJ0ICsgc3RhcnRcblxuICAvLyBjb3B5IVxuICBmb3IgKHZhciBpID0gMDsgaSA8IGVuZCAtIHN0YXJ0OyBpKyspXG4gICAgdGFyZ2V0W2kgKyB0YXJnZXRfc3RhcnRdID0gdGhpc1tpICsgc3RhcnRdXG59XG5cbmZ1bmN0aW9uIF9iYXNlNjRTbGljZSAoYnVmLCBzdGFydCwgZW5kKSB7XG4gIGlmIChzdGFydCA9PT0gMCAmJiBlbmQgPT09IGJ1Zi5sZW5ndGgpIHtcbiAgICByZXR1cm4gYmFzZTY0LmZyb21CeXRlQXJyYXkoYnVmKVxuICB9IGVsc2Uge1xuICAgIHJldHVybiBiYXNlNjQuZnJvbUJ5dGVBcnJheShidWYuc2xpY2Uoc3RhcnQsIGVuZCkpXG4gIH1cbn1cblxuZnVuY3Rpb24gX3V0ZjhTbGljZSAoYnVmLCBzdGFydCwgZW5kKSB7XG4gIHZhciByZXMgPSAnJ1xuICB2YXIgdG1wID0gJydcbiAgZW5kID0gTWF0aC5taW4oYnVmLmxlbmd0aCwgZW5kKVxuXG4gIGZvciAodmFyIGkgPSBzdGFydDsgaSA8IGVuZDsgaSsrKSB7XG4gICAgaWYgKGJ1ZltpXSA8PSAweDdGKSB7XG4gICAgICByZXMgKz0gZGVjb2RlVXRmOENoYXIodG1wKSArIFN0cmluZy5mcm9tQ2hhckNvZGUoYnVmW2ldKVxuICAgICAgdG1wID0gJydcbiAgICB9IGVsc2Uge1xuICAgICAgdG1wICs9ICclJyArIGJ1ZltpXS50b1N0cmluZygxNilcbiAgICB9XG4gIH1cblxuICByZXR1cm4gcmVzICsgZGVjb2RlVXRmOENoYXIodG1wKVxufVxuXG5mdW5jdGlvbiBfYXNjaWlTbGljZSAoYnVmLCBzdGFydCwgZW5kKSB7XG4gIHZhciByZXQgPSAnJ1xuICBlbmQgPSBNYXRoLm1pbihidWYubGVuZ3RoLCBlbmQpXG5cbiAgZm9yICh2YXIgaSA9IHN0YXJ0OyBpIDwgZW5kOyBpKyspXG4gICAgcmV0ICs9IFN0cmluZy5mcm9tQ2hhckNvZGUoYnVmW2ldKVxuICByZXR1cm4gcmV0XG59XG5cbmZ1bmN0aW9uIF9iaW5hcnlTbGljZSAoYnVmLCBzdGFydCwgZW5kKSB7XG4gIHJldHVybiBfYXNjaWlTbGljZShidWYsIHN0YXJ0LCBlbmQpXG59XG5cbmZ1bmN0aW9uIF9oZXhTbGljZSAoYnVmLCBzdGFydCwgZW5kKSB7XG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG5cbiAgaWYgKCFzdGFydCB8fCBzdGFydCA8IDApIHN0YXJ0ID0gMFxuICBpZiAoIWVuZCB8fCBlbmQgPCAwIHx8IGVuZCA+IGxlbikgZW5kID0gbGVuXG5cbiAgdmFyIG91dCA9ICcnXG4gIGZvciAodmFyIGkgPSBzdGFydDsgaSA8IGVuZDsgaSsrKSB7XG4gICAgb3V0ICs9IHRvSGV4KGJ1ZltpXSlcbiAgfVxuICByZXR1cm4gb3V0XG59XG5cbi8vIFRPRE86IGFkZCB0ZXN0IHRoYXQgbW9kaWZ5aW5nIHRoZSBuZXcgYnVmZmVyIHNsaWNlIHdpbGwgbW9kaWZ5IG1lbW9yeSBpbiB0aGVcbi8vIG9yaWdpbmFsIGJ1ZmZlciEgVXNlIGNvZGUgZnJvbTpcbi8vIGh0dHA6Ly9ub2RlanMub3JnL2FwaS9idWZmZXIuaHRtbCNidWZmZXJfYnVmX3NsaWNlX3N0YXJ0X2VuZFxuZnVuY3Rpb24gQnVmZmVyU2xpY2UgKHN0YXJ0LCBlbmQpIHtcbiAgdmFyIGxlbiA9IHRoaXMubGVuZ3RoXG4gIHN0YXJ0ID0gY2xhbXAoc3RhcnQsIGxlbiwgMClcbiAgZW5kID0gY2xhbXAoZW5kLCBsZW4sIGxlbilcbiAgcmV0dXJuIGF1Z21lbnQodGhpcy5zdWJhcnJheShzdGFydCwgZW5kKSkgLy8gVWludDhBcnJheSBidWlsdC1pbiBtZXRob2Rcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZFVJbnQ4IChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHZhciBidWYgPSB0aGlzXG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWYubGVuZ3RoKVxuICAgIHJldHVyblxuXG4gIHJldHVybiBidWZbb2Zmc2V0XVxufVxuXG5mdW5jdGlvbiBfcmVhZFVJbnQxNiAoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDEgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHJlYWQgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAxID09PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoMikpXG4gICAgZHYuc2V0VWludDgoMCwgYnVmW2xlbiAtIDFdKVxuICAgIHJldHVybiBkdi5nZXRVaW50MTYoMCwgbGl0dGxlRW5kaWFuKVxuICB9IGVsc2Uge1xuICAgIHJldHVybiBidWYuX2RhdGF2aWV3LmdldFVpbnQxNihvZmZzZXQsIGxpdHRsZUVuZGlhbilcbiAgfVxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkVUludDE2TEUgKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIF9yZWFkVUludDE2KHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRVSW50MTZCRSAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWRVSW50MTYodGhpcywgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIF9yZWFkVUludDMyIChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMyA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbikge1xuICAgIHJldHVyblxuICB9IGVsc2UgaWYgKG9mZnNldCArIDMgPj0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDQpKVxuICAgIGZvciAodmFyIGkgPSAwOyBpICsgb2Zmc2V0IDwgbGVuOyBpKyspIHtcbiAgICAgIGR2LnNldFVpbnQ4KGksIGJ1ZltpICsgb2Zmc2V0XSlcbiAgICB9XG4gICAgcmV0dXJuIGR2LmdldFVpbnQzMigwLCBsaXR0bGVFbmRpYW4pXG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIGJ1Zi5fZGF0YXZpZXcuZ2V0VWludDMyKG9mZnNldCwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRVSW50MzJMRSAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWRVSW50MzIodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZFVJbnQzMkJFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZFVJbnQzMih0aGlzLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZEludDggKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1Zi5sZW5ndGgpXG4gICAgcmV0dXJuXG5cbiAgcmV0dXJuIGJ1Zi5fZGF0YXZpZXcuZ2V0SW50OChvZmZzZXQpXG59XG5cbmZ1bmN0aW9uIF9yZWFkSW50MTYgKGJ1Ziwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodHlwZW9mIChsaXR0bGVFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAxIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKSB7XG4gICAgcmV0dXJuXG4gIH0gZWxzZSBpZiAob2Zmc2V0ICsgMSA9PT0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDIpKVxuICAgIGR2LnNldFVpbnQ4KDAsIGJ1ZltsZW4gLSAxXSlcbiAgICByZXR1cm4gZHYuZ2V0SW50MTYoMCwgbGl0dGxlRW5kaWFuKVxuICB9IGVsc2Uge1xuICAgIHJldHVybiBidWYuX2RhdGF2aWV3LmdldEludDE2KG9mZnNldCwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRJbnQxNkxFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEludDE2KHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRJbnQxNkJFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEludDE2KHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfcmVhZEludDMyIChidWYsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgMyA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gIH1cblxuICB2YXIgbGVuID0gYnVmLmxlbmd0aFxuICBpZiAob2Zmc2V0ID49IGxlbikge1xuICAgIHJldHVyblxuICB9IGVsc2UgaWYgKG9mZnNldCArIDMgPj0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDQpKVxuICAgIGZvciAodmFyIGkgPSAwOyBpICsgb2Zmc2V0IDwgbGVuOyBpKyspIHtcbiAgICAgIGR2LnNldFVpbnQ4KGksIGJ1ZltpICsgb2Zmc2V0XSlcbiAgICB9XG4gICAgcmV0dXJuIGR2LmdldEludDMyKDAsIGxpdHRsZUVuZGlhbilcbiAgfSBlbHNlIHtcbiAgICByZXR1cm4gYnVmLl9kYXRhdmlldy5nZXRJbnQzMihvZmZzZXQsIGxpdHRsZUVuZGlhbilcbiAgfVxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkSW50MzJMRSAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWRJbnQzMih0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJSZWFkSW50MzJCRSAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWRJbnQzMih0aGlzLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gX3JlYWRGbG9hdCAoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHJldHVybiBidWYuX2RhdGF2aWV3LmdldEZsb2F0MzIob2Zmc2V0LCBsaXR0bGVFbmRpYW4pXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRGbG9hdExFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEZsb2F0KHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWRGbG9hdEJFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZEZsb2F0KHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfcmVhZERvdWJsZSAoYnVmLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgKyA3IDwgYnVmLmxlbmd0aCwgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgfVxuXG4gIHJldHVybiBidWYuX2RhdGF2aWV3LmdldEZsb2F0NjQob2Zmc2V0LCBsaXR0bGVFbmRpYW4pXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlclJlYWREb3VibGVMRSAob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gX3JlYWREb3VibGUodGhpcywgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyUmVhZERvdWJsZUJFIChvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiBfcmVhZERvdWJsZSh0aGlzLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVVSW50OCAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZiA9IHRoaXNcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0IDwgYnVmLmxlbmd0aCwgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmKVxuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWYubGVuZ3RoKSByZXR1cm5cblxuICBidWZbb2Zmc2V0XSA9IHZhbHVlXG59XG5cbmZ1bmN0aW9uIF93cml0ZVVJbnQxNiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDEgPCBidWYubGVuZ3RoLCAndHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZnVpbnQodmFsdWUsIDB4ZmZmZilcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKSB7XG4gICAgcmV0dXJuXG4gIH0gZWxzZSBpZiAob2Zmc2V0ICsgMSA9PT0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDIpKVxuICAgIGR2LnNldFVpbnQxNigwLCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICAgIGJ1ZltvZmZzZXRdID0gZHYuZ2V0VWludDgoMClcbiAgfSBlbHNlIHtcbiAgICBidWYuX2RhdGF2aWV3LnNldFVpbnQxNihvZmZzZXQsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVVSW50MTZMRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlVUludDE2KHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZVVJbnQxNkJFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVVSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfd3JpdGVVSW50MzIgKGJ1ZiwgdmFsdWUsIG9mZnNldCwgbGl0dGxlRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0KHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsICdtaXNzaW5nIHZhbHVlJylcbiAgICBhc3NlcnQodHlwZW9mIChsaXR0bGVFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJylcbiAgICBhc3NlcnQob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLCAnbWlzc2luZyBvZmZzZXQnKVxuICAgIGFzc2VydChvZmZzZXQgKyAzIDwgYnVmLmxlbmd0aCwgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmZmZmZmZmKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAzID49IGxlbikge1xuICAgIHZhciBkdiA9IG5ldyB4RGF0YVZpZXcobmV3IHhBcnJheUJ1ZmZlcig0KSlcbiAgICBkdi5zZXRVaW50MzIoMCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgICBmb3IgKHZhciBpID0gMDsgaSArIG9mZnNldCA8IGxlbjsgaSsrKSB7XG4gICAgICBidWZbaSArIG9mZnNldF0gPSBkdi5nZXRVaW50OChpKVxuICAgIH1cbiAgfSBlbHNlIHtcbiAgICBidWYuX2RhdGF2aWV3LnNldFVpbnQzMihvZmZzZXQsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVVSW50MzJMRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlVUludDMyKHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZVVJbnQzMkJFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVVSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZUludDggKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHZhciBidWYgPSB0aGlzXG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCA8IGJ1Zi5sZW5ndGgsICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmc2ludCh2YWx1ZSwgMHg3ZiwgLTB4ODApXG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1Zi5sZW5ndGgpIHJldHVyblxuXG4gIGJ1Zi5fZGF0YXZpZXcuc2V0SW50OChvZmZzZXQsIHZhbHVlKVxufVxuXG5mdW5jdGlvbiBfd3JpdGVJbnQxNiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDEgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZnNpbnQodmFsdWUsIDB4N2ZmZiwgLTB4ODAwMClcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKSB7XG4gICAgcmV0dXJuXG4gIH0gZWxzZSBpZiAob2Zmc2V0ICsgMSA9PT0gbGVuKSB7XG4gICAgdmFyIGR2ID0gbmV3IHhEYXRhVmlldyhuZXcgeEFycmF5QnVmZmVyKDIpKVxuICAgIGR2LnNldEludDE2KDAsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gICAgYnVmW29mZnNldF0gPSBkdi5nZXRVaW50OCgwKVxuICB9IGVsc2Uge1xuICAgIGJ1Zi5fZGF0YXZpZXcuc2V0SW50MTYob2Zmc2V0LCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlSW50MTZMRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlSW50MTZCRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfd3JpdGVJbnQzMiAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDMgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZnNpbnQodmFsdWUsIDB4N2ZmZmZmZmYsIC0weDgwMDAwMDAwKVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAzID49IGxlbikge1xuICAgIHZhciBkdiA9IG5ldyB4RGF0YVZpZXcobmV3IHhBcnJheUJ1ZmZlcig0KSlcbiAgICBkdi5zZXRJbnQzMigwLCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICAgIGZvciAodmFyIGkgPSAwOyBpICsgb2Zmc2V0IDwgbGVuOyBpKyspIHtcbiAgICAgIGJ1ZltpICsgb2Zmc2V0XSA9IGR2LmdldFVpbnQ4KGkpXG4gICAgfVxuICB9IGVsc2Uge1xuICAgIGJ1Zi5fZGF0YXZpZXcuc2V0SW50MzIob2Zmc2V0LCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlSW50MzJMRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpXG59XG5cbmZ1bmN0aW9uIEJ1ZmZlcldyaXRlSW50MzJCRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBfd3JpdGVGbG9hdCAoYnVmLCB2YWx1ZSwgb2Zmc2V0LCBsaXR0bGVFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCwgJ21pc3NpbmcgdmFsdWUnKVxuICAgIGFzc2VydCh0eXBlb2YgKGxpdHRsZUVuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKVxuICAgIGFzc2VydChvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsICdtaXNzaW5nIG9mZnNldCcpXG4gICAgYXNzZXJ0KG9mZnNldCArIDMgPCBidWYubGVuZ3RoLCAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJylcbiAgICB2ZXJpZklFRUU3NTQodmFsdWUsIDMuNDAyODIzNDY2Mzg1Mjg4NmUrMzgsIC0zLjQwMjgyMzQ2NjM4NTI4ODZlKzM4KVxuICB9XG5cbiAgdmFyIGxlbiA9IGJ1Zi5sZW5ndGhcbiAgaWYgKG9mZnNldCA+PSBsZW4pIHtcbiAgICByZXR1cm5cbiAgfSBlbHNlIGlmIChvZmZzZXQgKyAzID49IGxlbikge1xuICAgIHZhciBkdiA9IG5ldyB4RGF0YVZpZXcobmV3IHhBcnJheUJ1ZmZlcig0KSlcbiAgICBkdi5zZXRGbG9hdDMyKDAsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gICAgZm9yICh2YXIgaSA9IDA7IGkgKyBvZmZzZXQgPCBsZW47IGkrKykge1xuICAgICAgYnVmW2kgKyBvZmZzZXRdID0gZHYuZ2V0VWludDgoaSlcbiAgICB9XG4gIH0gZWxzZSB7XG4gICAgYnVmLl9kYXRhdmlldy5zZXRGbG9hdDMyKG9mZnNldCwgdmFsdWUsIGxpdHRsZUVuZGlhbilcbiAgfVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZUZsb2F0TEUgKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZUZsb2F0KHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZUZsb2F0QkUgKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIF93cml0ZUZsb2F0KHRoaXMsIHZhbHVlLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydClcbn1cblxuZnVuY3Rpb24gX3dyaXRlRG91YmxlIChidWYsIHZhbHVlLCBvZmZzZXQsIGxpdHRsZUVuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydCh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLCAnbWlzc2luZyB2YWx1ZScpXG4gICAgYXNzZXJ0KHR5cGVvZiAobGl0dGxlRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpXG4gICAgYXNzZXJ0KG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCwgJ21pc3Npbmcgb2Zmc2V0JylcbiAgICBhc3NlcnQob2Zmc2V0ICsgNyA8IGJ1Zi5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKVxuICAgIHZlcmlmSUVFRTc1NCh2YWx1ZSwgMS43OTc2OTMxMzQ4NjIzMTU3RSszMDgsIC0xLjc5NzY5MzEzNDg2MjMxNTdFKzMwOClcbiAgfVxuXG4gIHZhciBsZW4gPSBidWYubGVuZ3RoXG4gIGlmIChvZmZzZXQgPj0gbGVuKSB7XG4gICAgcmV0dXJuXG4gIH0gZWxzZSBpZiAob2Zmc2V0ICsgNyA+PSBsZW4pIHtcbiAgICB2YXIgZHYgPSBuZXcgeERhdGFWaWV3KG5ldyB4QXJyYXlCdWZmZXIoOCkpXG4gICAgZHYuc2V0RmxvYXQ2NCgwLCB2YWx1ZSwgbGl0dGxlRW5kaWFuKVxuICAgIGZvciAodmFyIGkgPSAwOyBpICsgb2Zmc2V0IDwgbGVuOyBpKyspIHtcbiAgICAgIGJ1ZltpICsgb2Zmc2V0XSA9IGR2LmdldFVpbnQ4KGkpXG4gICAgfVxuICB9IGVsc2Uge1xuICAgIGJ1Zi5fZGF0YXZpZXcuc2V0RmxvYXQ2NChvZmZzZXQsIHZhbHVlLCBsaXR0bGVFbmRpYW4pXG4gIH1cbn1cblxuZnVuY3Rpb24gQnVmZmVyV3JpdGVEb3VibGVMRSAodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgX3dyaXRlRG91YmxlKHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KVxufVxuXG5mdW5jdGlvbiBCdWZmZXJXcml0ZURvdWJsZUJFICh2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICBfd3JpdGVEb3VibGUodGhpcywgdmFsdWUsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KVxufVxuXG4vLyBmaWxsKHZhbHVlLCBzdGFydD0wLCBlbmQ9YnVmZmVyLmxlbmd0aClcbmZ1bmN0aW9uIEJ1ZmZlckZpbGwgKHZhbHVlLCBzdGFydCwgZW5kKSB7XG4gIGlmICghdmFsdWUpIHZhbHVlID0gMFxuICBpZiAoIXN0YXJ0KSBzdGFydCA9IDBcbiAgaWYgKCFlbmQpIGVuZCA9IHRoaXMubGVuZ3RoXG5cbiAgaWYgKHR5cGVvZiB2YWx1ZSA9PT0gJ3N0cmluZycpIHtcbiAgICB2YWx1ZSA9IHZhbHVlLmNoYXJDb2RlQXQoMClcbiAgfVxuXG4gIGlmICh0eXBlb2YgdmFsdWUgIT09ICdudW1iZXInIHx8IGlzTmFOKHZhbHVlKSkge1xuICAgIHRocm93IG5ldyBFcnJvcigndmFsdWUgaXMgbm90IGEgbnVtYmVyJylcbiAgfVxuXG4gIGlmIChlbmQgPCBzdGFydCkgdGhyb3cgbmV3IEVycm9yKCdlbmQgPCBzdGFydCcpXG5cbiAgLy8gRmlsbCAwIGJ5dGVzOyB3ZSdyZSBkb25lXG4gIGlmIChlbmQgPT09IHN0YXJ0KSByZXR1cm5cbiAgaWYgKHRoaXMubGVuZ3RoID09PSAwKSByZXR1cm5cblxuICBpZiAoc3RhcnQgPCAwIHx8IHN0YXJ0ID49IHRoaXMubGVuZ3RoKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdzdGFydCBvdXQgb2YgYm91bmRzJylcbiAgfVxuXG4gIGlmIChlbmQgPCAwIHx8IGVuZCA+IHRoaXMubGVuZ3RoKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdlbmQgb3V0IG9mIGJvdW5kcycpXG4gIH1cblxuICBmb3IgKHZhciBpID0gc3RhcnQ7IGkgPCBlbmQ7IGkrKykge1xuICAgIHRoaXNbaV0gPSB2YWx1ZVxuICB9XG59XG5cbmZ1bmN0aW9uIEJ1ZmZlckluc3BlY3QgKCkge1xuICB2YXIgb3V0ID0gW11cbiAgdmFyIGxlbiA9IHRoaXMubGVuZ3RoXG4gIGZvciAodmFyIGkgPSAwOyBpIDwgbGVuOyBpKyspIHtcbiAgICBvdXRbaV0gPSB0b0hleCh0aGlzW2ldKVxuICAgIGlmIChpID09PSBleHBvcnRzLklOU1BFQ1RfTUFYX0JZVEVTKSB7XG4gICAgICBvdXRbaSArIDFdID0gJy4uLidcbiAgICAgIGJyZWFrXG4gICAgfVxuICB9XG4gIHJldHVybiAnPEJ1ZmZlciAnICsgb3V0LmpvaW4oJyAnKSArICc+J1xufVxuXG4vLyBDcmVhdGVzIGEgbmV3IGBBcnJheUJ1ZmZlcmAgd2l0aCB0aGUgKmNvcGllZCogbWVtb3J5IG9mIHRoZSBidWZmZXIgaW5zdGFuY2UuXG4vLyBBZGRlZCBpbiBOb2RlIDAuMTIuXG5mdW5jdGlvbiBCdWZmZXJUb0FycmF5QnVmZmVyICgpIHtcbiAgcmV0dXJuIChuZXcgQnVmZmVyKHRoaXMpKS5idWZmZXJcbn1cblxuXG4vLyBIRUxQRVIgRlVOQ1RJT05TXG4vLyA9PT09PT09PT09PT09PT09XG5cbmZ1bmN0aW9uIHN0cmluZ3RyaW0gKHN0cikge1xuICBpZiAoc3RyLnRyaW0pIHJldHVybiBzdHIudHJpbSgpXG4gIHJldHVybiBzdHIucmVwbGFjZSgvXlxccyt8XFxzKyQvZywgJycpXG59XG5cbi8qKlxuICogQ2xhc3M6IFByb3h5QnVmZmVyXG4gKiA9PT09PT09PT09PT09PT09PT1cbiAqXG4gKiBPbmx5IHVzZWQgaW4gRmlyZWZveCwgc2luY2UgRmlyZWZveCBkb2VzIG5vdCBhbGxvdyBhdWdtZW50aW5nIFwibmF0aXZlXCJcbiAqIG9iamVjdHMgKGxpa2UgVWludDhBcnJheSBpbnN0YW5jZXMpIHdpdGggbmV3IHByb3BlcnRpZXMgZm9yIHNvbWUgdW5rbm93blxuICogKHByb2JhYmx5IHNpbGx5KSByZWFzb24uIFNvIHdlJ2xswqB1c2UgYW4gRVM2IFByb3h5IChzdXBwb3J0ZWQgc2luY2VcbiAqIEZpcmVmb3ggMTgpIHRvIHdyYXAgdGhlIFVpbnQ4QXJyYXkgaW5zdGFuY2Ugd2l0aG91dCBhY3R1YWxseSBhZGRpbmcgYW55XG4gKiBwcm9wZXJ0aWVzIHRvIGl0LlxuICpcbiAqIEluc3RhbmNlcyBvZiB0aGlzIFwiZmFrZVwiIEJ1ZmZlciBjbGFzcyBhcmUgdGhlIFwidGFyZ2V0XCIgb2YgdGhlXG4gKiBFUzYgUHJveHkgKHNlZSBgYXVnbWVudGAgZnVuY3Rpb24pLlxuICpcbiAqIFdlIGNvdWxkbid0IGp1c3QgdXNlIHRoZSBgVWludDhBcnJheWAgYXMgdGhlIHRhcmdldCBvZiB0aGUgYFByb3h5YCBiZWNhdXNlXG4gKiBQcm94aWVzIGhhdmUgYW4gaW1wb3J0YW50IGxpbWl0YXRpb24gb24gdHJhcHBpbmcgdGhlIGB0b1N0cmluZ2AgbWV0aG9kLlxuICogYE9iamVjdC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbChwcm94eSlgIGdldHMgY2FsbGVkIHdoZW5ldmVyIHNvbWV0aGluZyBpc1xuICogaW1wbGljaXRseSBjYXN0IHRvIGEgU3RyaW5nLiBVbmZvcnR1bmF0ZWx5LCB3aXRoIGEgYFByb3h5YCB0aGlzXG4gKiB1bmNvbmRpdGlvbmFsbHkgcmV0dXJucyBgT2JqZWN0LnByb3RvdHlwZS50b1N0cmluZy5jYWxsKHRhcmdldClgIHdoaWNoIHdvdWxkXG4gKiBhbHdheXMgcmV0dXJuIFwiW29iamVjdCBVaW50OEFycmF5XVwiIGlmIHdlIHVzZWQgdGhlIGBVaW50OEFycmF5YCBpbnN0YW5jZSBhc1xuICogdGhlIHRhcmdldC4gQW5kLCByZW1lbWJlciwgaW4gRmlyZWZveCB3ZSBjYW5ub3QgcmVkZWZpbmUgdGhlIGBVaW50OEFycmF5YFxuICogaW5zdGFuY2UncyBgdG9TdHJpbmdgIG1ldGhvZC5cbiAqXG4gKiBTbywgd2UgdXNlIHRoaXMgYFByb3h5QnVmZmVyYCBjbGFzcyBhcyB0aGUgcHJveHkncyBcInRhcmdldFwiLiBTaW5jZSB0aGlzIGNsYXNzXG4gKiBoYXMgaXRzIG93biBjdXN0b20gYHRvU3RyaW5nYCBtZXRob2QsIGl0IHdpbGwgZ2V0IGNhbGxlZCB3aGVuZXZlciBgdG9TdHJpbmdgXG4gKiBnZXRzIGNhbGxlZCwgaW1wbGljaXRseSBvciBleHBsaWNpdGx5LCBvbiB0aGUgYFByb3h5YCBpbnN0YW5jZS5cbiAqXG4gKiBXZSBhbHNvIGhhdmUgdG8gZGVmaW5lIHRoZSBVaW50OEFycmF5IG1ldGhvZHMgYHN1YmFycmF5YCBhbmQgYHNldGAgb25cbiAqIGBQcm94eUJ1ZmZlcmAgYmVjYXVzZSBpZiB3ZSBkaWRuJ3QgdGhlbiBgcHJveHkuc3ViYXJyYXkoMClgIHdvdWxkIGhhdmUgaXRzXG4gKiBgdGhpc2Agc2V0IHRvIGBwcm94eWAgKGEgYFByb3h5YCBpbnN0YW5jZSkgd2hpY2ggdGhyb3dzIGFuIGV4Y2VwdGlvbiBpblxuICogRmlyZWZveCB3aGljaCBleHBlY3RzIGl0IHRvIGJlIGEgYFR5cGVkQXJyYXlgIGluc3RhbmNlLlxuICovXG5mdW5jdGlvbiBQcm94eUJ1ZmZlciAoYXJyKSB7XG4gIHRoaXMuX2FyciA9IGFyclxuXG4gIGlmIChhcnIuYnl0ZUxlbmd0aCAhPT0gMClcbiAgICB0aGlzLl9kYXRhdmlldyA9IG5ldyB4RGF0YVZpZXcoYXJyLmJ1ZmZlciwgYXJyLmJ5dGVPZmZzZXQsIGFyci5ieXRlTGVuZ3RoKVxufVxuXG5Qcm94eUJ1ZmZlci5wcm90b3R5cGUgPSB7XG4gIF9pc0J1ZmZlcjogdHJ1ZSxcbiAgd3JpdGU6IEJ1ZmZlcldyaXRlLFxuICB0b1N0cmluZzogQnVmZmVyVG9TdHJpbmcsXG4gIHRvTG9jYWxlU3RyaW5nOiBCdWZmZXJUb1N0cmluZyxcbiAgdG9KU09OOiBCdWZmZXJUb0pTT04sXG4gIGNvcHk6IEJ1ZmZlckNvcHksXG4gIHNsaWNlOiBCdWZmZXJTbGljZSxcbiAgcmVhZFVJbnQ4OiBCdWZmZXJSZWFkVUludDgsXG4gIHJlYWRVSW50MTZMRTogQnVmZmVyUmVhZFVJbnQxNkxFLFxuICByZWFkVUludDE2QkU6IEJ1ZmZlclJlYWRVSW50MTZCRSxcbiAgcmVhZFVJbnQzMkxFOiBCdWZmZXJSZWFkVUludDMyTEUsXG4gIHJlYWRVSW50MzJCRTogQnVmZmVyUmVhZFVJbnQzMkJFLFxuICByZWFkSW50ODogQnVmZmVyUmVhZEludDgsXG4gIHJlYWRJbnQxNkxFOiBCdWZmZXJSZWFkSW50MTZMRSxcbiAgcmVhZEludDE2QkU6IEJ1ZmZlclJlYWRJbnQxNkJFLFxuICByZWFkSW50MzJMRTogQnVmZmVyUmVhZEludDMyTEUsXG4gIHJlYWRJbnQzMkJFOiBCdWZmZXJSZWFkSW50MzJCRSxcbiAgcmVhZEZsb2F0TEU6IEJ1ZmZlclJlYWRGbG9hdExFLFxuICByZWFkRmxvYXRCRTogQnVmZmVyUmVhZEZsb2F0QkUsXG4gIHJlYWREb3VibGVMRTogQnVmZmVyUmVhZERvdWJsZUxFLFxuICByZWFkRG91YmxlQkU6IEJ1ZmZlclJlYWREb3VibGVCRSxcbiAgd3JpdGVVSW50ODogQnVmZmVyV3JpdGVVSW50OCxcbiAgd3JpdGVVSW50MTZMRTogQnVmZmVyV3JpdGVVSW50MTZMRSxcbiAgd3JpdGVVSW50MTZCRTogQnVmZmVyV3JpdGVVSW50MTZCRSxcbiAgd3JpdGVVSW50MzJMRTogQnVmZmVyV3JpdGVVSW50MzJMRSxcbiAgd3JpdGVVSW50MzJCRTogQnVmZmVyV3JpdGVVSW50MzJCRSxcbiAgd3JpdGVJbnQ4OiBCdWZmZXJXcml0ZUludDgsXG4gIHdyaXRlSW50MTZMRTogQnVmZmVyV3JpdGVJbnQxNkxFLFxuICB3cml0ZUludDE2QkU6IEJ1ZmZlcldyaXRlSW50MTZCRSxcbiAgd3JpdGVJbnQzMkxFOiBCdWZmZXJXcml0ZUludDMyTEUsXG4gIHdyaXRlSW50MzJCRTogQnVmZmVyV3JpdGVJbnQzMkJFLFxuICB3cml0ZUZsb2F0TEU6IEJ1ZmZlcldyaXRlRmxvYXRMRSxcbiAgd3JpdGVGbG9hdEJFOiBCdWZmZXJXcml0ZUZsb2F0QkUsXG4gIHdyaXRlRG91YmxlTEU6IEJ1ZmZlcldyaXRlRG91YmxlTEUsXG4gIHdyaXRlRG91YmxlQkU6IEJ1ZmZlcldyaXRlRG91YmxlQkUsXG4gIGZpbGw6IEJ1ZmZlckZpbGwsXG4gIGluc3BlY3Q6IEJ1ZmZlckluc3BlY3QsXG4gIHRvQXJyYXlCdWZmZXI6IEJ1ZmZlclRvQXJyYXlCdWZmZXIsXG4gIHN1YmFycmF5OiBmdW5jdGlvbiAoKSB7XG4gICAgcmV0dXJuIHRoaXMuX2Fyci5zdWJhcnJheS5hcHBseSh0aGlzLl9hcnIsIGFyZ3VtZW50cylcbiAgfSxcbiAgc2V0OiBmdW5jdGlvbiAoKSB7XG4gICAgcmV0dXJuIHRoaXMuX2Fyci5zZXQuYXBwbHkodGhpcy5fYXJyLCBhcmd1bWVudHMpXG4gIH1cbn1cblxudmFyIFByb3h5SGFuZGxlciA9IHtcbiAgZ2V0OiBmdW5jdGlvbiAodGFyZ2V0LCBuYW1lKSB7XG4gICAgaWYgKG5hbWUgaW4gdGFyZ2V0KSByZXR1cm4gdGFyZ2V0W25hbWVdXG4gICAgZWxzZSByZXR1cm4gdGFyZ2V0Ll9hcnJbbmFtZV1cbiAgfSxcbiAgc2V0OiBmdW5jdGlvbiAodGFyZ2V0LCBuYW1lLCB2YWx1ZSkge1xuICAgIHRhcmdldC5fYXJyW25hbWVdID0gdmFsdWVcbiAgfVxufVxuXG5mdW5jdGlvbiBhdWdtZW50IChhcnIpIHtcbiAgaWYgKGJyb3dzZXJTdXBwb3J0KSB7XG4gICAgYXJyLl9pc0J1ZmZlciA9IHRydWVcblxuICAgIC8vIEF1Z21lbnQgdGhlIFVpbnQ4QXJyYXkgKmluc3RhbmNlKiAobm90IHRoZSBjbGFzcyEpIHdpdGggQnVmZmVyIG1ldGhvZHNcbiAgICBhcnIud3JpdGUgPSBCdWZmZXJXcml0ZVxuICAgIGFyci50b1N0cmluZyA9IEJ1ZmZlclRvU3RyaW5nXG4gICAgYXJyLnRvTG9jYWxlU3RyaW5nID0gQnVmZmVyVG9TdHJpbmdcbiAgICBhcnIudG9KU09OID0gQnVmZmVyVG9KU09OXG4gICAgYXJyLmNvcHkgPSBCdWZmZXJDb3B5XG4gICAgYXJyLnNsaWNlID0gQnVmZmVyU2xpY2VcbiAgICBhcnIucmVhZFVJbnQ4ID0gQnVmZmVyUmVhZFVJbnQ4XG4gICAgYXJyLnJlYWRVSW50MTZMRSA9IEJ1ZmZlclJlYWRVSW50MTZMRVxuICAgIGFyci5yZWFkVUludDE2QkUgPSBCdWZmZXJSZWFkVUludDE2QkVcbiAgICBhcnIucmVhZFVJbnQzMkxFID0gQnVmZmVyUmVhZFVJbnQzMkxFXG4gICAgYXJyLnJlYWRVSW50MzJCRSA9IEJ1ZmZlclJlYWRVSW50MzJCRVxuICAgIGFyci5yZWFkSW50OCA9IEJ1ZmZlclJlYWRJbnQ4XG4gICAgYXJyLnJlYWRJbnQxNkxFID0gQnVmZmVyUmVhZEludDE2TEVcbiAgICBhcnIucmVhZEludDE2QkUgPSBCdWZmZXJSZWFkSW50MTZCRVxuICAgIGFyci5yZWFkSW50MzJMRSA9IEJ1ZmZlclJlYWRJbnQzMkxFXG4gICAgYXJyLnJlYWRJbnQzMkJFID0gQnVmZmVyUmVhZEludDMyQkVcbiAgICBhcnIucmVhZEZsb2F0TEUgPSBCdWZmZXJSZWFkRmxvYXRMRVxuICAgIGFyci5yZWFkRmxvYXRCRSA9IEJ1ZmZlclJlYWRGbG9hdEJFXG4gICAgYXJyLnJlYWREb3VibGVMRSA9IEJ1ZmZlclJlYWREb3VibGVMRVxuICAgIGFyci5yZWFkRG91YmxlQkUgPSBCdWZmZXJSZWFkRG91YmxlQkVcbiAgICBhcnIud3JpdGVVSW50OCA9IEJ1ZmZlcldyaXRlVUludDhcbiAgICBhcnIud3JpdGVVSW50MTZMRSA9IEJ1ZmZlcldyaXRlVUludDE2TEVcbiAgICBhcnIud3JpdGVVSW50MTZCRSA9IEJ1ZmZlcldyaXRlVUludDE2QkVcbiAgICBhcnIud3JpdGVVSW50MzJMRSA9IEJ1ZmZlcldyaXRlVUludDMyTEVcbiAgICBhcnIud3JpdGVVSW50MzJCRSA9IEJ1ZmZlcldyaXRlVUludDMyQkVcbiAgICBhcnIud3JpdGVJbnQ4ID0gQnVmZmVyV3JpdGVJbnQ4XG4gICAgYXJyLndyaXRlSW50MTZMRSA9IEJ1ZmZlcldyaXRlSW50MTZMRVxuICAgIGFyci53cml0ZUludDE2QkUgPSBCdWZmZXJXcml0ZUludDE2QkVcbiAgICBhcnIud3JpdGVJbnQzMkxFID0gQnVmZmVyV3JpdGVJbnQzMkxFXG4gICAgYXJyLndyaXRlSW50MzJCRSA9IEJ1ZmZlcldyaXRlSW50MzJCRVxuICAgIGFyci53cml0ZUZsb2F0TEUgPSBCdWZmZXJXcml0ZUZsb2F0TEVcbiAgICBhcnIud3JpdGVGbG9hdEJFID0gQnVmZmVyV3JpdGVGbG9hdEJFXG4gICAgYXJyLndyaXRlRG91YmxlTEUgPSBCdWZmZXJXcml0ZURvdWJsZUxFXG4gICAgYXJyLndyaXRlRG91YmxlQkUgPSBCdWZmZXJXcml0ZURvdWJsZUJFXG4gICAgYXJyLmZpbGwgPSBCdWZmZXJGaWxsXG4gICAgYXJyLmluc3BlY3QgPSBCdWZmZXJJbnNwZWN0XG5cbiAgICAvLyBPbmx5IGFkZCBgdG9BcnJheUJ1ZmZlcmAgaWYgdGhlIGJyb3dzZXIgc3VwcG9ydHMgQXJyYXlCdWZmZXIgbmF0aXZlbHlcbiAgICBpZiAoeFVpbnQ4QXJyYXkgIT09IFRBLlVpbnQ4QXJyYXkpXG4gICAgICBhcnIudG9BcnJheUJ1ZmZlciA9IEJ1ZmZlclRvQXJyYXlCdWZmZXJcblxuICAgIGlmIChhcnIuYnl0ZUxlbmd0aCAhPT0gMClcbiAgICAgIGFyci5fZGF0YXZpZXcgPSBuZXcgeERhdGFWaWV3KGFyci5idWZmZXIsIGFyci5ieXRlT2Zmc2V0LCBhcnIuYnl0ZUxlbmd0aClcblxuICAgIHJldHVybiBhcnJcblxuICB9IGVsc2Uge1xuICAgIC8vIFRoaXMgaXMgYSBicm93c2VyIHRoYXQgZG9lc24ndCBzdXBwb3J0IGF1Z21lbnRpbmcgdGhlIGBVaW50OEFycmF5YFxuICAgIC8vIGluc3RhbmNlICgqYWhlbSogRmlyZWZveCkgc28gdXNlIGFuIEVTNiBgUHJveHlgLlxuICAgIHZhciBwcm94eUJ1ZmZlciA9IG5ldyBQcm94eUJ1ZmZlcihhcnIpXG4gICAgdmFyIHByb3h5ID0gbmV3IFByb3h5KHByb3h5QnVmZmVyLCBQcm94eUhhbmRsZXIpXG4gICAgcHJveHlCdWZmZXIuX3Byb3h5ID0gcHJveHlcbiAgICByZXR1cm4gcHJveHlcbiAgfVxufVxuXG4vLyBzbGljZShzdGFydCwgZW5kKVxuZnVuY3Rpb24gY2xhbXAgKGluZGV4LCBsZW4sIGRlZmF1bHRWYWx1ZSkge1xuICBpZiAodHlwZW9mIGluZGV4ICE9PSAnbnVtYmVyJykgcmV0dXJuIGRlZmF1bHRWYWx1ZVxuICBpbmRleCA9IH5+aW5kZXg7ICAvLyBDb2VyY2UgdG8gaW50ZWdlci5cbiAgaWYgKGluZGV4ID49IGxlbikgcmV0dXJuIGxlblxuICBpZiAoaW5kZXggPj0gMCkgcmV0dXJuIGluZGV4XG4gIGluZGV4ICs9IGxlblxuICBpZiAoaW5kZXggPj0gMCkgcmV0dXJuIGluZGV4XG4gIHJldHVybiAwXG59XG5cbmZ1bmN0aW9uIGNvZXJjZSAobGVuZ3RoKSB7XG4gIC8vIENvZXJjZSBsZW5ndGggdG8gYSBudW1iZXIgKHBvc3NpYmx5IE5hTiksIHJvdW5kIHVwXG4gIC8vIGluIGNhc2UgaXQncyBmcmFjdGlvbmFsIChlLmcuIDEyMy40NTYpIHRoZW4gZG8gYVxuICAvLyBkb3VibGUgbmVnYXRlIHRvIGNvZXJjZSBhIE5hTiB0byAwLiBFYXN5LCByaWdodD9cbiAgbGVuZ3RoID0gfn5NYXRoLmNlaWwoK2xlbmd0aClcbiAgcmV0dXJuIGxlbmd0aCA8IDAgPyAwIDogbGVuZ3RoXG59XG5cbmZ1bmN0aW9uIGlzQXJyYXkgKHN1YmplY3QpIHtcbiAgcmV0dXJuIChBcnJheS5pc0FycmF5IHx8IGZ1bmN0aW9uIChzdWJqZWN0KSB7XG4gICAgcmV0dXJuIE9iamVjdC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbChzdWJqZWN0KSA9PT0gJ1tvYmplY3QgQXJyYXldJ1xuICB9KShzdWJqZWN0KVxufVxuXG5mdW5jdGlvbiBpc0FycmF5SXNoIChzdWJqZWN0KSB7XG4gIHJldHVybiBpc0FycmF5KHN1YmplY3QpIHx8IEJ1ZmZlci5pc0J1ZmZlcihzdWJqZWN0KSB8fFxuICAgICAgc3ViamVjdCAmJiB0eXBlb2Ygc3ViamVjdCA9PT0gJ29iamVjdCcgJiZcbiAgICAgIHR5cGVvZiBzdWJqZWN0Lmxlbmd0aCA9PT0gJ251bWJlcidcbn1cblxuZnVuY3Rpb24gdG9IZXggKG4pIHtcbiAgaWYgKG4gPCAxNikgcmV0dXJuICcwJyArIG4udG9TdHJpbmcoMTYpXG4gIHJldHVybiBuLnRvU3RyaW5nKDE2KVxufVxuXG5mdW5jdGlvbiB1dGY4VG9CeXRlcyAoc3RyKSB7XG4gIHZhciBieXRlQXJyYXkgPSBbXVxuICBmb3IgKHZhciBpID0gMDsgaSA8IHN0ci5sZW5ndGg7IGkrKylcbiAgICBpZiAoc3RyLmNoYXJDb2RlQXQoaSkgPD0gMHg3RilcbiAgICAgIGJ5dGVBcnJheS5wdXNoKHN0ci5jaGFyQ29kZUF0KGkpKVxuICAgIGVsc2Uge1xuICAgICAgdmFyIGggPSBlbmNvZGVVUklDb21wb25lbnQoc3RyLmNoYXJBdChpKSkuc3Vic3RyKDEpLnNwbGl0KCclJylcbiAgICAgIGZvciAodmFyIGogPSAwOyBqIDwgaC5sZW5ndGg7IGorKylcbiAgICAgICAgYnl0ZUFycmF5LnB1c2gocGFyc2VJbnQoaFtqXSwgMTYpKVxuICAgIH1cblxuICByZXR1cm4gYnl0ZUFycmF5XG59XG5cbmZ1bmN0aW9uIGFzY2lpVG9CeXRlcyAoc3RyKSB7XG4gIHZhciBieXRlQXJyYXkgPSBbXVxuICBmb3IgKHZhciBpID0gMDsgaSA8IHN0ci5sZW5ndGg7IGkrKykge1xuICAgIC8vIE5vZGUncyBjb2RlIHNlZW1zIHRvIGJlIGRvaW5nIHRoaXMgYW5kIG5vdCAmIDB4N0YuLlxuICAgIGJ5dGVBcnJheS5wdXNoKHN0ci5jaGFyQ29kZUF0KGkpICYgMHhGRilcbiAgfVxuXG4gIHJldHVybiBieXRlQXJyYXlcbn1cblxuZnVuY3Rpb24gYmFzZTY0VG9CeXRlcyAoc3RyKSB7XG4gIHJldHVybiBiYXNlNjQudG9CeXRlQXJyYXkoc3RyKVxufVxuXG5mdW5jdGlvbiBibGl0QnVmZmVyIChzcmMsIGRzdCwgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIHBvc1xuICBmb3IgKHZhciBpID0gMDsgaSA8IGxlbmd0aDsgaSsrKSB7XG4gICAgaWYgKChpICsgb2Zmc2V0ID49IGRzdC5sZW5ndGgpIHx8IChpID49IHNyYy5sZW5ndGgpKVxuICAgICAgYnJlYWtcbiAgICBkc3RbaSArIG9mZnNldF0gPSBzcmNbaV1cbiAgfVxuICByZXR1cm4gaVxufVxuXG5mdW5jdGlvbiBkZWNvZGVVdGY4Q2hhciAoc3RyKSB7XG4gIHRyeSB7XG4gICAgcmV0dXJuIGRlY29kZVVSSUNvbXBvbmVudChzdHIpXG4gIH0gY2F0Y2ggKGVycikge1xuICAgIHJldHVybiBTdHJpbmcuZnJvbUNoYXJDb2RlKDB4RkZGRCkgLy8gVVRGIDggaW52YWxpZCBjaGFyXG4gIH1cbn1cblxuLypcbiAqIFdlIGhhdmUgdG8gbWFrZSBzdXJlIHRoYXQgdGhlIHZhbHVlIGlzIGEgdmFsaWQgaW50ZWdlci4gVGhpcyBtZWFucyB0aGF0IGl0XG4gKiBpcyBub24tbmVnYXRpdmUuIEl0IGhhcyBubyBmcmFjdGlvbmFsIGNvbXBvbmVudCBhbmQgdGhhdCBpdCBkb2VzIG5vdFxuICogZXhjZWVkIHRoZSBtYXhpbXVtIGFsbG93ZWQgdmFsdWUuXG4gKlxuICogICAgICB2YWx1ZSAgICAgICAgICAgVGhlIG51bWJlciB0byBjaGVjayBmb3IgdmFsaWRpdHlcbiAqXG4gKiAgICAgIG1heCAgICAgICAgICAgICBUaGUgbWF4aW11bSB2YWx1ZVxuICovXG5mdW5jdGlvbiB2ZXJpZnVpbnQgKHZhbHVlLCBtYXgpIHtcbiAgYXNzZXJ0KHR5cGVvZiAodmFsdWUpID09ICdudW1iZXInLCAnY2Fubm90IHdyaXRlIGEgbm9uLW51bWJlciBhcyBhIG51bWJlcicpXG4gIGFzc2VydCh2YWx1ZSA+PSAwLFxuICAgICAgJ3NwZWNpZmllZCBhIG5lZ2F0aXZlIHZhbHVlIGZvciB3cml0aW5nIGFuIHVuc2lnbmVkIHZhbHVlJylcbiAgYXNzZXJ0KHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGlzIGxhcmdlciB0aGFuIG1heGltdW0gdmFsdWUgZm9yIHR5cGUnKVxuICBhc3NlcnQoTWF0aC5mbG9vcih2YWx1ZSkgPT09IHZhbHVlLCAndmFsdWUgaGFzIGEgZnJhY3Rpb25hbCBjb21wb25lbnQnKVxufVxuXG4vKlxuICogQSBzZXJpZXMgb2YgY2hlY2tzIHRvIG1ha2Ugc3VyZSB3ZSBhY3R1YWxseSBoYXZlIGEgc2lnbmVkIDMyLWJpdCBudW1iZXJcbiAqL1xuZnVuY3Rpb24gdmVyaWZzaW50KHZhbHVlLCBtYXgsIG1pbikge1xuICBhc3NlcnQodHlwZW9mICh2YWx1ZSkgPT0gJ251bWJlcicsICdjYW5ub3Qgd3JpdGUgYSBub24tbnVtYmVyIGFzIGEgbnVtYmVyJylcbiAgYXNzZXJ0KHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGxhcmdlciB0aGFuIG1heGltdW0gYWxsb3dlZCB2YWx1ZScpXG4gIGFzc2VydCh2YWx1ZSA+PSBtaW4sICd2YWx1ZSBzbWFsbGVyIHRoYW4gbWluaW11bSBhbGxvd2VkIHZhbHVlJylcbiAgYXNzZXJ0KE1hdGguZmxvb3IodmFsdWUpID09PSB2YWx1ZSwgJ3ZhbHVlIGhhcyBhIGZyYWN0aW9uYWwgY29tcG9uZW50Jylcbn1cblxuZnVuY3Rpb24gdmVyaWZJRUVFNzU0KHZhbHVlLCBtYXgsIG1pbikge1xuICBhc3NlcnQodHlwZW9mICh2YWx1ZSkgPT0gJ251bWJlcicsICdjYW5ub3Qgd3JpdGUgYSBub24tbnVtYmVyIGFzIGEgbnVtYmVyJylcbiAgYXNzZXJ0KHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGxhcmdlciB0aGFuIG1heGltdW0gYWxsb3dlZCB2YWx1ZScpXG4gIGFzc2VydCh2YWx1ZSA+PSBtaW4sICd2YWx1ZSBzbWFsbGVyIHRoYW4gbWluaW11bSBhbGxvd2VkIHZhbHVlJylcbn1cblxuZnVuY3Rpb24gYXNzZXJ0ICh0ZXN0LCBtZXNzYWdlKSB7XG4gIGlmICghdGVzdCkgdGhyb3cgbmV3IEVycm9yKG1lc3NhZ2UgfHwgJ0ZhaWxlZCBhc3NlcnRpb24nKVxufVxuIiwiKGZ1bmN0aW9uIChleHBvcnRzKSB7XG5cdCd1c2Ugc3RyaWN0JztcblxuXHR2YXIgbG9va3VwID0gJ0FCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaYWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5Ky8nO1xuXG5cdGZ1bmN0aW9uIGI2NFRvQnl0ZUFycmF5KGI2NCkge1xuXHRcdHZhciBpLCBqLCBsLCB0bXAsIHBsYWNlSG9sZGVycywgYXJyO1xuXHRcblx0XHRpZiAoYjY0Lmxlbmd0aCAlIDQgPiAwKSB7XG5cdFx0XHR0aHJvdyAnSW52YWxpZCBzdHJpbmcuIExlbmd0aCBtdXN0IGJlIGEgbXVsdGlwbGUgb2YgNCc7XG5cdFx0fVxuXG5cdFx0Ly8gdGhlIG51bWJlciBvZiBlcXVhbCBzaWducyAocGxhY2UgaG9sZGVycylcblx0XHQvLyBpZiB0aGVyZSBhcmUgdHdvIHBsYWNlaG9sZGVycywgdGhhbiB0aGUgdHdvIGNoYXJhY3RlcnMgYmVmb3JlIGl0XG5cdFx0Ly8gcmVwcmVzZW50IG9uZSBieXRlXG5cdFx0Ly8gaWYgdGhlcmUgaXMgb25seSBvbmUsIHRoZW4gdGhlIHRocmVlIGNoYXJhY3RlcnMgYmVmb3JlIGl0IHJlcHJlc2VudCAyIGJ5dGVzXG5cdFx0Ly8gdGhpcyBpcyBqdXN0IGEgY2hlYXAgaGFjayB0byBub3QgZG8gaW5kZXhPZiB0d2ljZVxuXHRcdHBsYWNlSG9sZGVycyA9IGluZGV4T2YoYjY0LCAnPScpO1xuXHRcdHBsYWNlSG9sZGVycyA9IHBsYWNlSG9sZGVycyA+IDAgPyBiNjQubGVuZ3RoIC0gcGxhY2VIb2xkZXJzIDogMDtcblxuXHRcdC8vIGJhc2U2NCBpcyA0LzMgKyB1cCB0byB0d28gY2hhcmFjdGVycyBvZiB0aGUgb3JpZ2luYWwgZGF0YVxuXHRcdGFyciA9IFtdOy8vbmV3IFVpbnQ4QXJyYXkoYjY0Lmxlbmd0aCAqIDMgLyA0IC0gcGxhY2VIb2xkZXJzKTtcblxuXHRcdC8vIGlmIHRoZXJlIGFyZSBwbGFjZWhvbGRlcnMsIG9ubHkgZ2V0IHVwIHRvIHRoZSBsYXN0IGNvbXBsZXRlIDQgY2hhcnNcblx0XHRsID0gcGxhY2VIb2xkZXJzID4gMCA/IGI2NC5sZW5ndGggLSA0IDogYjY0Lmxlbmd0aDtcblxuXHRcdGZvciAoaSA9IDAsIGogPSAwOyBpIDwgbDsgaSArPSA0LCBqICs9IDMpIHtcblx0XHRcdHRtcCA9IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpKSkgPDwgMTgpIHwgKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkgKyAxKSkgPDwgMTIpIHwgKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkgKyAyKSkgPDwgNikgfCBpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpICsgMykpO1xuXHRcdFx0YXJyLnB1c2goKHRtcCAmIDB4RkYwMDAwKSA+PiAxNik7XG5cdFx0XHRhcnIucHVzaCgodG1wICYgMHhGRjAwKSA+PiA4KTtcblx0XHRcdGFyci5wdXNoKHRtcCAmIDB4RkYpO1xuXHRcdH1cblxuXHRcdGlmIChwbGFjZUhvbGRlcnMgPT09IDIpIHtcblx0XHRcdHRtcCA9IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpKSkgPDwgMikgfCAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSArIDEpKSA+PiA0KTtcblx0XHRcdGFyci5wdXNoKHRtcCAmIDB4RkYpO1xuXHRcdH0gZWxzZSBpZiAocGxhY2VIb2xkZXJzID09PSAxKSB7XG5cdFx0XHR0bXAgPSAoaW5kZXhPZihsb29rdXAsIGI2NC5jaGFyQXQoaSkpIDw8IDEwKSB8IChpbmRleE9mKGxvb2t1cCwgYjY0LmNoYXJBdChpICsgMSkpIDw8IDQpIHwgKGluZGV4T2YobG9va3VwLCBiNjQuY2hhckF0KGkgKyAyKSkgPj4gMik7XG5cdFx0XHRhcnIucHVzaCgodG1wID4+IDgpICYgMHhGRik7XG5cdFx0XHRhcnIucHVzaCh0bXAgJiAweEZGKTtcblx0XHR9XG5cblx0XHRyZXR1cm4gYXJyO1xuXHR9XG5cblx0ZnVuY3Rpb24gdWludDhUb0Jhc2U2NCh1aW50OCkge1xuXHRcdHZhciBpLFxuXHRcdFx0ZXh0cmFCeXRlcyA9IHVpbnQ4Lmxlbmd0aCAlIDMsIC8vIGlmIHdlIGhhdmUgMSBieXRlIGxlZnQsIHBhZCAyIGJ5dGVzXG5cdFx0XHRvdXRwdXQgPSBcIlwiLFxuXHRcdFx0dGVtcCwgbGVuZ3RoO1xuXG5cdFx0ZnVuY3Rpb24gdHJpcGxldFRvQmFzZTY0IChudW0pIHtcblx0XHRcdHJldHVybiBsb29rdXAuY2hhckF0KG51bSA+PiAxOCAmIDB4M0YpICsgbG9va3VwLmNoYXJBdChudW0gPj4gMTIgJiAweDNGKSArIGxvb2t1cC5jaGFyQXQobnVtID4+IDYgJiAweDNGKSArIGxvb2t1cC5jaGFyQXQobnVtICYgMHgzRik7XG5cdFx0fTtcblxuXHRcdC8vIGdvIHRocm91Z2ggdGhlIGFycmF5IGV2ZXJ5IHRocmVlIGJ5dGVzLCB3ZSdsbCBkZWFsIHdpdGggdHJhaWxpbmcgc3R1ZmYgbGF0ZXJcblx0XHRmb3IgKGkgPSAwLCBsZW5ndGggPSB1aW50OC5sZW5ndGggLSBleHRyYUJ5dGVzOyBpIDwgbGVuZ3RoOyBpICs9IDMpIHtcblx0XHRcdHRlbXAgPSAodWludDhbaV0gPDwgMTYpICsgKHVpbnQ4W2kgKyAxXSA8PCA4KSArICh1aW50OFtpICsgMl0pO1xuXHRcdFx0b3V0cHV0ICs9IHRyaXBsZXRUb0Jhc2U2NCh0ZW1wKTtcblx0XHR9XG5cblx0XHQvLyBwYWQgdGhlIGVuZCB3aXRoIHplcm9zLCBidXQgbWFrZSBzdXJlIHRvIG5vdCBmb3JnZXQgdGhlIGV4dHJhIGJ5dGVzXG5cdFx0c3dpdGNoIChleHRyYUJ5dGVzKSB7XG5cdFx0XHRjYXNlIDE6XG5cdFx0XHRcdHRlbXAgPSB1aW50OFt1aW50OC5sZW5ndGggLSAxXTtcblx0XHRcdFx0b3V0cHV0ICs9IGxvb2t1cC5jaGFyQXQodGVtcCA+PiAyKTtcblx0XHRcdFx0b3V0cHV0ICs9IGxvb2t1cC5jaGFyQXQoKHRlbXAgPDwgNCkgJiAweDNGKTtcblx0XHRcdFx0b3V0cHV0ICs9ICc9PSc7XG5cdFx0XHRcdGJyZWFrO1xuXHRcdFx0Y2FzZSAyOlxuXHRcdFx0XHR0ZW1wID0gKHVpbnQ4W3VpbnQ4Lmxlbmd0aCAtIDJdIDw8IDgpICsgKHVpbnQ4W3VpbnQ4Lmxlbmd0aCAtIDFdKTtcblx0XHRcdFx0b3V0cHV0ICs9IGxvb2t1cC5jaGFyQXQodGVtcCA+PiAxMCk7XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXAuY2hhckF0KCh0ZW1wID4+IDQpICYgMHgzRik7XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXAuY2hhckF0KCh0ZW1wIDw8IDIpICYgMHgzRik7XG5cdFx0XHRcdG91dHB1dCArPSAnPSc7XG5cdFx0XHRcdGJyZWFrO1xuXHRcdH1cblxuXHRcdHJldHVybiBvdXRwdXQ7XG5cdH1cblxuXHRtb2R1bGUuZXhwb3J0cy50b0J5dGVBcnJheSA9IGI2NFRvQnl0ZUFycmF5O1xuXHRtb2R1bGUuZXhwb3J0cy5mcm9tQnl0ZUFycmF5ID0gdWludDhUb0Jhc2U2NDtcbn0oKSk7XG5cbmZ1bmN0aW9uIGluZGV4T2YgKGFyciwgZWx0IC8qLCBmcm9tKi8pIHtcblx0dmFyIGxlbiA9IGFyci5sZW5ndGg7XG5cblx0dmFyIGZyb20gPSBOdW1iZXIoYXJndW1lbnRzWzFdKSB8fCAwO1xuXHRmcm9tID0gKGZyb20gPCAwKVxuXHRcdD8gTWF0aC5jZWlsKGZyb20pXG5cdFx0OiBNYXRoLmZsb29yKGZyb20pO1xuXHRpZiAoZnJvbSA8IDApXG5cdFx0ZnJvbSArPSBsZW47XG5cblx0Zm9yICg7IGZyb20gPCBsZW47IGZyb20rKykge1xuXHRcdGlmICgodHlwZW9mIGFyciA9PT0gJ3N0cmluZycgJiYgYXJyLmNoYXJBdChmcm9tKSA9PT0gZWx0KSB8fFxuXHRcdFx0XHQodHlwZW9mIGFyciAhPT0gJ3N0cmluZycgJiYgYXJyW2Zyb21dID09PSBlbHQpKSB7XG5cdFx0XHRyZXR1cm4gZnJvbTtcblx0XHR9XG5cdH1cblx0cmV0dXJuIC0xO1xufVxuIiwidmFyIHByb2Nlc3M9cmVxdWlyZShcIl9fYnJvd3NlcmlmeV9wcm9jZXNzXCIpLGdsb2JhbD10eXBlb2Ygc2VsZiAhPT0gXCJ1bmRlZmluZWRcIiA/IHNlbGYgOiB0eXBlb2Ygd2luZG93ICE9PSBcInVuZGVmaW5lZFwiID8gd2luZG93IDoge307LyohXG4gKiBCZW5jaG1hcmsuanMgdjEuMC4wIDxodHRwOi8vYmVuY2htYXJranMuY29tLz5cbiAqIENvcHlyaWdodCAyMDEwLTIwMTIgTWF0aGlhcyBCeW5lbnMgPGh0dHA6Ly9tdGhzLmJlLz5cbiAqIEJhc2VkIG9uIEpTTGl0bXVzLmpzLCBjb3B5cmlnaHQgUm9iZXJ0IEtpZWZmZXIgPGh0dHA6Ly9icm9vZmEuY29tLz5cbiAqIE1vZGlmaWVkIGJ5IEpvaG4tRGF2aWQgRGFsdG9uIDxodHRwOi8vYWxseW91Y2FubGVldC5jb20vPlxuICogQXZhaWxhYmxlIHVuZGVyIE1JVCBsaWNlbnNlIDxodHRwOi8vbXRocy5iZS9taXQ+XG4gKi9cbjsoZnVuY3Rpb24od2luZG93LCB1bmRlZmluZWQpIHtcbiAgJ3VzZSBzdHJpY3QnO1xuXG4gIC8qKiBVc2VkIHRvIGFzc2lnbiBlYWNoIGJlbmNobWFyayBhbiBpbmNyaW1lbnRlZCBpZCAqL1xuICB2YXIgY291bnRlciA9IDA7XG5cbiAgLyoqIERldGVjdCBET00gZG9jdW1lbnQgb2JqZWN0ICovXG4gIHZhciBkb2MgPSBpc0hvc3RUeXBlKHdpbmRvdywgJ2RvY3VtZW50JykgJiYgZG9jdW1lbnQ7XG5cbiAgLyoqIERldGVjdCBmcmVlIHZhcmlhYmxlIGBkZWZpbmVgICovXG4gIHZhciBmcmVlRGVmaW5lID0gdHlwZW9mIGRlZmluZSA9PSAnZnVuY3Rpb24nICYmXG4gICAgdHlwZW9mIGRlZmluZS5hbWQgPT0gJ29iamVjdCcgJiYgZGVmaW5lLmFtZCAmJiBkZWZpbmU7XG5cbiAgLyoqIERldGVjdCBmcmVlIHZhcmlhYmxlIGBleHBvcnRzYCAqL1xuICB2YXIgZnJlZUV4cG9ydHMgPSB0eXBlb2YgZXhwb3J0cyA9PSAnb2JqZWN0JyAmJiBleHBvcnRzICYmXG4gICAgKHR5cGVvZiBnbG9iYWwgPT0gJ29iamVjdCcgJiYgZ2xvYmFsICYmIGdsb2JhbCA9PSBnbG9iYWwuZ2xvYmFsICYmICh3aW5kb3cgPSBnbG9iYWwpLCBleHBvcnRzKTtcblxuICAvKiogRGV0ZWN0IGZyZWUgdmFyaWFibGUgYHJlcXVpcmVgICovXG4gIHZhciBmcmVlUmVxdWlyZSA9IHR5cGVvZiByZXF1aXJlID09ICdmdW5jdGlvbicgJiYgcmVxdWlyZTtcblxuICAvKiogVXNlZCB0byBjcmF3bCBhbGwgcHJvcGVydGllcyByZWdhcmRsZXNzIG9mIGVudW1lcmFiaWxpdHkgKi9cbiAgdmFyIGdldEFsbEtleXMgPSBPYmplY3QuZ2V0T3duUHJvcGVydHlOYW1lcztcblxuICAvKiogVXNlZCB0byBnZXQgcHJvcGVydHkgZGVzY3JpcHRvcnMgKi9cbiAgdmFyIGdldERlc2NyaXB0b3IgPSBPYmplY3QuZ2V0T3duUHJvcGVydHlEZXNjcmlwdG9yO1xuXG4gIC8qKiBVc2VkIGluIGNhc2UgYW4gb2JqZWN0IGRvZXNuJ3QgaGF2ZSBpdHMgb3duIG1ldGhvZCAqL1xuICB2YXIgaGFzT3duUHJvcGVydHkgPSB7fS5oYXNPd25Qcm9wZXJ0eTtcblxuICAvKiogVXNlZCB0byBjaGVjayBpZiBhbiBvYmplY3QgaXMgZXh0ZW5zaWJsZSAqL1xuICB2YXIgaXNFeHRlbnNpYmxlID0gT2JqZWN0LmlzRXh0ZW5zaWJsZSB8fCBmdW5jdGlvbigpIHsgcmV0dXJuIHRydWU7IH07XG5cbiAgLyoqIFVzZWQgdG8gYWNjZXNzIFdhZGUgU2ltbW9ucycgTm9kZSBtaWNyb3RpbWUgbW9kdWxlICovXG4gIHZhciBtaWNyb3RpbWVPYmplY3QgPSByZXEoJ21pY3JvdGltZScpO1xuXG4gIC8qKiBVc2VkIHRvIGFjY2VzcyB0aGUgYnJvd3NlcidzIGhpZ2ggcmVzb2x1dGlvbiB0aW1lciAqL1xuICB2YXIgcGVyZk9iamVjdCA9IGlzSG9zdFR5cGUod2luZG93LCAncGVyZm9ybWFuY2UnKSAmJiBwZXJmb3JtYW5jZTtcblxuICAvKiogVXNlZCB0byBjYWxsIHRoZSBicm93c2VyJ3MgaGlnaCByZXNvbHV0aW9uIHRpbWVyICovXG4gIHZhciBwZXJmTmFtZSA9IHBlcmZPYmplY3QgJiYgKFxuICAgIHBlcmZPYmplY3Qubm93ICYmICdub3cnIHx8XG4gICAgcGVyZk9iamVjdC53ZWJraXROb3cgJiYgJ3dlYmtpdE5vdydcbiAgKTtcblxuICAvKiogVXNlZCB0byBhY2Nlc3MgTm9kZSdzIGhpZ2ggcmVzb2x1dGlvbiB0aW1lciAqL1xuICB2YXIgcHJvY2Vzc09iamVjdCA9IGlzSG9zdFR5cGUod2luZG93LCAncHJvY2VzcycpICYmIHByb2Nlc3M7XG5cbiAgLyoqIFVzZWQgdG8gY2hlY2sgaWYgYW4gb3duIHByb3BlcnR5IGlzIGVudW1lcmFibGUgKi9cbiAgdmFyIHByb3BlcnR5SXNFbnVtZXJhYmxlID0ge30ucHJvcGVydHlJc0VudW1lcmFibGU7XG5cbiAgLyoqIFVzZWQgdG8gc2V0IHByb3BlcnR5IGRlc2NyaXB0b3JzICovXG4gIHZhciBzZXREZXNjcmlwdG9yID0gT2JqZWN0LmRlZmluZVByb3BlcnR5O1xuXG4gIC8qKiBVc2VkIHRvIHJlc29sdmUgYSB2YWx1ZSdzIGludGVybmFsIFtbQ2xhc3NdXSAqL1xuICB2YXIgdG9TdHJpbmcgPSB7fS50b1N0cmluZztcblxuICAvKiogVXNlZCB0byBwcmV2ZW50IGEgYHJlbW92ZUNoaWxkYCBtZW1vcnkgbGVhayBpbiBJRSA8IDkgKi9cbiAgdmFyIHRyYXNoID0gZG9jICYmIGRvYy5jcmVhdGVFbGVtZW50KCdkaXYnKTtcblxuICAvKiogVXNlZCB0byBpbnRlZ3JpdHkgY2hlY2sgY29tcGlsZWQgdGVzdHMgKi9cbiAgdmFyIHVpZCA9ICd1aWQnICsgKCtuZXcgRGF0ZSk7XG5cbiAgLyoqIFVzZWQgdG8gYXZvaWQgaW5maW5pdGUgcmVjdXJzaW9uIHdoZW4gbWV0aG9kcyBjYWxsIGVhY2ggb3RoZXIgKi9cbiAgdmFyIGNhbGxlZEJ5ID0ge307XG5cbiAgLyoqIFVzZWQgdG8gYXZvaWQgaHogb2YgSW5maW5pdHkgKi9cbiAgdmFyIGRpdmlzb3JzID0ge1xuICAgICcxJzogNDA5NixcbiAgICAnMic6IDUxMixcbiAgICAnMyc6IDY0LFxuICAgICc0JzogOCxcbiAgICAnNSc6IDBcbiAgfTtcblxuICAvKipcbiAgICogVC1EaXN0cmlidXRpb24gdHdvLXRhaWxlZCBjcml0aWNhbCB2YWx1ZXMgZm9yIDk1JSBjb25maWRlbmNlXG4gICAqIGh0dHA6Ly93d3cuaXRsLm5pc3QuZ292L2Rpdjg5OC9oYW5kYm9vay9lZGEvc2VjdGlvbjMvZWRhMzY3Mi5odG1cbiAgICovXG4gIHZhciB0VGFibGUgPSB7XG4gICAgJzEnOiAgMTIuNzA2LCcyJzogIDQuMzAzLCAnMyc6ICAzLjE4MiwgJzQnOiAgMi43NzYsICc1JzogIDIuNTcxLCAnNic6ICAyLjQ0NyxcbiAgICAnNyc6ICAyLjM2NSwgJzgnOiAgMi4zMDYsICc5JzogIDIuMjYyLCAnMTAnOiAyLjIyOCwgJzExJzogMi4yMDEsICcxMic6IDIuMTc5LFxuICAgICcxMyc6IDIuMTYsICAnMTQnOiAyLjE0NSwgJzE1JzogMi4xMzEsICcxNic6IDIuMTIsICAnMTcnOiAyLjExLCAgJzE4JzogMi4xMDEsXG4gICAgJzE5JzogMi4wOTMsICcyMCc6IDIuMDg2LCAnMjEnOiAyLjA4LCAgJzIyJzogMi4wNzQsICcyMyc6IDIuMDY5LCAnMjQnOiAyLjA2NCxcbiAgICAnMjUnOiAyLjA2LCAgJzI2JzogMi4wNTYsICcyNyc6IDIuMDUyLCAnMjgnOiAyLjA0OCwgJzI5JzogMi4wNDUsICczMCc6IDIuMDQyLFxuICAgICdpbmZpbml0eSc6IDEuOTZcbiAgfTtcblxuICAvKipcbiAgICogQ3JpdGljYWwgTWFubi1XaGl0bmV5IFUtdmFsdWVzIGZvciA5NSUgY29uZmlkZW5jZVxuICAgKiBodHRwOi8vd3d3LnNhYnVyY2hpbGwuY29tL0lCYmlvbG9neS9zdGF0cy8wMDMuaHRtbFxuICAgKi9cbiAgdmFyIHVUYWJsZSA9IHtcbiAgICAnNSc6ICBbMCwgMSwgMl0sXG4gICAgJzYnOiAgWzEsIDIsIDMsIDVdLFxuICAgICc3JzogIFsxLCAzLCA1LCA2LCA4XSxcbiAgICAnOCc6ICBbMiwgNCwgNiwgOCwgMTAsIDEzXSxcbiAgICAnOSc6ICBbMiwgNCwgNywgMTAsIDEyLCAxNSwgMTddLFxuICAgICcxMCc6IFszLCA1LCA4LCAxMSwgMTQsIDE3LCAyMCwgMjNdLFxuICAgICcxMSc6IFszLCA2LCA5LCAxMywgMTYsIDE5LCAyMywgMjYsIDMwXSxcbiAgICAnMTInOiBbNCwgNywgMTEsIDE0LCAxOCwgMjIsIDI2LCAyOSwgMzMsIDM3XSxcbiAgICAnMTMnOiBbNCwgOCwgMTIsIDE2LCAyMCwgMjQsIDI4LCAzMywgMzcsIDQxLCA0NV0sXG4gICAgJzE0JzogWzUsIDksIDEzLCAxNywgMjIsIDI2LCAzMSwgMzYsIDQwLCA0NSwgNTAsIDU1XSxcbiAgICAnMTUnOiBbNSwgMTAsIDE0LCAxOSwgMjQsIDI5LCAzNCwgMzksIDQ0LCA0OSwgNTQsIDU5LCA2NF0sXG4gICAgJzE2JzogWzYsIDExLCAxNSwgMjEsIDI2LCAzMSwgMzcsIDQyLCA0NywgNTMsIDU5LCA2NCwgNzAsIDc1XSxcbiAgICAnMTcnOiBbNiwgMTEsIDE3LCAyMiwgMjgsIDM0LCAzOSwgNDUsIDUxLCA1NywgNjMsIDY3LCA3NSwgODEsIDg3XSxcbiAgICAnMTgnOiBbNywgMTIsIDE4LCAyNCwgMzAsIDM2LCA0MiwgNDgsIDU1LCA2MSwgNjcsIDc0LCA4MCwgODYsIDkzLCA5OV0sXG4gICAgJzE5JzogWzcsIDEzLCAxOSwgMjUsIDMyLCAzOCwgNDUsIDUyLCA1OCwgNjUsIDcyLCA3OCwgODUsIDkyLCA5OSwgMTA2LCAxMTNdLFxuICAgICcyMCc6IFs4LCAxNCwgMjAsIDI3LCAzNCwgNDEsIDQ4LCA1NSwgNjIsIDY5LCA3NiwgODMsIDkwLCA5OCwgMTA1LCAxMTIsIDExOSwgMTI3XSxcbiAgICAnMjEnOiBbOCwgMTUsIDIyLCAyOSwgMzYsIDQzLCA1MCwgNTgsIDY1LCA3MywgODAsIDg4LCA5NiwgMTAzLCAxMTEsIDExOSwgMTI2LCAxMzQsIDE0Ml0sXG4gICAgJzIyJzogWzksIDE2LCAyMywgMzAsIDM4LCA0NSwgNTMsIDYxLCA2OSwgNzcsIDg1LCA5MywgMTAxLCAxMDksIDExNywgMTI1LCAxMzMsIDE0MSwgMTUwLCAxNThdLFxuICAgICcyMyc6IFs5LCAxNywgMjQsIDMyLCA0MCwgNDgsIDU2LCA2NCwgNzMsIDgxLCA4OSwgOTgsIDEwNiwgMTE1LCAxMjMsIDEzMiwgMTQwLCAxNDksIDE1NywgMTY2LCAxNzVdLFxuICAgICcyNCc6IFsxMCwgMTcsIDI1LCAzMywgNDIsIDUwLCA1OSwgNjcsIDc2LCA4NSwgOTQsIDEwMiwgMTExLCAxMjAsIDEyOSwgMTM4LCAxNDcsIDE1NiwgMTY1LCAxNzQsIDE4MywgMTkyXSxcbiAgICAnMjUnOiBbMTAsIDE4LCAyNywgMzUsIDQ0LCA1MywgNjIsIDcxLCA4MCwgODksIDk4LCAxMDcsIDExNywgMTI2LCAxMzUsIDE0NSwgMTU0LCAxNjMsIDE3MywgMTgyLCAxOTIsIDIwMSwgMjExXSxcbiAgICAnMjYnOiBbMTEsIDE5LCAyOCwgMzcsIDQ2LCA1NSwgNjQsIDc0LCA4MywgOTMsIDEwMiwgMTEyLCAxMjIsIDEzMiwgMTQxLCAxNTEsIDE2MSwgMTcxLCAxODEsIDE5MSwgMjAwLCAyMTAsIDIyMCwgMjMwXSxcbiAgICAnMjcnOiBbMTEsIDIwLCAyOSwgMzgsIDQ4LCA1NywgNjcsIDc3LCA4NywgOTcsIDEwNywgMTE4LCAxMjUsIDEzOCwgMTQ3LCAxNTgsIDE2OCwgMTc4LCAxODgsIDE5OSwgMjA5LCAyMTksIDIzMCwgMjQwLCAyNTBdLFxuICAgICcyOCc6IFsxMiwgMjEsIDMwLCA0MCwgNTAsIDYwLCA3MCwgODAsIDkwLCAxMDEsIDExMSwgMTIyLCAxMzIsIDE0MywgMTU0LCAxNjQsIDE3NSwgMTg2LCAxOTYsIDIwNywgMjE4LCAyMjgsIDIzOSwgMjUwLCAyNjEsIDI3Ml0sXG4gICAgJzI5JzogWzEzLCAyMiwgMzIsIDQyLCA1MiwgNjIsIDczLCA4MywgOTQsIDEwNSwgMTE2LCAxMjcsIDEzOCwgMTQ5LCAxNjAsIDE3MSwgMTgyLCAxOTMsIDIwNCwgMjE1LCAyMjYsIDIzOCwgMjQ5LCAyNjAsIDI3MSwgMjgyLCAyOTRdLFxuICAgICczMCc6IFsxMywgMjMsIDMzLCA0MywgNTQsIDY1LCA3NiwgODcsIDk4LCAxMDksIDEyMCwgMTMxLCAxNDMsIDE1NCwgMTY2LCAxNzcsIDE4OSwgMjAwLCAyMTIsIDIyMywgMjM1LCAyNDcsIDI1OCwgMjcwLCAyODIsIDI5MywgMzA1LCAzMTddXG4gIH07XG5cbiAgLyoqXG4gICAqIEFuIG9iamVjdCB1c2VkIHRvIGZsYWcgZW52aXJvbm1lbnRzL2ZlYXR1cmVzLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHR5cGUgT2JqZWN0XG4gICAqL1xuICB2YXIgc3VwcG9ydCA9IHt9O1xuXG4gIChmdW5jdGlvbigpIHtcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBBZG9iZSBBSVIuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnN1cHBvcnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgc3VwcG9ydC5haXIgPSBpc0NsYXNzT2Yod2luZG93LnJ1bnRpbWUsICdTY3JpcHRCcmlkZ2luZ1Byb3h5T2JqZWN0Jyk7XG5cbiAgICAvKipcbiAgICAgKiBEZXRlY3QgaWYgYGFyZ3VtZW50c2Agb2JqZWN0cyBoYXZlIHRoZSBjb3JyZWN0IGludGVybmFsIFtbQ2xhc3NdXSB2YWx1ZS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LmFyZ3VtZW50c0NsYXNzID0gaXNDbGFzc09mKGFyZ3VtZW50cywgJ0FyZ3VtZW50cycpO1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIGluIGEgYnJvd3NlciBlbnZpcm9ubWVudC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LmJyb3dzZXIgPSBkb2MgJiYgaXNIb3N0VHlwZSh3aW5kb3csICduYXZpZ2F0b3InKTtcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBpZiBzdHJpbmdzIHN1cHBvcnQgYWNjZXNzaW5nIGNoYXJhY3RlcnMgYnkgaW5kZXguXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnN1cHBvcnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgc3VwcG9ydC5jaGFyQnlJbmRleCA9XG4gICAgICAvLyBJRSA4IHN1cHBvcnRzIGluZGV4ZXMgb24gc3RyaW5nIGxpdGVyYWxzIGJ1dCBub3Qgc3RyaW5nIG9iamVjdHNcbiAgICAgICgneCdbMF0gKyBPYmplY3QoJ3gnKVswXSkgPT0gJ3h4JztcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBpZiBzdHJpbmdzIGhhdmUgaW5kZXhlcyBhcyBvd24gcHJvcGVydGllcy5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0LmNoYXJCeU93bkluZGV4ID1cbiAgICAgIC8vIE5hcndoYWwsIFJoaW5vLCBSaW5nb0pTLCBJRSA4LCBhbmQgT3BlcmEgPCAxMC41MiBzdXBwb3J0IGluZGV4ZXMgb25cbiAgICAgIC8vIHN0cmluZ3MgYnV0IGRvbid0IGRldGVjdCB0aGVtIGFzIG93biBwcm9wZXJ0aWVzXG4gICAgICBzdXBwb3J0LmNoYXJCeUluZGV4ICYmIGhhc0tleSgneCcsICcwJyk7XG5cbiAgICAvKipcbiAgICAgKiBEZXRlY3QgaWYgSmF2YSBpcyBlbmFibGVkL2V4cG9zZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnN1cHBvcnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgc3VwcG9ydC5qYXZhID0gaXNDbGFzc09mKHdpbmRvdy5qYXZhLCAnSmF2YVBhY2thZ2UnKTtcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBpZiB0aGUgVGltZXJzIEFQSSBleGlzdHMuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnN1cHBvcnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgc3VwcG9ydC50aW1lb3V0ID0gaXNIb3N0VHlwZSh3aW5kb3csICdzZXRUaW1lb3V0JykgJiYgaXNIb3N0VHlwZSh3aW5kb3csICdjbGVhclRpbWVvdXQnKTtcblxuICAgIC8qKlxuICAgICAqIERldGVjdCBpZiBmdW5jdGlvbnMgc3VwcG9ydCBkZWNvbXBpbGF0aW9uLlxuICAgICAqXG4gICAgICogQG5hbWUgZGVjb21waWxhdGlvblxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICB0cnkge1xuICAgICAgLy8gU2FmYXJpIDIueCByZW1vdmVzIGNvbW1hcyBpbiBvYmplY3QgbGl0ZXJhbHNcbiAgICAgIC8vIGZyb20gRnVuY3Rpb24jdG9TdHJpbmcgcmVzdWx0c1xuICAgICAgLy8gaHR0cDovL3dlYmsuaXQvMTE2MDlcbiAgICAgIC8vIEZpcmVmb3ggMy42IGFuZCBPcGVyYSA5LjI1IHN0cmlwIGdyb3VwaW5nXG4gICAgICAvLyBwYXJlbnRoZXNlcyBmcm9tIEZ1bmN0aW9uI3RvU3RyaW5nIHJlc3VsdHNcbiAgICAgIC8vIGh0dHA6Ly9idWd6aWwubGEvNTU5NDM4XG4gICAgICBzdXBwb3J0LmRlY29tcGlsYXRpb24gPSBGdW5jdGlvbihcbiAgICAgICAgJ3JldHVybiAoJyArIChmdW5jdGlvbih4KSB7IHJldHVybiB7ICd4JzogJycgKyAoMSArIHgpICsgJycsICd5JzogMCB9OyB9KSArICcpJ1xuICAgICAgKSgpKDApLnggPT09ICcxJztcbiAgICB9IGNhdGNoKGUpIHtcbiAgICAgIHN1cHBvcnQuZGVjb21waWxhdGlvbiA9IGZhbHNlO1xuICAgIH1cblxuICAgIC8qKlxuICAgICAqIERldGVjdCBFUzUrIHByb3BlcnR5IGRlc2NyaXB0b3IgQVBJLlxuICAgICAqXG4gICAgICogQG5hbWUgZGVzY3JpcHRvcnNcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnN1cHBvcnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgdHJ5IHtcbiAgICAgIHZhciBvID0ge307XG4gICAgICBzdXBwb3J0LmRlc2NyaXB0b3JzID0gKHNldERlc2NyaXB0b3IobywgbywgbyksICd2YWx1ZScgaW4gZ2V0RGVzY3JpcHRvcihvLCBvKSk7XG4gICAgfSBjYXRjaChlKSB7XG4gICAgICBzdXBwb3J0LmRlc2NyaXB0b3JzID0gZmFsc2U7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IEVTNSsgT2JqZWN0LmdldE93blByb3BlcnR5TmFtZXMoKS5cbiAgICAgKlxuICAgICAqIEBuYW1lIGdldEFsbEtleXNcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnN1cHBvcnRcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgdHJ5IHtcbiAgICAgIHN1cHBvcnQuZ2V0QWxsS2V5cyA9IC9cXGJ2YWx1ZU9mXFxiLy50ZXN0KGdldEFsbEtleXMoT2JqZWN0LnByb3RvdHlwZSkpO1xuICAgIH0gY2F0Y2goZSkge1xuICAgICAgc3VwcG9ydC5nZXRBbGxLZXlzID0gZmFsc2U7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIG93biBwcm9wZXJ0aWVzIGFyZSBpdGVyYXRlZCBiZWZvcmUgaW5oZXJpdGVkIHByb3BlcnRpZXMgKGFsbCBidXQgSUUgPCA5KS5cbiAgICAgKlxuICAgICAqIEBuYW1lIGl0ZXJhdGVzT3duTGFzdFxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICBzdXBwb3J0Lml0ZXJhdGVzT3duRmlyc3QgPSAoZnVuY3Rpb24oKSB7XG4gICAgICB2YXIgcHJvcHMgPSBbXTtcbiAgICAgIGZ1bmN0aW9uIGN0b3IoKSB7IHRoaXMueCA9IDE7IH1cbiAgICAgIGN0b3IucHJvdG90eXBlID0geyAneSc6IDEgfTtcbiAgICAgIGZvciAodmFyIHByb3AgaW4gbmV3IGN0b3IpIHsgcHJvcHMucHVzaChwcm9wKTsgfVxuICAgICAgcmV0dXJuIHByb3BzWzBdID09ICd4JztcbiAgICB9KCkpO1xuXG4gICAgLyoqXG4gICAgICogRGV0ZWN0IGlmIGEgbm9kZSdzIFtbQ2xhc3NdXSBpcyByZXNvbHZhYmxlIChhbGwgYnV0IElFIDwgOSlcbiAgICAgKiBhbmQgdGhhdCB0aGUgSlMgZW5naW5lIGVycm9ycyB3aGVuIGF0dGVtcHRpbmcgdG8gY29lcmNlIGFuIG9iamVjdCB0byBhXG4gICAgICogc3RyaW5nIHdpdGhvdXQgYSBgdG9TdHJpbmdgIHByb3BlcnR5IHZhbHVlIG9mIGB0eXBlb2ZgIFwiZnVuY3Rpb25cIi5cbiAgICAgKlxuICAgICAqIEBuYW1lIG5vZGVDbGFzc1xuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuc3VwcG9ydFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICB0cnkge1xuICAgICAgc3VwcG9ydC5ub2RlQ2xhc3MgPSAoeyAndG9TdHJpbmcnOiAwIH0gKyAnJywgdG9TdHJpbmcuY2FsbChkb2MgfHwgMCkgIT0gJ1tvYmplY3QgT2JqZWN0XScpO1xuICAgIH0gY2F0Y2goZSkge1xuICAgICAgc3VwcG9ydC5ub2RlQ2xhc3MgPSB0cnVlO1xuICAgIH1cbiAgfSgpKTtcblxuICAvKipcbiAgICogVGltZXIgb2JqZWN0IHVzZWQgYnkgYGNsb2NrKClgIGFuZCBgRGVmZXJyZWQjcmVzb2x2ZWAuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEB0eXBlIE9iamVjdFxuICAgKi9cbiAgdmFyIHRpbWVyID0ge1xuXG4gICAvKipcbiAgICAqIFRoZSB0aW1lciBuYW1lc3BhY2Ugb2JqZWN0IG9yIGNvbnN0cnVjdG9yLlxuICAgICpcbiAgICAqIEBwcml2YXRlXG4gICAgKiBAbWVtYmVyT2YgdGltZXJcbiAgICAqIEB0eXBlIEZ1bmN0aW9ufE9iamVjdFxuICAgICovXG4gICAgJ25zJzogRGF0ZSxcblxuICAgLyoqXG4gICAgKiBTdGFydHMgdGhlIGRlZmVycmVkIHRpbWVyLlxuICAgICpcbiAgICAqIEBwcml2YXRlXG4gICAgKiBAbWVtYmVyT2YgdGltZXJcbiAgICAqIEBwYXJhbSB7T2JqZWN0fSBkZWZlcnJlZCBUaGUgZGVmZXJyZWQgaW5zdGFuY2UuXG4gICAgKi9cbiAgICAnc3RhcnQnOiBudWxsLCAvLyBsYXp5IGRlZmluZWQgaW4gYGNsb2NrKClgXG5cbiAgIC8qKlxuICAgICogU3RvcHMgdGhlIGRlZmVycmVkIHRpbWVyLlxuICAgICpcbiAgICAqIEBwcml2YXRlXG4gICAgKiBAbWVtYmVyT2YgdGltZXJcbiAgICAqIEBwYXJhbSB7T2JqZWN0fSBkZWZlcnJlZCBUaGUgZGVmZXJyZWQgaW5zdGFuY2UuXG4gICAgKi9cbiAgICAnc3RvcCc6IG51bGwgLy8gbGF6eSBkZWZpbmVkIGluIGBjbG9jaygpYFxuICB9O1xuXG4gIC8qKiBTaG9ydGN1dCBmb3IgaW52ZXJzZSByZXN1bHRzICovXG4gIHZhciBub0FyZ3VtZW50c0NsYXNzID0gIXN1cHBvcnQuYXJndW1lbnRzQ2xhc3MsXG4gICAgICBub0NoYXJCeUluZGV4ID0gIXN1cHBvcnQuY2hhckJ5SW5kZXgsXG4gICAgICBub0NoYXJCeU93bkluZGV4ID0gIXN1cHBvcnQuY2hhckJ5T3duSW5kZXg7XG5cbiAgLyoqIE1hdGggc2hvcnRjdXRzICovXG4gIHZhciBhYnMgICA9IE1hdGguYWJzLFxuICAgICAgZmxvb3IgPSBNYXRoLmZsb29yLFxuICAgICAgbWF4ICAgPSBNYXRoLm1heCxcbiAgICAgIG1pbiAgID0gTWF0aC5taW4sXG4gICAgICBwb3cgICA9IE1hdGgucG93LFxuICAgICAgc3FydCAgPSBNYXRoLnNxcnQ7XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIFRoZSBCZW5jaG1hcmsgY29uc3RydWN0b3IuXG4gICAqXG4gICAqIEBjb25zdHJ1Y3RvclxuICAgKiBAcGFyYW0ge1N0cmluZ30gbmFtZSBBIG5hbWUgdG8gaWRlbnRpZnkgdGhlIGJlbmNobWFyay5cbiAgICogQHBhcmFtIHtGdW5jdGlvbnxTdHJpbmd9IGZuIFRoZSB0ZXN0IHRvIGJlbmNobWFyay5cbiAgICogQHBhcmFtIHtPYmplY3R9IFtvcHRpb25zPXt9XSBPcHRpb25zIG9iamVjdC5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gYmFzaWMgdXNhZ2UgKHRoZSBgbmV3YCBvcGVyYXRvciBpcyBvcHRpb25hbClcbiAgICogdmFyIGJlbmNoID0gbmV3IEJlbmNobWFyayhmbik7XG4gICAqXG4gICAqIC8vIG9yIHVzaW5nIGEgbmFtZSBmaXJzdFxuICAgKiB2YXIgYmVuY2ggPSBuZXcgQmVuY2htYXJrKCdmb28nLCBmbik7XG4gICAqXG4gICAqIC8vIG9yIHdpdGggb3B0aW9uc1xuICAgKiB2YXIgYmVuY2ggPSBuZXcgQmVuY2htYXJrKCdmb28nLCBmbiwge1xuICAgKlxuICAgKiAgIC8vIGRpc3BsYXllZCBieSBCZW5jaG1hcmsjdG9TdHJpbmcgaWYgYG5hbWVgIGlzIG5vdCBhdmFpbGFibGVcbiAgICogICAnaWQnOiAneHl6JyxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiB0aGUgYmVuY2htYXJrIHN0YXJ0cyBydW5uaW5nXG4gICAqICAgJ29uU3RhcnQnOiBvblN0YXJ0LFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCBhZnRlciBlYWNoIHJ1biBjeWNsZVxuICAgKiAgICdvbkN5Y2xlJzogb25DeWNsZSxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiBhYm9ydGVkXG4gICAqICAgJ29uQWJvcnQnOiBvbkFib3J0LFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIGEgdGVzdCBlcnJvcnNcbiAgICogICAnb25FcnJvcic6IG9uRXJyb3IsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIHdoZW4gcmVzZXRcbiAgICogICAnb25SZXNldCc6IG9uUmVzZXQsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIHdoZW4gdGhlIGJlbmNobWFyayBjb21wbGV0ZXMgcnVubmluZ1xuICAgKiAgICdvbkNvbXBsZXRlJzogb25Db21wbGV0ZSxcbiAgICpcbiAgICogICAvLyBjb21waWxlZC9jYWxsZWQgYmVmb3JlIHRoZSB0ZXN0IGxvb3BcbiAgICogICAnc2V0dXAnOiBzZXR1cCxcbiAgICpcbiAgICogICAvLyBjb21waWxlZC9jYWxsZWQgYWZ0ZXIgdGhlIHRlc3QgbG9vcFxuICAgKiAgICd0ZWFyZG93bic6IHRlYXJkb3duXG4gICAqIH0pO1xuICAgKlxuICAgKiAvLyBvciBuYW1lIGFuZCBvcHRpb25zXG4gICAqIHZhciBiZW5jaCA9IG5ldyBCZW5jaG1hcmsoJ2ZvbycsIHtcbiAgICpcbiAgICogICAvLyBhIGZsYWcgdG8gaW5kaWNhdGUgdGhlIGJlbmNobWFyayBpcyBkZWZlcnJlZFxuICAgKiAgICdkZWZlcic6IHRydWUsXG4gICAqXG4gICAqICAgLy8gYmVuY2htYXJrIHRlc3QgZnVuY3Rpb25cbiAgICogICAnZm4nOiBmdW5jdGlvbihkZWZlcnJlZCkge1xuICAgKiAgICAgLy8gY2FsbCByZXNvbHZlKCkgd2hlbiB0aGUgZGVmZXJyZWQgdGVzdCBpcyBmaW5pc2hlZFxuICAgKiAgICAgZGVmZXJyZWQucmVzb2x2ZSgpO1xuICAgKiAgIH1cbiAgICogfSk7XG4gICAqXG4gICAqIC8vIG9yIG9wdGlvbnMgb25seVxuICAgKiB2YXIgYmVuY2ggPSBuZXcgQmVuY2htYXJrKHtcbiAgICpcbiAgICogICAvLyBiZW5jaG1hcmsgbmFtZVxuICAgKiAgICduYW1lJzogJ2ZvbycsXG4gICAqXG4gICAqICAgLy8gYmVuY2htYXJrIHRlc3QgYXMgYSBzdHJpbmdcbiAgICogICAnZm4nOiAnWzEsMiwzLDRdLnNvcnQoKSdcbiAgICogfSk7XG4gICAqXG4gICAqIC8vIGEgdGVzdCdzIGB0aGlzYCBiaW5kaW5nIGlzIHNldCB0byB0aGUgYmVuY2htYXJrIGluc3RhbmNlXG4gICAqIHZhciBiZW5jaCA9IG5ldyBCZW5jaG1hcmsoJ2ZvbycsIGZ1bmN0aW9uKCkge1xuICAgKiAgICdNeSBuYW1lIGlzICcuY29uY2F0KHRoaXMubmFtZSk7IC8vIE15IG5hbWUgaXMgZm9vXG4gICAqIH0pO1xuICAgKi9cbiAgZnVuY3Rpb24gQmVuY2htYXJrKG5hbWUsIGZuLCBvcHRpb25zKSB7XG4gICAgdmFyIG1lID0gdGhpcztcblxuICAgIC8vIGFsbG93IGluc3RhbmNlIGNyZWF0aW9uIHdpdGhvdXQgdGhlIGBuZXdgIG9wZXJhdG9yXG4gICAgaWYgKG1lID09IG51bGwgfHwgbWUuY29uc3RydWN0b3IgIT0gQmVuY2htYXJrKSB7XG4gICAgICByZXR1cm4gbmV3IEJlbmNobWFyayhuYW1lLCBmbiwgb3B0aW9ucyk7XG4gICAgfVxuICAgIC8vIGp1Z2dsZSBhcmd1bWVudHNcbiAgICBpZiAoaXNDbGFzc09mKG5hbWUsICdPYmplY3QnKSkge1xuICAgICAgLy8gMSBhcmd1bWVudCAob3B0aW9ucylcbiAgICAgIG9wdGlvbnMgPSBuYW1lO1xuICAgIH1cbiAgICBlbHNlIGlmIChpc0NsYXNzT2YobmFtZSwgJ0Z1bmN0aW9uJykpIHtcbiAgICAgIC8vIDIgYXJndW1lbnRzIChmbiwgb3B0aW9ucylcbiAgICAgIG9wdGlvbnMgPSBmbjtcbiAgICAgIGZuID0gbmFtZTtcbiAgICB9XG4gICAgZWxzZSBpZiAoaXNDbGFzc09mKGZuLCAnT2JqZWN0JykpIHtcbiAgICAgIC8vIDIgYXJndW1lbnRzIChuYW1lLCBvcHRpb25zKVxuICAgICAgb3B0aW9ucyA9IGZuO1xuICAgICAgZm4gPSBudWxsO1xuICAgICAgbWUubmFtZSA9IG5hbWU7XG4gICAgfVxuICAgIGVsc2Uge1xuICAgICAgLy8gMyBhcmd1bWVudHMgKG5hbWUsIGZuIFssIG9wdGlvbnNdKVxuICAgICAgbWUubmFtZSA9IG5hbWU7XG4gICAgfVxuICAgIHNldE9wdGlvbnMobWUsIG9wdGlvbnMpO1xuICAgIG1lLmlkIHx8IChtZS5pZCA9ICsrY291bnRlcik7XG4gICAgbWUuZm4gPT0gbnVsbCAmJiAobWUuZm4gPSBmbik7XG4gICAgbWUuc3RhdHMgPSBkZWVwQ2xvbmUobWUuc3RhdHMpO1xuICAgIG1lLnRpbWVzID0gZGVlcENsb25lKG1lLnRpbWVzKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBUaGUgRGVmZXJyZWQgY29uc3RydWN0b3IuXG4gICAqXG4gICAqIEBjb25zdHJ1Y3RvclxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBjbG9uZSBUaGUgY2xvbmVkIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICovXG4gIGZ1bmN0aW9uIERlZmVycmVkKGNsb25lKSB7XG4gICAgdmFyIG1lID0gdGhpcztcbiAgICBpZiAobWUgPT0gbnVsbCB8fCBtZS5jb25zdHJ1Y3RvciAhPSBEZWZlcnJlZCkge1xuICAgICAgcmV0dXJuIG5ldyBEZWZlcnJlZChjbG9uZSk7XG4gICAgfVxuICAgIG1lLmJlbmNobWFyayA9IGNsb25lO1xuICAgIGNsb2NrKG1lKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBUaGUgRXZlbnQgY29uc3RydWN0b3IuXG4gICAqXG4gICAqIEBjb25zdHJ1Y3RvclxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7U3RyaW5nfE9iamVjdH0gdHlwZSBUaGUgZXZlbnQgdHlwZS5cbiAgICovXG4gIGZ1bmN0aW9uIEV2ZW50KHR5cGUpIHtcbiAgICB2YXIgbWUgPSB0aGlzO1xuICAgIHJldHVybiAobWUgPT0gbnVsbCB8fCBtZS5jb25zdHJ1Y3RvciAhPSBFdmVudClcbiAgICAgID8gbmV3IEV2ZW50KHR5cGUpXG4gICAgICA6ICh0eXBlIGluc3RhbmNlb2YgRXZlbnQpXG4gICAgICAgICAgPyB0eXBlXG4gICAgICAgICAgOiBleHRlbmQobWUsIHsgJ3RpbWVTdGFtcCc6ICtuZXcgRGF0ZSB9LCB0eXBlb2YgdHlwZSA9PSAnc3RyaW5nJyA/IHsgJ3R5cGUnOiB0eXBlIH0gOiB0eXBlKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBUaGUgU3VpdGUgY29uc3RydWN0b3IuXG4gICAqXG4gICAqIEBjb25zdHJ1Y3RvclxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBuYW1lIEEgbmFtZSB0byBpZGVudGlmeSB0aGUgc3VpdGUuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBbb3B0aW9ucz17fV0gT3B0aW9ucyBvYmplY3QuXG4gICAqIEBleGFtcGxlXG4gICAqXG4gICAqIC8vIGJhc2ljIHVzYWdlICh0aGUgYG5ld2Agb3BlcmF0b3IgaXMgb3B0aW9uYWwpXG4gICAqIHZhciBzdWl0ZSA9IG5ldyBCZW5jaG1hcmsuU3VpdGU7XG4gICAqXG4gICAqIC8vIG9yIHVzaW5nIGEgbmFtZSBmaXJzdFxuICAgKiB2YXIgc3VpdGUgPSBuZXcgQmVuY2htYXJrLlN1aXRlKCdmb28nKTtcbiAgICpcbiAgICogLy8gb3Igd2l0aCBvcHRpb25zXG4gICAqIHZhciBzdWl0ZSA9IG5ldyBCZW5jaG1hcmsuU3VpdGUoJ2ZvbycsIHtcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiB0aGUgc3VpdGUgc3RhcnRzIHJ1bm5pbmdcbiAgICogICAnb25TdGFydCc6IG9uU3RhcnQsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIGJldHdlZW4gcnVubmluZyBiZW5jaG1hcmtzXG4gICAqICAgJ29uQ3ljbGUnOiBvbkN5Y2xlLFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCB3aGVuIGFib3J0ZWRcbiAgICogICAnb25BYm9ydCc6IG9uQWJvcnQsXG4gICAqXG4gICAqICAgLy8gY2FsbGVkIHdoZW4gYSB0ZXN0IGVycm9yc1xuICAgKiAgICdvbkVycm9yJzogb25FcnJvcixcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiByZXNldFxuICAgKiAgICdvblJlc2V0Jzogb25SZXNldCxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgd2hlbiB0aGUgc3VpdGUgY29tcGxldGVzIHJ1bm5pbmdcbiAgICogICAnb25Db21wbGV0ZSc6IG9uQ29tcGxldGVcbiAgICogfSk7XG4gICAqL1xuICBmdW5jdGlvbiBTdWl0ZShuYW1lLCBvcHRpb25zKSB7XG4gICAgdmFyIG1lID0gdGhpcztcblxuICAgIC8vIGFsbG93IGluc3RhbmNlIGNyZWF0aW9uIHdpdGhvdXQgdGhlIGBuZXdgIG9wZXJhdG9yXG4gICAgaWYgKG1lID09IG51bGwgfHwgbWUuY29uc3RydWN0b3IgIT0gU3VpdGUpIHtcbiAgICAgIHJldHVybiBuZXcgU3VpdGUobmFtZSwgb3B0aW9ucyk7XG4gICAgfVxuICAgIC8vIGp1Z2dsZSBhcmd1bWVudHNcbiAgICBpZiAoaXNDbGFzc09mKG5hbWUsICdPYmplY3QnKSkge1xuICAgICAgLy8gMSBhcmd1bWVudCAob3B0aW9ucylcbiAgICAgIG9wdGlvbnMgPSBuYW1lO1xuICAgIH0gZWxzZSB7XG4gICAgICAvLyAyIGFyZ3VtZW50cyAobmFtZSBbLCBvcHRpb25zXSlcbiAgICAgIG1lLm5hbWUgPSBuYW1lO1xuICAgIH1cbiAgICBzZXRPcHRpb25zKG1lLCBvcHRpb25zKTtcbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBOb3RlOiBTb21lIGFycmF5IG1ldGhvZHMgaGF2ZSBiZWVuIGltcGxlbWVudGVkIGluIHBsYWluIEphdmFTY3JpcHQgdG8gYXZvaWRcbiAgICogYnVncyBpbiBJRSwgT3BlcmEsIFJoaW5vLCBhbmQgTW9iaWxlIFNhZmFyaS5cbiAgICpcbiAgICogSUUgY29tcGF0aWJpbGl0eSBtb2RlIGFuZCBJRSA8IDkgaGF2ZSBidWdneSBBcnJheSBgc2hpZnQoKWAgYW5kIGBzcGxpY2UoKWBcbiAgICogZnVuY3Rpb25zIHRoYXQgZmFpbCB0byByZW1vdmUgdGhlIGxhc3QgZWxlbWVudCwgYG9iamVjdFswXWAsIG9mXG4gICAqIGFycmF5LWxpa2Utb2JqZWN0cyBldmVuIHRob3VnaCB0aGUgYGxlbmd0aGAgcHJvcGVydHkgaXMgc2V0IHRvIGAwYC5cbiAgICogVGhlIGBzaGlmdCgpYCBtZXRob2QgaXMgYnVnZ3kgaW4gSUUgOCBjb21wYXRpYmlsaXR5IG1vZGUsIHdoaWxlIGBzcGxpY2UoKWBcbiAgICogaXMgYnVnZ3kgcmVnYXJkbGVzcyBvZiBtb2RlIGluIElFIDwgOSBhbmQgYnVnZ3kgaW4gY29tcGF0aWJpbGl0eSBtb2RlIGluIElFIDkuXG4gICAqXG4gICAqIEluIE9wZXJhIDwgOS41MCBhbmQgc29tZSBvbGRlci9iZXRhIE1vYmlsZSBTYWZhcmkgdmVyc2lvbnMgdXNpbmcgYHVuc2hpZnQoKWBcbiAgICogZ2VuZXJpY2FsbHkgdG8gYXVnbWVudCB0aGUgYGFyZ3VtZW50c2Agb2JqZWN0IHdpbGwgcGF2ZSB0aGUgdmFsdWUgYXQgaW5kZXggMFxuICAgKiB3aXRob3V0IGluY3JpbWVudGluZyB0aGUgb3RoZXIgdmFsdWVzJ3MgaW5kZXhlcy5cbiAgICogaHR0cHM6Ly9naXRodWIuY29tL2RvY3VtZW50Y2xvdWQvdW5kZXJzY29yZS9pc3N1ZXMvOVxuICAgKlxuICAgKiBSaGlubyBhbmQgZW52aXJvbm1lbnRzIGl0IHBvd2VycywgbGlrZSBOYXJ3aGFsIGFuZCBSaW5nb0pTLCBtYXkgaGF2ZVxuICAgKiBidWdneSBBcnJheSBgY29uY2F0KClgLCBgcmV2ZXJzZSgpYCwgYHNoaWZ0KClgLCBgc2xpY2UoKWAsIGBzcGxpY2UoKWAgYW5kXG4gICAqIGB1bnNoaWZ0KClgIGZ1bmN0aW9ucyB0aGF0IG1ha2Ugc3BhcnNlIGFycmF5cyBub24tc3BhcnNlIGJ5IGFzc2lnbmluZyB0aGVcbiAgICogdW5kZWZpbmVkIGluZGV4ZXMgYSB2YWx1ZSBvZiB1bmRlZmluZWQuXG4gICAqIGh0dHBzOi8vZ2l0aHViLmNvbS9tb3ppbGxhL3JoaW5vL2NvbW1pdC83MDJhYmZlZDNmOGNhMDQzYjI2MzZlZmQzMWMxNGJhNzU1MjYwM2RkXG4gICAqL1xuXG4gIC8qKlxuICAgKiBDcmVhdGVzIGFuIGFycmF5IGNvbnRhaW5pbmcgdGhlIGVsZW1lbnRzIG9mIHRoZSBob3N0IGFycmF5IGZvbGxvd2VkIGJ5IHRoZVxuICAgKiBlbGVtZW50cyBvZiBlYWNoIGFyZ3VtZW50IGluIG9yZGVyLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEByZXR1cm5zIHtBcnJheX0gVGhlIG5ldyBhcnJheS5cbiAgICovXG4gIGZ1bmN0aW9uIGNvbmNhdCgpIHtcbiAgICB2YXIgdmFsdWUsXG4gICAgICAgIGogPSAtMSxcbiAgICAgICAgbGVuZ3RoID0gYXJndW1lbnRzLmxlbmd0aCxcbiAgICAgICAgcmVzdWx0ID0gc2xpY2UuY2FsbCh0aGlzKSxcbiAgICAgICAgaW5kZXggPSByZXN1bHQubGVuZ3RoO1xuXG4gICAgd2hpbGUgKCsraiA8IGxlbmd0aCkge1xuICAgICAgdmFsdWUgPSBhcmd1bWVudHNbal07XG4gICAgICBpZiAoaXNDbGFzc09mKHZhbHVlLCAnQXJyYXknKSkge1xuICAgICAgICBmb3IgKHZhciBrID0gMCwgbCA9IHZhbHVlLmxlbmd0aDsgayA8IGw7IGsrKywgaW5kZXgrKykge1xuICAgICAgICAgIGlmIChrIGluIHZhbHVlKSB7XG4gICAgICAgICAgICByZXN1bHRbaW5kZXhdID0gdmFsdWVba107XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9IGVsc2Uge1xuICAgICAgICByZXN1bHRbaW5kZXgrK10gPSB2YWx1ZTtcbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBVdGlsaXR5IGZ1bmN0aW9uIHVzZWQgYnkgYHNoaWZ0KClgLCBgc3BsaWNlKClgLCBhbmQgYHVuc2hpZnQoKWAuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7TnVtYmVyfSBzdGFydCBUaGUgaW5kZXggdG8gc3RhcnQgaW5zZXJ0aW5nIGVsZW1lbnRzLlxuICAgKiBAcGFyYW0ge051bWJlcn0gZGVsZXRlQ291bnQgVGhlIG51bWJlciBvZiBlbGVtZW50cyB0byBkZWxldGUgZnJvbSB0aGUgaW5zZXJ0IHBvaW50LlxuICAgKiBAcGFyYW0ge0FycmF5fSBlbGVtZW50cyBUaGUgZWxlbWVudHMgdG8gaW5zZXJ0LlxuICAgKiBAcmV0dXJucyB7QXJyYXl9IEFuIGFycmF5IG9mIGRlbGV0ZWQgZWxlbWVudHMuXG4gICAqL1xuICBmdW5jdGlvbiBpbnNlcnQoc3RhcnQsIGRlbGV0ZUNvdW50LCBlbGVtZW50cykge1xuICAgIC8vIGByZXN1bHRgIHNob3VsZCBoYXZlIGl0cyBsZW5ndGggc2V0IHRvIHRoZSBgZGVsZXRlQ291bnRgXG4gICAgLy8gc2VlIGh0dHBzOi8vYnVncy5lY21hc2NyaXB0Lm9yZy9zaG93X2J1Zy5jZ2k/aWQ9MzMyXG4gICAgdmFyIGRlbGV0ZUVuZCA9IHN0YXJ0ICsgZGVsZXRlQ291bnQsXG4gICAgICAgIGVsZW1lbnRDb3VudCA9IGVsZW1lbnRzID8gZWxlbWVudHMubGVuZ3RoIDogMCxcbiAgICAgICAgaW5kZXggPSBzdGFydCAtIDEsXG4gICAgICAgIGxlbmd0aCA9IHN0YXJ0ICsgZWxlbWVudENvdW50LFxuICAgICAgICBvYmplY3QgPSB0aGlzLFxuICAgICAgICByZXN1bHQgPSBBcnJheShkZWxldGVDb3VudCksXG4gICAgICAgIHRhaWwgPSBzbGljZS5jYWxsKG9iamVjdCwgZGVsZXRlRW5kKTtcblxuICAgIC8vIGRlbGV0ZSBlbGVtZW50cyBmcm9tIHRoZSBhcnJheVxuICAgIHdoaWxlICgrK2luZGV4IDwgZGVsZXRlRW5kKSB7XG4gICAgICBpZiAoaW5kZXggaW4gb2JqZWN0KSB7XG4gICAgICAgIHJlc3VsdFtpbmRleCAtIHN0YXJ0XSA9IG9iamVjdFtpbmRleF07XG4gICAgICAgIGRlbGV0ZSBvYmplY3RbaW5kZXhdO1xuICAgICAgfVxuICAgIH1cbiAgICAvLyBpbnNlcnQgZWxlbWVudHNcbiAgICBpbmRleCA9IHN0YXJ0IC0gMTtcbiAgICB3aGlsZSAoKytpbmRleCA8IGxlbmd0aCkge1xuICAgICAgb2JqZWN0W2luZGV4XSA9IGVsZW1lbnRzW2luZGV4IC0gc3RhcnRdO1xuICAgIH1cbiAgICAvLyBhcHBlbmQgdGFpbCBlbGVtZW50c1xuICAgIHN0YXJ0ID0gaW5kZXgtLTtcbiAgICBsZW5ndGggPSBtYXgoMCwgKG9iamVjdC5sZW5ndGggPj4+IDApIC0gZGVsZXRlQ291bnQgKyBlbGVtZW50Q291bnQpO1xuICAgIHdoaWxlICgrK2luZGV4IDwgbGVuZ3RoKSB7XG4gICAgICBpZiAoKGluZGV4IC0gc3RhcnQpIGluIHRhaWwpIHtcbiAgICAgICAgb2JqZWN0W2luZGV4XSA9IHRhaWxbaW5kZXggLSBzdGFydF07XG4gICAgICB9IGVsc2UgaWYgKGluZGV4IGluIG9iamVjdCkge1xuICAgICAgICBkZWxldGUgb2JqZWN0W2luZGV4XTtcbiAgICAgIH1cbiAgICB9XG4gICAgLy8gZGVsZXRlIGV4Y2VzcyBlbGVtZW50c1xuICAgIGRlbGV0ZUNvdW50ID0gZGVsZXRlQ291bnQgPiBlbGVtZW50Q291bnQgPyBkZWxldGVDb3VudCAtIGVsZW1lbnRDb3VudCA6IDA7XG4gICAgd2hpbGUgKGRlbGV0ZUNvdW50LS0pIHtcbiAgICAgIGluZGV4ID0gbGVuZ3RoICsgZGVsZXRlQ291bnQ7XG4gICAgICBpZiAoaW5kZXggaW4gb2JqZWN0KSB7XG4gICAgICAgIGRlbGV0ZSBvYmplY3RbaW5kZXhdO1xuICAgICAgfVxuICAgIH1cbiAgICBvYmplY3QubGVuZ3RoID0gbGVuZ3RoO1xuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogUmVhcnJhbmdlIHRoZSBob3N0IGFycmF5J3MgZWxlbWVudHMgaW4gcmV2ZXJzZSBvcmRlci5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcmV0dXJucyB7QXJyYXl9IFRoZSByZXZlcnNlZCBhcnJheS5cbiAgICovXG4gIGZ1bmN0aW9uIHJldmVyc2UoKSB7XG4gICAgdmFyIHVwcGVySW5kZXgsXG4gICAgICAgIHZhbHVlLFxuICAgICAgICBpbmRleCA9IC0xLFxuICAgICAgICBvYmplY3QgPSBPYmplY3QodGhpcyksXG4gICAgICAgIGxlbmd0aCA9IG9iamVjdC5sZW5ndGggPj4+IDAsXG4gICAgICAgIG1pZGRsZSA9IGZsb29yKGxlbmd0aCAvIDIpO1xuXG4gICAgaWYgKGxlbmd0aCA+IDEpIHtcbiAgICAgIHdoaWxlICgrK2luZGV4IDwgbWlkZGxlKSB7XG4gICAgICAgIHVwcGVySW5kZXggPSBsZW5ndGggLSBpbmRleCAtIDE7XG4gICAgICAgIHZhbHVlID0gdXBwZXJJbmRleCBpbiBvYmplY3QgPyBvYmplY3RbdXBwZXJJbmRleF0gOiB1aWQ7XG4gICAgICAgIGlmIChpbmRleCBpbiBvYmplY3QpIHtcbiAgICAgICAgICBvYmplY3RbdXBwZXJJbmRleF0gPSBvYmplY3RbaW5kZXhdO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIGRlbGV0ZSBvYmplY3RbdXBwZXJJbmRleF07XG4gICAgICAgIH1cbiAgICAgICAgaWYgKHZhbHVlICE9IHVpZCkge1xuICAgICAgICAgIG9iamVjdFtpbmRleF0gPSB2YWx1ZTtcbiAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICBkZWxldGUgb2JqZWN0W2luZGV4XTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gb2JqZWN0O1xuICB9XG5cbiAgLyoqXG4gICAqIFJlbW92ZXMgdGhlIGZpcnN0IGVsZW1lbnQgb2YgdGhlIGhvc3QgYXJyYXkgYW5kIHJldHVybnMgaXQuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHJldHVybnMge01peGVkfSBUaGUgZmlyc3QgZWxlbWVudCBvZiB0aGUgYXJyYXkuXG4gICAqL1xuICBmdW5jdGlvbiBzaGlmdCgpIHtcbiAgICByZXR1cm4gaW5zZXJ0LmNhbGwodGhpcywgMCwgMSlbMF07XG4gIH1cblxuICAvKipcbiAgICogQ3JlYXRlcyBhbiBhcnJheSBvZiB0aGUgaG9zdCBhcnJheSdzIGVsZW1lbnRzIGZyb20gdGhlIHN0YXJ0IGluZGV4IHVwIHRvLFxuICAgKiBidXQgbm90IGluY2x1ZGluZywgdGhlIGVuZCBpbmRleC5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge051bWJlcn0gc3RhcnQgVGhlIHN0YXJ0aW5nIGluZGV4LlxuICAgKiBAcGFyYW0ge051bWJlcn0gZW5kIFRoZSBlbmQgaW5kZXguXG4gICAqIEByZXR1cm5zIHtBcnJheX0gVGhlIG5ldyBhcnJheS5cbiAgICovXG4gIGZ1bmN0aW9uIHNsaWNlKHN0YXJ0LCBlbmQpIHtcbiAgICB2YXIgaW5kZXggPSAtMSxcbiAgICAgICAgb2JqZWN0ID0gT2JqZWN0KHRoaXMpLFxuICAgICAgICBsZW5ndGggPSBvYmplY3QubGVuZ3RoID4+PiAwLFxuICAgICAgICByZXN1bHQgPSBbXTtcblxuICAgIHN0YXJ0ID0gdG9JbnRlZ2VyKHN0YXJ0KTtcbiAgICBzdGFydCA9IHN0YXJ0IDwgMCA/IG1heChsZW5ndGggKyBzdGFydCwgMCkgOiBtaW4oc3RhcnQsIGxlbmd0aCk7XG4gICAgc3RhcnQtLTtcbiAgICBlbmQgPSBlbmQgPT0gbnVsbCA/IGxlbmd0aCA6IHRvSW50ZWdlcihlbmQpO1xuICAgIGVuZCA9IGVuZCA8IDAgPyBtYXgobGVuZ3RoICsgZW5kLCAwKSA6IG1pbihlbmQsIGxlbmd0aCk7XG5cbiAgICB3aGlsZSAoKCsraW5kZXgsICsrc3RhcnQpIDwgZW5kKSB7XG4gICAgICBpZiAoc3RhcnQgaW4gb2JqZWN0KSB7XG4gICAgICAgIHJlc3VsdFtpbmRleF0gPSBvYmplY3Rbc3RhcnRdO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIEFsbG93cyByZW1vdmluZyBhIHJhbmdlIG9mIGVsZW1lbnRzIGFuZC9vciBpbnNlcnRpbmcgZWxlbWVudHMgaW50byB0aGVcbiAgICogaG9zdCBhcnJheS5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge051bWJlcn0gc3RhcnQgVGhlIHN0YXJ0IGluZGV4LlxuICAgKiBAcGFyYW0ge051bWJlcn0gZGVsZXRlQ291bnQgVGhlIG51bWJlciBvZiBlbGVtZW50cyB0byBkZWxldGUuXG4gICAqIEBwYXJhbSB7TWl4ZWR9IFt2YWwxLCB2YWwyLCAuLi5dIHZhbHVlcyB0byBpbnNlcnQgYXQgdGhlIGBzdGFydGAgaW5kZXguXG4gICAqIEByZXR1cm5zIHtBcnJheX0gQW4gYXJyYXkgb2YgcmVtb3ZlZCBlbGVtZW50cy5cbiAgICovXG4gIGZ1bmN0aW9uIHNwbGljZShzdGFydCwgZGVsZXRlQ291bnQpIHtcbiAgICB2YXIgb2JqZWN0ID0gT2JqZWN0KHRoaXMpLFxuICAgICAgICBsZW5ndGggPSBvYmplY3QubGVuZ3RoID4+PiAwO1xuXG4gICAgc3RhcnQgPSB0b0ludGVnZXIoc3RhcnQpO1xuICAgIHN0YXJ0ID0gc3RhcnQgPCAwID8gbWF4KGxlbmd0aCArIHN0YXJ0LCAwKSA6IG1pbihzdGFydCwgbGVuZ3RoKTtcblxuICAgIC8vIHN1cHBvcnQgdGhlIGRlLWZhY3RvIFNwaWRlck1vbmtleSBleHRlbnNpb25cbiAgICAvLyBodHRwczovL2RldmVsb3Blci5tb3ppbGxhLm9yZy9lbi9KYXZhU2NyaXB0L1JlZmVyZW5jZS9HbG9iYWxfT2JqZWN0cy9BcnJheS9zcGxpY2UjUGFyYW1ldGVyc1xuICAgIC8vIGh0dHBzOi8vYnVncy5lY21hc2NyaXB0Lm9yZy9zaG93X2J1Zy5jZ2k/aWQ9NDI5XG4gICAgZGVsZXRlQ291bnQgPSBhcmd1bWVudHMubGVuZ3RoID09IDFcbiAgICAgID8gbGVuZ3RoIC0gc3RhcnRcbiAgICAgIDogbWluKG1heCh0b0ludGVnZXIoZGVsZXRlQ291bnQpLCAwKSwgbGVuZ3RoIC0gc3RhcnQpO1xuXG4gICAgcmV0dXJuIGluc2VydC5jYWxsKG9iamVjdCwgc3RhcnQsIGRlbGV0ZUNvdW50LCBzbGljZS5jYWxsKGFyZ3VtZW50cywgMikpO1xuICB9XG5cbiAgLyoqXG4gICAqIENvbnZlcnRzIHRoZSBzcGVjaWZpZWQgYHZhbHVlYCB0byBhbiBpbnRlZ2VyLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge01peGVkfSB2YWx1ZSBUaGUgdmFsdWUgdG8gY29udmVydC5cbiAgICogQHJldHVybnMge051bWJlcn0gVGhlIHJlc3VsdGluZyBpbnRlZ2VyLlxuICAgKi9cbiAgZnVuY3Rpb24gdG9JbnRlZ2VyKHZhbHVlKSB7XG4gICAgdmFsdWUgPSArdmFsdWU7XG4gICAgcmV0dXJuIHZhbHVlID09PSAwIHx8ICFpc0Zpbml0ZSh2YWx1ZSkgPyB2YWx1ZSB8fCAwIDogdmFsdWUgLSAodmFsdWUgJSAxKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBBcHBlbmRzIGFyZ3VtZW50cyB0byB0aGUgaG9zdCBhcnJheS5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcmV0dXJucyB7TnVtYmVyfSBUaGUgbmV3IGxlbmd0aC5cbiAgICovXG4gIGZ1bmN0aW9uIHVuc2hpZnQoKSB7XG4gICAgdmFyIG9iamVjdCA9IE9iamVjdCh0aGlzKTtcbiAgICBpbnNlcnQuY2FsbChvYmplY3QsIDAsIDAsIGFyZ3VtZW50cyk7XG4gICAgcmV0dXJuIG9iamVjdC5sZW5ndGg7XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogQSBnZW5lcmljIGBGdW5jdGlvbiNiaW5kYCBsaWtlIG1ldGhvZC5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gZm4gVGhlIGZ1bmN0aW9uIHRvIGJlIGJvdW5kIHRvIGB0aGlzQXJnYC5cbiAgICogQHBhcmFtIHtNaXhlZH0gdGhpc0FyZyBUaGUgYHRoaXNgIGJpbmRpbmcgZm9yIHRoZSBnaXZlbiBmdW5jdGlvbi5cbiAgICogQHJldHVybnMge0Z1bmN0aW9ufSBUaGUgYm91bmQgZnVuY3Rpb24uXG4gICAqL1xuICBmdW5jdGlvbiBiaW5kKGZuLCB0aGlzQXJnKSB7XG4gICAgcmV0dXJuIGZ1bmN0aW9uKCkgeyBmbi5hcHBseSh0aGlzQXJnLCBhcmd1bWVudHMpOyB9O1xuICB9XG5cbiAgLyoqXG4gICAqIENyZWF0ZXMgYSBmdW5jdGlvbiBmcm9tIHRoZSBnaXZlbiBhcmd1bWVudHMgc3RyaW5nIGFuZCBib2R5LlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge1N0cmluZ30gYXJncyBUaGUgY29tbWEgc2VwYXJhdGVkIGZ1bmN0aW9uIGFyZ3VtZW50cy5cbiAgICogQHBhcmFtIHtTdHJpbmd9IGJvZHkgVGhlIGZ1bmN0aW9uIGJvZHkuXG4gICAqIEByZXR1cm5zIHtGdW5jdGlvbn0gVGhlIG5ldyBmdW5jdGlvbi5cbiAgICovXG4gIGZ1bmN0aW9uIGNyZWF0ZUZ1bmN0aW9uKCkge1xuICAgIC8vIGxhenkgZGVmaW5lXG4gICAgY3JlYXRlRnVuY3Rpb24gPSBmdW5jdGlvbihhcmdzLCBib2R5KSB7XG4gICAgICB2YXIgcmVzdWx0LFxuICAgICAgICAgIGFuY2hvciA9IGZyZWVEZWZpbmUgPyBkZWZpbmUuYW1kIDogQmVuY2htYXJrLFxuICAgICAgICAgIHByb3AgPSB1aWQgKyAnY3JlYXRlRnVuY3Rpb24nO1xuXG4gICAgICBydW5TY3JpcHQoKGZyZWVEZWZpbmUgPyAnZGVmaW5lLmFtZC4nIDogJ0JlbmNobWFyay4nKSArIHByb3AgKyAnPWZ1bmN0aW9uKCcgKyBhcmdzICsgJyl7JyArIGJvZHkgKyAnfScpO1xuICAgICAgcmVzdWx0ID0gYW5jaG9yW3Byb3BdO1xuICAgICAgZGVsZXRlIGFuY2hvcltwcm9wXTtcbiAgICAgIHJldHVybiByZXN1bHQ7XG4gICAgfTtcbiAgICAvLyBmaXggSmFlZ2VyTW9ua2V5IGJ1Z1xuICAgIC8vIGh0dHA6Ly9idWd6aWwubGEvNjM5NzIwXG4gICAgY3JlYXRlRnVuY3Rpb24gPSBzdXBwb3J0LmJyb3dzZXIgJiYgKGNyZWF0ZUZ1bmN0aW9uKCcnLCAncmV0dXJuXCInICsgdWlkICsgJ1wiJykgfHwgbm9vcCkoKSA9PSB1aWQgPyBjcmVhdGVGdW5jdGlvbiA6IEZ1bmN0aW9uO1xuICAgIHJldHVybiBjcmVhdGVGdW5jdGlvbi5hcHBseShudWxsLCBhcmd1bWVudHMpO1xuICB9XG5cbiAgLyoqXG4gICAqIERlbGF5IHRoZSBleGVjdXRpb24gb2YgYSBmdW5jdGlvbiBiYXNlZCBvbiB0aGUgYmVuY2htYXJrJ3MgYGRlbGF5YCBwcm9wZXJ0eS5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IGJlbmNoIFRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBmbiBUaGUgZnVuY3Rpb24gdG8gZXhlY3V0ZS5cbiAgICovXG4gIGZ1bmN0aW9uIGRlbGF5KGJlbmNoLCBmbikge1xuICAgIGJlbmNoLl90aW1lcklkID0gc2V0VGltZW91dChmbiwgYmVuY2guZGVsYXkgKiAxZTMpO1xuICB9XG5cbiAgLyoqXG4gICAqIERlc3Ryb3lzIHRoZSBnaXZlbiBlbGVtZW50LlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge0VsZW1lbnR9IGVsZW1lbnQgVGhlIGVsZW1lbnQgdG8gZGVzdHJveS5cbiAgICovXG4gIGZ1bmN0aW9uIGRlc3Ryb3lFbGVtZW50KGVsZW1lbnQpIHtcbiAgICB0cmFzaC5hcHBlbmRDaGlsZChlbGVtZW50KTtcbiAgICB0cmFzaC5pbm5lckhUTUwgPSAnJztcbiAgfVxuXG4gIC8qKlxuICAgKiBJdGVyYXRlcyBvdmVyIGFuIG9iamVjdCdzIHByb3BlcnRpZXMsIGV4ZWN1dGluZyB0aGUgYGNhbGxiYWNrYCBmb3IgZWFjaC5cbiAgICogQ2FsbGJhY2tzIG1heSB0ZXJtaW5hdGUgdGhlIGxvb3AgYnkgZXhwbGljaXRseSByZXR1cm5pbmcgYGZhbHNlYC5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IG9iamVjdCBUaGUgb2JqZWN0IHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uIGV4ZWN1dGVkIHBlciBvd24gcHJvcGVydHkuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBvcHRpb25zIFRoZSBvcHRpb25zIG9iamVjdC5cbiAgICogQHJldHVybnMge09iamVjdH0gUmV0dXJucyB0aGUgb2JqZWN0IGl0ZXJhdGVkIG92ZXIuXG4gICAqL1xuICBmdW5jdGlvbiBmb3JQcm9wcygpIHtcbiAgICB2YXIgZm9yU2hhZG93ZWQsXG4gICAgICAgIHNraXBTZWVuLFxuICAgICAgICBmb3JBcmdzID0gdHJ1ZSxcbiAgICAgICAgc2hhZG93ZWQgPSBbJ2NvbnN0cnVjdG9yJywgJ2hhc093blByb3BlcnR5JywgJ2lzUHJvdG90eXBlT2YnLCAncHJvcGVydHlJc0VudW1lcmFibGUnLCAndG9Mb2NhbGVTdHJpbmcnLCAndG9TdHJpbmcnLCAndmFsdWVPZiddO1xuXG4gICAgKGZ1bmN0aW9uKGVudW1GbGFnLCBrZXkpIHtcbiAgICAgIC8vIG11c3QgdXNlIGEgbm9uLW5hdGl2ZSBjb25zdHJ1Y3RvciB0byBjYXRjaCB0aGUgU2FmYXJpIDIgaXNzdWVcbiAgICAgIGZ1bmN0aW9uIEtsYXNzKCkgeyB0aGlzLnZhbHVlT2YgPSAwOyB9O1xuICAgICAgS2xhc3MucHJvdG90eXBlLnZhbHVlT2YgPSAwO1xuICAgICAgLy8gY2hlY2sgdmFyaW91cyBmb3ItaW4gYnVnc1xuICAgICAgZm9yIChrZXkgaW4gbmV3IEtsYXNzKSB7XG4gICAgICAgIGVudW1GbGFnICs9IGtleSA9PSAndmFsdWVPZicgPyAxIDogMDtcbiAgICAgIH1cbiAgICAgIC8vIGNoZWNrIGlmIGBhcmd1bWVudHNgIG9iamVjdHMgaGF2ZSBub24tZW51bWVyYWJsZSBpbmRleGVzXG4gICAgICBmb3IgKGtleSBpbiBhcmd1bWVudHMpIHtcbiAgICAgICAga2V5ID09ICcwJyAmJiAoZm9yQXJncyA9IGZhbHNlKTtcbiAgICAgIH1cbiAgICAgIC8vIFNhZmFyaSAyIGl0ZXJhdGVzIG92ZXIgc2hhZG93ZWQgcHJvcGVydGllcyB0d2ljZVxuICAgICAgLy8gaHR0cDovL3JlcGxheS53YXliYWNrbWFjaGluZS5vcmcvMjAwOTA0MjgyMjI5NDEvaHR0cDovL3RvYmllbGFuZ2VsLmNvbS8yMDA3LzEvMjkvZm9yLWluLWxvb3AtYnJva2VuLWluLXNhZmFyaS9cbiAgICAgIHNraXBTZWVuID0gZW51bUZsYWcgPT0gMjtcbiAgICAgIC8vIElFIDwgOSBpbmNvcnJlY3RseSBtYWtlcyBhbiBvYmplY3QncyBwcm9wZXJ0aWVzIG5vbi1lbnVtZXJhYmxlIGlmIHRoZXkgaGF2ZVxuICAgICAgLy8gdGhlIHNhbWUgbmFtZSBhcyBvdGhlciBub24tZW51bWVyYWJsZSBwcm9wZXJ0aWVzIGluIGl0cyBwcm90b3R5cGUgY2hhaW4uXG4gICAgICBmb3JTaGFkb3dlZCA9ICFlbnVtRmxhZztcbiAgICB9KDApKTtcblxuICAgIC8vIGxhenkgZGVmaW5lXG4gICAgZm9yUHJvcHMgPSBmdW5jdGlvbihvYmplY3QsIGNhbGxiYWNrLCBvcHRpb25zKSB7XG4gICAgICBvcHRpb25zIHx8IChvcHRpb25zID0ge30pO1xuXG4gICAgICB2YXIgcmVzdWx0ID0gb2JqZWN0O1xuICAgICAgb2JqZWN0ID0gT2JqZWN0KG9iamVjdCk7XG5cbiAgICAgIHZhciBjdG9yLFxuICAgICAgICAgIGtleSxcbiAgICAgICAgICBrZXlzLFxuICAgICAgICAgIHNraXBDdG9yLFxuICAgICAgICAgIGRvbmUgPSAhcmVzdWx0LFxuICAgICAgICAgIHdoaWNoID0gb3B0aW9ucy53aGljaCxcbiAgICAgICAgICBhbGxGbGFnID0gd2hpY2ggPT0gJ2FsbCcsXG4gICAgICAgICAgaW5kZXggPSAtMSxcbiAgICAgICAgICBpdGVyYXRlZSA9IG9iamVjdCxcbiAgICAgICAgICBsZW5ndGggPSBvYmplY3QubGVuZ3RoLFxuICAgICAgICAgIG93bkZsYWcgPSBhbGxGbGFnIHx8IHdoaWNoID09ICdvd24nLFxuICAgICAgICAgIHNlZW4gPSB7fSxcbiAgICAgICAgICBza2lwUHJvdG8gPSBpc0NsYXNzT2Yob2JqZWN0LCAnRnVuY3Rpb24nKSxcbiAgICAgICAgICB0aGlzQXJnID0gb3B0aW9ucy5iaW5kO1xuXG4gICAgICBpZiAodGhpc0FyZyAhPT0gdW5kZWZpbmVkKSB7XG4gICAgICAgIGNhbGxiYWNrID0gYmluZChjYWxsYmFjaywgdGhpc0FyZyk7XG4gICAgICB9XG4gICAgICAvLyBpdGVyYXRlIGFsbCBwcm9wZXJ0aWVzXG4gICAgICBpZiAoYWxsRmxhZyAmJiBzdXBwb3J0LmdldEFsbEtleXMpIHtcbiAgICAgICAgZm9yIChpbmRleCA9IDAsIGtleXMgPSBnZXRBbGxLZXlzKG9iamVjdCksIGxlbmd0aCA9IGtleXMubGVuZ3RoOyBpbmRleCA8IGxlbmd0aDsgaW5kZXgrKykge1xuICAgICAgICAgIGtleSA9IGtleXNbaW5kZXhdO1xuICAgICAgICAgIGlmIChjYWxsYmFjayhvYmplY3Rba2V5XSwga2V5LCBvYmplY3QpID09PSBmYWxzZSkge1xuICAgICAgICAgICAgYnJlYWs7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9XG4gICAgICAvLyBlbHNlIGl0ZXJhdGUgb25seSBlbnVtZXJhYmxlIHByb3BlcnRpZXNcbiAgICAgIGVsc2Uge1xuICAgICAgICBmb3IgKGtleSBpbiBvYmplY3QpIHtcbiAgICAgICAgICAvLyBGaXJlZm94IDwgMy42LCBPcGVyYSA+IDkuNTAgLSBPcGVyYSA8IDExLjYwLCBhbmQgU2FmYXJpIDwgNS4xXG4gICAgICAgICAgLy8gKGlmIHRoZSBwcm90b3R5cGUgb3IgYSBwcm9wZXJ0eSBvbiB0aGUgcHJvdG90eXBlIGhhcyBiZWVuIHNldClcbiAgICAgICAgICAvLyBpbmNvcnJlY3RseSBzZXQgYSBmdW5jdGlvbidzIGBwcm90b3R5cGVgIHByb3BlcnR5IFtbRW51bWVyYWJsZV1dIHZhbHVlXG4gICAgICAgICAgLy8gdG8gYHRydWVgLiBCZWNhdXNlIG9mIHRoaXMgd2Ugc3RhbmRhcmRpemUgb24gc2tpcHBpbmcgdGhlIGBwcm90b3R5cGVgXG4gICAgICAgICAgLy8gcHJvcGVydHkgb2YgZnVuY3Rpb25zIHJlZ2FyZGxlc3Mgb2YgdGhlaXIgW1tFbnVtZXJhYmxlXV0gdmFsdWUuXG4gICAgICAgICAgaWYgKChkb25lID1cbiAgICAgICAgICAgICAgIShza2lwUHJvdG8gJiYga2V5ID09ICdwcm90b3R5cGUnKSAmJlxuICAgICAgICAgICAgICAhKHNraXBTZWVuICYmIChoYXNLZXkoc2Vlbiwga2V5KSB8fCAhKHNlZW5ba2V5XSA9IHRydWUpKSkgJiZcbiAgICAgICAgICAgICAgKCFvd25GbGFnIHx8IG93bkZsYWcgJiYgaGFzS2V5KG9iamVjdCwga2V5KSkgJiZcbiAgICAgICAgICAgICAgY2FsbGJhY2sob2JqZWN0W2tleV0sIGtleSwgb2JqZWN0KSA9PT0gZmFsc2UpKSB7XG4gICAgICAgICAgICBicmVhaztcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgICAgLy8gaW4gSUUgPCA5IHN0cmluZ3MgZG9uJ3Qgc3VwcG9ydCBhY2Nlc3NpbmcgY2hhcmFjdGVycyBieSBpbmRleFxuICAgICAgICBpZiAoIWRvbmUgJiYgKGZvckFyZ3MgJiYgaXNBcmd1bWVudHMob2JqZWN0KSB8fFxuICAgICAgICAgICAgKChub0NoYXJCeUluZGV4IHx8IG5vQ2hhckJ5T3duSW5kZXgpICYmIGlzQ2xhc3NPZihvYmplY3QsICdTdHJpbmcnKSAmJlxuICAgICAgICAgICAgICAoaXRlcmF0ZWUgPSBub0NoYXJCeUluZGV4ID8gb2JqZWN0LnNwbGl0KCcnKSA6IG9iamVjdCkpKSkge1xuICAgICAgICAgIHdoaWxlICgrK2luZGV4IDwgbGVuZ3RoKSB7XG4gICAgICAgICAgICBpZiAoKGRvbmUgPVxuICAgICAgICAgICAgICAgIGNhbGxiYWNrKGl0ZXJhdGVlW2luZGV4XSwgU3RyaW5nKGluZGV4KSwgb2JqZWN0KSA9PT0gZmFsc2UpKSB7XG4gICAgICAgICAgICAgIGJyZWFrO1xuICAgICAgICAgICAgfVxuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgICBpZiAoIWRvbmUgJiYgZm9yU2hhZG93ZWQpIHtcbiAgICAgICAgICAvLyBCZWNhdXNlIElFIDwgOSBjYW4ndCBzZXQgdGhlIGBbW0VudW1lcmFibGVdXWAgYXR0cmlidXRlIG9mIGFuIGV4aXN0aW5nXG4gICAgICAgICAgLy8gcHJvcGVydHkgYW5kIHRoZSBgY29uc3RydWN0b3JgIHByb3BlcnR5IG9mIGEgcHJvdG90eXBlIGRlZmF1bHRzIHRvXG4gICAgICAgICAgLy8gbm9uLWVudW1lcmFibGUsIHdlIG1hbnVhbGx5IHNraXAgdGhlIGBjb25zdHJ1Y3RvcmAgcHJvcGVydHkgd2hlbiB3ZVxuICAgICAgICAgIC8vIHRoaW5rIHdlIGFyZSBpdGVyYXRpbmcgb3ZlciBhIGBwcm90b3R5cGVgIG9iamVjdC5cbiAgICAgICAgICBjdG9yID0gb2JqZWN0LmNvbnN0cnVjdG9yO1xuICAgICAgICAgIHNraXBDdG9yID0gY3RvciAmJiBjdG9yLnByb3RvdHlwZSAmJiBjdG9yLnByb3RvdHlwZS5jb25zdHJ1Y3RvciA9PT0gY3RvcjtcbiAgICAgICAgICBmb3IgKGluZGV4ID0gMDsgaW5kZXggPCA3OyBpbmRleCsrKSB7XG4gICAgICAgICAgICBrZXkgPSBzaGFkb3dlZFtpbmRleF07XG4gICAgICAgICAgICBpZiAoIShza2lwQ3RvciAmJiBrZXkgPT0gJ2NvbnN0cnVjdG9yJykgJiZcbiAgICAgICAgICAgICAgICBoYXNLZXkob2JqZWN0LCBrZXkpICYmXG4gICAgICAgICAgICAgICAgY2FsbGJhY2sob2JqZWN0W2tleV0sIGtleSwgb2JqZWN0KSA9PT0gZmFsc2UpIHtcbiAgICAgICAgICAgICAgYnJlYWs7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9XG4gICAgICByZXR1cm4gcmVzdWx0O1xuICAgIH07XG4gICAgcmV0dXJuIGZvclByb3BzLmFwcGx5KG51bGwsIGFyZ3VtZW50cyk7XG4gIH1cblxuICAvKipcbiAgICogR2V0cyB0aGUgbmFtZSBvZiB0aGUgZmlyc3QgYXJndW1lbnQgZnJvbSBhIGZ1bmN0aW9uJ3Mgc291cmNlLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBmbiBUaGUgZnVuY3Rpb24uXG4gICAqIEByZXR1cm5zIHtTdHJpbmd9IFRoZSBhcmd1bWVudCBuYW1lLlxuICAgKi9cbiAgZnVuY3Rpb24gZ2V0Rmlyc3RBcmd1bWVudChmbikge1xuICAgIHJldHVybiAoIWhhc0tleShmbiwgJ3RvU3RyaW5nJykgJiZcbiAgICAgICgvXltcXHMoXSpmdW5jdGlvblteKF0qXFwoKFteXFxzLCldKykvLmV4ZWMoZm4pIHx8IDApWzFdKSB8fCAnJztcbiAgfVxuXG4gIC8qKlxuICAgKiBDb21wdXRlcyB0aGUgYXJpdGhtZXRpYyBtZWFuIG9mIGEgc2FtcGxlLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge0FycmF5fSBzYW1wbGUgVGhlIHNhbXBsZS5cbiAgICogQHJldHVybnMge051bWJlcn0gVGhlIG1lYW4uXG4gICAqL1xuICBmdW5jdGlvbiBnZXRNZWFuKHNhbXBsZSkge1xuICAgIHJldHVybiByZWR1Y2Uoc2FtcGxlLCBmdW5jdGlvbihzdW0sIHgpIHtcbiAgICAgIHJldHVybiBzdW0gKyB4O1xuICAgIH0pIC8gc2FtcGxlLmxlbmd0aCB8fCAwO1xuICB9XG5cbiAgLyoqXG4gICAqIEdldHMgdGhlIHNvdXJjZSBjb2RlIG9mIGEgZnVuY3Rpb24uXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7RnVuY3Rpb259IGZuIFRoZSBmdW5jdGlvbi5cbiAgICogQHBhcmFtIHtTdHJpbmd9IGFsdFNvdXJjZSBBIHN0cmluZyB1c2VkIHdoZW4gYSBmdW5jdGlvbidzIHNvdXJjZSBjb2RlIGlzIHVucmV0cmlldmFibGUuXG4gICAqIEByZXR1cm5zIHtTdHJpbmd9IFRoZSBmdW5jdGlvbidzIHNvdXJjZSBjb2RlLlxuICAgKi9cbiAgZnVuY3Rpb24gZ2V0U291cmNlKGZuLCBhbHRTb3VyY2UpIHtcbiAgICB2YXIgcmVzdWx0ID0gYWx0U291cmNlO1xuICAgIGlmIChpc1N0cmluZ2FibGUoZm4pKSB7XG4gICAgICByZXN1bHQgPSBTdHJpbmcoZm4pO1xuICAgIH0gZWxzZSBpZiAoc3VwcG9ydC5kZWNvbXBpbGF0aW9uKSB7XG4gICAgICAvLyBlc2NhcGUgdGhlIGB7YCBmb3IgRmlyZWZveCAxXG4gICAgICByZXN1bHQgPSAoL15bXntdK1xceyhbXFxzXFxTXSopfVxccyokLy5leGVjKGZuKSB8fCAwKVsxXTtcbiAgICB9XG4gICAgLy8gdHJpbSBzdHJpbmdcbiAgICByZXN1bHQgPSAocmVzdWx0IHx8ICcnKS5yZXBsYWNlKC9eXFxzK3xcXHMrJC9nLCAnJyk7XG5cbiAgICAvLyBkZXRlY3Qgc3RyaW5ncyBjb250YWluaW5nIG9ubHkgdGhlIFwidXNlIHN0cmljdFwiIGRpcmVjdGl2ZVxuICAgIHJldHVybiAvXig/OlxcL1xcKitbXFx3fFxcV10qP1xcKlxcL3xcXC9cXC8uKj9bXFxuXFxyXFx1MjAyOFxcdTIwMjldfFxccykqKFtcIiddKXVzZSBzdHJpY3RcXDE7PyQvLnRlc3QocmVzdWx0KVxuICAgICAgPyAnJ1xuICAgICAgOiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogQ2hlY2tzIGlmIGEgdmFsdWUgaXMgYW4gYGFyZ3VtZW50c2Agb2JqZWN0LlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge01peGVkfSB2YWx1ZSBUaGUgdmFsdWUgdG8gY2hlY2suXG4gICAqIEByZXR1cm5zIHtCb29sZWFufSBSZXR1cm5zIGB0cnVlYCBpZiB0aGUgdmFsdWUgaXMgYW4gYGFyZ3VtZW50c2Agb2JqZWN0LCBlbHNlIGBmYWxzZWAuXG4gICAqL1xuICBmdW5jdGlvbiBpc0FyZ3VtZW50cygpIHtcbiAgICAvLyBsYXp5IGRlZmluZVxuICAgIGlzQXJndW1lbnRzID0gZnVuY3Rpb24odmFsdWUpIHtcbiAgICAgIHJldHVybiB0b1N0cmluZy5jYWxsKHZhbHVlKSA9PSAnW29iamVjdCBBcmd1bWVudHNdJztcbiAgICB9O1xuICAgIGlmIChub0FyZ3VtZW50c0NsYXNzKSB7XG4gICAgICBpc0FyZ3VtZW50cyA9IGZ1bmN0aW9uKHZhbHVlKSB7XG4gICAgICAgIHJldHVybiBoYXNLZXkodmFsdWUsICdjYWxsZWUnKSAmJlxuICAgICAgICAgICEocHJvcGVydHlJc0VudW1lcmFibGUgJiYgcHJvcGVydHlJc0VudW1lcmFibGUuY2FsbCh2YWx1ZSwgJ2NhbGxlZScpKTtcbiAgICAgIH07XG4gICAgfVxuICAgIHJldHVybiBpc0FyZ3VtZW50cyhhcmd1bWVudHNbMF0pO1xuICB9XG5cbiAgLyoqXG4gICAqIENoZWNrcyBpZiBhbiBvYmplY3QgaXMgb2YgdGhlIHNwZWNpZmllZCBjbGFzcy5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtNaXhlZH0gdmFsdWUgVGhlIHZhbHVlIHRvIGNoZWNrLlxuICAgKiBAcGFyYW0ge1N0cmluZ30gbmFtZSBUaGUgbmFtZSBvZiB0aGUgY2xhc3MuXG4gICAqIEByZXR1cm5zIHtCb29sZWFufSBSZXR1cm5zIGB0cnVlYCBpZiB0aGUgdmFsdWUgaXMgb2YgdGhlIHNwZWNpZmllZCBjbGFzcywgZWxzZSBgZmFsc2VgLlxuICAgKi9cbiAgZnVuY3Rpb24gaXNDbGFzc09mKHZhbHVlLCBuYW1lKSB7XG4gICAgcmV0dXJuIHZhbHVlICE9IG51bGwgJiYgdG9TdHJpbmcuY2FsbCh2YWx1ZSkgPT0gJ1tvYmplY3QgJyArIG5hbWUgKyAnXSc7XG4gIH1cblxuICAvKipcbiAgICogSG9zdCBvYmplY3RzIGNhbiByZXR1cm4gdHlwZSB2YWx1ZXMgdGhhdCBhcmUgZGlmZmVyZW50IGZyb20gdGhlaXIgYWN0dWFsXG4gICAqIGRhdGEgdHlwZS4gVGhlIG9iamVjdHMgd2UgYXJlIGNvbmNlcm5lZCB3aXRoIHVzdWFsbHkgcmV0dXJuIG5vbi1wcmltaXRpdmVcbiAgICogdHlwZXMgb2Ygb2JqZWN0LCBmdW5jdGlvbiwgb3IgdW5rbm93bi5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtNaXhlZH0gb2JqZWN0IFRoZSBvd25lciBvZiB0aGUgcHJvcGVydHkuXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBwcm9wZXJ0eSBUaGUgcHJvcGVydHkgdG8gY2hlY2suXG4gICAqIEByZXR1cm5zIHtCb29sZWFufSBSZXR1cm5zIGB0cnVlYCBpZiB0aGUgcHJvcGVydHkgdmFsdWUgaXMgYSBub24tcHJpbWl0aXZlLCBlbHNlIGBmYWxzZWAuXG4gICAqL1xuICBmdW5jdGlvbiBpc0hvc3RUeXBlKG9iamVjdCwgcHJvcGVydHkpIHtcbiAgICB2YXIgdHlwZSA9IG9iamVjdCAhPSBudWxsID8gdHlwZW9mIG9iamVjdFtwcm9wZXJ0eV0gOiAnbnVtYmVyJztcbiAgICByZXR1cm4gIS9eKD86Ym9vbGVhbnxudW1iZXJ8c3RyaW5nfHVuZGVmaW5lZCkkLy50ZXN0KHR5cGUpICYmXG4gICAgICAodHlwZSA9PSAnb2JqZWN0JyA/ICEhb2JqZWN0W3Byb3BlcnR5XSA6IHRydWUpO1xuICB9XG5cbiAgLyoqXG4gICAqIENoZWNrcyBpZiBhIGdpdmVuIGB2YWx1ZWAgaXMgYW4gb2JqZWN0IGNyZWF0ZWQgYnkgdGhlIGBPYmplY3RgIGNvbnN0cnVjdG9yXG4gICAqIGFzc3VtaW5nIG9iamVjdHMgY3JlYXRlZCBieSB0aGUgYE9iamVjdGAgY29uc3RydWN0b3IgaGF2ZSBubyBpbmhlcml0ZWRcbiAgICogZW51bWVyYWJsZSBwcm9wZXJ0aWVzIGFuZCB0aGF0IHRoZXJlIGFyZSBubyBgT2JqZWN0LnByb3RvdHlwZWAgZXh0ZW5zaW9ucy5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtNaXhlZH0gdmFsdWUgVGhlIHZhbHVlIHRvIGNoZWNrLlxuICAgKiBAcmV0dXJucyB7Qm9vbGVhbn0gUmV0dXJucyBgdHJ1ZWAgaWYgdGhlIGB2YWx1ZWAgaXMgYSBwbGFpbiBgT2JqZWN0YCBvYmplY3QsIGVsc2UgYGZhbHNlYC5cbiAgICovXG4gIGZ1bmN0aW9uIGlzUGxhaW5PYmplY3QodmFsdWUpIHtcbiAgICAvLyBhdm9pZCBub24tb2JqZWN0cyBhbmQgZmFsc2UgcG9zaXRpdmVzIGZvciBgYXJndW1lbnRzYCBvYmplY3RzIGluIElFIDwgOVxuICAgIHZhciByZXN1bHQgPSBmYWxzZTtcbiAgICBpZiAoISh2YWx1ZSAmJiB0eXBlb2YgdmFsdWUgPT0gJ29iamVjdCcpIHx8IChub0FyZ3VtZW50c0NsYXNzICYmIGlzQXJndW1lbnRzKHZhbHVlKSkpIHtcbiAgICAgIHJldHVybiByZXN1bHQ7XG4gICAgfVxuICAgIC8vIElFIDwgOSBwcmVzZW50cyBET00gbm9kZXMgYXMgYE9iamVjdGAgb2JqZWN0cyBleGNlcHQgdGhleSBoYXZlIGB0b1N0cmluZ2BcbiAgICAvLyBtZXRob2RzIHRoYXQgYXJlIGB0eXBlb2ZgIFwic3RyaW5nXCIgYW5kIHN0aWxsIGNhbiBjb2VyY2Ugbm9kZXMgdG8gc3RyaW5ncy5cbiAgICAvLyBBbHNvIGNoZWNrIHRoYXQgdGhlIGNvbnN0cnVjdG9yIGlzIGBPYmplY3RgIChpLmUuIGBPYmplY3QgaW5zdGFuY2VvZiBPYmplY3RgKVxuICAgIHZhciBjdG9yID0gdmFsdWUuY29uc3RydWN0b3I7XG4gICAgaWYgKChzdXBwb3J0Lm5vZGVDbGFzcyB8fCAhKHR5cGVvZiB2YWx1ZS50b1N0cmluZyAhPSAnZnVuY3Rpb24nICYmIHR5cGVvZiAodmFsdWUgKyAnJykgPT0gJ3N0cmluZycpKSAmJlxuICAgICAgICAoIWlzQ2xhc3NPZihjdG9yLCAnRnVuY3Rpb24nKSB8fCBjdG9yIGluc3RhbmNlb2YgY3RvcikpIHtcbiAgICAgIC8vIEluIG1vc3QgZW52aXJvbm1lbnRzIGFuIG9iamVjdCdzIG93biBwcm9wZXJ0aWVzIGFyZSBpdGVyYXRlZCBiZWZvcmVcbiAgICAgIC8vIGl0cyBpbmhlcml0ZWQgcHJvcGVydGllcy4gSWYgdGhlIGxhc3QgaXRlcmF0ZWQgcHJvcGVydHkgaXMgYW4gb2JqZWN0J3NcbiAgICAgIC8vIG93biBwcm9wZXJ0eSB0aGVuIHRoZXJlIGFyZSBubyBpbmhlcml0ZWQgZW51bWVyYWJsZSBwcm9wZXJ0aWVzLlxuICAgICAgaWYgKHN1cHBvcnQuaXRlcmF0ZXNPd25GaXJzdCkge1xuICAgICAgICBmb3JQcm9wcyh2YWx1ZSwgZnVuY3Rpb24oc3ViVmFsdWUsIHN1YktleSkge1xuICAgICAgICAgIHJlc3VsdCA9IHN1YktleTtcbiAgICAgICAgfSk7XG4gICAgICAgIHJldHVybiByZXN1bHQgPT09IGZhbHNlIHx8IGhhc0tleSh2YWx1ZSwgcmVzdWx0KTtcbiAgICAgIH1cbiAgICAgIC8vIElFIDwgOSBpdGVyYXRlcyBpbmhlcml0ZWQgcHJvcGVydGllcyBiZWZvcmUgb3duIHByb3BlcnRpZXMuIElmIHRoZSBmaXJzdFxuICAgICAgLy8gaXRlcmF0ZWQgcHJvcGVydHkgaXMgYW4gb2JqZWN0J3Mgb3duIHByb3BlcnR5IHRoZW4gdGhlcmUgYXJlIG5vIGluaGVyaXRlZFxuICAgICAgLy8gZW51bWVyYWJsZSBwcm9wZXJ0aWVzLlxuICAgICAgZm9yUHJvcHModmFsdWUsIGZ1bmN0aW9uKHN1YlZhbHVlLCBzdWJLZXkpIHtcbiAgICAgICAgcmVzdWx0ID0gIWhhc0tleSh2YWx1ZSwgc3ViS2V5KTtcbiAgICAgICAgcmV0dXJuIGZhbHNlO1xuICAgICAgfSk7XG4gICAgICByZXR1cm4gcmVzdWx0ID09PSBmYWxzZTtcbiAgICB9XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBDaGVja3MgaWYgYSB2YWx1ZSBjYW4gYmUgc2FmZWx5IGNvZXJjZWQgdG8gYSBzdHJpbmcuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHZhbHVlIFRoZSB2YWx1ZSB0byBjaGVjay5cbiAgICogQHJldHVybnMge0Jvb2xlYW59IFJldHVybnMgYHRydWVgIGlmIHRoZSB2YWx1ZSBjYW4gYmUgY29lcmNlZCwgZWxzZSBgZmFsc2VgLlxuICAgKi9cbiAgZnVuY3Rpb24gaXNTdHJpbmdhYmxlKHZhbHVlKSB7XG4gICAgcmV0dXJuIGhhc0tleSh2YWx1ZSwgJ3RvU3RyaW5nJykgfHwgaXNDbGFzc09mKHZhbHVlLCAnU3RyaW5nJyk7XG4gIH1cblxuICAvKipcbiAgICogV3JhcHMgYSBmdW5jdGlvbiBhbmQgcGFzc2VzIGB0aGlzYCB0byB0aGUgb3JpZ2luYWwgZnVuY3Rpb24gYXMgdGhlXG4gICAqIGZpcnN0IGFyZ3VtZW50LlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBmbiBUaGUgZnVuY3Rpb24gdG8gYmUgd3JhcHBlZC5cbiAgICogQHJldHVybnMge0Z1bmN0aW9ufSBUaGUgbmV3IGZ1bmN0aW9uLlxuICAgKi9cbiAgZnVuY3Rpb24gbWV0aG9kaXplKGZuKSB7XG4gICAgcmV0dXJuIGZ1bmN0aW9uKCkge1xuICAgICAgdmFyIGFyZ3MgPSBbdGhpc107XG4gICAgICBhcmdzLnB1c2guYXBwbHkoYXJncywgYXJndW1lbnRzKTtcbiAgICAgIHJldHVybiBmbi5hcHBseShudWxsLCBhcmdzKTtcbiAgICB9O1xuICB9XG5cbiAgLyoqXG4gICAqIEEgbm8tb3BlcmF0aW9uIGZ1bmN0aW9uLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKi9cbiAgZnVuY3Rpb24gbm9vcCgpIHtcbiAgICAvLyBubyBvcGVyYXRpb24gcGVyZm9ybWVkXG4gIH1cblxuICAvKipcbiAgICogQSB3cmFwcGVyIGFyb3VuZCByZXF1aXJlKCkgdG8gc3VwcHJlc3MgYG1vZHVsZSBtaXNzaW5nYCBlcnJvcnMuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBpZCBUaGUgbW9kdWxlIGlkLlxuICAgKiBAcmV0dXJucyB7TWl4ZWR9IFRoZSBleHBvcnRlZCBtb2R1bGUgb3IgYG51bGxgLlxuICAgKi9cbiAgZnVuY3Rpb24gcmVxKGlkKSB7XG4gICAgdHJ5IHtcbiAgICAgIHZhciByZXN1bHQgPSBmcmVlRXhwb3J0cyAmJiBmcmVlUmVxdWlyZShpZCk7XG4gICAgfSBjYXRjaChlKSB7IH1cbiAgICByZXR1cm4gcmVzdWx0IHx8IG51bGw7XG4gIH1cblxuICAvKipcbiAgICogUnVucyBhIHNuaXBwZXQgb2YgSmF2YVNjcmlwdCB2aWEgc2NyaXB0IGluamVjdGlvbi5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtTdHJpbmd9IGNvZGUgVGhlIGNvZGUgdG8gcnVuLlxuICAgKi9cbiAgZnVuY3Rpb24gcnVuU2NyaXB0KGNvZGUpIHtcbiAgICB2YXIgYW5jaG9yID0gZnJlZURlZmluZSA/IGRlZmluZS5hbWQgOiBCZW5jaG1hcmssXG4gICAgICAgIHNjcmlwdCA9IGRvYy5jcmVhdGVFbGVtZW50KCdzY3JpcHQnKSxcbiAgICAgICAgc2libGluZyA9IGRvYy5nZXRFbGVtZW50c0J5VGFnTmFtZSgnc2NyaXB0JylbMF0sXG4gICAgICAgIHBhcmVudCA9IHNpYmxpbmcucGFyZW50Tm9kZSxcbiAgICAgICAgcHJvcCA9IHVpZCArICdydW5TY3JpcHQnLFxuICAgICAgICBwcmVmaXggPSAnKCcgKyAoZnJlZURlZmluZSA/ICdkZWZpbmUuYW1kLicgOiAnQmVuY2htYXJrLicpICsgcHJvcCArICd8fGZ1bmN0aW9uKCl7fSkoKTsnO1xuXG4gICAgLy8gRmlyZWZveCAyLjAuMC4yIGNhbm5vdCB1c2Ugc2NyaXB0IGluamVjdGlvbiBhcyBpbnRlbmRlZCBiZWNhdXNlIGl0IGV4ZWN1dGVzXG4gICAgLy8gYXN5bmNocm9ub3VzbHksIGJ1dCB0aGF0J3MgT0sgYmVjYXVzZSBzY3JpcHQgaW5qZWN0aW9uIGlzIG9ubHkgdXNlZCB0byBhdm9pZFxuICAgIC8vIHRoZSBwcmV2aW91c2x5IGNvbW1lbnRlZCBKYWVnZXJNb25rZXkgYnVnLlxuICAgIHRyeSB7XG4gICAgICAvLyByZW1vdmUgdGhlIGluc2VydGVkIHNjcmlwdCAqYmVmb3JlKiBydW5uaW5nIHRoZSBjb2RlIHRvIGF2b2lkIGRpZmZlcmVuY2VzXG4gICAgICAvLyBpbiB0aGUgZXhwZWN0ZWQgc2NyaXB0IGVsZW1lbnQgY291bnQvb3JkZXIgb2YgdGhlIGRvY3VtZW50LlxuICAgICAgc2NyaXB0LmFwcGVuZENoaWxkKGRvYy5jcmVhdGVUZXh0Tm9kZShwcmVmaXggKyBjb2RlKSk7XG4gICAgICBhbmNob3JbcHJvcF0gPSBmdW5jdGlvbigpIHsgZGVzdHJveUVsZW1lbnQoc2NyaXB0KTsgfTtcbiAgICB9IGNhdGNoKGUpIHtcbiAgICAgIHBhcmVudCA9IHBhcmVudC5jbG9uZU5vZGUoZmFsc2UpO1xuICAgICAgc2libGluZyA9IG51bGw7XG4gICAgICBzY3JpcHQudGV4dCA9IGNvZGU7XG4gICAgfVxuICAgIHBhcmVudC5pbnNlcnRCZWZvcmUoc2NyaXB0LCBzaWJsaW5nKTtcbiAgICBkZWxldGUgYW5jaG9yW3Byb3BdO1xuICB9XG5cbiAgLyoqXG4gICAqIEEgaGVscGVyIGZ1bmN0aW9uIGZvciBzZXR0aW5nIG9wdGlvbnMvZXZlbnQgaGFuZGxlcnMuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBiZW5jaCBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKiBAcGFyYW0ge09iamVjdH0gW29wdGlvbnM9e31dIE9wdGlvbnMgb2JqZWN0LlxuICAgKi9cbiAgZnVuY3Rpb24gc2V0T3B0aW9ucyhiZW5jaCwgb3B0aW9ucykge1xuICAgIG9wdGlvbnMgPSBleHRlbmQoe30sIGJlbmNoLmNvbnN0cnVjdG9yLm9wdGlvbnMsIG9wdGlvbnMpO1xuICAgIGJlbmNoLm9wdGlvbnMgPSBmb3JPd24ob3B0aW9ucywgZnVuY3Rpb24odmFsdWUsIGtleSkge1xuICAgICAgaWYgKHZhbHVlICE9IG51bGwpIHtcbiAgICAgICAgLy8gYWRkIGV2ZW50IGxpc3RlbmVyc1xuICAgICAgICBpZiAoL15vbltBLVpdLy50ZXN0KGtleSkpIHtcbiAgICAgICAgICBmb3JFYWNoKGtleS5zcGxpdCgnICcpLCBmdW5jdGlvbihrZXkpIHtcbiAgICAgICAgICAgIGJlbmNoLm9uKGtleS5zbGljZSgyKS50b0xvd2VyQ2FzZSgpLCB2YWx1ZSk7XG4gICAgICAgICAgfSk7XG4gICAgICAgIH0gZWxzZSBpZiAoIWhhc0tleShiZW5jaCwga2V5KSkge1xuICAgICAgICAgIGJlbmNoW2tleV0gPSBkZWVwQ2xvbmUodmFsdWUpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfSk7XG4gIH1cblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogSGFuZGxlcyBjeWNsaW5nL2NvbXBsZXRpbmcgdGhlIGRlZmVycmVkIGJlbmNobWFyay5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5EZWZlcnJlZFxuICAgKi9cbiAgZnVuY3Rpb24gcmVzb2x2ZSgpIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICBjbG9uZSA9IG1lLmJlbmNobWFyayxcbiAgICAgICAgYmVuY2ggPSBjbG9uZS5fb3JpZ2luYWw7XG5cbiAgICBpZiAoYmVuY2guYWJvcnRlZCkge1xuICAgICAgLy8gY3ljbGUoKSAtPiBjbG9uZSBjeWNsZS9jb21wbGV0ZSBldmVudCAtPiBjb21wdXRlKCkncyBpbnZva2VkIGJlbmNoLnJ1bigpIGN5Y2xlL2NvbXBsZXRlXG4gICAgICBtZS50ZWFyZG93bigpO1xuICAgICAgY2xvbmUucnVubmluZyA9IGZhbHNlO1xuICAgICAgY3ljbGUobWUpO1xuICAgIH1cbiAgICBlbHNlIGlmICgrK21lLmN5Y2xlcyA8IGNsb25lLmNvdW50KSB7XG4gICAgICAvLyBjb250aW51ZSB0aGUgdGVzdCBsb29wXG4gICAgICBpZiAoc3VwcG9ydC50aW1lb3V0KSB7XG4gICAgICAgIC8vIHVzZSBzZXRUaW1lb3V0IHRvIGF2b2lkIGEgY2FsbCBzdGFjayBvdmVyZmxvdyBpZiBjYWxsZWQgcmVjdXJzaXZlbHlcbiAgICAgICAgc2V0VGltZW91dChmdW5jdGlvbigpIHsgY2xvbmUuY29tcGlsZWQuY2FsbChtZSwgdGltZXIpOyB9LCAwKTtcbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIGNsb25lLmNvbXBpbGVkLmNhbGwobWUsIHRpbWVyKTtcbiAgICAgIH1cbiAgICB9XG4gICAgZWxzZSB7XG4gICAgICB0aW1lci5zdG9wKG1lKTtcbiAgICAgIG1lLnRlYXJkb3duKCk7XG4gICAgICBkZWxheShjbG9uZSwgZnVuY3Rpb24oKSB7IGN5Y2xlKG1lKTsgfSk7XG4gICAgfVxuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIEEgZGVlcCBjbG9uZSB1dGlsaXR5LlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtNaXhlZH0gdmFsdWUgVGhlIHZhbHVlIHRvIGNsb25lLlxuICAgKiBAcmV0dXJucyB7TWl4ZWR9IFRoZSBjbG9uZWQgdmFsdWUuXG4gICAqL1xuICBmdW5jdGlvbiBkZWVwQ2xvbmUodmFsdWUpIHtcbiAgICB2YXIgYWNjZXNzb3IsXG4gICAgICAgIGNpcmN1bGFyLFxuICAgICAgICBjbG9uZSxcbiAgICAgICAgY3RvcixcbiAgICAgICAgZGVzY3JpcHRvcixcbiAgICAgICAgZXh0ZW5zaWJsZSxcbiAgICAgICAga2V5LFxuICAgICAgICBsZW5ndGgsXG4gICAgICAgIG1hcmtlcktleSxcbiAgICAgICAgcGFyZW50LFxuICAgICAgICByZXN1bHQsXG4gICAgICAgIHNvdXJjZSxcbiAgICAgICAgc3ViSW5kZXgsXG4gICAgICAgIGRhdGEgPSB7ICd2YWx1ZSc6IHZhbHVlIH0sXG4gICAgICAgIGluZGV4ID0gMCxcbiAgICAgICAgbWFya2VkID0gW10sXG4gICAgICAgIHF1ZXVlID0geyAnbGVuZ3RoJzogMCB9LFxuICAgICAgICB1bm1hcmtlZCA9IFtdO1xuXG4gICAgLyoqXG4gICAgICogQW4gZWFzaWx5IGRldGVjdGFibGUgZGVjb3JhdG9yIGZvciBjbG9uZWQgdmFsdWVzLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIE1hcmtlcihvYmplY3QpIHtcbiAgICAgIHRoaXMucmF3ID0gb2JqZWN0O1xuICAgIH1cblxuICAgIC8qKlxuICAgICAqIFRoZSBjYWxsYmFjayB1c2VkIGJ5IGBmb3JQcm9wcygpYC5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiBmb3JQcm9wc0NhbGxiYWNrKHN1YlZhbHVlLCBzdWJLZXkpIHtcbiAgICAgIC8vIGV4aXQgZWFybHkgdG8gYXZvaWQgY2xvbmluZyB0aGUgbWFya2VyXG4gICAgICBpZiAoc3ViVmFsdWUgJiYgc3ViVmFsdWUuY29uc3RydWN0b3IgPT0gTWFya2VyKSB7XG4gICAgICAgIHJldHVybjtcbiAgICAgIH1cbiAgICAgIC8vIGFkZCBvYmplY3RzIHRvIHRoZSBxdWV1ZVxuICAgICAgaWYgKHN1YlZhbHVlID09PSBPYmplY3Qoc3ViVmFsdWUpKSB7XG4gICAgICAgIHF1ZXVlW3F1ZXVlLmxlbmd0aCsrXSA9IHsgJ2tleSc6IHN1YktleSwgJ3BhcmVudCc6IGNsb25lLCAnc291cmNlJzogdmFsdWUgfTtcbiAgICAgIH1cbiAgICAgIC8vIGFzc2lnbiBub24tb2JqZWN0c1xuICAgICAgZWxzZSB7XG4gICAgICAgIHRyeSB7XG4gICAgICAgICAgLy8gd2lsbCB0aHJvdyBhbiBlcnJvciBpbiBzdHJpY3QgbW9kZSBpZiB0aGUgcHJvcGVydHkgaXMgcmVhZC1vbmx5XG4gICAgICAgICAgY2xvbmVbc3ViS2V5XSA9IHN1YlZhbHVlO1xuICAgICAgICB9IGNhdGNoKGUpIHsgfVxuICAgICAgfVxuICAgIH1cblxuICAgIC8qKlxuICAgICAqIEdldHMgYW4gYXZhaWxhYmxlIG1hcmtlciBrZXkgZm9yIHRoZSBnaXZlbiBvYmplY3QuXG4gICAgICovXG4gICAgZnVuY3Rpb24gZ2V0TWFya2VyS2V5KG9iamVjdCkge1xuICAgICAgLy8gYXZvaWQgY29sbGlzaW9ucyB3aXRoIGV4aXN0aW5nIGtleXNcbiAgICAgIHZhciByZXN1bHQgPSB1aWQ7XG4gICAgICB3aGlsZSAob2JqZWN0W3Jlc3VsdF0gJiYgb2JqZWN0W3Jlc3VsdF0uY29uc3RydWN0b3IgIT0gTWFya2VyKSB7XG4gICAgICAgIHJlc3VsdCArPSAxO1xuICAgICAgfVxuICAgICAgcmV0dXJuIHJlc3VsdDtcbiAgICB9XG5cbiAgICBkbyB7XG4gICAgICBrZXkgPSBkYXRhLmtleTtcbiAgICAgIHBhcmVudCA9IGRhdGEucGFyZW50O1xuICAgICAgc291cmNlID0gZGF0YS5zb3VyY2U7XG4gICAgICBjbG9uZSA9IHZhbHVlID0gc291cmNlID8gc291cmNlW2tleV0gOiBkYXRhLnZhbHVlO1xuICAgICAgYWNjZXNzb3IgPSBjaXJjdWxhciA9IGRlc2NyaXB0b3IgPSBmYWxzZTtcblxuICAgICAgLy8gY3JlYXRlIGEgYmFzaWMgY2xvbmUgdG8gZmlsdGVyIG91dCBmdW5jdGlvbnMsIERPTSBlbGVtZW50cywgYW5kXG4gICAgICAvLyBvdGhlciBub24gYE9iamVjdGAgb2JqZWN0c1xuICAgICAgaWYgKHZhbHVlID09PSBPYmplY3QodmFsdWUpKSB7XG4gICAgICAgIC8vIHVzZSBjdXN0b20gZGVlcCBjbG9uZSBmdW5jdGlvbiBpZiBhdmFpbGFibGVcbiAgICAgICAgaWYgKGlzQ2xhc3NPZih2YWx1ZS5kZWVwQ2xvbmUsICdGdW5jdGlvbicpKSB7XG4gICAgICAgICAgY2xvbmUgPSB2YWx1ZS5kZWVwQ2xvbmUoKTtcbiAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICBjdG9yID0gdmFsdWUuY29uc3RydWN0b3I7XG4gICAgICAgICAgc3dpdGNoICh0b1N0cmluZy5jYWxsKHZhbHVlKSkge1xuICAgICAgICAgICAgY2FzZSAnW29iamVjdCBBcnJheV0nOlxuICAgICAgICAgICAgICBjbG9uZSA9IG5ldyBjdG9yKHZhbHVlLmxlbmd0aCk7XG4gICAgICAgICAgICAgIGJyZWFrO1xuXG4gICAgICAgICAgICBjYXNlICdbb2JqZWN0IEJvb2xlYW5dJzpcbiAgICAgICAgICAgICAgY2xvbmUgPSBuZXcgY3Rvcih2YWx1ZSA9PSB0cnVlKTtcbiAgICAgICAgICAgICAgYnJlYWs7XG5cbiAgICAgICAgICAgIGNhc2UgJ1tvYmplY3QgRGF0ZV0nOlxuICAgICAgICAgICAgICBjbG9uZSA9IG5ldyBjdG9yKCt2YWx1ZSk7XG4gICAgICAgICAgICAgIGJyZWFrO1xuXG4gICAgICAgICAgICBjYXNlICdbb2JqZWN0IE9iamVjdF0nOlxuICAgICAgICAgICAgICBpc1BsYWluT2JqZWN0KHZhbHVlKSAmJiAoY2xvbmUgPSB7fSk7XG4gICAgICAgICAgICAgIGJyZWFrO1xuXG4gICAgICAgICAgICBjYXNlICdbb2JqZWN0IE51bWJlcl0nOlxuICAgICAgICAgICAgY2FzZSAnW29iamVjdCBTdHJpbmddJzpcbiAgICAgICAgICAgICAgY2xvbmUgPSBuZXcgY3Rvcih2YWx1ZSk7XG4gICAgICAgICAgICAgIGJyZWFrO1xuXG4gICAgICAgICAgICBjYXNlICdbb2JqZWN0IFJlZ0V4cF0nOlxuICAgICAgICAgICAgICBjbG9uZSA9IGN0b3IodmFsdWUuc291cmNlLFxuICAgICAgICAgICAgICAgICh2YWx1ZS5nbG9iYWwgICAgID8gJ2cnIDogJycpICtcbiAgICAgICAgICAgICAgICAodmFsdWUuaWdub3JlQ2FzZSA/ICdpJyA6ICcnKSArXG4gICAgICAgICAgICAgICAgKHZhbHVlLm11bHRpbGluZSAgPyAnbScgOiAnJykpO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgICAvLyBjb250aW51ZSBjbG9uZSBpZiBgdmFsdWVgIGRvZXNuJ3QgaGF2ZSBhbiBhY2Nlc3NvciBkZXNjcmlwdG9yXG4gICAgICAgIC8vIGh0dHA6Ly9lczUuZ2l0aHViLmNvbS8jeDguMTAuMVxuICAgICAgICBpZiAoY2xvbmUgJiYgY2xvbmUgIT0gdmFsdWUgJiZcbiAgICAgICAgICAgICEoZGVzY3JpcHRvciA9IHNvdXJjZSAmJiBzdXBwb3J0LmRlc2NyaXB0b3JzICYmIGdldERlc2NyaXB0b3Ioc291cmNlLCBrZXkpLFxuICAgICAgICAgICAgICBhY2Nlc3NvciA9IGRlc2NyaXB0b3IgJiYgKGRlc2NyaXB0b3IuZ2V0IHx8IGRlc2NyaXB0b3Iuc2V0KSkpIHtcbiAgICAgICAgICAvLyB1c2UgYW4gZXhpc3RpbmcgY2xvbmUgKGNpcmN1bGFyIHJlZmVyZW5jZSlcbiAgICAgICAgICBpZiAoKGV4dGVuc2libGUgPSBpc0V4dGVuc2libGUodmFsdWUpKSkge1xuICAgICAgICAgICAgbWFya2VyS2V5ID0gZ2V0TWFya2VyS2V5KHZhbHVlKTtcbiAgICAgICAgICAgIGlmICh2YWx1ZVttYXJrZXJLZXldKSB7XG4gICAgICAgICAgICAgIGNpcmN1bGFyID0gY2xvbmUgPSB2YWx1ZVttYXJrZXJLZXldLnJhdztcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgICAgLy8gZm9yIGZyb3plbi9zZWFsZWQgb2JqZWN0c1xuICAgICAgICAgICAgZm9yIChzdWJJbmRleCA9IDAsIGxlbmd0aCA9IHVubWFya2VkLmxlbmd0aDsgc3ViSW5kZXggPCBsZW5ndGg7IHN1YkluZGV4KyspIHtcbiAgICAgICAgICAgICAgZGF0YSA9IHVubWFya2VkW3N1YkluZGV4XTtcbiAgICAgICAgICAgICAgaWYgKGRhdGEub2JqZWN0ID09PSB2YWx1ZSkge1xuICAgICAgICAgICAgICAgIGNpcmN1bGFyID0gY2xvbmUgPSBkYXRhLmNsb25lO1xuICAgICAgICAgICAgICAgIGJyZWFrO1xuICAgICAgICAgICAgICB9XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuICAgICAgICAgIGlmICghY2lyY3VsYXIpIHtcbiAgICAgICAgICAgIC8vIG1hcmsgb2JqZWN0IHRvIGFsbG93IHF1aWNrbHkgZGV0ZWN0aW5nIGNpcmN1bGFyIHJlZmVyZW5jZXMgYW5kIHRpZSBpdCB0byBpdHMgY2xvbmVcbiAgICAgICAgICAgIGlmIChleHRlbnNpYmxlKSB7XG4gICAgICAgICAgICAgIHZhbHVlW21hcmtlcktleV0gPSBuZXcgTWFya2VyKGNsb25lKTtcbiAgICAgICAgICAgICAgbWFya2VkLnB1c2goeyAna2V5JzogbWFya2VyS2V5LCAnb2JqZWN0JzogdmFsdWUgfSk7XG4gICAgICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgICAgICAvLyBmb3IgZnJvemVuL3NlYWxlZCBvYmplY3RzXG4gICAgICAgICAgICAgIHVubWFya2VkLnB1c2goeyAnY2xvbmUnOiBjbG9uZSwgJ29iamVjdCc6IHZhbHVlIH0pO1xuICAgICAgICAgICAgfVxuICAgICAgICAgICAgLy8gaXRlcmF0ZSBvdmVyIG9iamVjdCBwcm9wZXJ0aWVzXG4gICAgICAgICAgICBmb3JQcm9wcyh2YWx1ZSwgZm9yUHJvcHNDYWxsYmFjaywgeyAnd2hpY2gnOiAnYWxsJyB9KTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIGlmIChwYXJlbnQpIHtcbiAgICAgICAgLy8gZm9yIGN1c3RvbSBwcm9wZXJ0eSBkZXNjcmlwdG9yc1xuICAgICAgICBpZiAoYWNjZXNzb3IgfHwgKGRlc2NyaXB0b3IgJiYgIShkZXNjcmlwdG9yLmNvbmZpZ3VyYWJsZSAmJiBkZXNjcmlwdG9yLmVudW1lcmFibGUgJiYgZGVzY3JpcHRvci53cml0YWJsZSkpKSB7XG4gICAgICAgICAgaWYgKCd2YWx1ZScgaW4gZGVzY3JpcHRvcikge1xuICAgICAgICAgICAgZGVzY3JpcHRvci52YWx1ZSA9IGNsb25lO1xuICAgICAgICAgIH1cbiAgICAgICAgICBzZXREZXNjcmlwdG9yKHBhcmVudCwga2V5LCBkZXNjcmlwdG9yKTtcbiAgICAgICAgfVxuICAgICAgICAvLyBmb3IgZGVmYXVsdCBwcm9wZXJ0eSBkZXNjcmlwdG9yc1xuICAgICAgICBlbHNlIHtcbiAgICAgICAgICBwYXJlbnRba2V5XSA9IGNsb25lO1xuICAgICAgICB9XG4gICAgICB9IGVsc2Uge1xuICAgICAgICByZXN1bHQgPSBjbG9uZTtcbiAgICAgIH1cbiAgICB9IHdoaWxlICgoZGF0YSA9IHF1ZXVlW2luZGV4KytdKSk7XG5cbiAgICAvLyByZW1vdmUgbWFya2Vyc1xuICAgIGZvciAoaW5kZXggPSAwLCBsZW5ndGggPSBtYXJrZWQubGVuZ3RoOyBpbmRleCA8IGxlbmd0aDsgaW5kZXgrKykge1xuICAgICAgZGF0YSA9IG1hcmtlZFtpbmRleF07XG4gICAgICBkZWxldGUgZGF0YS5vYmplY3RbZGF0YS5rZXldO1xuICAgIH1cbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIEFuIGl0ZXJhdGlvbiB1dGlsaXR5IGZvciBhcnJheXMgYW5kIG9iamVjdHMuXG4gICAqIENhbGxiYWNrcyBtYXkgdGVybWluYXRlIHRoZSBsb29wIGJ5IGV4cGxpY2l0bHkgcmV0dXJuaW5nIGBmYWxzZWAuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge0FycmF5fE9iamVjdH0gb2JqZWN0IFRoZSBvYmplY3QgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHRoaXNBcmcgVGhlIGB0aGlzYCBiaW5kaW5nIGZvciB0aGUgY2FsbGJhY2suXG4gICAqIEByZXR1cm5zIHtBcnJheXxPYmplY3R9IFJldHVybnMgdGhlIG9iamVjdCBpdGVyYXRlZCBvdmVyLlxuICAgKi9cbiAgZnVuY3Rpb24gZWFjaChvYmplY3QsIGNhbGxiYWNrLCB0aGlzQXJnKSB7XG4gICAgdmFyIHJlc3VsdCA9IG9iamVjdDtcbiAgICBvYmplY3QgPSBPYmplY3Qob2JqZWN0KTtcblxuICAgIHZhciBmbiA9IGNhbGxiYWNrLFxuICAgICAgICBpbmRleCA9IC0xLFxuICAgICAgICBsZW5ndGggPSBvYmplY3QubGVuZ3RoLFxuICAgICAgICBpc1NuYXBzaG90ID0gISEob2JqZWN0LnNuYXBzaG90SXRlbSAmJiAobGVuZ3RoID0gb2JqZWN0LnNuYXBzaG90TGVuZ3RoKSksXG4gICAgICAgIGlzU3BsaXR0YWJsZSA9IChub0NoYXJCeUluZGV4IHx8IG5vQ2hhckJ5T3duSW5kZXgpICYmIGlzQ2xhc3NPZihvYmplY3QsICdTdHJpbmcnKSxcbiAgICAgICAgaXNDb252ZXJ0YWJsZSA9IGlzU25hcHNob3QgfHwgaXNTcGxpdHRhYmxlIHx8ICdpdGVtJyBpbiBvYmplY3QsXG4gICAgICAgIG9yaWdPYmplY3QgPSBvYmplY3Q7XG5cbiAgICAvLyBpbiBPcGVyYSA8IDEwLjUgYGhhc0tleShvYmplY3QsICdsZW5ndGgnKWAgcmV0dXJucyBgZmFsc2VgIGZvciBOb2RlTGlzdHNcbiAgICBpZiAobGVuZ3RoID09PSBsZW5ndGggPj4+IDApIHtcbiAgICAgIGlmIChpc0NvbnZlcnRhYmxlKSB7XG4gICAgICAgIC8vIHRoZSB0aGlyZCBhcmd1bWVudCBvZiB0aGUgY2FsbGJhY2sgaXMgdGhlIG9yaWdpbmFsIG5vbi1hcnJheSBvYmplY3RcbiAgICAgICAgY2FsbGJhY2sgPSBmdW5jdGlvbih2YWx1ZSwgaW5kZXgpIHtcbiAgICAgICAgICByZXR1cm4gZm4uY2FsbCh0aGlzLCB2YWx1ZSwgaW5kZXgsIG9yaWdPYmplY3QpO1xuICAgICAgICB9O1xuICAgICAgICAvLyBpbiBJRSA8IDkgc3RyaW5ncyBkb24ndCBzdXBwb3J0IGFjY2Vzc2luZyBjaGFyYWN0ZXJzIGJ5IGluZGV4XG4gICAgICAgIGlmIChpc1NwbGl0dGFibGUpIHtcbiAgICAgICAgICBvYmplY3QgPSBvYmplY3Quc3BsaXQoJycpO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIG9iamVjdCA9IFtdO1xuICAgICAgICAgIHdoaWxlICgrK2luZGV4IDwgbGVuZ3RoKSB7XG4gICAgICAgICAgICAvLyBpbiBTYWZhcmkgMiBgaW5kZXggaW4gb2JqZWN0YCBpcyBhbHdheXMgYGZhbHNlYCBmb3IgTm9kZUxpc3RzXG4gICAgICAgICAgICBvYmplY3RbaW5kZXhdID0gaXNTbmFwc2hvdCA/IHJlc3VsdC5zbmFwc2hvdEl0ZW0oaW5kZXgpIDogcmVzdWx0W2luZGV4XTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIGZvckVhY2gob2JqZWN0LCBjYWxsYmFjaywgdGhpc0FyZyk7XG4gICAgfSBlbHNlIHtcbiAgICAgIGZvck93bihvYmplY3QsIGNhbGxiYWNrLCB0aGlzQXJnKTtcbiAgICB9XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBDb3BpZXMgZW51bWVyYWJsZSBwcm9wZXJ0aWVzIGZyb20gdGhlIHNvdXJjZShzKSBvYmplY3QgdG8gdGhlIGRlc3RpbmF0aW9uIG9iamVjdC5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBkZXN0aW5hdGlvbiBUaGUgZGVzdGluYXRpb24gb2JqZWN0LlxuICAgKiBAcGFyYW0ge09iamVjdH0gW3NvdXJjZT17fV0gVGhlIHNvdXJjZSBvYmplY3QuXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBkZXN0aW5hdGlvbiBvYmplY3QuXG4gICAqL1xuICBmdW5jdGlvbiBleHRlbmQoZGVzdGluYXRpb24sIHNvdXJjZSkge1xuICAgIC8vIENocm9tZSA8IDE0IGluY29ycmVjdGx5IHNldHMgYGRlc3RpbmF0aW9uYCB0byBgdW5kZWZpbmVkYCB3aGVuIHdlIGBkZWxldGUgYXJndW1lbnRzWzBdYFxuICAgIC8vIGh0dHA6Ly9jb2RlLmdvb2dsZS5jb20vcC92OC9pc3N1ZXMvZGV0YWlsP2lkPTgzOVxuICAgIHZhciByZXN1bHQgPSBkZXN0aW5hdGlvbjtcbiAgICBkZWxldGUgYXJndW1lbnRzWzBdO1xuXG4gICAgZm9yRWFjaChhcmd1bWVudHMsIGZ1bmN0aW9uKHNvdXJjZSkge1xuICAgICAgZm9yUHJvcHMoc291cmNlLCBmdW5jdGlvbih2YWx1ZSwga2V5KSB7XG4gICAgICAgIHJlc3VsdFtrZXldID0gdmFsdWU7XG4gICAgICB9KTtcbiAgICB9KTtcbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIEEgZ2VuZXJpYyBgQXJyYXkjZmlsdGVyYCBsaWtlIG1ldGhvZC5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl9IGFycmF5IFRoZSBhcnJheSB0byBpdGVyYXRlIG92ZXIuXG4gICAqIEBwYXJhbSB7RnVuY3Rpb258U3RyaW5nfSBjYWxsYmFjayBUaGUgZnVuY3Rpb24vYWxpYXMgY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHRoaXNBcmcgVGhlIGB0aGlzYCBiaW5kaW5nIGZvciB0aGUgY2FsbGJhY2suXG4gICAqIEByZXR1cm5zIHtBcnJheX0gQSBuZXcgYXJyYXkgb2YgdmFsdWVzIHRoYXQgcGFzc2VkIGNhbGxiYWNrIGZpbHRlci5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gZ2V0IG9kZCBudW1iZXJzXG4gICAqIEJlbmNobWFyay5maWx0ZXIoWzEsIDIsIDMsIDQsIDVdLCBmdW5jdGlvbihuKSB7XG4gICAqICAgcmV0dXJuIG4gJSAyO1xuICAgKiB9KTsgLy8gLT4gWzEsIDMsIDVdO1xuICAgKlxuICAgKiAvLyBnZXQgZmFzdGVzdCBiZW5jaG1hcmtzXG4gICAqIEJlbmNobWFyay5maWx0ZXIoYmVuY2hlcywgJ2Zhc3Rlc3QnKTtcbiAgICpcbiAgICogLy8gZ2V0IHNsb3dlc3QgYmVuY2htYXJrc1xuICAgKiBCZW5jaG1hcmsuZmlsdGVyKGJlbmNoZXMsICdzbG93ZXN0Jyk7XG4gICAqXG4gICAqIC8vIGdldCBiZW5jaG1hcmtzIHRoYXQgY29tcGxldGVkIHdpdGhvdXQgZXJyb3JpbmdcbiAgICogQmVuY2htYXJrLmZpbHRlcihiZW5jaGVzLCAnc3VjY2Vzc2Z1bCcpO1xuICAgKi9cbiAgZnVuY3Rpb24gZmlsdGVyKGFycmF5LCBjYWxsYmFjaywgdGhpc0FyZykge1xuICAgIHZhciByZXN1bHQ7XG5cbiAgICBpZiAoY2FsbGJhY2sgPT0gJ3N1Y2Nlc3NmdWwnKSB7XG4gICAgICAvLyBjYWxsYmFjayB0byBleGNsdWRlIHRob3NlIHRoYXQgYXJlIGVycm9yZWQsIHVucnVuLCBvciBoYXZlIGh6IG9mIEluZmluaXR5XG4gICAgICBjYWxsYmFjayA9IGZ1bmN0aW9uKGJlbmNoKSB7IHJldHVybiBiZW5jaC5jeWNsZXMgJiYgaXNGaW5pdGUoYmVuY2guaHopOyB9O1xuICAgIH1cbiAgICBlbHNlIGlmIChjYWxsYmFjayA9PSAnZmFzdGVzdCcgfHwgY2FsbGJhY2sgPT0gJ3Nsb3dlc3QnKSB7XG4gICAgICAvLyBnZXQgc3VjY2Vzc2Z1bCwgc29ydCBieSBwZXJpb2QgKyBtYXJnaW4gb2YgZXJyb3IsIGFuZCBmaWx0ZXIgZmFzdGVzdC9zbG93ZXN0XG4gICAgICByZXN1bHQgPSBmaWx0ZXIoYXJyYXksICdzdWNjZXNzZnVsJykuc29ydChmdW5jdGlvbihhLCBiKSB7XG4gICAgICAgIGEgPSBhLnN0YXRzOyBiID0gYi5zdGF0cztcbiAgICAgICAgcmV0dXJuIChhLm1lYW4gKyBhLm1vZSA+IGIubWVhbiArIGIubW9lID8gMSA6IC0xKSAqIChjYWxsYmFjayA9PSAnZmFzdGVzdCcgPyAxIDogLTEpO1xuICAgICAgfSk7XG4gICAgICByZXN1bHQgPSBmaWx0ZXIocmVzdWx0LCBmdW5jdGlvbihiZW5jaCkge1xuICAgICAgICByZXR1cm4gcmVzdWx0WzBdLmNvbXBhcmUoYmVuY2gpID09IDA7XG4gICAgICB9KTtcbiAgICB9XG4gICAgcmV0dXJuIHJlc3VsdCB8fCByZWR1Y2UoYXJyYXksIGZ1bmN0aW9uKHJlc3VsdCwgdmFsdWUsIGluZGV4KSB7XG4gICAgICByZXR1cm4gY2FsbGJhY2suY2FsbCh0aGlzQXJnLCB2YWx1ZSwgaW5kZXgsIGFycmF5KSA/IChyZXN1bHQucHVzaCh2YWx1ZSksIHJlc3VsdCkgOiByZXN1bHQ7XG4gICAgfSwgW10pO1xuICB9XG5cbiAgLyoqXG4gICAqIEEgZ2VuZXJpYyBgQXJyYXkjZm9yRWFjaGAgbGlrZSBtZXRob2QuXG4gICAqIENhbGxiYWNrcyBtYXkgdGVybWluYXRlIHRoZSBsb29wIGJ5IGV4cGxpY2l0bHkgcmV0dXJuaW5nIGBmYWxzZWAuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge0FycmF5fSBhcnJheSBUaGUgYXJyYXkgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHRoaXNBcmcgVGhlIGB0aGlzYCBiaW5kaW5nIGZvciB0aGUgY2FsbGJhY2suXG4gICAqIEByZXR1cm5zIHtBcnJheX0gUmV0dXJucyB0aGUgYXJyYXkgaXRlcmF0ZWQgb3Zlci5cbiAgICovXG4gIGZ1bmN0aW9uIGZvckVhY2goYXJyYXksIGNhbGxiYWNrLCB0aGlzQXJnKSB7XG4gICAgdmFyIGluZGV4ID0gLTEsXG4gICAgICAgIGxlbmd0aCA9IChhcnJheSA9IE9iamVjdChhcnJheSkpLmxlbmd0aCA+Pj4gMDtcblxuICAgIGlmICh0aGlzQXJnICE9PSB1bmRlZmluZWQpIHtcbiAgICAgIGNhbGxiYWNrID0gYmluZChjYWxsYmFjaywgdGhpc0FyZyk7XG4gICAgfVxuICAgIHdoaWxlICgrK2luZGV4IDwgbGVuZ3RoKSB7XG4gICAgICBpZiAoaW5kZXggaW4gYXJyYXkgJiZcbiAgICAgICAgICBjYWxsYmFjayhhcnJheVtpbmRleF0sIGluZGV4LCBhcnJheSkgPT09IGZhbHNlKSB7XG4gICAgICAgIGJyZWFrO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gYXJyYXk7XG4gIH1cblxuICAvKipcbiAgICogSXRlcmF0ZXMgb3ZlciBhbiBvYmplY3QncyBvd24gcHJvcGVydGllcywgZXhlY3V0aW5nIHRoZSBgY2FsbGJhY2tgIGZvciBlYWNoLlxuICAgKiBDYWxsYmFja3MgbWF5IHRlcm1pbmF0ZSB0aGUgbG9vcCBieSBleHBsaWNpdGx5IHJldHVybmluZyBgZmFsc2VgLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtPYmplY3R9IG9iamVjdCBUaGUgb2JqZWN0IHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uIGV4ZWN1dGVkIHBlciBvd24gcHJvcGVydHkuXG4gICAqIEBwYXJhbSB7TWl4ZWR9IHRoaXNBcmcgVGhlIGB0aGlzYCBiaW5kaW5nIGZvciB0aGUgY2FsbGJhY2suXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFJldHVybnMgdGhlIG9iamVjdCBpdGVyYXRlZCBvdmVyLlxuICAgKi9cbiAgZnVuY3Rpb24gZm9yT3duKG9iamVjdCwgY2FsbGJhY2ssIHRoaXNBcmcpIHtcbiAgICByZXR1cm4gZm9yUHJvcHMob2JqZWN0LCBjYWxsYmFjaywgeyAnYmluZCc6IHRoaXNBcmcsICd3aGljaCc6ICdvd24nIH0pO1xuICB9XG5cbiAgLyoqXG4gICAqIENvbnZlcnRzIGEgbnVtYmVyIHRvIGEgbW9yZSByZWFkYWJsZSBjb21tYS1zZXBhcmF0ZWQgc3RyaW5nIHJlcHJlc2VudGF0aW9uLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtOdW1iZXJ9IG51bWJlciBUaGUgbnVtYmVyIHRvIGNvbnZlcnQuXG4gICAqIEByZXR1cm5zIHtTdHJpbmd9IFRoZSBtb3JlIHJlYWRhYmxlIHN0cmluZyByZXByZXNlbnRhdGlvbi5cbiAgICovXG4gIGZ1bmN0aW9uIGZvcm1hdE51bWJlcihudW1iZXIpIHtcbiAgICBudW1iZXIgPSBTdHJpbmcobnVtYmVyKS5zcGxpdCgnLicpO1xuICAgIHJldHVybiBudW1iZXJbMF0ucmVwbGFjZSgvKD89KD86XFxkezN9KSskKSg/IVxcYikvZywgJywnKSArXG4gICAgICAobnVtYmVyWzFdID8gJy4nICsgbnVtYmVyWzFdIDogJycpO1xuICB9XG5cbiAgLyoqXG4gICAqIENoZWNrcyBpZiBhbiBvYmplY3QgaGFzIHRoZSBzcGVjaWZpZWQga2V5IGFzIGEgZGlyZWN0IHByb3BlcnR5LlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtPYmplY3R9IG9iamVjdCBUaGUgb2JqZWN0IHRvIGNoZWNrLlxuICAgKiBAcGFyYW0ge1N0cmluZ30ga2V5IFRoZSBrZXkgdG8gY2hlY2sgZm9yLlxuICAgKiBAcmV0dXJucyB7Qm9vbGVhbn0gUmV0dXJucyBgdHJ1ZWAgaWYga2V5IGlzIGEgZGlyZWN0IHByb3BlcnR5LCBlbHNlIGBmYWxzZWAuXG4gICAqL1xuICBmdW5jdGlvbiBoYXNLZXkoKSB7XG4gICAgLy8gbGF6eSBkZWZpbmUgZm9yIHdvcnN0IGNhc2UgZmFsbGJhY2sgKG5vdCBhcyBhY2N1cmF0ZSlcbiAgICBoYXNLZXkgPSBmdW5jdGlvbihvYmplY3QsIGtleSkge1xuICAgICAgdmFyIHBhcmVudCA9IG9iamVjdCAhPSBudWxsICYmIChvYmplY3QuY29uc3RydWN0b3IgfHwgT2JqZWN0KS5wcm90b3R5cGU7XG4gICAgICByZXR1cm4gISFwYXJlbnQgJiYga2V5IGluIE9iamVjdChvYmplY3QpICYmICEoa2V5IGluIHBhcmVudCAmJiBvYmplY3Rba2V5XSA9PT0gcGFyZW50W2tleV0pO1xuICAgIH07XG4gICAgLy8gZm9yIG1vZGVybiBicm93c2Vyc1xuICAgIGlmIChpc0NsYXNzT2YoaGFzT3duUHJvcGVydHksICdGdW5jdGlvbicpKSB7XG4gICAgICBoYXNLZXkgPSBmdW5jdGlvbihvYmplY3QsIGtleSkge1xuICAgICAgICByZXR1cm4gb2JqZWN0ICE9IG51bGwgJiYgaGFzT3duUHJvcGVydHkuY2FsbChvYmplY3QsIGtleSk7XG4gICAgICB9O1xuICAgIH1cbiAgICAvLyBmb3IgU2FmYXJpIDJcbiAgICBlbHNlIGlmICh7fS5fX3Byb3RvX18gPT0gT2JqZWN0LnByb3RvdHlwZSkge1xuICAgICAgaGFzS2V5ID0gZnVuY3Rpb24ob2JqZWN0LCBrZXkpIHtcbiAgICAgICAgdmFyIHJlc3VsdCA9IGZhbHNlO1xuICAgICAgICBpZiAob2JqZWN0ICE9IG51bGwpIHtcbiAgICAgICAgICBvYmplY3QgPSBPYmplY3Qob2JqZWN0KTtcbiAgICAgICAgICBvYmplY3QuX19wcm90b19fID0gW29iamVjdC5fX3Byb3RvX18sIG9iamVjdC5fX3Byb3RvX18gPSBudWxsLCByZXN1bHQgPSBrZXkgaW4gb2JqZWN0XVswXTtcbiAgICAgICAgfVxuICAgICAgICByZXR1cm4gcmVzdWx0O1xuICAgICAgfTtcbiAgICB9XG4gICAgcmV0dXJuIGhhc0tleS5hcHBseSh0aGlzLCBhcmd1bWVudHMpO1xuICB9XG5cbiAgLyoqXG4gICAqIEEgZ2VuZXJpYyBgQXJyYXkjaW5kZXhPZmAgbGlrZSBtZXRob2QuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge0FycmF5fSBhcnJheSBUaGUgYXJyYXkgdG8gaXRlcmF0ZSBvdmVyLlxuICAgKiBAcGFyYW0ge01peGVkfSB2YWx1ZSBUaGUgdmFsdWUgdG8gc2VhcmNoIGZvci5cbiAgICogQHBhcmFtIHtOdW1iZXJ9IFtmcm9tSW5kZXg9MF0gVGhlIGluZGV4IHRvIHN0YXJ0IHNlYXJjaGluZyBmcm9tLlxuICAgKiBAcmV0dXJucyB7TnVtYmVyfSBUaGUgaW5kZXggb2YgdGhlIG1hdGNoZWQgdmFsdWUgb3IgYC0xYC5cbiAgICovXG4gIGZ1bmN0aW9uIGluZGV4T2YoYXJyYXksIHZhbHVlLCBmcm9tSW5kZXgpIHtcbiAgICB2YXIgaW5kZXggPSB0b0ludGVnZXIoZnJvbUluZGV4KSxcbiAgICAgICAgbGVuZ3RoID0gKGFycmF5ID0gT2JqZWN0KGFycmF5KSkubGVuZ3RoID4+PiAwO1xuXG4gICAgaW5kZXggPSAoaW5kZXggPCAwID8gbWF4KDAsIGxlbmd0aCArIGluZGV4KSA6IGluZGV4KSAtIDE7XG4gICAgd2hpbGUgKCsraW5kZXggPCBsZW5ndGgpIHtcbiAgICAgIGlmIChpbmRleCBpbiBhcnJheSAmJiB2YWx1ZSA9PT0gYXJyYXlbaW5kZXhdKSB7XG4gICAgICAgIHJldHVybiBpbmRleDtcbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIC0xO1xuICB9XG5cbiAgLyoqXG4gICAqIE1vZGlmeSBhIHN0cmluZyBieSByZXBsYWNpbmcgbmFtZWQgdG9rZW5zIHdpdGggbWF0Y2hpbmcgb2JqZWN0IHByb3BlcnR5IHZhbHVlcy5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7U3RyaW5nfSBzdHJpbmcgVGhlIHN0cmluZyB0byBtb2RpZnkuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBvYmplY3QgVGhlIHRlbXBsYXRlIG9iamVjdC5cbiAgICogQHJldHVybnMge1N0cmluZ30gVGhlIG1vZGlmaWVkIHN0cmluZy5cbiAgICovXG4gIGZ1bmN0aW9uIGludGVycG9sYXRlKHN0cmluZywgb2JqZWN0KSB7XG4gICAgZm9yT3duKG9iamVjdCwgZnVuY3Rpb24odmFsdWUsIGtleSkge1xuICAgICAgLy8gZXNjYXBlIHJlZ2V4cCBzcGVjaWFsIGNoYXJhY3RlcnMgaW4gYGtleWBcbiAgICAgIHN0cmluZyA9IHN0cmluZy5yZXBsYWNlKFJlZ0V4cCgnI1xcXFx7JyArIGtleS5yZXBsYWNlKC8oWy4qKz9ePSE6JHt9KCl8W1xcXVxcL1xcXFxdKS9nLCAnXFxcXCQxJykgKyAnXFxcXH0nLCAnZycpLCB2YWx1ZSk7XG4gICAgfSk7XG4gICAgcmV0dXJuIHN0cmluZztcbiAgfVxuXG4gIC8qKlxuICAgKiBJbnZva2VzIGEgbWV0aG9kIG9uIGFsbCBpdGVtcyBpbiBhbiBhcnJheS5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl9IGJlbmNoZXMgQXJyYXkgb2YgYmVuY2htYXJrcyB0byBpdGVyYXRlIG92ZXIuXG4gICAqIEBwYXJhbSB7U3RyaW5nfE9iamVjdH0gbmFtZSBUaGUgbmFtZSBvZiB0aGUgbWV0aG9kIHRvIGludm9rZSBPUiBvcHRpb25zIG9iamVjdC5cbiAgICogQHBhcmFtIHtNaXhlZH0gW2FyZzEsIGFyZzIsIC4uLl0gQXJndW1lbnRzIHRvIGludm9rZSB0aGUgbWV0aG9kIHdpdGguXG4gICAqIEByZXR1cm5zIHtBcnJheX0gQSBuZXcgYXJyYXkgb2YgdmFsdWVzIHJldHVybmVkIGZyb20gZWFjaCBtZXRob2QgaW52b2tlZC5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gaW52b2tlIGByZXNldGAgb24gYWxsIGJlbmNobWFya3NcbiAgICogQmVuY2htYXJrLmludm9rZShiZW5jaGVzLCAncmVzZXQnKTtcbiAgICpcbiAgICogLy8gaW52b2tlIGBlbWl0YCB3aXRoIGFyZ3VtZW50c1xuICAgKiBCZW5jaG1hcmsuaW52b2tlKGJlbmNoZXMsICdlbWl0JywgJ2NvbXBsZXRlJywgbGlzdGVuZXIpO1xuICAgKlxuICAgKiAvLyBpbnZva2UgYHJ1bih0cnVlKWAsIHRyZWF0IGJlbmNobWFya3MgYXMgYSBxdWV1ZSwgYW5kIHJlZ2lzdGVyIGludm9rZSBjYWxsYmFja3NcbiAgICogQmVuY2htYXJrLmludm9rZShiZW5jaGVzLCB7XG4gICAqXG4gICAqICAgLy8gaW52b2tlIHRoZSBgcnVuYCBtZXRob2RcbiAgICogICAnbmFtZSc6ICdydW4nLFxuICAgKlxuICAgKiAgIC8vIHBhc3MgYSBzaW5nbGUgYXJndW1lbnRcbiAgICogICAnYXJncyc6IHRydWUsXG4gICAqXG4gICAqICAgLy8gdHJlYXQgYXMgcXVldWUsIHJlbW92aW5nIGJlbmNobWFya3MgZnJvbSBmcm9udCBvZiBgYmVuY2hlc2AgdW50aWwgZW1wdHlcbiAgICogICAncXVldWVkJzogdHJ1ZSxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgYmVmb3JlIGFueSBiZW5jaG1hcmtzIGhhdmUgYmVlbiBpbnZva2VkLlxuICAgKiAgICdvblN0YXJ0Jzogb25TdGFydCxcbiAgICpcbiAgICogICAvLyBjYWxsZWQgYmV0d2VlbiBpbnZva2luZyBiZW5jaG1hcmtzXG4gICAqICAgJ29uQ3ljbGUnOiBvbkN5Y2xlLFxuICAgKlxuICAgKiAgIC8vIGNhbGxlZCBhZnRlciBhbGwgYmVuY2htYXJrcyBoYXZlIGJlZW4gaW52b2tlZC5cbiAgICogICAnb25Db21wbGV0ZSc6IG9uQ29tcGxldGVcbiAgICogfSk7XG4gICAqL1xuICBmdW5jdGlvbiBpbnZva2UoYmVuY2hlcywgbmFtZSkge1xuICAgIHZhciBhcmdzLFxuICAgICAgICBiZW5jaCxcbiAgICAgICAgcXVldWVkLFxuICAgICAgICBpbmRleCA9IC0xLFxuICAgICAgICBldmVudFByb3BzID0geyAnY3VycmVudFRhcmdldCc6IGJlbmNoZXMgfSxcbiAgICAgICAgb3B0aW9ucyA9IHsgJ29uU3RhcnQnOiBub29wLCAnb25DeWNsZSc6IG5vb3AsICdvbkNvbXBsZXRlJzogbm9vcCB9LFxuICAgICAgICByZXN1bHQgPSBtYXAoYmVuY2hlcywgZnVuY3Rpb24oYmVuY2gpIHsgcmV0dXJuIGJlbmNoOyB9KTtcblxuICAgIC8qKlxuICAgICAqIEludm9rZXMgdGhlIG1ldGhvZCBvZiB0aGUgY3VycmVudCBvYmplY3QgYW5kIGlmIHN5bmNocm9ub3VzLCBmZXRjaGVzIHRoZSBuZXh0LlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIGV4ZWN1dGUoKSB7XG4gICAgICB2YXIgbGlzdGVuZXJzLFxuICAgICAgICAgIGFzeW5jID0gaXNBc3luYyhiZW5jaCk7XG5cbiAgICAgIGlmIChhc3luYykge1xuICAgICAgICAvLyB1c2UgYGdldE5leHRgIGFzIHRoZSBmaXJzdCBsaXN0ZW5lclxuICAgICAgICBiZW5jaC5vbignY29tcGxldGUnLCBnZXROZXh0KTtcbiAgICAgICAgbGlzdGVuZXJzID0gYmVuY2guZXZlbnRzLmNvbXBsZXRlO1xuICAgICAgICBsaXN0ZW5lcnMuc3BsaWNlKDAsIDAsIGxpc3RlbmVycy5wb3AoKSk7XG4gICAgICB9XG4gICAgICAvLyBleGVjdXRlIG1ldGhvZFxuICAgICAgcmVzdWx0W2luZGV4XSA9IGlzQ2xhc3NPZihiZW5jaCAmJiBiZW5jaFtuYW1lXSwgJ0Z1bmN0aW9uJykgPyBiZW5jaFtuYW1lXS5hcHBseShiZW5jaCwgYXJncykgOiB1bmRlZmluZWQ7XG4gICAgICAvLyBpZiBzeW5jaHJvbm91cyByZXR1cm4gdHJ1ZSB1bnRpbCBmaW5pc2hlZFxuICAgICAgcmV0dXJuICFhc3luYyAmJiBnZXROZXh0KCk7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogRmV0Y2hlcyB0aGUgbmV4dCBiZW5jaCBvciBleGVjdXRlcyBgb25Db21wbGV0ZWAgY2FsbGJhY2suXG4gICAgICovXG4gICAgZnVuY3Rpb24gZ2V0TmV4dChldmVudCkge1xuICAgICAgdmFyIGN5Y2xlRXZlbnQsXG4gICAgICAgICAgbGFzdCA9IGJlbmNoLFxuICAgICAgICAgIGFzeW5jID0gaXNBc3luYyhsYXN0KTtcblxuICAgICAgaWYgKGFzeW5jKSB7XG4gICAgICAgIGxhc3Qub2ZmKCdjb21wbGV0ZScsIGdldE5leHQpO1xuICAgICAgICBsYXN0LmVtaXQoJ2NvbXBsZXRlJyk7XG4gICAgICB9XG4gICAgICAvLyBlbWl0IFwiY3ljbGVcIiBldmVudFxuICAgICAgZXZlbnRQcm9wcy50eXBlID0gJ2N5Y2xlJztcbiAgICAgIGV2ZW50UHJvcHMudGFyZ2V0ID0gbGFzdDtcbiAgICAgIGN5Y2xlRXZlbnQgPSBFdmVudChldmVudFByb3BzKTtcbiAgICAgIG9wdGlvbnMub25DeWNsZS5jYWxsKGJlbmNoZXMsIGN5Y2xlRXZlbnQpO1xuXG4gICAgICAvLyBjaG9vc2UgbmV4dCBiZW5jaG1hcmsgaWYgbm90IGV4aXRpbmcgZWFybHlcbiAgICAgIGlmICghY3ljbGVFdmVudC5hYm9ydGVkICYmIHJhaXNlSW5kZXgoKSAhPT0gZmFsc2UpIHtcbiAgICAgICAgYmVuY2ggPSBxdWV1ZWQgPyBiZW5jaGVzWzBdIDogcmVzdWx0W2luZGV4XTtcbiAgICAgICAgaWYgKGlzQXN5bmMoYmVuY2gpKSB7XG4gICAgICAgICAgZGVsYXkoYmVuY2gsIGV4ZWN1dGUpO1xuICAgICAgICB9XG4gICAgICAgIGVsc2UgaWYgKGFzeW5jKSB7XG4gICAgICAgICAgLy8gcmVzdW1lIGV4ZWN1dGlvbiBpZiBwcmV2aW91c2x5IGFzeW5jaHJvbm91cyBidXQgbm93IHN5bmNocm9ub3VzXG4gICAgICAgICAgd2hpbGUgKGV4ZWN1dGUoKSkgeyB9XG4gICAgICAgIH1cbiAgICAgICAgZWxzZSB7XG4gICAgICAgICAgLy8gY29udGludWUgc3luY2hyb25vdXMgZXhlY3V0aW9uXG4gICAgICAgICAgcmV0dXJuIHRydWU7XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSB7XG4gICAgICAgIC8vIGVtaXQgXCJjb21wbGV0ZVwiIGV2ZW50XG4gICAgICAgIGV2ZW50UHJvcHMudHlwZSA9ICdjb21wbGV0ZSc7XG4gICAgICAgIG9wdGlvbnMub25Db21wbGV0ZS5jYWxsKGJlbmNoZXMsIEV2ZW50KGV2ZW50UHJvcHMpKTtcbiAgICAgIH1cbiAgICAgIC8vIFdoZW4gdXNlZCBhcyBhIGxpc3RlbmVyIGBldmVudC5hYm9ydGVkID0gdHJ1ZWAgd2lsbCBjYW5jZWwgdGhlIHJlc3Qgb2ZcbiAgICAgIC8vIHRoZSBcImNvbXBsZXRlXCIgbGlzdGVuZXJzIGJlY2F1c2UgdGhleSB3ZXJlIGFscmVhZHkgY2FsbGVkIGFib3ZlIGFuZCB3aGVuXG4gICAgICAvLyB1c2VkIGFzIHBhcnQgb2YgYGdldE5leHRgIHRoZSBgcmV0dXJuIGZhbHNlYCB3aWxsIGV4aXQgdGhlIGV4ZWN1dGlvbiB3aGlsZS1sb29wLlxuICAgICAgaWYgKGV2ZW50KSB7XG4gICAgICAgIGV2ZW50LmFib3J0ZWQgPSB0cnVlO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgcmV0dXJuIGZhbHNlO1xuICAgICAgfVxuICAgIH1cblxuICAgIC8qKlxuICAgICAqIENoZWNrcyBpZiBpbnZva2luZyBgQmVuY2htYXJrI3J1bmAgd2l0aCBhc3luY2hyb25vdXMgY3ljbGVzLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIGlzQXN5bmMob2JqZWN0KSB7XG4gICAgICAvLyBhdm9pZCB1c2luZyBgaW5zdGFuY2VvZmAgaGVyZSBiZWNhdXNlIG9mIElFIG1lbW9yeSBsZWFrIGlzc3VlcyB3aXRoIGhvc3Qgb2JqZWN0c1xuICAgICAgdmFyIGFzeW5jID0gYXJnc1swXSAmJiBhcmdzWzBdLmFzeW5jO1xuICAgICAgcmV0dXJuIE9iamVjdChvYmplY3QpLmNvbnN0cnVjdG9yID09IEJlbmNobWFyayAmJiBuYW1lID09ICdydW4nICYmXG4gICAgICAgICgoYXN5bmMgPT0gbnVsbCA/IG9iamVjdC5vcHRpb25zLmFzeW5jIDogYXN5bmMpICYmIHN1cHBvcnQudGltZW91dCB8fCBvYmplY3QuZGVmZXIpO1xuICAgIH1cblxuICAgIC8qKlxuICAgICAqIFJhaXNlcyBgaW5kZXhgIHRvIHRoZSBuZXh0IGRlZmluZWQgaW5kZXggb3IgcmV0dXJucyBgZmFsc2VgLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIHJhaXNlSW5kZXgoKSB7XG4gICAgICB2YXIgbGVuZ3RoID0gcmVzdWx0Lmxlbmd0aDtcbiAgICAgIGlmIChxdWV1ZWQpIHtcbiAgICAgICAgLy8gaWYgcXVldWVkIHJlbW92ZSB0aGUgcHJldmlvdXMgYmVuY2ggYW5kIHN1YnNlcXVlbnQgc2tpcHBlZCBub24tZW50cmllc1xuICAgICAgICBkbyB7XG4gICAgICAgICAgKytpbmRleCA+IDAgJiYgc2hpZnQuY2FsbChiZW5jaGVzKTtcbiAgICAgICAgfSB3aGlsZSAoKGxlbmd0aCA9IGJlbmNoZXMubGVuZ3RoKSAmJiAhKCcwJyBpbiBiZW5jaGVzKSk7XG4gICAgICB9XG4gICAgICBlbHNlIHtcbiAgICAgICAgd2hpbGUgKCsraW5kZXggPCBsZW5ndGggJiYgIShpbmRleCBpbiByZXN1bHQpKSB7IH1cbiAgICAgIH1cbiAgICAgIC8vIGlmIHdlIHJlYWNoZWQgdGhlIGxhc3QgaW5kZXggdGhlbiByZXR1cm4gYGZhbHNlYFxuICAgICAgcmV0dXJuIChxdWV1ZWQgPyBsZW5ndGggOiBpbmRleCA8IGxlbmd0aCkgPyBpbmRleCA6IChpbmRleCA9IGZhbHNlKTtcbiAgICB9XG5cbiAgICAvLyBqdWdnbGUgYXJndW1lbnRzXG4gICAgaWYgKGlzQ2xhc3NPZihuYW1lLCAnU3RyaW5nJykpIHtcbiAgICAgIC8vIDIgYXJndW1lbnRzIChhcnJheSwgbmFtZSlcbiAgICAgIGFyZ3MgPSBzbGljZS5jYWxsKGFyZ3VtZW50cywgMik7XG4gICAgfSBlbHNlIHtcbiAgICAgIC8vIDIgYXJndW1lbnRzIChhcnJheSwgb3B0aW9ucylcbiAgICAgIG9wdGlvbnMgPSBleHRlbmQob3B0aW9ucywgbmFtZSk7XG4gICAgICBuYW1lID0gb3B0aW9ucy5uYW1lO1xuICAgICAgYXJncyA9IGlzQ2xhc3NPZihhcmdzID0gJ2FyZ3MnIGluIG9wdGlvbnMgPyBvcHRpb25zLmFyZ3MgOiBbXSwgJ0FycmF5JykgPyBhcmdzIDogW2FyZ3NdO1xuICAgICAgcXVldWVkID0gb3B0aW9ucy5xdWV1ZWQ7XG4gICAgfVxuXG4gICAgLy8gc3RhcnQgaXRlcmF0aW5nIG92ZXIgdGhlIGFycmF5XG4gICAgaWYgKHJhaXNlSW5kZXgoKSAhPT0gZmFsc2UpIHtcbiAgICAgIC8vIGVtaXQgXCJzdGFydFwiIGV2ZW50XG4gICAgICBiZW5jaCA9IHJlc3VsdFtpbmRleF07XG4gICAgICBldmVudFByb3BzLnR5cGUgPSAnc3RhcnQnO1xuICAgICAgZXZlbnRQcm9wcy50YXJnZXQgPSBiZW5jaDtcbiAgICAgIG9wdGlvbnMub25TdGFydC5jYWxsKGJlbmNoZXMsIEV2ZW50KGV2ZW50UHJvcHMpKTtcblxuICAgICAgLy8gZW5kIGVhcmx5IGlmIHRoZSBzdWl0ZSB3YXMgYWJvcnRlZCBpbiBhbiBcIm9uU3RhcnRcIiBsaXN0ZW5lclxuICAgICAgaWYgKGJlbmNoZXMuYWJvcnRlZCAmJiBiZW5jaGVzLmNvbnN0cnVjdG9yID09IFN1aXRlICYmIG5hbWUgPT0gJ3J1bicpIHtcbiAgICAgICAgLy8gZW1pdCBcImN5Y2xlXCIgZXZlbnRcbiAgICAgICAgZXZlbnRQcm9wcy50eXBlID0gJ2N5Y2xlJztcbiAgICAgICAgb3B0aW9ucy5vbkN5Y2xlLmNhbGwoYmVuY2hlcywgRXZlbnQoZXZlbnRQcm9wcykpO1xuICAgICAgICAvLyBlbWl0IFwiY29tcGxldGVcIiBldmVudFxuICAgICAgICBldmVudFByb3BzLnR5cGUgPSAnY29tcGxldGUnO1xuICAgICAgICBvcHRpb25zLm9uQ29tcGxldGUuY2FsbChiZW5jaGVzLCBFdmVudChldmVudFByb3BzKSk7XG4gICAgICB9XG4gICAgICAvLyBlbHNlIHN0YXJ0XG4gICAgICBlbHNlIHtcbiAgICAgICAgaWYgKGlzQXN5bmMoYmVuY2gpKSB7XG4gICAgICAgICAgZGVsYXkoYmVuY2gsIGV4ZWN1dGUpO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIHdoaWxlIChleGVjdXRlKCkpIHsgfVxuICAgICAgICB9XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiByZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogQ3JlYXRlcyBhIHN0cmluZyBvZiBqb2luZWQgYXJyYXkgdmFsdWVzIG9yIG9iamVjdCBrZXktdmFsdWUgcGFpcnMuXG4gICAqXG4gICAqIEBzdGF0aWNcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge0FycmF5fE9iamVjdH0gb2JqZWN0IFRoZSBvYmplY3QgdG8gb3BlcmF0ZSBvbi5cbiAgICogQHBhcmFtIHtTdHJpbmd9IFtzZXBhcmF0b3IxPScsJ10gVGhlIHNlcGFyYXRvciB1c2VkIGJldHdlZW4ga2V5LXZhbHVlIHBhaXJzLlxuICAgKiBAcGFyYW0ge1N0cmluZ30gW3NlcGFyYXRvcjI9JzogJ10gVGhlIHNlcGFyYXRvciB1c2VkIGJldHdlZW4ga2V5cyBhbmQgdmFsdWVzLlxuICAgKiBAcmV0dXJucyB7U3RyaW5nfSBUaGUgam9pbmVkIHJlc3VsdC5cbiAgICovXG4gIGZ1bmN0aW9uIGpvaW4ob2JqZWN0LCBzZXBhcmF0b3IxLCBzZXBhcmF0b3IyKSB7XG4gICAgdmFyIHJlc3VsdCA9IFtdLFxuICAgICAgICBsZW5ndGggPSAob2JqZWN0ID0gT2JqZWN0KG9iamVjdCkpLmxlbmd0aCxcbiAgICAgICAgYXJyYXlMaWtlID0gbGVuZ3RoID09PSBsZW5ndGggPj4+IDA7XG5cbiAgICBzZXBhcmF0b3IyIHx8IChzZXBhcmF0b3IyID0gJzogJyk7XG4gICAgZWFjaChvYmplY3QsIGZ1bmN0aW9uKHZhbHVlLCBrZXkpIHtcbiAgICAgIHJlc3VsdC5wdXNoKGFycmF5TGlrZSA/IHZhbHVlIDoga2V5ICsgc2VwYXJhdG9yMiArIHZhbHVlKTtcbiAgICB9KTtcbiAgICByZXR1cm4gcmVzdWx0LmpvaW4oc2VwYXJhdG9yMSB8fCAnLCcpO1xuICB9XG5cbiAgLyoqXG4gICAqIEEgZ2VuZXJpYyBgQXJyYXkjbWFwYCBsaWtlIG1ldGhvZC5cbiAgICpcbiAgICogQHN0YXRpY1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7QXJyYXl9IGFycmF5IFRoZSBhcnJheSB0byBpdGVyYXRlIG92ZXIuXG4gICAqIEBwYXJhbSB7RnVuY3Rpb259IGNhbGxiYWNrIFRoZSBmdW5jdGlvbiBjYWxsZWQgcGVyIGl0ZXJhdGlvbi5cbiAgICogQHBhcmFtIHtNaXhlZH0gdGhpc0FyZyBUaGUgYHRoaXNgIGJpbmRpbmcgZm9yIHRoZSBjYWxsYmFjay5cbiAgICogQHJldHVybnMge0FycmF5fSBBIG5ldyBhcnJheSBvZiB2YWx1ZXMgcmV0dXJuZWQgYnkgdGhlIGNhbGxiYWNrLlxuICAgKi9cbiAgZnVuY3Rpb24gbWFwKGFycmF5LCBjYWxsYmFjaywgdGhpc0FyZykge1xuICAgIHJldHVybiByZWR1Y2UoYXJyYXksIGZ1bmN0aW9uKHJlc3VsdCwgdmFsdWUsIGluZGV4KSB7XG4gICAgICByZXN1bHRbaW5kZXhdID0gY2FsbGJhY2suY2FsbCh0aGlzQXJnLCB2YWx1ZSwgaW5kZXgsIGFycmF5KTtcbiAgICAgIHJldHVybiByZXN1bHQ7XG4gICAgfSwgQXJyYXkoT2JqZWN0KGFycmF5KS5sZW5ndGggPj4+IDApKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBSZXRyaWV2ZXMgdGhlIHZhbHVlIG9mIGEgc3BlY2lmaWVkIHByb3BlcnR5IGZyb20gYWxsIGl0ZW1zIGluIGFuIGFycmF5LlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtBcnJheX0gYXJyYXkgVGhlIGFycmF5IHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtTdHJpbmd9IHByb3BlcnR5IFRoZSBwcm9wZXJ0eSB0byBwbHVjay5cbiAgICogQHJldHVybnMge0FycmF5fSBBIG5ldyBhcnJheSBvZiBwcm9wZXJ0eSB2YWx1ZXMuXG4gICAqL1xuICBmdW5jdGlvbiBwbHVjayhhcnJheSwgcHJvcGVydHkpIHtcbiAgICByZXR1cm4gbWFwKGFycmF5LCBmdW5jdGlvbihvYmplY3QpIHtcbiAgICAgIHJldHVybiBvYmplY3QgPT0gbnVsbCA/IHVuZGVmaW5lZCA6IG9iamVjdFtwcm9wZXJ0eV07XG4gICAgfSk7XG4gIH1cblxuICAvKipcbiAgICogQSBnZW5lcmljIGBBcnJheSNyZWR1Y2VgIGxpa2UgbWV0aG9kLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtBcnJheX0gYXJyYXkgVGhlIGFycmF5IHRvIGl0ZXJhdGUgb3Zlci5cbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uIGNhbGxlZCBwZXIgaXRlcmF0aW9uLlxuICAgKiBAcGFyYW0ge01peGVkfSBhY2N1bXVsYXRvciBJbml0aWFsIHZhbHVlIG9mIHRoZSBhY2N1bXVsYXRvci5cbiAgICogQHJldHVybnMge01peGVkfSBUaGUgYWNjdW11bGF0b3IuXG4gICAqL1xuICBmdW5jdGlvbiByZWR1Y2UoYXJyYXksIGNhbGxiYWNrLCBhY2N1bXVsYXRvcikge1xuICAgIHZhciBub2FjY3VtID0gYXJndW1lbnRzLmxlbmd0aCA8IDM7XG4gICAgZm9yRWFjaChhcnJheSwgZnVuY3Rpb24odmFsdWUsIGluZGV4KSB7XG4gICAgICBhY2N1bXVsYXRvciA9IG5vYWNjdW0gPyAobm9hY2N1bSA9IGZhbHNlLCB2YWx1ZSkgOiBjYWxsYmFjayhhY2N1bXVsYXRvciwgdmFsdWUsIGluZGV4LCBhcnJheSk7XG4gICAgfSk7XG4gICAgcmV0dXJuIGFjY3VtdWxhdG9yO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIEFib3J0cyBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGUuXG4gICAqXG4gICAqIEBuYW1lIGFib3J0XG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIHN1aXRlIGluc3RhbmNlLlxuICAgKi9cbiAgZnVuY3Rpb24gYWJvcnRTdWl0ZSgpIHtcbiAgICB2YXIgZXZlbnQsXG4gICAgICAgIG1lID0gdGhpcyxcbiAgICAgICAgcmVzZXR0aW5nID0gY2FsbGVkQnkucmVzZXRTdWl0ZTtcblxuICAgIGlmIChtZS5ydW5uaW5nKSB7XG4gICAgICBldmVudCA9IEV2ZW50KCdhYm9ydCcpO1xuICAgICAgbWUuZW1pdChldmVudCk7XG4gICAgICBpZiAoIWV2ZW50LmNhbmNlbGxlZCB8fCByZXNldHRpbmcpIHtcbiAgICAgICAgLy8gYXZvaWQgaW5maW5pdGUgcmVjdXJzaW9uXG4gICAgICAgIGNhbGxlZEJ5LmFib3J0U3VpdGUgPSB0cnVlO1xuICAgICAgICBtZS5yZXNldCgpO1xuICAgICAgICBkZWxldGUgY2FsbGVkQnkuYWJvcnRTdWl0ZTtcblxuICAgICAgICBpZiAoIXJlc2V0dGluZykge1xuICAgICAgICAgIG1lLmFib3J0ZWQgPSB0cnVlO1xuICAgICAgICAgIGludm9rZShtZSwgJ2Fib3J0Jyk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyoqXG4gICAqIEFkZHMgYSB0ZXN0IHRvIHRoZSBiZW5jaG1hcmsgc3VpdGUuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHBhcmFtIHtTdHJpbmd9IG5hbWUgQSBuYW1lIHRvIGlkZW50aWZ5IHRoZSBiZW5jaG1hcmsuXG4gICAqIEBwYXJhbSB7RnVuY3Rpb258U3RyaW5nfSBmbiBUaGUgdGVzdCB0byBiZW5jaG1hcmsuXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBbb3B0aW9ucz17fV0gT3B0aW9ucyBvYmplY3QuXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqIEBleGFtcGxlXG4gICAqXG4gICAqIC8vIGJhc2ljIHVzYWdlXG4gICAqIHN1aXRlLmFkZChmbik7XG4gICAqXG4gICAqIC8vIG9yIHVzaW5nIGEgbmFtZSBmaXJzdFxuICAgKiBzdWl0ZS5hZGQoJ2ZvbycsIGZuKTtcbiAgICpcbiAgICogLy8gb3Igd2l0aCBvcHRpb25zXG4gICAqIHN1aXRlLmFkZCgnZm9vJywgZm4sIHtcbiAgICogICAnb25DeWNsZSc6IG9uQ3ljbGUsXG4gICAqICAgJ29uQ29tcGxldGUnOiBvbkNvbXBsZXRlXG4gICAqIH0pO1xuICAgKlxuICAgKiAvLyBvciBuYW1lIGFuZCBvcHRpb25zXG4gICAqIHN1aXRlLmFkZCgnZm9vJywge1xuICAgKiAgICdmbic6IGZuLFxuICAgKiAgICdvbkN5Y2xlJzogb25DeWNsZSxcbiAgICogICAnb25Db21wbGV0ZSc6IG9uQ29tcGxldGVcbiAgICogfSk7XG4gICAqXG4gICAqIC8vIG9yIG9wdGlvbnMgb25seVxuICAgKiBzdWl0ZS5hZGQoe1xuICAgKiAgICduYW1lJzogJ2ZvbycsXG4gICAqICAgJ2ZuJzogZm4sXG4gICAqICAgJ29uQ3ljbGUnOiBvbkN5Y2xlLFxuICAgKiAgICdvbkNvbXBsZXRlJzogb25Db21wbGV0ZVxuICAgKiB9KTtcbiAgICovXG4gIGZ1bmN0aW9uIGFkZChuYW1lLCBmbiwgb3B0aW9ucykge1xuICAgIHZhciBtZSA9IHRoaXMsXG4gICAgICAgIGJlbmNoID0gQmVuY2htYXJrKG5hbWUsIGZuLCBvcHRpb25zKSxcbiAgICAgICAgZXZlbnQgPSBFdmVudCh7ICd0eXBlJzogJ2FkZCcsICd0YXJnZXQnOiBiZW5jaCB9KTtcblxuICAgIGlmIChtZS5lbWl0KGV2ZW50KSwgIWV2ZW50LmNhbmNlbGxlZCkge1xuICAgICAgbWUucHVzaChiZW5jaCk7XG4gICAgfVxuICAgIHJldHVybiBtZTtcbiAgfVxuXG4gIC8qKlxuICAgKiBDcmVhdGVzIGEgbmV3IHN1aXRlIHdpdGggY2xvbmVkIGJlbmNobWFya3MuXG4gICAqXG4gICAqIEBuYW1lIGNsb25lXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IG9wdGlvbnMgT3B0aW9ucyBvYmplY3QgdG8gb3ZlcndyaXRlIGNsb25lZCBvcHRpb25zLlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgbmV3IHN1aXRlIGluc3RhbmNlLlxuICAgKi9cbiAgZnVuY3Rpb24gY2xvbmVTdWl0ZShvcHRpb25zKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgcmVzdWx0ID0gbmV3IG1lLmNvbnN0cnVjdG9yKGV4dGVuZCh7fSwgbWUub3B0aW9ucywgb3B0aW9ucykpO1xuXG4gICAgLy8gY29weSBvd24gcHJvcGVydGllc1xuICAgIGZvck93bihtZSwgZnVuY3Rpb24odmFsdWUsIGtleSkge1xuICAgICAgaWYgKCFoYXNLZXkocmVzdWx0LCBrZXkpKSB7XG4gICAgICAgIHJlc3VsdFtrZXldID0gdmFsdWUgJiYgaXNDbGFzc09mKHZhbHVlLmNsb25lLCAnRnVuY3Rpb24nKVxuICAgICAgICAgID8gdmFsdWUuY2xvbmUoKVxuICAgICAgICAgIDogZGVlcENsb25lKHZhbHVlKTtcbiAgICAgIH1cbiAgICB9KTtcbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIEFuIGBBcnJheSNmaWx0ZXJgIGxpa2UgbWV0aG9kLlxuICAgKlxuICAgKiBAbmFtZSBmaWx0ZXJcbiAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge0Z1bmN0aW9ufFN0cmluZ30gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uL2FsaWFzIGNhbGxlZCBwZXIgaXRlcmF0aW9uLlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBBIG5ldyBzdWl0ZSBvZiBiZW5jaG1hcmtzIHRoYXQgcGFzc2VkIGNhbGxiYWNrIGZpbHRlci5cbiAgICovXG4gIGZ1bmN0aW9uIGZpbHRlclN1aXRlKGNhbGxiYWNrKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgcmVzdWx0ID0gbmV3IG1lLmNvbnN0cnVjdG9yO1xuXG4gICAgcmVzdWx0LnB1c2guYXBwbHkocmVzdWx0LCBmaWx0ZXIobWUsIGNhbGxiYWNrKSk7XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qKlxuICAgKiBSZXNldHMgYWxsIGJlbmNobWFya3MgaW4gdGhlIHN1aXRlLlxuICAgKlxuICAgKiBAbmFtZSByZXNldFxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBzdWl0ZSBpbnN0YW5jZS5cbiAgICovXG4gIGZ1bmN0aW9uIHJlc2V0U3VpdGUoKSB7XG4gICAgdmFyIGV2ZW50LFxuICAgICAgICBtZSA9IHRoaXMsXG4gICAgICAgIGFib3J0aW5nID0gY2FsbGVkQnkuYWJvcnRTdWl0ZTtcblxuICAgIGlmIChtZS5ydW5uaW5nICYmICFhYm9ydGluZykge1xuICAgICAgLy8gbm8gd29ycmllcywgYHJlc2V0U3VpdGUoKWAgaXMgY2FsbGVkIHdpdGhpbiBgYWJvcnRTdWl0ZSgpYFxuICAgICAgY2FsbGVkQnkucmVzZXRTdWl0ZSA9IHRydWU7XG4gICAgICBtZS5hYm9ydCgpO1xuICAgICAgZGVsZXRlIGNhbGxlZEJ5LnJlc2V0U3VpdGU7XG4gICAgfVxuICAgIC8vIHJlc2V0IGlmIHRoZSBzdGF0ZSBoYXMgY2hhbmdlZFxuICAgIGVsc2UgaWYgKChtZS5hYm9ydGVkIHx8IG1lLnJ1bm5pbmcpICYmXG4gICAgICAgIChtZS5lbWl0KGV2ZW50ID0gRXZlbnQoJ3Jlc2V0JykpLCAhZXZlbnQuY2FuY2VsbGVkKSkge1xuICAgICAgbWUucnVubmluZyA9IGZhbHNlO1xuICAgICAgaWYgKCFhYm9ydGluZykge1xuICAgICAgICBpbnZva2UobWUsICdyZXNldCcpO1xuICAgICAgfVxuICAgIH1cbiAgICByZXR1cm4gbWU7XG4gIH1cblxuICAvKipcbiAgICogUnVucyB0aGUgc3VpdGUuXG4gICAqXG4gICAqIEBuYW1lIHJ1blxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBbb3B0aW9ucz17fV0gT3B0aW9ucyBvYmplY3QuXG4gICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBzdWl0ZSBpbnN0YW5jZS5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gYmFzaWMgdXNhZ2VcbiAgICogc3VpdGUucnVuKCk7XG4gICAqXG4gICAqIC8vIG9yIHdpdGggb3B0aW9uc1xuICAgKiBzdWl0ZS5ydW4oeyAnYXN5bmMnOiB0cnVlLCAncXVldWVkJzogdHJ1ZSB9KTtcbiAgICovXG4gIGZ1bmN0aW9uIHJ1blN1aXRlKG9wdGlvbnMpIHtcbiAgICB2YXIgbWUgPSB0aGlzO1xuXG4gICAgbWUucmVzZXQoKTtcbiAgICBtZS5ydW5uaW5nID0gdHJ1ZTtcbiAgICBvcHRpb25zIHx8IChvcHRpb25zID0ge30pO1xuXG4gICAgaW52b2tlKG1lLCB7XG4gICAgICAnbmFtZSc6ICdydW4nLFxuICAgICAgJ2FyZ3MnOiBvcHRpb25zLFxuICAgICAgJ3F1ZXVlZCc6IG9wdGlvbnMucXVldWVkLFxuICAgICAgJ29uU3RhcnQnOiBmdW5jdGlvbihldmVudCkge1xuICAgICAgICBtZS5lbWl0KGV2ZW50KTtcbiAgICAgIH0sXG4gICAgICAnb25DeWNsZSc6IGZ1bmN0aW9uKGV2ZW50KSB7XG4gICAgICAgIHZhciBiZW5jaCA9IGV2ZW50LnRhcmdldDtcbiAgICAgICAgaWYgKGJlbmNoLmVycm9yKSB7XG4gICAgICAgICAgbWUuZW1pdCh7ICd0eXBlJzogJ2Vycm9yJywgJ3RhcmdldCc6IGJlbmNoIH0pO1xuICAgICAgICB9XG4gICAgICAgIG1lLmVtaXQoZXZlbnQpO1xuICAgICAgICBldmVudC5hYm9ydGVkID0gbWUuYWJvcnRlZDtcbiAgICAgIH0sXG4gICAgICAnb25Db21wbGV0ZSc6IGZ1bmN0aW9uKGV2ZW50KSB7XG4gICAgICAgIG1lLnJ1bm5pbmcgPSBmYWxzZTtcbiAgICAgICAgbWUuZW1pdChldmVudCk7XG4gICAgICB9XG4gICAgfSk7XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIEV4ZWN1dGVzIGFsbCByZWdpc3RlcmVkIGxpc3RlbmVycyBvZiB0aGUgc3BlY2lmaWVkIGV2ZW50IHR5cGUuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmssIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge1N0cmluZ3xPYmplY3R9IHR5cGUgVGhlIGV2ZW50IHR5cGUgb3Igb2JqZWN0LlxuICAgKiBAcmV0dXJucyB7TWl4ZWR9IFJldHVybnMgdGhlIHJldHVybiB2YWx1ZSBvZiB0aGUgbGFzdCBsaXN0ZW5lciBleGVjdXRlZC5cbiAgICovXG4gIGZ1bmN0aW9uIGVtaXQodHlwZSkge1xuICAgIHZhciBsaXN0ZW5lcnMsXG4gICAgICAgIG1lID0gdGhpcyxcbiAgICAgICAgZXZlbnQgPSBFdmVudCh0eXBlKSxcbiAgICAgICAgZXZlbnRzID0gbWUuZXZlbnRzLFxuICAgICAgICBhcmdzID0gKGFyZ3VtZW50c1swXSA9IGV2ZW50LCBhcmd1bWVudHMpO1xuXG4gICAgZXZlbnQuY3VycmVudFRhcmdldCB8fCAoZXZlbnQuY3VycmVudFRhcmdldCA9IG1lKTtcbiAgICBldmVudC50YXJnZXQgfHwgKGV2ZW50LnRhcmdldCA9IG1lKTtcbiAgICBkZWxldGUgZXZlbnQucmVzdWx0O1xuXG4gICAgaWYgKGV2ZW50cyAmJiAobGlzdGVuZXJzID0gaGFzS2V5KGV2ZW50cywgZXZlbnQudHlwZSkgJiYgZXZlbnRzW2V2ZW50LnR5cGVdKSkge1xuICAgICAgZm9yRWFjaChsaXN0ZW5lcnMuc2xpY2UoKSwgZnVuY3Rpb24obGlzdGVuZXIpIHtcbiAgICAgICAgaWYgKChldmVudC5yZXN1bHQgPSBsaXN0ZW5lci5hcHBseShtZSwgYXJncykpID09PSBmYWxzZSkge1xuICAgICAgICAgIGV2ZW50LmNhbmNlbGxlZCA9IHRydWU7XG4gICAgICAgIH1cbiAgICAgICAgcmV0dXJuICFldmVudC5hYm9ydGVkO1xuICAgICAgfSk7XG4gICAgfVxuICAgIHJldHVybiBldmVudC5yZXN1bHQ7XG4gIH1cblxuICAvKipcbiAgICogUmV0dXJucyBhbiBhcnJheSBvZiBldmVudCBsaXN0ZW5lcnMgZm9yIGEgZ2l2ZW4gdHlwZSB0aGF0IGNhbiBiZSBtYW5pcHVsYXRlZFxuICAgKiB0byBhZGQgb3IgcmVtb3ZlIGxpc3RlbmVycy5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFyaywgQmVuY2htYXJrLlN1aXRlXG4gICAqIEBwYXJhbSB7U3RyaW5nfSB0eXBlIFRoZSBldmVudCB0eXBlLlxuICAgKiBAcmV0dXJucyB7QXJyYXl9IFRoZSBsaXN0ZW5lcnMgYXJyYXkuXG4gICAqL1xuICBmdW5jdGlvbiBsaXN0ZW5lcnModHlwZSkge1xuICAgIHZhciBtZSA9IHRoaXMsXG4gICAgICAgIGV2ZW50cyA9IG1lLmV2ZW50cyB8fCAobWUuZXZlbnRzID0ge30pO1xuXG4gICAgcmV0dXJuIGhhc0tleShldmVudHMsIHR5cGUpID8gZXZlbnRzW3R5cGVdIDogKGV2ZW50c1t0eXBlXSA9IFtdKTtcbiAgfVxuXG4gIC8qKlxuICAgKiBVbnJlZ2lzdGVycyBhIGxpc3RlbmVyIGZvciB0aGUgc3BlY2lmaWVkIGV2ZW50IHR5cGUocyksXG4gICAqIG9yIHVucmVnaXN0ZXJzIGFsbCBsaXN0ZW5lcnMgZm9yIHRoZSBzcGVjaWZpZWQgZXZlbnQgdHlwZShzKSxcbiAgICogb3IgdW5yZWdpc3RlcnMgYWxsIGxpc3RlbmVycyBmb3IgYWxsIGV2ZW50IHR5cGVzLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLCBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHBhcmFtIHtTdHJpbmd9IFt0eXBlXSBUaGUgZXZlbnQgdHlwZS5cbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gW2xpc3RlbmVyXSBUaGUgZnVuY3Rpb24gdG8gdW5yZWdpc3Rlci5cbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gdW5yZWdpc3RlciBhIGxpc3RlbmVyIGZvciBhbiBldmVudCB0eXBlXG4gICAqIGJlbmNoLm9mZignY3ljbGUnLCBsaXN0ZW5lcik7XG4gICAqXG4gICAqIC8vIHVucmVnaXN0ZXIgYSBsaXN0ZW5lciBmb3IgbXVsdGlwbGUgZXZlbnQgdHlwZXNcbiAgICogYmVuY2gub2ZmKCdzdGFydCBjeWNsZScsIGxpc3RlbmVyKTtcbiAgICpcbiAgICogLy8gdW5yZWdpc3RlciBhbGwgbGlzdGVuZXJzIGZvciBhbiBldmVudCB0eXBlXG4gICAqIGJlbmNoLm9mZignY3ljbGUnKTtcbiAgICpcbiAgICogLy8gdW5yZWdpc3RlciBhbGwgbGlzdGVuZXJzIGZvciBtdWx0aXBsZSBldmVudCB0eXBlc1xuICAgKiBiZW5jaC5vZmYoJ3N0YXJ0IGN5Y2xlIGNvbXBsZXRlJyk7XG4gICAqXG4gICAqIC8vIHVucmVnaXN0ZXIgYWxsIGxpc3RlbmVycyBmb3IgYWxsIGV2ZW50IHR5cGVzXG4gICAqIGJlbmNoLm9mZigpO1xuICAgKi9cbiAgZnVuY3Rpb24gb2ZmKHR5cGUsIGxpc3RlbmVyKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgZXZlbnRzID0gbWUuZXZlbnRzO1xuXG4gICAgZXZlbnRzICYmIGVhY2godHlwZSA/IHR5cGUuc3BsaXQoJyAnKSA6IGV2ZW50cywgZnVuY3Rpb24obGlzdGVuZXJzLCB0eXBlKSB7XG4gICAgICB2YXIgaW5kZXg7XG4gICAgICBpZiAodHlwZW9mIGxpc3RlbmVycyA9PSAnc3RyaW5nJykge1xuICAgICAgICB0eXBlID0gbGlzdGVuZXJzO1xuICAgICAgICBsaXN0ZW5lcnMgPSBoYXNLZXkoZXZlbnRzLCB0eXBlKSAmJiBldmVudHNbdHlwZV07XG4gICAgICB9XG4gICAgICBpZiAobGlzdGVuZXJzKSB7XG4gICAgICAgIGlmIChsaXN0ZW5lcikge1xuICAgICAgICAgIGluZGV4ID0gaW5kZXhPZihsaXN0ZW5lcnMsIGxpc3RlbmVyKTtcbiAgICAgICAgICBpZiAoaW5kZXggPiAtMSkge1xuICAgICAgICAgICAgbGlzdGVuZXJzLnNwbGljZShpbmRleCwgMSk7XG4gICAgICAgICAgfVxuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIGxpc3RlbmVycy5sZW5ndGggPSAwO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfSk7XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyoqXG4gICAqIFJlZ2lzdGVycyBhIGxpc3RlbmVyIGZvciB0aGUgc3BlY2lmaWVkIGV2ZW50IHR5cGUocykuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmssIEJlbmNobWFyay5TdWl0ZVxuICAgKiBAcGFyYW0ge1N0cmluZ30gdHlwZSBUaGUgZXZlbnQgdHlwZS5cbiAgICogQHBhcmFtIHtGdW5jdGlvbn0gbGlzdGVuZXIgVGhlIGZ1bmN0aW9uIHRvIHJlZ2lzdGVyLlxuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKiBAZXhhbXBsZVxuICAgKlxuICAgKiAvLyByZWdpc3RlciBhIGxpc3RlbmVyIGZvciBhbiBldmVudCB0eXBlXG4gICAqIGJlbmNoLm9uKCdjeWNsZScsIGxpc3RlbmVyKTtcbiAgICpcbiAgICogLy8gcmVnaXN0ZXIgYSBsaXN0ZW5lciBmb3IgbXVsdGlwbGUgZXZlbnQgdHlwZXNcbiAgICogYmVuY2gub24oJ3N0YXJ0IGN5Y2xlJywgbGlzdGVuZXIpO1xuICAgKi9cbiAgZnVuY3Rpb24gb24odHlwZSwgbGlzdGVuZXIpIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICBldmVudHMgPSBtZS5ldmVudHMgfHwgKG1lLmV2ZW50cyA9IHt9KTtcblxuICAgIGZvckVhY2godHlwZS5zcGxpdCgnICcpLCBmdW5jdGlvbih0eXBlKSB7XG4gICAgICAoaGFzS2V5KGV2ZW50cywgdHlwZSlcbiAgICAgICAgPyBldmVudHNbdHlwZV1cbiAgICAgICAgOiAoZXZlbnRzW3R5cGVdID0gW10pXG4gICAgICApLnB1c2gobGlzdGVuZXIpO1xuICAgIH0pO1xuICAgIHJldHVybiBtZTtcbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBBYm9ydHMgdGhlIGJlbmNobWFyayB3aXRob3V0IHJlY29yZGluZyB0aW1lcy5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgYmVuY2htYXJrIGluc3RhbmNlLlxuICAgKi9cbiAgZnVuY3Rpb24gYWJvcnQoKSB7XG4gICAgdmFyIGV2ZW50LFxuICAgICAgICBtZSA9IHRoaXMsXG4gICAgICAgIHJlc2V0dGluZyA9IGNhbGxlZEJ5LnJlc2V0O1xuXG4gICAgaWYgKG1lLnJ1bm5pbmcpIHtcbiAgICAgIGV2ZW50ID0gRXZlbnQoJ2Fib3J0Jyk7XG4gICAgICBtZS5lbWl0KGV2ZW50KTtcbiAgICAgIGlmICghZXZlbnQuY2FuY2VsbGVkIHx8IHJlc2V0dGluZykge1xuICAgICAgICAvLyBhdm9pZCBpbmZpbml0ZSByZWN1cnNpb25cbiAgICAgICAgY2FsbGVkQnkuYWJvcnQgPSB0cnVlO1xuICAgICAgICBtZS5yZXNldCgpO1xuICAgICAgICBkZWxldGUgY2FsbGVkQnkuYWJvcnQ7XG5cbiAgICAgICAgaWYgKHN1cHBvcnQudGltZW91dCkge1xuICAgICAgICAgIGNsZWFyVGltZW91dChtZS5fdGltZXJJZCk7XG4gICAgICAgICAgZGVsZXRlIG1lLl90aW1lcklkO1xuICAgICAgICB9XG4gICAgICAgIGlmICghcmVzZXR0aW5nKSB7XG4gICAgICAgICAgbWUuYWJvcnRlZCA9IHRydWU7XG4gICAgICAgICAgbWUucnVubmluZyA9IGZhbHNlO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiBtZTtcbiAgfVxuXG4gIC8qKlxuICAgKiBDcmVhdGVzIGEgbmV3IGJlbmNobWFyayB1c2luZyB0aGUgc2FtZSB0ZXN0IGFuZCBvcHRpb25zLlxuICAgKlxuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBvcHRpb25zIE9wdGlvbnMgb2JqZWN0IHRvIG92ZXJ3cml0ZSBjbG9uZWQgb3B0aW9ucy5cbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIG5ldyBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqIEBleGFtcGxlXG4gICAqXG4gICAqIHZhciBiaXphcnJvID0gYmVuY2guY2xvbmUoe1xuICAgKiAgICduYW1lJzogJ2RvcHBlbGdhbmdlcidcbiAgICogfSk7XG4gICAqL1xuICBmdW5jdGlvbiBjbG9uZShvcHRpb25zKSB7XG4gICAgdmFyIG1lID0gdGhpcyxcbiAgICAgICAgcmVzdWx0ID0gbmV3IG1lLmNvbnN0cnVjdG9yKGV4dGVuZCh7fSwgbWUsIG9wdGlvbnMpKTtcblxuICAgIC8vIGNvcnJlY3QgdGhlIGBvcHRpb25zYCBvYmplY3RcbiAgICByZXN1bHQub3B0aW9ucyA9IGV4dGVuZCh7fSwgbWUub3B0aW9ucywgb3B0aW9ucyk7XG5cbiAgICAvLyBjb3B5IG93biBjdXN0b20gcHJvcGVydGllc1xuICAgIGZvck93bihtZSwgZnVuY3Rpb24odmFsdWUsIGtleSkge1xuICAgICAgaWYgKCFoYXNLZXkocmVzdWx0LCBrZXkpKSB7XG4gICAgICAgIHJlc3VsdFtrZXldID0gZGVlcENsb25lKHZhbHVlKTtcbiAgICAgIH1cbiAgICB9KTtcbiAgICByZXR1cm4gcmVzdWx0O1xuICB9XG5cbiAgLyoqXG4gICAqIERldGVybWluZXMgaWYgYSBiZW5jaG1hcmsgaXMgZmFzdGVyIHRoYW4gYW5vdGhlci5cbiAgICpcbiAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgKiBAcGFyYW0ge09iamVjdH0gb3RoZXIgVGhlIGJlbmNobWFyayB0byBjb21wYXJlLlxuICAgKiBAcmV0dXJucyB7TnVtYmVyfSBSZXR1cm5zIGAtMWAgaWYgc2xvd2VyLCBgMWAgaWYgZmFzdGVyLCBhbmQgYDBgIGlmIGluZGV0ZXJtaW5hdGUuXG4gICAqL1xuICBmdW5jdGlvbiBjb21wYXJlKG90aGVyKSB7XG4gICAgdmFyIGNyaXRpY2FsLFxuICAgICAgICB6U3RhdCxcbiAgICAgICAgbWUgPSB0aGlzLFxuICAgICAgICBzYW1wbGUxID0gbWUuc3RhdHMuc2FtcGxlLFxuICAgICAgICBzYW1wbGUyID0gb3RoZXIuc3RhdHMuc2FtcGxlLFxuICAgICAgICBzaXplMSA9IHNhbXBsZTEubGVuZ3RoLFxuICAgICAgICBzaXplMiA9IHNhbXBsZTIubGVuZ3RoLFxuICAgICAgICBtYXhTaXplID0gbWF4KHNpemUxLCBzaXplMiksXG4gICAgICAgIG1pblNpemUgPSBtaW4oc2l6ZTEsIHNpemUyKSxcbiAgICAgICAgdTEgPSBnZXRVKHNhbXBsZTEsIHNhbXBsZTIpLFxuICAgICAgICB1MiA9IGdldFUoc2FtcGxlMiwgc2FtcGxlMSksXG4gICAgICAgIHUgPSBtaW4odTEsIHUyKTtcblxuICAgIGZ1bmN0aW9uIGdldFNjb3JlKHhBLCBzYW1wbGVCKSB7XG4gICAgICByZXR1cm4gcmVkdWNlKHNhbXBsZUIsIGZ1bmN0aW9uKHRvdGFsLCB4Qikge1xuICAgICAgICByZXR1cm4gdG90YWwgKyAoeEIgPiB4QSA/IDAgOiB4QiA8IHhBID8gMSA6IDAuNSk7XG4gICAgICB9LCAwKTtcbiAgICB9XG5cbiAgICBmdW5jdGlvbiBnZXRVKHNhbXBsZUEsIHNhbXBsZUIpIHtcbiAgICAgIHJldHVybiByZWR1Y2Uoc2FtcGxlQSwgZnVuY3Rpb24odG90YWwsIHhBKSB7XG4gICAgICAgIHJldHVybiB0b3RhbCArIGdldFNjb3JlKHhBLCBzYW1wbGVCKTtcbiAgICAgIH0sIDApO1xuICAgIH1cblxuICAgIGZ1bmN0aW9uIGdldFoodSkge1xuICAgICAgcmV0dXJuICh1IC0gKChzaXplMSAqIHNpemUyKSAvIDIpKSAvIHNxcnQoKHNpemUxICogc2l6ZTIgKiAoc2l6ZTEgKyBzaXplMiArIDEpKSAvIDEyKTtcbiAgICB9XG5cbiAgICAvLyBleGl0IGVhcmx5IGlmIGNvbXBhcmluZyB0aGUgc2FtZSBiZW5jaG1hcmtcbiAgICBpZiAobWUgPT0gb3RoZXIpIHtcbiAgICAgIHJldHVybiAwO1xuICAgIH1cbiAgICAvLyByZWplY3QgdGhlIG51bGwgaHlwaG90aGVzaXMgdGhlIHR3byBzYW1wbGVzIGNvbWUgZnJvbSB0aGVcbiAgICAvLyBzYW1lIHBvcHVsYXRpb24gKGkuZS4gaGF2ZSB0aGUgc2FtZSBtZWRpYW4pIGlmLi4uXG4gICAgaWYgKHNpemUxICsgc2l6ZTIgPiAzMCkge1xuICAgICAgLy8gLi4udGhlIHotc3RhdCBpcyBncmVhdGVyIHRoYW4gMS45NiBvciBsZXNzIHRoYW4gLTEuOTZcbiAgICAgIC8vIGh0dHA6Ly93d3cuc3RhdGlzdGljc2xlY3R1cmVzLmNvbS90b3BpY3MvbWFubndoaXRuZXl1L1xuICAgICAgelN0YXQgPSBnZXRaKHUpO1xuICAgICAgcmV0dXJuIGFicyh6U3RhdCkgPiAxLjk2ID8gKHpTdGF0ID4gMCA/IC0xIDogMSkgOiAwO1xuICAgIH1cbiAgICAvLyAuLi50aGUgVSB2YWx1ZSBpcyBsZXNzIHRoYW4gb3IgZXF1YWwgdGhlIGNyaXRpY2FsIFUgdmFsdWVcbiAgICAvLyBodHRwOi8vd3d3Lmdlb2liLmNvbS9tYW5uLXdoaXRuZXktdS10ZXN0Lmh0bWxcbiAgICBjcml0aWNhbCA9IG1heFNpemUgPCA1IHx8IG1pblNpemUgPCAzID8gMCA6IHVUYWJsZVttYXhTaXplXVttaW5TaXplIC0gM107XG4gICAgcmV0dXJuIHUgPD0gY3JpdGljYWwgPyAodSA9PSB1MSA/IDEgOiAtMSkgOiAwO1xuICB9XG5cbiAgLyoqXG4gICAqIFJlc2V0IHByb3BlcnRpZXMgYW5kIGFib3J0IGlmIHJ1bm5pbmcuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICovXG4gIGZ1bmN0aW9uIHJlc2V0KCkge1xuICAgIHZhciBkYXRhLFxuICAgICAgICBldmVudCxcbiAgICAgICAgbWUgPSB0aGlzLFxuICAgICAgICBpbmRleCA9IDAsXG4gICAgICAgIGNoYW5nZXMgPSB7ICdsZW5ndGgnOiAwIH0sXG4gICAgICAgIHF1ZXVlID0geyAnbGVuZ3RoJzogMCB9O1xuXG4gICAgaWYgKG1lLnJ1bm5pbmcgJiYgIWNhbGxlZEJ5LmFib3J0KSB7XG4gICAgICAvLyBubyB3b3JyaWVzLCBgcmVzZXQoKWAgaXMgY2FsbGVkIHdpdGhpbiBgYWJvcnQoKWBcbiAgICAgIGNhbGxlZEJ5LnJlc2V0ID0gdHJ1ZTtcbiAgICAgIG1lLmFib3J0KCk7XG4gICAgICBkZWxldGUgY2FsbGVkQnkucmVzZXQ7XG4gICAgfVxuICAgIGVsc2Uge1xuICAgICAgLy8gYSBub24tcmVjdXJzaXZlIHNvbHV0aW9uIHRvIGNoZWNrIGlmIHByb3BlcnRpZXMgaGF2ZSBjaGFuZ2VkXG4gICAgICAvLyBodHRwOi8vd3d3LmpzbGFiLmRrL2FydGljbGVzL25vbi5yZWN1cnNpdmUucHJlb3JkZXIudHJhdmVyc2FsLnBhcnQ0XG4gICAgICBkYXRhID0geyAnZGVzdGluYXRpb24nOiBtZSwgJ3NvdXJjZSc6IGV4dGVuZCh7fSwgbWUuY29uc3RydWN0b3IucHJvdG90eXBlLCBtZS5vcHRpb25zKSB9O1xuICAgICAgZG8ge1xuICAgICAgICBmb3JPd24oZGF0YS5zb3VyY2UsIGZ1bmN0aW9uKHZhbHVlLCBrZXkpIHtcbiAgICAgICAgICB2YXIgY2hhbmdlZCxcbiAgICAgICAgICAgICAgZGVzdGluYXRpb24gPSBkYXRhLmRlc3RpbmF0aW9uLFxuICAgICAgICAgICAgICBjdXJyVmFsdWUgPSBkZXN0aW5hdGlvbltrZXldO1xuXG4gICAgICAgICAgaWYgKHZhbHVlICYmIHR5cGVvZiB2YWx1ZSA9PSAnb2JqZWN0Jykge1xuICAgICAgICAgICAgaWYgKGlzQ2xhc3NPZih2YWx1ZSwgJ0FycmF5JykpIHtcbiAgICAgICAgICAgICAgLy8gY2hlY2sgaWYgYW4gYXJyYXkgdmFsdWUgaGFzIGNoYW5nZWQgdG8gYSBub24tYXJyYXkgdmFsdWVcbiAgICAgICAgICAgICAgaWYgKCFpc0NsYXNzT2YoY3VyclZhbHVlLCAnQXJyYXknKSkge1xuICAgICAgICAgICAgICAgIGNoYW5nZWQgPSBjdXJyVmFsdWUgPSBbXTtcbiAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgICAvLyBvciBoYXMgY2hhbmdlZCBpdHMgbGVuZ3RoXG4gICAgICAgICAgICAgIGlmIChjdXJyVmFsdWUubGVuZ3RoICE9IHZhbHVlLmxlbmd0aCkge1xuICAgICAgICAgICAgICAgIGNoYW5nZWQgPSBjdXJyVmFsdWUgPSBjdXJyVmFsdWUuc2xpY2UoMCwgdmFsdWUubGVuZ3RoKTtcbiAgICAgICAgICAgICAgICBjdXJyVmFsdWUubGVuZ3RoID0gdmFsdWUubGVuZ3RoO1xuICAgICAgICAgICAgICB9XG4gICAgICAgICAgICB9XG4gICAgICAgICAgICAvLyBjaGVjayBpZiBhbiBvYmplY3QgaGFzIGNoYW5nZWQgdG8gYSBub24tb2JqZWN0IHZhbHVlXG4gICAgICAgICAgICBlbHNlIGlmICghY3VyclZhbHVlIHx8IHR5cGVvZiBjdXJyVmFsdWUgIT0gJ29iamVjdCcpIHtcbiAgICAgICAgICAgICAgY2hhbmdlZCA9IGN1cnJWYWx1ZSA9IHt9O1xuICAgICAgICAgICAgfVxuICAgICAgICAgICAgLy8gcmVnaXN0ZXIgYSBjaGFuZ2VkIG9iamVjdFxuICAgICAgICAgICAgaWYgKGNoYW5nZWQpIHtcbiAgICAgICAgICAgICAgY2hhbmdlc1tjaGFuZ2VzLmxlbmd0aCsrXSA9IHsgJ2Rlc3RpbmF0aW9uJzogZGVzdGluYXRpb24sICdrZXknOiBrZXksICd2YWx1ZSc6IGN1cnJWYWx1ZSB9O1xuICAgICAgICAgICAgfVxuICAgICAgICAgICAgcXVldWVbcXVldWUubGVuZ3RoKytdID0geyAnZGVzdGluYXRpb24nOiBjdXJyVmFsdWUsICdzb3VyY2UnOiB2YWx1ZSB9O1xuICAgICAgICAgIH1cbiAgICAgICAgICAvLyByZWdpc3RlciBhIGNoYW5nZWQgcHJpbWl0aXZlXG4gICAgICAgICAgZWxzZSBpZiAodmFsdWUgIT09IGN1cnJWYWx1ZSAmJiAhKHZhbHVlID09IG51bGwgfHwgaXNDbGFzc09mKHZhbHVlLCAnRnVuY3Rpb24nKSkpIHtcbiAgICAgICAgICAgIGNoYW5nZXNbY2hhbmdlcy5sZW5ndGgrK10gPSB7ICdkZXN0aW5hdGlvbic6IGRlc3RpbmF0aW9uLCAna2V5Jzoga2V5LCAndmFsdWUnOiB2YWx1ZSB9O1xuICAgICAgICAgIH1cbiAgICAgICAgfSk7XG4gICAgICB9XG4gICAgICB3aGlsZSAoKGRhdGEgPSBxdWV1ZVtpbmRleCsrXSkpO1xuXG4gICAgICAvLyBpZiBjaGFuZ2VkIGVtaXQgdGhlIGByZXNldGAgZXZlbnQgYW5kIGlmIGl0IGlzbid0IGNhbmNlbGxlZCByZXNldCB0aGUgYmVuY2htYXJrXG4gICAgICBpZiAoY2hhbmdlcy5sZW5ndGggJiYgKG1lLmVtaXQoZXZlbnQgPSBFdmVudCgncmVzZXQnKSksICFldmVudC5jYW5jZWxsZWQpKSB7XG4gICAgICAgIGZvckVhY2goY2hhbmdlcywgZnVuY3Rpb24oZGF0YSkge1xuICAgICAgICAgIGRhdGEuZGVzdGluYXRpb25bZGF0YS5rZXldID0gZGF0YS52YWx1ZTtcbiAgICAgICAgfSk7XG4gICAgICB9XG4gICAgfVxuICAgIHJldHVybiBtZTtcbiAgfVxuXG4gIC8qKlxuICAgKiBEaXNwbGF5cyByZWxldmFudCBiZW5jaG1hcmsgaW5mb3JtYXRpb24gd2hlbiBjb2VyY2VkIHRvIGEgc3RyaW5nLlxuICAgKlxuICAgKiBAbmFtZSB0b1N0cmluZ1xuICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAqIEByZXR1cm5zIHtTdHJpbmd9IEEgc3RyaW5nIHJlcHJlc2VudGF0aW9uIG9mIHRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqL1xuICBmdW5jdGlvbiB0b1N0cmluZ0JlbmNoKCkge1xuICAgIHZhciBtZSA9IHRoaXMsXG4gICAgICAgIGVycm9yID0gbWUuZXJyb3IsXG4gICAgICAgIGh6ID0gbWUuaHosXG4gICAgICAgIGlkID0gbWUuaWQsXG4gICAgICAgIHN0YXRzID0gbWUuc3RhdHMsXG4gICAgICAgIHNpemUgPSBzdGF0cy5zYW1wbGUubGVuZ3RoLFxuICAgICAgICBwbSA9IHN1cHBvcnQuamF2YSA/ICcrLy0nIDogJ1xceGIxJyxcbiAgICAgICAgcmVzdWx0ID0gbWUubmFtZSB8fCAoaXNOYU4oaWQpID8gaWQgOiAnPFRlc3QgIycgKyBpZCArICc+Jyk7XG5cbiAgICBpZiAoZXJyb3IpIHtcbiAgICAgIHJlc3VsdCArPSAnOiAnICsgam9pbihlcnJvcik7XG4gICAgfSBlbHNlIHtcbiAgICAgIHJlc3VsdCArPSAnIHggJyArIGZvcm1hdE51bWJlcihoei50b0ZpeGVkKGh6IDwgMTAwID8gMiA6IDApKSArICcgb3BzL3NlYyAnICsgcG0gK1xuICAgICAgICBzdGF0cy5ybWUudG9GaXhlZCgyKSArICclICgnICsgc2l6ZSArICcgcnVuJyArIChzaXplID09IDEgPyAnJyA6ICdzJykgKyAnIHNhbXBsZWQpJztcbiAgICB9XG4gICAgcmV0dXJuIHJlc3VsdDtcbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBDbG9ja3MgdGhlIHRpbWUgdGFrZW4gdG8gZXhlY3V0ZSBhIHRlc3QgcGVyIGN5Y2xlIChzZWNzKS5cbiAgICpcbiAgICogQHByaXZhdGVcbiAgICogQHBhcmFtIHtPYmplY3R9IGJlbmNoIFRoZSBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAqIEByZXR1cm5zIHtOdW1iZXJ9IFRoZSB0aW1lIHRha2VuLlxuICAgKi9cbiAgZnVuY3Rpb24gY2xvY2soKSB7XG4gICAgdmFyIGFwcGxldCxcbiAgICAgICAgb3B0aW9ucyA9IEJlbmNobWFyay5vcHRpb25zLFxuICAgICAgICB0ZW1wbGF0ZSA9IHsgJ2JlZ2luJzogJ3MkPW5ldyBuJCcsICdlbmQnOiAnciQ9KG5ldyBuJC1zJCkvMWUzJywgJ3VpZCc6IHVpZCB9LFxuICAgICAgICB0aW1lcnMgPSBbeyAnbnMnOiB0aW1lci5ucywgJ3Jlcyc6IG1heCgwLjAwMTUsIGdldFJlcygnbXMnKSksICd1bml0JzogJ21zJyB9XTtcblxuICAgIC8vIGxhenkgZGVmaW5lIGZvciBoaS1yZXMgdGltZXJzXG4gICAgY2xvY2sgPSBmdW5jdGlvbihjbG9uZSkge1xuICAgICAgdmFyIGRlZmVycmVkO1xuICAgICAgaWYgKGNsb25lIGluc3RhbmNlb2YgRGVmZXJyZWQpIHtcbiAgICAgICAgZGVmZXJyZWQgPSBjbG9uZTtcbiAgICAgICAgY2xvbmUgPSBkZWZlcnJlZC5iZW5jaG1hcms7XG4gICAgICB9XG5cbiAgICAgIHZhciBiZW5jaCA9IGNsb25lLl9vcmlnaW5hbCxcbiAgICAgICAgICBmbiA9IGJlbmNoLmZuLFxuICAgICAgICAgIGZuQXJnID0gZGVmZXJyZWQgPyBnZXRGaXJzdEFyZ3VtZW50KGZuKSB8fCAnZGVmZXJyZWQnIDogJycsXG4gICAgICAgICAgc3RyaW5nYWJsZSA9IGlzU3RyaW5nYWJsZShmbik7XG5cbiAgICAgIHZhciBzb3VyY2UgPSB7XG4gICAgICAgICdzZXR1cCc6IGdldFNvdXJjZShiZW5jaC5zZXR1cCwgcHJlcHJvY2VzcygnbSQuc2V0dXAoKScpKSxcbiAgICAgICAgJ2ZuJzogZ2V0U291cmNlKGZuLCBwcmVwcm9jZXNzKCdtJC5mbignICsgZm5BcmcgKyAnKScpKSxcbiAgICAgICAgJ2ZuQXJnJzogZm5BcmcsXG4gICAgICAgICd0ZWFyZG93bic6IGdldFNvdXJjZShiZW5jaC50ZWFyZG93biwgcHJlcHJvY2VzcygnbSQudGVhcmRvd24oKScpKVxuICAgICAgfTtcblxuICAgICAgdmFyIGNvdW50ID0gYmVuY2guY291bnQgPSBjbG9uZS5jb3VudCxcbiAgICAgICAgICBkZWNvbXBpbGFibGUgPSBzdXBwb3J0LmRlY29tcGlsYXRpb24gfHwgc3RyaW5nYWJsZSxcbiAgICAgICAgICBpZCA9IGJlbmNoLmlkLFxuICAgICAgICAgIGlzRW1wdHkgPSAhKHNvdXJjZS5mbiB8fCBzdHJpbmdhYmxlKSxcbiAgICAgICAgICBuYW1lID0gYmVuY2gubmFtZSB8fCAodHlwZW9mIGlkID09ICdudW1iZXInID8gJzxUZXN0ICMnICsgaWQgKyAnPicgOiBpZCksXG4gICAgICAgICAgbnMgPSB0aW1lci5ucyxcbiAgICAgICAgICByZXN1bHQgPSAwO1xuXG4gICAgICAvLyBpbml0IGBtaW5UaW1lYCBpZiBuZWVkZWRcbiAgICAgIGNsb25lLm1pblRpbWUgPSBiZW5jaC5taW5UaW1lIHx8IChiZW5jaC5taW5UaW1lID0gYmVuY2gub3B0aW9ucy5taW5UaW1lID0gb3B0aW9ucy5taW5UaW1lKTtcblxuICAgICAgLy8gcmVwYWlyIG5hbm9zZWNvbmQgdGltZXJcbiAgICAgIC8vIChzb21lIENocm9tZSBidWlsZHMgZXJhc2UgdGhlIGBuc2AgdmFyaWFibGUgYWZ0ZXIgbWlsbGlvbnMgb2YgZXhlY3V0aW9ucylcbiAgICAgIGlmIChhcHBsZXQpIHtcbiAgICAgICAgdHJ5IHtcbiAgICAgICAgICBucy5uYW5vVGltZSgpO1xuICAgICAgICB9IGNhdGNoKGUpIHtcbiAgICAgICAgICAvLyB1c2Ugbm9uLWVsZW1lbnQgdG8gYXZvaWQgaXNzdWVzIHdpdGggbGlicyB0aGF0IGF1Z21lbnQgdGhlbVxuICAgICAgICAgIG5zID0gdGltZXIubnMgPSBuZXcgYXBwbGV0LlBhY2thZ2VzLm5hbm87XG4gICAgICAgIH1cbiAgICAgIH1cblxuICAgICAgLy8gQ29tcGlsZSBpbiBzZXR1cC90ZWFyZG93biBmdW5jdGlvbnMgYW5kIHRoZSB0ZXN0IGxvb3AuXG4gICAgICAvLyBDcmVhdGUgYSBuZXcgY29tcGlsZWQgdGVzdCwgaW5zdGVhZCBvZiB1c2luZyB0aGUgY2FjaGVkIGBiZW5jaC5jb21waWxlZGAsXG4gICAgICAvLyB0byBhdm9pZCBwb3RlbnRpYWwgZW5naW5lIG9wdGltaXphdGlvbnMgZW5hYmxlZCBvdmVyIHRoZSBsaWZlIG9mIHRoZSB0ZXN0LlxuICAgICAgdmFyIGNvbXBpbGVkID0gYmVuY2guY29tcGlsZWQgPSBjcmVhdGVGdW5jdGlvbihwcmVwcm9jZXNzKCd0JCcpLCBpbnRlcnBvbGF0ZShcbiAgICAgICAgcHJlcHJvY2VzcyhkZWZlcnJlZFxuICAgICAgICAgID8gJ3ZhciBkJD10aGlzLCN7Zm5Bcmd9PWQkLG0kPWQkLmJlbmNobWFyay5fb3JpZ2luYWwsZiQ9bSQuZm4sc3UkPW0kLnNldHVwLHRkJD1tJC50ZWFyZG93bjsnICtcbiAgICAgICAgICAgIC8vIHdoZW4gYGRlZmVycmVkLmN5Y2xlc2AgaXMgYDBgIHRoZW4uLi5cbiAgICAgICAgICAgICdpZighZCQuY3ljbGVzKXsnICtcbiAgICAgICAgICAgIC8vIHNldCBgZGVmZXJyZWQuZm5gXG4gICAgICAgICAgICAnZCQuZm49ZnVuY3Rpb24oKXt2YXIgI3tmbkFyZ309ZCQ7aWYodHlwZW9mIGYkPT1cImZ1bmN0aW9uXCIpe3RyeXsje2ZufVxcbn1jYXRjaChlJCl7ZiQoZCQpfX1lbHNleyN7Zm59XFxufX07JyArXG4gICAgICAgICAgICAvLyBzZXQgYGRlZmVycmVkLnRlYXJkb3duYFxuICAgICAgICAgICAgJ2QkLnRlYXJkb3duPWZ1bmN0aW9uKCl7ZCQuY3ljbGVzPTA7aWYodHlwZW9mIHRkJD09XCJmdW5jdGlvblwiKXt0cnl7I3t0ZWFyZG93bn1cXG59Y2F0Y2goZSQpe3RkJCgpfX1lbHNleyN7dGVhcmRvd259XFxufX07JyArXG4gICAgICAgICAgICAvLyBleGVjdXRlIHRoZSBiZW5jaG1hcmsncyBgc2V0dXBgXG4gICAgICAgICAgICAnaWYodHlwZW9mIHN1JD09XCJmdW5jdGlvblwiKXt0cnl7I3tzZXR1cH1cXG59Y2F0Y2goZSQpe3N1JCgpfX1lbHNleyN7c2V0dXB9XFxufTsnICtcbiAgICAgICAgICAgIC8vIHN0YXJ0IHRpbWVyXG4gICAgICAgICAgICAndCQuc3RhcnQoZCQpOycgK1xuICAgICAgICAgICAgLy8gZXhlY3V0ZSBgZGVmZXJyZWQuZm5gIGFuZCByZXR1cm4gYSBkdW1teSBvYmplY3RcbiAgICAgICAgICAgICd9ZCQuZm4oKTtyZXR1cm57fSdcblxuICAgICAgICAgIDogJ3ZhciByJCxzJCxtJD10aGlzLGYkPW0kLmZuLGkkPW0kLmNvdW50LG4kPXQkLm5zOyN7c2V0dXB9XFxuI3tiZWdpbn07JyArXG4gICAgICAgICAgICAnd2hpbGUoaSQtLSl7I3tmbn1cXG59I3tlbmR9OyN7dGVhcmRvd259XFxucmV0dXJue2VsYXBzZWQ6ciQsdWlkOlwiI3t1aWR9XCJ9JyksXG4gICAgICAgIHNvdXJjZVxuICAgICAgKSk7XG5cbiAgICAgIHRyeSB7XG4gICAgICAgIGlmIChpc0VtcHR5KSB7XG4gICAgICAgICAgLy8gRmlyZWZveCBtYXkgcmVtb3ZlIGRlYWQgY29kZSBmcm9tIEZ1bmN0aW9uI3RvU3RyaW5nIHJlc3VsdHNcbiAgICAgICAgICAvLyBodHRwOi8vYnVnemlsLmxhLzUzNjA4NVxuICAgICAgICAgIHRocm93IG5ldyBFcnJvcignVGhlIHRlc3QgXCInICsgbmFtZSArICdcIiBpcyBlbXB0eS4gVGhpcyBtYXkgYmUgdGhlIHJlc3VsdCBvZiBkZWFkIGNvZGUgcmVtb3ZhbC4nKTtcbiAgICAgICAgfVxuICAgICAgICBlbHNlIGlmICghZGVmZXJyZWQpIHtcbiAgICAgICAgICAvLyBwcmV0ZXN0IHRvIGRldGVybWluZSBpZiBjb21waWxlZCBjb2RlIGlzIGV4aXRzIGVhcmx5LCB1c3VhbGx5IGJ5IGFcbiAgICAgICAgICAvLyByb2d1ZSBgcmV0dXJuYCBzdGF0ZW1lbnQsIGJ5IGNoZWNraW5nIGZvciBhIHJldHVybiBvYmplY3Qgd2l0aCB0aGUgdWlkXG4gICAgICAgICAgYmVuY2guY291bnQgPSAxO1xuICAgICAgICAgIGNvbXBpbGVkID0gKGNvbXBpbGVkLmNhbGwoYmVuY2gsIHRpbWVyKSB8fCB7fSkudWlkID09IHVpZCAmJiBjb21waWxlZDtcbiAgICAgICAgICBiZW5jaC5jb3VudCA9IGNvdW50O1xuICAgICAgICB9XG4gICAgICB9IGNhdGNoKGUpIHtcbiAgICAgICAgY29tcGlsZWQgPSBudWxsO1xuICAgICAgICBjbG9uZS5lcnJvciA9IGUgfHwgbmV3IEVycm9yKFN0cmluZyhlKSk7XG4gICAgICAgIGJlbmNoLmNvdW50ID0gY291bnQ7XG4gICAgICB9XG4gICAgICAvLyBmYWxsYmFjayB3aGVuIGEgdGVzdCBleGl0cyBlYXJseSBvciBlcnJvcnMgZHVyaW5nIHByZXRlc3RcbiAgICAgIGlmIChkZWNvbXBpbGFibGUgJiYgIWNvbXBpbGVkICYmICFkZWZlcnJlZCAmJiAhaXNFbXB0eSkge1xuICAgICAgICBjb21waWxlZCA9IGNyZWF0ZUZ1bmN0aW9uKHByZXByb2Nlc3MoJ3QkJyksIGludGVycG9sYXRlKFxuICAgICAgICAgIHByZXByb2Nlc3MoXG4gICAgICAgICAgICAoY2xvbmUuZXJyb3IgJiYgIXN0cmluZ2FibGVcbiAgICAgICAgICAgICAgPyAndmFyIHIkLHMkLG0kPXRoaXMsZiQ9bSQuZm4saSQ9bSQuY291bnQnXG4gICAgICAgICAgICAgIDogJ2Z1bmN0aW9uIGYkKCl7I3tmbn1cXG59dmFyIHIkLHMkLG0kPXRoaXMsaSQ9bSQuY291bnQnXG4gICAgICAgICAgICApICtcbiAgICAgICAgICAgICcsbiQ9dCQubnM7I3tzZXR1cH1cXG4je2JlZ2lufTttJC5mJD1mJDt3aGlsZShpJC0tKXttJC5mJCgpfSN7ZW5kfTsnICtcbiAgICAgICAgICAgICdkZWxldGUgbSQuZiQ7I3t0ZWFyZG93bn1cXG5yZXR1cm57ZWxhcHNlZDpyJH0nXG4gICAgICAgICAgKSxcbiAgICAgICAgICBzb3VyY2VcbiAgICAgICAgKSk7XG5cbiAgICAgICAgdHJ5IHtcbiAgICAgICAgICAvLyBwcmV0ZXN0IG9uZSBtb3JlIHRpbWUgdG8gY2hlY2sgZm9yIGVycm9yc1xuICAgICAgICAgIGJlbmNoLmNvdW50ID0gMTtcbiAgICAgICAgICBjb21waWxlZC5jYWxsKGJlbmNoLCB0aW1lcik7XG4gICAgICAgICAgYmVuY2guY29tcGlsZWQgPSBjb21waWxlZDtcbiAgICAgICAgICBiZW5jaC5jb3VudCA9IGNvdW50O1xuICAgICAgICAgIGRlbGV0ZSBjbG9uZS5lcnJvcjtcbiAgICAgICAgfVxuICAgICAgICBjYXRjaChlKSB7XG4gICAgICAgICAgYmVuY2guY291bnQgPSBjb3VudDtcbiAgICAgICAgICBpZiAoY2xvbmUuZXJyb3IpIHtcbiAgICAgICAgICAgIGNvbXBpbGVkID0gbnVsbDtcbiAgICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgICAgYmVuY2guY29tcGlsZWQgPSBjb21waWxlZDtcbiAgICAgICAgICAgIGNsb25lLmVycm9yID0gZSB8fCBuZXcgRXJyb3IoU3RyaW5nKGUpKTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICAgIC8vIGFzc2lnbiBgY29tcGlsZWRgIHRvIGBjbG9uZWAgYmVmb3JlIGNhbGxpbmcgaW4gY2FzZSBhIGRlZmVycmVkIGJlbmNobWFya1xuICAgICAgLy8gaW1tZWRpYXRlbHkgY2FsbHMgYGRlZmVycmVkLnJlc29sdmUoKWBcbiAgICAgIGNsb25lLmNvbXBpbGVkID0gY29tcGlsZWQ7XG4gICAgICAvLyBpZiBubyBlcnJvcnMgcnVuIHRoZSBmdWxsIHRlc3QgbG9vcFxuICAgICAgaWYgKCFjbG9uZS5lcnJvcikge1xuICAgICAgICByZXN1bHQgPSBjb21waWxlZC5jYWxsKGRlZmVycmVkIHx8IGJlbmNoLCB0aW1lcikuZWxhcHNlZDtcbiAgICAgIH1cbiAgICAgIHJldHVybiByZXN1bHQ7XG4gICAgfTtcblxuICAgIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAgIC8qKlxuICAgICAqIEdldHMgdGhlIGN1cnJlbnQgdGltZXIncyBtaW5pbXVtIHJlc29sdXRpb24gKHNlY3MpLlxuICAgICAqL1xuICAgIGZ1bmN0aW9uIGdldFJlcyh1bml0KSB7XG4gICAgICB2YXIgbWVhc3VyZWQsXG4gICAgICAgICAgYmVnaW4sXG4gICAgICAgICAgY291bnQgPSAzMCxcbiAgICAgICAgICBkaXZpc29yID0gMWUzLFxuICAgICAgICAgIG5zID0gdGltZXIubnMsXG4gICAgICAgICAgc2FtcGxlID0gW107XG5cbiAgICAgIC8vIGdldCBhdmVyYWdlIHNtYWxsZXN0IG1lYXN1cmFibGUgdGltZVxuICAgICAgd2hpbGUgKGNvdW50LS0pIHtcbiAgICAgICAgaWYgKHVuaXQgPT0gJ3VzJykge1xuICAgICAgICAgIGRpdmlzb3IgPSAxZTY7XG4gICAgICAgICAgaWYgKG5zLnN0b3ApIHtcbiAgICAgICAgICAgIG5zLnN0YXJ0KCk7XG4gICAgICAgICAgICB3aGlsZSAoIShtZWFzdXJlZCA9IG5zLm1pY3Jvc2Vjb25kcygpKSkgeyB9XG4gICAgICAgICAgfSBlbHNlIGlmIChuc1twZXJmTmFtZV0pIHtcbiAgICAgICAgICAgIGRpdmlzb3IgPSAxZTM7XG4gICAgICAgICAgICBtZWFzdXJlZCA9IEZ1bmN0aW9uKCduJywgJ3ZhciByLHM9bi4nICsgcGVyZk5hbWUgKyAnKCk7d2hpbGUoIShyPW4uJyArIHBlcmZOYW1lICsgJygpLXMpKXt9O3JldHVybiByJykobnMpO1xuICAgICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgICBiZWdpbiA9IG5zKCk7XG4gICAgICAgICAgICB3aGlsZSAoIShtZWFzdXJlZCA9IG5zKCkgLSBiZWdpbikpIHsgfVxuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgICBlbHNlIGlmICh1bml0ID09ICducycpIHtcbiAgICAgICAgICBkaXZpc29yID0gMWU5O1xuICAgICAgICAgIGlmIChucy5uYW5vVGltZSkge1xuICAgICAgICAgICAgYmVnaW4gPSBucy5uYW5vVGltZSgpO1xuICAgICAgICAgICAgd2hpbGUgKCEobWVhc3VyZWQgPSBucy5uYW5vVGltZSgpIC0gYmVnaW4pKSB7IH1cbiAgICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgICAgYmVnaW4gPSAoYmVnaW4gPSBucygpKVswXSArIChiZWdpblsxXSAvIGRpdmlzb3IpO1xuICAgICAgICAgICAgd2hpbGUgKCEobWVhc3VyZWQgPSAoKG1lYXN1cmVkID0gbnMoKSlbMF0gKyAobWVhc3VyZWRbMV0gLyBkaXZpc29yKSkgLSBiZWdpbikpIHsgfVxuICAgICAgICAgICAgZGl2aXNvciA9IDE7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICAgIGVsc2Uge1xuICAgICAgICAgIGJlZ2luID0gbmV3IG5zO1xuICAgICAgICAgIHdoaWxlICghKG1lYXN1cmVkID0gbmV3IG5zIC0gYmVnaW4pKSB7IH1cbiAgICAgICAgfVxuICAgICAgICAvLyBjaGVjayBmb3IgYnJva2VuIHRpbWVycyAobmFub1RpbWUgbWF5IGhhdmUgaXNzdWVzKVxuICAgICAgICAvLyBodHRwOi8vYWxpdmVidXRzbGVlcHkuc3JuZXQuY3ovdW5yZWxpYWJsZS1zeXN0ZW0tbmFub3RpbWUvXG4gICAgICAgIGlmIChtZWFzdXJlZCA+IDApIHtcbiAgICAgICAgICBzYW1wbGUucHVzaChtZWFzdXJlZCk7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgc2FtcGxlLnB1c2goSW5maW5pdHkpO1xuICAgICAgICAgIGJyZWFrO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgICAvLyBjb252ZXJ0IHRvIHNlY29uZHNcbiAgICAgIHJldHVybiBnZXRNZWFuKHNhbXBsZSkgLyBkaXZpc29yO1xuICAgIH1cblxuICAgIC8qKlxuICAgICAqIFJlcGxhY2VzIGFsbCBvY2N1cnJlbmNlcyBvZiBgJGAgd2l0aCBhIHVuaXF1ZSBudW1iZXIgYW5kXG4gICAgICogdGVtcGxhdGUgdG9rZW5zIHdpdGggY29udGVudC5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiBwcmVwcm9jZXNzKGNvZGUpIHtcbiAgICAgIHJldHVybiBpbnRlcnBvbGF0ZShjb2RlLCB0ZW1wbGF0ZSkucmVwbGFjZSgvXFwkL2csIC9cXGQrLy5leGVjKHVpZCkpO1xuICAgIH1cblxuICAgIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAgIC8vIGRldGVjdCBuYW5vc2Vjb25kIHN1cHBvcnQgZnJvbSBhIEphdmEgYXBwbGV0XG4gICAgZWFjaChkb2MgJiYgZG9jLmFwcGxldHMgfHwgW10sIGZ1bmN0aW9uKGVsZW1lbnQpIHtcbiAgICAgIHJldHVybiAhKHRpbWVyLm5zID0gYXBwbGV0ID0gJ25hbm9UaW1lJyBpbiBlbGVtZW50ICYmIGVsZW1lbnQpO1xuICAgIH0pO1xuXG4gICAgLy8gY2hlY2sgdHlwZSBpbiBjYXNlIFNhZmFyaSByZXR1cm5zIGFuIG9iamVjdCBpbnN0ZWFkIG9mIGEgbnVtYmVyXG4gICAgdHJ5IHtcbiAgICAgIGlmICh0eXBlb2YgdGltZXIubnMubmFub1RpbWUoKSA9PSAnbnVtYmVyJykge1xuICAgICAgICB0aW1lcnMucHVzaCh7ICducyc6IHRpbWVyLm5zLCAncmVzJzogZ2V0UmVzKCducycpLCAndW5pdCc6ICducycgfSk7XG4gICAgICB9XG4gICAgfSBjYXRjaChlKSB7IH1cblxuICAgIC8vIGRldGVjdCBDaHJvbWUncyBtaWNyb3NlY29uZCB0aW1lcjpcbiAgICAvLyBlbmFibGUgYmVuY2htYXJraW5nIHZpYSB0aGUgLS1lbmFibGUtYmVuY2htYXJraW5nIGNvbW1hbmRcbiAgICAvLyBsaW5lIHN3aXRjaCBpbiBhdCBsZWFzdCBDaHJvbWUgNyB0byB1c2UgY2hyb21lLkludGVydmFsXG4gICAgdHJ5IHtcbiAgICAgIGlmICgodGltZXIubnMgPSBuZXcgKHdpbmRvdy5jaHJvbWUgfHwgd2luZG93LmNocm9taXVtKS5JbnRlcnZhbCkpIHtcbiAgICAgICAgdGltZXJzLnB1c2goeyAnbnMnOiB0aW1lci5ucywgJ3Jlcyc6IGdldFJlcygndXMnKSwgJ3VuaXQnOiAndXMnIH0pO1xuICAgICAgfVxuICAgIH0gY2F0Y2goZSkgeyB9XG5cbiAgICAvLyBkZXRlY3QgYHBlcmZvcm1hbmNlLm5vd2AgbWljcm9zZWNvbmQgcmVzb2x1dGlvbiB0aW1lclxuICAgIGlmICgodGltZXIubnMgPSBwZXJmTmFtZSAmJiBwZXJmT2JqZWN0KSkge1xuICAgICAgdGltZXJzLnB1c2goeyAnbnMnOiB0aW1lci5ucywgJ3Jlcyc6IGdldFJlcygndXMnKSwgJ3VuaXQnOiAndXMnIH0pO1xuICAgIH1cblxuICAgIC8vIGRldGVjdCBOb2RlJ3MgbmFub3NlY29uZCByZXNvbHV0aW9uIHRpbWVyIGF2YWlsYWJsZSBpbiBOb2RlID49IDAuOFxuICAgIGlmIChwcm9jZXNzT2JqZWN0ICYmIHR5cGVvZiAodGltZXIubnMgPSBwcm9jZXNzT2JqZWN0LmhydGltZSkgPT0gJ2Z1bmN0aW9uJykge1xuICAgICAgdGltZXJzLnB1c2goeyAnbnMnOiB0aW1lci5ucywgJ3Jlcyc6IGdldFJlcygnbnMnKSwgJ3VuaXQnOiAnbnMnIH0pO1xuICAgIH1cblxuICAgIC8vIGRldGVjdCBXYWRlIFNpbW1vbnMnIE5vZGUgbWljcm90aW1lIG1vZHVsZVxuICAgIGlmIChtaWNyb3RpbWVPYmplY3QgJiYgdHlwZW9mICh0aW1lci5ucyA9IG1pY3JvdGltZU9iamVjdC5ub3cpID09ICdmdW5jdGlvbicpIHtcbiAgICAgIHRpbWVycy5wdXNoKHsgJ25zJzogdGltZXIubnMsICAncmVzJzogZ2V0UmVzKCd1cycpLCAndW5pdCc6ICd1cycgfSk7XG4gICAgfVxuXG4gICAgLy8gcGljayB0aW1lciB3aXRoIGhpZ2hlc3QgcmVzb2x1dGlvblxuICAgIHRpbWVyID0gcmVkdWNlKHRpbWVycywgZnVuY3Rpb24odGltZXIsIG90aGVyKSB7XG4gICAgICByZXR1cm4gb3RoZXIucmVzIDwgdGltZXIucmVzID8gb3RoZXIgOiB0aW1lcjtcbiAgICB9KTtcblxuICAgIC8vIHJlbW92ZSB1bnVzZWQgYXBwbGV0XG4gICAgaWYgKHRpbWVyLnVuaXQgIT0gJ25zJyAmJiBhcHBsZXQpIHtcbiAgICAgIGFwcGxldCA9IGRlc3Ryb3lFbGVtZW50KGFwcGxldCk7XG4gICAgfVxuICAgIC8vIGVycm9yIGlmIHRoZXJlIGFyZSBubyB3b3JraW5nIHRpbWVyc1xuICAgIGlmICh0aW1lci5yZXMgPT0gSW5maW5pdHkpIHtcbiAgICAgIHRocm93IG5ldyBFcnJvcignQmVuY2htYXJrLmpzIHdhcyB1bmFibGUgdG8gZmluZCBhIHdvcmtpbmcgdGltZXIuJyk7XG4gICAgfVxuICAgIC8vIHVzZSBBUEkgb2YgY2hvc2VuIHRpbWVyXG4gICAgaWYgKHRpbWVyLnVuaXQgPT0gJ25zJykge1xuICAgICAgaWYgKHRpbWVyLm5zLm5hbm9UaW1lKSB7XG4gICAgICAgIGV4dGVuZCh0ZW1wbGF0ZSwge1xuICAgICAgICAgICdiZWdpbic6ICdzJD1uJC5uYW5vVGltZSgpJyxcbiAgICAgICAgICAnZW5kJzogJ3IkPShuJC5uYW5vVGltZSgpLXMkKS8xZTknXG4gICAgICAgIH0pO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgZXh0ZW5kKHRlbXBsYXRlLCB7XG4gICAgICAgICAgJ2JlZ2luJzogJ3MkPW4kKCknLFxuICAgICAgICAgICdlbmQnOiAnciQ9biQocyQpO3IkPXIkWzBdKyhyJFsxXS8xZTkpJ1xuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICB9XG4gICAgZWxzZSBpZiAodGltZXIudW5pdCA9PSAndXMnKSB7XG4gICAgICBpZiAodGltZXIubnMuc3RvcCkge1xuICAgICAgICBleHRlbmQodGVtcGxhdGUsIHtcbiAgICAgICAgICAnYmVnaW4nOiAncyQ9biQuc3RhcnQoKScsXG4gICAgICAgICAgJ2VuZCc6ICdyJD1uJC5taWNyb3NlY29uZHMoKS8xZTYnXG4gICAgICAgIH0pO1xuICAgICAgfSBlbHNlIGlmIChwZXJmTmFtZSkge1xuICAgICAgICBleHRlbmQodGVtcGxhdGUsIHtcbiAgICAgICAgICAnYmVnaW4nOiAncyQ9biQuJyArIHBlcmZOYW1lICsgJygpJyxcbiAgICAgICAgICAnZW5kJzogJ3IkPShuJC4nICsgcGVyZk5hbWUgKyAnKCktcyQpLzFlMydcbiAgICAgICAgfSk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBleHRlbmQodGVtcGxhdGUsIHtcbiAgICAgICAgICAnYmVnaW4nOiAncyQ9biQoKScsXG4gICAgICAgICAgJ2VuZCc6ICdyJD0obiQoKS1zJCkvMWU2J1xuICAgICAgICB9KTtcbiAgICAgIH1cbiAgICB9XG5cbiAgICAvLyBkZWZpbmUgYHRpbWVyYCBtZXRob2RzXG4gICAgdGltZXIuc3RhcnQgPSBjcmVhdGVGdW5jdGlvbihwcmVwcm9jZXNzKCdvJCcpLFxuICAgICAgcHJlcHJvY2VzcygndmFyIG4kPXRoaXMubnMsI3tiZWdpbn07byQuZWxhcHNlZD0wO28kLnRpbWVTdGFtcD1zJCcpKTtcblxuICAgIHRpbWVyLnN0b3AgPSBjcmVhdGVGdW5jdGlvbihwcmVwcm9jZXNzKCdvJCcpLFxuICAgICAgcHJlcHJvY2VzcygndmFyIG4kPXRoaXMubnMscyQ9byQudGltZVN0YW1wLCN7ZW5kfTtvJC5lbGFwc2VkPXIkJykpO1xuXG4gICAgLy8gcmVzb2x2ZSB0aW1lIHNwYW4gcmVxdWlyZWQgdG8gYWNoaWV2ZSBhIHBlcmNlbnQgdW5jZXJ0YWludHkgb2YgYXQgbW9zdCAxJVxuICAgIC8vIGh0dHA6Ly9zcGlmZi5yaXQuZWR1L2NsYXNzZXMvcGh5czI3My91bmNlcnQvdW5jZXJ0Lmh0bWxcbiAgICBvcHRpb25zLm1pblRpbWUgfHwgKG9wdGlvbnMubWluVGltZSA9IG1heCh0aW1lci5yZXMgLyAyIC8gMC4wMSwgMC4wNSkpO1xuICAgIHJldHVybiBjbG9jay5hcHBseShudWxsLCBhcmd1bWVudHMpO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLyoqXG4gICAqIENvbXB1dGVzIHN0YXRzIG9uIGJlbmNobWFyayByZXN1bHRzLlxuICAgKlxuICAgKiBAcHJpdmF0ZVxuICAgKiBAcGFyYW0ge09iamVjdH0gYmVuY2ggVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQHBhcmFtIHtPYmplY3R9IG9wdGlvbnMgVGhlIG9wdGlvbnMgb2JqZWN0LlxuICAgKi9cbiAgZnVuY3Rpb24gY29tcHV0ZShiZW5jaCwgb3B0aW9ucykge1xuICAgIG9wdGlvbnMgfHwgKG9wdGlvbnMgPSB7fSk7XG5cbiAgICB2YXIgYXN5bmMgPSBvcHRpb25zLmFzeW5jLFxuICAgICAgICBlbGFwc2VkID0gMCxcbiAgICAgICAgaW5pdENvdW50ID0gYmVuY2guaW5pdENvdW50LFxuICAgICAgICBtaW5TYW1wbGVzID0gYmVuY2gubWluU2FtcGxlcyxcbiAgICAgICAgcXVldWUgPSBbXSxcbiAgICAgICAgc2FtcGxlID0gYmVuY2guc3RhdHMuc2FtcGxlO1xuXG4gICAgLyoqXG4gICAgICogQWRkcyBhIGNsb25lIHRvIHRoZSBxdWV1ZS5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiBlbnF1ZXVlKCkge1xuICAgICAgcXVldWUucHVzaChiZW5jaC5jbG9uZSh7XG4gICAgICAgICdfb3JpZ2luYWwnOiBiZW5jaCxcbiAgICAgICAgJ2V2ZW50cyc6IHtcbiAgICAgICAgICAnYWJvcnQnOiBbdXBkYXRlXSxcbiAgICAgICAgICAnY3ljbGUnOiBbdXBkYXRlXSxcbiAgICAgICAgICAnZXJyb3InOiBbdXBkYXRlXSxcbiAgICAgICAgICAnc3RhcnQnOiBbdXBkYXRlXVxuICAgICAgICB9XG4gICAgICB9KSk7XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogVXBkYXRlcyB0aGUgY2xvbmUvb3JpZ2luYWwgYmVuY2htYXJrcyB0byBrZWVwIHRoZWlyIGRhdGEgaW4gc3luYy5cbiAgICAgKi9cbiAgICBmdW5jdGlvbiB1cGRhdGUoZXZlbnQpIHtcbiAgICAgIHZhciBjbG9uZSA9IHRoaXMsXG4gICAgICAgICAgdHlwZSA9IGV2ZW50LnR5cGU7XG5cbiAgICAgIGlmIChiZW5jaC5ydW5uaW5nKSB7XG4gICAgICAgIGlmICh0eXBlID09ICdzdGFydCcpIHtcbiAgICAgICAgICAvLyBOb3RlOiBgY2xvbmUubWluVGltZWAgcHJvcCBpcyBpbml0ZWQgaW4gYGNsb2NrKClgXG4gICAgICAgICAgY2xvbmUuY291bnQgPSBiZW5jaC5pbml0Q291bnQ7XG4gICAgICAgIH1cbiAgICAgICAgZWxzZSB7XG4gICAgICAgICAgaWYgKHR5cGUgPT0gJ2Vycm9yJykge1xuICAgICAgICAgICAgYmVuY2guZXJyb3IgPSBjbG9uZS5lcnJvcjtcbiAgICAgICAgICB9XG4gICAgICAgICAgaWYgKHR5cGUgPT0gJ2Fib3J0Jykge1xuICAgICAgICAgICAgYmVuY2guYWJvcnQoKTtcbiAgICAgICAgICAgIGJlbmNoLmVtaXQoJ2N5Y2xlJyk7XG4gICAgICAgICAgfSBlbHNlIHtcbiAgICAgICAgICAgIGV2ZW50LmN1cnJlbnRUYXJnZXQgPSBldmVudC50YXJnZXQgPSBiZW5jaDtcbiAgICAgICAgICAgIGJlbmNoLmVtaXQoZXZlbnQpO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgfSBlbHNlIGlmIChiZW5jaC5hYm9ydGVkKSB7XG4gICAgICAgIC8vIGNsZWFyIGFib3J0IGxpc3RlbmVycyB0byBhdm9pZCB0cmlnZ2VyaW5nIGJlbmNoJ3MgYWJvcnQvY3ljbGUgYWdhaW5cbiAgICAgICAgY2xvbmUuZXZlbnRzLmFib3J0Lmxlbmd0aCA9IDA7XG4gICAgICAgIGNsb25lLmFib3J0KCk7XG4gICAgICB9XG4gICAgfVxuXG4gICAgLyoqXG4gICAgICogRGV0ZXJtaW5lcyBpZiBtb3JlIGNsb25lcyBzaG91bGQgYmUgcXVldWVkIG9yIGlmIGN5Y2xpbmcgc2hvdWxkIHN0b3AuXG4gICAgICovXG4gICAgZnVuY3Rpb24gZXZhbHVhdGUoZXZlbnQpIHtcbiAgICAgIHZhciBjcml0aWNhbCxcbiAgICAgICAgICBkZixcbiAgICAgICAgICBtZWFuLFxuICAgICAgICAgIG1vZSxcbiAgICAgICAgICBybWUsXG4gICAgICAgICAgc2QsXG4gICAgICAgICAgc2VtLFxuICAgICAgICAgIHZhcmlhbmNlLFxuICAgICAgICAgIGNsb25lID0gZXZlbnQudGFyZ2V0LFxuICAgICAgICAgIGRvbmUgPSBiZW5jaC5hYm9ydGVkLFxuICAgICAgICAgIG5vdyA9ICtuZXcgRGF0ZSxcbiAgICAgICAgICBzaXplID0gc2FtcGxlLnB1c2goY2xvbmUudGltZXMucGVyaW9kKSxcbiAgICAgICAgICBtYXhlZE91dCA9IHNpemUgPj0gbWluU2FtcGxlcyAmJiAoZWxhcHNlZCArPSBub3cgLSBjbG9uZS50aW1lcy50aW1lU3RhbXApIC8gMWUzID4gYmVuY2gubWF4VGltZSxcbiAgICAgICAgICB0aW1lcyA9IGJlbmNoLnRpbWVzLFxuICAgICAgICAgIHZhck9mID0gZnVuY3Rpb24oc3VtLCB4KSB7IHJldHVybiBzdW0gKyBwb3coeCAtIG1lYW4sIDIpOyB9O1xuXG4gICAgICAvLyBleGl0IGVhcmx5IGZvciBhYm9ydGVkIG9yIHVuY2xvY2thYmxlIHRlc3RzXG4gICAgICBpZiAoZG9uZSB8fCBjbG9uZS5oeiA9PSBJbmZpbml0eSkge1xuICAgICAgICBtYXhlZE91dCA9ICEoc2l6ZSA9IHNhbXBsZS5sZW5ndGggPSBxdWV1ZS5sZW5ndGggPSAwKTtcbiAgICAgIH1cblxuICAgICAgaWYgKCFkb25lKSB7XG4gICAgICAgIC8vIHNhbXBsZSBtZWFuIChlc3RpbWF0ZSBvZiB0aGUgcG9wdWxhdGlvbiBtZWFuKVxuICAgICAgICBtZWFuID0gZ2V0TWVhbihzYW1wbGUpO1xuICAgICAgICAvLyBzYW1wbGUgdmFyaWFuY2UgKGVzdGltYXRlIG9mIHRoZSBwb3B1bGF0aW9uIHZhcmlhbmNlKVxuICAgICAgICB2YXJpYW5jZSA9IHJlZHVjZShzYW1wbGUsIHZhck9mLCAwKSAvIChzaXplIC0gMSkgfHwgMDtcbiAgICAgICAgLy8gc2FtcGxlIHN0YW5kYXJkIGRldmlhdGlvbiAoZXN0aW1hdGUgb2YgdGhlIHBvcHVsYXRpb24gc3RhbmRhcmQgZGV2aWF0aW9uKVxuICAgICAgICBzZCA9IHNxcnQodmFyaWFuY2UpO1xuICAgICAgICAvLyBzdGFuZGFyZCBlcnJvciBvZiB0aGUgbWVhbiAoYS5rLmEuIHRoZSBzdGFuZGFyZCBkZXZpYXRpb24gb2YgdGhlIHNhbXBsaW5nIGRpc3RyaWJ1dGlvbiBvZiB0aGUgc2FtcGxlIG1lYW4pXG4gICAgICAgIHNlbSA9IHNkIC8gc3FydChzaXplKTtcbiAgICAgICAgLy8gZGVncmVlcyBvZiBmcmVlZG9tXG4gICAgICAgIGRmID0gc2l6ZSAtIDE7XG4gICAgICAgIC8vIGNyaXRpY2FsIHZhbHVlXG4gICAgICAgIGNyaXRpY2FsID0gdFRhYmxlW01hdGgucm91bmQoZGYpIHx8IDFdIHx8IHRUYWJsZS5pbmZpbml0eTtcbiAgICAgICAgLy8gbWFyZ2luIG9mIGVycm9yXG4gICAgICAgIG1vZSA9IHNlbSAqIGNyaXRpY2FsO1xuICAgICAgICAvLyByZWxhdGl2ZSBtYXJnaW4gb2YgZXJyb3JcbiAgICAgICAgcm1lID0gKG1vZSAvIG1lYW4pICogMTAwIHx8IDA7XG5cbiAgICAgICAgZXh0ZW5kKGJlbmNoLnN0YXRzLCB7XG4gICAgICAgICAgJ2RldmlhdGlvbic6IHNkLFxuICAgICAgICAgICdtZWFuJzogbWVhbixcbiAgICAgICAgICAnbW9lJzogbW9lLFxuICAgICAgICAgICdybWUnOiBybWUsXG4gICAgICAgICAgJ3NlbSc6IHNlbSxcbiAgICAgICAgICAndmFyaWFuY2UnOiB2YXJpYW5jZVxuICAgICAgICB9KTtcblxuICAgICAgICAvLyBBYm9ydCB0aGUgY3ljbGUgbG9vcCB3aGVuIHRoZSBtaW5pbXVtIHNhbXBsZSBzaXplIGhhcyBiZWVuIGNvbGxlY3RlZFxuICAgICAgICAvLyBhbmQgdGhlIGVsYXBzZWQgdGltZSBleGNlZWRzIHRoZSBtYXhpbXVtIHRpbWUgYWxsb3dlZCBwZXIgYmVuY2htYXJrLlxuICAgICAgICAvLyBXZSBkb24ndCBjb3VudCBjeWNsZSBkZWxheXMgdG93YXJkIHRoZSBtYXggdGltZSBiZWNhdXNlIGRlbGF5cyBtYXkgYmVcbiAgICAgICAgLy8gaW5jcmVhc2VkIGJ5IGJyb3dzZXJzIHRoYXQgY2xhbXAgdGltZW91dHMgZm9yIGluYWN0aXZlIHRhYnMuXG4gICAgICAgIC8vIGh0dHBzOi8vZGV2ZWxvcGVyLm1vemlsbGEub3JnL2VuL3dpbmRvdy5zZXRUaW1lb3V0I0luYWN0aXZlX3RhYnNcbiAgICAgICAgaWYgKG1heGVkT3V0KSB7XG4gICAgICAgICAgLy8gcmVzZXQgdGhlIGBpbml0Q291bnRgIGluIGNhc2UgdGhlIGJlbmNobWFyayBpcyByZXJ1blxuICAgICAgICAgIGJlbmNoLmluaXRDb3VudCA9IGluaXRDb3VudDtcbiAgICAgICAgICBiZW5jaC5ydW5uaW5nID0gZmFsc2U7XG4gICAgICAgICAgZG9uZSA9IHRydWU7XG4gICAgICAgICAgdGltZXMuZWxhcHNlZCA9IChub3cgLSB0aW1lcy50aW1lU3RhbXApIC8gMWUzO1xuICAgICAgICB9XG4gICAgICAgIGlmIChiZW5jaC5oeiAhPSBJbmZpbml0eSkge1xuICAgICAgICAgIGJlbmNoLmh6ID0gMSAvIG1lYW47XG4gICAgICAgICAgdGltZXMuY3ljbGUgPSBtZWFuICogYmVuY2guY291bnQ7XG4gICAgICAgICAgdGltZXMucGVyaW9kID0gbWVhbjtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgLy8gaWYgdGltZSBwZXJtaXRzLCBpbmNyZWFzZSBzYW1wbGUgc2l6ZSB0byByZWR1Y2UgdGhlIG1hcmdpbiBvZiBlcnJvclxuICAgICAgaWYgKHF1ZXVlLmxlbmd0aCA8IDIgJiYgIW1heGVkT3V0KSB7XG4gICAgICAgIGVucXVldWUoKTtcbiAgICAgIH1cbiAgICAgIC8vIGFib3J0IHRoZSBpbnZva2UgY3ljbGUgd2hlbiBkb25lXG4gICAgICBldmVudC5hYm9ydGVkID0gZG9uZTtcbiAgICB9XG5cbiAgICAvLyBpbml0IHF1ZXVlIGFuZCBiZWdpblxuICAgIGVucXVldWUoKTtcbiAgICBpbnZva2UocXVldWUsIHtcbiAgICAgICduYW1lJzogJ3J1bicsXG4gICAgICAnYXJncyc6IHsgJ2FzeW5jJzogYXN5bmMgfSxcbiAgICAgICdxdWV1ZWQnOiB0cnVlLFxuICAgICAgJ29uQ3ljbGUnOiBldmFsdWF0ZSxcbiAgICAgICdvbkNvbXBsZXRlJzogZnVuY3Rpb24oKSB7IGJlbmNoLmVtaXQoJ2NvbXBsZXRlJyk7IH1cbiAgICB9KTtcbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBDeWNsZXMgYSBiZW5jaG1hcmsgdW50aWwgYSBydW4gYGNvdW50YCBjYW4gYmUgZXN0YWJsaXNoZWQuXG4gICAqXG4gICAqIEBwcml2YXRlXG4gICAqIEBwYXJhbSB7T2JqZWN0fSBjbG9uZSBUaGUgY2xvbmVkIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQHBhcmFtIHtPYmplY3R9IG9wdGlvbnMgVGhlIG9wdGlvbnMgb2JqZWN0LlxuICAgKi9cbiAgZnVuY3Rpb24gY3ljbGUoY2xvbmUsIG9wdGlvbnMpIHtcbiAgICBvcHRpb25zIHx8IChvcHRpb25zID0ge30pO1xuXG4gICAgdmFyIGRlZmVycmVkO1xuICAgIGlmIChjbG9uZSBpbnN0YW5jZW9mIERlZmVycmVkKSB7XG4gICAgICBkZWZlcnJlZCA9IGNsb25lO1xuICAgICAgY2xvbmUgPSBjbG9uZS5iZW5jaG1hcms7XG4gICAgfVxuXG4gICAgdmFyIGNsb2NrZWQsXG4gICAgICAgIGN5Y2xlcyxcbiAgICAgICAgZGl2aXNvcixcbiAgICAgICAgZXZlbnQsXG4gICAgICAgIG1pblRpbWUsXG4gICAgICAgIHBlcmlvZCxcbiAgICAgICAgYXN5bmMgPSBvcHRpb25zLmFzeW5jLFxuICAgICAgICBiZW5jaCA9IGNsb25lLl9vcmlnaW5hbCxcbiAgICAgICAgY291bnQgPSBjbG9uZS5jb3VudCxcbiAgICAgICAgdGltZXMgPSBjbG9uZS50aW1lcztcblxuICAgIC8vIGNvbnRpbnVlLCBpZiBub3QgYWJvcnRlZCBiZXR3ZWVuIGN5Y2xlc1xuICAgIGlmIChjbG9uZS5ydW5uaW5nKSB7XG4gICAgICAvLyBgbWluVGltZWAgaXMgc2V0IHRvIGBCZW5jaG1hcmsub3B0aW9ucy5taW5UaW1lYCBpbiBgY2xvY2soKWBcbiAgICAgIGN5Y2xlcyA9ICsrY2xvbmUuY3ljbGVzO1xuICAgICAgY2xvY2tlZCA9IGRlZmVycmVkID8gZGVmZXJyZWQuZWxhcHNlZCA6IGNsb2NrKGNsb25lKTtcbiAgICAgIG1pblRpbWUgPSBjbG9uZS5taW5UaW1lO1xuXG4gICAgICBpZiAoY3ljbGVzID4gYmVuY2guY3ljbGVzKSB7XG4gICAgICAgIGJlbmNoLmN5Y2xlcyA9IGN5Y2xlcztcbiAgICAgIH1cbiAgICAgIGlmIChjbG9uZS5lcnJvcikge1xuICAgICAgICBldmVudCA9IEV2ZW50KCdlcnJvcicpO1xuICAgICAgICBldmVudC5tZXNzYWdlID0gY2xvbmUuZXJyb3I7XG4gICAgICAgIGNsb25lLmVtaXQoZXZlbnQpO1xuICAgICAgICBpZiAoIWV2ZW50LmNhbmNlbGxlZCkge1xuICAgICAgICAgIGNsb25lLmFib3J0KCk7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG5cbiAgICAvLyBjb250aW51ZSwgaWYgbm90IGVycm9yZWRcbiAgICBpZiAoY2xvbmUucnVubmluZykge1xuICAgICAgLy8gdGltZSB0YWtlbiB0byBjb21wbGV0ZSBsYXN0IHRlc3QgY3ljbGVcbiAgICAgIGJlbmNoLnRpbWVzLmN5Y2xlID0gdGltZXMuY3ljbGUgPSBjbG9ja2VkO1xuICAgICAgLy8gc2Vjb25kcyBwZXIgb3BlcmF0aW9uXG4gICAgICBwZXJpb2QgPSBiZW5jaC50aW1lcy5wZXJpb2QgPSB0aW1lcy5wZXJpb2QgPSBjbG9ja2VkIC8gY291bnQ7XG4gICAgICAvLyBvcHMgcGVyIHNlY29uZFxuICAgICAgYmVuY2guaHogPSBjbG9uZS5oeiA9IDEgLyBwZXJpb2Q7XG4gICAgICAvLyBhdm9pZCB3b3JraW5nIG91ciB3YXkgdXAgdG8gdGhpcyBuZXh0IHRpbWVcbiAgICAgIGJlbmNoLmluaXRDb3VudCA9IGNsb25lLmluaXRDb3VudCA9IGNvdW50O1xuICAgICAgLy8gZG8gd2UgbmVlZCB0byBkbyBhbm90aGVyIGN5Y2xlP1xuICAgICAgY2xvbmUucnVubmluZyA9IGNsb2NrZWQgPCBtaW5UaW1lO1xuXG4gICAgICBpZiAoY2xvbmUucnVubmluZykge1xuICAgICAgICAvLyB0ZXN0cyBtYXkgY2xvY2sgYXQgYDBgIHdoZW4gYGluaXRDb3VudGAgaXMgYSBzbWFsbCBudW1iZXIsXG4gICAgICAgIC8vIHRvIGF2b2lkIHRoYXQgd2Ugc2V0IGl0cyBjb3VudCB0byBzb21ldGhpbmcgYSBiaXQgaGlnaGVyXG4gICAgICAgIGlmICghY2xvY2tlZCAmJiAoZGl2aXNvciA9IGRpdmlzb3JzW2Nsb25lLmN5Y2xlc10pICE9IG51bGwpIHtcbiAgICAgICAgICBjb3VudCA9IGZsb29yKDRlNiAvIGRpdmlzb3IpO1xuICAgICAgICB9XG4gICAgICAgIC8vIGNhbGN1bGF0ZSBob3cgbWFueSBtb3JlIGl0ZXJhdGlvbnMgaXQgd2lsbCB0YWtlIHRvIGFjaGl2ZSB0aGUgYG1pblRpbWVgXG4gICAgICAgIGlmIChjb3VudCA8PSBjbG9uZS5jb3VudCkge1xuICAgICAgICAgIGNvdW50ICs9IE1hdGguY2VpbCgobWluVGltZSAtIGNsb2NrZWQpIC8gcGVyaW9kKTtcbiAgICAgICAgfVxuICAgICAgICBjbG9uZS5ydW5uaW5nID0gY291bnQgIT0gSW5maW5pdHk7XG4gICAgICB9XG4gICAgfVxuICAgIC8vIHNob3VsZCB3ZSBleGl0IGVhcmx5P1xuICAgIGV2ZW50ID0gRXZlbnQoJ2N5Y2xlJyk7XG4gICAgY2xvbmUuZW1pdChldmVudCk7XG4gICAgaWYgKGV2ZW50LmFib3J0ZWQpIHtcbiAgICAgIGNsb25lLmFib3J0KCk7XG4gICAgfVxuICAgIC8vIGZpZ3VyZSBvdXQgd2hhdCB0byBkbyBuZXh0XG4gICAgaWYgKGNsb25lLnJ1bm5pbmcpIHtcbiAgICAgIC8vIHN0YXJ0IGEgbmV3IGN5Y2xlXG4gICAgICBjbG9uZS5jb3VudCA9IGNvdW50O1xuICAgICAgaWYgKGRlZmVycmVkKSB7XG4gICAgICAgIGNsb25lLmNvbXBpbGVkLmNhbGwoZGVmZXJyZWQsIHRpbWVyKTtcbiAgICAgIH0gZWxzZSBpZiAoYXN5bmMpIHtcbiAgICAgICAgZGVsYXkoY2xvbmUsIGZ1bmN0aW9uKCkgeyBjeWNsZShjbG9uZSwgb3B0aW9ucyk7IH0pO1xuICAgICAgfSBlbHNlIHtcbiAgICAgICAgY3ljbGUoY2xvbmUpO1xuICAgICAgfVxuICAgIH1cbiAgICBlbHNlIHtcbiAgICAgIC8vIGZpeCBUcmFjZU1vbmtleSBidWcgYXNzb2NpYXRlZCB3aXRoIGNsb2NrIGZhbGxiYWNrc1xuICAgICAgLy8gaHR0cDovL2J1Z3ppbC5sYS81MDkwNjlcbiAgICAgIGlmIChzdXBwb3J0LmJyb3dzZXIpIHtcbiAgICAgICAgcnVuU2NyaXB0KHVpZCArICc9MTtkZWxldGUgJyArIHVpZCk7XG4gICAgICB9XG4gICAgICAvLyBkb25lXG4gICAgICBjbG9uZS5lbWl0KCdjb21wbGV0ZScpO1xuICAgIH1cbiAgfVxuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8qKlxuICAgKiBSdW5zIHRoZSBiZW5jaG1hcmsuXG4gICAqXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICogQHBhcmFtIHtPYmplY3R9IFtvcHRpb25zPXt9XSBPcHRpb25zIG9iamVjdC5cbiAgICogQHJldHVybnMge09iamVjdH0gVGhlIGJlbmNobWFyayBpbnN0YW5jZS5cbiAgICogQGV4YW1wbGVcbiAgICpcbiAgICogLy8gYmFzaWMgdXNhZ2VcbiAgICogYmVuY2gucnVuKCk7XG4gICAqXG4gICAqIC8vIG9yIHdpdGggb3B0aW9uc1xuICAgKiBiZW5jaC5ydW4oeyAnYXN5bmMnOiB0cnVlIH0pO1xuICAgKi9cbiAgZnVuY3Rpb24gcnVuKG9wdGlvbnMpIHtcbiAgICB2YXIgbWUgPSB0aGlzLFxuICAgICAgICBldmVudCA9IEV2ZW50KCdzdGFydCcpO1xuXG4gICAgLy8gc2V0IGBydW5uaW5nYCB0byBgZmFsc2VgIHNvIGByZXNldCgpYCB3b24ndCBjYWxsIGBhYm9ydCgpYFxuICAgIG1lLnJ1bm5pbmcgPSBmYWxzZTtcbiAgICBtZS5yZXNldCgpO1xuICAgIG1lLnJ1bm5pbmcgPSB0cnVlO1xuXG4gICAgbWUuY291bnQgPSBtZS5pbml0Q291bnQ7XG4gICAgbWUudGltZXMudGltZVN0YW1wID0gK25ldyBEYXRlO1xuICAgIG1lLmVtaXQoZXZlbnQpO1xuXG4gICAgaWYgKCFldmVudC5jYW5jZWxsZWQpIHtcbiAgICAgIG9wdGlvbnMgPSB7ICdhc3luYyc6ICgob3B0aW9ucyA9IG9wdGlvbnMgJiYgb3B0aW9ucy5hc3luYykgPT0gbnVsbCA/IG1lLmFzeW5jIDogb3B0aW9ucykgJiYgc3VwcG9ydC50aW1lb3V0IH07XG5cbiAgICAgIC8vIGZvciBjbG9uZXMgY3JlYXRlZCB3aXRoaW4gYGNvbXB1dGUoKWBcbiAgICAgIGlmIChtZS5fb3JpZ2luYWwpIHtcbiAgICAgICAgaWYgKG1lLmRlZmVyKSB7XG4gICAgICAgICAgRGVmZXJyZWQobWUpO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIGN5Y2xlKG1lLCBvcHRpb25zKTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgICAgLy8gZm9yIG9yaWdpbmFsIGJlbmNobWFya3NcbiAgICAgIGVsc2Uge1xuICAgICAgICBjb21wdXRlKG1lLCBvcHRpb25zKTtcbiAgICAgIH1cbiAgICB9XG4gICAgcmV0dXJuIG1lO1xuICB9XG5cbiAgLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbiAgLy8gRmlyZWZveCAxIGVycm9uZW91c2x5IGRlZmluZXMgdmFyaWFibGUgYW5kIGFyZ3VtZW50IG5hbWVzIG9mIGZ1bmN0aW9ucyBvblxuICAvLyB0aGUgZnVuY3Rpb24gaXRzZWxmIGFzIG5vbi1jb25maWd1cmFibGUgcHJvcGVydGllcyB3aXRoIGB1bmRlZmluZWRgIHZhbHVlcy5cbiAgLy8gVGhlIGJ1Z2dpbmVzcyBjb250aW51ZXMgYXMgdGhlIGBCZW5jaG1hcmtgIGNvbnN0cnVjdG9yIGhhcyBhbiBhcmd1bWVudFxuICAvLyBuYW1lZCBgb3B0aW9uc2AgYW5kIEZpcmVmb3ggMSB3aWxsIG5vdCBhc3NpZ24gYSB2YWx1ZSB0byBgQmVuY2htYXJrLm9wdGlvbnNgLFxuICAvLyBtYWtpbmcgaXQgbm9uLXdyaXRhYmxlIGluIHRoZSBwcm9jZXNzLCB1bmxlc3MgaXQgaXMgdGhlIGZpcnN0IHByb3BlcnR5XG4gIC8vIGFzc2lnbmVkIGJ5IGZvci1pbiBsb29wIG9mIGBleHRlbmQoKWAuXG4gIGV4dGVuZChCZW5jaG1hcmssIHtcblxuICAgIC8qKlxuICAgICAqIFRoZSBkZWZhdWx0IG9wdGlvbnMgY29waWVkIGJ5IGJlbmNobWFyayBpbnN0YW5jZXMuXG4gICAgICpcbiAgICAgKiBAc3RhdGljXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICdvcHRpb25zJzoge1xuXG4gICAgICAvKipcbiAgICAgICAqIEEgZmxhZyB0byBpbmRpY2F0ZSB0aGF0IGJlbmNobWFyayBjeWNsZXMgd2lsbCBleGVjdXRlIGFzeW5jaHJvbm91c2x5XG4gICAgICAgKiBieSBkZWZhdWx0LlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgQm9vbGVhblxuICAgICAgICovXG4gICAgICAnYXN5bmMnOiBmYWxzZSxcblxuICAgICAgLyoqXG4gICAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgdGhhdCB0aGUgYmVuY2htYXJrIGNsb2NrIGlzIGRlZmVycmVkLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgQm9vbGVhblxuICAgICAgICovXG4gICAgICAnZGVmZXInOiBmYWxzZSxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgZGVsYXkgYmV0d2VlbiB0ZXN0IGN5Y2xlcyAoc2VjcykuXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAnZGVsYXknOiAwLjAwNSxcblxuICAgICAgLyoqXG4gICAgICAgKiBEaXNwbGF5ZWQgYnkgQmVuY2htYXJrI3RvU3RyaW5nIHdoZW4gYSBgbmFtZWAgaXMgbm90IGF2YWlsYWJsZVxuICAgICAgICogKGF1dG8tZ2VuZXJhdGVkIGlmIGFic2VudCkuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBTdHJpbmdcbiAgICAgICAqL1xuICAgICAgJ2lkJzogdW5kZWZpbmVkLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBkZWZhdWx0IG51bWJlciBvZiB0aW1lcyB0byBleGVjdXRlIGEgdGVzdCBvbiBhIGJlbmNobWFyaydzIGZpcnN0IGN5Y2xlLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdpbml0Q291bnQnOiAxLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBtYXhpbXVtIHRpbWUgYSBiZW5jaG1hcmsgaXMgYWxsb3dlZCB0byBydW4gYmVmb3JlIGZpbmlzaGluZyAoc2VjcykuXG4gICAgICAgKiBOb3RlOiBDeWNsZSBkZWxheXMgYXJlbid0IGNvdW50ZWQgdG93YXJkIHRoZSBtYXhpbXVtIHRpbWUuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ21heFRpbWUnOiA1LFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBtaW5pbXVtIHNhbXBsZSBzaXplIHJlcXVpcmVkIHRvIHBlcmZvcm0gc3RhdGlzdGljYWwgYW5hbHlzaXMuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ21pblNhbXBsZXMnOiA1LFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSB0aW1lIG5lZWRlZCB0byByZWR1Y2UgdGhlIHBlcmNlbnQgdW5jZXJ0YWludHkgb2YgbWVhc3VyZW1lbnQgdG8gMSUgKHNlY3MpLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdtaW5UaW1lJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgbmFtZSBvZiB0aGUgYmVuY2htYXJrLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgU3RyaW5nXG4gICAgICAgKi9cbiAgICAgICduYW1lJzogdW5kZWZpbmVkLFxuXG4gICAgICAvKipcbiAgICAgICAqIEFuIGV2ZW50IGxpc3RlbmVyIGNhbGxlZCB3aGVuIHRoZSBiZW5jaG1hcmsgaXMgYWJvcnRlZC5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIEZ1bmN0aW9uXG4gICAgICAgKi9cbiAgICAgICdvbkFib3J0JzogdW5kZWZpbmVkLFxuXG4gICAgICAvKipcbiAgICAgICAqIEFuIGV2ZW50IGxpc3RlbmVyIGNhbGxlZCB3aGVuIHRoZSBiZW5jaG1hcmsgY29tcGxldGVzIHJ1bm5pbmcuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBGdW5jdGlvblxuICAgICAgICovXG4gICAgICAnb25Db21wbGV0ZSc6IHVuZGVmaW5lZCxcblxuICAgICAgLyoqXG4gICAgICAgKiBBbiBldmVudCBsaXN0ZW5lciBjYWxsZWQgYWZ0ZXIgZWFjaCBydW4gY3ljbGUuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBGdW5jdGlvblxuICAgICAgICovXG4gICAgICAnb25DeWNsZSc6IHVuZGVmaW5lZCxcblxuICAgICAgLyoqXG4gICAgICAgKiBBbiBldmVudCBsaXN0ZW5lciBjYWxsZWQgd2hlbiBhIHRlc3QgZXJyb3JzLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsub3B0aW9uc1xuICAgICAgICogQHR5cGUgRnVuY3Rpb25cbiAgICAgICAqL1xuICAgICAgJ29uRXJyb3InOiB1bmRlZmluZWQsXG5cbiAgICAgIC8qKlxuICAgICAgICogQW4gZXZlbnQgbGlzdGVuZXIgY2FsbGVkIHdoZW4gdGhlIGJlbmNobWFyayBpcyByZXNldC5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLm9wdGlvbnNcbiAgICAgICAqIEB0eXBlIEZ1bmN0aW9uXG4gICAgICAgKi9cbiAgICAgICdvblJlc2V0JzogdW5kZWZpbmVkLFxuXG4gICAgICAvKipcbiAgICAgICAqIEFuIGV2ZW50IGxpc3RlbmVyIGNhbGxlZCB3aGVuIHRoZSBiZW5jaG1hcmsgc3RhcnRzIHJ1bm5pbmcuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5vcHRpb25zXG4gICAgICAgKiBAdHlwZSBGdW5jdGlvblxuICAgICAgICovXG4gICAgICAnb25TdGFydCc6IHVuZGVmaW5lZFxuICAgIH0sXG5cbiAgICAvKipcbiAgICAgKiBQbGF0Zm9ybSBvYmplY3Qgd2l0aCBwcm9wZXJ0aWVzIGRlc2NyaWJpbmcgdGhpbmdzIGxpa2UgYnJvd3NlciBuYW1lLFxuICAgICAqIHZlcnNpb24sIGFuZCBvcGVyYXRpbmcgc3lzdGVtLlxuICAgICAqXG4gICAgICogQHN0YXRpY1xuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBPYmplY3RcbiAgICAgKi9cbiAgICAncGxhdGZvcm0nOiByZXEoJ3BsYXRmb3JtJykgfHwgd2luZG93LnBsYXRmb3JtIHx8IHtcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgcGxhdGZvcm0gZGVzY3JpcHRpb24uXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nXG4gICAgICAgKi9cbiAgICAgICdkZXNjcmlwdGlvbic6IHdpbmRvdy5uYXZpZ2F0b3IgJiYgbmF2aWdhdG9yLnVzZXJBZ2VudCB8fCBudWxsLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBuYW1lIG9mIHRoZSBicm93c2VyIGxheW91dCBlbmdpbmUuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nfE51bGxcbiAgICAgICAqL1xuICAgICAgJ2xheW91dCc6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG5hbWUgb2YgdGhlIHByb2R1Y3QgaG9zdGluZyB0aGUgYnJvd3Nlci5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBTdHJpbmd8TnVsbFxuICAgICAgICovXG4gICAgICAncHJvZHVjdCc6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIG5hbWUgb2YgdGhlIGJyb3dzZXIvZW52aXJvbm1lbnQuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nfE51bGxcbiAgICAgICAqL1xuICAgICAgJ25hbWUnOiBudWxsLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBuYW1lIG9mIHRoZSBwcm9kdWN0J3MgbWFudWZhY3R1cmVyLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsucGxhdGZvcm1cbiAgICAgICAqIEB0eXBlIFN0cmluZ3xOdWxsXG4gICAgICAgKi9cbiAgICAgICdtYW51ZmFjdHVyZXInOiBudWxsLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBuYW1lIG9mIHRoZSBvcGVyYXRpbmcgc3lzdGVtLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsucGxhdGZvcm1cbiAgICAgICAqIEB0eXBlIFN0cmluZ3xOdWxsXG4gICAgICAgKi9cbiAgICAgICdvcyc6IG51bGwsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIGFscGhhL2JldGEgcmVsZWFzZSBpbmRpY2F0b3IuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nfE51bGxcbiAgICAgICAqL1xuICAgICAgJ3ByZXJlbGVhc2UnOiBudWxsLFxuXG4gICAgICAvKipcbiAgICAgICAqIFRoZSBicm93c2VyL2Vudmlyb25tZW50IHZlcnNpb24uXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyay5wbGF0Zm9ybVxuICAgICAgICogQHR5cGUgU3RyaW5nfE51bGxcbiAgICAgICAqL1xuICAgICAgJ3ZlcnNpb24nOiBudWxsLFxuXG4gICAgICAvKipcbiAgICAgICAqIFJldHVybiBwbGF0Zm9ybSBkZXNjcmlwdGlvbiB3aGVuIHRoZSBwbGF0Zm9ybSBvYmplY3QgaXMgY29lcmNlZCB0byBhIHN0cmluZy5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLnBsYXRmb3JtXG4gICAgICAgKiBAdHlwZSBGdW5jdGlvblxuICAgICAgICogQHJldHVybnMge1N0cmluZ30gVGhlIHBsYXRmb3JtIGRlc2NyaXB0aW9uLlxuICAgICAgICovXG4gICAgICAndG9TdHJpbmcnOiBmdW5jdGlvbigpIHtcbiAgICAgICAgcmV0dXJuIHRoaXMuZGVzY3JpcHRpb24gfHwgJyc7XG4gICAgICB9XG4gICAgfSxcblxuICAgIC8qKlxuICAgICAqIFRoZSBzZW1hbnRpYyB2ZXJzaW9uIG51bWJlci5cbiAgICAgKlxuICAgICAqIEBzdGF0aWNcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgU3RyaW5nXG4gICAgICovXG4gICAgJ3ZlcnNpb24nOiAnMS4wLjAnLFxuXG4gICAgLy8gYW4gb2JqZWN0IG9mIGVudmlyb25tZW50L2ZlYXR1cmUgZGV0ZWN0aW9uIGZsYWdzXG4gICAgJ3N1cHBvcnQnOiBzdXBwb3J0LFxuXG4gICAgLy8gY2xvbmUgb2JqZWN0c1xuICAgICdkZWVwQ2xvbmUnOiBkZWVwQ2xvbmUsXG5cbiAgICAvLyBpdGVyYXRpb24gdXRpbGl0eVxuICAgICdlYWNoJzogZWFjaCxcblxuICAgIC8vIGF1Z21lbnQgb2JqZWN0c1xuICAgICdleHRlbmQnOiBleHRlbmQsXG5cbiAgICAvLyBnZW5lcmljIEFycmF5I2ZpbHRlclxuICAgICdmaWx0ZXInOiBmaWx0ZXIsXG5cbiAgICAvLyBnZW5lcmljIEFycmF5I2ZvckVhY2hcbiAgICAnZm9yRWFjaCc6IGZvckVhY2gsXG5cbiAgICAvLyBnZW5lcmljIG93biBwcm9wZXJ0eSBpdGVyYXRpb24gdXRpbGl0eVxuICAgICdmb3JPd24nOiBmb3JPd24sXG5cbiAgICAvLyBjb252ZXJ0cyBhIG51bWJlciB0byBhIGNvbW1hLXNlcGFyYXRlZCBzdHJpbmdcbiAgICAnZm9ybWF0TnVtYmVyJzogZm9ybWF0TnVtYmVyLFxuXG4gICAgLy8gZ2VuZXJpYyBPYmplY3QjaGFzT3duUHJvcGVydHlcbiAgICAvLyAodHJpZ2dlciBoYXNLZXkncyBsYXp5IGRlZmluZSBiZWZvcmUgYXNzaWduaW5nIGl0IHRvIEJlbmNobWFyaylcbiAgICAnaGFzS2V5JzogKGhhc0tleShCZW5jaG1hcmssICcnKSwgaGFzS2V5KSxcblxuICAgIC8vIGdlbmVyaWMgQXJyYXkjaW5kZXhPZlxuICAgICdpbmRleE9mJzogaW5kZXhPZixcblxuICAgIC8vIHRlbXBsYXRlIHV0aWxpdHlcbiAgICAnaW50ZXJwb2xhdGUnOiBpbnRlcnBvbGF0ZSxcblxuICAgIC8vIGludm9rZXMgYSBtZXRob2Qgb24gZWFjaCBpdGVtIGluIGFuIGFycmF5XG4gICAgJ2ludm9rZSc6IGludm9rZSxcblxuICAgIC8vIGdlbmVyaWMgQXJyYXkjam9pbiBmb3IgYXJyYXlzIGFuZCBvYmplY3RzXG4gICAgJ2pvaW4nOiBqb2luLFxuXG4gICAgLy8gZ2VuZXJpYyBBcnJheSNtYXBcbiAgICAnbWFwJzogbWFwLFxuXG4gICAgLy8gcmV0cmlldmVzIGEgcHJvcGVydHkgdmFsdWUgZnJvbSBlYWNoIGl0ZW0gaW4gYW4gYXJyYXlcbiAgICAncGx1Y2snOiBwbHVjayxcblxuICAgIC8vIGdlbmVyaWMgQXJyYXkjcmVkdWNlXG4gICAgJ3JlZHVjZSc6IHJlZHVjZVxuICB9KTtcblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICBleHRlbmQoQmVuY2htYXJrLnByb3RvdHlwZSwge1xuXG4gICAgLyoqXG4gICAgICogVGhlIG51bWJlciBvZiB0aW1lcyBhIHRlc3Qgd2FzIGV4ZWN1dGVkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE51bWJlclxuICAgICAqL1xuICAgICdjb3VudCc6IDAsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgbnVtYmVyIG9mIGN5Y2xlcyBwZXJmb3JtZWQgd2hpbGUgYmVuY2htYXJraW5nLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE51bWJlclxuICAgICAqL1xuICAgICdjeWNsZXMnOiAwLFxuXG4gICAgLyoqXG4gICAgICogVGhlIG51bWJlciBvZiBleGVjdXRpb25zIHBlciBzZWNvbmQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ2h6JzogMCxcblxuICAgIC8qKlxuICAgICAqIFRoZSBjb21waWxlZCB0ZXN0IGZ1bmN0aW9uLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIEZ1bmN0aW9ufFN0cmluZ1xuICAgICAqL1xuICAgICdjb21waWxlZCc6IHVuZGVmaW5lZCxcblxuICAgIC8qKlxuICAgICAqIFRoZSBlcnJvciBvYmplY3QgaWYgdGhlIHRlc3QgZmFpbGVkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICdlcnJvcic6IHVuZGVmaW5lZCxcblxuICAgIC8qKlxuICAgICAqIFRoZSB0ZXN0IHRvIGJlbmNobWFyay5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBGdW5jdGlvbnxTdHJpbmdcbiAgICAgKi9cbiAgICAnZm4nOiB1bmRlZmluZWQsXG5cbiAgICAvKipcbiAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgaWYgdGhlIGJlbmNobWFyayBpcyBhYm9ydGVkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICAnYWJvcnRlZCc6IGZhbHNlLFxuXG4gICAgLyoqXG4gICAgICogQSBmbGFnIHRvIGluZGljYXRlIGlmIHRoZSBiZW5jaG1hcmsgaXMgcnVubmluZy5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBCb29sZWFuXG4gICAgICovXG4gICAgJ3J1bm5pbmcnOiBmYWxzZSxcblxuICAgIC8qKlxuICAgICAqIENvbXBpbGVkIGludG8gdGhlIHRlc3QgYW5kIGV4ZWN1dGVkIGltbWVkaWF0ZWx5ICoqYmVmb3JlKiogdGhlIHRlc3QgbG9vcC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmtcbiAgICAgKiBAdHlwZSBGdW5jdGlvbnxTdHJpbmdcbiAgICAgKiBAZXhhbXBsZVxuICAgICAqXG4gICAgICogLy8gYmFzaWMgdXNhZ2VcbiAgICAgKiB2YXIgYmVuY2ggPSBCZW5jaG1hcmsoe1xuICAgICAqICAgJ3NldHVwJzogZnVuY3Rpb24oKSB7XG4gICAgICogICAgIHZhciBjID0gdGhpcy5jb3VudCxcbiAgICAgKiAgICAgICAgIGVsZW1lbnQgPSBkb2N1bWVudC5nZXRFbGVtZW50QnlJZCgnY29udGFpbmVyJyk7XG4gICAgICogICAgIHdoaWxlIChjLS0pIHtcbiAgICAgKiAgICAgICBlbGVtZW50LmFwcGVuZENoaWxkKGRvY3VtZW50LmNyZWF0ZUVsZW1lbnQoJ2RpdicpKTtcbiAgICAgKiAgICAgfVxuICAgICAqICAgfSxcbiAgICAgKiAgICdmbic6IGZ1bmN0aW9uKCkge1xuICAgICAqICAgICBlbGVtZW50LnJlbW92ZUNoaWxkKGVsZW1lbnQubGFzdENoaWxkKTtcbiAgICAgKiAgIH1cbiAgICAgKiB9KTtcbiAgICAgKlxuICAgICAqIC8vIGNvbXBpbGVzIHRvIHNvbWV0aGluZyBsaWtlOlxuICAgICAqIHZhciBjID0gdGhpcy5jb3VudCxcbiAgICAgKiAgICAgZWxlbWVudCA9IGRvY3VtZW50LmdldEVsZW1lbnRCeUlkKCdjb250YWluZXInKTtcbiAgICAgKiB3aGlsZSAoYy0tKSB7XG4gICAgICogICBlbGVtZW50LmFwcGVuZENoaWxkKGRvY3VtZW50LmNyZWF0ZUVsZW1lbnQoJ2RpdicpKTtcbiAgICAgKiB9XG4gICAgICogdmFyIHN0YXJ0ID0gbmV3IERhdGU7XG4gICAgICogd2hpbGUgKGNvdW50LS0pIHtcbiAgICAgKiAgIGVsZW1lbnQucmVtb3ZlQ2hpbGQoZWxlbWVudC5sYXN0Q2hpbGQpO1xuICAgICAqIH1cbiAgICAgKiB2YXIgZW5kID0gbmV3IERhdGUgLSBzdGFydDtcbiAgICAgKlxuICAgICAqIC8vIG9yIHVzaW5nIHN0cmluZ3NcbiAgICAgKiB2YXIgYmVuY2ggPSBCZW5jaG1hcmsoe1xuICAgICAqICAgJ3NldHVwJzogJ1xcXG4gICAgICogICAgIHZhciBhID0gMDtcXG5cXFxuICAgICAqICAgICAoZnVuY3Rpb24oKSB7XFxuXFxcbiAgICAgKiAgICAgICAoZnVuY3Rpb24oKSB7XFxuXFxcbiAgICAgKiAgICAgICAgIChmdW5jdGlvbigpIHsnLFxuICAgICAqICAgJ2ZuJzogJ2EgKz0gMTsnLFxuICAgICAqICAgJ3RlYXJkb3duJzogJ1xcXG4gICAgICogICAgICAgICAgfSgpKVxcblxcXG4gICAgICogICAgICAgIH0oKSlcXG5cXFxuICAgICAqICAgICAgfSgpKSdcbiAgICAgKiB9KTtcbiAgICAgKlxuICAgICAqIC8vIGNvbXBpbGVzIHRvIHNvbWV0aGluZyBsaWtlOlxuICAgICAqIHZhciBhID0gMDtcbiAgICAgKiAoZnVuY3Rpb24oKSB7XG4gICAgICogICAoZnVuY3Rpb24oKSB7XG4gICAgICogICAgIChmdW5jdGlvbigpIHtcbiAgICAgKiAgICAgICB2YXIgc3RhcnQgPSBuZXcgRGF0ZTtcbiAgICAgKiAgICAgICB3aGlsZSAoY291bnQtLSkge1xuICAgICAqICAgICAgICAgYSArPSAxO1xuICAgICAqICAgICAgIH1cbiAgICAgKiAgICAgICB2YXIgZW5kID0gbmV3IERhdGUgLSBzdGFydDtcbiAgICAgKiAgICAgfSgpKVxuICAgICAqICAgfSgpKVxuICAgICAqIH0oKSlcbiAgICAgKi9cbiAgICAnc2V0dXAnOiBub29wLFxuXG4gICAgLyoqXG4gICAgICogQ29tcGlsZWQgaW50byB0aGUgdGVzdCBhbmQgZXhlY3V0ZWQgaW1tZWRpYXRlbHkgKiphZnRlcioqIHRoZSB0ZXN0IGxvb3AuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrXG4gICAgICogQHR5cGUgRnVuY3Rpb258U3RyaW5nXG4gICAgICovXG4gICAgJ3RlYXJkb3duJzogbm9vcCxcblxuICAgIC8qKlxuICAgICAqIEFuIG9iamVjdCBvZiBzdGF0cyBpbmNsdWRpbmcgbWVhbiwgbWFyZ2luIG9yIGVycm9yLCBhbmQgc3RhbmRhcmQgZGV2aWF0aW9uLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICdzdGF0cyc6IHtcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgbWFyZ2luIG9mIGVycm9yLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjc3RhdHNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAnbW9lJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgcmVsYXRpdmUgbWFyZ2luIG9mIGVycm9yIChleHByZXNzZWQgYXMgYSBwZXJjZW50YWdlIG9mIHRoZSBtZWFuKS5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrI3N0YXRzXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ3JtZSc6IDAsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIHN0YW5kYXJkIGVycm9yIG9mIHRoZSBtZWFuLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjc3RhdHNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAnc2VtJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgc2FtcGxlIHN0YW5kYXJkIGRldmlhdGlvbi5cbiAgICAgICAqXG4gICAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrI3N0YXRzXG4gICAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgICAqL1xuICAgICAgJ2RldmlhdGlvbic6IDAsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIHNhbXBsZSBhcml0aG1ldGljIG1lYW4uXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayNzdGF0c1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdtZWFuJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgYXJyYXkgb2Ygc2FtcGxlZCBwZXJpb2RzLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjc3RhdHNcbiAgICAgICAqIEB0eXBlIEFycmF5XG4gICAgICAgKi9cbiAgICAgICdzYW1wbGUnOiBbXSxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgc2FtcGxlIHZhcmlhbmNlLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjc3RhdHNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAndmFyaWFuY2UnOiAwXG4gICAgfSxcblxuICAgIC8qKlxuICAgICAqIEFuIG9iamVjdCBvZiB0aW1pbmcgZGF0YSBpbmNsdWRpbmcgY3ljbGUsIGVsYXBzZWQsIHBlcmlvZCwgc3RhcnQsIGFuZCBzdG9wLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFya1xuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICd0aW1lcyc6IHtcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgdGltZSB0YWtlbiB0byBjb21wbGV0ZSB0aGUgbGFzdCBjeWNsZSAoc2VjcykuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayN0aW1lc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdjeWNsZSc6IDAsXG5cbiAgICAgIC8qKlxuICAgICAgICogVGhlIHRpbWUgdGFrZW4gdG8gY29tcGxldGUgdGhlIGJlbmNobWFyayAoc2VjcykuXG4gICAgICAgKlxuICAgICAgICogQG1lbWJlck9mIEJlbmNobWFyayN0aW1lc1xuICAgICAgICogQHR5cGUgTnVtYmVyXG4gICAgICAgKi9cbiAgICAgICdlbGFwc2VkJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBUaGUgdGltZSB0YWtlbiB0byBleGVjdXRlIHRoZSB0ZXN0IG9uY2UgKHNlY3MpLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjdGltZXNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAncGVyaW9kJzogMCxcblxuICAgICAgLyoqXG4gICAgICAgKiBBIHRpbWVzdGFtcCBvZiB3aGVuIHRoZSBiZW5jaG1hcmsgc3RhcnRlZCAobXMpLlxuICAgICAgICpcbiAgICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsjdGltZXNcbiAgICAgICAqIEB0eXBlIE51bWJlclxuICAgICAgICovXG4gICAgICAndGltZVN0YW1wJzogMFxuICAgIH0sXG5cbiAgICAvLyBhYm9ydHMgYmVuY2htYXJrIChkb2VzIG5vdCByZWNvcmQgdGltZXMpXG4gICAgJ2Fib3J0JzogYWJvcnQsXG5cbiAgICAvLyBjcmVhdGVzIGEgbmV3IGJlbmNobWFyayB1c2luZyB0aGUgc2FtZSB0ZXN0IGFuZCBvcHRpb25zXG4gICAgJ2Nsb25lJzogY2xvbmUsXG5cbiAgICAvLyBjb21wYXJlcyBiZW5jaG1hcmsncyBoZXJ0eiB3aXRoIGFub3RoZXJcbiAgICAnY29tcGFyZSc6IGNvbXBhcmUsXG5cbiAgICAvLyBleGVjdXRlcyBsaXN0ZW5lcnNcbiAgICAnZW1pdCc6IGVtaXQsXG5cbiAgICAvLyBnZXQgbGlzdGVuZXJzXG4gICAgJ2xpc3RlbmVycyc6IGxpc3RlbmVycyxcblxuICAgIC8vIHVucmVnaXN0ZXIgbGlzdGVuZXJzXG4gICAgJ29mZic6IG9mZixcblxuICAgIC8vIHJlZ2lzdGVyIGxpc3RlbmVyc1xuICAgICdvbic6IG9uLFxuXG4gICAgLy8gcmVzZXQgYmVuY2htYXJrIHByb3BlcnRpZXNcbiAgICAncmVzZXQnOiByZXNldCxcblxuICAgIC8vIHJ1bnMgdGhlIGJlbmNobWFya1xuICAgICdydW4nOiBydW4sXG5cbiAgICAvLyBwcmV0dHkgcHJpbnQgYmVuY2htYXJrIGluZm9cbiAgICAndG9TdHJpbmcnOiB0b1N0cmluZ0JlbmNoXG4gIH0pO1xuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIGV4dGVuZChEZWZlcnJlZC5wcm90b3R5cGUsIHtcblxuICAgIC8qKlxuICAgICAqIFRoZSBkZWZlcnJlZCBiZW5jaG1hcmsgaW5zdGFuY2UuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkRlZmVycmVkXG4gICAgICogQHR5cGUgT2JqZWN0XG4gICAgICovXG4gICAgJ2JlbmNobWFyayc6IG51bGwsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgbnVtYmVyIG9mIGRlZmVycmVkIGN5Y2xlcyBwZXJmb3JtZWQgd2hpbGUgYmVuY2htYXJraW5nLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5EZWZlcnJlZFxuICAgICAqIEB0eXBlIE51bWJlclxuICAgICAqL1xuICAgICdjeWNsZXMnOiAwLFxuXG4gICAgLyoqXG4gICAgICogVGhlIHRpbWUgdGFrZW4gdG8gY29tcGxldGUgdGhlIGRlZmVycmVkIGJlbmNobWFyayAoc2VjcykuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkRlZmVycmVkXG4gICAgICogQHR5cGUgTnVtYmVyXG4gICAgICovXG4gICAgJ2VsYXBzZWQnOiAwLFxuXG4gICAgLyoqXG4gICAgICogQSB0aW1lc3RhbXAgb2Ygd2hlbiB0aGUgZGVmZXJyZWQgYmVuY2htYXJrIHN0YXJ0ZWQgKG1zKS5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuRGVmZXJyZWRcbiAgICAgKiBAdHlwZSBOdW1iZXJcbiAgICAgKi9cbiAgICAndGltZVN0YW1wJzogMCxcblxuICAgIC8vIGN5Y2xlcy9jb21wbGV0ZXMgdGhlIGRlZmVycmVkIGJlbmNobWFya1xuICAgICdyZXNvbHZlJzogcmVzb2x2ZVxuICB9KTtcblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICBleHRlbmQoRXZlbnQucHJvdG90eXBlLCB7XG5cbiAgICAvKipcbiAgICAgKiBBIGZsYWcgdG8gaW5kaWNhdGUgaWYgdGhlIGVtaXR0ZXJzIGxpc3RlbmVyIGl0ZXJhdGlvbiBpcyBhYm9ydGVkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5FdmVudFxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICAnYWJvcnRlZCc6IGZhbHNlLFxuXG4gICAgLyoqXG4gICAgICogQSBmbGFnIHRvIGluZGljYXRlIGlmIHRoZSBkZWZhdWx0IGFjdGlvbiBpcyBjYW5jZWxsZWQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkV2ZW50XG4gICAgICogQHR5cGUgQm9vbGVhblxuICAgICAqL1xuICAgICdjYW5jZWxsZWQnOiBmYWxzZSxcblxuICAgIC8qKlxuICAgICAqIFRoZSBvYmplY3Qgd2hvc2UgbGlzdGVuZXJzIGFyZSBjdXJyZW50bHkgYmVpbmcgcHJvY2Vzc2VkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5FdmVudFxuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICdjdXJyZW50VGFyZ2V0JzogdW5kZWZpbmVkLFxuXG4gICAgLyoqXG4gICAgICogVGhlIHJldHVybiB2YWx1ZSBvZiB0aGUgbGFzdCBleGVjdXRlZCBsaXN0ZW5lci5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuRXZlbnRcbiAgICAgKiBAdHlwZSBNaXhlZFxuICAgICAqL1xuICAgICdyZXN1bHQnOiB1bmRlZmluZWQsXG5cbiAgICAvKipcbiAgICAgKiBUaGUgb2JqZWN0IHRvIHdoaWNoIHRoZSBldmVudCB3YXMgb3JpZ2luYWxseSBlbWl0dGVkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5FdmVudFxuICAgICAqIEB0eXBlIE9iamVjdFxuICAgICAqL1xuICAgICd0YXJnZXQnOiB1bmRlZmluZWQsXG5cbiAgICAvKipcbiAgICAgKiBBIHRpbWVzdGFtcCBvZiB3aGVuIHRoZSBldmVudCB3YXMgY3JlYXRlZCAobXMpLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5FdmVudFxuICAgICAqIEB0eXBlIE51bWJlclxuICAgICAqL1xuICAgICd0aW1lU3RhbXAnOiAwLFxuXG4gICAgLyoqXG4gICAgICogVGhlIGV2ZW50IHR5cGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLkV2ZW50XG4gICAgICogQHR5cGUgU3RyaW5nXG4gICAgICovXG4gICAgJ3R5cGUnOiAnJ1xuICB9KTtcblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICAvKipcbiAgICogVGhlIGRlZmF1bHQgb3B0aW9ucyBjb3BpZWQgYnkgc3VpdGUgaW5zdGFuY2VzLlxuICAgKlxuICAgKiBAc3RhdGljXG4gICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICogQHR5cGUgT2JqZWN0XG4gICAqL1xuICBTdWl0ZS5vcHRpb25zID0ge1xuXG4gICAgLyoqXG4gICAgICogVGhlIG5hbWUgb2YgdGhlIHN1aXRlLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZS5vcHRpb25zXG4gICAgICogQHR5cGUgU3RyaW5nXG4gICAgICovXG4gICAgJ25hbWUnOiB1bmRlZmluZWRcbiAgfTtcblxuICAvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuICBleHRlbmQoU3VpdGUucHJvdG90eXBlLCB7XG5cbiAgICAvKipcbiAgICAgKiBUaGUgbnVtYmVyIG9mIGJlbmNobWFya3MgaW4gdGhlIHN1aXRlLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEB0eXBlIE51bWJlclxuICAgICAqL1xuICAgICdsZW5ndGgnOiAwLFxuXG4gICAgLyoqXG4gICAgICogQSBmbGFnIHRvIGluZGljYXRlIGlmIHRoZSBzdWl0ZSBpcyBhYm9ydGVkLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICAnYWJvcnRlZCc6IGZhbHNlLFxuXG4gICAgLyoqXG4gICAgICogQSBmbGFnIHRvIGluZGljYXRlIGlmIHRoZSBzdWl0ZSBpcyBydW5uaW5nLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEB0eXBlIEJvb2xlYW5cbiAgICAgKi9cbiAgICAncnVubmluZyc6IGZhbHNlLFxuXG4gICAgLyoqXG4gICAgICogQW4gYEFycmF5I2ZvckVhY2hgIGxpa2UgbWV0aG9kLlxuICAgICAqIENhbGxiYWNrcyBtYXkgdGVybWluYXRlIHRoZSBsb29wIGJ5IGV4cGxpY2l0bHkgcmV0dXJuaW5nIGBmYWxzZWAuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHBhcmFtIHtGdW5jdGlvbn0gY2FsbGJhY2sgVGhlIGZ1bmN0aW9uIGNhbGxlZCBwZXIgaXRlcmF0aW9uLlxuICAgICAqIEByZXR1cm5zIHtPYmplY3R9IFRoZSBzdWl0ZSBpdGVyYXRlZCBvdmVyLlxuICAgICAqL1xuICAgICdmb3JFYWNoJzogbWV0aG9kaXplKGZvckVhY2gpLFxuXG4gICAgLyoqXG4gICAgICogQW4gYEFycmF5I2luZGV4T2ZgIGxpa2UgbWV0aG9kLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEBwYXJhbSB7TWl4ZWR9IHZhbHVlIFRoZSB2YWx1ZSB0byBzZWFyY2ggZm9yLlxuICAgICAqIEByZXR1cm5zIHtOdW1iZXJ9IFRoZSBpbmRleCBvZiB0aGUgbWF0Y2hlZCB2YWx1ZSBvciBgLTFgLlxuICAgICAqL1xuICAgICdpbmRleE9mJzogbWV0aG9kaXplKGluZGV4T2YpLFxuXG4gICAgLyoqXG4gICAgICogSW52b2tlcyBhIG1ldGhvZCBvbiBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHBhcmFtIHtTdHJpbmd8T2JqZWN0fSBuYW1lIFRoZSBuYW1lIG9mIHRoZSBtZXRob2QgdG8gaW52b2tlIE9SIG9wdGlvbnMgb2JqZWN0LlxuICAgICAqIEBwYXJhbSB7TWl4ZWR9IFthcmcxLCBhcmcyLCAuLi5dIEFyZ3VtZW50cyB0byBpbnZva2UgdGhlIG1ldGhvZCB3aXRoLlxuICAgICAqIEByZXR1cm5zIHtBcnJheX0gQSBuZXcgYXJyYXkgb2YgdmFsdWVzIHJldHVybmVkIGZyb20gZWFjaCBtZXRob2QgaW52b2tlZC5cbiAgICAgKi9cbiAgICAnaW52b2tlJzogbWV0aG9kaXplKGludm9rZSksXG5cbiAgICAvKipcbiAgICAgKiBDb252ZXJ0cyB0aGUgc3VpdGUgb2YgYmVuY2htYXJrcyB0byBhIHN0cmluZy5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcGFyYW0ge1N0cmluZ30gW3NlcGFyYXRvcj0nLCddIEEgc3RyaW5nIHRvIHNlcGFyYXRlIGVhY2ggZWxlbWVudCBvZiB0aGUgYXJyYXkuXG4gICAgICogQHJldHVybnMge1N0cmluZ30gVGhlIHN0cmluZy5cbiAgICAgKi9cbiAgICAnam9pbic6IFtdLmpvaW4sXG5cbiAgICAvKipcbiAgICAgKiBBbiBgQXJyYXkjbWFwYCBsaWtlIG1ldGhvZC5cbiAgICAgKlxuICAgICAqIEBtZW1iZXJPZiBCZW5jaG1hcmsuU3VpdGVcbiAgICAgKiBAcGFyYW0ge0Z1bmN0aW9ufSBjYWxsYmFjayBUaGUgZnVuY3Rpb24gY2FsbGVkIHBlciBpdGVyYXRpb24uXG4gICAgICogQHJldHVybnMge0FycmF5fSBBIG5ldyBhcnJheSBvZiB2YWx1ZXMgcmV0dXJuZWQgYnkgdGhlIGNhbGxiYWNrLlxuICAgICAqL1xuICAgICdtYXAnOiBtZXRob2RpemUobWFwKSxcblxuICAgIC8qKlxuICAgICAqIFJldHJpZXZlcyB0aGUgdmFsdWUgb2YgYSBzcGVjaWZpZWQgcHJvcGVydHkgZnJvbSBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHBhcmFtIHtTdHJpbmd9IHByb3BlcnR5IFRoZSBwcm9wZXJ0eSB0byBwbHVjay5cbiAgICAgKiBAcmV0dXJucyB7QXJyYXl9IEEgbmV3IGFycmF5IG9mIHByb3BlcnR5IHZhbHVlcy5cbiAgICAgKi9cbiAgICAncGx1Y2snOiBtZXRob2RpemUocGx1Y2spLFxuXG4gICAgLyoqXG4gICAgICogUmVtb3ZlcyB0aGUgbGFzdCBiZW5jaG1hcmsgZnJvbSB0aGUgc3VpdGUgYW5kIHJldHVybnMgaXQuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHJldHVybnMge01peGVkfSBUaGUgcmVtb3ZlZCBiZW5jaG1hcmsuXG4gICAgICovXG4gICAgJ3BvcCc6IFtdLnBvcCxcblxuICAgIC8qKlxuICAgICAqIEFwcGVuZHMgYmVuY2htYXJrcyB0byB0aGUgc3VpdGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHJldHVybnMge051bWJlcn0gVGhlIHN1aXRlJ3MgbmV3IGxlbmd0aC5cbiAgICAgKi9cbiAgICAncHVzaCc6IFtdLnB1c2gsXG5cbiAgICAvKipcbiAgICAgKiBTb3J0cyB0aGUgYmVuY2htYXJrcyBvZiB0aGUgc3VpdGUuXG4gICAgICpcbiAgICAgKiBAbWVtYmVyT2YgQmVuY2htYXJrLlN1aXRlXG4gICAgICogQHBhcmFtIHtGdW5jdGlvbn0gW2NvbXBhcmVGbj1udWxsXSBBIGZ1bmN0aW9uIHRoYXQgZGVmaW5lcyB0aGUgc29ydCBvcmRlci5cbiAgICAgKiBAcmV0dXJucyB7T2JqZWN0fSBUaGUgc29ydGVkIHN1aXRlLlxuICAgICAqL1xuICAgICdzb3J0JzogW10uc29ydCxcblxuICAgIC8qKlxuICAgICAqIEFuIGBBcnJheSNyZWR1Y2VgIGxpa2UgbWV0aG9kLlxuICAgICAqXG4gICAgICogQG1lbWJlck9mIEJlbmNobWFyay5TdWl0ZVxuICAgICAqIEBwYXJhbSB7RnVuY3Rpb259IGNhbGxiYWNrIFRoZSBmdW5jdGlvbiBjYWxsZWQgcGVyIGl0ZXJhdGlvbi5cbiAgICAgKiBAcGFyYW0ge01peGVkfSBhY2N1bXVsYXRvciBJbml0aWFsIHZhbHVlIG9mIHRoZSBhY2N1bXVsYXRvci5cbiAgICAgKiBAcmV0dXJucyB7TWl4ZWR9IFRoZSBhY2N1bXVsYXRvci5cbiAgICAgKi9cbiAgICAncmVkdWNlJzogbWV0aG9kaXplKHJlZHVjZSksXG5cbiAgICAvLyBhYm9ydHMgYWxsIGJlbmNobWFya3MgaW4gdGhlIHN1aXRlXG4gICAgJ2Fib3J0JzogYWJvcnRTdWl0ZSxcblxuICAgIC8vIGFkZHMgYSBiZW5jaG1hcmsgdG8gdGhlIHN1aXRlXG4gICAgJ2FkZCc6IGFkZCxcblxuICAgIC8vIGNyZWF0ZXMgYSBuZXcgc3VpdGUgd2l0aCBjbG9uZWQgYmVuY2htYXJrc1xuICAgICdjbG9uZSc6IGNsb25lU3VpdGUsXG5cbiAgICAvLyBleGVjdXRlcyBsaXN0ZW5lcnMgb2YgYSBzcGVjaWZpZWQgdHlwZVxuICAgICdlbWl0JzogZW1pdCxcblxuICAgIC8vIGNyZWF0ZXMgYSBuZXcgc3VpdGUgb2YgZmlsdGVyZWQgYmVuY2htYXJrc1xuICAgICdmaWx0ZXInOiBmaWx0ZXJTdWl0ZSxcblxuICAgIC8vIGdldCBsaXN0ZW5lcnNcbiAgICAnbGlzdGVuZXJzJzogbGlzdGVuZXJzLFxuXG4gICAgLy8gdW5yZWdpc3RlciBsaXN0ZW5lcnNcbiAgICAnb2ZmJzogb2ZmLFxuXG4gICAvLyByZWdpc3RlciBsaXN0ZW5lcnNcbiAgICAnb24nOiBvbixcblxuICAgIC8vIHJlc2V0cyBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGVcbiAgICAncmVzZXQnOiByZXNldFN1aXRlLFxuXG4gICAgLy8gcnVucyBhbGwgYmVuY2htYXJrcyBpbiB0aGUgc3VpdGVcbiAgICAncnVuJzogcnVuU3VpdGUsXG5cbiAgICAvLyBhcnJheSBtZXRob2RzXG4gICAgJ2NvbmNhdCc6IGNvbmNhdCxcblxuICAgICdyZXZlcnNlJzogcmV2ZXJzZSxcblxuICAgICdzaGlmdCc6IHNoaWZ0LFxuXG4gICAgJ3NsaWNlJzogc2xpY2UsXG5cbiAgICAnc3BsaWNlJzogc3BsaWNlLFxuXG4gICAgJ3Vuc2hpZnQnOiB1bnNoaWZ0XG4gIH0pO1xuXG4gIC8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG4gIC8vIGV4cG9zZSBEZWZlcnJlZCwgRXZlbnQgYW5kIFN1aXRlXG4gIGV4dGVuZChCZW5jaG1hcmssIHtcbiAgICAnRGVmZXJyZWQnOiBEZWZlcnJlZCxcbiAgICAnRXZlbnQnOiBFdmVudCxcbiAgICAnU3VpdGUnOiBTdWl0ZVxuICB9KTtcblxuICAvLyBleHBvc2UgQmVuY2htYXJrXG4gIC8vIHNvbWUgQU1EIGJ1aWxkIG9wdGltaXplcnMsIGxpa2Ugci5qcywgY2hlY2sgZm9yIHNwZWNpZmljIGNvbmRpdGlvbiBwYXR0ZXJucyBsaWtlIHRoZSBmb2xsb3dpbmc6XG4gIGlmICh0eXBlb2YgZGVmaW5lID09ICdmdW5jdGlvbicgJiYgdHlwZW9mIGRlZmluZS5hbWQgPT0gJ29iamVjdCcgJiYgZGVmaW5lLmFtZCkge1xuICAgIC8vIGRlZmluZSBhcyBhbiBhbm9ueW1vdXMgbW9kdWxlIHNvLCB0aHJvdWdoIHBhdGggbWFwcGluZywgaXQgY2FuIGJlIGFsaWFzZWRcbiAgICBkZWZpbmUoZnVuY3Rpb24oKSB7XG4gICAgICByZXR1cm4gQmVuY2htYXJrO1xuICAgIH0pO1xuICB9XG4gIC8vIGNoZWNrIGZvciBgZXhwb3J0c2AgYWZ0ZXIgYGRlZmluZWAgaW4gY2FzZSBhIGJ1aWxkIG9wdGltaXplciBhZGRzIGFuIGBleHBvcnRzYCBvYmplY3RcbiAgZWxzZSBpZiAoZnJlZUV4cG9ydHMpIHtcbiAgICAvLyBpbiBOb2RlLmpzIG9yIFJpbmdvSlMgdjAuOC4wK1xuICAgIGlmICh0eXBlb2YgbW9kdWxlID09ICdvYmplY3QnICYmIG1vZHVsZSAmJiBtb2R1bGUuZXhwb3J0cyA9PSBmcmVlRXhwb3J0cykge1xuICAgICAgKG1vZHVsZS5leHBvcnRzID0gQmVuY2htYXJrKS5CZW5jaG1hcmsgPSBCZW5jaG1hcms7XG4gICAgfVxuICAgIC8vIGluIE5hcndoYWwgb3IgUmluZ29KUyB2MC43LjAtXG4gICAgZWxzZSB7XG4gICAgICBmcmVlRXhwb3J0cy5CZW5jaG1hcmsgPSBCZW5jaG1hcms7XG4gICAgfVxuICB9XG4gIC8vIGluIGEgYnJvd3NlciBvciBSaGlub1xuICBlbHNlIHtcbiAgICAvLyB1c2Ugc3F1YXJlIGJyYWNrZXQgbm90YXRpb24gc28gQ2xvc3VyZSBDb21waWxlciB3b24ndCBtdW5nZSBgQmVuY2htYXJrYFxuICAgIC8vIGh0dHA6Ly9jb2RlLmdvb2dsZS5jb20vY2xvc3VyZS9jb21waWxlci9kb2NzL2FwaS10dXRvcmlhbDMuaHRtbCNleHBvcnRcbiAgICB3aW5kb3dbJ0JlbmNobWFyayddID0gQmVuY2htYXJrO1xuICB9XG5cbiAgLy8gdHJpZ2dlciBjbG9jaydzIGxhenkgZGVmaW5lIGVhcmx5IHRvIGF2b2lkIGEgc2VjdXJpdHkgZXJyb3JcbiAgaWYgKHN1cHBvcnQuYWlyKSB7XG4gICAgY2xvY2soeyAnX29yaWdpbmFsJzogeyAnZm4nOiBub29wLCAnY291bnQnOiAxLCAnb3B0aW9ucyc6IHt9IH0gfSk7XG4gIH1cbn0odGhpcykpO1xuIiwiZXhwb3J0cy5yZWFkSUVFRTc1NCA9IGZ1bmN0aW9uKGJ1ZmZlciwgb2Zmc2V0LCBpc0JFLCBtTGVuLCBuQnl0ZXMpIHtcbiAgdmFyIGUsIG0sXG4gICAgICBlTGVuID0gbkJ5dGVzICogOCAtIG1MZW4gLSAxLFxuICAgICAgZU1heCA9ICgxIDw8IGVMZW4pIC0gMSxcbiAgICAgIGVCaWFzID0gZU1heCA+PiAxLFxuICAgICAgbkJpdHMgPSAtNyxcbiAgICAgIGkgPSBpc0JFID8gMCA6IChuQnl0ZXMgLSAxKSxcbiAgICAgIGQgPSBpc0JFID8gMSA6IC0xLFxuICAgICAgcyA9IGJ1ZmZlcltvZmZzZXQgKyBpXTtcblxuICBpICs9IGQ7XG5cbiAgZSA9IHMgJiAoKDEgPDwgKC1uQml0cykpIC0gMSk7XG4gIHMgPj49ICgtbkJpdHMpO1xuICBuQml0cyArPSBlTGVuO1xuICBmb3IgKDsgbkJpdHMgPiAwOyBlID0gZSAqIDI1NiArIGJ1ZmZlcltvZmZzZXQgKyBpXSwgaSArPSBkLCBuQml0cyAtPSA4KTtcblxuICBtID0gZSAmICgoMSA8PCAoLW5CaXRzKSkgLSAxKTtcbiAgZSA+Pj0gKC1uQml0cyk7XG4gIG5CaXRzICs9IG1MZW47XG4gIGZvciAoOyBuQml0cyA+IDA7IG0gPSBtICogMjU2ICsgYnVmZmVyW29mZnNldCArIGldLCBpICs9IGQsIG5CaXRzIC09IDgpO1xuXG4gIGlmIChlID09PSAwKSB7XG4gICAgZSA9IDEgLSBlQmlhcztcbiAgfSBlbHNlIGlmIChlID09PSBlTWF4KSB7XG4gICAgcmV0dXJuIG0gPyBOYU4gOiAoKHMgPyAtMSA6IDEpICogSW5maW5pdHkpO1xuICB9IGVsc2Uge1xuICAgIG0gPSBtICsgTWF0aC5wb3coMiwgbUxlbik7XG4gICAgZSA9IGUgLSBlQmlhcztcbiAgfVxuICByZXR1cm4gKHMgPyAtMSA6IDEpICogbSAqIE1hdGgucG93KDIsIGUgLSBtTGVuKTtcbn07XG5cbmV4cG9ydHMud3JpdGVJRUVFNzU0ID0gZnVuY3Rpb24oYnVmZmVyLCB2YWx1ZSwgb2Zmc2V0LCBpc0JFLCBtTGVuLCBuQnl0ZXMpIHtcbiAgdmFyIGUsIG0sIGMsXG4gICAgICBlTGVuID0gbkJ5dGVzICogOCAtIG1MZW4gLSAxLFxuICAgICAgZU1heCA9ICgxIDw8IGVMZW4pIC0gMSxcbiAgICAgIGVCaWFzID0gZU1heCA+PiAxLFxuICAgICAgcnQgPSAobUxlbiA9PT0gMjMgPyBNYXRoLnBvdygyLCAtMjQpIC0gTWF0aC5wb3coMiwgLTc3KSA6IDApLFxuICAgICAgaSA9IGlzQkUgPyAobkJ5dGVzIC0gMSkgOiAwLFxuICAgICAgZCA9IGlzQkUgPyAtMSA6IDEsXG4gICAgICBzID0gdmFsdWUgPCAwIHx8ICh2YWx1ZSA9PT0gMCAmJiAxIC8gdmFsdWUgPCAwKSA/IDEgOiAwO1xuXG4gIHZhbHVlID0gTWF0aC5hYnModmFsdWUpO1xuXG4gIGlmIChpc05hTih2YWx1ZSkgfHwgdmFsdWUgPT09IEluZmluaXR5KSB7XG4gICAgbSA9IGlzTmFOKHZhbHVlKSA/IDEgOiAwO1xuICAgIGUgPSBlTWF4O1xuICB9IGVsc2Uge1xuICAgIGUgPSBNYXRoLmZsb29yKE1hdGgubG9nKHZhbHVlKSAvIE1hdGguTE4yKTtcbiAgICBpZiAodmFsdWUgKiAoYyA9IE1hdGgucG93KDIsIC1lKSkgPCAxKSB7XG4gICAgICBlLS07XG4gICAgICBjICo9IDI7XG4gICAgfVxuICAgIGlmIChlICsgZUJpYXMgPj0gMSkge1xuICAgICAgdmFsdWUgKz0gcnQgLyBjO1xuICAgIH0gZWxzZSB7XG4gICAgICB2YWx1ZSArPSBydCAqIE1hdGgucG93KDIsIDEgLSBlQmlhcyk7XG4gICAgfVxuICAgIGlmICh2YWx1ZSAqIGMgPj0gMikge1xuICAgICAgZSsrO1xuICAgICAgYyAvPSAyO1xuICAgIH1cblxuICAgIGlmIChlICsgZUJpYXMgPj0gZU1heCkge1xuICAgICAgbSA9IDA7XG4gICAgICBlID0gZU1heDtcbiAgICB9IGVsc2UgaWYgKGUgKyBlQmlhcyA+PSAxKSB7XG4gICAgICBtID0gKHZhbHVlICogYyAtIDEpICogTWF0aC5wb3coMiwgbUxlbik7XG4gICAgICBlID0gZSArIGVCaWFzO1xuICAgIH0gZWxzZSB7XG4gICAgICBtID0gdmFsdWUgKiBNYXRoLnBvdygyLCBlQmlhcyAtIDEpICogTWF0aC5wb3coMiwgbUxlbik7XG4gICAgICBlID0gMDtcbiAgICB9XG4gIH1cblxuICBmb3IgKDsgbUxlbiA+PSA4OyBidWZmZXJbb2Zmc2V0ICsgaV0gPSBtICYgMHhmZiwgaSArPSBkLCBtIC89IDI1NiwgbUxlbiAtPSA4KTtcblxuICBlID0gKGUgPDwgbUxlbikgfCBtO1xuICBlTGVuICs9IG1MZW47XG4gIGZvciAoOyBlTGVuID4gMDsgYnVmZmVyW29mZnNldCArIGldID0gZSAmIDB4ZmYsIGkgKz0gZCwgZSAvPSAyNTYsIGVMZW4gLT0gOCk7XG5cbiAgYnVmZmVyW29mZnNldCArIGkgLSBkXSB8PSBzICogMTI4O1xufTtcbiIsInZhciBhc3NlcnQ7XG5leHBvcnRzLkJ1ZmZlciA9IEJ1ZmZlcjtcbmV4cG9ydHMuU2xvd0J1ZmZlciA9IEJ1ZmZlcjtcbkJ1ZmZlci5wb29sU2l6ZSA9IDgxOTI7XG5leHBvcnRzLklOU1BFQ1RfTUFYX0JZVEVTID0gNTA7XG5cbmZ1bmN0aW9uIHN0cmluZ3RyaW0oc3RyKSB7XG4gIGlmIChzdHIudHJpbSkgcmV0dXJuIHN0ci50cmltKCk7XG4gIHJldHVybiBzdHIucmVwbGFjZSgvXlxccyt8XFxzKyQvZywgJycpO1xufVxuXG5mdW5jdGlvbiBCdWZmZXIoc3ViamVjdCwgZW5jb2RpbmcsIG9mZnNldCkge1xuICBpZighYXNzZXJ0KSBhc3NlcnQ9IHJlcXVpcmUoJ2Fzc2VydCcpO1xuICBpZiAoISh0aGlzIGluc3RhbmNlb2YgQnVmZmVyKSkge1xuICAgIHJldHVybiBuZXcgQnVmZmVyKHN1YmplY3QsIGVuY29kaW5nLCBvZmZzZXQpO1xuICB9XG4gIHRoaXMucGFyZW50ID0gdGhpcztcbiAgdGhpcy5vZmZzZXQgPSAwO1xuXG4gIC8vIFdvcmstYXJvdW5kOiBub2RlJ3MgYmFzZTY0IGltcGxlbWVudGF0aW9uXG4gIC8vIGFsbG93cyBmb3Igbm9uLXBhZGRlZCBzdHJpbmdzIHdoaWxlIGJhc2U2NC1qc1xuICAvLyBkb2VzIG5vdC4uXG4gIGlmIChlbmNvZGluZyA9PSBcImJhc2U2NFwiICYmIHR5cGVvZiBzdWJqZWN0ID09IFwic3RyaW5nXCIpIHtcbiAgICBzdWJqZWN0ID0gc3RyaW5ndHJpbShzdWJqZWN0KTtcbiAgICB3aGlsZSAoc3ViamVjdC5sZW5ndGggJSA0ICE9IDApIHtcbiAgICAgIHN1YmplY3QgPSBzdWJqZWN0ICsgXCI9XCI7IFxuICAgIH1cbiAgfVxuXG4gIHZhciB0eXBlO1xuXG4gIC8vIEFyZSB3ZSBzbGljaW5nP1xuICBpZiAodHlwZW9mIG9mZnNldCA9PT0gJ251bWJlcicpIHtcbiAgICB0aGlzLmxlbmd0aCA9IGNvZXJjZShlbmNvZGluZyk7XG4gICAgLy8gc2xpY2luZyB3b3Jrcywgd2l0aCBsaW1pdGF0aW9ucyAobm8gcGFyZW50IHRyYWNraW5nL3VwZGF0ZSlcbiAgICAvLyBjaGVjayBodHRwczovL2dpdGh1Yi5jb20vdG9vdHMvYnVmZmVyLWJyb3dzZXJpZnkvaXNzdWVzLzE5XG4gICAgZm9yICh2YXIgaSA9IDA7IGkgPCB0aGlzLmxlbmd0aDsgaSsrKSB7XG4gICAgICAgIHRoaXNbaV0gPSBzdWJqZWN0LmdldChpK29mZnNldCk7XG4gICAgfVxuICB9IGVsc2Uge1xuICAgIC8vIEZpbmQgdGhlIGxlbmd0aFxuICAgIHN3aXRjaCAodHlwZSA9IHR5cGVvZiBzdWJqZWN0KSB7XG4gICAgICBjYXNlICdudW1iZXInOlxuICAgICAgICB0aGlzLmxlbmd0aCA9IGNvZXJjZShzdWJqZWN0KTtcbiAgICAgICAgYnJlYWs7XG5cbiAgICAgIGNhc2UgJ3N0cmluZyc6XG4gICAgICAgIHRoaXMubGVuZ3RoID0gQnVmZmVyLmJ5dGVMZW5ndGgoc3ViamVjdCwgZW5jb2RpbmcpO1xuICAgICAgICBicmVhaztcblxuICAgICAgY2FzZSAnb2JqZWN0JzogLy8gQXNzdW1lIG9iamVjdCBpcyBhbiBhcnJheVxuICAgICAgICB0aGlzLmxlbmd0aCA9IGNvZXJjZShzdWJqZWN0Lmxlbmd0aCk7XG4gICAgICAgIGJyZWFrO1xuXG4gICAgICBkZWZhdWx0OlxuICAgICAgICB0aHJvdyBuZXcgRXJyb3IoJ0ZpcnN0IGFyZ3VtZW50IG5lZWRzIHRvIGJlIGEgbnVtYmVyLCAnICtcbiAgICAgICAgICAgICAgICAgICAgICAgICdhcnJheSBvciBzdHJpbmcuJyk7XG4gICAgfVxuXG4gICAgLy8gVHJlYXQgYXJyYXktaXNoIG9iamVjdHMgYXMgYSBieXRlIGFycmF5LlxuICAgIGlmIChpc0FycmF5SXNoKHN1YmplY3QpKSB7XG4gICAgICBmb3IgKHZhciBpID0gMDsgaSA8IHRoaXMubGVuZ3RoOyBpKyspIHtcbiAgICAgICAgaWYgKHN1YmplY3QgaW5zdGFuY2VvZiBCdWZmZXIpIHtcbiAgICAgICAgICB0aGlzW2ldID0gc3ViamVjdC5yZWFkVUludDgoaSk7XG4gICAgICAgIH1cbiAgICAgICAgZWxzZSB7XG4gICAgICAgICAgdGhpc1tpXSA9IHN1YmplY3RbaV07XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9IGVsc2UgaWYgKHR5cGUgPT0gJ3N0cmluZycpIHtcbiAgICAgIC8vIFdlIGFyZSBhIHN0cmluZ1xuICAgICAgdGhpcy5sZW5ndGggPSB0aGlzLndyaXRlKHN1YmplY3QsIDAsIGVuY29kaW5nKTtcbiAgICB9IGVsc2UgaWYgKHR5cGUgPT09ICdudW1iZXInKSB7XG4gICAgICBmb3IgKHZhciBpID0gMDsgaSA8IHRoaXMubGVuZ3RoOyBpKyspIHtcbiAgICAgICAgdGhpc1tpXSA9IDA7XG4gICAgICB9XG4gICAgfVxuICB9XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUuZ2V0ID0gZnVuY3Rpb24gZ2V0KGkpIHtcbiAgaWYgKGkgPCAwIHx8IGkgPj0gdGhpcy5sZW5ndGgpIHRocm93IG5ldyBFcnJvcignb29iJyk7XG4gIHJldHVybiB0aGlzW2ldO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS5zZXQgPSBmdW5jdGlvbiBzZXQoaSwgdikge1xuICBpZiAoaSA8IDAgfHwgaSA+PSB0aGlzLmxlbmd0aCkgdGhyb3cgbmV3IEVycm9yKCdvb2InKTtcbiAgcmV0dXJuIHRoaXNbaV0gPSB2O1xufTtcblxuQnVmZmVyLmJ5dGVMZW5ndGggPSBmdW5jdGlvbiAoc3RyLCBlbmNvZGluZykge1xuICBzd2l0Y2ggKGVuY29kaW5nIHx8IFwidXRmOFwiKSB7XG4gICAgY2FzZSAnaGV4JzpcbiAgICAgIHJldHVybiBzdHIubGVuZ3RoIC8gMjtcblxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICAgIHJldHVybiB1dGY4VG9CeXRlcyhzdHIpLmxlbmd0aDtcblxuICAgIGNhc2UgJ2FzY2lpJzpcbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIHN0ci5sZW5ndGg7XG5cbiAgICBjYXNlICdiYXNlNjQnOlxuICAgICAgcmV0dXJuIGJhc2U2NFRvQnl0ZXMoc3RyKS5sZW5ndGg7XG5cbiAgICBkZWZhdWx0OlxuICAgICAgdGhyb3cgbmV3IEVycm9yKCdVbmtub3duIGVuY29kaW5nJyk7XG4gIH1cbn07XG5cbkJ1ZmZlci5wcm90b3R5cGUudXRmOFdyaXRlID0gZnVuY3Rpb24gKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgdmFyIGJ5dGVzLCBwb3M7XG4gIHJldHVybiBCdWZmZXIuX2NoYXJzV3JpdHRlbiA9ICBibGl0QnVmZmVyKHV0ZjhUb0J5dGVzKHN0cmluZyksIHRoaXMsIG9mZnNldCwgbGVuZ3RoKTtcbn07XG5cbkJ1ZmZlci5wcm90b3R5cGUuYXNjaWlXcml0ZSA9IGZ1bmN0aW9uIChzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIHZhciBieXRlcywgcG9zO1xuICByZXR1cm4gQnVmZmVyLl9jaGFyc1dyaXR0ZW4gPSAgYmxpdEJ1ZmZlcihhc2NpaVRvQnl0ZXMoc3RyaW5nKSwgdGhpcywgb2Zmc2V0LCBsZW5ndGgpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS5iaW5hcnlXcml0ZSA9IEJ1ZmZlci5wcm90b3R5cGUuYXNjaWlXcml0ZTtcblxuQnVmZmVyLnByb3RvdHlwZS5iYXNlNjRXcml0ZSA9IGZ1bmN0aW9uIChzdHJpbmcsIG9mZnNldCwgbGVuZ3RoKSB7XG4gIHZhciBieXRlcywgcG9zO1xuICByZXR1cm4gQnVmZmVyLl9jaGFyc1dyaXR0ZW4gPSBibGl0QnVmZmVyKGJhc2U2NFRvQnl0ZXMoc3RyaW5nKSwgdGhpcywgb2Zmc2V0LCBsZW5ndGgpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS5iYXNlNjRTbGljZSA9IGZ1bmN0aW9uIChzdGFydCwgZW5kKSB7XG4gIHZhciBieXRlcyA9IEFycmF5LnByb3RvdHlwZS5zbGljZS5hcHBseSh0aGlzLCBhcmd1bWVudHMpXG4gIHJldHVybiByZXF1aXJlKFwiYmFzZTY0LWpzXCIpLmZyb21CeXRlQXJyYXkoYnl0ZXMpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS51dGY4U2xpY2UgPSBmdW5jdGlvbiAoKSB7XG4gIHZhciBieXRlcyA9IEFycmF5LnByb3RvdHlwZS5zbGljZS5hcHBseSh0aGlzLCBhcmd1bWVudHMpO1xuICB2YXIgcmVzID0gXCJcIjtcbiAgdmFyIHRtcCA9IFwiXCI7XG4gIHZhciBpID0gMDtcbiAgd2hpbGUgKGkgPCBieXRlcy5sZW5ndGgpIHtcbiAgICBpZiAoYnl0ZXNbaV0gPD0gMHg3Rikge1xuICAgICAgcmVzICs9IGRlY29kZVV0ZjhDaGFyKHRtcCkgKyBTdHJpbmcuZnJvbUNoYXJDb2RlKGJ5dGVzW2ldKTtcbiAgICAgIHRtcCA9IFwiXCI7XG4gICAgfSBlbHNlXG4gICAgICB0bXAgKz0gXCIlXCIgKyBieXRlc1tpXS50b1N0cmluZygxNik7XG5cbiAgICBpKys7XG4gIH1cblxuICByZXR1cm4gcmVzICsgZGVjb2RlVXRmOENoYXIodG1wKTtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5hc2NpaVNsaWNlID0gZnVuY3Rpb24gKCkge1xuICB2YXIgYnl0ZXMgPSBBcnJheS5wcm90b3R5cGUuc2xpY2UuYXBwbHkodGhpcywgYXJndW1lbnRzKTtcbiAgdmFyIHJldCA9IFwiXCI7XG4gIGZvciAodmFyIGkgPSAwOyBpIDwgYnl0ZXMubGVuZ3RoOyBpKyspXG4gICAgcmV0ICs9IFN0cmluZy5mcm9tQ2hhckNvZGUoYnl0ZXNbaV0pO1xuICByZXR1cm4gcmV0O1xufVxuXG5CdWZmZXIucHJvdG90eXBlLmJpbmFyeVNsaWNlID0gQnVmZmVyLnByb3RvdHlwZS5hc2NpaVNsaWNlO1xuXG5CdWZmZXIucHJvdG90eXBlLmluc3BlY3QgPSBmdW5jdGlvbigpIHtcbiAgdmFyIG91dCA9IFtdLFxuICAgICAgbGVuID0gdGhpcy5sZW5ndGg7XG4gIGZvciAodmFyIGkgPSAwOyBpIDwgbGVuOyBpKyspIHtcbiAgICBvdXRbaV0gPSB0b0hleCh0aGlzW2ldKTtcbiAgICBpZiAoaSA9PSBleHBvcnRzLklOU1BFQ1RfTUFYX0JZVEVTKSB7XG4gICAgICBvdXRbaSArIDFdID0gJy4uLic7XG4gICAgICBicmVhaztcbiAgICB9XG4gIH1cbiAgcmV0dXJuICc8QnVmZmVyICcgKyBvdXQuam9pbignICcpICsgJz4nO1xufTtcblxuXG5CdWZmZXIucHJvdG90eXBlLmhleFNsaWNlID0gZnVuY3Rpb24oc3RhcnQsIGVuZCkge1xuICB2YXIgbGVuID0gdGhpcy5sZW5ndGg7XG5cbiAgaWYgKCFzdGFydCB8fCBzdGFydCA8IDApIHN0YXJ0ID0gMDtcbiAgaWYgKCFlbmQgfHwgZW5kIDwgMCB8fCBlbmQgPiBsZW4pIGVuZCA9IGxlbjtcblxuICB2YXIgb3V0ID0gJyc7XG4gIGZvciAodmFyIGkgPSBzdGFydDsgaSA8IGVuZDsgaSsrKSB7XG4gICAgb3V0ICs9IHRvSGV4KHRoaXNbaV0pO1xuICB9XG4gIHJldHVybiBvdXQ7XG59O1xuXG5cbkJ1ZmZlci5wcm90b3R5cGUudG9TdHJpbmcgPSBmdW5jdGlvbihlbmNvZGluZywgc3RhcnQsIGVuZCkge1xuICBlbmNvZGluZyA9IFN0cmluZyhlbmNvZGluZyB8fCAndXRmOCcpLnRvTG93ZXJDYXNlKCk7XG4gIHN0YXJ0ID0gK3N0YXJ0IHx8IDA7XG4gIGlmICh0eXBlb2YgZW5kID09ICd1bmRlZmluZWQnKSBlbmQgPSB0aGlzLmxlbmd0aDtcblxuICAvLyBGYXN0cGF0aCBlbXB0eSBzdHJpbmdzXG4gIGlmICgrZW5kID09IHN0YXJ0KSB7XG4gICAgcmV0dXJuICcnO1xuICB9XG5cbiAgc3dpdGNoIChlbmNvZGluZykge1xuICAgIGNhc2UgJ2hleCc6XG4gICAgICByZXR1cm4gdGhpcy5oZXhTbGljZShzdGFydCwgZW5kKTtcblxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICAgIHJldHVybiB0aGlzLnV0ZjhTbGljZShzdGFydCwgZW5kKTtcblxuICAgIGNhc2UgJ2FzY2lpJzpcbiAgICAgIHJldHVybiB0aGlzLmFzY2lpU2xpY2Uoc3RhcnQsIGVuZCk7XG5cbiAgICBjYXNlICdiaW5hcnknOlxuICAgICAgcmV0dXJuIHRoaXMuYmluYXJ5U2xpY2Uoc3RhcnQsIGVuZCk7XG5cbiAgICBjYXNlICdiYXNlNjQnOlxuICAgICAgcmV0dXJuIHRoaXMuYmFzZTY0U2xpY2Uoc3RhcnQsIGVuZCk7XG5cbiAgICBjYXNlICd1Y3MyJzpcbiAgICBjYXNlICd1Y3MtMic6XG4gICAgICByZXR1cm4gdGhpcy51Y3MyU2xpY2Uoc3RhcnQsIGVuZCk7XG5cbiAgICBkZWZhdWx0OlxuICAgICAgdGhyb3cgbmV3IEVycm9yKCdVbmtub3duIGVuY29kaW5nJyk7XG4gIH1cbn07XG5cblxuQnVmZmVyLnByb3RvdHlwZS5oZXhXcml0ZSA9IGZ1bmN0aW9uKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpIHtcbiAgb2Zmc2V0ID0gK29mZnNldCB8fCAwO1xuICB2YXIgcmVtYWluaW5nID0gdGhpcy5sZW5ndGggLSBvZmZzZXQ7XG4gIGlmICghbGVuZ3RoKSB7XG4gICAgbGVuZ3RoID0gcmVtYWluaW5nO1xuICB9IGVsc2Uge1xuICAgIGxlbmd0aCA9ICtsZW5ndGg7XG4gICAgaWYgKGxlbmd0aCA+IHJlbWFpbmluZykge1xuICAgICAgbGVuZ3RoID0gcmVtYWluaW5nO1xuICAgIH1cbiAgfVxuXG4gIC8vIG11c3QgYmUgYW4gZXZlbiBudW1iZXIgb2YgZGlnaXRzXG4gIHZhciBzdHJMZW4gPSBzdHJpbmcubGVuZ3RoO1xuICBpZiAoc3RyTGVuICUgMikge1xuICAgIHRocm93IG5ldyBFcnJvcignSW52YWxpZCBoZXggc3RyaW5nJyk7XG4gIH1cbiAgaWYgKGxlbmd0aCA+IHN0ckxlbiAvIDIpIHtcbiAgICBsZW5ndGggPSBzdHJMZW4gLyAyO1xuICB9XG4gIGZvciAodmFyIGkgPSAwOyBpIDwgbGVuZ3RoOyBpKyspIHtcbiAgICB2YXIgYnl0ZSA9IHBhcnNlSW50KHN0cmluZy5zdWJzdHIoaSAqIDIsIDIpLCAxNik7XG4gICAgaWYgKGlzTmFOKGJ5dGUpKSB0aHJvdyBuZXcgRXJyb3IoJ0ludmFsaWQgaGV4IHN0cmluZycpO1xuICAgIHRoaXNbb2Zmc2V0ICsgaV0gPSBieXRlO1xuICB9XG4gIEJ1ZmZlci5fY2hhcnNXcml0dGVuID0gaSAqIDI7XG4gIHJldHVybiBpO1xufTtcblxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlID0gZnVuY3Rpb24oc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCwgZW5jb2RpbmcpIHtcbiAgLy8gU3VwcG9ydCBib3RoIChzdHJpbmcsIG9mZnNldCwgbGVuZ3RoLCBlbmNvZGluZylcbiAgLy8gYW5kIHRoZSBsZWdhY3kgKHN0cmluZywgZW5jb2RpbmcsIG9mZnNldCwgbGVuZ3RoKVxuICBpZiAoaXNGaW5pdGUob2Zmc2V0KSkge1xuICAgIGlmICghaXNGaW5pdGUobGVuZ3RoKSkge1xuICAgICAgZW5jb2RpbmcgPSBsZW5ndGg7XG4gICAgICBsZW5ndGggPSB1bmRlZmluZWQ7XG4gICAgfVxuICB9IGVsc2UgeyAgLy8gbGVnYWN5XG4gICAgdmFyIHN3YXAgPSBlbmNvZGluZztcbiAgICBlbmNvZGluZyA9IG9mZnNldDtcbiAgICBvZmZzZXQgPSBsZW5ndGg7XG4gICAgbGVuZ3RoID0gc3dhcDtcbiAgfVxuXG4gIG9mZnNldCA9ICtvZmZzZXQgfHwgMDtcbiAgdmFyIHJlbWFpbmluZyA9IHRoaXMubGVuZ3RoIC0gb2Zmc2V0O1xuICBpZiAoIWxlbmd0aCkge1xuICAgIGxlbmd0aCA9IHJlbWFpbmluZztcbiAgfSBlbHNlIHtcbiAgICBsZW5ndGggPSArbGVuZ3RoO1xuICAgIGlmIChsZW5ndGggPiByZW1haW5pbmcpIHtcbiAgICAgIGxlbmd0aCA9IHJlbWFpbmluZztcbiAgICB9XG4gIH1cbiAgZW5jb2RpbmcgPSBTdHJpbmcoZW5jb2RpbmcgfHwgJ3V0ZjgnKS50b0xvd2VyQ2FzZSgpO1xuXG4gIHN3aXRjaCAoZW5jb2RpbmcpIHtcbiAgICBjYXNlICdoZXgnOlxuICAgICAgcmV0dXJuIHRoaXMuaGV4V3JpdGUoc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCk7XG5cbiAgICBjYXNlICd1dGY4JzpcbiAgICBjYXNlICd1dGYtOCc6XG4gICAgICByZXR1cm4gdGhpcy51dGY4V3JpdGUoc3RyaW5nLCBvZmZzZXQsIGxlbmd0aCk7XG5cbiAgICBjYXNlICdhc2NpaSc6XG4gICAgICByZXR1cm4gdGhpcy5hc2NpaVdyaXRlKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpO1xuXG4gICAgY2FzZSAnYmluYXJ5JzpcbiAgICAgIHJldHVybiB0aGlzLmJpbmFyeVdyaXRlKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpO1xuXG4gICAgY2FzZSAnYmFzZTY0JzpcbiAgICAgIHJldHVybiB0aGlzLmJhc2U2NFdyaXRlKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpO1xuXG4gICAgY2FzZSAndWNzMic6XG4gICAgY2FzZSAndWNzLTInOlxuICAgICAgcmV0dXJuIHRoaXMudWNzMldyaXRlKHN0cmluZywgb2Zmc2V0LCBsZW5ndGgpO1xuXG4gICAgZGVmYXVsdDpcbiAgICAgIHRocm93IG5ldyBFcnJvcignVW5rbm93biBlbmNvZGluZycpO1xuICB9XG59O1xuXG4vLyBzbGljZShzdGFydCwgZW5kKVxuZnVuY3Rpb24gY2xhbXAoaW5kZXgsIGxlbiwgZGVmYXVsdFZhbHVlKSB7XG4gIGlmICh0eXBlb2YgaW5kZXggIT09ICdudW1iZXInKSByZXR1cm4gZGVmYXVsdFZhbHVlO1xuICBpbmRleCA9IH5+aW5kZXg7ICAvLyBDb2VyY2UgdG8gaW50ZWdlci5cbiAgaWYgKGluZGV4ID49IGxlbikgcmV0dXJuIGxlbjtcbiAgaWYgKGluZGV4ID49IDApIHJldHVybiBpbmRleDtcbiAgaW5kZXggKz0gbGVuO1xuICBpZiAoaW5kZXggPj0gMCkgcmV0dXJuIGluZGV4O1xuICByZXR1cm4gMDtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5zbGljZSA9IGZ1bmN0aW9uKHN0YXJ0LCBlbmQpIHtcbiAgdmFyIGxlbiA9IHRoaXMubGVuZ3RoO1xuICBzdGFydCA9IGNsYW1wKHN0YXJ0LCBsZW4sIDApO1xuICBlbmQgPSBjbGFtcChlbmQsIGxlbiwgbGVuKTtcbiAgcmV0dXJuIG5ldyBCdWZmZXIodGhpcywgZW5kIC0gc3RhcnQsICtzdGFydCk7XG59O1xuXG4vLyBjb3B5KHRhcmdldEJ1ZmZlciwgdGFyZ2V0U3RhcnQ9MCwgc291cmNlU3RhcnQ9MCwgc291cmNlRW5kPWJ1ZmZlci5sZW5ndGgpXG5CdWZmZXIucHJvdG90eXBlLmNvcHkgPSBmdW5jdGlvbih0YXJnZXQsIHRhcmdldF9zdGFydCwgc3RhcnQsIGVuZCkge1xuICB2YXIgc291cmNlID0gdGhpcztcbiAgc3RhcnQgfHwgKHN0YXJ0ID0gMCk7XG4gIGlmIChlbmQgPT09IHVuZGVmaW5lZCB8fCBpc05hTihlbmQpKSB7XG4gICAgZW5kID0gdGhpcy5sZW5ndGg7XG4gIH1cbiAgdGFyZ2V0X3N0YXJ0IHx8ICh0YXJnZXRfc3RhcnQgPSAwKTtcblxuICBpZiAoZW5kIDwgc3RhcnQpIHRocm93IG5ldyBFcnJvcignc291cmNlRW5kIDwgc291cmNlU3RhcnQnKTtcblxuICAvLyBDb3B5IDAgYnl0ZXM7IHdlJ3JlIGRvbmVcbiAgaWYgKGVuZCA9PT0gc3RhcnQpIHJldHVybiAwO1xuICBpZiAodGFyZ2V0Lmxlbmd0aCA9PSAwIHx8IHNvdXJjZS5sZW5ndGggPT0gMCkgcmV0dXJuIDA7XG5cbiAgaWYgKHRhcmdldF9zdGFydCA8IDAgfHwgdGFyZ2V0X3N0YXJ0ID49IHRhcmdldC5sZW5ndGgpIHtcbiAgICB0aHJvdyBuZXcgRXJyb3IoJ3RhcmdldFN0YXJ0IG91dCBvZiBib3VuZHMnKTtcbiAgfVxuXG4gIGlmIChzdGFydCA8IDAgfHwgc3RhcnQgPj0gc291cmNlLmxlbmd0aCkge1xuICAgIHRocm93IG5ldyBFcnJvcignc291cmNlU3RhcnQgb3V0IG9mIGJvdW5kcycpO1xuICB9XG5cbiAgaWYgKGVuZCA8IDAgfHwgZW5kID4gc291cmNlLmxlbmd0aCkge1xuICAgIHRocm93IG5ldyBFcnJvcignc291cmNlRW5kIG91dCBvZiBib3VuZHMnKTtcbiAgfVxuXG4gIC8vIEFyZSB3ZSBvb2I/XG4gIGlmIChlbmQgPiB0aGlzLmxlbmd0aCkge1xuICAgIGVuZCA9IHRoaXMubGVuZ3RoO1xuICB9XG5cbiAgaWYgKHRhcmdldC5sZW5ndGggLSB0YXJnZXRfc3RhcnQgPCBlbmQgLSBzdGFydCkge1xuICAgIGVuZCA9IHRhcmdldC5sZW5ndGggLSB0YXJnZXRfc3RhcnQgKyBzdGFydDtcbiAgfVxuXG4gIHZhciB0ZW1wID0gW107XG4gIGZvciAodmFyIGk9c3RhcnQ7IGk8ZW5kOyBpKyspIHtcbiAgICBhc3NlcnQub2sodHlwZW9mIHRoaXNbaV0gIT09ICd1bmRlZmluZWQnLCBcImNvcHlpbmcgdW5kZWZpbmVkIGJ1ZmZlciBieXRlcyFcIik7XG4gICAgdGVtcC5wdXNoKHRoaXNbaV0pO1xuICB9XG5cbiAgZm9yICh2YXIgaT10YXJnZXRfc3RhcnQ7IGk8dGFyZ2V0X3N0YXJ0K3RlbXAubGVuZ3RoOyBpKyspIHtcbiAgICB0YXJnZXRbaV0gPSB0ZW1wW2ktdGFyZ2V0X3N0YXJ0XTtcbiAgfVxufTtcblxuLy8gZmlsbCh2YWx1ZSwgc3RhcnQ9MCwgZW5kPWJ1ZmZlci5sZW5ndGgpXG5CdWZmZXIucHJvdG90eXBlLmZpbGwgPSBmdW5jdGlvbiBmaWxsKHZhbHVlLCBzdGFydCwgZW5kKSB7XG4gIHZhbHVlIHx8ICh2YWx1ZSA9IDApO1xuICBzdGFydCB8fCAoc3RhcnQgPSAwKTtcbiAgZW5kIHx8IChlbmQgPSB0aGlzLmxlbmd0aCk7XG5cbiAgaWYgKHR5cGVvZiB2YWx1ZSA9PT0gJ3N0cmluZycpIHtcbiAgICB2YWx1ZSA9IHZhbHVlLmNoYXJDb2RlQXQoMCk7XG4gIH1cbiAgaWYgKCEodHlwZW9mIHZhbHVlID09PSAnbnVtYmVyJykgfHwgaXNOYU4odmFsdWUpKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCd2YWx1ZSBpcyBub3QgYSBudW1iZXInKTtcbiAgfVxuXG4gIGlmIChlbmQgPCBzdGFydCkgdGhyb3cgbmV3IEVycm9yKCdlbmQgPCBzdGFydCcpO1xuXG4gIC8vIEZpbGwgMCBieXRlczsgd2UncmUgZG9uZVxuICBpZiAoZW5kID09PSBzdGFydCkgcmV0dXJuIDA7XG4gIGlmICh0aGlzLmxlbmd0aCA9PSAwKSByZXR1cm4gMDtcblxuICBpZiAoc3RhcnQgPCAwIHx8IHN0YXJ0ID49IHRoaXMubGVuZ3RoKSB7XG4gICAgdGhyb3cgbmV3IEVycm9yKCdzdGFydCBvdXQgb2YgYm91bmRzJyk7XG4gIH1cblxuICBpZiAoZW5kIDwgMCB8fCBlbmQgPiB0aGlzLmxlbmd0aCkge1xuICAgIHRocm93IG5ldyBFcnJvcignZW5kIG91dCBvZiBib3VuZHMnKTtcbiAgfVxuXG4gIGZvciAodmFyIGkgPSBzdGFydDsgaSA8IGVuZDsgaSsrKSB7XG4gICAgdGhpc1tpXSA9IHZhbHVlO1xuICB9XG59XG5cbi8vIFN0YXRpYyBtZXRob2RzXG5CdWZmZXIuaXNCdWZmZXIgPSBmdW5jdGlvbiBpc0J1ZmZlcihiKSB7XG4gIHJldHVybiBiIGluc3RhbmNlb2YgQnVmZmVyIHx8IGIgaW5zdGFuY2VvZiBCdWZmZXI7XG59O1xuXG5CdWZmZXIuY29uY2F0ID0gZnVuY3Rpb24gKGxpc3QsIHRvdGFsTGVuZ3RoKSB7XG4gIGlmICghaXNBcnJheShsaXN0KSkge1xuICAgIHRocm93IG5ldyBFcnJvcihcIlVzYWdlOiBCdWZmZXIuY29uY2F0KGxpc3QsIFt0b3RhbExlbmd0aF0pXFxuIFxcXG4gICAgICBsaXN0IHNob3VsZCBiZSBhbiBBcnJheS5cIik7XG4gIH1cblxuICBpZiAobGlzdC5sZW5ndGggPT09IDApIHtcbiAgICByZXR1cm4gbmV3IEJ1ZmZlcigwKTtcbiAgfSBlbHNlIGlmIChsaXN0Lmxlbmd0aCA9PT0gMSkge1xuICAgIHJldHVybiBsaXN0WzBdO1xuICB9XG5cbiAgaWYgKHR5cGVvZiB0b3RhbExlbmd0aCAhPT0gJ251bWJlcicpIHtcbiAgICB0b3RhbExlbmd0aCA9IDA7XG4gICAgZm9yICh2YXIgaSA9IDA7IGkgPCBsaXN0Lmxlbmd0aDsgaSsrKSB7XG4gICAgICB2YXIgYnVmID0gbGlzdFtpXTtcbiAgICAgIHRvdGFsTGVuZ3RoICs9IGJ1Zi5sZW5ndGg7XG4gICAgfVxuICB9XG5cbiAgdmFyIGJ1ZmZlciA9IG5ldyBCdWZmZXIodG90YWxMZW5ndGgpO1xuICB2YXIgcG9zID0gMDtcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBsaXN0Lmxlbmd0aDsgaSsrKSB7XG4gICAgdmFyIGJ1ZiA9IGxpc3RbaV07XG4gICAgYnVmLmNvcHkoYnVmZmVyLCBwb3MpO1xuICAgIHBvcyArPSBidWYubGVuZ3RoO1xuICB9XG4gIHJldHVybiBidWZmZXI7XG59O1xuXG5CdWZmZXIuaXNFbmNvZGluZyA9IGZ1bmN0aW9uKGVuY29kaW5nKSB7XG4gIHN3aXRjaCAoKGVuY29kaW5nICsgJycpLnRvTG93ZXJDYXNlKCkpIHtcbiAgICBjYXNlICdoZXgnOlxuICAgIGNhc2UgJ3V0ZjgnOlxuICAgIGNhc2UgJ3V0Zi04JzpcbiAgICBjYXNlICdhc2NpaSc6XG4gICAgY2FzZSAnYmluYXJ5JzpcbiAgICBjYXNlICdiYXNlNjQnOlxuICAgIGNhc2UgJ3VjczInOlxuICAgIGNhc2UgJ3Vjcy0yJzpcbiAgICBjYXNlICd1dGYxNmxlJzpcbiAgICBjYXNlICd1dGYtMTZsZSc6XG4gICAgY2FzZSAncmF3JzpcbiAgICAgIHJldHVybiB0cnVlO1xuXG4gICAgZGVmYXVsdDpcbiAgICAgIHJldHVybiBmYWxzZTtcbiAgfVxufTtcblxuLy8gaGVscGVyc1xuXG5mdW5jdGlvbiBjb2VyY2UobGVuZ3RoKSB7XG4gIC8vIENvZXJjZSBsZW5ndGggdG8gYSBudW1iZXIgKHBvc3NpYmx5IE5hTiksIHJvdW5kIHVwXG4gIC8vIGluIGNhc2UgaXQncyBmcmFjdGlvbmFsIChlLmcuIDEyMy40NTYpIHRoZW4gZG8gYVxuICAvLyBkb3VibGUgbmVnYXRlIHRvIGNvZXJjZSBhIE5hTiB0byAwLiBFYXN5LCByaWdodD9cbiAgbGVuZ3RoID0gfn5NYXRoLmNlaWwoK2xlbmd0aCk7XG4gIHJldHVybiBsZW5ndGggPCAwID8gMCA6IGxlbmd0aDtcbn1cblxuZnVuY3Rpb24gaXNBcnJheShzdWJqZWN0KSB7XG4gIHJldHVybiAoQXJyYXkuaXNBcnJheSB8fFxuICAgIGZ1bmN0aW9uKHN1YmplY3Qpe1xuICAgICAgcmV0dXJuIHt9LnRvU3RyaW5nLmFwcGx5KHN1YmplY3QpID09ICdbb2JqZWN0IEFycmF5XSdcbiAgICB9KVxuICAgIChzdWJqZWN0KVxufVxuXG5mdW5jdGlvbiBpc0FycmF5SXNoKHN1YmplY3QpIHtcbiAgcmV0dXJuIGlzQXJyYXkoc3ViamVjdCkgfHwgQnVmZmVyLmlzQnVmZmVyKHN1YmplY3QpIHx8XG4gICAgICAgICBzdWJqZWN0ICYmIHR5cGVvZiBzdWJqZWN0ID09PSAnb2JqZWN0JyAmJlxuICAgICAgICAgdHlwZW9mIHN1YmplY3QubGVuZ3RoID09PSAnbnVtYmVyJztcbn1cblxuZnVuY3Rpb24gdG9IZXgobikge1xuICBpZiAobiA8IDE2KSByZXR1cm4gJzAnICsgbi50b1N0cmluZygxNik7XG4gIHJldHVybiBuLnRvU3RyaW5nKDE2KTtcbn1cblxuZnVuY3Rpb24gdXRmOFRvQnl0ZXMoc3RyKSB7XG4gIHZhciBieXRlQXJyYXkgPSBbXTtcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBzdHIubGVuZ3RoOyBpKyspXG4gICAgaWYgKHN0ci5jaGFyQ29kZUF0KGkpIDw9IDB4N0YpXG4gICAgICBieXRlQXJyYXkucHVzaChzdHIuY2hhckNvZGVBdChpKSk7XG4gICAgZWxzZSB7XG4gICAgICB2YXIgaCA9IGVuY29kZVVSSUNvbXBvbmVudChzdHIuY2hhckF0KGkpKS5zdWJzdHIoMSkuc3BsaXQoJyUnKTtcbiAgICAgIGZvciAodmFyIGogPSAwOyBqIDwgaC5sZW5ndGg7IGorKylcbiAgICAgICAgYnl0ZUFycmF5LnB1c2gocGFyc2VJbnQoaFtqXSwgMTYpKTtcbiAgICB9XG5cbiAgcmV0dXJuIGJ5dGVBcnJheTtcbn1cblxuZnVuY3Rpb24gYXNjaWlUb0J5dGVzKHN0cikge1xuICB2YXIgYnl0ZUFycmF5ID0gW11cbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBzdHIubGVuZ3RoOyBpKysgKVxuICAgIC8vIE5vZGUncyBjb2RlIHNlZW1zIHRvIGJlIGRvaW5nIHRoaXMgYW5kIG5vdCAmIDB4N0YuLlxuICAgIGJ5dGVBcnJheS5wdXNoKCBzdHIuY2hhckNvZGVBdChpKSAmIDB4RkYgKTtcblxuICByZXR1cm4gYnl0ZUFycmF5O1xufVxuXG5mdW5jdGlvbiBiYXNlNjRUb0J5dGVzKHN0cikge1xuICByZXR1cm4gcmVxdWlyZShcImJhc2U2NC1qc1wiKS50b0J5dGVBcnJheShzdHIpO1xufVxuXG5mdW5jdGlvbiBibGl0QnVmZmVyKHNyYywgZHN0LCBvZmZzZXQsIGxlbmd0aCkge1xuICB2YXIgcG9zLCBpID0gMDtcbiAgd2hpbGUgKGkgPCBsZW5ndGgpIHtcbiAgICBpZiAoKGkrb2Zmc2V0ID49IGRzdC5sZW5ndGgpIHx8IChpID49IHNyYy5sZW5ndGgpKVxuICAgICAgYnJlYWs7XG5cbiAgICBkc3RbaSArIG9mZnNldF0gPSBzcmNbaV07XG4gICAgaSsrO1xuICB9XG4gIHJldHVybiBpO1xufVxuXG5mdW5jdGlvbiBkZWNvZGVVdGY4Q2hhcihzdHIpIHtcbiAgdHJ5IHtcbiAgICByZXR1cm4gZGVjb2RlVVJJQ29tcG9uZW50KHN0cik7XG4gIH0gY2F0Y2ggKGVycikge1xuICAgIHJldHVybiBTdHJpbmcuZnJvbUNoYXJDb2RlKDB4RkZGRCk7IC8vIFVURiA4IGludmFsaWQgY2hhclxuICB9XG59XG5cbi8vIHJlYWQvd3JpdGUgYml0LXR3aWRkbGluZ1xuXG5CdWZmZXIucHJvdG90eXBlLnJlYWRVSW50OCA9IGZ1bmN0aW9uKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZmZlciA9IHRoaXM7XG5cbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayhvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIG9mZnNldCcpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCA8IGJ1ZmZlci5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWZmZXIubGVuZ3RoKSByZXR1cm47XG5cbiAgcmV0dXJuIGJ1ZmZlcltvZmZzZXRdO1xufTtcblxuZnVuY3Rpb24gcmVhZFVJbnQxNihidWZmZXIsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIHZhciB2YWwgPSAwO1xuXG5cbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayh0eXBlb2YgKGlzQmlnRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3Npbmcgb2Zmc2V0Jyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICsgMSA8IGJ1ZmZlci5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWZmZXIubGVuZ3RoKSByZXR1cm4gMDtcblxuICBpZiAoaXNCaWdFbmRpYW4pIHtcbiAgICB2YWwgPSBidWZmZXJbb2Zmc2V0XSA8PCA4O1xuICAgIGlmIChvZmZzZXQgKyAxIDwgYnVmZmVyLmxlbmd0aCkge1xuICAgICAgdmFsIHw9IGJ1ZmZlcltvZmZzZXQgKyAxXTtcbiAgICB9XG4gIH0gZWxzZSB7XG4gICAgdmFsID0gYnVmZmVyW29mZnNldF07XG4gICAgaWYgKG9mZnNldCArIDEgPCBidWZmZXIubGVuZ3RoKSB7XG4gICAgICB2YWwgfD0gYnVmZmVyW29mZnNldCArIDFdIDw8IDg7XG4gICAgfVxuICB9XG5cbiAgcmV0dXJuIHZhbDtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkVUludDE2TEUgPSBmdW5jdGlvbihvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiByZWFkVUludDE2KHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KTtcbn07XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQxNkJFID0gZnVuY3Rpb24ob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gcmVhZFVJbnQxNih0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cbmZ1bmN0aW9uIHJlYWRVSW50MzIoYnVmZmVyLCBvZmZzZXQsIGlzQmlnRW5kaWFuLCBub0Fzc2VydCkge1xuICB2YXIgdmFsID0gMDtcblxuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0Lm9rKHR5cGVvZiAoaXNCaWdFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyAzIDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJyk7XG4gIH1cblxuICBpZiAob2Zmc2V0ID49IGJ1ZmZlci5sZW5ndGgpIHJldHVybiAwO1xuXG4gIGlmIChpc0JpZ0VuZGlhbikge1xuICAgIGlmIChvZmZzZXQgKyAxIDwgYnVmZmVyLmxlbmd0aClcbiAgICAgIHZhbCA9IGJ1ZmZlcltvZmZzZXQgKyAxXSA8PCAxNjtcbiAgICBpZiAob2Zmc2V0ICsgMiA8IGJ1ZmZlci5sZW5ndGgpXG4gICAgICB2YWwgfD0gYnVmZmVyW29mZnNldCArIDJdIDw8IDg7XG4gICAgaWYgKG9mZnNldCArIDMgPCBidWZmZXIubGVuZ3RoKVxuICAgICAgdmFsIHw9IGJ1ZmZlcltvZmZzZXQgKyAzXTtcbiAgICB2YWwgPSB2YWwgKyAoYnVmZmVyW29mZnNldF0gPDwgMjQgPj4+IDApO1xuICB9IGVsc2Uge1xuICAgIGlmIChvZmZzZXQgKyAyIDwgYnVmZmVyLmxlbmd0aClcbiAgICAgIHZhbCA9IGJ1ZmZlcltvZmZzZXQgKyAyXSA8PCAxNjtcbiAgICBpZiAob2Zmc2V0ICsgMSA8IGJ1ZmZlci5sZW5ndGgpXG4gICAgICB2YWwgfD0gYnVmZmVyW29mZnNldCArIDFdIDw8IDg7XG4gICAgdmFsIHw9IGJ1ZmZlcltvZmZzZXRdO1xuICAgIGlmIChvZmZzZXQgKyAzIDwgYnVmZmVyLmxlbmd0aClcbiAgICAgIHZhbCA9IHZhbCArIChidWZmZXJbb2Zmc2V0ICsgM10gPDwgMjQgPj4+IDApO1xuICB9XG5cbiAgcmV0dXJuIHZhbDtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkVUludDMyTEUgPSBmdW5jdGlvbihvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiByZWFkVUludDMyKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KTtcbn07XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZFVJbnQzMkJFID0gZnVuY3Rpb24ob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gcmVhZFVJbnQzMih0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cblxuLypcbiAqIFNpZ25lZCBpbnRlZ2VyIHR5cGVzLCB5YXkgdGVhbSEgQSByZW1pbmRlciBvbiBob3cgdHdvJ3MgY29tcGxlbWVudCBhY3R1YWxseVxuICogd29ya3MuIFRoZSBmaXJzdCBiaXQgaXMgdGhlIHNpZ25lZCBiaXQsIGkuZS4gdGVsbHMgdXMgd2hldGhlciBvciBub3QgdGhlXG4gKiBudW1iZXIgc2hvdWxkIGJlIHBvc2l0aXZlIG9yIG5lZ2F0aXZlLiBJZiB0aGUgdHdvJ3MgY29tcGxlbWVudCB2YWx1ZSBpc1xuICogcG9zaXRpdmUsIHRoZW4gd2UncmUgZG9uZSwgYXMgaXQncyBlcXVpdmFsZW50IHRvIHRoZSB1bnNpZ25lZCByZXByZXNlbnRhdGlvbi5cbiAqXG4gKiBOb3cgaWYgdGhlIG51bWJlciBpcyBwb3NpdGl2ZSwgeW91J3JlIHByZXR0eSBtdWNoIGRvbmUsIHlvdSBjYW4ganVzdCBsZXZlcmFnZVxuICogdGhlIHVuc2lnbmVkIHRyYW5zbGF0aW9ucyBhbmQgcmV0dXJuIHRob3NlLiBVbmZvcnR1bmF0ZWx5LCBuZWdhdGl2ZSBudW1iZXJzXG4gKiBhcmVuJ3QgcXVpdGUgdGhhdCBzdHJhaWdodGZvcndhcmQuXG4gKlxuICogQXQgZmlyc3QgZ2xhbmNlLCBvbmUgbWlnaHQgYmUgaW5jbGluZWQgdG8gdXNlIHRoZSB0cmFkaXRpb25hbCBmb3JtdWxhIHRvXG4gKiB0cmFuc2xhdGUgYmluYXJ5IG51bWJlcnMgYmV0d2VlbiB0aGUgcG9zaXRpdmUgYW5kIG5lZ2F0aXZlIHZhbHVlcyBpbiB0d28nc1xuICogY29tcGxlbWVudC4gKFRob3VnaCBpdCBkb2Vzbid0IHF1aXRlIHdvcmsgZm9yIHRoZSBtb3N0IG5lZ2F0aXZlIHZhbHVlKVxuICogTWFpbmx5OlxuICogIC0gaW52ZXJ0IGFsbCB0aGUgYml0c1xuICogIC0gYWRkIG9uZSB0byB0aGUgcmVzdWx0XG4gKlxuICogT2YgY291cnNlLCB0aGlzIGRvZXNuJ3QgcXVpdGUgd29yayBpbiBKYXZhc2NyaXB0LiBUYWtlIGZvciBleGFtcGxlIHRoZSB2YWx1ZVxuICogb2YgLTEyOC4gVGhpcyBjb3VsZCBiZSByZXByZXNlbnRlZCBpbiAxNiBiaXRzIChiaWctZW5kaWFuKSBhcyAweGZmODAuIEJ1dCBvZlxuICogY291cnNlLCBKYXZhc2NyaXB0IHdpbGwgZG8gdGhlIGZvbGxvd2luZzpcbiAqXG4gKiA+IH4weGZmODBcbiAqIC02NTQwOVxuICpcbiAqIFdob2ggdGhlcmUsIEphdmFzY3JpcHQsIHRoYXQncyBub3QgcXVpdGUgcmlnaHQuIEJ1dCB3YWl0LCBhY2NvcmRpbmcgdG9cbiAqIEphdmFzY3JpcHQgdGhhdCdzIHBlcmZlY3RseSBjb3JyZWN0LiBXaGVuIEphdmFzY3JpcHQgZW5kcyB1cCBzZWVpbmcgdGhlXG4gKiBjb25zdGFudCAweGZmODAsIGl0IGhhcyBubyBub3Rpb24gdGhhdCBpdCBpcyBhY3R1YWxseSBhIHNpZ25lZCBudW1iZXIuIEl0XG4gKiBhc3N1bWVzIHRoYXQgd2UndmUgaW5wdXQgdGhlIHVuc2lnbmVkIHZhbHVlIDB4ZmY4MC4gVGh1cywgd2hlbiBpdCBkb2VzIHRoZVxuICogYmluYXJ5IG5lZ2F0aW9uLCBpdCBjYXN0cyBpdCBpbnRvIGEgc2lnbmVkIHZhbHVlLCAocG9zaXRpdmUgMHhmZjgwKS4gVGhlblxuICogd2hlbiB5b3UgcGVyZm9ybSBiaW5hcnkgbmVnYXRpb24gb24gdGhhdCwgaXQgdHVybnMgaXQgaW50byBhIG5lZ2F0aXZlIG51bWJlci5cbiAqXG4gKiBJbnN0ZWFkLCB3ZSdyZSBnb2luZyB0byBoYXZlIHRvIHVzZSB0aGUgZm9sbG93aW5nIGdlbmVyYWwgZm9ybXVsYSwgdGhhdCB3b3Jrc1xuICogaW4gYSByYXRoZXIgSmF2YXNjcmlwdCBmcmllbmRseSB3YXkuIEknbSBnbGFkIHdlIGRvbid0IHN1cHBvcnQgdGhpcyBraW5kIG9mXG4gKiB3ZWlyZCBudW1iZXJpbmcgc2NoZW1lIGluIHRoZSBrZXJuZWwuXG4gKlxuICogKEJJVC1NQVggLSAodW5zaWduZWQpdmFsICsgMSkgKiAtMVxuICpcbiAqIFRoZSBhc3R1dGUgb2JzZXJ2ZXIsIG1heSB0aGluayB0aGF0IHRoaXMgZG9lc24ndCBtYWtlIHNlbnNlIGZvciA4LWJpdCBudW1iZXJzXG4gKiAocmVhbGx5IGl0IGlzbid0IG5lY2Vzc2FyeSBmb3IgdGhlbSkuIEhvd2V2ZXIsIHdoZW4geW91IGdldCAxNi1iaXQgbnVtYmVycyxcbiAqIHlvdSBkby4gTGV0J3MgZ28gYmFjayB0byBvdXIgcHJpb3IgZXhhbXBsZSBhbmQgc2VlIGhvdyB0aGlzIHdpbGwgbG9vazpcbiAqXG4gKiAoMHhmZmZmIC0gMHhmZjgwICsgMSkgKiAtMVxuICogKDB4MDA3ZiArIDEpICogLTFcbiAqICgweDAwODApICogLTFcbiAqL1xuQnVmZmVyLnByb3RvdHlwZS5yZWFkSW50OCA9IGZ1bmN0aW9uKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZmZlciA9IHRoaXM7XG4gIHZhciBuZWc7XG5cbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayhvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIG9mZnNldCcpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCA8IGJ1ZmZlci5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuICB9XG5cbiAgaWYgKG9mZnNldCA+PSBidWZmZXIubGVuZ3RoKSByZXR1cm47XG5cbiAgbmVnID0gYnVmZmVyW29mZnNldF0gJiAweDgwO1xuICBpZiAoIW5lZykge1xuICAgIHJldHVybiAoYnVmZmVyW29mZnNldF0pO1xuICB9XG5cbiAgcmV0dXJuICgoMHhmZiAtIGJ1ZmZlcltvZmZzZXRdICsgMSkgKiAtMSk7XG59O1xuXG5mdW5jdGlvbiByZWFkSW50MTYoYnVmZmVyLCBvZmZzZXQsIGlzQmlnRW5kaWFuLCBub0Fzc2VydCkge1xuICB2YXIgbmVnLCB2YWw7XG5cbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayh0eXBlb2YgKGlzQmlnRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3Npbmcgb2Zmc2V0Jyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICsgMSA8IGJ1ZmZlci5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gcmVhZCBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuICB9XG5cbiAgdmFsID0gcmVhZFVJbnQxNihidWZmZXIsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KTtcbiAgbmVnID0gdmFsICYgMHg4MDAwO1xuICBpZiAoIW5lZykge1xuICAgIHJldHVybiB2YWw7XG4gIH1cblxuICByZXR1cm4gKDB4ZmZmZiAtIHZhbCArIDEpICogLTE7XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZEludDE2TEUgPSBmdW5jdGlvbihvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiByZWFkSW50MTYodGhpcywgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkSW50MTZCRSA9IGZ1bmN0aW9uKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIHJlYWRJbnQxNih0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cbmZ1bmN0aW9uIHJlYWRJbnQzMihidWZmZXIsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIHZhciBuZWcsIHZhbDtcblxuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0Lm9rKHR5cGVvZiAoaXNCaWdFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyAzIDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJyk7XG4gIH1cblxuICB2YWwgPSByZWFkVUludDMyKGJ1ZmZlciwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpO1xuICBuZWcgPSB2YWwgJiAweDgwMDAwMDAwO1xuICBpZiAoIW5lZykge1xuICAgIHJldHVybiAodmFsKTtcbiAgfVxuXG4gIHJldHVybiAoMHhmZmZmZmZmZiAtIHZhbCArIDEpICogLTE7XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZEludDMyTEUgPSBmdW5jdGlvbihvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiByZWFkSW50MzIodGhpcywgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkSW50MzJCRSA9IGZ1bmN0aW9uKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIHJlYWRJbnQzMih0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cbmZ1bmN0aW9uIHJlYWRGbG9hdChidWZmZXIsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQub2sodHlwZW9mIChpc0JpZ0VuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyAzIDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJyk7XG4gIH1cblxuICByZXR1cm4gcmVxdWlyZSgnLi9idWZmZXJfaWVlZTc1NCcpLnJlYWRJRUVFNzU0KGJ1ZmZlciwgb2Zmc2V0LCBpc0JpZ0VuZGlhbixcbiAgICAgIDIzLCA0KTtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkRmxvYXRMRSA9IGZ1bmN0aW9uKG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgcmV0dXJuIHJlYWRGbG9hdCh0aGlzLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydCk7XG59O1xuXG5CdWZmZXIucHJvdG90eXBlLnJlYWRGbG9hdEJFID0gZnVuY3Rpb24ob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gcmVhZEZsb2F0KHRoaXMsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpO1xufTtcblxuZnVuY3Rpb24gcmVhZERvdWJsZShidWZmZXIsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQub2sodHlwZW9mIChpc0JpZ0VuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyA3IDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byByZWFkIGJleW9uZCBidWZmZXIgbGVuZ3RoJyk7XG4gIH1cblxuICByZXR1cm4gcmVxdWlyZSgnLi9idWZmZXJfaWVlZTc1NCcpLnJlYWRJRUVFNzU0KGJ1ZmZlciwgb2Zmc2V0LCBpc0JpZ0VuZGlhbixcbiAgICAgIDUyLCA4KTtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS5yZWFkRG91YmxlTEUgPSBmdW5jdGlvbihvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHJldHVybiByZWFkRG91YmxlKHRoaXMsIG9mZnNldCwgZmFsc2UsIG5vQXNzZXJ0KTtcbn07XG5cbkJ1ZmZlci5wcm90b3R5cGUucmVhZERvdWJsZUJFID0gZnVuY3Rpb24ob2Zmc2V0LCBub0Fzc2VydCkge1xuICByZXR1cm4gcmVhZERvdWJsZSh0aGlzLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cblxuLypcbiAqIFdlIGhhdmUgdG8gbWFrZSBzdXJlIHRoYXQgdGhlIHZhbHVlIGlzIGEgdmFsaWQgaW50ZWdlci4gVGhpcyBtZWFucyB0aGF0IGl0IGlzXG4gKiBub24tbmVnYXRpdmUuIEl0IGhhcyBubyBmcmFjdGlvbmFsIGNvbXBvbmVudCBhbmQgdGhhdCBpdCBkb2VzIG5vdCBleGNlZWQgdGhlXG4gKiBtYXhpbXVtIGFsbG93ZWQgdmFsdWUuXG4gKlxuICogICAgICB2YWx1ZSAgICAgICAgICAgVGhlIG51bWJlciB0byBjaGVjayBmb3IgdmFsaWRpdHlcbiAqXG4gKiAgICAgIG1heCAgICAgICAgICAgICBUaGUgbWF4aW11bSB2YWx1ZVxuICovXG5mdW5jdGlvbiB2ZXJpZnVpbnQodmFsdWUsIG1heCkge1xuICBhc3NlcnQub2sodHlwZW9mICh2YWx1ZSkgPT0gJ251bWJlcicsXG4gICAgICAnY2Fubm90IHdyaXRlIGEgbm9uLW51bWJlciBhcyBhIG51bWJlcicpO1xuXG4gIGFzc2VydC5vayh2YWx1ZSA+PSAwLFxuICAgICAgJ3NwZWNpZmllZCBhIG5lZ2F0aXZlIHZhbHVlIGZvciB3cml0aW5nIGFuIHVuc2lnbmVkIHZhbHVlJyk7XG5cbiAgYXNzZXJ0Lm9rKHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGlzIGxhcmdlciB0aGFuIG1heGltdW0gdmFsdWUgZm9yIHR5cGUnKTtcblxuICBhc3NlcnQub2soTWF0aC5mbG9vcih2YWx1ZSkgPT09IHZhbHVlLCAndmFsdWUgaGFzIGEgZnJhY3Rpb25hbCBjb21wb25lbnQnKTtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZVVJbnQ4ID0gZnVuY3Rpb24odmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgdmFyIGJ1ZmZlciA9IHRoaXM7XG5cbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyB2YWx1ZScpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3Npbmcgb2Zmc2V0Jyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0IDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmKTtcbiAgfVxuXG4gIGlmIChvZmZzZXQgPCBidWZmZXIubGVuZ3RoKSB7XG4gICAgYnVmZmVyW29mZnNldF0gPSB2YWx1ZTtcbiAgfVxufTtcblxuZnVuY3Rpb24gd3JpdGVVSW50MTYoYnVmZmVyLCB2YWx1ZSwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyB2YWx1ZScpO1xuXG4gICAgYXNzZXJ0Lm9rKHR5cGVvZiAoaXNCaWdFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyAxIDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmZmYpO1xuICB9XG5cbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBNYXRoLm1pbihidWZmZXIubGVuZ3RoIC0gb2Zmc2V0LCAyKTsgaSsrKSB7XG4gICAgYnVmZmVyW29mZnNldCArIGldID1cbiAgICAgICAgKHZhbHVlICYgKDB4ZmYgPDwgKDggKiAoaXNCaWdFbmRpYW4gPyAxIC0gaSA6IGkpKSkpID4+PlxuICAgICAgICAgICAgKGlzQmlnRW5kaWFuID8gMSAtIGkgOiBpKSAqIDg7XG4gIH1cblxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlVUludDE2TEUgPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB3cml0ZVVJbnQxNih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZVVJbnQxNkJFID0gZnVuY3Rpb24odmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgd3JpdGVVSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpO1xufTtcblxuZnVuY3Rpb24gd3JpdGVVSW50MzIoYnVmZmVyLCB2YWx1ZSwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyB2YWx1ZScpO1xuXG4gICAgYXNzZXJ0Lm9rKHR5cGVvZiAoaXNCaWdFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyAzIDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ3RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuXG4gICAgdmVyaWZ1aW50KHZhbHVlLCAweGZmZmZmZmZmKTtcbiAgfVxuXG4gIGZvciAodmFyIGkgPSAwOyBpIDwgTWF0aC5taW4oYnVmZmVyLmxlbmd0aCAtIG9mZnNldCwgNCk7IGkrKykge1xuICAgIGJ1ZmZlcltvZmZzZXQgKyBpXSA9XG4gICAgICAgICh2YWx1ZSA+Pj4gKGlzQmlnRW5kaWFuID8gMyAtIGkgOiBpKSAqIDgpICYgMHhmZjtcbiAgfVxufVxuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlVUludDMyTEUgPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB3cml0ZVVJbnQzMih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZVVJbnQzMkJFID0gZnVuY3Rpb24odmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgd3JpdGVVSW50MzIodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpO1xufTtcblxuXG4vKlxuICogV2Ugbm93IG1vdmUgb250byBvdXIgZnJpZW5kcyBpbiB0aGUgc2lnbmVkIG51bWJlciBjYXRlZ29yeS4gVW5saWtlIHVuc2lnbmVkXG4gKiBudW1iZXJzLCB3ZSdyZSBnb2luZyB0byBoYXZlIHRvIHdvcnJ5IGEgYml0IG1vcmUgYWJvdXQgaG93IHdlIHB1dCB2YWx1ZXMgaW50b1xuICogYXJyYXlzLiBTaW5jZSB3ZSBhcmUgb25seSB3b3JyeWluZyBhYm91dCBzaWduZWQgMzItYml0IHZhbHVlcywgd2UncmUgaW5cbiAqIHNsaWdodGx5IGJldHRlciBzaGFwZS4gVW5mb3J0dW5hdGVseSwgd2UgcmVhbGx5IGNhbid0IGRvIG91ciBmYXZvcml0ZSBiaW5hcnlcbiAqICYgaW4gdGhpcyBzeXN0ZW0uIEl0IHJlYWxseSBzZWVtcyB0byBkbyB0aGUgd3JvbmcgdGhpbmcuIEZvciBleGFtcGxlOlxuICpcbiAqID4gLTMyICYgMHhmZlxuICogMjI0XG4gKlxuICogV2hhdCdzIGhhcHBlbmluZyBhYm92ZSBpcyByZWFsbHk6IDB4ZTAgJiAweGZmID0gMHhlMC4gSG93ZXZlciwgdGhlIHJlc3VsdHMgb2ZcbiAqIHRoaXMgYXJlbid0IHRyZWF0ZWQgYXMgYSBzaWduZWQgbnVtYmVyLiBVbHRpbWF0ZWx5IGEgYmFkIHRoaW5nLlxuICpcbiAqIFdoYXQgd2UncmUgZ29pbmcgdG8gd2FudCB0byBkbyBpcyBiYXNpY2FsbHkgY3JlYXRlIHRoZSB1bnNpZ25lZCBlcXVpdmFsZW50IG9mXG4gKiBvdXIgcmVwcmVzZW50YXRpb24gYW5kIHBhc3MgdGhhdCBvZmYgdG8gdGhlIHd1aW50KiBmdW5jdGlvbnMuIFRvIGRvIHRoYXRcbiAqIHdlJ3JlIGdvaW5nIHRvIGRvIHRoZSBmb2xsb3dpbmc6XG4gKlxuICogIC0gaWYgdGhlIHZhbHVlIGlzIHBvc2l0aXZlXG4gKiAgICAgIHdlIGNhbiBwYXNzIGl0IGRpcmVjdGx5IG9mZiB0byB0aGUgZXF1aXZhbGVudCB3dWludFxuICogIC0gaWYgdGhlIHZhbHVlIGlzIG5lZ2F0aXZlXG4gKiAgICAgIHdlIGRvIHRoZSBmb2xsb3dpbmcgY29tcHV0YXRpb246XG4gKiAgICAgICAgIG1iICsgdmFsICsgMSwgd2hlcmVcbiAqICAgICAgICAgbWIgICBpcyB0aGUgbWF4aW11bSB1bnNpZ25lZCB2YWx1ZSBpbiB0aGF0IGJ5dGUgc2l6ZVxuICogICAgICAgICB2YWwgIGlzIHRoZSBKYXZhc2NyaXB0IG5lZ2F0aXZlIGludGVnZXJcbiAqXG4gKlxuICogQXMgYSBjb25jcmV0ZSB2YWx1ZSwgdGFrZSAtMTI4LiBJbiBzaWduZWQgMTYgYml0cyB0aGlzIHdvdWxkIGJlIDB4ZmY4MC4gSWZcbiAqIHlvdSBkbyBvdXQgdGhlIGNvbXB1dGF0aW9uczpcbiAqXG4gKiAweGZmZmYgLSAxMjggKyAxXG4gKiAweGZmZmYgLSAxMjdcbiAqIDB4ZmY4MFxuICpcbiAqIFlvdSBjYW4gdGhlbiBlbmNvZGUgdGhpcyB2YWx1ZSBhcyB0aGUgc2lnbmVkIHZlcnNpb24uIFRoaXMgaXMgcmVhbGx5IHJhdGhlclxuICogaGFja3ksIGJ1dCBpdCBzaG91bGQgd29yayBhbmQgZ2V0IHRoZSBqb2IgZG9uZSB3aGljaCBpcyBvdXIgZ29hbCBoZXJlLlxuICovXG5cbi8qXG4gKiBBIHNlcmllcyBvZiBjaGVja3MgdG8gbWFrZSBzdXJlIHdlIGFjdHVhbGx5IGhhdmUgYSBzaWduZWQgMzItYml0IG51bWJlclxuICovXG5mdW5jdGlvbiB2ZXJpZnNpbnQodmFsdWUsIG1heCwgbWluKSB7XG4gIGFzc2VydC5vayh0eXBlb2YgKHZhbHVlKSA9PSAnbnVtYmVyJyxcbiAgICAgICdjYW5ub3Qgd3JpdGUgYSBub24tbnVtYmVyIGFzIGEgbnVtYmVyJyk7XG5cbiAgYXNzZXJ0Lm9rKHZhbHVlIDw9IG1heCwgJ3ZhbHVlIGxhcmdlciB0aGFuIG1heGltdW0gYWxsb3dlZCB2YWx1ZScpO1xuXG4gIGFzc2VydC5vayh2YWx1ZSA+PSBtaW4sICd2YWx1ZSBzbWFsbGVyIHRoYW4gbWluaW11bSBhbGxvd2VkIHZhbHVlJyk7XG5cbiAgYXNzZXJ0Lm9rKE1hdGguZmxvb3IodmFsdWUpID09PSB2YWx1ZSwgJ3ZhbHVlIGhhcyBhIGZyYWN0aW9uYWwgY29tcG9uZW50Jyk7XG59XG5cbmZ1bmN0aW9uIHZlcmlmSUVFRTc1NCh2YWx1ZSwgbWF4LCBtaW4pIHtcbiAgYXNzZXJ0Lm9rKHR5cGVvZiAodmFsdWUpID09ICdudW1iZXInLFxuICAgICAgJ2Nhbm5vdCB3cml0ZSBhIG5vbi1udW1iZXIgYXMgYSBudW1iZXInKTtcblxuICBhc3NlcnQub2sodmFsdWUgPD0gbWF4LCAndmFsdWUgbGFyZ2VyIHRoYW4gbWF4aW11bSBhbGxvd2VkIHZhbHVlJyk7XG5cbiAgYXNzZXJ0Lm9rKHZhbHVlID49IG1pbiwgJ3ZhbHVlIHNtYWxsZXIgdGhhbiBtaW5pbXVtIGFsbG93ZWQgdmFsdWUnKTtcbn1cblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZUludDggPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB2YXIgYnVmZmVyID0gdGhpcztcblxuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0Lm9rKHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIHZhbHVlJyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgPCBidWZmZXIubGVuZ3RoLFxuICAgICAgICAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJyk7XG5cbiAgICB2ZXJpZnNpbnQodmFsdWUsIDB4N2YsIC0weDgwKTtcbiAgfVxuXG4gIGlmICh2YWx1ZSA+PSAwKSB7XG4gICAgYnVmZmVyLndyaXRlVUludDgodmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpO1xuICB9IGVsc2Uge1xuICAgIGJ1ZmZlci53cml0ZVVJbnQ4KDB4ZmYgKyB2YWx1ZSArIDEsIG9mZnNldCwgbm9Bc3NlcnQpO1xuICB9XG59O1xuXG5mdW5jdGlvbiB3cml0ZUludDE2KGJ1ZmZlciwgdmFsdWUsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQub2sodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3NpbmcgdmFsdWUnKTtcblxuICAgIGFzc2VydC5vayh0eXBlb2YgKGlzQmlnRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3Npbmcgb2Zmc2V0Jyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICsgMSA8IGJ1ZmZlci5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKTtcblxuICAgIHZlcmlmc2ludCh2YWx1ZSwgMHg3ZmZmLCAtMHg4MDAwKTtcbiAgfVxuXG4gIGlmICh2YWx1ZSA+PSAwKSB7XG4gICAgd3JpdGVVSW50MTYoYnVmZmVyLCB2YWx1ZSwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpO1xuICB9IGVsc2Uge1xuICAgIHdyaXRlVUludDE2KGJ1ZmZlciwgMHhmZmZmICsgdmFsdWUgKyAxLCBvZmZzZXQsIGlzQmlnRW5kaWFuLCBub0Fzc2VydCk7XG4gIH1cbn1cblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZUludDE2TEUgPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB3cml0ZUludDE2KHRoaXMsIHZhbHVlLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydCk7XG59O1xuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlSW50MTZCRSA9IGZ1bmN0aW9uKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHdyaXRlSW50MTYodGhpcywgdmFsdWUsIG9mZnNldCwgdHJ1ZSwgbm9Bc3NlcnQpO1xufTtcblxuZnVuY3Rpb24gd3JpdGVJbnQzMihidWZmZXIsIHZhbHVlLCBvZmZzZXQsIGlzQmlnRW5kaWFuLCBub0Fzc2VydCkge1xuICBpZiAoIW5vQXNzZXJ0KSB7XG4gICAgYXNzZXJ0Lm9rKHZhbHVlICE9PSB1bmRlZmluZWQgJiYgdmFsdWUgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIHZhbHVlJyk7XG5cbiAgICBhc3NlcnQub2sodHlwZW9mIChpc0JpZ0VuZGlhbikgPT09ICdib29sZWFuJyxcbiAgICAgICAgJ21pc3Npbmcgb3IgaW52YWxpZCBlbmRpYW4nKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgIT09IHVuZGVmaW5lZCAmJiBvZmZzZXQgIT09IG51bGwsXG4gICAgICAgICdtaXNzaW5nIG9mZnNldCcpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCArIDMgPCBidWZmZXIubGVuZ3RoLFxuICAgICAgICAnVHJ5aW5nIHRvIHdyaXRlIGJleW9uZCBidWZmZXIgbGVuZ3RoJyk7XG5cbiAgICB2ZXJpZnNpbnQodmFsdWUsIDB4N2ZmZmZmZmYsIC0weDgwMDAwMDAwKTtcbiAgfVxuXG4gIGlmICh2YWx1ZSA+PSAwKSB7XG4gICAgd3JpdGVVSW50MzIoYnVmZmVyLCB2YWx1ZSwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpO1xuICB9IGVsc2Uge1xuICAgIHdyaXRlVUludDMyKGJ1ZmZlciwgMHhmZmZmZmZmZiArIHZhbHVlICsgMSwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpO1xuICB9XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVJbnQzMkxFID0gZnVuY3Rpb24odmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgd3JpdGVJbnQzMih0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZUludDMyQkUgPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB3cml0ZUludDMyKHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cbmZ1bmN0aW9uIHdyaXRlRmxvYXQoYnVmZmVyLCB2YWx1ZSwgb2Zmc2V0LCBpc0JpZ0VuZGlhbiwgbm9Bc3NlcnQpIHtcbiAgaWYgKCFub0Fzc2VydCkge1xuICAgIGFzc2VydC5vayh2YWx1ZSAhPT0gdW5kZWZpbmVkICYmIHZhbHVlICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyB2YWx1ZScpO1xuXG4gICAgYXNzZXJ0Lm9rKHR5cGVvZiAoaXNCaWdFbmRpYW4pID09PSAnYm9vbGVhbicsXG4gICAgICAgICdtaXNzaW5nIG9yIGludmFsaWQgZW5kaWFuJyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICE9PSB1bmRlZmluZWQgJiYgb2Zmc2V0ICE9PSBudWxsLFxuICAgICAgICAnbWlzc2luZyBvZmZzZXQnKTtcblxuICAgIGFzc2VydC5vayhvZmZzZXQgKyAzIDwgYnVmZmVyLmxlbmd0aCxcbiAgICAgICAgJ1RyeWluZyB0byB3cml0ZSBiZXlvbmQgYnVmZmVyIGxlbmd0aCcpO1xuXG4gICAgdmVyaWZJRUVFNzU0KHZhbHVlLCAzLjQwMjgyMzQ2NjM4NTI4ODZlKzM4LCAtMy40MDI4MjM0NjYzODUyODg2ZSszOCk7XG4gIH1cblxuICByZXF1aXJlKCcuL2J1ZmZlcl9pZWVlNzU0Jykud3JpdGVJRUVFNzU0KGJ1ZmZlciwgdmFsdWUsIG9mZnNldCwgaXNCaWdFbmRpYW4sXG4gICAgICAyMywgNCk7XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVGbG9hdExFID0gZnVuY3Rpb24odmFsdWUsIG9mZnNldCwgbm9Bc3NlcnQpIHtcbiAgd3JpdGVGbG9hdCh0aGlzLCB2YWx1ZSwgb2Zmc2V0LCBmYWxzZSwgbm9Bc3NlcnQpO1xufTtcblxuQnVmZmVyLnByb3RvdHlwZS53cml0ZUZsb2F0QkUgPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB3cml0ZUZsb2F0KHRoaXMsIHZhbHVlLCBvZmZzZXQsIHRydWUsIG5vQXNzZXJ0KTtcbn07XG5cbmZ1bmN0aW9uIHdyaXRlRG91YmxlKGJ1ZmZlciwgdmFsdWUsIG9mZnNldCwgaXNCaWdFbmRpYW4sIG5vQXNzZXJ0KSB7XG4gIGlmICghbm9Bc3NlcnQpIHtcbiAgICBhc3NlcnQub2sodmFsdWUgIT09IHVuZGVmaW5lZCAmJiB2YWx1ZSAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3NpbmcgdmFsdWUnKTtcblxuICAgIGFzc2VydC5vayh0eXBlb2YgKGlzQmlnRW5kaWFuKSA9PT0gJ2Jvb2xlYW4nLFxuICAgICAgICAnbWlzc2luZyBvciBpbnZhbGlkIGVuZGlhbicpO1xuXG4gICAgYXNzZXJ0Lm9rKG9mZnNldCAhPT0gdW5kZWZpbmVkICYmIG9mZnNldCAhPT0gbnVsbCxcbiAgICAgICAgJ21pc3Npbmcgb2Zmc2V0Jyk7XG5cbiAgICBhc3NlcnQub2sob2Zmc2V0ICsgNyA8IGJ1ZmZlci5sZW5ndGgsXG4gICAgICAgICdUcnlpbmcgdG8gd3JpdGUgYmV5b25kIGJ1ZmZlciBsZW5ndGgnKTtcblxuICAgIHZlcmlmSUVFRTc1NCh2YWx1ZSwgMS43OTc2OTMxMzQ4NjIzMTU3RSszMDgsIC0xLjc5NzY5MzEzNDg2MjMxNTdFKzMwOCk7XG4gIH1cblxuICByZXF1aXJlKCcuL2J1ZmZlcl9pZWVlNzU0Jykud3JpdGVJRUVFNzU0KGJ1ZmZlciwgdmFsdWUsIG9mZnNldCwgaXNCaWdFbmRpYW4sXG4gICAgICA1MiwgOCk7XG59XG5cbkJ1ZmZlci5wcm90b3R5cGUud3JpdGVEb3VibGVMRSA9IGZ1bmN0aW9uKHZhbHVlLCBvZmZzZXQsIG5vQXNzZXJ0KSB7XG4gIHdyaXRlRG91YmxlKHRoaXMsIHZhbHVlLCBvZmZzZXQsIGZhbHNlLCBub0Fzc2VydCk7XG59O1xuXG5CdWZmZXIucHJvdG90eXBlLndyaXRlRG91YmxlQkUgPSBmdW5jdGlvbih2YWx1ZSwgb2Zmc2V0LCBub0Fzc2VydCkge1xuICB3cml0ZURvdWJsZSh0aGlzLCB2YWx1ZSwgb2Zmc2V0LCB0cnVlLCBub0Fzc2VydCk7XG59O1xuIiwiKGZ1bmN0aW9uIChleHBvcnRzKSB7XG5cdCd1c2Ugc3RyaWN0JztcblxuXHR2YXIgbG9va3VwID0gJ0FCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaYWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5Ky8nO1xuXG5cdGZ1bmN0aW9uIGI2NFRvQnl0ZUFycmF5KGI2NCkge1xuXHRcdHZhciBpLCBqLCBsLCB0bXAsIHBsYWNlSG9sZGVycywgYXJyO1xuXHRcblx0XHRpZiAoYjY0Lmxlbmd0aCAlIDQgPiAwKSB7XG5cdFx0XHR0aHJvdyAnSW52YWxpZCBzdHJpbmcuIExlbmd0aCBtdXN0IGJlIGEgbXVsdGlwbGUgb2YgNCc7XG5cdFx0fVxuXG5cdFx0Ly8gdGhlIG51bWJlciBvZiBlcXVhbCBzaWducyAocGxhY2UgaG9sZGVycylcblx0XHQvLyBpZiB0aGVyZSBhcmUgdHdvIHBsYWNlaG9sZGVycywgdGhhbiB0aGUgdHdvIGNoYXJhY3RlcnMgYmVmb3JlIGl0XG5cdFx0Ly8gcmVwcmVzZW50IG9uZSBieXRlXG5cdFx0Ly8gaWYgdGhlcmUgaXMgb25seSBvbmUsIHRoZW4gdGhlIHRocmVlIGNoYXJhY3RlcnMgYmVmb3JlIGl0IHJlcHJlc2VudCAyIGJ5dGVzXG5cdFx0Ly8gdGhpcyBpcyBqdXN0IGEgY2hlYXAgaGFjayB0byBub3QgZG8gaW5kZXhPZiB0d2ljZVxuXHRcdHBsYWNlSG9sZGVycyA9IGI2NC5pbmRleE9mKCc9Jyk7XG5cdFx0cGxhY2VIb2xkZXJzID0gcGxhY2VIb2xkZXJzID4gMCA/IGI2NC5sZW5ndGggLSBwbGFjZUhvbGRlcnMgOiAwO1xuXG5cdFx0Ly8gYmFzZTY0IGlzIDQvMyArIHVwIHRvIHR3byBjaGFyYWN0ZXJzIG9mIHRoZSBvcmlnaW5hbCBkYXRhXG5cdFx0YXJyID0gW107Ly9uZXcgVWludDhBcnJheShiNjQubGVuZ3RoICogMyAvIDQgLSBwbGFjZUhvbGRlcnMpO1xuXG5cdFx0Ly8gaWYgdGhlcmUgYXJlIHBsYWNlaG9sZGVycywgb25seSBnZXQgdXAgdG8gdGhlIGxhc3QgY29tcGxldGUgNCBjaGFyc1xuXHRcdGwgPSBwbGFjZUhvbGRlcnMgPiAwID8gYjY0Lmxlbmd0aCAtIDQgOiBiNjQubGVuZ3RoO1xuXG5cdFx0Zm9yIChpID0gMCwgaiA9IDA7IGkgPCBsOyBpICs9IDQsIGogKz0gMykge1xuXHRcdFx0dG1wID0gKGxvb2t1cC5pbmRleE9mKGI2NFtpXSkgPDwgMTgpIHwgKGxvb2t1cC5pbmRleE9mKGI2NFtpICsgMV0pIDw8IDEyKSB8IChsb29rdXAuaW5kZXhPZihiNjRbaSArIDJdKSA8PCA2KSB8IGxvb2t1cC5pbmRleE9mKGI2NFtpICsgM10pO1xuXHRcdFx0YXJyLnB1c2goKHRtcCAmIDB4RkYwMDAwKSA+PiAxNik7XG5cdFx0XHRhcnIucHVzaCgodG1wICYgMHhGRjAwKSA+PiA4KTtcblx0XHRcdGFyci5wdXNoKHRtcCAmIDB4RkYpO1xuXHRcdH1cblxuXHRcdGlmIChwbGFjZUhvbGRlcnMgPT09IDIpIHtcblx0XHRcdHRtcCA9IChsb29rdXAuaW5kZXhPZihiNjRbaV0pIDw8IDIpIHwgKGxvb2t1cC5pbmRleE9mKGI2NFtpICsgMV0pID4+IDQpO1xuXHRcdFx0YXJyLnB1c2godG1wICYgMHhGRik7XG5cdFx0fSBlbHNlIGlmIChwbGFjZUhvbGRlcnMgPT09IDEpIHtcblx0XHRcdHRtcCA9IChsb29rdXAuaW5kZXhPZihiNjRbaV0pIDw8IDEwKSB8IChsb29rdXAuaW5kZXhPZihiNjRbaSArIDFdKSA8PCA0KSB8IChsb29rdXAuaW5kZXhPZihiNjRbaSArIDJdKSA+PiAyKTtcblx0XHRcdGFyci5wdXNoKCh0bXAgPj4gOCkgJiAweEZGKTtcblx0XHRcdGFyci5wdXNoKHRtcCAmIDB4RkYpO1xuXHRcdH1cblxuXHRcdHJldHVybiBhcnI7XG5cdH1cblxuXHRmdW5jdGlvbiB1aW50OFRvQmFzZTY0KHVpbnQ4KSB7XG5cdFx0dmFyIGksXG5cdFx0XHRleHRyYUJ5dGVzID0gdWludDgubGVuZ3RoICUgMywgLy8gaWYgd2UgaGF2ZSAxIGJ5dGUgbGVmdCwgcGFkIDIgYnl0ZXNcblx0XHRcdG91dHB1dCA9IFwiXCIsXG5cdFx0XHR0ZW1wLCBsZW5ndGg7XG5cblx0XHRmdW5jdGlvbiB0cmlwbGV0VG9CYXNlNjQgKG51bSkge1xuXHRcdFx0cmV0dXJuIGxvb2t1cFtudW0gPj4gMTggJiAweDNGXSArIGxvb2t1cFtudW0gPj4gMTIgJiAweDNGXSArIGxvb2t1cFtudW0gPj4gNiAmIDB4M0ZdICsgbG9va3VwW251bSAmIDB4M0ZdO1xuXHRcdH07XG5cblx0XHQvLyBnbyB0aHJvdWdoIHRoZSBhcnJheSBldmVyeSB0aHJlZSBieXRlcywgd2UnbGwgZGVhbCB3aXRoIHRyYWlsaW5nIHN0dWZmIGxhdGVyXG5cdFx0Zm9yIChpID0gMCwgbGVuZ3RoID0gdWludDgubGVuZ3RoIC0gZXh0cmFCeXRlczsgaSA8IGxlbmd0aDsgaSArPSAzKSB7XG5cdFx0XHR0ZW1wID0gKHVpbnQ4W2ldIDw8IDE2KSArICh1aW50OFtpICsgMV0gPDwgOCkgKyAodWludDhbaSArIDJdKTtcblx0XHRcdG91dHB1dCArPSB0cmlwbGV0VG9CYXNlNjQodGVtcCk7XG5cdFx0fVxuXG5cdFx0Ly8gcGFkIHRoZSBlbmQgd2l0aCB6ZXJvcywgYnV0IG1ha2Ugc3VyZSB0byBub3QgZm9yZ2V0IHRoZSBleHRyYSBieXRlc1xuXHRcdHN3aXRjaCAoZXh0cmFCeXRlcykge1xuXHRcdFx0Y2FzZSAxOlxuXHRcdFx0XHR0ZW1wID0gdWludDhbdWludDgubGVuZ3RoIC0gMV07XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXBbdGVtcCA+PiAyXTtcblx0XHRcdFx0b3V0cHV0ICs9IGxvb2t1cFsodGVtcCA8PCA0KSAmIDB4M0ZdO1xuXHRcdFx0XHRvdXRwdXQgKz0gJz09Jztcblx0XHRcdFx0YnJlYWs7XG5cdFx0XHRjYXNlIDI6XG5cdFx0XHRcdHRlbXAgPSAodWludDhbdWludDgubGVuZ3RoIC0gMl0gPDwgOCkgKyAodWludDhbdWludDgubGVuZ3RoIC0gMV0pO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwW3RlbXAgPj4gMTBdO1xuXHRcdFx0XHRvdXRwdXQgKz0gbG9va3VwWyh0ZW1wID4+IDQpICYgMHgzRl07XG5cdFx0XHRcdG91dHB1dCArPSBsb29rdXBbKHRlbXAgPDwgMikgJiAweDNGXTtcblx0XHRcdFx0b3V0cHV0ICs9ICc9Jztcblx0XHRcdFx0YnJlYWs7XG5cdFx0fVxuXG5cdFx0cmV0dXJuIG91dHB1dDtcblx0fVxuXG5cdG1vZHVsZS5leHBvcnRzLnRvQnl0ZUFycmF5ID0gYjY0VG9CeXRlQXJyYXk7XG5cdG1vZHVsZS5leHBvcnRzLmZyb21CeXRlQXJyYXkgPSB1aW50OFRvQmFzZTY0O1xufSgpKTtcbiIsInZhciB1bmRlZmluZWQgPSAodm9pZCAwKTsgLy8gUGFyYW5vaWFcblxuLy8gQmV5b25kIHRoaXMgdmFsdWUsIGluZGV4IGdldHRlcnMvc2V0dGVycyAoaS5lLiBhcnJheVswXSwgYXJyYXlbMV0pIGFyZSBzbyBzbG93IHRvXG4vLyBjcmVhdGUsIGFuZCBjb25zdW1lIHNvIG11Y2ggbWVtb3J5LCB0aGF0IHRoZSBicm93c2VyIGFwcGVhcnMgZnJvemVuLlxudmFyIE1BWF9BUlJBWV9MRU5HVEggPSAxZTU7XG5cbi8vIEFwcHJveGltYXRpb25zIG9mIGludGVybmFsIEVDTUFTY3JpcHQgY29udmVyc2lvbiBmdW5jdGlvbnNcbnZhciBFQ01BU2NyaXB0ID0gKGZ1bmN0aW9uKCkge1xuICAvLyBTdGFzaCBhIGNvcHkgaW4gY2FzZSBvdGhlciBzY3JpcHRzIG1vZGlmeSB0aGVzZVxuICB2YXIgb3B0cyA9IE9iamVjdC5wcm90b3R5cGUudG9TdHJpbmcsXG4gICAgICBvcGhvcCA9IE9iamVjdC5wcm90b3R5cGUuaGFzT3duUHJvcGVydHk7XG5cbiAgcmV0dXJuIHtcbiAgICAvLyBDbGFzcyByZXR1cm5zIGludGVybmFsIFtbQ2xhc3NdXSBwcm9wZXJ0eSwgdXNlZCB0byBhdm9pZCBjcm9zcy1mcmFtZSBpbnN0YW5jZW9mIGlzc3VlczpcbiAgICBDbGFzczogZnVuY3Rpb24odikgeyByZXR1cm4gb3B0cy5jYWxsKHYpLnJlcGxhY2UoL15cXFtvYmplY3QgKnxcXF0kL2csICcnKTsgfSxcbiAgICBIYXNQcm9wZXJ0eTogZnVuY3Rpb24obywgcCkgeyByZXR1cm4gcCBpbiBvOyB9LFxuICAgIEhhc093blByb3BlcnR5OiBmdW5jdGlvbihvLCBwKSB7IHJldHVybiBvcGhvcC5jYWxsKG8sIHApOyB9LFxuICAgIElzQ2FsbGFibGU6IGZ1bmN0aW9uKG8pIHsgcmV0dXJuIHR5cGVvZiBvID09PSAnZnVuY3Rpb24nOyB9LFxuICAgIFRvSW50MzI6IGZ1bmN0aW9uKHYpIHsgcmV0dXJuIHYgPj4gMDsgfSxcbiAgICBUb1VpbnQzMjogZnVuY3Rpb24odikgeyByZXR1cm4gdiA+Pj4gMDsgfVxuICB9O1xufSgpKTtcblxuLy8gU25hcHNob3QgaW50cmluc2ljc1xudmFyIExOMiA9IE1hdGguTE4yLFxuICAgIGFicyA9IE1hdGguYWJzLFxuICAgIGZsb29yID0gTWF0aC5mbG9vcixcbiAgICBsb2cgPSBNYXRoLmxvZyxcbiAgICBtaW4gPSBNYXRoLm1pbixcbiAgICBwb3cgPSBNYXRoLnBvdyxcbiAgICByb3VuZCA9IE1hdGgucm91bmQ7XG5cbi8vIEVTNTogbG9jayBkb3duIG9iamVjdCBwcm9wZXJ0aWVzXG5mdW5jdGlvbiBjb25maWd1cmVQcm9wZXJ0aWVzKG9iaikge1xuICBpZiAoZ2V0T3duUHJvcE5hbWVzICYmIGRlZmluZVByb3ApIHtcbiAgICB2YXIgcHJvcHMgPSBnZXRPd25Qcm9wTmFtZXMob2JqKSwgaTtcbiAgICBmb3IgKGkgPSAwOyBpIDwgcHJvcHMubGVuZ3RoOyBpICs9IDEpIHtcbiAgICAgIGRlZmluZVByb3Aob2JqLCBwcm9wc1tpXSwge1xuICAgICAgICB2YWx1ZTogb2JqW3Byb3BzW2ldXSxcbiAgICAgICAgd3JpdGFibGU6IGZhbHNlLFxuICAgICAgICBlbnVtZXJhYmxlOiBmYWxzZSxcbiAgICAgICAgY29uZmlndXJhYmxlOiBmYWxzZVxuICAgICAgfSk7XG4gICAgfVxuICB9XG59XG5cbi8vIGVtdWxhdGUgRVM1IGdldHRlci9zZXR0ZXIgQVBJIHVzaW5nIGxlZ2FjeSBBUElzXG4vLyBodHRwOi8vYmxvZ3MubXNkbi5jb20vYi9pZS9hcmNoaXZlLzIwMTAvMDkvMDcvdHJhbnNpdGlvbmluZy1leGlzdGluZy1jb2RlLXRvLXRoZS1lczUtZ2V0dGVyLXNldHRlci1hcGlzLmFzcHhcbi8vIChzZWNvbmQgY2xhdXNlIHRlc3RzIGZvciBPYmplY3QuZGVmaW5lUHJvcGVydHkoKSBpbiBJRTw5IHRoYXQgb25seSBzdXBwb3J0cyBleHRlbmRpbmcgRE9NIHByb3RvdHlwZXMsIGJ1dFxuLy8gbm90ZSB0aGF0IElFPDkgZG9lcyBub3Qgc3VwcG9ydCBfX2RlZmluZUdldHRlcl9fIG9yIF9fZGVmaW5lU2V0dGVyX18gc28gaXQganVzdCByZW5kZXJzIHRoZSBtZXRob2QgaGFybWxlc3MpXG52YXIgZGVmaW5lUHJvcFxuaWYgKE9iamVjdC5kZWZpbmVQcm9wZXJ0eSAmJiAoZnVuY3Rpb24oKSB7XG4gICAgICB0cnkge1xuICAgICAgICBPYmplY3QuZGVmaW5lUHJvcGVydHkoe30sICd4Jywge30pO1xuICAgICAgICByZXR1cm4gdHJ1ZTtcbiAgICAgIH0gY2F0Y2ggKGUpIHtcbiAgICAgICAgcmV0dXJuIGZhbHNlO1xuICAgICAgfVxuICAgIH0pKCkpIHtcbiAgZGVmaW5lUHJvcCA9IE9iamVjdC5kZWZpbmVQcm9wZXJ0eTtcbn0gZWxzZSB7XG4gIGRlZmluZVByb3AgPSBmdW5jdGlvbihvLCBwLCBkZXNjKSB7XG4gICAgaWYgKCFvID09PSBPYmplY3QobykpIHRocm93IG5ldyBUeXBlRXJyb3IoXCJPYmplY3QuZGVmaW5lUHJvcGVydHkgY2FsbGVkIG9uIG5vbi1vYmplY3RcIik7XG4gICAgaWYgKEVDTUFTY3JpcHQuSGFzUHJvcGVydHkoZGVzYywgJ2dldCcpICYmIE9iamVjdC5wcm90b3R5cGUuX19kZWZpbmVHZXR0ZXJfXykgeyBPYmplY3QucHJvdG90eXBlLl9fZGVmaW5lR2V0dGVyX18uY2FsbChvLCBwLCBkZXNjLmdldCk7IH1cbiAgICBpZiAoRUNNQVNjcmlwdC5IYXNQcm9wZXJ0eShkZXNjLCAnc2V0JykgJiYgT2JqZWN0LnByb3RvdHlwZS5fX2RlZmluZVNldHRlcl9fKSB7IE9iamVjdC5wcm90b3R5cGUuX19kZWZpbmVTZXR0ZXJfXy5jYWxsKG8sIHAsIGRlc2Muc2V0KTsgfVxuICAgIGlmIChFQ01BU2NyaXB0Lkhhc1Byb3BlcnR5KGRlc2MsICd2YWx1ZScpKSB7IG9bcF0gPSBkZXNjLnZhbHVlOyB9XG4gICAgcmV0dXJuIG87XG4gIH07XG59XG5cbnZhciBnZXRPd25Qcm9wTmFtZXMgPSBPYmplY3QuZ2V0T3duUHJvcGVydHlOYW1lcyB8fCBmdW5jdGlvbiAobykge1xuICBpZiAobyAhPT0gT2JqZWN0KG8pKSB0aHJvdyBuZXcgVHlwZUVycm9yKFwiT2JqZWN0LmdldE93blByb3BlcnR5TmFtZXMgY2FsbGVkIG9uIG5vbi1vYmplY3RcIik7XG4gIHZhciBwcm9wcyA9IFtdLCBwO1xuICBmb3IgKHAgaW4gbykge1xuICAgIGlmIChFQ01BU2NyaXB0Lkhhc093blByb3BlcnR5KG8sIHApKSB7XG4gICAgICBwcm9wcy5wdXNoKHApO1xuICAgIH1cbiAgfVxuICByZXR1cm4gcHJvcHM7XG59O1xuXG4vLyBFUzU6IE1ha2Ugb2JqW2luZGV4XSBhbiBhbGlhcyBmb3Igb2JqLl9nZXR0ZXIoaW5kZXgpL29iai5fc2V0dGVyKGluZGV4LCB2YWx1ZSlcbi8vIGZvciBpbmRleCBpbiAwIC4uLiBvYmoubGVuZ3RoXG5mdW5jdGlvbiBtYWtlQXJyYXlBY2Nlc3NvcnMob2JqKSB7XG4gIGlmICghZGVmaW5lUHJvcCkgeyByZXR1cm47IH1cblxuICBpZiAob2JqLmxlbmd0aCA+IE1BWF9BUlJBWV9MRU5HVEgpIHRocm93IG5ldyBSYW5nZUVycm9yKFwiQXJyYXkgdG9vIGxhcmdlIGZvciBwb2x5ZmlsbFwiKTtcblxuICBmdW5jdGlvbiBtYWtlQXJyYXlBY2Nlc3NvcihpbmRleCkge1xuICAgIGRlZmluZVByb3Aob2JqLCBpbmRleCwge1xuICAgICAgJ2dldCc6IGZ1bmN0aW9uKCkgeyByZXR1cm4gb2JqLl9nZXR0ZXIoaW5kZXgpOyB9LFxuICAgICAgJ3NldCc6IGZ1bmN0aW9uKHYpIHsgb2JqLl9zZXR0ZXIoaW5kZXgsIHYpOyB9LFxuICAgICAgZW51bWVyYWJsZTogdHJ1ZSxcbiAgICAgIGNvbmZpZ3VyYWJsZTogZmFsc2VcbiAgICB9KTtcbiAgfVxuXG4gIHZhciBpO1xuICBmb3IgKGkgPSAwOyBpIDwgb2JqLmxlbmd0aDsgaSArPSAxKSB7XG4gICAgbWFrZUFycmF5QWNjZXNzb3IoaSk7XG4gIH1cbn1cblxuLy8gSW50ZXJuYWwgY29udmVyc2lvbiBmdW5jdGlvbnM6XG4vLyAgICBwYWNrPFR5cGU+KCkgICAtIHRha2UgYSBudW1iZXIgKGludGVycHJldGVkIGFzIFR5cGUpLCBvdXRwdXQgYSBieXRlIGFycmF5XG4vLyAgICB1bnBhY2s8VHlwZT4oKSAtIHRha2UgYSBieXRlIGFycmF5LCBvdXRwdXQgYSBUeXBlLWxpa2UgbnVtYmVyXG5cbmZ1bmN0aW9uIGFzX3NpZ25lZCh2YWx1ZSwgYml0cykgeyB2YXIgcyA9IDMyIC0gYml0czsgcmV0dXJuICh2YWx1ZSA8PCBzKSA+PiBzOyB9XG5mdW5jdGlvbiBhc191bnNpZ25lZCh2YWx1ZSwgYml0cykgeyB2YXIgcyA9IDMyIC0gYml0czsgcmV0dXJuICh2YWx1ZSA8PCBzKSA+Pj4gczsgfVxuXG5mdW5jdGlvbiBwYWNrSTgobikgeyByZXR1cm4gW24gJiAweGZmXTsgfVxuZnVuY3Rpb24gdW5wYWNrSTgoYnl0ZXMpIHsgcmV0dXJuIGFzX3NpZ25lZChieXRlc1swXSwgOCk7IH1cblxuZnVuY3Rpb24gcGFja1U4KG4pIHsgcmV0dXJuIFtuICYgMHhmZl07IH1cbmZ1bmN0aW9uIHVucGFja1U4KGJ5dGVzKSB7IHJldHVybiBhc191bnNpZ25lZChieXRlc1swXSwgOCk7IH1cblxuZnVuY3Rpb24gcGFja1U4Q2xhbXBlZChuKSB7IG4gPSByb3VuZChOdW1iZXIobikpOyByZXR1cm4gW24gPCAwID8gMCA6IG4gPiAweGZmID8gMHhmZiA6IG4gJiAweGZmXTsgfVxuXG5mdW5jdGlvbiBwYWNrSTE2KG4pIHsgcmV0dXJuIFsobiA+PiA4KSAmIDB4ZmYsIG4gJiAweGZmXTsgfVxuZnVuY3Rpb24gdW5wYWNrSTE2KGJ5dGVzKSB7IHJldHVybiBhc19zaWduZWQoYnl0ZXNbMF0gPDwgOCB8IGJ5dGVzWzFdLCAxNik7IH1cblxuZnVuY3Rpb24gcGFja1UxNihuKSB7IHJldHVybiBbKG4gPj4gOCkgJiAweGZmLCBuICYgMHhmZl07IH1cbmZ1bmN0aW9uIHVucGFja1UxNihieXRlcykgeyByZXR1cm4gYXNfdW5zaWduZWQoYnl0ZXNbMF0gPDwgOCB8IGJ5dGVzWzFdLCAxNik7IH1cblxuZnVuY3Rpb24gcGFja0kzMihuKSB7IHJldHVybiBbKG4gPj4gMjQpICYgMHhmZiwgKG4gPj4gMTYpICYgMHhmZiwgKG4gPj4gOCkgJiAweGZmLCBuICYgMHhmZl07IH1cbmZ1bmN0aW9uIHVucGFja0kzMihieXRlcykgeyByZXR1cm4gYXNfc2lnbmVkKGJ5dGVzWzBdIDw8IDI0IHwgYnl0ZXNbMV0gPDwgMTYgfCBieXRlc1syXSA8PCA4IHwgYnl0ZXNbM10sIDMyKTsgfVxuXG5mdW5jdGlvbiBwYWNrVTMyKG4pIHsgcmV0dXJuIFsobiA+PiAyNCkgJiAweGZmLCAobiA+PiAxNikgJiAweGZmLCAobiA+PiA4KSAmIDB4ZmYsIG4gJiAweGZmXTsgfVxuZnVuY3Rpb24gdW5wYWNrVTMyKGJ5dGVzKSB7IHJldHVybiBhc191bnNpZ25lZChieXRlc1swXSA8PCAyNCB8IGJ5dGVzWzFdIDw8IDE2IHwgYnl0ZXNbMl0gPDwgOCB8IGJ5dGVzWzNdLCAzMik7IH1cblxuZnVuY3Rpb24gcGFja0lFRUU3NTQodiwgZWJpdHMsIGZiaXRzKSB7XG5cbiAgdmFyIGJpYXMgPSAoMSA8PCAoZWJpdHMgLSAxKSkgLSAxLFxuICAgICAgcywgZSwgZiwgbG4sXG4gICAgICBpLCBiaXRzLCBzdHIsIGJ5dGVzO1xuXG4gIGZ1bmN0aW9uIHJvdW5kVG9FdmVuKG4pIHtcbiAgICB2YXIgdyA9IGZsb29yKG4pLCBmID0gbiAtIHc7XG4gICAgaWYgKGYgPCAwLjUpXG4gICAgICByZXR1cm4gdztcbiAgICBpZiAoZiA+IDAuNSlcbiAgICAgIHJldHVybiB3ICsgMTtcbiAgICByZXR1cm4gdyAlIDIgPyB3ICsgMSA6IHc7XG4gIH1cblxuICAvLyBDb21wdXRlIHNpZ24sIGV4cG9uZW50LCBmcmFjdGlvblxuICBpZiAodiAhPT0gdikge1xuICAgIC8vIE5hTlxuICAgIC8vIGh0dHA6Ly9kZXYudzMub3JnLzIwMDYvd2ViYXBpL1dlYklETC8jZXMtdHlwZS1tYXBwaW5nXG4gICAgZSA9ICgxIDw8IGViaXRzKSAtIDE7IGYgPSBwb3coMiwgZmJpdHMgLSAxKTsgcyA9IDA7XG4gIH0gZWxzZSBpZiAodiA9PT0gSW5maW5pdHkgfHwgdiA9PT0gLUluZmluaXR5KSB7XG4gICAgZSA9ICgxIDw8IGViaXRzKSAtIDE7IGYgPSAwOyBzID0gKHYgPCAwKSA/IDEgOiAwO1xuICB9IGVsc2UgaWYgKHYgPT09IDApIHtcbiAgICBlID0gMDsgZiA9IDA7IHMgPSAoMSAvIHYgPT09IC1JbmZpbml0eSkgPyAxIDogMDtcbiAgfSBlbHNlIHtcbiAgICBzID0gdiA8IDA7XG4gICAgdiA9IGFicyh2KTtcblxuICAgIGlmICh2ID49IHBvdygyLCAxIC0gYmlhcykpIHtcbiAgICAgIGUgPSBtaW4oZmxvb3IobG9nKHYpIC8gTE4yKSwgMTAyMyk7XG4gICAgICBmID0gcm91bmRUb0V2ZW4odiAvIHBvdygyLCBlKSAqIHBvdygyLCBmYml0cykpO1xuICAgICAgaWYgKGYgLyBwb3coMiwgZmJpdHMpID49IDIpIHtcbiAgICAgICAgZSA9IGUgKyAxO1xuICAgICAgICBmID0gMTtcbiAgICAgIH1cbiAgICAgIGlmIChlID4gYmlhcykge1xuICAgICAgICAvLyBPdmVyZmxvd1xuICAgICAgICBlID0gKDEgPDwgZWJpdHMpIC0gMTtcbiAgICAgICAgZiA9IDA7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICAvLyBOb3JtYWxpemVkXG4gICAgICAgIGUgPSBlICsgYmlhcztcbiAgICAgICAgZiA9IGYgLSBwb3coMiwgZmJpdHMpO1xuICAgICAgfVxuICAgIH0gZWxzZSB7XG4gICAgICAvLyBEZW5vcm1hbGl6ZWRcbiAgICAgIGUgPSAwO1xuICAgICAgZiA9IHJvdW5kVG9FdmVuKHYgLyBwb3coMiwgMSAtIGJpYXMgLSBmYml0cykpO1xuICAgIH1cbiAgfVxuXG4gIC8vIFBhY2sgc2lnbiwgZXhwb25lbnQsIGZyYWN0aW9uXG4gIGJpdHMgPSBbXTtcbiAgZm9yIChpID0gZmJpdHM7IGk7IGkgLT0gMSkgeyBiaXRzLnB1c2goZiAlIDIgPyAxIDogMCk7IGYgPSBmbG9vcihmIC8gMik7IH1cbiAgZm9yIChpID0gZWJpdHM7IGk7IGkgLT0gMSkgeyBiaXRzLnB1c2goZSAlIDIgPyAxIDogMCk7IGUgPSBmbG9vcihlIC8gMik7IH1cbiAgYml0cy5wdXNoKHMgPyAxIDogMCk7XG4gIGJpdHMucmV2ZXJzZSgpO1xuICBzdHIgPSBiaXRzLmpvaW4oJycpO1xuXG4gIC8vIEJpdHMgdG8gYnl0ZXNcbiAgYnl0ZXMgPSBbXTtcbiAgd2hpbGUgKHN0ci5sZW5ndGgpIHtcbiAgICBieXRlcy5wdXNoKHBhcnNlSW50KHN0ci5zdWJzdHJpbmcoMCwgOCksIDIpKTtcbiAgICBzdHIgPSBzdHIuc3Vic3RyaW5nKDgpO1xuICB9XG4gIHJldHVybiBieXRlcztcbn1cblxuZnVuY3Rpb24gdW5wYWNrSUVFRTc1NChieXRlcywgZWJpdHMsIGZiaXRzKSB7XG5cbiAgLy8gQnl0ZXMgdG8gYml0c1xuICB2YXIgYml0cyA9IFtdLCBpLCBqLCBiLCBzdHIsXG4gICAgICBiaWFzLCBzLCBlLCBmO1xuXG4gIGZvciAoaSA9IGJ5dGVzLmxlbmd0aDsgaTsgaSAtPSAxKSB7XG4gICAgYiA9IGJ5dGVzW2kgLSAxXTtcbiAgICBmb3IgKGogPSA4OyBqOyBqIC09IDEpIHtcbiAgICAgIGJpdHMucHVzaChiICUgMiA/IDEgOiAwKTsgYiA9IGIgPj4gMTtcbiAgICB9XG4gIH1cbiAgYml0cy5yZXZlcnNlKCk7XG4gIHN0ciA9IGJpdHMuam9pbignJyk7XG5cbiAgLy8gVW5wYWNrIHNpZ24sIGV4cG9uZW50LCBmcmFjdGlvblxuICBiaWFzID0gKDEgPDwgKGViaXRzIC0gMSkpIC0gMTtcbiAgcyA9IHBhcnNlSW50KHN0ci5zdWJzdHJpbmcoMCwgMSksIDIpID8gLTEgOiAxO1xuICBlID0gcGFyc2VJbnQoc3RyLnN1YnN0cmluZygxLCAxICsgZWJpdHMpLCAyKTtcbiAgZiA9IHBhcnNlSW50KHN0ci5zdWJzdHJpbmcoMSArIGViaXRzKSwgMik7XG5cbiAgLy8gUHJvZHVjZSBudW1iZXJcbiAgaWYgKGUgPT09ICgxIDw8IGViaXRzKSAtIDEpIHtcbiAgICByZXR1cm4gZiAhPT0gMCA/IE5hTiA6IHMgKiBJbmZpbml0eTtcbiAgfSBlbHNlIGlmIChlID4gMCkge1xuICAgIC8vIE5vcm1hbGl6ZWRcbiAgICByZXR1cm4gcyAqIHBvdygyLCBlIC0gYmlhcykgKiAoMSArIGYgLyBwb3coMiwgZmJpdHMpKTtcbiAgfSBlbHNlIGlmIChmICE9PSAwKSB7XG4gICAgLy8gRGVub3JtYWxpemVkXG4gICAgcmV0dXJuIHMgKiBwb3coMiwgLShiaWFzIC0gMSkpICogKGYgLyBwb3coMiwgZmJpdHMpKTtcbiAgfSBlbHNlIHtcbiAgICByZXR1cm4gcyA8IDAgPyAtMCA6IDA7XG4gIH1cbn1cblxuZnVuY3Rpb24gdW5wYWNrRjY0KGIpIHsgcmV0dXJuIHVucGFja0lFRUU3NTQoYiwgMTEsIDUyKTsgfVxuZnVuY3Rpb24gcGFja0Y2NCh2KSB7IHJldHVybiBwYWNrSUVFRTc1NCh2LCAxMSwgNTIpOyB9XG5mdW5jdGlvbiB1bnBhY2tGMzIoYikgeyByZXR1cm4gdW5wYWNrSUVFRTc1NChiLCA4LCAyMyk7IH1cbmZ1bmN0aW9uIHBhY2tGMzIodikgeyByZXR1cm4gcGFja0lFRUU3NTQodiwgOCwgMjMpOyB9XG5cblxuLy9cbi8vIDMgVGhlIEFycmF5QnVmZmVyIFR5cGVcbi8vXG5cbihmdW5jdGlvbigpIHtcblxuICAvKiogQGNvbnN0cnVjdG9yICovXG4gIHZhciBBcnJheUJ1ZmZlciA9IGZ1bmN0aW9uIEFycmF5QnVmZmVyKGxlbmd0aCkge1xuICAgIGxlbmd0aCA9IEVDTUFTY3JpcHQuVG9JbnQzMihsZW5ndGgpO1xuICAgIGlmIChsZW5ndGggPCAwKSB0aHJvdyBuZXcgUmFuZ2VFcnJvcignQXJyYXlCdWZmZXIgc2l6ZSBpcyBub3QgYSBzbWFsbCBlbm91Z2ggcG9zaXRpdmUgaW50ZWdlcicpO1xuXG4gICAgdGhpcy5ieXRlTGVuZ3RoID0gbGVuZ3RoO1xuICAgIHRoaXMuX2J5dGVzID0gW107XG4gICAgdGhpcy5fYnl0ZXMubGVuZ3RoID0gbGVuZ3RoO1xuXG4gICAgdmFyIGk7XG4gICAgZm9yIChpID0gMDsgaSA8IHRoaXMuYnl0ZUxlbmd0aDsgaSArPSAxKSB7XG4gICAgICB0aGlzLl9ieXRlc1tpXSA9IDA7XG4gICAgfVxuXG4gICAgY29uZmlndXJlUHJvcGVydGllcyh0aGlzKTtcbiAgfTtcblxuICBleHBvcnRzLkFycmF5QnVmZmVyID0gZXhwb3J0cy5BcnJheUJ1ZmZlciB8fCBBcnJheUJ1ZmZlcjtcblxuICAvL1xuICAvLyA0IFRoZSBBcnJheUJ1ZmZlclZpZXcgVHlwZVxuICAvL1xuXG4gIC8vIE5PVEU6IHRoaXMgY29uc3RydWN0b3IgaXMgbm90IGV4cG9ydGVkXG4gIC8qKiBAY29uc3RydWN0b3IgKi9cbiAgdmFyIEFycmF5QnVmZmVyVmlldyA9IGZ1bmN0aW9uIEFycmF5QnVmZmVyVmlldygpIHtcbiAgICAvL3RoaXMuYnVmZmVyID0gbnVsbDtcbiAgICAvL3RoaXMuYnl0ZU9mZnNldCA9IDA7XG4gICAgLy90aGlzLmJ5dGVMZW5ndGggPSAwO1xuICB9O1xuXG4gIC8vXG4gIC8vIDUgVGhlIFR5cGVkIEFycmF5IFZpZXcgVHlwZXNcbiAgLy9cblxuICBmdW5jdGlvbiBtYWtlQ29uc3RydWN0b3IoYnl0ZXNQZXJFbGVtZW50LCBwYWNrLCB1bnBhY2spIHtcbiAgICAvLyBFYWNoIFR5cGVkQXJyYXkgdHlwZSByZXF1aXJlcyBhIGRpc3RpbmN0IGNvbnN0cnVjdG9yIGluc3RhbmNlIHdpdGhcbiAgICAvLyBpZGVudGljYWwgbG9naWMsIHdoaWNoIHRoaXMgcHJvZHVjZXMuXG5cbiAgICB2YXIgY3RvcjtcbiAgICBjdG9yID0gZnVuY3Rpb24oYnVmZmVyLCBieXRlT2Zmc2V0LCBsZW5ndGgpIHtcbiAgICAgIHZhciBhcnJheSwgc2VxdWVuY2UsIGksIHM7XG5cbiAgICAgIGlmICghYXJndW1lbnRzLmxlbmd0aCB8fCB0eXBlb2YgYXJndW1lbnRzWzBdID09PSAnbnVtYmVyJykge1xuICAgICAgICAvLyBDb25zdHJ1Y3Rvcih1bnNpZ25lZCBsb25nIGxlbmd0aClcbiAgICAgICAgdGhpcy5sZW5ndGggPSBFQ01BU2NyaXB0LlRvSW50MzIoYXJndW1lbnRzWzBdKTtcbiAgICAgICAgaWYgKGxlbmd0aCA8IDApIHRocm93IG5ldyBSYW5nZUVycm9yKCdBcnJheUJ1ZmZlclZpZXcgc2l6ZSBpcyBub3QgYSBzbWFsbCBlbm91Z2ggcG9zaXRpdmUgaW50ZWdlcicpO1xuXG4gICAgICAgIHRoaXMuYnl0ZUxlbmd0aCA9IHRoaXMubGVuZ3RoICogdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgdGhpcy5idWZmZXIgPSBuZXcgQXJyYXlCdWZmZXIodGhpcy5ieXRlTGVuZ3RoKTtcbiAgICAgICAgdGhpcy5ieXRlT2Zmc2V0ID0gMDtcbiAgICAgIH0gZWxzZSBpZiAodHlwZW9mIGFyZ3VtZW50c1swXSA9PT0gJ29iamVjdCcgJiYgYXJndW1lbnRzWzBdLmNvbnN0cnVjdG9yID09PSBjdG9yKSB7XG4gICAgICAgIC8vIENvbnN0cnVjdG9yKFR5cGVkQXJyYXkgYXJyYXkpXG4gICAgICAgIGFycmF5ID0gYXJndW1lbnRzWzBdO1xuXG4gICAgICAgIHRoaXMubGVuZ3RoID0gYXJyYXkubGVuZ3RoO1xuICAgICAgICB0aGlzLmJ5dGVMZW5ndGggPSB0aGlzLmxlbmd0aCAqIHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG4gICAgICAgIHRoaXMuYnVmZmVyID0gbmV3IEFycmF5QnVmZmVyKHRoaXMuYnl0ZUxlbmd0aCk7XG4gICAgICAgIHRoaXMuYnl0ZU9mZnNldCA9IDA7XG5cbiAgICAgICAgZm9yIChpID0gMDsgaSA8IHRoaXMubGVuZ3RoOyBpICs9IDEpIHtcbiAgICAgICAgICB0aGlzLl9zZXR0ZXIoaSwgYXJyYXkuX2dldHRlcihpKSk7XG4gICAgICAgIH1cbiAgICAgIH0gZWxzZSBpZiAodHlwZW9mIGFyZ3VtZW50c1swXSA9PT0gJ29iamVjdCcgJiZcbiAgICAgICAgICAgICAgICAgIShhcmd1bWVudHNbMF0gaW5zdGFuY2VvZiBBcnJheUJ1ZmZlciB8fCBFQ01BU2NyaXB0LkNsYXNzKGFyZ3VtZW50c1swXSkgPT09ICdBcnJheUJ1ZmZlcicpKSB7XG4gICAgICAgIC8vIENvbnN0cnVjdG9yKHNlcXVlbmNlPHR5cGU+IGFycmF5KVxuICAgICAgICBzZXF1ZW5jZSA9IGFyZ3VtZW50c1swXTtcblxuICAgICAgICB0aGlzLmxlbmd0aCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoc2VxdWVuY2UubGVuZ3RoKTtcbiAgICAgICAgdGhpcy5ieXRlTGVuZ3RoID0gdGhpcy5sZW5ndGggKiB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UO1xuICAgICAgICB0aGlzLmJ1ZmZlciA9IG5ldyBBcnJheUJ1ZmZlcih0aGlzLmJ5dGVMZW5ndGgpO1xuICAgICAgICB0aGlzLmJ5dGVPZmZzZXQgPSAwO1xuXG4gICAgICAgIGZvciAoaSA9IDA7IGkgPCB0aGlzLmxlbmd0aDsgaSArPSAxKSB7XG4gICAgICAgICAgcyA9IHNlcXVlbmNlW2ldO1xuICAgICAgICAgIHRoaXMuX3NldHRlcihpLCBOdW1iZXIocykpO1xuICAgICAgICB9XG4gICAgICB9IGVsc2UgaWYgKHR5cGVvZiBhcmd1bWVudHNbMF0gPT09ICdvYmplY3QnICYmXG4gICAgICAgICAgICAgICAgIChhcmd1bWVudHNbMF0gaW5zdGFuY2VvZiBBcnJheUJ1ZmZlciB8fCBFQ01BU2NyaXB0LkNsYXNzKGFyZ3VtZW50c1swXSkgPT09ICdBcnJheUJ1ZmZlcicpKSB7XG4gICAgICAgIC8vIENvbnN0cnVjdG9yKEFycmF5QnVmZmVyIGJ1ZmZlcixcbiAgICAgICAgLy8gICAgICAgICAgICAgb3B0aW9uYWwgdW5zaWduZWQgbG9uZyBieXRlT2Zmc2V0LCBvcHRpb25hbCB1bnNpZ25lZCBsb25nIGxlbmd0aClcbiAgICAgICAgdGhpcy5idWZmZXIgPSBidWZmZXI7XG5cbiAgICAgICAgdGhpcy5ieXRlT2Zmc2V0ID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihieXRlT2Zmc2V0KTtcbiAgICAgICAgaWYgKHRoaXMuYnl0ZU9mZnNldCA+IHRoaXMuYnVmZmVyLmJ5dGVMZW5ndGgpIHtcbiAgICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcImJ5dGVPZmZzZXQgb3V0IG9mIHJhbmdlXCIpO1xuICAgICAgICB9XG5cbiAgICAgICAgaWYgKHRoaXMuYnl0ZU9mZnNldCAlIHRoaXMuQllURVNfUEVSX0VMRU1FTlQpIHtcbiAgICAgICAgICAvLyBUaGUgZ2l2ZW4gYnl0ZU9mZnNldCBtdXN0IGJlIGEgbXVsdGlwbGUgb2YgdGhlIGVsZW1lbnRcbiAgICAgICAgICAvLyBzaXplIG9mIHRoZSBzcGVjaWZpYyB0eXBlLCBvdGhlcndpc2UgYW4gZXhjZXB0aW9uIGlzIHJhaXNlZC5cbiAgICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcIkFycmF5QnVmZmVyIGxlbmd0aCBtaW51cyB0aGUgYnl0ZU9mZnNldCBpcyBub3QgYSBtdWx0aXBsZSBvZiB0aGUgZWxlbWVudCBzaXplLlwiKTtcbiAgICAgICAgfVxuXG4gICAgICAgIGlmIChhcmd1bWVudHMubGVuZ3RoIDwgMykge1xuICAgICAgICAgIHRoaXMuYnl0ZUxlbmd0aCA9IHRoaXMuYnVmZmVyLmJ5dGVMZW5ndGggLSB0aGlzLmJ5dGVPZmZzZXQ7XG5cbiAgICAgICAgICBpZiAodGhpcy5ieXRlTGVuZ3RoICUgdGhpcy5CWVRFU19QRVJfRUxFTUVOVCkge1xuICAgICAgICAgICAgdGhyb3cgbmV3IFJhbmdlRXJyb3IoXCJsZW5ndGggb2YgYnVmZmVyIG1pbnVzIGJ5dGVPZmZzZXQgbm90IGEgbXVsdGlwbGUgb2YgdGhlIGVsZW1lbnQgc2l6ZVwiKTtcbiAgICAgICAgICB9XG4gICAgICAgICAgdGhpcy5sZW5ndGggPSB0aGlzLmJ5dGVMZW5ndGggLyB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UO1xuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIHRoaXMubGVuZ3RoID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihsZW5ndGgpO1xuICAgICAgICAgIHRoaXMuYnl0ZUxlbmd0aCA9IHRoaXMubGVuZ3RoICogdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgfVxuXG4gICAgICAgIGlmICgodGhpcy5ieXRlT2Zmc2V0ICsgdGhpcy5ieXRlTGVuZ3RoKSA+IHRoaXMuYnVmZmVyLmJ5dGVMZW5ndGgpIHtcbiAgICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcImJ5dGVPZmZzZXQgYW5kIGxlbmd0aCByZWZlcmVuY2UgYW4gYXJlYSBiZXlvbmQgdGhlIGVuZCBvZiB0aGUgYnVmZmVyXCIpO1xuICAgICAgICB9XG4gICAgICB9IGVsc2Uge1xuICAgICAgICB0aHJvdyBuZXcgVHlwZUVycm9yKFwiVW5leHBlY3RlZCBhcmd1bWVudCB0eXBlKHMpXCIpO1xuICAgICAgfVxuXG4gICAgICB0aGlzLmNvbnN0cnVjdG9yID0gY3RvcjtcblxuICAgICAgY29uZmlndXJlUHJvcGVydGllcyh0aGlzKTtcbiAgICAgIG1ha2VBcnJheUFjY2Vzc29ycyh0aGlzKTtcbiAgICB9O1xuXG4gICAgY3Rvci5wcm90b3R5cGUgPSBuZXcgQXJyYXlCdWZmZXJWaWV3KCk7XG4gICAgY3Rvci5wcm90b3R5cGUuQllURVNfUEVSX0VMRU1FTlQgPSBieXRlc1BlckVsZW1lbnQ7XG4gICAgY3Rvci5wcm90b3R5cGUuX3BhY2sgPSBwYWNrO1xuICAgIGN0b3IucHJvdG90eXBlLl91bnBhY2sgPSB1bnBhY2s7XG4gICAgY3Rvci5CWVRFU19QRVJfRUxFTUVOVCA9IGJ5dGVzUGVyRWxlbWVudDtcblxuICAgIC8vIGdldHRlciB0eXBlICh1bnNpZ25lZCBsb25nIGluZGV4KTtcbiAgICBjdG9yLnByb3RvdHlwZS5fZ2V0dGVyID0gZnVuY3Rpb24oaW5kZXgpIHtcbiAgICAgIGlmIChhcmd1bWVudHMubGVuZ3RoIDwgMSkgdGhyb3cgbmV3IFN5bnRheEVycm9yKFwiTm90IGVub3VnaCBhcmd1bWVudHNcIik7XG5cbiAgICAgIGluZGV4ID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihpbmRleCk7XG4gICAgICBpZiAoaW5kZXggPj0gdGhpcy5sZW5ndGgpIHtcbiAgICAgICAgcmV0dXJuIHVuZGVmaW5lZDtcbiAgICAgIH1cblxuICAgICAgdmFyIGJ5dGVzID0gW10sIGksIG87XG4gICAgICBmb3IgKGkgPSAwLCBvID0gdGhpcy5ieXRlT2Zmc2V0ICsgaW5kZXggKiB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UO1xuICAgICAgICAgICBpIDwgdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgICAgaSArPSAxLCBvICs9IDEpIHtcbiAgICAgICAgYnl0ZXMucHVzaCh0aGlzLmJ1ZmZlci5fYnl0ZXNbb10pO1xuICAgICAgfVxuICAgICAgcmV0dXJuIHRoaXMuX3VucGFjayhieXRlcyk7XG4gICAgfTtcblxuICAgIC8vIE5PTlNUQU5EQVJEOiBjb252ZW5pZW5jZSBhbGlhcyBmb3IgZ2V0dGVyOiB0eXBlIGdldCh1bnNpZ25lZCBsb25nIGluZGV4KTtcbiAgICBjdG9yLnByb3RvdHlwZS5nZXQgPSBjdG9yLnByb3RvdHlwZS5fZ2V0dGVyO1xuXG4gICAgLy8gc2V0dGVyIHZvaWQgKHVuc2lnbmVkIGxvbmcgaW5kZXgsIHR5cGUgdmFsdWUpO1xuICAgIGN0b3IucHJvdG90eXBlLl9zZXR0ZXIgPSBmdW5jdGlvbihpbmRleCwgdmFsdWUpIHtcbiAgICAgIGlmIChhcmd1bWVudHMubGVuZ3RoIDwgMikgdGhyb3cgbmV3IFN5bnRheEVycm9yKFwiTm90IGVub3VnaCBhcmd1bWVudHNcIik7XG5cbiAgICAgIGluZGV4ID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihpbmRleCk7XG4gICAgICBpZiAoaW5kZXggPj0gdGhpcy5sZW5ndGgpIHtcbiAgICAgICAgcmV0dXJuIHVuZGVmaW5lZDtcbiAgICAgIH1cblxuICAgICAgdmFyIGJ5dGVzID0gdGhpcy5fcGFjayh2YWx1ZSksIGksIG87XG4gICAgICBmb3IgKGkgPSAwLCBvID0gdGhpcy5ieXRlT2Zmc2V0ICsgaW5kZXggKiB0aGlzLkJZVEVTX1BFUl9FTEVNRU5UO1xuICAgICAgICAgICBpIDwgdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgICAgaSArPSAxLCBvICs9IDEpIHtcbiAgICAgICAgdGhpcy5idWZmZXIuX2J5dGVzW29dID0gYnl0ZXNbaV07XG4gICAgICB9XG4gICAgfTtcblxuICAgIC8vIHZvaWQgc2V0KFR5cGVkQXJyYXkgYXJyYXksIG9wdGlvbmFsIHVuc2lnbmVkIGxvbmcgb2Zmc2V0KTtcbiAgICAvLyB2b2lkIHNldChzZXF1ZW5jZTx0eXBlPiBhcnJheSwgb3B0aW9uYWwgdW5zaWduZWQgbG9uZyBvZmZzZXQpO1xuICAgIGN0b3IucHJvdG90eXBlLnNldCA9IGZ1bmN0aW9uKGluZGV4LCB2YWx1ZSkge1xuICAgICAgaWYgKGFyZ3VtZW50cy5sZW5ndGggPCAxKSB0aHJvdyBuZXcgU3ludGF4RXJyb3IoXCJOb3QgZW5vdWdoIGFyZ3VtZW50c1wiKTtcbiAgICAgIHZhciBhcnJheSwgc2VxdWVuY2UsIG9mZnNldCwgbGVuLFxuICAgICAgICAgIGksIHMsIGQsXG4gICAgICAgICAgYnl0ZU9mZnNldCwgYnl0ZUxlbmd0aCwgdG1wO1xuXG4gICAgICBpZiAodHlwZW9mIGFyZ3VtZW50c1swXSA9PT0gJ29iamVjdCcgJiYgYXJndW1lbnRzWzBdLmNvbnN0cnVjdG9yID09PSB0aGlzLmNvbnN0cnVjdG9yKSB7XG4gICAgICAgIC8vIHZvaWQgc2V0KFR5cGVkQXJyYXkgYXJyYXksIG9wdGlvbmFsIHVuc2lnbmVkIGxvbmcgb2Zmc2V0KTtcbiAgICAgICAgYXJyYXkgPSBhcmd1bWVudHNbMF07XG4gICAgICAgIG9mZnNldCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoYXJndW1lbnRzWzFdKTtcblxuICAgICAgICBpZiAob2Zmc2V0ICsgYXJyYXkubGVuZ3RoID4gdGhpcy5sZW5ndGgpIHtcbiAgICAgICAgICB0aHJvdyBuZXcgUmFuZ2VFcnJvcihcIk9mZnNldCBwbHVzIGxlbmd0aCBvZiBhcnJheSBpcyBvdXQgb2YgcmFuZ2VcIik7XG4gICAgICAgIH1cblxuICAgICAgICBieXRlT2Zmc2V0ID0gdGhpcy5ieXRlT2Zmc2V0ICsgb2Zmc2V0ICogdGhpcy5CWVRFU19QRVJfRUxFTUVOVDtcbiAgICAgICAgYnl0ZUxlbmd0aCA9IGFycmF5Lmxlbmd0aCAqIHRoaXMuQllURVNfUEVSX0VMRU1FTlQ7XG5cbiAgICAgICAgaWYgKGFycmF5LmJ1ZmZlciA9PT0gdGhpcy5idWZmZXIpIHtcbiAgICAgICAgICB0bXAgPSBbXTtcbiAgICAgICAgICBmb3IgKGkgPSAwLCBzID0gYXJyYXkuYnl0ZU9mZnNldDsgaSA8IGJ5dGVMZW5ndGg7IGkgKz0gMSwgcyArPSAxKSB7XG4gICAgICAgICAgICB0bXBbaV0gPSBhcnJheS5idWZmZXIuX2J5dGVzW3NdO1xuICAgICAgICAgIH1cbiAgICAgICAgICBmb3IgKGkgPSAwLCBkID0gYnl0ZU9mZnNldDsgaSA8IGJ5dGVMZW5ndGg7IGkgKz0gMSwgZCArPSAxKSB7XG4gICAgICAgICAgICB0aGlzLmJ1ZmZlci5fYnl0ZXNbZF0gPSB0bXBbaV07XG4gICAgICAgICAgfVxuICAgICAgICB9IGVsc2Uge1xuICAgICAgICAgIGZvciAoaSA9IDAsIHMgPSBhcnJheS5ieXRlT2Zmc2V0LCBkID0gYnl0ZU9mZnNldDtcbiAgICAgICAgICAgICAgIGkgPCBieXRlTGVuZ3RoOyBpICs9IDEsIHMgKz0gMSwgZCArPSAxKSB7XG4gICAgICAgICAgICB0aGlzLmJ1ZmZlci5fYnl0ZXNbZF0gPSBhcnJheS5idWZmZXIuX2J5dGVzW3NdO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuICAgICAgfSBlbHNlIGlmICh0eXBlb2YgYXJndW1lbnRzWzBdID09PSAnb2JqZWN0JyAmJiB0eXBlb2YgYXJndW1lbnRzWzBdLmxlbmd0aCAhPT0gJ3VuZGVmaW5lZCcpIHtcbiAgICAgICAgLy8gdm9pZCBzZXQoc2VxdWVuY2U8dHlwZT4gYXJyYXksIG9wdGlvbmFsIHVuc2lnbmVkIGxvbmcgb2Zmc2V0KTtcbiAgICAgICAgc2VxdWVuY2UgPSBhcmd1bWVudHNbMF07XG4gICAgICAgIGxlbiA9IEVDTUFTY3JpcHQuVG9VaW50MzIoc2VxdWVuY2UubGVuZ3RoKTtcbiAgICAgICAgb2Zmc2V0ID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihhcmd1bWVudHNbMV0pO1xuXG4gICAgICAgIGlmIChvZmZzZXQgKyBsZW4gPiB0aGlzLmxlbmd0aCkge1xuICAgICAgICAgIHRocm93IG5ldyBSYW5nZUVycm9yKFwiT2Zmc2V0IHBsdXMgbGVuZ3RoIG9mIGFycmF5IGlzIG91dCBvZiByYW5nZVwiKTtcbiAgICAgICAgfVxuXG4gICAgICAgIGZvciAoaSA9IDA7IGkgPCBsZW47IGkgKz0gMSkge1xuICAgICAgICAgIHMgPSBzZXF1ZW5jZVtpXTtcbiAgICAgICAgICB0aGlzLl9zZXR0ZXIob2Zmc2V0ICsgaSwgTnVtYmVyKHMpKTtcbiAgICAgICAgfVxuICAgICAgfSBlbHNlIHtcbiAgICAgICAgdGhyb3cgbmV3IFR5cGVFcnJvcihcIlVuZXhwZWN0ZWQgYXJndW1lbnQgdHlwZShzKVwiKTtcbiAgICAgIH1cbiAgICB9O1xuXG4gICAgLy8gVHlwZWRBcnJheSBzdWJhcnJheShsb25nIGJlZ2luLCBvcHRpb25hbCBsb25nIGVuZCk7XG4gICAgY3Rvci5wcm90b3R5cGUuc3ViYXJyYXkgPSBmdW5jdGlvbihzdGFydCwgZW5kKSB7XG4gICAgICBmdW5jdGlvbiBjbGFtcCh2LCBtaW4sIG1heCkgeyByZXR1cm4gdiA8IG1pbiA/IG1pbiA6IHYgPiBtYXggPyBtYXggOiB2OyB9XG5cbiAgICAgIHN0YXJ0ID0gRUNNQVNjcmlwdC5Ub0ludDMyKHN0YXJ0KTtcbiAgICAgIGVuZCA9IEVDTUFTY3JpcHQuVG9JbnQzMihlbmQpO1xuXG4gICAgICBpZiAoYXJndW1lbnRzLmxlbmd0aCA8IDEpIHsgc3RhcnQgPSAwOyB9XG4gICAgICBpZiAoYXJndW1lbnRzLmxlbmd0aCA8IDIpIHsgZW5kID0gdGhpcy5sZW5ndGg7IH1cblxuICAgICAgaWYgKHN0YXJ0IDwgMCkgeyBzdGFydCA9IHRoaXMubGVuZ3RoICsgc3RhcnQ7IH1cbiAgICAgIGlmIChlbmQgPCAwKSB7IGVuZCA9IHRoaXMubGVuZ3RoICsgZW5kOyB9XG5cbiAgICAgIHN0YXJ0ID0gY2xhbXAoc3RhcnQsIDAsIHRoaXMubGVuZ3RoKTtcbiAgICAgIGVuZCA9IGNsYW1wKGVuZCwgMCwgdGhpcy5sZW5ndGgpO1xuXG4gICAgICB2YXIgbGVuID0gZW5kIC0gc3RhcnQ7XG4gICAgICBpZiAobGVuIDwgMCkge1xuICAgICAgICBsZW4gPSAwO1xuICAgICAgfVxuXG4gICAgICByZXR1cm4gbmV3IHRoaXMuY29uc3RydWN0b3IoXG4gICAgICAgIHRoaXMuYnVmZmVyLCB0aGlzLmJ5dGVPZmZzZXQgKyBzdGFydCAqIHRoaXMuQllURVNfUEVSX0VMRU1FTlQsIGxlbik7XG4gICAgfTtcblxuICAgIHJldHVybiBjdG9yO1xuICB9XG5cbiAgdmFyIEludDhBcnJheSA9IG1ha2VDb25zdHJ1Y3RvcigxLCBwYWNrSTgsIHVucGFja0k4KTtcbiAgdmFyIFVpbnQ4QXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoMSwgcGFja1U4LCB1bnBhY2tVOCk7XG4gIHZhciBVaW50OENsYW1wZWRBcnJheSA9IG1ha2VDb25zdHJ1Y3RvcigxLCBwYWNrVThDbGFtcGVkLCB1bnBhY2tVOCk7XG4gIHZhciBJbnQxNkFycmF5ID0gbWFrZUNvbnN0cnVjdG9yKDIsIHBhY2tJMTYsIHVucGFja0kxNik7XG4gIHZhciBVaW50MTZBcnJheSA9IG1ha2VDb25zdHJ1Y3RvcigyLCBwYWNrVTE2LCB1bnBhY2tVMTYpO1xuICB2YXIgSW50MzJBcnJheSA9IG1ha2VDb25zdHJ1Y3Rvcig0LCBwYWNrSTMyLCB1bnBhY2tJMzIpO1xuICB2YXIgVWludDMyQXJyYXkgPSBtYWtlQ29uc3RydWN0b3IoNCwgcGFja1UzMiwgdW5wYWNrVTMyKTtcbiAgdmFyIEZsb2F0MzJBcnJheSA9IG1ha2VDb25zdHJ1Y3Rvcig0LCBwYWNrRjMyLCB1bnBhY2tGMzIpO1xuICB2YXIgRmxvYXQ2NEFycmF5ID0gbWFrZUNvbnN0cnVjdG9yKDgsIHBhY2tGNjQsIHVucGFja0Y2NCk7XG5cbiAgZXhwb3J0cy5JbnQ4QXJyYXkgPSBleHBvcnRzLkludDhBcnJheSB8fCBJbnQ4QXJyYXk7XG4gIGV4cG9ydHMuVWludDhBcnJheSA9IGV4cG9ydHMuVWludDhBcnJheSB8fCBVaW50OEFycmF5O1xuICBleHBvcnRzLlVpbnQ4Q2xhbXBlZEFycmF5ID0gZXhwb3J0cy5VaW50OENsYW1wZWRBcnJheSB8fCBVaW50OENsYW1wZWRBcnJheTtcbiAgZXhwb3J0cy5JbnQxNkFycmF5ID0gZXhwb3J0cy5JbnQxNkFycmF5IHx8IEludDE2QXJyYXk7XG4gIGV4cG9ydHMuVWludDE2QXJyYXkgPSBleHBvcnRzLlVpbnQxNkFycmF5IHx8IFVpbnQxNkFycmF5O1xuICBleHBvcnRzLkludDMyQXJyYXkgPSBleHBvcnRzLkludDMyQXJyYXkgfHwgSW50MzJBcnJheTtcbiAgZXhwb3J0cy5VaW50MzJBcnJheSA9IGV4cG9ydHMuVWludDMyQXJyYXkgfHwgVWludDMyQXJyYXk7XG4gIGV4cG9ydHMuRmxvYXQzMkFycmF5ID0gZXhwb3J0cy5GbG9hdDMyQXJyYXkgfHwgRmxvYXQzMkFycmF5O1xuICBleHBvcnRzLkZsb2F0NjRBcnJheSA9IGV4cG9ydHMuRmxvYXQ2NEFycmF5IHx8IEZsb2F0NjRBcnJheTtcbn0oKSk7XG5cbi8vXG4vLyA2IFRoZSBEYXRhVmlldyBWaWV3IFR5cGVcbi8vXG5cbihmdW5jdGlvbigpIHtcbiAgZnVuY3Rpb24gcihhcnJheSwgaW5kZXgpIHtcbiAgICByZXR1cm4gRUNNQVNjcmlwdC5Jc0NhbGxhYmxlKGFycmF5LmdldCkgPyBhcnJheS5nZXQoaW5kZXgpIDogYXJyYXlbaW5kZXhdO1xuICB9XG5cbiAgdmFyIElTX0JJR19FTkRJQU4gPSAoZnVuY3Rpb24oKSB7XG4gICAgdmFyIHUxNmFycmF5ID0gbmV3KGV4cG9ydHMuVWludDE2QXJyYXkpKFsweDEyMzRdKSxcbiAgICAgICAgdThhcnJheSA9IG5ldyhleHBvcnRzLlVpbnQ4QXJyYXkpKHUxNmFycmF5LmJ1ZmZlcik7XG4gICAgcmV0dXJuIHIodThhcnJheSwgMCkgPT09IDB4MTI7XG4gIH0oKSk7XG5cbiAgLy8gQ29uc3RydWN0b3IoQXJyYXlCdWZmZXIgYnVmZmVyLFxuICAvLyAgICAgICAgICAgICBvcHRpb25hbCB1bnNpZ25lZCBsb25nIGJ5dGVPZmZzZXQsXG4gIC8vICAgICAgICAgICAgIG9wdGlvbmFsIHVuc2lnbmVkIGxvbmcgYnl0ZUxlbmd0aClcbiAgLyoqIEBjb25zdHJ1Y3RvciAqL1xuICB2YXIgRGF0YVZpZXcgPSBmdW5jdGlvbiBEYXRhVmlldyhidWZmZXIsIGJ5dGVPZmZzZXQsIGJ5dGVMZW5ndGgpIHtcbiAgICBpZiAoYXJndW1lbnRzLmxlbmd0aCA9PT0gMCkge1xuICAgICAgYnVmZmVyID0gbmV3IGV4cG9ydHMuQXJyYXlCdWZmZXIoMCk7XG4gICAgfSBlbHNlIGlmICghKGJ1ZmZlciBpbnN0YW5jZW9mIGV4cG9ydHMuQXJyYXlCdWZmZXIgfHwgRUNNQVNjcmlwdC5DbGFzcyhidWZmZXIpID09PSAnQXJyYXlCdWZmZXInKSkge1xuICAgICAgdGhyb3cgbmV3IFR5cGVFcnJvcihcIlR5cGVFcnJvclwiKTtcbiAgICB9XG5cbiAgICB0aGlzLmJ1ZmZlciA9IGJ1ZmZlciB8fCBuZXcgZXhwb3J0cy5BcnJheUJ1ZmZlcigwKTtcblxuICAgIHRoaXMuYnl0ZU9mZnNldCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoYnl0ZU9mZnNldCk7XG4gICAgaWYgKHRoaXMuYnl0ZU9mZnNldCA+IHRoaXMuYnVmZmVyLmJ5dGVMZW5ndGgpIHtcbiAgICAgIHRocm93IG5ldyBSYW5nZUVycm9yKFwiYnl0ZU9mZnNldCBvdXQgb2YgcmFuZ2VcIik7XG4gICAgfVxuXG4gICAgaWYgKGFyZ3VtZW50cy5sZW5ndGggPCAzKSB7XG4gICAgICB0aGlzLmJ5dGVMZW5ndGggPSB0aGlzLmJ1ZmZlci5ieXRlTGVuZ3RoIC0gdGhpcy5ieXRlT2Zmc2V0O1xuICAgIH0gZWxzZSB7XG4gICAgICB0aGlzLmJ5dGVMZW5ndGggPSBFQ01BU2NyaXB0LlRvVWludDMyKGJ5dGVMZW5ndGgpO1xuICAgIH1cblxuICAgIGlmICgodGhpcy5ieXRlT2Zmc2V0ICsgdGhpcy5ieXRlTGVuZ3RoKSA+IHRoaXMuYnVmZmVyLmJ5dGVMZW5ndGgpIHtcbiAgICAgIHRocm93IG5ldyBSYW5nZUVycm9yKFwiYnl0ZU9mZnNldCBhbmQgbGVuZ3RoIHJlZmVyZW5jZSBhbiBhcmVhIGJleW9uZCB0aGUgZW5kIG9mIHRoZSBidWZmZXJcIik7XG4gICAgfVxuXG4gICAgY29uZmlndXJlUHJvcGVydGllcyh0aGlzKTtcbiAgfTtcblxuICBmdW5jdGlvbiBtYWtlR2V0dGVyKGFycmF5VHlwZSkge1xuICAgIHJldHVybiBmdW5jdGlvbihieXRlT2Zmc2V0LCBsaXR0bGVFbmRpYW4pIHtcblxuICAgICAgYnl0ZU9mZnNldCA9IEVDTUFTY3JpcHQuVG9VaW50MzIoYnl0ZU9mZnNldCk7XG5cbiAgICAgIGlmIChieXRlT2Zmc2V0ICsgYXJyYXlUeXBlLkJZVEVTX1BFUl9FTEVNRU5UID4gdGhpcy5ieXRlTGVuZ3RoKSB7XG4gICAgICAgIHRocm93IG5ldyBSYW5nZUVycm9yKFwiQXJyYXkgaW5kZXggb3V0IG9mIHJhbmdlXCIpO1xuICAgICAgfVxuICAgICAgYnl0ZU9mZnNldCArPSB0aGlzLmJ5dGVPZmZzZXQ7XG5cbiAgICAgIHZhciB1aW50OEFycmF5ID0gbmV3IGV4cG9ydHMuVWludDhBcnJheSh0aGlzLmJ1ZmZlciwgYnl0ZU9mZnNldCwgYXJyYXlUeXBlLkJZVEVTX1BFUl9FTEVNRU5UKSxcbiAgICAgICAgICBieXRlcyA9IFtdLCBpO1xuICAgICAgZm9yIChpID0gMDsgaSA8IGFycmF5VHlwZS5CWVRFU19QRVJfRUxFTUVOVDsgaSArPSAxKSB7XG4gICAgICAgIGJ5dGVzLnB1c2gocih1aW50OEFycmF5LCBpKSk7XG4gICAgICB9XG5cbiAgICAgIGlmIChCb29sZWFuKGxpdHRsZUVuZGlhbikgPT09IEJvb2xlYW4oSVNfQklHX0VORElBTikpIHtcbiAgICAgICAgYnl0ZXMucmV2ZXJzZSgpO1xuICAgICAgfVxuXG4gICAgICByZXR1cm4gcihuZXcgYXJyYXlUeXBlKG5ldyBleHBvcnRzLlVpbnQ4QXJyYXkoYnl0ZXMpLmJ1ZmZlciksIDApO1xuICAgIH07XG4gIH1cblxuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0VWludDggPSBtYWtlR2V0dGVyKGV4cG9ydHMuVWludDhBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5nZXRJbnQ4ID0gbWFrZUdldHRlcihleHBvcnRzLkludDhBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5nZXRVaW50MTYgPSBtYWtlR2V0dGVyKGV4cG9ydHMuVWludDE2QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0SW50MTYgPSBtYWtlR2V0dGVyKGV4cG9ydHMuSW50MTZBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5nZXRVaW50MzIgPSBtYWtlR2V0dGVyKGV4cG9ydHMuVWludDMyQXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuZ2V0SW50MzIgPSBtYWtlR2V0dGVyKGV4cG9ydHMuSW50MzJBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5nZXRGbG9hdDMyID0gbWFrZUdldHRlcihleHBvcnRzLkZsb2F0MzJBcnJheSk7XG4gIERhdGFWaWV3LnByb3RvdHlwZS5nZXRGbG9hdDY0ID0gbWFrZUdldHRlcihleHBvcnRzLkZsb2F0NjRBcnJheSk7XG5cbiAgZnVuY3Rpb24gbWFrZVNldHRlcihhcnJheVR5cGUpIHtcbiAgICByZXR1cm4gZnVuY3Rpb24oYnl0ZU9mZnNldCwgdmFsdWUsIGxpdHRsZUVuZGlhbikge1xuXG4gICAgICBieXRlT2Zmc2V0ID0gRUNNQVNjcmlwdC5Ub1VpbnQzMihieXRlT2Zmc2V0KTtcbiAgICAgIGlmIChieXRlT2Zmc2V0ICsgYXJyYXlUeXBlLkJZVEVTX1BFUl9FTEVNRU5UID4gdGhpcy5ieXRlTGVuZ3RoKSB7XG4gICAgICAgIHRocm93IG5ldyBSYW5nZUVycm9yKFwiQXJyYXkgaW5kZXggb3V0IG9mIHJhbmdlXCIpO1xuICAgICAgfVxuXG4gICAgICAvLyBHZXQgYnl0ZXNcbiAgICAgIHZhciB0eXBlQXJyYXkgPSBuZXcgYXJyYXlUeXBlKFt2YWx1ZV0pLFxuICAgICAgICAgIGJ5dGVBcnJheSA9IG5ldyBleHBvcnRzLlVpbnQ4QXJyYXkodHlwZUFycmF5LmJ1ZmZlciksXG4gICAgICAgICAgYnl0ZXMgPSBbXSwgaSwgYnl0ZVZpZXc7XG5cbiAgICAgIGZvciAoaSA9IDA7IGkgPCBhcnJheVR5cGUuQllURVNfUEVSX0VMRU1FTlQ7IGkgKz0gMSkge1xuICAgICAgICBieXRlcy5wdXNoKHIoYnl0ZUFycmF5LCBpKSk7XG4gICAgICB9XG5cbiAgICAgIC8vIEZsaXAgaWYgbmVjZXNzYXJ5XG4gICAgICBpZiAoQm9vbGVhbihsaXR0bGVFbmRpYW4pID09PSBCb29sZWFuKElTX0JJR19FTkRJQU4pKSB7XG4gICAgICAgIGJ5dGVzLnJldmVyc2UoKTtcbiAgICAgIH1cblxuICAgICAgLy8gV3JpdGUgdGhlbVxuICAgICAgYnl0ZVZpZXcgPSBuZXcgZXhwb3J0cy5VaW50OEFycmF5KHRoaXMuYnVmZmVyLCBieXRlT2Zmc2V0LCBhcnJheVR5cGUuQllURVNfUEVSX0VMRU1FTlQpO1xuICAgICAgYnl0ZVZpZXcuc2V0KGJ5dGVzKTtcbiAgICB9O1xuICB9XG5cbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldFVpbnQ4ID0gbWFrZVNldHRlcihleHBvcnRzLlVpbnQ4QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuc2V0SW50OCA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5JbnQ4QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuc2V0VWludDE2ID0gbWFrZVNldHRlcihleHBvcnRzLlVpbnQxNkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldEludDE2ID0gbWFrZVNldHRlcihleHBvcnRzLkludDE2QXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuc2V0VWludDMyID0gbWFrZVNldHRlcihleHBvcnRzLlVpbnQzMkFycmF5KTtcbiAgRGF0YVZpZXcucHJvdG90eXBlLnNldEludDMyID0gbWFrZVNldHRlcihleHBvcnRzLkludDMyQXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuc2V0RmxvYXQzMiA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5GbG9hdDMyQXJyYXkpO1xuICBEYXRhVmlldy5wcm90b3R5cGUuc2V0RmxvYXQ2NCA9IG1ha2VTZXR0ZXIoZXhwb3J0cy5GbG9hdDY0QXJyYXkpO1xuXG4gIGV4cG9ydHMuRGF0YVZpZXcgPSBleHBvcnRzLkRhdGFWaWV3IHx8IERhdGFWaWV3O1xuXG59KCkpO1xuIiwidmFyIGdsb2JhbD10eXBlb2Ygc2VsZiAhPT0gXCJ1bmRlZmluZWRcIiA/IHNlbGYgOiB0eXBlb2Ygd2luZG93ICE9PSBcInVuZGVmaW5lZFwiID8gd2luZG93IDoge30sQnVmZmVyPXJlcXVpcmUoXCJfX2Jyb3dzZXJpZnlfQnVmZmVyXCIpO3ZhciBiZW5jaG1hcmsgPSByZXF1aXJlKCdiZW5jaG1hcmsnKVxudmFyIHN1aXRlID0gbmV3IGJlbmNobWFyay5TdWl0ZSgpXG5cbmdsb2JhbC5OZXdCdWZmZXIgPSByZXF1aXJlKCcuLi8uLi8nKS5CdWZmZXIgLy8gbmF0aXZlLWJ1ZmZlci1icm93c2VyaWZ5XG5nbG9iYWwuT2xkQnVmZmVyID0gcmVxdWlyZSgnYnVmZmVyLWJyb3dzZXJpZnknKS5CdWZmZXIgLy8gYnVmZmVyLWJyb3dzZXJpZnlcblxudmFyIExFTkdUSCA9IDUwXG5cbnZhciBuZXdUYXJnZXQgPSBOZXdCdWZmZXIoTEVOR1RIKVxudmFyIG9sZFRhcmdldCA9IE9sZEJ1ZmZlcihMRU5HVEgpXG52YXIgbm9kZVRhcmdldCA9IEJ1ZmZlcihMRU5HVEgpXG5cbnN1aXRlLmFkZCgnTmV3QnVmZmVyI2JyYWNrZXQtbm90YXRpb24nLCBmdW5jdGlvbiAoKSB7XG4gIGZvciAodmFyIGkgPSAwOyBpIDwgTEVOR1RIOyBpKyspIHtcbiAgICBuZXdUYXJnZXRbaV0gPSBpICsgOTdcbiAgfVxufSlcbi5hZGQoJ09sZEJ1ZmZlciNicmFja2V0LW5vdGF0aW9uJywgZnVuY3Rpb24gKCkge1xuICBmb3IgKHZhciBpID0gMDsgaSA8IExFTkdUSDsgaSsrKSB7XG4gICAgb2xkVGFyZ2V0W2ldID0gaSArIDk3XG4gIH1cbn0pXG4uYWRkKCdCdWZmZXIjYnJhY2tldC1ub3RhdGlvbicsIGZ1bmN0aW9uICgpIHtcbiAgZm9yICh2YXIgaSA9IDA7IGkgPCBMRU5HVEg7IGkrKykge1xuICAgIG5vZGVUYXJnZXRbaV0gPSBpICsgOTdcbiAgfVxufSlcbi5vbignZXJyb3InLCBmdW5jdGlvbiAoZXZlbnQpIHtcbiAgY29uc29sZS5lcnJvcihldmVudC50YXJnZXQuZXJyb3Iuc3RhY2spXG59KVxuLm9uKCdjeWNsZScsIGZ1bmN0aW9uIChldmVudCkge1xuICBjb25zb2xlLmxvZyhTdHJpbmcoZXZlbnQudGFyZ2V0KSlcbn0pXG4ub24oJ2NvbXBsZXRlJywgZnVuY3Rpb24gKCkge1xuICBjb25zb2xlLmxvZygnRmFzdGVzdCBpcyAnICsgdGhpcy5maWx0ZXIoJ2Zhc3Rlc3QnKS5wbHVjaygnbmFtZScpKVxufSlcbi5ydW4oeyAnYXN5bmMnOiB0cnVlIH0pXG4iLCIvLyBodHRwOi8vd2lraS5jb21tb25qcy5vcmcvd2lraS9Vbml0X1Rlc3RpbmcvMS4wXG4vL1xuLy8gVEhJUyBJUyBOT1QgVEVTVEVEIE5PUiBMSUtFTFkgVE8gV09SSyBPVVRTSURFIFY4IVxuLy9cbi8vIE9yaWdpbmFsbHkgZnJvbSBuYXJ3aGFsLmpzIChodHRwOi8vbmFyd2hhbGpzLm9yZylcbi8vIENvcHlyaWdodCAoYykgMjAwOSBUaG9tYXMgUm9iaW5zb24gPDI4MG5vcnRoLmNvbT5cbi8vXG4vLyBQZXJtaXNzaW9uIGlzIGhlcmVieSBncmFudGVkLCBmcmVlIG9mIGNoYXJnZSwgdG8gYW55IHBlcnNvbiBvYnRhaW5pbmcgYSBjb3B5XG4vLyBvZiB0aGlzIHNvZnR3YXJlIGFuZCBhc3NvY2lhdGVkIGRvY3VtZW50YXRpb24gZmlsZXMgKHRoZSAnU29mdHdhcmUnKSwgdG9cbi8vIGRlYWwgaW4gdGhlIFNvZnR3YXJlIHdpdGhvdXQgcmVzdHJpY3Rpb24sIGluY2x1ZGluZyB3aXRob3V0IGxpbWl0YXRpb24gdGhlXG4vLyByaWdodHMgdG8gdXNlLCBjb3B5LCBtb2RpZnksIG1lcmdlLCBwdWJsaXNoLCBkaXN0cmlidXRlLCBzdWJsaWNlbnNlLCBhbmQvb3Jcbi8vIHNlbGwgY29waWVzIG9mIHRoZSBTb2Z0d2FyZSwgYW5kIHRvIHBlcm1pdCBwZXJzb25zIHRvIHdob20gdGhlIFNvZnR3YXJlIGlzXG4vLyBmdXJuaXNoZWQgdG8gZG8gc28sIHN1YmplY3QgdG8gdGhlIGZvbGxvd2luZyBjb25kaXRpb25zOlxuLy9cbi8vIFRoZSBhYm92ZSBjb3B5cmlnaHQgbm90aWNlIGFuZCB0aGlzIHBlcm1pc3Npb24gbm90aWNlIHNoYWxsIGJlIGluY2x1ZGVkIGluXG4vLyBhbGwgY29waWVzIG9yIHN1YnN0YW50aWFsIHBvcnRpb25zIG9mIHRoZSBTb2Z0d2FyZS5cbi8vXG4vLyBUSEUgU09GVFdBUkUgSVMgUFJPVklERUQgJ0FTIElTJywgV0lUSE9VVCBXQVJSQU5UWSBPRiBBTlkgS0lORCwgRVhQUkVTUyBPUlxuLy8gSU1QTElFRCwgSU5DTFVESU5HIEJVVCBOT1QgTElNSVRFRCBUTyBUSEUgV0FSUkFOVElFUyBPRiBNRVJDSEFOVEFCSUxJVFksXG4vLyBGSVRORVNTIEZPUiBBIFBBUlRJQ1VMQVIgUFVSUE9TRSBBTkQgTk9OSU5GUklOR0VNRU5ULiBJTiBOTyBFVkVOVCBTSEFMTCBUSEVcbi8vIEFVVEhPUlMgQkUgTElBQkxFIEZPUiBBTlkgQ0xBSU0sIERBTUFHRVMgT1IgT1RIRVIgTElBQklMSVRZLCBXSEVUSEVSIElOIEFOXG4vLyBBQ1RJT04gT0YgQ09OVFJBQ1QsIFRPUlQgT1IgT1RIRVJXSVNFLCBBUklTSU5HIEZST00sIE9VVCBPRiBPUiBJTiBDT05ORUNUSU9OXG4vLyBXSVRIIFRIRSBTT0ZUV0FSRSBPUiBUSEUgVVNFIE9SIE9USEVSIERFQUxJTkdTIElOIFRIRSBTT0ZUV0FSRS5cblxuLy8gd2hlbiB1c2VkIGluIG5vZGUsIHRoaXMgd2lsbCBhY3R1YWxseSBsb2FkIHRoZSB1dGlsIG1vZHVsZSB3ZSBkZXBlbmQgb25cbi8vIHZlcnN1cyBsb2FkaW5nIHRoZSBidWlsdGluIHV0aWwgbW9kdWxlIGFzIGhhcHBlbnMgb3RoZXJ3aXNlXG4vLyB0aGlzIGlzIGEgYnVnIGluIG5vZGUgbW9kdWxlIGxvYWRpbmcgYXMgZmFyIGFzIEkgYW0gY29uY2VybmVkXG52YXIgdXRpbCA9IHJlcXVpcmUoJ3V0aWwvJyk7XG5cbnZhciBwU2xpY2UgPSBBcnJheS5wcm90b3R5cGUuc2xpY2U7XG52YXIgaGFzT3duID0gT2JqZWN0LnByb3RvdHlwZS5oYXNPd25Qcm9wZXJ0eTtcblxuLy8gMS4gVGhlIGFzc2VydCBtb2R1bGUgcHJvdmlkZXMgZnVuY3Rpb25zIHRoYXQgdGhyb3dcbi8vIEFzc2VydGlvbkVycm9yJ3Mgd2hlbiBwYXJ0aWN1bGFyIGNvbmRpdGlvbnMgYXJlIG5vdCBtZXQuIFRoZVxuLy8gYXNzZXJ0IG1vZHVsZSBtdXN0IGNvbmZvcm0gdG8gdGhlIGZvbGxvd2luZyBpbnRlcmZhY2UuXG5cbnZhciBhc3NlcnQgPSBtb2R1bGUuZXhwb3J0cyA9IG9rO1xuXG4vLyAyLiBUaGUgQXNzZXJ0aW9uRXJyb3IgaXMgZGVmaW5lZCBpbiBhc3NlcnQuXG4vLyBuZXcgYXNzZXJ0LkFzc2VydGlvbkVycm9yKHsgbWVzc2FnZTogbWVzc2FnZSxcbi8vICAgICAgICAgICAgICAgICAgICAgICAgICAgICBhY3R1YWw6IGFjdHVhbCxcbi8vICAgICAgICAgICAgICAgICAgICAgICAgICAgICBleHBlY3RlZDogZXhwZWN0ZWQgfSlcblxuYXNzZXJ0LkFzc2VydGlvbkVycm9yID0gZnVuY3Rpb24gQXNzZXJ0aW9uRXJyb3Iob3B0aW9ucykge1xuICB0aGlzLm5hbWUgPSAnQXNzZXJ0aW9uRXJyb3InO1xuICB0aGlzLmFjdHVhbCA9IG9wdGlvbnMuYWN0dWFsO1xuICB0aGlzLmV4cGVjdGVkID0gb3B0aW9ucy5leHBlY3RlZDtcbiAgdGhpcy5vcGVyYXRvciA9IG9wdGlvbnMub3BlcmF0b3I7XG4gIGlmIChvcHRpb25zLm1lc3NhZ2UpIHtcbiAgICB0aGlzLm1lc3NhZ2UgPSBvcHRpb25zLm1lc3NhZ2U7XG4gICAgdGhpcy5nZW5lcmF0ZWRNZXNzYWdlID0gZmFsc2U7XG4gIH0gZWxzZSB7XG4gICAgdGhpcy5tZXNzYWdlID0gZ2V0TWVzc2FnZSh0aGlzKTtcbiAgICB0aGlzLmdlbmVyYXRlZE1lc3NhZ2UgPSB0cnVlO1xuICB9XG4gIHZhciBzdGFja1N0YXJ0RnVuY3Rpb24gPSBvcHRpb25zLnN0YWNrU3RhcnRGdW5jdGlvbiB8fCBmYWlsO1xuXG4gIGlmIChFcnJvci5jYXB0dXJlU3RhY2tUcmFjZSkge1xuICAgIEVycm9yLmNhcHR1cmVTdGFja1RyYWNlKHRoaXMsIHN0YWNrU3RhcnRGdW5jdGlvbik7XG4gIH1cbn07XG5cbi8vIGFzc2VydC5Bc3NlcnRpb25FcnJvciBpbnN0YW5jZW9mIEVycm9yXG51dGlsLmluaGVyaXRzKGFzc2VydC5Bc3NlcnRpb25FcnJvciwgRXJyb3IpO1xuXG5mdW5jdGlvbiByZXBsYWNlcihrZXksIHZhbHVlKSB7XG4gIGlmICh1dGlsLmlzVW5kZWZpbmVkKHZhbHVlKSkge1xuICAgIHJldHVybiAnJyArIHZhbHVlO1xuICB9XG4gIGlmICh1dGlsLmlzTnVtYmVyKHZhbHVlKSAmJiAoaXNOYU4odmFsdWUpIHx8ICFpc0Zpbml0ZSh2YWx1ZSkpKSB7XG4gICAgcmV0dXJuIHZhbHVlLnRvU3RyaW5nKCk7XG4gIH1cbiAgaWYgKHV0aWwuaXNGdW5jdGlvbih2YWx1ZSkgfHwgdXRpbC5pc1JlZ0V4cCh2YWx1ZSkpIHtcbiAgICByZXR1cm4gdmFsdWUudG9TdHJpbmcoKTtcbiAgfVxuICByZXR1cm4gdmFsdWU7XG59XG5cbmZ1bmN0aW9uIHRydW5jYXRlKHMsIG4pIHtcbiAgaWYgKHV0aWwuaXNTdHJpbmcocykpIHtcbiAgICByZXR1cm4gcy5sZW5ndGggPCBuID8gcyA6IHMuc2xpY2UoMCwgbik7XG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIHM7XG4gIH1cbn1cblxuZnVuY3Rpb24gZ2V0TWVzc2FnZShzZWxmKSB7XG4gIHJldHVybiB0cnVuY2F0ZShKU09OLnN0cmluZ2lmeShzZWxmLmFjdHVhbCwgcmVwbGFjZXIpLCAxMjgpICsgJyAnICtcbiAgICAgICAgIHNlbGYub3BlcmF0b3IgKyAnICcgK1xuICAgICAgICAgdHJ1bmNhdGUoSlNPTi5zdHJpbmdpZnkoc2VsZi5leHBlY3RlZCwgcmVwbGFjZXIpLCAxMjgpO1xufVxuXG4vLyBBdCBwcmVzZW50IG9ubHkgdGhlIHRocmVlIGtleXMgbWVudGlvbmVkIGFib3ZlIGFyZSB1c2VkIGFuZFxuLy8gdW5kZXJzdG9vZCBieSB0aGUgc3BlYy4gSW1wbGVtZW50YXRpb25zIG9yIHN1YiBtb2R1bGVzIGNhbiBwYXNzXG4vLyBvdGhlciBrZXlzIHRvIHRoZSBBc3NlcnRpb25FcnJvcidzIGNvbnN0cnVjdG9yIC0gdGhleSB3aWxsIGJlXG4vLyBpZ25vcmVkLlxuXG4vLyAzLiBBbGwgb2YgdGhlIGZvbGxvd2luZyBmdW5jdGlvbnMgbXVzdCB0aHJvdyBhbiBBc3NlcnRpb25FcnJvclxuLy8gd2hlbiBhIGNvcnJlc3BvbmRpbmcgY29uZGl0aW9uIGlzIG5vdCBtZXQsIHdpdGggYSBtZXNzYWdlIHRoYXRcbi8vIG1heSBiZSB1bmRlZmluZWQgaWYgbm90IHByb3ZpZGVkLiAgQWxsIGFzc2VydGlvbiBtZXRob2RzIHByb3ZpZGVcbi8vIGJvdGggdGhlIGFjdHVhbCBhbmQgZXhwZWN0ZWQgdmFsdWVzIHRvIHRoZSBhc3NlcnRpb24gZXJyb3IgZm9yXG4vLyBkaXNwbGF5IHB1cnBvc2VzLlxuXG5mdW5jdGlvbiBmYWlsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2UsIG9wZXJhdG9yLCBzdGFja1N0YXJ0RnVuY3Rpb24pIHtcbiAgdGhyb3cgbmV3IGFzc2VydC5Bc3NlcnRpb25FcnJvcih7XG4gICAgbWVzc2FnZTogbWVzc2FnZSxcbiAgICBhY3R1YWw6IGFjdHVhbCxcbiAgICBleHBlY3RlZDogZXhwZWN0ZWQsXG4gICAgb3BlcmF0b3I6IG9wZXJhdG9yLFxuICAgIHN0YWNrU3RhcnRGdW5jdGlvbjogc3RhY2tTdGFydEZ1bmN0aW9uXG4gIH0pO1xufVxuXG4vLyBFWFRFTlNJT04hIGFsbG93cyBmb3Igd2VsbCBiZWhhdmVkIGVycm9ycyBkZWZpbmVkIGVsc2V3aGVyZS5cbmFzc2VydC5mYWlsID0gZmFpbDtcblxuLy8gNC4gUHVyZSBhc3NlcnRpb24gdGVzdHMgd2hldGhlciBhIHZhbHVlIGlzIHRydXRoeSwgYXMgZGV0ZXJtaW5lZFxuLy8gYnkgISFndWFyZC5cbi8vIGFzc2VydC5vayhndWFyZCwgbWVzc2FnZV9vcHQpO1xuLy8gVGhpcyBzdGF0ZW1lbnQgaXMgZXF1aXZhbGVudCB0byBhc3NlcnQuZXF1YWwodHJ1ZSwgISFndWFyZCxcbi8vIG1lc3NhZ2Vfb3B0KTsuIFRvIHRlc3Qgc3RyaWN0bHkgZm9yIHRoZSB2YWx1ZSB0cnVlLCB1c2Vcbi8vIGFzc2VydC5zdHJpY3RFcXVhbCh0cnVlLCBndWFyZCwgbWVzc2FnZV9vcHQpOy5cblxuZnVuY3Rpb24gb2sodmFsdWUsIG1lc3NhZ2UpIHtcbiAgaWYgKCF2YWx1ZSkgZmFpbCh2YWx1ZSwgdHJ1ZSwgbWVzc2FnZSwgJz09JywgYXNzZXJ0Lm9rKTtcbn1cbmFzc2VydC5vayA9IG9rO1xuXG4vLyA1LiBUaGUgZXF1YWxpdHkgYXNzZXJ0aW9uIHRlc3RzIHNoYWxsb3csIGNvZXJjaXZlIGVxdWFsaXR5IHdpdGhcbi8vID09LlxuLy8gYXNzZXJ0LmVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2Vfb3B0KTtcblxuYXNzZXJ0LmVxdWFsID0gZnVuY3Rpb24gZXF1YWwoYWN0dWFsLCBleHBlY3RlZCwgbWVzc2FnZSkge1xuICBpZiAoYWN0dWFsICE9IGV4cGVjdGVkKSBmYWlsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2UsICc9PScsIGFzc2VydC5lcXVhbCk7XG59O1xuXG4vLyA2LiBUaGUgbm9uLWVxdWFsaXR5IGFzc2VydGlvbiB0ZXN0cyBmb3Igd2hldGhlciB0d28gb2JqZWN0cyBhcmUgbm90IGVxdWFsXG4vLyB3aXRoICE9IGFzc2VydC5ub3RFcXVhbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlX29wdCk7XG5cbmFzc2VydC5ub3RFcXVhbCA9IGZ1bmN0aW9uIG5vdEVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2UpIHtcbiAgaWYgKGFjdHVhbCA9PSBleHBlY3RlZCkge1xuICAgIGZhaWwoYWN0dWFsLCBleHBlY3RlZCwgbWVzc2FnZSwgJyE9JywgYXNzZXJ0Lm5vdEVxdWFsKTtcbiAgfVxufTtcblxuLy8gNy4gVGhlIGVxdWl2YWxlbmNlIGFzc2VydGlvbiB0ZXN0cyBhIGRlZXAgZXF1YWxpdHkgcmVsYXRpb24uXG4vLyBhc3NlcnQuZGVlcEVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2Vfb3B0KTtcblxuYXNzZXJ0LmRlZXBFcXVhbCA9IGZ1bmN0aW9uIGRlZXBFcXVhbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlKSB7XG4gIGlmICghX2RlZXBFcXVhbChhY3R1YWwsIGV4cGVjdGVkKSkge1xuICAgIGZhaWwoYWN0dWFsLCBleHBlY3RlZCwgbWVzc2FnZSwgJ2RlZXBFcXVhbCcsIGFzc2VydC5kZWVwRXF1YWwpO1xuICB9XG59O1xuXG5mdW5jdGlvbiBfZGVlcEVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQpIHtcbiAgLy8gNy4xLiBBbGwgaWRlbnRpY2FsIHZhbHVlcyBhcmUgZXF1aXZhbGVudCwgYXMgZGV0ZXJtaW5lZCBieSA9PT0uXG4gIGlmIChhY3R1YWwgPT09IGV4cGVjdGVkKSB7XG4gICAgcmV0dXJuIHRydWU7XG5cbiAgfSBlbHNlIGlmICh1dGlsLmlzQnVmZmVyKGFjdHVhbCkgJiYgdXRpbC5pc0J1ZmZlcihleHBlY3RlZCkpIHtcbiAgICBpZiAoYWN0dWFsLmxlbmd0aCAhPSBleHBlY3RlZC5sZW5ndGgpIHJldHVybiBmYWxzZTtcblxuICAgIGZvciAodmFyIGkgPSAwOyBpIDwgYWN0dWFsLmxlbmd0aDsgaSsrKSB7XG4gICAgICBpZiAoYWN0dWFsW2ldICE9PSBleHBlY3RlZFtpXSkgcmV0dXJuIGZhbHNlO1xuICAgIH1cblxuICAgIHJldHVybiB0cnVlO1xuXG4gIC8vIDcuMi4gSWYgdGhlIGV4cGVjdGVkIHZhbHVlIGlzIGEgRGF0ZSBvYmplY3QsIHRoZSBhY3R1YWwgdmFsdWUgaXNcbiAgLy8gZXF1aXZhbGVudCBpZiBpdCBpcyBhbHNvIGEgRGF0ZSBvYmplY3QgdGhhdCByZWZlcnMgdG8gdGhlIHNhbWUgdGltZS5cbiAgfSBlbHNlIGlmICh1dGlsLmlzRGF0ZShhY3R1YWwpICYmIHV0aWwuaXNEYXRlKGV4cGVjdGVkKSkge1xuICAgIHJldHVybiBhY3R1YWwuZ2V0VGltZSgpID09PSBleHBlY3RlZC5nZXRUaW1lKCk7XG5cbiAgLy8gNy4zIElmIHRoZSBleHBlY3RlZCB2YWx1ZSBpcyBhIFJlZ0V4cCBvYmplY3QsIHRoZSBhY3R1YWwgdmFsdWUgaXNcbiAgLy8gZXF1aXZhbGVudCBpZiBpdCBpcyBhbHNvIGEgUmVnRXhwIG9iamVjdCB3aXRoIHRoZSBzYW1lIHNvdXJjZSBhbmRcbiAgLy8gcHJvcGVydGllcyAoYGdsb2JhbGAsIGBtdWx0aWxpbmVgLCBgbGFzdEluZGV4YCwgYGlnbm9yZUNhc2VgKS5cbiAgfSBlbHNlIGlmICh1dGlsLmlzUmVnRXhwKGFjdHVhbCkgJiYgdXRpbC5pc1JlZ0V4cChleHBlY3RlZCkpIHtcbiAgICByZXR1cm4gYWN0dWFsLnNvdXJjZSA9PT0gZXhwZWN0ZWQuc291cmNlICYmXG4gICAgICAgICAgIGFjdHVhbC5nbG9iYWwgPT09IGV4cGVjdGVkLmdsb2JhbCAmJlxuICAgICAgICAgICBhY3R1YWwubXVsdGlsaW5lID09PSBleHBlY3RlZC5tdWx0aWxpbmUgJiZcbiAgICAgICAgICAgYWN0dWFsLmxhc3RJbmRleCA9PT0gZXhwZWN0ZWQubGFzdEluZGV4ICYmXG4gICAgICAgICAgIGFjdHVhbC5pZ25vcmVDYXNlID09PSBleHBlY3RlZC5pZ25vcmVDYXNlO1xuXG4gIC8vIDcuNC4gT3RoZXIgcGFpcnMgdGhhdCBkbyBub3QgYm90aCBwYXNzIHR5cGVvZiB2YWx1ZSA9PSAnb2JqZWN0JyxcbiAgLy8gZXF1aXZhbGVuY2UgaXMgZGV0ZXJtaW5lZCBieSA9PS5cbiAgfSBlbHNlIGlmICghdXRpbC5pc09iamVjdChhY3R1YWwpICYmICF1dGlsLmlzT2JqZWN0KGV4cGVjdGVkKSkge1xuICAgIHJldHVybiBhY3R1YWwgPT0gZXhwZWN0ZWQ7XG5cbiAgLy8gNy41IEZvciBhbGwgb3RoZXIgT2JqZWN0IHBhaXJzLCBpbmNsdWRpbmcgQXJyYXkgb2JqZWN0cywgZXF1aXZhbGVuY2UgaXNcbiAgLy8gZGV0ZXJtaW5lZCBieSBoYXZpbmcgdGhlIHNhbWUgbnVtYmVyIG9mIG93bmVkIHByb3BlcnRpZXMgKGFzIHZlcmlmaWVkXG4gIC8vIHdpdGggT2JqZWN0LnByb3RvdHlwZS5oYXNPd25Qcm9wZXJ0eS5jYWxsKSwgdGhlIHNhbWUgc2V0IG9mIGtleXNcbiAgLy8gKGFsdGhvdWdoIG5vdCBuZWNlc3NhcmlseSB0aGUgc2FtZSBvcmRlciksIGVxdWl2YWxlbnQgdmFsdWVzIGZvciBldmVyeVxuICAvLyBjb3JyZXNwb25kaW5nIGtleSwgYW5kIGFuIGlkZW50aWNhbCAncHJvdG90eXBlJyBwcm9wZXJ0eS4gTm90ZTogdGhpc1xuICAvLyBhY2NvdW50cyBmb3IgYm90aCBuYW1lZCBhbmQgaW5kZXhlZCBwcm9wZXJ0aWVzIG9uIEFycmF5cy5cbiAgfSBlbHNlIHtcbiAgICByZXR1cm4gb2JqRXF1aXYoYWN0dWFsLCBleHBlY3RlZCk7XG4gIH1cbn1cblxuZnVuY3Rpb24gaXNBcmd1bWVudHMob2JqZWN0KSB7XG4gIHJldHVybiBPYmplY3QucHJvdG90eXBlLnRvU3RyaW5nLmNhbGwob2JqZWN0KSA9PSAnW29iamVjdCBBcmd1bWVudHNdJztcbn1cblxuZnVuY3Rpb24gb2JqRXF1aXYoYSwgYikge1xuICBpZiAodXRpbC5pc051bGxPclVuZGVmaW5lZChhKSB8fCB1dGlsLmlzTnVsbE9yVW5kZWZpbmVkKGIpKVxuICAgIHJldHVybiBmYWxzZTtcbiAgLy8gYW4gaWRlbnRpY2FsICdwcm90b3R5cGUnIHByb3BlcnR5LlxuICBpZiAoYS5wcm90b3R5cGUgIT09IGIucHJvdG90eXBlKSByZXR1cm4gZmFsc2U7XG4gIC8vfn5+SSd2ZSBtYW5hZ2VkIHRvIGJyZWFrIE9iamVjdC5rZXlzIHRocm91Z2ggc2NyZXd5IGFyZ3VtZW50cyBwYXNzaW5nLlxuICAvLyAgIENvbnZlcnRpbmcgdG8gYXJyYXkgc29sdmVzIHRoZSBwcm9ibGVtLlxuICBpZiAoaXNBcmd1bWVudHMoYSkpIHtcbiAgICBpZiAoIWlzQXJndW1lbnRzKGIpKSB7XG4gICAgICByZXR1cm4gZmFsc2U7XG4gICAgfVxuICAgIGEgPSBwU2xpY2UuY2FsbChhKTtcbiAgICBiID0gcFNsaWNlLmNhbGwoYik7XG4gICAgcmV0dXJuIF9kZWVwRXF1YWwoYSwgYik7XG4gIH1cbiAgdHJ5IHtcbiAgICB2YXIga2EgPSBvYmplY3RLZXlzKGEpLFxuICAgICAgICBrYiA9IG9iamVjdEtleXMoYiksXG4gICAgICAgIGtleSwgaTtcbiAgfSBjYXRjaCAoZSkgey8vaGFwcGVucyB3aGVuIG9uZSBpcyBhIHN0cmluZyBsaXRlcmFsIGFuZCB0aGUgb3RoZXIgaXNuJ3RcbiAgICByZXR1cm4gZmFsc2U7XG4gIH1cbiAgLy8gaGF2aW5nIHRoZSBzYW1lIG51bWJlciBvZiBvd25lZCBwcm9wZXJ0aWVzIChrZXlzIGluY29ycG9yYXRlc1xuICAvLyBoYXNPd25Qcm9wZXJ0eSlcbiAgaWYgKGthLmxlbmd0aCAhPSBrYi5sZW5ndGgpXG4gICAgcmV0dXJuIGZhbHNlO1xuICAvL3RoZSBzYW1lIHNldCBvZiBrZXlzIChhbHRob3VnaCBub3QgbmVjZXNzYXJpbHkgdGhlIHNhbWUgb3JkZXIpLFxuICBrYS5zb3J0KCk7XG4gIGtiLnNvcnQoKTtcbiAgLy9+fn5jaGVhcCBrZXkgdGVzdFxuICBmb3IgKGkgPSBrYS5sZW5ndGggLSAxOyBpID49IDA7IGktLSkge1xuICAgIGlmIChrYVtpXSAhPSBrYltpXSlcbiAgICAgIHJldHVybiBmYWxzZTtcbiAgfVxuICAvL2VxdWl2YWxlbnQgdmFsdWVzIGZvciBldmVyeSBjb3JyZXNwb25kaW5nIGtleSwgYW5kXG4gIC8vfn5+cG9zc2libHkgZXhwZW5zaXZlIGRlZXAgdGVzdFxuICBmb3IgKGkgPSBrYS5sZW5ndGggLSAxOyBpID49IDA7IGktLSkge1xuICAgIGtleSA9IGthW2ldO1xuICAgIGlmICghX2RlZXBFcXVhbChhW2tleV0sIGJba2V5XSkpIHJldHVybiBmYWxzZTtcbiAgfVxuICByZXR1cm4gdHJ1ZTtcbn1cblxuLy8gOC4gVGhlIG5vbi1lcXVpdmFsZW5jZSBhc3NlcnRpb24gdGVzdHMgZm9yIGFueSBkZWVwIGluZXF1YWxpdHkuXG4vLyBhc3NlcnQubm90RGVlcEVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2Vfb3B0KTtcblxuYXNzZXJ0Lm5vdERlZXBFcXVhbCA9IGZ1bmN0aW9uIG5vdERlZXBFcXVhbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlKSB7XG4gIGlmIChfZGVlcEVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQpKSB7XG4gICAgZmFpbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlLCAnbm90RGVlcEVxdWFsJywgYXNzZXJ0Lm5vdERlZXBFcXVhbCk7XG4gIH1cbn07XG5cbi8vIDkuIFRoZSBzdHJpY3QgZXF1YWxpdHkgYXNzZXJ0aW9uIHRlc3RzIHN0cmljdCBlcXVhbGl0eSwgYXMgZGV0ZXJtaW5lZCBieSA9PT0uXG4vLyBhc3NlcnQuc3RyaWN0RXF1YWwoYWN0dWFsLCBleHBlY3RlZCwgbWVzc2FnZV9vcHQpO1xuXG5hc3NlcnQuc3RyaWN0RXF1YWwgPSBmdW5jdGlvbiBzdHJpY3RFcXVhbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlKSB7XG4gIGlmIChhY3R1YWwgIT09IGV4cGVjdGVkKSB7XG4gICAgZmFpbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlLCAnPT09JywgYXNzZXJ0LnN0cmljdEVxdWFsKTtcbiAgfVxufTtcblxuLy8gMTAuIFRoZSBzdHJpY3Qgbm9uLWVxdWFsaXR5IGFzc2VydGlvbiB0ZXN0cyBmb3Igc3RyaWN0IGluZXF1YWxpdHksIGFzXG4vLyBkZXRlcm1pbmVkIGJ5ICE9PS4gIGFzc2VydC5ub3RTdHJpY3RFcXVhbChhY3R1YWwsIGV4cGVjdGVkLCBtZXNzYWdlX29wdCk7XG5cbmFzc2VydC5ub3RTdHJpY3RFcXVhbCA9IGZ1bmN0aW9uIG5vdFN0cmljdEVxdWFsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2UpIHtcbiAgaWYgKGFjdHVhbCA9PT0gZXhwZWN0ZWQpIHtcbiAgICBmYWlsKGFjdHVhbCwgZXhwZWN0ZWQsIG1lc3NhZ2UsICchPT0nLCBhc3NlcnQubm90U3RyaWN0RXF1YWwpO1xuICB9XG59O1xuXG5mdW5jdGlvbiBleHBlY3RlZEV4Y2VwdGlvbihhY3R1YWwsIGV4cGVjdGVkKSB7XG4gIGlmICghYWN0dWFsIHx8ICFleHBlY3RlZCkge1xuICAgIHJldHVybiBmYWxzZTtcbiAgfVxuXG4gIGlmIChPYmplY3QucHJvdG90eXBlLnRvU3RyaW5nLmNhbGwoZXhwZWN0ZWQpID09ICdbb2JqZWN0IFJlZ0V4cF0nKSB7XG4gICAgcmV0dXJuIGV4cGVjdGVkLnRlc3QoYWN0dWFsKTtcbiAgfSBlbHNlIGlmIChhY3R1YWwgaW5zdGFuY2VvZiBleHBlY3RlZCkge1xuICAgIHJldHVybiB0cnVlO1xuICB9IGVsc2UgaWYgKGV4cGVjdGVkLmNhbGwoe30sIGFjdHVhbCkgPT09IHRydWUpIHtcbiAgICByZXR1cm4gdHJ1ZTtcbiAgfVxuXG4gIHJldHVybiBmYWxzZTtcbn1cblxuZnVuY3Rpb24gX3Rocm93cyhzaG91bGRUaHJvdywgYmxvY2ssIGV4cGVjdGVkLCBtZXNzYWdlKSB7XG4gIHZhciBhY3R1YWw7XG5cbiAgaWYgKHV0aWwuaXNTdHJpbmcoZXhwZWN0ZWQpKSB7XG4gICAgbWVzc2FnZSA9IGV4cGVjdGVkO1xuICAgIGV4cGVjdGVkID0gbnVsbDtcbiAgfVxuXG4gIHRyeSB7XG4gICAgYmxvY2soKTtcbiAgfSBjYXRjaCAoZSkge1xuICAgIGFjdHVhbCA9IGU7XG4gIH1cblxuICBtZXNzYWdlID0gKGV4cGVjdGVkICYmIGV4cGVjdGVkLm5hbWUgPyAnICgnICsgZXhwZWN0ZWQubmFtZSArICcpLicgOiAnLicpICtcbiAgICAgICAgICAgIChtZXNzYWdlID8gJyAnICsgbWVzc2FnZSA6ICcuJyk7XG5cbiAgaWYgKHNob3VsZFRocm93ICYmICFhY3R1YWwpIHtcbiAgICBmYWlsKGFjdHVhbCwgZXhwZWN0ZWQsICdNaXNzaW5nIGV4cGVjdGVkIGV4Y2VwdGlvbicgKyBtZXNzYWdlKTtcbiAgfVxuXG4gIGlmICghc2hvdWxkVGhyb3cgJiYgZXhwZWN0ZWRFeGNlcHRpb24oYWN0dWFsLCBleHBlY3RlZCkpIHtcbiAgICBmYWlsKGFjdHVhbCwgZXhwZWN0ZWQsICdHb3QgdW53YW50ZWQgZXhjZXB0aW9uJyArIG1lc3NhZ2UpO1xuICB9XG5cbiAgaWYgKChzaG91bGRUaHJvdyAmJiBhY3R1YWwgJiYgZXhwZWN0ZWQgJiZcbiAgICAgICFleHBlY3RlZEV4Y2VwdGlvbihhY3R1YWwsIGV4cGVjdGVkKSkgfHwgKCFzaG91bGRUaHJvdyAmJiBhY3R1YWwpKSB7XG4gICAgdGhyb3cgYWN0dWFsO1xuICB9XG59XG5cbi8vIDExLiBFeHBlY3RlZCB0byB0aHJvdyBhbiBlcnJvcjpcbi8vIGFzc2VydC50aHJvd3MoYmxvY2ssIEVycm9yX29wdCwgbWVzc2FnZV9vcHQpO1xuXG5hc3NlcnQudGhyb3dzID0gZnVuY3Rpb24oYmxvY2ssIC8qb3B0aW9uYWwqL2Vycm9yLCAvKm9wdGlvbmFsKi9tZXNzYWdlKSB7XG4gIF90aHJvd3MuYXBwbHkodGhpcywgW3RydWVdLmNvbmNhdChwU2xpY2UuY2FsbChhcmd1bWVudHMpKSk7XG59O1xuXG4vLyBFWFRFTlNJT04hIFRoaXMgaXMgYW5ub3lpbmcgdG8gd3JpdGUgb3V0c2lkZSB0aGlzIG1vZHVsZS5cbmFzc2VydC5kb2VzTm90VGhyb3cgPSBmdW5jdGlvbihibG9jaywgLypvcHRpb25hbCovbWVzc2FnZSkge1xuICBfdGhyb3dzLmFwcGx5KHRoaXMsIFtmYWxzZV0uY29uY2F0KHBTbGljZS5jYWxsKGFyZ3VtZW50cykpKTtcbn07XG5cbmFzc2VydC5pZkVycm9yID0gZnVuY3Rpb24oZXJyKSB7IGlmIChlcnIpIHt0aHJvdyBlcnI7fX07XG5cbnZhciBvYmplY3RLZXlzID0gT2JqZWN0LmtleXMgfHwgZnVuY3Rpb24gKG9iaikge1xuICB2YXIga2V5cyA9IFtdO1xuICBmb3IgKHZhciBrZXkgaW4gb2JqKSB7XG4gICAgaWYgKGhhc093bi5jYWxsKG9iaiwga2V5KSkga2V5cy5wdXNoKGtleSk7XG4gIH1cbiAgcmV0dXJuIGtleXM7XG59O1xuIiwiaWYgKHR5cGVvZiBPYmplY3QuY3JlYXRlID09PSAnZnVuY3Rpb24nKSB7XG4gIC8vIGltcGxlbWVudGF0aW9uIGZyb20gc3RhbmRhcmQgbm9kZS5qcyAndXRpbCcgbW9kdWxlXG4gIG1vZHVsZS5leHBvcnRzID0gZnVuY3Rpb24gaW5oZXJpdHMoY3Rvciwgc3VwZXJDdG9yKSB7XG4gICAgY3Rvci5zdXBlcl8gPSBzdXBlckN0b3JcbiAgICBjdG9yLnByb3RvdHlwZSA9IE9iamVjdC5jcmVhdGUoc3VwZXJDdG9yLnByb3RvdHlwZSwge1xuICAgICAgY29uc3RydWN0b3I6IHtcbiAgICAgICAgdmFsdWU6IGN0b3IsXG4gICAgICAgIGVudW1lcmFibGU6IGZhbHNlLFxuICAgICAgICB3cml0YWJsZTogdHJ1ZSxcbiAgICAgICAgY29uZmlndXJhYmxlOiB0cnVlXG4gICAgICB9XG4gICAgfSk7XG4gIH07XG59IGVsc2Uge1xuICAvLyBvbGQgc2Nob29sIHNoaW0gZm9yIG9sZCBicm93c2Vyc1xuICBtb2R1bGUuZXhwb3J0cyA9IGZ1bmN0aW9uIGluaGVyaXRzKGN0b3IsIHN1cGVyQ3Rvcikge1xuICAgIGN0b3Iuc3VwZXJfID0gc3VwZXJDdG9yXG4gICAgdmFyIFRlbXBDdG9yID0gZnVuY3Rpb24gKCkge31cbiAgICBUZW1wQ3Rvci5wcm90b3R5cGUgPSBzdXBlckN0b3IucHJvdG90eXBlXG4gICAgY3Rvci5wcm90b3R5cGUgPSBuZXcgVGVtcEN0b3IoKVxuICAgIGN0b3IucHJvdG90eXBlLmNvbnN0cnVjdG9yID0gY3RvclxuICB9XG59XG4iLCJtb2R1bGUuZXhwb3J0cyA9IGZ1bmN0aW9uIGlzQnVmZmVyKGFyZykge1xuICByZXR1cm4gYXJnICYmIHR5cGVvZiBhcmcgPT09ICdvYmplY3QnXG4gICAgJiYgdHlwZW9mIGFyZy5jb3B5ID09PSAnZnVuY3Rpb24nXG4gICAgJiYgdHlwZW9mIGFyZy5maWxsID09PSAnZnVuY3Rpb24nXG4gICAgJiYgdHlwZW9mIGFyZy5yZWFkVUludDggPT09ICdmdW5jdGlvbic7XG59IiwidmFyIHByb2Nlc3M9cmVxdWlyZShcIl9fYnJvd3NlcmlmeV9wcm9jZXNzXCIpLGdsb2JhbD10eXBlb2Ygc2VsZiAhPT0gXCJ1bmRlZmluZWRcIiA/IHNlbGYgOiB0eXBlb2Ygd2luZG93ICE9PSBcInVuZGVmaW5lZFwiID8gd2luZG93IDoge307Ly8gQ29weXJpZ2h0IEpveWVudCwgSW5jLiBhbmQgb3RoZXIgTm9kZSBjb250cmlidXRvcnMuXG4vL1xuLy8gUGVybWlzc2lvbiBpcyBoZXJlYnkgZ3JhbnRlZCwgZnJlZSBvZiBjaGFyZ2UsIHRvIGFueSBwZXJzb24gb2J0YWluaW5nIGFcbi8vIGNvcHkgb2YgdGhpcyBzb2Z0d2FyZSBhbmQgYXNzb2NpYXRlZCBkb2N1bWVudGF0aW9uIGZpbGVzICh0aGVcbi8vIFwiU29mdHdhcmVcIiksIHRvIGRlYWwgaW4gdGhlIFNvZnR3YXJlIHdpdGhvdXQgcmVzdHJpY3Rpb24sIGluY2x1ZGluZ1xuLy8gd2l0aG91dCBsaW1pdGF0aW9uIHRoZSByaWdodHMgdG8gdXNlLCBjb3B5LCBtb2RpZnksIG1lcmdlLCBwdWJsaXNoLFxuLy8gZGlzdHJpYnV0ZSwgc3VibGljZW5zZSwgYW5kL29yIHNlbGwgY29waWVzIG9mIHRoZSBTb2Z0d2FyZSwgYW5kIHRvIHBlcm1pdFxuLy8gcGVyc29ucyB0byB3aG9tIHRoZSBTb2Z0d2FyZSBpcyBmdXJuaXNoZWQgdG8gZG8gc28sIHN1YmplY3QgdG8gdGhlXG4vLyBmb2xsb3dpbmcgY29uZGl0aW9uczpcbi8vXG4vLyBUaGUgYWJvdmUgY29weXJpZ2h0IG5vdGljZSBhbmQgdGhpcyBwZXJtaXNzaW9uIG5vdGljZSBzaGFsbCBiZSBpbmNsdWRlZFxuLy8gaW4gYWxsIGNvcGllcyBvciBzdWJzdGFudGlhbCBwb3J0aW9ucyBvZiB0aGUgU29mdHdhcmUuXG4vL1xuLy8gVEhFIFNPRlRXQVJFIElTIFBST1ZJREVEIFwiQVMgSVNcIiwgV0lUSE9VVCBXQVJSQU5UWSBPRiBBTlkgS0lORCwgRVhQUkVTU1xuLy8gT1IgSU1QTElFRCwgSU5DTFVESU5HIEJVVCBOT1QgTElNSVRFRCBUTyBUSEUgV0FSUkFOVElFUyBPRlxuLy8gTUVSQ0hBTlRBQklMSVRZLCBGSVRORVNTIEZPUiBBIFBBUlRJQ1VMQVIgUFVSUE9TRSBBTkQgTk9OSU5GUklOR0VNRU5ULiBJTlxuLy8gTk8gRVZFTlQgU0hBTEwgVEhFIEFVVEhPUlMgT1IgQ09QWVJJR0hUIEhPTERFUlMgQkUgTElBQkxFIEZPUiBBTlkgQ0xBSU0sXG4vLyBEQU1BR0VTIE9SIE9USEVSIExJQUJJTElUWSwgV0hFVEhFUiBJTiBBTiBBQ1RJT04gT0YgQ09OVFJBQ1QsIFRPUlQgT1Jcbi8vIE9USEVSV0lTRSwgQVJJU0lORyBGUk9NLCBPVVQgT0YgT1IgSU4gQ09OTkVDVElPTiBXSVRIIFRIRSBTT0ZUV0FSRSBPUiBUSEVcbi8vIFVTRSBPUiBPVEhFUiBERUFMSU5HUyBJTiBUSEUgU09GVFdBUkUuXG5cbnZhciBmb3JtYXRSZWdFeHAgPSAvJVtzZGolXS9nO1xuZXhwb3J0cy5mb3JtYXQgPSBmdW5jdGlvbihmKSB7XG4gIGlmICghaXNTdHJpbmcoZikpIHtcbiAgICB2YXIgb2JqZWN0cyA9IFtdO1xuICAgIGZvciAodmFyIGkgPSAwOyBpIDwgYXJndW1lbnRzLmxlbmd0aDsgaSsrKSB7XG4gICAgICBvYmplY3RzLnB1c2goaW5zcGVjdChhcmd1bWVudHNbaV0pKTtcbiAgICB9XG4gICAgcmV0dXJuIG9iamVjdHMuam9pbignICcpO1xuICB9XG5cbiAgdmFyIGkgPSAxO1xuICB2YXIgYXJncyA9IGFyZ3VtZW50cztcbiAgdmFyIGxlbiA9IGFyZ3MubGVuZ3RoO1xuICB2YXIgc3RyID0gU3RyaW5nKGYpLnJlcGxhY2UoZm9ybWF0UmVnRXhwLCBmdW5jdGlvbih4KSB7XG4gICAgaWYgKHggPT09ICclJScpIHJldHVybiAnJSc7XG4gICAgaWYgKGkgPj0gbGVuKSByZXR1cm4geDtcbiAgICBzd2l0Y2ggKHgpIHtcbiAgICAgIGNhc2UgJyVzJzogcmV0dXJuIFN0cmluZyhhcmdzW2krK10pO1xuICAgICAgY2FzZSAnJWQnOiByZXR1cm4gTnVtYmVyKGFyZ3NbaSsrXSk7XG4gICAgICBjYXNlICclaic6XG4gICAgICAgIHRyeSB7XG4gICAgICAgICAgcmV0dXJuIEpTT04uc3RyaW5naWZ5KGFyZ3NbaSsrXSk7XG4gICAgICAgIH0gY2F0Y2ggKF8pIHtcbiAgICAgICAgICByZXR1cm4gJ1tDaXJjdWxhcl0nO1xuICAgICAgICB9XG4gICAgICBkZWZhdWx0OlxuICAgICAgICByZXR1cm4geDtcbiAgICB9XG4gIH0pO1xuICBmb3IgKHZhciB4ID0gYXJnc1tpXTsgaSA8IGxlbjsgeCA9IGFyZ3NbKytpXSkge1xuICAgIGlmIChpc051bGwoeCkgfHwgIWlzT2JqZWN0KHgpKSB7XG4gICAgICBzdHIgKz0gJyAnICsgeDtcbiAgICB9IGVsc2Uge1xuICAgICAgc3RyICs9ICcgJyArIGluc3BlY3QoeCk7XG4gICAgfVxuICB9XG4gIHJldHVybiBzdHI7XG59O1xuXG5cbi8vIE1hcmsgdGhhdCBhIG1ldGhvZCBzaG91bGQgbm90IGJlIHVzZWQuXG4vLyBSZXR1cm5zIGEgbW9kaWZpZWQgZnVuY3Rpb24gd2hpY2ggd2FybnMgb25jZSBieSBkZWZhdWx0LlxuLy8gSWYgLS1uby1kZXByZWNhdGlvbiBpcyBzZXQsIHRoZW4gaXQgaXMgYSBuby1vcC5cbmV4cG9ydHMuZGVwcmVjYXRlID0gZnVuY3Rpb24oZm4sIG1zZykge1xuICAvLyBBbGxvdyBmb3IgZGVwcmVjYXRpbmcgdGhpbmdzIGluIHRoZSBwcm9jZXNzIG9mIHN0YXJ0aW5nIHVwLlxuICBpZiAoaXNVbmRlZmluZWQoZ2xvYmFsLnByb2Nlc3MpKSB7XG4gICAgcmV0dXJuIGZ1bmN0aW9uKCkge1xuICAgICAgcmV0dXJuIGV4cG9ydHMuZGVwcmVjYXRlKGZuLCBtc2cpLmFwcGx5KHRoaXMsIGFyZ3VtZW50cyk7XG4gICAgfTtcbiAgfVxuXG4gIGlmIChwcm9jZXNzLm5vRGVwcmVjYXRpb24gPT09IHRydWUpIHtcbiAgICByZXR1cm4gZm47XG4gIH1cblxuICB2YXIgd2FybmVkID0gZmFsc2U7XG4gIGZ1bmN0aW9uIGRlcHJlY2F0ZWQoKSB7XG4gICAgaWYgKCF3YXJuZWQpIHtcbiAgICAgIGlmIChwcm9jZXNzLnRocm93RGVwcmVjYXRpb24pIHtcbiAgICAgICAgdGhyb3cgbmV3IEVycm9yKG1zZyk7XG4gICAgICB9IGVsc2UgaWYgKHByb2Nlc3MudHJhY2VEZXByZWNhdGlvbikge1xuICAgICAgICBjb25zb2xlLnRyYWNlKG1zZyk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBjb25zb2xlLmVycm9yKG1zZyk7XG4gICAgICB9XG4gICAgICB3YXJuZWQgPSB0cnVlO1xuICAgIH1cbiAgICByZXR1cm4gZm4uYXBwbHkodGhpcywgYXJndW1lbnRzKTtcbiAgfVxuXG4gIHJldHVybiBkZXByZWNhdGVkO1xufTtcblxuXG52YXIgZGVidWdzID0ge307XG52YXIgZGVidWdFbnZpcm9uO1xuZXhwb3J0cy5kZWJ1Z2xvZyA9IGZ1bmN0aW9uKHNldCkge1xuICBpZiAoaXNVbmRlZmluZWQoZGVidWdFbnZpcm9uKSlcbiAgICBkZWJ1Z0Vudmlyb24gPSBwcm9jZXNzLmVudi5OT0RFX0RFQlVHIHx8ICcnO1xuICBzZXQgPSBzZXQudG9VcHBlckNhc2UoKTtcbiAgaWYgKCFkZWJ1Z3Nbc2V0XSkge1xuICAgIGlmIChuZXcgUmVnRXhwKCdcXFxcYicgKyBzZXQgKyAnXFxcXGInLCAnaScpLnRlc3QoZGVidWdFbnZpcm9uKSkge1xuICAgICAgdmFyIHBpZCA9IHByb2Nlc3MucGlkO1xuICAgICAgZGVidWdzW3NldF0gPSBmdW5jdGlvbigpIHtcbiAgICAgICAgdmFyIG1zZyA9IGV4cG9ydHMuZm9ybWF0LmFwcGx5KGV4cG9ydHMsIGFyZ3VtZW50cyk7XG4gICAgICAgIGNvbnNvbGUuZXJyb3IoJyVzICVkOiAlcycsIHNldCwgcGlkLCBtc2cpO1xuICAgICAgfTtcbiAgICB9IGVsc2Uge1xuICAgICAgZGVidWdzW3NldF0gPSBmdW5jdGlvbigpIHt9O1xuICAgIH1cbiAgfVxuICByZXR1cm4gZGVidWdzW3NldF07XG59O1xuXG5cbi8qKlxuICogRWNob3MgdGhlIHZhbHVlIG9mIGEgdmFsdWUuIFRyeXMgdG8gcHJpbnQgdGhlIHZhbHVlIG91dFxuICogaW4gdGhlIGJlc3Qgd2F5IHBvc3NpYmxlIGdpdmVuIHRoZSBkaWZmZXJlbnQgdHlwZXMuXG4gKlxuICogQHBhcmFtIHtPYmplY3R9IG9iaiBUaGUgb2JqZWN0IHRvIHByaW50IG91dC5cbiAqIEBwYXJhbSB7T2JqZWN0fSBvcHRzIE9wdGlvbmFsIG9wdGlvbnMgb2JqZWN0IHRoYXQgYWx0ZXJzIHRoZSBvdXRwdXQuXG4gKi9cbi8qIGxlZ2FjeTogb2JqLCBzaG93SGlkZGVuLCBkZXB0aCwgY29sb3JzKi9cbmZ1bmN0aW9uIGluc3BlY3Qob2JqLCBvcHRzKSB7XG4gIC8vIGRlZmF1bHQgb3B0aW9uc1xuICB2YXIgY3R4ID0ge1xuICAgIHNlZW46IFtdLFxuICAgIHN0eWxpemU6IHN0eWxpemVOb0NvbG9yXG4gIH07XG4gIC8vIGxlZ2FjeS4uLlxuICBpZiAoYXJndW1lbnRzLmxlbmd0aCA+PSAzKSBjdHguZGVwdGggPSBhcmd1bWVudHNbMl07XG4gIGlmIChhcmd1bWVudHMubGVuZ3RoID49IDQpIGN0eC5jb2xvcnMgPSBhcmd1bWVudHNbM107XG4gIGlmIChpc0Jvb2xlYW4ob3B0cykpIHtcbiAgICAvLyBsZWdhY3kuLi5cbiAgICBjdHguc2hvd0hpZGRlbiA9IG9wdHM7XG4gIH0gZWxzZSBpZiAob3B0cykge1xuICAgIC8vIGdvdCBhbiBcIm9wdGlvbnNcIiBvYmplY3RcbiAgICBleHBvcnRzLl9leHRlbmQoY3R4LCBvcHRzKTtcbiAgfVxuICAvLyBzZXQgZGVmYXVsdCBvcHRpb25zXG4gIGlmIChpc1VuZGVmaW5lZChjdHguc2hvd0hpZGRlbikpIGN0eC5zaG93SGlkZGVuID0gZmFsc2U7XG4gIGlmIChpc1VuZGVmaW5lZChjdHguZGVwdGgpKSBjdHguZGVwdGggPSAyO1xuICBpZiAoaXNVbmRlZmluZWQoY3R4LmNvbG9ycykpIGN0eC5jb2xvcnMgPSBmYWxzZTtcbiAgaWYgKGlzVW5kZWZpbmVkKGN0eC5jdXN0b21JbnNwZWN0KSkgY3R4LmN1c3RvbUluc3BlY3QgPSB0cnVlO1xuICBpZiAoY3R4LmNvbG9ycykgY3R4LnN0eWxpemUgPSBzdHlsaXplV2l0aENvbG9yO1xuICByZXR1cm4gZm9ybWF0VmFsdWUoY3R4LCBvYmosIGN0eC5kZXB0aCk7XG59XG5leHBvcnRzLmluc3BlY3QgPSBpbnNwZWN0O1xuXG5cbi8vIGh0dHA6Ly9lbi53aWtpcGVkaWEub3JnL3dpa2kvQU5TSV9lc2NhcGVfY29kZSNncmFwaGljc1xuaW5zcGVjdC5jb2xvcnMgPSB7XG4gICdib2xkJyA6IFsxLCAyMl0sXG4gICdpdGFsaWMnIDogWzMsIDIzXSxcbiAgJ3VuZGVybGluZScgOiBbNCwgMjRdLFxuICAnaW52ZXJzZScgOiBbNywgMjddLFxuICAnd2hpdGUnIDogWzM3LCAzOV0sXG4gICdncmV5JyA6IFs5MCwgMzldLFxuICAnYmxhY2snIDogWzMwLCAzOV0sXG4gICdibHVlJyA6IFszNCwgMzldLFxuICAnY3lhbicgOiBbMzYsIDM5XSxcbiAgJ2dyZWVuJyA6IFszMiwgMzldLFxuICAnbWFnZW50YScgOiBbMzUsIDM5XSxcbiAgJ3JlZCcgOiBbMzEsIDM5XSxcbiAgJ3llbGxvdycgOiBbMzMsIDM5XVxufTtcblxuLy8gRG9uJ3QgdXNlICdibHVlJyBub3QgdmlzaWJsZSBvbiBjbWQuZXhlXG5pbnNwZWN0LnN0eWxlcyA9IHtcbiAgJ3NwZWNpYWwnOiAnY3lhbicsXG4gICdudW1iZXInOiAneWVsbG93JyxcbiAgJ2Jvb2xlYW4nOiAneWVsbG93JyxcbiAgJ3VuZGVmaW5lZCc6ICdncmV5JyxcbiAgJ251bGwnOiAnYm9sZCcsXG4gICdzdHJpbmcnOiAnZ3JlZW4nLFxuICAnZGF0ZSc6ICdtYWdlbnRhJyxcbiAgLy8gXCJuYW1lXCI6IGludGVudGlvbmFsbHkgbm90IHN0eWxpbmdcbiAgJ3JlZ2V4cCc6ICdyZWQnXG59O1xuXG5cbmZ1bmN0aW9uIHN0eWxpemVXaXRoQ29sb3Ioc3RyLCBzdHlsZVR5cGUpIHtcbiAgdmFyIHN0eWxlID0gaW5zcGVjdC5zdHlsZXNbc3R5bGVUeXBlXTtcblxuICBpZiAoc3R5bGUpIHtcbiAgICByZXR1cm4gJ1xcdTAwMWJbJyArIGluc3BlY3QuY29sb3JzW3N0eWxlXVswXSArICdtJyArIHN0ciArXG4gICAgICAgICAgICdcXHUwMDFiWycgKyBpbnNwZWN0LmNvbG9yc1tzdHlsZV1bMV0gKyAnbSc7XG4gIH0gZWxzZSB7XG4gICAgcmV0dXJuIHN0cjtcbiAgfVxufVxuXG5cbmZ1bmN0aW9uIHN0eWxpemVOb0NvbG9yKHN0ciwgc3R5bGVUeXBlKSB7XG4gIHJldHVybiBzdHI7XG59XG5cblxuZnVuY3Rpb24gYXJyYXlUb0hhc2goYXJyYXkpIHtcbiAgdmFyIGhhc2ggPSB7fTtcblxuICBhcnJheS5mb3JFYWNoKGZ1bmN0aW9uKHZhbCwgaWR4KSB7XG4gICAgaGFzaFt2YWxdID0gdHJ1ZTtcbiAgfSk7XG5cbiAgcmV0dXJuIGhhc2g7XG59XG5cblxuZnVuY3Rpb24gZm9ybWF0VmFsdWUoY3R4LCB2YWx1ZSwgcmVjdXJzZVRpbWVzKSB7XG4gIC8vIFByb3ZpZGUgYSBob29rIGZvciB1c2VyLXNwZWNpZmllZCBpbnNwZWN0IGZ1bmN0aW9ucy5cbiAgLy8gQ2hlY2sgdGhhdCB2YWx1ZSBpcyBhbiBvYmplY3Qgd2l0aCBhbiBpbnNwZWN0IGZ1bmN0aW9uIG9uIGl0XG4gIGlmIChjdHguY3VzdG9tSW5zcGVjdCAmJlxuICAgICAgdmFsdWUgJiZcbiAgICAgIGlzRnVuY3Rpb24odmFsdWUuaW5zcGVjdCkgJiZcbiAgICAgIC8vIEZpbHRlciBvdXQgdGhlIHV0aWwgbW9kdWxlLCBpdCdzIGluc3BlY3QgZnVuY3Rpb24gaXMgc3BlY2lhbFxuICAgICAgdmFsdWUuaW5zcGVjdCAhPT0gZXhwb3J0cy5pbnNwZWN0ICYmXG4gICAgICAvLyBBbHNvIGZpbHRlciBvdXQgYW55IHByb3RvdHlwZSBvYmplY3RzIHVzaW5nIHRoZSBjaXJjdWxhciBjaGVjay5cbiAgICAgICEodmFsdWUuY29uc3RydWN0b3IgJiYgdmFsdWUuY29uc3RydWN0b3IucHJvdG90eXBlID09PSB2YWx1ZSkpIHtcbiAgICB2YXIgcmV0ID0gdmFsdWUuaW5zcGVjdChyZWN1cnNlVGltZXMsIGN0eCk7XG4gICAgaWYgKCFpc1N0cmluZyhyZXQpKSB7XG4gICAgICByZXQgPSBmb3JtYXRWYWx1ZShjdHgsIHJldCwgcmVjdXJzZVRpbWVzKTtcbiAgICB9XG4gICAgcmV0dXJuIHJldDtcbiAgfVxuXG4gIC8vIFByaW1pdGl2ZSB0eXBlcyBjYW5ub3QgaGF2ZSBwcm9wZXJ0aWVzXG4gIHZhciBwcmltaXRpdmUgPSBmb3JtYXRQcmltaXRpdmUoY3R4LCB2YWx1ZSk7XG4gIGlmIChwcmltaXRpdmUpIHtcbiAgICByZXR1cm4gcHJpbWl0aXZlO1xuICB9XG5cbiAgLy8gTG9vayB1cCB0aGUga2V5cyBvZiB0aGUgb2JqZWN0LlxuICB2YXIga2V5cyA9IE9iamVjdC5rZXlzKHZhbHVlKTtcbiAgdmFyIHZpc2libGVLZXlzID0gYXJyYXlUb0hhc2goa2V5cyk7XG5cbiAgaWYgKGN0eC5zaG93SGlkZGVuKSB7XG4gICAga2V5cyA9IE9iamVjdC5nZXRPd25Qcm9wZXJ0eU5hbWVzKHZhbHVlKTtcbiAgfVxuXG4gIC8vIElFIGRvZXNuJ3QgbWFrZSBlcnJvciBmaWVsZHMgbm9uLWVudW1lcmFibGVcbiAgLy8gaHR0cDovL21zZG4ubWljcm9zb2Z0LmNvbS9lbi11cy9saWJyYXJ5L2llL2R3dzUyc2J0KHY9dnMuOTQpLmFzcHhcbiAgaWYgKGlzRXJyb3IodmFsdWUpXG4gICAgICAmJiAoa2V5cy5pbmRleE9mKCdtZXNzYWdlJykgPj0gMCB8fCBrZXlzLmluZGV4T2YoJ2Rlc2NyaXB0aW9uJykgPj0gMCkpIHtcbiAgICByZXR1cm4gZm9ybWF0RXJyb3IodmFsdWUpO1xuICB9XG5cbiAgLy8gU29tZSB0eXBlIG9mIG9iamVjdCB3aXRob3V0IHByb3BlcnRpZXMgY2FuIGJlIHNob3J0Y3V0dGVkLlxuICBpZiAoa2V5cy5sZW5ndGggPT09IDApIHtcbiAgICBpZiAoaXNGdW5jdGlvbih2YWx1ZSkpIHtcbiAgICAgIHZhciBuYW1lID0gdmFsdWUubmFtZSA/ICc6ICcgKyB2YWx1ZS5uYW1lIDogJyc7XG4gICAgICByZXR1cm4gY3R4LnN0eWxpemUoJ1tGdW5jdGlvbicgKyBuYW1lICsgJ10nLCAnc3BlY2lhbCcpO1xuICAgIH1cbiAgICBpZiAoaXNSZWdFeHAodmFsdWUpKSB7XG4gICAgICByZXR1cm4gY3R4LnN0eWxpemUoUmVnRXhwLnByb3RvdHlwZS50b1N0cmluZy5jYWxsKHZhbHVlKSwgJ3JlZ2V4cCcpO1xuICAgIH1cbiAgICBpZiAoaXNEYXRlKHZhbHVlKSkge1xuICAgICAgcmV0dXJuIGN0eC5zdHlsaXplKERhdGUucHJvdG90eXBlLnRvU3RyaW5nLmNhbGwodmFsdWUpLCAnZGF0ZScpO1xuICAgIH1cbiAgICBpZiAoaXNFcnJvcih2YWx1ZSkpIHtcbiAgICAgIHJldHVybiBmb3JtYXRFcnJvcih2YWx1ZSk7XG4gICAgfVxuICB9XG5cbiAgdmFyIGJhc2UgPSAnJywgYXJyYXkgPSBmYWxzZSwgYnJhY2VzID0gWyd7JywgJ30nXTtcblxuICAvLyBNYWtlIEFycmF5IHNheSB0aGF0IHRoZXkgYXJlIEFycmF5XG4gIGlmIChpc0FycmF5KHZhbHVlKSkge1xuICAgIGFycmF5ID0gdHJ1ZTtcbiAgICBicmFjZXMgPSBbJ1snLCAnXSddO1xuICB9XG5cbiAgLy8gTWFrZSBmdW5jdGlvbnMgc2F5IHRoYXQgdGhleSBhcmUgZnVuY3Rpb25zXG4gIGlmIChpc0Z1bmN0aW9uKHZhbHVlKSkge1xuICAgIHZhciBuID0gdmFsdWUubmFtZSA/ICc6ICcgKyB2YWx1ZS5uYW1lIDogJyc7XG4gICAgYmFzZSA9ICcgW0Z1bmN0aW9uJyArIG4gKyAnXSc7XG4gIH1cblxuICAvLyBNYWtlIFJlZ0V4cHMgc2F5IHRoYXQgdGhleSBhcmUgUmVnRXhwc1xuICBpZiAoaXNSZWdFeHAodmFsdWUpKSB7XG4gICAgYmFzZSA9ICcgJyArIFJlZ0V4cC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbCh2YWx1ZSk7XG4gIH1cblxuICAvLyBNYWtlIGRhdGVzIHdpdGggcHJvcGVydGllcyBmaXJzdCBzYXkgdGhlIGRhdGVcbiAgaWYgKGlzRGF0ZSh2YWx1ZSkpIHtcbiAgICBiYXNlID0gJyAnICsgRGF0ZS5wcm90b3R5cGUudG9VVENTdHJpbmcuY2FsbCh2YWx1ZSk7XG4gIH1cblxuICAvLyBNYWtlIGVycm9yIHdpdGggbWVzc2FnZSBmaXJzdCBzYXkgdGhlIGVycm9yXG4gIGlmIChpc0Vycm9yKHZhbHVlKSkge1xuICAgIGJhc2UgPSAnICcgKyBmb3JtYXRFcnJvcih2YWx1ZSk7XG4gIH1cblxuICBpZiAoa2V5cy5sZW5ndGggPT09IDAgJiYgKCFhcnJheSB8fCB2YWx1ZS5sZW5ndGggPT0gMCkpIHtcbiAgICByZXR1cm4gYnJhY2VzWzBdICsgYmFzZSArIGJyYWNlc1sxXTtcbiAgfVxuXG4gIGlmIChyZWN1cnNlVGltZXMgPCAwKSB7XG4gICAgaWYgKGlzUmVnRXhwKHZhbHVlKSkge1xuICAgICAgcmV0dXJuIGN0eC5zdHlsaXplKFJlZ0V4cC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbCh2YWx1ZSksICdyZWdleHAnKTtcbiAgICB9IGVsc2Uge1xuICAgICAgcmV0dXJuIGN0eC5zdHlsaXplKCdbT2JqZWN0XScsICdzcGVjaWFsJyk7XG4gICAgfVxuICB9XG5cbiAgY3R4LnNlZW4ucHVzaCh2YWx1ZSk7XG5cbiAgdmFyIG91dHB1dDtcbiAgaWYgKGFycmF5KSB7XG4gICAgb3V0cHV0ID0gZm9ybWF0QXJyYXkoY3R4LCB2YWx1ZSwgcmVjdXJzZVRpbWVzLCB2aXNpYmxlS2V5cywga2V5cyk7XG4gIH0gZWxzZSB7XG4gICAgb3V0cHV0ID0ga2V5cy5tYXAoZnVuY3Rpb24oa2V5KSB7XG4gICAgICByZXR1cm4gZm9ybWF0UHJvcGVydHkoY3R4LCB2YWx1ZSwgcmVjdXJzZVRpbWVzLCB2aXNpYmxlS2V5cywga2V5LCBhcnJheSk7XG4gICAgfSk7XG4gIH1cblxuICBjdHguc2Vlbi5wb3AoKTtcblxuICByZXR1cm4gcmVkdWNlVG9TaW5nbGVTdHJpbmcob3V0cHV0LCBiYXNlLCBicmFjZXMpO1xufVxuXG5cbmZ1bmN0aW9uIGZvcm1hdFByaW1pdGl2ZShjdHgsIHZhbHVlKSB7XG4gIGlmIChpc1VuZGVmaW5lZCh2YWx1ZSkpXG4gICAgcmV0dXJuIGN0eC5zdHlsaXplKCd1bmRlZmluZWQnLCAndW5kZWZpbmVkJyk7XG4gIGlmIChpc1N0cmluZyh2YWx1ZSkpIHtcbiAgICB2YXIgc2ltcGxlID0gJ1xcJycgKyBKU09OLnN0cmluZ2lmeSh2YWx1ZSkucmVwbGFjZSgvXlwifFwiJC9nLCAnJylcbiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIC5yZXBsYWNlKC8nL2csIFwiXFxcXCdcIilcbiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIC5yZXBsYWNlKC9cXFxcXCIvZywgJ1wiJykgKyAnXFwnJztcbiAgICByZXR1cm4gY3R4LnN0eWxpemUoc2ltcGxlLCAnc3RyaW5nJyk7XG4gIH1cbiAgaWYgKGlzTnVtYmVyKHZhbHVlKSlcbiAgICByZXR1cm4gY3R4LnN0eWxpemUoJycgKyB2YWx1ZSwgJ251bWJlcicpO1xuICBpZiAoaXNCb29sZWFuKHZhbHVlKSlcbiAgICByZXR1cm4gY3R4LnN0eWxpemUoJycgKyB2YWx1ZSwgJ2Jvb2xlYW4nKTtcbiAgLy8gRm9yIHNvbWUgcmVhc29uIHR5cGVvZiBudWxsIGlzIFwib2JqZWN0XCIsIHNvIHNwZWNpYWwgY2FzZSBoZXJlLlxuICBpZiAoaXNOdWxsKHZhbHVlKSlcbiAgICByZXR1cm4gY3R4LnN0eWxpemUoJ251bGwnLCAnbnVsbCcpO1xufVxuXG5cbmZ1bmN0aW9uIGZvcm1hdEVycm9yKHZhbHVlKSB7XG4gIHJldHVybiAnWycgKyBFcnJvci5wcm90b3R5cGUudG9TdHJpbmcuY2FsbCh2YWx1ZSkgKyAnXSc7XG59XG5cblxuZnVuY3Rpb24gZm9ybWF0QXJyYXkoY3R4LCB2YWx1ZSwgcmVjdXJzZVRpbWVzLCB2aXNpYmxlS2V5cywga2V5cykge1xuICB2YXIgb3V0cHV0ID0gW107XG4gIGZvciAodmFyIGkgPSAwLCBsID0gdmFsdWUubGVuZ3RoOyBpIDwgbDsgKytpKSB7XG4gICAgaWYgKGhhc093blByb3BlcnR5KHZhbHVlLCBTdHJpbmcoaSkpKSB7XG4gICAgICBvdXRwdXQucHVzaChmb3JtYXRQcm9wZXJ0eShjdHgsIHZhbHVlLCByZWN1cnNlVGltZXMsIHZpc2libGVLZXlzLFxuICAgICAgICAgIFN0cmluZyhpKSwgdHJ1ZSkpO1xuICAgIH0gZWxzZSB7XG4gICAgICBvdXRwdXQucHVzaCgnJyk7XG4gICAgfVxuICB9XG4gIGtleXMuZm9yRWFjaChmdW5jdGlvbihrZXkpIHtcbiAgICBpZiAoIWtleS5tYXRjaCgvXlxcZCskLykpIHtcbiAgICAgIG91dHB1dC5wdXNoKGZvcm1hdFByb3BlcnR5KGN0eCwgdmFsdWUsIHJlY3Vyc2VUaW1lcywgdmlzaWJsZUtleXMsXG4gICAgICAgICAga2V5LCB0cnVlKSk7XG4gICAgfVxuICB9KTtcbiAgcmV0dXJuIG91dHB1dDtcbn1cblxuXG5mdW5jdGlvbiBmb3JtYXRQcm9wZXJ0eShjdHgsIHZhbHVlLCByZWN1cnNlVGltZXMsIHZpc2libGVLZXlzLCBrZXksIGFycmF5KSB7XG4gIHZhciBuYW1lLCBzdHIsIGRlc2M7XG4gIGRlc2MgPSBPYmplY3QuZ2V0T3duUHJvcGVydHlEZXNjcmlwdG9yKHZhbHVlLCBrZXkpIHx8IHsgdmFsdWU6IHZhbHVlW2tleV0gfTtcbiAgaWYgKGRlc2MuZ2V0KSB7XG4gICAgaWYgKGRlc2Muc2V0KSB7XG4gICAgICBzdHIgPSBjdHguc3R5bGl6ZSgnW0dldHRlci9TZXR0ZXJdJywgJ3NwZWNpYWwnKTtcbiAgICB9IGVsc2Uge1xuICAgICAgc3RyID0gY3R4LnN0eWxpemUoJ1tHZXR0ZXJdJywgJ3NwZWNpYWwnKTtcbiAgICB9XG4gIH0gZWxzZSB7XG4gICAgaWYgKGRlc2Muc2V0KSB7XG4gICAgICBzdHIgPSBjdHguc3R5bGl6ZSgnW1NldHRlcl0nLCAnc3BlY2lhbCcpO1xuICAgIH1cbiAgfVxuICBpZiAoIWhhc093blByb3BlcnR5KHZpc2libGVLZXlzLCBrZXkpKSB7XG4gICAgbmFtZSA9ICdbJyArIGtleSArICddJztcbiAgfVxuICBpZiAoIXN0cikge1xuICAgIGlmIChjdHguc2Vlbi5pbmRleE9mKGRlc2MudmFsdWUpIDwgMCkge1xuICAgICAgaWYgKGlzTnVsbChyZWN1cnNlVGltZXMpKSB7XG4gICAgICAgIHN0ciA9IGZvcm1hdFZhbHVlKGN0eCwgZGVzYy52YWx1ZSwgbnVsbCk7XG4gICAgICB9IGVsc2Uge1xuICAgICAgICBzdHIgPSBmb3JtYXRWYWx1ZShjdHgsIGRlc2MudmFsdWUsIHJlY3Vyc2VUaW1lcyAtIDEpO1xuICAgICAgfVxuICAgICAgaWYgKHN0ci5pbmRleE9mKCdcXG4nKSA+IC0xKSB7XG4gICAgICAgIGlmIChhcnJheSkge1xuICAgICAgICAgIHN0ciA9IHN0ci5zcGxpdCgnXFxuJykubWFwKGZ1bmN0aW9uKGxpbmUpIHtcbiAgICAgICAgICAgIHJldHVybiAnICAnICsgbGluZTtcbiAgICAgICAgICB9KS5qb2luKCdcXG4nKS5zdWJzdHIoMik7XG4gICAgICAgIH0gZWxzZSB7XG4gICAgICAgICAgc3RyID0gJ1xcbicgKyBzdHIuc3BsaXQoJ1xcbicpLm1hcChmdW5jdGlvbihsaW5lKSB7XG4gICAgICAgICAgICByZXR1cm4gJyAgICcgKyBsaW5lO1xuICAgICAgICAgIH0pLmpvaW4oJ1xcbicpO1xuICAgICAgICB9XG4gICAgICB9XG4gICAgfSBlbHNlIHtcbiAgICAgIHN0ciA9IGN0eC5zdHlsaXplKCdbQ2lyY3VsYXJdJywgJ3NwZWNpYWwnKTtcbiAgICB9XG4gIH1cbiAgaWYgKGlzVW5kZWZpbmVkKG5hbWUpKSB7XG4gICAgaWYgKGFycmF5ICYmIGtleS5tYXRjaCgvXlxcZCskLykpIHtcbiAgICAgIHJldHVybiBzdHI7XG4gICAgfVxuICAgIG5hbWUgPSBKU09OLnN0cmluZ2lmeSgnJyArIGtleSk7XG4gICAgaWYgKG5hbWUubWF0Y2goL15cIihbYS16QS1aX11bYS16QS1aXzAtOV0qKVwiJC8pKSB7XG4gICAgICBuYW1lID0gbmFtZS5zdWJzdHIoMSwgbmFtZS5sZW5ndGggLSAyKTtcbiAgICAgIG5hbWUgPSBjdHguc3R5bGl6ZShuYW1lLCAnbmFtZScpO1xuICAgIH0gZWxzZSB7XG4gICAgICBuYW1lID0gbmFtZS5yZXBsYWNlKC8nL2csIFwiXFxcXCdcIilcbiAgICAgICAgICAgICAgICAgLnJlcGxhY2UoL1xcXFxcIi9nLCAnXCInKVxuICAgICAgICAgICAgICAgICAucmVwbGFjZSgvKF5cInxcIiQpL2csIFwiJ1wiKTtcbiAgICAgIG5hbWUgPSBjdHguc3R5bGl6ZShuYW1lLCAnc3RyaW5nJyk7XG4gICAgfVxuICB9XG5cbiAgcmV0dXJuIG5hbWUgKyAnOiAnICsgc3RyO1xufVxuXG5cbmZ1bmN0aW9uIHJlZHVjZVRvU2luZ2xlU3RyaW5nKG91dHB1dCwgYmFzZSwgYnJhY2VzKSB7XG4gIHZhciBudW1MaW5lc0VzdCA9IDA7XG4gIHZhciBsZW5ndGggPSBvdXRwdXQucmVkdWNlKGZ1bmN0aW9uKHByZXYsIGN1cikge1xuICAgIG51bUxpbmVzRXN0Kys7XG4gICAgaWYgKGN1ci5pbmRleE9mKCdcXG4nKSA+PSAwKSBudW1MaW5lc0VzdCsrO1xuICAgIHJldHVybiBwcmV2ICsgY3VyLnJlcGxhY2UoL1xcdTAwMWJcXFtcXGRcXGQ/bS9nLCAnJykubGVuZ3RoICsgMTtcbiAgfSwgMCk7XG5cbiAgaWYgKGxlbmd0aCA+IDYwKSB7XG4gICAgcmV0dXJuIGJyYWNlc1swXSArXG4gICAgICAgICAgIChiYXNlID09PSAnJyA/ICcnIDogYmFzZSArICdcXG4gJykgK1xuICAgICAgICAgICAnICcgK1xuICAgICAgICAgICBvdXRwdXQuam9pbignLFxcbiAgJykgK1xuICAgICAgICAgICAnICcgK1xuICAgICAgICAgICBicmFjZXNbMV07XG4gIH1cblxuICByZXR1cm4gYnJhY2VzWzBdICsgYmFzZSArICcgJyArIG91dHB1dC5qb2luKCcsICcpICsgJyAnICsgYnJhY2VzWzFdO1xufVxuXG5cbi8vIE5PVEU6IFRoZXNlIHR5cGUgY2hlY2tpbmcgZnVuY3Rpb25zIGludGVudGlvbmFsbHkgZG9uJ3QgdXNlIGBpbnN0YW5jZW9mYFxuLy8gYmVjYXVzZSBpdCBpcyBmcmFnaWxlIGFuZCBjYW4gYmUgZWFzaWx5IGZha2VkIHdpdGggYE9iamVjdC5jcmVhdGUoKWAuXG5mdW5jdGlvbiBpc0FycmF5KGFyKSB7XG4gIHJldHVybiBBcnJheS5pc0FycmF5KGFyKTtcbn1cbmV4cG9ydHMuaXNBcnJheSA9IGlzQXJyYXk7XG5cbmZ1bmN0aW9uIGlzQm9vbGVhbihhcmcpIHtcbiAgcmV0dXJuIHR5cGVvZiBhcmcgPT09ICdib29sZWFuJztcbn1cbmV4cG9ydHMuaXNCb29sZWFuID0gaXNCb29sZWFuO1xuXG5mdW5jdGlvbiBpc051bGwoYXJnKSB7XG4gIHJldHVybiBhcmcgPT09IG51bGw7XG59XG5leHBvcnRzLmlzTnVsbCA9IGlzTnVsbDtcblxuZnVuY3Rpb24gaXNOdWxsT3JVbmRlZmluZWQoYXJnKSB7XG4gIHJldHVybiBhcmcgPT0gbnVsbDtcbn1cbmV4cG9ydHMuaXNOdWxsT3JVbmRlZmluZWQgPSBpc051bGxPclVuZGVmaW5lZDtcblxuZnVuY3Rpb24gaXNOdW1iZXIoYXJnKSB7XG4gIHJldHVybiB0eXBlb2YgYXJnID09PSAnbnVtYmVyJztcbn1cbmV4cG9ydHMuaXNOdW1iZXIgPSBpc051bWJlcjtcblxuZnVuY3Rpb24gaXNTdHJpbmcoYXJnKSB7XG4gIHJldHVybiB0eXBlb2YgYXJnID09PSAnc3RyaW5nJztcbn1cbmV4cG9ydHMuaXNTdHJpbmcgPSBpc1N0cmluZztcblxuZnVuY3Rpb24gaXNTeW1ib2woYXJnKSB7XG4gIHJldHVybiB0eXBlb2YgYXJnID09PSAnc3ltYm9sJztcbn1cbmV4cG9ydHMuaXNTeW1ib2wgPSBpc1N5bWJvbDtcblxuZnVuY3Rpb24gaXNVbmRlZmluZWQoYXJnKSB7XG4gIHJldHVybiBhcmcgPT09IHZvaWQgMDtcbn1cbmV4cG9ydHMuaXNVbmRlZmluZWQgPSBpc1VuZGVmaW5lZDtcblxuZnVuY3Rpb24gaXNSZWdFeHAocmUpIHtcbiAgcmV0dXJuIGlzT2JqZWN0KHJlKSAmJiBvYmplY3RUb1N0cmluZyhyZSkgPT09ICdbb2JqZWN0IFJlZ0V4cF0nO1xufVxuZXhwb3J0cy5pc1JlZ0V4cCA9IGlzUmVnRXhwO1xuXG5mdW5jdGlvbiBpc09iamVjdChhcmcpIHtcbiAgcmV0dXJuIHR5cGVvZiBhcmcgPT09ICdvYmplY3QnICYmIGFyZyAhPT0gbnVsbDtcbn1cbmV4cG9ydHMuaXNPYmplY3QgPSBpc09iamVjdDtcblxuZnVuY3Rpb24gaXNEYXRlKGQpIHtcbiAgcmV0dXJuIGlzT2JqZWN0KGQpICYmIG9iamVjdFRvU3RyaW5nKGQpID09PSAnW29iamVjdCBEYXRlXSc7XG59XG5leHBvcnRzLmlzRGF0ZSA9IGlzRGF0ZTtcblxuZnVuY3Rpb24gaXNFcnJvcihlKSB7XG4gIHJldHVybiBpc09iamVjdChlKSAmJlxuICAgICAgKG9iamVjdFRvU3RyaW5nKGUpID09PSAnW29iamVjdCBFcnJvcl0nIHx8IGUgaW5zdGFuY2VvZiBFcnJvcik7XG59XG5leHBvcnRzLmlzRXJyb3IgPSBpc0Vycm9yO1xuXG5mdW5jdGlvbiBpc0Z1bmN0aW9uKGFyZykge1xuICByZXR1cm4gdHlwZW9mIGFyZyA9PT0gJ2Z1bmN0aW9uJztcbn1cbmV4cG9ydHMuaXNGdW5jdGlvbiA9IGlzRnVuY3Rpb247XG5cbmZ1bmN0aW9uIGlzUHJpbWl0aXZlKGFyZykge1xuICByZXR1cm4gYXJnID09PSBudWxsIHx8XG4gICAgICAgICB0eXBlb2YgYXJnID09PSAnYm9vbGVhbicgfHxcbiAgICAgICAgIHR5cGVvZiBhcmcgPT09ICdudW1iZXInIHx8XG4gICAgICAgICB0eXBlb2YgYXJnID09PSAnc3RyaW5nJyB8fFxuICAgICAgICAgdHlwZW9mIGFyZyA9PT0gJ3N5bWJvbCcgfHwgIC8vIEVTNiBzeW1ib2xcbiAgICAgICAgIHR5cGVvZiBhcmcgPT09ICd1bmRlZmluZWQnO1xufVxuZXhwb3J0cy5pc1ByaW1pdGl2ZSA9IGlzUHJpbWl0aXZlO1xuXG5leHBvcnRzLmlzQnVmZmVyID0gcmVxdWlyZSgnLi9zdXBwb3J0L2lzQnVmZmVyJyk7XG5cbmZ1bmN0aW9uIG9iamVjdFRvU3RyaW5nKG8pIHtcbiAgcmV0dXJuIE9iamVjdC5wcm90b3R5cGUudG9TdHJpbmcuY2FsbChvKTtcbn1cblxuXG5mdW5jdGlvbiBwYWQobikge1xuICByZXR1cm4gbiA8IDEwID8gJzAnICsgbi50b1N0cmluZygxMCkgOiBuLnRvU3RyaW5nKDEwKTtcbn1cblxuXG52YXIgbW9udGhzID0gWydKYW4nLCAnRmViJywgJ01hcicsICdBcHInLCAnTWF5JywgJ0p1bicsICdKdWwnLCAnQXVnJywgJ1NlcCcsXG4gICAgICAgICAgICAgICdPY3QnLCAnTm92JywgJ0RlYyddO1xuXG4vLyAyNiBGZWIgMTY6MTk6MzRcbmZ1bmN0aW9uIHRpbWVzdGFtcCgpIHtcbiAgdmFyIGQgPSBuZXcgRGF0ZSgpO1xuICB2YXIgdGltZSA9IFtwYWQoZC5nZXRIb3VycygpKSxcbiAgICAgICAgICAgICAgcGFkKGQuZ2V0TWludXRlcygpKSxcbiAgICAgICAgICAgICAgcGFkKGQuZ2V0U2Vjb25kcygpKV0uam9pbignOicpO1xuICByZXR1cm4gW2QuZ2V0RGF0ZSgpLCBtb250aHNbZC5nZXRNb250aCgpXSwgdGltZV0uam9pbignICcpO1xufVxuXG5cbi8vIGxvZyBpcyBqdXN0IGEgdGhpbiB3cmFwcGVyIHRvIGNvbnNvbGUubG9nIHRoYXQgcHJlcGVuZHMgYSB0aW1lc3RhbXBcbmV4cG9ydHMubG9nID0gZnVuY3Rpb24oKSB7XG4gIGNvbnNvbGUubG9nKCclcyAtICVzJywgdGltZXN0YW1wKCksIGV4cG9ydHMuZm9ybWF0LmFwcGx5KGV4cG9ydHMsIGFyZ3VtZW50cykpO1xufTtcblxuXG4vKipcbiAqIEluaGVyaXQgdGhlIHByb3RvdHlwZSBtZXRob2RzIGZyb20gb25lIGNvbnN0cnVjdG9yIGludG8gYW5vdGhlci5cbiAqXG4gKiBUaGUgRnVuY3Rpb24ucHJvdG90eXBlLmluaGVyaXRzIGZyb20gbGFuZy5qcyByZXdyaXR0ZW4gYXMgYSBzdGFuZGFsb25lXG4gKiBmdW5jdGlvbiAobm90IG9uIEZ1bmN0aW9uLnByb3RvdHlwZSkuIE5PVEU6IElmIHRoaXMgZmlsZSBpcyB0byBiZSBsb2FkZWRcbiAqIGR1cmluZyBib290c3RyYXBwaW5nIHRoaXMgZnVuY3Rpb24gbmVlZHMgdG8gYmUgcmV3cml0dGVuIHVzaW5nIHNvbWUgbmF0aXZlXG4gKiBmdW5jdGlvbnMgYXMgcHJvdG90eXBlIHNldHVwIHVzaW5nIG5vcm1hbCBKYXZhU2NyaXB0IGRvZXMgbm90IHdvcmsgYXNcbiAqIGV4cGVjdGVkIGR1cmluZyBib290c3RyYXBwaW5nIChzZWUgbWlycm9yLmpzIGluIHIxMTQ5MDMpLlxuICpcbiAqIEBwYXJhbSB7ZnVuY3Rpb259IGN0b3IgQ29uc3RydWN0b3IgZnVuY3Rpb24gd2hpY2ggbmVlZHMgdG8gaW5oZXJpdCB0aGVcbiAqICAgICBwcm90b3R5cGUuXG4gKiBAcGFyYW0ge2Z1bmN0aW9ufSBzdXBlckN0b3IgQ29uc3RydWN0b3IgZnVuY3Rpb24gdG8gaW5oZXJpdCBwcm90b3R5cGUgZnJvbS5cbiAqL1xuZXhwb3J0cy5pbmhlcml0cyA9IHJlcXVpcmUoJ2luaGVyaXRzJyk7XG5cbmV4cG9ydHMuX2V4dGVuZCA9IGZ1bmN0aW9uKG9yaWdpbiwgYWRkKSB7XG4gIC8vIERvbid0IGRvIGFueXRoaW5nIGlmIGFkZCBpc24ndCBhbiBvYmplY3RcbiAgaWYgKCFhZGQgfHwgIWlzT2JqZWN0KGFkZCkpIHJldHVybiBvcmlnaW47XG5cbiAgdmFyIGtleXMgPSBPYmplY3Qua2V5cyhhZGQpO1xuICB2YXIgaSA9IGtleXMubGVuZ3RoO1xuICB3aGlsZSAoaS0tKSB7XG4gICAgb3JpZ2luW2tleXNbaV1dID0gYWRkW2tleXNbaV1dO1xuICB9XG4gIHJldHVybiBvcmlnaW47XG59O1xuXG5mdW5jdGlvbiBoYXNPd25Qcm9wZXJ0eShvYmosIHByb3ApIHtcbiAgcmV0dXJuIE9iamVjdC5wcm90b3R5cGUuaGFzT3duUHJvcGVydHkuY2FsbChvYmosIHByb3ApO1xufVxuIl19
