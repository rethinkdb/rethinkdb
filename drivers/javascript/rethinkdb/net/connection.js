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
	this.defaultDbName_ = db_name || 'Welcome-rdb';
    this.outstandingQueries_ = {};
    this.nextToken_ = 1;

    rethinkdb.net.last_connection = this;
};

rethinkdb.net.Connection.prototype.send_ = goog.abstractMethod;

rethinkdb.net.Connection.prototype.close = goog.nullFunction;

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with the result.
 * @param {rethinkdb.query.BaseQuery} expr The expression to run.
 * @param {function(ArrayBuffer)} callback Function to invoke with response.
 */
rethinkdb.net.Connection.prototype.run = function(expr, callback) {
    var query = expr.buildQuery();

    // Assign a token
    query.setToken((this.nextToken_++).toString());

    this.outstandingQueries_[query.getToken()] = callback;

    var serializer = new goog.proto2.WireFormatSerializer();
    var data = serializer.serialize(query);

    // RDB protocol requires a 4 byte little endian length field at the head of the request
    var length = data.byteLength;
    var finalArray = new Uint8Array(length + 4);
    (new DataView(finalArray.buffer)).setInt32(0, length, true);
    finalArray.set(data, 4);

    this.send_(finalArray.buffer);
};
goog.exportProperty(rethinkdb.net.Connection.prototype, 'run',
                    rethinkdb.net.Connection.prototype.run);

/**
 * Called when data is received on underlying connection.
 * @private
 */
rethinkdb.net.Connection.prototype.recv_ = function(data) {
    goog.asserts.assert(data.length >= 4);
    var msgLength = (new DataView(data.buffer)).getUint32(0, true);
    if (msgLength !== (data.length - 4)) {
        //TODO handle this error
        throw "message is wrong length";
    }

    try {
        var serializer = new goog.proto2.WireFormatSerializer();
        var response = new Response();
        serializer.deserializeTo(response, new Uint8Array(data.buffer, 4));

        var responseStatus = response.getStatusCode();
        var callback = this.outstandingQueries_[response.getToken()];
        switch(responseStatus) {
        case Response.StatusCode.BROKEN_CLIENT:
            throw "Broken client";
            break;
        case Response.StatusCode.RUNTIME_ERROR:
            throw "Runtime error";
            break;
        case Response.StatusCode.BAD_QUERY:
            throw "bad query";
            break;
        case Response.StatusCode.SUCCESS_EMPTY:
            delete this.outstandingQueries_[response.getToken()]
            if (callback) callback();
            break;
        case Response.StatusCode.SUCCESS_STREAM:
            delete this.outstandingQueries_[response.getToken()]
            if (callback) {
                var results = response.responseArray().map(JSON.parse);
                callback(results);
            }
            break;
        case Response.StatusCode.SUCCESS_JSON:
            delete this.outstandingQueries_[response.getToken()]
            if (callback) {
                var result = JSON.parse(response.getResponse(0));
                callback(result);
            }
            break;
        case Response.StatusCode.SUCCESS_PARTIAL:
            throw "partial results not yet implemented";
            break;
        default:
            throw "unknown response status code";
            break;
        }

    } catch(err) {
        // Deserialization failed
        // TODO how to express this erro?
        throw err;
    }
};

rethinkdb.net.Connection.prototype.use = function(db_name) {
	this.defaultDbName_ = db_name;
};

rethinkdb.net.Connection.prototype.getDefaultDb = function() {
    return this.defaultDbName_;
};
