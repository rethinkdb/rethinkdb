var vm = require('vm');
var browserify = require('../');
var test = require('tap').test;

test('dnode', function (t) {
    t.plan(3);
    
    var b = browserify();
    b.require('dnode');
    
    var c = {
        console: console,
        setTimeout: setTimeout
    };
    
    b.bundle(function (err, src) {
        vm.runInNewContext(src, c);
        var dnode = c.require('dnode');
        
        t.equal(typeof dnode, 'function', 'dnode object exists');
        t.equal(
            dnode.connect, undefined,
            "dnode.connect doesn't exist in browsers"
        );
        t.ok(dnode().pipe, "dnode() is pipe-able");
    });
});
