var r = require('rethinkdb');

// This would be a lot easier if you used .each...
function log_rows(cursor, i, callback) {
    if (!cursor.hasNext()) {
        callback();
    } else {
        cursor.next(function(err, row) {
            console.log(["Row " + i + ":", row]);
            log_rows(cursor, i + 1, callback);
        });
    }
}

function insert_data(conn, cb) {
    var docs = [];
    for (var i = 0; i < 10; ++i) {
        docs.push({'value': i});
    }
    r.table('numbers').insert(docs).run(conn, cb);
}

r.connect({host: 'localhost', port: 28015}, function(err, conn) {
    r.tableCreate('numbers').run(conn, function() {
        insert_data(conn, function() {
            r.table('numbers').run(conn, function(err, cursor) {
                log_rows(cursor, 0, function() {
                    conn.close();
                });
            });
        });
    });
});
