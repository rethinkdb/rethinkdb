/////
// Tests the driver API for converting dates to native Date objects
/////

var path = require('path');
var fs = require('fs');
var spawn = require('child_process').spawn

var assert = require('assert');

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// --

var rethinkdbExe = process.env.RDB_EXE_PATH
if (!process.env.RDB_EXE_PATH) {
    throw new Error('RDB_EXE_PATH ENV variable was not set!')
}

var testDefault = process.env.TEST_DEFAULT_PORT == "1"

var driver_port = null;

var assertNoError = function(err) {
    if (err) {
        throw new Error("Error '"+err+"' not expected")
    }
};

var withConnection = function(f){
    return function(done){
        r.connect({port:driver_port}, function(err, c){
            assert.equal(err, null);
            f(done, c);
        });
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
    var cpp_server;
    var server_out_log;
    var server_err_log;
    var cluster_port;

    beforeEach(function(done){
        driver_port = Math.floor(Math.random()*(65535 - 1025)+1025);
        cluster_port = driver_port + 1;
        server_out_log = fs.openSync('run/server-log.txt', 'a');
        server_err_log = fs.openSync('run/server-error-log.txt', 'a');
        cpp_server = spawn(
            rethinkdbExe,
            ['--driver-port', driver_port, '--http-port', '0', '--cluster-port', cluster_port, '--cache-size', '512'],
            {stdio: ['ignore', server_out_log, server_err_log]});
        this.timeout(5000);
        setTimeout(done, 2000);
    });

    afterEach(function(done){
        cpp_server.kill();
        spawn('rm', ['-rf', 'rethinkdb_data']);
        setTimeout(done, 10);
        fs.close(server_out_log);
        fs.close(server_err_log);
    });

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
