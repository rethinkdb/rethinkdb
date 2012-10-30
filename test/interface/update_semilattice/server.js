// Copyright 2010-2012 RethinkDB, all rights reserved.
// Include libraries
var http = require('http');
var request = require('request');
var fs = require('fs');
var $ = require('jquery')

// Slice arguments and look for port.
var arguments = process.argv.splice(2);
if (arguments.length < 1) {
    console.log('Usage: node update_server.js <node-port-offset>')
    return 1
}
node_port_offset = parseInt(arguments[0]);
if (isNaN(node_port_offset)) {
    console.log('node-port-offset must be an integer');
    return 1
}

// Global variables
var service_port = 29015+node_port_offset+123


var send_file = function(req, res, file, type) {
    fs.readFile(file, function(error, content) {
        if (error) {
            res.writeHead(500);
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
    switch(req.url) {
        case '/':
            send_file(req, res, './index.html', 'text/html');
            break;
        case '/get_semilattice':
             request('http://newton:'+(node_port_offset+8080)+'/ajax/semilattice', function (error, response, body) {
                if (!error && response.statusCode == 200) {
                    res.writeHead(200, {'Content-Type': 'text/plain'});
                    res.end(body);
                }
            });
             break;
        case '/style.css':
            send_file(req, res, './style.css', 'text/css');
            break;
        case '/main.js':
            send_file(req, res, './main.js', 'application/javascript');
            break;
        case '/jquery-1.8.2.min.js':
            send_file(req, res, './jquery-1.8.2.min.js', 'application/javascript');
            break;
        case '/handlebars-1.0.0.beta.6.js':
            send_file(req, res, './handlebars-1.0.0.beta.6.js', 'application/javascript');
            break;
        case '/arrow_down.gif':
            send_file(req, res, './arrow_down.gif', 'image/gif');
            break;
        case '/arrow_right.gif':
            send_file(req, res, './arrow_right.gif', 'image/gif');
            break;
        case '/submit_semilattice':
            data = '{';
            data = '';
            req.on('data', function(chunk) {
                data += chunk.toString()
            });
            req.on('end', function() {
                //data += '}'
                data = JSON.parse(data);
                delete data['me']; // It's a read only value
                for(var namespace in data['rdb_namespaces']) {
                    delete data['rdb_namespaces'][namespace]['blueprint']; // Removing read only values
                }


                //console.log('   //+ Data sent by the user');
                //console.log(data);
                //console.log('   //+ End data sent by the user');
                //console.log(' ');


                // It's shameful to use jquery, but request doesn't want to work...
                console.log('Submitting...');
                $.ajax({
                    contentType: 'application/json',
                    url: 'http://newton:'+(node_port_offset+8080)+'/ajax/semilattice',
                    type: 'POST',
                    data: JSON.stringify(data),
                    success: function() {
                        console.log('Data sent.');
                    }
                });

                // Keeping it for later if we use request. There is a */ in Accept...
                default_headers = {
                    'User-Agent': 'Mozilla/5.0 (X11; Linux i686; rv:7.0.1) Gecko/20100101 Firefox/7.0.1',
                    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
                    'Accept-Language': 'en-us,en;q=0.5',
                    'Accept-Encoding': 'gzip, deflate',
                    'Accept-Charset': 'ISO-8859-1,utf-8;q=0.7,*;q=0.7',
                    'Cache-Control': 'max-age=0',
                    'Content-Type': 'application/json'
                };

                /*
                // This thing send data but the server doesn't get it... (node does...)
                request({
                        url: 'http://newton:'+(node_port_offset+8080)+'/ajax/semilattice',
                        headers: default_headers,
                        method: 'POST',
                        body: JSON.stringify(data)
                    },
                    function (err, res2, body) {
                    if (!err && res2.statusCode == 200) {
                        console.log('ok');
                    }
                });
                request({
                        url: 'http://newton:'+(service_port)+'/test',
                        headers: default_headers,
                        method: 'POST',
                        body: JSON.stringify(data)
                    },
                    function (err, res2, body) {
                    if (!err && res2.statusCode == 200) {
                        console.log('ok');
                    }
                });
                */                                                                 

                /*
                request.post('http://newton:'+(node_port_offset+8080)+'/ajax/semilattice').form(data);
                request.post('http://newton:'+(service_port)+'/').form(data);
                */

                res.end('', 'utf-8');
            });
            break;
        case '/test': // Just to test the data received
            data = '';
            req.on('data', function(chunk) {
                data += chunk.toString()
            });
            req.on('end', function() {
                console.log('   //+ Data sent to the server');
                console.log(data);
                console.log('   //+ End data sent to the server');
                console.log(' ');
            });
    }


}).listen((service_port));

console.log('Server running on port:'+(service_port));

