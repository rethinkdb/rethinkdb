var browserify = require('../');
var test = require('tap').test;

test('standalone bundle close event', {timeout: 1000}, function (t) {
    t.plan(4);

    var ended = false;
    var closed = false;

    var b = browserify(__dirname + '/standalone/main.js');
    b.on('_ready', function() {
        var r = b.bundle({standalone: 'stand-test'});
        r.on('end', function() {
            t.ok(!ended);
            t.ok(!closed);
            ended = true;
        });
        r.on('close', function() {
            t.ok(ended);
            t.ok(!closed);
            closed = true;
            t.end();
        });
    });
});