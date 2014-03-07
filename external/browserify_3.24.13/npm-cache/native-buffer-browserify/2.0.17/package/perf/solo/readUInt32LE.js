var benchmark = require('benchmark')
var suite = new benchmark.Suite()

global.NewBuffer = require('../../').Buffer // native-buffer-browserify

var LENGTH = 20

var newTarget = NewBuffer(LENGTH * 4)

for (var i = 0; i < LENGTH; i++) {
  newTarget.writeUInt32LE(7000 + i, i * 4)
}

suite.add('NewBuffer#readUInt32LE', function () {
  for (var i = 0; i < LENGTH; i++) {
    var x = newTarget.readUInt32LE(i * 4)
  }
})
.on('error', function (event) {
  console.error(event.target.error.stack)
})
.on('cycle', function (event) {
  console.log(String(event.target))
})

.run({ 'async': true })
