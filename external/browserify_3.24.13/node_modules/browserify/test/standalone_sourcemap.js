var browserify = require('../');
var fs = require('fs');
var vm = require('vm');
var test = require('tap').test;
var derequire = require('derequire');

test('standalone in debug mode', function (t) {
    t.plan(4);

    var main = fs.readFileSync(__dirname + '/standalone/main.js');

    var b = browserify(__dirname + '/standalone/main.js');
    b.bundle({standalone: 'stand-test', debug: true}, function (err, src) {
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
        t.equal(0, src.split('\n').slice(1).join('\n').indexOf(derequire("!function(require){"+main+"}").slice(19,-1)),
                'preserves preamble line count');
    });
});

function done(t) {
    return function (one, two) {
        t.equal(one, 1);
        t.equal(two, 2);
        t.end();
    };
}

