var parser = require('../');
var test = require('tape');
var JSONStream = require('JSONStream');
var packer = require('browser-pack');
var through = require('through');
var concat = require('concat-stream');

test('read from a stream', function (t) {
    t.plan(1);
    var tr = through();
    var p = parser(tr);
    
    p.on('error', t.fail.bind(t));
    var pack = packer();
    
    p.pipe(JSONStream.stringify()).pipe(pack)
        .pipe(concat({ encoding: 'string' }, function (src) {
            Function(['t'], src)(t);
        }))
    ;
    
    tr.queue('t.ok(true)');
    tr.queue(null);
});
