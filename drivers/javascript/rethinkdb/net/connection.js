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
	this.defaultDbName_ = db_name || 'Welcome-db';
    this.outstandingQueries_ = {};
    this.nextToken_ = 1;

    rethinkdb.net.last_connection = this;
};

rethinkdb.net.Connection.prototype.send_ = goog.abstractMethod;

rethinkdb.net.Connection.prototype.close = goog.nullFunction;

/**
 * Internal run used by external run and iter
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
 * Internal helper that takes a google closure protobuf object and sends it to the server
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
 * @param {rethinkdb.query.BaseQuery} expr The expression to run.
 * @param {function(ArrayBuffer)} opt_callback Function to invoke with response.
 */
rethinkdb.net.Connection.prototype.run = function(expr, opt_callback) {
    this.run_(expr, false, opt_callback);
};
goog.exportProperty(rethinkdb.net.Connection.prototype, 'run',
                    rethinkdb.net.Connection.prototype.run);

/**
 * Evaluates the given ReQL expression on the server and invokes
 * callback with each element of the result. The main advantage of using iter
 * over run is that results are lazily loaded as they are returned from the
 * server. Use anytime the result is expected to be very large.
 * @param {rethinkdb.query.BaseQuery} expr The expression to run.
 * @param {function(ArrayBuffer)} opt_callback Function to invoke with response.
 */
rethinkdb.net.Connection.prototype.iter = function(expr, opt_callback) {
    this.run_(expr, true, opt_callback);
};
goog.exportProperty(rethinkdb.net.Connection.prototype, 'iter',
                    rethinkdb.net.Connection.prototype.iter);


/**
 * @constructor
 * @extends {Error}
 */
function RuntimeError(msg) {
    this.name = "Runtime Error";
    this.message = msg || "The RDB runtime experienced an error";
}
goog.inherits(RuntimeError, Error);

/**
 * @constructor
 * @extends {Error}
 */
function BrokenClient() {
    this.name = "Broken Client";
    this.message = "The client sent the server an incorrectly formatted message";
}
goog.inherits(BrokenClient, Error);

/**
 * @constructor
 * @extends {Error}
 */
function BadQuery(msg) {
    this.name = "Bad Query";
    this.message = msg || "This query contains type errors";
}
goog.inherits(BadQuery, Error);

/**
 * Called when data is received on underlying connection.
 * @private
 */
rethinkdb.net.Connection.prototype.recv_ = function(data) {
    goog.asserts.assert(data.length >= 4);
    var msgLength = (new DataView(data.buffer)).getUint32(0, true);
    if (msgLength !== (data.length - 4)) {
        throw new ClientError("RDB response corrupted, length inaccurate");
    }

    var serializer = new goog.proto2.WireFormatSerializer();
    var response = new Response();

    try {
        serializer.deserializeTo(response, new Uint8Array(data.buffer, 4));
    } catch(err) {
        throw new ClientError("response deserialization failed");
    }

    var responseStatus = response.getStatusCode();
    var request = this.outstandingQueries_[response.getToken()];
    if (request) {

        switch(responseStatus) {
        case Response.StatusCode.BROKEN_CLIENT:
            throw new BrokenClient();
            break;
        case Response.StatusCode.RUNTIME_ERROR:
            throw new RuntimeError(response.getErrorMessage());
            break;
        case Response.StatusCode.BAD_QUERY:
            throw new BadQuery(response.getErrorMessage());
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
            throw new ClientError("unknown response status code");
            break;
        }
    } // else no matching request
};

rethinkdb.net.Connection.prototype.use = function(db_name) {
	this.defaultDbName_ = db_name;
};

rethinkdb.net.Connection.prototype.getDefaultDb = function() {
    return this.defaultDbName_;
};
