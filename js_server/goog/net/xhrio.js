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
 * @fileoverview Wrapper class for handling XmlHttpRequests.
 *
 * One off requests can be sent through goog.net.XhrIo.send() or an
 * instance can be created to send multiple requests.  Each request uses its
 * own XmlHttpRequest object and handles clearing of the event callback to
 * ensure no leaks.
 *
 * XhrIo is event based, it dispatches events when a request finishes, fails or
 * succeeds or when the ready-state changes. The ready-state or timeout event
 * fires first, followed by a generic completed event. Then the abort, error,
 * or success event is fired as appropriate. Lastly, the ready event will fire
 * to indicate that the object may be used to make another request.
 *
 * The error event may also be called before completed and
 * ready-state-change if the XmlHttpRequest.open() or .send() methods throw.
 *
 * This class does not support multiple requests, queuing, or prioritization.
 *
 * Tested = IE6, FF1.5, Safari, Opera 8.5
 *
 * TODO(user): Error cases aren't playing nicely in Safari.
 *
 */


goog.provide('goog.net.XhrIo');
goog.provide('goog.net.XhrIo.ResponseType');

goog.require('goog.Timer');
goog.require('goog.debug.Logger');
goog.require('goog.debug.entryPointRegistry');
goog.require('goog.debug.errorHandlerWeakDep');
goog.require('goog.events.EventTarget');
goog.require('goog.json');
goog.require('goog.net.ErrorCode');
goog.require('goog.net.EventType');
goog.require('goog.net.HttpStatus');
goog.require('goog.net.XmlHttp');
goog.require('goog.object');
goog.require('goog.structs');
goog.require('goog.structs.Map');
goog.require('goog.uri.utils');



/**
 * Basic class for handling XMLHttpRequests.
 * @param {goog.net.XmlHttpFactory=} opt_xmlHttpFactory Factory to use when
 *     creating XMLHttpRequest objects.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
goog.net.XhrIo = function(opt_xmlHttpFactory) {
  goog.events.EventTarget.call(this);

  /**
   * Map of default headers to add to every request, use:
   * XhrIo.headers.set(name, value)
   * @type {goog.structs.Map}
   */
  this.headers = new goog.structs.Map();

  /**
   * Optional XmlHttpFactory
   * @type {goog.net.XmlHttpFactory}
   * @private
   */
  this.xmlHttpFactory_ = opt_xmlHttpFactory || null;
};
goog.inherits(goog.net.XhrIo, goog.events.EventTarget);


/**
 * Response types that may be requested for XMLHttpRequests.
 * @enum {string}
 * @see http://www.w3.org/TR/XMLHttpRequest/#the-responsetype-attribute
 */
goog.net.XhrIo.ResponseType = {
  DEFAULT: '',
  TEXT: 'text',
  DOCUMENT: 'document',
  // Not supported as of Chrome 10.0.612.1 dev
  BLOB: 'blob',
  ARRAY_BUFFER: 'arraybuffer'
};


/**
 * A reference to the XhrIo logger
 * @type {goog.debug.Logger}
 * @private
 */
goog.net.XhrIo.prototype.logger_ =
    goog.debug.Logger.getLogger('goog.net.XhrIo');


/**
 * The Content-Type HTTP header name
 * @type {string}
 */
goog.net.XhrIo.CONTENT_TYPE_HEADER = 'Content-Type';


/**
 * The pattern matching the 'http' and 'https' URI schemes
 * @type {!RegExp}
 */
goog.net.XhrIo.HTTP_SCHEME_PATTERN = /^https?$/i;


/**
 * The Content-Type HTTP header value for a url-encoded form
 * @type {string}
 */
goog.net.XhrIo.FORM_CONTENT_TYPE =
    'application/x-www-form-urlencoded;charset=utf-8';


/**
 * All non-disposed instances of goog.net.XhrIo created
 * by {@link goog.net.XhrIo.send} are in this Array.
 * @see goog.net.XhrIo.cleanup
 * @type {Array.<goog.net.XhrIo>}
 * @private
 */
goog.net.XhrIo.sendInstances_ = [];


/**
 * Static send that creates a short lived instance of XhrIo to send the
 * request.
 * @see goog.net.XhrIo.cleanup
 * @param {string|goog.Uri} url Uri to make request to.
 * @param {Function=} opt_callback Callback function for when request is
 *     complete.
 * @param {string=} opt_method Send method, default: GET.
 * @param {string|GearsBlob=} opt_content Post data. This can be a Gears blob
 *     if the underlying HTTP request object is a Gears HTTP request.
 * @param {Object|goog.structs.Map=} opt_headers Map of headers to add to the
 *     request.
 * @param {number=} opt_timeoutInterval Number of milliseconds after which an
 *     incomplete request will be aborted; 0 means no timeout is set.
 */
goog.net.XhrIo.send = function(url, opt_callback, opt_method, opt_content,
                               opt_headers, opt_timeoutInterval) {
  var x = new goog.net.XhrIo();
  goog.net.XhrIo.sendInstances_.push(x);
  if (opt_callback) {
    goog.events.listen(x, goog.net.EventType.COMPLETE, opt_callback);
  }
  goog.events.listen(x,
                     goog.net.EventType.READY,
                     goog.partial(goog.net.XhrIo.cleanupSend_, x));
  if (opt_timeoutInterval) {
    x.setTimeoutInterval(opt_timeoutInterval);
  }
  x.send(url, opt_method, opt_content, opt_headers);
};


/**
 * Disposes all non-disposed instances of goog.net.XhrIo created by
 * {@link goog.net.XhrIo.send}.
 * {@link goog.net.XhrIo.send} cleans up the goog.net.XhrIo instance
 * it creates when the request completes or fails.  However, if
 * the request never completes, then the goog.net.XhrIo is not disposed.
 * This can occur if the window is unloaded before the request completes.
 * We could have {@link goog.net.XhrIo.send} return the goog.net.XhrIo
 * it creates and make the client of {@link goog.net.XhrIo.send} be
 * responsible for disposing it in this case.  However, this makes things
 * significantly more complicated for the client, and the whole point
 * of {@link goog.net.XhrIo.send} is that it's simple and easy to use.
 * Clients of {@link goog.net.XhrIo.send} should call
 * {@link goog.net.XhrIo.cleanup} when doing final
 * cleanup on window unload.
 */
goog.net.XhrIo.cleanup = function() {
  var instances = goog.net.XhrIo.sendInstances_;
  while (instances.length) {
    instances.pop().dispose();
  }
};


/**
 * Installs exception protection for all entry point introduced by
 * goog.net.XhrIo instances which are not protected by
 * {@link goog.debug.ErrorHandler#protectWindowSetTimeout},
 * {@link goog.debug.ErrorHandler#protectWindowSetInterval}, or
 * {@link goog.events.protectBrowserEventEntryPoint}.
 *
 * @param {goog.debug.ErrorHandler} errorHandler Error handler with which to
 *     protect the entry point(s).
 */
goog.net.XhrIo.protectEntryPoints = function(errorHandler) {
  goog.net.XhrIo.prototype.onReadyStateChangeEntryPoint_ =
      errorHandler.protectEntryPoint(
          goog.net.XhrIo.prototype.onReadyStateChangeEntryPoint_);
};


/**
 * Disposes of the specified goog.net.XhrIo created by
 * {@link goog.net.XhrIo.send} and removes it from
 * {@link goog.net.XhrIo.pendingStaticSendInstances_}.
 * @param {goog.net.XhrIo} XhrIo An XhrIo created by
 *     {@link goog.net.XhrIo.send}.
 * @private
 */
goog.net.XhrIo.cleanupSend_ = function(XhrIo) {
  XhrIo.dispose();
  goog.array.remove(goog.net.XhrIo.sendInstances_, XhrIo);
};


/**
 * Whether XMLHttpRequest is active.  A request is active from the time send()
 * is called until onReadyStateChange() is complete, or error() or abort()
 * is called.
 * @type {boolean}
 * @private
 */
goog.net.XhrIo.prototype.active_ = false;


/**
 * Reference to an XMLHttpRequest object that is being used for the transfer.
 * @type {XMLHttpRequest|GearsHttpRequest}
 * @private
 */
goog.net.XhrIo.prototype.xhr_ = null;


/**
 * The options to use with the current XMLHttpRequest object.
 * @type {Object}
 * @private
 */
goog.net.XhrIo.prototype.xhrOptions_ = null;


/**
 * Last URL that was requested.
 * @type {string|goog.Uri}
 * @private
 */
goog.net.XhrIo.prototype.lastUri_ = '';


/**
 * Method for the last request.
 * @type {string}
 * @private
 */
goog.net.XhrIo.prototype.lastMethod_ = '';


/**
 * Last error code.
 * @type {goog.net.ErrorCode}
 * @private
 */
goog.net.XhrIo.prototype.lastErrorCode_ = goog.net.ErrorCode.NO_ERROR;


/**
 * Last error message.
 * @type {Error|string}
 * @private
 */
goog.net.XhrIo.prototype.lastError_ = '';


/**
 * This is used to ensure that we don't dispatch an multiple ERROR events. This
 * can happen in IE when it does a synchronous load and one error is handled in
 * the ready statte change and one is handled due to send() throwing an
 * exception.
 * @type {boolean}
 * @private
 */
goog.net.XhrIo.prototype.errorDispatched_ = false;


/**
 * Used to make sure we don't fire the complete event from inside a send call.
 * @type {boolean}
 * @private
 */
goog.net.XhrIo.prototype.inSend_ = false;


/**
 * Used in determining if a call to {@link #onReadyStateChange_} is from within
 * a call to this.xhr_.open.
 * @type {boolean}
 * @private
 */
goog.net.XhrIo.prototype.inOpen_ = false;


/**
 * Used in determining if a call to {@link #onReadyStateChange_} is from within
 * a call to this.xhr_.abort.
 * @type {boolean}
 * @private
 */
goog.net.XhrIo.prototype.inAbort_ = false;


/**
 * Number of milliseconds after which an incomplete request will be aborted and
 * a {@link goog.net.EventType.TIMEOUT} event raised; 0 means no timeout is set.
 * @type {number}
 * @private
 */
goog.net.XhrIo.prototype.timeoutInterval_ = 0;


/**
 * Window timeout ID used to cancel the timeout event handler if the request
 * completes successfully.
 * @type {Object}
 * @private
 */
goog.net.XhrIo.prototype.timeoutId_ = null;


/**
 * The requested type for the response. The empty string means use the default
 * XHR behavior.
 * @type {goog.net.XhrIo.ResponseType}
 * @private
 */
goog.net.XhrIo.prototype.responseType_ = goog.net.XhrIo.ResponseType.DEFAULT;


/**
 * Whether a "credentialed" request is to be sent (one that is aware of cookies
 * and authentication) . This is applicable only for cross-domain requests and
 * more recent browsers that support this part of the HTTP Access Control
 * standard.
 *
 * @see http://www.w3.org/TR/XMLHttpRequest/#the-withcredentials-attribute
 *
 * @type {boolean}
 * @private
 */
goog.net.XhrIo.prototype.withCredentials_ = false;


/**
 * Returns the number of milliseconds after which an incomplete request will be
 * aborted, or 0 if no timeout is set.
 * @return {number} Timeout interval in milliseconds.
 */
goog.net.XhrIo.prototype.getTimeoutInterval = function() {
  return this.timeoutInterval_;
};


/**
 * Sets the number of milliseconds after which an incomplete request will be
 * aborted and a {@link goog.net.EventType.TIMEOUT} event raised; 0 means no
 * timeout is set.
 * @param {number} ms Timeout interval in milliseconds; 0 means none.
 */
goog.net.XhrIo.prototype.setTimeoutInterval = function(ms) {
  this.timeoutInterval_ = Math.max(0, ms);
};


/**
 * Sets the desired type for the response. At time of writing, this is only
 * supported in very recent versions of WebKit (10.0.612.1 dev and later).
 *
 * If this is used, the response may only be accessed via {@link #getResponse}.
 *
 * @param {goog.net.XhrIo.ResponseType} type The desired type for the response.
 */
goog.net.XhrIo.prototype.setResponseType = function(type) {
  this.responseType_ = type;
};


/**
 * Gets the desired type for the response.
 * @return {goog.net.XhrIo.ResponseType} The desired type for the response.
 */
goog.net.XhrIo.prototype.getResponseType = function() {
  return this.responseType_;
};


/**
 * Sets whether a "credentialed" request that is aware of cookie and
 * authentication information should be made. This option is only supported by
 * browsers that support HTTP Access Control. As of this writing, this option
 * is not supported in IE.
 *
 * @param {boolean} withCredentials Whether this should be a "credentialed"
 *     request.
 */
goog.net.XhrIo.prototype.setWithCredentials = function(withCredentials) {
  this.withCredentials_ = withCredentials;
};


/**
 * Gets whether a "credentialed" request is to be sent.
 * @return {boolean} The desired type for the response.
 */
goog.net.XhrIo.prototype.getWithCredentials = function() {
  return this.withCredentials_;
};


/**
 * Instance send that actually uses XMLHttpRequest to make a server call.
 * @param {string|goog.Uri} url Uri to make request to.
 * @param {string=} opt_method Send method, default: GET.
 * @param {string|GearsBlob=} opt_content Post data. This can be a Gears blob
 *     if the underlying HTTP request object is a Gears HTTP request.
 * @param {Object|goog.structs.Map=} opt_headers Map of headers to add to the
 *     request.
 */
goog.net.XhrIo.prototype.send = function(url, opt_method, opt_content,
                                         opt_headers) {
  if (this.xhr_) {
    throw Error('[goog.net.XhrIo] Object is active with another request');
  }

  var method = opt_method ? opt_method.toUpperCase() : 'GET';

  this.lastUri_ = url;
  this.lastError_ = '';
  this.lastErrorCode_ = goog.net.ErrorCode.NO_ERROR;
  this.lastMethod_ = method;
  this.errorDispatched_ = false;
  this.active_ = true;

  // Use the factory to create the XHR object and options
  this.xhr_ = this.createXhr();
  this.xhrOptions_ = this.xmlHttpFactory_ ?
      this.xmlHttpFactory_.getOptions() : goog.net.XmlHttp.getOptions();

  // Set up the onreadystatechange callback
  this.xhr_.onreadystatechange = goog.bind(this.onReadyStateChange_, this);

  /**
   * Try to open the XMLHttpRequest (always async), if an error occurs here it
   * is generally permission denied
   * @preserveTry
   */
  try {
    this.logger_.fine(this.formatMsg_('Opening Xhr'));
    this.inOpen_ = true;
    this.xhr_.open(method, url, true);  // Always async!
    this.inOpen_ = false;
  } catch (err) {
    this.logger_.fine(this.formatMsg_('Error opening Xhr: ' + err.message));
    this.error_(goog.net.ErrorCode.EXCEPTION, err);
    return;
  }

  // We can't use null since this won't allow POSTs to have a content length
  // specified which will cause some proxies to return a 411 error.
  var content = opt_content || '';

  var headers = this.headers.clone();

  // Add headers specific to this request
  if (opt_headers) {
    goog.structs.forEach(opt_headers, function(value, key) {
      headers.set(key, value);
    });
  }

  if (method == 'POST' &&
      !headers.containsKey(goog.net.XhrIo.CONTENT_TYPE_HEADER)) {
    // For POST requests, default to the url-encoded form content type.
    headers.set(goog.net.XhrIo.CONTENT_TYPE_HEADER,
                goog.net.XhrIo.FORM_CONTENT_TYPE);
  }

  // Add the headers to the Xhr object
  goog.structs.forEach(headers, function(value, key) {
    this.xhr_.setRequestHeader(key, value);
  }, this);

  if (this.responseType_) {
    this.xhr_.responseType = this.responseType_;
  }

  if (goog.object.containsKey(this.xhr_, 'withCredentials')) {
    this.xhr_.withCredentials = this.withCredentials_;
  }

  /**
   * Try to send the request, or other wise report an error (404 not found).
   * @preserveTry
   */
  try {
    if (this.timeoutId_) {
      // This should never happen, since the if (this.active_) above shouldn't
      // let execution reach this point if there is a request in progress...
      goog.Timer.defaultTimerObject.clearTimeout(this.timeoutId_);
      this.timeoutId_ = null;
    }
    if (this.timeoutInterval_ > 0) {
      this.logger_.fine(this.formatMsg_('Will abort after ' +
          this.timeoutInterval_ + 'ms if incomplete'));
      this.timeoutId_ = goog.Timer.defaultTimerObject.setTimeout(
          goog.bind(this.timeout_, this), this.timeoutInterval_);
    }
    this.logger_.fine(this.formatMsg_('Sending request'));
    this.inSend_ = true;
    this.xhr_.send(content);
    this.inSend_ = false;

  } catch (err) {
    this.logger_.fine(this.formatMsg_('Send error: ' + err.message));
    this.error_(goog.net.ErrorCode.EXCEPTION, err);
  }
};


/**
 * Creates a new XHR object.
 * @return {XMLHttpRequest|GearsHttpRequest} The newly created XHR object.
 * @protected
 */
goog.net.XhrIo.prototype.createXhr = function() {
  return this.xmlHttpFactory_ ?
      this.xmlHttpFactory_.createInstance() : goog.net.XmlHttp();
};


/**
 * The request didn't complete after {@link goog.net.XhrIo#timeoutInterval_}
 * milliseconds; raises a {@link goog.net.EventType.TIMEOUT} event and aborts
 * the request.
 * @private
 */
goog.net.XhrIo.prototype.timeout_ = function() {
  if (typeof goog == 'undefined') {
    // If goog is undefined then the callback has occurred as the application
    // is unloading and will error.  Thus we let it silently fail.
  } else if (this.xhr_) {
    this.lastError_ = 'Timed out after ' + this.timeoutInterval_ +
                      'ms, aborting';
    this.lastErrorCode_ = goog.net.ErrorCode.TIMEOUT;
    this.logger_.fine(this.formatMsg_(this.lastError_));
    this.dispatchEvent(goog.net.EventType.TIMEOUT);
    this.abort(goog.net.ErrorCode.TIMEOUT);
  }
};


/**
 * Something errorred, so inactivate, fire error callback and clean up
 * @param {goog.net.ErrorCode} errorCode The error code.
 * @param {Error} err The error object.
 * @private
 */
goog.net.XhrIo.prototype.error_ = function(errorCode, err) {
  this.active_ = false;
  if (this.xhr_) {
    this.inAbort_ = true;
    this.xhr_.abort();  // Ensures XHR isn't hung (FF)
    this.inAbort_ = false;
  }
  this.lastError_ = err;
  this.lastErrorCode_ = errorCode;
  this.dispatchErrors_();
  this.cleanUpXhr_();
};


/**
 * Dispatches COMPLETE and ERROR in case of an error. This ensures that we do
 * not dispatch multiple error events.
 * @private
 */
goog.net.XhrIo.prototype.dispatchErrors_ = function() {
  if (!this.errorDispatched_) {
    this.errorDispatched_ = true;
    this.dispatchEvent(goog.net.EventType.COMPLETE);
    this.dispatchEvent(goog.net.EventType.ERROR);
  }
};


/**
 * Abort the current XMLHttpRequest
 * @param {goog.net.ErrorCode=} opt_failureCode Optional error code to use -
 *     defaults to ABORT.
 */
goog.net.XhrIo.prototype.abort = function(opt_failureCode) {
  if (this.xhr_ && this.active_) {
    this.logger_.fine(this.formatMsg_('Aborting'));
    this.active_ = false;
    this.inAbort_ = true;
    this.xhr_.abort();
    this.inAbort_ = false;
    this.lastErrorCode_ = opt_failureCode || goog.net.ErrorCode.ABORT;
    this.dispatchEvent(goog.net.EventType.COMPLETE);
    this.dispatchEvent(goog.net.EventType.ABORT);
    this.cleanUpXhr_();
  }
};


/**
 * Nullifies all callbacks to reduce risks of leaks.
 * @override
 * @protected
 */
goog.net.XhrIo.prototype.disposeInternal = function() {
  if (this.xhr_) {
    // We explicitly do not call xhr_.abort() unless active_ is still true.
    // This is to avoid unnecessarily aborting a successful request when
    // dispose() is called in a callback triggered by a complete response, but
    // in which browser cleanup has not yet finished.
    // (See http://b/issue?id=1684217.)
    if (this.active_) {
      this.active_ = false;
      this.inAbort_ = true;
      this.xhr_.abort();
      this.inAbort_ = false;
    }
    this.cleanUpXhr_(true);
  }

  goog.net.XhrIo.superClass_.disposeInternal.call(this);
};


/**
 * Internal handler for the XHR object's readystatechange event.  This method
 * checks the status and the readystate and fires the correct callbacks.
 * If the request has ended, the handlers are cleaned up and the XHR object is
 * nullified.
 * @private
 */
goog.net.XhrIo.prototype.onReadyStateChange_ = function() {
  if (!this.inOpen_ && !this.inSend_ && !this.inAbort_) {
    // Were not being called from within a call to this.xhr_.send
    // this.xhr_.abort, or this.xhr_.open, so this is an entry point
    this.onReadyStateChangeEntryPoint_();
  } else {
    this.onReadyStateChangeHelper_();
  }
};


/**
 * Used to protect the onreadystatechange handler entry point.  Necessary
 * as {#onReadyStateChange_} maybe called from within send or abort, this
 * method is only called when {#onReadyStateChange_} is called as an
 * entry point.
 * {@see #protectEntryPoints}
 * @private
 */
goog.net.XhrIo.prototype.onReadyStateChangeEntryPoint_ = function() {
  this.onReadyStateChangeHelper_();
};


/**
 * Helper for {@link #onReadyStateChange_}.  This is used so that
 * entry point calls to {@link #onReadyStateChange_} can be routed through
 * {@link #onReadyStateChangeEntryPoint_}.
 * @private
 */
goog.net.XhrIo.prototype.onReadyStateChangeHelper_ = function() {
  if (!this.active_) {
    // can get called inside abort call
    return;
  }

  if (typeof goog == 'undefined') {
    // NOTE(user): If goog is undefined then the callback has occurred as the
    // application is unloading and will error.  Thus we let it silently fail.

  } else if (
      this.xhrOptions_[goog.net.XmlHttp.OptionType.LOCAL_REQUEST_ERROR] &&
      this.getReadyState() == goog.net.XmlHttp.ReadyState.COMPLETE &&
      this.getStatus() == 2) {
    // NOTE(user): In IE if send() errors on a *local* request the readystate
    // is still changed to COMPLETE.  We need to ignore it and allow the
    // try/catch around send() to pick up the error.
    this.logger_.fine(this.formatMsg_(
        'Local request error detected and ignored'));

  } else {

    // In IE when the response has been cached we sometimes get the callback
    // from inside the send call and this usually breaks code that assumes that
    // XhrIo is asynchronous.  If that is the case we delay the callback
    // using a timer.
    if (this.inSend_ &&
        this.getReadyState() == goog.net.XmlHttp.ReadyState.COMPLETE) {
      goog.Timer.defaultTimerObject.setTimeout(
          goog.bind(this.onReadyStateChange_, this), 0);
      return;
    }

    this.dispatchEvent(goog.net.EventType.READY_STATE_CHANGE);

    // readyState indicates the transfer has finished
    if (this.isComplete()) {
      this.logger_.fine(this.formatMsg_('Request complete'));

      this.active_ = false;

      try {
        // Call the specific callbacks for success or failure. Only call the
        // success if the status is 200 (HTTP_OK) or 304 (HTTP_CACHED)
        if (this.isSuccess()) {
          this.dispatchEvent(goog.net.EventType.COMPLETE);
          this.dispatchEvent(goog.net.EventType.SUCCESS);
        } else {
          this.lastErrorCode_ = goog.net.ErrorCode.HTTP_ERROR;
          this.lastError_ =
              this.getStatusText() + ' [' + this.getStatus() + ']';
          this.dispatchErrors_();
        }
      } finally {
        this.cleanUpXhr_();
      }
    }
  }
};


/**
 * Remove the listener to protect against leaks, and nullify the XMLHttpRequest
 * object.
 * @param {boolean=} opt_fromDispose If this is from the dispose (don't want to
 *     fire any events).
 * @private
 */
goog.net.XhrIo.prototype.cleanUpXhr_ = function(opt_fromDispose) {
  if (this.xhr_) {
    // Save reference so we can mark it as closed after the READY event.  The
    // READY event may trigger another request, thus we must nullify this.xhr_
    var xhr = this.xhr_;
    var clearedOnReadyStateChange =
        this.xhrOptions_[goog.net.XmlHttp.OptionType.USE_NULL_FUNCTION] ?
            goog.nullFunction : null;
    this.xhr_ = null;
    this.xhrOptions_ = null;

    if (this.timeoutId_) {
      // Cancel any pending timeout event handler.
      goog.Timer.defaultTimerObject.clearTimeout(this.timeoutId_);
      this.timeoutId_ = null;
    }

    if (!opt_fromDispose) {
      this.dispatchEvent(goog.net.EventType.READY);
    }

    try {
      // NOTE(user): Not nullifying in FireFox can still leak if the callbacks
      // are defined in the same scope as the instance of XhrIo. But, IE doesn't
      // allow you to set the onreadystatechange to NULL so nullFunction is
      // used.
      xhr.onreadystatechange = clearedOnReadyStateChange;
    } catch (e) {
      // This seems to occur with a Gears HTTP request. Delayed the setting of
      // this onreadystatechange until after READY is sent out and catching the
      // error to see if we can track down the problem.
      this.logger_.severe('Problem encountered resetting onreadystatechange: ' +
                          e.message);
    }
  }
};


/**
 * @return {boolean} Whether there is an active request.
 */
goog.net.XhrIo.prototype.isActive = function() {
  return !!this.xhr_;
};


/**
 * @return {boolean} Whether the request has completed.
 */
goog.net.XhrIo.prototype.isComplete = function() {
  return this.getReadyState() == goog.net.XmlHttp.ReadyState.COMPLETE;
};


/**
 * @return {boolean} Whether the request completed with a success.
 */
goog.net.XhrIo.prototype.isSuccess = function() {
  var status = this.getStatus();
  // A zero status code is considered successful for local files.
  return goog.net.HttpStatus.isSuccess(status) ||
      status === 0 && !this.isLastUriEffectiveSchemeHttp_();
};


/**
 * @return {boolean} whether the effective scheme of the last URI that was
 *     fetched was 'http' or 'https'.
 * @private
 */
goog.net.XhrIo.prototype.isLastUriEffectiveSchemeHttp_ = function() {
  var scheme = goog.uri.utils.getEffectiveScheme(String(this.lastUri_));
  return goog.net.XhrIo.HTTP_SCHEME_PATTERN.test(scheme);
};


/**
 * Get the readystate from the Xhr object
 * Will only return correct result when called from the context of a callback
 * @return {goog.net.XmlHttp.ReadyState} goog.net.XmlHttp.ReadyState.*.
 */
goog.net.XhrIo.prototype.getReadyState = function() {
  return this.xhr_ ?
      /** @type {goog.net.XmlHttp.ReadyState} */ (this.xhr_.readyState) :
      goog.net.XmlHttp.ReadyState.UNINITIALIZED;
};


/**
 * Get the status from the Xhr object
 * Will only return correct result when called from the context of a callback
 * @return {number} Http status.
 */
goog.net.XhrIo.prototype.getStatus = function() {
  /**
   * IE doesn't like you checking status until the readystate is greater than 2
   * (i.e. it is recieving or complete).  The try/catch is used for when the
   * page is unloading and an ERROR_NOT_AVAILABLE may occur when accessing xhr_.
   * @preserveTry
   */
  try {
    return this.getReadyState() > goog.net.XmlHttp.ReadyState.LOADED ?
        this.xhr_.status : -1;
  } catch (e) {
    this.logger_.warning('Can not get status: ' + e.message);
    return -1;
  }
};


/**
 * Get the status text from the Xhr object
 * Will only return correct result when called from the context of a callback
 * @return {string} Status text.
 */
goog.net.XhrIo.prototype.getStatusText = function() {
  /**
   * IE doesn't like you checking status until the readystate is greater than 2
   * (i.e. it is recieving or complete).  The try/catch is used for when the
   * page is unloading and an ERROR_NOT_AVAILABLE may occur when accessing xhr_.
   * @preserveTry
   */
  try {
    return this.getReadyState() > goog.net.XmlHttp.ReadyState.LOADED ?
        this.xhr_.statusText : '';
  } catch (e) {
    this.logger_.fine('Can not get status: ' + e.message);
    return '';
  }
};


/**
 * Get the last Uri that was requested
 * @return {string} Last Uri.
 */
goog.net.XhrIo.prototype.getLastUri = function() {
  return String(this.lastUri_);
};


/**
 * Get the response text from the Xhr object
 * Will only return correct result when called from the context of a callback.
 * @return {string} Result from the server, or '' if no result available.
 */
goog.net.XhrIo.prototype.getResponseText = function() {
  /** @preserveTry */
  try {
    return this.xhr_ ? this.xhr_.responseText : '';
  } catch (e) {
    // http://www.w3.org/TR/XMLHttpRequest/#the-responsetext-attribute
    // states that responseText should return '' (and responseXML null)
    // when the state is not LOADING or DONE. Instead, IE and Gears can
    // throw unexpected exceptions, eg, when a request is aborted or no
    // data is available yet.
    this.logger_.fine('Can not get responseText: ' + e.message);
    return '';
  }
};


/**
 * Get the response XML from the Xhr object
 * Will only return correct result when called from the context of a callback.
 * @return {Document} The DOM Document representing the XML file, or null
 * if no result available.
 */
goog.net.XhrIo.prototype.getResponseXml = function() {
  /** @preserveTry */
  try {
    return this.xhr_ ? this.xhr_.responseXML : null;
  } catch (e) {
    this.logger_.fine('Can not get responseXML: ' + e.message);
    return null;
  }
};


/**
 * Get the response and evaluates it as JSON from the Xhr object
 * Will only return correct result when called from the context of a callback
 * @param {string=} opt_xssiPrefix Optional XSSI prefix string to use for
 *     stripping of the response before parsing. This needs to be set only if
 *     your backend server prepends the same prefix string to the JSON response.
 * @return {Object|undefined} JavaScript object.
 */
goog.net.XhrIo.prototype.getResponseJson = function(opt_xssiPrefix) {
  if (!this.xhr_) {
    return undefined;
  }

  var responseText = this.xhr_.responseText;
  if (opt_xssiPrefix && responseText.indexOf(opt_xssiPrefix) == 0) {
    responseText = responseText.substring(opt_xssiPrefix.length);
  }

  return goog.json.parse(responseText);
};


/**
 * Get the response as the type specificed by {@link #setResponseType}. At time
 * of writing, this is only directly supported in very recent versions of WebKit
 * (10.0.612.1 dev and later). If the field is not supported directly, we will
 * try to emulate it.
 *
 * Emulating the response means following the rules laid out at
 * http://www.w3.org/TR/XMLHttpRequest/#the-response-attribute
 *
 * On browsers with no support for this (Chrome < 10, Firefox < 4, etc), only
 * response types of DEFAULT or TEXT may be used, and the response returned will
 * be the text response.
 *
 * On browsers with Mozilla's draft support for array buffers (Firefox 4, 5),
 * only response types of DEFAULT, TEXT, and ARRAY_BUFFER may be used, and the
 * response returned will be either the text response or the Mozilla
 * implementation of the array buffer response.
 *
 * On browsers will full support, any valid response type supported by the
 * browser may be used, and the response provided by the browser will be
 * returned.
 *
 * @return {*} The response.
 */
goog.net.XhrIo.prototype.getResponse = function() {
  /** @preserveTry */
  try {
    if (!this.xhr_) {
      return null;
    }
    if ('response' in this.xhr_) {
      return this.xhr_.response;
    }
    switch (this.responseType_) {
      case goog.net.XhrIo.ResponseType.DEFAULT:
      case goog.net.XhrIo.ResponseType.TEXT:
        return this.xhr_.responseText;
        // DOCUMENT and BLOB don't need to be handled here because they are
        // introduced in the same spec that adds the .response field, and would
        // have been caught above.
        // ARRAY_BUFFER needs an implementation for Firefox 4, where it was
        // implemented using a draft spec rather than the final spec.
      case goog.net.XhrIo.ResponseType.ARRAY_BUFFER:
        if ('mozResponseArrayBuffer' in this.xhr_) {
          return this.xhr_.mozResponseArrayBuffer;
        }
    }
    // Fell through to a response type that is not supported on this browser.
    this.logger_.severe('Response type ' + this.responseType_ + ' is not ' +
                        'supported on this browser');
    return null;
  } catch (e) {
    this.logger_.fine('Can not get response: ' + e.message);
    return null;
  }
};


/**
 * Get the value of the response-header with the given name from the Xhr object
 * Will only return correct result when called from the context of a callback
 * and the request has completed
 * @param {string} key The name of the response-header to retrieve.
 * @return {string|undefined} The value of the response-header named key.
 */
goog.net.XhrIo.prototype.getResponseHeader = function(key) {
  return this.xhr_ && this.isComplete() ?
      this.xhr_.getResponseHeader(key) : undefined;
};


/**
 * Gets the text of all the headers in the response.
 * Will only return correct result when called from the context of a callback
 * and the request has completed.
 * @return {string} The value of the response headers or empty string.
 */
goog.net.XhrIo.prototype.getAllResponseHeaders = function() {
  return this.xhr_ && this.isComplete() ?
      this.xhr_.getAllResponseHeaders() : '';
};


/**
 * Get the last error message
 * @return {goog.net.ErrorCode} Last error code.
 */
goog.net.XhrIo.prototype.getLastErrorCode = function() {
  return this.lastErrorCode_;
};


/**
 * Get the last error message
 * @return {string} Last error message.
 */
goog.net.XhrIo.prototype.getLastError = function() {
  return goog.isString(this.lastError_) ? this.lastError_ :
      String(this.lastError_);
};


/**
 * Adds the last method, status and URI to the message.  This is used to add
 * this information to the logging calls.
 * @param {string} msg The message text that we want to add the extra text to.
 * @return {string} The message with the extra text appended.
 * @private
 */
goog.net.XhrIo.prototype.formatMsg_ = function(msg) {
  return msg + ' [' + this.lastMethod_ + ' ' + this.lastUri_ + ' ' +
      this.getStatus() + ']';
};


// Register the xhr handler as an entry point, so that
// it can be monitored for exception handling, etc.
goog.debug.entryPointRegistry.register(
    /**
     * @param {function(!Function): !Function} transformer The transforming
     *     function.
     */
    function(transformer) {
      goog.net.XhrIo.prototype.onReadyStateChangeEntryPoint_ =
          transformer(goog.net.XhrIo.prototype.onReadyStateChangeEntryPoint_);
    });
