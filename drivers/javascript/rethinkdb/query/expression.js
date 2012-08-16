goog.provide('rethinkdb.query.Expression');

goog.require('rethinkdb.query');
goog.require('Query');

/**
 * @constructor
 */
rethinkdb.query.Expression = function() {};

/**
 * @param {function()} callback The callback to invoke with the result.
 * @param {rethinkdb.net.Connection} conn The connection to run this expression on.
 */
rethinkdb.query.Expression.prototype.run = function(callback, conn) {
    conn = conn || rethinkdb.net.last_connection;
    conn.run(this, callback);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'run',
    rethinkdb.query.Expression.prototype.run);

/** @return {!Term} */
rethinkdb.query.Expression.prototype.compile = goog.abstractMethod;

/** @return {!Query} */
rethinkdb.query.Expression.prototype.buildQuery = goog.abstractMethod;

/** 
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.ReadExpression = function() {};
goog.inherits(rethinkdb.query.ReadExpression, rethinkdb.query.Expression);

/**
 * @override
 * @return {!Query}
 */
rethinkdb.query.ReadExpression.prototype.buildQuery = function() {
    var readQuery = new ReadQuery();
    var term = this.compile();
    readQuery.setTerm(term);

    var query = new Query();
    query.setType(Query.QueryType.READ);
    query.setReadQuery(readQuery);

    return query;
};

/** 
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.WriteExpression = function() {};
goog.inherits(rethinkdb.query.WriteExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.WriteExpression.prototype.buildQuery = goog.abstractMethod;

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.JSONExpression = function(json_value) {
    this.value_ = json_value;
};
goog.inherits(rethinkdb.query.JSONExpression, rethinkdb.query.ReadExpression);

/**
 * @override
 * @return {!Term}
 */
rethinkdb.query.JSONExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.JSON);
    term.setJsonstring(JSON.stringify(this.value_));
    return term;
};

function makeBinaryConstructor() {
    /**
     * @constructor
     * @extends {rethinkdb.query.Expression}
     */
    return function(left, right) {
        this.left_ = left;
        this.right_ = right;   
    };
}

/**
 * @param {!Builtin.BuiltinType} builtinType
 * @param {Builtin.Comparison=} compareType
 */
function makeBinaryBuiltinCompile(builtinType, compareType) {
    /**
     * @override
     * @return {!Term}
     */
    return function() {
        var term = new Term();
        term.setType(Term.TermType.CALL);

        var call = new Term.Call();
        var builtin = new Builtin();
        builtin.setType(builtinType);
        if (compareType) builtin.setComparison(compareType);
        call.setBuiltin(builtin);
        call.addArgs(this.left_.compile());
        call.addArgs(this.right_.compile());

        term.setCall(call);
        return term;
    }
}

/** 
 * @param {Builtin.Comparison} compareType
 */
function makeCompareCompile(compareType) {
    return makeBinaryBuiltinCompile(Builtin.BuiltinType.COMPARE, compareType);
}

/**
 * @param {string} className
 * @param {Builtin.BuiltinType|Builtin.Comparison} builtinType
 * @param {string} chainName
 * @param {boolean=} comparison
 */
function makeBinary(className, builtinType, chainName, comparison) {

    // Constructor
    var newClass = rethinkdb.query[className] = makeBinaryConstructor();
    goog.inherits(newClass, rethinkdb.query.ReadExpression);

    // Compile method
    newClass.prototype.compile = comparison ? 
        makeCompareCompile(/**@type {Builtin.Comparison} */(builtinType)) :
        makeBinaryBuiltinCompile(/**@type {Builtin.BuiltinType} */(builtinType));

    // Chainable method on Expression
    rethinkdb.query.Expression.prototype[chainName] = function(other) {
        if (other instanceof rethinkdb.query.Expression)
            return new newClass(this, other);
        else
            return new newClass(this, rethinkdb.query.expr(other));
    };
}

/**
 * @param {string} className
 * @param {Builtin.Comparison} compareType
 * @param {string} chainName
 */
function makeComparison(className, compareType, chainName) {
    makeBinary(className, compareType, chainName, true);
}

makeBinary('AddExpression', Builtin.BuiltinType.ADD, 'add');
makeBinary('SubtractExpression', Builtin.BuiltinType.SUBTRACT, 'sub');
makeBinary('MultiplyExpression', Builtin.BuiltinType.MULTIPLY, 'mul');
makeBinary('DivideExpression', Builtin.BuiltinType.DIVIDE, 'div');
makeBinary('ModuloExpression', Builtin.BuiltinType.MODULO, 'mod');

makeComparison('EqualsExpression', Builtin.Comparison.EQ, 'eq');
makeComparison('NotEqualsExpression', Builtin.Comparison.NE, 'ne');
makeComparison('LessThanExpression', Builtin.Comparison.LT, 'lt');
makeComparison('LessThanOrEqualsExpression', Builtin.Comparison.LE, 'le');
makeComparison('GreaterThanExpression', Builtin.Comparison.GT, 'gt');
makeComparison('GreaterThanOrEqualsExpression', Builtin.Comparison.GE, 'ge');

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.LengthExpression = function(leftExpr) {
    this.leftExpr_ = leftExpr;
};
goog.inherits(rethinkdb.query.LengthExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.LengthExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.LENGTH);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());
    
    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

rethinkdb.query.Expression.prototype.length = function() {
    return new rethinkdb.query.LengthExpression(this);
};

/**
 * @param {...rethinkdb.query.Expression} var_args
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.BooleanExpression = function(var_args) {
    /**
     * Responsibility of subclasses to define
     * @type {Builtin.BuiltinType}
     * @private
     */
    this.builtinType_;

    /**
     * @type Array.<rethinkdb.query.Expression>
     * @private
     */
    this.predicates_ = [];

    for (var key in arguments) {
        if (arguments.hasOwnProperty(key)) {
            this.addPredicate(arguments[key]);
        }
    }
};
goog.inherits(rethinkdb.query.BooleanExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.BooleanExpression.prototype.addPredicate = function(predicate) {
    this.predicates_.push(predicate);
};

rethinkdb.query.BooleanExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(this.builtinType_);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    for (var i = 0; i < this.predicates_.length; i++) {
        call.addArgs(this.predicates_[i].compile());
    }

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @param {...rethinkdb.query.Expression} var_args
 * @constructor
 * @extends {rethinkdb.query.BooleanExpression}
 */
rethinkdb.query.AllExpression = function(var_args) {
    rethinkdb.query.BooleanExpression.apply(this, arguments);
    this.builtinType_ = Builtin.BuiltinType.ALL;
};
goog.inherits(rethinkdb.query.AllExpression, rethinkdb.query.BooleanExpression);

rethinkdb.query.Expression.prototype.and = function(predicate) {
    return new rethinkdb.query.AllExpression(this, predicate);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'and',
                    rethinkdb.query.Expression.prototype.and);

/**
 * @param {...rethinkdb.query.Expression} var_args
 * @constructor
 * @extends {rethinkdb.query.BooleanExpression}
 */
rethinkdb.query.AnyExpression = function(var_args) {
    rethinkdb.query.BooleanExpression.apply(this, arguments);
    this.builtinType_ = Builtin.BuiltinType.ANY;
};
goog.inherits(rethinkdb.query.AnyExpression, rethinkdb.query.BooleanExpression);

rethinkdb.query.Expression.prototype.or = function(predicate) {
    return new rethinkdb.query.AnyExpression(this, predicate);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'or',
                    rethinkdb.query.Expression.prototype.or);

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.RangeExpression = function(leftExpr,
                                           start_key,
                                           end_key,
                                           start_inclusive,
                                           end_inclusive) {
    this.leftExpr_ = leftExpr;
    this.startKey_ = start_key;
    this.endKey_ = end_key;
    this.startInclusive_ = (typeof start_inclusive === 'undefined') ?
                                false : start_inclusive;
    this.endInclusive_ = (typeof end_inclusive === 'undefined') ?
                                false : end_inclusive;
};
goog.inherits(rethinkdb.query.RangeExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.RangeExpression.prototype.compile = function() {
    var range = new Builtin.Range();
    range.setAttrname('id');
    range.setLowerbound(this.startKey_.compile());
    range.setUpperbound(this.endKey_.compile());

    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.RANGE);
    builtin.setRange(range);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @return {rethinkdb.query.Expression}
 * @param {*} start_key
 * @param {*} end_key
 * @param {boolean=} start_inclusive 
 * @param {boolean=} end_inclusive 
 */
rethinkdb.query.Expression.prototype.between =
        function(start_key, end_key, start_inclusive, end_inclusive) {
    return new rethinkdb.query.RangeExpression(this,
                                               start_key,
                                               end_key,
                                               start_inclusive,
                                               end_inclusive);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'between',
                    rethinkdb.query.Expression.prototype.between);

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {number} leftExtent
 * @param {number=} opt_rightExtent
 */
rethinkdb.query.SliceExpression = function(leftExpr, leftExtent, opt_rightExtent) {
    this.leftExpr_ = leftExpr;
    this.leftExtent_ = leftExtent;
    this.rightExtent_ = opt_rightExtent || null;
};
goog.inherits(rethinkdb.query.SliceExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.SliceExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.CALL);

    var call = new Term.Call();
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.SLICE);
    call.setBuiltin(builtin);

    call.addArgs(this.leftExpr_.compile());

    var leftExtent = new Term();
    leftExtent.setType(Term.TermType.NUMBER);
    leftExtent.setNumber(this.leftExtent_);
    call.addArgs(leftExtent);

    var rightExtent = new Term();
    if (this.rightExtent_ !== null) {
        rightExtent.setType(Term.TermType.NUMBER);
        rightExtent.setNumber(/**@type {number}*/this.rightExtent_);
    } else {
        rightExtent.setType(Term.TermType.JSON_NULL);
    }
    call.addArgs(rightExtent);

    term.setCall(call);

    return term;
};

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.slice = function(startIndex, opt_endIndex) {
    return new rethinkdb.query.SliceExpression(this, startIndex, opt_endIndex);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'slice',
    rethinkdb.query.Expression.prototype.slice);

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.limit = function(index) {
    return new rethinkdb.query.SliceExpression(this, 0, index);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'limit',
    rethinkdb.query.Expression.prototype.limit);

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.skip = function(index) {
    return new rethinkdb.query.SliceExpression(this, index);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'skip',
    rethinkdb.query.Expression.prototype.skip);

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {number} index
 */
rethinkdb.query.NthExpression = function(leftExpr, index) {
    this.leftExpr_ = leftExpr;
    this.index_ = index;
};
goog.inherits(rethinkdb.query.NthExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.NthExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.CALL);

    var call = new Term.Call();
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.NTH);
    call.setBuiltin(builtin);

    call.addArgs(this.leftExpr_.compile());

    var index = new Term();
    index.setType(Term.TermType.NUMBER);
    index.setNumber(this.index_);
    call.addArgs(index);

    term.setCall(call);

    return term;
};

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.nth = function(index) {
    return new rethinkdb.query.NthExpression(this, index);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'nth',
    rethinkdb.query.Expression.prototype.nth);

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.FilterExpression = function(leftExpr, predicate) {
    this.leftExpr_ = leftExpr;

    if (predicate instanceof rethinkdb.query.FunctionExpression) {
        this.predicateFunction_ = predicate;
    } else if (predicate instanceof rethinkdb.query.Expression) {
        this.predicateFunction_ = new rethinkdb.query.FunctionExpression([''], predicate);
    } else {
        var q = rethinkdb.query;
        var ands = new rethinkdb.query.AllExpression();
        for (var key in predicate) {
            if (predicate.hasOwnProperty(key)) {
                ands.addPredicate(q.R(key)['eq'](q.expr(predicate[key])));
            }
        }
        this.predicateFunction_ = new rethinkdb.query.FunctionExpression([''], ands);
    }
};
goog.inherits(rethinkdb.query.FilterExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.FilterExpression.prototype.compile = function() {
    var predicate = new Predicate();
    predicate.setArg(this.predicateFunction_.args[0]);
    predicate.setBody(this.predicateFunction_.body.compile());

    var filter = new Builtin.Filter();
    filter.setPredicate(predicate);

    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.FILTER);
    builtin.setFilter(filter);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.filter = function(selector) {
    return new rethinkdb.query.FilterExpression(this, selector);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'filter',
                    rethinkdb.query.Expression.prototype.filter);

/**
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {rethinkdb.query.FunctionExpression|rethinkdb.query.Expression} mapping
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.MapExpression = function(leftExpr, mapping) {
    this.leftExpr_ = leftExpr;

    if (mapping instanceof rethinkdb.query.FunctionExpression) {
        this.mappingFunction_ = mapping;
    } else if (mapping instanceof rethinkdb.query.Expression) {
        this.mappingFunction_ = rethinkdb.query.fn('', mapping);
    }
};
goog.inherits(rethinkdb.query.MapExpression, rethinkdb.query.ReadExpression);

rethinkdb.query.MapExpression.prototype.compile = function() {
    var mapping = new Mapping();
    mapping.setArg(this.mappingFunction_.args[0]);
    mapping.setBody(this.mappingFunction_.body.compile());

    var map = new Builtin.Map();
    map.setMapping(mapping);

    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.MAP);
    builtin.setMap(map);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

rethinkdb.query.Expression.prototype.map = function(mapping) {
    return new rethinkdb.query.MapExpression(this, mapping);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'map',
                    rethinkdb.query.Expression.prototype.map);

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.OrderByExpression = function(leftExpr, orderings) {
    this.leftExpr_ = leftExpr;
    this.orderings_ = orderings;
};
goog.inherits(rethinkdb.query.OrderByExpression, rethinkdb.query.ReadExpression);

/** @override */
rethinkdb.query.OrderByExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.ORDERBY);

    for (var i = 0; i < this.orderings_.length; i++) {
        var ascending = true;
        var attr = this.orderings_[i];

        if (attr[0] === '-') {
            ascending = false;
            attr = attr.slice(1);
        }

        var orderby = new Builtin.OrderBy();
        orderby.setAttr(attr);
        orderby.setAscending(ascending);

        builtin.addOrderBy(orderby);
    }

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @param {...string} var_args
 */
rethinkdb.query.Expression.prototype.orderby = function(var_args) {
    var orderings = Array.prototype.slice.call(arguments, 0);
    return new rethinkdb.query.OrderByExpression(this, orderings);
}
goog.exportProperty(rethinkdb.query.Expression.prototype, 'orderby',
                    rethinkdb.query.Expression.prototype.orderby);

/**
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {string=} opt_attr
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.DistinctExpression = function(leftExpr, opt_attr) {
    this.leftExpr_ = leftExpr;
    this.attr = opt_attr || null;
};
goog.inherits(rethinkdb.query.DistinctExpression, rethinkdb.query.ReadExpression);

/** @override */
rethinkdb.query.DistinctExpression.prototype.compile = function() {
    var leftExpr = this.leftExpr_;
    if (this.attr) {
        leftExpr = leftExpr.map(rethinkdb.query.R(this.attr));
    }

    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.DISTINCT);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(leftExpr.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @param {string=} opt_attr
 */
rethinkdb.query.Expression.prototype.distinct = function(opt_attr) {
    return new rethinkdb.query.DistinctExpression(this, opt_attr);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'distinct',
                    rethinkdb.query.Expression.prototype.distinct);

/**
 * @constructor
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.ReduceExpression = function(leftExpr, base, reduction) {
    this.leftExpr_ = leftExpr;
    this.base_ = base;
    this.reduction_ = reduction;
};
goog.inherits(rethinkdb.query.ReduceExpression, rethinkdb.query.ReadExpression);

/** @override */
rethinkdb.query.ReduceExpression.prototype.compile = function() {
    var reduction = new Reduction();
    reduction.setBase(this.base_.compile());
    reduction.setVar1(this.reduction_.args[0]);
    reduction.setVar2(this.reduction_.args[1]);
    reduction.setBody(this.reduction_.body.compile());

    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.REDUCE);
    builtin.setReduce(reduction);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

rethinkdb.query.Expression.prototype.reduce = function(base, reduction) {
    return new rethinkdb.query.ReduceExpression(this, base, reduction);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'reduce',
                    rethinkdb.query.Expression.prototype.reduce);
