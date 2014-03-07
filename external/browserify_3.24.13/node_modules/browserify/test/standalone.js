var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('standalone', function (t) {
    t.plan(3);
    
    var b = browserify(__dirname + '/standalone/main.js');
    b.bundle({standalone: 'stand-test'}, function (err, src) {
        t.test('window global', function (t) {
            t.plan(2);
            var c = {
                window: {},
                done : done(t)
            };
            vm.runInNewContext(src + 'window.standTest(done)', c);
        });
        t.test('CommonJS', function (t) {
            t.plan(2);
            var exp = {};
            var c = {
                module: { exports: exp },
                exports: exp,
                done : done(t)
            };
            vm.runInNewContext(src + 'module.exports(done)', c);
        });
        t.test('RequireJS', function (t) {
            t.plan(2);
            var c = {
                define: function (fn) {
                    fn()(done(t));
                }
            };
            c.define.amd = true;
            vm.runInNewContext(src, c);
        });
    });
});

test('A.B.C standalone', function (t) {
    t.plan(3);
    
    var b = browserify(__dirname + '/standalone/main.js');
    b.bundle({standalone: 'A.B.C'}, function (err, src) {
        t.test('window global', function (t) {
            t.plan(2);
            var c = { window: {} };
            vm.runInNewContext(src, c);
            c.window.A.B.C(done(t));
        });
        t.test('CommonJS', function (t) {
            t.plan(2);
            var exp = {};
            var c = {
                module: { exports: exp },
                exports: exp
            };
            vm.runInNewContext(src, c);
            c.module.exports(done(t));
        });
        t.test('RequireJS', function (t) {
            t.plan(2);
            var c = {
                define: function (fn) {
                    fn()(done(t));
                }
            };
            c.define.amd = true;
            vm.runInNewContext(src, c);
        });
    });
});

function done(t) {
    return function (one, two) {
        t.equal(one, 1);
        t.equal(two, 2);
        t.end();
    };
}
