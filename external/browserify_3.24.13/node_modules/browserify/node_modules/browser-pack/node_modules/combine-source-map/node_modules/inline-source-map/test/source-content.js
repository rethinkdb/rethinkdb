'use strict';
/*jshint asi: true*/

var test = require('tap').test
var generator = require('..');

var foo = '' + function foo () {
  var hello = 'hello';
  var world = 'world';
  console.log('%s %s', hello, world);
}

var bar = '' + function bar () {
  console.log('yes?');
}

function decode(base64) {
  return new Buffer(base64, 'base64').toString();
} 

function inspect(obj, depth) {
  console.log(require('util').inspect(obj, false, depth || 5, true));
}

test('generated mappings', function (t) {

  t.test('one file with source content', function (t) {
    var gen = generator()
      .addGeneratedMappings('foo.js', foo)
      .addSourceContent('foo.js', foo)

    t.deepEqual(
        gen.toJSON()
      , { "version": 3,
          "file": "",
          "sources": [
            "foo.js"
          ],
          "names": [],
          "mappings": "AAAA;AACA;AACA;AACA;AACA",
          "sourcesContent": [
            "function foo() {\n  var hello = 'hello';\n  var world = 'world';\n  console.log('%s %s', hello, world);\n}"
          ]
        }
      , 'includes source content'
    )

    t.deepEqual(
        decode(gen.base64Encode()) 
      , '{"version":3,"file":"","sources":["foo.js"],"names":[],"mappings":"AAAA;AACA;AACA;AACA;AACA","sourcesContent":["function foo() {\\n  var hello = \'hello\';\\n  var world = \'world\';\\n  console.log(\'%s %s\', hello, world);\\n}"]}'
      , 'encodes generated mappings including source content'
    )
    t.equal(
        gen.inlineMappingUrl()
      , '//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiIiwic291cmNlcyI6WyJmb28uanMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFBQUE7QUFDQTtBQUNBO0FBQ0E7QUFDQSIsInNvdXJjZXNDb250ZW50IjpbImZ1bmN0aW9uIGZvbygpIHtcbiAgdmFyIGhlbGxvID0gJ2hlbGxvJztcbiAgdmFyIHdvcmxkID0gJ3dvcmxkJztcbiAgY29uc29sZS5sb2coJyVzICVzJywgaGVsbG8sIHdvcmxkKTtcbn0iXX0='
      , 'returns correct inline mapping url including source content'
    )
    t.end()
  })

  t.test('two files with source content', function (t) {
    var gen = generator()
      .addGeneratedMappings('foo.js', foo)
      .addSourceContent('foo.js', foo)
      .addGeneratedMappings('bar.js', bar)
      .addSourceContent('bar.js', bar)

    t.deepEqual(
        gen.toJSON()
      ,  { "version": 3,
          "file": "",
          "sources": [
            "foo.js",
            "bar.js"
          ],
          "names": [],
          "mappings": "ACAA,ADAA;ACCA,ADAA;ACCA,ADAA;AACA;AACA",
          "sourcesContent": [
            "function foo() {\n  var hello = 'hello';\n  var world = 'world';\n  console.log('%s %s', hello, world);\n}",
            "function bar() {\n  console.log('yes?');\n}"
          ]
        }
      , 'includes source content for both files'
    )

    t.deepEqual(
        decode(gen.base64Encode()) 
      , '{"version":3,"file":"","sources":["foo.js","bar.js"],"names":[],"mappings":"ACAA,ADAA;ACCA,ADAA;ACCA,ADAA;AACA;AACA","sourcesContent":["function foo() {\\n  var hello = \'hello\';\\n  var world = \'world\';\\n  console.log(\'%s %s\', hello, world);\\n}","function bar() {\\n  console.log(\'yes?\');\\n}"]}'
      , 'encodes generated mappings including source content'
    )
    t.equal(
        gen.inlineMappingUrl()
      , '//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiIiwic291cmNlcyI6WyJmb28uanMiLCJiYXIuanMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFDQUEsQURBQTtBQ0NBLEFEQUE7QUNDQSxBREFBO0FBQ0E7QUFDQSIsInNvdXJjZXNDb250ZW50IjpbImZ1bmN0aW9uIGZvbygpIHtcbiAgdmFyIGhlbGxvID0gJ2hlbGxvJztcbiAgdmFyIHdvcmxkID0gJ3dvcmxkJztcbiAgY29uc29sZS5sb2coJyVzICVzJywgaGVsbG8sIHdvcmxkKTtcbn0iLCJmdW5jdGlvbiBiYXIoKSB7XG4gIGNvbnNvbGUubG9nKCd5ZXM/Jyk7XG59Il19'
      , 'returns correct inline mapping url including source content'
    )
    t.end()
  })

  t.test('two files, only one with source content', function (t) {
    var gen = generator()
      .addGeneratedMappings('foo.js', foo)
      .addGeneratedMappings('bar.js', bar)
      .addSourceContent('bar.js', bar)

    t.deepEqual(
        gen.toJSON()
      ,  { "version": 3,
          "file": "",
          "sources": [
            "foo.js",
            "bar.js"
          ],
          "names": [],
          "mappings": "ACAA,ADAA;ACCA,ADAA;ACCA,ADAA;AACA;AACA",
          "sourcesContent": [ null, "function bar() {\n  console.log('yes?');\n}" ]
        }
      , 'includes source content for the file with source content and [null] for the other file'
    )

    t.deepEqual(
        decode(gen.base64Encode()) 
      , '{"version":3,"file":"","sources":["foo.js","bar.js"],"names":[],"mappings":"ACAA,ADAA;ACCA,ADAA;ACCA,ADAA;AACA;AACA","sourcesContent":[null,"function bar() {\\n  console.log(\'yes?\');\\n}"]}'
      , 'encodes generated mappings including source content'
    )
    t.equal(
        gen.inlineMappingUrl()
      , '//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiIiwic291cmNlcyI6WyJmb28uanMiLCJiYXIuanMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFDQUEsQURBQTtBQ0NBLEFEQUE7QUNDQSxBREFBO0FBQ0E7QUFDQSIsInNvdXJjZXNDb250ZW50IjpbbnVsbCwiZnVuY3Rpb24gYmFyKCkge1xuICBjb25zb2xlLmxvZygneWVzPycpO1xufSJdfQ=='
      , 'returns correct inline mapping url including source content'
    )
    t.end()
  })
})
