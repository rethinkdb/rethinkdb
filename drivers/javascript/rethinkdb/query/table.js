goog.provide('rethinkdb.query.Table');

goog.require('rethinkdb.query.Expression');

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

/**
 * Insert a json document into a table
 */
rethinkdb.query.Table.prototype.insert = function(docs) {
    return new rethinkdb.query.InsertQuery(this, docs);
};
goog.exportProperty(rethinkdb.query.Table.prototype, 'insert',
                    rethinkdb.query.Table.prototype.insert);

/**
 * @param {rethinkdb.query.Expression} view
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.DeleteQuery = function(view) {
    this.view_ = view;
};
goog.inherits(rethinkdb.query.DeleteQuery, rethinkdb.query.BaseQuery);

rethinkdb.query.DeleteQuery.prototype.buildQuery = function() {
    var del = new WriteQuery.Delete();
    del.setView(this.view_.compile());

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.DELETE);
    write.setDelete(del);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Deletes all rows the current view
 */
rethinkdb.query.Expression.prototype.del = function() {
    return new rethinkdb.query.DeleteQuery(this);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'del',
                    rethinkdb.query.Expression.prototype.del);

/**
 * @param {rethinkdb.query.Expression} view
 * @param {rethinkdb.query.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.UpdateQuery = function(view, mapping) {
    this.view_ = view;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.query.UpdateQuery, rethinkdb.query.BaseQuery);

rethinkdb.query.UpdateQuery.prototype.buildQuery = function() {
    var mapping = new Mapping();
    mapping.setArg(this.mapping_.args[0]);
    mapping.setBody(this.mapping_.body.compile());

    var update = new WriteQuery.Update();
    update.setView(this.view_.compile());
    update.setMapping(mapping);

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.UPDATE);
    write.setUpdate(update);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Updates all rows according to the given mapping function
 */
rethinkdb.query.Expression.prototype.update = function(mapping) {
    mapping = functionWrap_(mapping);
    return new rethinkdb.query.UpdateQuery(this, mapping);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'update',
                    rethinkdb.query.Expression.prototype.update);

/**
 * @param {rethinkdb.query.Expression} view
 * @param {rethinkdb.query.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.MutateQuery = function(view, mapping) {
    this.view_ = view;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.query.MutateQuery, rethinkdb.query.BaseQuery);

rethinkdb.query.MutateQuery.prototype.buildQuery = function() {
    var mapping = new Mapping();
    mapping.setArg(this.mapping_.args[0]);
    mapping.setBody(this.mapping_.body.compile());

    var mutate = new WriteQuery.Mutate();
    mutate.setView(this.view_.compile());
    mutate.setMapping(mapping);

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.MUTATE);
    write.setMutate(mutate);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Replcaces each row the the result of the mapping expression
 */
rethinkdb.query.Expression.prototype.mutate = function(mapping) {
    mapping = functionWrap_(mapping);
    return new rethinkdb.query.MutateQuery(this, mapping);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'mutate',
                    rethinkdb.query.Expression.prototype.mutate);
