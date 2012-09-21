/**
 * This file contains no publically exported functions but does
 * coordinate compilation and loading.
 */

goog.provide('topLevel');

// All exported modules must be reachable from this file according to the dependency chain
goog.require('rethinkdb.net');
goog.require('rethinkdb.errors');
goog.require('rethinkdb.Connection');
goog.require('rethinkdb.query2');
goog.require('rethinkdb.Table');
goog.require('rethinkdb.Database');
goog.require('rethinkdb.Expression');
goog.require('rethinkdb');

// Export RethinDB namespace to commonjs module
if (typeof exports !== 'undefined') {
	if (typeof module !== 'undefined' && module.exports) {
        exports = module.exports = goog.global.rethinkdb;
	} else {
	    exports.rethinkdb = goog.global.rethinkdb;
    }
}
