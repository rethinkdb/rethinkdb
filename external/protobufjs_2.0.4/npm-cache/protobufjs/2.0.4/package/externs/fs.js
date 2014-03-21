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
 * @fileoverview Definitions for node's "fs" module.
 * @see http://nodejs.org/api/fs.html
 * @externs
 * @author Daniel Wirtz <dcode@dcode.io>
 */

/**
 BEGIN_NODE_INCLUDE
 var fs = require('fs');
 END_NODE_INCLUDE
 */

var fs = {};

/**
 * @param {string} oldPath
 * @param {string} newPath
 * @param {function(...)=} callback
 */
fs.rename = function(oldPath, newPath, callback) {};

/**
 * @param {string} oldPath
 * @param {string} newPath
 */
fs.renameSync = function(oldPath, newPath) {};

/**
 * @param {*} fd
 * @param {number} len
 * @param {function(...)=} callback
 */
fs.truncate = function(fd, len, callback) {};

/**
 * @param {*} fd
 * @param {number} len
 */
fs.truncateSync = function(fd, len) {};

/**
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 * @param {function(...)=} callback
 */
fs.chown = function(path, uid, gid, callback) {};

/**
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 */
fs.chownSync = function(path, uid, gid) {};

/**
 * @param {*} fd
 * @param {number} uid
 * @param {number} gid
 * @param {function(...)=} callback
 */
fs.fchown = function(fd, uid, gid, callback) {};

/**
 * @param {*} fd
 * @param {number} uid
 * @param {number} gid
 */
fs.fchownSync = function(fd, uid, gid) {};

/**
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 * @param {function(...)=} callback
 */
fs.lchown = function(path, uid, gid, callback) {};

/**
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 */
fs.lchownSync = function(path, uid, gid) {};

/**
 * @param {string} path
 * @param {number} mode
 * @param {function(...)=} callback
 */
fs.chmod = function(path, mode, callback) {};

/**
 * @param {string} path
 * @param {number} mode
 */
fs.chmodSync = function(path, mode) {};

/**
 * @param {*} fd
 * @param {number} mode
 * @param {function(...)=} callback
 */
fs.fchmod = function(fd, mode, callback) {};

/**
 * @param {*} fd
 * @param {number} mode
 */
fs.fchmodSync = function(fd, mode) {};

/**
 * @param {string} path
 * @param {number} mode
 * @param {function(...)=} callback
 */
fs.lchmod = function(path, mode, callback) {};

/**
 * @param {string} path
 * @param {number} mode
 */
fs.lchmodSync = function(path, mode) {};

/**
 * @param {string} path
 * @param {function(string, fs.Stats)=} callback
 * @nosideeffects
 */
fs.stat = function(path, callback) {};

/**
 * @param {string} path
 * @return {fs.Stats}
 * @nosideeffects
 */
fs.statSync = function(path) {}

/**
 * @param {*} fd
 * @param {function(string, fs.Stats)=} callback
 * @nosideeffects
 */
fs.fstat = function(fd, callback) {};

/**
 * @param {*} fd
 * @return {fs.Stats}
 * @nosideeffects
 */
fs.fstatSync = function(fd) {}

/**
 * @param {string} path
 * @param {function(string, fs.Stats)=} callback
 * @nosideeffects
 */
fs.lstat = function(path, callback) {};

/**
 * @param {string} path
 * @return {fs.Stats}
 * @nosideeffects
 */
fs.lstatSync = function(path) {}

/**
 * @param {string} srcpath
 * @param {string} dstpath
 * @param {function(...)=} callback
 */
fs.link = function(srcpath, dstpath, callback) {};

/**
 * @param {string} srcpath
 * @param {string} dstpath
 */
fs.linkSync = function(srcpath, dstpath) {};

/**
 * @param {string} srcpath
 * @param {string} dstpath
 * @param {string=} type
 * @param {function(...)=} callback
 */
fs.symlink = function(srcpath, dstpath, type, callback) {};

/**
 * @param {string} srcpath
 * @param {string} dstpath
 * @param {string=} type
 */
fs.symlinkSync = function(srcpath, dstpath, type) {};

/**
 * @param {string} path
 * @param {function(string, string)=} callback
 * @nosideeffects
 */
fs.readlink = function(path, callback) {};

/**
 * @param {string} path
 * @return {string}
 * @nosideeffects
 */
fs.readlinkSync = function(path) {};

/**
 * @param {string} path
 * @param {object.<string,string>=|function(string, string)=} cache
 * @param {function(string, string)=} callback
 * @nosideeffects
 */
fs.realpath = function(path, cache, callback) {};

/**
 * @param {string} path
 * @param {object.<string,string>=} cache
 * @return {string}
 * @nosideeffects
 */
fs.realpathSync = function(path, cache) {};

/**
 * @param {string} path
 * @param {function(...)=} callback
 */
fs.unlink = function(path, callback) {};

/**
 * @param {string} path
 */
fs.unlinkSync = function(path) {};

/**
 * @param {string} path
 * @param {function(...)=} callback
 */
fs.rmdir = function(path, callback) {};

/**
 * @param {string} path
 */
fs.rmdirSync = function(path) {};

/**
 * @param {string} path
 * @param {number=} mode
 * @param {function(...)=} callback
 */
fs.mkdir = function(path, mode, callback) {};

/**
 * @param {string} path
 * @param {number=} mode
 */
fs.mkdirSync = function(path, mode) {};

/**
 * @param {string} path
 * @param {function(string,array.<string>)=} callback
 * @nosideeffects
 */
fs.readdir = function(path, callback) {};

/**
 * @param {string} path
 * @return {array.<string>}
 * @nosideeffects
 */
fs.readdirSync = function(path) {};

/**
 * @param {*} fd
 * @param {function(...)=} callback
 */
fs.close = function(fd, callback) {};

/**
 * @param {*} fd
 */
fs.closeSync = function(fd) {};

/**
 * @param {string} path
 * @param {string} flags
 * @param {number=} mode
 * @param {function(string, *)=} callback
 * @nosideeffects
 */
fs.open = function(path, flags, mode, callback) {};

/**
 * @param {string} path
 * @param {string} flags
 * @param {number=} mode
 * @return {*}
 * @nosideeffects
 */
fs.openSync = function(path, flags, mode) {};

/**
 * @param {string} path
 * @param {number} atime
 * @param {number} mtime
 * @param {function(...)=} callback
 * @nosideeffects
 */
fs.utimes = function(path, atime, mtime, callback) {};

/**
 * @param {string} path
 * @param {number} atime
 * @param {number} mtime
 * @nosideeffects
 */
fs.utimesSync = function(path, atime, mtime) {};

/**
 * @param {*} fd
 * @param {number} atime
 * @param {number} mtime
 * @param {function(...)=} callback
 * @nosideeffects
 */
fs.futimes = function(fd, atime, mtime, callback) {};

/**
 * @param {*} fd
 * @param {number} atime
 * @param {number} mtime
 * @nosideeffects
 */
fs.futimesSync = function(fd, atime, mtime) {};

/**
 * @param {*} fd
 * @param {function(...)=} callback
 */
fs.fsync = function(fd, callback) {};

/**
 * @param {*} fd
 */
fs.fsyncSync = function(fd) {};

/**
 * @param {*} fd
 * @param {*} buffer
 * @param {number} offset
 * @param {number} length
 * @param {number} position
 * @param {function(string, number, *)=} callback
 */
fs.write = function(fd, buffer, offset, length, position, callback) {};

/**
 * @param {*} fd
 * @param {*} buffer
 * @param {number} offset
 * @param {number} length
 * @param {number} position
 * @return {number}
 */
fs.writeSync = function(fd, buffer, offset, length, position) {};

/**
 * @param {*} fd
 * @param {*} buffer
 * @param {number} offset
 * @param {number} length
 * @param {number} position
 * @param {function(string, number, *)=} callback
 * @nosideeffects
 */
fs.read = function(fd, buffer, offset, length, position, callback) {};

/**
 * @param {*} fd
 * @param {*} buffer
 * @param {number} offset
 * @param {number} length
 * @param {number} position
 * @return {number}
 * @nosideeffects
 */
fs.readSync = function(fd, buffer, offset, length, position) {};

/**
 * @param {string} filename
 * @param {string=|function(string, *)=}encoding
 * @param {function(string, *)=} callback
 * @nosideeffects
 */
fs.readFile = function(filename, encoding, callback) {};

/**
 * @param {string} filename
 * @param {string=} encoding
 * @nosideeffects
 */
fs.readFileSync = function(filename, encoding) {};

/**
 * @param {string} filename
 * @param {*} data
 * @param {string=|function(string)=} encoding
 * @param {function(string)=} callback
 */
fs.writeFile = function(filename, data, encoding, callback) {};

/**
 * @param {string} filename
 * @param {*} data
 * @param {string=} encoding
 */
fs.writeFileSync = function(filename, data, encoding) {};

/**
 * @param {string} filename
 * @param {*} data
 * @param {string=|function(string)=} encoding
 * @param {function(string)=} callback
 */
fs.appendFile = function(filename, data, encoding, callback) {};

/**
 * @param {string} filename
 * @param {*} data
 * @param {string=|function(string)=} encoding
 */
fs.appendFileSync = function(filename, data, encoding) {};

/**
 * @param {string} filename
 * @param {{persistent: boolean, interval: number}=|function(*,*)} options
 * @param {function(*,*)=} listener
 */
fs.watchFile = function(filename, options, listener) {};

/**
 * @param {string} filename
 * @param {function=} listener
 */
fs.unwatchFile = function(filename, listener) {};

/**
 * 
 * @param {string} filename
 * @param {{persistent: boolean}=|function(string, string)} options
 * @param {function(string, string)=} listener
 * @return {fs.FSWatcher}
 */
fs.watch = function(filename, options, listener) {};

/**
 * @param {string} path
 * @param {function(boolean)} callback
 * @nosideeffects
 */
fs.exists = function(path, callback) {};

/**
 * @param {string} path
 * @nosideeffects
 */
fs.existsSync = function(path) {};

/**
 * @constructor
 */
fs.Stats = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isFile = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isDirectory = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isBlockDevice = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isCharacterDevice = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isSymbolicLink = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isFIFO = function() {};

/**
 * @return {boolean}
 * @nosideeffects
 */
fs.Stats.prototype.isSocket = function() {};

/**
 * @type {number}
 */
fs.Stats.prototype.dev = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.ino = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.mode = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.nlink = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.uid = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.gid = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.rdev = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.size = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.blkSize = 0;

/**
 * @type {number}
 */
fs.Stats.prototype.blocks = 0;

/**
 * @type {Date}
 */
fs.Stats.prototype.atime = 0;

/**
 * @type {Date}
 */
fs.Stats.prototype.mtime = 0;

/**
 * @type {Date}
 */
fs.Stats.prototype.ctime = 0;

/**
 * @param {string} path
 * @param {{flags: string, encoding: ?string, fd: *, mode: number, bufferSize: number}=} options
 * @nosideeffects
 */
fs.createReadStream = function(path, options) {};

/**
 * @constructor
 * @extends {stream.ReadableStream}
 */
fs.ReadStream = function() {};

/**
 * @param {string} path
 * @param {{flags: string, encoding: ?string, mode: number}=} options
 * @nosideeffects
 */
fs.createWriteStream = function(path, options) {};

/**
 * @constructor
 * @extends {stream.WritableStream}
 */
fs.WriteStream = function() {};

/**
 * @param {string} event
 * @param {function(...)} callback
 */
fs.WriteStream.prototype.on = function(event, callback) {};

/**
 * @constructor
 */
fs.FSWatcher = function() {};

/**
 */
fs.FSWatcher.prototype.close = function() {};

/**
 * @param {string} event
 * @param {function(...)} callback
 */
fs.FSWatcher.prototype.on = function(event, callback) {};
