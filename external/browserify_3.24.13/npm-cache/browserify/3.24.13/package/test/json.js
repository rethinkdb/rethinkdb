var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('json', function (t) {
    t.plan(2);
    var b = browserify();
    b.add(__dirname + '/json/main.js');
    b.bundle(function (err, src) {
        if (err) t.fail(err);
        var c = {
            ex : function (obj) {
                t.same(obj, { beep : 'boop', x : 555 });
            }
        };
        vm.runInNewContext(src, c);
    });
});
