////
// Tests the driver cursor API
/////

process.on('uncaughtException', function(err) {
    console.log(err);
    console.log(err.toString() + err.stack.toString());
});

var r = require('../../../drivers/javascript/build/rethinkdb');

var assertNoError = function(err) {
    if (err) {
        throw "Error "+err+" not expected"
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

var assert = function(predicate) {
    if (!predicate) {
        throw new Error("Assert failed");
    }
};

var port = parseInt(process.argv[2], 10)

assertArgError(2, 0, function() { r.connect(); });

r.connect({port:port}, function(err, c) {
    assertNoError(err);

    var tbl = r.table('test');
    var num_rows = parseInt(process.argv[3], 10);
    console.log("Testing for "+num_rows);

    tbl.run(c, function(err, cur) {
        assertNoError(err);

        // Test that closing the cursor cleans
        // up the state associated with it
        assert(Object.keys(c.outstandingCallbacks).length === 1);
        cur.close();
        assert(Object.keys(c.outstandingCallbacks).length === 0);

        tbl.run(c, function(err, cur) {
            assertNoError(err);

            // This is getting unruley but we want to make sure that array results
            // support the connection api too
            r([1,2,3]).run(c, function(err, res) {
                assertNoError(err);

                // yes, res is an array that supports array ops
                res.push(4, 5);
                res.toArray(function(err, res) {
                    assertNoError(err);
                    assert(res.length == 5);
                    for (var i = 1; i <= 5; ++i) {
                        assert(res[i - 1] == i);
                    }
                });
            });

            // These simply test that we appropriately check arg numbers for
            // cursor api methods
            assertArgError(1, 0, function() { cur.each(); });
            assertArgError(1, 0, function() { cur.toArray(); });
            assertArgError(1, 0, function() { cur.toArray(); });
            assertArgError(1, 0, function() { cur.next(); });
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
