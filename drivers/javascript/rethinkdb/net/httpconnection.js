// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net.HttpConnection');

goog.require('rethinkdb.net.Connection');

rethinkdb.net.HttpConnection = function(path, db_name) {
	//TODO
};
goog.inherits(rethinkdb.net.HttpConnection, rethinkdb.net.Connection);

/**
 * Private method for actually sending request to the server
 * over the http connection.
 * @param {rethinkdb.query.Expression} expr The expression to evaluate.
 * @param {Function} callback To be called back with the result.
 * @private
 */
rethinkdb.net.TcpConnection.prototype.send_ = function(expr, callback) {
	//TODO
};
