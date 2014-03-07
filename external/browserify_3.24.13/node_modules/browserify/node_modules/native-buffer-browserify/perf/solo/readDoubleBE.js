var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 10

var newTarget = NewBuffer(LENGTH * 8)

for (var i = 0; i < LENGTH; i++) {
  newTarget.writeDoubleBE(97.1919 + i, i * 8)
}

suite.add('NewBuffer#readDoubleBE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = newTarget.readDoubleBE(i * 8)
  }
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })
