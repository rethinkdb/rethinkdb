var test = require('tap').test;
var mine = require('../');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/nested.js');

test('nested', function (t) {
    t.deepEqual(mine(src), [
      {name: 'a', offset: 49}, 
      {name: 'b', offset: 169}, 
      {name: 'c', offset: 281} ]);
    t.end();
});
