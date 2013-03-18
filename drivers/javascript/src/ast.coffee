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

    run: (conn, cb) ->
        use_outdated = false
        if typeof(conn) is 'object' and not (conn instanceof Connection)
            use_outdated = !!conn.use_outdated
            for own key of conn
                unless key in ['connection', 'use_outdated']
                    throw new RqlDriverError ("Invalid option '"+key+"' in first argument to `run`.")
            conn = conn.connection
        unless conn instanceof Connection
            throw new RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, use_outdated: <bool> }."
        unless typeof(cb) is 'function'
            throw new RqlDriverError "Second argument to `run` must be a callback to invoke "+
                                     "with either an error or the result of the query."
        conn._start @, cb, use_outdated

    toString: -> RqlQueryPrinter::printQuery(@)

class RDBVal extends TermBase
    eq: (others...) -> new Eq {}, @, others...
    ne: (others...) -> new Ne {}, @, others...
    lt: (others...) -> new Lt {}, @, others...
    le: (others...) -> new Le {}, @, others...
    gt: (others...) -> new Gt {}, @, others...
    ge: (others...) -> new Ge {}, @, others...

    not: -> new Not {}, @

    add: (others...) -> new Add {}, @, others...
    sub: (others...) -> new Sub {}, @, others...
    mul: (others...) -> new Mul {}, @, others...
    div: (others...) -> new Div {}, @, others...
    mod: (other) -> new Mod {}, @, other

    append: ar('append', (val) -> new Append {}, @, val)
    slice: ar('slice', (left, right) -> new Slice {}, @, left, right)
    skip: ar('skip', (index) -> new Skip {}, @, index)
    limit: ar('limit', (index) -> new Limit {}, @, index)
    getAttr: ar('getAttr', (field) -> new GetAttr {}, @, field)
    contains: (fields...) -> new Contains {}, @, fields...
    pluck: (fields...) -> new Pluck {}, @, fields...
    without: (fields...) -> new Without {}, @, fields...
    merge: ar('merge', (other) -> new Merge {}, @, other)
    between: ar('between', (left, right) -> new Between {leftBound: (if left? then left else undefined), rightBound: (if right? then right else undefined)}, @)
    reduce: aropt('reduce', (func, base) -> new Reduce {base:base}, @, funcWrap(func))
    map: ar('map', (func) -> new Map {}, @, funcWrap(func))
    filter: ar('filter', (predicate) -> new Filter {}, @, funcWrap(predicate))
    concatMap: ar('concatMap', (func) -> new ConcatMap {}, @, funcWrap(func))
    orderBy: (fields...) -> new OrderBy {}, @, fields...
    distinct: ar('distinct', () -> new Distinct {}, @)
    count: ar('count', () -> new Count {}, @)
    union: (others...) -> new Union {}, @, others...
    nth: ar('nth', (index) -> new Nth {}, @, index)
    groupedMapReduce: aropt('groupedMapReduce', (group, map, reduce, base) -> new GroupedMapReduce {base:base}, @, funcWrap(group), funcWrap(map), funcWrap(reduce))
    innerJoin: ar('innerJoin', (other, predicate) -> new InnerJoin {}, @, other, predicate)
    outerJoin: ar('outerJoin', (other, predicate) -> new OuterJoin {}, @, other, predicate)
    eqJoin: ar('eqJoin', (left_attr, right) -> new EqJoin {}, @, left_attr, right)
    zip: ar('zip', () -> new Zip {}, @)
    coerceTo: ar('coerceTo', (type) -> new CoerceTo {}, @, type)
    typeOf: -> new TypeOf {}, @
    update: aropt('update', (func, opts) -> new Update opts, @, funcWrap(func))
    delete: ar('delete', () -> new Delete {}, @)
    replace: aropt('replace', (func, opts) -> new Replace opts, @, funcWrap(func))
    do: ar('do', (func) -> new FunCall {}, funcWrap(func), @)

    or: (others...) -> new Any {}, @, others...
    and: (others...) -> new All {}, @, others...

    forEach: ar('forEach', (func) -> new ForEach {}, @, funcWrap(func))

    groupBy: (attrs..., collector = null) ->
        arg_count = attrs.length + (collector && 1 || 0)
        if collector == null
            throw new RqlDriverError "Expected at least 2 argument(s) but found #{arg_count}."
        new GroupBy {}, @, attrs, collector

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
                    throw new RqlDriverError "Unknown datum value `#{@data}`, did you forget a `return`?"
        term = new Term
        term.setType Term.TermType.DATUM
        term.setDatum datum
        return term

    @deconstruct: (datum) ->
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
                DatumTerm.deconstruct dt for dt in datum.rArrayArray()
            when Datum.DatumType.R_OBJECT
                obj = {}
                for pair in datum.rObjectArray()
                    obj[pair.getKey()] = DatumTerm.deconstruct pair.getVal()
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
            when 'leftBound' then 'left_bound'
            when 'rightBound' then 'right_bound'
            else key

        if key is undefined or val is undefined then continue
        result[key] = rethinkdb.expr val
    return result

class RDBOp extends RDBVal
    constructor: (optargs, args...) ->
        self = super()
        self.args = (rethinkdb.expr arg for arg in args)
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
    compose: (args) -> ['[', intsp(args), ']']

class MakeObject extends RDBOp
    tt: Term.TermType.MAKE_OBJ

    constructor: (obj) ->
        self = super({})
        self.optargs = {}
        for own key,val of obj
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

    tableCreate: aropt('tableCreate', (tblName, opts) -> new TableCreate opts, @, tblName)
    tableDrop: ar('tableDrop', (tblName) -> new TableDrop {}, @, tblName)
    tableList: ar(-> new TableList {}, @)

    table: aropt('table', (tblName, opts) -> new Table opts, @, tblName)

class Table extends RDBOp
    tt: Term.TermType.TABLE

    get: ar('get', (key) -> new Get {}, @, key)
    insert: aropt('insert', (doc, opts) -> new Insert opts, @, doc)

    compose: (args, optargs) ->
        if @args[0] instanceof Db
            [args[0], '.table(', args[1], ')']
        else
            ['r.table(', args[0], ')']

class Get extends RDBOp
    tt: Term.TermType.GET
    mt: 'get'

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
    compose: (args) -> [args[0], '(', args[1], ')']

class Contains extends RDBOp
    tt: Term.TermType.CONTAINS
    mt: 'contains'

class Pluck extends RDBOp
    tt: Term.TermType.PLUCK
    mt: 'pluck'

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

class FunCall extends RDBOp
    tt: Term.TermType.FUNCALL
    compose: (args) ->
        if args.length > 2
            ['r.do(', intsp(args[1..]), ', ', args[0], ')']
        else
            if shouldWrap(@args[1])
                args[1] = ['r(', args[1], ')']
            [args[1], '.do(', args[0], ')']

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

    constructor: (optargs, func) ->
        args = []
        argNums = []
        i = 0
        while i < func.length
            argNums.push i
            args.push new Var {}, i
            i++
        body = func(args...)
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
