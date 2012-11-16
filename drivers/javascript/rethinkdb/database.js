// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.Database');

goog.require('rethinkdbmdl');

/**
 * Construct a new adminquery node of the given type on the given database
 * @class A query to perform operations on top level databases
 * @param {MetaQuery.MetaQueryType} type The metaquery type to perform
 * @param {string=} opt_dbName The database to perform this query on
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.AdminQuery = function(type, opt_dbName) {
    this.type_ = type;
    this.dbName_ = opt_dbName || null;
};
goog.inherits(rethinkdb.AdminQuery, rethinkdb.Query);

/** @override */
rethinkdb.AdminQuery.prototype.buildQuery = function(opt_buildOpts) {
    var meta = new MetaQuery();
    meta.setType(this.type_);
    if (this.dbName_) {
        meta.setDbName(this.dbName_);
    }

    var query = new Query();
    query.setType(Query.QueryType.META);
    query.setMetaQuery(meta);

    return query;
};

/** @override */
rethinkdb.AdminQuery.prototype.formatQuery = function(bt) {

    var name = this.dbName_ ? "'"+this.dbName_+"'" : "";
    var methodName;
    var finalize = '';

    switch (this.type_) {
    case MetaQuery.MetaQueryType.CREATE_DB:
        methodName = 'dbCreate';
    break;
    case MetaQuery.MetaQueryType.DROP_DB:
        methodName = 'dbDrop';
    break;
    case MetaQuery.MetaQueryType.LIST_DBS:
        methodName = 'dbList';
    break;
    case MetaQuery.MetaQueryType.LIST_TABLES:
        methodName = 'db';
        finalize = '.tableList()';
    }

    var result1 = "r."+methodName+"("+name+")";
    var result2 = result1+finalize;

    if (!bt) {
        return result2;
    } else {
        goog.asserts.assert(bt.length === 0);
        return rethinkdb.util.carrotify_(result1)+rethinkdb.util.spaceify_(finalize);
    }
};

/**
 * Create a new database with the given name.
 * @param {string} dbName
 * @export
 */
rethinkdb.dbCreate = function(dbName) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(dbName, 'string');
    return new rethinkdb.AdminQuery(MetaQuery.MetaQueryType.CREATE_DB, dbName);
};

/**
 * Drop the given database.
 * @param {string} dbName
 * @export
 */
rethinkdb.dbDrop = function(dbName) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(dbName, 'string');
    return new rethinkdb.AdminQuery(MetaQuery.MetaQueryType.DROP_DB, dbName);
};

/**
 * List all databases.
 * @export
 */
rethinkdb.dbList = function() {
    return new rethinkdb.AdminQuery(MetaQuery.MetaQueryType.LIST_DBS);
};

/**
 * @class A reference to a database.
 * @param {string} dbName
 * @constructor
 */
rethinkdb.Database = function(dbName) {
    this.name_ = dbName;
};

/**
 * Construct a database reference.
 * @param {string} dbName
 * @return {rethinkdb.Database}
 * @export
 */
rethinkdb.db = function(dbName) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(dbName, 'string');
    return new rethinkdb.Database(dbName);
};

/**
 * List all tables in this database
 */
rethinkdb.Database.prototype.tableList = function() {
    return new rethinkdb.AdminQuery(MetaQuery.MetaQueryType.LIST_TABLES, this.name_);
};
goog.exportProperty(rethinkdb.Database.prototype, 'tableList',
                    rethinkdb.Database.prototype.tableList);

/**
 * @class A query that creates a table in a database
 * @param {string} dataCenter
 * @param {string} dbName
 * @param {string} tableName
 * @param {string} primaryKey
 * @param {string} cacheSize
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.CreateTableQuery = function(dataCenter, dbName, tableName, primaryKey, cacheSize) {
    this.dataCenter_ = dataCenter;
    this.dbName_ = dbName;
    this.tableName_ = tableName;
    this.primaryKey_ = primaryKey;
    this.cacheSize_ = cacheSize;
};
goog.inherits(rethinkdb.CreateTableQuery, rethinkdb.Query);

/** @override */
rethinkdb.CreateTableQuery.prototype.buildQuery = function(opt_buildOpts) {
    var tableref = new TableRef();
    tableref.setDbName(this.dbName_);
    tableref.setTableName(this.tableName_);

    var createtable = new MetaQuery.CreateTable();
    if (this.dataCenter_) createtable.setDatacenter(this.dataCenter_);
    createtable.setTableRef(tableref);
    if (this.primaryKey_) createtable.setPrimaryKey(this.primaryKey_);
    if (this.cacheSize_) createtable.setCacheSize(this.cacheSize_);

    var meta = new MetaQuery();
    meta.setType(MetaQuery.MetaQueryType.CREATE_TABLE);
    meta.setCreateTable(createtable);

    var query = new Query();
    query.setType(Query.QueryType.META);
    query.setMetaQuery(meta);

    return query;
};

/** @override */
rethinkdb.CreateTableQuery.prototype.formatQuery = function(bt) {
    var args;
    if (this.dataCenter_ || this.primaryKey_ || this.cacheSize_) {
        args = [];
        if (this.dataCenter_) args.push("dataCenter: '"+this.dataCenter_+"'");
        if (this.primaryKey_) args.push("primaryKey: '"+this.primaryKey_+"'");
        if (this.cacheSize_) args.push("cacheSize: '"+this.cacheSize_+"'");
        if (this.tableName_) args.push("tableName: '"+this.tableName_+"'");
        args = "{"+ args.join(', ') + "}";
    } else {
        args = "'"+this.tableName_+"'";
    }

    var result = "r.db('"+this.dbName_+"').create("+args+")";

    if (!bt) {
        return result;
    } else {
        return rethinkdb.util.carrotify_(result);
    }
};

/**
 * Create a new table in the database
 * @param {string|Object} tableNameOrOptions Either pass a string giving
 *  the name of the new table or an options dictionary providing:
 *      dataCenter - the primary datacenter for the new table
 *      tableName - the name of the new table
 *      primaryKey - the primary key for the new table
 *      cacheSize - the cache size limit for the new table
 */
rethinkdb.Database.prototype.tableCreate = function(tableNameOrOptions) {
    rethinkdb.util.argCheck_(arguments, 1);

    var options = {};
    if (typeof tableNameOrOptions === 'string') {
        options['tableName'] = tableNameOrOptions;
    } else {
        options = tableNameOrOptions;
    }

    rethinkdb.util.typeCheck_(options['tableName'], 'string');
    rethinkdb.util.typeCheck_(options['primaryKey'], 'string');
    rethinkdb.util.typeCheck_(options['dataCenter'], 'string');
    rethinkdb.util.typeCheck_(options['cacheSize'], 'string');

    return new rethinkdb.CreateTableQuery(options['dataCenter'], this.name_, options['tableName'], options['primaryKey'], options['cacheSize']);
};
goog.exportProperty(rethinkdb.Database.prototype, 'tableCreate',
                    rethinkdb.Database.prototype.tableCreate);

/**
 * @class A query that drops a table from a database
 * @param {string} dbName
 * @param {string} tableName
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.DropTableQuery = function(dbName, tableName) {
    rethinkdb.util.argCheck_(arguments, 1);
    this.dbName_ = dbName;
    this.tableName_ = tableName;
};
goog.inherits(rethinkdb.DropTableQuery, rethinkdb.Query);

/** @override */
rethinkdb.DropTableQuery.prototype.buildQuery = function(opt_buildOpts) {
    var tableref = new TableRef();
    tableref.setDbName(this.dbName_);
    tableref.setTableName(this.tableName_);

    var meta = new MetaQuery();
    meta.setType(MetaQuery.MetaQueryType.DROP_TABLE);
    meta.setDropTable(tableref);

    var query = new Query();
    query.setType(Query.QueryType.META);
    query.setMetaQuery(meta);

    return query;
};

/** @override */
rethinkdb.DropTableQuery.prototype.formatQuery = function(bt) {
    if (!bt) {
        return "r.db('"+this.dbName_+"').drop('"+this.tableName_+"')";
    } else {
        return rethinkdb.util.carrotify_("r.db('"+this.dbName_+"').drop('"+this.tableName_+"')");
    }
};

/**
 * Drop a table from this database
 * @param {string} tableName
 */
rethinkdb.Database.prototype.tableDrop = function(tableName) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(tableName, 'string');
    return new rethinkdb.DropTableQuery(this.name_, tableName);
};
goog.exportProperty(rethinkdb.Database.prototype, 'tableDrop',
                    rethinkdb.Database.prototype.tableDrop);

/**
 * Construct a table reference for a table in this database
 * @param {string} tableName
 * @param {boolean=} opt_allowOutdated
 */
rethinkdb.Database.prototype.table = function(tableName, opt_allowOutdated) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(tableName, 'string');
    rethinkdb.util.typeCheck_(opt_allowOutdated, 'boolean');
    return new rethinkdb.Table(tableName, this.name_, opt_allowOutdated);
};
goog.exportProperty(rethinkdb.Database.prototype, 'table',
                    rethinkdb.Database.prototype.table);
