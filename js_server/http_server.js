// Include libraries
var http = require('http');
var fs = require('fs');

var port_offset = 888;

// Slice arguments and look for port.
var arguments = process.argv.splice(2);
if (arguments.length < 1) {
    console.log('Usage: node http_server.js <port>')
    return 1
}
port = parseInt(arguments[0]);
if (isNaN(port)) {
    console.log('Port must be an integer');
    return 1
}


var send_file = function(req, res, file, type) {
    fs.readFile(file, function(error, content) {
        if (error) {
            res.writeHead(499);
            res.end();
        }
        else {
            res.writeHead(200, { 'Content-Type': type });
            res.end(content, 'utf-8');
        }
    });

}

// Create an HTTP server
http.createServer(function (req, res) {
    //console.log(req.url);

    if (req.url.indexOf('.js') != -1) {
        type = 'text/javascript';
    }
    else if (req.url.indexOf('.css') != -1) {
        type = 'text/css';
    }
    else if (req.url.indexOf('.png') != -1) {
        type = 'image/png';
    }
    else if (req.url.indexOf('.gif') != -1) {
        type = 'image/gif';
    }
    else {
        type = 'text/html';
    }
    send_file(req, res, '.'+req.url, type);
    /*
    switch(req.url) {
        case '/':
            send_file(req, res, './index.html', 'text/html');
            break;
        case '/rethinkdb.js':
            send_file(req, res, './rethinkdb.js', 'text/javascript');
            break;
        case '/js_server.js':
            send_file(req, res, './js_server.js', 'text/javascript');
            break;
    }
    */


}).listen((port+port_offset));

console.log('Server running on port:'+(port+port_offset));

