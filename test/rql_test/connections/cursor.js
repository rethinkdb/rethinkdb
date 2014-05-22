////
// Tests the driver cursor API
/////

process.on('uncaughtException', function(err) {
    console.log(err);
    if (err.stack) {
        console.log(err.toString() + err.stack.toString());
    } else {
        console.log(err.toString());
    }
    process.exit(1)
});

var r = require('../../../build/packages/js/rethinkdb');

var assertNoError = function(err) {
    if (err) {
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
    } else if (!(errFound.msg !== "Expected "+expected+" argument(s) but found "+found)) {
        throw new Error("Wrong error message: "+errFound.msg);
    }
};

var assertArgVarError = function(min, max, found, callback) {
    var errFound = null;
    try {
        callback();
    } catch (err) {
        errFound = err;
    }

    if (!errFound) {
        throw new Error("No error thrown");
    } else if (
            (errFound.msg !== "Expected "+min+" or more argument(s) but found "+found+".")
            && (errFound.msg !== "Expected "+max+" or fewer argument(s) but found "+found+".")
            && (errFound.msg !== "Expected between "+min+" and "+max+" argument(s) but found "+found+".")) {
        throw new Error("Wrong error message: "+errFound.msg);
    }
};


var assert = function(predicate) {
    if (!predicate) {
        throw new Error("Assert failed");
    }
};

var port = parseInt(process.argv[2], 10)

r.connect({port:port}, function(err, c) {
    assertNoError(err);

    var tbl = r.table('test');
    var num_rows = parseInt(process.argv[3], 10);
    console.log("Testing for "+num_rows);

    tbl.run(c, function(err, cur) {
        assertNoError(err);

        // Closing the cursor won't immediately clean up the callback state since
        // we need to leave a callback behind to deal with the STOP response.
        cur.close();

        tbl.run(c, function(err, cur) {
            assertNoError(err);

            // This is getting unruley but we want to make sure that array results
            // support the connection api too
            
            var ar_to_send = []
            var limit=10000; // Keep a "big" value to try hitting `maximum call stack exceed`
            for(var i=0; i<limit; i++) {
                ar_to_send.push(i);
            }
            r(ar_to_send).run(c, function(err, res) {
                var i = 0;
                res.each(function(err, res2) {
                    assert(res2 === ar_to_send[i])
                    i++;
                })
            })

            // Test the toArray
            limit = 3;
            var ar_to_send2 = [0, 1, 2]
            r(ar_to_send2).run(c, function(err, res) {
                res.toArray(function(err, res2) {
                    // Make sure we didn't create a copy here
                    assert(res === res2);

                    // Test values
                    for(var i=0; i<ar_to_send2.length; i++) {
                        assert(ar_to_send2[i] === res2[i]);
                    }
                })
                res.next( function(err, row) {
                    assert(row === ar_to_send2[0])
                    res.toArray(function(err, res2) {
                        // Make sure we didn't create a copy here
                        assert(res2.length === (ar_to_send2.length-1));

                        // Test values
                        for(var i=0; i<res2.length; i++) {
                            assert(ar_to_send2[i+1] === res2[i]);
                        }
                    })
                })


            })

            // Test that we really have an array and can play with it
            limit = 3;
            var ar_to_send3 = [0, 1, 2]
            r(ar_to_send3).run(c, function(err, res) {
                assertNoError(err);

                // yes, res is an array that supports array ops
                res.push(limit, limit+1);
                assert(res[limit] === limit);
                assert(res[limit+1] === limit+1);
                assert(res.length == (limit+2));
            });

            // These simply test that we appropriately check arg numbers for
            // cursor api methods
            assertArgError(1, 0, function() { cur.each(); });
            assertArgError(0, 1, function() { cur.hasNext(1); });
            assertArgError(0, 1, function() { cur.close(1); });
            assertArgError(0, 1, function() { cur.toString(1); });

            var i = 0;
            cur.each(function(err, row) {
                assertNoError(err);
                i++;
            }, function() {
                if (i === num_rows) {
                    console.log("Test passed!");
                } else {
                    console.log("Test failed: expected "+num_rows+" rows but found "+i+".");
                    process.exit(1)
                }
                c.close();
            });
        });
    });
});
