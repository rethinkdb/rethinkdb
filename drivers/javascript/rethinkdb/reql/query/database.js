// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.reql.query.Database');

goog.require('rethinkdb.reql.query.Table');

/**
 * @constructor
 */
rethinkdb.reql.query.Database = function(db_name) {

};

rethinkdb.reql.query.Database.prototype.create = function(table_name, primary_key) {
    primary_key = primary_key || 'id';
};

rethinkdb.reql.query.Database.prototype.drop = function(table_name) {
    
};

rethinkdb.reql.query.Database.prototype.list = function() {

};

rethinkdb.reql.query.Database.prototype.table = function(table_name) {
    return new rethinkdb.reql.query.Table(table_name, this);
};
