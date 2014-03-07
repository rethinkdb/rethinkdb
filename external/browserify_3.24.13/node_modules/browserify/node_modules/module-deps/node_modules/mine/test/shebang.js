var test = require('tap').test;
var mine = require('../');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/shebang.js');

test('shebang', function (t) {
    t.plan(1);
    t.deepEqual(mine(src), [
      {name: 'a', offset: 38}, 
      {name: 'b', offset: 60}, 
      {name: 'c', offset: 82}
    ]);
});
