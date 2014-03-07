var test = require('tap').test;
var mine = require('../');
var fs = require('fs');
var src = [ 'require("a")\nreturn' ];

test('return', function (t) {
    t.plan(1);
    t.deepEqual(mine(src), [ 
      {name: 'a', offset: 9}
    ]);
});
