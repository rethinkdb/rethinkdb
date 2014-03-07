var common = require('../common.js');
var PORT = common.PORT;

var bench = common.createBenchmark(main, {
  // unicode confuses ab on os x.
  type: ['bytes', 'buffer'],
  length: [4, 1024, 102400],
  c: [50, 500]
});

function main(conf) {
  process.env.PORT = PORT;
  var spawn = require('child_process').spawn;
  var server = require('../http_simple.js');
  setTimeout(function() {
    var path = '/' + conf.type + '/' + conf.length; //+ '/' + conf.chunks;
    var args = ['-r', 5000, '-t', 8, '-c', conf.c];

    bench.http(path, args, function() {
      server.close();
    });
  }, 2000);
}
