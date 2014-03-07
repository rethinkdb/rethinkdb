var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 10

suite.add('NewBuffer#new', function () {
  var buf = NewBuffer(LENGTH)
})
.add('Uint8Array#new', function () {
  var buf = new Uint8Array(LENGTH)
})
.add('OldBuffer#new', function () {
  var buf = OldBuffer(LENGTH)
})
.add('Buffer#new', function () {
  var buf = Buffer(LENGTH)
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