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
 * @constructor
 * @param {number=} number
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.NumberExpression = function(number) {
    this.value_ = (typeof number === 'undefined') ? null : number;
};
goog.inherits(rethinkdb.query.NumberExpression, rethinkdb.query.Expression);

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
 * @param {string} expr
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.JSExpression = function(expr) {
    this.src_ = expr;
};
goog.inherits(rethinkdb.query.JSExpression, rethinkdb.query.Expression);

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
 */
rethinkdb.query.JSFunctionExpression = function(fun) {
    var match = rethinkdb.query.JSFunctionExpression.parseRegexp_.exec(fun.toString());
    if (!match) throw TypeError('Expected a function');

    var args = match[1].split(',').map(function(a){return a.trim()});
    var body = new rethinkdb.query.JSExpression(match[2]);

    goog.base(this, args, body);
};
goog.inherits(rethinkdb.query.JSFunctionExpression, rethinkdb.query.FunctionExpression);

rethinkdb.query.JSFunctionExpression.parseRegexp_ = /function [^(]*\(([^)]*)\) *{([^]*)}/m;
//rethinkdb.query.JSFunctionExpression.parseRegexp_.compile();

/**
 * @param {...*} var_args
 * @return {rethinkdb.query.FunctionExpression}
 * @export
 */
rethinkdb.query.fn = function(var_args) {
    if (typeof var_args === 'function') {
        return new rethinkdb.query.JSFunctionExpression(var_args);
    } else {
        var body = arguments[arguments.length - 1];
        var args = Array.prototype.slice.call(arguments, 0, arguments.length - 1);

        return new rethinkdb.query.FunctionExpression(args, body);
    }
};

/**
 * @param {!Builtin.BuiltinType} builtinType
 * @param {!Array} args
 * @param {function(Builtin)=} opt_additional
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.BuiltinExpression = function(builtinType, args, opt_additional) {
    this.builtinType_ = builtinType;
    this.args_ = args;
    this.additional_ = opt_additional || null;
};
goog.inherits(rethinkdb.query.BuiltinExpression, rethinkdb.query.Expression);

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
 * @param {!Builtin.BuiltinType} builtinType
 * @param {!string} chainName
 */
function makeBinary(builtinType, chainName) {
    rethinkdb.query.Expression.prototype[chainName] = function(other) {
        if (!(other instanceof rethinkdb.query.Expression)) {
            other = rethinkdb.query.expr(other);
        }
        return new rethinkdb.query.BuiltinExpression(builtinType, [this, other]);
    };
}

/**
 * @param {!Builtin.Comparison} comparison
 * @param {!string} chainName
 */
function makeComparison(comparison, chainName) {
    rethinkdb.query.Expression.prototype[chainName] = function(other) {
        if (!(other instanceof rethinkdb.query.Expression)) {
            other = rethinkdb.query.expr(other);
        }
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.COMPARE,
                [this, other], function(builtin) {
            builtin.setComparison(comparison);
        });
    };
}

makeBinary(Builtin.BuiltinType.ADD, 'add');
makeBinary(Builtin.BuiltinType.SUBTRACT, 'sub');
makeBinary(Builtin.BuiltinType.MULTIPLY, 'mul');
makeBinary(Builtin.BuiltinType.DIVIDE, 'div');
makeBinary(Builtin.BuiltinType.MODULO, 'mod');

makeComparison(Builtin.Comparison.EQ, 'eq');
makeComparison(Builtin.Comparison.NE, 'ne');
makeComparison(Builtin.Comparison.LT, 'lt');
makeComparison(Builtin.Comparison.LE, 'le');
makeComparison(Builtin.Comparison.GT, 'gt');
makeComparison(Builtin.Comparison.GE, 'ge');

/**
 * Boolean inverse
 */
rethinkdb.query.Expression.prototype.not = function() {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.NOT, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'not',
                    rethinkdb.query.Expression.prototype.not);

/**
 * Length of array or stream
 */
rethinkdb.query.Expression.prototype.length = function() {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.LENGTH, [this]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'length',
                    rethinkdb.query.Expression.prototype.length);

/**
 * Boolean and
 */
rethinkdb.query.Expression.prototype.and = function(predicate) {
    if (predicate instanceof rethinkdb.query.Expression)
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ALL, [this, predicate]);
    else
        return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ALL,
                                                     [this, rethinkdb.query.expr(predicate)]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'and',
                    rethinkdb.query.Expression.prototype.and);

/**
 * Boolean or
 */
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
 * Grabs values between two keys
 * @return {rethinkdb.query.Expression}
 * @param {*} start_key
 * @param {*} end_key
 * @param {boolean=} start_inclusive
 * @param {boolean=} end_inclusive
 */
rethinkdb.query.Expression.prototype.between =
        function(start_key, end_key, start_inclusive, end_inclusive) {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.RANGE, [this],
                    function(builtin) {
                        var range = new Builtin.Range();
                        range.setAttrname('id');
                        range.setLowerbound(start_key.compile());
                        range.setUpperbound(end_key.compile());
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
 */
rethinkdb.query.SliceExpression = function(leftExpr, leftExtent, opt_rightExtent) {
    goog.base(this, Builtin.BuiltinType.SLICE, [
        leftExpr,
        new rethinkdb.query.NumberExpression(leftExtent),
        new rethinkdb.query.NumberExpression(opt_rightExtent)
    ]);
};
goog.inherits(rethinkdb.query.SliceExpression, rethinkdb.query.BuiltinExpression);

/**
 * Double ended slice
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.slice = function(startIndex, opt_endIndex) {
    return new rethinkdb.query.SliceExpression(this, startIndex, opt_endIndex);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'slice',
    rethinkdb.query.Expression.prototype.slice);

/**
 * Right ended slice
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.limit = function(index) {
    return new rethinkdb.query.SliceExpression(this, 0, index);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'limit',
    rethinkdb.query.Expression.prototype.limit);

/**
 * Left ended slice
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.skip = function(index) {
    return new rethinkdb.query.SliceExpression(this, index);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'skip',
    rethinkdb.query.Expression.prototype.skip);

/**
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.nth = function(index) {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.NTH,
            [this, new rethinkdb.query.NumberExpression(index)]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'nth',
    rethinkdb.query.Expression.prototype.nth);

/**
 * Filter a list or stream accoring to the selector
 * @return {rethinkdb.query.Expression}
 */
rethinkdb.query.Expression.prototype.filter = function(selector) {
    var predicateFunction;
    if (typeof selector === 'function') {
        predicateFunction = new rethinkdb.query.JSFunctionExpression(selector);
    } else if (selector instanceof rethinkdb.query.FunctionExpression) {
        predicateFunction = selector;
    } else if (selector instanceof rethinkdb.query.Expression) {
        predicateFunction = new rethinkdb.query.FunctionExpression([''], selector);
    } else {
        var ands = [];
        var q = rethinkdb.query;
        for (var key in selector) {
            if (selector.hasOwnProperty(key)) {
                ands.push(q.R(key)['eq'](q.expr(selector[key])));
            }
        }
        predicateFunction = new rethinkdb.query.FunctionExpression([''],
            new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ALL, ands));
    }

    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.FILTER, [this],
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
 * Map a function over a list or a stream
 */
rethinkdb.query.Expression.prototype.map = function(mapping) {
    var mappingFunction;
    if (mapping instanceof rethinkdb.query.FunctionExpression) {
        mappingFunction = mapping;
    } else if (mapping instanceof rethinkdb.query.Expression) {
        mappingFunction = rethinkdb.query.fn('', mapping);
    } else if(typeof mapping === 'function') {
        mappingFunction = rethinkdb.query.fn(mapping);
    } else {
        // invalid mapping
    }

    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.MAP, [this],
        function(builtin) {
            var mapping = new Mapping();
            mapping.setArg(mappingFunction.args[0]);
            mapping.setBody(mappingFunction.body.compile());

            var map = new Builtin.Map();
            map.setMapping(mapping);

            builtin.setMap(map);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'map',
                    rethinkdb.query.Expression.prototype.map);

/**
 * Order the stream by the value of the key
 * @param {...string} var_args
 */
rethinkdb.query.Expression.prototype.orderby = function(var_args) {
    var orderings = Array.prototype.slice.call(arguments, 0);

    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.ORDERBY, [this],
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
 * Remove duplicates
 * @param {string=} opt_attr
 */
rethinkdb.query.Expression.prototype.distinct = function(opt_attr) {
    var leftExpr = this; //opt_attr ? this.map(rethinkdb.query.R(opt_attr)) : this;
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.DISTINCT, [leftExpr]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'distinct',
                    rethinkdb.query.Expression.prototype.distinct);

/**
 * A reduction
 * @param {rethinkdb.query.Expression} base
 * @param {rethinkdb.query.FunctionExpression|function(...)} reduce
 */
rethinkdb.query.Expression.prototype.reduce = function(base, reduce) {
    if (!(base instanceof rethinkdb.query.Expression)) {
        base = rethinkdb.query.expr(base);
    }

    if (!(reduce instanceof rethinkdb.query.FunctionExpression)) {
        if (typeof reduce === 'function') {
            reduce = new rethinkdb.query.JSFunctionExpression(reduce);
        } else {
            throw TypeError('reduce argument expected to be a function');
        }
    }

    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.REDUCE, [this],
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
 * Returns true if expression has given attribute
 */
rethinkdb.query.Expression.prototype.hasAttr = function(attr) {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.HASATTR, [this],
        function(builtin) {
            builtin.setAttr(attr);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'hasAttr',
                    rethinkdb.query.Expression.prototype.hasAttr);

/**
 * Select only the given attribute
 */
rethinkdb.query.Expression.prototype.getAttr = function(attr) {
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.GETATTR, [this],
        function(builtin) {
            builtin.setAttr(attr);
        });
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'getAttr',
                    rethinkdb.query.Expression.prototype.getAttr);

/**
 * Select only the given attributes from an object
 */
rethinkdb.query.Expression.prototype.pickAttrs = function(attrs) {
    if (!goog.isArray(attrs)) {
        attrs = [attrs];
    }
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.PICKATTRS, [this],
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
 * Shortcut to map a pick attrs over stream
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
 */
rethinkdb.query.ImplicitAttrExpression = function(attrName) {
    goog.base(this, Builtin.BuiltinType.IMPLICIT_GETATTR, [], function(builtin) {
        builtin.setAttr(attrName);
    });
};
goog.inherits(rethinkdb.query.ImplicitAttrExpression, rethinkdb.query.BuiltinExpression);

/**
 * @return {rethinkdb.query.Expression}
 * @export
 */
rethinkdb.query.R = function(varString) {
    var attrChain = varString.split('.');

    var curName = attrChain.shift();
    var curExpr = null;
    if (curName[0] === '@') {
        curName = attrChain.shift();
    }

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

/**
 * Extend this object with properties from another
 */
rethinkdb.query.Expression.prototype.extend = function(other) {
    if (!(other instanceof rethinkdb.query.Expression)) {
        other = rethinkdb.query.expr(other);
    }
    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.MAPMERGE, [this, other]);
};
goog.exportProperty(rethinkdb.query.Expression.prototype, 'extend',
                    rethinkdb.query.Expression.prototype.extend);

/**
 * Apply mapping and then concat to an array.
 */
rethinkdb.query.Expression.prototype.concatMap = function(mapping) {
    var mappingFunction;
    if (mapping instanceof rethinkdb.query.FunctionExpression) {
        mappingFunction = mapping;
    } else if (mapping instanceof rethinkdb.query.Expression) {
        mappingFunction = rethinkdb.query.fn('', mapping);
    } else {
        // invalid mapping
    }

    return new rethinkdb.query.BuiltinExpression(Builtin.BuiltinType.CONCATMAP, [this],
        function(builtin) {
            var mapping = new Mapping();
            mapping.setArg(mappingFunction.args[0]);
            mapping.setBody(mappingFunction.body.compile());

            var concatmap = new Builtin.ConcatMap();
            concatmap.setMapping(mapping);

            builtin.setConcatMap(concatmap);
        });
};

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
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
 * If then else
 * @export
 */
rethinkdb.query.ifThenElse = function(test, trueBranch, falseBranch) {
    return new rethinkdb.query.IfExpression(test, trueBranch, falseBranch);
};

/**
 * @constructor
 * @extends {rethinkdb.query.Expression}
 */
rethinkdb.query.LetExpression = function(bindings, body) {
    this.bindings_ = bindings;
    this.body_ = body;
};
goog.inherits(rethinkdb.query.LetExpression, rethinkdb.query.Expression);

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
 * Bind values to variables in body
 * @export
 */
rethinkdb.query.let = function(var_args) {
    var bindings = Array.prototype.slice.call(arguments, 0, arguments.length - 1);
    var body = arguments[arguments.length - 1];
    return new rethinkdb.query.LetExpression(bindings, body);
};
