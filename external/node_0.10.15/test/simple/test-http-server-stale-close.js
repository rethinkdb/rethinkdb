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
var util = require('util');
var fork = require('child_process').fork;

if (process.env.NODE_TEST_FORK) {
  var req = http.request({
    headers: {'Content-Length': '42'},
    method: 'POST',
    host: '127.0.0.1',
    port: common.PORT,
  }, process.exit);
  req.write('BAM');
  req.end();
}
else {
  var server = http.createServer(function(req, res) {
    res.writeHead(200, {'Content-Length': '42'});
    req.pipe(res);
    req.on('close', function() {
      server.close();
      res.end();
    });
  });
  server.listen(common.PORT, function() {
    fork(__filename, {
      env: util._extend(process.env, {NODE_TEST_FORK: '1'})
    });
  });
}
