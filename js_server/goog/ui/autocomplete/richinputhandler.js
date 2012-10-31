// Copyright 2007 The Closure Library Authors. All Rights Reserved.
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
 * @fileoverview This is a stub for backward compatibility.
 * For actual documentation, please see {@link goog.ui.ac.RichInputHandler}.
 *
 */

goog.provide('goog.ui.AutoComplete.RichInputHandler');

goog.require('goog.ui.AutoComplete');
/**
 * @suppress {extraRequire} This is left here only for dependency management
 *     until all existing usages are transitioned to the new namespace.
 */
goog.require('goog.ui.AutoComplete.InputHandler');
goog.require('goog.ui.ac.RichInputHandler');



//TODO(user): Remove this after known usages are replaced.
/**
 * This is a stub for backward compatibility.
 * For actual documentation, please see {@link goog.ui.ac.RichInputHandler}.
 * @param {?string=} opt_separators Opt_separators.
 * @param {?string=} opt_literals Opt_literals.
 * @param {?boolean=} opt_multi Opt_multi.
 * @param {?number=} opt_throttleTime Opt_throttleTime.
 * @constructor
 * @extends {goog.ui.ac.InputHandler}
 * @deprecated Use {@link goog.ui.ac.RichInputHandler} instead.
 */
goog.ui.AutoComplete.RichInputHandler = goog.ui.ac.RichInputHandler;
