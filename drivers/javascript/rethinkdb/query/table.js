goog.provide('rethinkdb.Table');

goog.require('rethinkdb');
goog.require('rethinkdb.Expression');

/**
 * @class A table in a database
 * @param {string} tableName
 * @param {string=} opt_dbName
 * @constructor
 * @extends {rethinkdb.Expression}
 */
rethinkdb.Table = function(tableName, opt_dbName) {
    this.db_ = opt_dbName || null;
    this.name_ = tableName;
};
goog.inherits(rethinkdb.Table, rethinkdb.Expression);

/**
 * @override
 * @ignore
 */
rethinkdb.Table.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.TABLE);

    var table = new Term.Table();
    var tableRef = new TableRef();
    tableRef.setDbName(this.db_ || rethinkdb.last_connection_.getDefaultDb());
    tableRef.setTableName(this.name_);
    table.setTableRef(tableRef);

    term.setTable(table);
    return term;
};

/**
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.GetExpression = function(table, key) {
    this.table_ = table;
    this.key_ = key;
};
goog.inherits(rethinkdb.GetExpression, rethinkdb.Expression);

/** @override */
rethinkdb.GetExpression.prototype.compile = function() {
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

/**
 * Return a single row of this table by key
 * @param {*} key
 */
rethinkdb.Table.prototype.get = function(key) {
    argCheck_(arguments, 1);
    key = wrapIf_(key);
    return newExpr_(rethinkdb.GetExpression, this, key);
};
goog.exportProperty(rethinkdb.Table.prototype, 'get',
                    rethinkdb.Table.prototype.get);

/**
 * @param {rethinkdb.Table} table
 * @param {Array.<rethinkdb.Expression>} docs
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.InsertQuery = function(table, docs) {
    this.table_ = table;
    this.docs_ = docs;
};
goog.inherits(rethinkdb.InsertQuery, rethinkdb.Query);

/** @override */
rethinkdb.InsertQuery.prototype.buildQuery = function() {
    var tableTerm = this.table_.compile();
    var tableRef = tableTerm.getTable().getTableRefOrDefault();

    var insert = new WriteQuery.Insert();
    insert.setTableRef(tableRef);

    for (var i = 0; i < this.docs_.length; i++) {
        insert.addTerms(this.docs_[i].compile());
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
 * Insert a json document into this table
 * @param {*} docs An object or list of objects to insert
 */
rethinkdb.Table.prototype.insert = function(docs) {
    argCheck_(arguments, 1);
    if (!goog.isArray(docs))
        docs = [docs];
    docs = docs.map(rethinkdb.expr);
    return new rethinkdb.InsertQuery(this, docs);
};
goog.exportProperty(rethinkdb.Table.prototype, 'insert',
                    rethinkdb.Table.prototype.insert);

/**
 * @param {rethinkdb.Expression} view
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.DeleteQuery = function(view) {
    this.view_ = view;
};
goog.inherits(rethinkdb.DeleteQuery, rethinkdb.Query);

/** @override */
rethinkdb.DeleteQuery.prototype.buildQuery = function() {
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
 * @param {rethinkdb.Table} table
 * @param {rethinkdb.Expression} key
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.PointDeleteQuery = function(table, key) {
    this.table_ = table;
    this.key_ = key;
};
goog.inherits(rethinkdb.PointDeleteQuery, rethinkdb.Query);

/** @override */
rethinkdb.PointDeleteQuery.prototype.buildQuery = function() {
    var pointdelete = new WriteQuery.PointDelete();
    pointdelete.setTableRef(this.table_.compile().getTableOrDefault().getTableRefOrDefault());
    pointdelete.setAttrname('id');
    pointdelete.setKey(this.key_.compile());

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.POINTDELETE);
    write.setPointDelete(pointdelete);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Deletes all rows the current view
 */
rethinkdb.Expression.prototype.del = function() {
    if (this instanceof rethinkdb.GetExpression) {
        return new rethinkdb.PointDeleteQuery(this.table_, this.key_);
    } else {
        return new rethinkdb.DeleteQuery(this);
    }
};
goog.exportProperty(rethinkdb.Expression.prototype, 'del',
                    rethinkdb.Expression.prototype.del);

/**
 * @param {rethinkdb.Expression} view
 * @param {rethinkdb.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.UpdateQuery = function(view, mapping) {
    this.view_ = view;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.UpdateQuery, rethinkdb.Query);

/** @override */
rethinkdb.UpdateQuery.prototype.buildQuery = function() {
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
 * @param {rethinkdb.Table} table
 * @param {rethinkdb.Expression} key
 * @param {rethinkdb.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.PointUpdateQuery = function(table, key, mapping) {
    this.table_ = table;
    this.key_ = key;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.PointUpdateQuery, rethinkdb.Query);

/** @override */
rethinkdb.PointUpdateQuery.prototype.buildQuery = function() {
    var mapping = new Mapping();
    mapping.setArg(this.mapping_.args[0]);
    mapping.setBody(this.mapping_.body.compile());

    var pointupdate = new WriteQuery.PointUpdate();
    pointupdate.setTableRef(this.table_.compile().getTableOrDefault().getTableRefOrDefault());
    pointupdate.setAttrname('id');
    pointupdate.setKey(this.key_.compile());
    pointupdate.setMapping(mapping);

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.POINTUPDATE);
    write.setPointUpdate(pointupdate);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Updates each row in the current view by merging in the result
 * of the given mapping function applied to that row
 * @param {function(...)|rethinkdb.FunctionExpression|rethinkdb.Expression} mapping
 */
rethinkdb.Expression.prototype.update = function(mapping) {
    argCheck_(arguments, 1);
    mapping = functionWrap_(mapping);
    if (this instanceof rethinkdb.GetExpression) {
        return new rethinkdb.PointUpdateQuery(this.table_, this.key_, mapping);
    } else {
        return new rethinkdb.UpdateQuery(this, mapping);
    }
};
goog.exportProperty(rethinkdb.Expression.prototype, 'update',
                    rethinkdb.Expression.prototype.update);

/**
 * @param {rethinkdb.Expression} view
 * @param {rethinkdb.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.MutateQuery = function(view, mapping) {
    this.view_ = view;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.MutateQuery, rethinkdb.Query);

/** @override */
rethinkdb.MutateQuery.prototype.buildQuery = function() {
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
 * @param {rethinkdb.Table} table
 * @param {rethinkdb.Expression} key
 * @param {rethinkdb.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.PointMutateQuery = function(table, key, mapping) {
    this.table_ = table;
    this.key_ = key;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.PointMutateQuery, rethinkdb.Query);

/** @override */
rethinkdb.PointMutateQuery.prototype.buildQuery = function() {
    var mapping = new Mapping();
    mapping.setArg(this.mapping_.args[0]);
    mapping.setBody(this.mapping_.body.compile());

    var pointmutate = new WriteQuery.PointMutate();
    pointmutate.setTableRef(this.table_.compile().getTableOrDefault().getTableRefOrDefault());
    pointmutate.setAttrname('id');
    pointmutate.setKey(this.key_.compile());
    pointmutate.setMapping(mapping);

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.POINTMUTATE);
    write.setPointMutate(pointmutate);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Replcaces each row of the current view with the result of the
 * mapping function as applied to the current row.
 * @param {function(...)|rethinkdb.FunctionExpression|rethinkdb.Expression} mapping
 */
rethinkdb.Expression.prototype.mutate = function(mapping) {
    argCheck_(arguments, 1);
    mapping = functionWrap_(mapping);
    if (this instanceof rethinkdb.GetExpression) {
        return new rethinkdb.PointMutateQuery(this.table_, this.key_, mapping);
    } else {
        return new rethinkdb.MutateQuery(this, mapping);
    }
};
goog.exportProperty(rethinkdb.Expression.prototype, 'mutate',
                    rethinkdb.Expression.prototype.mutate);
