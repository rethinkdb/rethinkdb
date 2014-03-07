# umd
<img src="http://i.imgur.com/ypw29XY.png" align="right"/>

Universal Module Definition for use in automated build systems

 - simple synchronous wrapping of a string
 - optional wrapping of a "stream" with genuine streaming
 - `return` style module support
 - CommonJS support
 - prevents internal UMDs from conflicting

[![Build Status](https://travis-ci.org/ForbesLindesay/umd.png?branch=master)](https://travis-ci.org/ForbesLindesay/umd)
[![Dependency Status](https://gemnasium.com/ForbesLindesay/umd.png)](https://gemnasium.com/ForbesLindesay/umd)
[![NPM version](https://badge.fury.io/js/umd.png)](http://badge.fury.io/js/umd)

## Source Format

In order for the UMD wrapper to work the source code for your module should `return` the export, e.g.

```javascript
function method() {
  //code
}
method.helper = function () {
  //code
}
return method;
```

For examples, see the examples directory.  The CommonJS module format is also supported by passing true as the second argument to methods.

## API

### umd(name, [commonJS = false], [source])

  The `name` should the the name of the module.  Use a string like name, all lower case with hyphens instead of spaces.

  If CommonJS is `true` then it will accept CommonJS source instead of source code which `return`s the module.

  If `source` is provided and is a string, then it is wrapped in umd and returned as a string.  If it is not provided, a duplex stream is returned which wraps the modules (see examples/build.js).

  Both commonJS and source are optional and can be provided in either order.

### umd.prelude(module, [commonJS = false])

  return the text which will be inserted before a module.

### umd.postlude(module, [commonJS = false])

  return the text which will be inserted after a module.

## Command Line

```
Usage: umd <name> <source> <destination> [options]

Pipe Usage: umd <name> [options] < source > destination

Options:

 -h --help     Display usage information
 -c --commonJS Use CommonJS module format
 ```

## License

  MIT
