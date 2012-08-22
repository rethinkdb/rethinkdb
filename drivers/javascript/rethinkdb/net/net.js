goog.provide('rethinkdb.net');

goog.require('rethinkdb.net.Connection');
goog.require('rethinkdb.net.TcpConnection');
goog.require('rethinkdb.net.HttpConnection');

/**
 * Shorthand for connection constructor.
 * @export
 */
rethinkdb.net.connect = function(host_or_list, onConnect, onFailure) {
    if (typeof require !== 'undefined' && require('net')) {
        return new rethinkdb.net.TcpConnection(host_or_list, onConnect, onFailure);
    } else {
        return new rethinkdb.net.HttpConnection(host_or_list, onConnect, onFailure);
    }
}

/**
 * Reference to the last created connection.
 * @type {?rethinkdb.net.Connection}
 * @export
 */
rethinkdb.net.last_connection = null;
