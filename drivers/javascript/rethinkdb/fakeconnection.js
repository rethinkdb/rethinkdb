goog.provide('rethinkdb.FakeConnection');

goog.require('rethinkdb.Connection');


/**
 * Connect to fake server
 * @param {Object} fake server.
 * @constructor
 * @extends {rethinkdb.Connection}
 * @export
 */
rethinkdb.FakeConnection = function(fake_server) {
    this.fake_server = fake_server;
    goog.base(this, fake_server, undefined);
};
goog.inherits(rethinkdb.FakeConnection, rethinkdb.Connection);

/**
 * Send data to the fake server
 * @param {ArrayBuffer} data The data to send.
 * @private
 * @override
 */
rethinkdb.FakeConnection.prototype.send_ = function(data) {
    var response = this.fake_server.execute(data);
    this.recv_(new Uint8Array(/**@type{ArrayBuffer}*/(response)));
};


