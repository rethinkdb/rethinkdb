
// Load text encoder, still haven't resolved this issue
var te = require('./encoding.js');
global.TextEncoder = te.TextEncoder;
global.TextDecoder = te.TextDecoder;
var rethinkdb = require('./rethinkdb');

var q = rethinkdb.query;
var welcome = q.table('Welcome-rdb');

var conn = new rethinkdb.net.TcpConnection({host:'newton', port:12346},
function() {
    console.log('connected');

    welcome.distinct('id').run(function(response) {
        console.log('response'); 
        console.log(response);

        conn.close();
    });
},
function() {
    console.log('failed to connect');
});
