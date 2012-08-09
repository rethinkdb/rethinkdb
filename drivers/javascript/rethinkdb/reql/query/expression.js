// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.reql.query.Expression');

goog.provide('rethinkdb.reql.query.JSONExpression');

goog.require('rethinkdb.net');
goog.require('Query');

/**
 * @constructor
 */
rethinkdb.reql.query.Expression = function() {};

/**
 * @param {function()} callback The callback to invoke with the result.
 * @param {rethinkdb.net.Connection} conn The connection to run this expression on.
 */
rethinkdb.reql.query.Expression.prototype.run = function(callback, conn) {
    conn = conn || rethinkdb.net.last_connection;
    conn.run(this, callback);
};
goog.exportProperty(rethinkdb.reql.query.Expression.prototype, 'run',
    rethinkdb.reql.query.Expression.prototype.run);

/**
 * @return {!Term}
 */
rethinkdb.reql.query.Expression.prototype.compile = goog.abstractMethod;

/**
 * @constructor
 * @extends {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.JSONExpression = function(json_value) {
    this.value_ = json_value;
};
goog.inherits(rethinkdb.reql.query.JSONExpression, rethinkdb.reql.query.Expression);

/**
 * @override
 * @return {!Term}
 */
rethinkdb.reql.query.JSONExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.JSON);
    term.setJsonstring(JSON.stringify(this.value_));
    return term;
};

/**
 * @constructor
 * @extends {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.AddExpression = function(left, right) {
    this.left_ = left;
    this.right_ = right;
};
goog.inherits(rethinkdb.reql.query.AddExpression, rethinkdb.reql.query.Expression);

/**
 * @override
 * @return {!Term}
 */
rethinkdb.reql.query.AddExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.CALL);

    var call = new Term.Call();
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.ADD);
    call.setBuiltin(builtin);
    call.addArgs(this.left_.compile());
    call.addArgs(this.right_.compile());

    term.setCall(call);
    return term;
};

/**
 * @constructor
 * @extends {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.LessThanExpression = function(left, right) {
    this.left_ = left;
    this.right_ = right;
};
goog.inherits(rethinkdb.reql.query.LessThanExpression, rethinkdb.reql.query.Expression);

/**
 * @return {!Term}
 */
rethinkdb.reql.query.LessThanExpression.prototype.compile = function() {
    var term = new Term();
    term.setType(Term.TermType.CALL);

    var call = new Term.Call();
    var builtin = new Builtin();
    builtin.setType(Builtin.BuiltinType.COMPARE);
    builtin.setComparison(Builtin.Comparison.LT);
    call.setBuiltin(builtin);
    call.addArgs(this.left_.compile());
    call.addArgs(this.right_.compile());

    term.setCall(call);
    return term;
};

/**
 * @return {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.Expression.prototype.add = function(other) {
    return new rethinkdb.reql.query.AddExpression(this, other);
};

//TODO other arithmetic operators

/**
 * @return {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.Expression.prototype.lt = function(other) {
    return new rethinkdb.reql.query.LessThanExpression(this, other);
};

//TODO other comparison operators

/**
 * @return {rethinkdb.reql.query.Expression}
 */
rethinkdb.reql.query.Expression.prototype.filter = function(selector) {

};


