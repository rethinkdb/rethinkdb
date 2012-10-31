// Copyright 2011 The Closure Library Authors. All Rights Reserved.
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
 * @fileoverview Adds a keyboard shortcut for the link command.
 *
 */

goog.provide('goog.editor.plugins.LinkShortcutPlugin');

goog.require('goog.editor.Command');
goog.require('goog.editor.Link');
goog.require('goog.editor.Plugin');
goog.require('goog.string');



/**
 * Plugin to add a keyboard shortcut for the link command
 * @constructor
 * @extends {goog.editor.Plugin}
 */
goog.editor.plugins.LinkShortcutPlugin = function() {
  goog.base(this);
};
goog.inherits(goog.editor.plugins.LinkShortcutPlugin, goog.editor.Plugin);


/** @override */
goog.editor.plugins.LinkShortcutPlugin.prototype.getTrogClassId = function() {
  return 'LinkShortcutPlugin';
};


/**
 * @inheritDoc
 */
goog.editor.plugins.LinkShortcutPlugin.prototype.handleKeyboardShortcut =
    function(e, key, isModifierPressed) {
  var command;
  if (isModifierPressed && key == 'k') {
    var link = /** @type {goog.editor.Link?} */ (
        this.fieldObject.execCommand(goog.editor.Command.LINK));
    if (link) {
      link.finishLinkCreation(this.fieldObject);
    }
    return true;
  }

  return false;
};

