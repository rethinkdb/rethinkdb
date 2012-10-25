// Copyright 2012 Hexagram 49 Inc. All rights reserved.


/**
 * All external symbols used by the library must be declared here or the
 * closure compiler will mangle what are really public facing names from
 * the external code. This includes the names of rethinkdb internal variables
 * returned from external code or the externally defined properties of these
 * variables will be renamed.
 */

function require(modName) {};
var exports;
exports.rethinkdb;
var module;
module.exports;
var net_node_;
net_node_.connect;
var socket_node_;
socket_node_.on;
socket_node_.removeAllListeners;
socket_node_.write;
socket_node_.setNoDelay;
var JSON;
JSON.stringify;
JSON.parse;
var console;
console.log;
var Buffer;
var node_data;
node_data.length;
node_data.readUInt32LE;
var process;
var global;
