goog.provide('rethinkdb.query2');

goog.require('rethinkdb');

/**
 * Constructs a primitive ReQL type from a javascript value.
 * @param {*} value The value to wrap.
 * @returns {rethinkdb.Expression}
 * @export
 */
rethinkdb.expr = function(value) {
    if (value instanceof rethinkdb.Expression) {
        return value;
    } else if (goog.isNumber(value)) {
        return newExpr_(rethinkdb.NumberExpression, value);
    } else if (goog.isBoolean(value)) {
        return newExpr_(rethinkdb.BooleanExpression, value);
    } else if (goog.isString(value)) {
        return newExpr_(rethinkdb.StringExpression, value);
    } else if (goog.isArray(value)) {
        return newExpr_(rethinkdb.ArrayExpression, value);
    } else if (goog.isObject(value)) {
        return newExpr_(rethinkdb.ObjectExpression, value);
    } else {
        return newExpr_(rethinkdb.JSONExpression, value);
    }
};

/**
 * Construct a ReQL JS expression from a JavaScript code string.
 * @param {string} jsExpr A JavaScript code string
 * @returns {rethinkdb.Expression}
 * @export
 */
rethinkdb.js = function(jsExpr) {
    argCheck_(arguments, 1);
    typeCheck_(jsExpr, 'string');
    return newExpr_(rethinkdb.JSExpression, jsExpr);
};

/**
 * Construct a table reference
 * @param {string} table_name A string giving a table name.
 * @returns {rethinkdb.Expression}
 * @export
 */
rethinkdb.table = function(table_name) {
    argCheck_(arguments, 1);
    typeCheck_(table_name, 'string');

    return newExpr_(rethinkdb.Table, table_name);
};

/**
 * @class A ReQL query that can be evaluated by a RethinkDB sever.
 * @constructor
 */
rethinkdb.Query = function() { };

/**
 * A shortcut for conn.run(this). If the connection is omitted the last created
 * connection is used.
 * @param {function(...)} callback The callback to invoke with the result.
 * @param {rethinkdb.Connection=} opt_conn The connection to run this expression on.
 */
rethinkdb.Query.prototype.run = function(callback, opt_conn) {
    opt_conn = opt_conn || rethinkdb.last_connection_;
    opt_conn.run(this, callback);
};
goog.exportProperty(rethinkdb.Query.prototype, 'run',
                    rethinkdb.Query.prototype.run);

/**
 * A shortcut for conn.iter(this). If the connection is omitted the last created
 * connection is used.
 * @param {function(...)} callback The callback to invoke with the result.
 * @param {function(...)=} doneCallback The callback to invoke when the stream has ended.
 */
rethinkdb.Query.prototype.iter = function(callback, doneCallback) {
    var conn = rethinkdb.last_connection_;
    conn.iter(this, callback, doneCallback);
};
goog.exportProperty(rethinkdb.Query.prototype, 'iter',
                    rethinkdb.Query.prototype.iter);

/**
 * Returns a protobuf message tree for this query ast
 * @function
 * @return {!Query}
 * @ignore
 */
rethinkdb.Query.prototype.buildQuery = goog.abstractMethod;

/**
 * Returns a formatted string representing the text that would
 * have generated this ReQL ast. Used for reporting server errors.
 * @param {Array=} bt
 * @return {string}
 * @ignore
 */
rethinkdb.Query.prototype.formatQuery = function(bt) {
    return "----";
};

/**
 * @class A query representing a ReQL read operation
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.ReadQuery = function() { };
goog.inherits(rethinkdb.ReadQuery, rethinkdb.Query);

/** @override */
rethinkdb.ReadQuery.prototype.buildQuery = function() {
    var readQuery = new ReadQuery();
    var term = this.compile();
    readQuery.setTerm(term);

    var query = new Query();
    query.setType(Query.QueryType.READ);
    query.setReadQuery(readQuery);

    return query;
};
