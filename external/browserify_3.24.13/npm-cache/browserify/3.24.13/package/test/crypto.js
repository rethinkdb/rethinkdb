var test = require('tap').test;
var spawn = require('child_process').spawn;
var path = require('path');
var fs = require('fs');
var vm = require('vm');
var concat = require('concat-stream');

var mkdirp = require('mkdirp');
var tmpdir = '/tmp/browserify-test/' + Math.random().toString(16).slice(2);
mkdirp.sync(tmpdir);

fs.writeFileSync(tmpdir + '/main.js', 'beep(require("crypto"))\n');

test('*-browserify libs from node_modules/', function (t) {
    t.plan(2);
    
    var bin = __dirname + '/../bin/cmd.js';
    var ps = spawn(bin, [ 'main.js' ], { cwd : tmpdir });
    
    ps.stderr.pipe(process.stderr, { end : false });
    
    ps.on('exit', function (code) {
        t.equal(code, 0);
    });
    
    ps.stdout.pipe(concat(function (src) {
        var c = {
            ArrayBuffer: ArrayBuffer,
            Uint8Array: Uint8Array,
            DataView: DataView,
            beep : function (c) {
                t.equal(typeof c.createHash, 'function');
            }
        };
        vm.runInNewContext(src.toString('utf8'), c);
    }));
});
