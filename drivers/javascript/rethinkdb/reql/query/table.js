// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.reql.query.Table');

goog.require('rethinkdb.net');

/**
 * @constructor
 */
rethinkdb.reql.query.Table = function(table_name, db_expr) {
    db_expr = db_expr || rethinkdb.net.last_connection.defalut_db;
};

/** @export */
rethinkdb.reql.query.Table.prototype.get = function(key) {

};
