// A stand-alone web UI for RethinkDB

var express = require('express');
var proxyServer = require('http-route-proxy');

var rethinkdbWeb = process.argv[2] || 'localhost:8080';
var port = process.env.PORT || 3000;

var app = express();

app.use(express.static('dist'));

app.use(proxyServer.connect({
    to: rethinkdbWeb,
    route: ['/ajax']
}));

app.listen(port, function () {
    console.log('Listening on port ' + port);
});
