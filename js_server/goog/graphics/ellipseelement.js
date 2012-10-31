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
 * @fileoverview A thin wrapper around the DOM element for ellipses.
 * @author arv@google.com (Erik Arvidsson)
 * @author yoah@google.com (Yoah Bar-David)
 */


goog.provide('goog.graphics.EllipseElement');

goog.require('goog.graphics.StrokeAndFillElement');



/**
 * Interface for a graphics ellipse element.
 * You should not construct objects from this constructor. The graphics
 * will return an implementation of this interface for you.
 * @param {Element} element The DOM element to wrap.
 * @param {goog.graphics.AbstractGraphics} graphics The graphics creating
 *     this element.
 * @param {goog.graphics.Stroke?} stroke The stroke to use for this element.
 * @param {goog.graphics.Fill?} fill The fill to use for this element.
 * @constructor
 * @extends {goog.graphics.StrokeAndFillElement}
 */
goog.graphics.EllipseElement = function(element, graphics, stroke, fill) {
  goog.graphics.StrokeAndFillElement.call(this, element, graphics, stroke,
      fill);
};
goog.inherits(goog.graphics.EllipseElement, goog.graphics.StrokeAndFillElement);


/**
 * Update the center point of the ellipse.
 * @param {number} cx  Center X coordinate.
 * @param {number} cy  Center Y coordinate.
 */
goog.graphics.EllipseElement.prototype.setCenter = goog.abstractMethod;


/**
 * Update the radius of the ellipse.
 * @param {number} rx  Radius length for the x-axis.
 * @param {number} ry  Radius length for the y-axis.
 */
goog.graphics.EllipseElement.prototype.setRadius = goog.abstractMethod;
