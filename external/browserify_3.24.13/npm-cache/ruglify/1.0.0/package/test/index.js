var assert = require('assert');
var rfile = require('rfile');
var ruglify = require('../');
describe('ruglify(file)', function () {
  it('returns the minified source code for the file', function () {
    assert.equal(ruglify('./fixture/jquery.js'), rfile('./fixture/jquery.min.js'));
  });
});