// Copyright 2005 The Closure Library Authors. All Rights Reserved.
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
 * @fileoverview Implementation of EventTarget as defined by W3C DOM 2/3.
 *
 * @author arv@google.com (Erik Arvidsson) [Original implementation]
 * @author pupius@google.com (Daniel Pupius) [Port to use goog.events]
 * @see ../demos/eventtarget.html
 */


/**
 * Namespace for events
 */
goog.provide('goog.events.EventTarget');

goog.require('goog.Disposable');
goog.require('goog.events');



/**
 * Inherit from this class to give your object the ability to dispatch events.
 * Note that this class provides event <em>sending</em> behaviour, not event
 * receiving behaviour: your object will be able to broadcast events, and other
 * objects will be able to listen for those events using goog.events.listen().
 *
 * <p>The name "EventTarget" reflects the fact that this class implements the
 * <a href="http://www.w3.org/TR/DOM-Level-2-Events/events.html">
 * EventTarget interface</a> as defined by W3C DOM 2/3, with a few differences:
 * <ul>
 * <li>Event objects do not have to implement the Event interface. An object
 *     is treated as an event object if it has a 'type' property.
 * <li>You can use a plain string instead of an event object; an event-like
 *     object will be created with the 'type' set to the string value.
 * </ul>
 *
 * <p>Unless propagation is stopped, an event dispatched by an EventTarget
 * will bubble to the parent returned by <code>getParentEventTarget</code>.
 * To set the parent, call <code>setParentEventTarget</code> or override
 * <code>getParentEventTarget</code> in a subclass.  Subclasses that don't
 * support changing the parent should override the setter to throw an error.
 *
 * <p>Example usage:
 * <pre>
 *   var source = new goog.events.EventTarget();
 *   function handleEvent(event) {
 *     alert('Type: ' + e.type + '\nTarget: ' + e.target);
 *   }
 *   goog.events.listen(source, 'foo', handleEvent);
 *   ...
 *   source.dispatchEvent({type: 'foo'}); // will call handleEvent
 *   // or source.dispatchEvent('foo');
 *   ...
 *   goog.events.unlisten(source, 'foo', handleEvent);
 *
 *   // You can also use the Listener interface:
 *   var listener = {
 *     handleEvent: function(event) {
 *       ...
 *     }
 *   };
 *   goog.events.listen(source, 'bar', listener);
 * </pre>
 *
 * @constructor
 * @extends {goog.Disposable}
 */
goog.events.EventTarget = function() {
  goog.Disposable.call(this);
};
goog.inherits(goog.events.EventTarget, goog.Disposable);


/**
 * Used to tell if an event is a real event in goog.events.listen() so we don't
 * get listen() calling addEventListener() and vice-versa.
 * @type {boolean}
 * @private
 */
goog.events.EventTarget.prototype.customEvent_ = true;


/**
 * Parent event target, used during event bubbling.
 * @type {goog.events.EventTarget?}
 * @private
 */
goog.events.EventTarget.prototype.parentEventTarget_ = null;


/**
 * Returns the parent of this event target to use for bubbling.
 *
 * @return {goog.events.EventTarget} The parent EventTarget or null if there
 * is no parent.
 */
goog.events.EventTarget.prototype.getParentEventTarget = function() {
  return this.parentEventTarget_;
};


/**
 * Sets the parent of this event target to use for bubbling.
 *
 * @param {goog.events.EventTarget?} parent Parent EventTarget (null if none).
 */
goog.events.EventTarget.prototype.setParentEventTarget = function(parent) {
  this.parentEventTarget_ = parent;
};


/**
 * Adds an event listener to the event target. The same handler can only be
 * added once per the type. Even if you add the same handler multiple times
 * using the same type then it will only be called once when the event is
 * dispatched.
 *
 * Supported for legacy but use goog.events.listen(src, type, handler) instead.
 *
 * @param {string} type The type of the event to listen for.
 * @param {Function|Object} handler The function to handle the event. The
 *     handler can also be an object that implements the handleEvent method
 *     which takes the event object as argument.
 * @param {boolean=} opt_capture In DOM-compliant browsers, this determines
 *     whether the listener is fired during the capture or bubble phase
 *     of the event.
 * @param {Object=} opt_handlerScope Object in whose scope to call the listener.
 */
goog.events.EventTarget.prototype.addEventListener = function(
    type, handler, opt_capture, opt_handlerScope) {
  goog.events.listen(this, type, handler, opt_capture, opt_handlerScope);
};


/**
 * Removes an event listener from the event target. The handler must be the
 * same object as the one added. If the handler has not been added then
 * nothing is done.
 * @param {string} type The type of the event to listen for.
 * @param {Function|Object} handler The function to handle the event. The
 *     handler can also be an object that implements the handleEvent method
 *     which takes the event object as argument.
 * @param {boolean=} opt_capture In DOM-compliant browsers, this determines
 *     whether the listener is fired during the capture or bubble phase
 *     of the event.
 * @param {Object=} opt_handlerScope Object in whose scope to call the listener.
 */
goog.events.EventTarget.prototype.removeEventListener = function(
    type, handler, opt_capture, opt_handlerScope) {
  goog.events.unlisten(this, type, handler, opt_capture, opt_handlerScope);
};


/**
 * Dispatches an event (or event like object) and calls all listeners
 * listening for events of this type. The type of the event is decided by the
 * type property on the event object.
 *
 * If any of the listeners returns false OR calls preventDefault then this
 * function will return false.  If one of the capture listeners calls
 * stopPropagation, then the bubble listeners won't fire.
 *
 * @param {string|Object|goog.events.Event} e Event object.
 * @return {boolean} If anyone called preventDefault on the event object (or
 *     if any of the handlers returns false this will also return false.
 */
goog.events.EventTarget.prototype.dispatchEvent = function(e) {
  return goog.events.dispatchEvent(this, e);
};


/**
 * Unattach listeners from this object.  Classes that extend EventTarget may
 * need to override this method in order to remove references to DOM Elements
 * and additional listeners, it should be something like this:
 * <pre>
 * MyClass.prototype.disposeInternal = function() {
 *   MyClass.superClass_.disposeInternal.call(this);
 *   // Dispose logic for MyClass
 * };
 * </pre>
 * @override
 * @protected
 */
goog.events.EventTarget.prototype.disposeInternal = function() {
  goog.events.EventTarget.superClass_.disposeInternal.call(this);
  goog.events.removeAll(this);
  this.parentEventTarget_ = null;
};
