var mdeps = require('../');
var test = require('tape');
var JSONStream = require('JSONStream');
var packer = require('browser-pack');
var through = require('through');

test('transform', function (t) {
    t.plan(3);
    var p = mdeps(__dirname + '/files/tr_sh/main.js', {
        transform: function (file) {
            return through(function (buf) {
                this.queue(String(buf)
                    .replace(/AAA/g, '5')
                    .replace(/BBB/g, '50')
                );
            });
        },
        transformKey: [ 'browserify', 'transform' ]
    });
    var pack = packer();
    
    p.pipe(JSONStream.stringify()).pipe(pack);
    
    var src = '';
    pack.on('data', function (buf) { src += buf });
    pack.on('end', function () {
        Function('t', src)(t);
    });
});
