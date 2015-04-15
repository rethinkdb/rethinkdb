/////
// Tests the driver API for converting dates to native Date objects
/////

var assert = require('assert');
var path = require('path');

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// -- get ENV inforamtion

var testDefault = process.env.TEST_DEFAULT_PORT == "1"
var driverPort = process.env.RDB_DRIVER_PORT || 28015;
var serverHost = process.env.RDB_SERVER_HOST || 'localhost';

/// -- global variables

var sharedConnection = null;

// -- helper functions

var assertNoError = function(err) {
    if (err) {
        try {
            sharedConnection.close();
        } catch(err) {}
        sharedConnection = null;
        throw new Error("Error '"+err+"' not expected")
    }
};

var withConnection = function(f){
    return function(done){
        if (sharedConnection) {
            f(done, sharedConnection);
        } else {
            r.connect({host:serverHost, port:driverPort}, function(err, conn){
                sharedConnection = conn;
                assert.equal(err, null);
                f(done, sharedConnection);
            });
        }
    };
};

Object.size = function(obj) {
    var size = 0, key;
    for (key in obj) {
        if (obj.hasOwnProperty(key)) size++;
    }
    return size;
};

describe('Javascript date pseudotype conversion', function(){
    
    it("flat time", withConnection(function(done, conn){
        r.expr(r.epochTime(896571240)).run(conn, function(err, time) {
            assertNoError(err);
            var expected = new Date(896571240000)
            assert(time instanceof Date)
            assert.equal(time - expected, 0)
            done()
        });
    }));

    it("array with a time inside an object", withConnection(function(done, conn){
        r.expr({stuff:r.epochTime(896571240), more:[r.epochTime(996571240)]}).run(conn, function(err, obj) {
            assertNoError(err);
            var expected_stuff = new Date(896571240000)
            var expected_more = new Date(996571240000)
            assert.equal(Object.size(obj), 2)
            assert(obj['stuff'] instanceof Date)
            assert.equal(obj['stuff'] - expected_stuff, 0)
            assert.equal(obj['more'].length, 1)
            assert(obj['more'][0] instanceof Date)
            assert.equal(obj['more'][0] - expected_more, 0)
            done()
        });
    }));

    it("object with a time inside an array", withConnection(function(done, conn){
        r.expr([r.epochTime(796571240), r.epochTime(896571240), {stuff:r.epochTime(996571240)}]).run(conn, function(err, arr) {
            assertNoError(err);
            var expected_first = new Date(796571240000)
            var expected_second = new Date(896571240000)
            var expected_stuff = new Date(996571240000)
            assert.equal(arr.length, 3)
            assert(arr[0] instanceof Date)
            assert.equal(arr[0] - expected_first, 0)
            assert(arr[1] instanceof Date)
            assert.equal(arr[1] - expected_second, 0)
            assert.equal(Object.size(arr[2]), 1)
            assert(arr[2]['stuff'] instanceof Date)
            assert.equal(arr[2]['stuff'] - expected_stuff, 0)
            done()
        });
    }));

    it("nested object with a time", withConnection(function(done, conn){
        r.expr({nested:{time:r.epochTime(896571240)}}).run(conn, function(err, obj) {
            assertNoError(err);
            var expected = new Date(896571240000)
            assert.equal(Object.size(obj), 1)
            assert.equal(Object.size(obj['nested']), 1)
            assert(obj['nested']['time'] instanceof Date)
            assert.equal(obj['nested']['time'] - expected, 0)
            done()
        });
    }));

    it("nested array with a time", withConnection(function(done, conn){
        r.expr([1, "two", ["a", r.epochTime(896571240), 3]]).run(conn, function(err, arr) {
            assertNoError(err);
            var expected = new Date(896571240000)
            assert.equal(arr.length, 3)
            assert.equal(arr[0], 1)
            assert.equal(arr[1], "two")
            assert.equal(arr[2].length, 3)
            assert.equal(arr[2][0], "a")
            assert(arr[2][1] instanceof Date)
            assert.equal(arr[2][1] - expected, 0)
            assert.equal(arr[2][2], 3)
            done()
        });
    }));

    it("group with times", withConnection(function(done, conn){
        r.expr([{id:0,time:r.epochTime(896571240)}]).group('time').coerceTo('array').run(conn, function(err, res){
            var expectedDate = new Date(896571240000)
            assert.equal(res.length, 1)
            assert.equal(res[0].group.getTime(), expectedDate.getTime())
            assert.equal(res[0].reduction.length, 1)
            assert.equal(res[0].reduction[0].id, 0)
            assert.equal(res[0].reduction[0].time.getTime(), expectedDate.getTime())
            done()
        });
    }));

    it("Errors should have a stack trace", withConnection(function(done, conn){
        try {
            r.epochTime(undefined)
            done(new Error("Was expecting an error"))
        }
        catch(err) {
            assert(err.stack);
            done()
        }
    }));

});
