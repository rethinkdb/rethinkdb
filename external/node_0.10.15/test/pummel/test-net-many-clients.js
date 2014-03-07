// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

var common = require('../common');
var assert = require('assert');
var net = require('net');

// settings
var bytes = 1024 * 40;
var concurrency = 100;
var connections_per_client = 5;

// measured
var total_connections = 0;

var body = '';
for (var i = 0; i < bytes; i++) {
  body += 'C';
}

var server = net.createServer(function(c) {
  console.log('connected');
  total_connections++;
  common.print('#');
  c.write(body);
  c.end();
});

function runClient(callback) {
  var client = net.createConnection(common.PORT);

  client.connections = 0;

  client.setEncoding('utf8');

  client.on('connect', function() {
    common.print('c');
    client.recved = '';
    client.connections += 1;
  });

  client.on('data', function(chunk) {
    this.recved += chunk;
  });

  client.on('end', function() {
    client.end();
  });

  client.on('error', function(e) {
    console.log('\n\nERROOOOOr');
    throw e;
  });

  client.on('close', function(had_error) {
    common.print('.');
    assert.equal(false, had_error);
    assert.equal(bytes, client.recved.length);

    if (client.fd) {
      console.log(client.fd);
    }
    assert.ok(!client.fd);

    if (this.connections < connections_per_client) {
      this.connect(common.PORT);
    } else {
      callback();
    }
  });
}

server.listen(common.PORT, function() {
  var finished_clients = 0;
  for (var i = 0; i < concurrency; i++) {
    runClient(function() {
      if (++finished_clients == concurrency) server.close();
    });
  }
});

process.on('exit', function() {
  assert.equal(connections_per_client * concurrency, total_connections);
  console.log('\nokay!');
});
