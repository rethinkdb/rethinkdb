function raise(msg) {
    throw msg;
}

function assertEqual(val, expected) {
    if (val !== expected) {
        raise("test failed");
    }
}

function print(arg) {
    console.log(arg);
}

// Load text encoder, still haven't resolved this issue
var te = require('./encoding.js');
global.TextEncoder = te.TextEncoder;
global.TextDecoder = te.TextDecoder;

var rethinkdb = require('./rethinkdb');
var q = rethinkdb.query;
var welcome = q.table('Welcome-rdb');

function runTests() {
    q(1).add(3).mul(q(8).div(2)).run(print);
    welcome.map(q.R('num').add(12)).run(print);

    welcome.length().run(print);
    welcome.filter({'id':1}).run(print);
    welcome.filter(q.R('id').eq(1)).run(print);
    welcome.filter(q.fn('row', q.R('$row.id').eq(1))).run(print);
}

var conn = new rethinkdb.net.TcpConnection({host:'newton', port:12346},
function() {
    runTests();
    conn.close();
},
function() {
    raise("Could not establish connection");
});
