goog.provide('rethinkdb.reql.query');

goog.require('goog.asserts');
goog.require('rethinkdb.reql.query.Database');

/**
 * @return {rethinkdb.reql.query.Database}
 */
rethinkdb.reql.query.db = function(db_name) {
     return new rethinkdb.reql.query.Database(db_name);
};

rethinkdb.reql.query.db_create = function(db_name, primary_datacenter) {
    //TODO how to get cluster level default?
    primary_datacenter = primary_datacenter || 'cluster-level-default?';
};

rethinkdb.reql.query.db_drop = function(db_name) {
    
};

rethinkdb.reql.query.db_list = function() {
    
};

/** @export */
rethinkdb.reql.query.expr = function(value) {
    return new rethinkdb.reql.query.JSONExpression(value);
};

/**
 * Constructs an expression with the given variables bound.
 */
rethinkdb.reql.query.fn = function(/** variable */) {
    for (var i = 0; i < arguments.length - 1; ++i) {
        goog.asserts.assertString(arguments[i]); 
    }
};

rethinkdb.reql.query.table = function(table_identifier) {
    var db_table_array = table_identifier.split('.');

    var db_name = db_table_array[0];
    var table_name = db_table_array[1];
    var db;
    if (table_name === undefined) {
        table_name = db_name;
        db = undefined;
    } else {
        db = new rethinkdb.reql.query.Database(db_name);
    }

    return new rethinkdb.reql.query.Table(table_name, db);
};

/**
 * @return {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.R = function(varExpression) {
    /* varExpression is a string referencing some variable
     * or attribute availale in the current reql scope.
     */
};
