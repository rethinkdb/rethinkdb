goog.provide('rethinkdb.query.Table');

goog.require('rethinkdb.net');

/**
 * @constructor
 */
rethinkdb.query.Table = function(table_name, db_expr) {
    db_expr = db_expr || rethinkdb.net.last_connection.getDefaultDb();
};

/** @export */
rethinkdb.query.Table.prototype.get = function(key) {

};
