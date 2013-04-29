/////
// Tests the driver API for making connections and excercising the networking code
/////

var fs = require('fs');
var spawn = require('child_process').spawn

var assert = require('assert');

var r = require('../../../drivers/javascript/build/rethinkdb');
var build_dir = process.env.BUILD_DIR || '../../../build/debug'
var testDefault = process.env.TEST_DEFAULT_PORT == "1"

var port = null;

var assertErr = function(err, type, msg) {
    assertNotNull(err);
    assert.equal(err.constructor.name, type);
    var _msg = err.message.replace(/ in:\n([\r\n]|.)*/m, "");
    assert.equal(_msg, msg);
};

var givesError = function(type, msg, done){
    return function(err){
        assertErr(err, type, msg);
        if(done) {
          done();
        }
    };
};

var checkError = function(type, msg) {
    return function(err) {
      assertErr(err, type, msg);
      return true; // error is correct
    }
};

var withConnection = function(f){
    return function(done){
        r.connect({port:port}, function(err, c){
            assertNull(err);
            f(done, c);
        });
    };
};

var testSimpleQuery = function(c, done){
    r(1).run(c, function(err, res){
        assertNull(err);
        assert.equal(res, 1);
        done();
    });
};

var noError = function(f){
    return function(err){
        assertNull(err);
        f.apply({}, arguments);
    };
}

var assertNull = function(x){
    assert.equal(x, null);
}

var assertNotNull = function(x){
    assert.notEqual(x, null);
}

describe('Javascript connection API', function(){
    describe('With no server', function(){
        it("fails when trying to connect", function(done){
            r.connect({}, givesError("RqlDriverError", "Could not connect to localhost:28015.", function(){
                r.connect({port:11221}, givesError("RqlDriverError", "Could not connect to localhost:11221.", function(){
                    r.connect({host:'0.0.0.0'}, givesError("RqlDriverError", "Could not connect to 0.0.0.0:28015.", function(){
                        r.connect({host:'0.0.0.0', port:11221}, givesError("RqlDriverError", "Could not connect to 0.0.0.0:11221.", done))}))}))}));
        });

        it("empty run", function(done) {
          assert.throws(function(){ r.expr(1).run(); },
                        checkError("RqlDriverError",
                                   "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool> }."));
          done();
        });
    });

    describe('With a server', function(){

        var cpp_server;
        var server_out_log;
        var server_err_log

        beforeEach(function(done){
            port = Math.floor(Math.random()*(65535 - 1025)+1025)
            server_out_log = fs.openSync('run/server-log.txt', 'a');
            server_err_log = fs.openSync('run/server-error-log.txt', 'a');
            cpp_server = spawn(
                build_dir + '/rethinkdb',
                ['--driver-port', port, '--http-port', '0', '--cluster-port', '0'],
                {stdio: ['ignore', server_out_log, server_err_log]});
            setTimeout(done, 1000);
        });

        afterEach(function(done){
            cpp_server.kill();
            spawn('rm', ['-rf', 'rethinkdb_data']);
            setTimeout(done, 10);
            fs.close(server_out_log);
            fs.close(server_err_log);
        });

        // TODO: test default port

        it("close twice and reconnect", withConnection(function(done, c){
            testSimpleQuery(c, function(){
                assert.doesNotThrow(function(){
                    c.close();
                    c.close();
                    c.reconnect(function(err, c){
                        assertNull(err);
                        testSimpleQuery(c, done);
                    });
                });
            });
        }));

        it("fails to query after close", withConnection(function(done, c){
            c.close();
            assert.throws(function(){ r(1).run(c, function(){}); },
                          checkError("RqlDriverError", "Connection is closed."));
            done();
        }));

        it("test use", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.dbCreate('db2').run(c, noError(function(){
                    r.db('db2').tableCreate('t2').run(c, noError(function(){
                        c.use('db2');
                        r.table('t2').run(c, noError(function(){
                            c.use('test');
                            r.table('t2').run(c, givesError("RqlRuntimeError", "Table `t2` does not exist.",
                                                            done));
                        }));}));}));}));}));

        it("useOutdated", withConnection(function(done ,c){
            r.db('test').tableCreate('t1').run(c, function(){
                r.db('test').table('t1', {useOutdated:true}).run(c, function(){
                    r.table('t1').run({connection: c, useOutdated: true}, done);});});
        }));

        it("fails to query after kill", withConnection(function(done, c){
            cpp_server.kill();
            setTimeout(function() {
                assert.throws(function(){
                    r(1).run(c, function(err, res) { assert.ok(false, "This callback should never get called"); });
                }, checkError("RqlDriverError", "Connection is closed."));
                done();
            }, 100);
        }));

        it("pretty-printing", function(){
            assert.equal(r.db('db1').table('tbl1').map(function(x){ return x; }).toString(),
                        'r.db("db1").table("tbl1").map(function(var_0) { return var_0; })')
        });

        it("connect() with default db", withConnection(function(done, c){
            r.dbCreate('foo').run(c, function(err, res){
                r.db('foo').tableCreate('bar').run(c, function(err, res){
                    c.close();
                    r.connect({port:port, db:'foo'}, noError(function(err, c){
                        r.table('bar').run(c, noError(done));
                    }));
                });
            });
        }));

        it("missing arguments cause exception", withConnection(function(done,c){
            assert.throws(function(){ r.connect(); });
            assert.throws(function(){ c.use(); });
            done();
        }));
    });
});

