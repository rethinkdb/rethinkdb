util = require('./util')
err = require('./errors')
net = require('./net')
protoTermType = require('./proto-def').Term.TermType
Promise = require('bluebird')

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

hasImplicit = (args) ->
    # args is an array of (strings and arrays)
    # We recurse to look for `r.row` which is an implicit var
    if Array.isArray(args)
        for arg in args
            if hasImplicit(arg) is true
                return true
    else if args is 'r.row'
        return true
    return false

# AST classes

class TermBase
    showRunWarning: true
    constructor: ->
        self = (ar (field) -> self.getField(field))
        self.__proto__ = @.__proto__
        return self

    run: (connection, options, callback) ->
        # Valid syntaxes are
        # connection, callback
        # connection, options # return a Promise
        # connection, options, callback
        # connection, null, callback
        # connection, null # return a Promise
        # 
        # Depreciated syntaxes are
        # optionsWithConnection, callback
        
        if net.isConnection(connection) is true
            # Handle run(connection, callback)
            if typeof options is "function" 
                if callback is undefined
                    callback = options
                    options = {}
                else
                    throw new err.RqlDriverError "Second argument to `run` cannot be a function is a third argument is provided."
            # else we suppose that we have run(connection[, options][, callback])
        else if connection?.constructor is Object
            if @showRunWarning is true
                process?.stderr.write("RethinkDB warning: This syntax is deprecated. Please use `run(connection[, options], callback)`.")
                @showRunWarning = false
            # Handle run(connectionWithOptions, callback)
            callback = options
            options = connection
            connection = connection.connection
            delete options["connection"]

        options = {} if not options?

        # Check if the arguments are valid types
        for own key of options
            unless key in ['useOutdated', 'noreply', 'timeFormat', 'profile', 'durability', 'groupFormat', 'batchConf']
                throw new err.RqlDriverError "Found "+key+" which is not a valid option. valid options are {useOutdated: <bool>, noreply: <bool>, timeFormat: <string>, groupFormat: <string>, profile: <bool>, durability: <string>}."
        if net.isConnection(connection) is false
            throw new err.RqlDriverError "First argument to `run` must be an open connection."

        if options.noreply is true or typeof callback is 'function'
            try
                connection._start @, callback, options
            catch e
                # It was decided that, if we can, we prefer to invoke the callback
                # with any errors rather than throw them as normal exceptions.
                # Thus we catch errors here and invoke the callback instead of
                # letting the error bubble up.
                if typeof(callback) is 'function'
                    callback(e)
        else
            new Promise (resolve, reject) =>
                callback = (err, result) ->
                    if err?
                        reject(err)
                    else
                        resolve(result)

                try
                    connection._start @, callback, options
                catch e
                    callback(e)

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
    slice: varar(1, 3, (left, right_or_opts, opts) -> 
        if opts?
            new Slice opts, @, left, right_or_opts
        else if typeof right_or_opts isnt 'undefined'
            if (Object::toString.call(right_or_opts) is '[object Object]') and not (right_or_opts instanceof TermBase)
                new Slice right_or_opts, @, left
            else
                new Slice {}, @, left, right_or_opts
        else
            new Slice {}, @, left
        )
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
    reduce: ar (func) -> new Reduce {}, @, funcWrap(func)
    map: ar (func) -> new Map {}, @, funcWrap(func)
    filter: aropt (predicate, opts) -> new Filter opts, @, funcWrap(predicate)
    concatMap: ar (func) -> new ConcatMap {}, @, funcWrap(func)
    distinct: ar () -> new Distinct {}, @
    count: varar(0, 1, (fun...) -> new Count {}, @, fun.map(funcWrap)...)
    union: varar(1, null, (others...) -> new Union {}, @, others...)
    nth: ar (index) -> new Nth {}, @, index
    match: ar (pattern) -> new Match {}, @, pattern
    split: varar(0, null, (fields...) -> new Split {}, @, fields.map(funcWrap)...)
    upcase: ar () -> new Upcase {}, @
    downcase: ar () -> new Downcase {}, @
    isEmpty: ar () -> new IsEmpty {}, @
    innerJoin: ar (other, predicate) -> new InnerJoin {}, @, other, predicate
    outerJoin: ar (other, predicate) -> new OuterJoin {}, @, other, predicate
    eqJoin: aropt (left_attr, right, opts) -> new EqJoin opts, @, funcWrap(left_attr), right
    zip: ar () -> new Zip {}, @
    coerceTo: ar (type) -> new CoerceTo {}, @, type
    ungroup: ar () -> new Ungroup {}, @
    typeOf: ar () -> new TypeOf {}, @
    update: aropt (func, opts) -> new Update opts, @, funcWrap(func)
    delete: aropt (opts) -> new Delete opts, @
    replace: aropt (func, opts) -> new Replace opts, @, funcWrap(func)
    do: ar (func) -> new FunCall {}, funcWrap(func), @
    default: ar (x) -> new Default {}, @, x

    or: varar(1, null, (others...) -> new Any {}, @, others...)
    and: varar(1, null, (others...) -> new All {}, @, others...)

    forEach: ar (func) -> new ForEach {}, @, funcWrap(func)

    sum: varar(0, null, (fields...) -> new Sum {}, @, fields.map(funcWrap)...)
    avg: varar(0, null, (fields...) -> new Avg {}, @, fields.map(funcWrap)...)
    min: varar(0, null, (fields...) -> new Min {}, @, fields.map(funcWrap)...)
    max: varar(0, null, (fields...) -> new Max {}, @, fields.map(funcWrap)...)

    info: ar () -> new Info {}, @
    sample: ar (count) -> new Sample {}, @, count

    group: (fieldsAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        fields = fieldsAndOpts

        # Look for opts dict
        perhapsOptDict = fieldsAndOpts[fieldsAndOpts.length - 1]
        if perhapsOptDict and
                (Object::toString.call(perhapsOptDict) is '[object Object]') and
                not (perhapsOptDict instanceof TermBase)
            opts = perhapsOptDict
            fields = fieldsAndOpts[0...(fieldsAndOpts.length - 1)]

        new Group opts, @, fields.map(funcWrap)...

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

    insert: aropt (doc, opts) -> new Insert opts, @, rethinkdb.expr(doc)
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
        if typeof(@data) is 'number'
            if !isFinite(@data)
                throw new TypeError("Illegal non-finite number `" + @data.toString() + "`.")
        @data

translateBackOptargs = (optargs) ->
    result = {}
    for own key,val of optargs
        key = switch key
            when 'primary_key' then 'primaryKey'
            when 'return_vals' then 'returnVals'
            when 'use_outdated' then 'useOutdated'
            when 'non_atomic' then 'nonAtomic'
            when 'left_bound' then 'leftBound'
            when 'right_bound' then 'rightBound'
            when 'default_timezone' then 'defaultTimezone'
            when 'result_format' then 'resultFormat'
            else key

        result[key] = val
    return result

translateOptargs = (optargs) ->
    result = {}
    for own key,val of optargs
        # We translate known two word opt-args to camel case for your convience
        key = switch key
            when 'primaryKey' then 'primary_key'
            when 'returnVals' then 'return_vals'
            when 'useOutdated' then 'use_outdated'
            when 'nonAtomic' then 'non_atomic'
            when 'leftBound' then 'left_bound'
            when 'rightBound' then 'right_bound'
            when 'defaultTimezone' then 'default_timezone'
            when 'resultFormat' then 'result_format'
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
        res = [@tt, []]
        for arg in @args
            res[1].push(arg.build())

        opts = {}
        add_opts = false

        for own key,val of @optargs
            add_opts = true
            opts[key] = val.build()

        if add_opts
            res.push(opts)
        res

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
        argrepr.push(kved(translateBackOptargs(optargs)))
    return argrepr

shouldWrap = (arg) ->
    arg instanceof DatumTerm or arg instanceof MakeArray or arg instanceof MakeObject

class MakeArray extends RDBOp
    tt: protoTermType.MAKE_ARRAY
    st: '[...]' # This is only used by the `undefined` argument checker

    compose: (args) -> ['[', intsp(args), ']']

class MakeObject extends RDBOp
    tt: protoTermType.MAKE_OBJECT
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

    build: ->
        res = {}
        for own key,val of @optargs
            res[key] = val.build()
        return res

class Var extends RDBOp
    tt: protoTermType.VAR
    compose: (args) -> ['var_'+args[0]]

class JavaScript extends RDBOp
    tt: protoTermType.JAVASCRIPT
    st: 'js'

class Http extends RDBOp
    tt: protoTermType.HTTP
    st: 'http'

class Json extends RDBOp
    tt: protoTermType.JSON
    st: 'json'

class UserError extends RDBOp
    tt: protoTermType.ERROR
    st: 'error'

class Random extends RDBOp
    tt: protoTermType.RANDOM
    st: 'random'

class ImplicitVar extends RDBOp
    tt: protoTermType.IMPLICIT_VAR
    compose: -> ['r.row']

class Db extends RDBOp
    tt: protoTermType.DB
    st: 'db'

class Table extends RDBOp
    tt: protoTermType.TABLE
    st: 'table'

    compose: (args, optargs) ->
        if @args[0] instanceof Db
            [args[0], '.table(', intspallargs(args[1..], optargs), ')']
        else
            ['r.table(', intspallargs(args, optargs), ')']

class Get extends RDBOp
    tt: protoTermType.GET
    mt: 'get'

class GetAll extends RDBOp
    tt: protoTermType.GET_ALL
    mt: 'getAll'

class Eq extends RDBOp
    tt: protoTermType.EQ
    mt: 'eq'

class Ne extends RDBOp
    tt: protoTermType.NE
    mt: 'ne'

class Lt extends RDBOp
    tt: protoTermType.LT
    mt: 'lt'

class Le extends RDBOp
    tt: protoTermType.LE
    mt: 'le'

class Gt extends RDBOp
    tt: protoTermType.GT
    mt: 'gt'

class Ge extends RDBOp
    tt: protoTermType.GE
    mt: 'ge'

class Not extends RDBOp
    tt: protoTermType.NOT
    mt: 'not'

class Add extends RDBOp
    tt: protoTermType.ADD
    mt: 'add'

class Sub extends RDBOp
    tt: protoTermType.SUB
    mt: 'sub'

class Mul extends RDBOp
    tt: protoTermType.MUL
    mt: 'mul'

class Div extends RDBOp
    tt: protoTermType.DIV
    mt: 'div'

class Mod extends RDBOp
    tt: protoTermType.MOD
    mt: 'mod'

class Append extends RDBOp
    tt: protoTermType.APPEND
    mt: 'append'

class Prepend extends RDBOp
    tt: protoTermType.PREPEND
    mt: 'prepend'

class Difference extends RDBOp
    tt: protoTermType.DIFFERENCE
    mt: 'difference'

class SetInsert extends RDBOp
    tt: protoTermType.SET_INSERT
    mt: 'setInsert'

class SetUnion extends RDBOp
    tt: protoTermType.SET_UNION
    mt: 'setUnion'

class SetIntersection extends RDBOp
    tt: protoTermType.SET_INTERSECTION
    mt: 'setIntersection'

class SetDifference extends RDBOp
    tt: protoTermType.SET_DIFFERENCE
    mt: 'setDifference'

class Slice extends RDBOp
    tt: protoTermType.SLICE
    mt: 'slice'

class Skip extends RDBOp
    tt: protoTermType.SKIP
    mt: 'skip'

class Limit extends RDBOp
    tt: protoTermType.LIMIT
    mt: 'limit'

class GetField extends RDBOp
    tt: protoTermType.GET_FIELD
    st: '(...)' # This is only used by the `undefined` argument checker

    compose: (args) -> [args[0], '(', args[1], ')']

class Contains extends RDBOp
    tt: protoTermType.CONTAINS
    mt: 'contains'

class InsertAt extends RDBOp
    tt: protoTermType.INSERT_AT
    mt: 'insertAt'

class SpliceAt extends RDBOp
    tt: protoTermType.SPLICE_AT
    mt: 'spliceAt'

class DeleteAt extends RDBOp
    tt: protoTermType.DELETE_AT
    mt: 'deleteAt'

class ChangeAt extends RDBOp
    tt: protoTermType.CHANGE_AT
    mt: 'changeAt'

class Contains extends RDBOp
    tt: protoTermType.CONTAINS
    mt: 'contains'

class HasFields extends RDBOp
    tt: protoTermType.HAS_FIELDS
    mt: 'hasFields'

class WithFields extends RDBOp
    tt: protoTermType.WITH_FIELDS
    mt: 'withFields'

class Keys extends RDBOp
    tt: protoTermType.KEYS
    mt: 'keys'

class Object_ extends RDBOp
    tt: protoTermType.OBJECT
    mt: 'object'

class Pluck extends RDBOp
    tt: protoTermType.PLUCK
    mt: 'pluck'

class IndexesOf extends RDBOp
    tt: protoTermType.INDEXES_OF
    mt: 'indexesOf'

class Without extends RDBOp
    tt: protoTermType.WITHOUT
    mt: 'without'

class Merge extends RDBOp
    tt: protoTermType.MERGE
    mt: 'merge'

class Between extends RDBOp
    tt: protoTermType.BETWEEN
    mt: 'between'

class Reduce extends RDBOp
    tt: protoTermType.REDUCE
    mt: 'reduce'

class Map extends RDBOp
    tt: protoTermType.MAP
    mt: 'map'

class Filter extends RDBOp
    tt: protoTermType.FILTER
    mt: 'filter'

class ConcatMap extends RDBOp
    tt: protoTermType.CONCATMAP
    mt: 'concatMap'

class OrderBy extends RDBOp
    tt: protoTermType.ORDERBY
    mt: 'orderBy'

class Distinct extends RDBOp
    tt: protoTermType.DISTINCT
    mt: 'distinct'

class Count extends RDBOp
    tt: protoTermType.COUNT
    mt: 'count'

class Union extends RDBOp
    tt: protoTermType.UNION
    mt: 'union'

class Nth extends RDBOp
    tt: protoTermType.NTH
    mt: 'nth'

class Match extends RDBOp
    tt: protoTermType.MATCH
    mt: 'match'

class Split extends RDBOp
    tt: protoTermType.SPLIT
    mt: 'split'

class Upcase extends RDBOp
    tt: protoTermType.UPCASE
    mt: 'upcase'

class Downcase extends RDBOp
    tt: protoTermType.DOWNCASE
    mt: 'downcase'

class IsEmpty extends RDBOp
    tt: protoTermType.IS_EMPTY
    mt: 'isEmpty'

class Group extends RDBOp
    tt: protoTermType.GROUP
    mt: 'group'

class Sum extends RDBOp
    tt: protoTermType.SUM
    mt: 'sum'

class Avg extends RDBOp
    tt: protoTermType.AVG
    mt: 'avg'

class Min extends RDBOp
    tt: protoTermType.MIN
    mt: 'min'

class Max extends RDBOp
    tt: protoTermType.MAX
    mt: 'max'

class InnerJoin extends RDBOp
    tt: protoTermType.INNER_JOIN
    mt: 'innerJoin'

class OuterJoin extends RDBOp
    tt: protoTermType.OUTER_JOIN
    mt: 'outerJoin'

class EqJoin extends RDBOp
    tt: protoTermType.EQ_JOIN
    mt: 'eqJoin'

class Zip extends RDBOp
    tt: protoTermType.ZIP
    mt: 'zip'

class CoerceTo extends RDBOp
    tt: protoTermType.COERCE_TO
    mt: 'coerceTo'

class Ungroup extends RDBOp
    tt: protoTermType.UNGROUP
    mt: 'ungroup'

class TypeOf extends RDBOp
    tt: protoTermType.TYPEOF
    mt: 'typeOf'

class Info extends RDBOp
    tt: protoTermType.INFO
    mt: 'info'

class Sample extends RDBOp
    tt: protoTermType.SAMPLE
    mt: 'sample'

class Update extends RDBOp
    tt: protoTermType.UPDATE
    mt: 'update'

class Delete extends RDBOp
    tt: protoTermType.DELETE
    mt: 'delete'

class Replace extends RDBOp
    tt: protoTermType.REPLACE
    mt: 'replace'

class Insert extends RDBOp
    tt: protoTermType.INSERT
    mt: 'insert'

class DbCreate extends RDBOp
    tt: protoTermType.DB_CREATE
    st: 'dbCreate'

class DbDrop extends RDBOp
    tt: protoTermType.DB_DROP
    st: 'dbDrop'

class DbList extends RDBOp
    tt: protoTermType.DB_LIST
    st: 'dbList'

class TableCreate extends RDBOp
    tt: protoTermType.TABLE_CREATE
    mt: 'tableCreate'

class TableDrop extends RDBOp
    tt: protoTermType.TABLE_DROP
    mt: 'tableDrop'

class TableList extends RDBOp
    tt: protoTermType.TABLE_LIST
    mt: 'tableList'

class IndexCreate extends RDBOp
    tt: protoTermType.INDEX_CREATE
    mt: 'indexCreate'

class IndexDrop extends RDBOp
    tt: protoTermType.INDEX_DROP
    mt: 'indexDrop'

class IndexList extends RDBOp
    tt: protoTermType.INDEX_LIST
    mt: 'indexList'

class IndexStatus extends RDBOp
    tt: protoTermType.INDEX_STATUS
    mt: 'indexStatus'

class IndexWait extends RDBOp
    tt: protoTermType.INDEX_WAIT
    mt: 'indexWait'

class Sync extends RDBOp
    tt: protoTermType.SYNC
    mt: 'sync'

class FunCall extends RDBOp
    tt: protoTermType.FUNCALL
    st: 'do' # This is only used by the `undefined` argument checker

    compose: (args) ->
        if args.length > 2
            ['r.do(', intsp(args[1..]), ', ', args[0], ')']
        else
            if shouldWrap(@args[1])
                args[1] = ['r(', args[1], ')']
            [args[1], '.do(', args[0], ')']

class Default extends RDBOp
    tt: protoTermType.DEFAULT
    mt: 'default'

class Branch extends RDBOp
    tt: protoTermType.BRANCH
    st: 'branch'

class Any extends RDBOp
    tt: protoTermType.ANY
    mt: 'or'

class All extends RDBOp
    tt: protoTermType.ALL
    mt: 'and'

class ForEach extends RDBOp
    tt: protoTermType.FOREACH
    mt: 'forEach'

class Func extends RDBOp
    tt: protoTermType.FUNC
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
        if hasImplicit(args[1]) is true
            [args[1]]
        else
            varStr = ""
            for arg, i in args[0][1] # ['0', ', ', '1']
                if i%2 is 0
                    varStr += Var::compose(arg)
                else
                    varStr += arg
            ['function(', varStr, ') { return ', args[1], '; }']

class Asc extends RDBOp
    tt: protoTermType.ASC
    st: 'asc'

class Desc extends RDBOp
    tt: protoTermType.DESC
    st: 'desc'

class Literal extends RDBOp
    tt: protoTermType.LITERAL
    st: 'literal'

class ISO8601 extends RDBOp
    tt: protoTermType.ISO8601
    st: 'ISO8601'

class ToISO8601 extends RDBOp
    tt: protoTermType.TO_ISO8601
    mt: 'toISO8601'

class EpochTime extends RDBOp
    tt: protoTermType.EPOCH_TIME
    st: 'epochTime'

class ToEpochTime extends RDBOp
    tt: protoTermType.TO_EPOCH_TIME
    mt: 'toEpochTime'

class Now extends RDBOp
    tt: protoTermType.NOW
    st: 'now'

class InTimezone extends RDBOp
    tt: protoTermType.IN_TIMEZONE
    mt: 'inTimezone'

class During extends RDBOp
    tt: protoTermType.DURING
    mt: 'during'

class RQLDate extends RDBOp
    tt: protoTermType.DATE
    mt: 'date'

class TimeOfDay extends RDBOp
    tt: protoTermType.TIME_OF_DAY
    mt: 'timeOfDay'

class Timezone extends RDBOp
    tt: protoTermType.TIMEZONE
    mt: 'timezone'

class Year extends RDBOp
    tt: protoTermType.YEAR
    mt: 'year'

class Month extends RDBOp
    tt: protoTermType.MONTH
    mt: 'month'

class Day extends RDBOp
    tt: protoTermType.DAY
    mt: 'day'

class DayOfWeek extends RDBOp
    tt: protoTermType.DAY_OF_WEEK
    mt: 'dayOfWeek'

class DayOfYear extends RDBOp
    tt: protoTermType.DAY_OF_YEAR
    mt: 'dayOfYear'

class Hours extends RDBOp
    tt: protoTermType.HOURS
    mt: 'hours'

class Minutes extends RDBOp
    tt: protoTermType.MINUTES
    mt: 'minutes'

class Seconds extends RDBOp
    tt: protoTermType.SECONDS
    mt: 'seconds'

class Time extends RDBOp
    tt: protoTermType.TIME
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
    else if typeof(val) is 'number'
        new DatumTerm val
    else if val == Object(val)
        obj = {}
        for own k,v of val
            if typeof v is 'undefined'
                throw new err.RqlDriverError "Object field '#{k}' may not be undefined"
            obj[k] = rethinkdb.expr(v, nestingDepth - 1)
        new MakeObject obj
    else
        new DatumTerm val

rethinkdb.js = aropt (jssrc, opts) -> new JavaScript opts, jssrc

rethinkdb.http = aropt (url, opts) -> new Http opts, url

rethinkdb.json = ar (jsonsrc) -> new Json {}, jsonsrc

rethinkdb.error = varar 0, 1, (args...) -> new UserError {}, args...

rethinkdb.random = (limitsAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        limits = limitsAndOpts

        # Look for opts dict
        perhapsOptDict = limitsAndOpts[limitsAndOpts.length - 1]
        if perhapsOptDict and
                ((Object::toString.call(perhapsOptDict) is '[object Object]') and not (perhapsOptDict instanceof TermBase))
            opts = perhapsOptDict
            limits = limitsAndOpts[0...(limitsAndOpts.length - 1)]

        new Random opts, limits...

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

rethinkdb.monday = new (class extends RDBOp then tt: protoTermType.MONDAY)()
rethinkdb.tuesday = new (class extends RDBOp then tt: protoTermType.TUESDAY)()
rethinkdb.wednesday = new (class extends RDBOp then tt: protoTermType.WEDNESDAY)()
rethinkdb.thursday = new (class extends RDBOp then tt: protoTermType.THURSDAY)()
rethinkdb.friday = new (class extends RDBOp then tt: protoTermType.FRIDAY)()
rethinkdb.saturday = new (class extends RDBOp then tt: protoTermType.SATURDAY)()
rethinkdb.sunday = new (class extends RDBOp then tt: protoTermType.SUNDAY)()

rethinkdb.january = new (class extends RDBOp then tt: protoTermType.JANUARY)()
rethinkdb.february = new (class extends RDBOp then tt: protoTermType.FEBRUARY)()
rethinkdb.march = new (class extends RDBOp then tt: protoTermType.MARCH)()
rethinkdb.april = new (class extends RDBOp then tt: protoTermType.APRIL)()
rethinkdb.may = new (class extends RDBOp then tt: protoTermType.MAY)()
rethinkdb.june = new (class extends RDBOp then tt: protoTermType.JUNE)()
rethinkdb.july = new (class extends RDBOp then tt: protoTermType.JULY)()
rethinkdb.august = new (class extends RDBOp then tt: protoTermType.AUGUST)()
rethinkdb.september = new (class extends RDBOp then tt: protoTermType.SEPTEMBER)()
rethinkdb.october = new (class extends RDBOp then tt: protoTermType.OCTOBER)()
rethinkdb.november = new (class extends RDBOp then tt: protoTermType.NOVEMBER)()
rethinkdb.december = new (class extends RDBOp then tt: protoTermType.DECEMBER)()

rethinkdb.object = varar 0, null, (args...) -> new Object_ {}, args...

# Export all names defined on rethinkdb
module.exports = rethinkdb
