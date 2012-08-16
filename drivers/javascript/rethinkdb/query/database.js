goog.provide('rethinkdb.query.Database');

/**
 * @constructor
 */
rethinkdb.query.Database = function(db_name) {
    this.name_ = db_name;
};

rethinkdb.query.Database.prototype.create = function(table_name, primary_key) {
    primary_key = primary_key || 'id';
};

rethinkdb.query.Database.prototype.drop = function(table_name) {
    
};

rethinkdb.query.Database.prototype.list = function() {

};

rethinkdb.query.Database.prototype.table = function(table_name) {
    return new rethinkdb.query.Table(table_name, this.name_);
};
goog.exportProperty(rethinkdb.query.Database.prototype, 'table',
                    rethinkdb.query.Database.prototype.table);
