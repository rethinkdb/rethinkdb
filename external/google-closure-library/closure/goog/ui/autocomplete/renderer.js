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
 * For actual documentation, please see {@link goog.ui.ac.Renderer}.
 *
 */

goog.provide('goog.ui.AutoComplete.Renderer');
goog.provide('goog.ui.AutoComplete.Renderer.CustomRenderer');

goog.require('goog.ui.AutoComplete');
goog.require('goog.ui.ac.Renderer');
goog.require('goog.ui.ac.Renderer.CustomRenderer');



//TODO(user): Remove this after known usages are replaced.
/**
 * This is a stub for backward compatibility.
 * For actual documentation, please see {@link goog.ui.ac.Renderer}.
 *
 * @constructor
 * @param {Element=} opt_parentNode Opt_parentNode.
 * @param {?({renderRow}|{render})=} opt_customRenderer Opt_customRenderer.
 * @param {boolean=} opt_rightAlign Opt_rightAlign.
 * @param {boolean=} opt_useStandardHighlighting Opt_useStandardHighlighting.
 * @extends {goog.events.EventTarget}
 * @deprecated Use {@link goog.ui.ac.Renderer} instead.
 */
goog.ui.AutoComplete.Renderer = goog.ui.ac.Renderer;


/**
 * This is a stub for backward compatibility.
 * For actual documentation, please see {@link goog.ui.ac.Renderer}.
 * @type {number}
 */
goog.ui.AutoComplete.Renderer.DELAY_BEFORE_MOUSEOVER =
    goog.ui.ac.Renderer.DELAY_BEFORE_MOUSEOVER;



/**
 * This is a stub for backward compatibility. For actual documentation,
 * please see {@link goog.ui.ac.Renderer.CustomRenderer}.
 * @constructor
 */
goog.ui.AutoComplete.Renderer.CustomRenderer =
    goog.ui.ac.Renderer.CustomRenderer;
