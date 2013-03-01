goog.provide("rethinkdb.query")

goog.require("rethinkdb.base")
goog.require("rethinkdb.ast")

# Function wrapper that enforces that the function is
# called with the correct number of arguments
ar = (fun) -> (args...) ->
    if args.length isnt fun.length
        throw new RqlDriverError "Expected #{fun.length} argument(s) but found #{args.length}."
    fun.apply(@, args)

rethinkdb.expr = (val) ->
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

rethinkdb.js = (jssrc) -> new JavaScript {}, jssrc

rethinkdb.error = ar (errstr) -> new UserError {}, errstr

rethinkdb.row = new ImplicitVar {}

rethinkdb.db = (dbName) -> new Db {}, dbName

rethinkdb.dbCreate = (dbName) -> new DbCreate {}, dbName

rethinkdb.dbDrop = (dbName) -> new DbDrop {}, dbName

rethinkdb.dbList = -> new DbList {}

rethinkdb.do = (args...) -> new FunCall {}, funcWrap(args[-1..][0]), args[...-1]...

rethinkdb.branch = (test, trueBranch, falseBranch) -> new Branch {}, test, trueBranch, falseBranch

rethinkdb.count =           {'COUNT': true}
rethinkdb.sum   = (attr) -> {'SUM': attr}
rethinkdb.avg   = (attr) -> {'AVG': attr}
