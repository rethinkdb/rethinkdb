var fs = require ('fs');
var net = require('net');
var join = require('path').join;
var file = join(__dirname, 'fixtures','all_npm.json');
var JSONStream = require('../');


var server = net.createServer(function(client) {
    var parser = JSONStream.parse([]);
    parser.on('close', function() {
        console.error('PASSED');
        server.close();
    });
    client.pipe(parser);
    client.destroy();
});
server.listen(9999);


var client = net.connect({ port : 9999 }, function() {
    fs.createReadStream(file).pipe(client);
});
