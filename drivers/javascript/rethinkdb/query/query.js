goog.provide('rethinkdb.query');

goog.require('goog.asserts');
goog.require('rethinkdb.query.Database');

/**
 * @return {rethinkdb.query.Database}
 * @export
 */
rethinkdb.query.db = function(db_name) {
     return new rethinkdb.query.Database(db_name);
};

/** @export */
rethinkdb.query.db_create = function(db_name, primary_datacenter) {
    //TODO how to get cluster level default?
    primary_datacenter = primary_datacenter || 'cluster-level-default?';
};

/** @export */
rethinkdb.query.db_drop = function(db_name) {
    
};

/** @export */
rethinkdb.query.db_list = function() {
    
};

/** @export */
rethinkdb.query.expr = function(value) {
    return new rethinkdb.query.JSONExpression(value);
};

/** @export */
rethinkdb.query.table = function(table_identifier) {
    var db_table_array = table_identifier.split('.');

    var db_name = db_table_array[0];
    var table_name = db_table_array[1];
    if (table_name === undefined) {
        table_name = db_name;
        db_name = undefined;
    }

    return new rethinkdb.query.Table(table_name, db_name);
};

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
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.VarExpression = function(varName) {
    this.varName_ = varName;
};
goog.inherits(rethinkdb.query.VarExpression, rethinkdb.query.ReadExpression);

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
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.AttrExpression = function(leftExpr, attrName) {
    this.leftExpr_ = leftExpr;
    this.attrName_ = attrName;
};
goog.inherits(rethinkdb.query.AttrExpression, rethinkdb.query.ReadExpression);

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
 * @extends {rethinkdb.query.ReadExpression}
 */
rethinkdb.query.ImplicitAttrExpression = function(attrName) {
    this.attrName_ = attrName;
};
goog.inherits(rethinkdb.query.ImplicitAttrExpression, rethinkdb.query.ReadExpression);

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
