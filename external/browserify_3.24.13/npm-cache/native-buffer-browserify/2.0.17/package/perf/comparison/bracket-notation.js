var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 50

var newTarget = NewBuffer(LENGTH)
var oldTarget = OldBuffer(LENGTH)
var nodeTarget = Buffer(LENGTH)

suite.add('NewBuffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    newTarget[i] = i + 97
  }
})
.add('OldBuffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    oldTarget[i] = i + 97
  }
})
.add('Buffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    nodeTarget[i] = i + 97
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
