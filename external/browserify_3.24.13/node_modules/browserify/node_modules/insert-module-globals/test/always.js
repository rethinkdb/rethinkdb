var test = require('tap').test;
var mdeps = require('module-deps');
var bpack = require('browser-pack');
var insert = require('../');
var concat = require('concat-stream');
var vm = require('vm');

test('always insert', function (t) {
    t.plan(6);
    var files = [ __dirname + '/always/main.js' ];
    var s = mdeps(files, {
        transform: inserter,
        modules: {
            buffer: require.resolve('native-buffer-browserify')
        }
    });
    s.pipe(bpack({ raw: true })).pipe(concat(function (src) {
        var c = {
            t: t,
            self: { xyz: 555 }
        };
        vm.runInNewContext(src, c);
    }));
});

function inserter (file) {
    return insert(file, { always: true });
}
