goog.provide('rethinkdb.Expression');

goog.require('rethinkdb');
goog.require('rethinkdb.query2');
goog.require('Query');

/**
 * @class Base for all ReQL expression objects
 * @constructor
 * @extends {rethinkdb.ReadQuery}
 */
rethinkdb.Expression = function() { };
goog.inherits(rethinkdb.Expression, rethinkdb.ReadQuery);

/**
 * @function
 * @return {!Term}
 * @ignore
 */
rethinkdb.Expression.prototype.compile = goog.abstractMethod;

/**
 * @class Generates a runtime error on the server
 * @constructor
 * @param {string=} opt_msg
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.ErrorExpression = function(opt_msg) {
    this.msg_ = opt_msg || "";
};
goog.inherits(rethinkdb.ErrorExpression, rethinkdb.Expression);

/** @override */
rethinkdb.ErrorExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.ERROR);
    term.setError(this.msg_);
    return term;
};

/** @override */
rethinkdb.ErrorExpression.prototype.formatQuery = function(bt) {
    if (!bt) {
        return "r.error()";
    } else {
        return "^^^^^^^^^";
    }
};

/**
 * Throw a runtime error on the server
 * @return {rethinkdb.Expression}
 * @export
 */
rethinkdb.error = function() {
    return new rethinkdb.ErrorExpression();
};

/**
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.JSONExpression = function(json_value) {
    this.value_ = json_value;
};
goog.inherits(rethinkdb.JSONExpression, rethinkdb.Expression);

/** @override */
rethinkdb.JSONExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.JSON);
    term.setJsonstring(JSON.stringify(this.value_));
    return term;
};

/**
 * @constructor
 * @param {number=} number
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.NumberExpression = function(number) {
    this.value_ = (typeof number === 'undefined') ? null : number;
};
goog.inherits(rethinkdb.NumberExpression, rethinkdb.Expression);

/** @override */
rethinkdb.NumberExpression.prototype.compile = function() {
    var term = new Term();
    if (this.value_ === null) {
        term.setType(Term.TermType.JSON_NULL);
    } else {
        term.setType(Term.TermType.NUMBER);
        term.setNumber(this.value_);
    }

    return term;
};

/** @override */
rethinkdb.NumberExpression.prototype.formatQuery = function(bt) {
    var result = "r.expr("+this.value_+")";
    if (!bt) {
        return result;
    } else {
        return carrotify_(result);
    }
};

/**
 * @constructor
 * @param {boolean} bool
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.BooleanExpression = function(bool) {
    this.value_ = bool;
};
goog.inherits(rethinkdb.BooleanExpression, rethinkdb.Expression);

/** @override */
rethinkdb.BooleanExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.BOOL);
    term.setValuebool(this.value_);

    return term;
};

/**
 * @constructor
 * @param {string} string
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.StringExpression = function(string) {
    this.value_ = string;
};
goog.inherits(rethinkdb.StringExpression, rethinkdb.Expression);

/** @override */
rethinkdb.StringExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.STRING);
    term.setValuestring(this.value_);

    return term;
};

/** @override */
rethinkdb.StringExpression.prototype.formatQuery = function(bt) {
    var result = "r.expr('"+this.value_+"')";
    if (!bt) {
        return result;
    } else {
        return carrotify_(result);
    }
};

/**
 * @constructor
 * @param {*} array
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.ArrayExpression = function(array) {
    this.value_ = [];

    for (var i = 0; i < array.length; i++) {
        var val = wrapIf_(array[i]);
        this.value_.push(val);
    }
};
goog.inherits(rethinkdb.ArrayExpression, rethinkdb.Expression);


/** @override */
rethinkdb.ArrayExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.ARRAY);

    for (var i = 0; i < this.value_.length; i++) {
        term.addArray(this.value_[i].compile());
    }

    return term;
};

/** @override */
rethinkdb.ArrayExpression.prototype.formatQuery = function(bt) {
    var strVals = this.value_.map(function(a) {return a.formatQuery()});
    if (!bt) {
        return 'r.expr(['+strVals.join(', ')+'])';
    } else {
        var a = bt[0].split(':');
        goog.asserts.assert(a[0] === 'elem');
        bt.shift();
        var elem = parseInt(a[1], 10);
        strVals = strVals.map(spaceify_);
        strVals[elem] = this.value_[elem].formatQuery(bt);
        return '        '+strVals.join('  ')+'  ';
    }
};

/**
 * @constructor
 * @param {Object} object
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.ObjectExpression = function(object) {
    this.value_ = {};

    for (var key in object) {
        if (object.hasOwnProperty(key)) {
            var val = wrapIf_(object[key]);
            this.value_[key] = val;
        }
    }
};
goog.inherits(rethinkdb.ObjectExpression, rethinkdb.Expression);

/** @override */
rethinkdb.ObjectExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.OBJECT);

    for (var key in this.value_) {
        if (this.value_.hasOwnProperty(key)) {
            var tuple = new VarTermTuple();
            tuple.setVar(key);
            tuple.setTerm(this.value_[key].compile());

            term.addObject(tuple);
        }
    }

    return term;
};

/** @override */
rethinkdb.ObjectExpression.prototype.formatQuery = function(bt) {
    if (!bt) {
        var results = [];
        for (var key in this.value_) {
            if (this.value_.hasOwnProperty(key)) {
                results.push("'"+key+"':"+this.value_[key].formatQuery());
            }
        }
        return "r.expr({"+results.join(', ')+"})";
    } else {
        var a = bt[0].split(':');
        bt.shift();
        goog.asserts.assert(a[0] === 'key');
        var badKey = a[1];
        var results = [];
        for (var key in this.value_) {
            if (this.value_.hasOwnProperty(key)) {
                if (key === badKey) {
                    results.push(spaceify_("'"+key+"':")+this.value_[key].formatQuery(bt));
                } else {
                    results.push(spaceify_("'"+key+"':"+this.value_[key].formatQuery()));
                }
            }
        }
        return "        "+results.join('  ')+"  ";
    }
};

/**
 * @constructor
 * @param {string} expr
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.JSExpression = function(expr) {
    this.src_ = expr;
};
goog.inherits(rethinkdb.JSExpression, rethinkdb.Expression);

/** @override */
rethinkdb.JSExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.JAVASCRIPT);
    term.setJavascript(this.src_);

    return term;
};

/**
 * @param {Array.<string>} args
 * @param {rethinkdb.Expression} body
 * @constructor
 * @ignore
 */
rethinkdb.FunctionExpression = function(args, body) {
    if (args.length < 1)
        args.push('');

    /** @type {Array.<string>} */
    this.args = args;

    /** @type {rethinkdb.Expression} */
    this.body = body;
};

rethinkdb.FunctionExpression.prototype.formatQuery = function(bt) {
    if(!bt) {
        return 'function('+this.args.join(', ')+') {return '+this.body.formatQuery()+'}';
    } else {
        return spaceify_('function('+this.args.join(', ')+') {return ')+
            this.body.formatQuery(bt)+' ';
    }
};

/**
 * @param {function(...)} fun
 * @constructor
 * @extends {rethinkdb.FunctionExpression}
 * @ignore
 */
rethinkdb.JSFunctionExpression = function(fun) {
    var match = rethinkdb.JSFunctionExpression.parseRegexp_.exec(fun.toString());
    if (!match) throw TypeError('Expected a function');

    var args = match[1].split(',').map(function(a){return a.trim()});
    var body = newExpr_(rethinkdb.JSExpression, match[2]);

    goog.base(this, args, body);
};
goog.inherits(rethinkdb.JSFunctionExpression, rethinkdb.FunctionExpression);

rethinkdb.JSFunctionExpression.parseRegexp_ = /function [^(]*\(([^)]*)\) *{([^]*)}/m;

/**
 * Not exported in favor of js function syntax. Still used internally though.
 *
 * Constructs a ReQL expression representing a function of some number of variables.
 * This may be invoked with either some number of strings with a body given as a ReQL
 * expression or as a JavaScript function expression.
 * @param {...*} var_args The first N-1 args are strings giving the formal parameters
        of the function. The last gives the body of the function. Alternatively, a
        single JavaScript function expression may be provided.
 * @return {rethinkdb.FunctionExpression}
 */
rethinkdb.fn = function(var_args) {
    var args;
    var body;
    if (typeof var_args === 'function' && !(var_args instanceof rethinkdb.Query)) {
        // generate arg names and get body by evaluating function
        // similar to ruby block syntax

        args = [];
        for (var i = 0; i < var_args.length; i++) {
            args.push('genSym'+args.length);
        }

        body = var_args.apply(null, args.map(function(argName) {
            return rethinkdb.letVar(argName);
        }));

        if (body === undefined) {
            throw new rethinkdb.errors.ClientError("ReQL function must return ReQL expression");
        }

    } else {
        body = arguments[arguments.length - 1];
        args = Array.prototype.slice.call(arguments, 0, arguments.length - 1);
    }

    body = wrapIf_(body);
    args.forEach(function(arg) {return typeCheck_(arg, 'string')});
    return newExpr_(rethinkdb.FunctionExpression, args, body);
};

/**
 * @param {!Builtin.BuiltinType} builtinType
 * @param {!Array} args
 * @param {function()} formatQuery
 * @param {function(Builtin)=} opt_additional
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.BuiltinExpression = function(builtinType, args, formatQuery,
        opt_additional) {
    this.builtinType_ = builtinType;
    this.args_ = args;
    this.additional_ = opt_additional || null;
    this.formatQuery = formatQuery;
};
goog.inherits(rethinkdb.BuiltinExpression, rethinkdb.Expression);

/** @override */
rethinkdb.BuiltinExpression.prototype.compile = function() {
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
    rethinkdb.Expression.prototype[chainName] = function(other) {
        argCheck_(arguments, 1);
        other = wrapIf_(other);
        return newExpr_(rethinkdb.BuiltinExpression, builtinType, [this, other],
            makeFormat_(chainName, this, other));
    };
}

/**
 * Shortcut to construct the many binary comparison functions available in ReQL
 * @param {!Builtin.Comparison} comparison
 * @param {!string} chainName
 * @ignore
 */
function makeComparison(comparison, chainName) {
    rethinkdb.Expression.prototype[chainName] = function(other) {
        argCheck_(arguments, 1);
        other = wrapIf_(other);
        return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.COMPARE,
                [this, other], makeFormat_(chainName, this, other),
            function(builtin) {
                builtin.setComparison(comparison);
            });
    };
}

/**
 * Arithmetic addition
 * @name rethinkdb.Expression.prototype.add
 * @function
 * @param {*} other The value to add to this one
 * @return {rethinkdb.Expression}
 */
makeBinary(Builtin.BuiltinType.ADD, 'add');

/**
 * Arithmetic subtraction
 * @name rethinkdb.Expression.prototype.sub
 * @function
 * @param {*} other The value to subtract from this one
 * @return {rethinkdb.Expression}
 */
makeBinary(Builtin.BuiltinType.SUBTRACT, 'sub');

/**
 * Arithmetic multiplication
 * @name rethinkdb.Expression.prototype.mul
 * @function
 * @param {*} other The value to multiply by this one
 * @return {rethinkdb.Expression}
 */
makeBinary(Builtin.BuiltinType.MULTIPLY, 'mul');

/**
 * Arithmetic division
 * @name rethinkdb.Expression.prototype.div
 * @function
 * @param {*} other The value to divide into this one
 * @return {rethinkdb.Expression}
 */
makeBinary(Builtin.BuiltinType.DIVIDE, 'div');

/**
 * Arithmetic modulo
 * @name rethinkdb.Expression.prototype.mod
 * @function
 * @param {*} other The modulus
 * @return {rethinkdb.Expression}
 */
makeBinary(Builtin.BuiltinType.MODULO, 'mod');


/**
 * Equality comparison
 * @name rethinkdb.Expression.prototype.eq
 * @function
 * @param {*} other
 * @return {rethinkdb.Expression}
 */
makeComparison(Builtin.Comparison.EQ, 'eq');

/**
 * Inverse equality comparison
 * @name rethinkdb.Expression.prototype.ne
 * @function
 * @param {*} other
 * @return {rethinkdb.Expression}
 */
makeComparison(Builtin.Comparison.NE, 'ne');

/**
 * Less than comparison
 * @name rethinkdb.Expression.prototype.lt
 * @function
 * @param {*} other
 * @return {rethinkdb.Expression}
 */
makeComparison(Builtin.Comparison.LT, 'lt');

/**
 * Less than or equals comparison
 * @name rethinkdb.Expression.prototype.le
 * @function
 * @param {*} other
 * @return {rethinkdb.Expression}
 */
makeComparison(Builtin.Comparison.LE, 'le');

/**
 * Greater than comparison
 * @name rethinkdb.Expression.prototype.gt
 * @function
 * @param {*} other
 * @return {rethinkdb.Expression}
 */
makeComparison(Builtin.Comparison.GT, 'gt');

/**
 * Greater than or equals comparison
 * @name rethinkdb.Expression.prototype.ge
 * @function
 * @param {*} other
 * @return {rethinkdb.Expression}
 */
makeComparison(Builtin.Comparison.GE, 'ge');

/**
 * Boolean inverse
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.not = function() {
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.NOT, [this],
        makeFormat_('not', this));
};
goog.exportProperty(rethinkdb.Expression.prototype, 'not',
                    rethinkdb.Expression.prototype.not);

/**
 * Length of this sequence.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.count = function() {
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.LENGTH,
        [this], makeFormat_('count', this));
};
goog.exportProperty(rethinkdb.Expression.prototype, 'count',
                    rethinkdb.Expression.prototype.count);

/**
 * Boolean and
 * @param {*} predicate
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.and = function(predicate) {
    argCheck_(arguments, 1);
    predicate = wrapIf_(predicate);
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.ALL, [this, predicate], makeFormat_('and', this, predicate));
};
goog.exportProperty(rethinkdb.Expression.prototype, 'and',
                    rethinkdb.Expression.prototype.and);

/**
 * Boolean or
 * @param {*} predicate
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.or = function(predicate) {
    argCheck_(arguments, 1);
    predicate = wrapIf_(predicate);
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.ANY,
        [this, predicate], makeFormat_('or', this, predicate));
};
goog.exportProperty(rethinkdb.Expression.prototype, 'or',
                    rethinkdb.Expression.prototype.or);

/**
 * Grabs a range of rows between two primary key values
 * @return {rethinkdb.Expression}
 * @param {*} startKey The first key in the range, inclusive
 * @param {*} endKey The last key in the range, inclusive
 * @param {string=} opt_keyName
 */
rethinkdb.Expression.prototype.between =
        function(startKey, endKey, opt_keyName) {
    argCheck_(arguments, 2);

    startKey = wrapIf_(startKey);
    endKey = wrapIf_(endKey);
    typeCheck_(opt_keyName, 'string');
    var keyName = opt_keyName ? opt_keyName : 'id';

    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.RANGE, [this],
        makeFormat_('between', this, startKey, endKey, keyName),
            function(builtin) {
                var range = new Builtin.Range();
                range.setAttrname(keyName);
                range.setLowerbound(startKey.compile());
                range.setUpperbound(endKey.compile());
                builtin.setRange(range);
            });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'between',
                    rethinkdb.Expression.prototype.between);

/**
 * Appends a value to the end of this array
 * @return {rethinkdb.Expression}
 * @param {*} val The value to append
 */
rethinkdb.Expression.prototype.append = function(val) {
    argCheck_(arguments, 1);
    val = wrapIf_(val);
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.ARRAYAPPEND,
        [this, val], makeFormat_('append', this, val));
};
goog.exportProperty(rethinkdb.Expression.prototype, 'append',
                    rethinkdb.Expression.prototype.append);

/**
 * @constructor
 * @extends {rethinkdb.BuiltinExpression}
 * @param {rethinkdb.Expression} leftExpr
 * @param {number|rethinkdb.Expression} leftExtent
 * @param {number|rethinkdb.Expression=} opt_rightExtent
 * @ignore
 */
rethinkdb.SliceExpression = function(leftExpr, leftExtent, opt_rightExtent) {
    leftExtent = wrapIf_(leftExtent);
    opt_rightExtent = opt_rightExtent ? wrapIf_(opt_rightExtent) :
        new rethinkdb.JSONExpression(null);

    goog.base(this, Builtin.BuiltinType.SLICE, [leftExpr, leftExtent, opt_rightExtent],
        function(bt) {
            if (!bt) {
                return leftExpr.formatQuery()+'.slice('+leftExtent.formatQuery()+
                    (opt_rightExtent ? ', '+opt_rightExtent.formatQuery() : '') +')';
            } else {
                var a = bt[0].split(':');
                goog.asserts.assert(a[0] === 'arg');
                a[1] = parseInt(a[1], 10);
                bt.shift();

                var it;
                if (a[1] === 0) {
                    return leftExpr.formatQuery(bt)+spaceify_('.slice('+leftExtent.formatQuery()+
                        (opt_rightExtent ? ', '+opt_rightExtent.formatQuery() : '') +')');
                } else if(a[1] === 1) {
                    return spaceify_(leftExpr.formatQuery()+'.slice(')+leftExtent.formatQuery(bt)+
                        spaceify_((opt_rightExtent ? ', '+opt_rightExtent.formatQuery() : '') +')');
                } else {
                    goog.asserts.assert(a[1] === [2]);
                    return spaceify_(leftExpr.formatQuery()+'.slice('+leftExtent.formatQuery(bt)+
                        ', ')+opt_rightExtent.formatQuery(bt)+' ';
                }
            }
        });
};
goog.inherits(rethinkdb.SliceExpression, rethinkdb.BuiltinExpression);

/**
 * Grab a slice of this sequence from start index to end index (if given) or the end of the sequence.
 * @param {number} startIndex First index to include
 * @param {number} opt_endIndex Last index to include
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.slice = function(startIndex, opt_endIndex) {
    argCheck_(arguments, 1);
    return newExpr_(rethinkdb.SliceExpression, this, startIndex, opt_endIndex);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'slice',
    rethinkdb.Expression.prototype.slice);

/**
 * Restrict the results to only the first limit elements. Equivalent to a right ended slice.
 * @param {number} limit The number of results to include
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.limit = function(limit) {
    argCheck_(arguments, 1);
    return newExpr_(rethinkdb.SliceExpression, this, 0, limit);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'limit',
    rethinkdb.Expression.prototype.limit);

/**
 * Skip the first hop elements of the result. Equivalent to a left ended slice.
 * @param {number} hop The number of elements to hop over before returning results.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.skip = function(hop) {
    argCheck_(arguments, 1);
    return newExpr_(rethinkdb.SliceExpression, this, hop);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'skip',
    rethinkdb.Expression.prototype.skip);

/**
 * Return the nth result of the sequence.
 * @param {number} index The index of the element to return.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.nth = function(index) {
    argCheck_(arguments, 1);
    index = wrapIf_(index);
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.NTH,
            [this, index], makeFormat_('nth', this, index));
};
goog.exportProperty(rethinkdb.Expression.prototype, 'nth',
    rethinkdb.Expression.prototype.nth);

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
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.filter = function(selector) {
    var predicateFunction;
    if (typeof selector === 'object' && !(selector instanceof rethinkdb.Query)) {
        var ands = [];
        for (var key in selector) {
            if (selector.hasOwnProperty(key)) {
                ands.push(rethinkdb.R(key)['eq'](rethinkdb.expr(selector[key])));
            }
        }
        predicateFunction = newExpr_(rethinkdb.FunctionExpression,[''],
             newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.ALL, ands,
                 function(bt) {
                    var fomatted = ands.map(function(a) {return a.formatQuery()});

                    if (!bt) {
                        return fomatted.join(').and(');
                    } else {
                        console.log(bt);
                        var a = bt[0].split(':');
                        goog.asserts.assert(a[0] === 'arg');
                        bt.shift();
                        fomatted = fomatted.map(spaceify_);
                        fomatted[a[1]] = ands[a[1]].formatQuery(bt);
                        return fomatted.join('      ');
                    }
                 }));
        /*
        predicateFunction.formatQuery =  function(bt) {
            var pairs = [];
            for (var key in selector) {
                if (selector.hasOwnProperty(key)) {
                    pairs.push("'"+key+"':"+selector[key].formatQuery());
                }
            }

            if (!bt) {
                return '{'+pairs.join(', ')+'}';
            } else {
                return '???';
            }
         };
         */
    } else {
        predicateFunction = functionWrap_(selector);
    }

    var self = this;
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.FILTER, [this],
        function(bt) {
            if (!bt) {
                return self.formatQuery()+'.filter('+predicateFunction.formatQuery()+')';
            } else {
                if (bt[0] === 'predicate') {
                    bt.shift();
                    return spaceify_(self.formatQuery()+'.filter(')+
                        predicateFunction.formatQuery(bt)+' ';
                } else {
                    goog.asserts.assert(bt[0] === 'arg:0');
                    bt.shift();
                    return self.formatQuery(bt)+spaceify_('.filter('+
                        predicateFunction.formatQuery(bt)+')');
                }
            }
        },
        function(builtin) {
            var predicate = new Predicate();
            predicate.setArg(predicateFunction.args[0]);
            predicate.setBody(predicateFunction.body.compile());

            var filter = new Builtin.Filter();
            filter.setPredicate(predicate);
            builtin.setFilter(filter);
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'filter',
                    rethinkdb.Expression.prototype.filter);

/**
 * Returns a new sequence that is the result of evaluating mapFun on each element of this sequence.
 * @param {*} mapFun The mapping function. This may be expressed in a number of ways.
 *  A ReQL function expression - binds one argument and evaluates to any ReQL value.
 *  Any ReQL expression - assumed to reference the implicit variable.
 *  A JavaScript function - this is sent directly to the server and may only refer to locally
 *                          bound variables, the implicit variable, or the single argument.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.map = function(mapFun) {
    mapFun = functionWrap_(mapFun);
    var self = this;
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.MAP, [this],
        function(bt) {
            if (!bt) {
                return self.formatQuery()+'.map('+mapFun.formatQuery()+')';
            } else {
                if (bt[0] === 'arg:0') {
                    bt.shift();
                    return self.formatQuery(bt)+spaceify_('.map('+mapFun.formatQuery(bt)+')');
                } else {
                    goog.asserts.assert(bt[0] === 'mapping');
                    bt.shift();
                    return spaceify_(self.formatQuery()+'.map(')+mapFun.formatQuery(bt)+' ';
                }
            }
        },
        function(builtin) {
            var mapping = new Mapping();
            mapping.setArg(mapFun.args[0]);
            mapping.setBody(mapFun.body.compile());

            var map = new Builtin.Map();
            map.setMapping(mapping);

            builtin.setMap(map);
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'map',
                    rethinkdb.Expression.prototype.map);

/**
 * Order the elements of the sequence by their values of the specified attributes.
 * @param {...string} var_args Some number of strings giving the fields to orderby.
 *  Values are first orderd by the first field given, then by the second, etc. Prefix
 *  a field name with a '-' to request a decending order. Attributes without prefixes
 *  will be given in ascending order.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.orderby = function(var_args) {
    argCheck_(arguments, 1);
    var orderings = Array.prototype.slice.call(arguments, 0);
    orderings.forEach(function(order) {typeCheck_(order, 'string')});

    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.ORDERBY, [this], formatTodo_,
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
goog.exportProperty(rethinkdb.Expression.prototype, 'orderby',
                    rethinkdb.Expression.prototype.orderby);

/**
 * Remove duplicate values in this sequence.
 * @param {string=} opt_attr If given, returns distinct values of just this attribute
 * @returns {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.distinct = function(opt_attr) {
    typeCheck_(opt_attr, 'string');
    var leftExpr = opt_attr ? this.map(rethinkdb.R(opt_attr)) : this;
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.DISTINCT, [leftExpr], formatTodo_);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'distinct',
                    rethinkdb.Expression.prototype.distinct);

/**
 * Reduce this sequence to a single value by repeatedly applying the given reduction function.
 * @param {*} base The initial value to seed the accumulater. Must be of the same type as the
 *  the values of this sequence and the return type of the reduction function.
 * @param {*} reduce A function of two variables of the same type that returns a value of
 *  that type. May be expressed as a JavaScript function or a ReQL function expression.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.reduce = function(base, reduce) {
    argCheck_(arguments, 2);
    base = wrapIf_(base);
    reduce = functionWrap_(reduce);

    var self = this;
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.REDUCE, [this],
        function(bt) {
            if(!bt) {
                return self.formatQuery()+'.reduce('+base.formatQuery()+', '
                    +reduce.formatQuery()+')';
            } else {
                if (bt[0] === 'reduce') {
                    bt.shift();
                    if (bt[0] === 'body') {
                        bt.shift();
                        return spaceify_(self.formatQuery()+
                            '.reduce('+base.formatQuery()+', ')+reduce.formatQuery(bt)+' ';
                    } else if (bt[0] === 'base') {
                        bt.shift();
                        return spaceify_(self.formatQuery()+'.reduce(')+base.formatQuery(bt)+
                               spaceify_(', '+reduce.formatQuery()+')');
                    }
                } else {
                    goog.asserts.assert(bt[0] === 'arg:0');
                    bt.shift();
                    return self.formatQuery(bt)+spaceify_('.reduce('+base.formatQuery()+', '
                        +reduce.formatQuery()+')');
                }
            }
        },
        function(builtin) {
            var reduction = new Reduction();
            reduction.setBase(base.compile());
            reduction.setVar1(reduce.args[0]);
            reduction.setVar2(reduce.args[1]);
            reduction.setBody(reduce.body.compile());

            builtin.setReduce(reduction);
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'reduce',
                    rethinkdb.Expression.prototype.reduce);

/**
 * Divides the sequence into sets and then performs a map reduce per set.
 * @param {*} grouping A mapping function that returns a group id for each row.
 * @param {*} mapping The mapping function to apply on each set.
    See {@link rethinkdb.Expression#map}.
 * @param {*} base The base of the reduction. See {@link rethinkdb.Expression#reduce}.
 * @param {*} reduce The reduction function. See {@link rethinkdb.Expression#reduce}.o
 *  Each group is reduced separately to produce one final value per group.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.groupedMapReduce = function(grouping, mapping, base, reduce) {
    argCheck_(arguments, 4);
    grouping = functionWrap_(grouping);
    mapping  = functionWrap_(mapping);
    base     = wrapIf_(base);
    reduce   = functionWrap_(reduce);

    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.GROUPEDMAPREDUCE, [this], formatTodo_,
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
goog.exportProperty(rethinkdb.Expression.prototype, 'groupedMapReduce',
                    rethinkdb.Expression.prototype.groupedMapReduce);

/**
 * Returns true if this has the given attribute.
 * @param {string} attr Attribute to test.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.hasAttr = function(attr) {
    typeCheck_(attr, 'string');
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.HASATTR, [this], formatTodo_,
        function(builtin) {
            builtin.setAttr(attr);
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'hasAttr',
                    rethinkdb.Expression.prototype.hasAttr);

/**
 * Returns the value of the given attribute from this.
 * @param {string} attr Attribute to get.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.getAttr = function(attr) {
    typeCheck_(attr, 'string');
    var self = this;
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.GETATTR, [this],
        function(bt) {
            if (!bt) {
                return self.formatQuery()+"('"+attr+"')";
            } else {
                if (bt[0] === 'arg:0') {
                    bt.shift();
                    return self.formatQuery(bt);
                } else {
                    goog.asserts.assert(bt[0] === 'attr');
                    return spaceify_(self.formatQuery())+carrotify_("('"+attr+"')");
                }
            }
        },
        function(builtin) {
            builtin.setAttr(attr);
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'getAttr',
                    rethinkdb.Expression.prototype.getAttr);

/**
 * Returns a new object containing only the requested attributes from this.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.pickAttrs = function() {
    var attrs = Array.prototype.slice.call(arguments, 0);
    if (!goog.isArray(attrs)) {
        attrs = [attrs];
    }
    attrs.forEach(function(attr){typeCheck_(attr, 'string');});
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.PICKATTRS, [this], formatTodo_,
        function(builtin) {
            for (var key in attrs) {
                var attr = attrs[key];
                builtin.addAttrs(attr);
            }
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'pickAttrs',
                    rethinkdb.Expression.prototype.pickAttrs);

/**
 * Inverse of pickattrs. Returns an object consisting of all the attributes in this not
 * given by attrs.
 * @param {string|Array.<string>} attrs The attributes NOT to include in the result.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.without = function(attrs) {
    if (!goog.isArray(attrs)) {
        attrs = [attrs];
    }
    attrs.forEach(function(attr){typeCheck_(attr, 'string')});
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.WITHOUT, [this], formatTodo_,
        function(builtin) {
            for (var key in attrs) {
                var attr = attrs[key];
                builtin.addAttrs(attr);
            }
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'without',
                    rethinkdb.Expression.prototype.without);

/**
 * Shortcut to map a pick attrs over a sequence.
 * table('foo').pluck('a', 'b') returns a stream of objects only with 'a' and 'b' attributes.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.pluck = function() {
    var args = Array.prototype.slice.call(arguments, 0);
    return this.map(function(a) {
        return rethinkdb.Expression.prototype.pickAttrs.apply(a, args)
    });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'pluck',
                    rethinkdb.Expression.prototype.pluck);

/**
 * @param {string} varName
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.VarExpression = function(varName) {
    this.varName_ = varName;
};
goog.inherits(rethinkdb.VarExpression, rethinkdb.Expression);

/** @override */
rethinkdb.VarExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.VAR);
    term.setVar(this.varName_);
    return term;
};

/** @override */
rethinkdb.VarExpression.prototype.formatQuery = function(bt) {
    var result = "r.letVar('"+this.varName_+"')";
    if (!bt) {
        return result;
    } else {
        return carrotify_(result);
    }
};

/**
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.ImplicitVarExpression = function() { };
goog.inherits(rethinkdb.ImplicitVarExpression, rethinkdb.Expression);

/** @override */
rethinkdb.ImplicitVarExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.IMPLICIT_VAR);
    return term;
};

/** @override */
rethinkdb.ImplicitVarExpression.prototype.formatQuery = function(bt) {
    if (!bt) {
        return "r.R('@')";
    } else {
        return "^^^^^^^^";
    }
};

/**
 * @param {rethinkdb.Expression} leftExpr
 * @param {string} attrName
 * @constructor
 * @extends {rethinkdb.BuiltinExpression}
 * @ignore
 */
rethinkdb.AttrExpression = function(leftExpr, attrName) {
    goog.base(this, Builtin.BuiltinType.GETATTR, [leftExpr], function(bt) {
        if (!bt) {
            return leftExpr.formatQuery()+"('"+attrName+"')";
        } else {
            console.log(bt);
            var arg = bt.shift();
            if (arg === 'arg:0') {
                return leftExpr.formatQuery(bt)+"('"+attrName+"')";
            } else {
                goog.asserts.assert(arg === 'attr');
                return spaceify_(leftExpr.formatQuery())+carrotify_("('"+attrName+"')");
            }
        }
    }, function(builtin) {
        builtin.setAttr(attrName);
    });
};
goog.inherits(rethinkdb.AttrExpression, rethinkdb.BuiltinExpression);

/**
 * Reference the value of a bound variable or a field of a bound variable.
 * @param {string} varString The variable or field to reference. The special name @ references
 *  the implicit variable. If prefixed with a '$', references a free variable with that name.
 *  Otherwise references a field of the implicit variable. In any case, subfields can be
 *  referenced with dot notation.
 * @return {rethinkdb.Expression}
 * @export
 */
rethinkdb.R = function(varString) {
    typeCheck_(varString, 'string');
    var attrChain = varString.split('.');

    var curName = attrChain.shift();

    // @attrName should be treated like @.attrName
    if (curName[0] === '@' && curName.length > 1) {
        attrChain.unshift(curName.slice(1));
    } else if (curName[0] !== '@') {
        attrChain.unshift(curName);
    }

    var curExpr = newExpr_(rethinkdb.ImplicitVarExpression);

    while (curName = attrChain.shift()) {
        curExpr = newExpr_(rethinkdb.AttrExpression, curExpr, curName);
    }

    return curExpr;
};

/**
 * Reference a variable bound by an enclosing let statement
 * @param {string} varString The variable to reference. subfields can be
 *  referenced with dot notation.
 * @param {string} variable.
 * @export
 */
rethinkdb.letVar = function(varString) {
    typeCheck_(varString, 'string');
    var attrChain = varString.split('.');

    var curName = attrChain.shift();
    var curExpr = newExpr_(rethinkdb.VarExpression, curName);
    while (curName = attrChain.shift()) {
        curExpr = newExpr_(rethinkdb.AttrExpression, curExpr, curName);
    }

    return curExpr;
};

/**
 * Returns a new object this the union of the properties of this object with properties
 * of another. Properties of the same same give preference to other.
 * @param {*} other Some other object. Properties of other are given preference over the base
 *  object when conflicts exist.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.extend = function(other) {
    other = wrapIf_(other);
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.MAPMERGE, [this, other], formatTodo_);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'extend',
                    rethinkdb.Expression.prototype.extend);

/**
 * For each element of this sequence evaluate mapFun and concat the resulting sequences.
 * @param {*} mapFun A JavaScript function, ReQL function expression or ReQL expression
 *  referencing the implicit variable that evaluates to a sequence.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.concatMap = function(mapFun) {
    mapFun = functionWrap_(mapFun);
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.CONCATMAP, [this], formatTodo_,
        function(builtin) {
            var mapping = new Mapping();
            mapping.setArg(mapFun.args[0]);
            mapping.setBody(mapFun.body.compile());

            var concatmap = new Builtin.ConcatMap();
            concatmap.setMapping(mapping);

            builtin.setConcatMap(concatmap);
        });
};
goog.exportProperty(rethinkdb.Expression.prototype, 'concatMap',
                    rethinkdb.Expression.prototype.concatMap);

/**
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.IfExpression = function(test, trueBranch, falseBranch) {
    this.test_ = test;
    this.trueBranch_ = trueBranch;
    this.falseBranch_ = falseBranch;
};
goog.inherits(rethinkdb.IfExpression, rethinkdb.Expression);

/** @override */
rethinkdb.IfExpression.prototype.compile = function() {
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
 * @param {rethinkdb.Expression} test ReQL expression that evaluates to a boolean.
 * @param {rethinkdb.Expression} trueBranch Expression to evaluate if true.
 * @param {rethinkdb.Expression} falseBranch Expression to evaluate if false.
 * @return {rethinkdb.Expression}
 * @export
 */
rethinkdb.ifThenElse = function(test, trueBranch, falseBranch) {
    typeCheck_(trueBranch, rethinkdb.Query);
    typeCheck_(falseBranch, rethinkdb.Query);
    test = wrapIf_(test);
    return newExpr_(rethinkdb.IfExpression, test, trueBranch, falseBranch);
};

/**
 * @constructor
 * @extends {rethinkdb.Expression}
 * @ignore
 */
rethinkdb.LetExpression = function(bindings, body) {
    this.bindings_ = bindings;
    this.body_ = body;
};
goog.inherits(rethinkdb.LetExpression, rethinkdb.Expression);

/** @override */
rethinkdb.LetExpression.prototype.compile = function() {
    var let_ = new Term.Let();

    for (var varName in this.bindings_) {
        if (this.bindings_.hasOwnProperty(varName)) {
            var expr = this.bindings_[varName];

            var tuple = new VarTermTuple();
            tuple.setVar(varName);
            tuple.setTerm(expr.compile());

            let_.addBinds(tuple);
        }
    }
    let_.setExpr(this.body_.compile());

    var term = new Term();
    term.setType(Term.TermType.LET);
    term.setLet(let_);

    return term;
};

/**
 * Bind the result of an ReQL expression to a variable within an expression.
 * @param {Object} bindings An object giving name:expression pairs.
 * @param {rethinkdb.Query} body The ReQL query to evauluate in context.
 * @return {rethinkdb.Expression}
 * @export
 */
rethinkdb.let = function(bindings, body) {
    typeCheck_(body, rethinkdb.Query);

    for (var key in bindings) {
        if (bindings.hasOwnProperty(key)) {
            typeCheck_(bindings[key], rethinkdb.Expression);
        }
    }

    return newExpr_(rethinkdb.LetExpression, bindings, body);
};

/**
 * @constructor
 * @extends {rethinkdb.Query}
 * @ignore
 */
rethinkdb.ForEachQuery = function(leftExpr, fun) {
    this.leftExpr_ = leftExpr;
    this.fun_ = fun;
};
goog.inherits(rethinkdb.ForEachQuery, rethinkdb.Query);

/** @override */
rethinkdb.ForEachQuery.prototype.buildQuery = function() {
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
 * @param {rethinkdb.FunctionExpression|rethinkdb.Expression} fun A ReQL function
 *  expression binding a row or a ReQL expression relying on the implicit variable to evaluate
 *  for each row of this sequence.
 * @return {rethinkdb.Query}
 */
rethinkdb.Expression.prototype.forEach = function(fun) {
    fun = functionWrap_(fun);
    return newExpr_(rethinkdb.ForEachQuery, this, fun);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'forEach',
                    rethinkdb.Expression.prototype.forEach);

/**
 * Convert a stream to an array.
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.streamToArray = function() {
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.STREAMTOARRAY, [this], formatTodo_);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'streamToArray',
                    rethinkdb.Expression.prototype.streamToArray);

/**
 * Convert an array to a stream
 * @return {rethinkdb.Expression}
 */
rethinkdb.Expression.prototype.arrayToStream = function() {
    return newExpr_(rethinkdb.BuiltinExpression, Builtin.BuiltinType.ARRAYTOSTREAM, [this], formatTodo_);
};
goog.exportProperty(rethinkdb.Expression.prototype, 'arrayToStream',
                    rethinkdb.Expression.prototype.arrayToStream);
