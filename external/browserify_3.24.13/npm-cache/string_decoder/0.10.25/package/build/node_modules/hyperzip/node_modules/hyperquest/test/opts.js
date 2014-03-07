var test = require('tap').test;
var http = require('http');
var hyperquest = require('../');
var through = require('through');

var server = http.createServer(function (req, res) {
    res.setHeader('content-type', 'text/robot-speak');
    res.end('beep boop');
});

test('1st-arg options', function (t) {
    t.plan(2);
    server.listen(0, function () {
        var port = server.address().port;
        check(t, port);
    });
    t.on('end', server.close.bind(server));
});

function check (t, port) {
    var r = hyperquest(
        { uri: 'http://localhost:' + port },
        function (err, res) {
            t.equal(res.headers['content-type'], 'text/robot-speak');
        }
    );
    r.pipe(through(write, end));
    
    var data = '';
    function write (buf) { data += buf }
    function end () {
        t.equal(data, 'beep boop');
    }
}
