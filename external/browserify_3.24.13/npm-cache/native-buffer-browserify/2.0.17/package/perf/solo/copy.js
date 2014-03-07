var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 10

var newSubject = NewBuffer(LENGTH)
var newTarget = NewBuffer(LENGTH)

suite.add('NewBuffer#copy', function () {
  newSubject.copy(newTarget)
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })