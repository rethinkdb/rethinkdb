# concat-stream

Writable stream that concatenates strings or binary data and calls a callback with the result. Not a transform stream -- more of a stream sink.

[![NPM](https://nodei.co/npm/concat-stream.png)](https://nodei.co/npm/concat-stream/)

[![browser support](https://ci.testling.com/maxogden/concat-stream.png)](https://ci.testling.com/maxogden/concat-stream)

### examples

#### Buffers

```js
var concat = require('concat-stream')
var fs = require('fs')
    
var read = fs.createReadStream('readme.md')
var write = concat(function(data) {
  // data is all of readme.md as a Buffer
})
    
read.pipe(write)
```

#### Arrays

```js
var write = concat(function(data) {})
write.write([1,2,3])
write.write([4,5,6])
write.end()
// data will be [1,2,3,4,5,6] in the above callback
```

#### Uint8Arrays

```js
var write = concat(function(data) {})
var a = new Uint8Array(3)
a[0] = 97; a[1] = 98; a[2] = 99
write.write(a)
write.write('!')
write.end(Buffer('!!1'))
```

See `test/` for more examples

# methods

```js
var concat = require('concat-stream')
```

## var writable = concat(opts={}, cb)

Return a `writable` stream that will fire `cb(data)` with all of the data that
was written to the stream. Data can be written to `writable` as strings,
Buffers, arrays of byte integers, and Uint8Arrays. 

By default `concat-stream` will give you back the same data type as the type of the first buffer written to the stream. Use `opts.encoding` to set what format `data` should be returned as, e.g. if you if you don't want to rely on the built-in type checking or for some other reason.

* `string` - get a string
* `buffer` - get back a Buffer
* `array` - get an array of byte integers
* `uint8array`, `u8`, `uint8` - get back a Uint8Array
* `object`, get back an array of Objects

If you don't specify an encoding, and the types can't be inferred (e.g. you write things that aren't int he list above), it will try to convert concat them into a `Buffer`.

# license

MIT LICENSE
