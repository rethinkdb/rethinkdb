goog.provide('rethinkdb.net');

goog.require('rethinkdb.net.Connection');
goog.require('rethinkdb.net.TcpConnection');
goog.require('rethinkdb.net.HttpConnection');

/**
 * Shorthand for connection constructor.
 * @export	
 */
rethinkdb.net.connect = function(host_or_list, port, db_name) {
    return new rethinkdb.net.TcpConnection(host_or_list, port, db_name);
}

/**
 * Reference to the last created connection.
 * @export	
 */
rethinkdb.net.last_connection = null;
