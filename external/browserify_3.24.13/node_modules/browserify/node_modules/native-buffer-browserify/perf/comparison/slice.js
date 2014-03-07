var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 16

var newBuf = NewBuffer(LENGTH)
var oldBuf = OldBuffer(LENGTH)
var nodeBuf = Buffer(LENGTH)

suite.add('NewBuffer#slice', function () {
  var x = newBuf.slice(4)
})
.add('OldBuffer#slice', function () {
  var x = oldBuf.slice(4)
})
.add('Buffer#slice', function () {
  var x = nodeBuf.slice(4)
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