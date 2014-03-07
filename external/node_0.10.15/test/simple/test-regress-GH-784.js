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

// Regression test for GH-784
// https://github.com/joyent/node/issues/784
//
// The test works by making a total of 8 requests to the server.  The first
// two are made with the server off - they should come back as ECONNREFUSED.
// The next two are made with server on - they should come back successful.
// The next two are made with the server off - and so on.  Without the fix
// we were experiencing parse errors and instead of ECONNREFUSED.
var common = require('../common');
var http = require('http');
var assert = require('assert');


var server = http.createServer(function(req, res) {
  var body = '';

  req.setEncoding('utf8');
  req.on('data', function(chunk) {
    body += chunk;
  });

  req.on('end', function() {
    assert.equal('PING', body);
    res.writeHead(200);
    res.end('PONG');
  });
});


server.on('listening', pingping);


function serverOn() {
  console.error('Server ON');
  server.listen(common.PORT);
}


function serverOff() {
  console.error('Server OFF');
  server.close();
  pingping();
}

var responses = [];


function afterPing(result) {
  responses.push(result);
  console.error('afterPing. responses.length = ' + responses.length);
  switch (responses.length) {
    case 2:
      assert.ok(/ECONNREFUSED/.test(responses[0]));
      assert.ok(/ECONNREFUSED/.test(responses[1]));
      serverOn();
      break;

    case 4:
      assert.ok(/success/.test(responses[2]));
      assert.ok(/success/.test(responses[3]));
      serverOff();
      break;

    case 6:
      assert.ok(/ECONNREFUSED/.test(responses[4]));
      assert.ok(/ECONNREFUSED/.test(responses[5]));
      serverOn();
      break;

    case 8:
      assert.ok(/success/.test(responses[6]));
      assert.ok(/success/.test(responses[7]));
      server.close();
      // we should go to process.on('exit') from here.
      break;
  }
}


function ping() {
  console.error('making req');

  var opt = {
    port: common.PORT,
    path: '/ping',
    method: 'POST'
  };

  var req = http.request(opt, function(res) {
    var body = '';

    res.setEncoding('utf8');
    res.on('data', function(chunk) {
      body += chunk;
    });

    res.on('end', function() {
      assert.equal('PONG', body);
      assert.ok(!hadError);
      gotEnd = true;
      afterPing('success');
    });
  });

  req.end('PING');

  var gotEnd = false;
  var hadError = false;

  req.on('error', function(error) {
    console.log('Error making ping req: ' + error);
    hadError = true;
    assert.ok(!gotEnd);
    afterPing(error.message);
  });
}



function pingping() {
  ping();
  ping();
}


pingping();




process.on('exit', function() {
  console.error("process.on('exit')");
  console.error(responses);

  assert.equal(8, responses.length);
});
