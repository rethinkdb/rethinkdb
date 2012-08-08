// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net.Connection');

goog.require('goog.proto2.WireFormatSerializer');

/**
 * Connection, a connection over which queries may be sent.
 * This is an abstract base class for two different kinds of connections,
 * a tcp connection or an http connection (for use by browsers).
 * @constructor
 */
rethinkdb.net.Connection = function(db_name) {
	this.defaultDb_ = db_name || null;
    this.outstandingQueries_ = {};
};

/** @override */
rethinkdb.net.Connection.prototype.send_ = goog.abstractMethod;

/** @override */
rethinkdb.net.Connection.prototype.close = goog.nullFunction;

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with the result.
 * @param {rethinkdb.query.Expression} expr The expression to run.
 * @param {function(ArrayBuffer)}
 */
rethinkdb.net.Connection.prototype.run = function(expr, callback) {
    var query = expr.compile();
    this.outstandingQueries_[query.token] = callback;

    var serializer = new goog.proto2.WireFormatSerializer();
    var data = serializer.serialize(query);
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
    var response = serializer.deserialize(data);
    var callback = this.outstandingQueries_[response.token];
    delete this.outstandingQueries_[response.token]

    //TODO transform message object into appropriate object
    callback(response);
};

rethinkdb.net.Connection.prototype.use = function(db_name) {
	this.defaultDb_ = db_name;
};
