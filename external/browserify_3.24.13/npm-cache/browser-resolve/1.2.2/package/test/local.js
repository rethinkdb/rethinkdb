var assert = require('assert');
var resolve = require('../');

var fixtures_dir = __dirname + '/fixtures';

test('local', function(done) {
    // resolve needs a parent filename or paths to be able to lookup files
    // we provide a phony parent file
    resolve('./foo', { filename: fixtures_dir + '/phony.js' }, function(err, path) {
        assert.ifError(err);
        assert.equal(path, require.resolve('./fixtures/foo'));
        done();
    });
});

