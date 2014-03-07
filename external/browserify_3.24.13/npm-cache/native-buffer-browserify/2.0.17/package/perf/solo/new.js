var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 10

suite.add('NewBuffer#new', function () {
  var buf = NewBuffer(LENGTH)
})
.add('Uint8Array#new', function () {
  var buf = new Uint8Array(LENGTH)
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })