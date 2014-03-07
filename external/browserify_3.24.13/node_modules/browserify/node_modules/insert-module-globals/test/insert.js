var test = require('tap').test;
var mdeps = require('module-deps');
var bpack = require('browser-pack');
var insert = require('../');
var concat = require('concat-stream');
var vm = require('vm');

test('process.nextTick inserts', function (t) {
    t.plan(4);
    var files = [ __dirname + '/insert/main.js' ];
    var s = mdeps(files, { transform: [ inserter ] })
        .pipe(bpack({ raw: true }))
    ;
    s.pipe(concat(function (src) {
        var c = { t: t, setTimeout: setTimeout };
        vm.runInNewContext(src, c);
    }));
});

test('buffer inserts', function (t) {
    t.plan(2);
    var files = [ __dirname + '/insert/buffer.js' ];
    var s = mdeps(files, {
        transform: [ inserter ],
        modules: { buffer: require.resolve('native-buffer-browserify') }
    });
    s.pipe(bpack({ raw: true })).pipe(concat(function (src) {
        var c = {
            t: t,
            setTimeout: setTimeout,
            Uint8Array: Uint8Array,
            DataView: DataView
        };
        vm.runInNewContext(src, c);
    }));
});

function inserter (file) {
    return insert(file, {
        basedir: __dirname + '/insert'
    });
}
