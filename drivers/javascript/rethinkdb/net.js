// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.net');

goog.require('rethinkdb.Connection');
goog.require('rethinkdb.TcpConnection');
goog.require('rethinkdb.HttpConnection');
goog.require('rethinkdb.DemoConnection');

/**
 * This shortcut function invokes either the {@link rethinkdb.TcpConnection}
 * constructor or the {@link rethinkdb.HttpConnection} constructor. Prefers
 * TcpConnection if available.
 * @param {?string|Array.<string>|Object|Array.<Object>} host_or_list A host/port
 *      pair or list of host port pairs giving the servers to attempt to connect
 *      to. Either hostname or port may be omitted to utilize the default.
 * @param {function(...)=} onConnect
 * @param {function(...)=} onFailure
 * @see rethinkdb.HttpConnection
 * @see rethinkdb.TcpConnection
 * @export
 */
rethinkdb.connect = function(host_or_list, onConnect, onFailure) {
    if (rethinkdb.TcpConnection.isAvailable()) {
        return new rethinkdb.TcpConnection(host_or_list, onConnect, onFailure);
    } else {
        return new rethinkdb.HttpConnection(host_or_list, onConnect, onFailure);
    }
};

/** @export */
rethinkdb.demoConnect = function(demoSever) {
    return new rethinkdb.DemoConnection(demoSever);
};

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
