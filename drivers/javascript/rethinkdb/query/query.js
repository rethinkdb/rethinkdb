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
    } else if (value === null || goog.isNumber(value)) {
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
 * @param {string} tableName A string giving a table name
 * @param {boolean=} opt_allowOutdated Accept potentially outdated data to reduce latency
 * @returns {rethinkdb.Expression}
 * @export
 */
rethinkdb.table = function(tableName, opt_allowOutdated) {
    argCheck_(arguments, 1);
    typeCheck_(tableName, 'string');
    typeCheck_(opt_allowOutdated, 'boolean');

    return newExpr_(rethinkdb.Table, tableName, null, opt_allowOutdated);
};

/**
 * @class A ReQL query that can be evaluated by a RethinkDB sever.
 * @constructor
 */
rethinkdb.Query = function() { };

/**
 * A shortcut for conn.run(this). If the connection is omitted the last created
 * connection is used.
 * @param {function(...)|Object} callbackOrOpts The callback to invoke with the result.
 * @param {rethinkdb.Connection=} opt_conn The connection to run this expression on.
 */
rethinkdb.Query.prototype.run = function(callbackOrOpts, opt_conn) {

    var conn;
    var opts = {};
    if (!callbackOrOpts || typeof callbackOrOpts === 'function') {
        conn = opt_conn || rethinkdb.last_connection_;
        opts['callback'] = callbackOrOpts;
    } else {
        conn = callbackOrOpts['conn'] || rethinkdb.last_connection_;
        goog.object.extend(opts, callbackOrOpts);
        delete opts['conn'];
    }

    opts['expr'] = this;

    if (!conn) {
        throw new rethinkdb.errors.ClientError("No connection specified "+
            "and no last connection to default to.");
    }

    conn.run(opts);
};
goog.exportProperty(rethinkdb.Query.prototype, 'run',
                    rethinkdb.Query.prototype.run);

/**
 * A shortcut for conn.runp(this). If the connection is omitted the last created
 * connection is used.
 * @param {rethinkdb.Connection|Object=} connOrOpts The connection to run this expression on.
 */
rethinkdb.Query.prototype.runp = function(connOrOpts) {
    var conn;
    var opts = {};
    if (!connOrOpts || connOrOpts instanceof rethinkdb.Connection) {
        conn = connOrOpts || rethinkdb.last_connection_;
    } else {
        conn = connOrOpts['conn'] || rethinkdb.last_connection_;
        goog.object.extend(opts, connOrOpts);
        delete opts['conn'];
    }

    opts['expr'] = this;

    if (!conn) {
        throw new rethinkdb.errors.ClientError("No connection specified "+
            "and no last connection to default to.");
    }

    conn.runp(opts);
};
goog.exportProperty(rethinkdb.Query.prototype, 'runp',
                    rethinkdb.Query.prototype.runp);

/**
 * A shortcut for conn.iter(this). If the connection is omitted the last created
 * connection is used.
 * @param {function(...)|Object} callbackOrOpts The callback to invoke with the result.
 * @param {rethinkdb.Connection=} opt_conn The connection to run this expression on.
 */
rethinkdb.Query.prototype.iter = function(callbackOrOpts, opt_conn) {
    var conn;
    var opts = {};
    if (!callbackOrOpts || typeof callbackOrOpts === 'function') {
        conn = opt_conn || rethinkdb.last_connection_;
        opts['callback'] = callbackOrOpts;
    } else {
        conn = callbackOrOpts['conn'] || rethinkdb.last_connection_;
        goog.object.extend(opts, callbackOrOpts);
        delete opts['conn'];
    }

    opts['expr'] = this;

    if (!conn) {
        throw new rethinkdb.errors.ClientError("No connection specified "+
            "and no last connection to default to.");
    }

    conn.iter(opts);
};
goog.exportProperty(rethinkdb.Query.prototype, 'iter',
                    rethinkdb.Query.prototype.iter);

/**
 * Returns a protobuf message tree for this query ast
 * @function
 * @param {Object=} opt_buildOpts
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
rethinkdb.ReadQuery.prototype.buildQuery = function(opt_buildOpts) {
    var readQuery = new ReadQuery();
    var term = this.compile(opt_buildOpts);
    readQuery.setTerm(term);

    var query = new Query();
    query.setType(Query.QueryType.READ);
    query.setReadQuery(readQuery);

    return query;
};
