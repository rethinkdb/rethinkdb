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
 * @fileoverview Defines a static 'transform' function that provide a convenient
 * way to transform results of asynchronous operations.
 *
 * Example:
 *  <pre>
 *
 *  var result = xhr.getJson('testdata/xhr_test_json.data');
 *
 *  // Transform contents of returned data using 'processJson' and create a
 *  // transformed result to use returned JSON.
 *  var transformedResult = goog.labs.async.transform(result, processJson);
 *
 *  // Attach success and failure handlers to the tranformed result.
 *  goog.labs.async.wait.onSuccess(transformedResult, function(result) {
 *    var jsonData = result.getValue();
 *    assertEquals('ok', jsonData['stat']);
 *  });
 *
 *  goog.labs.async.wait.onError(transformedResult, function(error) {
 *    // Failed getJson call
 *  });
 *  </pre>
 *
 */


goog.provide('goog.labs.async.transform');

goog.require('goog.labs.async.Result');
goog.require('goog.labs.async.SimpleResult');
goog.require('goog.labs.async.wait');


/**
 * Given a result and a transform function, returns a new result whose value,
 * on success, will be the value of the given result after having been passed
 * through the transform function.
 *
 * If the given result is an error, the returned result is also an error and the
 * transform will not be called.
 *
 * @param {!goog.labs.async.Result} result The result whose value will be
 *     transformed.
 * @param {!Function} transformer The transformer
 *     function. The return value of this function will become the value of the
 *     returned result.
 *
 * @return {!goog.labs.async.Result} A new Result whose eventual value will be
 *     the returned value of the transformer function.
 */
goog.labs.async.transform = function(result, transformer) {
  var returnedResult = new goog.labs.async.SimpleResult();

  goog.labs.async.wait(result, function(res) {
    if (res.getState() == goog.labs.async.Result.State.SUCCESS) {
      returnedResult.setValue(transformer(res.getValue()));
    } else {
      returnedResult.setError(res.getError());
    }
  });

  return returnedResult;
};
