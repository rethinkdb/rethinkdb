/////
// Tests the driver API for making connections and excercising the networking code
/////

var assert = require('assert');
var path = require('path');
var fs = require('fs');
var spawn = require('child_process').spawn

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// -- get ENV inforamtion

var testDefault = process.env.TEST_DEFAULT_PORT == "1";
var driverPort = process.env.RDB_DRIVER_PORT;
var serverHost = process.env.RDB_SERVER_HOST || 'localhost';

// -- helper functions

var givesError = function(type, msg, done){
    return function(err){
        var error = null;
        try {
            assertNotNull(err);
            assert.equal(err.constructor.name, type);
            var _msg = err.message.replace(/ in:\n([\r\n]|.)*/m, "");
            _msg = _msg.replace(/\nFailed assertion:(.|\n)*/m, "")
            assert.equal(_msg, msg);
        } catch (e) {
            error = e;
        } finally {
            if (done) {
                done(error);
            } else if (error) {
                throw error;
            } else {
                return true; // error is correct
            }
        }
    };
};

var withConnection = function(f){
    return function(done){
        r.connect({host:serverHost, port:driverPort}, function(err, c){
            assertNull(err);
            f(done, c);
        });
    };
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
        // Setup dummy sever
        var server = require('net').createServer(function(c) {});
        server.listen(0, function() {
            var badPort = server.address().port;
            r.connect({port:badPort, timeout:1}, givesError("RqlDriverError", "Handshake timedout", done));
        });
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
        this.timeout(4000)
        // TODO: test default port
        
        describe('close twice and reconnect', function(){
            var simpleQuery = function(c) { return r(1).run(c); };
            
            it("noreplyWait=false", function(done) {
                var conn = null;
                r.connect({host:serverHost, port:driverPort})
                .then(function(c) {
                    conn = c;
                    return simpleQuery(conn);
                }).then(function() {
                    return conn.close({noreplyWait: false});
                }).then(function() {
                    return conn.close({noreplyWait: false});
                }).then(function() {
                    return conn.reconnect();
                }).then(function() {
                    return simpleQuery(conn);
                }).nodeify(done)
            });
            
            it("synchronously", function(done) {
                var conn = null;
                r.connect({host:serverHost, port:driverPort})
                .then(function(c) {
                    conn = c;
                    return simpleQuery(conn);
                }).then(function() {
                    return conn.close();
                }).then(function() {
                    return conn.close();
                }).then(function() {
                    return conn.reconnect();
                }).then(function() {
                    return simpleQuery(conn);
                }).nodeify(done)
            });
        })
        
        it("fails to query after close", withConnection(function(done, c){
            c.close({noreplyWait: false});
            r(1).run(c, givesError("RqlDriverError", "Connection is closed.", done));
        }));
        
        it("noreplyWait waits", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true}, function() {});
            c.noreplyWait(function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration >= 500);
                done();
            });
        }));
        
        it("close waits by default", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true}, function() {});
            c.close(function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration >= 500);
                done();
            });
        }));
        
        it("reconnect waits by default", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true}, function() {});
            c.reconnect(function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration >= 500);
                done();
            });
        }));
        
        it("close does not wait if we want it not to", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true}, function() {});
            c.close({'noreplyWait': false}, function (err) {
                assertNull(err);
                var duration = new Date().getTime() - t;
                assert(duration < 500);
                done();
            });
        }));
        
        it("reconnect does not wait if we want it not to", withConnection(function(done, c){
            var t = new Date().getTime();
            r.js('while(true);', {timeout: 0.5}).run(c, {noreply: true}, function() {});
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
            r.js('while(true);', {timeout: timeout}).run(c, {noreply: true}, function() {});
            c.close(function(err) {
                if (err) throw err;
                var duration = Date.now()-start;
                assert(duration > timeout*1000);
                done()
            });
        }));
        
        it("useOutdated", withConnection(function(done, c){
            r.db('test').tableCreate('useOutdated').run(c, function(){
                r.db('test').table('useOutdated', {useOutdated:true}).run(c, function(){
                    r.db('test').table('useOutdated').run(c, {useOutdated: true}, done);});});
        }));
        
        it("test default durability", withConnection(function(done, c){
            r.db('test').tableCreate('defaultDurability').run(c, noError(function(){
                r.db('test').table('defaultDurability').insert({data:"5"}).run(c, {durability: "default"},
                    givesError("RqlRuntimeError", "Durability option `default` unrecognized (options are \"hard\" and \"soft\")", done));
            }));
        }));
        
        it("test wrong durability", withConnection(function(done, c){
            r.db('test').tableCreate('wrongDurability').run(c, noError(function(){
                r.db('test').table('wrongDurability').insert({data:"5"}).run(c, {durability: "wrong"},
                    givesError("RqlRuntimeError", "Durability option `wrong` unrecognized (options are \"hard\" and \"soft\")", done))
            }));
        }));
        
        it("test soft durability", withConnection(function(done, c){
            r.db('test').tableCreate('softDurability').run(c, noError(function(){
                r.db('test').table('softDurability').insert({data:"5"}).run(c, {durability: "soft"}, noError(done));
            }));
        }));
        
        it("test hard durability", withConnection(function(done, c){
            r.db('test').tableCreate('hardDurability').run(c, noError(function(){
                r.db('test').table('hardDurability').insert({data:"5"}).run(c, {durability: "hard"}, noError(done));
            }));
        }));
        
        it("test non-deterministic durability", withConnection(function(done, c){
            r.db('test').tableCreate('nonDeterministicDurability').run(c, noError(function(){
                r.db('test').table('nonDeterministicDurability').insert({data:"5"}).run(c, {durability: r.js("'so' + 'ft'")}, noError(done));
            }));
        }));
        
        it("pretty-printing", function(){
            assert.equal(r.db('db1').table('tbl1').map(function(x){ return x; }).toString(),
                        'r.db("db1").table("tbl1").map(function(var_0) { return var_0; })')
        });
        
        it("connect() with default db", withConnection(function(done, c){
            r.dbCreate('foo').run(c, function(err, res){
                r.db('foo').tableCreate('bar').run(c, function(err, res){
                    c.close();
                    r.connect({host:serverHost, port:driverPort, db:'foo'}, noError(function(err, c){
                        r.table('bar').run(c, noError(done));
                    }));
                });
            });
        }));
        
        it("missing arguments cause exception", withConnection(function(done,c){
            assert.throws(function(){ c.use(); });
            done();
        }));
        
        // -- event emitter interface
        
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
        
        // -- bad options
        
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
        
        // -- auth_key tests
        
        describe('Authorization key tests', function() {
            
            var safeConn = null;
            before(function(done) {
                r.connect({host:serverHost, port:driverPort}, function(err, conn) {
                    assertNull(err);
                    safeConn = conn;
                    done();
                });
            })
            
            afterEach(function(done) {
                // undo any auth_key changes
				r.db('rethinkdb').table('cluster_config').update({auth_key:null}).run(safeConn).then(function(value, err) { done(err); });
            })
            
            // --
            
            it("authorization key when none needed", function(done){
                r.connect(
                    {host:serverHost, port:driverPort, authKey: "hunter2"},
                    givesError("RqlDriverError", "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"", done)
                );
            });
            
            it("correct authorization key", function(done){
                this.timeout(500);
                r.db('rethinkdb').table('cluster_config').update({auth_key: "hunter3"}).run(safeConn)
                .then(function() {
                    return r.connect({host:serverHost, port:driverPort, authKey: "hunter3"});
                }).then(function(authConn) {
                    return r.expr(1).run(authConn);
                }).nodeify(done)
            });
            
            it("wrong authorization key", function(done){
                this.timeout(500);
                r.db('rethinkdb').table('cluster_config').update({auth_key: "hunter4"}).run(safeConn)
                .then(function() {
                    return r.connect({host:serverHost, port:driverPort, authKey: "hunter-bad"});
                }).then(function() {
                    err = new Error('Unexpectedly connected with bad AuthKey');
                    done(err);
                }).catch(givesError("RqlDriverError", "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"", done))
            });
        });
        
        // -- use tests
        
        this.timeout(8000) // "test use" can take some time
        
        it("test use", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.dbCreate('db2').run(c, noError(function(){
                    r.db('db2').tableCreate('t2').run(c, noError(function(){
                        c.use('db2');
                        r.table('t2').run(c, noError(function(){
                            c.use('test');
                            r.table('t2').run(c, givesError("RqlRuntimeError", "Table `test.t2` does not exist", done));
        }));}));}));}));}));
    });
});
