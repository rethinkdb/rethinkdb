var join = require('path').join;
var read = require('fs').readFileSync;
var inspect = require('util').inspect;
var assert = require('assert');
var rfile = require('../');

var testCases = [];
function equal(reqPath, fullPath, options) {
  testCases.push([reqPath, fullPath, options]);
}

equal('./index.js', __filename);
equal('../package.json', join(__dirname, '..', 'package.json'));
equal('./index', __filename);
equal('../package', join(__dirname, '..', 'package.json'));
equal('../README', join(__dirname, '..', 'README.md'), {extensions: ['.md']});


describe('rfile.resolve', function () {
  testCases.forEach(function (testCase) {
    describe('(' + inspect(testCase[0]) + (testCase[2] ? ', ' + inspect(testCase[2]) : '') + ')', function () {
      it('resolves to ' + inspect(testCase[1]), function () {
        assert.equal(rfile.resolve(testCase[0], testCase[2]), testCase[1]);
      });
    });
  });
});

describe('rfile', function () {
  testCases.forEach(function (testCase) {
    describe('(' + inspect(testCase[0]) + (testCase[2] ? ', ' + inspect(testCase[2]) : '') + ')', function () {
      it('reads ' + inspect(testCase[1]), function () {
        assert.equal(rfile(testCase[0], testCase[2]), stripBOM(read(testCase[1]).toString()).replace(/\r/g, ''));
      });
    });
  });
});

function stripBOM(str){
  return 0xFEFF == str.charCodeAt(0)
    ? str.substring(1)
    : str;
}