goog.provide('rethinkdb.DemoConnection');

goog.require('rethinkdb.Connection');


/**
 * Connect to demo server
 * @param {Object} Demo server.
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 */
rethinkdb.DemoConnection = function(demo_server) {
    this.demo_server = demo_server;
    goog.base(this, demo_server, undefined);
};
goog.inherits(rethinkdb.DemoConnection, rethinkdb.Connection);

/**
 * Send data to the demo server
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.DemoConnection.prototype.send_ = function(data) {
    var response = this.demo_server.execute(data);
    this.recv_(new Uint8Array(/**@type{ArrayBuffer}*/(response)));
};


