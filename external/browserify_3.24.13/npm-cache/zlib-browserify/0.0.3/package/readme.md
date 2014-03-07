# zlib-browserify [![Build Status](https://travis-ci.org/brianloveswords/zlib-browserify.png?branch=master)](https://travis-ci.org/brianloveswords/zlib-browserify)

Zlib in yo' browser.

# What is this?

This is a very small wrapper for https://github.com/imaya/zlib.js. Fixes
some very minor API inconsistencies. Only implements `inflate`,
`deflate`, `gzip` and `gunzip` so if you're doing anything extra fancy
you're out of luck for now.

# Run tests

```bash
$ npm test
```

# Test methodology (a.k.a, "why doesn't the output match node's zlib?")

(zlibA = native, zlibB = browserified)

Pretending these are sync, I do the following to test:

```js
assert(zlibB.inflate(zlibA.deflate('test')) === "test");
assert(zlibA.inflate(zlibB.deflate('test')) === "test");
...
```

and so on for each of the methods supported. Note, I do **not** do 

```js
assert(zlibA.deflate('test') === zlibB.deflate('test'));
```

Because node's deflate and the imaya's version do not seem to use the
same defaults for compression level. I haven't figured out how to change
it yet. Anyway, rest assured, while `deflate` does not return the same
output, this is fully interoperable with node's native zlib.

# License

MIT
