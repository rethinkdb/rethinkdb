![ByteBuffer.js - A full-featured ByteBuffer implementation in JavaScript](https://raw.github.com/dcodeIO/ByteBuffer.js/master/ByteBuffer.png)
======================================
Provides a full-featured ByteBuffer implementation using typed arrays. It's one of the core components driving
[ProtoBuf.js](https://github.com/dcodeIO/ProtoBuf.js) and the [PSON](https://github.com/dcodeIO/PSON) reference
implementation.

*Note:* The API behind #toHex and #toString has changed with ByteBuffer 2, which is a generally revised release, in
favor of making this more intuitive.

What can it do?
---------------
* Mimics Java ByteBuffers as close as reasonable while using typed array terms
* Signed and unsigned integers (8, 16, 32, 64 bit through [Long.js](https://github.com/dcodeIO/Long.js)) with endianness support
* 32 and 64 bit floats
* Varints as known from protobuf including zig-zag encoding
* Includes an UTF8 and Base64 en-/decoder
* C-strings, V(arint-prefixed)-strings and UTF8 L(ength-prefixed)-strings 
* Rich string toolset (to hex, base64, binary, utf8, debug, columns)
* Relative and absolute zero-copy operations
* Manual and automatic resizing (efficiently doubles capacity)
* Chaining of all operations that do not return a specific value
* Slicing, appending, prepending, reversing, flip, mark, reset, etc.

And much more...

Features
--------
* [CommonJS](http://www.commonjs.org/) compatible
* [RequireJS](http://requirejs.org/)/AMD compatible
* [node.js](http://nodejs.org) compatible, also available via [npm](https://npmjs.org/package/bytebuffer)
* Browser compatible
* [Closure Compiler](https://developers.google.com/closure/compiler/) ADVANCED_OPTIMIZATIONS compatible (fully annotated,
  `ByteBuffer.min.js` has been compiled this way, `ByteBuffer.min.map` is the source map)
* Fully documented using [jsdoc3](https://github.com/jsdoc3/jsdoc)
* Well tested through [nodeunit](https://github.com/caolan/nodeunit)
* Zero production dependencies (Long.js is optional)
* Small footprint

Usage
-----
### Node.js / CommonJS ###
* Install: `npm install bytebuffer`

```javascript
var ByteBuffer = require("bytebuffer");
var bb = new ByteBuffer();
bb.writeLString("Hello world!").flip();
console.log(bb.readLString()+" from ByteBuffer.js");
```

### Browser ###

Optionally depends on [Long.js](https://github.com/dcodeIO/Long.js) for long (int64) support. If you do not require long
support, you can skip the Long.js include.

```html
<script src="Long.min.js"></script>
<script src="ByteBuffer.min.js"></script>
```

```javascript
var ByteBuffer = dcodeIO.ByteBuffer;
var bb = new ByteBuffer();
bb.writeLString("Hello world!").flip();
alert(bb.readLString()+" from ByteBuffer.js");
```

### Require.js / AMD ###

Optionally depends on [Long.js](https://github.com/dcodeIO/Long.js) for long (int64) support. If you do not require long
support, you can skip the Long.js config. [Require.js](http://requirejs.org/) example:

```javascript
require.config({
    "paths": {
        "Long": "/path/to/Long.js"
        "ByteBuffer": "/path/to/ByteBuffer.js"
    }
});
require(["ByteBuffer"], function(ByteBuffer) {
    var bb = new ByteBuffer();
    bb.writeLString("Hello world!");
    bb.flip();
    alert(bb.readLString()+" from ByteBuffer.js");
});
```

On long (int64) support
-----------------------
As of the [ECMAScript specification](http://ecma262-5.com/ELS5_HTML.htm#Section_8.5), number types have a maximum value
of 2^53. Beyond that, behaviour might be unexpected. However, real long support requires the full 64 bits
with the possibility to perform bitwise operations on the value for varint en-/decoding. So, to enable true long support
in ByteBuffer.js, it optionally depends on [Long.js](https://github.com/dcodeIO/Long.js), which actually utilizes two
32 bit numbers internally. If you do not require long support at all, you can skip it and save the additional bandwidth.
On node, long support is available by default through the [long](https://npmjs.org/package/long) dependency.

Downloads
---------
* [ZIP-Archive](https://github.com/dcodeIO/ByteBuffer.js/archive/master.zip)
* [Tarball](https://github.com/dcodeIO/ByteBuffer.js/tarball/master)

Documentation
-------------
* [View the API documentation](http://htmlpreview.github.com/?http://github.com/dcodeIO/ByteBuffer.js/master/docs/ByteBuffer.html)

Tests (& Examples) [![Build Status](https://travis-ci.org/dcodeIO/ByteBuffer.js.png?branch=master)](https://travis-ci.org/dcodeIO/ByteBuffer.js)
------------------
* [View source](https://github.com/dcodeIO/ByteBuffer.js/blob/master/tests/suite.js)
* [View report](https://travis-ci.org/dcodeIO/ByteBuffer.js)

Support for IE<10, FF<15, Chrome<9 etc.
---------------------------------------
* Requires working ArrayBuffer & DataView implementations (i.e. use a [polyfill](https://github.com/inexorabletash/polyfill#typed-arrays-polyfill))

Contributors
------------
[Dretch](https://github.com/Dretch) (IE8 compatibility)

License
-------
Apache License, Version 2.0 - http://www.apache.org/licenses/LICENSE-2.0.html
