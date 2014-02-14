// #ifdef UNDEFINED
/*
 Copyright 2013 Daniel Wirtz <dcode@dcode.io>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */
// #endif
/**
 * @alias ProtoBuf.Util
 * @expose
 */
ProtoBuf.Util = (function() {
    "use strict";

    // Object.create polyfill
    // ref: https://developer.mozilla.org/de/docs/JavaScript/Reference/Global_Objects/Object/create
    if (!Object.create) {
        /** @expose */
        Object.create = function (o) {
            if (arguments.length > 1) {
                throw new Error('Object.create implementation only accepts the first parameter.');
            }
            function F() {}
            F.prototype = o;
            return new F();
        };
    }

    /**
     * ProtoBuf utilities.
     * @exports ProtoBuf.Util
     * @namespace
     */
    var Util = {};

    /**
     * Flag if running in node or not.
     * @type {boolean}
     * @const
     * @expose
     */
    Util.IS_NODE = (typeof window === 'undefined' || !window.window) && typeof require === 'function' && typeof process !== 'undefined' && typeof process["nextTick"] === 'function';
    
    /**
     * Constructs a XMLHttpRequest object.
     * @return {XMLHttpRequest}
     * @throws {Error} If XMLHttpRequest is not supported
     * @expose
     */
    Util.XHR = function() {
        // No dependencies please, ref: http://www.quirksmode.org/js/xmlhttp.html
        var XMLHttpFactories = [
            function () {return new XMLHttpRequest()},
            function () {return new ActiveXObject("Msxml2.XMLHTTP")},
            function () {return new ActiveXObject("Msxml3.XMLHTTP")},
            function () {return new ActiveXObject("Microsoft.XMLHTTP")}
        ];
        /** @type {?XMLHttpRequest} */
        var xhr = null;
        for (var i=0;i<XMLHttpFactories.length;i++) {
            try { xhr = XMLHttpFactories[i](); }
            catch (e) { continue; }
            break;
        }
        if (!xhr) throw(new Error("XMLHttpRequest is not supported"));
        return xhr;
    };

    /**
     * Fetches a resource.
     * @param {string} path Resource path
     * @param {function(?string)=} callback Callback receiving the resource's contents. If omitted the resource will
     *   be fetched synchronously. If the request failed, contents will be null.
     * @return {?string|undefined} Resource contents if callback is omitted (null if the request failed), else undefined.
     * @expose
     */
    Util.fetch = function(path, callback) {
        if (callback && typeof callback != 'function') callback = null;
        if (Util.IS_NODE) {
            if (callback) {
                require("fs").readFile(path, function(err, data) {
                    if (err) {
                        callback(null);
                    }
                    else callback(""+data);
                });
            } else {
                try {
                    return require("fs").readFileSync(path);
                } catch (e) {
                    return null;
                }
            }
        } else {
            var xhr = Util.XHR();
            xhr.open('GET', path, callback ? true : false);
            // xhr.setRequestHeader('User-Agent', 'XMLHTTP/1.0');
            xhr.setRequestHeader('Accept', 'text/plain');
            if (typeof xhr.overrideMimeType === 'function') xhr.overrideMimeType('text/plain');
            if (callback) {
                xhr.onreadystatechange = function() {
                    if (xhr.readyState != 4) return;
                    if (/* remote */ xhr.status == 200 || /* local */ (xhr.status == 0 && typeof xhr.responseText === 'string')) {
                        callback(xhr.responseText);
                    } else {
                        callback(null);
                    }
                };
                if (xhr.readyState == 4) return;
                xhr.send(null);
            } else {
                xhr.send(null);
                if (/* remote */ xhr.status == 200 || /* local */ (xhr.status == 0 && typeof xhr.responseText === 'string')) {
                    return xhr.responseText;
                }
                return null;
            }
        }
    };

    /**
     * Tests if an object is an array.
     * @param {*} obj Object to test
     * @returns {boolean} true if it is an array, else false
     * @expose
     */
    Util.isArray = function(obj) {
        if (!obj) return false;
        if (obj instanceof Array) return true;
        if (Array.isArray) return Array.isArray(obj);
        return Object.prototype.toString.call(obj) === "[object Array]";
    };
    
    return Util;
})();