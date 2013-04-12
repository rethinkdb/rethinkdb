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

var port = parseInt(process.argv[2], 10)

assertArgError(2, 0, function() { r.connect(); });

r.connect({port:port}, function(err, c) {
    assertNoError(err);

    var tbl = r.table('test');
    var num_rows = parseInt(process.argv[3], 10);
    console.log("Testing for "+num_rows);

    tbl.run(c, function(err, cur) {
        assertNoError(err);

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

            // This is the only way to know if the `each` has ended
            if (!cur.hasNext()) {
                if (i === num_rows) {
                    console.log("Test passed!");
                } else {
                    console.log("Test failed: expected "+num_rows+" rows but found "+i+".");
                }
                c.close();
            }
        });
    });
});
