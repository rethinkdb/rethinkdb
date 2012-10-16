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
 * @fileoverview Simple image loader, used for preloading.
 * @author nnaze@google.com (Nathan Naze)
 */

goog.provide('goog.labs.net.image');

goog.require('goog.events.EventHandler');
goog.require('goog.labs.async.SimpleResult');
goog.require('goog.net.EventType');


/**
 * Loads a single image.  Useful for preloading images. May be combined with
 * goog.labs.async.combine to preload many images.
 *
 * @param {string} uri URI of the image.
 * @return {!goog.labs.async.Result} An asyncronous result that will succeed
 *     if the image successfully loads or error if the image load fails.
 */
goog.labs.net.image.load = function(uri) {

  // TODO(nnaze): If needed, allow a way to specify how the image is created
  // (in case an element needs to be created for a specific document, for
  // example). There was a mechanism for this in goog.net.ImageLoader, which
  // this file is intended to replace.

  var image = new Image();

  // IE's load event on images can be buggy.  Instead, we wait for
  // readystatechange events and check if readyState is 'complete'.
  // See:
  // http://msdn.microsoft.com/en-us/library/ie/ms536957(v=vs.85).aspx
  // http://msdn.microsoft.com/en-us/library/ie/ms534359(v=vs.85).aspx
  var loadEvent = goog.userAgent.IE ? goog.net.EventType.READY_STATE_CHANGE :
      goog.events.EventType.LOAD;

  var result = new goog.labs.async.SimpleResult();

  var handler = new goog.events.EventHandler();
  handler.listen(
      image,
      [loadEvent, goog.net.EventType.ABORT, goog.net.EventType.ERROR],
      function(e) {

        // We only registered listeners for READY_STATE_CHANGE for IE.
        // If readyState is now COMPLETE, the image has loaded.
        // See related comment above.
        if (e.type == goog.net.EventType.READY_STATE_CHANGE &&
            image.readyState != goog.net.EventType.COMPLETE) {
          return;
        }

        // At this point, we know whether the image load was successful
        // and no longer care about image events.
        goog.dispose(handler);

        // Whether the image successfully loaded.
        if (e.type == loadEvent) {
          result.setValue(image);
        } else {
          result.setError();
        }
      });

  // Initiate the image request.
  image.src = uri;

  return result;
};
