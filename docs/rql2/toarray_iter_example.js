var r = require('rethinkdb');

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
                cursor.toArray(function(err, rows) {
                    for (var i in rows) {
                        console.log(["Row " + i + ":", rows[i]]);
                    }
                    conn.close();
                });
            });
        });
    });
});
