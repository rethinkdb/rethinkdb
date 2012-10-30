// Copyright 2010-2012 RethinkDB, all rights reserved.
/**
 * @fileoverview This file is somewhat of a hack, designed to be included
 * first by the dependency generator so that we can provide the top level
 * shortcut function.
 */

/**
 * A shortcut function for referencing ReQL variables and attributes.
 * @name rethinkdb
 * @namespace namespace for all rethinkdb functions and classes
 * @suppress {checkTypes}
 */
goog.global.rethinkdb = function(jsobj) {
    return rethinkdb.R(jsobj);
};

goog.provide('rethinkdbmdl');
goog.provide('rethinkdb.util');

/**
 * Internal utility for wrapping API function arguments
 * @param {*} val The value to wrap
 * @returns rethinkdb.Expression
 * @ignore
 */
rethinkdb.util.wrapIf_ = function(val) {
    if (val instanceof rethinkdb.Query) {
        return val;
    } else {
        return rethinkdb.expr(val);
    }
}

/**
 * Internal utility to wrap API function arguments that
 * are expected to be function expressions.
 * @param {*} fun
 * @returns {rethinkdb.FunctionExpression}
 * @ignore
 */
rethinkdb.util.functionWrap_ = function(fun) {
    if (fun instanceof rethinkdb.FunctionExpression) {
        return fun;
    } else {
        return rethinkdb.fn(fun);
    }
}

/**
 * Internal utility to enforce API types
 * @ignore
 */
rethinkdb.util.typeCheck_ = function(value, types) {
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
rethinkdb.util.argCheck_ = function(args, expected) {
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
rethinkdb.util.newExpr_ = function(var_args) {
    var constructor = arguments[0];
    var args = Array.prototype.slice.call(arguments, 1);

    var newObj = function(prop) {
        return rethinkdb.Expression.prototype.getAttr.call(
                        /**@type rethinkdb.Expression*/(newObj), prop);
    };

    newObj.__proto__ = constructor.prototype;
    constructor.apply(newObj, args);
    return newObj;
}
