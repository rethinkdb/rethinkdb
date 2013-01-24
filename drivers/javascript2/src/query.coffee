goog.provide("rethinkdb.query")

goog.require("rethinkdb.base")
goog.require("rethinkdb.ast")

rethinkdb.expr = (val) ->
    if val instanceof TermBase
        val
    else if typeof val is 'function'
        new Func {}, val
    else if goog.isArray val
        new MakeArray {}, val...
    else if goog.isObject val
        new MakeObject val
    else
        new DatumTerm val

rethinkdb.js = (jssrc) -> new JavaScript {}, jssrc

rethinkdb.error = (errstr) -> new UserError {}, errstr

rethinkdb.row = new ImplicitVar {}

rethinkdb.db = (dbName) -> new Db {}, dbName

rethinkdb.table = (tblName, opts) -> new Table opts, tblName

rethinkdb.dbCreate = (dbName) -> new DbCreate {}, dbName

rethinkdb.dbDrop = (dbName) -> new DbDrop {}, dbName

rethinkdb.dbList = -> new DbList {}

rethinkdb.branch = (test, trueBranch, falseBranch) -> new Branch {}, test, trueBranch, falseBranch
