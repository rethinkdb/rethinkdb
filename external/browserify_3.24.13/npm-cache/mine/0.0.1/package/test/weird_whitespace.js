var test = require('tap').test;
var mine = require('../');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/weird_whitespace.js');

test('nested', function (t) {
    t.deepEqual(mine(src), [
      {name: 'c', offset: 49}
    ]);
    t.end();
});
