// Copyright 2006 The Closure Library Authors. All Rights Reserved.
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
 * For actual documentation, please see {@link goog.ui.ac.AutoComplete}.
 *
 * @see ../../demos/autocomplete-basic.html
 */

goog.provide('goog.ui.AutoComplete');
goog.provide('goog.ui.AutoComplete.EventType');

goog.require('goog.ui.ac.AutoComplete');
goog.require('goog.ui.ac.AutoComplete.EventType');



// TODO(user): Remove this after known usages are replaced.
/**
 * This is a stub for backward compatibility. For actual documentation,
 * please see {@link goog.ui.ac.AutoComplete}.
 *
 * @param {Object} matcher A matcher.
 * @param {goog.events.EventTarget} renderer A renderer.
 * @param {Object} selectionHandler A selectionHandler.
 *
 * @constructor
 * @extends {goog.events.EventTarget}
 * @deprecated Use {@link goog.ui.ac.AutoComplete} instead.
 */
goog.ui.AutoComplete = goog.ui.ac.AutoComplete;


/**
 * This is a stub for backward compatibility. For actual documentation,
 * please see {@link goog.ui.ac.AutoComplete.EventType}.
 * @enum {string}
 */
goog.ui.AutoComplete.EventType = goog.ui.ac.AutoComplete.EventType;
