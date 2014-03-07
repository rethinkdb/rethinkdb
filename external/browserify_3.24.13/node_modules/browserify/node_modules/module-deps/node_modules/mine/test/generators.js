var test = require('tap').test;
var mine = require('../');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/generators.js');

test('generators', function (t) {
    t.plan(1);
    t.deepEqual(mine(src), [
      {name: 'a', offset: 17}, 
      {name: 'b', offset: 58}
    ]);
});
