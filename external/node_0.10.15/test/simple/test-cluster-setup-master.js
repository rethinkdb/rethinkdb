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
var cluster = require('cluster');

if (cluster.isWorker) {

  // Just keep the worker alive
  process.send(process.argv[2]);

} else if (cluster.isMaster) {

  var checks = {
    args: false,
    setupEvent: false,
    settingsObject: false
  };

  var totalWorkers = 2;

  cluster.once('setup', function() {
    checks.setupEvent = true;

    var settings = cluster.settings;
    if (settings &&
        settings.args && settings.args[0] === 'custom argument' &&
        settings.silent === true &&
        settings.exec === process.argv[1]) {
      checks.settingsObject = true;
    }
  });

  // Setup master
  cluster.setupMaster({
    args: ['custom argument'],
    silent: true
  });

  var correctIn = 0;

  cluster.on('online', function lisenter(worker) {

    worker.once('message', function(data) {
      correctIn += (data === 'custom argument' ? 1 : 0);
      if (correctIn === totalWorkers) {
        checks.args = true;
      }
      worker.kill();
    });

    // All workers are online
    if (cluster.onlineWorkers === totalWorkers) {
      checks.workers = true;
    }
  });

  // Start all workers
  cluster.fork();
  cluster.fork();

  // Check all values
  process.once('exit', function() {
    assert.ok(checks.args, 'The arguments was noy send to the worker');
    assert.ok(checks.setupEvent, 'The setup event was never emitted');
    var m = 'The settingsObject do not have correct properties';
    assert.ok(checks.settingsObject, m);
  });

}
