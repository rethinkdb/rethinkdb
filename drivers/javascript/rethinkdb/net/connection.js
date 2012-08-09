// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net.Connection');

goog.require('goog.math.Long');
goog.require('goog.proto2.WireFormatSerializer');

/**
 * Connection, a connection over which queries may be sent.
 * This is an abstract base class for two different kinds of connections,
 * a tcp connection or an http connection (for use by browsers).
 * @constructor
 */
rethinkdb.net.Connection = function(db_name) {
	this.defaultDbName_ = db_name || null;
    this.outstandingQueries_ = {};
    rethinkdb.net.last_connection = this;
};

rethinkdb.net.Connection.prototype.send_ = goog.abstractMethod;

rethinkdb.net.Connection.prototype.close = goog.nullFunction;

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with the result.
 * @param {rethinkdb.reql.query.Expression} expr The expression to run.
 * @param {function(ArrayBuffer)} callback Function to invoke with response.
 */
rethinkdb.net.Connection.prototype.run = function(expr, callback) {
    var term = expr.compile();

    console.log(term);

    // Wrap in query
    var query = new Query();
    query.setType(Query.QueryType.READ);

    var readQuery = new ReadQuery();
    readQuery.setTerm(term);

    // Assign a token
    var token = new goog.math.Long(Math.random(), Math.random());
    query.setToken(token.toString());
    query.setReadQuery(readQuery);

    this.outstandingQueries_[query.getToken()] = callback;

    var serializer = new goog.proto2.WireFormatSerializer();
    var data = serializer.serialize(query);

    console.log(data);

    this.send_(data);
};
goog.exportProperty(rethinkdb.net.Connection.prototype, 'run',
                    rethinkdb.net.Connection.prototype.run);

/**
 * Called when data is received on underlying connection.
 * @private
 */
rethinkdb.net.Connection.prototype.recv_ = function(data) {
    var serializer = new goog.proto2.WireFormatSerializer();
    var response = new Response();
    serializer.deserializeTo(response, data);
    var callback = this.outstandingQueries_[response.getToken()];
    delete this.outstandingQueries_[response.getToken()]

    //TODO transform message object into appropriate object
    if (callback) callback(response);
};

rethinkdb.net.Connection.prototype.use = function(db_name) {
	this.defaultDbName_ = db_name;
};

rethinkdb.net.Connection.prototype.getDefaultDb = function() {
    return new rethinkdb.reql.query.Database(this.defaultDbName_);
};
