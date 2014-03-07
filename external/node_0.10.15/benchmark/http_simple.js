var path = require('path'),
    exec = require('child_process').exec,
    http = require('http');

var port = parseInt(process.env.PORT || 8000);

var fixed = makeString(20 * 1024, 'C'),
    storedBytes = {},
    storedBuffer = {},
    storedUnicode = {};

var useDomains = process.env.NODE_USE_DOMAINS;

// set up one global domain.
if (useDomains) {
  var domain = require('domain');
  var gdom = domain.create();
  gdom.on('error', function(er) {
    console.error('Error on global domain', er);
    throw er;
  });
  gdom.enter();
}

var server = module.exports = http.createServer(function (req, res) {
  if (useDomains) {
    var dom = domain.create();
    dom.add(req);
    dom.add(res);
  }

  var commands = req.url.split('/');
  var command = commands[1];
  var body = '';
  var arg = commands[2];
  var n_chunks = parseInt(commands[3], 10);
  var status = 200;

  if (command == 'bytes') {
    var n = ~~arg;
    if (n <= 0)
      throw new Error('bytes called with n <= 0')
    if (storedBytes[n] === undefined) {
      storedBytes[n] = makeString(n, 'C');
    }
    body = storedBytes[n];

  } else if (command == 'buffer') {
    var n = ~~arg;
    if (n <= 0)
      throw new Error('buffer called with n <= 0');
    if (storedBuffer[n] === undefined) {
      storedBuffer[n] = new Buffer(n);
      for (var i = 0; i < n; i++) {
        storedBuffer[n][i] = 'C'.charCodeAt(0);
      }
    }
    body = storedBuffer[n];

  } else if (command == 'unicode') {
    var n = ~~arg;
    if (n <= 0)
      throw new Error('unicode called with n <= 0');
    if (storedUnicode[n] === undefined) {
      storedUnicode[n] = makeString(n, '\u263A');
    }
    body = storedUnicode[n];

  } else if (command == 'quit') {
    res.connection.server.close();
    body = 'quitting';

  } else if (command == 'fixed') {
    body = fixed;

  } else if (command == 'echo') {
    res.writeHead(200, { 'Content-Type': 'text/plain',
                         'Transfer-Encoding': 'chunked' });
    req.pipe(res);
    return;

  } else {
    status = 404;
    body = 'not found\n';
  }

  // example: http://localhost:port/bytes/512/4
  // sends a 512 byte body in 4 chunks of 128 bytes
  if (n_chunks > 0) {
    res.writeHead(status, { 'Content-Type': 'text/plain',
                            'Transfer-Encoding': 'chunked' });
    // send body in chunks
    var len = body.length;
    var step = Math.floor(len / n_chunks) || 1;

    for (var i = 0, n = (n_chunks - 1); i < n; ++i) {
      res.write(body.slice(i * step, i * step + step));
    }
    res.end(body.slice((n_chunks - 1) * step));
  } else {
    var content_length = body.length.toString();

    res.writeHead(status, { 'Content-Type': 'text/plain',
                            'Content-Length': content_length });
    res.end(body);
  }
});

function makeString(size, c) {
  var s = '';
  while (s.length < size) {
    s += c;
  }
  return s;
}

server.listen(port, function () {
  if (module === require.main)
    console.error('Listening at http://127.0.0.1:'+port+'/');
});
