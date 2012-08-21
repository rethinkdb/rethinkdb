/**
 * This file contains no publically exported functions but does
 * coordinate compilation and loading.
 */

// Entry point for dependency walking
goog.provide('rethinkdb');

// All exported modules must be reachable from this file according to the dependency chain
goog.require('rethinkdb.net');
goog.require('rethinkdb.query');
goog.require('rethinkdb.query2');
goog.require('rethinkdb.query.Table');
goog.require('rethinkdb.query.Database');
goog.require('rethinkdb.query.Expression');

// Export RethinDB namespace to commonjs module
if (typeof exports !== 'undefined') {
	if (typeof module !== 'undefined' && module.exports) {
        exports = module.exports = goog.global.rethinkdb;
	} else {
	    exports.rethinkdb = goog.global.rethinkdb;
    }
}
