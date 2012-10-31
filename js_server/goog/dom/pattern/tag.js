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
 * @fileoverview DOM pattern to match a tag.
 *
 * @author robbyw@google.com (Robby Walker)
 */

goog.provide('goog.dom.pattern.Tag');

goog.require('goog.dom.pattern');
goog.require('goog.dom.pattern.AbstractPattern');
goog.require('goog.dom.pattern.MatchType');
goog.require('goog.object');



/**
 * Pattern object that matches an tag.
 *
 * @param {string|RegExp} tag Name of the tag.  Also will accept a regular
 *     expression to match against the tag name.
 * @param {goog.dom.TagWalkType} type Type of token to match.
 * @param {Object=} opt_attrs Optional map of attribute names to desired values.
 *     This pattern will only match when all attributes are present and match
 *     the string or regular expression value provided here.
 * @param {Object=} opt_styles Optional map of CSS style names to desired
 *     values. This pattern will only match when all styles are present and
 *     match the string or regular expression value provided here.
 * @param {Function=} opt_test Optional function that takes the element as a
 *     parameter and returns true if this pattern should match it.
 * @constructor
 * @extends {goog.dom.pattern.AbstractPattern}
 */
goog.dom.pattern.Tag = function(tag, type, opt_attrs, opt_styles, opt_test) {
  if (goog.isString(tag)) {
    this.tag_ = tag.toUpperCase();
  } else {
    this.tag_ = tag;
  }

  this.type_ = type;

  this.attrs_ = opt_attrs || null;
  this.styles_ = opt_styles || null;
  this.test_ = opt_test || null;
};
goog.inherits(goog.dom.pattern.Tag, goog.dom.pattern.AbstractPattern);


/**
 * The tag to match.
 *
 * @type {string|RegExp}
 * @private
 */
goog.dom.pattern.Tag.prototype.tag_;


/**
 * The type of token to match.
 *
 * @type {goog.dom.TagWalkType}
 * @private
 */
goog.dom.pattern.Tag.prototype.type_;


/**
 * The attributes to test for.
 *
 * @type {Object}
 * @private
 */
goog.dom.pattern.Tag.prototype.attrs_ = null;


/**
 * The styles to test for.
 *
 * @type {Object}
 * @private
 */
goog.dom.pattern.Tag.prototype.styles_ = null;


/**
 * Function that takes the element as a parameter and returns true if this
 * pattern should match it.
 *
 * @type {Function}
 * @private
 */
goog.dom.pattern.Tag.prototype.test_ = null;


/**
 * Test whether the given token is a tag token which matches the tag name,
 * style, and attributes provided in the constructor.
 *
 * @param {Node} token Token to match against.
 * @param {goog.dom.TagWalkType} type The type of token.
 * @return {goog.dom.pattern.MatchType} <code>MATCH</code> if the pattern
 *     matches, <code>NO_MATCH</code> otherwise.
 * @override
 */
goog.dom.pattern.Tag.prototype.matchToken = function(token, type) {
  // Check the direction and tag name.
  if (type == this.type_ &&
      goog.dom.pattern.matchStringOrRegex(this.tag_, token.nodeName)) {
    // Check the attributes.
    if (this.attrs_ &&
        !goog.object.every(
            this.attrs_,
            goog.dom.pattern.matchStringOrRegexMap,
            token)) {
      return goog.dom.pattern.MatchType.NO_MATCH;
    }
    // Check the styles.
    if (this.styles_ &&
        !goog.object.every(
            this.styles_,
            goog.dom.pattern.matchStringOrRegexMap,
            token.style)) {
      return goog.dom.pattern.MatchType.NO_MATCH;
    }

    if (this.test_ && !this.test_(token)) {
      return goog.dom.pattern.MatchType.NO_MATCH;
    }

    // If we reach this point, we have a match and should save it.
    this.matchedNode = token;
    return goog.dom.pattern.MatchType.MATCH;
  }

  return goog.dom.pattern.MatchType.NO_MATCH;
};
