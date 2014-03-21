/**
 * @fileoverview Minimal environment to compile colour.js.
 * @externs
 */

/**
 * @param {string} moduleName
 * @returns {!Object}
 */
var require = function(moduleName) {};

/**
 * @type {Module}
 */
var module;

/**
 * @constructor
 * @private
 */
var Module = function() {};

/**
 * @type {*}
 */
Module.prototype.exports;

/**
 * @param {string} name
 * @param {function()} contructor
 */
var define = function(name, contructor) {};

/**
 * @type {boolean}
 */
define.amd;
