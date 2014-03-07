var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 10

var newTarget = NewBuffer(LENGTH * 4)

suite.add('NewBuffer#writeFloatBE', function () {
  for (var i = 0; i < LENGTH; i++) {
    newTarget.writeFloatBE(97.1919 + i, i * 4)
  }
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })
