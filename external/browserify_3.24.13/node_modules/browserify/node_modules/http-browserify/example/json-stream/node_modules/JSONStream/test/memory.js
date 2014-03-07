var JSONStream = require('..')
var from       = require('from')
var assert     = require('assert')
var probe      = require('probe-stream')({interval: 1000})

var letters = '.pyfgcrlaoeuidhthtnsqjbmwvmbbkjqAOFEXACGOBQRCBK>RCMORPKGPOCRKB'

function randWord (l) {
  var s = ''
  l = l || 1000
  while (l --)
    s = letters.substring(~~(Math.random()*letters.length))
  return s
}

function randObj (d) {
  if (0 >= d) return []
  return {
      row: d,
      timestamp: Date.now(),
      date: new Date(),
      thing: Math.random() < 0.3 ? {} : randObj(d - 1),
      array: [Math.random(), Math.random(), Math.random()],
      object: {
        A: '#'+Math.random(),
        B: randWord(),
        C: Math.random() < 0.1 ? {} : randObj(d - 1)
      }
    }
}

var objs = []
var l = 6
while(l --)
  objs.push(new Buffer(JSON.stringify(randObj(l * 3))))

//console.log('objs', objs)

//return
var I = 0
from(function (i, next) {
    if(i > 1000) return this.emit('data', ']'), this.emit('end')
    if(!i) this.emit('data', '[\n')
    I = i
    this.emit('data', objs[~~(Math.random()*objs.length)])
    this.emit('data', '\n,\n')

//    if(i % 1000) return true
    process.nextTick(next)
  })
//  .pipe(process.stdout)

//  .pipe(JSONStream.stringify())
//  .on('data', console.log)
  .pipe(probe.createProbe())
  .pipe(JSONStream.parse())
//  .on('end', probe.end.bind(probe))

//  .pipe(fs.createWriteStream('/tmp/test-reverse'))
//

setInterval(function () {
  var mem = process.memoryUsage()
  console.log(mem, I)
  if(mem.heapUsed > 200000000)
    throw new Error('too much memory used')
  console.log(mem)
}, 1e3)
//*/
