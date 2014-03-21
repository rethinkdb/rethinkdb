/*
 * Copyright 2012 The Closure Compiler Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview Definitions for colour.js.
 * @externs
 * @author Daniel Wirtz <dcode@dcode.io>
 */
var colour = function() {};

/**
 * @type {string}
 */
colour.mode;

/**
 * @type {boolean}
 */
colour.headless;

/**
 * @param {string} str
 * @returns {function(this:String)}
 */
colour.color = function(str) {};

/**
 * @type {Object.<string,Object.<string,*>>}
 */
colour.themes;

/**
 * @param {string} name
 * @param {function(string):string} map
 */
colour.addSequencer = function(name, map) {};

/**
 * @param {Object.<string,*>} theme
 * @returns {Object.<string,*>|Error|undefined}
 */
colour.setTheme = function (theme) {};

/**
 * @returns {boolean}
 */
colour.uninstall = function() {};

/**
 * @returns {boolean}
 */
colour.install = function() {};

/**
 * @name String.prototype.bold
 * @type {string}
 */
// String.prototype.bold;

/**
 * @type {string}
 */
String.prototype.italic;

/**
 * @type {string}
 */
String.prototype.underline;

/**
 * @type {string}
 */
String.prototype.inverse;

/**
 * @type {string}
 */
String.prototype.strikethrough;

/**
 * @type {string}
 */
String.prototype.white;

/**
 * @type {string}
 */
String.prototype.grey;

/**
 * @type {string}
 */
String.prototype.black;

/**
 * @type {string}
 */
String.prototype.blue;

/**
 * @type {string}
 */
String.prototype.cyan;

/**
 * @type {string}
 */
String.prototype.green;

/**
 * @type {string}
 */
String.prototype.magenta;

/**
 * @type {string}
 */
String.prototype.red;

/**
 * @type {string}
 */
String.prototype.yellow;
