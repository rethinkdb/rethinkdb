goog.provide('rethinkdb.net.HttpConnection');

goog.require('rethinkdb.net.Connection');

/**
 * @constructor
 * @extends {rethinkdb.net.Connection}
 * @export
 */
rethinkdb.net.HttpConnection = function(host_or_list, onConnect, onFailure) {
    var /** @const */ DEFAULT_PORT = 13457;
	var /** @const */ DEFAULT_HOST = 'localhost';

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

            var url = 'http://'+host+':'+port;

            var xhr = new XMLHttpRequest();
            xhr.open("GET", url, true);

            xhr.onreadystatechange = function(e) {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        self.url_ = url;
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
 * Private method for actually sending request to the server
 * over the http connection.
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.net.HttpConnection.prototype.send_ = function(data) {
    if (!this.url_)
        throw "Connection not open";

    var xhr = new XMLHttpRequest();
    xhr.responseType = "arraybuffer";
    xhr.open("POST", this.url_, true);

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
 * @override
 */
rethinkdb.net.TcpConnection.prototype.close = function() {
    if (!this.url_)
        throw "Connection not open";

    this.url_ = null;
};
goog.exportProperty(rethinkdb.net.HttpConnection.prototype, 'close',
                    rethinkdb.net.HttpConnection.prototype.close);
