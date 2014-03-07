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

var assert = require('assert');
var util = require('util');
var events = require('events');

var UDP = process.binding('udp_wrap').UDP;

var BIND_STATE_UNBOUND = 0;
var BIND_STATE_BINDING = 1;
var BIND_STATE_BOUND = 2;

// lazily loaded
var cluster = null;
var dns = null;
var net = null;


// no-op callback
function noop() {
}


function isIP(address) {
  if (!net)
    net = require('net');

  return net.isIP(address);
}


function lookup(address, family, callback) {
  if (!dns)
    dns = require('dns');

  return dns.lookup(address, family, callback);
}


function lookup4(address, callback) {
  return lookup(address || '0.0.0.0', 4, callback);
}


function lookup6(address, callback) {
  return lookup(address || '::0', 6, callback);
}


function newHandle(type) {
  if (type == 'udp4') {
    var handle = new UDP;
    handle.lookup = lookup4;
    return handle;
  }

  if (type == 'udp6') {
    var handle = new UDP;
    handle.lookup = lookup6;
    handle.bind = handle.bind6;
    handle.send = handle.send6;
    return handle;
  }

  if (type == 'unix_dgram')
    throw new Error('unix_dgram sockets are not supported any more.');

  throw new Error('Bad socket type specified. Valid types are: udp4, udp6');
}


exports._createSocketHandle = function(address, port, addressType, fd) {
  // Opening an existing fd is not supported for UDP handles.
  assert(typeof fd !== 'number' || fd < 0);

  var handle = newHandle(addressType);

  if (port || address) {
    var r = handle.bind(address, port || 0, 0);
    if (r == -1) {
      handle.close();
      handle = null;
    }
  }

  return handle;
};


function Socket(type, listener) {
  events.EventEmitter.call(this);

  var handle = newHandle(type);
  handle.owner = this;

  this._handle = handle;
  this._receiving = false;
  this._bindState = BIND_STATE_UNBOUND;
  this.type = type;
  this.fd = null; // compatibility hack

  if (typeof listener === 'function')
    this.on('message', listener);
}
util.inherits(Socket, events.EventEmitter);
exports.Socket = Socket;


exports.createSocket = function(type, listener) {
  return new Socket(type, listener);
};


function startListening(socket) {
  socket._handle.onmessage = onMessage;
  // Todo: handle errors
  socket._handle.recvStart();
  socket._receiving = true;
  socket._bindState = BIND_STATE_BOUND;
  socket.fd = -42; // compatibility hack

  socket.emit('listening');
}

function replaceHandle(self, newHandle) {

  // Set up the handle that we got from master.
  newHandle.lookup = self._handle.lookup;
  newHandle.bind = self._handle.bind;
  newHandle.send = self._handle.send;
  newHandle.owner = self;

  // Replace the existing handle by the handle we got from master.
  self._handle.close();
  self._handle = newHandle;
}

Socket.prototype.bind = function(/*port, address, callback*/) {
  var self = this;

  self._healthCheck();

  if (this._bindState != BIND_STATE_UNBOUND)
    throw new Error('Socket is already bound');

  this._bindState = BIND_STATE_BINDING;

  if (typeof arguments[arguments.length - 1] === 'function')
    self.once('listening', arguments[arguments.length - 1]);

  var UDP = process.binding('udp_wrap').UDP;
  if (arguments[0] instanceof UDP) {
    replaceHandle(self, arguments[0]);
    startListening(self);
    return;
  }

  var port = arguments[0];
  var address = arguments[1];
  if (typeof address === 'function') address = '';  // a.k.a. "any address"

  // resolve address first
  self._handle.lookup(address, function(err, ip) {
    if (err) {
      self._bindState = BIND_STATE_UNBOUND;
      self.emit('error', err);
      return;
    }

    if (!cluster)
      cluster = require('cluster');

    if (cluster.isWorker) {
      cluster._getServer(self, ip, port, self.type, -1, function(handle) {
        if (!self._handle)
          // handle has been closed in the mean time.
          return handle.close();

        replaceHandle(self, handle);
        startListening(self);
      });

    } else {
      if (!self._handle)
        return; // handle has been closed in the mean time

      if (self._handle.bind(ip, port || 0, /*flags=*/ 0)) {
        self.emit('error', errnoException(process._errno, 'bind'));
        self._bindState = BIND_STATE_UNBOUND;
        // Todo: close?
        return;
      }

      startListening(self);
    }
  });
};


// thin wrapper around `send`, here for compatibility with dgram_legacy.js
Socket.prototype.sendto = function(buffer,
                                   offset,
                                   length,
                                   port,
                                   address,
                                   callback) {
  if (typeof offset !== 'number' || typeof length !== 'number')
    throw new Error('send takes offset and length as args 2 and 3');

  if (typeof address !== 'string')
    throw new Error(this.type + ' sockets must send to port, address');

  this.send(buffer, offset, length, port, address, callback);
};


Socket.prototype.send = function(buffer,
                                 offset,
                                 length,
                                 port,
                                 address,
                                 callback) {
  var self = this;

  if (!Buffer.isBuffer(buffer))
    throw new TypeError('First argument must be a buffer object.');

  if (offset >= buffer.length)
    throw new Error('Offset into buffer too large');

  if (offset + length > buffer.length)
    throw new Error('Offset + length beyond buffer length');

  callback = callback || noop;

  self._healthCheck();

  if (self._bindState == BIND_STATE_UNBOUND)
    self.bind(0, null);

  // If the socket hasn't been bound yet, push the outbound packet onto the
  // send queue and send after binding is complete.
  if (self._bindState != BIND_STATE_BOUND) {
    // If the send queue hasn't been initialized yet, do it, and install an
    // event handler that flushes the send queue after binding is done.
    if (!self._sendQueue) {
      self._sendQueue = [];
      self.once('listening', function() {
        // Flush the send queue.
        for (var i = 0; i < self._sendQueue.length; i++)
          self.send.apply(self, self._sendQueue[i]);
        self._sendQueue = undefined;
      });
    }
    self._sendQueue.push([buffer, offset, length, port, address, callback]);
    return;
  }

  self._handle.lookup(address, function(err, ip) {
    if (err) {
      if (callback) callback(err);
      self.emit('error', err);
    }
    else if (self._handle) {
      var req = self._handle.send(buffer, offset, length, port, ip);
      if (req) {
        req.oncomplete = afterSend;
        req.cb = callback;
      }
      else {
        // don't emit as error, dgram_legacy.js compatibility
        var err = errnoException(process._errno, 'send');
        process.nextTick(function() {
          callback(err);
        });
      }
    }
  });
};


function afterSend(status, handle, req, buffer) {
  var self = handle.owner;

  if (req.cb)
    req.cb(null, buffer.length); // compatibility with dgram_legacy.js
}


Socket.prototype.close = function() {
  this._healthCheck();
  this._stopReceiving();
  this._handle.close();
  this._handle = null;
  this.emit('close');
};


Socket.prototype.address = function() {
  this._healthCheck();

  var address = this._handle.getsockname();
  if (!address)
    throw errnoException(process._errno, 'getsockname');

  return address;
};


Socket.prototype.setBroadcast = function(arg) {
  if (this._handle.setBroadcast((arg) ? 1 : 0)) {
    throw errnoException(process._errno, 'setBroadcast');
  }
};


Socket.prototype.setTTL = function(arg) {
  if (typeof arg !== 'number') {
    throw new TypeError('Argument must be a number');
  }

  if (this._handle.setTTL(arg)) {
    throw errnoException(process._errno, 'setTTL');
  }

  return arg;
};


Socket.prototype.setMulticastTTL = function(arg) {
  if (typeof arg !== 'number') {
    throw new TypeError('Argument must be a number');
  }

  if (this._handle.setMulticastTTL(arg)) {
    throw errnoException(process._errno, 'setMulticastTTL');
  }

  return arg;
};


Socket.prototype.setMulticastLoopback = function(arg) {
  arg = arg ? 1 : 0;

  if (this._handle.setMulticastLoopback(arg)) {
    throw errnoException(process._errno, 'setMulticastLoopback');
  }

  return arg; // 0.4 compatibility
};


Socket.prototype.addMembership = function(multicastAddress,
                                          interfaceAddress) {
  this._healthCheck();

  if (!multicastAddress) {
    throw new Error('multicast address must be specified');
  }

  if (this._handle.addMembership(multicastAddress, interfaceAddress)) {
    throw new errnoException(process._errno, 'addMembership');
  }
};


Socket.prototype.dropMembership = function(multicastAddress,
                                           interfaceAddress) {
  this._healthCheck();

  if (!multicastAddress) {
    throw new Error('multicast address must be specified');
  }

  if (this._handle.dropMembership(multicastAddress, interfaceAddress)) {
    throw new errnoException(process._errno, 'dropMembership');
  }
};


Socket.prototype._healthCheck = function() {
  if (!this._handle)
    throw new Error('Not running'); // error message from dgram_legacy.js
};


Socket.prototype._stopReceiving = function() {
  if (!this._receiving)
    return;

  this._handle.recvStop();
  this._receiving = false;
  this.fd = null; // compatibility hack
};


function onMessage(handle, slab, start, len, rinfo) {
  var self = handle.owner;
  if (!slab) {
    return self.emit('error', errnoException(process._errno, 'recvmsg'));
  }
  rinfo.size = len; // compatibility
  self.emit('message', slab.slice(start, start + len), rinfo);
}


Socket.prototype.ref = function() {
  if (this._handle)
    this._handle.ref();
};


Socket.prototype.unref = function() {
  if (this._handle)
    this._handle.unref();
};

// TODO share with net_uv and others
function errnoException(errorno, syscall) {
  var e = new Error(syscall + ' ' + errorno);
  e.errno = e.code = errorno;
  e.syscall = syscall;
  return e;
}
