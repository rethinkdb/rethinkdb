goog.provide('rethinkdb.net.TcpConnection');

goog.require('rethinkdb.net.Connection');

/**
 * TcpConnection, a connection which uses a TCP connection to communicate
 * with the rethinkdb server.
 * @constructor
 * @extends {rethinkdb.net.Connection}
 * @export
 */
rethinkdb.net.TcpConnection = function(host_or_list, onConnect, onFailure) {
	var /** @const */ DEFAULT_PORT = 11211;
	var /** @const */ DEFAULT_HOST = 'localhost';

    goog.base(this, null);

    this.recvBuffer_ = null;
    this.revdD_ = 0;

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
                socket_node_.on('data', goog.bind(self.tcpRecv_, self));
				self.socket_ = socket_node_;
				if (onConnect) onConnect(self);
			});

			socket_node_.on('error', tryNext);
		} else {
			if (onFailure) onFailure();
		}
	})();
};
goog.inherits(rethinkdb.net.TcpConnection, rethinkdb.net.Connection);

/**
 * Private method for actually sending request to the server
 * over the tcp connection.
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.net.TcpConnection.prototype.send_ = function(data) {
    if (this.socket_) {

        // This is required because the node socket has to take a native node
        // buffer, not an ArrayBuffer. Even though ArrayBuffers are supported
        // by node, there is no simple way to convert between them.
        var bufferData = new Buffer(data.byteLength);
        var byteArray = new Uint8Array(data);
        for (var i = 0; i < data.byteLength; i++) {
            bufferData[i] = byteArray[i];
        }

        this.socket_.write(bufferData);
    } else {
        throw "Connection not open";
    }
};

/**
 * @private
 */
rethinkdb.net.TcpConnection.prototype.tcpRecv_ = function(node_data) {
    if (this.recvBuffer_ === null) {
        // This is the head of a new response

        goog.asserts.assert(node_data.length >= 4);
        var responseSize = node_data.readUInt32LE(0);
        this.recvBuffer_ = new Uint8Array(responseSize + 4);
        this.recvD_ = 0;

    } //else We're still buffering from an earlier response

    var byteArray = new Uint8Array(node_data);

    this.recvBuffer_.set(byteArray, this.recvD_);
    this.recvD_ += byteArray.length;

    if (this.recvD_ >= this.recvBuffer_.length) {
        // We've finished this response, send it up

        var buff = this.recvBuffer_;
        this.recvBuffer_ = null;
        this.recvD_ = 0;
        this.recv_(buff);

    } //else We still need to wait for more data
};

/**
 * @override
 */
rethinkdb.net.TcpConnection.prototype.close = function() {
    if (this.socket_) {
        this.socket_.end();
        this.socket_ = null;
    } else {
        throw "Connection not open";
    }
};
goog.exportProperty(rethinkdb.net.TcpConnection.prototype, 'close',
                    rethinkdb.net.TcpConnection.prototype.close);
