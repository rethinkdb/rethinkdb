var mdeps = require('../');
var test = require('tape');
var JSONStream = require('JSONStream');
var packer = require('browser-pack');
var concat = require('concat-stream');

test('pkg filter', function (t) {
    t.plan(2);
    
    var p = mdeps(__dirname + '/files/pkg_filter/test.js', {
        packageFilter: function (pkg) {
            t.equal(pkg.main, 'one.js');
            pkg.main = 'two.js'
            return pkg;
        }
    });
    var pack = packer();
    p.pipe(JSONStream.stringify()).pipe(pack);
    
    pack.pipe(concat(function (src) {
        Function('t', src)(t);
    }));
});
