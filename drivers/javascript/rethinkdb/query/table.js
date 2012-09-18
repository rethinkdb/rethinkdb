goog.provide('rethinkdb.query.Table');

goog.require('rethinkdb.query.Expression');

/**
 * @class A table in a database
 * @param {string} tableName
 * @param {string=} opt_dbName
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.Table = function(tableName, opt_dbName) {
    this.db_ = opt_dbName || null;
    this.name_ = tableName;
};
goog.inherits(rethinkdb.query.Table, rethinkdb.query.Expression);

/**
 * @override
 * @ignore
 */
rethinkdb.query.Table.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.TABLE);

    var table = new Term.Table();
    var tableRef = new TableRef();
    tableRef.setDbName(this.db_ || rethinkdb.net.last_connection_.getDefaultDb());
    tableRef.setTableName(this.name_);
    table.setTableRef(tableRef);

    term.setTable(table);
    return term;
};

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.GetExpression = function(table, key) {
    this.table_ = table;
    this.key_ = key;
};
goog.inherits(rethinkdb.query.GetExpression, rethinkdb.query.Expression);

/** @override */
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

/**
 * Return a single row of this table by key
 * @param {*} key
 */
rethinkdb.query.Table.prototype.get = function(key) {
    argCheck_(arguments, 1);
    key = wrapIf_(key);
    return newExpr_(rethinkdb.query.GetExpression, this, key);
};
goog.exportProperty(rethinkdb.query.Table.prototype, 'get',
                    rethinkdb.query.Table.prototype.get);

/**
 * @param {rethinkdb.query.Table} table
 * @param {Array.<rethinkdb.query.Expression>} docs
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.InsertQuery = function(table, docs) {
    this.table_ = table;
    this.docs_ = docs;
};
goog.inherits(rethinkdb.query.InsertQuery, rethinkdb.query.Query);

/** @override */
rethinkdb.query.InsertQuery.prototype.buildQuery = function() {
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
rethinkdb.query.Table.prototype.insert = function(docs) {
    argCheck_(arguments, 1);
    if (!goog.isArray(docs))
        docs = [docs];
    docs = docs.map(rethinkdb.query.expr);
    return new rethinkdb.query.InsertQuery(this, docs);
};
goog.exportProperty(rethinkdb.query.Table.prototype, 'insert',
                    rethinkdb.query.Table.prototype.insert);

/**
 * @param {rethinkdb.query.Expression} view
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.DeleteQuery = function(view) {
    this.view_ = view;
};
goog.inherits(rethinkdb.query.DeleteQuery, rethinkdb.query.Query);

/** @override */
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
 * @param {rethinkdb.query.Table} table
 * @param {rethinkdb.query.Expression} key
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.PointDeleteQuery = function(table, key) {
    this.table_ = table;
    this.key_ = key;
};
goog.inherits(rethinkdb.query.PointDeleteQuery, rethinkdb.query.Query);

/** @override */
rethinkdb.query.PointDeleteQuery.prototype.buildQuery = function() {
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
rethinkdb.query.Expression.prototype.del = function() {
    if (this instanceof rethinkdb.query.GetExpression) {
        return new rethinkdb.query.PointDeleteQuery(this.table_, this.key_);
    } else {
        return new rethinkdb.query.DeleteQuery(this);
    }
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'del',
                    rethinkdb.query.Expression.prototype.del);

/**
 * @param {rethinkdb.query.Expression} view
 * @param {rethinkdb.query.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.UpdateQuery = function(view, mapping) {
    this.view_ = view;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.query.UpdateQuery, rethinkdb.query.Query);

/** @override */
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
 * @param {rethinkdb.query.Table} table
 * @param {rethinkdb.query.Expression} key
 * @param {rethinkdb.query.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.PointUpdateQuery = function(table, key, mapping) {
    this.table_ = table;
    this.key_ = key;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.query.PointUpdateQuery, rethinkdb.query.Query);

/** @override */
rethinkdb.query.PointUpdateQuery.prototype.buildQuery = function() {
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
 * @param {function(...)|rethinkdb.query.FunctionExpression|rethinkdb.query.Expression} mapping
 */
rethinkdb.query.Expression.prototype.update = function(mapping) {
    argCheck_(arguments, 1);
    mapping = functionWrap_(mapping);
    if (this instanceof rethinkdb.query.GetExpression) {
        return new rethinkdb.query.PointUpdateQuery(this.table_, this.key_, mapping);
    } else {
        return new rethinkdb.query.UpdateQuery(this, mapping);
    }
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'update',
                    rethinkdb.query.Expression.prototype.update);

/**
 * @param {rethinkdb.query.Expression} view
 * @param {rethinkdb.query.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.MutateQuery = function(view, mapping) {
    this.view_ = view;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.query.MutateQuery, rethinkdb.query.Query);

/** @override */
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
 * @param {rethinkdb.query.Table} table
 * @param {rethinkdb.query.Expression} key
 * @param {rethinkdb.query.FunctionExpression} mapping
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.PointMutateQuery = function(table, key, mapping) {
    this.table_ = table;
    this.key_ = key;
    this.mapping_ = mapping;
};
goog.inherits(rethinkdb.query.PointMutateQuery, rethinkdb.query.Query);

/** @override */
rethinkdb.query.PointMutateQuery.prototype.buildQuery = function() {
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
 * @param {function(...)|rethinkdb.query.FunctionExpression|rethinkdb.query.Expression} mapping
 */
rethinkdb.query.Expression.prototype.mutate = function(mapping) {
    argCheck_(arguments, 1);
    mapping = functionWrap_(mapping);
    if (this instanceof rethinkdb.query.GetExpression) {
        return new rethinkdb.query.PointMutateQuery(this.table_, this.key_, mapping);
    } else {
        return new rethinkdb.query.MutateQuery(this, mapping);
    }
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'mutate',
                    rethinkdb.query.Expression.prototype.mutate);
