goog.provide('rethinkdb.query2');

goog.require('rethinkdb.query');
goog.require('goog.asserts');

/** @export */
rethinkdb.query.expr = function(value) {
    if (typeof value === 'number') {
        return new rethinkdb.query.NumberExpression(value);
    } else {
        return new rethinkdb.query.JSONExpression(value);
    }
};

/** @export */
rethinkdb.query.table = function(table_identifier) {
    var db_table_array = table_identifier.split('.');

    var db_name = db_table_array[0];
    var table_name = db_table_array[1];
    if (table_name === undefined) {
        table_name = db_name;
        db_name = undefined;
    }

    return new rethinkdb.query.Table(table_name, db_name);
};

/**
 * @constructor
 */
rethinkdb.query.BaseQuery = function() { };

/**
 * @param {function()} callback The callback to invoke with the result.
 * @param {rethinkdb.net.Connection=} conn The connection to run this expression on.
 */
rethinkdb.query.BaseQuery.prototype.run = function(callback, conn) {
    conn = conn || rethinkdb.net.last_connection;
    conn.run(this, callback);
};
goog.exportProperty(rethinkdb.query.BaseQuery.prototype, 'run',
    rethinkdb.query.BaseQuery.prototype.run);

/** @return {!Query} */
rethinkdb.query.BaseQuery.prototype.buildQuery = goog.abstractMethod;

/**
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.ReadQuery = function() { };
goog.inherits(rethinkdb.query.ReadQuery, rethinkdb.query.BaseQuery);

/**
 * @override
 * @return {!Query}
 */
rethinkdb.query.ReadQuery.prototype.buildQuery = function() {
    var readQuery = new ReadQuery();
    var term = this.compile();
    readQuery.setTerm(term);

    var query = new Query();
    query.setType(Query.QueryType.READ);
    query.setReadQuery(readQuery);

    return query;
};
