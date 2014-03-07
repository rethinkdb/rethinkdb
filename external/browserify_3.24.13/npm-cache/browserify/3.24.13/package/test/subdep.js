var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('subdep', function (t) {
    t.plan(1);
    
    var b = browserify();
    b.expose('subdep', __dirname + '/subdep/index.js');
    
    b.bundle(function (err, src) {
        var c = {};
        vm.runInNewContext(src, c);
        t.equal(c.require('subdep'), 'zzz');
    });
});
