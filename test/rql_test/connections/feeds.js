////
// Tests for feeds
/////

var assert = require('assert');
var path = require('path');

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// --

var port = parseInt(process.argv[2] || process.env.RDB_DRIVER_PORT, 10);
var idValue = Math.floor(Math.random()*1000);

var tableName = "floor" + (Math.floor(Math.random()*1000) + 1);

process.on('uncaughtException', function(err) {
    console.log(err);
    if (err.stack) {
        console.log(err.toString() + err.stack.toString());
    } else {
        console.log(err.toString());
    }
    process.exit(1)
});

var numWrites = 20;
setTimeout(function() {
    throw new Error("Timeout")
}, 60000);


function test1() {
    // Test insert, update, replace, delete and the basic case for `next`
    console.log("Running test1");
    
    r.connect({port:port}, function(err, conn) {   
        if (err) throw err;
        r.table(tableName).changes().run(conn, function(err, feed) {
            if (err) throw err;
    
            var count = 0;
            var fn = function(err, data) {
                if (err) {
                    if ((count === 4) && (err.message.match(/^Changefeed aborted \(table unavailable/))) {
                        r.tableCreate(tableName).run(conn, function(err, result) {
                            if (err) throw err;
                            conn.close();
                            test2();
                        })
                    }
                    else {
                        throw err;
                    }
    
                }
                else {
                    if (count === 0) {
                        assert.equal(data.old_val, null);
                        assert.deepEqual(data.new_val, {id: idValue, value: 'insert'});
                    }
                    else if (count === 1) {
                        assert.deepEqual(data.old_val, {id: idValue, value: 'insert'});
                        assert.deepEqual(data.new_val, {id: idValue, value: 'update'});
                    }
                    else if (count === 2) {
                        assert.deepEqual(data.old_val, {id: idValue, value: 'update'});
                        assert.deepEqual(data.new_val, {id: idValue, value: 'replace'});
                    }
                    else if (count === 3) {
                        assert.deepEqual(data.old_val, {id: idValue, value: 'replace'});
                        assert.equal(data.new_val, null);
                        r.tableDrop(tableName).run(conn).then(function(err) {
                            if (err) throw err;
                        }).error(function(err) { throw err});
                    }
                    count++;
                    feed.next(fn);
                }
            }
            feed.next(fn);
        });
    
        setTimeout(function() { // Wait 5 seconds to retrieve some empty responses
            r.connect({port: port}, function(err, conn) {
                r.table(tableName).insert({id: idValue, value: 'insert'}).run(conn).then(function() {
                    return r.table(tableName).get(idValue).update({value: 'update'}).run(conn);
                }).then(function() {
                    return r.table(tableName).get(idValue).replace({id: idValue, value: 'replace'}).run(conn);
                }).then(function() {
                    return r.table(tableName).get(idValue).delete().run(conn);
                }).error(function(err) {
                    throw err;
                });
            });
        }, 5000);
    });
}

function test2() {
    // Test `each`
    console.log("Running test2");
    r.connect({port:port}, function(err, conn) {
        if (err) throw err;

        r.table(tableName).changes().run(conn, function(err, feed) {
            if (err) throw err;

            var count = 0;
            feed.each(function(err, data) {
                if (err) {
                    if ((count === 3) && (err.message.match(/^Changefeed aborted \(table unavailable/))) {
                        r.tableCreate(tableName).run(conn, function(err, result) {
                            if (err) throw err;
                            test3();
                        })
                    }
                    else {
                        throw err;
                    }
                }
                else {
                    if (count < 3) {
                        count++;
                    }
                    else {
                        throw new Error("Test shouldn't have enter this part of the code")
                    }
                }
            });
        });
        setTimeout(function() { // Wait one seconds before doing the writes to be sure that the feed is opened
            r.connect({port: port}, function(err, conn) {
                r.table(tableName).insert({}).run(conn).then(function() {
                    return r.table(tableName).insert({}).run(conn);
                }).then(function() {
                    return r.table(tableName).insert({}).run(conn);
                }).then(function() {
                    return r.tableDrop(tableName).run(conn);
                }).error(function(err) {
                    throw err;
                });
            });
        }, 1000);
    });
}

function test3() {
    // Test that some methods are not available
    console.log("Running test3");
    
    r.connect({port:port}, function(err, conn) {
        if (err) throw err;

        r.table(tableName).changes().run(conn, function(err, feed) {
            if (err) throw err;
            assert.throws(function() {
                feed.hasNext();
            }, function(err) {
                if (err.message === "`hasNext` is not available for feeds.") {
                    return true;
                }
            });

            assert.throws(function() {
                feed.toArray();
            }, function(err) {
                if (err.message === "`toArray` is not available for feeds.") {
                    return true;
                }
            });
            conn.close();
            test4();
        });
    });
}

function test4() {
    // Test `events
    console.log("Running test4");
    r.connect({port:port}, function(err, conn) {
        if (err) throw err;

        r.table(tableName).changes().run(conn, function(err, feed) {
            if (err) throw err;

            var count = 0;
            feed.on('error', function(err) {
                if ((count === 3) && (err.message.match(/^Changefeed aborted \(table unavailable/))) {
                    
                    // Test that Cursor's method were deactivated
                    assert.throws(function() {
                        feed.each();
                    }, function(err) {
                        if (err.message === "You cannot use the cursor interface and the EventEmitter interface at the same time.") {
                            return true;
                        }
                    });

                    assert.throws(function() {
                        feed.next();
                    }, function(err) {
                        if (err.message === "You cannot use the cursor interface and the EventEmitter interface at the same time.") {
                            return true;
                        }
                    });

                    r.tableCreate(tableName).run(conn, function(err, result) {
                        if (err) throw err;
                        conn.close();
                        test5();
                    })

                }
                else {
                    throw err;
                }
            })
            feed.on('data', function(data) {
                if (count < 3) {
                    count++;
                }
                else {
                    throw new Error("Test shouldn't have enter this part of the code")
                }
            });
        });

        setTimeout(function() { // Wait one seconds before doing the writes to be sure that the feed is opened
            r.connect({port: port}, function(err, conn) {
                r.table(tableName).insert({}).run(conn).then(function() {
                    return r.table(tableName).insert({}).run(conn);
                }).then(function() {
                    return r.table(tableName).insert({}).run(conn);
                }).then(function() {
                    return r.tableDrop(tableName).run(conn);
                }).error(function(err) {
                    throw err;
                });
            });
        }, 1000);
    });
}

function test5() {
    // Close a feed
    console.log("Running test5");
    r.connect({port:port}, function(err, conn) {
        if (err) throw err;

        r.table(tableName).changes().run(conn, function(err, feed) {
            if (err) throw err;

            feed.close();
            setTimeout(function() {
                // We give the feed 1 second to get closed
                assert.equal(feed._endFlag, true);
                conn.close();
                done();
            }, 1000);
        });
    });

}

function done() {
    console.log("Tests done.");
    process.exit(0);
}

r.connect({port:port}, function(err, conn) {
    if (err) throw err;
    r.tableList().run(conn, function(err, tables) {
        if (tables.indexOf(tableName) > -1) {
            test1();
        } else {
            r.tableCreate(tableName).run(conn, function(err, result) {
                if (err) throw err;
                test1();
            });
        }
    });
});
