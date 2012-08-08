// Copyright 2012 Hexagram 49 Inc. All rights reserved.

goog.provide('rethinkdb.net.HttpConnection');

goog.require('rethinkdb.net.Connection');

/**
 * @constructor
 * @export
 */
rethinkdb.net.HttpConnection = function(url, db_name) {
    goog.base(this, db_name);
    var self = this;

    var xhr = self.xhr_ = new XMLHttpRequest();
    xhr.open("GET", url, true);

    xhr.onreadystatechange = function(e) {
        if (xhr.readyState === 4 &&
            xhr.status === 200) {
                self.recv_(xhr.response); 
        }
    };
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
    if (this.xhr_) {
        this.xhr_.send(data);
    } else {
        throw "Connection not open";
    }
};
