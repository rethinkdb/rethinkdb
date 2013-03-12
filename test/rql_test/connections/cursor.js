////
// Tests the driver cursor API
/////

process.on('uncaughtException', function(err) {
    console.log(err);
    console.log(err.toString() + err.stack.toString());
});

var r = require('../../../drivers/javascript/build/rethinkdb');

var port = parseInt(process.env.RQL_TEST_SERVER_PORT, 10)

var num_rows = parseInt(process.env.RQL_TEST_NUM_ROWS, 10);

exports['test cursor'] = function(test){
    console.log("Testing for " + num_rows + " rows");

    test.expect(num_rows + 4);

    var go = function(){
        r.connect({port:port}, function(err, c) {
            test.equal(err, null);

            var tbl = r.table('test');

            tbl.run(c, function(err, cur) {
                test.equal(err, null);

                var i = 0;
                cur.each(function(err, row) {
                    test.equal(err, null);

                    i++;

                    // This is the only way to know if the `each` has ended
                    if (!cur.hasNext()) {
                        test.equal(i, num_rows);
                        c.close();
                        test.done();
                    }
                });
            });
        });
    };
    test.doesNotThrow(go);
};
