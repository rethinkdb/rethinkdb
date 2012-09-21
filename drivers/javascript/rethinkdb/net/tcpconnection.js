goog.provide('rethinkdb.TcpConnection');

goog.require('rethinkdb.Connection');

/**
 * Host or list behavior is being abandoned for now. Could be brought back
 * in the future.
 *
 * Constructs a TCP based connection in one of several ways.
 * If no host is supplied the server will attempt to connect to the default host
 * over the default port. If a single host is given the constructor will attempt
 * to connect to that host over the default port. An object specifying both host
 * and port may also be supplied. Finally, if a list of host strings or host/port
 * objects are supplied the constructor will attempt to connect to given hosts in
 * the order given. Once a connection to a host has been established the optional
 * onConnect handler is called. If no connection can be made, the optional
 * onFailure handler is called.
 * @class A TCP based connection to the RethinkDB server.
 * Use this connection type when running this client in a server side execution
 * environment like node.js. It is to be preferred to the HTTP connection when
 * available.
 * @param {?string|Array.<string>|Object|Array.<Object>} host_or_list A host/port
 *      pair or list of host port pairs giving the servers to attempt to connect
 *      to. Either hostname or port may be omitted to utilize the default.
 * @param {function(...)=} onConnect Function to call when connection is established
 * @param {function(...)=} onFailure Error handler to use for this connection. Will be
 *  called if a connection cannot be established. Can also be set with setErrorHandler.
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 * @ignore
 */
/*
rethinkdb.TcpConnection = function(host_or_list, onConnect, onFailure) {
    typeCheck_(onConnect, 'function');
    typeCheck_(onFailure, 'function');

	var DEFAULT_PORT = 12346;
	var DEFAULT_HOST = 'localhost';

    if (!rethinkdb.TcpConnection.isAvailable()) {
        this.error_(new rethinkdb.errors.ClientError("Cannot instantiate TcpConnection, "+
            "TCP sockets are not available in this environment."));
    }

    goog.base(this, null, onFailure);

    this.recvBuffer_ = null;
    this.revdD_ = 0;
    this.socket_ = null;

	var self = this;

	// Will be set when connection established
	self.socket_ = null;

	var hosts;
	if (host_or_list) {
		if (goog.isArray(host_or_list)) {
			hosts = host_or_list;
		} else {
			hosts = [host_or_list];
		}
	} else {
 		hosts = [DEFAULT_HOST]
	}

	var net_node_ = require('net');

	(function tryNext() {
		var hostObj = hosts.shift();
		if (hostObj) {

			var host;
			var port;
			if (typeof hostObj === 'string') {
				host = hostObj;
				port = DEFAULT_PORT;
			} else {
				host = hostObj['host'] || DEFAULT_HOST;
				port = hostObj['port'] || DEFAULT_PORT;
			}

			var socket_node_ = net_node_.connect(port, host, function() {
                socket_node_.write("35ba61af", "hex");
                socket_node_.on('data', goog.bind(self.tcpRecv_, self));
				self.socket_ = socket_node_;
				if (onConnect) onConnect(self);
			});

			socket_node_.on('error', tryNext);
		} else {
            self.error_(new rethinkdb.errors.ClientError('Unabled to establish TCP connection'));
		}
	})();
};
goog.inherits(rethinkdb.TcpConnection, rethinkdb.Connection);
*/

/**
 * Constructs a TCP based connection in one of several ways.
 * @class A TCP based connection to the RethinkDB server.
 * Use this connection type when running this client in a server side execution
 * environment like node.js. It is to be preferred to the HTTP connection when
 * available.
 * @param {?string|Object} host Either a string giving the host to connect to or
 *      an object specifying host and/or port and/or db for the default database to
 *      use on this connection. Any key not supplied will revert to the default.
 * @param {function(...)=} onConnect Function to call when connection is established
 * @param {function(...)=} onFailure Error handler to use for this connection. Will be
 *  called if a connection cannot be established. Can also be set with setErrorHandler.
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 */
rethinkdb.TcpConnection = function(host, onConnect, onFailure) {
    if (!rethinkdb.TcpConnection.isAvailable()) {
        this.error_(new rethinkdb.errors.ClientError("Cannot instantiate TcpConnection, "+
            "TCP sockets are not available in this environment."));
    }

    typeCheck_(onConnect, 'function');
    typeCheck_(onFailure, 'function');

    goog.base(this, host, onFailure);

    this.recvBuffer_ = null;
    this.revdD_ = 0;
    this.socket_ = null;

	var self = this;

	// Will be set when connection established
	self.socket_ = null;

	var net_node_ = require('net');

    var socket_node_ = net_node_.connect(this.port_, this.host_, function() {
        socket_node_.write("35ba61af", "hex");
        socket_node_.on('data', goog.bind(self.tcpRecv_, self));
        socket_node_.on('end', function() {
            self.socket_ = null;
            self.error_(new rethinkdb.errors.RuntimeError('Server disconnected'));
        });
        self.socket_ = socket_node_;
        if (onConnect) onConnect(self);
    });

	socket_node_.on('error', function() {
        self.error_(new rethinkdb.errors.ClientError('Unabled to establish TCP connection'));
    });
};
goog.inherits(rethinkdb.TcpConnection, rethinkdb.Connection);

/**
 * Returns true if TcpConnection is available in the current environment.
 * @returns {boolean}
 * @static
 * @export
 */
rethinkdb.TcpConnection.isAvailable = function() {
    return (typeof require !== 'undefined' && require('net'));
};

/**
 * Send data over the underlying TCP connection.
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.TcpConnection.prototype.send_ = function(data) {
    if (!this.socket_)
        this.error_(new rethinkdb.errors.ClientError("Connection not open"));

    // This is required because the node socket has to take a native node
    // buffer, not an ArrayBuffer. Even though ArrayBuffers are supported
    // by node, there is no simple way to convert between them.
    var bufferData = new Buffer(data.byteLength);
    var byteArray = new Uint8Array(data);
    for (var i = 0; i < data.byteLength; i++) {
        bufferData[i] = byteArray[i];
    }

    this.socket_.write(bufferData);
};

/**
 * Handler for data events generated by the underlying socket.
 * Buffers data until a full response message is received.
 * @private
 */
rethinkdb.TcpConnection.prototype.tcpRecv_ = function(node_data) {
    var rcvArray = new Uint8Array(node_data);
    var read = 0;
    var available = rcvArray.byteLength;

    while (available > 0) {
        if (this.recvBuffer_ === null) {
            // This is the head of a new response

            goog.asserts.assert(available >= 4);
            var view = new DataView(rcvArray.buffer, read, available);
            var responseSize = view.getUint32(0, true);
            this.recvBuffer_ = new Uint8Array(responseSize + 4);
            this.recvD_ = 0;
        }

        var left = this.recvBuffer_.byteLength - this.recvD_;
        var toRead = available < left ? available : left;

        var readView = new Uint8Array(rcvArray.buffer, read, toRead);
        this.recvBuffer_.set(readView, this.recvD_);

        this.recvD_ += toRead;
        read += toRead;
        available -= toRead;
        left -= toRead;

        if (left <= 0) {
            // We've finished this response, send it up

            var buff = this.recvBuffer_;
            this.recvBuffer_ = null;
            this.recvD_ = 0;
            this.recv_(buff);
        }
    }
};

/**
 * Close the underlying TCP connection.
 * @override
 */
rethinkdb.TcpConnection.prototype.close = function() {
    if (!this.socket_)
        this.error_(new rethinkdb.errors.ClientError("Connection not open"));

    var self = this;

    // Clear end event so we don't throw an exception when we close the connecton;
    this.socket_.removeAllListeners('end');
    this.socket_.on('close', function() {
        self.socket_ = null;
    });

    this.socket_.end();
};
goog.exportProperty(rethinkdb.TcpConnection.prototype, 'close',
                    rethinkdb.TcpConnection.prototype.close);
