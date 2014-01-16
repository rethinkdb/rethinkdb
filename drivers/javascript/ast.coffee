util = require('./util')
err = require('./errors')

# Import some names to this namespace for convienience
ar = util.ar
varar = util.varar
aropt = util.aropt

# rethinkdb is both the main export object for the module
# and a function that shortcuts `r.expr`.
rethinkdb = (args...) -> rethinkdb.expr(args...)

# Utilities

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

# AST classes

class TermBase
    constructor: ->
        self = (ar (field) -> self.getField(field))
        self.__proto__ = @.__proto__
        return self

    run: (connOrOptions, cb) ->
        useOutdated = undefined

        # Parse out run options from connOrOptions object
        if connOrOptions? and connOrOptions.constructor is Object
            for own key of connOrOptions
                unless key in ['connection', 'useOutdated', 'noreply', 'timeFormat', 'profile', 'durability']
                    throw new err.RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool>, noreply: <bool>, timeFormat: <string>, profile: <bool>, durability: <string>}."
            conn = connOrOptions.connection
            opts = connOrOptions
        else
            conn = connOrOptions
            opts = {}

        # This only checks that the argument is of the right type, connection
        # closed errors will be handled elsewhere
        unless conn? and conn._start?
            throw new err.RqlDriverError "First argument to `run` must be an open connection or { connection: <connection>, useOutdated: <bool>, noreply: <bool>, timeFormat: <string>, profile: <bool>, durability: <string>}."

        # We only require a callback if noreply isn't set
        if not opts.noreply and typeof(cb) isnt 'function'
            throw new err.RqlDriverError "Second argument to `run` must be a callback to invoke "+
                                         "with either an error or the result of the query."

        try
            conn._start @, cb, opts
        catch e
            # It was decided that, if we can, we prefer to invoke the callback
            # with any errors rather than throw them as normal exceptions.
            # Thus we catch errors here and invoke the callback instead of
            # letting the error bubble up.
            if typeof(cb) is 'function'
                cb(e)
            else
                throw e

    toString: -> err.printQuery(@)

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
    difference: ar (val) -> new Difference {}, @, val
    setInsert: ar (val) -> new SetInsert {}, @, val
    setUnion: ar (val) -> new SetUnion {}, @, val
    setIntersection: ar (val) -> new SetIntersection {}, @, val
    setDifference: ar (val) -> new SetDifference {}, @, val
    slice: aropt (left, right, opts) -> new Slice opts, @, left, right
    skip: ar (index) -> new Skip {}, @, index
    limit: ar (index) -> new Limit {}, @, index
    getField: ar (field) -> new GetField {}, @, field
    contains: varar(1, null, (fields...) -> new Contains {}, @, fields.map(funcWrap)...)
    insertAt: ar (index, value) -> new InsertAt {}, @, index, value
    spliceAt: ar (index, value) -> new SpliceAt {}, @, index, value
    deleteAt: varar(1, 2, (others...) -> new DeleteAt {}, @, others...)
    changeAt: ar (index, value) -> new ChangeAt {}, @, index, value
    indexesOf: ar (which) -> new IndexesOf {}, @, funcWrap(which)
    hasFields: varar(0, null, (fields...) -> new HasFields {}, @, fields...)
    withFields: varar(0, null, (fields...) -> new WithFields {}, @, fields...)
    keys: ar(-> new Keys {}, @)

    # pluck and without on zero fields are allowed
    pluck: (fields...) -> new Pluck {}, @, fields...
    without: (fields...) -> new Without {}, @, fields...

    merge: varar(1, null, (fields...) -> new Merge {}, @, fields.map(funcWrap)...)
    between: aropt (left, right, opts) -> new Between opts, @, left, right
    reduce: varar(1, 2, (func) -> new Reduce {}, @, funcWrap(func))
    map: ar (func) -> new Map {}, @, funcWrap(func)
    filter: aropt (predicate, opts) -> new Filter opts, @, funcWrap(predicate)
    concatMap: ar (func) -> new ConcatMap {}, @, funcWrap(func)
    distinct: ar () -> new Distinct {}, @
    count: varar(0, 1, (fun...) -> new Count {}, @, fun.map(funcWrap)...)
    union: varar(1, null, (others...) -> new Union {}, @, others...)
    nth: ar (index) -> new Nth {}, @, index
    match: ar (pattern) -> new Match {}, @, pattern
    upcase: ar () -> new Upcase {}, @
    downcase: ar () -> new Downcase {}, @
    isEmpty: ar () -> new IsEmpty {}, @
    groupedMapReduce: varar(3, 4, (group, map, reduce) -> new GroupedMapReduce {}, @, funcWrap(group), funcWrap(map), funcWrap(reduce))
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
            throw new err.RqlDriverError "Expected 2 or more argument(s) but found #{numArgs}."
        new GroupBy {}, @, attrs, collector

    info: ar () -> new Info {}, @
    sample: ar (count) -> new Sample {}, @, count

    orderBy: (attrsAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        attrs = attrsAndOpts

        # Look for opts dict
        perhapsOptDict = attrsAndOpts[attrsAndOpts.length - 1]
        if perhapsOptDict and
                (Object::toString.call(perhapsOptDict) is '[object Object]') and
                not (perhapsOptDict instanceof TermBase)
            opts = perhapsOptDict
            attrs = attrsAndOpts[0...(attrsAndOpts.length - 1)]

        attrs = (for attr in attrs
            if attr instanceof Asc or attr instanceof Desc
                attr
            else
                funcWrap(attr)
        )

        new OrderBy opts, @, attrs...

    # Database operations

    tableCreate: aropt (tblName, opts) -> new TableCreate opts, @, tblName
    tableDrop: ar (tblName) -> new TableDrop {}, @, tblName
    tableList: ar(-> new TableList {}, @)

    table: aropt (tblName, opts) -> new Table opts, @, tblName

    # Table operations

    get: ar (key) -> new Get {}, @, key

    getAll: (keysAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        keys = keysAndOpts

        # Look for opts dict
        perhapsOptDict = keysAndOpts[keysAndOpts.length - 1]
        if perhapsOptDict and
                ((Object::toString.call(perhapsOptDict) is '[object Object]') and not (perhapsOptDict instanceof TermBase))
            opts = perhapsOptDict
            keys = keysAndOpts[0...(keysAndOpts.length - 1)]

        new GetAll opts, @, keys...

    # For this function only use `exprJSON` rather than letting it default to regular
    # `expr`. This will attempt to serialize as much of the document as JSON as possible.
    # This behavior can be manually overridden with either direct JSON serialization
    # or ReQL datum serialization by first wrapping the argument with `r.expr` or `r.json`.
    insert: aropt (doc, opts) -> new Insert opts, @, rethinkdb.exprJSON(doc)
    indexCreate: varar(1, 3, (name, defun_or_opts, opts) ->
        if opts?
            new IndexCreate opts, @, name, funcWrap(defun_or_opts)
        else if defun_or_opts?
            if (Object::toString.call(defun_or_opts) is '[object Object]') and not (defun_or_opts instanceof Function) and not (defun_or_opts instanceof TermBase)
                new IndexCreate defun_or_opts, @, name
            else
                new IndexCreate {}, @, name, funcWrap(defun_or_opts)
        else
            new IndexCreate {}, @, name
        )
    indexDrop: ar (name) -> new IndexDrop {}, @, name
    indexList: ar () -> new IndexList {}, @
    indexStatus: varar(0, null, (others...) -> new IndexStatus {}, @, others...)
    indexWait: varar(0, null, (others...) -> new IndexWait {}, @, others...)

    sync: ar () -> new Sync {}, @

    toISO8601: ar () -> new ToISO8601 {}, @
    toEpochTime: ar () -> new ToEpochTime {}, @
    inTimezone: ar (tzstr) -> new InTimezone {}, @, tzstr
    during: aropt (t2, t3, opts) -> new During opts, @, t2, t3
    date: ar () -> new RQLDate {}, @
    timeOfDay: ar () -> new TimeOfDay {}, @
    timezone: ar () -> new Timezone {}, @

    year: ar () -> new Year {}, @
    month: ar () -> new Month {}, @
    day: ar () -> new Day {}, @
    dayOfWeek: ar () -> new DayOfWeek {}, @
    dayOfYear: ar () -> new DayOfYear {}, @
    hours: ar () -> new Hours {}, @
    minutes: ar () -> new Minutes {}, @
    seconds: ar () -> new Seconds {}, @

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
        datum = {}
        if @data is null
            datum.type = "R_NULL"
        else
            switch typeof @data
                when 'number'
                    datum.type = "R_NUM"
                    datum.r_num = @data
                when 'boolean'
                    datum.type = "R_BOOL"
                    datum.r_bool = @data
                when 'string'
                    datum.type = "R_STR"
                    datum.r_str = @data
                else
                    throw new err.RqlDriverError "Cannot convert `#{@data}` to Datum."
        term =
            type: "DATUM"
            datum: datum
        return term

translateOptargs = (optargs) ->
    result = {}
    for own key,val of optargs
        # We translate known two word opt-args to camel case for your convience
        key = switch key
            when 'primaryKey' then 'primary_key'
            when 'returnVals' then 'return_vals'
            when 'useOutdated' then 'use_outdated'
            when 'nonAtomic' then 'non_atomic'
            when 'cacheSize' then 'cache_size'
            when 'leftBound' then 'left_bound'
            when 'rightBound' then 'right_bound'
            when 'defaultTimezone' then 'default_timezone'
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
                    throw new err.RqlDriverError "Argument #{i} to #{@st || @mt} may not be `undefined`."
        self.optargs = translateOptargs(optargs)
        return self

    build: ->
        term = {args:[], optargs:[]}
        term.type = @tt
        for arg in @args
            term.args.push(arg.build())
        for own key,val of @optargs
            pair =
                key: key
                val: val.build()
            term.optargs.push(pair)
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
    tt: "MAKE_ARRAY"
    st: '[...]' # This is only used by the `undefined` argument checker

    compose: (args) -> ['[', intsp(args), ']']

class MakeObject extends RDBOp
    tt: "MAKE_OBJ"
    st: '{...}' # This is only used by the `undefined` argument checker

    constructor: (obj) ->
        self = super({})
        self.optargs = {}
        for own key,val of obj
            if typeof val is 'undefined'
                throw new err.RqlDriverError "Object field '#{key}' may not be undefined"
            self.optargs[key] = rethinkdb.expr val
        return self

    compose: (args, optargs) -> kved(optargs)

class Var extends RDBOp
    tt: "VAR"
    compose: (args) -> ['var_'+args[0]]

class JavaScript extends RDBOp
    tt: "JAVASCRIPT"
    st: 'js'

class Json extends RDBOp
    tt: "JSON"
    st: 'json'

class UserError extends RDBOp
    tt: "ERROR"
    st: 'error'

class ImplicitVar extends RDBOp
    tt: "IMPLICIT_VAR"
    compose: -> ['r.row']

class Db extends RDBOp
    tt: "DB"
    st: 'db'

class Table extends RDBOp
    tt: "TABLE"
    st: 'table'

    compose: (args, optargs) ->
        if @args[0] instanceof Db
            [args[0], '.table(', args[1], ')']
        else
            ['r.table(', args[0], ')']

class Get extends RDBOp
    tt: "GET"
    mt: 'get'

class GetAll extends RDBOp
    tt: "GET_ALL"
    mt: 'getAll'

class Eq extends RDBOp
    tt: "EQ"
    mt: 'eq'

class Ne extends RDBOp
    tt: "NE"
    mt: 'ne'

class Lt extends RDBOp
    tt: "LT"
    mt: 'lt'

class Le extends RDBOp
    tt: "LE"
    mt: 'le'

class Gt extends RDBOp
    tt: "GT"
    mt: 'gt'

class Ge extends RDBOp
    tt: "GE"
    mt: 'ge'

class Not extends RDBOp
    tt: "NOT"
    mt: 'not'

class Add extends RDBOp
    tt: "ADD"
    mt: 'add'

class Sub extends RDBOp
    tt: "SUB"
    mt: 'sub'

class Mul extends RDBOp
    tt: "MUL"
    mt: 'mul'

class Div extends RDBOp
    tt: "DIV"
    mt: 'div'

class Mod extends RDBOp
    tt: "MOD"
    mt: 'mod'

class Append extends RDBOp
    tt: "APPEND"
    mt: 'append'

class Prepend extends RDBOp
    tt: "PREPEND"
    mt: 'prepend'

class Difference extends RDBOp
    tt: "DIFFERENCE"
    mt: 'difference'

class SetInsert extends RDBOp
    tt: "SET_INSERT"
    mt: 'setInsert'

class SetUnion extends RDBOp
    tt: "SET_UNION"
    mt: 'setUnion'

class SetIntersection extends RDBOp
    tt: "SET_INTERSECTION"
    mt: 'setIntersection'

class SetDifference extends RDBOp
    tt: "SET_DIFFERENCE"
    mt: 'setDifference'

class Slice extends RDBOp
    tt: "SLICE"
    mt: 'slice'

class Skip extends RDBOp
    tt: "SKIP"
    mt: 'skip'

class Limit extends RDBOp
    tt: "LIMIT"
    mt: 'limit'

class GetField extends RDBOp
    tt: "GET_FIELD"
    st: '(...)' # This is only used by the `undefined` argument checker

    compose: (args) -> [args[0], '(', args[1], ')']

class Contains extends RDBOp
    tt: "CONTAINS"
    mt: 'contains'

class InsertAt extends RDBOp
    tt: "INSERT_AT"
    mt: 'insertAt'

class SpliceAt extends RDBOp
    tt: "SPLICE_AT"
    mt: 'spliceAt'

class DeleteAt extends RDBOp
    tt: "DELETE_AT"
    mt: 'deleteAt'

class ChangeAt extends RDBOp
    tt: "CHANGE_AT"
    mt: 'changeAt'

class Contains extends RDBOp
    tt: "CONTAINS"
    mt: 'contains'

class HasFields extends RDBOp
    tt: "HAS_FIELDS"
    mt: 'hasFields'

class WithFields extends RDBOp
    tt: "WITH_FIELDS"
    mt: 'withFields'

class Keys extends RDBOp
    tt: "KEYS"
    mt: 'keys'

class Pluck extends RDBOp
    tt: "PLUCK"
    mt: 'pluck'

class IndexesOf extends RDBOp
    tt: "INDEXES_OF"
    mt: 'indexesOf'

class Without extends RDBOp
    tt: "WITHOUT"
    mt: 'without'

class Merge extends RDBOp
    tt: "MERGE"
    mt: 'merge'

class Between extends RDBOp
    tt: "BETWEEN"
    mt: 'between'

class Reduce extends RDBOp
    tt: "REDUCE"
    mt: 'reduce'

class Map extends RDBOp
    tt: "MAP"
    mt: 'map'

class Filter extends RDBOp
    tt: "FILTER"
    mt: 'filter'

class ConcatMap extends RDBOp
    tt: "CONCATMAP"
    mt: 'concatMap'

class OrderBy extends RDBOp
    tt: "ORDERBY"
    mt: 'orderBy'

class Distinct extends RDBOp
    tt: "DISTINCT"
    mt: 'distinct'

class Count extends RDBOp
    tt: "COUNT"
    mt: 'count'

class Union extends RDBOp
    tt: "UNION"
    mt: 'union'

class Nth extends RDBOp
    tt: "NTH"
    mt: 'nth'

class Match extends RDBOp
    tt: "MATCH"
    mt: 'match'

class Upcase extends RDBOp
    tt: "UPCASE"
    mt: 'upcase'

class Downcase extends RDBOp
    tt: "DOWNCASE"
    mt: 'downcase'

class IsEmpty extends RDBOp
    tt: "IS_EMPTY"
    mt: 'isEmpty'

class GroupedMapReduce extends RDBOp
    tt: "GROUPED_MAP_REDUCE"
    mt: 'groupedMapReduce'

class GroupBy extends RDBOp
    tt: "GROUPBY"
    mt: 'groupBy'

class GroupBy extends RDBOp
    tt: "GROUPBY"
    mt: 'groupBy'

class InnerJoin extends RDBOp
    tt: "INNER_JOIN"
    mt: 'innerJoin'

class OuterJoin extends RDBOp
    tt: "OUTER_JOIN"
    mt: 'outerJoin'

class EqJoin extends RDBOp
    tt: "EQ_JOIN"
    mt: 'eqJoin'

class Zip extends RDBOp
    tt: "ZIP"
    mt: 'zip'

class CoerceTo extends RDBOp
    tt: "COERCE_TO"
    mt: 'coerceTo'

class TypeOf extends RDBOp
    tt: "TYPEOF"
    mt: 'typeOf'

class Info extends RDBOp
    tt: "INFO"
    mt: 'info'

class Sample extends RDBOp
    tt: "SAMPLE"
    mt: 'sample'

class Update extends RDBOp
    tt: "UPDATE"
    mt: 'update'

class Delete extends RDBOp
    tt: "DELETE"
    mt: 'delete'

class Replace extends RDBOp
    tt: "REPLACE"
    mt: 'replace'

class Insert extends RDBOp
    tt: "INSERT"
    mt: 'insert'

class DbCreate extends RDBOp
    tt: "DB_CREATE"
    st: 'dbCreate'

class DbDrop extends RDBOp
    tt: "DB_DROP"
    st: 'dbDrop'

class DbList extends RDBOp
    tt: "DB_LIST"
    st: 'dbList'

class TableCreate extends RDBOp
    tt: "TABLE_CREATE"
    mt: 'tableCreate'

class TableDrop extends RDBOp
    tt: "TABLE_DROP"
    mt: 'tableDrop'

class TableList extends RDBOp
    tt: "TABLE_LIST"
    mt: 'tableList'

class IndexCreate extends RDBOp
    tt: "INDEX_CREATE"
    mt: 'indexCreate'

class IndexDrop extends RDBOp
    tt: "INDEX_DROP"
    mt: 'indexDrop'

class IndexList extends RDBOp
    tt: "INDEX_LIST"
    mt: 'indexList'

class IndexStatus extends RDBOp
    tt: "INDEX_STATUS"
    mt: 'indexStatus'

class IndexWait extends RDBOp
    tt: "INDEX_WAIT"
    mt: 'indexWait'

class Sync extends RDBOp
    tt: "SYNC"
    mt: 'sync'

class FunCall extends RDBOp
    tt: "FUNCALL"
    st: 'do' # This is only used by the `undefined` argument checker

    compose: (args) ->
        if args.length > 2
            ['r.do(', intsp(args[1..]), ', ', args[0], ')']
        else
            if shouldWrap(@args[1])
                args[1] = ['r(', args[1], ')']
            [args[1], '.do(', args[0], ')']

class Default extends RDBOp
    tt: "DEFAULT"
    mt: 'default'

class Branch extends RDBOp
    tt: "BRANCH"
    st: 'branch'

class Any extends RDBOp
    tt: "ANY"
    mt: 'or'

class All extends RDBOp
    tt: "ALL"
    mt: 'and'

class ForEach extends RDBOp
    tt: "FOREACH"
    mt: 'forEach'

class Func extends RDBOp
    tt: "FUNC"
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
            throw new err.RqlDriverError "Anonymous function returned `undefined`. Did you forget a `return`?"

        argsArr = new MakeArray({}, argNums...)
        return super(optargs, argsArr, body)

    compose: (args) ->
        ['function(', (Var::compose(arg) for arg in args[0][1...-1]), ') { return ', args[1], '; }']

class Asc extends RDBOp
    tt: "ASC"
    st: 'asc'

class Desc extends RDBOp
    tt: "DESC"
    st: 'desc'

class Literal extends RDBOp
    tt: "LITERAL"
    st: 'literal'

class ISO8601 extends RDBOp
    tt: 'ISO8601'
    st: 'ISO8601'

class ToISO8601 extends RDBOp
    tt: 'TO_ISO8601'
    mt: 'toISO8601'

class EpochTime extends RDBOp
    tt: 'EPOCH_TIME'
    st: 'epochTime'

class ToEpochTime extends RDBOp
    tt: 'TO_EPOCH_TIME'
    mt: 'toEpochTime'

class Now extends RDBOp
    tt: 'NOW'
    st: 'now'

class InTimezone extends RDBOp
    tt: 'IN_TIMEZONE'
    mt: 'inTimezone'

class During extends RDBOp
    tt: 'DURING'
    mt: 'during'

class RQLDate extends RDBOp
    tt: 'DATE'
    mt: 'date'

class TimeOfDay extends RDBOp
    tt: 'TIME_OF_DAY'
    mt: 'timeOfDay'

class Timezone extends RDBOp
    tt: 'TIMEZONE'
    mt: 'timezone'

class Year extends RDBOp
    tt: 'YEAR'
    mt: 'year'

class Month extends RDBOp
    tt: 'MONTH'
    mt: 'month'

class Day extends RDBOp
    tt: 'DAY'
    mt: 'day'

class DayOfWeek extends RDBOp
    tt: 'DAY_OF_WEEK'
    mt: 'dayOfWeek'

class DayOfYear extends RDBOp
    tt: 'DAY_OF_YEAR'
    mt: 'dayOfYear'

class Hours extends RDBOp
    tt: 'HOURS'
    mt: 'hours'

class Minutes extends RDBOp
    tt: 'MINUTES'
    mt: 'minutes'

class Seconds extends RDBOp
    tt: 'SECONDS'
    mt: 'seconds'

class Time extends RDBOp
    tt: 'TIME'
    st: 'time'

# All top level exported functions

# Wrap a native JS value in an ReQL datum
rethinkdb.expr = varar 1, 2, (val, nestingDepth=20) ->
    if val is undefined
        throw new err.RqlDriverError "Cannot wrap undefined with r.expr()."

    if nestingDepth <= 0
        throw new err.RqlDriverError "Nesting depth limit exceeded"

    else if val instanceof TermBase
        val
    else if val instanceof Function
        new Func {}, val
    else if val instanceof Date
        new ISO8601 {}, val.toISOString()
    else if Array.isArray val
        val = (rethinkdb.expr(v, nestingDepth - 1) for v in val)
        new MakeArray {}, val...
    else if val == Object(val)
        obj = {}
        for own k,v of val
            if typeof v is 'undefined'
                throw new err.RqlDriverError "Object field '#{k}' may not be undefined"
            obj[k] = rethinkdb.expr(v, nestingDepth - 1)
        new MakeObject obj
    else
        new DatumTerm val

# Use r.json to serialize as much of the obect as JSON as is
# feasible to avoid doing too much protobuf serialization.
rethinkdb.exprJSON = varar 1, 2, (val, nestingDepth=20) ->
    if nestingDepth <= 0
        throw new err.RqlDriverError "Nesting depth limit exceeded"

    if isJSON(val, nestingDepth - 1)
        rethinkdb.json(JSON.stringify(val))
    else if (val instanceof TermBase)
        val
    else if (val instanceof Date)
        rethinkdb.expr(val)
    else
        if Array.isArray(val)
            wrapped = []
        else
            wrapped = {}

        for k,v of val
            wrapped[k] = rethinkdb.exprJSON(v, nestingDepth - 1)
        rethinkdb.expr(wrapped, nestingDepth - 1)

# Is this JS value representable as JSON?
isJSON = (val, nestingDepth=20) ->
    if nestingDepth <= 0
        throw new err.RqlDriverError "Nesting depth limit exceeded"

    if (val instanceof TermBase)
        false
    else if (val instanceof Function)
        false
    else if (val instanceof Date)
        false
    else if (val instanceof Object)
        # Covers array case as well
        for own k,v of val
            if not isJSON(v, nestingDepth - 1) then return false
        true
    else
        # Primitive types can always be represented as JSON
        true

rethinkdb.js = aropt (jssrc, opts) -> new JavaScript opts, jssrc

rethinkdb.json = ar (jsonsrc) -> new Json {}, jsonsrc

rethinkdb.error = varar 0, 1, (args...) -> new UserError {}, args...

rethinkdb.row = new ImplicitVar {}

rethinkdb.table = aropt (tblName, opts) -> new Table opts, tblName

rethinkdb.db = ar (dbName) -> new Db {}, dbName

rethinkdb.dbCreate = ar (dbName) -> new DbCreate {}, dbName
rethinkdb.dbDrop = ar (dbName) -> new DbDrop {}, dbName
rethinkdb.dbList = ar () -> new DbList {}

rethinkdb.tableCreate = aropt (tblName, opts) -> new TableCreate opts, tblName
rethinkdb.tableDrop = ar (tblName) -> new TableDrop {}, tblName
rethinkdb.tableList = ar () -> new TableList {}

rethinkdb.do = varar 1, null, (args...) ->
    new FunCall {}, funcWrap(args[-1..][0]), args[...-1]...

rethinkdb.branch = ar (test, trueBranch, falseBranch) -> new Branch {}, test, trueBranch, falseBranch

rethinkdb.count =              {'COUNT': true}
rethinkdb.sum   = ar (attr) -> {'SUM': attr}
rethinkdb.avg   = ar (attr) -> {'AVG': attr}

rethinkdb.asc = (attr) -> new Asc {}, funcWrap(attr)
rethinkdb.desc = (attr) -> new Desc {}, funcWrap(attr)

rethinkdb.eq = varar 2, null, (args...) -> new Eq {}, args...
rethinkdb.ne = varar 2, null, (args...) -> new Ne {}, args...
rethinkdb.lt = varar 2, null, (args...) -> new Lt {}, args...
rethinkdb.le = varar 2, null, (args...) -> new Le {}, args...
rethinkdb.gt = varar 2, null, (args...) -> new Gt {}, args...
rethinkdb.ge = varar 2, null, (args...) -> new Ge {}, args...
rethinkdb.or = varar 2, null, (args...) -> new Any {}, args...
rethinkdb.and = varar 2, null, (args...) -> new All {}, args...

rethinkdb.not = ar (x) -> new Not {}, x

rethinkdb.add = varar 2, null, (args...) -> new Add {}, args...
rethinkdb.sub = varar 2, null, (args...) -> new Sub {}, args...
rethinkdb.mul = varar 2, null, (args...) -> new Mul {}, args...
rethinkdb.div = varar 2, null, (args...) -> new Div {}, args...
rethinkdb.mod = ar (a, b) -> new Mod {}, a, b

rethinkdb.typeOf = ar (val) -> new TypeOf {}, val
rethinkdb.info = ar (val) -> new Info {}, val

rethinkdb.literal = varar 0, 1, (args...) -> new Literal {}, args...

rethinkdb.ISO8601 = aropt (str, opts) -> new ISO8601 opts, str
rethinkdb.epochTime = ar (num) -> new EpochTime {}, num
rethinkdb.now = ar () -> new Now {}
rethinkdb.time = varar 3, 7, (args...) -> new Time {}, args...

rethinkdb.monday = new (class extends RDBOp then tt: 'MONDAY')()
rethinkdb.tuesday = new (class extends RDBOp then tt: 'TUESDAY')()
rethinkdb.wednesday = new (class extends RDBOp then tt: 'WEDNESDAY')()
rethinkdb.thursday = new (class extends RDBOp then tt: 'THURSDAY')()
rethinkdb.friday = new (class extends RDBOp then tt: 'FRIDAY')()
rethinkdb.saturday = new (class extends RDBOp then tt: 'SATURDAY')()
rethinkdb.sunday = new (class extends RDBOp then tt: 'SUNDAY')()

rethinkdb.january = new (class extends RDBOp then tt: 'JANUARY')()
rethinkdb.february = new (class extends RDBOp then tt: 'FEBRUARY')()
rethinkdb.march = new (class extends RDBOp then tt: 'MARCH')()
rethinkdb.april = new (class extends RDBOp then tt: 'APRIL')()
rethinkdb.may = new (class extends RDBOp then tt: 'MAY')()
rethinkdb.june = new (class extends RDBOp then tt: 'JUNE')()
rethinkdb.july = new (class extends RDBOp then tt: 'JULY')()
rethinkdb.august = new (class extends RDBOp then tt: 'AUGUST')()
rethinkdb.september = new (class extends RDBOp then tt: 'SEPTEMBER')()
rethinkdb.october = new (class extends RDBOp then tt: 'OCTOBER')()
rethinkdb.november = new (class extends RDBOp then tt: 'NOVEMBER')()
rethinkdb.december = new (class extends RDBOp then tt: 'DECEMBER')()

# Export all names defined on rethinkdb
module.exports = rethinkdb
