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
    slice: (left, right) -> new Slice {left_extent:left, right_extent:right}, @
    getAttr: (field) -> new GetAttr {}, @, field
    contains: (fields...) -> new Contains {}, @, fields...
    pluck: (fields...) -> new Pluck {}, @, fields...
    without: (fields...) -> new Without {}, @, fields...
    merge: (other) -> new Merge {}, @, other
    between: (left, right) -> new Between {left_bound:left, right_bound:right}, @
    reduce: (base, func) -> new Reduce {}, @, base, func
    map: (func) -> new Map {}, @, func
    filter: (predicate) -> new Filter {}, @, predicate
    concatMap: (func) -> new ConcatMap {}, @, func
    orderBy: -> throw DriverError "Not implemented"
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
    constructor: (val) ->
        self = super()
        self.data = val
        return self

    compose: -> return @data

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
            self.optargs[key] = rethinkdb.expr val
        return self

    build: ->
        term = new Term2
        term.setType @tt
        for arg in @args
            term.addArgs arg.build()
        for own key,val of @optargs
            if val is undefined then continue
            pair = new Term2.AssocPair
            pair.setKey key
            pair.setVal val.build()
            term.addOptargs pair
        return term

class MakeArray extends RDBOp
    tt: Term2.TermType.MAKE_ARRAY

class MakeObject extends RDBOp
    tt: Term2.TermType.MAKE_OBJ

class Var extends RDBOp
    tt: Term2.TermType.VAR

class JavaScript extends RDBOp
    tt: Term2.TermType.JAVASCRIPT

class UserError extends RDBOp
    tt: Term2.TermType.ERROR

class ImplicitVar extends RDBOp
    tt: Term2.TermType.IMPLICIT_VAR

class Db extends RDBOp
    tt: Term2.TermType.DB

    tableCreate: (tblName) -> new TableCreate {}, tblName
    tableDrop: (tblName) -> new TableDrop {}, tblName
    tableList: -> new TableList {}

class Table extends RDBOp
    tt: Term2.TermType.TABLE

    get: (key, opts) -> new Get opts, @, key
    insert: (doc, opts) -> new Insert opts, @, doc

class Get extends RDBOp
    tt: Term2.TermType.GET

class Eq extends RDBOp
    tt: Term2.TermType.EQ

class Ne extends RDBOp
    tt: Term2.TermType.NE

class Lt extends RDBOp
    tt: Term2.TermType.LT

class Le extends RDBOp
    tt: Term2.TermType.LE

class Gt extends RDBOp
    tt: Term2.TermType.GT

class Ge extends RDBOp
    tt: Term2.TermType.GE

class Not extends RDBOp
    tt: Term2.TermType.NOT

class Add extends RDBOp
    tt: Term2.TermType.ADD

class Sub extends RDBOp
    tt: Term2.TermType.SUB

class Mul extends RDBOp
    tt: Term2.TermType.MUL

class Div extends RDBOp
    tt: Term2.TermType.DIV

class Mod extends RDBOp
    tt: Term2.TermType.MOD

class Append extends RDBOp
    tt: Term2.TermType.APPEND

class Slice extends RDBOp
    tt: Term2.TermType.SLICE

class GetAttr extends RDBOp
    tt: Term2.TermType.GETATTR

class Contains extends RDBOp
    tt: Term2.TermType.CONTAINS

class Pluck extends RDBOp
    tt: Term2.TermType.PLUCK

class Without extends RDBOp
    tt: Term2.TermType.WITHOUT

class Merge extends RDBOp
    tt: Term2.TermType.MERGE

class Between extends RDBOp
    tt: Term2.TermType.BETWEEN

class Reduce extends RDBOp
    tt: Term2.TermType.REDUCE

class Map extends RDBOp
    tt: Term2.TermType.MAP

class Filter extends RDBOp
    tt: Term2.TermType.FILTER

class ConcatMap extends RDBOp
    tt: Term2.TermType.CONCATMAP

class OrderBy extends RDBOp
    tt: Term2.TermType.ORDERBY

class Distinct extends RDBOp
    tt: Term2.TermType.DISTINCT

class Count extends RDBOp
    tt: Term2.TermType.COUNT

class Union extends RDBOp
    tt: Term2.TermType.UNION

class Nth extends RDBOp
    tt: Term2.TermType.NTH

class GroupedMapReduce extends RDBOp
    tt: Term2.TermType.GROUPED_MAP_REDUCE

class GroupBy extends RDBOp
    tt: Term2.TermType.GROUPBY

class InnerJoin extends RDBOp
    tt: Term2.TermType.INNER_JOIN

class OuterJoin extends RDBOp
    tt: Term2.TermType.OUTER_JOIN

class EqJoin extends RDBOp
    tt: Term2.TermType.EQ_JOIN

class Coerce extends RDBOp
    tt: Term2.TermType.COERCE

class TypeOf extends RDBOp
    tt: Term2.TermType.TYPEOF

class Update extends RDBOp
    tt: Term2.TermType.UPDATE

class Delete extends RDBOp
    tt: Term2.TermType.DELETE

class Replace extends RDBOp
    tt: Term2.TermType.REPLACE

class Insert extends RDBOp
    tt: Term2.TermType.INSERT

class DbCreate extends RDBOp
    tt: Term2.TermType.DB_CREATE

class DbDrop extends RDBOp
    tt: Term2.TermType.DB_DROP

class DbList extends RDBOp
    tt: Term2.TermType.DB_LIST

class TableCreate extends RDBOp
    tt: Term2.TermType.TABLE_CREATE

class TableDrop extends RDBOp
    tt: Term2.TermType.TABLE_DROP

class TableList extends RDBOp
    tt: Term2.TermType.TABLE_LIST

class FunCall extends RDBOp
    tt: Term2.TermType.FUNCALL

class Branch extends RDBOp
    tt: Term2.TermType.BRANCH

class Any extends RDBOp
    tt: Term2.TermType.ANY

class All extends RDBOp
    tt: Term2.TermType.ALL

class ForEach extends RDBOp
    tt: Term2.TermType.FOREACH

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
