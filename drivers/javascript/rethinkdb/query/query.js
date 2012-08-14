goog.provide('rethinkdb.query');

goog.require('goog.asserts');
goog.require('rethinkdb.query.Database');

/**
 * @return {rethinkdb.query.Database}
 */
rethinkdb.query.db = function(db_name) {
     return new rethinkdb.query.Database(db_name);
};

rethinkdb.query.db_create = function(db_name, primary_datacenter) {
    //TODO how to get cluster level default?
    primary_datacenter = primary_datacenter || 'cluster-level-default?';
};

rethinkdb.query.db_drop = function(db_name) {
    
};

rethinkdb.query.db_list = function() {
    
};

/** @export */
rethinkdb.query.expr = function(value) {
    return new rethinkdb.query.JSONExpression(value);
};

/**
 * Constructs an expression with the given variables bound.
 */
rethinkdb.query.fn = function(/** variable */) {
    for (var i = 0; i < arguments.length - 1; ++i) {
        goog.asserts.assertString(arguments[i]); 
    }
};

rethinkdb.query.table = function(table_identifier) {
    var db_table_array = table_identifier.split('.');

    var db_name = db_table_array[0];
    var table_name = db_table_array[1];
    var db;
    if (table_name === undefined) {
        table_name = db_name;
        db = undefined;
    } else {
        db = new rethinkdb.query.Database(db_name);
    }

    return new rethinkdb.query.Table(table_name, db);
};

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.R = function(varExpression) {
    /* varExpression is a string referencing some variable
     * or attribute availale in the current reql scope.
     */
};
