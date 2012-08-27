goog.provide('rethinkdb.query.Table');


/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.Table = function(tableName, opt_dbName) {
    this.db_ = opt_dbName || null;
    this.name_ = tableName;
};
goog.inherits(rethinkdb.query.Table, rethinkdb.query.Expression);

rethinkdb.query.Table.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.TABLE);

    var table = new Term.Table();
    var tableRef = new TableRef();
    tableRef.setDbName(this.db_ || rethinkdb.net.last_connection.getDefaultDb());
    tableRef.setTableName(this.name_);
    table.setTableRef(tableRef);

    term.setTable(table);
    return term;
};

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.GetExpression = function(table, key) {
    this.table_ = table;
    this.key_ = new rethinkdb.query.JSONExpression(key);
};
goog.inherits(rethinkdb.query.GetExpression, rethinkdb.query.Expression);

rethinkdb.query.GetExpression.prototype.compile = function() {
    var tableTerm = this.table_.compile();
    var tableRef = tableTerm.getTable().getTableRefOrDefault();

    var get = new Term.GetByKey();
    get.setTableRef(tableRef);
    get.setAttrname('id');
    get.setKey(this.key_.compile());

    var term = new Term();
    term.setType(Term.TermType.GETBYKEY);
    term.setGetByKey(get);

    return term;
};

rethinkdb.query.Table.prototype.get = function(key) {
    return new rethinkdb.query.GetExpression(this, key);
};
goog.exportProperty(rethinkdb.query.Table.prototype, 'get',
                    rethinkdb.query.Table.prototype.get);

/**
 * @param {rethinkdb.query.Table} table
 * @param {*} docs
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.InsertQuery = function(table, docs) {
    this.table_ = table;

    if (!goog.isArray(docs))
        docs = [docs];

    this.docs_ = docs;
};
goog.inherits(rethinkdb.query.InsertQuery, rethinkdb.query.BaseQuery);

/** @override */
rethinkdb.query.InsertQuery.prototype.buildQuery = function() {
    var tableTerm = this.table_.compile();
    var tableRef = tableTerm.getTable().getTableRefOrDefault();

    var insert = new WriteQuery.Insert();
    insert.setTableRef(tableRef);

    for (var i = 0; i < this.docs_.length; i++) {
        var jsonExpr = new rethinkdb.query.JSONExpression(this.docs_[i]);
        insert.addTerms(jsonExpr.compile());
    }

    var writeQuery = new WriteQuery();
    writeQuery.setType(WriteQuery.WriteQueryType.INSERT);
    writeQuery.setInsert(insert);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(writeQuery);

    return query;
};

rethinkdb.query.Table.prototype.insert = function(docs) {
    return new rethinkdb.query.InsertQuery(this, docs);
};
goog.exportProperty(rethinkdb.query.Table.prototype, 'insert',
                    rethinkdb.query.Table.prototype.insert);
