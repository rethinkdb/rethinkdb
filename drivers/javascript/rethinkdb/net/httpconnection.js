goog.provide('rethinkdb.net.HttpConnection');

goog.require('rethinkdb.net.Connection');

/**
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
 * @param {function(...)=} onConnect
 * @param {function(...)=} onFailure
 * @constructor
 * @extends {rethinkdb.net.Connection}
 * @export
 */
rethinkdb.net.HttpConnection = function(host_or_list, onConnect, onFailure) {
    typeCheck_(onConnect, 'function');
    typeCheck_(onFailure, 'function');

    /**
     * @const
     * @type {number}
     * @ignore
     */
    var DEFAULT_PORT = 21300;
    /**
     * @const
     * @type {string}
     * @ignore
     */
	var DEFAULT_HOST = 'localhost';

    goog.base(this, null);
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
			if (onFailure) onFailure();
		}
	})();
};
goog.inherits(rethinkdb.net.HttpConnection, rethinkdb.net.Connection);

/**
 * Send data over the underlying HTTP connection.
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.net.HttpConnection.prototype.send_ = function(data) {
    if (!this.url_)
        throw new rethinkdb.errors.ClientError("Connection not open");

    var xhr = new XMLHttpRequest();
    xhr.responseType = "arraybuffer";
    xhr.open("POST", this.url_+'?conn_id='+this.conn_id_, true);

    var self = this;
    xhr.onreadystatechange = function(e) {
        if (xhr.readyState === 4 &&
            xhr.status === 200) {
                self.recv_(new Uint8Array(xhr.response));
        }
    };

    xhr.send(data);
};

/**
 * Close the underlying HTTP connection.
 * @override
 */
rethinkdb.net.HttpConnection.prototype.close = function() {
    if (!this.url_)
        throw new rethinkdb.errors.ClientError("Connection not open");

    var xhr = new XMLHttpRequest();
    xhr.open("POST", this.url_+'close-connection?conn_id='+this.conn_id_, true);
    xhr.send();

    this.conn_id_ = null;
    this.url_ = null;
};
goog.exportProperty(rethinkdb.net.HttpConnection.prototype, 'close',
                    rethinkdb.net.HttpConnection.prototype.close);
