goog.provide("rethinkdb.ast")

goog.require("rethinkdb.base")
goog.require("rethinkdb.errors")
goog.require("Term2")
goog.require("Datum")

class TermBase
    constructor: ->
        self = ((field) -> self.getAttr(field))
        self.__proto__ = @.__proto__
        return self

    run: (cb) ->
        conn = Connection::connStack[-2..-1][0]
        unless conn? then conn = Connection::lastConn
        unless conn? then throw DriverError "No connection in use"
        conn.run @, cb

class RDBVal extends TermBase
    eq: (other) -> new Eq {}, @, other
    ne: (other) -> new Ne {}, @, other
    lt: (other) -> new Lt {}, @, other
    le: (other) -> new Le {}, @, other
    gt: (other) -> new Gt {}, @, other
    ge: (other) -> new Ge {}, @, other

    not: -> new Not {}, @

    add: (other) -> new Add {}, @, other
    sub: (other) -> new Sub {}, @, other
    mul: (other) -> new Mul {}, @, other
    div: (other) -> new Div {}, @, other
    mod: (other) -> new Mod {}, @, other

    append: (val) -> new Append {}, @, val
    slice: (left=0, right=-1) -> new Slice {}, @, left, right
    getAttr: (field) -> new GetAttr {}, @, field
    contains: (fields...) -> new Contains {}, @, fields...
    pluck: (fields...) -> new Pluck {}, @, fields...
    without: (fields...) -> new Without {}, @, fields...
    merge: (other) -> new Merge {}, @, other
    between: (left, right) -> new Between {left_bound:left, right_bound:right}, @
    reduce: (func, base) -> new Reduce {base:base}, @, func
    map: (func) -> new Map {}, @, func
    filter: (predicate) -> new Filter {}, @, predicate
    concatMap: (func) -> new ConcatMap {}, @, func
    orderBy: (fields...) -> new OrderBy {}, fields...
    distinct: -> new Distinct {}, @
    count: -> new Count {}, @
    union: (others...) -> new Union {}, @, other...
    nth: (index) -> new Nth {}, @, index
    groupedMapReduce: (group, map, reduce) -> new GroupedMapReduce {}, @, group, map, reduce
    groupBy: -> throw DriverError "Not implemented"
    innerJoin: (other, predicate) -> new InnerJoin {}, @, other, predicate
    outerJoin: (other, predicate) -> new OuterJoin {}, @, other, predicate
    eqJoin: (other, left, right) -> new InnerJoin {left_attr:left, right_attr:right}, @, other
    coerce: (type) -> new Coerce {}, @, type
    typeOf: -> new TypeOf {}, @
    update: (func) -> new Update {}, @, func
    delete: -> new Delete {}, @
    replace: (func) -> new Replace {}, @, func
    do: (func) -> new FunCall {}, func, @
    or: (other) -> new Any {}, @, other
    and: (other) -> new All {}, @, other
    forEach: (func) -> new ForEach {}, @, func

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
                    throw new DriverError "Unknown datum value: #{@data}"
        term = new Term2
        term.setType Term2.TermType.DATUM
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

class RDBOp extends RDBVal
    constructor: (optargs, args...) ->
        self = super()
        self.args = (rethinkdb.expr arg for arg in args)
        self.optargs = {}
        for own key,val of optargs
            if val is undefined then continue
            self.optargs[key] = rethinkdb.expr val
        return self

    build: ->
        term = new Term2
        term.setType @tt
        for arg in @args
            term.addArgs arg.build()
        for own key,val of @optargs
            pair = new Term2.AssocPair
            pair.setKey key
            pair.setVal val.build()
            term.addOptargs pair
        return term

    compose: (args, optargs) ->
        if @st
            ['r.', @st, '(', intsp(args), ')']
        else
            [args[0], '.', @mt, '(', intsp(args[1..]), ')']

intsp = (seq) ->
    res = [seq[0]]
    for e in seq[1..]
        res.push(', ', e)
    return res

kved = (optargs) ->
    ['{', intsp([k, ': ', v] for k,v in optargs), '}']

class MakeArray extends RDBOp
    tt: Term2.TermType.MAKE_ARRAY
    compose: (args) -> ['[', intsp(args), ']']
        

class MakeObject extends RDBOp
    tt: Term2.TermType.MAKE_OBJ
    compose: (args, optargs) -> kved(optargs)

class Var extends RDBOp
    tt: Term2.TermType.VAR
    compose: (args) -> ['var_'+args[0]]

class JavaScript extends RDBOp
    tt: Term2.TermType.JAVASCRIPT
    st: 'js'

class UserError extends RDBOp
    tt: Term2.TermType.ERROR
    st: 'error'

class ImplicitVar extends RDBOp
    tt: Term2.TermType.IMPLICIT_VAR
    compose: -> ['r.row']

class Db extends RDBOp
    tt: Term2.TermType.DB
    st: 'db'

    tableCreate: (tblName) -> new TableCreate {}, tblName
    tableDrop: (tblName) -> new TableDrop {}, tblName
    tableList: -> new TableList {}

class Table extends RDBOp
    tt: Term2.TermType.TABLE
    mt: 'table'

    get: (key, opts) -> new Get opts, @, key
    insert: (doc, opts) -> new Insert opts, @, doc

class Get extends RDBOp
    tt: Term2.TermType.GET
    mt: 'get'

class Eq extends RDBOp
    tt: Term2.TermType.EQ
    mt: 'eq'

class Ne extends RDBOp
    tt: Term2.TermType.NE
    mt: 'ne'

class Lt extends RDBOp
    tt: Term2.TermType.LT
    mt: 'lt'

class Le extends RDBOp
    tt: Term2.TermType.LE
    mt: 'le'

class Gt extends RDBOp
    tt: Term2.TermType.GT
    mt: 'gt'

class Ge extends RDBOp
    tt: Term2.TermType.GE
    mt: 'ge'

class Not extends RDBOp
    tt: Term2.TermType.NOT
    mt: 'not'

class Add extends RDBOp
    tt: Term2.TermType.ADD
    mt: 'add'

class Sub extends RDBOp
    tt: Term2.TermType.SUB
    mt: 'sub'

class Mul extends RDBOp
    tt: Term2.TermType.MUL
    mt: 'mul'

class Div extends RDBOp
    tt: Term2.TermType.DIV
    mt: 'div'

class Mod extends RDBOp
    tt: Term2.TermType.MOD
    mt: 'mod'

class Append extends RDBOp
    tt: Term2.TermType.APPEND
    mt: 'append'

class Slice extends RDBOp
    tt: Term2.TermType.SLICE
    st: 'slice'

class GetAttr extends RDBOp
    tt: Term2.TermType.GETATTR
    compose: (args) -> [args[0], '(', args[1], ')']

class Contains extends RDBOp
    tt: Term2.TermType.CONTAINS
    mt: 'contains'

class Pluck extends RDBOp
    tt: Term2.TermType.PLUCK
    mt: 'pluck'

class Without extends RDBOp
    tt: Term2.TermType.WITHOUT
    mt: 'without'

class Merge extends RDBOp
    tt: Term2.TermType.MERGE
    mt: 'merge'

class Between extends RDBOp
    tt: Term2.TermType.BETWEEN
    mt: 'between'

class Reduce extends RDBOp
    tt: Term2.TermType.REDUCE
    mt: 'reduce'

class Map extends RDBOp
    tt: Term2.TermType.MAP
    mt: 'map'

class Filter extends RDBOp
    tt: Term2.TermType.FILTER
    mt: 'filter'

class ConcatMap extends RDBOp
    tt: Term2.TermType.CONCATMAP
    mt: 'concatMap'

class OrderBy extends RDBOp
    tt: Term2.TermType.ORDERBY
    mt: 'orderBy'

class Distinct extends RDBOp
    tt: Term2.TermType.DISTINCT
    mt: 'distinct'

class Count extends RDBOp
    tt: Term2.TermType.COUNT
    mt: 'count'

class Union extends RDBOp
    tt: Term2.TermType.UNION
    mt: 'union'

class Nth extends RDBOp
    tt: Term2.TermType.NTH
    mt: 'nth'

class GroupedMapReduce extends RDBOp
    tt: Term2.TermType.GROUPED_MAP_REDUCE
    mt: 'groupedMapReduce'

class GroupBy extends RDBOp
    tt: Term2.TermType.GROUPBY
    mt: 'groupBy'

class InnerJoin extends RDBOp
    tt: Term2.TermType.INNER_JOIN
    mt: 'innerJoin'

class OuterJoin extends RDBOp
    tt: Term2.TermType.OUTER_JOIN
    mt: 'outerJoin'

class EqJoin extends RDBOp
    tt: Term2.TermType.EQ_JOIN
    mt: 'eqJoin'

class Coerce extends RDBOp
    tt: Term2.TermType.COERCE
    mt: 'coerce'

class TypeOf extends RDBOp
    tt: Term2.TermType.TYPEOF
    mt: 'typeOf'

class Update extends RDBOp
    tt: Term2.TermType.UPDATE
    mt: 'update'

class Delete extends RDBOp
    tt: Term2.TermType.DELETE
    mt: 'delete'

class Replace extends RDBOp
    tt: Term2.TermType.REPLACE
    mt: 'replace'

class Insert extends RDBOp
    tt: Term2.TermType.INSERT
    mt: 'insert'

class DbCreate extends RDBOp
    tt: Term2.TermType.DB_CREATE
    st: 'dbCreate'

class DbDrop extends RDBOp
    tt: Term2.TermType.DB_DROP
    st: 'dbDrop'

class DbList extends RDBOp
    tt: Term2.TermType.DB_LIST
    st: 'dbList'

class TableCreate extends RDBOp
    tt: Term2.TermType.TABLE_CREATE
    mt: 'tableCreate'

class TableDrop extends RDBOp
    tt: Term2.TermType.TABLE_DROP
    mt: 'tableDrop'

class TableList extends RDBOp
    tt: Term2.TermType.TABLE_LIST
    mt: 'tableList'

class FunCall extends RDBOp
    tt: Term2.TermType.FUNCALL
    compose: (args) ->
        if args.length == 2
            [args[1], '.do(', args[0], ')']
        else
            ['r.do(', instsp(args[1..]), ', ', args[0], ')']

class Branch extends RDBOp
    tt: Term2.TermType.BRANCH
    st: 'branch'

class Any extends RDBOp
    tt: Term2.TermType.ANY
    mt: 'or'

class All extends RDBOp
    tt: Term2.TermType.ALL
    mt: 'and'

class ForEach extends RDBOp
    tt: Term2.TermType.FOREACH
    mt: 'forEach'

class Func extends RDBOp
    tt: Term2.TermType.FUNC

    constructor: (optargs, func) ->
        args = []
        argNums = []
        i = 0
        while i < func.length
            argNums.push i
            args.push new Var {}, i
            i++
        body = func.apply({}, args)
        argsArr = new MakeArray({}, argNums...)
        return super(optargs, argsArr, body)

    compose: (args) ->
        ['function(', (Var::compose(arg) for arg in args[0][1...-1]), ') { return ', args[1], '; }']
