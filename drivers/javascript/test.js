
// Load text encoder, still haven't resolved this issue
var te = require('./encoding.js');
global.TextEncoder = te.TextEncoder;
global.TextDecoder = te.TextDecoder;
var rethinkdb = require('./rethinkdb');

var q = rethinkdb.query;

var conn = new rethinkdb.net.TcpConnection({host:'newton', port:12346},
function() {
    console.log('connected');

    conn.run(q.expr(1).add(q.expr(2)).lt(q.expr(2)), function(response) {
        console.log('response'); 
        console.log(response);

        conn.close();
    });
},
function() {
    console.log('failed to connect');
});
