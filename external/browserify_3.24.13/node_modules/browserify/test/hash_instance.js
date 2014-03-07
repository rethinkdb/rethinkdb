var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('hash instances', function (t) {
    t.plan(6);
    
    var b = browserify(__dirname + '/hash_instance/main.js');
    b.bundle(function (err, src) {
        var c = { t: t };
        t.equal(src.match(RegExp('// abcdefg', 'g')).length, 1);
        vm.runInNewContext(src, c);
    });
});
