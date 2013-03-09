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

r.connect({port:30363}, function(err, c) {
    assertNoError(err);

    var tbl = r.table('test');
    var num_rows = parseInt(process.argv[2], 10);
    console.log("Testing for "+num_rows);

    tbl.run(c, function(err, cur) {
        assertNoError(err);

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
