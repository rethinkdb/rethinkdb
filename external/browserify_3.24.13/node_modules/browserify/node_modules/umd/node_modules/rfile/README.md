# rfile

[![Build Status](https://secure.travis-ci.org/ForbesLindesay/rfile.png)](http://travis-ci.org/ForbesLindesay/rfile)
[![Dependency Status](https://gemnasium.com/ForbesLindesay/rfile.png)](https://gemnasium.com/ForbesLindesay/rfile)

require a plain text or binary file in node.js

## Installation

    $ npm install rfile

## Usage

```javascript
var rfile = require('rfile');

var text = rfile('./my-text-file.txt');
var mochaReadme = rfile('mocha/readme.md');
var mochaSource = rfile('mocha');
var image = rfile('image.png', {binary: true});
```

## API

### rfile(pkg, options)

  Uses `rfile.resolve` (see below) to look up your file `pkg`.  This means it supports all the same options as `rfile.resolve`.  Having found the file, it does the following:

```javascript
return options.binary ? read(path) : fixup(read(path).toString());
```

  `options.binary` defaults to `false` and `fixup` removes the UTF-8 BOM if present and removes any `\r` characters (added to newlines on windows only).

### rfile.resolve(pkg, options)

  Internally, [resolve](https://npmjs.org/package/resolve) is used to lookup your package, so it supports all the same options as that.  In addition t defaults `basedir` to the directory of the function which called `rfile` or `rfile.resolve`.

  The additional option `exclude` is useful if you wanted to create a wrapper arround this.  It specifies the filenames not to consider for `basedir` paths.  For example, you could create a module called `ruglify` for requiring and minifying JavaScript in one go.

  ruglify.js
```javascript
var rfile = require('rfile');
var uglify require('uglify-js').minify;

module.exports = ruglify;
function ruglify(path, options) {
  return minify(rfile.resolve(path, {exclude: [__filename]}), options).code;
}
```

#### From `resolve`

 - opts.basedir - directory to begin resolving from (defaults to `__dirname` of the calling module for `rfile`)
 - opts.extensions - array of file extensions to search in order (defaults to `['.js', '.json']` for `rfile`)
 - opts.readFile - how to read files asynchronously
 - opts.isFile - function to asynchronously test whether a file exists
 - opts.packageFilter - transform the parsed package.json contents before looking at the "main" field (useful for browserify etc.)
 - opts.paths - require.paths array to use if nothing is found on the normal node_modules recursive walk (probably don't use this)

## Notes

One of the interesting features of this is that it respects the `main` field of package.json files.  Say you had a module called `foo`, you could have a package.json like:

```json
{
  "name": "foo",
  "version": "1.0.0",
  "main": "./foo"
}
```

You might then have a `foo.js` file, containing the JavaScript code of the module, and a `foo.css` file containing the stylesheet for the module when used in the browser.  Using `rfile` you could load the css by simply calling:

```javascript
rfile('foo', {extensions: ['.css']});
```

## License

  MIT