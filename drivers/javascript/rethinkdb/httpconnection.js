// Copyright 2010-2012 RethinkDB, all rights reserved.
goog.provide('rethinkdb.HttpConnection');

goog.require('rethinkdb.Connection');

/**
 * Host or list behavior is being abandoned for now. Could be brought back
 * in the future.
 *
 * Constructs a HTTP based connection in one of several ways.
 * If no host is supplied the server will attempt to connect to the default host
 * over the default port. If a single host is given the constructor will attempt
 * to connect to that host over the default port. An object specifying both host
 * and port may also be supplied. Finally, if a list of host strings or host/port
 * objects are supplied the constructor will attempt to connect to given hosts in
 * the order given. Once a connection to a host has been established the optional
 * onConnect handler is called. If no connection can be made, the optional
 * onFailure handler is called.
 * @class An HTTP based connection to the RethinkDB server.
 * Use this connection type where TCP connections are unavailable, i.e. in a
 * browser. Prefer TCP connections when you can.
 * @param {?string|Array.<string>|Object|Array.<Object>} host_or_list A host/port
 *      pair or list of host port pairs giving the servers to attempt to connect
 *      to. Either hostname or port may be omitted to utilize the default.
 * @param {function(...)=} onConnect Function to call when connection is established
 * @param {function(...)=} onFailure Error handler to use for this connection. Will be
 *  called if a connection cannot be established. Can also be set with setErrorHandler.
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 * @ignore
 */
/*
rethinkdb.HttpConnection = function(host_or_list, onConnect, onFailure) {
    rethinkdb.util.typeCheck_(onConnect, 'function');
    rethinkdb.util.typeCheck_(onFailure, 'function');

    var DEFAULT_PORT = 8080;
	var DEFAULT_HOST = 'localhost';

    goog.base(this, null, onFailure);
    this.url_ = '';

    var self = this;

    var hosts;
	if (host_or_list) {
		if (goog.isArray(host_or_list)) {
			hosts = host_or_list;
		} else {
			hosts = [host_or_list];
		}
	} else {
 		hosts = [DEFAULT_HOST]
	}

    (function tryNext() {
		var hostObj = hosts.shift();
		if (hostObj) {

			var host;
			var port;
			if (typeof hostObj === 'string') {
				host = hostObj;
				port = DEFAULT_PORT;
			} else {
				host = hostObj['host'] || DEFAULT_HOST;
				port = hostObj['port'] || DEFAULT_PORT;
			}

            var url = 'http://'+host+':'+port+'/ajax/reql/';

            var xhr = new XMLHttpRequest();
            xhr.responseType = "arraybuffer";
            xhr.open("GET", url + 'open-new-connection', true);

            xhr.onreadystatechange = function(e) {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        self.url_ = url;
                        self.conn_id_ = (new DataView(xhr.response)).getInt32(0, true);
                        if (onConnect) onConnect(self);
                    } else {
                        tryNext();
                    }
                }
            };

            xhr.send();
		} else {
            self.error_(new rethinkdb.errors.ClientError('Unabled to establish HTTP connection'));
		}
	})();
};
goog.inherits(rethinkdb.HttpConnection, rethinkdb.Connection);
*/

/**
 * Constructs a HTTP based connection in one of several ways.
 * @class A TCP based connection to the RethinkDB server.
 * Use this connection type when running this client in a server side execution
 * environment like node.js. It is to be preferred to the HTTP connection when
 * available.
 * @param {?string|Object} host Either a string giving the host to connect to or
 *      an object specifying host and/or port and/or db for the default database to
 *      use on this connection. Any key not supplied will revert to the default.
 * @param {function(...)=} onConnect Function to call when connection is established
 * @param {function(...)=} onFailure Error handler to use for this connection. Will be
 *  called if a connection cannot be established. Can also be set with setErrorHandler.
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 */
rethinkdb.HttpConnection = function(host, onConnect, onFailure) {
    rethinkdb.util.typeCheck_(onConnect, 'function');
    rethinkdb.util.typeCheck_(onFailure, 'function');

    goog.base(this, host, onFailure);
    this.url_ = '';

    var self = this;

    var url = 'http://'+this.host_+':'+this.port_+'/ajax/reql/';

    var xhr = new XMLHttpRequest();
    xhr.open("GET", url + 'open-new-connection', true);
    xhr.responseType = "arraybuffer";

    xhr.onreadystatechange = function f(e) {
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                self.url_ = url;
                self.conn_id_ = (new DataView(/**@type{ArrayBuffer}*/(xhr.response))).getInt32(0, true);
                if (onConnect) onConnect(self);
            } else {
                self.error_(new rethinkdb.errors.ClientError('Unabled to establish HTTP connection'));
            }
        }
    };

    xhr.send();
};
goog.inherits(rethinkdb.HttpConnection, rethinkdb.Connection);

/**
 * @type {number}
 */
rethinkdb.HttpConnection.prototype.DEFAULT_PORT = 8080;

/**
 * Send data over the underlying HTTP connection.
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.HttpConnection.prototype.send_ = function(data) {
    if (!this.url_)
        this.error_(new rethinkdb.errors.ClientError("Connection not open"));

    var xhr = new XMLHttpRequest();
    xhr.open("POST", this.url_+'?conn_id='+this.conn_id_, true);
    xhr.responseType = "arraybuffer";

    var self = this;
    xhr.onreadystatechange = function(e) {
        if (xhr.readyState === 4 &&
            xhr.status === 200) {
                self.recv_(new Uint8Array(/**@type{ArrayBuffer}*/(xhr.response)));
        }
    };

    xhr.send(data);
};

/**
 * Close the underlying HTTP connection.
 * @override
 */
rethinkdb.HttpConnection.prototype.close = function() {
    if (!this.url_)
        this.error_(new rethinkdb.errors.ClientError("Connection not open"));

    var xhr = new XMLHttpRequest();
    xhr.open("POST", this.url_+'close-connection?conn_id='+this.conn_id_, true);
    xhr.send();

    this.conn_id_ = null;
    this.url_ = null;
};
goog.exportProperty(rethinkdb.HttpConnection.prototype, 'close',
                    rethinkdb.HttpConnection.prototype.close);
