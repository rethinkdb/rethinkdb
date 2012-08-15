
// Load text encoder, still haven't resolved this issue
var te = require('./encoding.js');
global.TextEncoder = te.TextEncoder;
global.TextDecoder = te.TextDecoder;
var rethinkdb = require('./rethinkdb');

var q = rethinkdb.query;

var conn = new rethinkdb.net.TcpConnection({host:'newton', port:12346},
function() {
    console.log('connected');

    conn.run(q.expr([1,2,3,4]).map(q.fn('a', q.expr(1))), function(response) {
        console.log('response'); 
        console.log(response);

        conn.close();
    });
},
function() {
    console.log('failed to connect');
});
