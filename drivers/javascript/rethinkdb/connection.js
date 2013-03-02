// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.Connection');

goog.require('rethinkdb.errors');
goog.require('rethinkdb.Cursor');
goog.require('goog.math.Long');
goog.require('goog.object');
goog.require('goog.proto2.WireFormatSerializer');

/**
 * As this is an abstract class you cannot use this constructor directly.
 * Construct instances of TcpConnection or HttpConnection instead.
 * @class A connection over which queries may be sent.
 * This is an abstract base class for two different kinds of connections,
 * a tcp connection or an http connection (for use by browsers).
 * @param {?string|Object} host Either a string giving the host to connect to or
 *      an object specifying host and/or port and/or db for the default database to
 *      use on this connection. Any key not supplied will revert to the default.
 * @param {?function(Error)=} opt_errorHandler optional error handler to use for this connection
 * @constructor
 */
rethinkdb.Connection = function(host, opt_errorHandler) {
    if (typeof host === 'undefined') {
        host = {};
    } else if (typeof host === 'string') {
        host = {'host':host};
    }

    this.host_ = host['host'] || this.DEFAULT_HOST;
    this.port_ = host['port'] || this.DEFAULT_PORT;
    this.db_   = host['db']   || this.DEFAULT_DB;


    rethinkdb.util.typeCheck_(this.host_, 'string');
    rethinkdb.util.typeCheck_(this.port_, 'number');
    rethinkdb.util.typeCheck_(this.db_,   'string');

    this.outstandingQueries_ = {};
    this.nextToken_ = 1;

    this.errorHandler_ = opt_errorHandler || null;

    rethinkdb.last_connection_ = this;
};

/**
 * The default host to use for new connections that don't specify a host
 * @constant
 * @type {string}
 */
rethinkdb.Connection.prototype.DEFAULT_HOST = 'localhost';

/**
 * The default port to use for new connections that don't specify a port
 * @type {number}
 */
rethinkdb.Connection.prototype.DEFAULT_PORT = 28015;

/**
 * The default database to use for new connections that don't specify one
 * @constant
 * @type {string}
 */
rethinkdb.Connection.prototype.DEFAULT_DB = 'test';



/**
 * Closes this connection and reopens a new connection to the same host
 * and port.
 * @param {function(...)=} onConnect
 */
rethinkdb.Connection.prototype.reconnect = function(onConnect) {
    this.constructor.call(this, {'host':this.host_,
                                 'port':this.port_,
                                 'db'  :this.db_}, onConnect, this.errorHandler_);
};
goog.exportProperty(rethinkdb.Connection.prototype, 'reconnect',
                    rethinkdb.Connection.prototype.reconnect);

/**
 * Invoke the error handler on this connection.
 * @param {Error} error The origional error thrown
 * @ignore
 */
rethinkdb.Connection.prototype.error_ = function(error) {
    if (this.errorHandler_) {
        this.errorHandler_(error);
    } else {
        // Simply rethrow the error. It'll go uncaught and show up in the developer's console.

        // THIS IS A HACAK, but Slava wants it
        if (this.printErr_) {
            console.log(error.name + ":" + error.message);
        } else {
            throw error;
        }
    }
};

/**
 * Set the error handler to use for this connection. Will be called with any error
 * returned by the server.
 * @param {function(Error)} handler The error handler to use
 */
rethinkdb.Connection.prototype.setErrorHandler = function(handler) {
    this.errorHandler_ = handler;
};
goog.exportProperty(rethinkdb.Connection.prototype, 'setErrorHandler',
                    rethinkdb.Connection.prototype.setErrorHandler);

/**
 * Send the protobuf over the underlying connection. Implemented by subclasses.
 * @param {ArrayBuffer} data The protocol buffer to send
 * @private
 */
rethinkdb.Connection.prototype.send_ = goog.abstractMethod;

/**
 * Close the underlying connection. Implemented by subclass.
 * @private
 */
rethinkdb.Connection.prototype.close = goog.abstractMethod;

/**
 * Serializes and sends protocol buffer object on this connection.
 * @param {goog.proto2.Message} pbObj
 * @private
 */
rethinkdb.Connection.prototype.sendProtoBuf_ = function(pbObj) {
    var serializer = new goog.proto2.WireFormatSerializer();
    var data = serializer.serialize(pbObj);

    // RDB protocol requires a 4 byte little endian length field at the head of the request
    var length = data.byteLength;
    var finalArray = new Uint8Array(length + 4);
    (new DataView(finalArray.buffer)).setInt32(0, length, true);
    finalArray.set(data, 4);

    this.send_(finalArray.buffer);
};

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with the result.
 * @param {rethinkdb.Query} query The query to run.
 * @param {?=} opt_callbackOrOptions If supplied, shortcut for calling next on the
 *  cursor or an options argument specifying callback and/or useOutdated.
 */
rethinkdb.Connection.prototype.run = function(query, opt_callbackOrOptions) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(query, rethinkdb.Query);

    var callback;
    var useOutdated;
    if (typeof opt_callbackOrOptions === 'function') {
        callback = opt_callbackOrOptions;
    } else {
        callback = opt_callbackOrOptions['callback'];
        useOutdated = opt_callbackOrOptions['useOutdated'];
    }

    var pb = query.buildQuery({defaultUseOutdated:useOutdated});

    // Assign a token
    pb.setToken((this.nextToken_++).toString());
    var cursor = new rethinkdb.Cursor(this, query, pb.getToken());
    this.outstandingQueries_[pb.getToken()] = cursor;

    this.sendProtoBuf_(pb);
    if (callback) {
        cursor.next(callback);
    }

    return cursor;
};
goog.exportProperty(rethinkdb.Connection.prototype, 'run',
                    rethinkdb.Connection.prototype.run);

/**
 * Called by subclass when data is received on the underlying connection.
 * @param {Uint8Array} data Received data from the connection
 * @private
 */
rethinkdb.Connection.prototype.recv_ = function(data) {
    goog.asserts.assert(data.length >= 4);
    var msgLength = (new DataView(data.buffer)).getUint32(0, true);
    if (msgLength !== (data.length - 4)) {
        this.error_(new rethinkdb.errors.ClientError("RDB response corrupted, length inaccurate"));
    }

    var serializer = new goog.proto2.WireFormatSerializer();
    var response = new Response();

    try {
        serializer.deserializeTo(response, new Uint8Array(data.buffer, 4));
    } catch(err) {
        this.error_(new rethinkdb.errors.ClientError("response deserialization failed"));
    }

    var cursor = this.outstandingQueries_[response.getToken()];
    if (!cursor) {
        // Ignore this result. This query was canceled.
        return;
    }

    var responseStatus = response.getStatusCode();
    switch(responseStatus) {
    case Response.StatusCode.BROKEN_CLIENT:
        var error = new rethinkdb.errors.BrokenClient(response.getErrorMessage())
        cursor.concatResults(error, true);
        break;
    case Response.StatusCode.RUNTIME_ERROR:
        var bt = response.getBacktrace();
        if (bt) { bt = bt.frameArray() } else { bt = [] }
        bt = JSON.stringify(bt)
        var error =
            new rethinkdb.errors.RuntimeError(rethinkdb.util.formatServerError_(response, cursor.query_));
        error['bt'] = bt;
        cursor.concatResults(error, true);
        break;
    case Response.StatusCode.BAD_QUERY:
        var error = new rethinkdb.errors.BadQuery(response.getErrorMessage());
        cursor.concatResults(error, true);
        break;
    case Response.StatusCode.SUCCESS_EMPTY:
        delete this.outstandingQueries_[response.getToken()]
        cursor.concatResults([], true);
        break;
    case Response.StatusCode.SUCCESS_STREAM:
        delete this.outstandingQueries_[response.getToken()]
        var results = response.responseArray().map(JSON.parse);
        cursor.concatResults(results, true);
        break;
    case Response.StatusCode.SUCCESS_JSON:
        delete this.outstandingQueries_[response.getToken()]
        var result = JSON.parse(response.getResponse(0));
        cursor.concatResults(result, true);
        break;
    case Response.StatusCode.SUCCESS_PARTIAL:
        var results = response.responseArray().map(JSON.parse);
        cursor.concatResults(results, false);
        break;
    default:
        var error = new rethinkdb.errors.ClientError("unknown response status code");
        cursor.concatResults(error, true);
        break;
    }
};

/**
 * Set the default db to use for this connection
 * @param {string} dbName
 */
rethinkdb.Connection.prototype.use = function(dbName) {
    rethinkdb.util.argCheck_(arguments, 1);
    rethinkdb.util.typeCheck_(dbName, 'string');
	this.db_ = dbName;
};
goog.exportProperty(rethinkdb.Connection.prototype, 'use',
                    rethinkdb.Connection.prototype.use);

/**
 * Return the current default db for this connection
 * @return {string}
 */
rethinkdb.Connection.prototype.getDefaultDb = function() {
    return this.db_;
};
goog.exportProperty(rethinkdb.Connection.prototype, 'getDefaultDb',
                    rethinkdb.Connection.prototype.getDefaultDb);

/** @ignore */
rethinkdb.Connection.prototype.requestNextBatch = function(token) {
    var cont = new Query();
    cont.setType(Query.QueryType.CONTINUE);
    cont.setToken(token);
    this.sendProtoBuf_(cont);
};

/** @ignore */
rethinkdb.Connection.prototype.forgetCursor = function(token) {
    delete this.outstandingQueries_[token];
};
