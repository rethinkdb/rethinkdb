goog.provide('rethinkdb.AST')

goog.require('rethinkdb.TypeChecker')

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
    type: null

    # Overloaded by each operation to specify how to evaluate
    op: -> throw "Abstract Method"

    eval: (context) ->
        # Eval arguments and check types
        args = []
        for n,i in @args
            try
                v = n.eval(context)
                args.push v
            catch err
                unless err instanceof RqlError then throw err
                err.backtrace.unshift i
                throw err

        optargs = {}
        for k,n of @optargs
            try
                v = n.eval(context)
                optargs[k] = v
            catch err
                unless err instanceof RqlError then throw err
                err.backtrace.unshift k
                throw err

        # Eval this node
        op = (args...) => @op(args...) # This binding in JS is so stupid
        @type.checkType op, args, optargs, context

class RDBWriteOp extends RDBOp
    eval: (context) ->
        res = super(context)
        context.universe.save()
        return res

class RDBMakeArray extends RDBOp
    type: tp "DATUM... -> ARRAY"
    op: (args) -> new RDBArray args

class RDBMakeObj extends RDBOp
    type: tp "-> OBJECT"
    op: (args, optargs) ->
        new RDBObject optargs

class RDBVar extends RDBOp
    type: tp "!NUMBER -> DATUM"
    op: (args, optargs, context) ->
        context.lookupVar args[0].asJSON()

class RDBJavaScript extends RDBOp
    type: tp "STRING -> DATUM"
    op: (args) -> new RDBPrimitive eval args[0].asJSON()

class RDBUserError extends RDBOp
    type: tp "STRING -> Error"
    op: (args) -> throw new RqlRuntimeError args[0].asJSON()

class RDBImplicitVar extends RDBOp
    type: tp "-> DATUM"
    op: (args, optargs, context) -> context.getImplicitVar()

class RDBDBRef extends RDBOp
    type: tp "STRING -> Database"
    op: (args, optargs, context) -> context.universe.getDatabase args[0]

class RDBTableRef extends RDBOp
    type: tp "Database, STRING, {use_outdated:BOOL} -> Table"
    op: (args, optargs) -> args[0].getTable args[1]

class RDBGetByKey extends RDBOp
    type: tp "Table, STRING -> SingleSelection | Table, NUMBER -> SingleSelection"
    op: (args) -> args[0].get args[1]

class RDBNot extends RDBOp
    type: tp "BOOL -> BOOL"
    op: (args) -> new RDBPrimitive not args[0].asJSON()

class RDBCompareOp extends RDBOp
    type: tp "DATUM... -> BOOL"
    cop: "Abstract class RDBvariable"
    op: (args) ->
        i = 1
        while i < args.length
            if not args[i-1][@cop](args[i]).asJSON()
                return new RDBPrimitive false
            i++
        return new RDBPrimitive true

class RDBEq extends RDBCompareOp
    cop: 'eq'

class RDBNe extends RDBCompareOp
    cop: 'ne'

class RDBLt extends RDBCompareOp
    cop: 'lt'

class RDBLe extends RDBCompareOp
    cop: 'le'

class RDBGt extends RDBCompareOp
    cop: 'gt'

class RDBGe extends RDBCompareOp
    cop: 'ge'

class RDBArithmeticOp extends RDBOp
    type: tp "NUMBER... -> NUMBER"
    op: (args) ->
        i = 1
        acc = args[0]
        while i < args.length
            acc = acc[@aop](args[i])
            i++
        return acc

class RDBAdd extends RDBArithmeticOp
    type: tp "NUMBER... -> NUMBER | STRING... -> STRING"
    aop: "add"

class RDBSub extends RDBArithmeticOp
    aop: "sub"

class RDBMul extends RDBArithmeticOp
    aop: "mul"

class RDBDiv extends RDBArithmeticOp
    aop: "div"

class RDBMod extends RDBArithmeticOp
    type: tp "NUMBER, NUMBER -> NUMBER"
    aop: "mod"

class RDBAppend extends RDBOp
    type: tp "ARRAY, DATUM -> ARRAY"
    op: (args) -> args[0].append args[1]

class RDBSlice extends RDBOp
    type: tp "Sequence, NUMBER, NUMBER -> Sequence"
    op: (args, optargs) ->
        args[0].slice args[1], args[2]

class RDBSkip extends RDBOp
    type: tp "Sequence, NUMBER -> Sequence"
    op: (args) -> args[0].slice args[1], new RDBPrimitive -1

class RDBLimit extends RDBOp
    type: tp "Sequence, NUMBER -> Sequence"
    op: (args) -> args[0].limit args[1]

class RDBGetAttr extends RDBOp
    type: tp "OBJECT, STRING -> DATUM"
    op: (args) -> args[0].get args[1]

class RDBContains extends RDBOp
    type: tp "OBJECT, STRING... -> BOOL"
    op: (args) -> args[0].contains args[1..]...

class RDBPluck extends RDBOp
    type: tp "Sequence, STRING... -> Sequence | OBJECT, STRING... -> OBJECT"
    op: (args) -> args[0].pluck args[1..]...

class RDBWithout extends RDBOp
    type: tp "Sequence, STRING... -> Sequence | OBJECT, STRING... -> OBJECT"
    op: (args) -> args[0].without args[1..]...

class RDBMerge extends RDBOp
    type: tp "OBJECT... -> OBJECT"
    op: (args) -> args[0].merge args[1..]...

class RDBBetween extends RDBOp
    type: tp "Sequence, {left_bound:DATUM; right_bound:DATUM} -> Sequence"
    op: (args, optargs) -> args[0].between optargs['left_bound'], optargs['right_bound']

class RDBReduce extends RDBOp
    type: tp "Sequence, Function(2), {base:DATUM} -> DATUM"
    op: (args, optargs) -> args[0].reduce args[1](1), optargs['base']

class RDBMap extends RDBOp
    type: tp "Sequence, Function(1) -> Sequence"
    op: (args, optargs, context) ->
        args[0].map context.bindIvar args[1](1)

class RDBFilter extends RDBOp
    type: tp "Sequence, Function(1) -> Sequence"
    op: (args, optargs, context) ->
        args[0].filter context.bindIvar args[1](1)

class RDBConcatMap extends RDBOp
    type: tp "Sequence, Function(1) -> Sequence"
    op: (args, optargs, context) ->
        args[0].concatMap context.bindIvar args[1](1)

class RDBOrderBy extends RDBOp
    type: tp "Sequence, !STRING... -> Sequence"
    op: (args) -> args[0].orderBy new RDBArray args[1..]

class RDBDistinct extends RDBOp
    type: tp "Sequence -> Sequence"
    op: (args) -> args[0].distinct()

class RDBCount extends RDBOp
    type: tp "Sequence -> NUMBER"
    op: (args) -> args[0].count()

class RDBUnion extends RDBOp
    type: tp "Sequence... -> Sequence"
    op: (args) -> args[0].union args[1..]...

class RDBNth extends RDBOp
    type: tp "Sequence, NUMBER, -> DATUM"
    op: (args) -> args[0].nth args[1]

class RDBGroupedMapReduce extends RDBOp
    type: tp "Sequence, Function(1), Function(1), Function(2) -> Sequence"
    op: (args) -> args[0].groupedMapReduce args[1](1), args[2](2), args[3](3)

class RDBGroupBy extends RDBOp
    type: tp "Sequence, ARRAY, !OBJECT -> Sequence"
    op: (args) -> args[0].groupBy args[1], args[2]

class RDBInnerJoin extends RDBOp
    type: tp "Sequence, Sequence, Function(2) -> Sequence"
    op: (args) -> args[0].innerJoin args[1], args[2](2)

class RDBOuterJoin extends RDBOp
    type: tp "Sequence, Sequence, Function(2) -> Sequence"
    op: (args) -> args[0].outerJoin args[1], args[2](2)

class RDBEqJoin extends RDBOp
    type: tp "Sequence, !STRING, Sequence -> Sequence"
    op: (args, optargs) -> args[0].eqJoin args[1], args[2]

class RDBZip extends RDBOp
    type: tp "Sequence -> Sequence"
    op: (args) -> args[0].zip()

class RDBCoerce extends RDBOp
    type: tp "Top, STRING -> Top"
    op: new RqlRuntimeError "Not implemented"

class RDBTypeOf extends RDBOp
    type: tp "Top -> STRING"
    op: new RqlRuntimeError "Not implemented"

class RDBUpdate extends RDBOp
    type: tp "StreamSelection, Function(1), {non_atomic_ok:BOOL} -> OBJECT | SingleSelection, Function(1), {non_atomic_ok:BOOL} -> OBJECT"
    op: (args) -> args[0].update args[1](1)

class RDBDelete extends RDBOp
    type: tp "StreamSelection -> OBJECT | SingleSelection -> OBJECT"
    op: (args) -> args[0].del()

class RDBReplace extends RDBOp
    type: tp "StreamSelection, Function(1), {non_atomic_ok:BOOL} -> OBJECT | SingleSelection, Function(1), {non_atomic_ok:BOOL} -> OBJECT"
    op: (args) -> args[0].replace args[1](1)

class RDBInsert extends RDBWriteOp
    type: tp "Table, OBJECT, {upsert:BOOL} -> OBJECT | Table, Sequence, {upsert:BOOL} -> OBJECT"
    op: (args) -> args[0].insert args[1]

class RDBDbCreate extends RDBWriteOp
    type: tp "STRING -> OBJECT"
    op: (args, optargs, context) ->
        context.universe.createDatabase args[0]

class RDBDbDrop extends RDBWriteOp
    type: tp "STRING -> OBJECT"
    op: (args, optargs, context) ->
        context.universe.dropDatabase args[0]

class RDBDbList extends RDBOp
    type: tp "-> ARRAY"
    op: (args, optargs, context) -> context.universe.listDatabases()

class RDBTableCreate extends RDBWriteOp
    type: tp "Database, STRING, {datacenter:STRING; primary_key:STRING; cache_size:NUMBER} -> OBJECT"
    op: (args) -> args[0].createTable args[1]

class RDBTableDrop extends RDBWriteOp
    type: tp "Database, STRING -> OBJECT"
    op: (args) -> args[0].dropTable args[1]

class RDBTableList extends RDBOp
    type: tp "Database -> ARRAY"
    op: (args) -> args[0].listTables()

class RDBFuncall extends RDBOp
    type: tp "Function, DATUM... -> DATUM"
    op: (args) -> args[0](0)(args[1..]...)

class RDBBranch extends RDBOp
    type: tp "BOOL, Top, Top -> Top"
    op: (args) -> if args[0].asJSON() then args[1] else args[2]

class RDBAny
    constructor: (args) ->
        @args = args

    type: tp "BOOL... -> BOOL"

    eval: (context) ->
        for arg in @args
            if arg.eval(context).asJSON()
                return new RDBPrimitive true
        return new RDBPrimitive false

class RDBAll
    constructor: (args) ->
        @args = args

    type: tp "BOOL... -> BOOL"

    eval: (context) ->
        for arg in @args
            if not arg.eval(context).asJSON()
                return new RDBPrimitive false
        return new RDBPrimitive true

class RDBForEach extends RDBOp
    type: tp "Sequence, Function(1) -> OBJECT"
    op: (args) -> args[0].forEach args[1](1)

class RDBFunc
    constructor: (args) ->
        @args = args

    type: tp "ARRAY, Top -> ARRAY -> Top"

    eval: (context) ->
        body = @args[1]
        formals = @args[0].eval(context)
        (arg_num) ->
            (actuals...) ->
                binds = {}
                for varId,i in formals.asArray()
                    binds[varId.asJSON()] = actuals[i]

                try
                    context.pushScope(binds)
                    result = body.eval(context)
                    context.popScope()
                    return result
                catch err
                    err.backtrace.unshift 1 # for the body of this func
                    err.backtrace.unshift arg_num # for whatever called us
                    throw err
