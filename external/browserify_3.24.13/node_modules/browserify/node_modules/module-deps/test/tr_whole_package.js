var mdeps = require('../');
var test = require('tape');
var JSONStream = require('JSONStream');
var packer = require('browser-pack');

test('transform', function (t) {
    t.plan(1);
    var p = mdeps(__dirname + '/files/tr_whole_package/main.js', {
        transformKey: [ 'browserify', 'transform' ]
    });
    var pack = packer();
    
    p.pipe(JSONStream.stringify()).pipe(pack);
    
    var src = '';
    pack.on('data', function (buf) { src += buf });
    pack.on('end', function () {
        Function('t', src)(t);
    });
});
