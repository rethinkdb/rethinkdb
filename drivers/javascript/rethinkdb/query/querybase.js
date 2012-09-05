goog.provide('rethinkdb.query');

/**
 * This is defined here so that it is defined first, before
 * any thing defined on the rethinkdb.query namespace is.
 */

/**
 * A shortcut function for wrapping values with RethinkDB expressions.
 * @export
 */
rethinkdb.query = function(jsobj) {
    if (typeof jsobj === 'string' && (jsobj[0] === '$' || jsobj[0] === '@')) {
        return rethinkdb.query.R(jsobj);
    } else {
        return rethinkdb.query.expr(jsobj);
    }
};

/**
 * @constructor
 * @extends {Error}
 */
function ClientError(msg) {
    this.name = "RDB Client Error";
    this.message = msg || "The RDB client has experienced an error";
}
goog.inherits(ClientError, Error);

/**
 * Internal utility for wrapping API function arguments
 */
function wrapIf_(val) {
    if (val instanceof rethinkdb.query.Expression) {
        return val;
    } else {
        return rethinkdb.query(val);
    }
}

/**
 * Internal utility for wrapping API function arguments that
 * are expected to be function expressions.
 */
function functionWrap_(fun) {
    if (fun instanceof rethinkdb.query.FunctionExpression) {
        // No wrap needed
    } else if (fun instanceof rethinkdb.query.Expression) {
        fun = rethinkdb.query.fn('', fun);
    } else if(typeof fun === 'function') {
        fun = rethinkdb.query.fn(fun);
    } else {
       throw TypeError("Argument expected to be a function expression");
    }

    return fun;
}
