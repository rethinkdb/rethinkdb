goog.provide('rethinkdb.AST')

class RDBNode
    eval: -> throw "Abstract Method"

class RDBDatum extends RDBNode
    constructor: (val) ->
        @data = val

    eval: -> @data

class RDBOp extends RDBNode
    constructor: (args, optargs) ->
        @args = args
        @optargs = optargs

    # Overloaded by each operation to specify its argument types
    type: ""

    # Overloaded by each operation to specify how to evaluate
    op: -> throw "Abstract Method"

    eval: ->
        # Eval arguments and check types
        args = []
        for n,i in @args
            try
                v = n.eval()
                # Ignore type checking for now
                args.push v
            catch err
                err.backtrace.unshift i
                throw err

        optargs = {}
        for k,n of @optargs
            try
                v = n.eval()
                # Ignore type checking for now
                optargs[k] = v
            catch err
                err.backtrace.unshift k
                throw err

        # Eval this node
        @op(args, optargs)

class MakeArray extends RDBOp
    type: "DATUM... -> Array"
    op: (args) -> new RDBArray args

class MakeObj extends RDBOp
    type: "k:v... -> Object"
    op: (args, optArgs) ->
        obj = new RDBObject
        for k,v of optargs
            obj[k] = v

class JavaScript extends RDBOp
    type: "STR -> DATUM"
    op: (args) -> eval args[0]

class UserError extends RDBNode
    eval: -> throw new RuntimeError

class DBRef extends RDBOp
    type: "STR -> DB"
    op: (args) -> universe.getDatabase arg[0]

class TableRef extends RDBOp
    type: "STR, DB -> TABLE"
    op: (args, optargs) ->
        arg[1].getTable args[0]

class GetByKey extends RDBOp
    type: "TABLE, STR -> DATUM"
    op: (args) -> args[0].get args[1]

class CompareOp extends RDBOp
    type: "DATUM... -> BOOL"
	cop: "Abstract class variable"
	op: (args) ->
        new RDBPrimitive args[1..].map(
            (v) -> v[cop](args[0]).asJSON()
        ).reduce(
            (acc, v) -> acc && v
        , true)

class Eq extends CompareOp
    cop: 'eq'

class Ne extends CompareOp
    cop: 'ne'

class Lt extends CompareOp
    cop: 'lt'

class Le extends CompareOp
    cop: 'le'

class Gt extends CompareOp
    cop: 'gt'

class Ge extends CompareOp
    cop: 'ge'
