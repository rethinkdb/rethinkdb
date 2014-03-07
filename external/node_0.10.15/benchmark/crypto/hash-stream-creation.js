// throughput benchmark
// creates a single hasher, then pushes a bunch of data through it
var common = require('../common.js');
var crypto = require('crypto');

var bench = common.createBenchmark(main, {
  writes: [500],
  algo: [ 'sha256', 'md5' ],
  type: ['asc', 'utf', 'buf'],
  out: ['hex', 'binary', 'buffer'],
  len: [2, 1024, 102400, 1024 * 1024],
  api: ['legacy', 'stream']
});

function main(conf) {
  var api = conf.api;
  if (api === 'stream' && process.version.match(/^v0\.[0-8]\./)) {
    console.error('Crypto streams not available until v0.10');
    // use the legacy, just so that we can compare them.
    api = 'legacy';
  }

  var crypto = require('crypto');
  var assert = require('assert');

  var message;
  var encoding;
  switch (conf.type) {
    case 'asc':
      message = new Array(conf.len + 1).join('a');
      encoding = 'ascii';
      break;
    case 'utf':
      message = new Array(conf.len / 2 + 1).join('ü');
      encoding = 'utf8';
      break;
    case 'buf':
      message = new Buffer(conf.len);
      message.fill('b');
      break;
    default:
      throw new Error('unknown message type: ' + conf.type);
  }

  var fn = api === 'stream' ? streamWrite : legacyWrite;

  bench.start();
  fn(conf.algo, message, encoding, conf.writes, conf.len, conf.out);
}

function legacyWrite(algo, message, encoding, writes, len, outEnc) {
  var written = writes * len;
  var bits = written * 8;
  var gbits = bits / (1024 * 1024 * 1024);

  while (writes-- > 0) {
    var h = crypto.createHash(algo);
    h.update(message, encoding);
    var res = h.digest(outEnc);

    // include buffer creation costs for older versions
    if (outEnc === 'buffer' && typeof res === 'string')
      res = new Buffer(res, 'binary');
  }

  bench.end(gbits);
}

function streamWrite(algo, message, encoding, writes, len, outEnc) {
  var written = writes * len;
  var bits = written * 8;
  var gbits = bits / (1024 * 1024 * 1024);

  while (writes-- > 0) {
    var h = crypto.createHash(algo);

    if (outEnc !== 'buffer')
      h.setEncoding(outEnc);

    h.write(message, encoding);
    h.end();
    h.read();
  }

  bench.end(gbits);
}
