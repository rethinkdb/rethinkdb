goog.provide('rethinkdb.net.Connection');

goog.require('rethinkdb.errors');
goog.require('goog.math.Long');
goog.require('goog.proto2.WireFormatSerializer');

/**
 * As this is an abstract class you cannot use this constructor directly.
 * Construct instances of TcpConnection or HttpConnection instead.
 * @class A connection over which queries may be sent.
 * This is an abstract base class for two different kinds of connections,
 * a tcp connection or an http connection (for use by browsers).
 * @param {?string=} opt_dbName optional default db to use for this connection
 * @constructor
 */
rethinkdb.net.Connection = function(opt_dbName) {
    typeCheck_(opt_dbName, 'string');
	this.defaultDbName_ = opt_dbName || 'Welcome-db';
    this.outstandingQueries_ = {};
    this.nextToken_ = 1;

    rethinkdb.net.last_connection_ = this;
};

/**
 * Send the protobuf over the underlying connection. Implemented by subclasses.
 * @param {ArrayBuffer} data The protocol buffer to send
 * @private
 */
rethinkdb.net.Connection.prototype.send_ = goog.abstractMethod;

/**
 * Close the underlying connection. Implemented by subclass.
 * @private
 */
rethinkdb.net.Connection.prototype.close = goog.abstractMethod;

/**
 * Construct and run the given query, used by public run and iter methods
 * @param {rethinkdb.query.Query} expr The expression to run
 * @param {boolean} iterate Iterate callback over results
 * @param {function(...)=} opt_callback Callback to which results are returned
 * @private
 */
rethinkdb.net.Connection.prototype.run_ = function(expr, iterate, opt_callback) {
    var query = expr.buildQuery();

    // Assign a token
    query.setToken((this.nextToken_++).toString());

    this.outstandingQueries_[query.getToken()] = {
        callback: (opt_callback || null),
        iterate : iterate,
        partial : []
    };

    this.sendProtoBuf_(query);
};

/**
 * Serializes and sends protocol buffer object on this connection.
 * @param {goog.proto2.Message} pbObj
 * @private
 */
rethinkdb.net.Connection.prototype.sendProtoBuf_ = function(pbObj) {
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
 * @param {rethinkdb.query.Query} expr The expression to run.
 * @param {function(...)} opt_callback Function to invoke with response.
 */
rethinkdb.net.Connection.prototype.run = function(expr, opt_callback) {
    typeCheck_(expr, rethinkdb.query.Query);
    typeCheck_(opt_callback, 'function');
    this.run_(expr, false, opt_callback);
};
goog.exportProperty(rethinkdb.net.Connection.prototype, 'run',
                    rethinkdb.net.Connection.prototype.run);

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with each element of the result. The main advantage of using iter
 * over run is that results are lazily loaded as they are returned from the
 * server. Use anytime the result is expected to be very large.
 * @param {rethinkdb.query.Query} expr The expression to run.
 * @param {function(...)} opt_callback Function to invoke with response.
 */
rethinkdb.net.Connection.prototype.iter = function(expr, opt_callback) {
    typeCheck_(expr, rethinkdb.query.Query);
    typeCheck_(opt_callback, 'function');
    this.run_(expr, true, opt_callback);
};
goog.exportProperty(rethinkdb.net.Connection.prototype, 'iter',
                    rethinkdb.net.Connection.prototype.iter);

/**
 * Called by subclass when data is received on the underlying connection.
 * @param {Uint8Array} data Received data from the connection
 * @private
 */
rethinkdb.net.Connection.prototype.recv_ = function(data) {
    goog.asserts.assert(data.length >= 4);
    var msgLength = (new DataView(data.buffer)).getUint32(0, true);
    if (msgLength !== (data.length - 4)) {
        throw new rethinkdb.errors.ClientError("RDB response corrupted, length inaccurate");
    }

    var serializer = new goog.proto2.WireFormatSerializer();
    var response = new Response();

    try {
        serializer.deserializeTo(response, new Uint8Array(data.buffer, 4));
    } catch(err) {
        throw new rethinkdb.errors.ClientError("response deserialization failed");
    }

    var responseStatus = response.getStatusCode();
    var request = this.outstandingQueries_[response.getToken()];
    if (request) {

        switch(responseStatus) {
        case Response.StatusCode.BROKEN_CLIENT:
            throw new rethinkdb.errors.BrokenClient(response.getErrorMessage());
            break;
        case Response.StatusCode.RUNTIME_ERROR:
            throw new rethinkdb.errors.RuntimeError(response.getErrorMessage());
            break;
        case Response.StatusCode.BAD_QUERY:
            throw new rethinkdb.errors.BadQuery(response.getErrorMessage());
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
            break;
        case Response.StatusCode.SUCCESS_JSON:
            delete this.outstandingQueries_[response.getToken()]
            if (request.callback) {
                var result = JSON.parse(response.getResponse(0));
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
            throw new rethinkdb.errors.ClientError("unknown response status code");
            break;
        }
    } // else no matching request
};

/**
 * Set the default db to use for this connection
 * @param {string} dbName
 */
rethinkdb.net.Connection.prototype.use = function(dbName) {
    typeCheck_(dbName, 'string');
	this.defaultDbName_ = dbName;
};

/**
 * Return the current default db for this connection
 * @return {string}
 */
rethinkdb.net.Connection.prototype.getDefaultDb = function() {
    return this.defaultDbName_;
};
