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
 * @fileoverview A class which automatically plays through a queue of
 * animations.  AnimationParallelQueue and AnimationSerialQueue provide
 * specific implementations of the abstract class AnimationQueue.
 *
 * @see ../demos/animationqueue.html
 */

goog.provide('goog.fx.AnimationParallelQueue');
goog.provide('goog.fx.AnimationQueue');
goog.provide('goog.fx.AnimationSerialQueue');

goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.events.EventHandler');
goog.require('goog.fx.Transition.EventType');
goog.require('goog.fx.TransitionBase');
goog.require('goog.fx.TransitionBase.State');



/**
 * Constructor for AnimationQueue object.
 *
 * @constructor
 * @extends {goog.fx.TransitionBase}
 */
goog.fx.AnimationQueue = function() {
  goog.base(this);

  /**
   * An array holding all animations in the queue.
   * @type {Array.<goog.fx.TransitionBase>}
   * @protected
   */
  this.queue = [];
};
goog.inherits(goog.fx.AnimationQueue, goog.fx.TransitionBase);


/**
 * Pushes an Animation to the end of the queue.
 * @param {goog.fx.TransitionBase} animation The animation to add to the queue.
 */
goog.fx.AnimationQueue.prototype.add = function(animation) {
  goog.asserts.assert(this.isStopped(),
      'Not allowed to add animations to a running animation queue.');

  if (goog.array.contains(this.queue, animation)) {
    return;
  }

  this.queue.push(animation);
  goog.events.listen(animation, goog.fx.Transition.EventType.FINISH,
                     this.onAnimationFinish, false, this);
};


/**
 * Removes an Animation from the queue.
 * @param {goog.fx.Animation} animation The animation to remove.
 */
goog.fx.AnimationQueue.prototype.remove = function(animation) {
  goog.asserts.assert(this.isStopped(),
      'Not allowed to remove animations from a running animation queue.');

  if (goog.array.remove(this.queue, animation)) {
    goog.events.unlisten(animation, goog.fx.Transition.EventType.FINISH,
                         this.onAnimationFinish, false, this);
  }
};


/**
 * Handles the event that an animation has finished.
 * @param {goog.events.Event} e The finishing event.
 * @protected
 */
goog.fx.AnimationQueue.prototype.onAnimationFinish = goog.abstractMethod;


/**
 * Disposes of the animations.
 * @override
 */
goog.fx.AnimationQueue.prototype.disposeInternal = function() {
  goog.array.forEach(this.queue, function(animation) {
    animation.dispose();
  });
  this.queue.length = 0;

  goog.base(this, 'disposeInternal');
};



/**
 * Constructor for AnimationParallelQueue object.
 * @constructor
 * @extends {goog.fx.AnimationQueue}
 */
goog.fx.AnimationParallelQueue = function() {
  goog.base(this);

  /**
   * Number of finished animations.
   * @type {number}
   * @private
   */
  this.finishedCounter_ = 0;
};
goog.inherits(goog.fx.AnimationParallelQueue, goog.fx.AnimationQueue);


/** @inheritDoc */
goog.fx.AnimationParallelQueue.prototype.play = function(opt_restart) {
  if (this.queue.length == 0) {
    return false;
  }

  if (opt_restart || this.isStopped()) {
    this.finishedCounter_ = 0;
    this.onBegin();
  } else if (this.isPlaying()) {
    return false;
  }

  this.onPlay();
  if (this.isPaused()) {
    this.onResume();
  }
  var resuming = this.isPaused() && !opt_restart;

  this.startTime = goog.now();
  this.endTime = null;
  this.setStatePlaying();

  goog.array.forEach(this.queue, function(anim) {
    if (!resuming || anim.isPaused()) {
      anim.play(opt_restart);
    }
  });

  return true;
};


/** @inheritDoc */
goog.fx.AnimationParallelQueue.prototype.pause = function() {
  if (this.isPlaying()) {
    goog.array.forEach(this.queue, function(anim) {
      if (anim.isPlaying()) {
        anim.pause();
      }
    });

    this.setStatePaused();
    this.onPause();
  }
};


/** @inheritDoc */
goog.fx.AnimationParallelQueue.prototype.stop = function(opt_gotoEnd) {
  goog.array.forEach(this.queue, function(anim) {
    if (!anim.isStopped()) {
      anim.stop(opt_gotoEnd);
    }
  });

  this.setStateStopped();
  this.endTime = goog.now();

  this.onStop();
  this.onEnd();
};


/** @inheritDoc */
goog.fx.AnimationParallelQueue.prototype.onAnimationFinish = function(e) {
  this.finishedCounter_++;
  if (this.finishedCounter_ == this.queue.length) {
    this.endTime = goog.now();

    this.setStateStopped();

    this.onFinish();
    this.onEnd();
  }
};



/**
 * Constructor for AnimationSerialQueue object.
 * @constructor
 * @extends {goog.fx.AnimationQueue}
 */
goog.fx.AnimationSerialQueue = function() {
  goog.base(this);

  /**
   * Current animation in queue currently active.
   * @type {number}
   * @private
   */
  this.current_ = 0;
};
goog.inherits(goog.fx.AnimationSerialQueue, goog.fx.AnimationQueue);


/** @inheritDoc */
goog.fx.AnimationSerialQueue.prototype.play = function(opt_restart) {
  if (this.queue.length == 0) {
    return false;
  }

  if (opt_restart || this.isStopped()) {
    if (this.current_ < this.queue.length &&
        !this.queue[this.current_].isStopped()) {
      this.queue[this.current_].stop(false);
    }

    this.current_ = 0;
    this.onBegin();
  } else if (this.isPlaying()) {
    return false;
  }

  this.onPlay();
  if (this.isPaused()) {
    this.onResume();
  }

  this.startTime = goog.now();
  this.endTime = null;
  this.setStatePlaying();

  this.queue[this.current_].play(opt_restart);

  return true;
};


/** @inheritDoc */
goog.fx.AnimationSerialQueue.prototype.pause = function() {
  if (this.isPlaying()) {
    this.queue[this.current_].pause();
    this.setStatePaused();
    this.onPause();
  }
};


/** @inheritDoc */
goog.fx.AnimationSerialQueue.prototype.stop = function(opt_gotoEnd) {
  this.setStateStopped();
  this.endTime = goog.now();

  if (opt_gotoEnd) {
    for (var i = this.current_; i < this.queue.length; ++i) {
      var anim = this.queue[i];
      // If the animation is stopped, start it to initiate rendering.  This
      // might be needed to make the next line work.
      if (anim.isStopped()) anim.play();
      // If the animation is not done, stop it and go to the end state of the
      // animation.
      if (!anim.isStopped()) anim.stop(true);
    }
  } else if (this.current_ < this.queue.length) {
    this.queue[this.current_].stop(false);
  }

  this.onStop();
  this.onEnd();
};


/** @inheritDoc */
goog.fx.AnimationSerialQueue.prototype.onAnimationFinish = function(e) {
  if (this.isPlaying()) {
    this.current_++;
    if (this.current_ < this.queue.length) {
      this.queue[this.current_].play();
    } else {
      this.endTime = goog.now();
      this.setStateStopped();

      this.onFinish();
      this.onEnd();
    }
  }
};
