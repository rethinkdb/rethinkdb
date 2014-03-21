/**
 * @param {string} moduleName
 * @returns {?}
 */
function require(moduleName) {};

/**
 * @param {string} name
 * @param {Array.<string>} deps
 * @param {function(...[*])} f
 */
function define(name, deps, f) {};

/**
 * @param {number} size
 * @constructor
 * @extends Array
 */
function Buffer(size) {};

/**
 * @param {*} buf
 * @return {boolean}
 */
Buffer.isBuffer = function(buf) {};

/**
 * @type {Object.<string,*>}
 */
var console = {};

/**
 * @param {string} s
 */
console.log = function(s) {};

/**
 * @type {*}
 */
var module = {};
