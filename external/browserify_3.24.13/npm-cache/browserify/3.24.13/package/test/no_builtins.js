var browserify = require('../');
var test = require('tap').test;
var vm = require('vm');

test(function (t) {
    t.plan(1);
    
    var b = browserify({
        entries: [ __dirname + '/no_builtins/main.js' ],
        commondir: false,
        builtins: false
    });
    b.bundle(function (err, src) {
        var c = {
            console: { log: function (msg) {
                t.equal(msg, 'beep boop\n');
            } },
            require: require
        };
        vm.runInNewContext(src, c);
    });
});
