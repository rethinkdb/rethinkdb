/**
 * Copyright (c) 2014 Petka Antonov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:</p>
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */
"use strict";
var global = require("./global.js");
var setTimeout = function(fn, ms) {
    var $_len = arguments.length;var args = new Array($_len - 2); for(var $_i = 2; $_i < $_len; ++$_i) {args[$_i - 2] = arguments[$_i];}
    global.setTimeout(function(){
        fn.apply(void 0, args);
    }, ms);
};

module.exports = function(Promise, INTERNAL) {
var util = require("./util.js");
var errors = require("./errors.js");
var apiRejection = require("./errors_api_rejection")(Promise);
var TimeoutError = Promise.TimeoutError;

var afterTimeout = function Promise$_afterTimeout(promise, message, ms) {
    if (!promise.isPending()) return;
    if (typeof message !== "string") {
        message = "operation timed out after" + " " + ms + " ms"
    }
    var err = new TimeoutError(message);
    errors.markAsOriginatingFromRejection(err);
    promise._attachExtraTrace(err);
    promise._rejectUnchecked(err);
};

var afterDelay = function Promise$_afterDelay(value, promise) {
    promise._fulfill(value);
};

var delay = Promise.delay = function Promise$Delay(value, ms) {
    if (ms === void 0) {
        ms = value;
        value = void 0;
    }
    ms = +ms;
    var maybePromise = Promise._cast(value, void 0);
    var promise = new Promise(INTERNAL);

    if (maybePromise instanceof Promise) {
        if (maybePromise._isBound()) {
            promise._setBoundTo(maybePromise._boundTo);
        }
        if (maybePromise._cancellable()) {
            promise._setCancellable();
            promise._cancellationParent = maybePromise;
        }
        promise._setTrace(maybePromise);
        promise._follow(maybePromise);
        return promise.then(function(value) {
            return Promise.delay(value, ms);
        });
    }
    else {
        promise._setTrace(void 0);
        setTimeout(afterDelay, ms, value, promise);
    }
    return promise;
};

Promise.prototype.delay = function Promise$delay(ms) {
    return delay(this, ms);
};

Promise.prototype.timeout = function Promise$timeout(ms, message) {
    ms = +ms;

    var ret = new Promise(INTERNAL);
    ret._setTrace(this);

    if (this._isBound()) ret._setBoundTo(this._boundTo);
    if (this._cancellable()) {
        ret._setCancellable();
        ret._cancellationParent = this;
    }
    ret._follow(this);
    setTimeout(afterTimeout, ms, ret, message, ms);
    return ret;
};

};
