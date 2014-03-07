var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 20

var newTarget = NewBuffer(LENGTH * 4)
var oldTarget = OldBuffer(LENGTH * 4)
var nodeTarget = Buffer(LENGTH * 4)

;[newTarget, oldTarget, nodeTarget].forEach(function (buf) {
  for (var i = 0; i < LENGTH; i++) {
    buf.writeUInt32LE(7000 + i, i * 4)
  }
})

suite.add('NewBuffer#readUInt32LE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = newTarget.readUInt32LE(i * 4)
  }
})
.add('OldBuffer#readUInt32LE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = oldTarget.readUInt32LE(i * 4)
  }
})
.add('Buffer#readUInt32LE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = nodeTarget.readUInt32LE(i * 4)
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
