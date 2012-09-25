goog.provide('rethinkdb.Connection');

goog.require('rethinkdb.errors');
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

    typeCheck_(this.host_, 'string');
    typeCheck_(this.port_, 'number');
    typeCheck_(this.db_,   'string');

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
rethinkdb.Connection.prototype.DEFAULT_PORT = 12346;

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
 * Construct and run the given query, used by public run and iter methods
 * @param {Object} options Run options
 *  Only valid if iterate = true
 * @private
 */
rethinkdb.Connection.prototype.run_ = function(options) {
    // Defaults
    var opts = {
        'expr':rethinkdb.expr(null),
        'callback':function(){},
        'iterate':false,
        'doneCallback':function(){},
        'allowOutdated':false
    };
    goog.object.extend(opts, options);

    typeCheck_(opts['expr'], rethinkdb.Query);
    typeCheck_(opts['callback'], 'function');
    typeCheck_(opts['iterate'], 'boolean');
    typeCheck_(opts['doneCallback'], 'function');
    typeCheck_(opts['allowOutdated'], 'boolean');

    var query = opts['expr'].buildQuery({defaultAllowOutdated:opts['allowOutdated']});

    // Assign a token
    query.setToken((this.nextToken_++).toString());

    this.outstandingQueries_[query.getToken()] = {
        callback: opts['callback'],
        query: opts['expr'], // Save origional ast for backtrace reconstructions
        iterate : opts['iterate'],
        done: opts['doneCallback'],
        partial : []
    };

    this.sendProtoBuf_(query);
};

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
 * @param {rethinkdb.Query|Object} exprOrOpts The expression to run.
 * @param {function(...)=} opt_callback Function to invoke with response.
 */
rethinkdb.Connection.prototype.run = function(exprOrOpts, opt_callback) {
    argCheck_(arguments, 1);

    // THIS IS A HACAK, but Slava wants it
    this.printErr_ = false;

    var opts = {};
    if (exprOrOpts instanceof rethinkdb.Query) {
        opts['expr'] = exprOrOpts;
        opts['callback'] = opt_callback;
    } else {
        goog.object.extend(opts, exprOrOpts);
    }

    this.run_(opts);
};
goog.exportProperty(rethinkdb.Connection.prototype, 'run',
                    rethinkdb.Connection.prototype.run);

/**
 * Version of run that prints the result rather than taking a callback
 * @param {rethinkdb.Query|Object} exprOrOpts The expression to run.
 */
rethinkdb.Connection.prototype.runp = function(exprOrOpts) {
    argCheck_(arguments, 1);

    // THIS IS A HACAK, but Slava wants it
    this.printErr_ = true;

    var opts = {};
    if (exprOrOpts instanceof rethinkdb.Query) {
        opts['expr'] = exprOrOpts;
    } else {
        goog.object.extend(opts, exprOrOpts);
    }

    // That which makes runp different from run
    opts['callback'] = function(val) {
        console.log(val);
    };

    this.run_(opts);
};
goog.exportProperty(rethinkdb.Connection.prototype, 'runp',
                    rethinkdb.Connection.prototype.runp);

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with each element of the result. The main advantage of using iter
 * over run is that results are lazily loaded as they are returned from the
 * server. Use anytime the result is expected to be very large.
 * @param {rethinkdb.Query|Object} exprOrOpts The expression to run.
 * @param {function(...)} opt_callback Function to invoke with response.
 * @param {function(...)=} opt_doneCallback Function to invoke when stream has ended.
 */
rethinkdb.Connection.prototype.iter = function(exprOrOpts, opt_callback, opt_doneCallback) {
    argCheck_(arguments, 1);

    // THIS IS A HACAK, but Slava wants it
    this.printErr_ = false;

    var opts = {};
    if (exprOrOpts instanceof rethinkdb.Query) {
        opts['expr'] = exprOrOpts;
        opts['callback'] = opt_callback
        opts['doneCallback'] = opt_doneCallback
    } else {
        goog.object.extend(opts, exprOrOpts);
    }

    // That which makes iter different from run
    opts['iterate'] = true;

    this.run_(opts);
};
goog.exportProperty(rethinkdb.Connection.prototype, 'iter',
                    rethinkdb.Connection.prototype.iter);

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

    var responseStatus = response.getStatusCode();
    var request = this.outstandingQueries_[response.getToken()];
    if (request) {

        switch(responseStatus) {
        case Response.StatusCode.BROKEN_CLIENT:
            this.error_(new rethinkdb.errors.BrokenClient(response.getErrorMessage()));
            break;
        case Response.StatusCode.RUNTIME_ERROR:
            this.error_(new rethinkdb.errors.RuntimeError(formatServerError_(response, request.query)));
            break;
        case Response.StatusCode.BAD_QUERY:
            this.error_(new rethinkdb.errors.BadQuery(response.getErrorMessage()));
            break;
        case Response.StatusCode.SUCCESS_EMPTY:
            delete this.outstandingQueries_[response.getToken()]
            if (request.callback) request.callback();
            break;
        case Response.StatusCode.SUCCESS_STREAM:
            delete this.outstandingQueries_[response.getToken()]
            var results = response.responseArray().map(JSON.parse);
            if (request.callback) {
                if (request.iterate) {
                    results.forEach(request.callback);
                } else {
                    request.callback(request.partial.concat(results));
                }
            }

            if (request.done && request.iterate) request.done();
            break;
        case Response.StatusCode.SUCCESS_JSON:
            delete this.outstandingQueries_[response.getToken()]
            var result = JSON.parse(response.getResponse(0));
            if (request.iterate) {
                if (result.forEach && request.callback) {
                    result.forEach(request.callback);
                } else if (request.callback) {
                    request.callback(result);
                }
                if (request.done) {
                    request.done();
                }
            } else if (request.callback) {
                request.callback(result);
            }
            break;
        case Response.StatusCode.SUCCESS_PARTIAL:
            var cont = new Query();
            cont.setType(Query.QueryType.CONTINUE);
            cont.setToken(response.getTokenOrDefault());
            this.sendProtoBuf_(cont);

            var results = response.responseArray().map(JSON.parse);
            if (request.callback) {
                if (request.iterate) {
                    results.forEach(request.callback);
                } else {
                    // Save results for eventual callback call
                    request.partial = request.partial.concat(results);
                }
            }
            break;
        default:
            this.error_(new rethinkdb.errors.ClientError("unknown response status code"));
            break;
        }
    } // else no matching request
};

/**
 * Set the default db to use for this connection
 * @param {string} dbName
 */
rethinkdb.Connection.prototype.use = function(dbName) {
    argCheck_(arguments, 1);
    typeCheck_(dbName, 'string');
	this.db_ = dbName;
};

/**
 * Return the current default db for this connection
 * @return {string}
 */
rethinkdb.Connection.prototype.getDefaultDb = function() {
    return this.db_;
};
