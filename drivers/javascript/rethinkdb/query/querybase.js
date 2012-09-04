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
