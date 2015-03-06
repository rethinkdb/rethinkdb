/////
// Tests the driver API for making connections and excercising the networking code
/////

var assert = require('assert');
var path = require('path');
var fs = require('fs');
var spawn = require('child_process').spawn

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// --

var rethinkdbExe = process.env.RDB_EXE_PATH || '../../../build/debug/rethinkdb' // - ToDo: replace the hard code
var testDefault = process.env.TEST_DEFAULT_PORT == "1"

var port = null;

var assertErr = function(err, type, msg) {
    assertNotNull(err);
    assert.equal(err.constructor.name, type);
    var _msg = err.message.replace(/ in:\n([\r\n]|.)*/m, "");
    _msg = _msg.replace(/\nFailed assertion:(.|\n)*/m, "")
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

var ifTestDefault = function(f, c){
    if(testDefault){
        return f(c);
    }else{
        return c();
    }
}

describe('Javascript connection API', function(){
    it("times out with a server that doesn't do the handshake correctly", function(done){
        var port = 8990;

        // Setup dummy sever
        require('net').createServer(function(c) {
            // Do nothing
        }).listen(port);

        r.connect({port:port, timeout:1}, givesError("RqlDriverError", "Handshake timedout", done));
    });

    describe('With no server', function(){
        it("fails when trying to connect", function(done){
            ifTestDefault(
                function(cont){
                    r.connect({}, givesError("RqlDriverError", "Could not connect to localhost:28015.\nconnect ECONNREFUSED", function(){
                        r.connect({host:'0.0.0.0'}, givesError("RqlDriverError", "Could not connect to 0.0.0.0:28015.\nconnect ECONNREFUSED", cont))})); },
                function(){
                    r.connect({port:11221}, givesError("RqlDriverError", "Could not connect to localhost:11221.\nconnect ECONNREFUSED", function(){
                        r.connect({host:'0.0.0.0', port:11221}, givesError("RqlDriverError", "Could not connect to 0.0.0.0:11221.\nconnect ECONNREFUSED", done))}))});
        });

        it("empty run", function(done) {
            r.expr(1).run().then(function() {
                done(new Error("Was expecting an error"))
            }).error(givesError("RqlDriverError", "First argument to `run` must be an open connection.", done));
        });
        it("run with an object", function(done) {
            r.expr(1).run({}).then(function() {
                done(new Error("Was expecting an error"))
            }).error(givesError("RqlDriverError", "First argument to `run` must be an open connection.", done));
        });
    });

    describe('With a server', function(){

        var cpp_server;
        var server_out_log;
        var server_err_log;
        var cluster_port;

        beforeEach(function(done){
            function setup() {
                port = Math.floor(Math.random()*(65535 - 1025)+1025);
                cluster_port = port + 1;
                server_out_log = fs.openSync('run/server-log.txt', 'a');
                server_err_log = fs.openSync('run/server-error-log.txt', 'a');
                cpp_server = spawn(
                    rethinkdbExe,
                    ['--driver-port', port, '--http-port', '0', '--cluster-port', cluster_port, '--cache-size', '512'],
                    {stdio: ['ignore', 'pipe', 'pipe']});

                cpp_server.stdout.on('data', function(data) {
                    fs.write(server_out_log, data)
                    if (data.toString().match(/Server ready/)) {
                        cpp_server.removeAllListeners();
                        setTimeout(done, 500); // We give the server 500ms to avoid errors like ("Cannot compute blueprint...")
                    }

                    if (data.toString().match(/Could not bind to/)) {
                        // The port is somehow used by someone else, let's spin up another server
                        cpp_server.removeAllListeners();
                        setup();
                    }
                });
                cpp_server.stderr.on('data', function(data) {
                    fs.write(server_err_log, data)
                });
            }

            setup();
        });

        afterEach(function(done){
            cpp_server.kill();
            cpp_server.removeAllListeners();
            spawn('rm', ['-rf', 'rethinkdb_data']);
            setTimeout(done, 10);
            fs.close(server_out_log);
            fs.close(server_err_log);
        });

        it("authorization key when none needed", function(done){
            r.connect({port: port, authKey: "hunter2"}, givesError("RqlDriverError", "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"", done));
        });

        it("correct authorization key", function(done){
            setTimeout(function(){
                r.connect({port: port}, function(e, c){
                    assertNull(e);
                    r.db('rethinkdb').table('cluster_config').update({auth_key: "hunter3"}).run(c, noError(function() {
                        r.connect({port: port, authKey: "hunter3"}, function(e, c){
                            assertNull(e);
                            r.expr(1).run(c, noError(done));
                        });
                    }));
                });
            }, 500);
        });

        it("wrong authorization key", function(done){
            setTimeout(function(){
                r.connect({port: port}, function(e, c){
                    assertNull(e);
                    r.db('rethinkdb').table('cluster_config').update({auth_key: "hunter4"}).run(c, noError(function() {
                        r.connect({port: port, authKey: "hunter-wrong"}, givesError("RqlDriverError", "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"", done));
                    }));
                });
            }, 500);
        });

        // TODO: test default port

        it("close twice and reconnect", withConnection(function(done, c){
            testSimpleQuery(c, function(){
                assert.doesNotThrow(function(){
                    c.close({noreplyWait: false});
                    c.close({noreplyWait: false});
                    c.reconnect(function(err, c){
                        assertNull(err);
                        testSimpleQuery(c, done);
                    });
                });
            });
        }));
        it("close twice (synchronously) and reconnect", withConnection(function(done, c){
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
            c.close({noreplyWait: false});
            r(1).run(c, givesError("RqlDriverError", "Connection is closed.", done));
        }));

        it("noreplyWait waits", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true});
            c.noreplyWait(function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration >= 500);
                done();
            });
        }));

        it("close waits by default", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true});
            c.close(function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration >= 500);
                done();
            });
        }));

        it("reconnect waits by default", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true});
            c.reconnect(function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration >= 500);
                done();
            });
        }));

        it("close does not wait if we want it not to", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true});
            c.close({'noreplyWait': false}, function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration < 500);
                done();
            });
        }));

        it("reconnect does not wait if we want it not to", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true});
            c.reconnect({'noreplyWait': false}, function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration < 500);
                done();
            });
        }));

        it("close waits even without callback", withConnection(function(done, c){
            var start = Date.now();
            var timeout = 1.5;
            r.js('while(true);', {timeout: timeout}).run(c, {noreply: true});
            c.close(function(err) {
                if (err) throw err;
                var duration = Date.now()-start;
                assert(duration > timeout*1000);
                done()
            });
        }));

        it("test use", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.dbCreate('db2').run(c, noError(function(){
                    r.db('db2').tableCreate('t2').run(c, noError(function(){
                        c.use('db2');
                        r.table('t2').run(c, noError(function(){
                            c.use('test');
                            r.table('t2').run(c, givesError("RqlRuntimeError", "Table `test.t2` does not exist",
                                                            done));
                        }));}));}));}));}));

        it("useOutdated", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, function(){
                r.db('test').table('t1', {useOutdated:true}).run(c, function(){
                    r.db('test').table('t1').run(c, {useOutdated: true}, done);});});
        }));

        it("test default durability", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.db('test').table('t1').insert({data:"5"}).run(c, {durability: "default"},
                    givesError("RqlRuntimeError", "Durability option `default` unrecognized (options are \"hard\" and \"soft\")", done));
            }));
        }));

        it("test wrong durability", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.db('test').table('t1').insert({data:"5"}).run(c, {durability: "wrong"},
                    givesError("RqlRuntimeError", "Durability option `wrong` unrecognized (options are \"hard\" and \"soft\")", done))
            }));
        }));

        it("test soft durability", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.db('test').table('t1').insert({data:"5"}).run(c, {durability: "soft"}, noError(done));
            }));
        }));

        it("test hard durability", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.db('test').table('t1').insert({data:"5"}).run(c, {durability: "hard"}, noError(done));
            }));
        }));

        it("test non-deterministic durability", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.db('test').table('t1').insert({data:"5"}).run(c, {durability: r.js("'so' + 'ft'")}, noError(done));
            }));
        }));

        it("fails to query after kill", withConnection(function(done, c){
            cpp_server.kill();
            setTimeout(function() {
                r(1).run(c, givesError("RqlDriverError", "Connection is closed.", done));
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
            assert.throws(function(){ c.use(); });
            done();
        }));

        // Test the event emitter interface

        it("Emits connect event on connect", withConnection(function(done, c){
            c.on('connect', function(err, c2) {
                done();
            });

            c.reconnect(function(){});
        }));

        it("Emits close event on close", withConnection(function(done, c){
            c.on('close', function() {
                done();
            });

            c.close();
        }));

        it("Non valid optarg", withConnection(function(done, c){
            r.expr(1).run(c, {nonValidOption: true}).then(function() {
                done(new Error("Was expecting an error"))
            }).error(givesError("RqlCompileError", "Unrecognized global optional argument `non_valid_option`", done));
        }));

        it("Extra argument", withConnection(function(done, c){
            r.expr(1).run(c, givesError("RqlDriverError", "Second argument to `run` cannot be a function if a third argument is provided.", done), 1)
        }));

        it("Callback must be a function", withConnection(function(done, c){
            r.expr(1).run(c, {}, "not_a_function").error(givesError("RqlDriverError", "If provided, the callback must be a function. Please use `run(connection[, options][, callback])", done))
        }));
    });
});

