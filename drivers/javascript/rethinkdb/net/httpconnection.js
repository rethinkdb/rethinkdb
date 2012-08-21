goog.provide('rethinkdb.net.HttpConnection');

goog.require('rethinkdb.net.Connection');

/**
 * @constructor
 * @extends {rethinkdb.net.Connection}
 * @export
 */
rethinkdb.net.HttpConnection = function(url, db_name) {
    goog.base(this, db_name);
    this.url_ = url;
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
