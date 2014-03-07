var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('multi entry', function (t) {
    t.plan(3);
    
    var b = browserify([
        __dirname + '/multi_entry/a.js',
        __dirname + '/multi_entry/b.js'
    ]);
    b.add(__dirname + '/multi_entry/c.js');
    
    b.bundle(function (err, src) {
        var c = {
            times : 0,
            t : t
        };
        vm.runInNewContext(src, c);
    });
});
