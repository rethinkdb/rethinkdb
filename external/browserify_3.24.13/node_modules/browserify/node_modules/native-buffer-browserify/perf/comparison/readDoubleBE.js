var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 10

var newTarget = NewBuffer(LENGTH * 8)
var oldTarget = OldBuffer(LENGTH * 8)
var nodeTarget = Buffer(LENGTH * 8)

;[newTarget, oldTarget, nodeTarget].forEach(function (buf) {
  for (var i = 0; i < LENGTH; i++) {
    buf.writeDoubleBE(97.1919 + i, i * 8)
  }
})

suite.add('NewBuffer#readDoubleBE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = newTarget.readDoubleBE(i * 8)
  }
})
.add('OldBuffer#readDoubleBE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = oldTarget.readDoubleBE(i * 8)
  }
})
.add('Buffer#readDoubleBE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = nodeTarget.readDoubleBE(i * 8)
  }
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
