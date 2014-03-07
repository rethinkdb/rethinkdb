var assert = require('assert');
var path = require('path');
var resolve = require('../');

var fixtures_dir = __dirname + '/fixtures';

test('false file', function(done) {
    var parent_file = fixtures_dir + '/node_modules/false/index.js';
    resolve('./fake.js', { filename: parent_file }, function(err, p) {
        assert.ifError(err);
        assert.equal(p, path.normalize(__dirname + '/../empty.js'));
        done();
    });
});

test('false module', function(done) {
    var parent_file = fixtures_dir + '/node_modules/false/index.js';
    resolve('ignore-me', { filename: parent_file }, function(err, p) {
        assert.ifError(err);
        assert.equal(p, path.normalize(__dirname + '/../empty.js'));
        done();
    });
});

