// Copyright 2012 The Closure Library Authors. All Rights Reserved.
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
 * @fileoverview A SimpleResult object that implements goog.labs.async.Result.
 * See below for a more detailed description.
 */

goog.provide('goog.labs.async.SimpleResult');
goog.provide('goog.labs.async.SimpleResult.StateError');

goog.require('goog.debug.Error');
goog.require('goog.labs.async.Result');



/**
 * A SimpleResult object is a basic implementation of the goog.labs.async.Result
 * interface. This could be subclassed(e.g. XHRResult) or instantiated and
 * returned by another class as a form of result. The caller receiving the
 * result could then attach handlers to be called when the result is resolved
 * (success or error).
 *
 * @constructor
 * @implements {goog.labs.async.Result}
 */
goog.labs.async.SimpleResult = function() {
  /**
   * The current state of this Result.
   * @type {goog.labs.async.Result.State}
   * @private
   */
  this.state_ = goog.labs.async.Result.State.PENDING;

  /**
   * The list of handlers to call when this Result is resolved.
   * @type {!Array.<!function(goog.labs.async.SimpleResult)>}
   * @private
   */
  this.handlers_ = [];
};


/**
 * The 'value' of this Result.
 * @type {*}
 * @private
 */
goog.labs.async.SimpleResult.prototype.value_;


/**
 * The error slug for this Result.
 * @type {*}
 * @private
 */
goog.labs.async.SimpleResult.prototype.error_;



/**
 * Error thrown if there is an attempt to set the value or error for this result
 * more than once.
 *
 * @constructor
 * @extends {goog.debug.Error}
 */
goog.labs.async.SimpleResult.StateError = function() {
  goog.base(this, 'Multiple attempts to set the state of this Result');
};
goog.inherits(goog.labs.async.SimpleResult.StateError, goog.debug.Error);


/** @override */
goog.labs.async.SimpleResult.prototype.getState = function() {
  return this.state_;
};


/** @override */
goog.labs.async.SimpleResult.prototype.getValue = function() {
  return this.value_;
};


/** @override */
goog.labs.async.SimpleResult.prototype.getError = function() {
  return this.error_;
};


/**
 * Attaches handlers to be called when the value of this Result is available.
 *
 * @param {!function(!goog.labs.async.SimpleResult)} handler The function called
 *     when the value is available. The function is passed the Result object
 *     as the only argument.
 * @override
 */
goog.labs.async.SimpleResult.prototype.wait = function(handler) {
  if (this.isPending_()) {
    this.handlers_.push(handler);
  } else {
    handler(this);
  }
};


/**
 * Sets the value of this Result, changing the state.
 *
 * @param {*} value The value to set for this Result.
 */
goog.labs.async.SimpleResult.prototype.setValue = function(value) {
  if (this.isPending_()) {
    this.value_ = value;
    this.state_ = goog.labs.async.Result.State.SUCCESS;
    this.callHandlers_();
  } else if (!this.isCanceled()) {
    // setValue is a no-op if this Result has been canceled.
    throw new goog.labs.async.SimpleResult.StateError();
  }
};


/**
 * Sets the Result to be an error Result.
 *
 * @param {*=} opt_error Optional error slug to set for this Result.
 */
goog.labs.async.SimpleResult.prototype.setError = function(opt_error) {
  if (this.isPending_()) {
    this.error_ = opt_error;
    this.state_ = goog.labs.async.Result.State.ERROR;
    this.callHandlers_();
  } else if (!this.isCanceled()) {
    // setError is a no-op if this Result has been canceled.
    throw new goog.labs.async.SimpleResult.StateError();
  }
};


/**
 * Calls the handlers registered for this Result.
 *
 * @private
 */
goog.labs.async.SimpleResult.prototype.callHandlers_ = function() {
  while (this.handlers_.length) {
    var callback = this.handlers_.shift();
    callback(this);
  }
};


/**
 * @return {boolean} Whether the Result is pending.
 * @private
 */
goog.labs.async.SimpleResult.prototype.isPending_ = function() {
  return this.state_ == goog.labs.async.Result.State.PENDING;
};


/**
 * Cancels the Result.
 *
 * @return {boolean} Whether the result was canceled. It will not be canceled if
 *    the result was already canceled or has already resolved.
 * @override
 */
goog.labs.async.SimpleResult.prototype.cancel = function() {
  // cancel is a no-op if the result has been resolved.
  if (this.isPending_()) {
    this.setError(new goog.labs.async.Result.CancelError());
    return true;
  }
  return false;
};


/** @override */
goog.labs.async.SimpleResult.prototype.isCanceled = function() {
  return this.state_ == goog.labs.async.Result.State.ERROR &&
         this.error_ instanceof goog.labs.async.Result.CancelError;
};
