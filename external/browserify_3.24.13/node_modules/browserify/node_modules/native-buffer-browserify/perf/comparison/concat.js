var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 16

var newBuf = NewBuffer(LENGTH)
var newBuf2 = NewBuffer(LENGTH)
var oldBuf = OldBuffer(LENGTH)
var oldBuf2 = OldBuffer(LENGTH)
var nodeBuf = Buffer(LENGTH)
var nodeBuf2 = Buffer(LENGTH)

;[newBuf, newBuf2, oldBuf, oldBuf2, nodeBuf, nodeBuf2].forEach(function (buf) {
  for (var i = 0; i < LENGTH; i++) {
    buf[i] = 42
  }
})

suite.add('NewBuffer#concat', function () {
  var x = Buffer.concat([newBuf, newBuf2])
})
.add('OldBuffer#concat', function () {
  var x = Buffer.concat([oldBuf, oldBuf2])
})
.add('Buffer#concat', function () {
  var x = Buffer.concat([nodeBuf, nodeBuf2])
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})
.on('complete', function () {
  console.log('Fastest is ' + this.filter('fastest').pluck('name'))
})
.run({ 'async': true })