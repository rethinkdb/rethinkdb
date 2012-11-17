// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.query2');

goog.require('rethinkdbmdl');

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
        return rethinkdb.util.newExpr_(rethinkdb.NumberExpression, value);
    } else if (goog.isBoolean(value)) {
        return rethinkdb.util.newExpr_(rethinkdb.BooleanExpression, value);
    } else if (goog.isString(value)) {
        return rethinkdb.util.newExpr_(rethinkdb.StringExpression, value);
    } else if (goog.isArray(value)) {
        return rethinkdb.util.newExpr_(rethinkdb.ArrayExpression, value);
    } else if (goog.isObject(value)) {
        return rethinkdb.util.newExpr_(rethinkdb.ObjectExpression, value);
    } else {
        return rethinkdb.util.newExpr_(rethinkdb.JSONExpression, value);
    }
};

/**
 * Construct a ReQL JS expression from a JavaScript code string.
 * @param {string} jsExpr A JavaScript code string
 * @returns {rethinkdb.Expression}
 * @export
 */
rethinkdb.js = function(jsExpr) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(jsExpr, 'string');
    return rethinkdb.util.newExpr_(rethinkdb.JSExpression, jsExpr);
};

/**
 * Construct a table reference
 * @param {string} tableName A string giving a table name
 * @param {boolean=} opt_allowOutdated Accept potentially outdated data to reduce latency
 * @returns {rethinkdb.Expression}
 * @export
 */
rethinkdb.table = function(tableName, opt_allowOutdated) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(tableName, 'string');
    rethinkdb.util.typeCheck_(opt_allowOutdated, 'boolean');

    return rethinkdb.util.newExpr_(rethinkdb.Table, tableName, null, opt_allowOutdated);
};

/**
 * Concatenate two or more sequences.
 * @param {...rethinkdb.Expression} var_args
 * @export
 */
rethinkdb.union = function(var_args) {
    rethinkdb.util.argCheck_(arguments, 2);
    var one = rethinkdb.util.wrapIf_(arguments[0]);
    return rethinkdb.Expression.prototype.union.apply(one,
                Array.prototype.slice.call(arguments, 1));
};

/**
 * @class A ReQL query that can be evaluated by a RethinkDB sever.
 * @constructor
 */
rethinkdb.Query = function() { };

/**
 * A shortcut for conn.run(this). If the connection is omitted the last created
 * connection is used.
 * @param {?=} opt_callbackOrOptions If a function then a callback to pass to
 *  the cursor created by running this query. Otherwise an object specifying options such as
 *  callback, connection, and useOutdated.
 * @return {rethinkdb.Cursor}
 */
rethinkdb.Query.prototype.run = function(opt_callbackOrOptions) {

    var callback;
    var conn;
    var useOutdated;
    if (typeof opt_callbackOrOptions === 'function') {
        callback = opt_callbackOrOptions;
    } else if (opt_callbackOrOptions) {
        callback = opt_callbackOrOptions['callback'];
        conn = opt_callbackOrOptions['connection'];
        useOutdated = opt_callbackOrOptions['useOutdated'];
    }

    conn = conn || rethinkdb.last_connection_;
    if (!conn) {
        throw new rethinkdb.errors.ClientError("No connection specified "+
            "and no last connection to default to.");
    }

    return conn.run(this, {
        'callback': callback,
        'useOutdated': useOutdated
    });
};
goog.exportProperty(rethinkdb.Query.prototype, 'run',
                    rethinkdb.Query.prototype.run);

/**
 * A shortcut for printing the results of a ReQL query to the console.
 * @param {rethinkdb.Connection=} opt_conn
 */
rethinkdb.Query.prototype.runp = function(opt_conn) {
    var c;
    if (opt_conn) {
        c = opt_conn.run(this);
    } else {
        c = this.run(/**@type{function()}*/(undefined));
    }
    c.collect(function(res) {
        if (res.length === 1) {
            res = res[0];
        }
        console.log(res);
    });
};
goog.exportProperty(rethinkdb.Query.prototype, 'runp',
                    rethinkdb.Query.prototype.runp);

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
