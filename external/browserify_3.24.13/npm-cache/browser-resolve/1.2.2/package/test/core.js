// test loading core modules
var assert = require('assert');
var resolve = require('../');

var shims = {
    events: 'foo'
};

test('shim found', function(done) {
    resolve('events', { modules: shims }, function(err, path) {
        assert.ifError(err);
        assert.equal(path, 'foo');
        done();
    });
});

test('core shim not found', function(done) {
    resolve('http', {}, function(err, path) {
        assert.ifError(err);
        assert.equal(path, 'http');
        done();
    });
});

