var test = require('tap').test;
var mine = require('../');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/yield.js');

test('yield', function (t) {
    t.plan(1);
    t.deepEqual(mine(src), [
      {name: 'a', offset: 17}, 
      {name: 'c', offset: 45}
    ]);
});
