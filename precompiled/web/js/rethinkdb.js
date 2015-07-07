require=(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);throw new Error("Cannot find module '"+o+"'")}var f=n[o]={exports:{}};t[o][0].call(f.exports,function(e){var n=t[o][1][e];return s(n?n:e)},f,f.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){

},{}],2:[function(require,module,exports){
// Copyright Joyent, Inc. and other Node contributors.
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

function EventEmitter() {
  this._events = this._events || {};
  this._maxListeners = this._maxListeners || undefined;
}
module.exports = EventEmitter;

// Backwards-compat with node 0.10.x
EventEmitter.EventEmitter = EventEmitter;

EventEmitter.prototype._events = undefined;
EventEmitter.prototype._maxListeners = undefined;

// By default EventEmitters will print a warning if more than 10 listeners are
// added to it. This is a useful default which helps finding memory leaks.
EventEmitter.defaultMaxListeners = 10;

// Obviously not all Emitters should be limited to 10. This function allows
// that to be increased. Set to zero for unlimited.
EventEmitter.prototype.setMaxListeners = function(n) {
  if (!isNumber(n) || n < 0 || isNaN(n))
    throw TypeError('n must be a positive number');
  this._maxListeners = n;
  return this;
};

EventEmitter.prototype.emit = function(type) {
  var er, handler, len, args, i, listeners;

  if (!this._events)
    this._events = {};

  // If there is no 'error' event listener then throw.
  if (type === 'error') {
    if (!this._events.error ||
        (isObject(this._events.error) && !this._events.error.length)) {
      er = arguments[1];
      if (er instanceof Error) {
        throw er; // Unhandled 'error' event
      } else {
        throw TypeError('Uncaught, unspecified "error" event.');
      }
      return false;
    }
  }

  handler = this._events[type];

  if (isUndefined(handler))
    return false;

  if (isFunction(handler)) {
    switch (arguments.length) {
      // fast cases
      case 1:
        handler.call(this);
        break;
      case 2:
        handler.call(this, arguments[1]);
        break;
      case 3:
        handler.call(this, arguments[1], arguments[2]);
        break;
      // slower
      default:
        len = arguments.length;
        args = new Array(len - 1);
        for (i = 1; i < len; i++)
          args[i - 1] = arguments[i];
        handler.apply(this, args);
    }
  } else if (isObject(handler)) {
    len = arguments.length;
    args = new Array(len - 1);
    for (i = 1; i < len; i++)
      args[i - 1] = arguments[i];

    listeners = handler.slice();
    len = listeners.length;
    for (i = 0; i < len; i++)
      listeners[i].apply(this, args);
  }

  return true;
};

EventEmitter.prototype.addListener = function(type, listener) {
  var m;

  if (!isFunction(listener))
    throw TypeError('listener must be a function');

  if (!this._events)
    this._events = {};

  // To avoid recursion in the case that type === "newListener"! Before
  // adding it to the listeners, first emit "newListener".
  if (this._events.newListener)
    this.emit('newListener', type,
              isFunction(listener.listener) ?
              listener.listener : listener);

  if (!this._events[type])
    // Optimize the case of one listener. Don't need the extra array object.
    this._events[type] = listener;
  else if (isObject(this._events[type]))
    // If we've already got an array, just append.
    this._events[type].push(listener);
  else
    // Adding the second element, need to change to array.
    this._events[type] = [this._events[type], listener];

  // Check for listener leak
  if (isObject(this._events[type]) && !this._events[type].warned) {
    var m;
    if (!isUndefined(this._maxListeners)) {
      m = this._maxListeners;
    } else {
      m = EventEmitter.defaultMaxListeners;
    }

    if (m && m > 0 && this._events[type].length > m) {
      this._events[type].warned = true;
      console.error('(node) warning: possible EventEmitter memory ' +
                    'leak detected. %d listeners added. ' +
                    'Use emitter.setMaxListeners() to increase limit.',
                    this._events[type].length);
      console.trace();
    }
  }

  return this;
};

EventEmitter.prototype.on = EventEmitter.prototype.addListener;

EventEmitter.prototype.once = function(type, listener) {
  if (!isFunction(listener))
    throw TypeError('listener must be a function');

  var fired = false;

  function g() {
    this.removeListener(type, g);

    if (!fired) {
      fired = true;
      listener.apply(this, arguments);
    }
  }

  g.listener = listener;
  this.on(type, g);

  return this;
};

// emits a 'removeListener' event iff the listener was removed
EventEmitter.prototype.removeListener = function(type, listener) {
  var list, position, length, i;

  if (!isFunction(listener))
    throw TypeError('listener must be a function');

  if (!this._events || !this._events[type])
    return this;

  list = this._events[type];
  length = list.length;
  position = -1;

  if (list === listener ||
      (isFunction(list.listener) && list.listener === listener)) {
    delete this._events[type];
    if (this._events.removeListener)
      this.emit('removeListener', type, listener);

  } else if (isObject(list)) {
    for (i = length; i-- > 0;) {
      if (list[i] === listener ||
          (list[i].listener && list[i].listener === listener)) {
        position = i;
        break;
      }
    }

    if (position < 0)
      return this;

    if (list.length === 1) {
      list.length = 0;
      delete this._events[type];
    } else {
      list.splice(position, 1);
    }

    if (this._events.removeListener)
      this.emit('removeListener', type, listener);
  }

  return this;
};

EventEmitter.prototype.removeAllListeners = function(type) {
  var key, listeners;

  if (!this._events)
    return this;

  // not listening for removeListener, no need to emit
  if (!this._events.removeListener) {
    if (arguments.length === 0)
      this._events = {};
    else if (this._events[type])
      delete this._events[type];
    return this;
  }

  // emit removeListener for all listeners on all events
  if (arguments.length === 0) {
    for (key in this._events) {
      if (key === 'removeListener') continue;
      this.removeAllListeners(key);
    }
    this.removeAllListeners('removeListener');
    this._events = {};
    return this;
  }

  listeners = this._events[type];

  if (isFunction(listeners)) {
    this.removeListener(type, listeners);
  } else {
    // LIFO order
    while (listeners.length)
      this.removeListener(type, listeners[listeners.length - 1]);
  }
  delete this._events[type];

  return this;
};

EventEmitter.prototype.listeners = function(type) {
  var ret;
  if (!this._events || !this._events[type])
    ret = [];
  else if (isFunction(this._events[type]))
    ret = [this._events[type]];
  else
    ret = this._events[type].slice();
  return ret;
};

EventEmitter.listenerCount = function(emitter, type) {
  var ret;
  if (!emitter._events || !emitter._events[type])
    ret = 0;
  else if (isFunction(emitter._events[type]))
    ret = 1;
  else
    ret = emitter._events[type].length;
  return ret;
};

function isFunction(arg) {
  return typeof arg === 'function';
}

function isNumber(arg) {
  return typeof arg === 'number';
}

function isObject(arg) {
  return typeof arg === 'object' && arg !== null;
}

function isUndefined(arg) {
  return arg === void 0;
}

},{}],3:[function(require,module,exports){
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
            var source = ev.source;
            if ((source === window || source === null) && ev.data === 'process-tick') {
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

},{}],4:[function(require,module,exports){
var base64 = require('base64-js')
var ieee754 = require('ieee754')

exports.Buffer = Buffer
exports.SlowBuffer = Buffer
exports.INSPECT_MAX_BYTES = 50
Buffer.poolSize = 8192

/**
 * If `Buffer._useTypedArrays`:
 *   === true    Use Uint8Array implementation (fastest)
 *   === false   Use Object implementation (compatible down to IE6)
 */
Buffer._useTypedArrays = (function () {
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
  if (Buffer._useTypedArrays) {
    // Preferred: Return an augmented `Uint8Array` instance for best performance
    buf = augment(new Uint8Array(length))
  } else {
    // Fallback: Return THIS instance of Buffer (created by `new`)
    buf = this
    buf.length = length
    buf._isBuffer = true
  }

  var i
  if (Buffer._useTypedArrays && typeof Uint8Array === 'function' &&
      subject instanceof Uint8Array) {
    // Speed optimization -- use set if we're copying from a Uint8Array
    buf._set(subject)
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
  } else if (type === 'number' && !Buffer._useTypedArrays && !noZero) {
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
    case 'ucs2':
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
      return true
    default:
      return false
  }
}

Buffer.isBuffer = function (b) {
  return !!(b !== null && b !== undefined && b._isBuffer)
}

Buffer.byteLength = function (str, encoding) {
  var ret
  str = str + ''
  switch (encoding || 'utf8') {
    case 'hex':
      ret = str.length / 2
      break
    case 'utf8':
    case 'utf-8':
      ret = utf8ToBytes(str).length
      break
    case 'ascii':
    case 'binary':
    case 'raw':
      ret = str.length
      break
    case 'base64':
      ret = base64ToBytes(str).length
      break
    case 'ucs2':
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
      ret = str.length * 2
      break
    default:
      throw new Error('Unknown encoding')
  }
  return ret
}

Buffer.concat = function (list, totalLength) {
  assert(isArray(list), 'Usage: Buffer.concat(list, [totalLength])\n' +
      'list should be an Array.')

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
  assert(strLen % 2 === 0, 'Invalid hex string')

  if (length > strLen / 2) {
    length = strLen / 2
  }
  for (var i = 0; i < length; i++) {
    var byte = parseInt(string.substr(i * 2, 2), 16)
    assert(!isNaN(byte), 'Invalid hex string')
    buf[offset + i] = byte
  }
  Buffer._charsWritten = i * 2
  return i
}

function _utf8Write (buf, string, offset, length) {
  var charsWritten = Buffer._charsWritten =
    blitBuffer(utf8ToBytes(string), buf, offset, length)
  return charsWritten
}

function _asciiWrite (buf, string, offset, length) {
  var charsWritten = Buffer._charsWritten =
    blitBuffer(asciiToBytes(string), buf, offset, length)
  return charsWritten
}

function _binaryWrite (buf, string, offset, length) {
  return _asciiWrite(buf, string, offset, length)
}

function _base64Write (buf, string, offset, length) {
  var charsWritten = Buffer._charsWritten =
    blitBuffer(base64ToBytes(string), buf, offset, length)
  return charsWritten
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
    case 'ucs2': // TODO: No support for ucs2 or utf16le encodings yet
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
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
    case 'ucs2': // TODO: No support for ucs2 or utf16le encodings yet
    case 'ucs-2':
    case 'utf16le':
    case 'utf-16le':
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
  assert(end >= start, 'sourceEnd < sourceStart')
  assert(target_start >= 0 && target_start < target.length,
      'targetStart out of bounds')
  assert(start >= 0 && start < source.length, 'sourceStart out of bounds')
  assert(end >= 0 && end <= source.length, 'sourceEnd out of bounds')

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

// http://nodejs.org/api/buffer.html#buffer_buf_slice_start_end
Buffer.prototype.slice = function (start, end) {
  var len = this.length
  start = clamp(start, len, 0)
  end = clamp(end, len, len)

  if (Buffer._useTypedArrays) {
    return augment(this.subarray(start, end))
  } else {
    var sliceLen = end - start
    var newBuf = new Buffer(sliceLen, undefined, true)
    for (var i = 0; i < sliceLen; i++) {
      newBuf[i] = this[i + start]
    }
    return newBuf
  }
}

// `get` will be removed in Node 0.13+
Buffer.prototype.get = function (offset) {
  console.log('.get() is deprecated. Access using array indexes instead.')
  return this.readUInt8(offset)
}

// `set` will be removed in Node 0.13+
Buffer.prototype.set = function (v, offset) {
  console.log('.set() is deprecated. Access using array indexes instead.')
  return this.writeUInt8(v, offset)
}

Buffer.prototype.readUInt8 = function (offset, noAssert) {
  if (!noAssert) {
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < this.length, 'Trying to read beyond buffer length')
  }

  if (offset >= this.length)
    return

  return this[offset]
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
  if (!noAssert) {
    assert(offset !== undefined && offset !== null,
        'missing offset')
    assert(offset < this.length, 'Trying to read beyond buffer length')
  }

  if (offset >= this.length)
    return

  var neg = this[offset] & 0x80
  if (neg)
    return (0xff - this[offset] + 1) * -1
  else
    return this[offset]
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
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < this.length, 'trying to write beyond buffer length')
    verifuint(value, 0xff)
  }

  if (offset >= this.length) return

  this[offset] = value
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
  if (!noAssert) {
    assert(value !== undefined && value !== null, 'missing value')
    assert(offset !== undefined && offset !== null, 'missing offset')
    assert(offset < this.length, 'Trying to write beyond buffer length')
    verifsint(value, 0x7f, -0x80)
  }

  if (offset >= this.length)
    return

  if (value >= 0)
    this.writeUInt8(value, offset, noAssert)
  else
    this.writeUInt8(0xff + value + 1, offset, noAssert)
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

  assert(typeof value === 'number' && !isNaN(value), 'value is not a number')
  assert(end >= start, 'end < start')

  // Fill 0 bytes; we're done
  if (end === start) return
  if (this.length === 0) return

  assert(start >= 0 && start < this.length, 'start out of bounds')
  assert(end >= 0 && end <= this.length, 'end out of bounds')

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
 * Added in Node 0.12. Only available in browsers that support ArrayBuffer.
 */
Buffer.prototype.toArrayBuffer = function () {
  if (typeof Uint8Array === 'function') {
    if (Buffer._useTypedArrays) {
      return (new Buffer(this)).buffer
    } else {
      var buf = new Uint8Array(this.length)
      for (var i = 0, len = buf.length; i < len; i += 1)
        buf[i] = this[i]
      return buf.buffer
    }
  } else {
    throw new Error('Buffer.toArrayBuffer not supported in this browser')
  }
}

// HELPER FUNCTIONS
// ================

function stringtrim (str) {
  if (str.trim) return str.trim()
  return str.replace(/^\s+|\s+$/g, '')
}

var BP = Buffer.prototype

/**
 * Augment the Uint8Array *instance* (not the class!) with Buffer methods
 */
function augment (arr) {
  arr._isBuffer = true

  // save reference to original Uint8Array get/set methods before overwriting
  arr._get = arr.get
  arr._set = arr.set

  // deprecated, will be removed in node 0.13+
  arr.get = BP.get
  arr.set = BP.set

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
  arr.toArrayBuffer = BP.toArrayBuffer

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
  for (var i = 0; i < str.length; i++) {
    var b = str.charCodeAt(i)
    if (b <= 0x7F)
      byteArray.push(str.charCodeAt(i))
    else {
      var start = i
      if (b >= 0xD800 && b <= 0xDFFF) i++
      var h = encodeURIComponent(str.slice(start, i+1)).substr(1).split('%')
      for (var j = 0; j < h.length; j++)
        byteArray.push(parseInt(h[j], 16))
    }
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

},{"base64-js":5,"ieee754":6}],5:[function(require,module,exports){
var lookup = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

;(function (exports) {
	'use strict';

  var Arr = (typeof Uint8Array !== 'undefined')
    ? Uint8Array
    : Array

	var ZERO   = '0'.charCodeAt(0)
	var PLUS   = '+'.charCodeAt(0)
	var SLASH  = '/'.charCodeAt(0)
	var NUMBER = '0'.charCodeAt(0)
	var LOWER  = 'a'.charCodeAt(0)
	var UPPER  = 'A'.charCodeAt(0)

	function decode (elt) {
		var code = elt.charCodeAt(0)
		if (code === PLUS)
			return 62 // '+'
		if (code === SLASH)
			return 63 // '/'
		if (code < NUMBER)
			return -1 //no match
		if (code < NUMBER + 10)
			return code - NUMBER + 26 + 26
		if (code < UPPER + 26)
			return code - UPPER
		if (code < LOWER + 26)
			return code - LOWER + 26
	}

	function b64ToByteArray (b64) {
		var i, j, l, tmp, placeHolders, arr

		if (b64.length % 4 > 0) {
			throw new Error('Invalid string. Length must be a multiple of 4')
		}

		// the number of equal signs (place holders)
		// if there are two placeholders, than the two characters before it
		// represent one byte
		// if there is only one, then the three characters before it represent 2 bytes
		// this is just a cheap hack to not do indexOf twice
		var len = b64.length
		placeHolders = '=' === b64.charAt(len - 2) ? 2 : '=' === b64.charAt(len - 1) ? 1 : 0

		// base64 is 4/3 + up to two characters of the original data
		arr = new Arr(b64.length * 3 / 4 - placeHolders)

		// if there are placeholders, only get up to the last complete 4 chars
		l = placeHolders > 0 ? b64.length - 4 : b64.length

		var L = 0

		function push (v) {
			arr[L++] = v
		}

		for (i = 0, j = 0; i < l; i += 4, j += 3) {
			tmp = (decode(b64.charAt(i)) << 18) | (decode(b64.charAt(i + 1)) << 12) | (decode(b64.charAt(i + 2)) << 6) | decode(b64.charAt(i + 3))
			push((tmp & 0xFF0000) >> 16)
			push((tmp & 0xFF00) >> 8)
			push(tmp & 0xFF)
		}

		if (placeHolders === 2) {
			tmp = (decode(b64.charAt(i)) << 2) | (decode(b64.charAt(i + 1)) >> 4)
			push(tmp & 0xFF)
		} else if (placeHolders === 1) {
			tmp = (decode(b64.charAt(i)) << 10) | (decode(b64.charAt(i + 1)) << 4) | (decode(b64.charAt(i + 2)) >> 2)
			push((tmp >> 8) & 0xFF)
			push(tmp & 0xFF)
		}

		return arr
	}

	function uint8ToBase64 (uint8) {
		var i,
			extraBytes = uint8.length % 3, // if we have 1 byte left, pad 2 bytes
			output = "",
			temp, length

		function encode (num) {
			return lookup.charAt(num)
		}

		function tripletToBase64 (num) {
			return encode(num >> 18 & 0x3F) + encode(num >> 12 & 0x3F) + encode(num >> 6 & 0x3F) + encode(num & 0x3F)
		}

		// go through the array every three bytes, we'll deal with trailing stuff later
		for (i = 0, length = uint8.length - extraBytes; i < length; i += 3) {
			temp = (uint8[i] << 16) + (uint8[i + 1] << 8) + (uint8[i + 2])
			output += tripletToBase64(temp)
		}

		// pad the end with zeros, but make sure to not forget the extra bytes
		switch (extraBytes) {
			case 1:
				temp = uint8[uint8.length - 1]
				output += encode(temp >> 2)
				output += encode((temp << 4) & 0x3F)
				output += '=='
				break
			case 2:
				temp = (uint8[uint8.length - 2] << 8) + (uint8[uint8.length - 1])
				output += encode(temp >> 10)
				output += encode((temp >> 4) & 0x3F)
				output += encode((temp << 2) & 0x3F)
				output += '='
				break
		}

		return output
	}

	module.exports.toByteArray = b64ToByteArray
	module.exports.fromByteArray = uint8ToBase64
}())

},{}],6:[function(require,module,exports){
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

},{}],7:[function(require,module,exports){
(function (process,Buffer){// Generated by CoffeeScript 1.7.1
var Add, And, Append, Args, Asc, Avg, Between, Binary, Bracket, Branch, ChangeAt, Changes, Circle, CoerceTo, ConcatMap, Config, Contains, Count, DatumTerm, Day, DayOfWeek, DayOfYear, Db, DbCreate, DbDrop, DbList, Default, Delete, DeleteAt, Desc, Difference, Distance, Distinct, Div, Downcase, During, EpochTime, Eq, EqJoin, Fill, Filter, ForEach, FunCall, Func, Ge, Geojson, Get, GetAll, GetField, GetIntersecting, GetNearest, Group, Gt, HasFields, Hours, Http, ISO8601, ImplicitVar, InTimezone, Includes, IndexCreate, IndexDrop, IndexList, IndexRename, IndexStatus, IndexWait, Info, InnerJoin, Insert, InsertAt, Intersects, IsEmpty, JavaScript, Json, Keys, Le, Limit, Line, Literal, Lt, MakeArray, MakeObject, Map, Match, Max, Merge, Min, Minutes, Mod, Month, Mul, Ne, Not, Now, Nth, Object_, OffsetsOf, Or, OrderBy, OuterJoin, Pluck, Point, Polygon, PolygonSub, Prepend, Promise, RDBConstant, RDBOp, RDBVal, RQLDate, Random, Range, Rebalance, Reconfigure, Reduce, Replace, Sample, Seconds, SetDifference, SetInsert, SetIntersection, SetUnion, Skip, Slice, SpliceAt, Split, Status, Sub, Sum, Sync, Table, TableCreate, TableDrop, TableList, TermBase, Time, TimeOfDay, Timezone, ToEpochTime, ToGeojson, ToISO8601, ToJsonString, TypeOf, UUID, Ungroup, Union, Upcase, Update, UserError, Var, Wait, WithFields, Without, Year, Zip, ar, aropt, err, funcWrap, hasImplicit, intsp, intspallargs, kved, net, protoTermType, rethinkdb, shouldWrap, translateBackOptargs, translateOptargs, util, varar,
  __slice = [].slice,
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

util = require('./util');

err = require('./errors');

net = require('./net');

protoTermType = require('./proto-def').Term.TermType;

Promise = require('bluebird');

ar = util.ar;

varar = util.varar;

aropt = util.aropt;

rethinkdb = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return rethinkdb.expr.apply(rethinkdb, args);
};

funcWrap = function(val) {
  var ivarScan;
  if (val === void 0) {
    return val;
  }
  val = rethinkdb.expr(val);
  ivarScan = function(node) {
    var k, v;
    if (!(node instanceof TermBase)) {
      return false;
    }
    if (node instanceof ImplicitVar) {
      return true;
    }
    if ((node.args.map(ivarScan)).some(function(a) {
      return a;
    })) {
      return true;
    }
    if (((function() {
      var _ref, _results;
      _ref = node.optargs;
      _results = [];
      for (k in _ref) {
        if (!__hasProp.call(_ref, k)) continue;
        v = _ref[k];
        _results.push(v);
      }
      return _results;
    })()).map(ivarScan).some(function(a) {
      return a;
    })) {
      return true;
    }
    return false;
  };
  if (ivarScan(val)) {
    return new Func({}, function(x) {
      return val;
    });
  }
  return val;
};

hasImplicit = function(args) {
  var arg, _i, _len;
  if (Array.isArray(args)) {
    for (_i = 0, _len = args.length; _i < _len; _i++) {
      arg = args[_i];
      if (hasImplicit(arg) === true) {
        return true;
      }
    }
  } else if (args === 'r.row') {
    return true;
  }
  return false;
};

TermBase = (function() {
  TermBase.prototype.showRunWarning = true;

  function TermBase() {
    var self;
    self = ar(function(field) {
      return self.bracket(field);
    });
    self.__proto__ = this.__proto__;
    return self;
  }

  TermBase.prototype.run = function(connection, options, callback) {
    if (net.isConnection(connection) === true) {
      if (typeof options === "function") {
        if (callback === void 0) {
          callback = options;
          options = {};
        } else {
          return Promise.reject(new err.RqlDriverError("Second argument to `run` cannot be a function if a third argument is provided.")).nodeify(options);
        }
      }
    } else if ((connection != null ? connection.constructor : void 0) === Object) {
      if (this.showRunWarning === true) {
        if (typeof process !== "undefined" && process !== null) {
          process.stderr.write("RethinkDB warning: This syntax is deprecated. Please use `run(connection[, options], callback)`.");
        }
        this.showRunWarning = false;
      }
      callback = options;
      options = connection;
      connection = connection.connection;
      delete options["connection"];
    }
    if (options == null) {
      options = {};
    }
    if ((callback != null) && typeof callback !== 'function') {
      return Promise.reject(new err.RqlDriverError("If provided, the callback must be a function. Please use `run(connection[, options][, callback])"));
    }
    if (net.isConnection(connection) === false) {
      return Promise.reject(new err.RqlDriverError("First argument to `run` must be an open connection.")).nodeify(callback);
    }
    return new Promise((function(_this) {
      return function(resolve, reject) {
        var e, wrappedCb;
        wrappedCb = function(err, result) {
          if (err != null) {
            return reject(err);
          } else {
            return resolve(result);
          }
        };
        try {
          return connection._start(_this, wrappedCb, options);
        } catch (_error) {
          e = _error;
          return wrappedCb(e);
        }
      };
    })(this)).nodeify(callback);
  };

  TermBase.prototype.toString = function() {
    return err.printQuery(this);
  };

  return TermBase;

})();

RDBVal = (function(_super) {
  __extends(RDBVal, _super);

  function RDBVal() {
    return RDBVal.__super__.constructor.apply(this, arguments);
  }

  RDBVal.prototype.eq = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Eq, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.ne = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Ne, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.lt = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Lt, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.le = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Le, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.gt = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Gt, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.ge = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Ge, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.not = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Not, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.add = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Add, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.sub = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Sub, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.mul = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Mul, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.div = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Div, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.mod = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Mod, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.append = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Append, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.prepend = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Prepend, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.difference = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Difference, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.setInsert = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(SetInsert, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.setUnion = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(SetUnion, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.setIntersection = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(SetIntersection, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.setDifference = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(SetDifference, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.slice = varar(1, 3, function(left, right_or_opts, opts) {
    if (opts != null) {
      return new Slice(opts, this, left, right_or_opts);
    } else if (typeof right_or_opts !== 'undefined') {
      if ((Object.prototype.toString.call(right_or_opts) === '[object Object]') && !(right_or_opts instanceof TermBase)) {
        return new Slice(right_or_opts, this, left);
      } else {
        return new Slice({}, this, left, right_or_opts);
      }
    } else {
      return new Slice({}, this, left);
    }
  });

  RDBVal.prototype.skip = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Skip, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.limit = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Limit, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.getField = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(GetField, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.contains = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Contains, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.insertAt = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(InsertAt, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.spliceAt = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(SpliceAt, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.deleteAt = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(DeleteAt, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.changeAt = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ChangeAt, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.offsetsOf = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(OffsetsOf, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.hasFields = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(HasFields, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.withFields = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(WithFields, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.keys = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Keys, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.changes = aropt(function(opts) {
    return new Changes(opts, this);
  });

  RDBVal.prototype.pluck = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Pluck, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.without = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Without, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.merge = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Merge, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.between = aropt(function(left, right, opts) {
    return new Between(opts, this, left, right);
  });

  RDBVal.prototype.reduce = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Reduce, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.map = varar(1, null, function() {
    var args, funcArg, _i;
    args = 2 <= arguments.length ? __slice.call(arguments, 0, _i = arguments.length - 1) : (_i = 0, []), funcArg = arguments[_i++];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Map, [{}, this].concat(__slice.call(args), [funcWrap(funcArg)]), function(){});
  });

  RDBVal.prototype.filter = aropt(function(predicate, opts) {
    return new Filter(opts, this, funcWrap(predicate));
  });

  RDBVal.prototype.concatMap = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ConcatMap, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.distinct = aropt(function(opts) {
    return new Distinct(opts, this);
  });

  RDBVal.prototype.count = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Count, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.union = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Union, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.nth = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Nth, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.bracket = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Bracket, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.toJSON = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ToJsonString, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.toJsonString = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ToJsonString, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.match = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Match, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.split = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Split, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.upcase = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Upcase, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.downcase = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Downcase, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.isEmpty = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(IsEmpty, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.innerJoin = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(InnerJoin, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.outerJoin = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(OuterJoin, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.eqJoin = aropt(function(left_attr, right, opts) {
    return new EqJoin(opts, this, funcWrap(left_attr), right);
  });

  RDBVal.prototype.zip = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Zip, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.coerceTo = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(CoerceTo, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.ungroup = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Ungroup, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.typeOf = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(TypeOf, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.update = aropt(function(func, opts) {
    return new Update(opts, this, funcWrap(func));
  });

  RDBVal.prototype["delete"] = aropt(function(opts) {
    return new Delete(opts, this);
  });

  RDBVal.prototype.replace = aropt(function(func, opts) {
    return new Replace(opts, this, funcWrap(func));
  });

  RDBVal.prototype["do"] = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(FunCall, [{}, funcWrap(args.slice(-1)[0]), this].concat(__slice.call(args.slice(0, -1))), function(){});
  };

  RDBVal.prototype["default"] = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Default, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.or = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Or, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.and = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(And, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.forEach = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ForEach, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.sum = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Sum, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.avg = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Avg, [{}, this].concat(__slice.call(args.map(funcWrap))), function(){});
  };

  RDBVal.prototype.info = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Info, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.sample = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Sample, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.group = function() {
    var fields, fieldsAndOpts, opts, perhapsOptDict;
    fieldsAndOpts = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    opts = {};
    fields = fieldsAndOpts;
    if (fieldsAndOpts.length > 0) {
      perhapsOptDict = fieldsAndOpts[fieldsAndOpts.length - 1];
      if (perhapsOptDict && (Object.prototype.toString.call(perhapsOptDict) === '[object Object]') && !(perhapsOptDict instanceof TermBase)) {
        opts = perhapsOptDict;
        fields = fieldsAndOpts.slice(0, fieldsAndOpts.length - 1);
      }
    }
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Group, [opts, this].concat(__slice.call(fields.map(funcWrap))), function(){});
  };

  RDBVal.prototype.orderBy = function() {
    var attr, attrs, attrsAndOpts, opts, perhapsOptDict;
    attrsAndOpts = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    opts = {};
    attrs = attrsAndOpts;
    perhapsOptDict = attrsAndOpts[attrsAndOpts.length - 1];
    if (perhapsOptDict && (Object.prototype.toString.call(perhapsOptDict) === '[object Object]') && !(perhapsOptDict instanceof TermBase)) {
      opts = perhapsOptDict;
      attrs = attrsAndOpts.slice(0, attrsAndOpts.length - 1);
    }
    attrs = (function() {
      var _i, _len, _results;
      _results = [];
      for (_i = 0, _len = attrs.length; _i < _len; _i++) {
        attr = attrs[_i];
        if (attr instanceof Asc || attr instanceof Desc) {
          _results.push(attr);
        } else {
          _results.push(funcWrap(attr));
        }
      }
      return _results;
    })();
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(OrderBy, [opts, this].concat(__slice.call(attrs)), function(){});
  };

  RDBVal.prototype.toGeojson = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ToGeojson, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.distance = aropt(function(g, opts) {
    return new Distance(opts, this, g);
  });

  RDBVal.prototype.intersects = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Intersects, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.includes = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Includes, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.fill = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Fill, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.polygonSub = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(PolygonSub, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.tableCreate = aropt(function(tblName, opts) {
    return new TableCreate(opts, this, tblName);
  });

  RDBVal.prototype.tableDrop = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(TableDrop, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.tableList = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(TableList, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.config = function() {
    return new Config({}, this);
  };

  RDBVal.prototype.status = function() {
    return new Status({}, this);
  };

  RDBVal.prototype.wait = aropt(function(opts) {
    return new Wait(opts, this);
  });

  RDBVal.prototype.table = aropt(function(tblName, opts) {
    return new Table(opts, this, tblName);
  });

  RDBVal.prototype.get = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Get, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.getAll = function() {
    var keys, keysAndOpts, opts, perhapsOptDict;
    keysAndOpts = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    opts = {};
    keys = keysAndOpts;
    if (keysAndOpts.length > 1) {
      perhapsOptDict = keysAndOpts[keysAndOpts.length - 1];
      if (perhapsOptDict && ((Object.prototype.toString.call(perhapsOptDict) === '[object Object]') && !(perhapsOptDict instanceof TermBase))) {
        opts = perhapsOptDict;
        keys = keysAndOpts.slice(0, keysAndOpts.length - 1);
      }
    }
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(GetAll, [opts, this].concat(__slice.call(keys)), function(){});
  };

  RDBVal.prototype.min = function() {
    var keys, keysAndOpts, opts, perhapsOptDict;
    keysAndOpts = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    opts = {};
    keys = keysAndOpts;
    if (keysAndOpts.length === 1) {
      perhapsOptDict = keysAndOpts[0];
      if (perhapsOptDict && ((Object.prototype.toString.call(perhapsOptDict) === '[object Object]') && !(perhapsOptDict instanceof TermBase))) {
        opts = perhapsOptDict;
        keys = [];
      }
    }
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Min, [opts, this].concat(__slice.call(keys.map(funcWrap))), function(){});
  };

  RDBVal.prototype.max = function() {
    var keys, keysAndOpts, opts, perhapsOptDict;
    keysAndOpts = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    opts = {};
    keys = keysAndOpts;
    if (keysAndOpts.length === 1) {
      perhapsOptDict = keysAndOpts[0];
      if (perhapsOptDict && ((Object.prototype.toString.call(perhapsOptDict) === '[object Object]') && !(perhapsOptDict instanceof TermBase))) {
        opts = perhapsOptDict;
        keys = [];
      }
    }
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Max, [opts, this].concat(__slice.call(keys.map(funcWrap))), function(){});
  };

  RDBVal.prototype.insert = aropt(function(doc, opts) {
    return new Insert(opts, this, rethinkdb.expr(doc));
  });

  RDBVal.prototype.indexCreate = varar(1, 3, function(name, defun_or_opts, opts) {
    if (opts != null) {
      return new IndexCreate(opts, this, name, funcWrap(defun_or_opts));
    } else if (defun_or_opts != null) {
      if ((Object.prototype.toString.call(defun_or_opts) === '[object Object]') && !(typeof defun_or_opts === 'function') && !(defun_or_opts instanceof TermBase)) {
        return new IndexCreate(defun_or_opts, this, name);
      } else {
        return new IndexCreate({}, this, name, funcWrap(defun_or_opts));
      }
    } else {
      return new IndexCreate({}, this, name);
    }
  });

  RDBVal.prototype.indexDrop = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(IndexDrop, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.indexList = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(IndexList, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.indexStatus = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(IndexStatus, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.indexWait = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(IndexWait, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.indexRename = aropt(function(old_name, new_name, opts) {
    return new IndexRename(opts, this, old_name, new_name);
  });

  RDBVal.prototype.reconfigure = function(opts) {
    return new Reconfigure(opts, this);
  };

  RDBVal.prototype.rebalance = function() {
    return new Rebalance({}, this);
  };

  RDBVal.prototype.sync = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Sync, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.toISO8601 = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ToISO8601, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.toEpochTime = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(ToEpochTime, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.inTimezone = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(InTimezone, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.during = aropt(function(t2, t3, opts) {
    return new During(opts, this, t2, t3);
  });

  RDBVal.prototype.date = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(RQLDate, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.timeOfDay = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(TimeOfDay, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.timezone = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Timezone, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.year = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Year, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.month = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Month, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.day = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Day, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.dayOfWeek = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(DayOfWeek, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.dayOfYear = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(DayOfYear, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.hours = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Hours, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.minutes = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Minutes, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.seconds = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(Seconds, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.uuid = function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(UUID, [{}, this].concat(__slice.call(args)), function(){});
  };

  RDBVal.prototype.getIntersecting = aropt(function(g, opts) {
    return new GetIntersecting(opts, this, g);
  });

  RDBVal.prototype.getNearest = aropt(function(g, opts) {
    return new GetNearest(opts, this, g);
  });

  return RDBVal;

})(TermBase);

DatumTerm = (function(_super) {
  __extends(DatumTerm, _super);

  DatumTerm.prototype.args = [];

  DatumTerm.prototype.optargs = {};

  function DatumTerm(val) {
    var self;
    self = DatumTerm.__super__.constructor.call(this);
    self.data = val;
    return self;
  }

  DatumTerm.prototype.compose = function() {
    switch (typeof this.data) {
      case 'string':
        return '"' + this.data + '"';
      default:
        return '' + this.data;
    }
  };

  DatumTerm.prototype.build = function() {
    if (typeof this.data === 'number') {
      if (!isFinite(this.data)) {
        throw new TypeError("Illegal non-finite number `" + this.data.toString() + "`.");
      }
    }
    return this.data;
  };

  return DatumTerm;

})(RDBVal);

translateBackOptargs = function(optargs) {
  var key, result, val;
  result = {};
  for (key in optargs) {
    if (!__hasProp.call(optargs, key)) continue;
    val = optargs[key];
    result[util.toCamelCase(key)] = val;
  }
  return result;
};

translateOptargs = function(optargs) {
  var key, result, val;
  result = {};
  for (key in optargs) {
    if (!__hasProp.call(optargs, key)) continue;
    val = optargs[key];
    if (key === void 0 || val === void 0) {
      continue;
    }
    result[util.fromCamelCase(key)] = rethinkdb.expr(val);
  }
  return result;
};

RDBOp = (function(_super) {
  __extends(RDBOp, _super);

  function RDBOp() {
    var arg, args, i, optargs, self;
    optargs = arguments[0], args = 2 <= arguments.length ? __slice.call(arguments, 1) : [];
    self = RDBOp.__super__.constructor.call(this);
    self.args = (function() {
      var _i, _len, _results;
      _results = [];
      for (i = _i = 0, _len = args.length; _i < _len; i = ++_i) {
        arg = args[i];
        if (arg !== void 0) {
          _results.push(rethinkdb.expr(arg));
        } else {
          throw new err.RqlDriverError("Argument " + i + " to " + (this.st || this.mt) + " may not be `undefined`.");
        }
      }
      return _results;
    }).call(this);
    self.optargs = translateOptargs(optargs);
    return self;
  }

  RDBOp.prototype.build = function() {
    var add_opts, arg, key, opts, res, val, _i, _len, _ref, _ref1;
    res = [this.tt, []];
    _ref = this.args;
    for (_i = 0, _len = _ref.length; _i < _len; _i++) {
      arg = _ref[_i];
      res[1].push(arg.build());
    }
    opts = {};
    add_opts = false;
    _ref1 = this.optargs;
    for (key in _ref1) {
      if (!__hasProp.call(_ref1, key)) continue;
      val = _ref1[key];
      add_opts = true;
      opts[key] = val.build();
    }
    if (add_opts) {
      res.push(opts);
    }
    return res;
  };

  RDBOp.prototype.compose = function(args, optargs) {
    if (this.st) {
      return ['r.', this.st, '(', intspallargs(args, optargs), ')'];
    } else {
      if (shouldWrap(this.args[0])) {
        args[0] = ['r(', args[0], ')'];
      }
      return [args[0], '.', this.mt, '(', intspallargs(args.slice(1), optargs), ')'];
    }
  };

  return RDBOp;

})(RDBVal);

RDBConstant = (function(_super) {
  __extends(RDBConstant, _super);

  function RDBConstant() {
    return RDBConstant.__super__.constructor.apply(this, arguments);
  }

  RDBConstant.prototype.compose = function(args, optargs) {
    return ['r.', this.st];
  };

  return RDBConstant;

})(RDBOp);

intsp = function(seq) {
  var e, res, _i, _len, _ref;
  if (seq[0] == null) {
    return [];
  }
  res = [seq[0]];
  _ref = seq.slice(1);
  for (_i = 0, _len = _ref.length; _i < _len; _i++) {
    e = _ref[_i];
    res.push(', ', e);
  }
  return res;
};

kved = function(optargs) {
  var k, v;
  return [
    '{', intsp((function() {
      var _results;
      _results = [];
      for (k in optargs) {
        if (!__hasProp.call(optargs, k)) continue;
        v = optargs[k];
        _results.push([k, ': ', v]);
      }
      return _results;
    })()), '}'
  ];
};

intspallargs = function(args, optargs) {
  var argrepr;
  argrepr = [];
  if (args.length > 0) {
    argrepr.push(intsp(args));
  }
  if (Object.keys(optargs).length > 0) {
    if (argrepr.length > 0) {
      argrepr.push(', ');
    }
    argrepr.push(kved(translateBackOptargs(optargs)));
  }
  return argrepr;
};

shouldWrap = function(arg) {
  return arg instanceof DatumTerm || arg instanceof MakeArray || arg instanceof MakeObject;
};

MakeArray = (function(_super) {
  __extends(MakeArray, _super);

  function MakeArray() {
    return MakeArray.__super__.constructor.apply(this, arguments);
  }

  MakeArray.prototype.tt = protoTermType.MAKE_ARRAY;

  MakeArray.prototype.st = '[...]';

  MakeArray.prototype.compose = function(args) {
    return ['[', intsp(args), ']'];
  };

  return MakeArray;

})(RDBOp);

MakeObject = (function(_super) {
  __extends(MakeObject, _super);

  MakeObject.prototype.tt = protoTermType.MAKE_OBJECT;

  MakeObject.prototype.st = '{...}';

  function MakeObject(obj, nestingDepth) {
    var key, self, val;
    if (nestingDepth == null) {
      nestingDepth = 20;
    }
    self = MakeObject.__super__.constructor.call(this, {});
    self.optargs = {};
    for (key in obj) {
      if (!__hasProp.call(obj, key)) continue;
      val = obj[key];
      if (typeof val === 'undefined') {
        throw new err.RqlDriverError("Object field '" + key + "' may not be undefined");
      }
      self.optargs[key] = rethinkdb.expr(val, nestingDepth - 1);
    }
    return self;
  }

  MakeObject.prototype.compose = function(args, optargs) {
    return kved(optargs);
  };

  MakeObject.prototype.build = function() {
    var key, res, val, _ref;
    res = {};
    _ref = this.optargs;
    for (key in _ref) {
      if (!__hasProp.call(_ref, key)) continue;
      val = _ref[key];
      res[key] = val.build();
    }
    return res;
  };

  return MakeObject;

})(RDBOp);

Var = (function(_super) {
  __extends(Var, _super);

  function Var() {
    return Var.__super__.constructor.apply(this, arguments);
  }

  Var.prototype.tt = protoTermType.VAR;

  Var.prototype.compose = function(args) {
    return ['var_' + args];
  };

  return Var;

})(RDBOp);

JavaScript = (function(_super) {
  __extends(JavaScript, _super);

  function JavaScript() {
    return JavaScript.__super__.constructor.apply(this, arguments);
  }

  JavaScript.prototype.tt = protoTermType.JAVASCRIPT;

  JavaScript.prototype.st = 'js';

  return JavaScript;

})(RDBOp);

Http = (function(_super) {
  __extends(Http, _super);

  function Http() {
    return Http.__super__.constructor.apply(this, arguments);
  }

  Http.prototype.tt = protoTermType.HTTP;

  Http.prototype.st = 'http';

  return Http;

})(RDBOp);

Json = (function(_super) {
  __extends(Json, _super);

  function Json() {
    return Json.__super__.constructor.apply(this, arguments);
  }

  Json.prototype.tt = protoTermType.JSON;

  Json.prototype.st = 'json';

  return Json;

})(RDBOp);

Binary = (function(_super) {
  __extends(Binary, _super);

  Binary.prototype.tt = protoTermType.BINARY;

  Binary.prototype.st = 'binary';

  function Binary(data) {
    var self;
    if (data instanceof TermBase) {
      self = Binary.__super__.constructor.call(this, {}, data);
    } else if (data instanceof Buffer) {
      self = Binary.__super__.constructor.call(this);
      self.base64_data = data.toString("base64");
    } else {
      throw new TypeError("Parameter to `r.binary` must be a Buffer object or RQL query.");
    }
    return self;
  }

  Binary.prototype.compose = function() {
    if (this.args.length === 0) {
      return 'r.binary(<data>)';
    } else {
      return Binary.__super__.compose.apply(this, arguments);
    }
  };

  Binary.prototype.build = function() {
    if (this.args.length === 0) {
      return {
        '$reql_type$': 'BINARY',
        'data': this.base64_data
      };
    } else {
      return Binary.__super__.build.apply(this, arguments);
    }
  };

  return Binary;

})(RDBOp);

Args = (function(_super) {
  __extends(Args, _super);

  function Args() {
    return Args.__super__.constructor.apply(this, arguments);
  }

  Args.prototype.tt = protoTermType.ARGS;

  Args.prototype.st = 'args';

  return Args;

})(RDBOp);

UserError = (function(_super) {
  __extends(UserError, _super);

  function UserError() {
    return UserError.__super__.constructor.apply(this, arguments);
  }

  UserError.prototype.tt = protoTermType.ERROR;

  UserError.prototype.st = 'error';

  return UserError;

})(RDBOp);

Random = (function(_super) {
  __extends(Random, _super);

  function Random() {
    return Random.__super__.constructor.apply(this, arguments);
  }

  Random.prototype.tt = protoTermType.RANDOM;

  Random.prototype.st = 'random';

  return Random;

})(RDBOp);

ImplicitVar = (function(_super) {
  __extends(ImplicitVar, _super);

  function ImplicitVar() {
    return ImplicitVar.__super__.constructor.apply(this, arguments);
  }

  ImplicitVar.prototype.tt = protoTermType.IMPLICIT_VAR;

  ImplicitVar.prototype.compose = function() {
    return ['r.row'];
  };

  return ImplicitVar;

})(RDBOp);

Db = (function(_super) {
  __extends(Db, _super);

  function Db() {
    return Db.__super__.constructor.apply(this, arguments);
  }

  Db.prototype.tt = protoTermType.DB;

  Db.prototype.st = 'db';

  return Db;

})(RDBOp);

Table = (function(_super) {
  __extends(Table, _super);

  function Table() {
    return Table.__super__.constructor.apply(this, arguments);
  }

  Table.prototype.tt = protoTermType.TABLE;

  Table.prototype.st = 'table';

  Table.prototype.compose = function(args, optargs) {
    if (this.args[0] instanceof Db) {
      return [args[0], '.table(', intspallargs(args.slice(1), optargs), ')'];
    } else {
      return ['r.table(', intspallargs(args, optargs), ')'];
    }
  };

  return Table;

})(RDBOp);

Get = (function(_super) {
  __extends(Get, _super);

  function Get() {
    return Get.__super__.constructor.apply(this, arguments);
  }

  Get.prototype.tt = protoTermType.GET;

  Get.prototype.mt = 'get';

  return Get;

})(RDBOp);

GetAll = (function(_super) {
  __extends(GetAll, _super);

  function GetAll() {
    return GetAll.__super__.constructor.apply(this, arguments);
  }

  GetAll.prototype.tt = protoTermType.GET_ALL;

  GetAll.prototype.mt = 'getAll';

  return GetAll;

})(RDBOp);

Eq = (function(_super) {
  __extends(Eq, _super);

  function Eq() {
    return Eq.__super__.constructor.apply(this, arguments);
  }

  Eq.prototype.tt = protoTermType.EQ;

  Eq.prototype.mt = 'eq';

  return Eq;

})(RDBOp);

Ne = (function(_super) {
  __extends(Ne, _super);

  function Ne() {
    return Ne.__super__.constructor.apply(this, arguments);
  }

  Ne.prototype.tt = protoTermType.NE;

  Ne.prototype.mt = 'ne';

  return Ne;

})(RDBOp);

Lt = (function(_super) {
  __extends(Lt, _super);

  function Lt() {
    return Lt.__super__.constructor.apply(this, arguments);
  }

  Lt.prototype.tt = protoTermType.LT;

  Lt.prototype.mt = 'lt';

  return Lt;

})(RDBOp);

Le = (function(_super) {
  __extends(Le, _super);

  function Le() {
    return Le.__super__.constructor.apply(this, arguments);
  }

  Le.prototype.tt = protoTermType.LE;

  Le.prototype.mt = 'le';

  return Le;

})(RDBOp);

Gt = (function(_super) {
  __extends(Gt, _super);

  function Gt() {
    return Gt.__super__.constructor.apply(this, arguments);
  }

  Gt.prototype.tt = protoTermType.GT;

  Gt.prototype.mt = 'gt';

  return Gt;

})(RDBOp);

Ge = (function(_super) {
  __extends(Ge, _super);

  function Ge() {
    return Ge.__super__.constructor.apply(this, arguments);
  }

  Ge.prototype.tt = protoTermType.GE;

  Ge.prototype.mt = 'ge';

  return Ge;

})(RDBOp);

Not = (function(_super) {
  __extends(Not, _super);

  function Not() {
    return Not.__super__.constructor.apply(this, arguments);
  }

  Not.prototype.tt = protoTermType.NOT;

  Not.prototype.mt = 'not';

  return Not;

})(RDBOp);

Add = (function(_super) {
  __extends(Add, _super);

  function Add() {
    return Add.__super__.constructor.apply(this, arguments);
  }

  Add.prototype.tt = protoTermType.ADD;

  Add.prototype.mt = 'add';

  return Add;

})(RDBOp);

Sub = (function(_super) {
  __extends(Sub, _super);

  function Sub() {
    return Sub.__super__.constructor.apply(this, arguments);
  }

  Sub.prototype.tt = protoTermType.SUB;

  Sub.prototype.mt = 'sub';

  return Sub;

})(RDBOp);

Mul = (function(_super) {
  __extends(Mul, _super);

  function Mul() {
    return Mul.__super__.constructor.apply(this, arguments);
  }

  Mul.prototype.tt = protoTermType.MUL;

  Mul.prototype.mt = 'mul';

  return Mul;

})(RDBOp);

Div = (function(_super) {
  __extends(Div, _super);

  function Div() {
    return Div.__super__.constructor.apply(this, arguments);
  }

  Div.prototype.tt = protoTermType.DIV;

  Div.prototype.mt = 'div';

  return Div;

})(RDBOp);

Mod = (function(_super) {
  __extends(Mod, _super);

  function Mod() {
    return Mod.__super__.constructor.apply(this, arguments);
  }

  Mod.prototype.tt = protoTermType.MOD;

  Mod.prototype.mt = 'mod';

  return Mod;

})(RDBOp);

Append = (function(_super) {
  __extends(Append, _super);

  function Append() {
    return Append.__super__.constructor.apply(this, arguments);
  }

  Append.prototype.tt = protoTermType.APPEND;

  Append.prototype.mt = 'append';

  return Append;

})(RDBOp);

Prepend = (function(_super) {
  __extends(Prepend, _super);

  function Prepend() {
    return Prepend.__super__.constructor.apply(this, arguments);
  }

  Prepend.prototype.tt = protoTermType.PREPEND;

  Prepend.prototype.mt = 'prepend';

  return Prepend;

})(RDBOp);

Difference = (function(_super) {
  __extends(Difference, _super);

  function Difference() {
    return Difference.__super__.constructor.apply(this, arguments);
  }

  Difference.prototype.tt = protoTermType.DIFFERENCE;

  Difference.prototype.mt = 'difference';

  return Difference;

})(RDBOp);

SetInsert = (function(_super) {
  __extends(SetInsert, _super);

  function SetInsert() {
    return SetInsert.__super__.constructor.apply(this, arguments);
  }

  SetInsert.prototype.tt = protoTermType.SET_INSERT;

  SetInsert.prototype.mt = 'setInsert';

  return SetInsert;

})(RDBOp);

SetUnion = (function(_super) {
  __extends(SetUnion, _super);

  function SetUnion() {
    return SetUnion.__super__.constructor.apply(this, arguments);
  }

  SetUnion.prototype.tt = protoTermType.SET_UNION;

  SetUnion.prototype.mt = 'setUnion';

  return SetUnion;

})(RDBOp);

SetIntersection = (function(_super) {
  __extends(SetIntersection, _super);

  function SetIntersection() {
    return SetIntersection.__super__.constructor.apply(this, arguments);
  }

  SetIntersection.prototype.tt = protoTermType.SET_INTERSECTION;

  SetIntersection.prototype.mt = 'setIntersection';

  return SetIntersection;

})(RDBOp);

SetDifference = (function(_super) {
  __extends(SetDifference, _super);

  function SetDifference() {
    return SetDifference.__super__.constructor.apply(this, arguments);
  }

  SetDifference.prototype.tt = protoTermType.SET_DIFFERENCE;

  SetDifference.prototype.mt = 'setDifference';

  return SetDifference;

})(RDBOp);

Slice = (function(_super) {
  __extends(Slice, _super);

  function Slice() {
    return Slice.__super__.constructor.apply(this, arguments);
  }

  Slice.prototype.tt = protoTermType.SLICE;

  Slice.prototype.mt = 'slice';

  return Slice;

})(RDBOp);

Skip = (function(_super) {
  __extends(Skip, _super);

  function Skip() {
    return Skip.__super__.constructor.apply(this, arguments);
  }

  Skip.prototype.tt = protoTermType.SKIP;

  Skip.prototype.mt = 'skip';

  return Skip;

})(RDBOp);

Limit = (function(_super) {
  __extends(Limit, _super);

  function Limit() {
    return Limit.__super__.constructor.apply(this, arguments);
  }

  Limit.prototype.tt = protoTermType.LIMIT;

  Limit.prototype.mt = 'limit';

  return Limit;

})(RDBOp);

GetField = (function(_super) {
  __extends(GetField, _super);

  function GetField() {
    return GetField.__super__.constructor.apply(this, arguments);
  }

  GetField.prototype.tt = protoTermType.GET_FIELD;

  GetField.prototype.mt = 'getField';

  return GetField;

})(RDBOp);

Bracket = (function(_super) {
  __extends(Bracket, _super);

  function Bracket() {
    return Bracket.__super__.constructor.apply(this, arguments);
  }

  Bracket.prototype.tt = protoTermType.BRACKET;

  Bracket.prototype.st = '(...)';

  Bracket.prototype.compose = function(args) {
    return [args[0], '(', args[1], ')'];
  };

  return Bracket;

})(RDBOp);

Contains = (function(_super) {
  __extends(Contains, _super);

  function Contains() {
    return Contains.__super__.constructor.apply(this, arguments);
  }

  Contains.prototype.tt = protoTermType.CONTAINS;

  Contains.prototype.mt = 'contains';

  return Contains;

})(RDBOp);

InsertAt = (function(_super) {
  __extends(InsertAt, _super);

  function InsertAt() {
    return InsertAt.__super__.constructor.apply(this, arguments);
  }

  InsertAt.prototype.tt = protoTermType.INSERT_AT;

  InsertAt.prototype.mt = 'insertAt';

  return InsertAt;

})(RDBOp);

SpliceAt = (function(_super) {
  __extends(SpliceAt, _super);

  function SpliceAt() {
    return SpliceAt.__super__.constructor.apply(this, arguments);
  }

  SpliceAt.prototype.tt = protoTermType.SPLICE_AT;

  SpliceAt.prototype.mt = 'spliceAt';

  return SpliceAt;

})(RDBOp);

DeleteAt = (function(_super) {
  __extends(DeleteAt, _super);

  function DeleteAt() {
    return DeleteAt.__super__.constructor.apply(this, arguments);
  }

  DeleteAt.prototype.tt = protoTermType.DELETE_AT;

  DeleteAt.prototype.mt = 'deleteAt';

  return DeleteAt;

})(RDBOp);

ChangeAt = (function(_super) {
  __extends(ChangeAt, _super);

  function ChangeAt() {
    return ChangeAt.__super__.constructor.apply(this, arguments);
  }

  ChangeAt.prototype.tt = protoTermType.CHANGE_AT;

  ChangeAt.prototype.mt = 'changeAt';

  return ChangeAt;

})(RDBOp);

HasFields = (function(_super) {
  __extends(HasFields, _super);

  function HasFields() {
    return HasFields.__super__.constructor.apply(this, arguments);
  }

  HasFields.prototype.tt = protoTermType.HAS_FIELDS;

  HasFields.prototype.mt = 'hasFields';

  return HasFields;

})(RDBOp);

WithFields = (function(_super) {
  __extends(WithFields, _super);

  function WithFields() {
    return WithFields.__super__.constructor.apply(this, arguments);
  }

  WithFields.prototype.tt = protoTermType.WITH_FIELDS;

  WithFields.prototype.mt = 'withFields';

  return WithFields;

})(RDBOp);

Keys = (function(_super) {
  __extends(Keys, _super);

  function Keys() {
    return Keys.__super__.constructor.apply(this, arguments);
  }

  Keys.prototype.tt = protoTermType.KEYS;

  Keys.prototype.mt = 'keys';

  return Keys;

})(RDBOp);

Changes = (function(_super) {
  __extends(Changes, _super);

  function Changes() {
    return Changes.__super__.constructor.apply(this, arguments);
  }

  Changes.prototype.tt = protoTermType.CHANGES;

  Changes.prototype.mt = 'changes';

  return Changes;

})(RDBOp);

Object_ = (function(_super) {
  __extends(Object_, _super);

  function Object_() {
    return Object_.__super__.constructor.apply(this, arguments);
  }

  Object_.prototype.tt = protoTermType.OBJECT;

  Object_.prototype.mt = 'object';

  return Object_;

})(RDBOp);

Pluck = (function(_super) {
  __extends(Pluck, _super);

  function Pluck() {
    return Pluck.__super__.constructor.apply(this, arguments);
  }

  Pluck.prototype.tt = protoTermType.PLUCK;

  Pluck.prototype.mt = 'pluck';

  return Pluck;

})(RDBOp);

OffsetsOf = (function(_super) {
  __extends(OffsetsOf, _super);

  function OffsetsOf() {
    return OffsetsOf.__super__.constructor.apply(this, arguments);
  }

  OffsetsOf.prototype.tt = protoTermType.OFFSETS_OF;

  OffsetsOf.prototype.mt = 'offsetsOf';

  return OffsetsOf;

})(RDBOp);

Without = (function(_super) {
  __extends(Without, _super);

  function Without() {
    return Without.__super__.constructor.apply(this, arguments);
  }

  Without.prototype.tt = protoTermType.WITHOUT;

  Without.prototype.mt = 'without';

  return Without;

})(RDBOp);

Merge = (function(_super) {
  __extends(Merge, _super);

  function Merge() {
    return Merge.__super__.constructor.apply(this, arguments);
  }

  Merge.prototype.tt = protoTermType.MERGE;

  Merge.prototype.mt = 'merge';

  return Merge;

})(RDBOp);

Between = (function(_super) {
  __extends(Between, _super);

  function Between() {
    return Between.__super__.constructor.apply(this, arguments);
  }

  Between.prototype.tt = protoTermType.BETWEEN;

  Between.prototype.mt = 'between';

  return Between;

})(RDBOp);

Reduce = (function(_super) {
  __extends(Reduce, _super);

  function Reduce() {
    return Reduce.__super__.constructor.apply(this, arguments);
  }

  Reduce.prototype.tt = protoTermType.REDUCE;

  Reduce.prototype.mt = 'reduce';

  return Reduce;

})(RDBOp);

Map = (function(_super) {
  __extends(Map, _super);

  function Map() {
    return Map.__super__.constructor.apply(this, arguments);
  }

  Map.prototype.tt = protoTermType.MAP;

  Map.prototype.mt = 'map';

  return Map;

})(RDBOp);

Filter = (function(_super) {
  __extends(Filter, _super);

  function Filter() {
    return Filter.__super__.constructor.apply(this, arguments);
  }

  Filter.prototype.tt = protoTermType.FILTER;

  Filter.prototype.mt = 'filter';

  return Filter;

})(RDBOp);

ConcatMap = (function(_super) {
  __extends(ConcatMap, _super);

  function ConcatMap() {
    return ConcatMap.__super__.constructor.apply(this, arguments);
  }

  ConcatMap.prototype.tt = protoTermType.CONCAT_MAP;

  ConcatMap.prototype.mt = 'concatMap';

  return ConcatMap;

})(RDBOp);

OrderBy = (function(_super) {
  __extends(OrderBy, _super);

  function OrderBy() {
    return OrderBy.__super__.constructor.apply(this, arguments);
  }

  OrderBy.prototype.tt = protoTermType.ORDER_BY;

  OrderBy.prototype.mt = 'orderBy';

  return OrderBy;

})(RDBOp);

Distinct = (function(_super) {
  __extends(Distinct, _super);

  function Distinct() {
    return Distinct.__super__.constructor.apply(this, arguments);
  }

  Distinct.prototype.tt = protoTermType.DISTINCT;

  Distinct.prototype.mt = 'distinct';

  return Distinct;

})(RDBOp);

Count = (function(_super) {
  __extends(Count, _super);

  function Count() {
    return Count.__super__.constructor.apply(this, arguments);
  }

  Count.prototype.tt = protoTermType.COUNT;

  Count.prototype.mt = 'count';

  return Count;

})(RDBOp);

Union = (function(_super) {
  __extends(Union, _super);

  function Union() {
    return Union.__super__.constructor.apply(this, arguments);
  }

  Union.prototype.tt = protoTermType.UNION;

  Union.prototype.mt = 'union';

  return Union;

})(RDBOp);

Nth = (function(_super) {
  __extends(Nth, _super);

  function Nth() {
    return Nth.__super__.constructor.apply(this, arguments);
  }

  Nth.prototype.tt = protoTermType.NTH;

  Nth.prototype.mt = 'nth';

  return Nth;

})(RDBOp);

ToJsonString = (function(_super) {
  __extends(ToJsonString, _super);

  function ToJsonString() {
    return ToJsonString.__super__.constructor.apply(this, arguments);
  }

  ToJsonString.prototype.tt = protoTermType.TO_JSON_STRING;

  ToJsonString.prototype.mt = 'toJsonString';

  return ToJsonString;

})(RDBOp);

Match = (function(_super) {
  __extends(Match, _super);

  function Match() {
    return Match.__super__.constructor.apply(this, arguments);
  }

  Match.prototype.tt = protoTermType.MATCH;

  Match.prototype.mt = 'match';

  return Match;

})(RDBOp);

Split = (function(_super) {
  __extends(Split, _super);

  function Split() {
    return Split.__super__.constructor.apply(this, arguments);
  }

  Split.prototype.tt = protoTermType.SPLIT;

  Split.prototype.mt = 'split';

  return Split;

})(RDBOp);

Upcase = (function(_super) {
  __extends(Upcase, _super);

  function Upcase() {
    return Upcase.__super__.constructor.apply(this, arguments);
  }

  Upcase.prototype.tt = protoTermType.UPCASE;

  Upcase.prototype.mt = 'upcase';

  return Upcase;

})(RDBOp);

Downcase = (function(_super) {
  __extends(Downcase, _super);

  function Downcase() {
    return Downcase.__super__.constructor.apply(this, arguments);
  }

  Downcase.prototype.tt = protoTermType.DOWNCASE;

  Downcase.prototype.mt = 'downcase';

  return Downcase;

})(RDBOp);

IsEmpty = (function(_super) {
  __extends(IsEmpty, _super);

  function IsEmpty() {
    return IsEmpty.__super__.constructor.apply(this, arguments);
  }

  IsEmpty.prototype.tt = protoTermType.IS_EMPTY;

  IsEmpty.prototype.mt = 'isEmpty';

  return IsEmpty;

})(RDBOp);

Group = (function(_super) {
  __extends(Group, _super);

  function Group() {
    return Group.__super__.constructor.apply(this, arguments);
  }

  Group.prototype.tt = protoTermType.GROUP;

  Group.prototype.mt = 'group';

  return Group;

})(RDBOp);

Sum = (function(_super) {
  __extends(Sum, _super);

  function Sum() {
    return Sum.__super__.constructor.apply(this, arguments);
  }

  Sum.prototype.tt = protoTermType.SUM;

  Sum.prototype.mt = 'sum';

  return Sum;

})(RDBOp);

Avg = (function(_super) {
  __extends(Avg, _super);

  function Avg() {
    return Avg.__super__.constructor.apply(this, arguments);
  }

  Avg.prototype.tt = protoTermType.AVG;

  Avg.prototype.mt = 'avg';

  return Avg;

})(RDBOp);

Min = (function(_super) {
  __extends(Min, _super);

  function Min() {
    return Min.__super__.constructor.apply(this, arguments);
  }

  Min.prototype.tt = protoTermType.MIN;

  Min.prototype.mt = 'min';

  return Min;

})(RDBOp);

Max = (function(_super) {
  __extends(Max, _super);

  function Max() {
    return Max.__super__.constructor.apply(this, arguments);
  }

  Max.prototype.tt = protoTermType.MAX;

  Max.prototype.mt = 'max';

  return Max;

})(RDBOp);

InnerJoin = (function(_super) {
  __extends(InnerJoin, _super);

  function InnerJoin() {
    return InnerJoin.__super__.constructor.apply(this, arguments);
  }

  InnerJoin.prototype.tt = protoTermType.INNER_JOIN;

  InnerJoin.prototype.mt = 'innerJoin';

  return InnerJoin;

})(RDBOp);

OuterJoin = (function(_super) {
  __extends(OuterJoin, _super);

  function OuterJoin() {
    return OuterJoin.__super__.constructor.apply(this, arguments);
  }

  OuterJoin.prototype.tt = protoTermType.OUTER_JOIN;

  OuterJoin.prototype.mt = 'outerJoin';

  return OuterJoin;

})(RDBOp);

EqJoin = (function(_super) {
  __extends(EqJoin, _super);

  function EqJoin() {
    return EqJoin.__super__.constructor.apply(this, arguments);
  }

  EqJoin.prototype.tt = protoTermType.EQ_JOIN;

  EqJoin.prototype.mt = 'eqJoin';

  return EqJoin;

})(RDBOp);

Zip = (function(_super) {
  __extends(Zip, _super);

  function Zip() {
    return Zip.__super__.constructor.apply(this, arguments);
  }

  Zip.prototype.tt = protoTermType.ZIP;

  Zip.prototype.mt = 'zip';

  return Zip;

})(RDBOp);

Range = (function(_super) {
  __extends(Range, _super);

  function Range() {
    return Range.__super__.constructor.apply(this, arguments);
  }

  Range.prototype.tt = protoTermType.RANGE;

  Range.prototype.st = 'range';

  return Range;

})(RDBOp);

CoerceTo = (function(_super) {
  __extends(CoerceTo, _super);

  function CoerceTo() {
    return CoerceTo.__super__.constructor.apply(this, arguments);
  }

  CoerceTo.prototype.tt = protoTermType.COERCE_TO;

  CoerceTo.prototype.mt = 'coerceTo';

  return CoerceTo;

})(RDBOp);

Ungroup = (function(_super) {
  __extends(Ungroup, _super);

  function Ungroup() {
    return Ungroup.__super__.constructor.apply(this, arguments);
  }

  Ungroup.prototype.tt = protoTermType.UNGROUP;

  Ungroup.prototype.mt = 'ungroup';

  return Ungroup;

})(RDBOp);

TypeOf = (function(_super) {
  __extends(TypeOf, _super);

  function TypeOf() {
    return TypeOf.__super__.constructor.apply(this, arguments);
  }

  TypeOf.prototype.tt = protoTermType.TYPE_OF;

  TypeOf.prototype.mt = 'typeOf';

  return TypeOf;

})(RDBOp);

Info = (function(_super) {
  __extends(Info, _super);

  function Info() {
    return Info.__super__.constructor.apply(this, arguments);
  }

  Info.prototype.tt = protoTermType.INFO;

  Info.prototype.mt = 'info';

  return Info;

})(RDBOp);

Sample = (function(_super) {
  __extends(Sample, _super);

  function Sample() {
    return Sample.__super__.constructor.apply(this, arguments);
  }

  Sample.prototype.tt = protoTermType.SAMPLE;

  Sample.prototype.mt = 'sample';

  return Sample;

})(RDBOp);

Update = (function(_super) {
  __extends(Update, _super);

  function Update() {
    return Update.__super__.constructor.apply(this, arguments);
  }

  Update.prototype.tt = protoTermType.UPDATE;

  Update.prototype.mt = 'update';

  return Update;

})(RDBOp);

Delete = (function(_super) {
  __extends(Delete, _super);

  function Delete() {
    return Delete.__super__.constructor.apply(this, arguments);
  }

  Delete.prototype.tt = protoTermType.DELETE;

  Delete.prototype.mt = 'delete';

  return Delete;

})(RDBOp);

Replace = (function(_super) {
  __extends(Replace, _super);

  function Replace() {
    return Replace.__super__.constructor.apply(this, arguments);
  }

  Replace.prototype.tt = protoTermType.REPLACE;

  Replace.prototype.mt = 'replace';

  return Replace;

})(RDBOp);

Insert = (function(_super) {
  __extends(Insert, _super);

  function Insert() {
    return Insert.__super__.constructor.apply(this, arguments);
  }

  Insert.prototype.tt = protoTermType.INSERT;

  Insert.prototype.mt = 'insert';

  return Insert;

})(RDBOp);

DbCreate = (function(_super) {
  __extends(DbCreate, _super);

  function DbCreate() {
    return DbCreate.__super__.constructor.apply(this, arguments);
  }

  DbCreate.prototype.tt = protoTermType.DB_CREATE;

  DbCreate.prototype.st = 'dbCreate';

  return DbCreate;

})(RDBOp);

DbDrop = (function(_super) {
  __extends(DbDrop, _super);

  function DbDrop() {
    return DbDrop.__super__.constructor.apply(this, arguments);
  }

  DbDrop.prototype.tt = protoTermType.DB_DROP;

  DbDrop.prototype.st = 'dbDrop';

  return DbDrop;

})(RDBOp);

DbList = (function(_super) {
  __extends(DbList, _super);

  function DbList() {
    return DbList.__super__.constructor.apply(this, arguments);
  }

  DbList.prototype.tt = protoTermType.DB_LIST;

  DbList.prototype.st = 'dbList';

  return DbList;

})(RDBOp);

TableCreate = (function(_super) {
  __extends(TableCreate, _super);

  function TableCreate() {
    return TableCreate.__super__.constructor.apply(this, arguments);
  }

  TableCreate.prototype.tt = protoTermType.TABLE_CREATE;

  TableCreate.prototype.mt = 'tableCreate';

  return TableCreate;

})(RDBOp);

TableDrop = (function(_super) {
  __extends(TableDrop, _super);

  function TableDrop() {
    return TableDrop.__super__.constructor.apply(this, arguments);
  }

  TableDrop.prototype.tt = protoTermType.TABLE_DROP;

  TableDrop.prototype.mt = 'tableDrop';

  return TableDrop;

})(RDBOp);

TableList = (function(_super) {
  __extends(TableList, _super);

  function TableList() {
    return TableList.__super__.constructor.apply(this, arguments);
  }

  TableList.prototype.tt = protoTermType.TABLE_LIST;

  TableList.prototype.mt = 'tableList';

  return TableList;

})(RDBOp);

IndexCreate = (function(_super) {
  __extends(IndexCreate, _super);

  function IndexCreate() {
    return IndexCreate.__super__.constructor.apply(this, arguments);
  }

  IndexCreate.prototype.tt = protoTermType.INDEX_CREATE;

  IndexCreate.prototype.mt = 'indexCreate';

  return IndexCreate;

})(RDBOp);

IndexDrop = (function(_super) {
  __extends(IndexDrop, _super);

  function IndexDrop() {
    return IndexDrop.__super__.constructor.apply(this, arguments);
  }

  IndexDrop.prototype.tt = protoTermType.INDEX_DROP;

  IndexDrop.prototype.mt = 'indexDrop';

  return IndexDrop;

})(RDBOp);

IndexRename = (function(_super) {
  __extends(IndexRename, _super);

  function IndexRename() {
    return IndexRename.__super__.constructor.apply(this, arguments);
  }

  IndexRename.prototype.tt = protoTermType.INDEX_RENAME;

  IndexRename.prototype.mt = 'indexRename';

  return IndexRename;

})(RDBOp);

IndexList = (function(_super) {
  __extends(IndexList, _super);

  function IndexList() {
    return IndexList.__super__.constructor.apply(this, arguments);
  }

  IndexList.prototype.tt = protoTermType.INDEX_LIST;

  IndexList.prototype.mt = 'indexList';

  return IndexList;

})(RDBOp);

IndexStatus = (function(_super) {
  __extends(IndexStatus, _super);

  function IndexStatus() {
    return IndexStatus.__super__.constructor.apply(this, arguments);
  }

  IndexStatus.prototype.tt = protoTermType.INDEX_STATUS;

  IndexStatus.prototype.mt = 'indexStatus';

  return IndexStatus;

})(RDBOp);

IndexWait = (function(_super) {
  __extends(IndexWait, _super);

  function IndexWait() {
    return IndexWait.__super__.constructor.apply(this, arguments);
  }

  IndexWait.prototype.tt = protoTermType.INDEX_WAIT;

  IndexWait.prototype.mt = 'indexWait';

  return IndexWait;

})(RDBOp);

Config = (function(_super) {
  __extends(Config, _super);

  function Config() {
    return Config.__super__.constructor.apply(this, arguments);
  }

  Config.prototype.tt = protoTermType.CONFIG;

  Config.prototype.mt = 'config';

  return Config;

})(RDBOp);

Status = (function(_super) {
  __extends(Status, _super);

  function Status() {
    return Status.__super__.constructor.apply(this, arguments);
  }

  Status.prototype.tt = protoTermType.STATUS;

  Status.prototype.mt = 'status';

  return Status;

})(RDBOp);

Wait = (function(_super) {
  __extends(Wait, _super);

  function Wait() {
    return Wait.__super__.constructor.apply(this, arguments);
  }

  Wait.prototype.tt = protoTermType.WAIT;

  Wait.prototype.mt = 'wait';

  return Wait;

})(RDBOp);

Reconfigure = (function(_super) {
  __extends(Reconfigure, _super);

  function Reconfigure() {
    return Reconfigure.__super__.constructor.apply(this, arguments);
  }

  Reconfigure.prototype.tt = protoTermType.RECONFIGURE;

  Reconfigure.prototype.mt = 'reconfigure';

  return Reconfigure;

})(RDBOp);

Rebalance = (function(_super) {
  __extends(Rebalance, _super);

  function Rebalance() {
    return Rebalance.__super__.constructor.apply(this, arguments);
  }

  Rebalance.prototype.tt = protoTermType.REBALANCE;

  Rebalance.prototype.mt = 'rebalance';

  return Rebalance;

})(RDBOp);

Sync = (function(_super) {
  __extends(Sync, _super);

  function Sync() {
    return Sync.__super__.constructor.apply(this, arguments);
  }

  Sync.prototype.tt = protoTermType.SYNC;

  Sync.prototype.mt = 'sync';

  return Sync;

})(RDBOp);

FunCall = (function(_super) {
  __extends(FunCall, _super);

  function FunCall() {
    return FunCall.__super__.constructor.apply(this, arguments);
  }

  FunCall.prototype.tt = protoTermType.FUNCALL;

  FunCall.prototype.st = 'do';

  FunCall.prototype.compose = function(args) {
    if (args.length > 2) {
      return ['r.do(', intsp(args.slice(1)), ', ', args[0], ')'];
    } else {
      if (shouldWrap(this.args[1])) {
        args[1] = ['r(', args[1], ')'];
      }
      return [args[1], '.do(', args[0], ')'];
    }
  };

  return FunCall;

})(RDBOp);

Default = (function(_super) {
  __extends(Default, _super);

  function Default() {
    return Default.__super__.constructor.apply(this, arguments);
  }

  Default.prototype.tt = protoTermType.DEFAULT;

  Default.prototype.mt = 'default';

  return Default;

})(RDBOp);

Branch = (function(_super) {
  __extends(Branch, _super);

  function Branch() {
    return Branch.__super__.constructor.apply(this, arguments);
  }

  Branch.prototype.tt = protoTermType.BRANCH;

  Branch.prototype.st = 'branch';

  return Branch;

})(RDBOp);

Or = (function(_super) {
  __extends(Or, _super);

  function Or() {
    return Or.__super__.constructor.apply(this, arguments);
  }

  Or.prototype.tt = protoTermType.OR;

  Or.prototype.mt = 'or';

  return Or;

})(RDBOp);

And = (function(_super) {
  __extends(And, _super);

  function And() {
    return And.__super__.constructor.apply(this, arguments);
  }

  And.prototype.tt = protoTermType.AND;

  And.prototype.mt = 'and';

  return And;

})(RDBOp);

ForEach = (function(_super) {
  __extends(ForEach, _super);

  function ForEach() {
    return ForEach.__super__.constructor.apply(this, arguments);
  }

  ForEach.prototype.tt = protoTermType.FOR_EACH;

  ForEach.prototype.mt = 'forEach';

  return ForEach;

})(RDBOp);

Func = (function(_super) {
  __extends(Func, _super);

  Func.prototype.tt = protoTermType.FUNC;

  Func.nextVarId = 0;

  function Func(optargs, func) {
    var argNums, args, argsArr, body, i;
    args = [];
    argNums = [];
    i = 0;
    while (i < func.length) {
      argNums.push(Func.nextVarId);
      args.push(new Var({}, Func.nextVarId));
      Func.nextVarId++;
      i++;
    }
    body = func.apply(null, args);
    if (body === void 0) {
      throw new err.RqlDriverError("Anonymous function returned `undefined`. Did you forget a `return`?");
    }
    argsArr = (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(MakeArray, [{}].concat(__slice.call(argNums)), function(){});
    return Func.__super__.constructor.call(this, optargs, argsArr, body);
  }

  Func.prototype.compose = function(args) {
    var arg, i, varStr, _i, _len, _ref;
    if (hasImplicit(args[1]) === true) {
      return [args[1]];
    } else {
      varStr = "";
      _ref = args[0][1];
      for (i = _i = 0, _len = _ref.length; _i < _len; i = ++_i) {
        arg = _ref[i];
        if (i % 2 === 0) {
          varStr += Var.prototype.compose(arg);
        } else {
          varStr += arg;
        }
      }
      return ['function(', varStr, ') { return ', args[1], '; }'];
    }
  };

  return Func;

})(RDBOp);

Asc = (function(_super) {
  __extends(Asc, _super);

  function Asc() {
    return Asc.__super__.constructor.apply(this, arguments);
  }

  Asc.prototype.tt = protoTermType.ASC;

  Asc.prototype.st = 'asc';

  return Asc;

})(RDBOp);

Desc = (function(_super) {
  __extends(Desc, _super);

  function Desc() {
    return Desc.__super__.constructor.apply(this, arguments);
  }

  Desc.prototype.tt = protoTermType.DESC;

  Desc.prototype.st = 'desc';

  return Desc;

})(RDBOp);

Literal = (function(_super) {
  __extends(Literal, _super);

  function Literal() {
    return Literal.__super__.constructor.apply(this, arguments);
  }

  Literal.prototype.tt = protoTermType.LITERAL;

  Literal.prototype.st = 'literal';

  return Literal;

})(RDBOp);

ISO8601 = (function(_super) {
  __extends(ISO8601, _super);

  function ISO8601() {
    return ISO8601.__super__.constructor.apply(this, arguments);
  }

  ISO8601.prototype.tt = protoTermType.ISO8601;

  ISO8601.prototype.st = 'ISO8601';

  return ISO8601;

})(RDBOp);

ToISO8601 = (function(_super) {
  __extends(ToISO8601, _super);

  function ToISO8601() {
    return ToISO8601.__super__.constructor.apply(this, arguments);
  }

  ToISO8601.prototype.tt = protoTermType.TO_ISO8601;

  ToISO8601.prototype.mt = 'toISO8601';

  return ToISO8601;

})(RDBOp);

EpochTime = (function(_super) {
  __extends(EpochTime, _super);

  function EpochTime() {
    return EpochTime.__super__.constructor.apply(this, arguments);
  }

  EpochTime.prototype.tt = protoTermType.EPOCH_TIME;

  EpochTime.prototype.st = 'epochTime';

  return EpochTime;

})(RDBOp);

ToEpochTime = (function(_super) {
  __extends(ToEpochTime, _super);

  function ToEpochTime() {
    return ToEpochTime.__super__.constructor.apply(this, arguments);
  }

  ToEpochTime.prototype.tt = protoTermType.TO_EPOCH_TIME;

  ToEpochTime.prototype.mt = 'toEpochTime';

  return ToEpochTime;

})(RDBOp);

Now = (function(_super) {
  __extends(Now, _super);

  function Now() {
    return Now.__super__.constructor.apply(this, arguments);
  }

  Now.prototype.tt = protoTermType.NOW;

  Now.prototype.st = 'now';

  return Now;

})(RDBOp);

InTimezone = (function(_super) {
  __extends(InTimezone, _super);

  function InTimezone() {
    return InTimezone.__super__.constructor.apply(this, arguments);
  }

  InTimezone.prototype.tt = protoTermType.IN_TIMEZONE;

  InTimezone.prototype.mt = 'inTimezone';

  return InTimezone;

})(RDBOp);

During = (function(_super) {
  __extends(During, _super);

  function During() {
    return During.__super__.constructor.apply(this, arguments);
  }

  During.prototype.tt = protoTermType.DURING;

  During.prototype.mt = 'during';

  return During;

})(RDBOp);

RQLDate = (function(_super) {
  __extends(RQLDate, _super);

  function RQLDate() {
    return RQLDate.__super__.constructor.apply(this, arguments);
  }

  RQLDate.prototype.tt = protoTermType.DATE;

  RQLDate.prototype.mt = 'date';

  return RQLDate;

})(RDBOp);

TimeOfDay = (function(_super) {
  __extends(TimeOfDay, _super);

  function TimeOfDay() {
    return TimeOfDay.__super__.constructor.apply(this, arguments);
  }

  TimeOfDay.prototype.tt = protoTermType.TIME_OF_DAY;

  TimeOfDay.prototype.mt = 'timeOfDay';

  return TimeOfDay;

})(RDBOp);

Timezone = (function(_super) {
  __extends(Timezone, _super);

  function Timezone() {
    return Timezone.__super__.constructor.apply(this, arguments);
  }

  Timezone.prototype.tt = protoTermType.TIMEZONE;

  Timezone.prototype.mt = 'timezone';

  return Timezone;

})(RDBOp);

Year = (function(_super) {
  __extends(Year, _super);

  function Year() {
    return Year.__super__.constructor.apply(this, arguments);
  }

  Year.prototype.tt = protoTermType.YEAR;

  Year.prototype.mt = 'year';

  return Year;

})(RDBOp);

Month = (function(_super) {
  __extends(Month, _super);

  function Month() {
    return Month.__super__.constructor.apply(this, arguments);
  }

  Month.prototype.tt = protoTermType.MONTH;

  Month.prototype.mt = 'month';

  return Month;

})(RDBOp);

Day = (function(_super) {
  __extends(Day, _super);

  function Day() {
    return Day.__super__.constructor.apply(this, arguments);
  }

  Day.prototype.tt = protoTermType.DAY;

  Day.prototype.mt = 'day';

  return Day;

})(RDBOp);

DayOfWeek = (function(_super) {
  __extends(DayOfWeek, _super);

  function DayOfWeek() {
    return DayOfWeek.__super__.constructor.apply(this, arguments);
  }

  DayOfWeek.prototype.tt = protoTermType.DAY_OF_WEEK;

  DayOfWeek.prototype.mt = 'dayOfWeek';

  return DayOfWeek;

})(RDBOp);

DayOfYear = (function(_super) {
  __extends(DayOfYear, _super);

  function DayOfYear() {
    return DayOfYear.__super__.constructor.apply(this, arguments);
  }

  DayOfYear.prototype.tt = protoTermType.DAY_OF_YEAR;

  DayOfYear.prototype.mt = 'dayOfYear';

  return DayOfYear;

})(RDBOp);

Hours = (function(_super) {
  __extends(Hours, _super);

  function Hours() {
    return Hours.__super__.constructor.apply(this, arguments);
  }

  Hours.prototype.tt = protoTermType.HOURS;

  Hours.prototype.mt = 'hours';

  return Hours;

})(RDBOp);

Minutes = (function(_super) {
  __extends(Minutes, _super);

  function Minutes() {
    return Minutes.__super__.constructor.apply(this, arguments);
  }

  Minutes.prototype.tt = protoTermType.MINUTES;

  Minutes.prototype.mt = 'minutes';

  return Minutes;

})(RDBOp);

Seconds = (function(_super) {
  __extends(Seconds, _super);

  function Seconds() {
    return Seconds.__super__.constructor.apply(this, arguments);
  }

  Seconds.prototype.tt = protoTermType.SECONDS;

  Seconds.prototype.mt = 'seconds';

  return Seconds;

})(RDBOp);

Time = (function(_super) {
  __extends(Time, _super);

  function Time() {
    return Time.__super__.constructor.apply(this, arguments);
  }

  Time.prototype.tt = protoTermType.TIME;

  Time.prototype.st = 'time';

  return Time;

})(RDBOp);

Geojson = (function(_super) {
  __extends(Geojson, _super);

  function Geojson() {
    return Geojson.__super__.constructor.apply(this, arguments);
  }

  Geojson.prototype.tt = protoTermType.GEOJSON;

  Geojson.prototype.st = 'geojson';

  return Geojson;

})(RDBOp);

ToGeojson = (function(_super) {
  __extends(ToGeojson, _super);

  function ToGeojson() {
    return ToGeojson.__super__.constructor.apply(this, arguments);
  }

  ToGeojson.prototype.tt = protoTermType.TO_GEOJSON;

  ToGeojson.prototype.mt = 'toGeojson';

  return ToGeojson;

})(RDBOp);

Point = (function(_super) {
  __extends(Point, _super);

  function Point() {
    return Point.__super__.constructor.apply(this, arguments);
  }

  Point.prototype.tt = protoTermType.POINT;

  Point.prototype.st = 'point';

  return Point;

})(RDBOp);

Line = (function(_super) {
  __extends(Line, _super);

  function Line() {
    return Line.__super__.constructor.apply(this, arguments);
  }

  Line.prototype.tt = protoTermType.LINE;

  Line.prototype.st = 'line';

  return Line;

})(RDBOp);

Polygon = (function(_super) {
  __extends(Polygon, _super);

  function Polygon() {
    return Polygon.__super__.constructor.apply(this, arguments);
  }

  Polygon.prototype.tt = protoTermType.POLYGON;

  Polygon.prototype.st = 'polygon';

  return Polygon;

})(RDBOp);

Distance = (function(_super) {
  __extends(Distance, _super);

  function Distance() {
    return Distance.__super__.constructor.apply(this, arguments);
  }

  Distance.prototype.tt = protoTermType.DISTANCE;

  Distance.prototype.mt = 'distance';

  return Distance;

})(RDBOp);

Intersects = (function(_super) {
  __extends(Intersects, _super);

  function Intersects() {
    return Intersects.__super__.constructor.apply(this, arguments);
  }

  Intersects.prototype.tt = protoTermType.INTERSECTS;

  Intersects.prototype.mt = 'intersects';

  return Intersects;

})(RDBOp);

Includes = (function(_super) {
  __extends(Includes, _super);

  function Includes() {
    return Includes.__super__.constructor.apply(this, arguments);
  }

  Includes.prototype.tt = protoTermType.INCLUDES;

  Includes.prototype.mt = 'includes';

  return Includes;

})(RDBOp);

Circle = (function(_super) {
  __extends(Circle, _super);

  function Circle() {
    return Circle.__super__.constructor.apply(this, arguments);
  }

  Circle.prototype.tt = protoTermType.CIRCLE;

  Circle.prototype.st = 'circle';

  return Circle;

})(RDBOp);

GetIntersecting = (function(_super) {
  __extends(GetIntersecting, _super);

  function GetIntersecting() {
    return GetIntersecting.__super__.constructor.apply(this, arguments);
  }

  GetIntersecting.prototype.tt = protoTermType.GET_INTERSECTING;

  GetIntersecting.prototype.mt = 'getIntersecting';

  return GetIntersecting;

})(RDBOp);

GetNearest = (function(_super) {
  __extends(GetNearest, _super);

  function GetNearest() {
    return GetNearest.__super__.constructor.apply(this, arguments);
  }

  GetNearest.prototype.tt = protoTermType.GET_NEAREST;

  GetNearest.prototype.mt = 'getNearest';

  return GetNearest;

})(RDBOp);

Fill = (function(_super) {
  __extends(Fill, _super);

  function Fill() {
    return Fill.__super__.constructor.apply(this, arguments);
  }

  Fill.prototype.tt = protoTermType.FILL;

  Fill.prototype.mt = 'fill';

  return Fill;

})(RDBOp);

PolygonSub = (function(_super) {
  __extends(PolygonSub, _super);

  function PolygonSub() {
    return PolygonSub.__super__.constructor.apply(this, arguments);
  }

  PolygonSub.prototype.tt = protoTermType.POLYGON_SUB;

  PolygonSub.prototype.mt = 'polygonSub';

  return PolygonSub;

})(RDBOp);

UUID = (function(_super) {
  __extends(UUID, _super);

  function UUID() {
    return UUID.__super__.constructor.apply(this, arguments);
  }

  UUID.prototype.tt = protoTermType.UUID;

  UUID.prototype.st = 'uuid';

  return UUID;

})(RDBOp);

rethinkdb.expr = varar(1, 2, function(val, nestingDepth) {
  var v;
  if (nestingDepth == null) {
    nestingDepth = 20;
  }
  if (val === void 0) {
    throw new err.RqlDriverError("Cannot wrap undefined with r.expr().");
  }
  if (nestingDepth <= 0) {
    throw new err.RqlDriverError("Nesting depth limit exceeded");
  }
  if (typeof nestingDepth !== "number" || isNaN(nestingDepth)) {
    throw new err.RqlDriverError("Second argument to `r.expr` must be a number or undefined.");
  } else if (val instanceof TermBase) {
    return val;
  } else if (typeof val === 'function') {
    return new Func({}, val);
  } else if (val instanceof Date) {
    return new ISO8601({}, val.toISOString());
  } else if (val instanceof Buffer) {
    return new Binary(val);
  } else if (Array.isArray(val)) {
    val = (function() {
      var _i, _len, _results;
      _results = [];
      for (_i = 0, _len = val.length; _i < _len; _i++) {
        v = val[_i];
        _results.push(rethinkdb.expr(v, nestingDepth - 1));
      }
      return _results;
    })();
    return (function(func, args, ctor) {
      ctor.prototype = func.prototype;
      var child = new ctor, result = func.apply(child, args);
      return Object(result) === result ? result : child;
    })(MakeArray, [{}].concat(__slice.call(val)), function(){});
  } else if (typeof val === 'number') {
    return new DatumTerm(val);
  } else if (Object.prototype.toString.call(val) === '[object Object]') {
    return new MakeObject(val, nestingDepth);
  } else {
    return new DatumTerm(val);
  }
});

rethinkdb.js = aropt(function(jssrc, opts) {
  return new JavaScript(opts, jssrc);
});

rethinkdb.http = aropt(function(url, opts) {
  return new Http(opts, url);
});

rethinkdb.json = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Json, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.error = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(UserError, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.random = function() {
  var limits, limitsAndOpts, opts, perhapsOptDict;
  limitsAndOpts = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  opts = {};
  limits = limitsAndOpts;
  perhapsOptDict = limitsAndOpts[limitsAndOpts.length - 1];
  if (perhapsOptDict && ((Object.prototype.toString.call(perhapsOptDict) === '[object Object]') && !(perhapsOptDict instanceof TermBase))) {
    opts = perhapsOptDict;
    limits = limitsAndOpts.slice(0, limitsAndOpts.length - 1);
  }
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Random, [opts].concat(__slice.call(limits)), function(){});
};

rethinkdb.binary = ar(function(data) {
  return new Binary(data);
});

rethinkdb.row = new ImplicitVar({});

rethinkdb.table = aropt(function(tblName, opts) {
  return new Table(opts, tblName);
});

rethinkdb.db = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Db, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.dbCreate = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(DbCreate, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.dbDrop = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(DbDrop, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.dbList = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(DbList, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.tableCreate = aropt(function(tblName, opts) {
  return new TableCreate(opts, tblName);
});

rethinkdb.tableDrop = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(TableDrop, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.tableList = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(TableList, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.wait = aropt(function(opts) {
  return new Wait(opts);
});

rethinkdb.reconfigure = function(opts) {
  return new Reconfigure(opts);
};

rethinkdb.rebalance = function() {
  return new Rebalance({});
};

rethinkdb["do"] = varar(1, null, function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(FunCall, [{}, funcWrap(args.slice(-1)[0])].concat(__slice.call(args.slice(0, -1))), function(){});
});

rethinkdb.branch = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Branch, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.map = varar(1, null, function() {
  var args, funcArg, _i;
  args = 2 <= arguments.length ? __slice.call(arguments, 0, _i = arguments.length - 1) : (_i = 0, []), funcArg = arguments[_i++];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Map, [{}].concat(__slice.call(args), [funcWrap(funcArg)]), function(){});
});

rethinkdb.asc = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Asc, [{}].concat(__slice.call(args.map(funcWrap))), function(){});
};

rethinkdb.desc = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Desc, [{}].concat(__slice.call(args.map(funcWrap))), function(){});
};

rethinkdb.eq = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Eq, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.ne = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Ne, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.lt = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Lt, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.le = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Le, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.gt = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Gt, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.ge = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Ge, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.or = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Or, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.and = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(And, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.not = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Not, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.add = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Add, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.sub = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Sub, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.div = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Div, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.mul = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Mul, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.mod = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Mod, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.typeOf = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(TypeOf, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.info = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Info, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.literal = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Literal, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.ISO8601 = aropt(function(str, opts) {
  return new ISO8601(opts, str);
});

rethinkdb.epochTime = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(EpochTime, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.now = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Now, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.time = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Time, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.monday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.MONDAY;

  _Class.prototype.st = 'monday';

  return _Class;

})(RDBConstant))();

rethinkdb.tuesday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.TUESDAY;

  _Class.prototype.st = 'tuesday';

  return _Class;

})(RDBConstant))();

rethinkdb.wednesday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.WEDNESDAY;

  _Class.prototype.st = 'wednesday';

  return _Class;

})(RDBConstant))();

rethinkdb.thursday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.THURSDAY;

  _Class.prototype.st = 'thursday';

  return _Class;

})(RDBConstant))();

rethinkdb.friday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.FRIDAY;

  _Class.prototype.st = 'friday';

  return _Class;

})(RDBConstant))();

rethinkdb.saturday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.SATURDAY;

  _Class.prototype.st = 'saturday';

  return _Class;

})(RDBConstant))();

rethinkdb.sunday = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.SUNDAY;

  _Class.prototype.st = 'sunday';

  return _Class;

})(RDBConstant))();

rethinkdb.january = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.JANUARY;

  _Class.prototype.st = 'january';

  return _Class;

})(RDBConstant))();

rethinkdb.february = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.FEBRUARY;

  _Class.prototype.st = 'february';

  return _Class;

})(RDBConstant))();

rethinkdb.march = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.MARCH;

  _Class.prototype.st = 'march';

  return _Class;

})(RDBConstant))();

rethinkdb.april = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.APRIL;

  _Class.prototype.st = 'april';

  return _Class;

})(RDBConstant))();

rethinkdb.may = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.MAY;

  _Class.prototype.st = 'may';

  return _Class;

})(RDBConstant))();

rethinkdb.june = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.JUNE;

  _Class.prototype.st = 'june';

  return _Class;

})(RDBConstant))();

rethinkdb.july = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.JULY;

  _Class.prototype.st = 'july';

  return _Class;

})(RDBConstant))();

rethinkdb.august = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.AUGUST;

  _Class.prototype.st = 'august';

  return _Class;

})(RDBConstant))();

rethinkdb.september = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.SEPTEMBER;

  _Class.prototype.st = 'september';

  return _Class;

})(RDBConstant))();

rethinkdb.october = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.OCTOBER;

  _Class.prototype.st = 'october';

  return _Class;

})(RDBConstant))();

rethinkdb.november = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.NOVEMBER;

  _Class.prototype.st = 'november';

  return _Class;

})(RDBConstant))();

rethinkdb.december = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.DECEMBER;

  _Class.prototype.st = 'december';

  return _Class;

})(RDBConstant))();

rethinkdb.minval = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.MINVAL;

  _Class.prototype.st = 'minval';

  return _Class;

})(RDBConstant))();

rethinkdb.maxval = new ((function(_super) {
  __extends(_Class, _super);

  function _Class() {
    return _Class.__super__.constructor.apply(this, arguments);
  }

  _Class.prototype.tt = protoTermType.MAXVAL;

  _Class.prototype.st = 'maxval';

  return _Class;

})(RDBConstant))();

rethinkdb.object = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Object_, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.args = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Args, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.geojson = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Geojson, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.point = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Point, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.line = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Line, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.polygon = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Polygon, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.intersects = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Intersects, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.distance = aropt(function(g1, g2, opts) {
  return new Distance(opts, g1, g2);
});

rethinkdb.circle = aropt(function(cen, rad, opts) {
  return new Circle(opts, cen, rad);
});

rethinkdb.uuid = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(UUID, [{}].concat(__slice.call(args)), function(){});
};

rethinkdb.range = function() {
  var args;
  args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
  return (function(func, args, ctor) {
    ctor.prototype = func.prototype;
    var child = new ctor, result = func.apply(child, args);
    return Object(result) === result ? result : child;
  })(Range, [{}].concat(__slice.call(args)), function(){});
};

module.exports = rethinkdb;
}).call(this,require("/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js"),require("buffer").Buffer)
},{"./errors":9,"./net":10,"./proto-def":46,"./util":49,"/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js":3,"bluebird":13,"buffer":4}],8:[function(require,module,exports){
// Generated by CoffeeScript 1.7.1
var ArrayResult, AtomFeed, Cursor, EventEmitter, Feed, IterableResult, OrderByLimitFeed, Promise, UnionedFeed, ar, aropt, err, mkErr, protoResponseType, setImmediate, util, varar,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __slice = [].slice,
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

err = require('./errors');

util = require('./util');

protoResponseType = require('./proto-def').Response.ResponseType;

Promise = require('bluebird');

EventEmitter = require('events').EventEmitter;

ar = util.ar;

varar = util.varar;

aropt = util.aropt;

mkErr = util.mkErr;

if (typeof setImmediate === "undefined" || setImmediate === null) {
  setImmediate = function(cb) {
    return setTimeout(cb, 0);
  };
}

IterableResult = (function() {
  IterableResult.prototype.stackSize = 100;

  function IterableResult(conn, token, opts, root) {
    this._eachCb = __bind(this._eachCb, this);
    this._conn = conn;
    this._token = token;
    this._opts = opts;
    this._root = root;
    this._responses = [];
    this._responseIndex = 0;
    this._outstandingRequests = 1;
    this._iterations = 0;
    this._endFlag = false;
    this._contFlag = false;
    this._closeAsap = false;
    this._cont = null;
    this._cbQueue = [];
    this._closeCb = null;
    this._closeCbPromise = null;
    this.next = this._next;
    this.each = this._each;
  }

  IterableResult.prototype._addResponse = function(response) {
    if (response.t === this._type || response.t === protoResponseType.SUCCESS_SEQUENCE) {
      if (response.r.length > 0) {
        this._responses.push(response);
      }
    } else {
      this._responses.push(response);
    }
    this._outstandingRequests -= 1;
    if (response.t !== this._type) {
      this._endFlag = true;
      if (this._closeCb != null) {
        switch (response.t) {
          case protoResponseType.COMPILE_ERROR:
            this._closeCb(mkErr(err.RqlRuntimeError, response, this._root));
            break;
          case protoResponseType.CLIENT_ERROR:
            this._closeCb(mkErr(err.RqlRuntimeError, response, this._root));
            break;
          case protoResponseType.RUNTIME_ERROR:
            this._closeCb(mkErr(err.RqlRuntimeError, response, this._root));
            break;
          default:
            this._closeCb();
        }
      }
    }
    this._contFlag = false;
    if (this._closeAsap === false) {
      this._promptNext();
    } else {
      this.close(this._closeCb);
    }
    return this;
  };

  IterableResult.prototype._getCallback = function() {
    var cb, immediateCb;
    this._iterations += 1;
    cb = this._cbQueue.shift();
    if (this._iterations % this.stackSize === this.stackSize - 1) {
      immediateCb = (function(err, row) {
        return setImmediate(function() {
          return cb(err, row);
        });
      });
      return immediateCb;
    } else {
      return cb;
    }
  };

  IterableResult.prototype._handleRow = function() {
    var cb, response, row;
    response = this._responses[0];
    row = util.recursivelyConvertPseudotype(response.r[this._responseIndex], this._opts);
    cb = this._getCallback();
    this._responseIndex += 1;
    if (this._responseIndex === response.r.length) {
      this._responses.shift();
      this._responseIndex = 0;
    }
    return cb(null, row);
  };

  IterableResult.prototype.bufferEmpty = function() {
    return this._responses.length === 0 || this._responses[0].r.length <= this._responseIndex;
  };

  IterableResult.prototype._promptNext = function() {
    var cb, response;
    while (this._cbQueue[0] != null) {
      if (this.bufferEmpty() === true) {
        if (this._endFlag === true) {
          cb = this._getCallback();
          cb(new err.RqlDriverError("No more rows in the cursor."));
        } else if (this._responses.length <= 1) {
          this._promptCont();
        }
        return;
      } else {
        response = this._responses[0];
        if (this._responses.length === 1) {
          this._promptCont();
        }
        switch (response.t) {
          case protoResponseType.SUCCESS_PARTIAL:
            this._handleRow();
            break;
          case protoResponseType.SUCCESS_SEQUENCE:
            if (response.r.length === 0) {
              this._responses.shift();
            } else {
              this._handleRow();
            }
            break;
          case protoResponseType.COMPILE_ERROR:
            this._responses.shift();
            cb = this._getCallback();
            cb(mkErr(err.RqlCompileError, response, this._root));
            break;
          case protoResponseType.CLIENT_ERROR:
            this._responses.shift();
            cb = this._getCallback();
            cb(mkErr(err.RqlClientError, response, this._root));
            break;
          case protoResponseType.RUNTIME_ERROR:
            this._responses.shift();
            cb = this._getCallback();
            cb(mkErr(err.RqlRuntimeError, response, this._root));
            break;
          default:
            this._responses.shift();
            cb = this._getCallback();
            cb(new err.RqlDriverError("Unknown response type for cursor"));
        }
      }
    }
  };

  IterableResult.prototype._promptCont = function() {
    if ((!this._contFlag) && (!this._endFlag) && this._conn.isOpen()) {
      this._contFlag = true;
      this._outstandingRequests += 1;
      return this._conn._continueQuery(this._token);
    }
  };

  IterableResult.prototype.hasNext = function() {
    throw new err.RqlDriverError("The `hasNext` command has been removed since 1.13. Use `next` instead.");
  };

  IterableResult.prototype._next = varar(0, 1, function(cb) {
    var fn, p;
    fn = (function(_this) {
      return function(cb) {
        _this._cbQueue.push(cb);
        return _this._promptNext();
      };
    })(this);
    if (typeof cb === "function") {
      return fn(cb);
    } else if (cb === void 0) {
      p = new Promise(function(resolve, reject) {
        cb = function(err, result) {
          if (err) {
            return reject(err);
          } else {
            return resolve(result);
          }
        };
        return fn(cb);
      });
      return p;
    } else {
      throw new err.RqlDriverError("First argument to `next` must be a function or undefined.");
    }
  });

  IterableResult.prototype.close = varar(0, 1, function(cb) {
    if (this._closeCbPromise != null) {
      if (this._closeCbPromise.isPending()) {
        this._closeCbPromise = this._closeCbPromise.nodeify(cb);
      } else {
        this._closeCbPromise = Promise.resolve().nodeify(cb);
      }
    } else {
      if (this._endFlag) {
        this._closeCbPromise = Promise.resolve().nodeify(cb);
      } else {
        this._closeCbPromise = new Promise((function(_this) {
          return function(resolve, reject) {
            _this._closeCb = function(err) {
              while (_this._cbQueue.length > 0) {
                _this._cbQueue.shift();
              }
              _this._outstandingRequests = 0;
              if (err) {
                return reject(err);
              } else {
                return resolve();
              }
            };
            _this._closeAsap = true;
            _this._outstandingRequests += 1;
            return _this._conn._endQuery(_this._token);
          };
        })(this)).nodeify(cb);
      }
    }
    return this._closeCbPromise;
  });

  IterableResult.prototype._each = varar(1, 2, function(cb, onFinished) {
    var nextCb, self, stopFlag;
    if (typeof cb !== 'function') {
      throw new err.RqlDriverError("First argument to each must be a function.");
    }
    if ((onFinished != null) && typeof onFinished !== 'function') {
      throw new err.RqlDriverError("Optional second argument to each must be a function.");
    }
    stopFlag = false;
    self = this;
    nextCb = (function(_this) {
      return function(err, data) {
        if (stopFlag !== true) {
          if (err != null) {
            if (err.message === 'No more rows in the cursor.') {
              if (onFinished != null) {
                return onFinished();
              }
            } else {
              return cb(err);
            }
          } else {
            stopFlag = cb(null, data) === false;
            return _this._next(nextCb);
          }
        } else if (onFinished != null) {
          return onFinished();
        }
      };
    })(this);
    return this._next(nextCb);
  });

  IterableResult.prototype.toArray = varar(0, 1, function(cb) {
    var fn;
    fn = (function(_this) {
      return function(cb) {
        var arr, eachCb, onFinish;
        arr = [];
        eachCb = function(err, row) {
          if (err != null) {
            return cb(err);
          } else {
            return arr.push(row);
          }
        };
        onFinish = function(err, ar) {
          return cb(null, arr);
        };
        return _this.each(eachCb, onFinish);
      };
    })(this);
    if ((cb != null) && typeof cb !== 'function') {
      throw new err.RqlDriverError("First argument to `toArray` must be a function or undefined.");
    }
    return new Promise((function(_this) {
      return function(resolve, reject) {
        var toArrayCb;
        toArrayCb = function(err, result) {
          if (err != null) {
            return reject(err);
          } else {
            return resolve(result);
          }
        };
        return fn(toArrayCb);
      };
    })(this)).nodeify(cb);
  });

  IterableResult.prototype._makeEmitter = function() {
    this.emitter = new EventEmitter;
    this.each = function() {
      throw new err.RqlDriverError("You cannot use the cursor interface and the EventEmitter interface at the same time.");
    };
    return this.next = function() {
      throw new err.RqlDriverError("You cannot use the cursor interface and the EventEmitter interface at the same time.");
    };
  };

  IterableResult.prototype.addListener = function(event, listener) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.addListener(event, listener);
  };

  IterableResult.prototype.on = function(event, listener) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.on(event, listener);
  };

  IterableResult.prototype.once = function(event, listener) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.once(event, listener);
  };

  IterableResult.prototype.removeListener = function(event, listener) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.removeListener(event, listener);
  };

  IterableResult.prototype.removeAllListeners = function(event) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.removeAllListeners(event);
  };

  IterableResult.prototype.setMaxListeners = function(n) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.setMaxListeners(n);
  };

  IterableResult.prototype.listeners = function(event) {
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return this.emitter.listeners(event);
  };

  IterableResult.prototype.emit = function() {
    var args, _ref;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    if (this.emitter == null) {
      this._makeEmitter();
      setImmediate((function(_this) {
        return function() {
          return _this._each(_this._eachCb);
        };
      })(this));
    }
    return (_ref = this.emitter).emit.apply(_ref, args);
  };

  IterableResult.prototype._eachCb = function(err, data) {
    if (err != null) {
      return this.emitter.emit('error', err);
    } else {
      return this.emitter.emit('data', data);
    }
  };

  return IterableResult;

})();

Cursor = (function(_super) {
  __extends(Cursor, _super);

  function Cursor() {
    this._type = protoResponseType.SUCCESS_PARTIAL;
    Cursor.__super__.constructor.apply(this, arguments);
  }

  Cursor.prototype.toString = ar(function() {
    return "[object Cursor]";
  });

  return Cursor;

})(IterableResult);

Feed = (function(_super) {
  __extends(Feed, _super);

  function Feed() {
    this._type = protoResponseType.SUCCESS_PARTIAL;
    Feed.__super__.constructor.apply(this, arguments);
  }

  Feed.prototype.hasNext = function() {
    throw new err.RqlDriverError("`hasNext` is not available for feeds.");
  };

  Feed.prototype.toArray = function() {
    throw new err.RqlDriverError("`toArray` is not available for feeds.");
  };

  Feed.prototype.toString = ar(function() {
    return "[object Feed]";
  });

  return Feed;

})(IterableResult);

UnionedFeed = (function(_super) {
  __extends(UnionedFeed, _super);

  function UnionedFeed() {
    this._type = protoResponseType.SUCCESS_PARTIAL;
    UnionedFeed.__super__.constructor.apply(this, arguments);
  }

  UnionedFeed.prototype.hasNext = function() {
    throw new err.RqlDriverError("`hasNext` is not available for feeds.");
  };

  UnionedFeed.prototype.toArray = function() {
    throw new err.RqlDriverError("`toArray` is not available for feeds.");
  };

  UnionedFeed.prototype.toString = ar(function() {
    return "[object UnionedFeed]";
  });

  return UnionedFeed;

})(IterableResult);

AtomFeed = (function(_super) {
  __extends(AtomFeed, _super);

  function AtomFeed() {
    this._type = protoResponseType.SUCCESS_PARTIAL;
    AtomFeed.__super__.constructor.apply(this, arguments);
  }

  AtomFeed.prototype.hasNext = function() {
    throw new err.RqlDriverError("`hasNext` is not available for feeds.");
  };

  AtomFeed.prototype.toArray = function() {
    throw new err.RqlDriverError("`toArray` is not available for feeds.");
  };

  AtomFeed.prototype.toString = ar(function() {
    return "[object AtomFeed]";
  });

  return AtomFeed;

})(IterableResult);

OrderByLimitFeed = (function(_super) {
  __extends(OrderByLimitFeed, _super);

  function OrderByLimitFeed() {
    this._type = protoResponseType.SUCCESS_PARTIAL;
    OrderByLimitFeed.__super__.constructor.apply(this, arguments);
  }

  OrderByLimitFeed.prototype.hasNext = function() {
    throw new err.RqlDriverError("`hasNext` is not available for feeds.");
  };

  OrderByLimitFeed.prototype.toArray = function() {
    throw new err.RqlDriverError("`toArray` is not available for feeds.");
  };

  OrderByLimitFeed.prototype.toString = ar(function() {
    return "[object OrderByLimitFeed]";
  });

  return OrderByLimitFeed;

})(IterableResult);

ArrayResult = (function(_super) {
  __extends(ArrayResult, _super);

  function ArrayResult() {
    return ArrayResult.__super__.constructor.apply(this, arguments);
  }

  ArrayResult.prototype._hasNext = ar(function() {
    if (this.__index == null) {
      this.__index = 0;
    }
    return this.__index < this.length;
  });

  ArrayResult.prototype._next = varar(0, 1, function(cb) {
    var fn;
    fn = (function(_this) {
      return function(cb) {
        var self;
        if (_this._hasNext() === true) {
          self = _this;
          if (self.__index % _this.stackSize === _this.stackSize - 1) {
            return setImmediate(function() {
              return cb(null, self[self.__index++]);
            });
          } else {
            return cb(null, self[self.__index++]);
          }
        } else {
          return cb(new err.RqlDriverError("No more rows in the cursor."));
        }
      };
    })(this);
    return new Promise(function(resolve, reject) {
      var nextCb;
      nextCb = function(err, result) {
        if (err) {
          return reject(err);
        } else {
          return resolve(result);
        }
      };
      return fn(nextCb);
    }).nodeify(cb);
  });

  ArrayResult.prototype.toArray = varar(0, 1, function(cb) {
    var fn;
    fn = (function(_this) {
      return function(cb) {
        if (_this.__index != null) {
          return cb(null, _this.slice(_this.__index, _this.length));
        } else {
          return cb(null, _this);
        }
      };
    })(this);
    return new Promise(function(resolve, reject) {
      var toArrayCb;
      toArrayCb = function(err, result) {
        if (err) {
          return reject(err);
        } else {
          return resolve(result);
        }
      };
      return fn(toArrayCb);
    }).nodeify(cb);
  });

  ArrayResult.prototype.close = function() {
    return this;
  };

  ArrayResult.prototype.makeIterable = function(response) {
    var method, name, _ref;
    response.__proto__ = {};
    _ref = ArrayResult.prototype;
    for (name in _ref) {
      method = _ref[name];
      if (name !== 'constructor') {
        if (name === '_each') {
          response.__proto__['each'] = method;
          response.__proto__['_each'] = method;
        } else if (name === '_next') {
          response.__proto__['next'] = method;
          response.__proto__['_next'] = method;
        } else {
          response.__proto__[name] = method;
        }
      }
    }
    response.__proto__.__proto__ = [].__proto__;
    return response;
  };

  return ArrayResult;

})(IterableResult);

module.exports.Cursor = Cursor;

module.exports.Feed = Feed;

module.exports.AtomFeed = AtomFeed;

module.exports.OrderByLimitFeed = OrderByLimitFeed;

module.exports.makeIterable = ArrayResult.prototype.makeIterable;

},{"./errors":9,"./proto-def":46,"./util":49,"bluebird":13,"events":2}],9:[function(require,module,exports){
// Generated by CoffeeScript 1.7.1
var RqlClientError, RqlCompileError, RqlDriverError, RqlQueryPrinter, RqlRuntimeError, RqlServerError,
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

RqlDriverError = (function(_super) {
  __extends(RqlDriverError, _super);

  function RqlDriverError(msg) {
    this.name = this.constructor.name;
    this.msg = msg;
    this.message = msg;
    if (Error.captureStackTrace != null) {
      Error.captureStackTrace(this, this);
    }
  }

  return RqlDriverError;

})(Error);

RqlServerError = (function(_super) {
  __extends(RqlServerError, _super);

  function RqlServerError(msg, term, frames) {
    this.name = this.constructor.name;
    this.msg = msg;
    this.frames = frames.slice(0);
    if (term != null) {
      if (msg[msg.length - 1] === '.') {
        this.message = "" + (msg.slice(0, msg.length - 1)) + " in:\n" + (RqlQueryPrinter.prototype.printQuery(term)) + "\n" + (RqlQueryPrinter.prototype.printCarrots(term, frames));
      } else {
        this.message = "" + msg + " in:\n" + (RqlQueryPrinter.prototype.printQuery(term)) + "\n" + (RqlQueryPrinter.prototype.printCarrots(term, frames));
      }
    } else {
      this.message = "" + msg;
    }
    if (Error.captureStackTrace != null) {
      Error.captureStackTrace(this, this);
    }
  }

  return RqlServerError;

})(Error);

RqlRuntimeError = (function(_super) {
  __extends(RqlRuntimeError, _super);

  function RqlRuntimeError() {
    return RqlRuntimeError.__super__.constructor.apply(this, arguments);
  }

  return RqlRuntimeError;

})(RqlServerError);

RqlCompileError = (function(_super) {
  __extends(RqlCompileError, _super);

  function RqlCompileError() {
    return RqlCompileError.__super__.constructor.apply(this, arguments);
  }

  return RqlCompileError;

})(RqlServerError);

RqlClientError = (function(_super) {
  __extends(RqlClientError, _super);

  function RqlClientError() {
    return RqlClientError.__super__.constructor.apply(this, arguments);
  }

  return RqlClientError;

})(RqlServerError);

RqlQueryPrinter = (function() {
  var carrotMarker, carrotify, composeCarrots, composeTerm, joinTree;

  function RqlQueryPrinter() {}

  RqlQueryPrinter.prototype.printQuery = function(term) {
    var tree;
    tree = composeTerm(term);
    return joinTree(tree);
  };

  composeTerm = function(term) {
    var arg, args, key, optargs, _ref;
    args = (function() {
      var _i, _len, _ref, _results;
      _ref = term.args;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        arg = _ref[_i];
        _results.push(composeTerm(arg));
      }
      return _results;
    })();
    optargs = {};
    _ref = term.optargs;
    for (key in _ref) {
      if (!__hasProp.call(_ref, key)) continue;
      arg = _ref[key];
      optargs[key] = composeTerm(arg);
    }
    return term.compose(args, optargs);
  };

  RqlQueryPrinter.prototype.printCarrots = function(term, frames) {
    var tree;
    if (frames.length === 0) {
      tree = [carrotify(composeTerm(term))];
    } else {
      tree = composeCarrots(term, frames);
    }
    return (joinTree(tree)).replace(/[^\^]/g, ' ');
  };

  composeCarrots = function(term, frames) {
    var arg, args, frame, i, key, optargs, _ref;
    frame = frames.shift();
    args = (function() {
      var _i, _len, _ref, _results;
      _ref = term.args;
      _results = [];
      for (i = _i = 0, _len = _ref.length; _i < _len; i = ++_i) {
        arg = _ref[i];
        if (frame === i) {
          _results.push(composeCarrots(arg, frames));
        } else {
          _results.push(composeTerm(arg));
        }
      }
      return _results;
    })();
    optargs = {};
    _ref = term.optargs;
    for (key in _ref) {
      if (!__hasProp.call(_ref, key)) continue;
      arg = _ref[key];
      if (frame === key) {
        optargs[key] = composeCarrots(arg, frames);
      } else {
        optargs[key] = composeTerm(arg);
      }
    }
    if (frame != null) {
      return term.compose(args, optargs);
    } else {
      return carrotify(term.compose(args, optargs));
    }
  };

  carrotMarker = {};

  carrotify = function(tree) {
    return [carrotMarker, tree];
  };

  joinTree = function(tree) {
    var str, term, _i, _len;
    str = '';
    for (_i = 0, _len = tree.length; _i < _len; _i++) {
      term = tree[_i];
      if (Array.isArray(term)) {
        if (term.length === 2 && term[0] === carrotMarker) {
          str += (joinTree(term[1])).replace(/./g, '^');
        } else {
          str += joinTree(term);
        }
      } else {
        str += term;
      }
    }
    return str;
  };

  return RqlQueryPrinter;

})();

module.exports.RqlDriverError = RqlDriverError;

module.exports.RqlRuntimeError = RqlRuntimeError;

module.exports.RqlCompileError = RqlCompileError;

module.exports.RqlClientError = RqlClientError;

module.exports.printQuery = RqlQueryPrinter.prototype.printQuery;

},{}],10:[function(require,module,exports){
(function (process,Buffer){// Generated by CoffeeScript 1.7.1
var Connection, HttpConnection, Promise, TcpConnection, ar, aropt, cursors, err, events, mkAtom, mkErr, net, protoProtocol, protoQueryType, protoResponseType, protoVersion, protodef, r, util, varar,
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

net = require('net');

events = require('events');

util = require('./util');

err = require('./errors');

cursors = require('./cursor');

protodef = require('./proto-def');

protoVersion = protodef.VersionDummy.Version.V0_4;

protoProtocol = protodef.VersionDummy.Protocol.JSON;

protoQueryType = protodef.Query.QueryType;

protoResponseType = protodef.Response.ResponseType;

r = require('./ast');

Promise = require('bluebird');

ar = util.ar;

varar = util.varar;

aropt = util.aropt;

mkAtom = util.mkAtom;

mkErr = util.mkErr;

Connection = (function(_super) {
  __extends(Connection, _super);

  Connection.prototype.DEFAULT_HOST = 'localhost';

  Connection.prototype.DEFAULT_PORT = 28015;

  Connection.prototype.DEFAULT_AUTH_KEY = '';

  Connection.prototype.DEFAULT_TIMEOUT = 20;

  function Connection(host, callback) {
    var conCallback, errCallback;
    if (typeof host === 'undefined') {
      host = {};
    } else if (typeof host === 'string') {
      host = {
        host: host
      };
    }
    this.host = host.host || this.DEFAULT_HOST;
    this.port = host.port || this.DEFAULT_PORT;
    this.db = host.db;
    this.authKey = host.authKey || this.DEFAULT_AUTH_KEY;
    this.timeout = host.timeout || this.DEFAULT_TIMEOUT;
    this.outstandingCallbacks = {};
    this.nextToken = 1;
    this.open = false;
    this.closing = false;
    this.buffer = new Buffer(0);
    this._events = this._events || {};
    errCallback = (function(_this) {
      return function(e) {
        _this.removeListener('connect', conCallback);
        if (e instanceof err.RqlDriverError) {
          return callback(e);
        } else {
          return callback(new err.RqlDriverError("Could not connect to " + _this.host + ":" + _this.port + ".\n" + e.message));
        }
      };
    })(this);
    this.once('error', errCallback);
    conCallback = (function(_this) {
      return function() {
        _this.removeListener('error', errCallback);
        _this.open = true;
        return callback(null, _this);
      };
    })(this);
    this.once('connect', conCallback);
    this._closePromise = null;
  }

  Connection.prototype._data = function(buf) {
    var response, responseBuffer, responseLength, token, _results;
    this.buffer = Buffer.concat([this.buffer, buf]);
    _results = [];
    while (this.buffer.length >= 12) {
      token = this.buffer.readUInt32LE(0) + 0x100000000 * this.buffer.readUInt32LE(4);
      responseLength = this.buffer.readUInt32LE(8);
      if (!(this.buffer.length >= (12 + responseLength))) {
        break;
      }
      responseBuffer = this.buffer.slice(12, responseLength + 12);
      response = JSON.parse(responseBuffer);
      this._processResponse(response, token);
      _results.push(this.buffer = this.buffer.slice(12 + responseLength));
    }
    return _results;
  };

  Connection.prototype._delQuery = function(token) {
    return delete this.outstandingCallbacks[token];
  };

  Connection.prototype._processResponse = function(response, token) {
    var cb, cursor, feed, note, opts, profile, root, _i, _len, _ref, _ref1;
    profile = response.p;
    if (this.outstandingCallbacks[token] != null) {
      _ref = this.outstandingCallbacks[token], cb = _ref.cb, root = _ref.root, cursor = _ref.cursor, opts = _ref.opts, feed = _ref.feed;
      if (cursor != null) {
        cursor._addResponse(response);
        if (cursor._endFlag && cursor._outstandingRequests === 0) {
          return this._delQuery(token);
        }
      } else if (cb != null) {
        switch (response.t) {
          case protoResponseType.COMPILE_ERROR:
            cb(mkErr(err.RqlCompileError, response, root));
            return this._delQuery(token);
          case protoResponseType.CLIENT_ERROR:
            cb(mkErr(err.RqlClientError, response, root));
            return this._delQuery(token);
          case protoResponseType.RUNTIME_ERROR:
            cb(mkErr(err.RqlRuntimeError, response, root));
            return this._delQuery(token);
          case protoResponseType.SUCCESS_ATOM:
            response = mkAtom(response, opts);
            if (Array.isArray(response)) {
              response = cursors.makeIterable(response);
            }
            if (profile != null) {
              response = {
                profile: profile,
                value: response
              };
            }
            cb(null, response);
            return this._delQuery(token);
          case protoResponseType.SUCCESS_PARTIAL:
            cursor = null;
            _ref1 = response.n;
            for (_i = 0, _len = _ref1.length; _i < _len; _i++) {
              note = _ref1[_i];
              switch (note) {
                case protodef.Response.ResponseNote.SEQUENCE_FEED:
                  if (cursor == null) {
                    cursor = new cursors.Feed(this, token, opts, root);
                  }
                  break;
                case protodef.Response.ResponseNote.UNIONED_FEED:
                  if (cursor == null) {
                    cursor = new cursors.UnionedFeed(this, token, opts, root);
                  }
                  break;
                case protodef.Response.ResponseNote.ATOM_FEED:
                  if (cursor == null) {
                    cursor = new cursors.AtomFeed(this, token, opts, root);
                  }
                  break;
                case protodef.Response.ResponseNote.ORDER_BY_LIMIT_FEED:
                  if (cursor == null) {
                    cursor = new cursors.OrderByLimitFeed(this, token, opts, root);
                  }
              }
            }
            if (cursor == null) {
              cursor = new cursors.Cursor(this, token, opts, root);
            }
            this.outstandingCallbacks[token].cursor = cursor;
            if (profile != null) {
              return cb(null, {
                profile: profile,
                value: cursor._addResponse(response)
              });
            } else {
              return cb(null, cursor._addResponse(response));
            }
            break;
          case protoResponseType.SUCCESS_SEQUENCE:
            cursor = new cursors.Cursor(this, token, opts, root);
            this._delQuery(token);
            if (profile != null) {
              return cb(null, {
                profile: profile,
                value: cursor._addResponse(response)
              });
            } else {
              return cb(null, cursor._addResponse(response));
            }
            break;
          case protoResponseType.WAIT_COMPLETE:
            this._delQuery(token);
            return cb(null, null);
          default:
            return cb(new err.RqlDriverError("Unknown response type"));
        }
      }
    } else {

    }
  };

  Connection.prototype.close = varar(0, 2, function(optsOrCallback, callback) {
    var cb, key, noreplyWait, opts;
    if (callback != null) {
      opts = optsOrCallback;
      if (Object.prototype.toString.call(opts) !== '[object Object]') {
        throw new err.RqlDriverError("First argument to two-argument `close` must be an object.");
      }
      cb = callback;
    } else if (Object.prototype.toString.call(optsOrCallback) === '[object Object]') {
      opts = optsOrCallback;
      cb = null;
    } else if (typeof optsOrCallback === 'function') {
      opts = {};
      cb = optsOrCallback;
    } else {
      opts = optsOrCallback;
      cb = null;
    }
    for (key in opts) {
      if (!__hasProp.call(opts, key)) continue;
      if (key !== 'noreplyWait') {
        throw new err.RqlDriverError("First argument to two-argument `close` must be { noreplyWait: <bool> }.");
      }
    }
    if (this._closePromise != null) {
      return this._closePromise.nodeify(cb);
    } else if (!this.open) {
      return new Promise((function(_this) {
        return function(resolve, reject) {
          if (cb != null) {
            cb(null, _this);
          } else {

          }
          return process.nextTick(resolve);
        };
      })(this));
    }
    this.closing = true;
    noreplyWait = ((opts.noreplyWait == null) || opts.noreplyWait) && this.open;
    return this._closePromise = new Promise((function(_this) {
      return function(resolve, reject) {
        var wrappedCb;
        wrappedCb = function(err, result) {
          _this.open = false;
          _this.closing = false;
          _this.cancel();
          _this._closePromise = null;
          if (err != null) {
            return reject(err);
          } else {
            return resolve(result);
          }
        };
        if (noreplyWait) {
          return _this.noreplyWait(wrappedCb);
        } else {
          return wrappedCb();
        }
      };
    })(this)).nodeify(cb);
  });

  Connection.prototype.noreplyWait = varar(0, 1, function(callback) {
    var query, token;
    if (!this.open) {
      return new Promise(function(resolve, reject) {
        return reject(new err.RqlDriverError("Connection is closed."));
      }).nodeify(callback);
    }
    token = this.nextToken++;
    query = {};
    query.type = protoQueryType.NOREPLY_WAIT;
    query.token = token;
    return new Promise((function(_this) {
      return function(resolve, reject) {
        var wrappedCb;
        wrappedCb = function(err, result) {
          if (err) {
            return reject(err);
          } else {
            return resolve(result);
          }
        };
        _this.outstandingCallbacks[token] = {
          cb: wrappedCb,
          root: null,
          opts: null
        };
        return _this._sendQuery(query);
      };
    })(this)).nodeify(callback);
  });

  Connection.prototype.cancel = ar(function() {
    var key, response, value, _ref;
    response = {
      t: protoResponseType.RUNTIME_ERROR,
      r: ["Connection is closed."],
      b: []
    };
    _ref = this.outstandingCallbacks;
    for (key in _ref) {
      if (!__hasProp.call(_ref, key)) continue;
      value = _ref[key];
      if (value.cursor != null) {
        value.cursor._addResponse(response);
      } else if (value.cb != null) {
        value.cb(mkErr(err.RqlRuntimeError, response, value.root));
      }
    }
    return this.outstandingCallbacks = {};
  });

  Connection.prototype.reconnect = varar(0, 2, function(optsOrCallback, callback) {
    var cb, opts;
    if (callback != null) {
      opts = optsOrCallback;
      cb = callback;
    } else if (typeof optsOrCallback === "function") {
      opts = {};
      cb = optsOrCallback;
    } else {
      if (optsOrCallback != null) {
        opts = optsOrCallback;
      } else {
        opts = {};
      }
      cb = callback;
    }
    return new Promise((function(_this) {
      return function(resolve, reject) {
        var closeCb;
        closeCb = function(err) {
          return _this.constructor.call(_this, {
            host: _this.host,
            port: _this.port,
            timeout: _this.timeout,
            authKey: _this.authKey
          }, function(err, conn) {
            if (err != null) {
              return reject(err);
            } else {
              return resolve(conn);
            }
          });
        };
        return _this.close(opts, closeCb);
      };
    })(this)).nodeify(cb);
  });

  Connection.prototype.use = ar(function(db) {
    return this.db = db;
  });

  Connection.prototype.isOpen = function() {
    return this.open && !this.closing;
  };

  Connection.prototype._start = function(term, cb, opts) {
    var key, query, token, value;
    if (!this.open) {
      throw new err.RqlDriverError("Connection is closed.");
    }
    token = this.nextToken++;
    query = {};
    query.global_optargs = {};
    query.type = protoQueryType.START;
    query.query = term.build();
    query.token = token;
    for (key in opts) {
      if (!__hasProp.call(opts, key)) continue;
      value = opts[key];
      query.global_optargs[util.fromCamelCase(key)] = r.expr(value).build();
    }
    if (this.db != null) {
      query.global_optargs['db'] = r.db(this.db).build();
    }
    if (opts.useOutdated != null) {
      query.global_optargs['use_outdated'] = r.expr(!!opts.useOutdated).build();
    }
    if (opts.noreply != null) {
      query.global_optargs['noreply'] = r.expr(!!opts.noreply).build();
    }
    if (opts.profile != null) {
      query.global_optargs['profile'] = r.expr(!!opts.profile).build();
    }
    if ((opts.noreply == null) || !opts.noreply) {
      this.outstandingCallbacks[token] = {
        cb: cb,
        root: term,
        opts: opts
      };
    }
    this._sendQuery(query);
    if ((opts.noreply != null) && opts.noreply && typeof cb === 'function') {
      return cb(null);
    }
  };

  Connection.prototype._continueQuery = function(token) {
    var query;
    if (!this.open) {
      throw new err.RqlDriverError("Connection is closed.");
    }
    query = {
      type: protoQueryType.CONTINUE,
      token: token
    };
    return this._sendQuery(query);
  };

  Connection.prototype._endQuery = function(token) {
    var query;
    if (!this.open) {
      throw new err.RqlDriverError("Connection is closed.");
    }
    query = {
      type: protoQueryType.STOP,
      token: token
    };
    return this._sendQuery(query);
  };

  Connection.prototype._sendQuery = function(query) {
    var data;
    data = [query.type];
    if (!(query.query === void 0)) {
      data.push(query.query);
      if ((query.global_optargs != null) && Object.keys(query.global_optargs).length > 0) {
        data.push(query.global_optargs);
      }
    }
    return this._writeQuery(query.token, JSON.stringify(data));
  };

  return Connection;

})(events.EventEmitter);

TcpConnection = (function(_super) {
  __extends(TcpConnection, _super);

  TcpConnection.isAvailable = function() {
    return !process.browser;
  };

  function TcpConnection(host, callback) {
    var timeout;
    if (!TcpConnection.isAvailable()) {
      throw new err.RqlDriverError("TCP sockets are not available in this environment");
    }
    TcpConnection.__super__.constructor.call(this, host, callback);
    this.rawSocket = net.connect(this.port, this.host);
    this.rawSocket.setNoDelay();
    timeout = setTimeout(((function(_this) {
      return function() {
        _this.rawSocket.destroy();
        return _this.emit('error', new err.RqlDriverError("Handshake timedout"));
      };
    })(this)), this.timeout * 1000);
    this.rawSocket.once('error', (function(_this) {
      return function() {
        return clearTimeout(timeout);
      };
    })(this));
    this.rawSocket.once('connect', (function(_this) {
      return function() {
        var auth_buffer, auth_length, handshake_callback, protocol, version;
        version = new Buffer(4);
        version.writeUInt32LE(protoVersion, 0);
        auth_buffer = new Buffer(_this.authKey, 'ascii');
        auth_length = new Buffer(4);
        auth_length.writeUInt32LE(auth_buffer.length, 0);
        protocol = new Buffer(4);
        protocol.writeUInt32LE(protoProtocol, 0);
        _this.rawSocket.write(Buffer.concat([version, auth_length, auth_buffer, protocol]));
        handshake_callback = function(buf) {
          var b, i, status_buf, status_str, _i, _len, _ref;
          _this.buffer = Buffer.concat([_this.buffer, buf]);
          _ref = _this.buffer;
          for (i = _i = 0, _len = _ref.length; _i < _len; i = ++_i) {
            b = _ref[i];
            if (b === 0) {
              _this.rawSocket.removeListener('data', handshake_callback);
              status_buf = _this.buffer.slice(0, i);
              _this.buffer = _this.buffer.slice(i + 1);
              status_str = status_buf.toString();
              clearTimeout(timeout);
              if (status_str === "SUCCESS") {
                _this.rawSocket.on('data', function(buf) {
                  return _this._data(buf);
                });
                _this.emit('connect');
                return;
              } else {
                _this.emit('error', new err.RqlDriverError("Server dropped connection with message: \"" + status_str.trim() + "\""));
                return;
              }
            }
          }
        };
        return _this.rawSocket.on('data', handshake_callback);
      };
    })(this));
    this.rawSocket.on('error', (function(_this) {
      return function(err) {
        return _this.emit('error', err);
      };
    })(this));
    this.rawSocket.on('close', (function(_this) {
      return function() {
        if (_this.isOpen()) {
          _this.close({
            noreplyWait: false
          });
        }
        return _this.emit('close');
      };
    })(this));
    this.rawSocket.on('timeout', (function(_this) {
      return function() {
        _this.open = false;
        return _this.emit('timeout');
      };
    })(this));
  }

  TcpConnection.prototype.close = varar(0, 2, function(optsOrCallback, callback) {
    var cb, opts;
    if (callback != null) {
      opts = optsOrCallback;
      cb = callback;
    } else if (Object.prototype.toString.call(optsOrCallback) === '[object Object]') {
      opts = optsOrCallback;
      cb = null;
    } else if (typeof optsOrCallback === "function") {
      opts = {};
      cb = optsOrCallback;
    } else {
      opts = {};
    }
    return new Promise((function(_this) {
      return function(resolve, reject) {
        var wrappedCb;
        wrappedCb = function(error, result) {
          var closeCb;
          closeCb = function() {
            if (error != null) {
              return reject(error);
            } else {
              return resolve(result);
            }
          };
          if (_this.rawSocket != null) {
            _this.rawSocket.once("close", function() {
              var _ref;
              closeCb();
              if ((_ref = _this.rawSocket) != null) {
                _ref.removeAllListeners();
              }
              return _this.rawSocket = null;
            });
            return _this.rawSocket.end();
          } else {
            return process.nextTick(closeCb);
          }
        };
        return TcpConnection.__super__.close.call(_this, opts, wrappedCb);
      };
    })(this)).nodeify(cb);
  });

  TcpConnection.prototype.cancel = function() {
    this.rawSocket.destroy();
    return TcpConnection.__super__.cancel.call(this);
  };

  TcpConnection.prototype._writeQuery = function(token, data) {
    var tokenBuf;
    tokenBuf = new Buffer(8);
    tokenBuf.writeUInt32LE(token & 0xFFFFFFFF, 0);
    tokenBuf.writeUInt32LE(Math.floor(token / 0xFFFFFFFF), 4);
    this.rawSocket.write(tokenBuf);
    return this.write(new Buffer(data));
  };

  TcpConnection.prototype.write = function(chunk) {
    var lengthBuffer;
    lengthBuffer = new Buffer(4);
    lengthBuffer.writeUInt32LE(chunk.length, 0);
    this.rawSocket.write(lengthBuffer);
    return this.rawSocket.write(chunk);
  };

  return TcpConnection;

})(Connection);

HttpConnection = (function(_super) {
  __extends(HttpConnection, _super);

  HttpConnection.prototype.DEFAULT_PROTOCOL = 'http';

  HttpConnection.isAvailable = function() {
    return typeof XMLHttpRequest !== "undefined";
  };

  function HttpConnection(host, callback) {
    var protocol, url, xhr;
    if (!HttpConnection.isAvailable()) {
      throw new err.RqlDriverError("XMLHttpRequest is not available in this environment");
    }
    HttpConnection.__super__.constructor.call(this, host, callback);
    protocol = host.protocol === 'https' ? 'https' : this.DEFAULT_PROTOCOL;
    url = "" + protocol + "://" + this.host + ":" + this.port + host.pathname + "ajax/reql/";
    xhr = new XMLHttpRequest;
    xhr.open("POST", url + ("open-new-connection?cacheBuster=" + (Math.random())), true);
    xhr.responseType = "text";
    xhr.onreadystatechange = (function(_this) {
      return function(e) {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            _this._url = url;
            _this._connId = xhr.response;
            return _this.emit('connect');
          } else {
            return _this.emit('error', new err.RqlDriverError("XHR error, http status " + xhr.status + "."));
          }
        }
      };
    })(this);
    xhr.send();
    this.xhr = xhr;
  }

  HttpConnection.prototype.cancel = function() {
    var xhr;
    if (this._connId != null) {
      this.xhr.abort();
      xhr = new XMLHttpRequest;
      xhr.open("POST", "" + this._url + "close-connection?conn_id=" + this._connId, true);
      xhr.responseType = "arraybuffer";
      xhr.send();
      this._url = null;
      this._connId = null;
      return HttpConnection.__super__.cancel.call(this);
    }
  };

  HttpConnection.prototype.close = varar(0, 2, function(optsOrCallback, callback) {
    var cb, opts;
    if (callback != null) {
      opts = optsOrCallback;
      cb = callback;
    } else if (Object.prototype.toString.call(optsOrCallback) === '[object Object]') {
      opts = optsOrCallback;
      cb = null;
    } else {
      opts = {};
      cb = optsOrCallback;
    }
    if (!((cb == null) || typeof cb === 'function')) {
      throw new err.RqlDriverError("Final argument to `close` must be a callback function or object.");
    }
    return HttpConnection.__super__.close.call(this, opts, cb);
  });

  HttpConnection.prototype._writeQuery = function(token, data) {
    var buf;
    buf = new Buffer(encodeURI(data).split(/%..|./).length - 1 + 8);
    buf.writeUInt32LE(token & 0xFFFFFFFF, 0);
    buf.writeUInt32LE(Math.floor(token / 0xFFFFFFFF), 4);
    buf.write(data, 8);
    return this.write(buf, token);
  };

  HttpConnection.prototype.write = function(chunk, token) {
    var i, view, xhr;
    xhr = new XMLHttpRequest;
    xhr.open("POST", "" + this._url + "?conn_id=" + this._connId, true);
    xhr.responseType = "arraybuffer";
    xhr.onreadystatechange = (function(_this) {
      return function(e) {
        var b, buf;
        if (xhr.readyState === 4 && xhr.status === 200) {
          buf = new Buffer((function() {
            var _i, _len, _ref, _results;
            _ref = new Uint8Array(xhr.response);
            _results = [];
            for (_i = 0, _len = _ref.length; _i < _len; _i++) {
              b = _ref[_i];
              _results.push(b);
            }
            return _results;
          })());
          return _this._data(buf);
        }
      };
    })(this);
    xhr.onerror = (function(_this) {
      return function(e) {
        return _this.outstandingCallbacks[token].cb(new Error("This HTTP connection is not open"));
      };
    })(this);
    view = new Uint8Array(chunk.length);
    i = 0;
    while (i < chunk.length) {
      view[i] = chunk[i];
      i++;
    }
    xhr.send(view);
    return this.xhr = xhr;
  };

  return HttpConnection;

})(Connection);

module.exports.isConnection = function(connection) {
  return connection instanceof Connection;
};

module.exports.connect = varar(0, 2, function(hostOrCallback, callback) {
  var host;
  if (typeof hostOrCallback === 'function') {
    host = {};
    callback = hostOrCallback;
  } else {
    host = hostOrCallback;
  }
  return new Promise(function(resolve, reject) {
    var create_connection, wrappedCb;
    create_connection = (function(_this) {
      return function(host, callback) {
        if (TcpConnection.isAvailable()) {
          return new TcpConnection(host, callback);
        } else if (HttpConnection.isAvailable()) {
          return new HttpConnection(host, callback);
        } else {
          throw new err.RqlDriverError("Neither TCP nor HTTP avaiable in this environment");
        }
      };
    })(this);
    wrappedCb = function(err, result) {
      if (err) {
        return reject(err);
      } else {
        return resolve(result);
      }
    };
    return create_connection(host, wrappedCb);
  }).nodeify(callback);
});
}).call(this,require("/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js"),require("buffer").Buffer)
},{"./ast":7,"./cursor":8,"./errors":9,"./proto-def":46,"./util":49,"/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js":3,"bluebird":13,"buffer":4,"events":2,"net":1}],11:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise) {
var SomePromiseArray = Promise._SomePromiseArray;
function Promise$_Any(promises) {
    var ret = new SomePromiseArray(promises);
    var promise = ret.promise();
    if (promise.isRejected()) {
        return promise;
    }
    ret.setHowMany(1);
    ret.setUnwrap();
    ret.init();
    return promise;
}

Promise.any = function Promise$Any(promises) {
    return Promise$_Any(promises);
};

Promise.prototype.any = function Promise$any() {
    return Promise$_Any(this);
};

};

},{}],12:[function(require,module,exports){
(function (process){/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var schedule = require("./schedule.js");
var Queue = require("./queue.js");
var errorObj = require("./util.js").errorObj;
var tryCatch1 = require("./util.js").tryCatch1;
var _process = typeof process !== "undefined" ? process : void 0;

function Async() {
    this._isTickUsed = false;
    this._schedule = schedule;
    this._length = 0;
    this._lateBuffer = new Queue(16);
    this._functionBuffer = new Queue(65536);
    var self = this;
    this.consumeFunctionBuffer = function Async$consumeFunctionBuffer() {
        self._consumeFunctionBuffer();
    };
}

Async.prototype.haveItemsQueued = function Async$haveItemsQueued() {
    return this._length > 0;
};

Async.prototype.invokeLater = function Async$invokeLater(fn, receiver, arg) {
    if (_process !== void 0 &&
        _process.domain != null &&
        !fn.domain) {
        fn = _process.domain.bind(fn);
    }
    this._lateBuffer.push(fn, receiver, arg);
    this._queueTick();
};

Async.prototype.invoke = function Async$invoke(fn, receiver, arg) {
    if (_process !== void 0 &&
        _process.domain != null &&
        !fn.domain) {
        fn = _process.domain.bind(fn);
    }
    var functionBuffer = this._functionBuffer;
    functionBuffer.push(fn, receiver, arg);
    this._length = functionBuffer.length();
    this._queueTick();
};

Async.prototype._consumeFunctionBuffer =
function Async$_consumeFunctionBuffer() {
    var functionBuffer = this._functionBuffer;
    while (functionBuffer.length() > 0) {
        var fn = functionBuffer.shift();
        var receiver = functionBuffer.shift();
        var arg = functionBuffer.shift();
        fn.call(receiver, arg);
    }
    this._reset();
    this._consumeLateBuffer();
};

Async.prototype._consumeLateBuffer = function Async$_consumeLateBuffer() {
    var buffer = this._lateBuffer;
    while(buffer.length() > 0) {
        var fn = buffer.shift();
        var receiver = buffer.shift();
        var arg = buffer.shift();
        var res = tryCatch1(fn, receiver, arg);
        if (res === errorObj) {
            this._queueTick();
            if (fn.domain != null) {
                fn.domain.emit("error", res.e);
            } else {
                throw res.e;
            }
        }
    }
};

Async.prototype._queueTick = function Async$_queue() {
    if (!this._isTickUsed) {
        this._schedule(this.consumeFunctionBuffer);
        this._isTickUsed = true;
    }
};

Async.prototype._reset = function Async$_reset() {
    this._isTickUsed = false;
    this._length = 0;
};

module.exports = new Async();
}).call(this,require("/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js"))
},{"./queue.js":35,"./schedule.js":38,"./util.js":45,"/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js":3}],13:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var Promise = require("./promise.js")();
module.exports = Promise;
},{"./promise.js":30}],14:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var cr = Object.create;
if (cr) {
    var callerCache = cr(null);
    var getterCache = cr(null);
    callerCache[" size"] = getterCache[" size"] = 0;
}

module.exports = function(Promise) {
var util = require("./util.js");
var canEvaluate = util.canEvaluate;
var isIdentifier = util.isIdentifier;

function makeMethodCaller (methodName) {
    return new Function("obj", "                                             \n\
        'use strict'                                                         \n\
        var len = this.length;                                               \n\
        switch(len) {                                                        \n\
            case 1: return obj.methodName(this[0]);                          \n\
            case 2: return obj.methodName(this[0], this[1]);                 \n\
            case 3: return obj.methodName(this[0], this[1], this[2]);        \n\
            case 0: return obj.methodName();                                 \n\
            default: return obj.methodName.apply(obj, this);                 \n\
        }                                                                    \n\
        ".replace(/methodName/g, methodName));
}

function makeGetter (propertyName) {
    return new Function("obj", "                                             \n\
        'use strict';                                                        \n\
        return obj.propertyName;                                             \n\
        ".replace("propertyName", propertyName));
}

function getCompiled(name, compiler, cache) {
    var ret = cache[name];
    if (typeof ret !== "function") {
        if (!isIdentifier(name)) {
            return null;
        }
        ret = compiler(name);
        cache[name] = ret;
        cache[" size"]++;
        if (cache[" size"] > 512) {
            var keys = Object.keys(cache);
            for (var i = 0; i < 256; ++i) delete cache[keys[i]];
            cache[" size"] = keys.length - 256;
        }
    }
    return ret;
}

function getMethodCaller(name) {
    return getCompiled(name, makeMethodCaller, callerCache);
}

function getGetter(name) {
    return getCompiled(name, makeGetter, getterCache);
}

function caller(obj) {
    return obj[this.pop()].apply(obj, this);
}
Promise.prototype.call = function Promise$call(methodName) {
    var $_len = arguments.length;var args = new Array($_len - 1); for(var $_i = 1; $_i < $_len; ++$_i) {args[$_i - 1] = arguments[$_i];}
    if (canEvaluate) {
        var maybeCaller = getMethodCaller(methodName);
        if (maybeCaller !== null) {
            return this._then(maybeCaller, void 0, void 0, args, void 0);
        }
    }
    args.push(methodName);
    return this._then(caller, void 0, void 0, args, void 0);
};

function namedGetter(obj) {
    return obj[this];
}
function indexedGetter(obj) {
    return obj[this];
}
Promise.prototype.get = function Promise$get(propertyName) {
    var isIndex = (typeof propertyName === "number");
    var getter;
    if (!isIndex) {
        if (canEvaluate) {
            var maybeGetter = getGetter(propertyName);
            getter = maybeGetter !== null ? maybeGetter : namedGetter;
        } else {
            getter = namedGetter;
        }
    } else {
        getter = indexedGetter;
    }
    return this._then(getter, void 0, void 0, propertyName, void 0);
};
};

},{"./util.js":45}],15:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL) {
var errors = require("./errors.js");
var canAttach = errors.canAttach;
var async = require("./async.js");
var CancellationError = errors.CancellationError;

Promise.prototype._cancel = function Promise$_cancel(reason) {
    if (!this.isCancellable()) return this;
    var parent;
    var promiseToReject = this;
    while ((parent = promiseToReject._cancellationParent) !== void 0 &&
        parent.isCancellable()) {
        promiseToReject = parent;
    }
    promiseToReject._attachExtraTrace(reason);
    promiseToReject._rejectUnchecked(reason);
};

Promise.prototype.cancel = function Promise$cancel(reason) {
    if (!this.isCancellable()) return this;
    reason = reason !== void 0
        ? (canAttach(reason) ? reason : new Error(reason + ""))
        : new CancellationError();
    async.invokeLater(this._cancel, this, reason);
    return this;
};

Promise.prototype.cancellable = function Promise$cancellable() {
    if (this._cancellable()) return this;
    this._setCancellable();
    this._cancellationParent = void 0;
    return this;
};

Promise.prototype.uncancellable = function Promise$uncancellable() {
    var ret = new Promise(INTERNAL);
    ret._propagateFrom(this, 2 | 4);
    ret._follow(this);
    ret._unsetCancellable();
    return ret;
};

Promise.prototype.fork =
function Promise$fork(didFulfill, didReject, didProgress) {
    var ret = this._then(didFulfill, didReject, didProgress,
                         void 0, void 0);

    ret._setCancellable();
    ret._cancellationParent = void 0;
    return ret;
};
};

},{"./async.js":12,"./errors.js":20}],16:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function() {
var inherits = require("./util.js").inherits;
var defineProperty = require("./es5.js").defineProperty;

var rignore = new RegExp(
    "\\b(?:[a-zA-Z0-9.]+\\$_\\w+|" +
    "tryCatch(?:1|2|3|4|Apply)|new \\w*PromiseArray|" +
    "\\w*PromiseArray\\.\\w*PromiseArray|" +
    "setTimeout|CatchFilter\\$_\\w+|makeNodePromisified|processImmediate|" +
    "process._tickCallback|nextTick|Async\\$\\w+)\\b"
);

var rtraceline = null;
var formatStack = null;

function formatNonError(obj) {
    var str;
    if (typeof obj === "function") {
        str = "[function " +
            (obj.name || "anonymous") +
            "]";
    } else {
        str = obj.toString();
        var ruselessToString = /\[object [a-zA-Z0-9$_]+\]/;
        if (ruselessToString.test(str)) {
            try {
                var newStr = JSON.stringify(obj);
                str = newStr;
            }
            catch(e) {

            }
        }
        if (str.length === 0) {
            str = "(empty array)";
        }
    }
    return ("(<" + snip(str) + ">, no stack trace)");
}

function snip(str) {
    var maxChars = 41;
    if (str.length < maxChars) {
        return str;
    }
    return str.substr(0, maxChars - 3) + "...";
}

function CapturedTrace(ignoreUntil, isTopLevel) {
    this.captureStackTrace(CapturedTrace, isTopLevel);

}
inherits(CapturedTrace, Error);

CapturedTrace.prototype.captureStackTrace =
function CapturedTrace$captureStackTrace(ignoreUntil, isTopLevel) {
    captureStackTrace(this, ignoreUntil, isTopLevel);
};

CapturedTrace.possiblyUnhandledRejection =
function CapturedTrace$PossiblyUnhandledRejection(reason) {
    if (typeof console === "object") {
        var message;
        if (typeof reason === "object" || typeof reason === "function") {
            var stack = reason.stack;
            message = "Possibly unhandled " + formatStack(stack, reason);
        } else {
            message = "Possibly unhandled " + String(reason);
        }
        if (typeof console.error === "function" ||
            typeof console.error === "object") {
            console.error(message);
        } else if (typeof console.log === "function" ||
            typeof console.log === "object") {
            console.log(message);
        }
    }
};

CapturedTrace.combine = function CapturedTrace$Combine(current, prev) {
    var curLast = current.length - 1;
    for (var i = prev.length - 1; i >= 0; --i) {
        var line = prev[i];
        if (current[curLast] === line) {
            current.pop();
            curLast--;
        } else {
            break;
        }
    }

    current.push("From previous event:");
    var lines = current.concat(prev);

    var ret = [];

    for (var i = 0, len = lines.length; i < len; ++i) {

        if (((rignore.test(lines[i]) && rtraceline.test(lines[i])) ||
            (i > 0 && !rtraceline.test(lines[i])) &&
            lines[i] !== "From previous event:")
       ) {
            continue;
        }
        ret.push(lines[i]);
    }
    return ret;
};

CapturedTrace.protectErrorMessageNewlines = function(stack) {
    for (var i = 0; i < stack.length; ++i) {
        if (rtraceline.test(stack[i])) {
            break;
        }
    }

    if (i <= 1) return;

    var errorMessageLines = [];
    for (var j = 0; j < i; ++j) {
        errorMessageLines.push(stack.shift());
    }
    stack.unshift(errorMessageLines.join("\u0002\u0000\u0001"));
};

CapturedTrace.isSupported = function CapturedTrace$IsSupported() {
    return typeof captureStackTrace === "function";
};

var captureStackTrace = (function stackDetection() {
    if (typeof Error.stackTraceLimit === "number" &&
        typeof Error.captureStackTrace === "function") {
        rtraceline = /^\s*at\s*/;
        formatStack = function(stack, error) {
            if (typeof stack === "string") return stack;

            if (error.name !== void 0 &&
                error.message !== void 0) {
                return error.name + ". " + error.message;
            }
            return formatNonError(error);


        };
        var captureStackTrace = Error.captureStackTrace;
        return function CapturedTrace$_captureStackTrace(
            receiver, ignoreUntil) {
            captureStackTrace(receiver, ignoreUntil);
        };
    }
    var err = new Error();

    if (typeof err.stack === "string" &&
        typeof "".startsWith === "function" &&
        (err.stack.startsWith("stackDetection@")) &&
        stackDetection.name === "stackDetection") {

        defineProperty(Error, "stackTraceLimit", {
            writable: true,
            enumerable: false,
            configurable: false,
            value: 25
        });
        rtraceline = /@/;
        var rline = /[@\n]/;

        formatStack = function(stack, error) {
            if (typeof stack === "string") {
                return (error.name + ". " + error.message + "\n" + stack);
            }

            if (error.name !== void 0 &&
                error.message !== void 0) {
                return error.name + ". " + error.message;
            }
            return formatNonError(error);
        };

        return function captureStackTrace(o) {
            var stack = new Error().stack;
            var split = stack.split(rline);
            var len = split.length;
            var ret = "";
            for (var i = 0; i < len; i += 2) {
                ret += split[i];
                ret += "@";
                ret += split[i + 1];
                ret += "\n";
            }
            o.stack = ret;
        };
    } else {
        formatStack = function(stack, error) {
            if (typeof stack === "string") return stack;

            if ((typeof error === "object" ||
                typeof error === "function") &&
                error.name !== void 0 &&
                error.message !== void 0) {
                return error.name + ". " + error.message;
            }
            return formatNonError(error);
        };

        return null;
    }
})();

return CapturedTrace;
};

},{"./es5.js":22,"./util.js":45}],17:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(NEXT_FILTER) {
var util = require("./util.js");
var errors = require("./errors.js");
var tryCatch1 = util.tryCatch1;
var errorObj = util.errorObj;
var keys = require("./es5.js").keys;
var TypeError = errors.TypeError;

function CatchFilter(instances, callback, promise) {
    this._instances = instances;
    this._callback = callback;
    this._promise = promise;
}

function CatchFilter$_safePredicate(predicate, e) {
    var safeObject = {};
    var retfilter = tryCatch1(predicate, safeObject, e);

    if (retfilter === errorObj) return retfilter;

    var safeKeys = keys(safeObject);
    if (safeKeys.length) {
        errorObj.e = new TypeError(
            "Catch filter must inherit from Error "
          + "or be a simple predicate function");
        return errorObj;
    }
    return retfilter;
}

CatchFilter.prototype.doFilter = function CatchFilter$_doFilter(e) {
    var cb = this._callback;
    var promise = this._promise;
    var boundTo = promise._boundTo;
    for (var i = 0, len = this._instances.length; i < len; ++i) {
        var item = this._instances[i];
        var itemIsErrorType = item === Error ||
            (item != null && item.prototype instanceof Error);

        if (itemIsErrorType && e instanceof item) {
            var ret = tryCatch1(cb, boundTo, e);
            if (ret === errorObj) {
                NEXT_FILTER.e = ret.e;
                return NEXT_FILTER;
            }
            return ret;
        } else if (typeof item === "function" && !itemIsErrorType) {
            var shouldHandle = CatchFilter$_safePredicate(item, e);
            if (shouldHandle === errorObj) {
                var trace = errors.canAttach(errorObj.e)
                    ? errorObj.e
                    : new Error(errorObj.e + "");
                this._promise._attachExtraTrace(trace);
                e = errorObj.e;
                break;
            } else if (shouldHandle) {
                var ret = tryCatch1(cb, boundTo, e);
                if (ret === errorObj) {
                    NEXT_FILTER.e = ret.e;
                    return NEXT_FILTER;
                }
                return ret;
            }
        }
    }
    NEXT_FILTER.e = e;
    return NEXT_FILTER;
};

return CatchFilter;
};

},{"./errors.js":20,"./es5.js":22,"./util.js":45}],18:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var util = require("./util.js");
var isPrimitive = util.isPrimitive;
var wrapsPrimitiveReceiver = util.wrapsPrimitiveReceiver;

module.exports = function(Promise) {
var returner = function Promise$_returner() {
    return this;
};
var thrower = function Promise$_thrower() {
    throw this;
};

var wrapper = function Promise$_wrapper(value, action) {
    if (action === 1) {
        return function Promise$_thrower() {
            throw value;
        };
    } else if (action === 2) {
        return function Promise$_returner() {
            return value;
        };
    }
};


Promise.prototype["return"] =
Promise.prototype.thenReturn =
function Promise$thenReturn(value) {
    if (wrapsPrimitiveReceiver && isPrimitive(value)) {
        return this._then(
            wrapper(value, 2),
            void 0,
            void 0,
            void 0,
            void 0
       );
    }
    return this._then(returner, void 0, void 0, value, void 0);
};

Promise.prototype["throw"] =
Promise.prototype.thenThrow =
function Promise$thenThrow(reason) {
    if (wrapsPrimitiveReceiver && isPrimitive(reason)) {
        return this._then(
            wrapper(reason, 1),
            void 0,
            void 0,
            void 0,
            void 0
       );
    }
    return this._then(thrower, void 0, void 0, reason, void 0);
};
};

},{"./util.js":45}],19:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL) {
var PromiseReduce = Promise.reduce;

Promise.prototype.each = function Promise$each(fn) {
    return PromiseReduce(this, fn, null, INTERNAL);
};

Promise.each = function Promise$Each(promises, fn) {
    return PromiseReduce(promises, fn, null, INTERNAL);
};
};

},{}],20:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var Objectfreeze = require("./es5.js").freeze;
var util = require("./util.js");
var inherits = util.inherits;
var notEnumerableProp = util.notEnumerableProp;

function markAsOriginatingFromRejection(e) {
    try {
        notEnumerableProp(e, "isOperational", true);
    }
    catch(ignore) {}
}

function originatesFromRejection(e) {
    if (e == null) return false;
    return ((e instanceof OperationalError) ||
        e["isOperational"] === true);
}

function isError(obj) {
    return obj instanceof Error;
}

function canAttach(obj) {
    return isError(obj);
}

function subError(nameProperty, defaultMessage) {
    function SubError(message) {
        if (!(this instanceof SubError)) return new SubError(message);
        this.message = typeof message === "string" ? message : defaultMessage;
        this.name = nameProperty;
        if (Error.captureStackTrace) {
            Error.captureStackTrace(this, this.constructor);
        }
    }
    inherits(SubError, Error);
    return SubError;
}

var _TypeError, _RangeError;
var CancellationError = subError("CancellationError", "cancellation error");
var TimeoutError = subError("TimeoutError", "timeout error");
var AggregateError = subError("AggregateError", "aggregate error");
try {
    _TypeError = TypeError;
    _RangeError = RangeError;
} catch(e) {
    _TypeError = subError("TypeError", "type error");
    _RangeError = subError("RangeError", "range error");
}

var methods = ("join pop push shift unshift slice filter forEach some " +
    "every map indexOf lastIndexOf reduce reduceRight sort reverse").split(" ");

for (var i = 0; i < methods.length; ++i) {
    if (typeof Array.prototype[methods[i]] === "function") {
        AggregateError.prototype[methods[i]] = Array.prototype[methods[i]];
    }
}

AggregateError.prototype.length = 0;
AggregateError.prototype["isOperational"] = true;
var level = 0;
AggregateError.prototype.toString = function() {
    var indent = Array(level * 4 + 1).join(" ");
    var ret = "\n" + indent + "AggregateError of:" + "\n";
    level++;
    indent = Array(level * 4 + 1).join(" ");
    for (var i = 0; i < this.length; ++i) {
        var str = this[i] === this ? "[Circular AggregateError]" : this[i] + "";
        var lines = str.split("\n");
        for (var j = 0; j < lines.length; ++j) {
            lines[j] = indent + lines[j];
        }
        str = lines.join("\n");
        ret += str + "\n";
    }
    level--;
    return ret;
};

function OperationalError(message) {
    this.name = "OperationalError";
    this.message = message;
    this.cause = message;
    this["isOperational"] = true;

    if (message instanceof Error) {
        this.message = message.message;
        this.stack = message.stack;
    } else if (Error.captureStackTrace) {
        Error.captureStackTrace(this, this.constructor);
    }

}
inherits(OperationalError, Error);

var key = "__BluebirdErrorTypes__";
var errorTypes = Error[key];
if (!errorTypes) {
    errorTypes = Objectfreeze({
        CancellationError: CancellationError,
        TimeoutError: TimeoutError,
        OperationalError: OperationalError,
        RejectionError: OperationalError,
        AggregateError: AggregateError
    });
    notEnumerableProp(Error, key, errorTypes);
}

module.exports = {
    Error: Error,
    TypeError: _TypeError,
    RangeError: _RangeError,
    CancellationError: errorTypes.CancellationError,
    OperationalError: errorTypes.OperationalError,
    TimeoutError: errorTypes.TimeoutError,
    AggregateError: errorTypes.AggregateError,
    originatesFromRejection: originatesFromRejection,
    markAsOriginatingFromRejection: markAsOriginatingFromRejection,
    canAttach: canAttach
};

},{"./es5.js":22,"./util.js":45}],21:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise) {
var TypeError = require('./errors.js').TypeError;

function apiRejection(msg) {
    var error = new TypeError(msg);
    var ret = Promise.rejected(error);
    var parent = ret._peekContext();
    if (parent != null) {
        parent._attachExtraTrace(error);
    }
    return ret;
}

return apiRejection;
};

},{"./errors.js":20}],22:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
var isES5 = (function(){
    "use strict";
    return this === void 0;
})();

if (isES5) {
    module.exports = {
        freeze: Object.freeze,
        defineProperty: Object.defineProperty,
        keys: Object.keys,
        getPrototypeOf: Object.getPrototypeOf,
        isArray: Array.isArray,
        isES5: isES5
    };
} else {
    var has = {}.hasOwnProperty;
    var str = {}.toString;
    var proto = {}.constructor.prototype;

    var ObjectKeys = function ObjectKeys(o) {
        var ret = [];
        for (var key in o) {
            if (has.call(o, key)) {
                ret.push(key);
            }
        }
        return ret;
    }

    var ObjectDefineProperty = function ObjectDefineProperty(o, key, desc) {
        o[key] = desc.value;
        return o;
    }

    var ObjectFreeze = function ObjectFreeze(obj) {
        return obj;
    }

    var ObjectGetPrototypeOf = function ObjectGetPrototypeOf(obj) {
        try {
            return Object(obj).constructor.prototype;
        }
        catch (e) {
            return proto;
        }
    }

    var ArrayIsArray = function ArrayIsArray(obj) {
        try {
            return str.call(obj) === "[object Array]";
        }
        catch(e) {
            return false;
        }
    }

    module.exports = {
        isArray: ArrayIsArray,
        keys: ObjectKeys,
        defineProperty: ObjectDefineProperty,
        freeze: ObjectFreeze,
        getPrototypeOf: ObjectGetPrototypeOf,
        isES5: isES5
    };
}

},{}],23:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL) {
var PromiseMap = Promise.map;

Promise.prototype.filter = function Promise$filter(fn, options) {
    return PromiseMap(this, fn, options, INTERNAL);
};

Promise.filter = function Promise$Filter(promises, fn, options) {
    return PromiseMap(promises, fn, options, INTERNAL);
};
};

},{}],24:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, NEXT_FILTER, cast) {
var util = require("./util.js");
var wrapsPrimitiveReceiver = util.wrapsPrimitiveReceiver;
var isPrimitive = util.isPrimitive;
var thrower = util.thrower;

function returnThis() {
    return this;
}
function throwThis() {
    throw this;
}
function return$(r) {
    return function Promise$_returner() {
        return r;
    };
}
function throw$(r) {
    return function Promise$_thrower() {
        throw r;
    };
}
function promisedFinally(ret, reasonOrValue, isFulfilled) {
    var then;
    if (wrapsPrimitiveReceiver && isPrimitive(reasonOrValue)) {
        then = isFulfilled ? return$(reasonOrValue) : throw$(reasonOrValue);
    } else {
        then = isFulfilled ? returnThis : throwThis;
    }
    return ret._then(then, thrower, void 0, reasonOrValue, void 0);
}

function finallyHandler(reasonOrValue) {
    var promise = this.promise;
    var handler = this.handler;

    var ret = promise._isBound()
                    ? handler.call(promise._boundTo)
                    : handler();

    if (ret !== void 0) {
        var maybePromise = cast(ret, void 0);
        if (maybePromise instanceof Promise) {
            return promisedFinally(maybePromise, reasonOrValue,
                                    promise.isFulfilled());
        }
    }

    if (promise.isRejected()) {
        NEXT_FILTER.e = reasonOrValue;
        return NEXT_FILTER;
    } else {
        return reasonOrValue;
    }
}

function tapHandler(value) {
    var promise = this.promise;
    var handler = this.handler;

    var ret = promise._isBound()
                    ? handler.call(promise._boundTo, value)
                    : handler(value);

    if (ret !== void 0) {
        var maybePromise = cast(ret, void 0);
        if (maybePromise instanceof Promise) {
            return promisedFinally(maybePromise, value, true);
        }
    }
    return value;
}

Promise.prototype._passThroughHandler =
function Promise$_passThroughHandler(handler, isFinally) {
    if (typeof handler !== "function") return this.then();

    var promiseAndHandler = {
        promise: this,
        handler: handler
    };

    return this._then(
            isFinally ? finallyHandler : tapHandler,
            isFinally ? finallyHandler : void 0, void 0,
            promiseAndHandler, void 0);
};

Promise.prototype.lastly =
Promise.prototype["finally"] = function Promise$finally(handler) {
    return this._passThroughHandler(handler, true);
};

Promise.prototype.tap = function Promise$tap(handler) {
    return this._passThroughHandler(handler, false);
};
};

},{"./util.js":45}],25:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, apiRejection, INTERNAL, cast) {
var errors = require("./errors.js");
var TypeError = errors.TypeError;
var deprecated = require("./util.js").deprecated;
var util = require("./util.js");
var errorObj = util.errorObj;
var tryCatch1 = util.tryCatch1;
var yieldHandlers = [];

function promiseFromYieldHandler(value, yieldHandlers) {
    var _errorObj = errorObj;
    var _Promise = Promise;
    var len = yieldHandlers.length;
    for (var i = 0; i < len; ++i) {
        var result = tryCatch1(yieldHandlers[i], void 0, value);
        if (result === _errorObj) {
            return _Promise.reject(_errorObj.e);
        }
        var maybePromise = cast(result, promiseFromYieldHandler);
        if (maybePromise instanceof _Promise) return maybePromise;
    }
    return null;
}

function PromiseSpawn(generatorFunction, receiver, yieldHandler) {
    var promise = this._promise = new Promise(INTERNAL);
    promise._setTrace(void 0);
    this._generatorFunction = generatorFunction;
    this._receiver = receiver;
    this._generator = void 0;
    this._yieldHandlers = typeof yieldHandler === "function"
        ? [yieldHandler].concat(yieldHandlers)
        : yieldHandlers;
}

PromiseSpawn.prototype.promise = function PromiseSpawn$promise() {
    return this._promise;
};

PromiseSpawn.prototype._run = function PromiseSpawn$_run() {
    this._generator = this._generatorFunction.call(this._receiver);
    this._receiver =
        this._generatorFunction = void 0;
    this._next(void 0);
};

PromiseSpawn.prototype._continue = function PromiseSpawn$_continue(result) {
    if (result === errorObj) {
        this._generator = void 0;
        var trace = errors.canAttach(result.e)
            ? result.e : new Error(result.e + "");
        this._promise._attachExtraTrace(trace);
        this._promise._reject(result.e, trace);
        return;
    }

    var value = result.value;
    if (result.done === true) {
        this._generator = void 0;
        if (!this._promise._tryFollow(value)) {
            this._promise._fulfill(value);
        }
    } else {
        var maybePromise = cast(value, void 0);
        if (!(maybePromise instanceof Promise)) {
            maybePromise =
                promiseFromYieldHandler(maybePromise, this._yieldHandlers);
            if (maybePromise === null) {
                this._throw(new TypeError("A value was yielded that could not be treated as a promise"));
                return;
            }
        }
        maybePromise._then(
            this._next,
            this._throw,
            void 0,
            this,
            null
       );
    }
};

PromiseSpawn.prototype._throw = function PromiseSpawn$_throw(reason) {
    if (errors.canAttach(reason))
        this._promise._attachExtraTrace(reason);
    this._continue(
        tryCatch1(this._generator["throw"], this._generator, reason)
   );
};

PromiseSpawn.prototype._next = function PromiseSpawn$_next(value) {
    this._continue(
        tryCatch1(this._generator.next, this._generator, value)
   );
};

Promise.coroutine =
function Promise$Coroutine(generatorFunction, options) {
    if (typeof generatorFunction !== "function") {
        throw new TypeError("generatorFunction must be a function");
    }
    var yieldHandler = Object(options).yieldHandler;
    var PromiseSpawn$ = PromiseSpawn;
    return function () {
        var generator = generatorFunction.apply(this, arguments);
        var spawn = new PromiseSpawn$(void 0, void 0, yieldHandler);
        spawn._generator = generator;
        spawn._next(void 0);
        return spawn.promise();
    };
};

Promise.coroutine.addYieldHandler = function(fn) {
    if (typeof fn !== "function") throw new TypeError("fn must be a function");
    yieldHandlers.push(fn);
};

Promise.spawn = function Promise$Spawn(generatorFunction) {
    deprecated("Promise.spawn is deprecated. Use Promise.coroutine instead.");
    if (typeof generatorFunction !== "function") {
        return apiRejection("generatorFunction must be a function");
    }
    var spawn = new PromiseSpawn(generatorFunction, this);
    var ret = spawn.promise();
    spawn._run(Promise.spawn);
    return ret;
};
};

},{"./errors.js":20,"./util.js":45}],26:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports =
function(Promise, PromiseArray, cast, INTERNAL) {
var util = require("./util.js");
var canEvaluate = util.canEvaluate;
var tryCatch1 = util.tryCatch1;
var errorObj = util.errorObj;


if (canEvaluate) {
    var thenCallback = function(i) {
        return new Function("value", "holder", "                             \n\
            'use strict';                                                    \n\
            holder.pIndex = value;                                           \n\
            holder.checkFulfillment(this);                                   \n\
            ".replace(/Index/g, i));
    };

    var caller = function(count) {
        var values = [];
        for (var i = 1; i <= count; ++i) values.push("holder.p" + i);
        return new Function("holder", "                                      \n\
            'use strict';                                                    \n\
            var callback = holder.fn;                                        \n\
            return callback(values);                                         \n\
            ".replace(/values/g, values.join(", ")));
    };
    var thenCallbacks = [];
    var callers = [void 0];
    for (var i = 1; i <= 5; ++i) {
        thenCallbacks.push(thenCallback(i));
        callers.push(caller(i));
    }

    var Holder = function(total, fn) {
        this.p1 = this.p2 = this.p3 = this.p4 = this.p5 = null;
        this.fn = fn;
        this.total = total;
        this.now = 0;
    };

    Holder.prototype.callers = callers;
    Holder.prototype.checkFulfillment = function(promise) {
        var now = this.now;
        now++;
        var total = this.total;
        if (now >= total) {
            var handler = this.callers[total];
            var ret = tryCatch1(handler, void 0, this);
            if (ret === errorObj) {
                promise._rejectUnchecked(ret.e);
            } else if (!promise._tryFollow(ret)) {
                promise._fulfillUnchecked(ret);
            }
        } else {
            this.now = now;
        }
    };
}




Promise.join = function Promise$Join() {
    var last = arguments.length - 1;
    var fn;
    if (last > 0 && typeof arguments[last] === "function") {
        fn = arguments[last];
        if (last < 6 && canEvaluate) {
            var ret = new Promise(INTERNAL);
            ret._setTrace(void 0);
            var holder = new Holder(last, fn);
            var reject = ret._reject;
            var callbacks = thenCallbacks;
            for (var i = 0; i < last; ++i) {
                var maybePromise = cast(arguments[i], void 0);
                if (maybePromise instanceof Promise) {
                    if (maybePromise.isPending()) {
                        maybePromise._then(callbacks[i], reject,
                                           void 0, ret, holder);
                    } else if (maybePromise.isFulfilled()) {
                        callbacks[i].call(ret,
                                          maybePromise._settledValue, holder);
                    } else {
                        ret._reject(maybePromise._settledValue);
                        maybePromise._unsetRejectionIsUnhandled();
                    }
                } else {
                    callbacks[i].call(ret, maybePromise, holder);
                }
            }
            return ret;
        }
    }
    var $_len = arguments.length;var args = new Array($_len); for(var $_i = 0; $_i < $_len; ++$_i) {args[$_i] = arguments[$_i];}
    var ret = new PromiseArray(args).promise();
    return fn !== void 0 ? ret.spread(fn) : ret;
};

};

},{"./util.js":45}],27:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, PromiseArray, apiRejection, cast, INTERNAL) {
var util = require("./util.js");
var tryCatch3 = util.tryCatch3;
var errorObj = util.errorObj;
var PENDING = {};
var EMPTY_ARRAY = [];

function MappingPromiseArray(promises, fn, limit, _filter) {
    this.constructor$(promises);
    this._callback = fn;
    this._preservedValues = _filter === INTERNAL
        ? new Array(this.length())
        : null;
    this._limit = limit;
    this._inFlight = 0;
    this._queue = limit >= 1 ? [] : EMPTY_ARRAY;
    this._init$(void 0, -2);
}
util.inherits(MappingPromiseArray, PromiseArray);

MappingPromiseArray.prototype._init = function MappingPromiseArray$_init() {};

MappingPromiseArray.prototype._promiseFulfilled =
function MappingPromiseArray$_promiseFulfilled(value, index) {
    var values = this._values;
    if (values === null) return;

    var length = this.length();
    var preservedValues = this._preservedValues;
    var limit = this._limit;
    if (values[index] === PENDING) {
        values[index] = value;
        if (limit >= 1) {
            this._inFlight--;
            this._drainQueue();
            if (this._isResolved()) return;
        }
    } else {
        if (limit >= 1 && this._inFlight >= limit) {
            values[index] = value;
            this._queue.push(index);
            return;
        }
        if (preservedValues !== null) preservedValues[index] = value;

        var callback = this._callback;
        var receiver = this._promise._boundTo;
        var ret = tryCatch3(callback, receiver, value, index, length);
        if (ret === errorObj) return this._reject(ret.e);

        var maybePromise = cast(ret, void 0);
        if (maybePromise instanceof Promise) {
            if (maybePromise.isPending()) {
                if (limit >= 1) this._inFlight++;
                values[index] = PENDING;
                return maybePromise._proxyPromiseArray(this, index);
            } else if (maybePromise.isFulfilled()) {
                ret = maybePromise.value();
            } else {
                maybePromise._unsetRejectionIsUnhandled();
                return this._reject(maybePromise.reason());
            }
        }
        values[index] = ret;
    }
    var totalResolved = ++this._totalResolved;
    if (totalResolved >= length) {
        if (preservedValues !== null) {
            this._filter(values, preservedValues);
        } else {
            this._resolve(values);
        }

    }
};

MappingPromiseArray.prototype._drainQueue =
function MappingPromiseArray$_drainQueue() {
    var queue = this._queue;
    var limit = this._limit;
    var values = this._values;
    while (queue.length > 0 && this._inFlight < limit) {
        var index = queue.pop();
        this._promiseFulfilled(values[index], index);
    }
};

MappingPromiseArray.prototype._filter =
function MappingPromiseArray$_filter(booleans, values) {
    var len = values.length;
    var ret = new Array(len);
    var j = 0;
    for (var i = 0; i < len; ++i) {
        if (booleans[i]) ret[j++] = values[i];
    }
    ret.length = j;
    this._resolve(ret);
};

MappingPromiseArray.prototype.preservedValues =
function MappingPromiseArray$preserveValues() {
    return this._preservedValues;
};

function map(promises, fn, options, _filter) {
    var limit = typeof options === "object" && options !== null
        ? options.concurrency
        : 0;
    limit = typeof limit === "number" &&
        isFinite(limit) && limit >= 1 ? limit : 0;
    return new MappingPromiseArray(promises, fn, limit, _filter);
}

Promise.prototype.map = function Promise$map(fn, options) {
    if (typeof fn !== "function") return apiRejection("fn must be a function");

    return map(this, fn, options, null).promise();
};

Promise.map = function Promise$Map(promises, fn, options, _filter) {
    if (typeof fn !== "function") return apiRejection("fn must be a function");
    return map(promises, fn, options, _filter).promise();
};


};

},{"./util.js":45}],28:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise) {
var util = require("./util.js");
var async = require("./async.js");
var tryCatch2 = util.tryCatch2;
var tryCatch1 = util.tryCatch1;
var errorObj = util.errorObj;

function thrower(r) {
    throw r;
}

function Promise$_spreadAdapter(val, receiver) {
    if (!util.isArray(val)) return Promise$_successAdapter(val, receiver);
    var ret = util.tryCatchApply(this, [null].concat(val), receiver);
    if (ret === errorObj) {
        async.invokeLater(thrower, void 0, ret.e);
    }
}

function Promise$_successAdapter(val, receiver) {
    var nodeback = this;
    var ret = val === void 0
        ? tryCatch1(nodeback, receiver, null)
        : tryCatch2(nodeback, receiver, null, val);
    if (ret === errorObj) {
        async.invokeLater(thrower, void 0, ret.e);
    }
}
function Promise$_errorAdapter(reason, receiver) {
    var nodeback = this;
    var ret = tryCatch1(nodeback, receiver, reason);
    if (ret === errorObj) {
        async.invokeLater(thrower, void 0, ret.e);
    }
}

Promise.prototype.nodeify = function Promise$nodeify(nodeback, options) {
    if (typeof nodeback == "function") {
        var adapter = Promise$_successAdapter;
        if (options !== void 0 && Object(options).spread) {
            adapter = Promise$_spreadAdapter;
        }
        this._then(
            adapter,
            Promise$_errorAdapter,
            void 0,
            nodeback,
            this._boundTo
        );
    }
    return this;
};
};

},{"./async.js":12,"./util.js":45}],29:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, PromiseArray) {
var util = require("./util.js");
var async = require("./async.js");
var errors = require("./errors.js");
var tryCatch1 = util.tryCatch1;
var errorObj = util.errorObj;

Promise.prototype.progressed = function Promise$progressed(handler) {
    return this._then(void 0, void 0, handler, void 0, void 0);
};

Promise.prototype._progress = function Promise$_progress(progressValue) {
    if (this._isFollowingOrFulfilledOrRejected()) return;
    this._progressUnchecked(progressValue);

};

Promise.prototype._clearFirstHandlerData$Base =
Promise.prototype._clearFirstHandlerData;
Promise.prototype._clearFirstHandlerData =
function Promise$_clearFirstHandlerData() {
    this._clearFirstHandlerData$Base();
    this._progressHandler0 = void 0;
};

Promise.prototype._progressHandlerAt =
function Promise$_progressHandlerAt(index) {
    return index === 0
        ? this._progressHandler0
        : this[(index << 2) + index - 5 + 2];
};

Promise.prototype._doProgressWith =
function Promise$_doProgressWith(progression) {
    var progressValue = progression.value;
    var handler = progression.handler;
    var promise = progression.promise;
    var receiver = progression.receiver;

    var ret = tryCatch1(handler, receiver, progressValue);
    if (ret === errorObj) {
        if (ret.e != null &&
            ret.e.name !== "StopProgressPropagation") {
            var trace = errors.canAttach(ret.e)
                ? ret.e : new Error(ret.e + "");
            promise._attachExtraTrace(trace);
            promise._progress(ret.e);
        }
    } else if (ret instanceof Promise) {
        ret._then(promise._progress, null, null, promise, void 0);
    } else {
        promise._progress(ret);
    }
};


Promise.prototype._progressUnchecked =
function Promise$_progressUnchecked(progressValue) {
    if (!this.isPending()) return;
    var len = this._length();
    var progress = this._progress;
    for (var i = 0; i < len; i++) {
        var handler = this._progressHandlerAt(i);
        var promise = this._promiseAt(i);
        if (!(promise instanceof Promise)) {
            var receiver = this._receiverAt(i);
            if (typeof handler === "function") {
                handler.call(receiver, progressValue, promise);
            } else if (receiver instanceof Promise && receiver._isProxied()) {
                receiver._progressUnchecked(progressValue);
            } else if (receiver instanceof PromiseArray) {
                receiver._promiseProgressed(progressValue, promise);
            }
            continue;
        }

        if (typeof handler === "function") {
            async.invoke(this._doProgressWith, this, {
                handler: handler,
                promise: promise,
                receiver: this._receiverAt(i),
                value: progressValue
            });
        } else {
            async.invoke(progress, promise, progressValue);
        }
    }
};
};

},{"./async.js":12,"./errors.js":20,"./util.js":45}],30:[function(require,module,exports){
(function (process){/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var old;
if (typeof Promise !== "undefined") old = Promise;
function noConflict(bluebird) {
    try { if (Promise === bluebird) Promise = old; }
    catch (e) {}
    return bluebird;
}
module.exports = function() {
var util = require("./util.js");
var async = require("./async.js");
var errors = require("./errors.js");

var INTERNAL = function(){};
var APPLY = {};
var NEXT_FILTER = {e: null};

var cast = require("./thenables.js")(Promise, INTERNAL);
var PromiseArray = require("./promise_array.js")(Promise, INTERNAL, cast);
var CapturedTrace = require("./captured_trace.js")();
var CatchFilter = require("./catch_filter.js")(NEXT_FILTER);
var PromiseResolver = require("./promise_resolver.js");

var isArray = util.isArray;

var errorObj = util.errorObj;
var tryCatch1 = util.tryCatch1;
var tryCatch2 = util.tryCatch2;
var tryCatchApply = util.tryCatchApply;
var RangeError = errors.RangeError;
var TypeError = errors.TypeError;
var CancellationError = errors.CancellationError;
var TimeoutError = errors.TimeoutError;
var OperationalError = errors.OperationalError;
var originatesFromRejection = errors.originatesFromRejection;
var markAsOriginatingFromRejection = errors.markAsOriginatingFromRejection;
var canAttach = errors.canAttach;
var thrower = util.thrower;
var apiRejection = require("./errors_api_rejection")(Promise);


var makeSelfResolutionError = function Promise$_makeSelfResolutionError() {
    return new TypeError("circular promise resolution chain");
};

function Promise(resolver) {
    if (typeof resolver !== "function") {
        throw new TypeError("the promise constructor requires a resolver function");
    }
    if (this.constructor !== Promise) {
        throw new TypeError("the promise constructor cannot be invoked directly");
    }
    this._bitField = 0;
    this._fulfillmentHandler0 = void 0;
    this._rejectionHandler0 = void 0;
    this._promise0 = void 0;
    this._receiver0 = void 0;
    this._settledValue = void 0;
    this._boundTo = void 0;
    if (resolver !== INTERNAL) this._resolveFromResolver(resolver);
}

function returnFirstElement(elements) {
    return elements[0];
}

Promise.prototype.bind = function Promise$bind(thisArg) {
    var maybePromise = cast(thisArg, void 0);
    var ret = new Promise(INTERNAL);
    if (maybePromise instanceof Promise) {
        var binder = maybePromise.then(function(thisArg) {
            ret._setBoundTo(thisArg);
        });
        var p = Promise.all([this, binder]).then(returnFirstElement);
        ret._follow(p);
    } else {
        ret._follow(this);
        ret._setBoundTo(thisArg);
    }
    ret._propagateFrom(this, 2 | 1);
    return ret;
};

Promise.prototype.toString = function Promise$toString() {
    return "[object Promise]";
};

Promise.prototype.caught = Promise.prototype["catch"] =
function Promise$catch(fn) {
    var len = arguments.length;
    if (len > 1) {
        var catchInstances = new Array(len - 1),
            j = 0, i;
        for (i = 0; i < len - 1; ++i) {
            var item = arguments[i];
            if (typeof item === "function") {
                catchInstances[j++] = item;
            } else {
                var catchFilterTypeError =
                    new TypeError(
                        "A catch filter must be an error constructor "
                        + "or a filter function");

                this._attachExtraTrace(catchFilterTypeError);
                async.invoke(this._reject, this, catchFilterTypeError);
                return;
            }
        }
        catchInstances.length = j;
        fn = arguments[i];

        this._resetTrace();
        var catchFilter = new CatchFilter(catchInstances, fn, this);
        return this._then(void 0, catchFilter.doFilter, void 0,
            catchFilter, void 0);
    }
    return this._then(void 0, fn, void 0, void 0, void 0);
};

Promise.prototype.then =
function Promise$then(didFulfill, didReject, didProgress) {
    return this._then(didFulfill, didReject, didProgress,
        void 0, void 0);
};


Promise.prototype.done =
function Promise$done(didFulfill, didReject, didProgress) {
    var promise = this._then(didFulfill, didReject, didProgress,
        void 0, void 0);
    promise._setIsFinal();
};

Promise.prototype.spread = function Promise$spread(didFulfill, didReject) {
    return this._then(didFulfill, didReject, void 0,
        APPLY, void 0);
};

Promise.prototype.isCancellable = function Promise$isCancellable() {
    return !this.isResolved() &&
        this._cancellable();
};

Promise.prototype.toJSON = function Promise$toJSON() {
    var ret = {
        isFulfilled: false,
        isRejected: false,
        fulfillmentValue: void 0,
        rejectionReason: void 0
    };
    if (this.isFulfilled()) {
        ret.fulfillmentValue = this._settledValue;
        ret.isFulfilled = true;
    } else if (this.isRejected()) {
        ret.rejectionReason = this._settledValue;
        ret.isRejected = true;
    }
    return ret;
};

Promise.prototype.all = function Promise$all() {
    return new PromiseArray(this).promise();
};


Promise.is = function Promise$Is(val) {
    return val instanceof Promise;
};

Promise.all = function Promise$All(promises) {
    return new PromiseArray(promises).promise();
};

Promise.prototype.error = function Promise$_error(fn) {
    return this.caught(originatesFromRejection, fn);
};

Promise.prototype._resolveFromSyncValue =
function Promise$_resolveFromSyncValue(value) {
    if (value === errorObj) {
        this._cleanValues();
        this._setRejected();
        this._settledValue = value.e;
        this._ensurePossibleRejectionHandled();
    } else {
        var maybePromise = cast(value, void 0);
        if (maybePromise instanceof Promise) {
            this._follow(maybePromise);
        } else {
            this._cleanValues();
            this._setFulfilled();
            this._settledValue = value;
        }
    }
};

Promise.method = function Promise$_Method(fn) {
    if (typeof fn !== "function") {
        throw new TypeError("fn must be a function");
    }
    return function Promise$_method() {
        var value;
        switch(arguments.length) {
        case 0: value = tryCatch1(fn, this, void 0); break;
        case 1: value = tryCatch1(fn, this, arguments[0]); break;
        case 2: value = tryCatch2(fn, this, arguments[0], arguments[1]); break;
        default:
            var $_len = arguments.length;var args = new Array($_len); for(var $_i = 0; $_i < $_len; ++$_i) {args[$_i] = arguments[$_i];}
            value = tryCatchApply(fn, args, this); break;
        }
        var ret = new Promise(INTERNAL);
        ret._setTrace(void 0);
        ret._resolveFromSyncValue(value);
        return ret;
    };
};

Promise.attempt = Promise["try"] = function Promise$_Try(fn, args, ctx) {
    if (typeof fn !== "function") {
        return apiRejection("fn must be a function");
    }
    var value = isArray(args)
        ? tryCatchApply(fn, args, ctx)
        : tryCatch1(fn, ctx, args);

    var ret = new Promise(INTERNAL);
    ret._setTrace(void 0);
    ret._resolveFromSyncValue(value);
    return ret;
};

Promise.defer = Promise.pending = function Promise$Defer() {
    var promise = new Promise(INTERNAL);
    promise._setTrace(void 0);
    return new PromiseResolver(promise);
};

Promise.bind = function Promise$Bind(thisArg) {
    var maybePromise = cast(thisArg, void 0);
    var ret = new Promise(INTERNAL);
    ret._setTrace(void 0);

    if (maybePromise instanceof Promise) {
        var p = maybePromise.then(function(thisArg) {
            ret._setBoundTo(thisArg);
        });
        ret._follow(p);
    } else {
        ret._setBoundTo(thisArg);
        ret._setFulfilled();
    }
    return ret;
};

Promise.cast = function Promise$_Cast(obj) {
    var ret = cast(obj, void 0);
    if (!(ret instanceof Promise)) {
        var val = ret;
        ret = new Promise(INTERNAL);
        ret._setTrace(void 0);
        ret._setFulfilled();
        ret._cleanValues();
        ret._settledValue = val;
    }
    return ret;
};

Promise.resolve = Promise.fulfilled = Promise.cast;

Promise.reject = Promise.rejected = function Promise$Reject(reason) {
    var ret = new Promise(INTERNAL);
    ret._setTrace(void 0);
    markAsOriginatingFromRejection(reason);
    ret._cleanValues();
    ret._setRejected();
    ret._settledValue = reason;
    if (!canAttach(reason)) {
        var trace = new Error(reason + "");
        ret._setCarriedStackTrace(trace);
    }
    ret._ensurePossibleRejectionHandled();
    return ret;
};

Promise.onPossiblyUnhandledRejection =
function Promise$OnPossiblyUnhandledRejection(fn) {
        CapturedTrace.possiblyUnhandledRejection = typeof fn === "function"
                                                    ? fn : void 0;
};

var unhandledRejectionHandled;
Promise.onUnhandledRejectionHandled =
function Promise$onUnhandledRejectionHandled(fn) {
    unhandledRejectionHandled = typeof fn === "function" ? fn : void 0;
};

var debugging = false || !!(
    typeof process !== "undefined" &&
    typeof process.execPath === "string" &&
    typeof process.env === "object" &&
    (process.env["BLUEBIRD_DEBUG"] ||
        process.env["NODE_ENV"] === "development")
);


Promise.longStackTraces = function Promise$LongStackTraces() {
    if (async.haveItemsQueued() &&
        debugging === false
   ) {
        throw new Error("cannot enable long stack traces after promises have been created");
    }
    debugging = CapturedTrace.isSupported();
};

Promise.hasLongStackTraces = function Promise$HasLongStackTraces() {
    return debugging && CapturedTrace.isSupported();
};

Promise.prototype._then =
function Promise$_then(
    didFulfill,
    didReject,
    didProgress,
    receiver,
    internalData
) {
    var haveInternalData = internalData !== void 0;
    var ret = haveInternalData ? internalData : new Promise(INTERNAL);

    if (!haveInternalData) {
        if (debugging) {
            var haveSameContext = this._peekContext() === this._traceParent;
            ret._traceParent = haveSameContext ? this._traceParent : this;
        }
        ret._propagateFrom(this, 7);
    }

    var callbackIndex =
        this._addCallbacks(didFulfill, didReject, didProgress, ret, receiver);

    if (this.isResolved()) {
        async.invoke(this._queueSettleAt, this, callbackIndex);
    }

    return ret;
};

Promise.prototype._length = function Promise$_length() {
    return this._bitField & 262143;
};

Promise.prototype._isFollowingOrFulfilledOrRejected =
function Promise$_isFollowingOrFulfilledOrRejected() {
    return (this._bitField & 939524096) > 0;
};

Promise.prototype._isFollowing = function Promise$_isFollowing() {
    return (this._bitField & 536870912) === 536870912;
};

Promise.prototype._setLength = function Promise$_setLength(len) {
    this._bitField = (this._bitField & -262144) |
        (len & 262143);
};

Promise.prototype._setFulfilled = function Promise$_setFulfilled() {
    this._bitField = this._bitField | 268435456;
};

Promise.prototype._setRejected = function Promise$_setRejected() {
    this._bitField = this._bitField | 134217728;
};

Promise.prototype._setFollowing = function Promise$_setFollowing() {
    this._bitField = this._bitField | 536870912;
};

Promise.prototype._setIsFinal = function Promise$_setIsFinal() {
    this._bitField = this._bitField | 33554432;
};

Promise.prototype._isFinal = function Promise$_isFinal() {
    return (this._bitField & 33554432) > 0;
};

Promise.prototype._cancellable = function Promise$_cancellable() {
    return (this._bitField & 67108864) > 0;
};

Promise.prototype._setCancellable = function Promise$_setCancellable() {
    this._bitField = this._bitField | 67108864;
};

Promise.prototype._unsetCancellable = function Promise$_unsetCancellable() {
    this._bitField = this._bitField & (~67108864);
};

Promise.prototype._setRejectionIsUnhandled =
function Promise$_setRejectionIsUnhandled() {
    this._bitField = this._bitField | 2097152;
};

Promise.prototype._unsetRejectionIsUnhandled =
function Promise$_unsetRejectionIsUnhandled() {
    this._bitField = this._bitField & (~2097152);
    if (this._isUnhandledRejectionNotified()) {
        this._unsetUnhandledRejectionIsNotified();
        this._notifyUnhandledRejectionIsHandled();
    }
};

Promise.prototype._isRejectionUnhandled =
function Promise$_isRejectionUnhandled() {
    return (this._bitField & 2097152) > 0;
};

Promise.prototype._setUnhandledRejectionIsNotified =
function Promise$_setUnhandledRejectionIsNotified() {
    this._bitField = this._bitField | 524288;
};

Promise.prototype._unsetUnhandledRejectionIsNotified =
function Promise$_unsetUnhandledRejectionIsNotified() {
    this._bitField = this._bitField & (~524288);
};

Promise.prototype._isUnhandledRejectionNotified =
function Promise$_isUnhandledRejectionNotified() {
    return (this._bitField & 524288) > 0;
};

Promise.prototype._setCarriedStackTrace =
function Promise$_setCarriedStackTrace(capturedTrace) {
    this._bitField = this._bitField | 1048576;
    this._fulfillmentHandler0 = capturedTrace;
};

Promise.prototype._unsetCarriedStackTrace =
function Promise$_unsetCarriedStackTrace() {
    this._bitField = this._bitField & (~1048576);
    this._fulfillmentHandler0 = void 0;
};

Promise.prototype._isCarryingStackTrace =
function Promise$_isCarryingStackTrace() {
    return (this._bitField & 1048576) > 0;
};

Promise.prototype._getCarriedStackTrace =
function Promise$_getCarriedStackTrace() {
    return this._isCarryingStackTrace()
        ? this._fulfillmentHandler0
        : void 0;
};

Promise.prototype._receiverAt = function Promise$_receiverAt(index) {
    var ret = index === 0
        ? this._receiver0
        : this[(index << 2) + index - 5 + 4];
    if (this._isBound() && ret === void 0) {
        return this._boundTo;
    }
    return ret;
};

Promise.prototype._promiseAt = function Promise$_promiseAt(index) {
    return index === 0
        ? this._promise0
        : this[(index << 2) + index - 5 + 3];
};

Promise.prototype._fulfillmentHandlerAt =
function Promise$_fulfillmentHandlerAt(index) {
    return index === 0
        ? this._fulfillmentHandler0
        : this[(index << 2) + index - 5 + 0];
};

Promise.prototype._rejectionHandlerAt =
function Promise$_rejectionHandlerAt(index) {
    return index === 0
        ? this._rejectionHandler0
        : this[(index << 2) + index - 5 + 1];
};

Promise.prototype._addCallbacks = function Promise$_addCallbacks(
    fulfill,
    reject,
    progress,
    promise,
    receiver
) {
    var index = this._length();

    if (index >= 262143 - 5) {
        index = 0;
        this._setLength(0);
    }

    if (index === 0) {
        this._promise0 = promise;
        if (receiver !== void 0) this._receiver0 = receiver;
        if (typeof fulfill === "function" && !this._isCarryingStackTrace())
            this._fulfillmentHandler0 = fulfill;
        if (typeof reject === "function") this._rejectionHandler0 = reject;
        if (typeof progress === "function") this._progressHandler0 = progress;
    } else {
        var base = (index << 2) + index - 5;
        this[base + 3] = promise;
        this[base + 4] = receiver;
        this[base + 0] = typeof fulfill === "function"
                                            ? fulfill : void 0;
        this[base + 1] = typeof reject === "function"
                                            ? reject : void 0;
        this[base + 2] = typeof progress === "function"
                                            ? progress : void 0;
    }
    this._setLength(index + 1);
    return index;
};

Promise.prototype._setProxyHandlers =
function Promise$_setProxyHandlers(receiver, promiseSlotValue) {
    var index = this._length();

    if (index >= 262143 - 5) {
        index = 0;
        this._setLength(0);
    }
    if (index === 0) {
        this._promise0 = promiseSlotValue;
        this._receiver0 = receiver;
    } else {
        var base = (index << 2) + index - 5;
        this[base + 3] = promiseSlotValue;
        this[base + 4] = receiver;
        this[base + 0] =
        this[base + 1] =
        this[base + 2] = void 0;
    }
    this._setLength(index + 1);
};

Promise.prototype._proxyPromiseArray =
function Promise$_proxyPromiseArray(promiseArray, index) {
    this._setProxyHandlers(promiseArray, index);
};

Promise.prototype._proxyPromise = function Promise$_proxyPromise(promise) {
    promise._setProxied();
    this._setProxyHandlers(promise, -15);
};

Promise.prototype._setBoundTo = function Promise$_setBoundTo(obj) {
    if (obj !== void 0) {
        this._bitField = this._bitField | 8388608;
        this._boundTo = obj;
    } else {
        this._bitField = this._bitField & (~8388608);
    }
};

Promise.prototype._isBound = function Promise$_isBound() {
    return (this._bitField & 8388608) === 8388608;
};

Promise.prototype._resolveFromResolver =
function Promise$_resolveFromResolver(resolver) {
    var promise = this;
    this._setTrace(void 0);
    this._pushContext();

    function Promise$_resolver(val) {
        if (promise._tryFollow(val)) {
            return;
        }
        promise._fulfill(val);
    }
    function Promise$_rejecter(val) {
        var trace = canAttach(val) ? val : new Error(val + "");
        promise._attachExtraTrace(trace);
        markAsOriginatingFromRejection(val);
        promise._reject(val, trace === val ? void 0 : trace);
    }
    var r = tryCatch2(resolver, void 0, Promise$_resolver, Promise$_rejecter);
    this._popContext();

    if (r !== void 0 && r === errorObj) {
        var e = r.e;
        var trace = canAttach(e) ? e : new Error(e + "");
        promise._reject(e, trace);
    }
};

Promise.prototype._spreadSlowCase =
function Promise$_spreadSlowCase(targetFn, promise, values, boundTo) {
    var promiseForAll = new PromiseArray(values).promise();
    var promise2 = promiseForAll._then(function() {
        return targetFn.apply(boundTo, arguments);
    }, void 0, void 0, APPLY, void 0);
    promise._follow(promise2);
};

Promise.prototype._callSpread =
function Promise$_callSpread(handler, promise, value) {
    var boundTo = this._boundTo;
    if (isArray(value)) {
        for (var i = 0, len = value.length; i < len; ++i) {
            if (cast(value[i], void 0) instanceof Promise) {
                this._spreadSlowCase(handler, promise, value, boundTo);
                return;
            }
        }
    }
    promise._pushContext();
    return tryCatchApply(handler, value, boundTo);
};

Promise.prototype._callHandler =
function Promise$_callHandler(
    handler, receiver, promise, value) {
    var x;
    if (receiver === APPLY && !this.isRejected()) {
        x = this._callSpread(handler, promise, value);
    } else {
        promise._pushContext();
        x = tryCatch1(handler, receiver, value);
    }
    promise._popContext();
    return x;
};

Promise.prototype._settlePromiseFromHandler =
function Promise$_settlePromiseFromHandler(
    handler, receiver, value, promise
) {
    if (!(promise instanceof Promise)) {
        handler.call(receiver, value, promise);
        return;
    }
    var x = this._callHandler(handler, receiver, promise, value);
    if (promise._isFollowing()) return;

    if (x === errorObj || x === promise || x === NEXT_FILTER) {
        var err = x === promise
                    ? makeSelfResolutionError()
                    : x.e;
        var trace = canAttach(err) ? err : new Error(err + "");
        if (x !== NEXT_FILTER) promise._attachExtraTrace(trace);
        promise._rejectUnchecked(err, trace);
    } else {
        var castValue = cast(x, promise);
        if (castValue instanceof Promise) {
            if (castValue.isRejected() &&
                !castValue._isCarryingStackTrace() &&
                !canAttach(castValue._settledValue)) {
                var trace = new Error(castValue._settledValue + "");
                promise._attachExtraTrace(trace);
                castValue._setCarriedStackTrace(trace);
            }
            promise._follow(castValue);
            promise._propagateFrom(castValue, 1);
        } else {
            promise._fulfillUnchecked(x);
        }
    }
};

Promise.prototype._follow =
function Promise$_follow(promise) {
    this._setFollowing();

    if (promise.isPending()) {
        this._propagateFrom(promise, 1);
        promise._proxyPromise(this);
    } else if (promise.isFulfilled()) {
        this._fulfillUnchecked(promise._settledValue);
    } else {
        this._rejectUnchecked(promise._settledValue,
            promise._getCarriedStackTrace());
    }

    if (promise._isRejectionUnhandled()) promise._unsetRejectionIsUnhandled();

    if (debugging &&
        promise._traceParent == null) {
        promise._traceParent = this;
    }
};

Promise.prototype._tryFollow =
function Promise$_tryFollow(value) {
    if (this._isFollowingOrFulfilledOrRejected() ||
        value === this) {
        return false;
    }
    var maybePromise = cast(value, void 0);
    if (!(maybePromise instanceof Promise)) {
        return false;
    }
    this._follow(maybePromise);
    return true;
};

Promise.prototype._resetTrace = function Promise$_resetTrace() {
    if (debugging) {
        this._trace = new CapturedTrace(this._peekContext() === void 0);
    }
};

Promise.prototype._setTrace = function Promise$_setTrace(parent) {
    if (debugging) {
        var context = this._peekContext();
        this._traceParent = context;
        var isTopLevel = context === void 0;
        if (parent !== void 0 &&
            parent._traceParent === context) {
            this._trace = parent._trace;
        } else {
            this._trace = new CapturedTrace(isTopLevel);
        }
    }
    return this;
};

Promise.prototype._attachExtraTrace =
function Promise$_attachExtraTrace(error) {
    if (debugging) {
        var promise = this;
        var stack = error.stack;
        stack = typeof stack === "string" ? stack.split("\n") : [];
        CapturedTrace.protectErrorMessageNewlines(stack);
        var headerLineCount = 1;
        var combinedTraces = 1;
        while(promise != null &&
            promise._trace != null) {
            stack = CapturedTrace.combine(
                stack,
                promise._trace.stack.split("\n")
            );
            promise = promise._traceParent;
            combinedTraces++;
        }

        var stackTraceLimit = Error.stackTraceLimit || 10;
        var max = (stackTraceLimit + headerLineCount) * combinedTraces;
        var len = stack.length;
        if (len > max) {
            stack.length = max;
        }

        if (len > 0)
            stack[0] = stack[0].split("\u0002\u0000\u0001").join("\n");

        if (stack.length <= headerLineCount) {
            error.stack = "(No stack trace)";
        } else {
            error.stack = stack.join("\n");
        }
    }
};

Promise.prototype._cleanValues = function Promise$_cleanValues() {
    if (this._cancellable()) {
        this._cancellationParent = void 0;
    }
};

Promise.prototype._propagateFrom =
function Promise$_propagateFrom(parent, flags) {
    if ((flags & 1) > 0 && parent._cancellable()) {
        this._setCancellable();
        this._cancellationParent = parent;
    }
    if ((flags & 4) > 0) {
        this._setBoundTo(parent._boundTo);
    }
    if ((flags & 2) > 0) {
        this._setTrace(parent);
    }
};

Promise.prototype._fulfill = function Promise$_fulfill(value) {
    if (this._isFollowingOrFulfilledOrRejected()) return;
    this._fulfillUnchecked(value);
};

Promise.prototype._reject =
function Promise$_reject(reason, carriedStackTrace) {
    if (this._isFollowingOrFulfilledOrRejected()) return;
    this._rejectUnchecked(reason, carriedStackTrace);
};

Promise.prototype._settlePromiseAt = function Promise$_settlePromiseAt(index) {
    var handler = this.isFulfilled()
        ? this._fulfillmentHandlerAt(index)
        : this._rejectionHandlerAt(index);

    var value = this._settledValue;
    var receiver = this._receiverAt(index);
    var promise = this._promiseAt(index);

    if (typeof handler === "function") {
        this._settlePromiseFromHandler(handler, receiver, value, promise);
    } else {
        var done = false;
        var isFulfilled = this.isFulfilled();
        if (receiver !== void 0) {
            if (receiver instanceof Promise &&
                receiver._isProxied()) {
                receiver._unsetProxied();

                if (isFulfilled) receiver._fulfillUnchecked(value);
                else receiver._rejectUnchecked(value,
                    this._getCarriedStackTrace());
                done = true;
            } else if (receiver instanceof PromiseArray) {
                if (isFulfilled) receiver._promiseFulfilled(value, promise);
                else receiver._promiseRejected(value, promise);
                done = true;
            }
        }

        if (!done) {
            if (isFulfilled) promise._fulfill(value);
            else promise._reject(value, this._getCarriedStackTrace());
        }
    }

    if (index >= 4) {
        this._queueGC();
    }
};

Promise.prototype._isProxied = function Promise$_isProxied() {
    return (this._bitField & 4194304) === 4194304;
};

Promise.prototype._setProxied = function Promise$_setProxied() {
    this._bitField = this._bitField | 4194304;
};

Promise.prototype._unsetProxied = function Promise$_unsetProxied() {
    this._bitField = this._bitField & (~4194304);
};

Promise.prototype._isGcQueued = function Promise$_isGcQueued() {
    return (this._bitField & -1073741824) === -1073741824;
};

Promise.prototype._setGcQueued = function Promise$_setGcQueued() {
    this._bitField = this._bitField | -1073741824;
};

Promise.prototype._unsetGcQueued = function Promise$_unsetGcQueued() {
    this._bitField = this._bitField & (~-1073741824);
};

Promise.prototype._queueGC = function Promise$_queueGC() {
    if (this._isGcQueued()) return;
    this._setGcQueued();
    async.invokeLater(this._gc, this, void 0);
};

Promise.prototype._gc = function Promise$gc() {
    var len = this._length() * 5 - 5;
    for (var i = 0; i < len; i++) {
        delete this[i];
    }
    this._clearFirstHandlerData();
    this._setLength(0);
    this._unsetGcQueued();
};

Promise.prototype._clearFirstHandlerData =
function Promise$_clearFirstHandlerData() {
    this._fulfillmentHandler0 = void 0;
    this._rejectionHandler0 = void 0;
    this._promise0 = void 0;
    this._receiver0 = void 0;
};

Promise.prototype._queueSettleAt = function Promise$_queueSettleAt(index) {
    if (this._isRejectionUnhandled()) this._unsetRejectionIsUnhandled();
    async.invoke(this._settlePromiseAt, this, index);
};

Promise.prototype._fulfillUnchecked =
function Promise$_fulfillUnchecked(value) {
    if (!this.isPending()) return;
    if (value === this) {
        var err = makeSelfResolutionError();
        this._attachExtraTrace(err);
        return this._rejectUnchecked(err, void 0);
    }
    this._cleanValues();
    this._setFulfilled();
    this._settledValue = value;
    var len = this._length();

    if (len > 0) {
        async.invoke(this._settlePromises, this, len);
    }
};

Promise.prototype._rejectUncheckedCheckError =
function Promise$_rejectUncheckedCheckError(reason) {
    var trace = canAttach(reason) ? reason : new Error(reason + "");
    this._rejectUnchecked(reason, trace === reason ? void 0 : trace);
};

Promise.prototype._rejectUnchecked =
function Promise$_rejectUnchecked(reason, trace) {
    if (!this.isPending()) return;
    if (reason === this) {
        var err = makeSelfResolutionError();
        this._attachExtraTrace(err);
        return this._rejectUnchecked(err);
    }
    this._cleanValues();
    this._setRejected();
    this._settledValue = reason;

    if (this._isFinal()) {
        async.invokeLater(thrower, void 0, trace === void 0 ? reason : trace);
        return;
    }
    var len = this._length();

    if (trace !== void 0) this._setCarriedStackTrace(trace);

    if (len > 0) {
        async.invoke(this._rejectPromises, this, null);
    } else {
        this._ensurePossibleRejectionHandled();
    }
};

Promise.prototype._rejectPromises = function Promise$_rejectPromises() {
    this._settlePromises();
    this._unsetCarriedStackTrace();
};

Promise.prototype._settlePromises = function Promise$_settlePromises() {
    var len = this._length();
    for (var i = 0; i < len; i++) {
        this._settlePromiseAt(i);
    }
};

Promise.prototype._ensurePossibleRejectionHandled =
function Promise$_ensurePossibleRejectionHandled() {
    this._setRejectionIsUnhandled();
    if (CapturedTrace.possiblyUnhandledRejection !== void 0) {
        async.invokeLater(this._notifyUnhandledRejection, this, void 0);
    }
};

Promise.prototype._notifyUnhandledRejectionIsHandled =
function Promise$_notifyUnhandledRejectionIsHandled() {
    if (typeof unhandledRejectionHandled === "function") {
        async.invokeLater(unhandledRejectionHandled, void 0, this);
    }
};

Promise.prototype._notifyUnhandledRejection =
function Promise$_notifyUnhandledRejection() {
    if (this._isRejectionUnhandled()) {
        var reason = this._settledValue;
        var trace = this._getCarriedStackTrace();

        this._setUnhandledRejectionIsNotified();

        if (trace !== void 0) {
            this._unsetCarriedStackTrace();
            reason = trace;
        }
        if (typeof CapturedTrace.possiblyUnhandledRejection === "function") {
            CapturedTrace.possiblyUnhandledRejection(reason, this);
        }
    }
};

var contextStack = [];
Promise.prototype._peekContext = function Promise$_peekContext() {
    var lastIndex = contextStack.length - 1;
    if (lastIndex >= 0) {
        return contextStack[lastIndex];
    }
    return void 0;

};

Promise.prototype._pushContext = function Promise$_pushContext() {
    if (!debugging) return;
    contextStack.push(this);
};

Promise.prototype._popContext = function Promise$_popContext() {
    if (!debugging) return;
    contextStack.pop();
};

Promise.noConflict = function Promise$NoConflict() {
    return noConflict(Promise);
};

Promise.setScheduler = function(fn) {
    if (typeof fn !== "function") throw new TypeError("fn must be a function");
    async._schedule = fn;
};

if (!CapturedTrace.isSupported()) {
    Promise.longStackTraces = function(){};
    debugging = false;
}

Promise._makeSelfResolutionError = makeSelfResolutionError;
require("./finally.js")(Promise, NEXT_FILTER, cast);
require("./direct_resolve.js")(Promise);
require("./synchronous_inspection.js")(Promise);
require("./join.js")(Promise, PromiseArray, cast, INTERNAL);
Promise.RangeError = RangeError;
Promise.CancellationError = CancellationError;
Promise.TimeoutError = TimeoutError;
Promise.TypeError = TypeError;
Promise.OperationalError = OperationalError;
Promise.RejectionError = OperationalError;
Promise.AggregateError = errors.AggregateError;

util.toFastProperties(Promise);
util.toFastProperties(Promise.prototype);
Promise.Promise = Promise;
require('./timers.js')(Promise,INTERNAL,cast);
require('./race.js')(Promise,INTERNAL,cast);
require('./call_get.js')(Promise);
require('./generators.js')(Promise,apiRejection,INTERNAL,cast);
require('./map.js')(Promise,PromiseArray,apiRejection,cast,INTERNAL);
require('./nodeify.js')(Promise);
require('./promisify.js')(Promise,INTERNAL);
require('./props.js')(Promise,PromiseArray,cast);
require('./reduce.js')(Promise,PromiseArray,apiRejection,cast,INTERNAL);
require('./settle.js')(Promise,PromiseArray);
require('./some.js')(Promise,PromiseArray,apiRejection);
require('./progress.js')(Promise,PromiseArray);
require('./cancel.js')(Promise,INTERNAL);
require('./filter.js')(Promise,INTERNAL);
require('./any.js')(Promise,PromiseArray);
require('./each.js')(Promise,INTERNAL);
require('./using.js')(Promise,apiRejection,cast);

Promise.prototype = Promise.prototype;
return Promise;

};
}).call(this,require("/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js"))
},{"./any.js":11,"./async.js":12,"./call_get.js":14,"./cancel.js":15,"./captured_trace.js":16,"./catch_filter.js":17,"./direct_resolve.js":18,"./each.js":19,"./errors.js":20,"./errors_api_rejection":21,"./filter.js":23,"./finally.js":24,"./generators.js":25,"./join.js":26,"./map.js":27,"./nodeify.js":28,"./progress.js":29,"./promise_array.js":31,"./promise_resolver.js":32,"./promisify.js":33,"./props.js":34,"./race.js":36,"./reduce.js":37,"./settle.js":39,"./some.js":40,"./synchronous_inspection.js":41,"./thenables.js":42,"./timers.js":43,"./using.js":44,"./util.js":45,"/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js":3}],31:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL, cast) {
var canAttach = require("./errors.js").canAttach;
var util = require("./util.js");
var isArray = util.isArray;

function toResolutionValue(val) {
    switch(val) {
    case -1: return void 0;
    case -2: return [];
    case -3: return {};
    }
}

function PromiseArray(values) {
    var promise = this._promise = new Promise(INTERNAL);
    var parent = void 0;
    if (values instanceof Promise) {
        parent = values;
        promise._propagateFrom(parent, 1 | 4);
    }
    promise._setTrace(parent);
    this._values = values;
    this._length = 0;
    this._totalResolved = 0;
    this._init(void 0, -2);
}
PromiseArray.prototype.length = function PromiseArray$length() {
    return this._length;
};

PromiseArray.prototype.promise = function PromiseArray$promise() {
    return this._promise;
};

PromiseArray.prototype._init =
function PromiseArray$_init(_, resolveValueIfEmpty) {
    var values = cast(this._values, void 0);
    if (values instanceof Promise) {
        this._values = values;
        values._setBoundTo(this._promise._boundTo);
        if (values.isFulfilled()) {
            values = values._settledValue;
            if (!isArray(values)) {
                var err = new Promise.TypeError("expecting an array, a promise or a thenable");
                this.__hardReject__(err);
                return;
            }
        } else if (values.isPending()) {
            values._then(
                PromiseArray$_init,
                this._reject,
                void 0,
                this,
                resolveValueIfEmpty
           );
            return;
        } else {
            values._unsetRejectionIsUnhandled();
            this._reject(values._settledValue);
            return;
        }
    } else if (!isArray(values)) {
        var err = new Promise.TypeError("expecting an array, a promise or a thenable");
        this.__hardReject__(err);
        return;
    }

    if (values.length === 0) {
        if (resolveValueIfEmpty === -5) {
            this._resolveEmptyArray();
        }
        else {
            this._resolve(toResolutionValue(resolveValueIfEmpty));
        }
        return;
    }
    var len = this.getActualLength(values.length);
    var newLen = len;
    var newValues = this.shouldCopyValues() ? new Array(len) : this._values;
    var isDirectScanNeeded = false;
    for (var i = 0; i < len; ++i) {
        var maybePromise = cast(values[i], void 0);
        if (maybePromise instanceof Promise) {
            if (maybePromise.isPending()) {
                maybePromise._proxyPromiseArray(this, i);
            } else {
                maybePromise._unsetRejectionIsUnhandled();
                isDirectScanNeeded = true;
            }
        } else {
            isDirectScanNeeded = true;
        }
        newValues[i] = maybePromise;
    }
    this._values = newValues;
    this._length = newLen;
    if (isDirectScanNeeded) {
        this._scanDirectValues(len);
    }
};

PromiseArray.prototype._settlePromiseAt =
function PromiseArray$_settlePromiseAt(index) {
    var value = this._values[index];
    if (!(value instanceof Promise)) {
        this._promiseFulfilled(value, index);
    } else if (value.isFulfilled()) {
        this._promiseFulfilled(value._settledValue, index);
    } else if (value.isRejected()) {
        this._promiseRejected(value._settledValue, index);
    }
};

PromiseArray.prototype._scanDirectValues =
function PromiseArray$_scanDirectValues(len) {
    for (var i = 0; i < len; ++i) {
        if (this._isResolved()) {
            break;
        }
        this._settlePromiseAt(i);
    }
};

PromiseArray.prototype._isResolved = function PromiseArray$_isResolved() {
    return this._values === null;
};

PromiseArray.prototype._resolve = function PromiseArray$_resolve(value) {
    this._values = null;
    this._promise._fulfill(value);
};

PromiseArray.prototype.__hardReject__ =
PromiseArray.prototype._reject = function PromiseArray$_reject(reason) {
    this._values = null;
    var trace = canAttach(reason) ? reason : new Error(reason + "");
    this._promise._attachExtraTrace(trace);
    this._promise._reject(reason, trace);
};

PromiseArray.prototype._promiseProgressed =
function PromiseArray$_promiseProgressed(progressValue, index) {
    if (this._isResolved()) return;
    this._promise._progress({
        index: index,
        value: progressValue
    });
};


PromiseArray.prototype._promiseFulfilled =
function PromiseArray$_promiseFulfilled(value, index) {
    if (this._isResolved()) return;
    this._values[index] = value;
    var totalResolved = ++this._totalResolved;
    if (totalResolved >= this._length) {
        this._resolve(this._values);
    }
};

PromiseArray.prototype._promiseRejected =
function PromiseArray$_promiseRejected(reason, index) {
    if (this._isResolved()) return;
    this._totalResolved++;
    this._reject(reason);
};

PromiseArray.prototype.shouldCopyValues =
function PromiseArray$_shouldCopyValues() {
    return true;
};

PromiseArray.prototype.getActualLength =
function PromiseArray$getActualLength(len) {
    return len;
};

return PromiseArray;
};

},{"./errors.js":20,"./util.js":45}],32:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var util = require("./util.js");
var maybeWrapAsError = util.maybeWrapAsError;
var errors = require("./errors.js");
var TimeoutError = errors.TimeoutError;
var OperationalError = errors.OperationalError;
var async = require("./async.js");
var haveGetters = util.haveGetters;
var es5 = require("./es5.js");

function isUntypedError(obj) {
    return obj instanceof Error &&
        es5.getPrototypeOf(obj) === Error.prototype;
}

function wrapAsOperationalError(obj) {
    var ret;
    if (isUntypedError(obj)) {
        ret = new OperationalError(obj);
    } else {
        ret = obj;
    }
    errors.markAsOriginatingFromRejection(ret);
    return ret;
}

function nodebackForPromise(promise) {
    function PromiseResolver$_callback(err, value) {
        if (promise === null) return;

        if (err) {
            var wrapped = wrapAsOperationalError(maybeWrapAsError(err));
            promise._attachExtraTrace(wrapped);
            promise._reject(wrapped);
        } else if (arguments.length > 2) {
            var $_len = arguments.length;var args = new Array($_len - 1); for(var $_i = 1; $_i < $_len; ++$_i) {args[$_i - 1] = arguments[$_i];}
            promise._fulfill(args);
        } else {
            promise._fulfill(value);
        }

        promise = null;
    }
    return PromiseResolver$_callback;
}


var PromiseResolver;
if (!haveGetters) {
    PromiseResolver = function PromiseResolver(promise) {
        this.promise = promise;
        this.asCallback = nodebackForPromise(promise);
        this.callback = this.asCallback;
    };
}
else {
    PromiseResolver = function PromiseResolver(promise) {
        this.promise = promise;
    };
}
if (haveGetters) {
    var prop = {
        get: function() {
            return nodebackForPromise(this.promise);
        }
    };
    es5.defineProperty(PromiseResolver.prototype, "asCallback", prop);
    es5.defineProperty(PromiseResolver.prototype, "callback", prop);
}

PromiseResolver._nodebackForPromise = nodebackForPromise;

PromiseResolver.prototype.toString = function PromiseResolver$toString() {
    return "[object PromiseResolver]";
};

PromiseResolver.prototype.resolve =
PromiseResolver.prototype.fulfill = function PromiseResolver$resolve(value) {
    if (!(this instanceof PromiseResolver)) {
        throw new TypeError("Illegal invocation, resolver resolve/reject must be called within a resolver context. Consider using the promise constructor instead.");
    }

    var promise = this.promise;
    if (promise._tryFollow(value)) {
        return;
    }
    async.invoke(promise._fulfill, promise, value);
};

PromiseResolver.prototype.reject = function PromiseResolver$reject(reason) {
    if (!(this instanceof PromiseResolver)) {
        throw new TypeError("Illegal invocation, resolver resolve/reject must be called within a resolver context. Consider using the promise constructor instead.");
    }

    var promise = this.promise;
    errors.markAsOriginatingFromRejection(reason);
    var trace = errors.canAttach(reason) ? reason : new Error(reason + "");
    promise._attachExtraTrace(trace);
    async.invoke(promise._reject, promise, reason);
    if (trace !== reason) {
        async.invoke(this._setCarriedStackTrace, this, trace);
    }
};

PromiseResolver.prototype.progress =
function PromiseResolver$progress(value) {
    if (!(this instanceof PromiseResolver)) {
        throw new TypeError("Illegal invocation, resolver resolve/reject must be called within a resolver context. Consider using the promise constructor instead.");
    }
    async.invoke(this.promise._progress, this.promise, value);
};

PromiseResolver.prototype.cancel = function PromiseResolver$cancel() {
    async.invoke(this.promise.cancel, this.promise, void 0);
};

PromiseResolver.prototype.timeout = function PromiseResolver$timeout() {
    this.reject(new TimeoutError("timeout"));
};

PromiseResolver.prototype.isResolved = function PromiseResolver$isResolved() {
    return this.promise.isResolved();
};

PromiseResolver.prototype.toJSON = function PromiseResolver$toJSON() {
    return this.promise.toJSON();
};

PromiseResolver.prototype._setCarriedStackTrace =
function PromiseResolver$_setCarriedStackTrace(trace) {
    if (this.promise.isRejected()) {
        this.promise._setCarriedStackTrace(trace);
    }
};

module.exports = PromiseResolver;

},{"./async.js":12,"./errors.js":20,"./es5.js":22,"./util.js":45}],33:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL) {
var THIS = {};
var util = require("./util.js");
var nodebackForPromise = require("./promise_resolver.js")
    ._nodebackForPromise;
var withAppended = util.withAppended;
var maybeWrapAsError = util.maybeWrapAsError;
var canEvaluate = util.canEvaluate;
var TypeError = require("./errors").TypeError;
var defaultSuffix = "Async";
var defaultFilter = function(name, func) {
    return util.isIdentifier(name) &&
        name.charAt(0) !== "_" &&
        !util.isClass(func);
};
var defaultPromisified = {__isPromisified__: true};


function escapeIdentRegex(str) {
    return str.replace(/([$])/, "\\$");
}

function isPromisified(fn) {
    try {
        return fn.__isPromisified__ === true;
    }
    catch (e) {
        return false;
    }
}

function hasPromisified(obj, key, suffix) {
    var val = util.getDataPropertyOrDefault(obj, key + suffix,
                                            defaultPromisified);
    return val ? isPromisified(val) : false;
}
function checkValid(ret, suffix, suffixRegexp) {
    for (var i = 0; i < ret.length; i += 2) {
        var key = ret[i];
        if (suffixRegexp.test(key)) {
            var keyWithoutAsyncSuffix = key.replace(suffixRegexp, "");
            for (var j = 0; j < ret.length; j += 2) {
                if (ret[j] === keyWithoutAsyncSuffix) {
                    throw new TypeError("Cannot promisify an API " +
                        "that has normal methods with '"+suffix+"'-suffix");
                }
            }
        }
    }
}

function promisifiableMethods(obj, suffix, suffixRegexp, filter) {
    var keys = util.inheritedDataKeys(obj);
    var ret = [];
    for (var i = 0; i < keys.length; ++i) {
        var key = keys[i];
        var value = obj[key];
        if (typeof value === "function" &&
            !isPromisified(value) &&
            !hasPromisified(obj, key, suffix) &&
            filter(key, value, obj)) {
            ret.push(key, value);
        }
    }
    checkValid(ret, suffix, suffixRegexp);
    return ret;
}

function switchCaseArgumentOrder(likelyArgumentCount) {
    var ret = [likelyArgumentCount];
    var min = Math.max(0, likelyArgumentCount - 1 - 5);
    for(var i = likelyArgumentCount - 1; i >= min; --i) {
        if (i === likelyArgumentCount) continue;
        ret.push(i);
    }
    for(var i = likelyArgumentCount + 1; i <= 5; ++i) {
        ret.push(i);
    }
    return ret;
}

function argumentSequence(argumentCount) {
    return util.filledRange(argumentCount, "arguments[", "]");
}

function parameterDeclaration(parameterCount) {
    return util.filledRange(parameterCount, "_arg", "");
}

function parameterCount(fn) {
    if (typeof fn.length === "number") {
        return Math.max(Math.min(fn.length, 1023 + 1), 0);
    }
    return 0;
}

function generatePropertyAccess(key) {
    if (util.isIdentifier(key)) {
        return "." + key;
    }
    else return "['" + key.replace(/(['\\])/g, "\\$1") + "']";
}

function makeNodePromisifiedEval(callback, receiver, originalName, fn, suffix) {
    var newParameterCount = Math.max(0, parameterCount(fn) - 1);
    var argumentOrder = switchCaseArgumentOrder(newParameterCount);
    var callbackName =
        (typeof originalName === "string" && util.isIdentifier(originalName)
            ? originalName + suffix
            : "promisified");

    function generateCallForArgumentCount(count) {
        var args = argumentSequence(count).join(", ");
        var comma = count > 0 ? ", " : "";
        var ret;
        if (typeof callback === "string") {
            ret = "                                                          \n\
                this.method(args, fn);                                       \n\
                break;                                                       \n\
            ".replace(".method", generatePropertyAccess(callback));
        } else if (receiver === THIS) {
            ret =  "                                                         \n\
                callback.call(this, args, fn);                               \n\
                break;                                                       \n\
            ";
        } else if (receiver !== void 0) {
            ret =  "                                                         \n\
                callback.call(receiver, args, fn);                           \n\
                break;                                                       \n\
            ";
        } else {
            ret =  "                                                         \n\
                callback(args, fn);                                          \n\
                break;                                                       \n\
            ";
        }
        return ret.replace("args", args).replace(", ", comma);
    }

    function generateArgumentSwitchCase() {
        var ret = "";
        for(var i = 0; i < argumentOrder.length; ++i) {
            ret += "case " + argumentOrder[i] +":" +
                generateCallForArgumentCount(argumentOrder[i]);
        }
        var codeForCall;
        if (typeof callback === "string") {
            codeForCall = "                                                  \n\
                this.property.apply(this, args);                             \n\
            "
                .replace(".property", generatePropertyAccess(callback));
        } else if (receiver === THIS) {
            codeForCall = "                                                  \n\
                callback.apply(this, args);                                  \n\
            ";
        } else {
            codeForCall = "                                                  \n\
                callback.apply(receiver, args);                              \n\
            ";
        }

        ret += "                                                             \n\
        default:                                                             \n\
            var args = new Array(len + 1);                                   \n\
            var i = 0;                                                       \n\
            for (var i = 0; i < len; ++i) {                                  \n\
               args[i] = arguments[i];                                       \n\
            }                                                                \n\
            args[i] = fn;                                                    \n\
            [CodeForCall]                                                    \n\
            break;                                                           \n\
        ".replace("[CodeForCall]", codeForCall);
        return ret;
    }

    return new Function("Promise",
                        "callback",
                        "receiver",
                        "withAppended",
                        "maybeWrapAsError",
                        "nodebackForPromise",
                        "INTERNAL","                                         \n\
        var ret = function FunctionName(Parameters) {                        \n\
            'use strict';                                                    \n\
            var len = arguments.length;                                      \n\
            var promise = new Promise(INTERNAL);                             \n\
            promise._setTrace(void 0);                                       \n\
            var fn = nodebackForPromise(promise);                            \n\
            try {                                                            \n\
                switch(len) {                                                \n\
                    [CodeForSwitchCase]                                      \n\
                }                                                            \n\
            } catch (e) {                                                    \n\
                var wrapped = maybeWrapAsError(e);                           \n\
                promise._attachExtraTrace(wrapped);                          \n\
                promise._reject(wrapped);                                    \n\
            }                                                                \n\
            return promise;                                                  \n\
        };                                                                   \n\
        ret.__isPromisified__ = true;                                        \n\
        return ret;                                                          \n\
        "
        .replace("FunctionName", callbackName)
        .replace("Parameters", parameterDeclaration(newParameterCount))
        .replace("[CodeForSwitchCase]", generateArgumentSwitchCase()))(
            Promise,
            callback,
            receiver,
            withAppended,
            maybeWrapAsError,
            nodebackForPromise,
            INTERNAL
        );
}

function makeNodePromisifiedClosure(callback, receiver) {
    function promisified() {
        var _receiver = receiver;
        if (receiver === THIS) _receiver = this;
        if (typeof callback === "string") {
            callback = _receiver[callback];
        }
        var promise = new Promise(INTERNAL);
        promise._setTrace(void 0);
        var fn = nodebackForPromise(promise);
        try {
            callback.apply(_receiver, withAppended(arguments, fn));
        } catch(e) {
            var wrapped = maybeWrapAsError(e);
            promise._attachExtraTrace(wrapped);
            promise._reject(wrapped);
        }
        return promise;
    }
    promisified.__isPromisified__ = true;
    return promisified;
}

var makeNodePromisified = canEvaluate
    ? makeNodePromisifiedEval
    : makeNodePromisifiedClosure;

function promisifyAll(obj, suffix, filter, promisifier) {
    var suffixRegexp = new RegExp(escapeIdentRegex(suffix) + "$");
    var methods =
        promisifiableMethods(obj, suffix, suffixRegexp, filter);

    for (var i = 0, len = methods.length; i < len; i+= 2) {
        var key = methods[i];
        var fn = methods[i+1];
        var promisifiedKey = key + suffix;
        obj[promisifiedKey] = promisifier === makeNodePromisified
                ? makeNodePromisified(key, THIS, key, fn, suffix)
                : promisifier(fn);
    }
    util.toFastProperties(obj);
    return obj;
}

function promisify(callback, receiver) {
    return makeNodePromisified(callback, receiver, void 0, callback);
}

Promise.promisify = function Promise$Promisify(fn, receiver) {
    if (typeof fn !== "function") {
        throw new TypeError("fn must be a function");
    }
    if (isPromisified(fn)) {
        return fn;
    }
    return promisify(fn, arguments.length < 2 ? THIS : receiver);
};

Promise.promisifyAll = function Promise$PromisifyAll(target, options) {
    if (typeof target !== "function" && typeof target !== "object") {
        throw new TypeError("the target of promisifyAll must be an object or a function");
    }
    options = Object(options);
    var suffix = options.suffix;
    if (typeof suffix !== "string") suffix = defaultSuffix;
    var filter = options.filter;
    if (typeof filter !== "function") filter = defaultFilter;
    var promisifier = options.promisifier;
    if (typeof promisifier !== "function") promisifier = makeNodePromisified;

    if (!util.isIdentifier(suffix)) {
        throw new RangeError("suffix must be a valid identifier");
    }

    var keys = util.inheritedDataKeys(target, {includeHidden: true});
    for (var i = 0; i < keys.length; ++i) {
        var value = target[keys[i]];
        if (keys[i] !== "constructor" &&
            util.isClass(value)) {
            promisifyAll(value.prototype, suffix, filter, promisifier);
            promisifyAll(value, suffix, filter, promisifier);
        }
    }

    return promisifyAll(target, suffix, filter, promisifier);
};
};


},{"./errors":20,"./promise_resolver.js":32,"./util.js":45}],34:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, PromiseArray, cast) {
var util = require("./util.js");
var apiRejection = require("./errors_api_rejection")(Promise);
var isObject = util.isObject;
var es5 = require("./es5.js");

function PropertiesPromiseArray(obj) {
    var keys = es5.keys(obj);
    var len = keys.length;
    var values = new Array(len * 2);
    for (var i = 0; i < len; ++i) {
        var key = keys[i];
        values[i] = obj[key];
        values[i + len] = key;
    }
    this.constructor$(values);
}
util.inherits(PropertiesPromiseArray, PromiseArray);

PropertiesPromiseArray.prototype._init =
function PropertiesPromiseArray$_init() {
    this._init$(void 0, -3) ;
};

PropertiesPromiseArray.prototype._promiseFulfilled =
function PropertiesPromiseArray$_promiseFulfilled(value, index) {
    if (this._isResolved()) return;
    this._values[index] = value;
    var totalResolved = ++this._totalResolved;
    if (totalResolved >= this._length) {
        var val = {};
        var keyOffset = this.length();
        for (var i = 0, len = this.length(); i < len; ++i) {
            val[this._values[i + keyOffset]] = this._values[i];
        }
        this._resolve(val);
    }
};

PropertiesPromiseArray.prototype._promiseProgressed =
function PropertiesPromiseArray$_promiseProgressed(value, index) {
    if (this._isResolved()) return;

    this._promise._progress({
        key: this._values[index + this.length()],
        value: value
    });
};

PropertiesPromiseArray.prototype.shouldCopyValues =
function PropertiesPromiseArray$_shouldCopyValues() {
    return false;
};

PropertiesPromiseArray.prototype.getActualLength =
function PropertiesPromiseArray$getActualLength(len) {
    return len >> 1;
};

function Promise$_Props(promises) {
    var ret;
    var castValue = cast(promises, void 0);

    if (!isObject(castValue)) {
        return apiRejection("cannot await properties of a non-object");
    } else if (castValue instanceof Promise) {
        ret = castValue._then(Promise.props, void 0, void 0, void 0, void 0);
    } else {
        ret = new PropertiesPromiseArray(castValue).promise();
    }

    if (castValue instanceof Promise) {
        ret._propagateFrom(castValue, 4);
    }
    return ret;
}

Promise.prototype.props = function Promise$props() {
    return Promise$_Props(this);
};

Promise.props = function Promise$Props(promises) {
    return Promise$_Props(promises);
};
};

},{"./errors_api_rejection":21,"./es5.js":22,"./util.js":45}],35:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
function arrayCopy(src, srcIndex, dst, dstIndex, len) {
    for (var j = 0; j < len; ++j) {
        dst[j + dstIndex] = src[j + srcIndex];
    }
}

function Queue(capacity) {
    this._capacity = capacity;
    this._length = 0;
    this._front = 0;
    this._makeCapacity();
}

Queue.prototype._willBeOverCapacity =
function Queue$_willBeOverCapacity(size) {
    return this._capacity < size;
};

Queue.prototype._pushOne = function Queue$_pushOne(arg) {
    var length = this.length();
    this._checkCapacity(length + 1);
    var i = (this._front + length) & (this._capacity - 1);
    this[i] = arg;
    this._length = length + 1;
};

Queue.prototype.push = function Queue$push(fn, receiver, arg) {
    var length = this.length() + 3;
    if (this._willBeOverCapacity(length)) {
        this._pushOne(fn);
        this._pushOne(receiver);
        this._pushOne(arg);
        return;
    }
    var j = this._front + length - 3;
    this._checkCapacity(length);
    var wrapMask = this._capacity - 1;
    this[(j + 0) & wrapMask] = fn;
    this[(j + 1) & wrapMask] = receiver;
    this[(j + 2) & wrapMask] = arg;
    this._length = length;
};

Queue.prototype.shift = function Queue$shift() {
    var front = this._front,
        ret = this[front];

    this[front] = void 0;
    this._front = (front + 1) & (this._capacity - 1);
    this._length--;
    return ret;
};

Queue.prototype.length = function Queue$length() {
    return this._length;
};

Queue.prototype._makeCapacity = function Queue$_makeCapacity() {
    var len = this._capacity;
    for (var i = 0; i < len; ++i) {
        this[i] = void 0;
    }
};

Queue.prototype._checkCapacity = function Queue$_checkCapacity(size) {
    if (this._capacity < size) {
        this._resizeTo(this._capacity << 3);
    }
};

Queue.prototype._resizeTo = function Queue$_resizeTo(capacity) {
    var oldFront = this._front;
    var oldCapacity = this._capacity;
    var oldQueue = new Array(oldCapacity);
    var length = this.length();

    arrayCopy(this, 0, oldQueue, 0, oldCapacity);
    this._capacity = capacity;
    this._makeCapacity();
    this._front = 0;
    if (oldFront + length <= oldCapacity) {
        arrayCopy(oldQueue, oldFront, this, 0, length);
    } else {        var lengthBeforeWrapping =
            length - ((oldFront + length) & (oldCapacity - 1));

        arrayCopy(oldQueue, oldFront, this, 0, lengthBeforeWrapping);
        arrayCopy(oldQueue, 0, this, lengthBeforeWrapping,
                    length - lengthBeforeWrapping);
    }
};

module.exports = Queue;

},{}],36:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL, cast) {
var apiRejection = require("./errors_api_rejection.js")(Promise);
var isArray = require("./util.js").isArray;

var raceLater = function Promise$_raceLater(promise) {
    return promise.then(function(array) {
        return Promise$_Race(array, promise);
    });
};

var hasOwn = {}.hasOwnProperty;
function Promise$_Race(promises, parent) {
    var maybePromise = cast(promises, void 0);

    if (maybePromise instanceof Promise) {
        return raceLater(maybePromise);
    } else if (!isArray(promises)) {
        return apiRejection("expecting an array, a promise or a thenable");
    }

    var ret = new Promise(INTERNAL);
    if (parent !== void 0) {
        ret._propagateFrom(parent, 7);
    } else {
        ret._setTrace(void 0);
    }
    var fulfill = ret._fulfill;
    var reject = ret._reject;
    for (var i = 0, len = promises.length; i < len; ++i) {
        var val = promises[i];

        if (val === void 0 && !(hasOwn.call(promises, i))) {
            continue;
        }

        Promise.cast(val)._then(fulfill, reject, void 0, ret, null);
    }
    return ret;
}

Promise.race = function Promise$Race(promises) {
    return Promise$_Race(promises, void 0);
};

Promise.prototype.race = function Promise$race() {
    return Promise$_Race(this, void 0);
};

};

},{"./errors_api_rejection.js":21,"./util.js":45}],37:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, PromiseArray, apiRejection, cast, INTERNAL) {
var util = require("./util.js");
var tryCatch4 = util.tryCatch4;
var tryCatch3 = util.tryCatch3;
var errorObj = util.errorObj;
function ReductionPromiseArray(promises, fn, accum, _each) {
    this.constructor$(promises);
    this._preservedValues = _each === INTERNAL ? [] : null;
    this._zerothIsAccum = (accum === void 0);
    this._gotAccum = false;
    this._reducingIndex = (this._zerothIsAccum ? 1 : 0);
    this._valuesPhase = undefined;

    var maybePromise = cast(accum, void 0);
    var rejected = false;
    var isPromise = maybePromise instanceof Promise;
    if (isPromise) {
        if (maybePromise.isPending()) {
            maybePromise._proxyPromiseArray(this, -1);
        } else if (maybePromise.isFulfilled()) {
            accum = maybePromise.value();
            this._gotAccum = true;
        } else {
            maybePromise._unsetRejectionIsUnhandled();
            this._reject(maybePromise.reason());
            rejected = true;
        }
    }
    if (!(isPromise || this._zerothIsAccum)) this._gotAccum = true;
    this._callback = fn;
    this._accum = accum;
    if (!rejected) this._init$(void 0, -5);
}
util.inherits(ReductionPromiseArray, PromiseArray);

ReductionPromiseArray.prototype._init =
function ReductionPromiseArray$_init() {};

ReductionPromiseArray.prototype._resolveEmptyArray =
function ReductionPromiseArray$_resolveEmptyArray() {
    if (this._gotAccum || this._zerothIsAccum) {
        this._resolve(this._preservedValues !== null
                        ? [] : this._accum);
    }
};

ReductionPromiseArray.prototype._promiseFulfilled =
function ReductionPromiseArray$_promiseFulfilled(value, index) {
    var values = this._values;
    if (values === null) return;
    var length = this.length();
    var preservedValues = this._preservedValues;
    var isEach = preservedValues !== null;
    var gotAccum = this._gotAccum;
    var valuesPhase = this._valuesPhase;
    var valuesPhaseIndex;
    if (!valuesPhase) {
        valuesPhase = this._valuesPhase = Array(length);
        for (valuesPhaseIndex=0; valuesPhaseIndex<length; ++valuesPhaseIndex) {
            valuesPhase[valuesPhaseIndex] = 0;
        }
    }
    valuesPhaseIndex = valuesPhase[index];

    if (index === 0 && this._zerothIsAccum) {
        if (!gotAccum) {
            this._accum = value;
            this._gotAccum = gotAccum = true;
        }
        valuesPhase[index] = ((valuesPhaseIndex === 0)
            ? 1 : 2);
    } else if (index === -1) {
        if (!gotAccum) {
            this._accum = value;
            this._gotAccum = gotAccum = true;
        }
    } else {
        if (valuesPhaseIndex === 0) {
            valuesPhase[index] = 1;
        }
        else {
            valuesPhase[index] = 2;
            if (gotAccum) {
                this._accum = value;
            }
        }
    }
    if (!gotAccum) return;

    var callback = this._callback;
    var receiver = this._promise._boundTo;
    var ret;

    for (var i = this._reducingIndex; i < length; ++i) {
        valuesPhaseIndex = valuesPhase[i];
        if (valuesPhaseIndex === 2) {
            this._reducingIndex = i + 1;
            continue;
        }
        if (valuesPhaseIndex !== 1) return;

        value = values[i];
        if (value instanceof Promise) {
            if (value.isFulfilled()) {
                value = value._settledValue;
            } else if (value.isPending()) {
                return;
            } else {
                value._unsetRejectionIsUnhandled();
                return this._reject(value.reason());
            }
        }

        if (isEach) {
            preservedValues.push(value);
            ret = tryCatch3(callback, receiver, value, i, length);
        }
        else {
            ret = tryCatch4(callback, receiver, this._accum, value, i, length);
        }

        if (ret === errorObj) return this._reject(ret.e);

        var maybePromise = cast(ret, void 0);
        if (maybePromise instanceof Promise) {
            if (maybePromise.isPending()) {
                valuesPhase[i] = 4;
                return maybePromise._proxyPromiseArray(this, i);
            } else if (maybePromise.isFulfilled()) {
                ret = maybePromise.value();
            } else {
                maybePromise._unsetRejectionIsUnhandled();
                return this._reject(maybePromise.reason());
            }
        }

        this._reducingIndex = i + 1;
        this._accum = ret;
    }

    if (this._reducingIndex < length) return;
    this._resolve(isEach ? preservedValues : this._accum);
};

function reduce(promises, fn, initialValue, _each) {
    if (typeof fn !== "function") return apiRejection("fn must be a function");
    var array = new ReductionPromiseArray(promises, fn, initialValue, _each);
    return array.promise();
}

Promise.prototype.reduce = function Promise$reduce(fn, initialValue) {
    return reduce(this, fn, initialValue, null);
};

Promise.reduce = function Promise$Reduce(promises, fn, initialValue, _each) {
    return reduce(promises, fn, initialValue, _each);
};
};

},{"./util.js":45}],38:[function(require,module,exports){
(function (process){/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var schedule;
var _MutationObserver;
if (typeof process === "object" && typeof process.version === "string") {
    schedule = function Promise$_Scheduler(fn) {
        process.nextTick(fn);
    };
}
else if ((typeof MutationObserver !== "undefined" &&
         (_MutationObserver = MutationObserver)) ||
         (typeof WebKitMutationObserver !== "undefined" &&
         (_MutationObserver = WebKitMutationObserver))) {
    schedule = (function() {
        var div = document.createElement("div");
        var queuedFn = void 0;
        var observer = new _MutationObserver(
            function Promise$_Scheduler() {
                var fn = queuedFn;
                queuedFn = void 0;
                fn();
            }
       );
        observer.observe(div, {
            attributes: true
        });
        return function Promise$_Scheduler(fn) {
            queuedFn = fn;
            div.classList.toggle("foo");
        };

    })();
}
else if (typeof setTimeout !== "undefined") {
    schedule = function Promise$_Scheduler(fn) {
        setTimeout(fn, 0);
    };
}
else throw new Error("no async scheduler available");
module.exports = schedule;
}).call(this,require("/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js"))
},{"/home/buildslave/buildslave/package-dist/build/build/external/browserify_3.24.13/node_modules/packed-browserify/node_modules/browserify/node_modules/insert-module-globals/node_modules/process/browser.js":3}],39:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports =
    function(Promise, PromiseArray) {
var PromiseInspection = Promise.PromiseInspection;
var util = require("./util.js");

function SettledPromiseArray(values) {
    this.constructor$(values);
}
util.inherits(SettledPromiseArray, PromiseArray);

SettledPromiseArray.prototype._promiseResolved =
function SettledPromiseArray$_promiseResolved(index, inspection) {
    this._values[index] = inspection;
    var totalResolved = ++this._totalResolved;
    if (totalResolved >= this._length) {
        this._resolve(this._values);
    }
};

SettledPromiseArray.prototype._promiseFulfilled =
function SettledPromiseArray$_promiseFulfilled(value, index) {
    if (this._isResolved()) return;
    var ret = new PromiseInspection();
    ret._bitField = 268435456;
    ret._settledValue = value;
    this._promiseResolved(index, ret);
};
SettledPromiseArray.prototype._promiseRejected =
function SettledPromiseArray$_promiseRejected(reason, index) {
    if (this._isResolved()) return;
    var ret = new PromiseInspection();
    ret._bitField = 134217728;
    ret._settledValue = reason;
    this._promiseResolved(index, ret);
};

Promise.settle = function Promise$Settle(promises) {
    return new SettledPromiseArray(promises).promise();
};

Promise.prototype.settle = function Promise$settle() {
    return new SettledPromiseArray(this).promise();
};
};

},{"./util.js":45}],40:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports =
function(Promise, PromiseArray, apiRejection) {
var util = require("./util.js");
var RangeError = require("./errors.js").RangeError;
var AggregateError = require("./errors.js").AggregateError;
var isArray = util.isArray;


function SomePromiseArray(values) {
    this.constructor$(values);
    this._howMany = 0;
    this._unwrap = false;
    this._initialized = false;
}
util.inherits(SomePromiseArray, PromiseArray);

SomePromiseArray.prototype._init = function SomePromiseArray$_init() {
    if (!this._initialized) {
        return;
    }
    if (this._howMany === 0) {
        this._resolve([]);
        return;
    }
    this._init$(void 0, -5);
    var isArrayResolved = isArray(this._values);
    if (!this._isResolved() &&
        isArrayResolved &&
        this._howMany > this._canPossiblyFulfill()) {
        this._reject(this._getRangeError(this.length()));
    }
};

SomePromiseArray.prototype.init = function SomePromiseArray$init() {
    this._initialized = true;
    this._init();
};

SomePromiseArray.prototype.setUnwrap = function SomePromiseArray$setUnwrap() {
    this._unwrap = true;
};

SomePromiseArray.prototype.howMany = function SomePromiseArray$howMany() {
    return this._howMany;
};

SomePromiseArray.prototype.setHowMany =
function SomePromiseArray$setHowMany(count) {
    if (this._isResolved()) return;
    this._howMany = count;
};

SomePromiseArray.prototype._promiseFulfilled =
function SomePromiseArray$_promiseFulfilled(value) {
    if (this._isResolved()) return;
    this._addFulfilled(value);
    if (this._fulfilled() === this.howMany()) {
        this._values.length = this.howMany();
        if (this.howMany() === 1 && this._unwrap) {
            this._resolve(this._values[0]);
        } else {
            this._resolve(this._values);
        }
    }

};
SomePromiseArray.prototype._promiseRejected =
function SomePromiseArray$_promiseRejected(reason) {
    if (this._isResolved()) return;
    this._addRejected(reason);
    if (this.howMany() > this._canPossiblyFulfill()) {
        var e = new AggregateError();
        for (var i = this.length(); i < this._values.length; ++i) {
            e.push(this._values[i]);
        }
        this._reject(e);
    }
};

SomePromiseArray.prototype._fulfilled = function SomePromiseArray$_fulfilled() {
    return this._totalResolved;
};

SomePromiseArray.prototype._rejected = function SomePromiseArray$_rejected() {
    return this._values.length - this.length();
};

SomePromiseArray.prototype._addRejected =
function SomePromiseArray$_addRejected(reason) {
    this._values.push(reason);
};

SomePromiseArray.prototype._addFulfilled =
function SomePromiseArray$_addFulfilled(value) {
    this._values[this._totalResolved++] = value;
};

SomePromiseArray.prototype._canPossiblyFulfill =
function SomePromiseArray$_canPossiblyFulfill() {
    return this.length() - this._rejected();
};

SomePromiseArray.prototype._getRangeError =
function SomePromiseArray$_getRangeError(count) {
    var message = "Input array must contain at least " +
            this._howMany + " items but contains only " + count + " items";
    return new RangeError(message);
};

SomePromiseArray.prototype._resolveEmptyArray =
function SomePromiseArray$_resolveEmptyArray() {
    this._reject(this._getRangeError(0));
};

function Promise$_Some(promises, howMany) {
    if ((howMany | 0) !== howMany || howMany < 0) {
        return apiRejection("expecting a positive integer");
    }
    var ret = new SomePromiseArray(promises);
    var promise = ret.promise();
    if (promise.isRejected()) {
        return promise;
    }
    ret.setHowMany(howMany);
    ret.init();
    return promise;
}

Promise.some = function Promise$Some(promises, howMany) {
    return Promise$_Some(promises, howMany);
};

Promise.prototype.some = function Promise$some(howMany) {
    return Promise$_Some(this, howMany);
};

Promise._SomePromiseArray = SomePromiseArray;
};

},{"./errors.js":20,"./util.js":45}],41:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise) {
function PromiseInspection(promise) {
    if (promise !== void 0) {
        this._bitField = promise._bitField;
        this._settledValue = promise.isResolved()
            ? promise._settledValue
            : void 0;
    }
    else {
        this._bitField = 0;
        this._settledValue = void 0;
    }
}

PromiseInspection.prototype.isFulfilled =
Promise.prototype.isFulfilled = function Promise$isFulfilled() {
    return (this._bitField & 268435456) > 0;
};

PromiseInspection.prototype.isRejected =
Promise.prototype.isRejected = function Promise$isRejected() {
    return (this._bitField & 134217728) > 0;
};

PromiseInspection.prototype.isPending =
Promise.prototype.isPending = function Promise$isPending() {
    return (this._bitField & 402653184) === 0;
};

PromiseInspection.prototype.value =
Promise.prototype.value = function Promise$value() {
    if (!this.isFulfilled()) {
        throw new TypeError("cannot get fulfillment value of a non-fulfilled promise");
    }
    return this._settledValue;
};

PromiseInspection.prototype.error =
PromiseInspection.prototype.reason =
Promise.prototype.reason = function Promise$reason() {
    if (!this.isRejected()) {
        throw new TypeError("cannot get rejection reason of a non-rejected promise");
    }
    return this._settledValue;
};

PromiseInspection.prototype.isResolved =
Promise.prototype.isResolved = function Promise$isResolved() {
    return (this._bitField & 402653184) > 0;
};

Promise.PromiseInspection = PromiseInspection;
};

},{}],42:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function(Promise, INTERNAL) {
var util = require("./util.js");
var canAttach = require("./errors.js").canAttach;
var errorObj = util.errorObj;
var isObject = util.isObject;

function getThen(obj) {
    try {
        return obj.then;
    }
    catch(e) {
        errorObj.e = e;
        return errorObj;
    }
}

function Promise$_Cast(obj, originalPromise) {
    if (isObject(obj)) {
        if (obj instanceof Promise) {
            return obj;
        }
        else if (isAnyBluebirdPromise(obj)) {
            var ret = new Promise(INTERNAL);
            ret._setTrace(void 0);
            obj._then(
                ret._fulfillUnchecked,
                ret._rejectUncheckedCheckError,
                ret._progressUnchecked,
                ret,
                null
            );
            ret._setFollowing();
            return ret;
        }
        var then = getThen(obj);
        if (then === errorObj) {
            if (originalPromise !== void 0 && canAttach(then.e)) {
                originalPromise._attachExtraTrace(then.e);
            }
            return Promise.reject(then.e);
        } else if (typeof then === "function") {
            return Promise$_doThenable(obj, then, originalPromise);
        }
    }
    return obj;
}

var hasProp = {}.hasOwnProperty;
function isAnyBluebirdPromise(obj) {
    return hasProp.call(obj, "_promise0");
}

function Promise$_doThenable(x, then, originalPromise) {
    var resolver = Promise.defer();
    var called = false;
    try {
        then.call(
            x,
            Promise$_resolveFromThenable,
            Promise$_rejectFromThenable,
            Promise$_progressFromThenable
        );
    } catch(e) {
        if (!called) {
            called = true;
            var trace = canAttach(e) ? e : new Error(e + "");
            if (originalPromise !== void 0) {
                originalPromise._attachExtraTrace(trace);
            }
            resolver.promise._reject(e, trace);
        }
    }
    return resolver.promise;

    function Promise$_resolveFromThenable(y) {
        if (called) return;
        called = true;

        if (x === y) {
            var e = Promise._makeSelfResolutionError();
            if (originalPromise !== void 0) {
                originalPromise._attachExtraTrace(e);
            }
            resolver.promise._reject(e, void 0);
            return;
        }
        resolver.resolve(y);
    }

    function Promise$_rejectFromThenable(r) {
        if (called) return;
        called = true;
        var trace = canAttach(r) ? r : new Error(r + "");
        if (originalPromise !== void 0) {
            originalPromise._attachExtraTrace(trace);
        }
        resolver.promise._reject(r, trace);
    }

    function Promise$_progressFromThenable(v) {
        if (called) return;
        var promise = resolver.promise;
        if (typeof promise._progress === "function") {
            promise._progress(v);
        }
    }
}

return Promise$_Cast;
};

},{"./errors.js":20,"./util.js":45}],43:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var _setTimeout = function(fn, ms) {
    var len = arguments.length;
    var arg0 = arguments[2];
    var arg1 = arguments[3];
    var arg2 = len >= 5 ? arguments[4] : void 0;
    setTimeout(function() {
        fn(arg0, arg1, arg2);
    }, ms|0);
};

module.exports = function(Promise, INTERNAL, cast) {
var util = require("./util.js");
var errors = require("./errors.js");
var apiRejection = require("./errors_api_rejection")(Promise);
var TimeoutError = Promise.TimeoutError;

var afterTimeout = function Promise$_afterTimeout(promise, message, ms) {
    if (!promise.isPending()) return;
    if (typeof message !== "string") {
        message = "operation timed out after" + " " + ms + " ms"
    }
    var err = new TimeoutError(message);
    errors.markAsOriginatingFromRejection(err);
    promise._attachExtraTrace(err);
    promise._cancel(err);
};

var afterDelay = function Promise$_afterDelay(value, promise) {
    promise._fulfill(value);
};

var delay = Promise.delay = function Promise$Delay(value, ms) {
    if (ms === void 0) {
        ms = value;
        value = void 0;
    }
    ms = +ms;
    var maybePromise = cast(value, void 0);
    var promise = new Promise(INTERNAL);

    if (maybePromise instanceof Promise) {
        promise._propagateFrom(maybePromise, 7);
        promise._follow(maybePromise);
        return promise.then(function(value) {
            return Promise.delay(value, ms);
        });
    } else {
        promise._setTrace(void 0);
        _setTimeout(afterDelay, ms, value, promise);
    }
    return promise;
};

Promise.prototype.delay = function Promise$delay(ms) {
    return delay(this, ms);
};

Promise.prototype.timeout = function Promise$timeout(ms, message) {
    ms = +ms;

    var ret = new Promise(INTERNAL);
    ret._propagateFrom(this, 7);
    ret._follow(this);
    _setTimeout(afterTimeout, ms, ret, message, ms);
    return ret.cancellable();
};

};

},{"./errors.js":20,"./errors_api_rejection":21,"./util.js":45}],44:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
module.exports = function (Promise, apiRejection, cast) {
    var TypeError = require("./errors.js").TypeError;
    var inherits = require("./util.js").inherits;
    var PromiseInspection = Promise.PromiseInspection;

    function inspectionMapper(inspections) {
        var len = inspections.length;
        for (var i = 0; i < len; ++i) {
            var inspection = inspections[i];
            if (inspection.isRejected()) {
                return Promise.reject(inspection.error());
            }
            inspections[i] = inspection.value();
        }
        return inspections;
    }

    function thrower(e) {
        setTimeout(function(){throw e;}, 0);
    }

    function castPreservingDisposable(thenable) {
        var maybePromise = cast(thenable, void 0);
        if (maybePromise !== thenable &&
            typeof thenable._isDisposable === "function" &&
            typeof thenable._getDisposer === "function" &&
            thenable._isDisposable()) {
            maybePromise._setDisposable(thenable._getDisposer());
        }
        return maybePromise;
    }
    function dispose(resources, inspection) {
        var i = 0;
        var len = resources.length;
        var ret = Promise.defer();
        function iterator() {
            if (i >= len) return ret.resolve();
            var maybePromise = castPreservingDisposable(resources[i++]);
            if (maybePromise instanceof Promise &&
                maybePromise._isDisposable()) {
                try {
                    maybePromise = cast(maybePromise._getDisposer()
                                        .tryDispose(inspection), void 0);
                } catch (e) {
                    return thrower(e);
                }
                if (maybePromise instanceof Promise) {
                    return maybePromise._then(iterator, thrower,
                                              null, null, null);
                }
            }
            iterator();
        }
        iterator();
        return ret.promise;
    }

    function disposerSuccess(value) {
        var inspection = new PromiseInspection();
        inspection._settledValue = value;
        inspection._bitField = 268435456;
        return dispose(this, inspection).thenReturn(value);
    }

    function disposerFail(reason) {
        var inspection = new PromiseInspection();
        inspection._settledValue = reason;
        inspection._bitField = 134217728;
        return dispose(this, inspection).thenThrow(reason);
    }

    function Disposer(data, promise) {
        this._data = data;
        this._promise = promise;
    }

    Disposer.prototype.data = function Disposer$data() {
        return this._data;
    };

    Disposer.prototype.promise = function Disposer$promise() {
        return this._promise;
    };

    Disposer.prototype.resource = function Disposer$resource() {
        if (this.promise().isFulfilled()) {
            return this.promise().value();
        }
        return null;
    };

    Disposer.prototype.tryDispose = function(inspection) {
        var resource = this.resource();
        var ret = resource !== null
            ? this.doDispose(resource, inspection) : null;
        this._promise._unsetDisposable();
        this._data = this._promise = null;
        return ret;
    };

    Disposer.isDisposer = function Disposer$isDisposer(d) {
        return (d != null &&
                typeof d.resource === "function" &&
                typeof d.tryDispose === "function");
    };

    function FunctionDisposer(fn, promise) {
        this.constructor$(fn, promise);
    }
    inherits(FunctionDisposer, Disposer);

    FunctionDisposer.prototype.doDispose = function (resource, inspection) {
        var fn = this.data();
        return fn.call(resource, resource, inspection);
    };

    Promise.using = function Promise$using() {
        var len = arguments.length;
        if (len < 2) return apiRejection(
                        "you must pass at least 2 arguments to Promise.using");
        var fn = arguments[len - 1];
        if (typeof fn !== "function") return apiRejection("fn must be a function");
        len--;
        var resources = new Array(len);
        for (var i = 0; i < len; ++i) {
            var resource = arguments[i];
            if (Disposer.isDisposer(resource)) {
                var disposer = resource;
                resource = resource.promise();
                resource._setDisposable(disposer);
            }
            resources[i] = resource;
        }

        return Promise.settle(resources)
            .then(inspectionMapper)
            .spread(fn)
            ._then(disposerSuccess, disposerFail, void 0, resources, void 0);
    };

    Promise.prototype._setDisposable =
    function Promise$_setDisposable(disposer) {
        this._bitField = this._bitField | 262144;
        this._disposer = disposer;
    };

    Promise.prototype._isDisposable = function Promise$_isDisposable() {
        return (this._bitField & 262144) > 0;
    };

    Promise.prototype._getDisposer = function Promise$_getDisposer() {
        return this._disposer;
    };

    Promise.prototype._unsetDisposable = function Promise$_unsetDisposable() {
        this._bitField = this._bitField & (~262144);
        this._disposer = void 0;
    };

    Promise.prototype.disposer = function Promise$disposer(fn) {
        if (typeof fn === "function") {
            return new FunctionDisposer(fn, this);
        }
        throw new TypeError();
    };

};

},{"./errors.js":20,"./util.js":45}],45:[function(require,module,exports){
/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var es5 = require("./es5.js");
var haveGetters = (function(){
    try {
        var o = {};
        es5.defineProperty(o, "f", {
            get: function () {
                return 3;
            }
        });
        return o.f === 3;
    }
    catch (e) {
        return false;
    }

})();
var canEvaluate = typeof navigator == "undefined";
var errorObj = {e: {}};
function tryCatch1(fn, receiver, arg) {
    try { return fn.call(receiver, arg); }
    catch (e) {
        errorObj.e = e;
        return errorObj;
    }
}

function tryCatch2(fn, receiver, arg, arg2) {
    try { return fn.call(receiver, arg, arg2); }
    catch (e) {
        errorObj.e = e;
        return errorObj;
    }
}

function tryCatch3(fn, receiver, arg, arg2, arg3) {
    try { return fn.call(receiver, arg, arg2, arg3); }
    catch (e) {
        errorObj.e = e;
        return errorObj;
    }
}

function tryCatch4(fn, receiver, arg, arg2, arg3, arg4) {
    try { return fn.call(receiver, arg, arg2, arg3, arg4); }
    catch (e) {
        errorObj.e = e;
        return errorObj;
    }
}

function tryCatchApply(fn, args, receiver) {
    try { return fn.apply(receiver, args); }
    catch (e) {
        errorObj.e = e;
        return errorObj;
    }
}

var inherits = function(Child, Parent) {
    var hasProp = {}.hasOwnProperty;

    function T() {
        this.constructor = Child;
        this.constructor$ = Parent;
        for (var propertyName in Parent.prototype) {
            if (hasProp.call(Parent.prototype, propertyName) &&
                propertyName.charAt(propertyName.length-1) !== "$"
           ) {
                this[propertyName + "$"] = Parent.prototype[propertyName];
            }
        }
    }
    T.prototype = Parent.prototype;
    Child.prototype = new T();
    return Child.prototype;
};

function asString(val) {
    return typeof val === "string" ? val : ("" + val);
}

function isPrimitive(val) {
    return val == null || val === true || val === false ||
        typeof val === "string" || typeof val === "number";

}

function isObject(value) {
    return !isPrimitive(value);
}

function maybeWrapAsError(maybeError) {
    if (!isPrimitive(maybeError)) return maybeError;

    return new Error(asString(maybeError));
}

function withAppended(target, appendee) {
    var len = target.length;
    var ret = new Array(len + 1);
    var i;
    for (i = 0; i < len; ++i) {
        ret[i] = target[i];
    }
    ret[i] = appendee;
    return ret;
}

function getDataPropertyOrDefault(obj, key, defaultValue) {
    if (es5.isES5) {
        var desc = Object.getOwnPropertyDescriptor(obj, key);
        if (desc != null) {
            return desc.get == null && desc.set == null
                    ? desc.value
                    : defaultValue;
        }
    } else {
        return {}.hasOwnProperty.call(obj, key) ? obj[key] : void 0;
    }
}

function notEnumerableProp(obj, name, value) {
    if (isPrimitive(obj)) return obj;
    var descriptor = {
        value: value,
        configurable: true,
        enumerable: false,
        writable: true
    };
    es5.defineProperty(obj, name, descriptor);
    return obj;
}


var wrapsPrimitiveReceiver = (function() {
    return this !== "string";
}).call("string");

function thrower(r) {
    throw r;
}

var inheritedDataKeys = (function() {
    if (es5.isES5) {
        return function(obj, opts) {
            var ret = [];
            var visitedKeys = Object.create(null);
            var getKeys = Object(opts).includeHidden
                ? Object.getOwnPropertyNames
                : Object.keys;
            while (obj != null) {
                var keys;
                try {
                    keys = getKeys(obj);
                } catch (e) {
                    return ret;
                }
                for (var i = 0; i < keys.length; ++i) {
                    var key = keys[i];
                    if (visitedKeys[key]) continue;
                    visitedKeys[key] = true;
                    var desc = Object.getOwnPropertyDescriptor(obj, key);
                    if (desc != null && desc.get == null && desc.set == null) {
                        ret.push(key);
                    }
                }
                obj = es5.getPrototypeOf(obj);
            }
            return ret;
        };
    } else {
        return function(obj) {
            var ret = [];
            /*jshint forin:false */
            for (var key in obj) {
                ret.push(key);
            }
            return ret;
        };
    }

})();

function isClass(fn) {
    try {
        if (typeof fn === "function") {
            var keys = es5.keys(fn.prototype);
            return keys.length > 0 &&
                   !(keys.length === 1 && keys[0] === "constructor");
        }
        return false;
    } catch (e) {
        return false;
    }
}

function toFastProperties(obj) {
    /*jshint -W027*/
    function f() {}
    f.prototype = obj;
    return f;
    eval(obj);
}

var rident = /^[a-z$_][a-z$_0-9]*$/i;
function isIdentifier(str) {
    return rident.test(str);
}

function filledRange(count, prefix, suffix) {
    var ret = new Array(count);
    for(var i = 0; i < count; ++i) {
        ret[i] = prefix + i + suffix;
    }
    return ret;
}

var ret = {
    isClass: isClass,
    isIdentifier: isIdentifier,
    inheritedDataKeys: inheritedDataKeys,
    getDataPropertyOrDefault: getDataPropertyOrDefault,
    thrower: thrower,
    isArray: es5.isArray,
    haveGetters: haveGetters,
    notEnumerableProp: notEnumerableProp,
    isPrimitive: isPrimitive,
    isObject: isObject,
    canEvaluate: canEvaluate,
    errorObj: errorObj,
    tryCatch1: tryCatch1,
    tryCatch2: tryCatch2,
    tryCatch3: tryCatch3,
    tryCatch4: tryCatch4,
    tryCatchApply: tryCatchApply,
    inherits: inherits,
    withAppended: withAppended,
    asString: asString,
    maybeWrapAsError: maybeWrapAsError,
    wrapsPrimitiveReceiver: wrapsPrimitiveReceiver,
    toFastProperties: toFastProperties,
    filledRange: filledRange
};

module.exports = ret;

},{"./es5.js":22}],46:[function(require,module,exports){
// DO NOT EDIT
// Autogenerated by convert_protofile

module.exports = {
	VersionDummy: {
		Version: {
			V0_1: 1063369270,
			V0_2: 1915781601,
			V0_3: 1601562686,
			V0_4: 1074539808
		},
		
		Protocol: {
			PROTOBUF: 656407617,
			JSON: 2120839367
		}
	},
	
	Query: {
		QueryType: {
			START: 1,
			CONTINUE: 2,
			STOP: 3,
			NOREPLY_WAIT: 4
		},
		
		AssocPair: {}
	},
	
	Frame: {
		FrameType: {
			POS: 1,
			OPT: 2
		}
	},
	
	Backtrace: {},
	
	Response: {
		ResponseType: {
			SUCCESS_ATOM: 1,
			SUCCESS_SEQUENCE: 2,
			SUCCESS_PARTIAL: 3,
			WAIT_COMPLETE: 4,
			CLIENT_ERROR: 16,
			COMPILE_ERROR: 17,
			RUNTIME_ERROR: 18
		},
		
		ResponseNote: {
			SEQUENCE_FEED: 1,
			ATOM_FEED: 2,
			ORDER_BY_LIMIT_FEED: 3,
			UNIONED_FEED: 4,
			INCLUDES_STATES: 5
		}
	},
	
	Datum: {
		DatumType: {
			R_NULL: 1,
			R_BOOL: 2,
			R_NUM: 3,
			R_STR: 4,
			R_ARRAY: 5,
			R_OBJECT: 6,
			R_JSON: 7
		},
		
		AssocPair: {}
	},
	
	Term: {
		TermType: {
			DATUM: 1,
			MAKE_ARRAY: 2,
			MAKE_OBJ: 3,
			VAR: 10,
			JAVASCRIPT: 11,
			UUID: 169,
			HTTP: 153,
			ERROR: 12,
			IMPLICIT_VAR: 13,
			DB: 14,
			TABLE: 15,
			GET: 16,
			GET_ALL: 78,
			EQ: 17,
			NE: 18,
			LT: 19,
			LE: 20,
			GT: 21,
			GE: 22,
			NOT: 23,
			ADD: 24,
			SUB: 25,
			MUL: 26,
			DIV: 27,
			MOD: 28,
			APPEND: 29,
			PREPEND: 80,
			DIFFERENCE: 95,
			SET_INSERT: 88,
			SET_INTERSECTION: 89,
			SET_UNION: 90,
			SET_DIFFERENCE: 91,
			SLICE: 30,
			SKIP: 70,
			LIMIT: 71,
			OFFSETS_OF: 87,
			CONTAINS: 93,
			GET_FIELD: 31,
			KEYS: 94,
			OBJECT: 143,
			HAS_FIELDS: 32,
			WITH_FIELDS: 96,
			PLUCK: 33,
			WITHOUT: 34,
			MERGE: 35,
			BETWEEN_DEPRECATED: 36,
			BETWEEN: 182,
			REDUCE: 37,
			MAP: 38,
			FILTER: 39,
			CONCAT_MAP: 40,
			ORDER_BY: 41,
			DISTINCT: 42,
			COUNT: 43,
			IS_EMPTY: 86,
			UNION: 44,
			NTH: 45,
			BRACKET: 170,
			INNER_JOIN: 48,
			OUTER_JOIN: 49,
			EQ_JOIN: 50,
			ZIP: 72,
			RANGE: 173,
			INSERT_AT: 82,
			DELETE_AT: 83,
			CHANGE_AT: 84,
			SPLICE_AT: 85,
			COERCE_TO: 51,
			TYPE_OF: 52,
			UPDATE: 53,
			DELETE: 54,
			REPLACE: 55,
			INSERT: 56,
			DB_CREATE: 57,
			DB_DROP: 58,
			DB_LIST: 59,
			TABLE_CREATE: 60,
			TABLE_DROP: 61,
			TABLE_LIST: 62,
			CONFIG: 174,
			STATUS: 175,
			WAIT: 177,
			RECONFIGURE: 176,
			REBALANCE: 179,
			SYNC: 138,
			INDEX_CREATE: 75,
			INDEX_DROP: 76,
			INDEX_LIST: 77,
			INDEX_STATUS: 139,
			INDEX_WAIT: 140,
			INDEX_RENAME: 156,
			FUNCALL: 64,
			BRANCH: 65,
			OR: 66,
			AND: 67,
			FOR_EACH: 68,
			FUNC: 69,
			ASC: 73,
			DESC: 74,
			INFO: 79,
			MATCH: 97,
			UPCASE: 141,
			DOWNCASE: 142,
			SAMPLE: 81,
			DEFAULT: 92,
			JSON: 98,
			TO_JSON_STRING: 172,
			ISO8601: 99,
			TO_ISO8601: 100,
			EPOCH_TIME: 101,
			TO_EPOCH_TIME: 102,
			NOW: 103,
			IN_TIMEZONE: 104,
			DURING: 105,
			DATE: 106,
			TIME_OF_DAY: 126,
			TIMEZONE: 127,
			YEAR: 128,
			MONTH: 129,
			DAY: 130,
			DAY_OF_WEEK: 131,
			DAY_OF_YEAR: 132,
			HOURS: 133,
			MINUTES: 134,
			SECONDS: 135,
			TIME: 136,
			MONDAY: 107,
			TUESDAY: 108,
			WEDNESDAY: 109,
			THURSDAY: 110,
			FRIDAY: 111,
			SATURDAY: 112,
			SUNDAY: 113,
			JANUARY: 114,
			FEBRUARY: 115,
			MARCH: 116,
			APRIL: 117,
			MAY: 118,
			JUNE: 119,
			JULY: 120,
			AUGUST: 121,
			SEPTEMBER: 122,
			OCTOBER: 123,
			NOVEMBER: 124,
			DECEMBER: 125,
			LITERAL: 137,
			GROUP: 144,
			SUM: 145,
			AVG: 146,
			MIN: 147,
			MAX: 148,
			SPLIT: 149,
			UNGROUP: 150,
			RANDOM: 151,
			CHANGES: 152,
			ARGS: 154,
			BINARY: 155,
			GEOJSON: 157,
			TO_GEOJSON: 158,
			POINT: 159,
			LINE: 160,
			POLYGON: 161,
			DISTANCE: 162,
			INTERSECTS: 163,
			INCLUDES: 164,
			CIRCLE: 165,
			GET_INTERSECTING: 166,
			FILL: 167,
			GET_NEAREST: 168,
			POLYGON_SUB: 171,
			MINVAL: 180,
			MAXVAL: 181
		},
		
		AssocPair: {}
	}
}

},{}],"rethinkdb":[function(require,module,exports){
module.exports=require('l5DA7R');
},{}],"l5DA7R":[function(require,module,exports){
// Generated by CoffeeScript 1.7.1
var error, net, rethinkdb;

net = require('./net');

rethinkdb = require('./ast');

error = require('./errors');

rethinkdb.connect = net.connect;

rethinkdb.Error = error;

module.exports = rethinkdb;

},{"./ast":7,"./errors":9,"./net":10}],49:[function(require,module,exports){
(function (Buffer){// Generated by CoffeeScript 1.7.1
var convertPseudotype, err, mkAtom, mkErr, mkSeq, plural, recursivelyConvertPseudotype,
  __slice = [].slice;

err = require('./errors');

plural = function(number) {
  if (number === 1) {
    return "";
  } else {
    return "s";
  }
};

module.exports.ar = function(fun) {
  return function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    if (args.length !== fun.length) {
      throw new err.RqlDriverError("Expected " + fun.length + " argument" + (plural(fun.length)) + " but found " + args.length + ".");
    }
    return fun.apply(this, args);
  };
};

module.exports.varar = function(min, max, fun) {
  return function() {
    var args;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    if (((min != null) && args.length < min) || ((max != null) && args.length > max)) {
      if ((min != null) && (max == null)) {
        throw new err.RqlDriverError("Expected " + min + " or more arguments but found " + args.length + ".");
      }
      if ((max != null) && (min == null)) {
        throw new err.RqlDriverError("Expected " + max + " or fewer arguments but found " + args.length + ".");
      }
      throw new err.RqlDriverError("Expected between " + min + " and " + max + " arguments but found " + args.length + ".");
    }
    return fun.apply(this, args);
  };
};

module.exports.aropt = function(fun) {
  return function() {
    var args, expectedPosArgs, numPosArgs, perhapsOptDict;
    args = 1 <= arguments.length ? __slice.call(arguments, 0) : [];
    expectedPosArgs = fun.length - 1;
    perhapsOptDict = args[expectedPosArgs];
    if ((perhapsOptDict != null) && (Object.prototype.toString.call(perhapsOptDict) !== '[object Object]')) {
      perhapsOptDict = null;
    }
    numPosArgs = args.length - (perhapsOptDict != null ? 1 : 0);
    if (expectedPosArgs !== numPosArgs) {
      if (expectedPosArgs !== 1) {
        throw new err.RqlDriverError("Expected " + expectedPosArgs + " arguments (not including options) but found " + numPosArgs + ".");
      } else {
        throw new err.RqlDriverError("Expected " + expectedPosArgs + " argument (not including options) but found " + numPosArgs + ".");
      }
    }
    return fun.apply(this, args);
  };
};

module.exports.toArrayBuffer = function(node_buffer) {
  var arr, i, value, _i, _len;
  arr = new Uint8Array(new ArrayBuffer(node_buffer.length));
  for (i = _i = 0, _len = node_buffer.length; _i < _len; i = ++_i) {
    value = node_buffer[i];
    arr[i] = value;
  }
  return arr.buffer;
};

module.exports.fromCamelCase = function(token) {
  return token.replace(/[A-Z]/g, (function(_this) {
    return function(match) {
      return "_" + match.toLowerCase();
    };
  })(this));
};

module.exports.toCamelCase = function(token) {
  return token.replace(/_[a-z]/g, (function(_this) {
    return function(match) {
      return match[1].toUpperCase();
    };
  })(this));
};

convertPseudotype = function(obj, opts) {
  var i, _i, _len, _ref, _results;
  switch (obj['$reql_type$']) {
    case 'TIME':
      switch (opts.timeFormat) {
        case 'native':
        case void 0:
          if (obj['epoch_time'] == null) {
            throw new err.RqlDriverError("pseudo-type TIME " + obj + " object missing expected field 'epoch_time'.");
          }
          return new Date(obj['epoch_time'] * 1000);
        case 'raw':
          return obj;
        default:
          throw new err.RqlDriverError("Unknown timeFormat run option " + opts.timeFormat + ".");
      }
      break;
    case 'GROUPED_DATA':
      switch (opts.groupFormat) {
        case 'native':
        case void 0:
          _ref = obj['data'];
          _results = [];
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            i = _ref[_i];
            _results.push({
              group: i[0],
              reduction: i[1]
            });
          }
          return _results;
          break;
        case 'raw':
          return obj;
        default:
          throw new err.RqlDriverError("Unknown groupFormat run option " + opts.groupFormat + ".");
      }
      break;
    case 'BINARY':
      switch (opts.binaryFormat) {
        case 'native':
        case void 0:
          if (obj['data'] == null) {
            throw new err.RqlDriverError("pseudo-type BINARY object missing expected field 'data'.");
          }
          return new Buffer(obj['data'], 'base64');
        case 'raw':
          return obj;
        default:
          throw new err.RqlDriverError("Unknown binaryFormat run option " + opts.binaryFormat + ".");
      }
      break;
    default:
      return obj;
  }
};

recursivelyConvertPseudotype = function(obj, opts) {
  var i, key, value, _i, _len;
  if (Array.isArray(obj)) {
    for (i = _i = 0, _len = obj.length; _i < _len; i = ++_i) {
      value = obj[i];
      obj[i] = recursivelyConvertPseudotype(value, opts);
    }
  } else if (obj && typeof obj === 'object') {
    for (key in obj) {
      value = obj[key];
      obj[key] = recursivelyConvertPseudotype(value, opts);
    }
    obj = convertPseudotype(obj, opts);
  }
  return obj;
};

mkAtom = function(response, opts) {
  return recursivelyConvertPseudotype(response.r[0], opts);
};

mkSeq = function(response, opts) {
  return recursivelyConvertPseudotype(response.r, opts);
};

mkErr = function(ErrClass, response, root) {
  return new ErrClass(mkAtom(response), root, response.b);
};

module.exports.recursivelyConvertPseudotype = recursivelyConvertPseudotype;

module.exports.mkAtom = mkAtom;

module.exports.mkSeq = mkSeq;

module.exports.mkErr = mkErr;
}).call(this,require("buffer").Buffer)
},{"./errors":9,"buffer":4}]},{},[])