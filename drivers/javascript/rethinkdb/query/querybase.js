goog.provide('rethinkdb.query');

/**
 * This is defined here so that it is defined first, before
 * any thing defined on the rethinkdb.query namespace is.
 * @export
 */
rethinkdb.query = function(jsobj) {
    return rethinkdb.query.expr(jsobj);
};
