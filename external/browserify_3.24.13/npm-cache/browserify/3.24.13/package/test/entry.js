var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('entry', function (t) {
    t.plan(2);
    
    var b = browserify(__dirname + '/entry/main.js');
    b.bundle(function (err, src) {
        var c = {
            done : function (one, two) {
                t.equal(one, 1);
                t.equal(two, 2);
                t.end();
            }
        };
        vm.runInNewContext(src, c);
    });
});
