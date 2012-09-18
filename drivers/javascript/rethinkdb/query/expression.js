goog.provide('rethinkdb.query.Expression');

goog.require('rethinkdb.query');
goog.require('rethinkdb.query2');
goog.require('Query');

/**
 * @class Base for all ReQL expression objects
 * @constructor
 * @extends {rethinkdb.query.ReadQuery}
 */
rethinkdb.query.Expression = function() { };
goog.inherits(rethinkdb.query.Expression, rethinkdb.query.ReadQuery);

/**
 * @function
 * @return {!Term}
 * @ignore
 */
rethinkdb.query.Expression.prototype.compile = goog.abstractMethod;

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.JSONExpression = function(json_value) {
    this.value_ = json_value;
};
goog.inherits(rethinkdb.query.JSONExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.JSONExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.JSON);
    term.setJsonstring(JSON.stringify(this.value_));
    return term;
};

/**
 * @constructor
 * @param {number=} number
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.NumberExpression = function(number) {
    this.value_ = (typeof number === 'undefined') ? null : number;
};
goog.inherits(rethinkdb.query.NumberExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.NumberExpression.prototype.compile = function() {
    var term = new Term();
    if (this.value_ === null) {
        term.setType(Term.TermType.JSON_NULL);
    } else {
        term.setType(Term.TermType.NUMBER);
        term.setNumber(this.value_);
    }

    return term;
};

/**
 * @constructor
 * @param {boolean} bool
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.BooleanExpression = function(bool) {
    this.value_ = bool;
};
goog.inherits(rethinkdb.query.BooleanExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.BooleanExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.BOOL);
    term.setValuebool(this.value_);

    return term;
};

/**
 * @constructor
 * @param {string} string
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.StringExpression = function(string) {
    this.value_ = string;
};
goog.inherits(rethinkdb.query.StringExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.StringExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.STRING);
    term.setValuestring(this.value_);

    return term;
};

/**
 * @constructor
 * @param {*} array
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.ArrayExpression = function(array) {
    this.value_ = [];

    for (var i = 0; i < array.length; i++) {
        var val = wrapIf_(array[i]);
        this.value_.push(val);
    }
};
goog.inherits(rethinkdb.query.ArrayExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.ArrayExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.ARRAY);

    for (var i = 0; i < this.value_.length; i++) {
        term.addArray(this.value_[i].compile());
    }

    return term;
};

/**
 * @constructor
 * @param {Object} object
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.ObjectExpression = function(object) {
    this.value_ = {};

    for (var key in object) {
        if (object.hasOwnProperty(key)) {
            var val = wrapIf_(object[key]);
            this.value_[key] = val;
        }
    }
};
goog.inherits(rethinkdb.query.ObjectExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.ObjectExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.OBJECT);

    for (var key in this.value_) {
        var tuple = new VarTermTuple();
        tuple.setVar(key);
        tuple.setTerm(this.value_[key].compile());

        term.addObject(tuple);
    }

    return term;
};

/**
 * @constructor
 * @param {string} expr
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.JSExpression = function(expr) {
    this.src_ = expr;
};
goog.inherits(rethinkdb.query.JSExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.JSExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.JAVASCRIPT);
    term.setJavascript(this.src_);

    return term;
};

/**
 * @param {Array.<string>} args
 * @param {rethinkdb.query.Expression} body
 * @constructor
 * @ignore
 */
rethinkdb.query.FunctionExpression = function(args, body) {
    /** @type {Array.<string>} */
    this.args = args;

    /** @type {rethinkdb.query.Expression} */
    this.body = body;
};

/**
 * @param {function(...)} fun
 * @constructor
 * @extends {rethinkdb.query.FunctionExpression}
 * @ignore
 */
rethinkdb.query.JSFunctionExpression = function(fun) {
    var match = rethinkdb.query.JSFunctionExpression.parseRegexp_.exec(fun.toString());
    if (!match) throw TypeError('Expected a function');

    var args = match[1].split(',').map(function(a){return a.trim()});
    var body = newExpr_(rethinkdb.query.JSExpression, match[2]);

    goog.base(this, args, body);
};
goog.inherits(rethinkdb.query.JSFunctionExpression, rethinkdb.query.FunctionExpression);

rethinkdb.query.JSFunctionExpression.parseRegexp_ = /function [^(]*\(([^)]*)\) *{([^]*)}/m;

/**
 * Constructs a ReQL expression representing a function of some number of variables.
 * This may be invoked with either some number of strings with a body given as a ReQL
 * expression or as a JavaScript function expression.
 * @param {...*} var_args The first N-1 args are strings giving the formal parameters
        of the function. The last gives the body of the function. Alternatively, a
        single JavaScript function expression may be provided.
 * @return {rethinkdb.query.FunctionExpression}
 * @export
 */
rethinkdb.query.fn = function(var_args) {
    var args;
    var body;
    if (arguments[arguments.length - 1] instanceof rethinkdb.query.Query) {
        body = arguments[arguments.length - 1];
        args = Array.prototype.slice.call(arguments, 0, arguments.length - 1);
    } else if (typeof var_args === 'function') {
        // generate arg names and get body by evaluating function
        // similar to ruby block syntax

        args = [];
        for (var i = 0; i < var_args.length; i++) {
            args.push('genSym'+args.length);
        }

        body = var_args.apply(null, args.map(function(argName) {
            return rethinkdb.query.R('$'+argName);
        }));
    }

    typeCheck_(body, rethinkdb.query.Query);
    args.forEach(function(arg) {return typeCheck_(arg, 'string')});
    return newExpr_(rethinkdb.query.FunctionExpression, args, body);
};

/**
 * @param {!Builtin.BuiltinType} builtinType
 * @param {!Array} args
 * @param {function(Builtin)=} opt_additional
 * @constructor
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.BuiltinExpression = function(builtinType, args, opt_additional) {
    this.builtinType_ = builtinType;
    this.args_ = args;
    this.additional_ = opt_additional || null;
};
goog.inherits(rethinkdb.query.BuiltinExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.BuiltinExpression.prototype.compile = function() {
    var builtin = new Builtin();
    builtin.setType(this.builtinType_);
    if (this.additional_) {
        this.additional_(builtin);
    }

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

/**
 * Shortcut to construct the many binary operator functions available in ReQL
 * @param {!Builtin.BuiltinType} builtinType
 * @param {!string} chainName
 * @ignore
 */
function makeBinary(builtinType, chainName) {
    rethinkdb.query.Expression.prototype[chainName] = function(other) {
        argCheck_(arguments, 1);
        other = wrapIf_(other);
        return newExpr_(rethinkdb.query.BuiltinExpression, builtinType, [this, other]);
    };
}

/**
 * Shortcut to construct the many binary comparison functions available in ReQL
 * @param {!Builtin.Comparison} comparison
 * @param {!string} chainName
 * @ignore
 */
function makeComparison(comparison, chainName) {
    rethinkdb.query.Expression.prototype[chainName] = function(other) {
        argCheck_(arguments, 1);
        other = wrapIf_(other);
        return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.COMPARE,
                [this, other], function(builtin) {
            builtin.setComparison(comparison);
        });
    };
}

/**
 * Arithmetic addition
 * @name rethinkdb.query.Expression.prototype.add
 * @function
 * @param {*} other The value to add to this one
 * @return {rethinkdb.query.Expression}
 */
makeBinary(Builtin.BuiltinType.ADD, 'add');

/**
 * Arithmetic subtraction
 * @name rethinkdb.query.Expression.prototype.sub
 * @function
 * @param {*} other The value to subtract from this one
 * @return {rethinkdb.query.Expression}
 */
makeBinary(Builtin.BuiltinType.SUBTRACT, 'sub');

/**
 * Arithmetic multiplication
 * @name rethinkdb.query.Expression.prototype.mul
 * @function
 * @param {*} other The value to multiply by this one
 * @return {rethinkdb.query.Expression}
 */
makeBinary(Builtin.BuiltinType.MULTIPLY, 'mul');

/**
 * Arithmetic division
 * @name rethinkdb.query.Expression.prototype.div
 * @function
 * @param {*} other The value to divide into this one
 * @return {rethinkdb.query.Expression}
 */
makeBinary(Builtin.BuiltinType.DIVIDE, 'div');

/**
 * Arithmetic modulo
 * @name rethinkdb.query.Expression.prototype.mod
 * @function
 * @param {*} other The modulus
 * @return {rethinkdb.query.Expression}
 */
makeBinary(Builtin.BuiltinType.MODULO, 'mod');


/**
 * Equality comparison
 * @name rethinkdb.query.Expression.prototype.eq
 * @function
 * @param {*} other
 * @return {rethinkdb.query.Expression}
 */
makeComparison(Builtin.Comparison.EQ, 'eq');

/**
 * Inverse equality comparison
 * @name rethinkdb.query.Expression.prototype.ne
 * @function
 * @param {*} other
 * @return {rethinkdb.query.Expression}
 */
makeComparison(Builtin.Comparison.NE, 'ne');

/**
 * Less than comparison
 * @name rethinkdb.query.Expression.prototype.lt
 * @function
 * @param {*} other
 * @return {rethinkdb.query.Expression}
 */
makeComparison(Builtin.Comparison.LT, 'lt');

/**
 * Less than or equals comparison
 * @name rethinkdb.query.Expression.prototype.le
 * @function
 * @param {*} other
 * @return {rethinkdb.query.Expression}
 */
makeComparison(Builtin.Comparison.LE, 'le');

/**
 * Greater than comparison
 * @name rethinkdb.query.Expression.prototype.gt
 * @function
 * @param {*} other
 * @return {rethinkdb.query.Expression}
 */
makeComparison(Builtin.Comparison.GT, 'gt');

/**
 * Greater than or equals comparison
 * @name rethinkdb.query.Expression.prototype.ge
 * @function
 * @param {*} other
 * @return {rethinkdb.query.Expression}
 */
makeComparison(Builtin.Comparison.GE, 'ge');

/**
 * Boolean inverse
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.not = function() {
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.NOT, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'not',
                    rethinkdb.query.Expression.prototype.not);

/**
 * Length of this sequence.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.count = function() {
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.LENGTH, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'count',
                    rethinkdb.query.Expression.prototype.count);

/**
 * Boolean and
 * @param {*} predicate
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.and = function(predicate) {
    argCheck_(arguments, 1);
    predicate = wrapIf_(predicate);
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.ALL, [this, predicate]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'and',
                    rethinkdb.query.Expression.prototype.and);

/**
 * Boolean or
 * @param {*} predicate
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.or = function(predicate) {
    argCheck_(arguments, 1);
    predicate = wrapIf_(predicate);
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.ANY,
                                                     [this, predicate]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'or',
                    rethinkdb.query.Expression.prototype.or);

/**
 * Grabs a range of rows between two primary key values
 * @return {rethinkdb.query.Expression}
 * @param {*} startKey The first key in the range, inclusive
 * @param {*} endKey The last key in the range, inclusive
 * @param {string=} opt_keyName
 */
rethinkdb.query.Expression.prototype.between =
        function(startKey, endKey, opt_keyName) {
    argCheck_(arguments, 2);

    startKey = wrapIf_(startKey);
    endKey = wrapIf_(endKey);
    typeCheck_(opt_keyName, 'string');
    var keyName = opt_keyName ? opt_keyName : 'id';

    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.RANGE, [this],
                    function(builtin) {
                        var range = new Builtin.Range();
                        range.setAttrname(keyName);
                        range.setLowerbound(startKey.compile());
                        range.setUpperbound(endKey.compile());
                        builtin.setRange(range);
                    });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'between',
                    rethinkdb.query.Expression.prototype.between);

/**
 * @constructor
 * @extends {rethinkdb.query.BuiltinExpression}
 * @param {rethinkdb.query.Expression} leftExpr
 * @param {number} leftExtent
 * @param {number=} opt_rightExtent
 * @ignore
 */
rethinkdb.query.SliceExpression = function(leftExpr, leftExtent, opt_rightExtent) {
    goog.base(this, Builtin.BuiltinType.SLICE, [
        leftExpr,
        newExpr_(rethinkdb.query.NumberExpression, leftExtent),
        newExpr_(rethinkdb.query.NumberExpression, opt_rightExtent)
    ]);
};
goog.inherits(rethinkdb.query.SliceExpression, rethinkdb.query.BuiltinExpression);

/**
 * Grab a slice of this sequence from start index to end index (if given) or the end of the sequence.
 * @param {number} startIndex First index to include
 * @param {number} opt_endIndex Last index to include
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.slice = function(startIndex, opt_endIndex) {
    argCheck_(arguments, 1);

    return newExpr_(rethinkdb.query.SliceExpression, this, startIndex, opt_endIndex);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'slice',
    rethinkdb.query.Expression.prototype.slice);

/**
 * Restrict the results to only the first limit elements. Equivalent to a right ended slice.
 * @param {number} limit The number of results to include
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.limit = function(limit) {
    argCheck_(arguments, 1);
    typeCheck_(limit, 'number');
    return newExpr_(rethinkdb.query.SliceExpression, this, 0, limit);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'limit',
    rethinkdb.query.Expression.prototype.limit);

/**
 * Skip the first hop elements of the result. Equivalent to a left ended slice.
 * @param {number} hop The number of elements to hop over before returning results.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.skip = function(hop) {
    argCheck_(arguments, 1);
    typeCheck_(hop, 'number');
    return newExpr_(rethinkdb.query.SliceExpression, this, hop);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'skip',
    rethinkdb.query.Expression.prototype.skip);

/**
 * Return the nth result of the sequence.
 * @param {number} index The index of the element to return.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.nth = function(index) {
    argCheck_(arguments, 1);
    typeCheck_(index, 'number');
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.NTH,
            [this, newExpr_(rethinkdb.query.NumberExpression, index)]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'nth',
    rethinkdb.query.Expression.prototype.nth);

/**
 * Filter this sequence based on the results of the given selector function as evaluated on each row.
 * @param {*} selector The selector can be given in any one of a number of formats. In each case,
 *  the result is a function expression that takes an element of this sequence and returns a boolean
 *  indicating true if the value is to be kept or false if it is to be filtered out. The
 *  possibilities are:
 *      A javascript function - this is sent directly to the server and may only refer to locally
 *                              bound variables, the implicit variable, or the single argument.
 *      A ReQL function expression - binding one variable and evaluating to a boolean.
 *      A ReQL expression - expected to reference the implicit variable and evaluate to a boolean.
 *      An object - shortcut for (@.key1 == val1) && (@.key2 == val2) etc.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.filter = function(selector) {
    var predicateFunction;
    if (selector instanceof rethinkdb.query.FunctionExpression) {
        predicateFunction = selector;
    } else if (selector instanceof rethinkdb.query.Expression) {
        predicateFunction = newExpr_(rethinkdb.query.FunctionExpression,[''],  selector);
    } else if (typeof selector === 'function') {
        predicateFunction = rethinkdb.query.fn(selector);
    } else if (typeof selector === 'object') {
        var ands = [];
        var q = rethinkdb.query;
        for (var key in selector) {
            if (selector.hasOwnProperty(key)) {
                ands.push(q.R(key)['eq'](q.expr(selector[key])));
            }
        }
        predicateFunction = newExpr_(rethinkdb.query.FunctionExpression,[''],
             newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.ALL, ands));
    } else {
        throw new TypeError("Filter selector of type "+ typeof(selector));
    }

    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.FILTER, [this],
        function(builtin) {
            var predicate = new Predicate();
            predicate.setArg(predicateFunction.args[0]);
            predicate.setBody(predicateFunction.body.compile());

            var filter = new Builtin.Filter();
            filter.setPredicate(predicate);
            builtin.setFilter(filter);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'filter',
                    rethinkdb.query.Expression.prototype.filter);

/**
 * Returns a new sequence that is the result of evaluating mapFun on each element of this sequence.
 * @param {*} mapFun The mapping function. This may be expressed in a number of ways.
 *  A ReQL function expression - binds one argument and evaluates to any ReQL value.
 *  Any ReQL expression - assumed to reference the implicit variable.
 *  A JavaScript function - this is sent directly to the server and may only refer to locally
 *                          bound variables, the implicit variable, or the single argument.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.map = function(mapFun) {
    mapFun = functionWrap_(mapFun);
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.MAP, [this],
        function(builtin) {
            var mapping = new Mapping();
            mapping.setArg(mapFun.args[0]);
            mapping.setBody(mapFun.body.compile());

            var map = new Builtin.Map();
            map.setMapping(mapping);

            builtin.setMap(map);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'map',
                    rethinkdb.query.Expression.prototype.map);

/**
 * Order the elements of the sequence by their values of the specified attributes.
 * @param {...string} var_args Some number of strings giving the fields to orderby.
 *  Values are first orderd by the first field given, then by the second, etc. Prefix
 *  a field name with a '-' to request a decending order. Attributes without prefixes
 *  will be given in ascending order.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.orderby = function(var_args) {
    argCheck_(arguments, 1);
    var orderings = Array.prototype.slice.call(arguments, 0);
    orderings.forEach(function(order) {typeCheck_(order, 'string')});

    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.ORDERBY, [this],
        function(builtin) {
            for (var i = 0; i < orderings.length; i++) {
                var ascending = true;
                var attr = orderings[i];

                if (attr[0] === '-') {
                    ascending = false;
                    attr = attr.slice(1);
                }

                var orderby = new Builtin.OrderBy();
                orderby.setAttr(attr);
                orderby.setAscending(ascending);

                builtin.addOrderBy(orderby);
            }
        });
}
goog.exportProperty(rethinkdb.query.Expression.prototype, 'orderby',
                    rethinkdb.query.Expression.prototype.orderby);

/**
 * Remove duplicate values in this sequence.
 * @param {string=} opt_attr If given, returns distinct values of just this attribute
 * @returns {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.distinct = function(opt_attr) {
    typeCheck_(opt_attr, 'string');
    var leftExpr = opt_attr ? this.map(rethinkdb.query.R(opt_attr)) : this;
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.DISTINCT, [leftExpr]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'distinct',
                    rethinkdb.query.Expression.prototype.distinct);

/**
 * Reduce this sequence to a single value by repeatedly applying the given reduction function.
 * @param {*} base The initial value to seed the accumulater. Must be of the same type as the
 *  the values of this sequence and the return type of the reduction function.
 * @param {*} reduce A function of two variables of the same type that returns a value of
 *  that type. May be expressed as a JavaScript function or a ReQL function expression.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.reduce = function(base, reduce) {
    argCheck_(arguments, 2);
    base = wrapIf_(base);
    reduce = functionWrap_(reduce);

    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.REDUCE, [this],
        function(builtin) {
            var reduction = new Reduction();
            reduction.setBase(base.compile());
            reduction.setVar1(reduce.args[0]);
            reduction.setVar2(reduce.args[1]);
            reduction.setBody(reduce.body.compile());

            builtin.setReduce(reduction);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'reduce',
                    rethinkdb.query.Expression.prototype.reduce);

/**
 * Divides the sequence into sets and then performs a map reduce per set.
 * @param {*} grouping A mapping function that returns a group id for each row.
 * @param {*} mapping The mapping function to apply on each set.
    See {@link rethinkdb.query.Expression#map}.
 * @param {*} base The base of the reduction. See {@link rethinkdb.query.Expression#reduce}.
 * @param {*} reduce The reduction function. See {@link rethinkdb.query.Expression#reduce}.o
 *  Each group is reduced separately to produce one final value per group.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.groupedMapReduce = function(grouping, mapping, base, reduce) {
    argCheck_(arguments, 4);
    grouping = functionWrap_(grouping);
    mapping  = functionWrap_(mapping);
    base     = wrapIf_(base);
    reduce   = functionWrap_(reduce);

    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.GROUPEDMAPREDUCE, [this],
        function(builtin) {
            var groupMapping = new Mapping();
            groupMapping.setArg(grouping.args[0]);
            groupMapping.setBody(grouping.body.compile());

            var valueMapping = new Mapping();
            valueMapping.setArg(mapping.args[0]);
            valueMapping.setBody(mapping.body.compile());

            var reduction = new Reduction();
            reduction.setBase(base.compile());
            reduction.setVar1(reduce.args[0]);
            reduction.setVar2(reduce.args[1]);
            reduction.setBody(reduce.body.compile());

            var groupedMapReduce = new Builtin.GroupedMapReduce();
            groupedMapReduce.setGroupMapping(groupMapping);
            groupedMapReduce.setValueMapping(valueMapping);
            groupedMapReduce.setReduction(reduction);

            builtin.setGroupedMapReduce(groupedMapReduce);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'groupedMapReduce',
                    rethinkdb.query.Expression.prototype.groupedMapReduce);

/**
 * Returns true if this has the given attribute.
 * @param {string} attr Attribute to test.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.hasAttr = function(attr) {
    typeCheck_(attr, 'string');
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.HASATTR, [this],
        function(builtin) {
            builtin.setAttr(attr);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'hasAttr',
                    rethinkdb.query.Expression.prototype.hasAttr);

/**
 * Returns the value of the given attribute from this.
 * @param {string} attr Attribute to get.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.getAttr = function(attr) {
    typeCheck_(attr, 'string');
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.GETATTR, [this],
        function(builtin) {
            builtin.setAttr(attr);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'getAttr',
                    rethinkdb.query.Expression.prototype.getAttr);

/**
 * Returns a new object containing only the requested attributes from this.
 * @param {string|Array.<string>} attrs An attribute to pick or a list of attributes to pick.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.pickAttrs = function(attrs) {
    if (!goog.isArray(attrs)) {
        attrs = [attrs];
    }
    attrs.forEach(function(attr){typeCheck_(attr, 'string')});
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.PICKATTRS, [this],
        function(builtin) {
            for (var key in attrs) {
                var attr = attrs[key];
                builtin.addAttrs(attr);
            }
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'pickAttrs',
                    rethinkdb.query.Expression.prototype.pickAttrs);

/**
 * Inverse of pickattrs. Returns an object consisting of all the attributes in this not
 * given by attrs.
 * @param {string|Array.<string>} attrs The attributes NOT to include in the result.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.without = function(attrs) {
    if (!goog.isArray(attrs)) {
        attrs = [attrs];
    }
    attrs.forEach(function(attr){typeCheck_(attr, 'string')});
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.WITHOUT, [this],
        function(builtin) {
            for (var key in attrs) {
                var attr = attrs[key];
                builtin.addAttrs(attr);
            }
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'without',
                    rethinkdb.query.Expression.prototype.without);

/**
 * Shortcut to map a pick attrs over a sequence.
 * @param {string|Array.<string>} attrs An attribute to pick or a list of attributes to pick.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.pluck = function(attrs) {
    return this.map(rethinkdb.query.fn('a', rethinkdb.query.R('$a').pickAttrs(attrs)));
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'pluck',
                    rethinkdb.query.Expression.prototype.pluck);

/**
 * @param {string} varName
 * @constructor
 * @extends {rethinkdb.query.Expression}
 * @ignore
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
 * @extends {rethinkdb.query.BuiltinExpression}
 * @ignore
 */
rethinkdb.query.AttrExpression = function(leftExpr, attrName) {
    goog.base(this, Builtin.BuiltinType.GETATTR, [leftExpr], function(builtin) {
        builtin.setAttr(attrName);
    });
};
goog.inherits(rethinkdb.query.AttrExpression, rethinkdb.query.BuiltinExpression);

/**
 * @param {string} attrName
 * @constructor
 * @extends {rethinkdb.query.BuiltinExpression}
 * @ignore
 */
rethinkdb.query.ImplicitAttrExpression = function(attrName) {
    goog.base(this, Builtin.BuiltinType.IMPLICIT_GETATTR, [], function(builtin) {
        builtin.setAttr(attrName);
    });
};
goog.inherits(rethinkdb.query.ImplicitAttrExpression, rethinkdb.query.BuiltinExpression);

/**
 * Reference the value of a bound variable or a field of a bound variable.
 * @param {string} varString The variable or field to reference. The special name @ references
 *  the implicit variable. If prefixed with a '$', references a free variable with that name.
 *  Otherwise references a field of the implicit variable. In any case, subfields can be
 *  referenced with dot notation.
 * @return {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.R = function(varString) {
    typeCheck_(varString, 'string');
    var attrChain = varString.split('.');

    var curName = attrChain.shift();
    var curExpr = null;
    if (curName[0] === '@') {
        curName = attrChain.shift();
    }

    if (curName[0] === '$') {
        curExpr = newExpr_(rethinkdb.query.VarExpression, curName.slice(1));
    } else {
        curExpr = newExpr_(rethinkdb.query.ImplicitAttrExpression, curName);
    }

    while (curName = attrChain.shift()) {
        curExpr = newExpr_(rethinkdb.query.AttrExpression, curExpr, curName);
    }

    return curExpr;
};

/**
 * Returns a new object this the union of the properties of this object with properties
 * of another. Properties of the same same give preference to other.
 * @param {*} other Some other object. Properties of other are given preference over the base
 *  object when conflicts exist.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.extend = function(other) {
    other = wrapIf_(other);
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.MAPMERGE, [this, other]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'extend',
                    rethinkdb.query.Expression.prototype.extend);

/**
 * For each element of this sequence evaluate mapFun and concat the resulting sequences.
 * @param {*} mapFun A JavaScript function, ReQL function expression or ReQL expression
 *  referencing the implicit variable that evaluates to a sequence.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.concatMap = function(mapFun) {
    mapFun = functionWrap_(mapFun);
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.CONCATMAP, [this],
        function(builtin) {
            var mapping = new Mapping();
            mapping.setArg(mapFun.args[0]);
            mapping.setBody(mapFun.body.compile());

            var concatmap = new Builtin.ConcatMap();
            concatmap.setMapping(mapping);

            builtin.setConcatMap(concatmap);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'concatMap',
                    rethinkdb.query.Expression.prototype.concatMap);

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.IfExpression = function(test, trueBranch, falseBranch) {
    this.test_ = test;
    this.trueBranch_ = trueBranch;
    this.falseBranch_ = falseBranch;
};
goog.inherits(rethinkdb.query.IfExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.IfExpression.prototype.compile = function() {
    var if_ = new Term.If();

    if_.setTest(this.test_.compile());
    if_.setTrueBranch(this.trueBranch_.compile());
    if_.setFalseBranch(this.falseBranch_.compile());

    var term = new Term();
    term.setType(Term.TermType.IF);
    term.setIf(if_);

    return term;
};

/**
 * Evaluate test, then evaluate and return the result of trueBranch if test evaluates to true,
 * or evaluate and return the result of falseBranch of test evaluates to false.
 * @param {rethinkdb.query.Expression} test ReQL expression that evaluates to a boolean.
 * @param {rethinkdb.query.Expression} trueBranch Expression to evaluate if true.
 * @param {rethinkdb.query.Expression} falseBranch Expression to evaluate if false.
 * @return {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.ifThenElse = function(test, trueBranch, falseBranch) {
    typeCheck_(trueBranch, rethinkdb.query.Query);
    typeCheck_(falseBranch, rethinkdb.query.Query);
    test = wrapIf_(test);
    return newExpr_(rethinkdb.query.IfExpression, test, trueBranch, falseBranch);
};

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 * @ignore
 */
rethinkdb.query.LetExpression = function(bindings, body) {
    this.bindings_ = bindings;
    this.body_ = body;
};
goog.inherits(rethinkdb.query.LetExpression, rethinkdb.query.Expression);

/** @override */
rethinkdb.query.LetExpression.prototype.compile = function() {
    var let_ = new Term.Let();
    for (var key in this.bindings_) {
        var binding = this.bindings_[key];

        var tuple = new VarTermTuple();
        tuple.setVar(binding[0]);
        tuple.setTerm(binding[1].compile());

        let_.addBinds(tuple);
    }
    let_.setExpr(this.body_.compile());

    var term = new Term();
    term.setType(Term.TermType.LET);
    term.setLet(let_);

    return term;
};

/**
 * Bind the result of an ReQL expression to a variable within an expression.
 * @param {...*} var_args The first N-1 arguments are expected to be tuples (expressed as
 *  JavaScript lists) of string ReQL expression pairs to bind. The last argument is a
 *  ReQL expression to evaluate within which those variables will be bound.
 * @return {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.let = function(var_args) {
    var bindings = Array.prototype.slice.call(arguments, 0, arguments.length - 1);
    var body = arguments[arguments.length - 1];

    if (!bindings.every(function(tuple) {
        return (typeof tuple[0] === 'string') && (tuple[1] instanceof rethinkdb.query.Expression);
    })) {
        throw new TypeError("Let bindings must be string, ReQL expression tuples");
    };
    typeCheck_(body, rethinkdb.query.Query);

    return newExpr_(rethinkdb.query.LetExpression, bindings, body);
};

/**
 * @constructor
 * @extends {rethinkdb.query.Query}
 * @ignore
 */
rethinkdb.query.ForEachQuery = function(leftExpr, fun) {
    this.leftExpr_ = leftExpr;
    this.fun_ = fun;
};
goog.inherits(rethinkdb.query.ForEachQuery, rethinkdb.query.Query);

/** @override */
rethinkdb.query.ForEachQuery.prototype.buildQuery = function() {
    var foreach = new WriteQuery.ForEach();
    foreach.setStream(this.leftExpr_.compile());
    foreach.setVar(this.fun_.args[0]);

    var sub = this.fun_.body.buildQuery().getWriteQuery();
    if (!sub) {
        throw new rethinkdb.errors.ClientError("For each requires a write query to execute");
    }

    foreach.addQueries(sub);

    var write = new WriteQuery();
    write.setType(WriteQuery.WriteQueryType.FOREACH);
    write.setForEach(foreach);

    var query = new Query();
    query.setType(Query.QueryType.WRITE);
    query.setWriteQuery(write);

    return query;
};

/**
 * Evaluate a ReQL query for each element of this sequence.
 * @param {rethinkdb.query.FunctionExpression|rethinkdb.query.Expression} fun A ReQL function
 *  expression binding a row or a ReQL expression relying on the implicit variable to evaluate
 *  for each row of this sequence.
 * @return {rethinkdb.query.Query}
 */
rethinkdb.query.Expression.prototype.forEach = function(fun) {
    fun = functionWrap_(fun);
    return newExpr_(rethinkdb.query.ForEachQuery, this, fun);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'forEach',
                    rethinkdb.query.Expression.prototype.forEach);

/**
 * Hack for demo
 */
rethinkdb.query.Expression.prototype.eqJoin = function(attr, table) {
    typeCheck_(attr, 'string');

    if (!(table instanceof rethinkdb.query.Table)) {
        table = rethinkdb.query.table(table);
    }

    var q = rethinkdb.query;
    return this.concatMap(q.fn('leftRow',
        q([table.get(q('$leftRow.'+attr))]).filter(q.fn('x',
            q('$x')['ne'](null))
        ).map(q.fn('rightRow',
            q('$rightRow').extend(q('$leftRow')))
        )
    ));
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'eqJoin',
                    rethinkdb.query.Expression.prototype.eqJoin);

/**
 * Convert a stream to an array.
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.streamToArray = function() {
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.STREAMTOARRAY, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'streamToArray',
                    rethinkdb.query.Expression.prototype.streamToArray);

/**
 * Convert an array to a stream
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.arrayToStream = function() {
    return newExpr_(rethinkdb.query.BuiltinExpression, Builtin.BuiltinType.ARRAYTOSTREAM, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'arrayToStream',
                    rethinkdb.query.Expression.prototype.arrayToStream);
