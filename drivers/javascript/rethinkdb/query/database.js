goog.provide('rethinkdb.query.Database');

goog.require('rethinkdb.query');

/**
 * @param {MetaQuery.MetaQueryType} type
 * @param {string=} opt_dbName
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.MetaQuery = function(type, opt_dbName) {
    this.type_ = type;
    this.dbName_ = opt_dbName || null;
};
goog.inherits(rethinkdb.query.MetaQuery, rethinkdb.query.BaseQuery);

/** @override */
rethinkdb.query.MetaQuery.prototype.buildQuery = function() {
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

/** @export */
rethinkdb.query.dbCreate = function(dbName, primary_datacenter) {
    //TODO how to get cluster level default?
    primary_datacenter = primary_datacenter || 'cluster-level-default?';

    return new rethinkdb.query.MetaQuery(MetaQuery.MetaQueryType.CREATE_DB, dbName);
};

/** @export */
rethinkdb.query.dbDrop = function(dbName) {
    return new rethinkdb.query.MetaQuery(MetaQuery.MetaQueryType.DROP_DB, dbName);
};

/** @export */
rethinkdb.query.dbList = function() {
    return new rethinkdb.query.MetaQuery(MetaQuery.MetaQueryType.LIST_DBS);
};

/**
 * @constructor
 */
rethinkdb.query.Database = function(db_name) {
    this.name_ = db_name;
};

/**
 * @return {rethinkdb.query.Database}
 * @export
 */
rethinkdb.query.db = function(db_name) {
     return new rethinkdb.query.Database(db_name);
};

rethinkdb.query.Database.prototype.list = function() {
    return new rethinkdb.query.MetaQuery(MetaQuery.MetaQueryType.LIST_TABLES, this.name_);
};
goog.exportProperty(rethinkdb.query.Database.prototype, 'list',
                    rethinkdb.query.Database.prototype.list);

/**
 * @param {string} dataCenter
 * @param {string} dbName
 * @param {string} tableName
 * @param {string=} opt_primaryKey
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.CreateTableQuery = function(dataCenter, dbName, tableName, opt_primaryKey) {
    this.dataCenter_ = dataCenter;
    this.dbName_ = dbName;
    this.tableName_ = tableName;
    this.primaryKey = opt_primaryKey || null;
};
goog.inherits(rethinkdb.query.CreateTableQuery, rethinkdb.query.BaseQuery);

rethinkdb.query.CreateTableQuery.prototype.buildQuery = function() {
    var tableref = new TableRef();
    tableref.setDbName(this.dbName_);
    tableref.setTableName(this.tableName_);

    var createtable = new MetaQuery.CreateTable();
    createtable.setDatacenter(this.dataCenter_);
    createtable.setTableRef(tableref);
    if (this.primaryKey_)
        createtable.setPrimaryKey(this.primaryKey_);

    var meta = new MetaQuery();
    meta.setType(MetaQuery.MetaQueryType.CREATE_TABLE);
    meta.setCreateTable(createtable);

    var query = new Query();
    query.setType(Query.QueryType.META);
    query.setMetaQuery(meta);

    return query;
};

/**
 * @param {string} tableName
 * @param {string=} opt_primaryKey
 */
rethinkdb.query.Database.prototype.create = function(tableName, opt_primaryKey) {
    return new rethinkdb.query.CreateTableQuery('Welcome-dc', this.name_, tableName, opt_primaryKey);
};
goog.exportProperty(rethinkdb.query.Database.prototype, 'create',
                    rethinkdb.query.Database.prototype.create);

/**
 * @param {string} dbName
 * @param {string} tableName
 * @constructor
 * @extends {rethinkdb.query.BaseQuery}
 */
rethinkdb.query.DropTableQuery = function(dbName, tableName) {
    this.dbName_ = dbName;
    this.tableName_ = tableName;
};
goog.inherits(rethinkdb.query.DropTableQuery, rethinkdb.query.BaseQuery);

rethinkdb.query.DropTableQuery.prototype.buildQuery = function() {
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

/**
 * drop table from this database
 */
rethinkdb.query.Database.prototype.drop = function(tableName) {
    return new rethinkdb.query.DropTableQuery(this.name_, tableName);
};
goog.exportProperty(rethinkdb.query.Database.prototype, 'drop',
                    rethinkdb.query.Database.prototype.drop);

rethinkdb.query.Database.prototype.table = function(tableName) {
    return new rethinkdb.query.Table(tableName, this.name_);
};
goog.exportProperty(rethinkdb.query.Database.prototype, 'table',
                    rethinkdb.query.Database.prototype.table);
