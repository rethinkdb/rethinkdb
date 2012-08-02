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
};

/** @override */
rethinkdb.net.Connection.prototype.send_ = function(){};//goog.abstractMethod;

/** @override */
rethinkdb.net.Connection.prototype.close = function(){};//goog.nullFunction;

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with the result.
 */
rethinkdb.net.Connection.prototype.run = function(expr, callback) {
    //TODO
};

rethinkdb.net.Connection.prototype.use = function(db_name) {
	this.defaultDb_ = db_name;
};
