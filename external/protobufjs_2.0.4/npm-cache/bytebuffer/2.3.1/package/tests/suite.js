/*
 Copyright 2013 Daniel Wirtz <dcode@dcode.io>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/**
 * ByteBuffer.js Test Suite.
 * @author Daniel Wirtz <dcode@dcode.io>
 */

/**
 * File to use.
 * @type {string}
 */
var FILE = "ByteBuffer.min.js";

/**
 * ByteBuffer.
 * @type {ByteBuffer}
 */
var ByteBuffer = require(__dirname+"/../"+FILE);

/**
 * Long.
 * @type {Long}
 */
var Long = ByteBuffer.Long;

/**
 * Constructs a new Sandbox for module loaders and shim testing.
 * @param {Object.<string,*>} properties Additional properties to set
 * @constructor
 */
var Sandbox = function(properties) {
    this.Int8Array = function() {};
    this.Uint8Array = function() {};
    this.Int16Array = function() {};
    this.Uint16Array = function() {};
    this.Int32Array = function() {};
    this.Uint32Array = function() {};
    this.Float32Array = function() {};
    this.Float64Array = function() {};
    for (var i in properties) {
        this[i] = properties[i];
    }
    this.console = {
        log: function(s) {
            console.log(s);
        }
    };
};

/**
 * Test suite.
 * @type {Object.<string,function>}
 */
var suite = {
    
    "init": function(test) {
        test.ok(typeof ByteBuffer == "function");
        test.ok(typeof ByteBuffer.encodeUTF8Char == "function");
        test.done();
    },

    "construct/allocate": function(test) {
        var bb = new ByteBuffer();
        test.equal(bb.array.byteLength, ByteBuffer.DEFAULT_CAPACITY);
        bb = ByteBuffer.allocate();
        test.equal(bb.array.byteLength, ByteBuffer.DEFAULT_CAPACITY);
        test.done();
    },
    
    "wrap(ArrayBuffer)": function(test) {
        var buf = new ArrayBuffer(1);
        var bb = ByteBuffer.wrap(buf);
        test.strictEqual(bb.array, buf);
        test.equal(bb.offset, 0);
        test.equal(bb.length, 1);
        test.done();
    },
    
    "wrap(Uint8Array)": function(test) {
        var buf = new Uint8Array(1);
        var bb = ByteBuffer.wrap(buf);
        test.strictEqual(bb.array, buf.buffer);
        test.done();
    },
    
    "wrap(ByteBuffer)": function(test) { // clones
        var bb2 = new ByteBuffer(4).writeInt32(0x12345678).flip();
        bb2.offset = 1;
        var bb = ByteBuffer.wrap(bb2);
        test.strictEqual(bb2.toString("debug"), bb.toString("debug"));
        test.done();
    },
    
    "wrap(String)": function(test) {
        var bb = ByteBuffer.wrap("test");
        test.equal(bb.offset, 0);
        test.equal(bb.length, 4);
        test.equal(bb.readUTF8String(4), "test");
        
        bb = ByteBuffer.wrap("6162", "hex");
        test.equal(bb.toHex(true), "<61 62>");
        
        bb = ByteBuffer.wrap("YWI=", "base64");
        test.equal(bb.toHex(true), "<61 62>");
        
        test.done();
    },
    
    "wrap(Buffer)": function(test) {
        var b = new Buffer("abc", "utf8");
        var bb = ByteBuffer.wrap(b);
        test.equal(bb.toString("debug"), "<61 62 63>");
        test.done();
    },

    "resize": function(test) {
        var bb = new ByteBuffer(1);
        bb.resize(2);
        test.equal(bb.array.byteLength, 2);
        test.equal(bb.toString("debug"), "|00 00 ");
        test.done();
    },
    
    "slice": function(test) {
        var bb = new ByteBuffer(3);
        bb.writeUint8(0x12).writeUint8(0x34).writeUint8(0x56);
        var bb2 = bb.slice(1,2);
        test.strictEqual(bb.array, bb2.array);
        test.equal(bb.offset, 3);
        test.equal(bb.length, 0);
        test.equal(bb2.offset, 1);
        test.equal(bb2.length, 2);
        test.done();
    },

    "ensureCapacity": function(test) {
        var bb = new ByteBuffer(5);
        test.equal(bb.array.byteLength, 5);
        bb.ensureCapacity(6);
        test.equal(bb.array.byteLength, 10);
        bb.ensureCapacity(21);
        test.equal(bb.array.byteLength, 21);
        test.done();
    },

    "flip": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeUint32(0x12345678);
        test.equal(bb.offset, 4);
        test.equal(bb.length, 0);
        bb.flip();
        test.equal(bb.offset, 0);
        test.equal(bb.length, 4);
        test.done();
    },

    "reset": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeUint32(0x12345678);
        bb.reset();
        test.equal(bb.offset, 0);
        test.equal(bb.length, 0);
        test.done();
    },
    
    "mark": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeUint16(0x1234);
        test.equal(bb.offset, 2);
        test.equal(bb.length, 0);
        test.equal(bb.markedOffset, -1);
        bb.mark();
        test.equal(bb.markedOffset, 2);
        bb.writeUint16(0x5678);
        test.equal(bb.offset, 4);
        test.equal(bb.markedOffset, 2);
        bb.reset();
        test.equal(bb.offset, 2);
        test.equal(bb.length, 0);
        test.equal(bb.markedOffset, -1);
        bb.reset();
        test.equal(bb.offset, 0);
        test.equal(bb.length, 0);
        test.equal(bb.markedOffset, -1);
        bb.mark(2);
        test.equal(bb.markedOffset, 2);
        test.done();
    },

    "clone": function(test) {
        var bb = new ByteBuffer(1);
        var bb2 = bb.clone();
        test.strictEqual(bb.array, bb2.array);
        test.equal(bb.offset, bb2.offset);
        test.equal(bb.length, bb2.length);
        test.notStrictEqual(bb, bb2);
        test.done();
    },
    
    "copy": function(test) {
        var bb = new ByteBuffer(1);
        bb.writeUint8(0x12);
        var bb2 = bb.copy();
        test.notStrictEqual(bb, bb2);
        test.notStrictEqual(bb.array, bb2.array);
        test.equal(bb2.offset, bb.offset);
        test.equal(bb2.length, bb.length);
        test.done();
    },
    
    "compact": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeUint8(0x12);
        var prevArray = bb.array;
        bb.compact();
        test.notStrictEqual(bb.array, prevArray);
        test.equal(bb.array.byteLength, 1);
        test.equal(bb.offset, 0);
        test.equal(bb.length, 1);
        test.done();
    },

    "compactEmpty": function(test) {
        var bb = new ByteBuffer(2);
        bb.compact();
        test.strictEqual(bb.offset, 0);
        test.strictEqual(bb.length, 0);
        test.strictEqual(bb.view, null); // Special case
        test.strictEqual(bb.array.byteLength, 0);
        bb.writeInt32(0xFFFFFFFF);
        bb.flip();
        test.strictEqual(bb.offset, 0);
        test.strictEqual(bb.length, 4);
        test.notStrictEqual(bb.view, null);
        test.strictEqual(bb.array.byteLength, 4); // Cannot double 0, so it takes 32bits
        test.done();
    },
    
    "destroy": function(test) {
        var bb = new ByteBuffer(1);
        bb.writeUint8(0x12);
        bb.destroy();
        test.strictEqual(bb.array, null);
        test.equal(bb.offset, 0);
        test.equal(bb.length, 0);
        test.equal(bb.toString("debug"), "DESTROYED");
        test.done();
    },
    
    "reverse": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeUint32(0x12345678);
        bb.flip();
        bb.reverse();
        test.equal(bb.toString("debug"), "<78 56 34 12>");
        test.done();
    },
    
    "append": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeUint16(0x1234);
        var bb2 = new ByteBuffer(2);
        bb2.writeUint16(0x5678);
        bb2.flip();
        bb.append(bb2);
        test.equal(bb.toString("debug"), ">12 34 56 78<");
        bb.append(bb2, 1);
        test.equal(bb.toString("debug"), ">12 56 78 78<");
        test.done();
    },
    
    "prepend": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeUint16(0x1234);
        bb.flip();
        var bb2 = new ByteBuffer(2);
        bb2.writeUint16(0x5678);
        bb2.flip();
        bb.prepend(bb2);
        test.equal(bb.toString("debug"), "<56 78 12 34>");
        bb.offset = 4;
        bb.prepend(bb2, 3);
        test.equal(bb.toString("debug"), " 56 56 78 34|")
        test.done();
    },
    
    "write/readInt8": function(test) {
        var bb = new ByteBuffer(1);
        bb.writeInt8(0xFF);
        bb.flip();
        test.equal(-1, bb.readInt8());
        test.done();
    },

    "write/readByte": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readInt8, bb.readByte);
        test.strictEqual(bb.writeInt8, bb.writeByte);
        test.done();
    },
    
    "write/readUint8": function(test) {
        var bb = new ByteBuffer(1);
        bb.writeUint8(0xFF);
        bb.flip();
        test.equal(0xFF, bb.readUint8());
        test.done();
    },
    
    "write/readInt16": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeInt16(0xFFFF);
        bb.flip();
        test.equal(-1, bb.readInt16());
        test.done();
    },
    
    "write/readShort": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readInt16, bb.readShort);
        test.strictEqual(bb.writeInt16, bb.writeShort);
        test.done();
    },

    "write/readUint16": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeUint16(0xFFFF);
        bb.flip();
        test.equal(0xFFFF, bb.readUint16());
        test.done();
    },
    
    "write/readInt32": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeInt32(0xFFFFFFFF);
        bb.flip();
        test.equal(-1, bb.readInt32());
        test.done();
    },
    
    "write/readInt": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readInt32, bb.readInt);
        test.strictEqual(bb.writeInt32, bb.writeInt);
        test.done();
    },
    
    "write/readUint32": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeUint32(0x12345678);
        bb.flip();
        test.equal(0x12345678, bb.readUint32());
        test.done();
    },
    
    "write/readFloat32": function(test) {
        var bb = new ByteBuffer(4);
        bb.writeFloat32(0.5);
        bb.flip();
        test.equal(0.5, bb.readFloat32()); // 0.5 remains 0.5 if Float32
        test.done();
    },
    
    "write/readFloat": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readFloat32, bb.readFloat);
        test.strictEqual(bb.writeFloat32, bb.writeFloat);
        test.done();
    },
    
    "write/readFloat64": function(test) {
        var bb = new ByteBuffer(8);
        bb.writeFloat64(0.1);
        bb.flip();
        test.equal(0.1, bb.readFloat64()); // would be 0.10000000149011612 if Float32
        test.done();
    },
    
    "write/readDouble": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readFloat64, bb.readDouble);
        test.strictEqual(bb.writeFloat64, bb.writeDouble);
        test.done();
    },
    
    "write/readInt64": function(test) {
        var bb = new ByteBuffer(8);
        
        var max = ByteBuffer.Long.MAX_SIGNED_VALUE.toNumber();
        bb.writeInt64(max).flip();
        test.equal(bb.toString("debug"), "<7F FF FF FF FF FF FF FF>");
        test.equal(bb.readInt64(0), max);
        
        var min = ByteBuffer.Long.MIN_SIGNED_VALUE.toNumber();
        bb.writeInt64(min).flip();
        test.equal(bb.toString("debug"), "<80 00 00 00 00 00 00 00>");
        test.equal(bb.readInt64(0), min);
        
        bb.writeInt64(-1).flip();
        test.equal(bb.toString("debug"), "<FF FF FF FF FF FF FF FF>");
        test.equal(bb.readInt64(0), -1);
        
        bb.reset();
        bb.LE().writeInt64(new ByteBuffer.Long(0x89ABCDEF, 0x01234567)).flip();
        test.equal(bb.toString("debug"), "<EF CD AB 89 67 45 23 01>");
        
        test.done();
    },
    
    "write/readUint64": function(test) {
        var bb = new ByteBuffer(8);

        var max = ByteBuffer.Long.MAX_UNSIGNED_VALUE.toNumber();
        bb.writeUint64(max).flip();
        test.equal(bb.toString("debug"), "<FF FF FF FF FF FF FF FF>");
        test.equal(bb.readUint64(0), max);

        var min = ByteBuffer.Long.MIN_UNSIGNED_VALUE.toNumber();
        bb.writeLong(min).flip();
        test.equal(bb.toString("debug"), "<00 00 00 00 00 00 00 00>");
        test.equal(bb.readUint64(0), min);

        bb.writeUint64(-1).flip();
        test.equal(bb.toString("debug"), "<00 00 00 00 00 00 00 00>");
        test.equal(bb.readUint64(0), 0);

        bb.reset();
        bb.LE().writeUint64(new ByteBuffer.Long(0x89ABCDEF, 0x01234567, true)).flip();
        test.equal(bb.toString("debug"), "<EF CD AB 89 67 45 23 01>");

        test.done();
    },
    
    "write/readLong": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readInt64, bb.readLong);
        test.strictEqual(bb.writeInt64, bb.writeLong);
        test.done();
    },
    
    "writeVarint64/readVarint32": function(test) {
        var bb = new ByteBuffer();
        bb.writeVarint64(Long.fromNumber(-1));
        bb.flip();
        var n = bb.readVarint32();
        test.equal(n, -1);
        test.done();
    },
    
    "LE/BE": function(test) {
        var bb = new ByteBuffer(8).LE().writeInt(1).BE().writeInt(2).flip();
        test.equal(bb.toString("debug"), "<01 00 00 00 00 00 00 02>");
        test.done();
    },
    
    "calculateVarint32/64": function(test) {
        test.equal(ByteBuffer.MAX_VARINT32_BYTES, 5);
        test.equal(ByteBuffer.MAX_VARINT64_BYTES, 10);
        var values = [
            [0, 1],
            [-1, 5, 10],
            [1<<7, 2],
            [1<<14, 3],
            [1<<21, 4],
            [1<<28, 5],
            [0x7FFFFFFF | 0, 5],
            [0xFFFFFFFF, 5],
            [0xFFFFFFFF | 0, 5, 10]
        ];
        for (var i=0; i<values.length; i++) {
            test.equal(ByteBuffer.calculateVarint32(values[i][0]), values[i][1]);
            test.equal(ByteBuffer.calculateVarint64(values[i][0]), values[i].length > 2 ? values[i][2] : values[i][1]);
        }
        var Long = ByteBuffer.Long;
        values = [
            [Long.fromNumber(1).shiftLeft(35), 6],
            [Long.fromNumber(1).shiftLeft(42), 7],
            [Long.fromNumber(1).shiftLeft(49), 8],
            [Long.fromNumber(1).shiftLeft(56), 9],
            [Long.fromNumber(1).shiftLeft(63), 10],
            [Long.fromNumber(1, true).shiftLeft(63), 10]
        ];
        for (i=0; i<values.length; i++) {
            test.equal(ByteBuffer.calculateVarint64(values[i][0]), values[i][1]);
        }
        test.done();
    },
    
    "zigZagEncode/Decode32/64": function(test) {
        var Long = ByteBuffer.Long;
        var values = [
            [ 0, 0],
            [-1, 1],
            [ 1, 2],
            [-2, 3],
            [ 2, 4],
            [-3, 5],
            [ 3, 6],
            [ 2147483647, 4294967294],
            [-2147483648, 4294967295]
        ];
        for (var i=0; i<values.length; i++) {
            test.equal(ByteBuffer.zigZagEncode32(values[i][0]), values[i][1]);
            test.equal(ByteBuffer.zigZagDecode32(values[i][1]), values[i][0]);
            test.equal(ByteBuffer.zigZagEncode64(values[i][0]).toNumber(), values[i][1]);
            test.equal(ByteBuffer.zigZagDecode64(values[i][1]).toNumber(), values[i][0]);
        }
        values = [
            [Long.MAX_SIGNED_VALUE, Long.MAX_UNSIGNED_VALUE.subtract(Long.ONE)],
            [Long.MIN_SIGNED_VALUE, Long.MAX_UNSIGNED_VALUE]
        ];
        // NOTE: Even 64bit doubles from toNumber() fail for these values so we are using toString() here
        for (i=0; i<values.length; i++) {
            test.equal(ByteBuffer.zigZagEncode64(values[i][0]).toString(), values[i][1].toString());
            test.equal(ByteBuffer.zigZagDecode64(values[i][1]).toString(), values[i][0].toString());
        }
        test.done();
    },
    
    "write/readVarint32": function(test) {
        var values = [
            [1,1],
            [300,300],
            [0x7FFFFFFF, 0x7FFFFFFF],
            [0xFFFFFFFF, -1],
            [0x80000000, -2147483648]
        ];
        var bb = new ByteBuffer(5);
        for (var i=0; i<values.length; i++) {
            var encLen = bb.writeVarint32(values[i][0], 0);
            var dec = bb.readVarint32(0);
            test.equal(values[i][1], dec['value']);
            test.equal(encLen, dec['length']);
        }
        test.done();
    },

    "write/readVarint64": function(test) {
        var Long = ByteBuffer.Long;
        var values = [
            [Long.ONE],
            [Long.fromNumber(300)],
            [Long.fromNumber(0x7FFFFFFF)],
            [Long.fromNumber(0xFFFFFFFF)],
            [Long.fromBits(0xFFFFFFFF, 0x7FFFFFFF)],
            [Long.fromBits(0xFFFFFFFF, 0xFFFFFFFF)]
        ];
        var bb = new ByteBuffer(10);
        for (var i=0; i<values.length; i++) {
            var encLen = bb.writeVarint64(values[i][0], 0);
            var dec = bb.readVarint64(0);
            test.equal((values[i].length > 1 ? values[i][1] : values[i][0]).toString(), dec['value'].toString());
            test.equal(encLen, dec['length']);
        }
        test.done();
    },

    "write/readZigZagVarint32": function(test) {
        var values = [
            0,
            1,
             300,
            -300,
             2147483647,
            -2147483648
        ];
        var bb = new ByteBuffer(10);
        for (var i=0; i<values.length; i++) {
            var encLen = bb.writeZigZagVarint32(values[i], 0);
            var dec = bb.readZigZagVarint32(0);
            test.equal(values[i], dec['value']);
            test.equal(encLen, dec['length']);
        }
        test.done();
    },

    "write/readZigZagVarint64": function(test) {
        var Long = ByteBuffer.Long;
        var values = [
            Long.ONE, 1,
            Long.fromNumber(-3),
            Long.fromNumber(300),
            Long.fromNumber(-300),
            Long.fromNumber(0x7FFFFFFF),
            Long.fromNumber(0x8FFFFFFF),
            Long.fromNumber(0xFFFFFFFF),
            Long.fromBits(0xFFFFFFFF, 0x7FFFFFFF),
            Long.fromBits(0xFFFFFFFF, 0xFFFFFFFF)
        ];
        var bb = new ByteBuffer(10);
        for (var i=0; i<values.length; i++) {
            var encLen = bb.writeZigZagVarint64(values[i], 0);
            var dec = bb.readZigZagVarint64(0);
            test.equal(values[i].toString(), dec['value'].toString());
            test.equal(encLen, dec['length']);
        }
        test.done();
    },
    
    "write/readVarint": function(test) {
        var bb = new ByteBuffer(1);
        test.strictEqual(bb.readVarint32, bb.readVarint);
        test.strictEqual(bb.writeVarint32, bb.writeVarint);
        test.done();
    },
    
    "write/readLString": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeLString("ab"); // resizes to 4
        test.equal(bb.array.byteLength, 4);
        test.equal(bb.offset, 3);
        test.equal(bb.length, 0);
        bb.flip();
        test.equal(bb.toString("debug"), "<02 61 62>00 ");
        test.deepEqual({"string": "ab", "length": 3}, bb.readLString(0));
        test.equal(bb.toString("debug"), "<02 61 62>00 ");
        test.equal("ab", bb.readLString());
        test.equal(bb.toString("debug"), " 02 61 62|00 ");
        test.done();
    },

    "write/readVString": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeVString("ab"); // resizes to 4
        test.equal(bb.array.byteLength, 4);
        test.equal(bb.offset, 3);
        test.equal(bb.length, 0);
        bb.flip();
        test.equal(bb.toString("debug"), "<02 61 62>00 ");
        test.deepEqual({"string": "ab", "length": 3}, bb.readVString(0));
        test.equal(bb.toString("debug"), "<02 61 62>00 ");
        test.equal("ab", bb.readLString());
        test.equal(bb.toString("debug"), " 02 61 62|00 ");
        test.done();
    },
    
    "write/readCString": function(test) {
        var bb = new ByteBuffer(2);
        bb.writeCString("ab"); // resizes to 4
        test.equal(bb.array.byteLength, 4);
        test.equal(bb.offset, 3);
        test.equal(bb.length, 0);
        bb.flip();
        test.equal(bb.toString("debug"), "<61 62 00>00 ");
        test.deepEqual({"string": "ab", "length": 3}, bb.readCString(0));
        test.equal(bb.toString("debug"), "<61 62 00>00 ");
        test.equal("ab", bb.readCString());
        test.equal(bb.toString("debug"), " 61 62 00|00 ");
        test.done();
    },
    
    "write/readJSON": function(test) {
        var bb = new ByteBuffer();
        var data = {"x":1};
        bb.writeJSON(data);
        bb.flip();
        test.deepEqual(data, bb.readJSON());
        test.done();
    },
    
    "toHex": function(test) {
        var bb = new ByteBuffer(3);
        bb.writeUint16(0x1234);
        test.equal(bb.flip().toHex(), "1234");
        test.done();
    },
    
    "toString": function(test) {
        var bb = new ByteBuffer(3);
        bb.writeUint16(0x6162).flip();
        test.equal(bb.toString(), "ByteBuffer(offset=0,markedOffset=-1,length=2,capacity=3)");
        test.equal(bb.toString("hex"), "6162");
        test.equal(bb.toString("base64"), "YWI=");
        test.equal(bb.toString("utf8"), "ab");
        test.equal(bb.toString("debug"), "<61 62>00 ");
        test.done();
    },
    
    "toArrayBuffer": function(test) {
        var bb = new ByteBuffer(3);
        bb.writeUint16(0x1234);
        var buf = bb.toArrayBuffer();
        test.equal(buf.byteLength, 2);
        test.equal(buf[0], 0x12);
        test.equal(buf[1], 0x34);
        test.equal(bb.offset, 2);
        test.equal(bb.length, 0);
        test.equal(bb.array.byteLength, 3);
        test.done();
    },
    
    "toBuffer": function(test) {
        var bb = new ByteBuffer(3);
        bb.writeUint16(0x1234);
        var buf;
        try {
            buf = bb.toBuffer();
        } catch (e) {
            console.trace(e);
        }
        test.equal(buf.length, 2);
        test.equal(buf[0], 0x12);
        test.equal(buf[1], 0x34);
        test.equal(bb.offset, 2);
        test.equal(bb.length, 0);
        test.equal(bb.array.byteLength, 3);
        test.done();
    },
    
    "printDebug": function(test) {
        var bb = new ByteBuffer(3);
        function callMe() { callMe.called = true; };
        bb.printDebug(callMe);
        test.ok(callMe.called);
        test.done();
    },
    
    "encode/decode/calculateUTF8Char": function(test) {
        var bb = new ByteBuffer(6)
          , chars = [0x00, 0x7F, 0x80, 0x7FF, 0x800, 0xFFFF, 0x10000, 0x1FFFFF, 0x200000, 0x3FFFFFF, 0x4000000, 0x7FFFFFFF]
          , dec;
        for (var i=0; i<chars.length;i++) {
            ByteBuffer.encodeUTF8Char(chars[i], bb, 0);
            dec = ByteBuffer.decodeUTF8Char(bb, 0);
            test.equal(chars[i], dec['char']);
            test.equal(ByteBuffer.calculateUTF8Char(chars[i]), dec["length"]);
            test.equal(String.fromCharCode(chars[i]), String.fromCharCode(dec['char']));
        }
        test.throws(function() {
            ByteBuffer.encodeUTF8Char(-1, bb, 0);
        });
        test.throws(function() {
            ByteBuffer.encodeUTF8Char(0x80000000, bb, 0);
        });
        bb.reset();
        bb.writeByte(0xFE).writeByte(0).writeByte(0).writeByte(0).writeByte(0).writeByte(0);
        bb.flip();
        test.throws(function() {
            ByteBuffer.decodeUTF8Char(bb, 0);
        });
        test.done();
    },

    "pbjsi19": function(test) {
        // test that this issue is fixed: https://github.com/dcodeIO/ProtoBuf.js/issues/19
        var bb = new ByteBuffer(9); // Trigger resize to 18 in writeVarint64
        bb.writeVarint32(16);
        bb.writeVarint32(2);
        bb.writeVarint32(24);
        bb.writeVarint32(0);
        bb.writeVarint32(32);
        bb.writeVarint64(ByteBuffer.Long.fromString("1368057600000"));
        bb.writeVarint32(40);
        bb.writeVarint64(ByteBuffer.Long.fromString("1235455123"));
        test.equal(bb.toString("debug"), ">10 02 18 00 20 80 B0 D9 B4 E8 27 28 93 99 8E CD 04<00 ");
        test.done();
    },
    
    "encode/decode64": function(test) {
        var values = [
            ["ProtoBuf.js", "UHJvdG9CdWYuanM="],
            ["ProtoBuf.j", "UHJvdG9CdWYuag=="],
            ["ProtoBuf.", "UHJvdG9CdWYu"],
            ["ProtoBuf", "UHJvdG9CdWY="]
        ];
        for (var i=0; i<values.length; i++) {
            var str = values[i][0],
                b64 = values[i][1];
            var bb = ByteBuffer.decode64(b64);
            test.strictEqual(bb.readUTF8String(str.length, 0).string, str);
            test.strictEqual(ByteBuffer.encode64(bb), b64);
            test.strictEqual(bb.toBase64(), b64);
            test.strictEqual(bb.toString("base64"), b64);
        }
        test.done();
    },

    "encode/decodeHex": function(test) {
        var bb = new ByteBuffer(4).writeInt32(0x12345678).flip(),
            str = bb.toString("hex");
        test.strictEqual(str, "12345678");
        var bb2 = ByteBuffer.wrap(str, "hex");
        test.deepEqual(bb.array, bb2.array);
        test.done();
    },

    "encode/decodeBinary": function(test) {
        var bb = new ByteBuffer(4).writeInt32(0x12345678).flip(),
            str = bb.toString("binary");
        test.strictEqual(str.length, 4);
        test.strictEqual(str, String.fromCharCode(0x12)+String.fromCharCode(0x34)+String.fromCharCode(0x56)+String.fromCharCode(0x78));
        var bb2 = ByteBuffer.wrap(str, "binary");
        test.deepEqual(bb.array, bb2.array);
        test.done();
    },

    "NaN": function(test) {
        var bb = new ByteBuffer(4);
        test.ok(isNaN(bb.writeFloat(NaN).flip().readFloat(0)));
        test.strictEqual(bb.writeFloat(+Infinity).flip().readFloat(0), +Infinity);
        test.strictEqual(bb.writeFloat(-Infinity).flip().readFloat(0), -Infinity);
        bb.resize(8);
        test.ok(isNaN(bb.writeDouble(NaN).flip().readDouble(0)));
        test.strictEqual(bb.writeDouble(+Infinity).flip().readDouble(0), +Infinity);
        test.strictEqual(bb.writeDouble(-Infinity).flip().readDouble(0), -Infinity);

        // Varints, however, always need a cast, which results in the following:
        test.strictEqual(NaN >>> 0, 0);
        test.strictEqual(NaN | 0, 0);
        test.strictEqual(Infinity >>> 0, 0);
        test.strictEqual(Infinity | 0, 0);
        test.strictEqual(-Infinity >>> 0, 0);
        test.strictEqual(-Infinity | 0, 0);

        test.done();
    },

    "ByteBuffer-like": function(test) {
        var bb = new ByteBuffer(4);
        var bbLike = {
            array: bb.array,
            view: bb.view,
            offset: bb.offset,
            markedOffset: bb.markedOffset,
            length: bb.length,
            littleEndian: bb.littleEndian
        };
        test.ok(ByteBuffer.isByteBuffer(bbLike));
        var bb2 = ByteBuffer.wrap(bbLike);
        test.ok(bb2 instanceof ByteBuffer);
        test.strictEqual(bbLike.array, bb2.array);
        test.strictEqual(bbLike.view, bb2.view);
        test.strictEqual(bbLike.offset, bb2.offset);
        test.strictEqual(bbLike.markedOffset, bb2.markedOffset);
        test.strictEqual(bbLike.length, bb2.length);
        test.strictEqual(bbLike.littleEndian, bb2.littleEndian);
        test.done();
    },
    
    "commonjs": function(test) {
        var fs = require("fs")
          , vm = require("vm")
          , util = require('util');
        
        var code = fs.readFileSync(__dirname+"/../"+FILE);
        var Long = ByteBuffer.Long;
        var sandbox = new Sandbox({
            require: function(moduleName) {
                if (moduleName == 'long') {
                    return Long;
                }
            },
            module: {
                exports: {}
            }
        });
        vm.runInNewContext(code, sandbox, "ByteBuffer.js in CommonJS-VM");
        // console.log(util.inspect(sandbox));
        test.ok(typeof sandbox.module.exports == 'function');
        test.ok(sandbox.module.exports.Long && sandbox.module.exports.Long == ByteBuffer.Long);
        test.done();
    },
    
    "amd": function(test) {
        var fs = require("fs")
          , vm = require("vm")
          , util = require('util');

        var code = fs.readFileSync(__dirname+"/../"+FILE);
        var sandbox = new Sandbox({
            require: function() {},
            define: (function() {
                function define(moduleName, dependencies, constructor) {
                    define.called = [moduleName, dependencies];
                }
                define.amd = true;
                define.called = null;
                return define;
            })()
        });
        vm.runInNewContext(code, sandbox, "ByteBuffer.js in AMD-VM");
        // console.log(util.inspect(sandbox));
        test.ok(sandbox.define.called && sandbox.define.called[0] == "ByteBuffer" && sandbox.define.called[1][0] == "Math/Long");
        test.done();
    },
    
    "shim": function(test) {
        var fs = require("fs")
            , vm = require("vm")
            , util = require('util');

        var code = fs.readFileSync(__dirname+"/../"+FILE);
        var sandbox = new Sandbox({
            dcodeIO: {
                Long: ByteBuffer.Long
            }
        });
        vm.runInNewContext(code, sandbox, "ByteBuffer.js in shim-VM");
        // console.log(util.inspect(sandbox));
        test.ok(typeof sandbox.dcodeIO != 'undefined' && typeof sandbox.dcodeIO.ByteBuffer != 'undefined');
        test.ok(sandbox.dcodeIO.ByteBuffer.Long && sandbox.dcodeIO.ByteBuffer.Long == ByteBuffer.Long);
        test.done();
    },
    
    "helloworld": function(test) {
        var bb = new ByteBuffer();
        bb.writeUTF8String("Hello world! from ByteBuffer.js. This is just a last visual test of ByteBuffer#printDebug.");
        bb.flip();
        console.log("");
        bb.printDebug(console.log);
        test.done();
    }
};

module.exports = suite;
