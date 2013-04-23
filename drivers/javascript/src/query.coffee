goog.provide("rethinkdb.query")

goog.require("rethinkdb.base")
goog.require("rethinkdb.ast")

rethinkdb.expr = ar (val) ->
    if val instanceof TermBase
        val
    else if val instanceof Function
        new Func {}, val
    else if goog.isArray val
        new MakeArray {}, val...
    else if goog.isObject val
        new MakeObject val
    else
        new DatumTerm val

rethinkdb.js = aropt (jssrc, opts) -> new JavaScript opts, jssrc

rethinkdb.error = ar (errstr) -> new UserError {}, errstr

rethinkdb.row = new ImplicitVar {}

rethinkdb.table = aropt (tblName, opts) -> new Table opts, tblName

rethinkdb.db = ar (dbName) -> new Db {}, dbName

rethinkdb.dbCreate = ar (dbName) -> new DbCreate {}, dbName

rethinkdb.dbDrop = ar (dbName) -> new DbDrop {}, dbName

rethinkdb.dbList = ar () -> new DbList {}

rethinkdb.do = (args...) ->
    unless args.length >= 1
        throw new RqlDriverError "Expected 1 or more argument(s) but found #{args.length}."
    new FunCall {}, funcWrap(args[-1..][0]), args[...-1]...

rethinkdb.branch = ar (test, trueBranch, falseBranch) -> new Branch {}, test, trueBranch, falseBranch

rethinkdb.count =           {'COUNT': true}
rethinkdb.sum   = ar (attr) -> {'SUM': attr}
rethinkdb.avg   = ar (attr) -> {'AVG': attr}

rethinkdb.asc = (attr) -> new Asc {}, attr
rethinkdb.desc = (attr) -> new Desc {}, attr

rethinkdb.eq = (args...) -> new Eq {}, args...
rethinkdb.ne = (args...) -> new Ne {}, args...
rethinkdb.lt = (args...) -> new Lt {}, args...
rethinkdb.le = (args...) -> new Le {}, args...
rethinkdb.gt = (args...) -> new Gt {}, args...
rethinkdb.ge = (args...) -> new Ge {}, args...
rethinkdb.or = (args...) -> new Any {}, args...
rethinkdb.and = (args...) -> new All {}, args...

rethinkdb.not = (x) -> new Not {}, x

rethinkdb.add = (args...) -> new Add {}, args...
rethinkdb.sub = (args...) -> new Sub {}, args...
rethinkdb.mul = (args...) -> new Mul {}, args...
rethinkdb.div = (args...) -> new Div {}, args...
rethinkdb.mod = (a, b) -> new Mod {}, a, b

