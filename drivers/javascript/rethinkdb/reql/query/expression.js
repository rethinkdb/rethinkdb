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
 * @constructor
 */
rethinkdb.reql.query.JSONExpression = function(value) {
    this.value_ = json_value;
};
goog.inherits(rethinkdb.reql.query.JSONExpression, rethinkdb.reql.query.Expression);

/**
 * @return {Term}
 */
rethinkdb.reql.query.JSONExpression.prototype.compile = function() {
    var term = new Term();
    term.setTermType(Term.JSON);
    term.setJsonstring(JSON.stringify(this.value_));
    return term;
};

/**
 * @constructor
 */
rethinkdb.reql.query.AddExpression = function(left, right) {
    this.left_ = left;
    this.right_ = right;
};
goog.inherits(rethinkdb.reql.query.JSONExpression, rethinkdb.reql.query.Expression);

/**
 * @return {Term}
 */
rethinkdb.reql.query.AddExpression.prototype.compile = function() {
    var term = new Term();
    term.setTermType(Term.CALL);

    var call = new Call();
    var builtin = new Builtin();
    builtin.setBuiltinType(BUILTIN.BuiltinType.ADD);
    builtin.addArgs(this.left_.compile(), this.right.compile());
    call.setBuiltin(builtin);

    term.setCall(call);
    return term;
};

/**
 * @constructor
 */
rethinkdb.reql.query.LessThanExpression = function(left, right) {
    this.left_ = left;
    this.right_ = right;
};
goog.inherits(rethinkdb.reql.query.JSONExpression, rethinkdb.reql.query.Expression);

/**
 * @return {Term}
 */
rethinkdb.reql.query.LessThanExpression.prototype.compile = function() {
    var term = new Term();
    term.setTermType(Term.CALL);

    var call = new Call();
    var builtin = new Builtin();
    builtin.setBuiltinType(Builtin.BuiltinType.COMPARE);
    builtin.setComparison(Builtin.Comparison.LT);
    builtin.addArgs(this.left_.compile(), this.right.compile());
    call.setBuiltin(builtin);

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

/**
 * @export
 */
rethinkdb.reql.query.Expression.prototype.run = function(conn, callback) {
    conn = conn || rethinkdb.net.last_connection;
    conn.run(this, callback);
};

/**
 * @return {goog.proto2.Message}
 */
rethinkdb.reql.query.Expression.prototype.compile = function() {
    //TODO
};
