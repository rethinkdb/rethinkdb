goog.provide('rethinkdb.query.Table');

goog.require('rethinkdb.net');
goog.require('rethinkdb.query.Expression');
goog.require('Term');

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.Table = function(tableName, opt_dbName) {
    this.db_ = opt_dbName || rethinkdb.net.last_connection.getDefaultDb();
    this.name_ = tableName;
};

rethinkdb.query.Table.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.TABLE);

    var table = new Term.Table();
    var tableRef = new TableRef();
    tableRef.setDbName(this.db_);
    tableRef.setTableName(this.name_);
    table.setTableRef(tableRef);

    term.setTable(table);
    return term;
};

rethinkdb.query.Table.prototype.get = function(key) {

};
