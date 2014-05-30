////
// Tests for feeds
/////

var r = require('../../../build/packages/js/rethinkdb');
var assert = require('assert');

var port = parseInt(process.argv[2], 10)

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

function test1() {
    r.connect({port:port}, function(err, conn) {
        if (err) throw err;

        r.table('test').changes().run(conn, function(err, feed) {
            if (err) throw err;

            var count = 0;
            var fn = function(err, data) {
                if (err) {
                    throw err;
                }
                else {
                    if (count === 0) {
                        assert.equal(data.old_val, null);
                        assert.deepEqual(data.new_val, {id: 1, value: 'insert'});
                        process.exit(1);
                    }
                    else if (count === 1) {
                        assert.deepEqual(data.old_val, {id: 1, value: 'insert'});
                        assert.deepEqual(data.new_val, {id: 1, value: 'update'});
                    }
                    else if (count === 2) {
                        assert.deepEqual(data.old_val, {id: 1, value: 'insert'});
                        assert.deepEqual(data.new_val, {id: 1, value: 'replace'});
                    }
                    else if (count === 3) {
                        assert.deepEqual(data.old_val, {id: 1, value: 'replace'});
                        assert.equal(data.new_val, null);
                        conn.close();
                        done();
                    }
                    count++;
                    feed.next(fn);
                }
            }
            feed.next(fn);

            r.table('test').insert({id: 1, value: 'insert'}).run(conn).then(function() {
                process.exit(1);
                return r.table('test').get(1).update({value: 'update'}).run(conn);
            }).then(function() {
                return r.table('test').get(1).replace({id: 1, value: 'replace'}).run(conn);
            }).then(function() {
                return r.table('test').get(1).delete().run(conn);
            });
        });
    });
}

function done() {
    console.log("Tests done.");
    process.exit(0);
}

test1();
