var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 50

var newTarget = NewBuffer(LENGTH)

suite.add('NewBuffer#bracket-notation', function () {
  for (var i = 0; i < LENGTH; i++) {
    newTarget[i] = i + 97
  }
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })
