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

assert.equal('NODE_UNIQUE_ID' in process.env, false,
      'NODE_UNIQUE_ID should be removed on startup');

function forEach(obj, fn) {
  Object.keys(obj).forEach(function(name, index) {
    fn(obj[name], name, index);
  });
}


if (cluster.isWorker) {
  var http = require('http');
  http.Server(function() {

  }).listen(common.PORT, '127.0.0.1');
}

else if (cluster.isMaster) {

  var checks = {
    cluster: {
      events: {
        fork: false,
        online: false,
        listening: false,
        exit: false
      },
      equal: {
        fork: false,
        online: false,
        listening: false,
        exit: false
      }
    },

    worker: {
      events: {
        online: false,
        listening: false,
        exit: false
      },
      equal: {
        online: false,
        listening: false,
        exit: false
      },
      states: {
        none: false,
        online: false,
        listening: false,
        dead: false
      }
    }
  };

  var worker;
  var stateNames = Object.keys(checks.worker.states);

  //Check events, states, and emit arguments
  forEach(checks.cluster.events, function(bool, name, index) {

    //Listen on event
    cluster.on(name, function(/* worker */) {

      //Set event
      checks.cluster.events[name] = true;

      //Check argument
      checks.cluster.equal[name] = worker === arguments[0];

      //Check state
      var state = stateNames[index];
      checks.worker.states[state] = (state === worker.state);
    });
  });

  //Kill worker when listening
  cluster.on('listening', function() {
    worker.kill();
  });

  //Kill process when worker is killed
  cluster.on('exit', function() {
    process.exit(0);
  });

  //Create worker
  worker = cluster.fork();
  assert.equal(worker.id, 1);
  assert.ok(worker instanceof cluster.Worker,
      'the worker is not a instance of the Worker constructor');

  //Check event
  forEach(checks.worker.events, function(bool, name, index) {
    worker.on(name, function() {
      //Set event
      checks.worker.events[name] = true;

      //Check argument
      checks.worker.equal[name] = (worker === this);

      switch (name) {
        case 'exit':
          assert.equal(arguments[0], worker.process.exitCode);
          assert.equal(arguments[1], worker.process.signalCode);
          assert.equal(arguments.length, 2);
          break;

        case 'listening':
          assert.equal(arguments.length, 1);
          var expect = { address: '127.0.0.1',
                         port: common.PORT,
                         addressType: 4,
                         fd: undefined };
          assert.deepEqual(arguments[0], expect);
          break;

        default:
          assert.equal(arguments.length, 0);
          break;
      }
    });
  });

  //Check all values
  process.once('exit', function() {
    //Check cluster events
    forEach(checks.cluster.events, function(check, name) {
      assert.ok(check, 'The cluster event "' + name + '" on the cluster ' +
                'object did not fire');
    });

    //Check cluster event arguments
    forEach(checks.cluster.equal, function(check, name) {
      assert.ok(check, 'The cluster event "' + name + '" did not emit ' +
                'with correct argument');
    });

    //Check worker states
    forEach(checks.worker.states, function(check, name) {
      assert.ok(check, 'The worker state "' + name + '" was not set to true');
    });

    //Check worker events
    forEach(checks.worker.events, function(check, name) {
      assert.ok(check, 'The worker event "' + name + '" on the worker object ' +
                'did not fire');
    });

    //Check worker event arguments
    forEach(checks.worker.equal, function(check, name) {
      assert.ok(check, 'The worker event "' + name + '" did not emit with ' +
                'corrent argument');
    });
  });

}
