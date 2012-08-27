goog.provide('rethinkdb.query.Expression');

goog.require('rethinkdb.query');
goog.require('rethinkdb.query2');
goog.require('Query');

/**
 * @constructor
 * @extends {rethinkdb.query.ReadQuery}
 */
rethinkdb.query.Expression = function() { };
goog.inherits(rethinkdb.query.Expression, rethinkdb.query.ReadQuery);

/** @return {!Term} */
rethinkdb.query.Expression.prototype.compile = goog.abstractMethod;

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.JSONExpression = function(json_value) {
    this.value_ = json_value;
};
goog.inherits(rethinkdb.query.JSONExpression, rethinkdb.query.Expression);

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

/**
 * @param {!Builtin.BuiltinType} builtinType
 * @param {!Array} args
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.BuiltinExpression = function(builtinType, args) {
    this.builtinType_ = builtinType;
    this.args_ = args;
};
goog.inherits(rethinkdb.query.BuiltinExpression, rethinkdb.query.Expression);

rethinkdb.query.BuiltinExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(this.builtinType_);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    for (var key in this.args_) {
        call.addArgs(this.args_[key].compile());
    }

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

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
    goog.inherits(newClass, rethinkdb.query.Expression);

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

rethinkdb.query.Expression.prototype.not = function() {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.NOT, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'not',
                    rethinkdb.query.Expression.prototype.not);

rethinkdb.query.Expression.prototype.length = function() {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.LENGTH, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'length',
                    rethinkdb.query.Expression.prototype.length);

rethinkdb.query.Expression.prototype.and = function(predicate) {
    if (predicate instanceof rethinkdb.query.Expression)
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ALL, [this, predicate]);
    else
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ALL,
                                                     [this, rethinkdb.query.expr(predicate)]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'and',
                    rethinkdb.query.Expression.prototype.and);

rethinkdb.query.Expression.prototype.or = function(predicate) {
    if (predicate instanceof rethinkdb.query.Expression)
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ANY,
                                                     [this, predicate]);
    else
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ANY,
                                                     [this, rethinkdb.query.expr(predicate)]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'or',
                    rethinkdb.query.Expression.prototype.or);

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
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
goog.inherits(rethinkdb.query.RangeExpression, rethinkdb.query.Expression);

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
 * @extends {rethinkdb.query.Expression}
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {number} leftExtent
 * @param {number=} opt_rightExtent
 */
rethinkdb.query.SliceExpression = function(leftExpr, leftExtent, opt_rightExtent) {
    this.leftExpr_ = leftExpr;
    this.leftExtent_ = leftExtent;
    this.rightExtent_ = opt_rightExtent || null;
};
goog.inherits(rethinkdb.query.SliceExpression, rethinkdb.query.Expression);

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
 * @extends {rethinkdb.query.Expression}
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {number} index
 */
rethinkdb.query.NthExpression = function(leftExpr, index) {
    this.leftExpr_ = leftExpr;
    this.index_ = index;
};
goog.inherits(rethinkdb.query.NthExpression, rethinkdb.query.Expression);

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
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.FilterExpression = function(leftExpr, predicate) {
    this.leftExpr_ = leftExpr;

    if (predicate instanceof rethinkdb.query.FunctionExpression) {
        this.predicateFunction_ = predicate;
    } else if (predicate instanceof rethinkdb.query.Expression) {
        this.predicateFunction_ = new rethinkdb.query.FunctionExpression([''], predicate);
    } else {
        var ands = [];
        var q = rethinkdb.query;
        for (var key in predicate) {
            if (predicate.hasOwnProperty(key)) {
                ands.push(q.R(key)['eq'](q.expr(predicate[key])));
            }
        }
        this.predicateFunction_ = new rethinkdb.query.FunctionExpression([''],
            new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ALL, ands));
    }
};
goog.inherits(rethinkdb.query.FilterExpression, rethinkdb.query.Expression);

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
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.MapExpression = function(leftExpr, mapping) {
    this.leftExpr_ = leftExpr;

    if (mapping instanceof rethinkdb.query.FunctionExpression) {
        this.mappingFunction_ = mapping;
    } else if (mapping instanceof rethinkdb.query.Expression) {
        this.mappingFunction_ = rethinkdb.query.fn('', mapping);
    }
};
goog.inherits(rethinkdb.query.MapExpression, rethinkdb.query.Expression);

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
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.OrderByExpression = function(leftExpr, orderings) {
    this.leftExpr_ = leftExpr;
    this.orderings_ = orderings;
};
goog.inherits(rethinkdb.query.OrderByExpression, rethinkdb.query.Expression);

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
 * @param {string=} opt_attr
 */
rethinkdb.query.Expression.prototype.distinct = function(opt_attr) {
    var leftExpr = opt_attr ? this.map(rethinkdb.query.R(opt_attr)) : this;
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.DISTINCT, [leftExpr]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'distinct',
                    rethinkdb.query.Expression.prototype.distinct);

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.ReduceExpression = function(leftExpr, base, reduction) {
    this.leftExpr_ = leftExpr;
    this.base_ = base;
    this.reduction_ = reduction;
};
goog.inherits(rethinkdb.query.ReduceExpression, rethinkdb.query.Expression);

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

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.PluckExpression = function(leftExpr, attrs) {
    if (!goog.isArray(attrs)) {
        attrs = [attrs];
    }

    this.leftExpr_ = leftExpr;
    this.attrs_ = attrs;
};
goog.inherits(rethinkdb.query.PluckExpression, rethinkdb.query.Expression);

rethinkdb.query.PluckExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.IMPLICIT_PICKATTRS);

    for (var key in this.attrs_) {
        var attr = this.attrs_[key];
        builtin.addAttrs(attr);
    }

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

rethinkdb.query.Expression.prototype.pluck = function(attrs) {
    return new rethinkdb.query.PluckExpression(this, attrs);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'pluck',
                    rethinkdb.query.Expression.prototype.pluck);

/**
 * @param {Array.<string>} args
 * @param {rethinkdb.query.Expression} body
 * @constructor
 */
rethinkdb.query.FunctionExpression = function(args, body) {
    this.args = args;
    this.body = body;
};

/**
 * @param {...*} var_args
 * @return {rethinkdb.query.FunctionExpression}
 * @export
 */
rethinkdb.query.fn = function(var_args) {
    var body = arguments[arguments.length - 1];
    var args = Array.prototype.slice.call(arguments, 0, arguments.length - 1);

    return new rethinkdb.query.FunctionExpression(args, body);
};

/**
 * @param {string} varName
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.VarExpression = function(varName) {
    this.varName_ = varName;
};
goog.inherits(rethinkdb.query.VarExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.VarExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.VAR);
    term.setVar(this.varName_);
    return term;
};

/**
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {string} attrName
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.AttrExpression = function(leftExpr, attrName) {
    this.leftExpr_ = leftExpr;
    this.attrName_ = attrName;
};
goog.inherits(rethinkdb.query.AttrExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.AttrExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.GETATTR);
    builtin.setAttr(this.attrName_);

    var call = new Term.Call();
    call.setBuiltin(builtin);
    call.addArgs(this.leftExpr_.compile());

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @param {string} attrName
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.ImplicitAttrExpression = function(attrName) {
    this.attrName_ = attrName;
};
goog.inherits(rethinkdb.query.ImplicitAttrExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.ImplicitAttrExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.IMPLICIT_GETATTR);
    builtin.setAttr(this.attrName_);

    var call = new Term.Call();
    call.setBuiltin(builtin);

    var term = new Term();
    term.setType(Term.TermType.CALL);
    term.setCall(call);

    return term;
};

/**
 * @return {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.R = function(varString) {
    var attrChain = varString.split('.');

    var curName = attrChain.shift();
    var curExpr = null;
    if (curName[0] === '$') {
        curExpr = new rethinkdb.query.VarExpression(curName.slice(1));
    } else {
        curExpr = new rethinkdb.query.ImplicitAttrExpression(curName);
    }

    while (curName = attrChain.shift()) {
        curExpr = new rethinkdb.query.AttrExpression(curExpr, curName);
    }

    return curExpr;
};

rethinkdb.query.Expression.prototype.extend = function(other) {
    if (!(other instanceof rethinkdb.query.Expression)) {
        other = rethinkdb.query.expr(other);
    }
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.MAPMERGE, [this, other]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'extend',
                    rethinkdb.query.Expression.prototype.extend);


