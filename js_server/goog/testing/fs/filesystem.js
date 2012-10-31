// Copyright 2011 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Mock filesystem object.
 *
 */

goog.provide('goog.testing.fs.FileSystem');

goog.require('goog.testing.fs.DirectoryEntry');



/**
 * A mock filesystem object.
 *
 * @param {string=} opt_name The name of the filesystem.
 * @constructor
 */
goog.testing.fs.FileSystem = function(opt_name) {
  /**
   * The name of the filesystem.
   * @type {string}
   * @private
   */
  this.name_ = opt_name || 'goog.testing.fs.FileSystem';

  /**
   * The root entry of the filesystem.
   * @type {!goog.testing.fs.DirectoryEntry}
   * @private
   */
  this.root_ = new goog.testing.fs.DirectoryEntry(this, null, '', {});
};


/**
 * @see {goog.fs.FileSystem#getName}
 * @return {string}
 */
goog.testing.fs.FileSystem.prototype.getName = function() {
  return this.name_;
};


/**
 * @see {goog.fs.FileSystem#getRoot}
 * @return {!goog.testing.fs.DirectoryEntry}
 */
goog.testing.fs.FileSystem.prototype.getRoot = function() {
  return this.root_;
};
