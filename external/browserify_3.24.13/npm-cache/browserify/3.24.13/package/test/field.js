var assert = require('assert');
var browserify = require('../');
var vm = require('vm');
var test = require('tap').test;

test('fieldString', function (t) {
    t.plan(1);
    
    var b = browserify();
    b.expose('./string.js', __dirname + '/field/string.js');
    b.bundle(function (err, src) {
        if (err) return t.fail(err);
        
        var c = {};
        vm.runInNewContext(src, c);
        t.equal(
            c.require('./string.js'),
            'browser'
        );
    });
});

test('fieldObject', function (t) {
    t.plan(1);
    
    var b = browserify();
    b.expose('./object.js', __dirname + '/field/object.js');
    b.bundle(function (err, src) {
        if (err) return t.fail(err);
        
        var c = {};
        vm.runInNewContext(src, c);
        t.equal(
            c.require('./object.js'),
            '!browser'
        );
    });
});

test('missObject', function (t) {
    t.plan(1);
    
    var b = browserify();
    b.expose('./miss.js', __dirname + '/field/miss.js');
    b.bundle(function (err, src) {
        if (err) return t.fail(err);
        
        var c = {};
        vm.runInNewContext(src, c);
        t.equal(
            c.require('./miss.js'),
            '!browser'
        );
    });
});

test('fieldSub', function (t) {
    t.plan(1);
    
    var b = browserify();
    b.expose('./sub.js', __dirname + '/field/sub.js');
    b.bundle(function (err, src) {
        if (err) return t.fail(err);
        
        var c = {};
        vm.runInNewContext(src, c);
        t.equal(
            c.require('./sub.js'),
            'browser'
        );
    });
});
