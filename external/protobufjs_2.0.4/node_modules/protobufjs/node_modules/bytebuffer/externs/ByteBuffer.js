/*
 * Copyright 2012 The Closure Compiler Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview Externs for ByteBuffer.js.
 * @see https://github.com/dcodeIO/ByteBuffer.js
 * @externs
 */

/**
 BEGIN_NODE_INCLUDE
 var ByteBuffer = require('bytebuffer');
 END_NODE_INCLUDE
 */

/**
 * @param {number=} capacity
 * @param {boolean=} littleEndian
 * @param {boolean=} sparse
 * @constructor
 */
function ByteBuffer(capacity, littleEndian, sparse) {};

/**
 * @type {?ArrayBuffer}
 */
ByteBuffer.prototype.array;

/**
 * @type {number}
 */
ByteBuffer.prototype.offset;

/**
 * @type {number}
 */
ByteBuffer.prototype.markedOffset;

/**
 * @type {number}
 */
ByteBuffer.prototype.length;

/**
 * @type {boolean}
 */
ByteBuffer.prototype.littleEndian;

/**
 * @type {string}
 * @const
 */
ByteBuffer.VERSION;

/**
 * @type {number}
 * @const
 */
ByteBuffer.DEFAULT_CAPACITY = 32;

/**
 * @type {boolean}
 * @const
 */
ByteBuffer.LITTLE_ENDIAN = true;

/**
 * @type {boolean}
 * @const
 */
ByteBuffer.BIG_ENDIAN = false;

/**
 * @param {number=} capacity
 * @param {boolean=} littleEndian
 * @returns {!ByteBuffer}
 * @nosideeffects
 */
ByteBuffer.allocate = function(capacity, littleEndian) {};

/**
 * @param {!ArrayBuffer|!Buffer|!{array: ArrayBuffer}|!{buffer: ArrayBuffer}|string} buffer
 * @param {(string|boolean)=} enc
 * @param {boolean=} littleEndian
 * @returns {!ByteBuffer}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.wrap = function(buffer, enc, littleEndian) {};

/**
 * @param {*} bb
 * @returns {boolean}
 * @nosideeffects
 */
ByteBuffer.isByteBuffer = function(bb) {};

/**
 * @param {boolean=} littleEndian
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.LE = function(littleEndian) {};

/**
 * @param {boolean=} bigEndian
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.BE = function(bigEndian) {};

/**
 * @param {number} capacity
 * @returns {boolean}
 */
ByteBuffer.prototype.resize = function(capacity) {};

/**
 * @param {number} begin
 * @param {number} end
 * @returns {!ByteBuffer}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.prototype.slice = function(begin, end) {};

/**
 * @param {number} begin
 * @param {number} end
 * @returns {!ByteBuffer}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.prototype.sliceAndCompact = function(begin, end) {};

/**
 * @param {number} capacity
 * @returns {boolean}
 */
ByteBuffer.prototype.ensureCapacity = function(capacity) {};

/**
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.flip = function() {};

/**
 * @param {number=} offset
 * @returns {!ByteBuffer}
 * @throws {Error}
 */
ByteBuffer.prototype.mark = function(offset) {};

/**
 * @returns {!ByteBuffer} this
 */
ByteBuffer.prototype.reset = function() {};

/**
 * @returns {!ByteBuffer}
 * @nosideeffects
 */
ByteBuffer.prototype.clone = function() {};

/**
 * @returns {!ByteBuffer}
 * @nosideeffects
 */
ByteBuffer.prototype.copy = function() {};

/**
 * @returns {number}
 * @nosideeffects
 */
ByteBuffer.prototype.remaining = function() {};

/**
 * @returns {number}
 * @nosideeffects
 */
ByteBuffer.prototype.capacity = function() {};

/**
 * @returns {!ByteBuffer}
 * @throws {Error}
 */
ByteBuffer.prototype.compact = function() {};

/**
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.destroy = function() {};

/**
 * @returns {!ByteBuffer}
 * @throws {Error}
 */
ByteBuffer.prototype.reverse = function() {};

/**
 * @param {!ByteBuffer} src
 * @param {number=} offset
 * @returns {!ByteBuffer}
 * @throws {Error}
 */
ByteBuffer.prototype.append = function(src, offset) {};

/**
 * @param {!ByteBuffer} src
 * @param {number=} offset
 * @returns {!ByteBuffer}
 * @throws {Error}
 */
ByteBuffer.prototype.prepend = function(src, offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeInt8 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readInt8 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeByte = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readByte = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeUint8 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readUint8 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeInt16 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readInt16 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeShort = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readShort = function (offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeUint16 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readUint16 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeInt32 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readInt32 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeInt = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readInt = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer} 
 */
ByteBuffer.prototype.writeUint32 = function(value, offset) {};

/**
 * @param {number=} offset 
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readUint32 = function(offset) {};

/**
 * @param {number|Long} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeInt64 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {Long}
 * @throws {Error}
 */
ByteBuffer.prototype.readInt64 = function(offset) {};

/**
 * @param {number|Long} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeUint64 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {Long}
 * @throws {Error}
 */
ByteBuffer.prototype.readUint64 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeFloat32 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readFloat32 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeFloat = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readFloat = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeFloat64 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readFloat64 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeDouble = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readDouble = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer}
 */
ByteBuffer.prototype.writeLong = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.prototype.readLong = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeVarint32 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number|!{value: number, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readVarint32 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeZigZagVarint32 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number|{value: number, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readZigZagVarint32 = function(offset) {};

/**
 * @param {number|Long} value
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 * @throws {Error}
 */
ByteBuffer.prototype.writeVarint64 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {!Long|{value: !Long, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readVarint64 = function(offset) {};

/**
 * @param {number|Long} value
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 * @throws {Error}
 */
ByteBuffer.prototype.writeZigZagVarint64 = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {!Long|!{value: !Long, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readZigZagVarint64 = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeVarint = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number|!{value: number, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readVarint = function(offset) {};

/**
 * @param {number} value
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeZigZagVarint = function(value, offset) {};

/**
 * @param {number=} offset
 * @returns {number|{value: number, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readZigZagVarint = function(offset) {};

/**
 * @param {number} value
 * @returns {number}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.calculateVarint32 = function(value) {};

/**
 * @param {number} value
 * @returns {number}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.calculateVarint64 = function(value) {};

/**
 * @param {string} str
 * @returns {number}
 * @nosideeffects
 */
ByteBuffer.calculateUTF8String = function(str) {};

/**
 * @param {string} str
 * @param {number=} offset 
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeUTF8String = function(str, offset) {};

/**
 * @param {number} chars
 * @param {number=} offset
 * @returns {string|!{string: string, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readUTF8String = function(chars, offset) {};

/**
 * @param {number} length
 * @param {number} offset
 * @throws {Error}
 */
ByteBuffer.prototype.readUTF8StringBytes = function(length, offset) {};

/**
 * @param {string} str
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeLString = function(str, offset) {};

/**
 * @param {number=} offset
 * @returns {string|!{string: string, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readLString = function(offset) {};

/**
 * @param {string} str
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeVString = function(str, offset) {};

/**
 * @param {number=} offset
 * @returns {string|!{string: string, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readVString = function(offset) {};

/**
 * @param {string} str
 * @param {number=} offset
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeCString = function(str, offset) {};

/**
 * @param {number=} offset
 * @returns {string|!{string: string, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readCString = function(offset) {};

/**
 * @param {*} data
 * @param {number=} offset
 * @param {(function(*):string)=} stringify
 * @returns {!ByteBuffer|number}
 */
ByteBuffer.prototype.writeJSON = function(data, offset, stringify) {};

/**
 * @param {number=} offset
 * @param {(function(string):*)=} parse
 * @returns {*|!{data: *, length: number}}
 * @throws {Error}
 */
ByteBuffer.prototype.readJSON = function(offset, parse) {};

/**
 * @param {number=} wrap
 * @returns {string}
 * @nosideeffects
 */
ByteBuffer.prototype.toColumns = function(wrap) {};

/**
 * @param {function(string)=} out
 */
ByteBuffer.prototype.printDebug = function(out) {};

/**
 * @param {boolean=} debug
 * @returns {string}
 * @nosideeffects
 */
ByteBuffer.prototype.toHex = function(debug) {};

/**
 * @returns {string}
 * @nosideeffects
 */
ByteBuffer.prototype.toBinary = function() {};

/**
 * @returns {string}
 * @nosideeffects
 */
ByteBuffer.prototype.toUTF8 = function() {};

/**
 * @returns {string}
 * @nosideeffects
 */
ByteBuffer.prototype.toBase64 = function() {};

/**
 * @param {string=} enc
 * @returns {string}
 * @nosideeffects
 */
ByteBuffer.prototype.toString = function(enc) {};

/**
 * @param {boolean=} forceCopy
 * @returns {ArrayBuffer}
 * @nosideeffects
 */
ByteBuffer.prototype.toArrayBuffer = function(forceCopy) {};

/**
 * @param {!ByteBuffer} src
 * @param {number} offset
 * @returns {!{char: number, length: number}}
 * @nosideeffects
 */
ByteBuffer.decodeUTF8Char = function(src, offset) {};

/**
 * @param {number} charCode
 * @param {!ByteBuffer} dst
 * @param {number} offset
 * @returns {number}
 * @throws {Error}
 */
ByteBuffer.encodeUTF8Char = function(charCode, dst, offset) {};

/**
 * @param {number} charCode
 * @returns {number}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.calculateUTF8Char = function(charCode) {};

/**
 * @param {number} n
 * @returns {number}
 * @nosideeffects
 */
ByteBuffer.zigZagEncode32 = function(n) {};

/**
 * @param {number} n
 * @returns {number}
 * @nosideeffects
 */
ByteBuffer.zigZagDecode32 = function(n) {};

/**
 * @param {!ByteBuffer} bb
 * @returns {string}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.encode64 = function(bb) {};

/**
 * @param {string} str
 * @param {boolean=} littleEndian
 * @returns {!ByteBuffer}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.decode64 = function(str, littleEndian) {};

/**
 * @param {!ByteBuffer} bb
 * @returns {string}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.encodeHex = function(bb) {};

/**
 * @param {string} str
 * @param {boolean=} littleEndian
 * @returns {!ByteBuffer}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.decodeHex = function(str, littleEndian) {};

/**
 * @param {!ByteBuffer} bb
 * @returns {string}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.encodeBinary = function(bb) {};

/**
 * @param {string} str
 * @param {boolean=} littleEndian
 * @returns {!ByteBuffer}
 * @throws {Error}
 * @nosideeffects
 */
ByteBuffer.decodeBinary = function(str, littleEndian) {};

/**
 * @type {number}
 * @const
 */
ByteBuffer.MAX_VARINT32_BYTES = 5;

/**
 * @type {number}
 * @const
 */
ByteBuffer.MAX_VARINT64_BYTES = 10;
