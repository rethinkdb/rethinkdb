/**
 * @name rethinkdb.net
 * @namespace Namespace for networking releated classes and functions
 */
goog.provide('rethinkdb.net');

goog.require('rethinkdb.net.Connection');
goog.require('rethinkdb.net.TcpConnection');
goog.require('rethinkdb.net.HttpConnection');

/**
 * This shortcut function invokes either the {@link rethinkdb.net.TcpConnection}
 * constructor or the {@link rethinkdb.net.HttpConnection} constructor. Prefers
 * TcpConnection if available.
 * @param {?string|Array.<string>|Object|Array.<Object>} host_or_list A host/port
 *      pair or list of host port pairs giving the servers to attempt to connect
 *      to. Either hostname or port may be omitted to utilize the default.
 * @param {function(...)=} onConnect
 * @param {function(...)=} onFailure
 * @see rethinkdb.net.HttpConnection
 * @see rethinkdb.net.TcpConnection
 * @export
 */
rethinkdb.net.connect = function(host_or_list, onConnect, onFailure) {
    if (rethinkdb.net.TcpConnection.isAvailable()) {
        return new rethinkdb.net.TcpConnection(host_or_list, onConnect, onFailure);
    } else {
        return new rethinkdb.net.HttpConnection(host_or_list, onConnect, onFailure);
    }
}

/**
 * Reference to the last created connection.
 * @type {?rethinkdb.net.Connection}
 * @ignore
 */
rethinkdb.net.last_connection_ = null;

/**
 * Get the last created connection.
 * @return {rethinkdb.net.Connection}
 * @export
 */
rethinkdb.net.getLastConnection = function() {
    return rethinkdb.net.last_connection_;
};
