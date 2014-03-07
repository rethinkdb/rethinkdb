# ieee754
[![Build Status](http://img.shields.io/travis/feross/ieee754.svg)](https://travis-ci.org/feross/ieee754)
[![NPM Version](http://img.shields.io/npm/v/ieee754.svg)](https://npmjs.org/package/ieee754)
[![NPM](http://img.shields.io/npm/dm/ieee754.svg)](https://npmjs.org/package/ieee754)
[![Gittip](http://img.shields.io/gittip/feross.svg)](https://www.gittip.com/feross/)

## Read/write IEEE754 floating point numbers from/to a Buffer or array-like object.

[![testling badge](https://ci.testling.com/feross/ieee754.png)](https://ci.testling.com/feross/ieee754)

## install

```
npm install ieee754
```

## methods

`var ieee754 = require('ieee754')`

The `ieee754` object has the following functions:

```
ieee754.read = function (buffer, offset, isLE, mLen, nBytes)
ieee754.write = function (buffer, value, offset, isLE, mLen, nBytes)
```

## ieee754?

The IEEE Standard for Floating-Point Arithmetic (IEEE 754) is a technical standard for floating-point computation. [Read more](http://en.wikipedia.org/wiki/IEEE_floating_point).

## license

MIT. Copyright (C) 2013 [Feross Aboukhadijeh](http://feross.org) & Romain Beauxis.
