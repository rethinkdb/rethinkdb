var test = require('tap').test;
var mine = require('../mine');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/chained.js');

test('chained', function (t) {
    t.deepEqual(mine(src), [
        {name: 'c', offset: 11}, 
        {name: 'b', offset: 42},
        {name: 'a', offset: 63}
    ]);
    t.end();
});
