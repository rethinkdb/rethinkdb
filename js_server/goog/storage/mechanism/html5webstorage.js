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
 * @fileoverview Base class that implements functionality common
 * across both session and local web storage mechanisms.
 *
 */

goog.provide('goog.storage.mechanism.HTML5WebStorage');

goog.require('goog.asserts');
goog.require('goog.iter.Iterator');
goog.require('goog.iter.StopIteration');
goog.require('goog.storage.mechanism.ErrorCode');
goog.require('goog.storage.mechanism.IterableMechanism');



/**
 * Provides a storage mechanism that uses HTML5 Web storage.
 *
 * @param {?Storage} storage The Web storage object.
 * @constructor
 * @extends {goog.storage.mechanism.IterableMechanism}
 */
goog.storage.mechanism.HTML5WebStorage = function(storage) {
  goog.base(this);
  this.storage_ = storage;
};
goog.inherits(goog.storage.mechanism.HTML5WebStorage,
              goog.storage.mechanism.IterableMechanism);


/**
 * The web storage object (window.localStorage or window.sessionStorage).
 *
 * @type {Storage}
 * @private
 */
goog.storage.mechanism.HTML5WebStorage.prototype.storage_;


/**
 * Determines whether or not the mechanism is available.
 * It works only if the provided web storage object exists and is enabled.
 *
 * @return {boolean} True if the mechanism is available.
 */
goog.storage.mechanism.HTML5WebStorage.prototype.isAvailable = function() {
  /** @preserveTry */
  try {
    // May throw a security exception if web storage is disabled.
    return !!this.storage_ && !!this.storage_.getItem;
  } catch (e) {}
  return false;
};


/** @override */
goog.storage.mechanism.HTML5WebStorage.prototype.set = function(key, value) {
  /** @preserveTry */
  try {
    // May throw an exception if storage quota is exceeded.
    this.storage_.setItem(key, value);
  } catch (e) {
    throw goog.storage.mechanism.ErrorCode.QUOTA_EXCEEDED;
  }
};


/** @override */
goog.storage.mechanism.HTML5WebStorage.prototype.get = function(key) {
  // According to W3C specs, values can be of any type. Since we only save
  // strings, any other type is a storage error. If we returned nulls for
  // such keys, i.e., treated them as non-existent, this would lead to a
  // paradox where a key exists, but it does not when it is retrieved.
  // http://www.w3.org/TR/2009/WD-webstorage-20091029/#the-storage-interface
  var value = this.storage_.getItem(key);
  if (goog.isString(value) || goog.isNull(value)) {
    return value;
  }
  throw goog.storage.mechanism.ErrorCode.INVALID_VALUE;
};


/** @override */
goog.storage.mechanism.HTML5WebStorage.prototype.remove = function(key) {
  this.storage_.removeItem(key);
};


/** @override */
goog.storage.mechanism.HTML5WebStorage.prototype.getCount = function() {
  return this.storage_.length;
};


/** @override */
goog.storage.mechanism.HTML5WebStorage.prototype.__iterator__ = function(
    opt_keys) {
  var i = 0;
  var newIter = new goog.iter.Iterator;
  var selfObj = this;
  newIter.next = function() {
    if (i >= selfObj.getCount()) {
      throw goog.iter.StopIteration;
    }
    var key = goog.asserts.assertString(selfObj.storage_.key(i++));
    if (opt_keys) {
      return key;
    }
    var value = selfObj.storage_.getItem(key);
    // The value must exist and be a string, otherwise it is a storage error.
    if (goog.isString(value)) {
      return value;
    }
    throw goog.storage.mechanism.ErrorCode.INVALID_VALUE;
  };
  return newIter;
};


/** @override */
goog.storage.mechanism.HTML5WebStorage.prototype.clear = function() {
  this.storage_.clear();
};
