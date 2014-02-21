![Long.js -A Long class for representing a 64-bit two's-complement integer ](https://raw.github.com/dcodeIO/Long.js/master/long.png)
=======
A Long class for representing a 64-bit two's-complement integer value derived from the [Closure Library](https://code.google.com/p/closure-library/)
for stand-alone use and extended with unsigned support.

Why?
----
As of the [ECMAScript specification](http://ecma262-5.com/ELS5_HTML.htm#Section_8.5), number types have a maximum value
of 2^53. Beyond that, behaviour might be unexpected. Furthermore, bitwise operations can only be performed on 32bit
numbers. However, in some use cases it is required to be able to perform reliable mathematical and/or bitwise operations
on the full 64bits. This is where Long.js comes into play.

Features
--------
* [CommonJS](http://www.commonjs.org/) compatible
* [RequireJS](http://requirejs.org/)/AMD compatible
* Shim compatible (include the script, then use var Long = dcodeIO.Long;)
* [node.js](http://nodejs.org) compatible, also available via [npm](https://npmjs.org/package/long)
* Fully documented using [jsdoc3](https://github.com/jsdoc3/jsdoc)
* API-compatible to the Closure Library implementation
* Zero production dependencies
* Small footprint

Usage
-----

#### node.js / CommonJS ####

Install: `npm install long`

```javascript
var Long = require("long");
var longVal = new Long(0xFFFFFFFF, 0x7FFFFFFF);
console.log(longVal.toString());
...
```

#### RequireJS / AMD ####

````javascript
require.config({
    "paths": {
        "Math/Long": "/path/to/Long.js"
    }
});
require(["Math/Long"], function(Long) {
    var longVal = new Long(0xFFFFFFFF, 0x7FFFFFFF);
    console.log(longVal.toString());
});
````

### Browser / shim ####

```html
<script src="Long.min.js"></script>
```

```javascript
var Long = dcodeIO.Long;
var longVal = new Long(0xFFFFFFFF, 0x7FFFFFFF);
alert(longVal.toString());
```

Documentation
-------------
* [View the API documentation](http://htmlpreview.github.com/?http://github.com/dcodeIO/Long.js/master/docs/Long.html)

Downloads
---------
* [ZIP-Archive](https://github.com/dcodeIO/Long.js/archive/master.zip)
* [Tarball](https://github.com/dcodeIO/Long.js/tarball/master)

License
-------
Apache License, Version 2.0 - http://www.apache.org/licenses/LICENSE-2.0.html
