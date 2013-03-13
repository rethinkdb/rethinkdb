/////
// Tests the driver API for making connections and excercising the networking code
/////

var spawn = require('child_process').spawn

var assert = require('assert');

var r = require('../../../drivers/javascript/build/rethinkdb');
var build = process.env.BUILD || 'debug'
var testDefault = process.env.TEST_DEFAULT_PORT == "1"

var port = Math.floor(Math.random()*(65535 - 1025)+1025)

var assertErr = function(err, type, msg) {
    assertNotNull(err);
    assert.equal(err.constructor.name, type);
    var _msg = err.message.replace(/ in:\n([\r\n]|.)*/m, "");
    assert.equal(_msg, msg);
};

var throwsError = function(type, msg, done){
    return function(err){
        assertErr(err, type, msg);
        done();
    };
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
            r.connect({}, throwsError("RqlDriverError", "Could not connect to localhost:28015.", function(){
                r.connect({port:11221}, throwsError("RqlDriverError", "Could not connect to localhost:11221.", function(){
                    r.connect({host:'0.0.0.0'}, throwsError("RqlDriverError", "Could not connect to 0.0.0.0:28015.", function(){
                        r.connect({host:'0.0.0.0', port:11221}, throwsError("RqlDriverError", "Could not connect to 0.0.0.0:11221.", done))}))}))}));
        });
    });

    describe('With a server', function(){

        var cpp_server;

        before(function(done){
            cpp_server = spawn(
                '../../build/'+build+'/rethinkdb',
                ['--driver-port', port, '--http-port', '0', '--cluster-port', '0']);
            // cpp_server.stderr.on('data',function(d){ console.log(d.toString()) });
            setTimeout(done, 1000);
        });

        after(function(){
            cpp_server.kill();
            spawn('rm', ['-rf', 'rethinkdb_data']);
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
            assert.throws(function(){ r(1).run(c); }, "RqlDriverError");
            done();
        }));

        it("test use", withConnection(function(done, c){
            r.db('test').tableCreate('t1').run(c, noError(function(){
                r.dbCreate('db2').run(c, noError(function(){
                    r.db('db2').tableCreate('t2').run(c, noError(function(){
                        c.use('db2');
                        r.table('t2').run(c, noError(function(){
                            c.use('test');
                            r.table('t2').run(c, throwsError("RqlRuntimeError", "Table `t2` does not exist.",
                                                             done))
                        }))}))}))}))}));

        it("fails to query after kill", function(done){
            cpp_server.kill();
            setTimeout(function() {
                assert.throws(function(){
                    r(1).run(c);
                }, "RqlDriverError", "Connection is closed.");
                done();
            }, 100);
        });
    });
});

// TODO: test cursors, streaming large values

