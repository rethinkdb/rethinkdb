var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 16

var newBuf = NewBuffer(LENGTH)
var newBuf2 = NewBuffer(LENGTH)

;[newBuf, newBuf2].forEach(function (buf) {
  for (var i = 0; i < LENGTH; i++) {
    buf[i] = 42
  }
})

suite.add('NewBuffer#concat', function () {
  var x = Buffer.concat([newBuf, newBuf2])
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })