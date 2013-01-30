goog.provide('rethinkdb.AST')

print = console.log

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

    eval: (context) ->
        # Eval arguments and check types
        args = []
        for n,i in @args
            try
                v = n.eval(context)
                # Ignore type checking for now
                args.push v
            catch err
                console.log err
                err.backtrace.unshift i
                throw err

        optargs = {}
        for k,n of @optargs
            try
                v = n.eval(context)
                # Ignore type checking for now
                optargs[k] = v
            catch err
                console.log err
                err.backtrace.unshift k
                throw err

        # Eval this node
        @op(args, optargs, context)

class RDBWriteOp extends RDBOp
    eval: (context) ->
        res = super(context)
        context.universe.save()
        return res

class MakeArray extends RDBOp
    type: "DATUM... -> ARRAY"
    op: (args) -> new RDBArray args

class MakeObj extends RDBOp
    type: "{...} -> OBJECT"
    op: (args, optargs) ->
        new RDBObject optargs

class Var extends RDBOp
    type: "!NUMBER -> DATUM"
    op: (args, optargs, context) ->
        context.lookupVar args[0].asJSON()

class JavaScript extends RDBOp
    type: "STRING -> DATUM"
    op: (args) -> new RDBPrimitive eval args[0].asJSON()

class UserError extends RDBOp
    op: (args) -> throw new RuntimeError args[0].asJSON()

class ImplicitVar extends RDBOp
    type: "-> DATUM"
    op: (args, optargs, context) -> context.getImplicitVar()

class DBRef extends RDBOp
    type: "STRING -> Database"
    op: (args, optargs, context) -> context.universe.getDatabase args[0]

class TableRef extends RDBOp
    type: "Database, STRING, {use_outdated:BOOL} -> TABLE"
    op: (args, optargs) -> args[0].getTable args[1]

class GetByKey extends RDBOp
    type: "Table, STRING -> SingleSelection"
    op: (args) -> args[0].get args[1]

class Not extends RDBOp
    type: "BOOL -> BOOL"
    op: (args) -> new RDBPrimitive not args[0].asJSON()

class CompareOp extends RDBOp
    type: "DATUM... -> BOOL"
    cop: "Abstract class variable"
    op: (args) ->
        i = 1
        while i < args.length
            if not args[i-1][@cop](args[i]).asJSON()
                return new RDBPrimitive false
            i++
        return new RDBPrimitive true

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

class ArithmeticOp extends RDBOp
    type: "NUMBER... -> NUMBER"
    op: (args) ->
        i = 1
        acc = args[0]
        while i < args.length
            acc = acc[@aop](args[i])
            i++
        return acc

class Add extends ArithmeticOp
    type: "| NUMBER... -> NUMBER | STRING... -> STRING"
    aop: "add"

class Sub extends ArithmeticOp
    aop: "sub"

class Mul extends ArithmeticOp
    aop: "mul"

class Div extends ArithmeticOp
    aop: "div"

class Mod extends ArithmeticOp
    type: "NUMBER, NUMBER -> NUMBER"
    aop: "mod"

class Append extends RDBOp
    type: "ARRAY, DATUM -> ARRAY"
    op: (args) -> args[0].append args[1]

class Slice extends RDBOp
    type: "Sequence, NUMBER, NUMBER -> Sequence"
    op: (args, optargs) ->
        args[0].slice args[1], args[2]

class Skip extends RDBOp
    type: "Sequence, NUMBER -> Sequence"
    op: (args) -> args[0].slice args[1], new RDBPrimitive -1

class Limit extends RDBOp
    type: "Sequence, NUMBER -> Sequence"
    op: (args) -> args[0].slice new RDBPrimitive 0, args[1]

class GetAttr extends RDBOp
    type: "OBJECT, STRING -> DATUM"
    op: (args) -> args[0].get args[1]

class Contains extends RDBOp
    type: "OBJECT, STRING... -> BOOL"
    op: (args) -> args[0].contains args[1..]...

class Pluck extends RDBOp
    type: "| Sequence, STRING... -> Sequence | OBJECT, STRING... -> OBJECT"
    op: (args) -> args[0].pluck args[1..]...

class Without extends RDBOp
    type: "| Sequence, STRING... -> Sequence | OBJECT, STRING... -> OBJECT"
    op: (args) -> args[0].without args[1..]...

class Merge extends RDBOp
    type: "OBJECT... -> OBJECT"
    op: (args) -> args[0].merge args[1]...

class Between extends RDBOp
    type: "SEQUENCE {left_bound:DATUM, right_bound:DATUM} -> SEQUENCE"
    op: (args, optargs) -> args[0].between optargs['left_bound'], optargs['right_bound']

class Reduce extends RDBOp
    type: "Sequence, Function(2), {base:DATUM} -> DATUM"
    op: (args, optargs) -> args[0].reduce args[1](1), optargs['base']

class Map extends RDBOp
    type: "Sequence, Function(1) -> Sequence"
    op: (args, optargs, context) ->
        args[0].map context.bindIvar args[1](1)

class Filter extends RDBOp
    type: "Sequence, Function(1) -> Sequence"
    op: (args, optargs, context) ->
        args[0].filter context.bindIvar args[1](1)

class ConcatMap extends RDBOp
    type: "Sequence, Function(1) -> Sequence"
    op: (args, optargs, context) ->
        args[0].concatMap context.bindIvar args[1](1)

class OrderBy extends RDBOp
    type: "Sequence, !STRING... -> Sequence"
    op: (args) -> args[0].orderBy new RDBArray args[1..]

class Distinct extends RDBOp
    type: "Sequence -> Sequence"
    op: (args) -> args[0].distinct()

class Count extends RDBOp
    type: "Sequence -> NUMBER"
    op: (args) -> args[0].count()

class Union extends RDBOp
    type: "Sequence... -> Sequence"
    op: (args) -> args[0].union args[1..]...

class Nth extends RDBOp
    type: "Sequence, NUMBER, -> DATUM"
    op: (args) -> args[0].nth args[1]

class GroupedMapReduce extends RDBOp
    type: "Sequence, Function(1), Function(1) Function(2) -> Sequence"
    op: (args) -> args[0].groupedMapReduce args[1](1), args[2](2), args[3](3)

class GroupBy extends RDBOp
    type: "Sequence, ARRAY, STRING -> OBJECT"
    op: (args) -> args[0].groupBy args[1], args[2]

class InnerJoin extends RDBOp
    type: "Sequence, Sequence -> Function(2) -> Sequence"
    op: (args) -> args[0].innerJoin args[1], args[2](2)

class OuterJoin extends RDBOp
    type: "Sequence, Sequence -> Function(2) -> Sequence"
    op: (args) -> args[0].outerJoin args[1], args[2](2)

class EqJoin extends RDBOp
    type: "Sequence, !STRING, Sequence -> Sequence"
    op: (args, optargs) -> args[0].eqJoin args[1], optargs

class Coerce extends RDBOp
    type: "Top, STRING -> Top"
    op: new RuntimeError "Not implemented"

class TypeOf extends RDBOp
    type: "Top -> STRING"
    op: new RuntimeError "Not implemented"

class Update extends RDBOp
    type: "Selection, Function(1), {non_atomic_ok:BOOL} -> OBJECT"
    op: (args) -> args[0].update args[1](1)

class Delete extends RDBOp
    type: "Selection -> OBJECT"
    op: (args) -> args[0].del()

class Replace extends RDBOp
    type: "Selection, Function(1), {non_atomic_ok:BOOL} -> OBJECT"
    op: (args) -> args[0].replace args[1](1)

class Insert extends RDBWriteOp
    type: "| Table, OBJECT, {upsert:BOOL} -> OBJECT | Table, Sequence, {upsert:BOOL} -> OBJECT"
    op: (args) -> args[0].insert args[1]

class DbCreate extends RDBWriteOp
    type: "STRING -> OBJECT"
    op: (args, optargs, context) ->
        context.universe.createDatabase args[0]

class DbDrop extends RDBWriteOp
    type: "STRING -> OBJECT"
    op: (args, optargs, context) ->
        context.universe.dropDatabase args[0]

class DbList extends RDBOp
    type: "-> ARRAY"
    op: (args, optargs, context) -> context.universe.listDatabases()

class TableCreate extends RDBWriteOp
    type: "Database, STRING, {datacenter:STRING, primary_key:STRING, cache_size:NUMBER} -> OBJECT"
    op: (args) -> args[0].createTable args[1]

class TableDrop extends RDBWriteOp
    type: "Database, STRING -> OBJECT"
    op: (args) -> args[0].dropTable args[1]

class TableList extends RDBOp
    type: "Database -> ARRAY"
    op: (args) -> args[0].listTables()

class Funcall extends RDBOp
    type: "Function, DATUM... -> DATUM"
    op: (args) -> args[0](0)(args[1..]...)

class Branch extends RDBOp
    type: "BOOL, Top, Top -> Top"
    op: (args) -> if args[0].asJSON() then args[1] else args[2]

class Any
    constructor: (args) ->
        @args = args

    type: "BOOL... -> BOOL"

    eval: (context) ->
        for arg in @args
            if arg.eval(context).asJSON()
                return new RDBPrimitive true
        return new RDBPrimitive false

class All
    constructor: (args) ->
        @args = args

    type: "BOOL... -> BOOL"

    eval: (context) ->
        for arg in @args
            if not arg.eval(context).asJSON()
                return new RDBPrimitive false
        return new RDBPrimitive true

class ForEach extends RDBOp
    type: "Sequence, Function(1) -> OBJECT"
    op: (args) -> args[0].forEach args[1](1)

class Func
    constructor: (args) ->
        @args = args

    type: "ARRAY, Top -> ARRAY -> Top"

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
                    console.log err
                    err.backtrace.unshift 1 # for the body of this func
                    err.backtrace.unshift arg_num # for whatever called us
                    throw err
