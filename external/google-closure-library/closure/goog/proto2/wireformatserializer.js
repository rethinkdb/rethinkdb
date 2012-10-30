// Copyright 2012 Hexagram 49 Inc. All rights reserved.

/**
 * @fileoverview Protocol Buffer 2 Serializer which serializes messages
 *  into the protocol buffer wire format. Because this serializer provides
 *  support for the same format used by the C++, Java, and Python canonical
 *  protocol buffer implementations it allows Javascript applications to
 *  share messages with protocol buffer allications from other languages.
 * 
 * @author bill@rethinkdb.com (William Rowan)
 */

goog.provide('goog.proto2.WireFormatSerializer');

goog.require('goog.proto2.Serializer');
goog.require('goog.debug.Error');
goog.require('goog.math.Long');

// Unused here but necessary to load type information to please the type checker
goog.require('goog.proto2.LazyDeserializer');

/**
 * Utility functions for converting between the UTF-8 encoded arrays expected by
 * protobuf wire format and the javascript strings used by the js protobuf library.
 */

/**
 * The resulting encoding is a string with characters in the range 0x0000-0x00FF
 * @param {Uint8Array} array
 * @return {string}
 */
function UTF8ArrayToASCIIString(array) {
    var chars = [];
    for (var i = 0; i < array.length; i++) {
        chars.push(String.fromCharCode(array[i]));
    }
    return chars.join('');

    // This results in a string with a bunch of null characters in it, why?
    //return Array.prototype.map.call(array, String.fromCharCode).join('');
}

function UTF8ArrayToString(array) {
    // The resulting encoding is a string in the javascript native USC-2 encoding
    return decodeURIComponent(escape(UTF8ArrayToASCIIString(array)));
}

function ASCIIStringToArray(string) {
    // Takes a string with characters in the range 0x0000-0x00FF
    // (i.e. all single byte values) and returns the corresponding Uint8Array
    return new Uint8Array(Array.prototype.map.call(string, function(ch) {
        return ch.charCodeAt(0);
    }));
}

function StringToUTF8Array(string) {
    // Takes a standard USC-2 javascript string and returns a UTF-8 encoded Uint8Array
    return ASCIIStringToArray(unescape(encodeURIComponent(string)));
}

/**
 * WireFormatSerializer, a serializer which serializes messages to protocol
 * buffer wire format suitable for use by other protocol buffer implemtations.
 * @constructor
 * @extends {goog.proto2.Serializer}
 */
goog.proto2.WireFormatSerializer = function() {
    /**
     * Used during deserialization to track position in the stream.
     * @type {number}
     * @private
     */
     this.deserializationIndex_ = 0;
}
goog.inherits(goog.proto2.WireFormatSerializer, goog.proto2.Serializer);

/**
 * Called when deserilizing incorrectly formated messages
 * @param {string} errMsg
 * @private
 */
goog.proto2.WireFormatSerializer.prototype.badMessageFormat_ = function(errMsg) {
    throw new goog.debug.Error("Error deserializing incorrectly formated message, " + errMsg);
};

/**
 * Serializes a message to an ArrayBuffer. 
 * @param {goog.proto2.Message} message The message to be serialized.
 * @return {!Uint8Array} The serialized message in wire format.
 * @override
 */
goog.proto2.WireFormatSerializer.prototype.serialize = function(message) {
    var descriptor = message.getDescriptor();
    var fields = descriptor.getFields();

    var fieldEncodings = [];
    var totalLength = 0;
    for(var i = 0; i < fields.length; i++) {
        var field = fields[i];

        if(!message.has(field)) {
            continue;
        }

        var r = 0;
        do {
            var fieldBuffer = this.getSerializedValue(field, message.get(field, r));
            fieldEncodings.push(fieldBuffer);
            totalLength += fieldBuffer.byteLength;
            r++;
        } while(field.isRepeated() && (r < message.countOf(field)));
    }

    var resultArray = new Uint8Array(totalLength);

    var offset = 0;
    for(var i = 0; i < fieldEncodings.length; i++) {
        var enc = fieldEncodings[i];

        resultArray.set(new Uint8Array(enc), offset);
        offset += enc.byteLength;
    }

    return resultArray;
}

/**
 * @param {goog.proto2.FieldDescriptor} field
 * @param {*} value
 * @override
 */
goog.proto2.WireFormatSerializer.prototype.getSerializedValue = function(field, value) {
    var tag = field.getTag();
    var fieldType = field.getFieldType();
    var wireType = goog.proto2.WireFormatSerializer.WireTypeForFieldType(fieldType);

    var keyInt = (tag << 3) | wireType;
    var keyArray = this.encodeVarInt(goog.math.Long.fromNumber(keyInt));

    var valArray;
    switch(wireType) {
    case goog.proto2.WireFormatSerializer.WireType.VARINT:
        var integer = goog.math.Long.fromNumber(/** @type {number} */(value));

        if(fieldType == goog.proto2.FieldDescriptor.FieldType.SINT64 ||
           fieldType == goog.proto2.FieldDescriptor.FieldType.SINT32) {
            integer = this.zigZagEncode(integer); 
        }

        valArray = this.encodeVarInt(integer);
        break;
    case goog.proto2.WireFormatSerializer.WireType.FIXED64:
        valArray = this.encodeFixed64(field,/** @type {number} */(value));
        break;
    case goog.proto2.WireFormatSerializer.WireType.LENGTH_DELIMITED:
        valArray = this.encodeLengthDelimited(field, value);
        break;
    case goog.proto2.WireFormatSerializer.WireType.START_GROUP:
        this.badMessageFormat_("Use of Groups deprecated and not supported");
        break;
    case goog.proto2.WireFormatSerializer.WireType.FIXED32:
        valArray = this.encodeFixed32(field,/** @type {number} */(value));
        break;
    default:
        this.badMessageFormat_("Unexpected wire type");
    }

    var fieldEncoding = new Uint8Array(keyArray.length + valArray.length);
    fieldEncoding.set(new Uint8Array(keyArray), 0);
    fieldEncoding.set(new Uint8Array(valArray), keyArray.length);

    return fieldEncoding;
}

/**
 * Zig-zag encodes the integer.
 * @param {!goog.math.Long} longint The integer to be encoded.
 * @return {!goog.math.Long} The zig zag encoded integer.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.zigZagEncode = function(longint) {
    return longint.shiftLeft(1).xor(longint.shiftRight(63));
}

/**
 * Zig-zag decodes the integer.
 * @param {!goog.math.Long} longint The integer to be encoded.
 * @return {!goog.math.Long} The zig zag encoded integer.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.zigZagDecode = function(longint) {
    return longint.shiftRight(1).xor(longint.shiftLeft(63));
}

/**
 * Encodes an integer as a varint.
 * @param {!goog.math.Long} longint The integer to be encoded.
 * @return {!Uint8Array} The encoded value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.encodeVarInt = function(longint) {

    function encodeVarIntArray(longint) {
        var leastVal = longint.modulo(goog.math.Long.fromNumber(128)).toInt();

        var bytesArray;
        if(longint.greaterThan(goog.math.Long.fromNumber(127))) {
            leastVal += 128; // Set continuation bit

            bytesArray = encodeVarIntArray(longint.shiftRight(7));
            bytesArray.unshift(leastVal);
        } else {
            bytesArray = [leastVal];
        }

        return bytesArray;
    }

    var bytesArray = encodeVarIntArray(longint);
    return new Uint8Array(bytesArray);
}

/**
 * Encodes given number to a fixed 64 bit value. All numeric values are supplied
 * as 64 bit floats. Therefore, converting to the appropriate binary
 * representation is necessary and cannot be done with full precision for
 * integers.
 * @param {!goog.proto2.FieldDescriptor} field The field to be encoded.
 * @param {!number} value The value to be encoded.
 * @return {!Uint8Array} An 8 byte little endian array representing the value.
 */
goog.proto2.WireFormatSerializer.prototype.encodeFixed64 = function(field, value) {
    var buffer = new ArrayBuffer(8);
    var view = new DataView(buffer);

    // The format specifies that multibyte values be specified in little
    // endian order, thus the last argument for the set* functions.

    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.DOUBLE:
        view.setFloat64(0, value, true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.FIXED64:
        // No set Uint64, do 4 bytes at a time
        var longForm = goog.math.Long.fromNumber(value);
        view.setUint32(0, longForm.getLowBitsUnsigned(), true);
        view.setUint32(4, longForm.getHighBits(), true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.SFIXED64:
        var longForm = goog.math.Long.fromNumber(value);
        view.setUint32(0, longForm.getLowBitsUnsigned(), true);
        view.setInt32(4, longForm.getHighBits(), true);
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
        break;
    }

    return new Uint8Array(buffer);
}

/**
 * Encodes given number to a fixed 32 bit value.
 * @param {!goog.proto2.FieldDescriptor} field The field to be encoded.
 * @param {number} value The value to be encoded.
 * @return {!Uint8Array} A 4 byte little endian array representing the value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.encodeFixed32 =
        function(field, value) {
    var buffer = new ArrayBuffer(4);
    var view = new DataView(buffer);

    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.FLOAT:
        view.setFloat32(0, value, true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.FIXED32:
        view.setUint32(0, value, true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.SFIXED32:
        view.setInt32(0, value, true);
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
    }

    return new Uint8Array(buffer);
}

/**
 * Encodes the given value as a length delimited field. How the value is
 * encoded depends on the field type.
 * @param {!goog.proto2.FieldDescriptor} field The field to be encoded.
 * @param {*} value The value to be encoded.
 * @return {!Uint8Array} A multibyte little endian array representing the value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.encodeLengthDelimited =
        function(field, value) {
    var resultArray;

    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.MESSAGE:
        resultArray = this.serialize(/** @type {goog.proto2.Message} */(value));
        break;
    case goog.proto2.FieldDescriptor.FieldType.STRING:
        resultArray = StringToUTF8Array(/** @type {string} */(value));
        break;
    case goog.proto2.FieldDescriptor.FieldType.BYTES:
        resultArray = ASCIIStringToArray(/** @type {string} */(value));
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
    }

    var lengthInt = resultArray.length;
    var lengthArray = this.encodeVarInt(goog.math.Long.fromNumber(lengthInt));
    var finalArray = new Uint8Array(lengthArray.length + lengthInt);
    finalArray.set(lengthArray, 0);
    finalArray.set(resultArray, lengthArray.length);

    return finalArray;
}

/**
 * Deserializes data stream and initializes message object
 * @param {goog.proto2.Message} message The message to initialize.
 * @param {*} data The stream to deserialize.
 * @override 
 */
goog.proto2.WireFormatSerializer.prototype.deserializeTo = function(message, data) {

    var messageDescriptor = message.getDescriptor();
    this.deserializationIndex_ = 0;
    
    while(this.deserializationIndex_ < data.length) {
        var keyInt = this.decodeVarInt(/** @type {!Uint8Array} */(data));

        var tag = keyInt >> 3;
        var wireType = keyInt % 8;

        var field = messageDescriptor.findFieldByTag(tag);

        // Though field may be null we still decode the value to remove it
        // from the stream.
        var value = this.decodeValue(wireType, field, /** @type {!Uint8Array} */(data));

        if((field != null) && (value != null)) {
            if(field.isRepeated()) {
                message.add(field, value);
            } else {
                message.set(field, value);
            }
        }
    }
}

/**
 * Deserializes varint from data stream.
 * @param {!Uint8Array} data The data stream to read from.
 * @return {number} Decoded value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.decodeVarInt = function(data) {
    var intval = data[this.deserializationIndex_++];

    if(intval > 127) {

        // Unset continuation bit
        intval -= 128;

        var rest = this.decodeVarInt(data);
        intval += (rest << 7);
    }

    return intval;
}

/**
 * Deserializes the next value from the data stream.
 * @param {!number} wireType The wiretype to decode.
 * @param {goog.proto2.FieldDescriptor} field The field to deserialize.
 * @param {!Uint8Array} data The data stream to read from.
 * @return {*} The decoded value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.decodeValue =
        function(wireType, field, data) {
    var value;
    switch(wireType) {
    case goog.proto2.WireFormatSerializer.WireType.VARINT:
        value = this.decodeVarIntTypes(field, data);
        break;
    case goog.proto2.WireFormatSerializer.WireType.LENGTH_DELIMITED:
        value = this.decodeLengthDelimited(field, data);
        break;
    case goog.proto2.WireFormatSerializer.WireType.FIXED32:
        value = this.decodeFixed32(field, data);
        break;
    case goog.proto2.WireFormatSerializer.WireType.FIXED64:
        value = this.decodeFixed64(field, data);
        break;
    default:
        this.badMessageFormat_("Unexpected wire type of " + wireType);
        break;
    }

    return value;
}

/**
 * Decodes a varint from the stream and converts it to the correct type.
 * @param {goog.proto2.FieldDescriptor} field The field to decode.
 * @param {!Uint8Array} data The data stream to read from.
 * @return {*} The decoded value. 
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.decodeVarIntTypes =
        function(field, data) {
    var intVal = this.decodeVarInt(data);

    // We deserialize the value before checking the paramter to ensure that
    // remove the unknown value from the stream even if we can't work with it.
    if(!field) {
        return null;
    }

    var value;
    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.INT64:
    case goog.proto2.FieldDescriptor.FieldType.UINT64:
        value = intVal.toString();
        break;
    case goog.proto2.FieldDescriptor.FieldType.INT32:
    case goog.proto2.FieldDescriptor.FieldType.UINT32:
    case goog.proto2.FieldDescriptor.FieldType.ENUM:
        value = intVal;
        break;
    case goog.proto2.FieldDescriptor.FieldType.SINT32:
        var longint = goog.math.Long.fromNumber(intVal);
        value = this.zigZagDecode(longint).toInt();
        break;
    case goog.proto2.FieldDescriptor.FieldType.SINT64:
        var longint = goog.math.Long.fromNumber(intVal);
        value = this.zigZagDecode(longint).toString();
        break;
    case goog.proto2.FieldDescriptor.FieldType.BOOL:
        value = (value === 0 ? false : true);
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
        break;
    }

    return value;
}

/**
 * Deserializes length delimited fields.
 * @param {goog.proto2.FieldDescriptor} field The field to deserialize.
 * @param {!Uint8Array} data The data stream to read from.
 * @return {*} The decoded value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.decodeLengthDelimited =
        function(field, data) {

    var fieldLength = this.decodeVarInt(data);
    var fieldArray = new Uint8Array(data.buffer,
                                    data.byteOffset + this.deserializationIndex_,
                                    fieldLength);
    this.deserializationIndex_ += fieldLength;

    if(!field) {
        return null;
    }

    var value;
    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.STRING:
        value = UTF8ArrayToString(fieldArray);
        break;
    case goog.proto2.FieldDescriptor.FieldType.BYTES:
        value = UTF8ArrayToASCIIString(fieldArray);
        break;
    case goog.proto2.FieldDescriptor.FieldType.MESSAGE:
        var serializer = new goog.proto2.WireFormatSerializer();
        value = serializer.deserialize(field.getFieldMessageType(), fieldArray);
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
        break;
    }

    return value;
}

/**
 * Deserializes a 4 byte value from the stream.
 * @param {goog.proto2.FieldDescriptor} field The field to deserialize.
 * @param {!Uint8Array} data The data stream to read from.
 * @return {*} The decoded value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.decodeFixed32 = function(field, data) {
    var view = new DataView(data.buffer, data.byteOffset+this.deserializationIndex_, 4);
    this.deserializationIndex_ += 4;

    if(!field) {
        return null;
    }

    var value;
    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.FLOAT:
        value = view.getFloat32(0, true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.FIXED32:
        value = view.getUint32(0, true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.SFIXED32:
        value = view.getInt32(0, true);
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
    }
    
    return value;
}

/**
 * Deserializes a 8 byte value from the stream.
 * @param {goog.proto2.FieldDescriptor} field The field to deserialize.
 * @param {!Uint8Array} data The data stream to read from.
 * @return {*} The decoded value.
 * @protected
 */
goog.proto2.WireFormatSerializer.prototype.decodeFixed64 = function(field, data) {
    var view = new DataView(data.buffer, data.byteOffset+this.deserializationIndex_, 8);
    this.deserializationIndex_ += 8;

    if(!field) {
        return null;
    }

    var value;
    switch(field.getFieldType()) {
    case goog.proto2.FieldDescriptor.FieldType.DOUBLE:
        value = view.getFloat64(0, true);
        break;
    case goog.proto2.FieldDescriptor.FieldType.FIXED64:
        value = (new goog.math.Long(view.getUint32(0, true),
                                    view.getUint32(4, true))).toString();
        break;
    case goog.proto2.FieldDescriptor.FieldType.SFIXED64:
        value = (new goog.math.Long(view.getUint32(0, true),
                                    view.getInt32(4, true))).toString();
        break;
    default:
        this.badMessageFormat_("Unexpected field type");
    }
    
    return value;
}

/**
 * Maps field types to wire types.
 * @param {!number} fieldType The field type to map.
 * @return {!number} The wire type mapped to.
 * @protected
 */
goog.proto2.WireFormatSerializer.WireTypeForFieldType = function(fieldType) {
    return goog.proto2.WireFormatSerializer.kWireTypeForFieldType[fieldType];
}

/**
 * Wire format wire types.
 * Should mirror values in wire_format_lite.h
 * @enum {number}
 * @private
 */
goog.proto2.WireFormatSerializer.WireType = {
    VARINT:            0,
    FIXED64:           1,
    LENGTH_DELIMITED:  2,
    START_GROUP:       3,
    END_GROUP:         4,
    FIXED32:           5
};

/**
 * Maps field types to wire types
 * Should mirror value in wire_format_lite.cc
 * @type {Array.<number>}
 * @private
 */
goog.proto2.WireFormatSerializer.kWireTypeForFieldType = [
  -1,                                                          // invalid
  goog.proto2.WireFormatSerializer.WireType.FIXED64,           // DOUBLE
  goog.proto2.WireFormatSerializer.WireType.FIXED32,           // FLOAT
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // INT64
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // UINT64
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // INT32
  goog.proto2.WireFormatSerializer.WireType.FIXED64,           // FIXED64
  goog.proto2.WireFormatSerializer.WireType.FIXED32,           // FIXED32
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // BOOL
  goog.proto2.WireFormatSerializer.WireType.LENGTH_DELIMITED,  // STRING
  goog.proto2.WireFormatSerializer.WireType.START_GROUP,       // GROUP
  goog.proto2.WireFormatSerializer.WireType.LENGTH_DELIMITED,  // MESSAGE
  goog.proto2.WireFormatSerializer.WireType.LENGTH_DELIMITED,  // BYTES
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // UINT32
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // ENUM
  goog.proto2.WireFormatSerializer.WireType.FIXED32,           // SFIXED32
  goog.proto2.WireFormatSerializer.WireType.FIXED64,           // SFIXED64
  goog.proto2.WireFormatSerializer.WireType.VARINT,            // SINT32
  goog.proto2.WireFormatSerializer.WireType.VARINT             // SINT64
];
