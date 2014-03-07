var mdeps = require('../');
var test = require('tape');
var JSONStream = require('JSONStream');
var packer = require('browser-pack');
var concat = require('concat-stream');

test('global transforms', function (t) {
    t.plan(1);
    
    var p = mdeps(__dirname + '/files/tr_global/main.js', {
        transform: [ 'tr-c', 'tr-d' ],
        globalTransform: [
            __dirname + '/files/tr_global/node_modules/tr-e',
            __dirname + '/files/tr_global/node_modules/tr-f'
        ],
        transformKey: [ 'browserify', 'transform' ]
    });
    var pack = packer();
    
    p.pipe(JSONStream.stringify()).pipe(pack).pipe(concat(function (src) {
        Function(['console'], src)({
            log: function (msg) {
                t.equal(msg, 111111);
            }
        });
    }));
});
