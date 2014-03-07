var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('dollar', function (t) {
    t.plan(3);
    var b = browserify();
    b.expose('dollar', __dirname + '/dollar/dollar/index.js');
    b.bundle(function (err, src) {
        t.ok(typeof src === 'string');
        t.ok(src.length > 0);
        
        var c = {};
        vm.runInNewContext(src, c);
        var res = vm.runInNewContext('require("dollar")(100)', c);
        t.equal(res, 10000);
    });
});
