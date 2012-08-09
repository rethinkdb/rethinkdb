var te = require('./encoding.js');
var TextEncoder = te.TextEncoder;
var TextDecoder = te.TextDecoder;
var rethinkdb = require('./rethinkdb');

var conn = new rethinkdb.net.TcpConnection({host:'localhost', port:11211});

var q = rethinkdb.reql.query;
var res = q.expr(1).compile();
q.expr(1).add(q.expr(3)).sub(q.expr(2)).between(1,2);
