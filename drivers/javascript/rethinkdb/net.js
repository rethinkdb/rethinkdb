// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net');

goog.require('rethinkdb.net.Connection');

/**
 * Shorthand for connection constructor.
 */
rethinkdb.net.connect = function(host_or_list, port, db_name) {
    return new rethinkdb.net.Connection(host_or_list, port, db_name);
}

/**
 * Reference to the last created connection.
 */
rethinkdb.net.last_connection = null;
