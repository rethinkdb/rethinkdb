# # AST

util = require('./util')
err = require('./errors')
net = require('./net')
protoTermType = require('./proto-def').Term.TermType
Promise = require('bluebird')
# Import some names to this namespace for convenience
ar = util.ar
varar = util.varar
aropt = util.aropt

# rethinkdb is both the main export object for the module
# and a function that shortcuts `r.expr`.
rethinkdb = (args...) -> rethinkdb.expr(args...)

# ## Utilities

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
        if {}.__proto__ != undefined
            self = (ar (field) -> self.bracket(field))
            self.__proto__ = @.__proto__
            return self
        else
            return @

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
                    #options is a function here
                    return Promise.reject(new err.ReqlDriverError("Second argument to `run` cannot be a function if a third argument is provided.")).nodeify options
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

        if callback? and typeof callback isnt 'function'
            return Promise.reject(new err.ReqlDriverError("If provided, the callback must be a function. Please use `run(connection[, options][, callback])"))

        if net.isConnection(connection) is false
            return Promise.reject(new err.ReqlDriverError("First argument to `run` must be an open connection.")).nodeify callback

        # if `noreply` is `true`, the callback will be immediately called without error
        # so we do not have to worry about bluebird complaining about errors not being
        # caught
        new Promise( (resolve, reject) =>
            wrappedCb = (err, result) ->
                if err?
                    reject(err)
                else
                    resolve(result)

            try
                connection._start @, wrappedCb, options
            catch e
                # It was decided that, if we can, we prefer to invoke the callback
                # with any errors rather than throw them as normal exceptions.
                # Thus we catch errors here and invoke the callback instead of
                # letting the error bubble up.
                wrappedCb(e)

        ).nodeify callback

    toString: -> err.printQuery(@)

class RDBVal extends TermBase
    eq: (args...) -> new Eq {}, @, args...
    ne: (args...) -> new Ne {}, @, args...
    lt: (args...) -> new Lt {}, @, args...
    le: (args...) -> new Le {}, @, args...
    gt: (args...) -> new Gt {}, @, args...
    ge: (args...) -> new Ge {}, @, args...

    not: (args...) -> new Not {}, @, args...

    add: (args...) -> new Add {}, @, args...
    sub: (args...) -> new Sub {}, @, args...
    mul: (args...) -> new Mul {}, @, args...
    div: (args...) -> new Div {}, @, args...
    mod: (args...) -> new Mod {}, @, args...
    floor: (args...) -> new Floor {}, @, args...
    ceil: (args...) -> new Ceil {}, @, args...
    round: (args...) -> new Round {}, @, args...

    append: (args...) -> new Append {}, @, args...
    prepend: (args...) -> new Prepend {}, @, args...
    difference: (args...) -> new Difference {}, @, args...
    setInsert: (args...) -> new SetInsert {}, @, args...
    setUnion: (args...) -> new SetUnion {}, @, args...
    setIntersection: (args...) -> new SetIntersection {}, @, args...
    setDifference: (args...) -> new SetDifference {}, @, args...
    slice: varar(1, 3, (left, right_or_opts, opts) ->
        if opts?
            new Slice opts, @, left, right_or_opts
        else if typeof right_or_opts isnt 'undefined'
            # FIXME
            if (Object::toString.call(right_or_opts) is '[object Object]') and not (right_or_opts instanceof TermBase)
                new Slice right_or_opts, @, left
            else
                new Slice {}, @, left, right_or_opts
        else
            new Slice {}, @, left
        )
    skip: (args...) -> new Skip {}, @, args...
    limit: (args...) -> new Limit {}, @, args...
    getField: (args...) -> new GetField {}, @, args...
    contains: (args...) -> new Contains {}, @, args.map(funcWrap)...
    insertAt: (args...) -> new InsertAt {}, @, args...
    spliceAt: (args...) -> new SpliceAt {}, @, args...
    deleteAt: (args...) -> new DeleteAt {}, @, args...
    changeAt: (args...) -> new ChangeAt {}, @, args...
    offsetsOf: (args...) -> new OffsetsOf {}, @, args.map(funcWrap)...
    hasFields: (args...) -> new HasFields {}, @, args...
    withFields: (args...) -> new WithFields {}, @, args...
    keys: (args...) -> new Keys {}, @, args...
    values: (args...) -> new Values {}, @, args...
    changes: aropt (opts) -> new Changes opts, @

    # pluck and without on zero fields are allowed
    pluck: (args...) -> new Pluck {}, @, args...
    without: (args...) -> new Without {}, @, args...

    merge: (args...) -> new Merge {}, @, args.map(funcWrap)...
    between: aropt (left, right, opts) -> new Between opts, @, left, right
    reduce: (args...) -> new Reduce {}, @, args.map(funcWrap)...
    map: varar 1, null, (args..., funcArg) -> new Map {}, @, args..., funcWrap(funcArg)
    fold: aropt (baseArg, accFuncArg, opts) -> new Fold opts, @, baseArg, funcWrap(accFuncArg)
    filter: aropt (predicate, opts) -> new Filter opts, @, funcWrap(predicate)
    concatMap: (args...) -> new ConcatMap {}, @, args.map(funcWrap)...
    distinct: aropt (opts) -> new Distinct opts, @
    count: (args...) -> new Count {}, @, args.map(funcWrap)...
    union: (attrsAndOpts...) ->
        opts = {}
        attrs = attrsAndOpts

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

        new Union opts, @, attrs...

    nth: (args...) -> new Nth {}, @, args...
    bracket: (args...) -> new Bracket {}, @, args...
    toJSON: (args...) -> new ToJsonString {}, @, args...
    toJsonString: (args...) -> new ToJsonString {}, @, args...
    match: (args...) -> new Match {}, @, args...
    split: (args...) -> new Split {}, @, args.map(funcWrap)...
    upcase: (args...) -> new Upcase {}, @, args...
    downcase: (args...) -> new Downcase {}, @, args...
    isEmpty: (args...) -> new IsEmpty {}, @, args...
    innerJoin: (args...) -> new InnerJoin {}, @, args...
    outerJoin: (args...) -> new OuterJoin {}, @, args...
    eqJoin: aropt (left_attr, right, opts) -> new EqJoin opts, @, funcWrap(left_attr), right
    zip: (args...) -> new Zip {}, @, args...
    coerceTo: (args...) -> new CoerceTo {}, @, args...
    ungroup: (args...) -> new Ungroup {}, @, args...
    typeOf: (args...) -> new TypeOf {}, @, args...
    update: aropt (func, opts) -> new Update opts, @, funcWrap(func)
    delete: aropt (opts) -> new Delete opts, @
    replace: aropt (func, opts) -> new Replace opts, @, funcWrap(func)
    do: (args...) ->
        new FunCall {}, funcWrap(args[-1..][0]), @, args[...-1]...

    default: (args...) -> new Default {}, @, args...

    or: (args...) -> new Or {}, @, args...
    and: (args...) -> new And {}, @, args...

    branch: (args...) -> new Branch {}, @, args...
    forEach: (args...) -> new ForEach {}, @, args.map(funcWrap)...

    sum: (args...) -> new Sum {}, @, args.map(funcWrap)...
    avg: (args...) -> new Avg {}, @, args.map(funcWrap)...

    info: (args...) -> new Info {}, @, args...
    sample: (args...) -> new Sample {}, @, args...

    group: (fieldsAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        fields = fieldsAndOpts

        # Look for opts dict
        if fieldsAndOpts.length > 0
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

    # Geo operations
    toGeojson: (args...) -> new ToGeojson {}, @, args...
    distance: aropt (g, opts) -> new Distance opts, @, g
    intersects: (args...) -> new Intersects {}, @, args...
    includes: (args...) -> new Includes {}, @, args...
    fill: (args...) -> new Fill {}, @, args...
    polygonSub: (args...) -> new PolygonSub {}, @, args...

    # Database operations

    tableCreate: aropt (tblName, opts) -> new TableCreate opts, @, tblName
    tableDrop: (args...) -> new TableDrop {}, @, args...
    tableList: (args...) -> new TableList {}, @, args...

    # Mixed db/table operations

    config: () -> new Config {}, @
    status: () -> new Status {}, @
    wait: aropt (opts) -> new Wait opts, @

    table: aropt (tblName, opts) -> new Table opts, @, tblName

    # Table operations

    get: (args...) -> new Get {}, @, args...

    getAll: (keysAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        keys = keysAndOpts
        # Look for opts dict
        if keysAndOpts.length > 1
            perhapsOptDict = keysAndOpts[keysAndOpts.length - 1]
            if perhapsOptDict and
                    ((Object::toString.call(perhapsOptDict) is '[object Object]') and not (perhapsOptDict instanceof TermBase))
                opts = perhapsOptDict
                keys = keysAndOpts[0...(keysAndOpts.length - 1)]
        new GetAll opts, @, keys...

    min: (keysAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        keys = keysAndOpts
        # Look for opts dict
        if keysAndOpts.length == 1
            perhapsOptDict = keysAndOpts[0]
            if perhapsOptDict and
                    ((Object::toString.call(perhapsOptDict) is '[object Object]') and not (perhapsOptDict instanceof TermBase))
                opts = perhapsOptDict
                keys = []
        new Min opts, @, keys.map(funcWrap)...

    max: (keysAndOpts...) ->
        # Default if no opts dict provided
        opts = {}
        keys = keysAndOpts
        # Look for opts dict
        if keysAndOpts.length == 1
            perhapsOptDict = keysAndOpts[0]
            if perhapsOptDict and
                    ((Object::toString.call(perhapsOptDict) is '[object Object]') and not (perhapsOptDict instanceof TermBase))
                opts = perhapsOptDict
                keys = []
        new Max opts, @, keys.map(funcWrap)...

    insert: aropt (doc, opts) -> new Insert opts, @, rethinkdb.expr(doc)
    indexCreate: varar(1, 3, (name, defun_or_opts, opts) ->
        if opts?
            new IndexCreate opts, @, name, funcWrap(defun_or_opts)
        else if defun_or_opts?
            # FIXME?
            if (Object::toString.call(defun_or_opts) is '[object Object]') and not (typeof defun_or_opts is 'function') and not (defun_or_opts instanceof TermBase)
                new IndexCreate defun_or_opts, @, name
            else
                new IndexCreate {}, @, name, funcWrap(defun_or_opts)
        else
            new IndexCreate {}, @, name
        )

    indexDrop: (args...) -> new IndexDrop {}, @, args...
    indexList: (args...) -> new IndexList {}, @, args...
    indexStatus: (args...) -> new IndexStatus {}, @, args...
    indexWait: (args...) -> new IndexWait {}, @, args...
    indexRename: aropt (old_name, new_name, opts) -> new IndexRename opts, @, old_name, new_name

    reconfigure: (opts) -> new Reconfigure opts, @
    rebalance: () -> new Rebalance {}, @

    sync: (args...) -> new Sync {}, @, args...

    grant: (args...) -> new Grant {}, @, args...

    toISO8601: (args...) -> new ToISO8601 {}, @, args...
    toEpochTime: (args...) -> new ToEpochTime {}, @, args...
    inTimezone: (args...) -> new InTimezone {}, @, args...
    during: aropt (t2, t3, opts) -> new During opts, @, t2, t3
    date: (args...) -> new RQLDate {}, @, args...
    timeOfDay: (args...) -> new TimeOfDay {}, @, args...
    timezone: (args...) -> new Timezone {}, @, args...

    year: (args...) -> new Year {}, @, args...
    month: (args...) -> new Month {}, @, args...
    day: (args...) -> new Day {}, @, args...
    dayOfWeek: (args...) -> new DayOfWeek {}, @, args...
    dayOfYear: (args...) -> new DayOfYear {}, @, args...
    hours: (args...) -> new Hours {}, @, args...
    minutes: (args...) -> new Minutes {}, @, args...
    seconds: (args...) -> new Seconds {}, @, args...

    uuid: (args...) -> new UUID {}, @, args...

    getIntersecting: aropt (g, opts) -> new GetIntersecting opts, @, g
    getNearest: aropt (g, opts) -> new GetNearest opts, @, g

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
        result[util.toCamelCase(key)] = val
    return result

translateOptargs = (optargs) ->
    result = {}
    for own key,val of optargs
        # We translate opt-args to camel case for your convience
        if key is undefined or val is undefined then continue
        result[util.fromCamelCase(key)] = rethinkdb.expr val
    return result

class RDBOp extends RDBVal
    constructor: (optargs, args...) ->
        self = super()
        self.args =
            for arg,i in args
                if arg isnt undefined
                    rethinkdb.expr arg
                else
                    throw new err.ReqlDriverCompileError "Argument #{i} to #{@st || @mt} may not be `undefined`."
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

    # a class that extends RDBOp should have a property `st` or `mt` depending on
    # how the term should be printed (see `compose` below)
    compose: (args, optargs) ->
        if @st
            return ['r.', @st, '(', intspallargs(args, optargs), ')']
        else # @mt is defined
            if shouldWrap(@args[0])
                args[0] = ['r(', args[0], ')']
            return [args[0], '.', @mt, '(', intspallargs(args[1..], optargs), ')']

class RDBConstant extends RDBOp
    compose: (args, optargs) ->
        # Constants should not have any args or optargs, and should not look like a call
        return ['r.', @st]

intsp = (seq) ->
    unless seq[0]? then return []
    res = [seq[0]]
    for e in seq[1..]
        res.push(', ', e)
    return res

kved = (optargs) ->
    ['{', intsp([JSON.stringify(k), ': ', v] for own k,v of optargs), '}']

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
    tt: protoTermType.MAKE_OBJ
    st: '{...}' # This is only used by the `undefined` argument checker

    constructor: (obj, nestingDepth=20) ->
        self = super({})
        self.optargs = {}
        for own key,val of obj
            if typeof val is 'undefined'
                throw new err.ReqlDriverCompileError "Object field '#{key}' may not be undefined"
            self.optargs[key] = rethinkdb.expr val, nestingDepth-1
        return self

    compose: (args, optargs) -> kved(optargs)

    build: ->
        res = {}
        for own key,val of @optargs
            res[key] = val.build()
        return res

class Var extends RDBOp
    tt: protoTermType.VAR
    compose: (args) -> ['var_'+args]

class JavaScript extends RDBOp
    tt: protoTermType.JAVASCRIPT
    st: 'js'

class Http extends RDBOp
    tt: protoTermType.HTTP
    st: 'http'

class Json extends RDBOp
    tt: protoTermType.JSON
    st: 'json'

class Binary extends RDBOp
    tt: protoTermType.BINARY
    st: 'binary'

    constructor: (data) ->
        if data instanceof TermBase
            self = super({}, data)
        else if data instanceof Buffer
            self = super()
            self.base64_data = data.toString("base64")
        else
            throw new TypeError("Parameter to `r.binary` must be a Buffer object or RQL query.")

        return self

    compose: ->
        if @args.length == 0
            'r.binary(<data>)'
        else
            super

    build: ->
        if @args.length == 0
            { '$reql_type$': 'BINARY', 'data': @base64_data }
        else
            super

class Args extends RDBOp
    tt: protoTermType.ARGS
    st: 'args'

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

class Floor extends RDBOp
    tt: protoTermType.FLOOR
    mt: 'floor'

class Ceil extends RDBOp
    tt: protoTermType.CEIL
    mt: 'ceil'

class Round extends RDBOp
    tt: protoTermType.ROUND
    mt: 'round'

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
    mt: 'getField'

class Bracket extends RDBOp
    tt: protoTermType.BRACKET
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

class HasFields extends RDBOp
    tt: protoTermType.HAS_FIELDS
    mt: 'hasFields'

class WithFields extends RDBOp
    tt: protoTermType.WITH_FIELDS
    mt: 'withFields'

class Keys extends RDBOp
    tt: protoTermType.KEYS
    mt: 'keys'

class Values extends RDBOp
    tt: protoTermType.VALUES
    mt: 'values'

class Changes extends RDBOp
    tt: protoTermType.CHANGES
    mt: 'changes'

class Object_ extends RDBOp
    tt: protoTermType.OBJECT
    mt: 'object'

class Pluck extends RDBOp
    tt: protoTermType.PLUCK
    mt: 'pluck'

class OffsetsOf extends RDBOp
    tt: protoTermType.OFFSETS_OF
    mt: 'offsetsOf'

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

class Fold extends RDBOp
    tt: protoTermType.FOLD
    mt: 'fold'

class Filter extends RDBOp
    tt: protoTermType.FILTER
    mt: 'filter'

class ConcatMap extends RDBOp
    tt: protoTermType.CONCAT_MAP
    mt: 'concatMap'

class OrderBy extends RDBOp
    tt: protoTermType.ORDER_BY
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

class ToJsonString extends RDBOp
    tt: protoTermType.TO_JSON_STRING
    mt: 'toJsonString'

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

class Range extends RDBOp
    tt: protoTermType.RANGE
    st: 'range'

class CoerceTo extends RDBOp
    tt: protoTermType.COERCE_TO
    mt: 'coerceTo'

class Ungroup extends RDBOp
    tt: protoTermType.UNGROUP
    mt: 'ungroup'

class TypeOf extends RDBOp
    tt: protoTermType.TYPE_OF
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

class IndexRename extends RDBOp
    tt: protoTermType.INDEX_RENAME
    mt: 'indexRename'

class IndexList extends RDBOp
    tt: protoTermType.INDEX_LIST
    mt: 'indexList'

class IndexStatus extends RDBOp
    tt: protoTermType.INDEX_STATUS
    mt: 'indexStatus'

class IndexWait extends RDBOp
    tt: protoTermType.INDEX_WAIT
    mt: 'indexWait'

class Config extends RDBOp
    tt: protoTermType.CONFIG
    mt: 'config'

class Status extends RDBOp
    tt: protoTermType.STATUS
    mt: 'status'

class Wait extends RDBOp
    tt: protoTermType.WAIT
    mt: 'wait'

class Reconfigure extends RDBOp
    tt: protoTermType.RECONFIGURE
    mt: 'reconfigure'

class Rebalance extends RDBOp
    tt: protoTermType.REBALANCE
    mt: 'rebalance'

class Sync extends RDBOp
    tt: protoTermType.SYNC
    mt: 'sync'

class Grant extends RDBOp
    tt: protoTermType.GRANT
    mt: 'grant'

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

class Or extends RDBOp
    tt: protoTermType.OR
    mt: 'or'

class And extends RDBOp
    tt: protoTermType.AND
    mt: 'and'

class ForEach extends RDBOp
    tt: protoTermType.FOR_EACH
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
            throw new err.ReqlDriverCompileError "Anonymous function returned `undefined`. Did you forget a `return`?"

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

class Geojson extends RDBOp
    tt: protoTermType.GEOJSON
    st: 'geojson'

class ToGeojson extends RDBOp
    tt: protoTermType.TO_GEOJSON
    mt: 'toGeojson'

class Point extends RDBOp
    tt: protoTermType.POINT
    st: 'point'

class Line extends RDBOp
    tt: protoTermType.LINE
    st: 'line'

class Polygon extends RDBOp
    tt: protoTermType.POLYGON
    st: 'polygon'

class Distance extends RDBOp
    tt: protoTermType.DISTANCE
    mt: 'distance'

class Intersects extends RDBOp
    tt: protoTermType.INTERSECTS
    mt: 'intersects'

class Includes extends RDBOp
    tt: protoTermType.INCLUDES
    mt: 'includes'

class Circle extends RDBOp
    tt: protoTermType.CIRCLE
    st: 'circle'

class GetIntersecting extends RDBOp
    tt: protoTermType.GET_INTERSECTING
    mt: 'getIntersecting'

class GetNearest extends RDBOp
    tt: protoTermType.GET_NEAREST
    mt: 'getNearest'

class Fill extends RDBOp
    tt: protoTermType.FILL
    mt: 'fill'

class PolygonSub extends RDBOp
    tt: protoTermType.POLYGON_SUB
    mt: 'polygonSub'

class UUID extends RDBOp
    tt: protoTermType.UUID
    st: 'uuid'


# All top level exported functions

# Wrap a native JS value in an ReQL datum
rethinkdb.expr = varar 1, 2, (val, nestingDepth=20) ->
    if val is undefined
        throw new err.ReqlDriverCompileError "Cannot wrap undefined with r.expr()."

    if nestingDepth <= 0
        throw new err.ReqlDriverCompileError "Nesting depth limit exceeded."

    if typeof nestingDepth isnt "number" or isNaN(nestingDepth)
        throw new err.ReqlDriverCompileError "Second argument to `r.expr` must be a number or undefined."

    else if val instanceof TermBase
        val
    else if typeof val is 'function'
        new Func {}, val
    else if val instanceof Date
        new ISO8601 {}, val.toISOString()
    else if val instanceof Buffer
        new Binary val
    else if Array.isArray val
        val = (rethinkdb.expr(v, nestingDepth - 1) for v in val)
        new MakeArray {}, val...
    else if typeof(val) is 'number'
        new DatumTerm val
    else if Object::toString.call(val) is '[object Object]'
        new MakeObject val, nestingDepth
    else
        new DatumTerm val

rethinkdb.js = aropt (jssrc, opts) -> new JavaScript opts, jssrc

rethinkdb.http = aropt (url, opts) -> new Http opts, url

rethinkdb.json = (args...) -> new Json {}, args...

rethinkdb.error = (args...) -> new UserError {}, args...

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

rethinkdb.binary = ar (data) -> new Binary data

rethinkdb.row = new ImplicitVar {}

rethinkdb.table = aropt (tblName, opts) -> new Table opts, tblName

rethinkdb.db = (args...) -> new Db {}, args...

rethinkdb.dbCreate = (args...) -> new DbCreate {}, args...
rethinkdb.dbDrop = (args...) -> new DbDrop {}, args...
rethinkdb.dbList = (args...) -> new DbList {}, args...

rethinkdb.tableCreate = aropt (tblName, opts) -> new TableCreate opts, tblName
rethinkdb.tableDrop = (args...) -> new TableDrop {}, args...
rethinkdb.tableList = (args...) -> new TableList {}, args...

rethinkdb.grant = (args...) -> new Grant {}, args...

rethinkdb.do = varar 1, null, (args...) ->
    new FunCall {}, funcWrap(args[-1..][0]), args[...-1]...

rethinkdb.branch = (args...) -> new Branch {}, args...
rethinkdb.map = varar 1, null, (args..., funcArg) -> new Map {}, args..., funcWrap(funcArg)

rethinkdb.group = (args...) -> new Group {}, args.map(funcWrap)...
rethinkdb.reduce = (args...) -> new Reduce {}, args.map(funcWrap)...
rethinkdb.count = (args...) -> new Count {}, args.map(funcWrap)...
rethinkdb.sum = (args...) -> new Sum {}, args.map(funcWrap)...
rethinkdb.avg = (args...) -> new Avg {}, args.map(funcWrap)...
rethinkdb.min = (args...) -> new Min {}, args.map(funcWrap)...
rethinkdb.max = (args...) -> new Max {}, args.map(funcWrap)...
rethinkdb.distinct = (args...) -> new Distinct {}, args...
rethinkdb.contains = (args...) -> new Contains {}, args.map(funcWrap)...

rethinkdb.asc = (args...) -> new Asc {}, args.map(funcWrap)...
rethinkdb.desc = (args...) -> new Desc {}, args.map(funcWrap)...

rethinkdb.eq = (args...) -> new Eq {}, args...
rethinkdb.ne = (args...) -> new Ne {}, args...
rethinkdb.lt = (args...) -> new Lt {}, args...
rethinkdb.le = (args...) -> new Le {}, args...
rethinkdb.gt = (args...) -> new Gt {}, args...
rethinkdb.ge = (args...) -> new Ge {}, args...
rethinkdb.or = (args...) -> new Or {}, args...
rethinkdb.and = (args...) -> new And {}, args...

rethinkdb.not = (args...) -> new Not {}, args...

rethinkdb.add = (args...) -> new Add {}, args...
rethinkdb.sub = (args...) -> new Sub {}, args...
rethinkdb.div = (args...) -> new Div {}, args...
rethinkdb.mul = (args...) -> new Mul {}, args...
rethinkdb.mod = (args...) -> new Mod {}, args...
rethinkdb.floor = (args...) -> new Floor {}, args...
rethinkdb.ceil = (args...) -> new Ceil {}, args...
rethinkdb.round = (args...) -> new Round {}, args...

rethinkdb.typeOf = (args...) -> new TypeOf {}, args...
rethinkdb.info = (args...) -> new Info {}, args...

rethinkdb.literal = (args...) -> new Literal {}, args...

rethinkdb.ISO8601 = aropt (str, opts) -> new ISO8601 opts, str
rethinkdb.epochTime = (args...) -> new EpochTime {}, args...
rethinkdb.now = (args...) -> new Now {}, args...
rethinkdb.time = (args...) -> new Time {}, args...

rethinkdb.monday = new (class extends RDBConstant then tt: protoTermType.MONDAY, st: 'monday')()
rethinkdb.tuesday = new (class extends RDBConstant then tt: protoTermType.TUESDAY, st: 'tuesday')()
rethinkdb.wednesday = new (class extends RDBConstant then tt: protoTermType.WEDNESDAY, st: 'wednesday')()
rethinkdb.thursday = new (class extends RDBConstant then tt: protoTermType.THURSDAY, st: 'thursday')()
rethinkdb.friday = new (class extends RDBConstant then tt: protoTermType.FRIDAY, st: 'friday')()
rethinkdb.saturday = new (class extends RDBConstant then tt: protoTermType.SATURDAY, st: 'saturday')()
rethinkdb.sunday = new (class extends RDBConstant then tt: protoTermType.SUNDAY, st: 'sunday')()

rethinkdb.january = new (class extends RDBConstant then tt: protoTermType.JANUARY, st: 'january')()
rethinkdb.february = new (class extends RDBConstant then tt: protoTermType.FEBRUARY, st: 'february')()
rethinkdb.march = new (class extends RDBConstant then tt: protoTermType.MARCH, st: 'march')()
rethinkdb.april = new (class extends RDBConstant then tt: protoTermType.APRIL, st: 'april')()
rethinkdb.may = new (class extends RDBConstant then tt: protoTermType.MAY, st: 'may')()
rethinkdb.june = new (class extends RDBConstant then tt: protoTermType.JUNE, st: 'june')()
rethinkdb.july = new (class extends RDBConstant then tt: protoTermType.JULY, st: 'july')()
rethinkdb.august = new (class extends RDBConstant then tt: protoTermType.AUGUST, st: 'august')()
rethinkdb.september = new (class extends RDBConstant then tt: protoTermType.SEPTEMBER, st: 'september')()
rethinkdb.october = new (class extends RDBConstant then tt: protoTermType.OCTOBER, st: 'october')()
rethinkdb.november = new (class extends RDBConstant then tt: protoTermType.NOVEMBER, st: 'november')()
rethinkdb.december = new (class extends RDBConstant then tt: protoTermType.DECEMBER, st: 'december')()

rethinkdb.minval = new (class extends RDBConstant then tt: protoTermType.MINVAL, st: 'minval')()
rethinkdb.maxval = new (class extends RDBConstant then tt: protoTermType.MAXVAL, st: 'maxval')()

rethinkdb.object = (args...) -> new Object_ {}, args...

rethinkdb.args = (args...) -> new Args {}, args...

rethinkdb.geojson = (args...) -> new Geojson {}, args...
rethinkdb.point = (args...) -> new Point {}, args...
rethinkdb.line = (args...) -> new Line {}, args...
rethinkdb.polygon = (args...) -> new Polygon {}, args...
rethinkdb.intersects = (args...) -> new Intersects {}, args...
rethinkdb.distance = aropt (g1, g2, opts) -> new Distance opts, g1, g2
rethinkdb.circle = aropt (cen, rad, opts) -> new Circle opts, cen, rad

rethinkdb.uuid = (args...) -> new UUID {}, args...

rethinkdb.range = (args...) -> new Range {}, args...

rethinkdb.union = (args...) -> new Union {}, args...

# Export all names defined on rethinkdb
module.exports = rethinkdb
