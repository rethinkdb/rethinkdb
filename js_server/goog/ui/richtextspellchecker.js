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
 * @fileoverview Rich text spell checker implementation.
 *
 * @author eae@google.com (Emil A Eklund)
 * @author sergeys@google.com (Sergey Solyanik)
 * @see ../demos/richtextspellchecker.html
 */

goog.provide('goog.ui.RichTextSpellChecker');

goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.dom.NodeType');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.string.StringBuffer');
goog.require('goog.ui.AbstractSpellChecker');
goog.require('goog.ui.AbstractSpellChecker.AsyncResult');



/**
 * Rich text spell checker implementation.
 *
 * @param {goog.spell.SpellCheck} handler Instance of the SpellCheckHandler
 *     support object to use. A single instance can be shared by multiple editor
 *     components.
 * @param {goog.dom.DomHelper=} opt_domHelper Optional DOM helper.
 * @constructor
 * @extends {goog.ui.AbstractSpellChecker}
 */
goog.ui.RichTextSpellChecker = function(handler, opt_domHelper) {
  goog.ui.AbstractSpellChecker.call(this, handler, opt_domHelper);

  /**
   * String buffer for use in reassembly of the original text.
   * @type {goog.string.StringBuffer}
   * @private
   */
  this.workBuffer_ = new goog.string.StringBuffer();

  /**
   * Bound async function (to avoid rebinding it on every call).
   * @type {Function}
   * @private
   */
  this.boundContinueAsyncFn_ = goog.bind(this.continueAsync_, this);
};
goog.inherits(goog.ui.RichTextSpellChecker, goog.ui.AbstractSpellChecker);


/**
 * Root node for rich editor.
 * @type {Node}
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.rootNode_;


/**
 * Current node where spell checker has interrupted to go to the next stack
 * frame.
 * @type {Node}
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.currentNode_;


/**
 * Counter of inserted elements. Used in processing loop to attempt to preserve
 * existing nodes if they contain no misspellings.
 * @type {number}
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.elementsInserted_ = 0;


/**
 * Number of words to scan to precharge the dictionary.
 * @type {number}
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.dictionaryPreScanSize_ = 1000;


/**
 * Class name for word spans.
 * @type {string}
 */
goog.ui.RichTextSpellChecker.prototype.wordClassName =
    goog.getCssName('goog-spellcheck-word');


/**
 * DomHelper to be used for interacting with the editable document/element.
 *
 * @type {goog.dom.DomHelper|undefined}
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.editorDom_;


/**
 * Tag name porition of the marker for the text that does not need to be checked
 * for spelling.
 *
 * @type {Array.<string|undefined>}
 */
goog.ui.RichTextSpellChecker.prototype.excludeTags;


/**
 * CSS Style text for invalid words. As it's set inside the rich edit iframe
 * classes defined in the parent document are not availble, thus the style is
 * set inline.
 * @type {string}
 */
goog.ui.RichTextSpellChecker.prototype.invalidWordCssText =
    'background: yellow;';


/**
 * Creates the initial DOM representation for the component.
 *
 * @throws {Error} Not supported. Use decorate.
 * @see #decorate
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.createDom = function() {
  throw Error('Render not supported for goog.ui.RichTextSpellChecker.');
};


/**
 * Decorates the element for the UI component.
 *
 * @param {Element} element Element to decorate.
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.decorateInternal = function(element) {
  this.setElementInternal(element);
  if (element.contentDocument || element.contentWindow) {
    var doc = element.contentDocument || element.contentWindow.document;
    this.rootNode_ = doc.body;
    this.editorDom_ = goog.dom.getDomHelper(doc);
  } else {
    this.rootNode_ = element;
    this.editorDom_ = goog.dom.getDomHelper(element);
  }
};


/** @override */
goog.ui.RichTextSpellChecker.prototype.enterDocument = function() {
  goog.ui.RichTextSpellChecker.superClass_.enterDocument.call(this);
  this.initSuggestionsMenu();
};


/**
 * Checks spelling for all text and displays correction UI.
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.check = function() {
  this.blockReadyEvents();
  this.preChargeDictionary_(this.rootNode_, this.dictionaryPreScanSize_);
  this.unblockReadyEvents();

  goog.events.listen(this.handler_, goog.spell.SpellCheck.EventType.READY,
                     this.onDictionaryCharged_, true, this);
  this.handler_.processPending();
};


/**
 * Processes nodes recursively.
 *
 * @param {Node} node Node to start with.
 * @param {number} words Max number of words to process.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.preChargeDictionary_ = function(node,
                                                                       words) {
  while (node) {
    var next = this.nextNode_(node);
    if (this.isExcluded_(node)) {
      node = next;
      continue;
    }
    if (node.nodeType == goog.dom.NodeType.TEXT) {
      if (node.nodeValue) {
        words -= this.populateDictionary(node.nodeValue, words);
        if (words <= 0) {
          return;
        }
      }
    } else if (node.nodeType == goog.dom.NodeType.ELEMENT) {
      if (node.firstChild) {
        next = node.firstChild;
      }
    }
    node = next;
  }
};


/**
 * Starts actual processing after the dictionary is charged.
 * @param {goog.events.Event} e goog.spell.SpellCheck.EventType.READY event.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.onDictionaryCharged_ = function(e) {
  e.stopPropagation();
  goog.events.unlisten(this.handler_, goog.spell.SpellCheck.EventType.READY,
                       this.onDictionaryCharged_, true, this);

  // Now actually do the spell checking.
  this.clearWordElements();
  this.initializeAsyncMode();
  this.elementsInserted_ = 0;
  var result = this.processNode_(this.rootNode_);
  if (result == goog.ui.AbstractSpellChecker.AsyncResult.PENDING) {
    goog.Timer.callOnce(this.boundContinueAsyncFn_);
    return;
  }
  this.finishAsyncProcessing();
  this.finishCheck_();
};


/**
 * Continues asynchrnonous spell checking.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.continueAsync_ = function() {
  var result = this.continueAsyncProcessing();
  if (result == goog.ui.AbstractSpellChecker.AsyncResult.PENDING) {
    goog.Timer.callOnce(this.boundContinueAsyncFn_);
    return;
  }
  result = this.processNode_(this.currentNode_);
  if (result == goog.ui.AbstractSpellChecker.AsyncResult.PENDING) {
    goog.Timer.callOnce(this.boundContinueAsyncFn_);
    return;
  }
  this.finishAsyncProcessing();
  this.finishCheck_();
};


/**
 * Finalizes spelling check.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.finishCheck_ = function() {
  delete this.currentNode_;
  this.handler_.processPending();

  if (!this.isVisible()) {
    goog.events.listen(this.rootNode_, goog.events.EventType.CLICK,
                       this.onWordClick_, false, this);
  }
  goog.ui.RichTextSpellChecker.superClass_.check.call(this);
};


/**
 * Finds next node in our enumeration of the tree.
 *
 * @param {Node} node The node to which we're computing the next node for.
 * @return {Node} The next node or null if none was found.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.nextNode_ = function(node) {
  while (node != this.rootNode_) {
    if (node.nextSibling) {
      return node.nextSibling;
    }
    node = node.parentNode;
  }
  return null;
};


/**
 * Determines if the node is text node without any children.
 *
 * @param {Node} node The node to check.
 * @return {boolean} Whether the node is a text leaf node.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.isTextLeaf_ = function(node) {
  return node != null &&
         node.nodeType == goog.dom.NodeType.TEXT &&
         !node.firstChild;
};


/** @override */
goog.ui.RichTextSpellChecker.prototype.setExcludeMarker = function(marker) {
  if (marker) {
    if (typeof marker == 'string') {
      marker = [marker];
    }

    this.excludeTags = [];
    this.excludeMarker = [];
    for (var i = 0; i < marker.length; i++) {
      var parts = marker[i].split('.');
      if (parts.length == 2) {
        this.excludeTags.push(parts[0]);
        this.excludeMarker.push(parts[1]);
      } else {
        this.excludeMarker.push(parts[0]);
        this.excludeTags.push(undefined);
      }
    }
  }
};


/**
 * Determines if the node is excluded from checking.
 *
 * @param {Node} node The node to check.
 * @return {boolean} Whether the node is excluded.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.isExcluded_ = function(node) {
  if (this.excludeMarker && node.className) {
    for (var i = 0; i < this.excludeMarker.length; i++) {
      var excludeTag = this.excludeTags[i];
      var excludeClass = this.excludeMarker[i];
      var isExcluded = !!(excludeClass &&
          node.className.indexOf(excludeClass) != -1 &&
          (!excludeTag || node.tagName == excludeTag));
      if (isExcluded) {
        return true;
      }
    }
  }
  return false;
};


/**
 * Processes nodes recursively.
 *
 * @param {Node} node Node where to start.
 * @return {goog.ui.AbstractSpellChecker.AsyncResult|undefined} Result code.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.processNode_ = function(node) {
  delete this.currentNode_;
  while (node) {
    var next = this.nextNode_(node);
    if (this.isExcluded_(node)) {
      node = next;
      continue;
    }
    if (node.nodeType == goog.dom.NodeType.TEXT) {
      var deleteNode = true;
      if (node.nodeValue) {
        var currentElements = this.elementsInserted_;
        var result = this.processTextAsync(node, node.nodeValue);
        if (result == goog.ui.AbstractSpellChecker.AsyncResult.PENDING) {
          // This markes node for deletion (empty nodes get deleted couple
          // of lines down this function). This is so our algorithm terminates.
          // In this case the node may be needlessly recreated, but it
          // happens rather infrequently and saves a lot of code.
          node.nodeValue = '';
          this.currentNode_ = node;
          return result;
        }
        // If we did not add nodes in processing, the current element is still
        // valid. Let's preserve it!
        if (currentElements == this.elementsInserted_) {
          deleteNode = false;
        }
      }
      if (deleteNode) {
        goog.dom.removeNode(node);
      }
    } else if (node.nodeType == goog.dom.NodeType.ELEMENT) {
      // If this is a spell checker element...
      if (node.className == this.wordClassName) {
        // First, reconsolidate the text nodes inside the element - editing
        // in IE splits them up.
        var runner = node.firstChild;
        while (runner) {
          if (this.isTextLeaf_(runner)) {
            while (this.isTextLeaf_(runner.nextSibling)) {
              // Yes, this is not super efficient in IE, but it will almost
              // never happen.
              runner.nodeValue += runner.nextSibling.nodeValue;
              goog.dom.removeNode(runner.nextSibling);
            }
          }
          runner = runner.nextSibling;
        }
        // Move its contents out and reprocess it on the next iteration.
        if (node.firstChild) {
          next = node.firstChild;
          while (node.firstChild) {
            node.parentNode.insertBefore(node.firstChild, node);
          }
        }
        // get rid of the empty shell.
        goog.dom.removeNode(node);
      } else {
        if (node.firstChild) {
          next = node.firstChild;
        }
      }
    }
    node = next;
  }
};


/**
 * Processes word.
 *
 * @param {Node} node Node containing word.
 * @param {string} word Word to process.
 * @param {goog.spell.SpellCheck.WordStatus} status Status of the word.
 * @protected
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.processWord = function(node, word,
                                                              status) {
  node.parentNode.insertBefore(this.createWordElement_(word, status), node);
  this.elementsInserted_++;
};


/**
 * Processes recognized text and separators.
 *
 * @param {Node} node Node containing separator.
 * @param {string} text Text to process.
 * @protected
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.processRange = function(node, text) {
  // The text does not change, it only gets split, so if the lengths are the
  // same, the text is the same, so keep the existing node.
  if (node.nodeType == goog.dom.NodeType.TEXT && node.nodeValue.length ==
      text.length) {
    return;
  }

  node.parentNode.insertBefore(this.editorDom_.createTextNode(text), node);
  this.elementsInserted_++;
};


/**
 * @override
 * @suppress {accessControls}
 */
goog.ui.RichTextSpellChecker.prototype.createWordElement_ = function(word,
                                                                     status) {
  var parameters = this.getElementProperties(status);
  var el = /** @type {HTMLSpanElement} */ (this.editorDom_.createDom('span',
      parameters, word));
  this.registerWordElement_(word, el);
  return el;
};


/**
 * Updates or replaces element based on word status.
 * @see goog.ui.AbstractSpellChecker.prototype.updateElement_
 *
 * Overridden from AbstractSpellChecker because we need to be mindful of
 * deleting the currentNode_ - this can break our pending processing.
 *
 * @param {Element} el Word element.
 * @param {string} word Word to update status for.
 * @param {goog.spell.SpellCheck.WordStatus} status Status of word.
 * @protected
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.updateElement = function(el, word,
    status) {
  if (status == goog.spell.SpellCheck.WordStatus.VALID && el !=
      this.currentNode_ && el.nextSibling != this.currentNode_) {
    this.removeMarkup(el);
  } else {
    goog.dom.setProperties(el, this.getElementProperties(status));
  }
};


/**
 * Hides correction UI.
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.resume = function() {
  goog.ui.RichTextSpellChecker.superClass_.resume.call(this);

  this.restoreNode_(this.rootNode_);

  goog.events.unlisten(this.rootNode_, goog.events.EventType.CLICK,
                       this.onWordClick_, false, this);
};


/**
 * Processes nodes recursively, removes all spell checker markup, and
 * consolidates text nodes.
 *
 * @param {Node} node node on which to recurse.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.restoreNode_ = function(node) {
  while (node) {
    if (this.isExcluded_(node)) {
      node = node.nextSibling;
      continue;
    }
    // Contents of the child of the element is usually 1 text element, but the
    // user can actually add multiple nodes in it during editing. So we move
    // all the children out, prepend, and reprocess (pointer is set back to
    // the first node that's been moved out, and the loop repeats).
    if (node.nodeType == goog.dom.NodeType.ELEMENT && node.className ==
        this.wordClassName) {
      var firstElement = node.firstChild;
      var next;
      for (var child = firstElement; child; child = next) {
        next = child.nextSibling;
        node.parentNode.insertBefore(child, node);
      }
      next = firstElement || node.nextSibling;
      goog.dom.removeNode(node);
      node = next;
      continue;
    }
    // If this is a chain of text elements, we're trying to consolidate it.
    var textLeaf = this.isTextLeaf_(node);
    if (textLeaf) {
      var textNodes = 1;
      var next = node.nextSibling;
      while (this.isTextLeaf_(node.previousSibling)) {
        node = node.previousSibling;
        ++textNodes;
      }
      while (this.isTextLeaf_(next)) {
        next = next.nextSibling;
        ++textNodes;
      }
      if (textNodes > 1) {
        this.workBuffer_.append(node.nodeValue);
        while (this.isTextLeaf_(node.nextSibling)) {
          this.workBuffer_.append(node.nextSibling.nodeValue);
          goog.dom.removeNode(node.nextSibling);
        }
        node.nodeValue = this.workBuffer_.toString();
        this.workBuffer_.clear();
      }
    }
    // Process child nodes, if any.
    if (node.firstChild) {
      this.restoreNode_(node.firstChild);
    }
    node = node.nextSibling;
  }
};


/**
 * Returns desired element properties for the specified status.
 *
 * @param {goog.spell.SpellCheck.WordStatus} status Status of the word.
 * @return {Object} Properties to apply to word element.
 * @protected
 * @override
 */
goog.ui.RichTextSpellChecker.prototype.getElementProperties =
    function(status) {
  return {
    'class': this.wordClassName,
    'style': (status == goog.spell.SpellCheck.WordStatus.INVALID) ?
        this.invalidWordCssText : ''
  };
};


/**
 * Handler for click events.
 *
 * @param {goog.events.BrowserEvent} event Event object.
 * @private
 */
goog.ui.RichTextSpellChecker.prototype.onWordClick_ = function(event) {
  var target = /** @type {Element} */ (event.target);
  if (event.target.className == this.wordClassName &&
      this.handler_.checkWord(goog.dom.getTextContent(target)) ==
      goog.spell.SpellCheck.WordStatus.INVALID) {

    this.showSuggestionsMenu(target, event);

    // Prevent document click handler from closing the menu.
    event.stopPropagation();
  }
};


/** @override */
goog.ui.RichTextSpellChecker.prototype.disposeInternal = function() {
  goog.ui.RichTextSpellChecker.superClass_.disposeInternal.call(this);
  this.rootNode_ = null;
  this.editorDom_ = null;
};
