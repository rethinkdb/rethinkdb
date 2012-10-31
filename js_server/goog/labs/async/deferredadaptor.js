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
 * @fileoverview An adaptor from a Result to a Deferred.
 *
 * TODO (vbhasin): cancel() support.
 * TODO (vbhasin): See if we can make this a static.
 * TODO (gboyer, vbhasin): Rename to "Adapter" once this graduates; this is the
 * proper programmer spelling.
 */


goog.provide('goog.labs.async.DeferredAdaptor');

goog.require('goog.async.Deferred');
goog.require('goog.labs.async.Result');
goog.require('goog.labs.async.wait');



/**
 * An adaptor from Result to a Deferred, for use with existing Deferred chains.
 *
 * @param {!goog.labs.async.Result} result A result.
 * @constructor
 * @extends {goog.async.Deferred}
 */
goog.labs.async.DeferredAdaptor = function(result) {
  goog.base(this);
  goog.labs.async.wait(result, function(result) {
    if (this.hasFired()) {
      return;
    }
    if (result.getState() == goog.labs.async.Result.State.SUCCESS) {
      this.callback(result.getValue());
    } else if (result.getState() == goog.labs.async.Result.State.ERROR) {
      this.errback(result.getError());
    }
  }, this);
};
goog.inherits(goog.labs.async.DeferredAdaptor, goog.async.Deferred);
