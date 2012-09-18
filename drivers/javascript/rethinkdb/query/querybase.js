/**
 * @fileoverview This file is somewhat of a hack, designed to be included
 * first by the dependency generator so that we can provide the top level
 * shortcut function.
 */

/**
 * A shortcut function for referencing ReQL variables and attributes.
 * @name rethinkdb
 * @namespace namespace for all rethinkdb functions and classes
 */
goog.global.rethinkdb = function(jsobj) {
    return rethinkdb.R(jsobj);
};

goog.provide('rethinkdb');

/**
 * Internal utility for wrapping API function arguments
 * @param {*} val The value to wrap
 * @returns rethinkdb.Expression
 * @ignore
 */
function wrapIf_(val) {
    if (val instanceof rethinkdb.Expression) {
        return val;
    } else {
        return rethinkdb.expr(val);
    }
}

/**
 * Internal utility for wrapping API function arguments that
 * are expected to be function expressions.
 * @param {*} fun
        The function to wrap
 * @returns rethinkdb.FunctionExpression
 * @ignore
 */
function functionWrap_(fun) {
    if (fun instanceof rethinkdb.FunctionExpression) {
        // No wrap needed
    } else if (fun instanceof rethinkdb.Expression) {
        fun = rethinkdb.fn('', fun);
    } else if(typeof fun === 'function') {
        fun = rethinkdb.fn(fun);
    } else {
       throw TypeError("Argument expected to be a function expression");
    }

    return fun;
}

/**
 * Internal utility to enforce API types
 * @ignore
 */
function typeCheck_(value, types) {
    if (!value) return;

    var type_array = types;
    if (!(goog.isArray(types))) {
        type_array = [type_array];
    }

    if (!type_array.some(function(type) {
        return (typeof type === 'string') ? (typeof value === type) : (value instanceof type);
    })) {
        if (goog.isArray(types)) {
            throw new TypeError("Function argument "+value+" must be one of the types "+types);
        } else {
            throw new TypeError("Function argument "+value+" must be of the type "+types);
        }
    };
}

/**
 * Internal utility to verify min arg counts
 */
function argCheck_(args, expected) {
    if (args.length < expected) {
        throw new TypeError("Function requires at least "+expected+" argument"+
                            (expected > 1 ? 's.' : '.'));
    }
}

/**
 * Internal utility that creates new ReQL expression objects that are
 * themselves shortcut functions.
 * @param {...*} var_args
 * @ignore
 */
function newExpr_(var_args) {
    var constructor = arguments[0];
    var args = Array.prototype.slice.call(arguments, 1);

    var self = function(prop) {
        return rethinkdb.Expression.prototype.getAttr.call(self, prop);
    };

    self.__proto__ = constructor.prototype;
    constructor.apply(self, args);
    return self;
}
