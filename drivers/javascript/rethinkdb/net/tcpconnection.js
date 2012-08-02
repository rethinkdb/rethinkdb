// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net.TcpConnection');

goog.require('rethinkdb.net.Connection');

/**
 * TcpConnection, a connection which uses a TCP connection to communicate
 * with the rethinkdb server.
 * @constructor
 * @export
 */
rethinkdb.net.TcpConnection = function(host_or_list, onConnect, onFailure) {
	//TODO verify
	var DEFAULT_PORT = 11211;
	var DEFAULT_HOST = 'localhost';

	// Will be set when connection established
	this.socket_ = null;

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
	var self = this;

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
 * @param {rethinkdb.query.Expression} expr The expression to evaluate.
 * @param {Function} callback To be called back with the result.
 * @private
 */
rethinkdb.net.TcpConnection.prototype.send_ = function(expr, callback) {

};

/**
 * Private abstract method for closing underlying network connection.
 */
rethinkdb.net.TcpConnection.prototype.close = function() {

};
