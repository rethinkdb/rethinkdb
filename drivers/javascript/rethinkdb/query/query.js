goog.provide('rethinkdb.query2');

goog.require('rethinkdb.query');

/**
 * Constructs a primitive ReQL type from a javascript value.
 * @param {*} value The value to wrap.
 * @returns {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.expr = function(value) {
    if (value instanceof rethinkdb.query.Expression) {
        return value;
    } else if (goog.isNumber(value)) {
        return newExpr_(rethinkdb.query.NumberExpression, value);
    } else if (goog.isBoolean(value)) {
        return newExpr_(rethinkdb.query.BooleanExpression, value);
    } else if (goog.isString(value)) {
        return newExpr_(rethinkdb.query.StringExpression, value);
    } else if (goog.isArray(value)) {
        return newExpr_(rethinkdb.query.ArrayExpression, value);
    } else if (goog.isObject(value)) {
        return newExpr_(rethinkdb.query.ObjectExpression, value);
    } else {
        return newExpr_(rethinkdb.query.JSONExpression, value);
    }
};

/**
 * Construct a ReQL JS expression from a JavaScript code string.
 * @param {string} jsExpr A JavaScript code string
 * @returns {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.js = function(jsExpr) {
    argCheck_(arguments, 1);
    typeCheck_(jsExpr, 'string');
    return newExpr_(rethinkdb.query.JSExpression, jsExpr);
};

/**
 * Construct a table reference
 * @param {string} tableIdentifier A string giving either a table
 *      in the current defult dababase or a string formatted as
 *      "db name.table name" giving both the database and table.
 * @returns {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.table = function(tableIdentifier) {
    argCheck_(arguments, 1);
    typeCheck_(tableIdentifier, 'string');
    var db_table_array = tableIdentifier.split('.');

    var db_name = db_table_array[0];
    var table_name = db_table_array[1];
    if (table_name === undefined) {
        table_name = db_name;
        db_name = undefined;
    }

    return newExpr_(rethinkdb.query.Table, table_name, db_name);
};

/**
 * @class A ReQL query that can be evaluated by a RethinkDB sever.
 * @constructor
 */
rethinkdb.query.Query = function() { };

/**
 * A shortcut for conn.run(this). If the connection is omitted the last created
 * connection is used.
 * @param {function(...)} callback The callback to invoke with the result.
 * @param {rethinkdb.net.Connection=} opt_conn The connection to run this expression on.
 */
rethinkdb.query.Query.prototype.run = function(callback, opt_conn) {
    opt_conn = opt_conn || rethinkdb.net.last_connection_;
    opt_conn.run(this, callback);
};
goog.exportProperty(rethinkdb.query.Query.prototype, 'run',
                    rethinkdb.query.Query.prototype.run);

/**
 * A shortcut for conn.iter(this). If the connection is omitted the last created
 * connection is used.
 * @param {function()} callback The callback to invoke with the result.
 * @param {rethinkdb.net.Connection=} conn The connection to run this expression on.
 */
rethinkdb.query.Query.prototype.iter = function(callback, conn) {
    conn = conn || rethinkdb.net.last_connection_;
    conn.iter(this, callback);
};
goog.exportProperty(rethinkdb.query.Query.prototype, 'iter',
                    rethinkdb.query.Query.prototype.iter);

/**
 * Returns a protobuf message tree for this query ast
 * @function
 * @return {!Query}
 * @ignore
 */
rethinkdb.query.Query.prototype.buildQuery = goog.abstractMethod;

/**
 * @class A query representing a ReQL read operation
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.ReadQuery = function() { };
goog.inherits(rethinkdb.query.ReadQuery, rethinkdb.query.Query);

/** @override */
rethinkdb.query.ReadQuery.prototype.buildQuery = function() {
    var readQuery = new ReadQuery();
    var term = this.compile();
    readQuery.setTerm(term);

    var query = new Query();
    query.setType(Query.QueryType.READ);
    query.setReadQuery(readQuery);

    return query;
};
