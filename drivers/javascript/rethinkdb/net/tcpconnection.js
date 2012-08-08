// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net.TcpConnection');

goog.require('rethinkdb.net.Connection');

/**
 * TcpConnection, a connection which uses a TCP connection to communicate
 * with the rethinkdb server.
 * @constructor
 * @extends rethinkdb.net.Connection
 * @export
 */
rethinkdb.net.TcpConnection = function(host_or_list, onConnect, onFailure) {
	var /** @const */ DEFAULT_PORT = 11211;
	var /** @const */ DEFAULT_HOST = 'localhost';

    goog.base(this);
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
                socket_node_.on('data', self.recv_);
				self.socket_ = socket_node_;
				rethinkdb.net.last_connection = self;
				onConnect(self);
			});

			socket_node_.on('error', tryNext);
		} else {
			onFailure();
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
        this.socket_.write(data);
    } else {
        throw "Connection not open";
    }
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
