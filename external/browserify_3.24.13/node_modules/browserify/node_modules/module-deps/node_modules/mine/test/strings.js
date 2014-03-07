var test = require('tap').test;
var detective = require('../');
var fs = require('fs');
var src = fs.readFileSync(__dirname + '/files/strings.js');

test('single', function (t) {
    t.deepEqual(detective(src), [
      {name: 'a', offset: 17}, 
      {name: 'b', offset: 39}, 
      {name: 'c', offset: 61}, 
      {name: 'events', offset: 113}, 
      {name: 'doom', offset: 154},
      {name: 'y', offset: 216}, 
      {name: 'events2', offset: 257}
    ]);
    t.end();
});
