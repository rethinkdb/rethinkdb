goog.provide("rethinkdb.ast")

goog.require("rethinkdb.base")
goog.require("rethinkdb.errors")
goog.require("Term")
goog.require("Datum")

class TermBase
    constructor: ->
        self = ((field) -> self.getAttr(field))
        self.__proto__ = @.__proto__
        return self

    run: (connOrOptions, cb) ->
        useOutdated = undefined

        # Parse out run options from connOrOptions object
        if connOrOptions? and typeof(connOrOptions) is 'object' and not (connOrOptions instanceof Connection)
            useOutdated = !!connOrOptions.useOutdated
            noreply = !!connOrOptions.noreply
            for own key of connOrOptions
                unless key in ['connection', 'useOutdated', 'noreply']
                    throw new RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool>, noreply: <bool>}."
            conn = connOrOptions.connection
        else
            useOutdated = null
            noreply = null
            conn = connOrOptions

        # This only checks that the argument is of the right type, connection
        # closed errors will be handled elsewhere
        unless conn instanceof Connection
            throw new RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool>, noreply: <bool> }."

        # We only require a callback if noreply isn't set
        if not noreply and typeof(cb) isnt 'function'
            throw new RqlDriverError "Second argument to `run` must be a callback to invoke "+
                                     "with either an error or the result of the query."

        try
            conn._start @, cb, useOutdated, noreply
        catch e
            # It was decided that, if we can, we prefer to invoke the callback
            # with any errors rather than throw them as normal exceptions.
            # Thus we catch errors here and invoke the callback instead of
            # letting the error bubble up.
            if typeof(cb) is 'function'
                cb(e)
            else
                throw e

    toString: -> RqlQueryPrinter::printQuery(@)

class RDBVal extends TermBase
    eq: varar(1, null, (others...) -> new Eq {}, @, others...)
    ne: varar(1, null, (others...) -> new Ne {}, @, others...)
    lt: varar(1, null, (others...) -> new Lt {}, @, others...)
    le: varar(1, null, (others...) -> new Le {}, @, others...)
    gt: varar(1, null, (others...) -> new Gt {}, @, others...)
    ge: varar(1, null, (others...) -> new Ge {}, @, others...)

    not: ar () -> new Not {}, @

    add: varar(1, null, (others...) -> new Add {}, @, others...)
    sub: varar(1, null, (others...) -> new Sub {}, @, others...)
    mul: varar(1, null, (others...) -> new Mul {}, @, others...)
    div: varar(1, null, (others...) -> new Div {}, @, others...)
    mod: ar (other) -> new Mod {}, @, other

    append: ar (val) -> new Append {}, @, val
    prepend: ar (val) -> new Prepend {}, @, val
    slice: ar (left, right) -> new Slice {}, @, left, right
    skip: ar (index) -> new Skip {}, @, index
    limit: ar (index) -> new Limit {}, @, index
    getAttr: ar (field) -> new GetAttr {}, @, field
    contains: varar(1, null, (fields...) -> new Contains {}, @, fields...)
    insertAt: ar (index, value) -> new InsertAt {}, @, index, value
    spliceAt: ar (index, value) -> new SpliceAt {}, @, index, value
    deleteAt: varar(1, 2, (others...) -> new DeleteAt {}, @, others...)
    changeAt: ar (index, value) -> new ChangeAt {}, @, index, value
    indexesOf: ar (which) -> new IndexesOf {}, @, funcWrap(which)
    hasFields: varar(1, null, (fields...) -> new HasFields {}, @, fields...)
    keys: ar(-> new Keys {}, @)

    # pluck and without on zero fields are allowed
    pluck: (fields...) -> new Pluck {}, @, fields...
    without: (fields...) -> new Without {}, @, fields...

    merge: ar (other) -> new Merge {}, @, other
    between: aropt (left, right, opts) -> new Between opts, @, left, right
    reduce: aropt (func, base) -> new Reduce {base:base}, @, funcWrap(func)
    map: ar (func) -> new Map {}, @, funcWrap(func)
    filter: aropt (predicate, opts) -> new Filter opts, @, funcWrap(predicate)
    concatMap: ar (func) -> new ConcatMap {}, @, funcWrap(func)
    orderBy: varar(1, null, (fields...) -> new OrderBy {}, @, fields...)
    distinct: ar () -> new Distinct {}, @
    count: varar(0, 1, (fun...) -> new Count {}, @, fun...)
    union: varar(1, null, (others...) -> new Union {}, @, others...)
    nth: ar (index) -> new Nth {}, @, index
    isEmpty: ar () -> new IsEmpty {}, @
    groupedMapReduce: aropt (group, map, reduce, base) -> new GroupedMapReduce {base:base}, @, funcWrap(group), funcWrap(map), funcWrap(reduce)
    innerJoin: ar (other, predicate) -> new InnerJoin {}, @, other, predicate
    outerJoin: ar (other, predicate) -> new OuterJoin {}, @, other, predicate
    eqJoin: aropt (left_attr, right, opts) -> new EqJoin opts, @, left_attr, right
    zip: ar () -> new Zip {}, @
    coerceTo: ar (type) -> new CoerceTo {}, @, type
    typeOf: ar () -> new TypeOf {}, @
    update: aropt (func, opts) -> new Update opts, @, funcWrap(func)
    delete: aropt (opts) -> new Delete opts, @
    replace: aropt (func, opts) -> new Replace opts, @, funcWrap(func)
    do: ar (func) -> new FunCall {}, funcWrap(func), @
    default: ar (x) -> new Default {}, @, x

    or: varar(1, null, (others...) -> new Any {}, @, others...)
    and: varar(1, null, (others...) -> new All {}, @, others...)

    forEach: ar (func) -> new ForEach {}, @, funcWrap(func)

    groupBy: (attrs..., collector) ->
        unless collector? and attrs.length >= 1
            numArgs = attrs.length + (if collector? then 1 else 0)
            throw new RqlDriverError "Expected 2 or more argument(s) but found #{numArgs}."
        new GroupBy {}, @, attrs, collector

    info: ar () -> new Info {}, @
    sample: ar (count) -> new Sample {}, @, count

class DatumTerm extends RDBVal
    args: []
    optargs: {}

    constructor: (val) ->
        self = super()
        self.data = val
        return self

    compose: ->
        switch typeof @data
            when 'string'
                '"'+@data+'"'
            else
                ''+@data

    build: ->
        datum = new Datum
        if @data is null
            datum.setType Datum.DatumType.R_NULL
        else
            switch typeof @data
                when 'number'
                    datum.setType Datum.DatumType.R_NUM
                    datum.setRNum @data
                when 'boolean'
                    datum.setType Datum.DatumType.R_BOOL
                    datum.setRBool @data
                when 'string'
                    datum.setType Datum.DatumType.R_STR
                    datum.setRStr @data
                else
                    throw new RqlDriverError "Cannot convert `#{@data}` to Datum."
        term = new Term
        term.setType Term.TermType.DATUM
        term.setDatum datum
        return term

    deconstruct: (datum) ->
        switch datum.getType()
            when Datum.DatumType.R_NULL
                null
            when Datum.DatumType.R_BOOL
                datum.getRBool()
            when Datum.DatumType.R_NUM
                datum.getRNum()
            when Datum.DatumType.R_STR
                datum.getRStr()
            when Datum.DatumType.R_ARRAY
                DatumTerm::deconstruct dt for dt in datum.rArrayArray()
            when Datum.DatumType.R_OBJECT
                obj = {}
                for pair in datum.rObjectArray()
                    obj[pair.getKey()] = DatumTerm::deconstruct pair.getVal()
                obj

translateOptargs = (optargs) ->
    result = {}
    for own key,val of optargs
        # We translate known two word opt-args to camel case for your convience
        key = switch key
            when 'primaryKey' then 'primary_key'
            when 'useOutdated' then 'use_outdated'
            when 'nonAtomic' then 'non_atomic'
            when 'cacheSize' then 'cache_size'
            else key

        if key is undefined or val is undefined then continue
        result[key] = rethinkdb.expr val
    return result

class RDBOp extends RDBVal
    constructor: (optargs, args...) ->
        self = super()
        self.args =
            for arg,i in args
                if arg isnt undefined
                    rethinkdb.expr arg
                else
                    throw new RqlDriverError "Argument #{i} to #{@st || @mt} may not be `undefined`."
        self.optargs = translateOptargs(optargs)
        return self

    build: ->
        term = new Term
        term.setType @tt
        for arg in @args
            term.addArgs arg.build()
        for own key,val of @optargs
            pair = new Term.AssocPair
            pair.setKey key
            pair.setVal val.build()
            term.addOptargs pair
        return term

    compose: (args, optargs) ->
        if @st
            return ['r.', @st, '(', intspallargs(args, optargs), ')']
        else
            if shouldWrap(@args[0])
                args[0] = ['r(', args[0], ')']
            return [args[0], '.', @mt, '(', intspallargs(args[1..], optargs), ')']

intsp = (seq) ->
    unless seq[0]? then return []
    res = [seq[0]]
    for e in seq[1..]
        res.push(', ', e)
    return res

kved = (optargs) ->
    ['{', intsp([k, ': ', v] for own k,v of optargs), '}']

intspallargs = (args, optargs) ->
    argrepr = []
    if args.length > 0
        argrepr.push(intsp(args))
    if Object.keys(optargs).length > 0
        if argrepr.length > 0
            argrepr.push(', ')
        argrepr.push(kved(optargs))
    return argrepr

shouldWrap = (arg) ->
    arg instanceof DatumTerm or arg instanceof MakeArray or arg instanceof MakeObject

class MakeArray extends RDBOp
    tt: Term.TermType.MAKE_ARRAY
    st: '[...]' # This is only used by the `undefined` argument checker

    compose: (args) -> ['[', intsp(args), ']']

class MakeObject extends RDBOp
    tt: Term.TermType.MAKE_OBJ
    st: '{...}' # This is only used by the `undefined` argument checker

    constructor: (obj) ->
        self = super({})
        self.optargs = {}
        for own key,val of obj
            if typeof val is 'undefined'
                throw new RqlDriverError "Object field '#{key}' may not be undefined"
            self.optargs[key] = rethinkdb.expr val
        return self

    compose: (args, optargs) -> kved(optargs)

class Var extends RDBOp
    tt: Term.TermType.VAR
    compose: (args) -> ['var_'+args[0]]

class JavaScript extends RDBOp
    tt: Term.TermType.JAVASCRIPT
    st: 'js'

class UserError extends RDBOp
    tt: Term.TermType.ERROR
    st: 'error'

class ImplicitVar extends RDBOp
    tt: Term.TermType.IMPLICIT_VAR
    compose: -> ['r.row']

class Db extends RDBOp
    tt: Term.TermType.DB
    st: 'db'

    tableCreate: aropt (tblName, opts) -> new TableCreate opts, @, tblName
    tableDrop: ar (tblName) -> new TableDrop {}, @, tblName
    tableList: ar(-> new TableList {}, @)

    table: aropt (tblName, opts) -> new Table opts, @, tblName

class Table extends RDBOp
    tt: Term.TermType.TABLE
    st: 'table'

    get: ar (key) -> new Get {}, @, key
    getAll: aropt (key, opts) -> new GetAll opts, @, key
    insert: aropt (doc, opts) -> new Insert opts, @, doc
    indexCreate: varar(1, 2, (name, defun) ->
        if defun?
            new IndexCreate {}, @, name, funcWrap(defun)
        else
            new IndexCreate {}, @, name
        )
    indexDrop: ar (name) -> new IndexDrop {}, @, name
    indexList: ar () -> new IndexList {}, @

    compose: (args, optargs) ->
        if @args[0] instanceof Db
            [args[0], '.table(', args[1], ')']
        else
            ['r.table(', args[0], ')']

class Get extends RDBOp
    tt: Term.TermType.GET
    mt: 'get'

class GetAll extends RDBOp
    tt: Term.TermType.GET_ALL
    mt: 'getAll'

class Eq extends RDBOp
    tt: Term.TermType.EQ
    mt: 'eq'

class Ne extends RDBOp
    tt: Term.TermType.NE
    mt: 'ne'

class Lt extends RDBOp
    tt: Term.TermType.LT
    mt: 'lt'

class Le extends RDBOp
    tt: Term.TermType.LE
    mt: 'le'

class Gt extends RDBOp
    tt: Term.TermType.GT
    mt: 'gt'

class Ge extends RDBOp
    tt: Term.TermType.GE
    mt: 'ge'

class Not extends RDBOp
    tt: Term.TermType.NOT
    mt: 'not'

class Add extends RDBOp
    tt: Term.TermType.ADD
    mt: 'add'

class Sub extends RDBOp
    tt: Term.TermType.SUB
    mt: 'sub'

class Mul extends RDBOp
    tt: Term.TermType.MUL
    mt: 'mul'

class Div extends RDBOp
    tt: Term.TermType.DIV
    mt: 'div'

class Mod extends RDBOp
    tt: Term.TermType.MOD
    mt: 'mod'

class Append extends RDBOp
    tt: Term.TermType.APPEND
    mt: 'append'

class Prepend extends RDBOp
    tt: Term.TermType.PREPEND
    mt: 'prepend'

class Slice extends RDBOp
    tt: Term.TermType.SLICE
    st: 'slice'

class Skip extends RDBOp
    tt: Term.TermType.SKIP
    mt: 'skip'

class Limit extends RDBOp
    tt: Term.TermType.LIMIT
    st: 'limit'

class GetAttr extends RDBOp
    tt: Term.TermType.GETATTR
    st: '(...)' # This is only used by the `undefined` argument checker

    compose: (args) -> [args[0], '(', args[1], ')']

class Contains extends RDBOp
    tt: Term.TermType.CONTAINS
    mt: 'contains'

class InsertAt extends RDBOp
    tt: Term.TermType.INSERT_AT
    mt: 'insert_at'

class SpliceAt extends RDBOp
    tt: Term.TermType.SPLICE_AT
    mt: 'splice_at'

class DeleteAt extends RDBOp
    tt: Term.TermType.DELETE_AT
    mt: 'delete_at'

class ChangeAt extends RDBOp
    tt: Term.TermType.CHANGE_AT
    mt: 'change_at'

class Contains extends RDBOp
    tt: Term.TermType.CONTAINS
    mt: 'contains'

class HasFields extends RDBOp
    tt: Term.TermType.HAS_FIELDS
    mt: 'contains'

class Keys extends RDBOp
    tt: Term.TermType.KEYS
    mt: 'contains'

class Pluck extends RDBOp
    tt: Term.TermType.PLUCK
    mt: 'pluck'

class IndexesOf extends RDBOp
    tt: Term.TermType.INDEXES_OF
    mt: 'indexesOf'

class Without extends RDBOp
    tt: Term.TermType.WITHOUT
    mt: 'without'

class Merge extends RDBOp
    tt: Term.TermType.MERGE
    mt: 'merge'

class Between extends RDBOp
    tt: Term.TermType.BETWEEN
    mt: 'between'

class Reduce extends RDBOp
    tt: Term.TermType.REDUCE
    mt: 'reduce'

class Map extends RDBOp
    tt: Term.TermType.MAP
    mt: 'map'

class Filter extends RDBOp
    tt: Term.TermType.FILTER
    mt: 'filter'

class ConcatMap extends RDBOp
    tt: Term.TermType.CONCATMAP
    mt: 'concatMap'

class OrderBy extends RDBOp
    tt: Term.TermType.ORDERBY
    mt: 'orderBy'

class Distinct extends RDBOp
    tt: Term.TermType.DISTINCT
    mt: 'distinct'

class Count extends RDBOp
    tt: Term.TermType.COUNT
    mt: 'count'

class Union extends RDBOp
    tt: Term.TermType.UNION
    mt: 'union'

class Nth extends RDBOp
    tt: Term.TermType.NTH
    mt: 'nth'

class IsEmpty extends RDBOp
    tt: Term.TermType.IS_EMPTY
    mt: 'is_empty'

class GroupedMapReduce extends RDBOp
    tt: Term.TermType.GROUPED_MAP_REDUCE
    mt: 'groupedMapReduce'

class GroupBy extends RDBOp
    tt: Term.TermType.GROUPBY
    mt: 'groupBy'

class GroupBy extends RDBOp
    tt: Term.TermType.GROUPBY
    mt: 'groupBy'

class InnerJoin extends RDBOp
    tt: Term.TermType.INNER_JOIN
    mt: 'innerJoin'

class OuterJoin extends RDBOp
    tt: Term.TermType.OUTER_JOIN
    mt: 'outerJoin'

class EqJoin extends RDBOp
    tt: Term.TermType.EQ_JOIN
    mt: 'eqJoin'

class Zip extends RDBOp
    tt: Term.TermType.ZIP
    mt: 'zip'

class CoerceTo extends RDBOp
    tt: Term.TermType.COERCE_TO
    mt: 'coerceTo'

class TypeOf extends RDBOp
    tt: Term.TermType.TYPEOF
    mt: 'typeOf'

class Info extends RDBOp
    tt: Term.TermType.INFO
    mt: 'info'

class Sample extends RDBOp
    tt: Term.TermType.SAMPLE
    mt: 'sample'

class Update extends RDBOp
    tt: Term.TermType.UPDATE
    mt: 'update'

class Delete extends RDBOp
    tt: Term.TermType.DELETE
    mt: 'delete'

class Replace extends RDBOp
    tt: Term.TermType.REPLACE
    mt: 'replace'

class Insert extends RDBOp
    tt: Term.TermType.INSERT
    mt: 'insert'

class DbCreate extends RDBOp
    tt: Term.TermType.DB_CREATE
    st: 'dbCreate'

class DbDrop extends RDBOp
    tt: Term.TermType.DB_DROP
    st: 'dbDrop'

class DbList extends RDBOp
    tt: Term.TermType.DB_LIST
    st: 'dbList'

class TableCreate extends RDBOp
    tt: Term.TermType.TABLE_CREATE
    mt: 'tableCreate'

class TableDrop extends RDBOp
    tt: Term.TermType.TABLE_DROP
    mt: 'tableDrop'

class TableList extends RDBOp
    tt: Term.TermType.TABLE_LIST
    mt: 'tableList'

class IndexCreate extends RDBOp
    tt: Term.TermType.INDEX_CREATE
    mt: 'indexCreate'

class IndexDrop extends RDBOp
    tt: Term.TermType.INDEX_DROP
    mt: 'indexDrop'

class IndexList extends RDBOp
    tt: Term.TermType.INDEX_LIST
    mt: 'indexList'

class FunCall extends RDBOp
    tt: Term.TermType.FUNCALL
    st: 'do' # This is only used by the `undefined` argument checker

    compose: (args) ->
        if args.length > 2
            ['r.do(', intsp(args[1..]), ', ', args[0], ')']
        else
            if shouldWrap(@args[1])
                args[1] = ['r(', args[1], ')']
            [args[1], '.do(', args[0], ')']

class Default extends RDBOp
    tt: Term.TermType.DEFAULT
    mt: 'default'

class Branch extends RDBOp
    tt: Term.TermType.BRANCH
    st: 'branch'

class Any extends RDBOp
    tt: Term.TermType.ANY
    mt: 'or'

class All extends RDBOp
    tt: Term.TermType.ALL
    mt: 'and'

class ForEach extends RDBOp
    tt: Term.TermType.FOREACH
    mt: 'forEach'

funcWrap = (val) ->
    if val is undefined
        # Pass through the undefined value so it's caught by
        # the appropriate undefined checker
        return val

    val = rethinkdb.expr(val)

    ivarScan = (node) ->
        unless node instanceof TermBase then return false
        if node instanceof ImplicitVar then return true
        if (node.args.map ivarScan).some((a)->a) then return true
        if (v for own k,v of node.optargs).map(ivarScan).some((a)->a) then return true
        return false

    if ivarScan(val)
        return new Func {}, (x) -> val

    return val


class Func extends RDBOp
    tt: Term.TermType.FUNC
    @nextVarId: 0

    constructor: (optargs, func) ->
        args = []
        argNums = []
        i = 0
        while i < func.length
            argNums.push Func.nextVarId
            args.push new Var {}, Func.nextVarId
            Func.nextVarId++
            i++

        body = func(args...)
        if body is undefined
            throw new RqlDriverError "Annonymous function returned `undefined`. Did you forget a `return`?"

        argsArr = new MakeArray({}, argNums...)
        return super(optargs, argsArr, body)

    compose: (args) ->
        ['function(', (Var::compose(arg) for arg in args[0][1...-1]), ') { return ', args[1], '; }']

class Asc extends RDBOp
    tt: Term.TermType.ASC
    st: 'asc'

class Desc extends RDBOp
    tt: Term.TermType.DESC
    st: 'desc'
