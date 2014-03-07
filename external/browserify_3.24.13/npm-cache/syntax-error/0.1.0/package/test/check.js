var test = require('tape');

var fs = require('fs');
var check = require('../');

var file = __dirname + '/sources/check.js';
var src = fs.readFileSync(file);

test(function (t) {
    var err = check(src, file);
    t.ok(err);
    t.equal(err.line, 5);
    t.equal(err.column, 30);
    t.equal(err.message, 'Unexpected identifier');
    t.ok(String(err).indexOf(file) + ':5');
    t.end();
});
