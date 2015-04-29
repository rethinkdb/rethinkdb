////
// Tests the JavaScript driver using the promises interface
/////

var assert = require('assert');
var path = require('path');

// -- settings

var driverPort = process.env.RDB_DRIVER_PORT || (process.argv[2] ? parseInt(process.argv[2], 10) : 28015);
var serverHost = process.env.RDB_SERVER_HOST || (process.argv[3] ? parseInt(process.argv[3], 10) : 'localhost');

var dbName = 'test';
var tableName = 'test';
var numRows = parseInt(process.env.TEST_ROWS) || 100;

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// -- globals

var tbl = r.db(dbName).table(tableName);
var reqlConn = null;

// -- helper functions

var withConnection = function(fnct) {
    if (fnct) {
        return function(done) {
            r.expr(1).run(reqlConn, function(err) {
                if(err) {
                    reqlConn = null;
                    r.connect({host:serverHost, port:driverPort}, function(err, conn) {
                        if(err) { done(err) }
                        reqlConn = conn;
                        fnct(done, reqlConn);;
                    })
                } else {
                    fnct(done, reqlConn);
                }
            });
        };
    } else {
        return r.expr(1).run(reqlConn) // check the connection
        .then(function() {
            return reqlConn;
        })
        // re-establish the connection if it was bad
        .catch(r.Error.RqlDriverError, r.Error.RqlRuntimeError, function(err) {
        	reqlConn = null;
        	return r.connect({host:serverHost, port:driverPort})
        	.then(function(conn) {
                // cache the new connection
                reqlConn = conn;
                return reqlConn;
        	});
        })
    }
}

// -- tests

describe('Promise tests', function() {
    // ensure reqlConn is valid before each test
    beforeEach(function() { return withConnection(); });
        
    it("simple expr(1)", function() {
        return r.expr(1).run(reqlConn).nodeify(function(err, result) {
            assert.equal(result, 1, 'Did not get the expected value (1): ' + result)
        });
    });
    
    if("toArray on an array", function() {
        var cursorA = null;
        return r.expr([1,2,3]).run(reqlConn)
        .then(function(cur) {
            cursorA = cur;
            return cur.toArray();
        })
        .then(function(cursorB) {
            // we should get back a reference to what we already had
            assert.strictEqual(cursorB, cursorA);
        });
    });
    
    it("close a connection", function() {
        return reqlConn.close()
        .then(function() {
            return reqlConn.close();
        })
        .then(function() {
            // fail if we don't have an error
            throw "A second close on a connection did not raise an error";
        })
        .catch(r.Error.RqlDriverError, function() {})
    });
    
    it("reconnect", function() {
        return reqlConn.close()
        .then(function() {
            return reqlConn.reconnect();
        })
    });
    
    it("noreplyWait", function() {
        return reqlConn.noreplyWait();
    });
    
    describe('With ' + numRows + ' rows', function() { 
        before(function() {
            this.timeout(20000);
            
            return withConnection()
            // setup db/table
            .then(function() {
                return r.expr([dbName]).setDifference(r.dbList()).forEach(r.dbCreate(r.row)).run(reqlConn);
            }).then(function() {
                return r.expr([tableName]).setDifference(r.db(dbName).tableList()).forEach(r.db(dbName).tableCreate(r.row)).run(reqlConn);
            })
            // ensure content
            .then(function() {
                return tbl.delete().run(reqlConn);
            })
            .then(function() {
                return r.range(0, numRows).forEach(function(row) {
                    return tbl.insert(
                        {'id':row, 'nums':r.range(0, 500).coerceTo('array')}
                    )}
                ).run(reqlConn)
            });
        });
        
        it("array expr()", function() {
            var limit = 10000 // large number to try hitting `maximum call stack exceed`
            return r.range(0, limit).run(reqlConn)
            .then(function(resCursor) {
                var i = 0;
                resCursor.each(
                    function(err, value) {
                        assert.strictEqual(value, i, "Value at poisition: " + i + " was: " + value + " rather than the expected: " + i);
                        i++;
                    },
                    function() {
                        assert.strictEqual(i, limit, "Final value was: " + i + " rather than the expected: " + limit);
                    }
                );
            });
        });
        
        it("next on a table cursor", function() {
            return tbl.run(reqlConn).then(function(cur) { return cur.next(); });
        });
        
        it("toArray on a table cursor", function() {
            return tbl.run(reqlConn).then(function(cur) { return cur.toArray(); }).then(function(result) {
                assert.strictEqual(result.length, numRows);
            });
        });
    });
});
