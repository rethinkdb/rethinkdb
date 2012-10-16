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
 * Tests for {@code goog.positioning.ClientPosition}
 */

goog.provide('goog.positioning.clientPositionTest');
goog.setTestOnly('goog.positioning.clientPositionTest');

goog.require('goog.dom');
goog.require('goog.positioning.ClientPosition');
goog.require('goog.style');
goog.require('goog.testing.jsunit');


/**
 * Prefabricated popup element for convenient. This is created during
 * setUp and is not attached to the document at the beginning of the
 * test.
 * @type {Element}
 */
var popupElement;
var testArea;
var POPUP_HEIGHT = 100;
var POPUP_WIDTH = 150;


function setUp() {
  testArea = goog.dom.getElement('test-area');

  // Enlarges the test area to 5000x5000px so that we can be confident
  // that scrolling the document to some small (x,y) value would work.
  goog.style.setSize(testArea, 5000, 5000);

  window.scrollTo(0, 0);

  popupElement = goog.dom.createDom('div');
  goog.style.setSize(popupElement, POPUP_WIDTH, POPUP_HEIGHT);
  popupElement.style.position = 'absolute';

  // For ease of debugging.
  popupElement.style.background = 'blue';
}


function tearDown() {
  popupElement = null;
  testArea.innerHTML = '';
  testArea.setAttribute('style', '');
}


function testClientPositionWithZeroViewportOffset() {
  goog.dom.appendChild(testArea, popupElement);

  var x = 300;
  var y = 200;
  var pos = new goog.positioning.ClientPosition(x, y);

  pos.reposition(popupElement, goog.positioning.Corner.TOP_LEFT);
  assertPageOffset(x, y, popupElement);

  pos.reposition(popupElement, goog.positioning.Corner.TOP_RIGHT);
  assertPageOffset(x - POPUP_WIDTH, y, popupElement);

  pos.reposition(popupElement, goog.positioning.Corner.BOTTOM_LEFT);
  assertPageOffset(x, y - POPUP_HEIGHT, popupElement);

  pos.reposition(popupElement, goog.positioning.Corner.BOTTOM_RIGHT);
  assertPageOffset(x - POPUP_WIDTH, y - POPUP_HEIGHT, popupElement);
}


function testClientPositionWithSomeViewportOffset() {
  goog.dom.appendChild(testArea, popupElement);

  var x = 300;
  var y = 200;
  var scrollX = 135;
  var scrollY = 270;
  window.scrollTo(scrollX, scrollY);

  var pos = new goog.positioning.ClientPosition(x, y);
  pos.reposition(popupElement, goog.positioning.Corner.TOP_LEFT);
  assertPageOffset(scrollX + x, scrollY + y, popupElement);
}


function testClientPositionWithPositionContext() {
  var contextAbsoluteX = 90;
  var contextAbsoluteY = 110;
  var x = 300;
  var y = 200;

  var contextElement = goog.dom.createDom('div', undefined, popupElement);
  goog.style.setPosition(contextElement, contextAbsoluteX, contextAbsoluteY);
  contextElement.style.position = 'absolute';
  goog.dom.appendChild(testArea, contextElement);

  var pos = new goog.positioning.ClientPosition(x, y);
  pos.reposition(popupElement, goog.positioning.Corner.TOP_LEFT);
  assertPageOffset(x, y, popupElement);
}


function assertPageOffset(expectedX, expectedY, el) {
  var offsetCoordinate = goog.style.getPageOffset(el);
  assertEquals('x-coord page offset is wrong.', expectedX, offsetCoordinate.x);
  assertEquals('y-coord page offset is wrong.', expectedY, offsetCoordinate.y);
}
