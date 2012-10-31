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
 * @fileoverview Classes for doing animations and visual effects.
 *
 * (Based loosly on my animation code for 13thparallel.org, with extra
 * inspiration from the DojoToolkit's modifications to my code)
 */

goog.provide('goog.fx.Animation');
goog.provide('goog.fx.Animation.EventType');
goog.provide('goog.fx.Animation.State');
goog.provide('goog.fx.AnimationEvent');

goog.require('goog.array');
goog.require('goog.events.Event');
goog.require('goog.fx.Transition');  // Unreferenced: interface
goog.require('goog.fx.Transition.EventType');
goog.require('goog.fx.TransitionBase.State');
goog.require('goog.fx.anim');
goog.require('goog.fx.anim.Animated');  // Unreferenced: interface



/**
 * Constructor for an animation object.
 * @param {Array.<number>} start Array for start coordinates.
 * @param {Array.<number>} end Array for end coordinates.
 * @param {number} duration Length of animation in milliseconds.
 * @param {Function=} opt_acc Acceleration function, returns 0-1 for inputs 0-1.
 * @constructor
 * @implements {goog.fx.anim.Animated}
 * @implements {goog.fx.Transition}
 * @extends {goog.fx.TransitionBase}
 */
goog.fx.Animation = function(start, end, duration, opt_acc) {
  goog.base(this);

  if (!goog.isArray(start) || !goog.isArray(end)) {
    throw Error('Start and end parameters must be arrays');
  }

  if (start.length != end.length) {
    throw Error('Start and end points must be the same length');
  }

  /**
   * Start point.
   * @type {Array.<number>}
   * @protected
   */
  this.startPoint = start;

  /**
   * End point.
   * @type {Array.<number>}
   * @protected
   */
  this.endPoint = end;

  /**
   * Duration of animation in milliseconds.
   * @type {number}
   * @protected
   */
  this.duration = duration;

  /**
   * Acceleration function, which must return a number between 0 and 1 for
   * inputs between 0 and 1.
   * @type {Function|undefined}
   * @private
   */
  this.accel_ = opt_acc;

  /**
   * Current coordinate for animation.
   * @type {Array.<number>}
   * @protected
   */
  this.coords = [];

  /**
   * Whether the animation should use "right" rather than "left" to position
   * elements in RTL.  This is a temporary flag to allow clients to transition
   * to the new behavior at their convenience.  At some point it will be the
   * default.
   * @type {boolean}
   * @private
   */
  this.useRightPositioningForRtl_ = false;
};
goog.inherits(goog.fx.Animation, goog.fx.TransitionBase);


/**
 * Sets whether the animation should use "right" rather than "left" to position
 * elements.  This is a temporary flag to allow clients to transition
 * to the new component at their convenience.  At some point "right" will be
 * used for RTL elements by default.
 * @param {boolean} useRightPositioningForRtl True if "right" should be used for
 *     positioning, false if "left" should be used for positioning.
 */
goog.fx.Animation.prototype.enableRightPositioningForRtl =
    function(useRightPositioningForRtl) {
  this.useRightPositioningForRtl_ = useRightPositioningForRtl;
};


/**
 * Whether the animation should use "right" rather than "left" to position
 * elements.  This is a temporary flag to allow clients to transition
 * to the new component at their convenience.  At some point "right" will be
 * used for RTL elements by default.
 * @return {boolean} True if "right" should be used for positioning, false if
 *     "left" should be used for positioning.
 */
goog.fx.Animation.prototype.isRightPositioningForRtlEnabled = function() {
  return this.useRightPositioningForRtl_;
};


/**
 * Events fired by the animation.
 * @enum {string}
 */
goog.fx.Animation.EventType = {
  /**
   * Dispatched when played for the first time OR when it is resumed.
   * @deprecated Use goog.fx.Transition.EventType.PLAY.
   */
  PLAY: goog.fx.Transition.EventType.PLAY,

  /**
   * Dispatched only when the animation starts from the beginning.
   * @deprecated Use goog.fx.Transition.EventType.BEGIN.
   */
  BEGIN: goog.fx.Transition.EventType.BEGIN,

  /**
   * Dispatched only when animation is restarted after a pause.
   * @deprecated Use goog.fx.Transition.EventType.RESUME.
   */
  RESUME: goog.fx.Transition.EventType.RESUME,

  /**
   * Dispatched when animation comes to the end of its duration OR stop
   * is called.
   * @deprecated Use goog.fx.Transition.EventType.END.
   */
  END: goog.fx.Transition.EventType.END,

  /**
   * Dispatched only when stop is called.
   * @deprecated Use goog.fx.Transition.EventType.STOP.
   */
  STOP: goog.fx.Transition.EventType.STOP,

  /**
   * Dispatched only when animation comes to its end naturally.
   * @deprecated Use goog.fx.Transition.EventType.FINISH.
   */
  FINISH: goog.fx.Transition.EventType.FINISH,

  /**
   * Dispatched when an animation is paused.
   * @deprecated Use goog.fx.Transition.EventType.PAUSE.
   */
  PAUSE: goog.fx.Transition.EventType.PAUSE,

  /**
   * Dispatched each frame of the animation.  This is where the actual animator
   * will listen.
   */
  ANIMATE: 'animate',

  /**
   * Dispatched when the animation is destroyed.
   */
  DESTROY: 'destroy'
};


/**
 * @deprecated Use goog.fx.anim.TIMEOUT.
 */
goog.fx.Animation.TIMEOUT = goog.fx.anim.TIMEOUT;


/**
 * Enum for the possible states of an animation.
 * @deprecated Use goog.fx.Transition.State instead.
 * @enum {number}
 */
goog.fx.Animation.State = goog.fx.TransitionBase.State;


/**
 * @deprecated Use goog.fx.anim.setAnimationWindow.
 * @param {Window} animationWindow The window in which to animate elements.
 */
goog.fx.Animation.setAnimationWindow = function(animationWindow) {
  goog.fx.anim.setAnimationWindow(animationWindow);
};


/**
 * Current frame rate.
 * @type {number}
 * @private
 */
goog.fx.Animation.prototype.fps_ = 0;


/**
 * Percent of the way through the animation.
 * @type {number}
 * @protected
 */
goog.fx.Animation.prototype.progress = 0;


/**
 * Timestamp for when last frame was run.
 * @type {?number}
 * @protected
 */
goog.fx.Animation.prototype.lastFrame = null;


/**
 * Starts or resumes an animation.
 * @param {boolean=} opt_restart Whether to restart the
 *     animation from the beginning if it has been paused.
 * @return {boolean} Whether animation was started.
 * @override
 */
goog.fx.Animation.prototype.play = function(opt_restart) {
  if (opt_restart || this.isStopped()) {
    this.progress = 0;
    this.coords = this.startPoint;
  } else if (this.isPlaying()) {
    return false;
  }

  goog.fx.anim.unregisterAnimation(this);

  var now = /** @type {number} */ (goog.now());

  this.startTime = now;
  if (this.isPaused()) {
    this.startTime -= this.duration * this.progress;
  }

  this.endTime = this.startTime + this.duration;
  this.lastFrame = this.startTime;

  if (!this.progress) {
    this.onBegin();
  }

  this.onPlay();

  if (this.isPaused()) {
    this.onResume();
  }

  this.setStatePlaying();

  goog.fx.anim.registerAnimation(this);
  this.cycle(now);

  return true;
};


/**
 * Stops the animation.
 * @param {boolean=} opt_gotoEnd If true the animation will move to the
 *     end coords.
 * @override
 */
goog.fx.Animation.prototype.stop = function(opt_gotoEnd) {
  goog.fx.anim.unregisterAnimation(this);
  this.setStateStopped();

  if (!!opt_gotoEnd) {
    this.progress = 1;
  }

  this.updateCoords_(this.progress);

  this.onStop();
  this.onEnd();
};


/**
 * Pauses the animation (iff it's playing).
 * @override
 */
goog.fx.Animation.prototype.pause = function() {
  if (this.isPlaying()) {
    goog.fx.anim.unregisterAnimation(this);
    this.setStatePaused();
    this.onPause();
  }
};


/**
 * @return {number} The current progress of the animation, the number
 *     is between 0 and 1 inclusive.
 */
goog.fx.Animation.prototype.getProgress = function() {
  return this.progress;
};


/**
 * Sets the progress of the animation.
 * @param {number} progress The new progress of the animation.
 */
goog.fx.Animation.prototype.setProgress = function(progress) {
  this.progress = progress;
  if (this.isPlaying()) {
    var now = goog.now();
    // If the animation is already playing, we recompute startTime and endTime
    // such that the animation plays consistently, that is:
    // now = startTime + progress * duration.
    this.startTime = now - this.duration * this.progress;
    this.endTime = this.startTime + this.duration;
  }
};


/**
 * Disposes of the animation.  Stops an animation, fires a 'destroy' event and
 * then removes all the event handlers to clean up memory.
 * @override
 * @protected
 */
goog.fx.Animation.prototype.disposeInternal = function() {
  if (!this.isStopped()) {
    this.stop(false);
  }
  this.onDestroy();
  goog.base(this, 'disposeInternal');
};


/**
 * Stops an animation, fires a 'destroy' event and then removes all the event
 * handlers to clean up memory.
 * @deprecated Use dispose() instead.
 */
goog.fx.Animation.prototype.destroy = function() {
  this.dispose();
};


/** @inheritDoc */
goog.fx.Animation.prototype.onAnimationFrame = function(now) {
  this.cycle(now);
};


/**
 * Handles the actual iteration of the animation in a timeout
 * @param {number} now The current time.
 */
goog.fx.Animation.prototype.cycle = function(now) {
  this.progress = (now - this.startTime) / (this.endTime - this.startTime);

  if (this.progress >= 1) {
    this.progress = 1;
  }

  this.fps_ = 1000 / (now - this.lastFrame);
  this.lastFrame = now;

  this.updateCoords_(this.progress);

  // Animation has finished.
  if (this.progress == 1) {
    this.setStateStopped();
    goog.fx.anim.unregisterAnimation(this);

    this.onFinish();
    this.onEnd();

  // Animation is still under way.
  } else if (this.isPlaying()) {
    this.onAnimate();
  }
};


/**
 * Calculates current coordinates, based on the current state.  Applies
 * the accelleration function if it exists.
 * @param {number} t Percentage of the way through the animation as a decimal.
 * @private
 */
goog.fx.Animation.prototype.updateCoords_ = function(t) {
  if (goog.isFunction(this.accel_)) {
    t = this.accel_(t);
  }
  this.coords = new Array(this.startPoint.length);
  for (var i = 0; i < this.startPoint.length; i++) {
    this.coords[i] = (this.endPoint[i] - this.startPoint[i]) * t +
        this.startPoint[i];
  }
};


/**
 * Dispatches the ANIMATE event. Sub classes should override this instead
 * of listening to the event.
 * @protected
 */
goog.fx.Animation.prototype.onAnimate = function() {
  this.dispatchAnimationEvent(goog.fx.Animation.EventType.ANIMATE);
};


/**
 * Dispatches the DESTROY event. Sub classes should override this instead
 * of listening to the event.
 * @protected
 */
goog.fx.Animation.prototype.onDestroy = function() {
  this.dispatchAnimationEvent(goog.fx.Animation.EventType.DESTROY);
};


/** @override */
goog.fx.Animation.prototype.dispatchAnimationEvent = function(type) {
  this.dispatchEvent(new goog.fx.AnimationEvent(type, this));
};



/**
 * Class for an animation event object.
 * @param {string} type Event type.
 * @param {goog.fx.Animation} anim An animation object.
 * @constructor
 * @extends {goog.events.Event}
 */
goog.fx.AnimationEvent = function(type, anim) {
  goog.base(this, type);

  /**
   * The current coordinates.
   * @type {Array.<number>}
   */
  this.coords = anim.coords;

  /**
   * The x coordinate.
   * @type {number}
   */
  this.x = anim.coords[0];

  /**
   * The y coordinate.
   * @type {number}
   */
  this.y = anim.coords[1];

  /**
   * The z coordinate.
   * @type {number}
   */
  this.z = anim.coords[2];

  /**
   * The current duration.
   * @type {number}
   */
  this.duration = anim.duration;

  /**
   * The current progress.
   * @type {number}
   */
  this.progress = anim.getProgress();

  /**
   * Frames per second so far.
   */
  this.fps = anim.fps_;

  /**
   * The state of the animation.
   * @type {number}
   */
  this.state = anim.getStateInternal();

  /**
   * The animation object.
   * @type {goog.fx.Animation}
   */
  // TODO(arv): This can be removed as this is the same as the target
  this.anim = anim;
};
goog.inherits(goog.fx.AnimationEvent, goog.events.Event);


/**
 * Returns the coordinates as integers (rounded to nearest integer).
 * @return {Array.<number>} An array of the coordinates rounded to
 *     the nearest integer.
 */
goog.fx.AnimationEvent.prototype.coordsAsInts = function() {
  return goog.array.map(this.coords, Math.round);
};
