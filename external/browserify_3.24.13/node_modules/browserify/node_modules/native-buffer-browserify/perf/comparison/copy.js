var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify
global.OldBuffer = require('buffer-browserify').Buffer // buffer-browserify

var LENGTH = 10

var newSubject = NewBuffer(LENGTH)
var oldSubject = OldBuffer(LENGTH)
var nodeSubject = Buffer(LENGTH)

var newTarget = NewBuffer(LENGTH)
var oldTarget = OldBuffer(LENGTH)
var nodeTarget = Buffer(LENGTH)

suite.add('NewBuffer#copy', function () {
  newSubject.copy(newTarget)
})
.add('OldBuffer#copy', function () {
  oldSubject.copy(oldTarget)
})
.add('Buffer#copy', function () {
  nodeSubject.copy(nodeTarget)
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