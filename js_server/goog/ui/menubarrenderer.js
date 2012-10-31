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
 * @fileoverview Renderer for {@link goog.ui.menuBar}.
 *
 */

goog.provide('goog.ui.MenuBarRenderer');

goog.require('goog.dom');
goog.require('goog.dom.a11y');
goog.require('goog.dom.a11y.Role');
goog.require('goog.dom.a11y.State');
goog.require('goog.ui.ContainerRenderer');



/**
 * Default renderer for {@link goog.ui.menuBar}s, based on {@link
 * goog.ui.ContainerRenderer}.
 * @constructor
 * @extends {goog.ui.ContainerRenderer}
 */
goog.ui.MenuBarRenderer = function() {
  goog.base(this);
};
goog.inherits(goog.ui.MenuBarRenderer, goog.ui.ContainerRenderer);
goog.addSingletonGetter(goog.ui.MenuBarRenderer);


/**
 * Default CSS class to be applied to the root element of elements rendered
 * by this renderer.
 * @type {string}
 */
goog.ui.MenuBarRenderer.CSS_CLASS = goog.getCssName('goog-menubar');


/**
 * @override
 */
goog.ui.MenuBarRenderer.prototype.getAriaRole = function() {
  return goog.dom.a11y.Role.MENUBAR;
};


/**
 * @override
 */
goog.ui.MenuBarRenderer.prototype.getCssClass = function() {
  return goog.ui.MenuBarRenderer.CSS_CLASS;
};


/**
 * Returns the default orientation of containers rendered or decorated by this
 * renderer.  This implementation returns {@code HORIZONTAL}.
 * @return {goog.ui.Container.Orientation} Default orientation for containers
 *     created or decorated by this renderer.
 * @override
 */
goog.ui.MenuBarRenderer.prototype.getDefaultOrientation = function() {
  return goog.ui.Container.Orientation.HORIZONTAL;
};
