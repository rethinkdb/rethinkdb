var http = require('http');
var hyperquest = require('../');

var server = http.createServer(function (req, res) {
    res.write(req.url.slice(1) + '\n');
    setTimeout(res.end.bind(res), 3000);
});

server.listen(5000, function () {
    var pending = 20;
    for (var i = 0; i < 20; i++) {
        var r = hyperquest('http://localhost:5000/' + i);
        r.pipe(process.stdout, { end: false });
        r.on('end', function () {
            if (--pending === 0) server.close();
        });
    }
});

process.stdout.setMaxListeners(0); // turn off annoying warnings
