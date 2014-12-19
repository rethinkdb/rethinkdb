////
// Tests the driver cursor API
/////

var path = require('path');

process.on('uncaughtException', function(err) {
    console.log(err);
    if (err.stack) {
        console.log(err.toString() + err.stack.toString());
    } else {
        console.log(err.toString());
    }
    process.exit(1)
});

// -- get input

var port = parseInt(process.argv[2], 10)
var num_rows = parseInt(process.argv[3], 10);

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// --

var assertNoError = function(err) {
    if (err) {
        console.log(1);
        throw new Error("Error '"+err+"' not expected")
    }
};

var assertArgError = function(expected, found, callback) {
    var errFound = null;
    try {
        callback();
    } catch (err) {
        errFound = err;
    }

    if (!errFound) {
        throw new Error("No error thrown");
    } else if (!(errFound.msg !== "Expected "+expected+" argument"
                 +(expected === 1 ? "" : "s")+" but found "+found)) {
        throw new Error("Wrong error message: "+errFound.msg);
    }
};

var assert = function(predicate) {
    if (!predicate) {
        throw new Error("Assert failed");
    }
};

r.connect({port: port}).then(function(c) {
    
    var tbl = r.db('test').table('test');
    console.log("Testing for " + num_rows + " rows");


    tbl.run(c).then(function(cur) {
        cur.next().then(function(result) {
            assert(result);
            cur.toArray().then(function(result) {
                assert(result.length === (num_rows-1))
                var ar_to_send = []
                var limit=10000; // Keep a "big" value to try hitting `maximum call stack exceed`
                for(var i=0; i<limit; i++) {
                    ar_to_send.push(i);
                }
                r(ar_to_send).run(c).then(function(res) {
                    var i = 0;
                    res.each(function(err, res2) {
                        assert(res2 === ar_to_send[i])
                        i++;
                    })
                }).error(function(err) {
                    assertNoError(err);
                });

                // Test the toArray
                limit = 3;
                var ar_to_send2 = [0, 1, 2]
                r(ar_to_send2).run(c).then(function(res) {
                    res.toArray().then(function(res2) {
                        // Make sure we didn't create a copy here
                        assert(res === res2);

                        // Test values
                        for(var i=0; i<ar_to_send2.length; i++) {
                            assert(ar_to_send2[i] === res2[i]);
                        }
                    }).error(assertNoError)
                    res.next().then(function(row) {
                        assert(row === ar_to_send2[0])
                        res.toArray().then(function(res2) {
                            // Make sure we didn't create a copy here
                            assert(res2.length === (ar_to_send2.length-1));

                            // Test values
                            for(var i=0; i<res2.length; i++) {
                                assert(ar_to_send2[i+1] === res2[i]);
                            }
                            // Test reconnect, noreplyWait, close
                            c.reconnect().then(function(c) {
                                c.noreplyWait().then(function() {
                                    c.close().then(function() {
                                        c.reconnect().then(function(c) {
                                            c.noreplyWait().then(function() {
                                                c.close().then(function() {
                                                }).error(assertNoError)
                                            }).error(assertNoError)
                                        }).error(assertNoError)
                                    }).error(assertNoError)
                                }).error(assertNoError)
                            }).error(assertNoError)
                        }).error(assertNoError)
                    }).error(assertNoError)
                }).error(assertNoError)
            });
        });
    }).error(assertNoError)
}).error(assertNoError)

