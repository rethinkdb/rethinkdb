goog.provide('rethinkdb.DemoConnection');

goog.require('rethinkdb.Connection');


/**
 * Connect to demo server
 * @param {Object} demoServer The demo server
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 */
rethinkdb.DemoConnection = function(demoServer) {
    goog.base(this, "!!no-host!!");
    this.demoServer_ = demoServer;
};
goog.inherits(rethinkdb.DemoConnection, rethinkdb.Connection);

/**
 * Send data to the demo server
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.DemoConnection.prototype.send_ = function(data) {
    var response = this.demoServer_.execute(data);
    this.recv_(new Uint8Array(/**@type{ArrayBuffer}*/(response)));
};


