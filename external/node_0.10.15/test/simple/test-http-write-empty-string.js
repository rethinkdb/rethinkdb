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

var http = require('http');

var server = http.createServer(function(request, response) {
  console.log('responding to ' + request.url);

  response.writeHead(200, {'Content-Type': 'text/plain'});
  response.write('1\n');
  response.write('');
  response.write('2\n');
  response.write('');
  response.end('3\n');

  this.close();
});

var response = '';

process.on('exit', function() {
  assert.equal('1\n2\n3\n', response);
});


server.listen(common.PORT, function() {
  http.get({ port: common.PORT }, function(res) {
    assert.equal(200, res.statusCode);
    res.setEncoding('ascii');
    res.on('data', function(chunk) {
      response += chunk;
    });
    common.error('Got /hello response');
  });
});

