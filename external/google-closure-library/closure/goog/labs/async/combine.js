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
 * @fileoverview Defines static 'combine' functions that provide a convenient
 * way to wait on multiple asynchronous Results.
 *
 * Example:
 *  <pre>
 *
 *  var result1 = xhr.get('testdata/xhr_test_text.data');
 *
 *  // Get a second independent Result.
 *  var result2 = xhr.getJson('testdata/xhr_test_json.data');
 *
 *  // Create a Result that resolves when both prior results resolve.
 *  var combinedResult = goog.labs.async.combine.onSuccess(result1, result2);
 *
 *  // Process data after successful resolution of both results.
 *  goog.labs.async.wait.onSuccess(combinedResult, function(results) {
 *    var textData = results[0].getValue();
 *    var jsonData = results[1].getValue();
 *    assertEquals('Just some data.', textData);
 *    assertEquals('ok', jsonData['stat']);
 *  });
 *
 *  // Handle errors when either or both results failed.
 *  goog.labs.async.wait.onError(combinedResult, function(combined) {
 *    var results = combined.getError();
 *
 *    if (results[0].getState() == goog.labs.async.Result.State.ERROR) {
 *      alert('result1 failed');
 *    }
 *
 *    if (results[1].getState() == goog.labs.async.Result.State.ERROR) {
 *      alert('result2 failed');
 *    }
 *  });
 *  </pre>
 *
 */


goog.provide('goog.labs.async.combine');

goog.require('goog.array');
goog.require('goog.labs.async.Result');
goog.require('goog.labs.async.SimpleResult');
goog.require('goog.labs.async.wait');


/**
 * Returns a result that waits on all given results to resolve. Once all have
 * resolved, the returned result will succeed (and never error).
 *
 * @param {...!goog.labs.async.Result} var_args The results to wait on.
 *
 * @return {!goog.labs.async.Result} A new Result whose eventual value will be
 *     the resolved given Result objects.
 */
goog.labs.async.combine = function(var_args) {
  var results = goog.array.clone(arguments);
  var combinedResult = new goog.labs.async.SimpleResult();

  var isResolved = function(res) {
    return res.getState() != goog.labs.async.Result.State.PENDING;
  };

  var checkResults = function() {
    if (goog.array.every(results, isResolved)) {
      combinedResult.setValue(results);
    }
  };

  goog.array.forEach(results, function(result) {
    goog.labs.async.wait(result, checkResults);
  });

  return combinedResult;
};


/**
 * Returns a result that waits on all given results to resolve. Once all have
 * resolved, the returned result will succeed if and only if all given results
 * succeeded. Otherwise it will error.
 *
 * @param {...!goog.labs.async.Result} var_args The results to wait on.
 *
 * @return {!goog.labs.async.Result} A new Result whose eventual value will be
 *     an array of values of the given Result objects.
 */
goog.labs.async.combine.onSuccess = function(var_args) {
  var combinedResult = new goog.labs.async.SimpleResult();

  var resolvedSuccessfully = function(res) {
    return res.getState() == goog.labs.async.Result.State.SUCCESS;
  };

  goog.labs.async.wait(
      goog.labs.async.combine.apply(goog.labs.async.combine, arguments),
      // The combined result never ERRORs
      function(res) {
        var results = /** @type {Array} */ (res.getValue());
        if (goog.array.every(results, resolvedSuccessfully)) {
          combinedResult.setValue(results);
        } else {
          combinedResult.setError(results);
        }
      });

  return combinedResult;
};
