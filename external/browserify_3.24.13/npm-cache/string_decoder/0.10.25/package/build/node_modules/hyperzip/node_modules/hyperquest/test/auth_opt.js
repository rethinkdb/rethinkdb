var test = require('tap').test;
var http = require('http');
var hyperquest = require('../');
var through = require('through');

var server = http.createServer(function (req, res) {
    var au = req.headers.authorization;
    if (!au) return res.end('ACCESS DENIED');
    
    var buf = Buffer(au.replace(/^Basic\s+/, ''), 'base64');
    var s = buf.toString().split(':');
    
    if (s[0] === 'moo' && s[1] === 'hax') {
        res.end('WELCOME TO ZOMBO COM');
    }
    else {
        res.end('ACCESS DENIED!!!');
    }
});

test('basic opts.auth', function (t) {
    t.plan(3);
    server.listen(0, function () {
        var port = server.address().port;
        checkUnauth(t, port);
        checkValid(t, port);
        checkInvalid(t, port);
    });
    t.on('end', server.close.bind(server));
});

function checkUnauth (t, port) {
    var r = hyperquest('http://localhost:' + port);
    var data = '';
    r.on('data', function (buf) { data += buf });
    r.on('end', function () { t.equal(data, 'ACCESS DENIED') });
}

function checkValid (t, port) {
    var r = hyperquest('http://localhost:' + port, { auth: 'moo:hax' });
    var data = '';
    r.on('data', function (buf) { data += buf });
    r.on('end', function () { t.equal(data, 'WELCOME TO ZOMBO COM') });
}

function checkInvalid (t, port) {
    var r = hyperquest('http://localhost:' + port, { auth: 'beep:boop' });
    var data = '';
    r.on('data', function (buf) { data += buf });
    r.on('end', function () { t.equal(data, 'ACCESS DENIED!!!') });
}
