// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.net');

goog.require('rethinkdb.Connection');
goog.require('rethinkdb.TcpConnection');
goog.require('rethinkdb.HttpConnection');

/**
 * This shortcut function invokes either the {@link rethinkdb.TcpConnection}
 * constructor or the {@link rethinkdb.HttpConnection} constructor. Prefers
 * TcpConnection if available.
 * @param {?string|Object} host Either a string giving the host to connect to or
 *      an object specifying host and/or port and/or db for the default database to
 *      use on this connection. Any key not supplied will revert to the default.
 * @param {function(...)=} onConnect
 * @param {function(...)=} onFailure
 * @see rethinkdb.HttpConnection
 * @see rethinkdb.TcpConnection
 * @export
 */
rethinkdb.connect = function(host, onConnect, onFailure) {
    if (rethinkdb.TcpConnection.isAvailable()) {
        return new rethinkdb.TcpConnection(host, onConnect, onFailure);
    } else {
        return new rethinkdb.HttpConnection(host, onConnect, onFailure);
    }
}

/**
 * Reference to the last created connection.
 * @type {?rethinkdb.Connection}
 * @ignore
 */
rethinkdb.last_connection_ = null;

/**
 * Get the last created connection.
 * @return {rethinkdb.Connection}
 * @export
 */
rethinkdb.lastConnection = function() {
    return rethinkdb.last_connection_;
};
