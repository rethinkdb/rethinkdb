var test = require('tap').test;
var mdeps = require('module-deps');
var bpack = require('browser-pack');
var insert = require('../');
var concat = require('concat-stream');
var vm = require('vm');

test('early return', function (t) {
    t.plan(4);
    var files = [ __dirname + '/return/main.js' ];
    var s = mdeps(files, { transform: [ inserter ] })
        .pipe(bpack({ raw: true }))
    ;
    s.pipe(concat(function (src) {
        var c = { t: t, setTimeout: setTimeout };
        vm.runInNewContext(src, c);
    }));
});

function inserter (file) {
    return insert(file, {
        basedir: __dirname + '/return'
    });
}
