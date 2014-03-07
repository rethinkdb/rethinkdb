var browserify = require('../');
var test = require('tap').test;

test('bundle in debug mode', function (t) {
    var b = browserify();
    b.require('seq');
    b.bundle({ debug: true }, function (err, src) {
        t.plan(3);
        
        var secondtolastLine = src.split('\n').slice(-2);

        t.ok(typeof src === 'string');
        t.ok(src.length > 0);
        t.ok(/^\/\/# sourceMappingURL=/.test(secondtolastLine), 'includes sourcemap');
    });
});

test('bundle in non debug mode', function (t) {
    var b = browserify();
    b.require('seq');
    b.bundle(function (err, src) {
        t.plan(3);
        
        var secondtolastLine = src.split('\n').slice(-2);

        t.ok(typeof src === 'string');
        t.ok(src.length > 0);
        t.notOk(/^\/\/# sourceMappingURL=/.test(secondtolastLine), 'includes no sourcemap');
    });
});
