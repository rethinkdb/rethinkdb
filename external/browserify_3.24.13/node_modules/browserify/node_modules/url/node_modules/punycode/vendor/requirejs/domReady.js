/**
 * @license RequireJS domReady 1.0.0 Copyright (c) 2010-2011, The Dojo Foundation All Rights Reserved.
 * Available via the MIT or new BSD license.
 * see: http://github.com/jrburke/requirejs for details
 */
/*jslint strict: false, plusplus: false */
/*global require: false, define: false, requirejs: false,
  window: false, clearInterval: false, document: false,
  self: false, setInterval: false */


define(function () {
    var isBrowser = typeof window !== "undefined" && window.document,
        isPageLoaded = !isBrowser,
        doc = isBrowser ? document : null,
        readyCalls = [],
        readyLoaderCalls = [],
        //Bind to a specific implementation, but if not there, try a
        //a generic one under the "require" name.
        req = requirejs || require || {},
        oldResourcesReady = req.resourcesReady,
        scrollIntervalId;

    function runCallbacks(callbacks) {
        for (var i = 0, callback; (callback = callbacks[i]); i++) {
            callback(doc);
        }
    }

    function callReady() {
        var callbacks = readyCalls,
            loaderCallbacks = readyLoaderCalls;

        if (isPageLoaded) {
            //Call the DOM ready callbacks
            if (callbacks.length) {
                readyCalls = [];
                runCallbacks(callbacks);
            }

            //Now handle DOM ready + loader ready callbacks.
            if (req.resourcesDone && loaderCallbacks.length) {
                readyLoaderCalls = [];
                runCallbacks(loaderCallbacks);
            }
        }
    }

    /**
     * Add a method to require to get callbacks if there are loader resources still
     * being loaded. If so, then hold off calling "withResources" callbacks.
     *
     * @param {Boolean} isReady: pass true if all resources have been loaded.
     */
    if ('resourcesReady' in req) {
        req.resourcesReady = function (isReady) {
            //Call the old function if it is around.
            if (oldResourcesReady) {
                oldResourcesReady(isReady);
            }

            if (isReady) {
                callReady();
            }
        };
    }

    /**
     * Sets the page as loaded.
     */
    function pageLoaded() {
        if (!isPageLoaded) {
            isPageLoaded = true;
            if (scrollIntervalId) {
                clearInterval(scrollIntervalId);
            }

            callReady();
        }
    }

    if (isBrowser) {
        if (document.addEventListener) {
            //Standards. Hooray! Assumption here that if standards based,
            //it knows about DOMContentLoaded.
            document.addEventListener("DOMContentLoaded", pageLoaded, false);
            window.addEventListener("load", pageLoaded, false);
        } else if (window.attachEvent) {
            window.attachEvent("onload", pageLoaded);

            //DOMContentLoaded approximation, as found by Diego Perini:
            //http://javascript.nwbox.com/IEContentLoaded/
            if (self === self.top) {
                scrollIntervalId = setInterval(function () {
                    try {
                        //From this ticket:
                        //http://bugs.dojotoolkit.org/ticket/11106,
                        //In IE HTML Application (HTA), such as in a selenium test,
                        //javascript in the iframe can't see anything outside
                        //of it, so self===self.top is true, but the iframe is
                        //not the top window and doScroll will be available
                        //before document.body is set. Test document.body
                        //before trying the doScroll trick.
                        if (document.body) {
                            document.documentElement.doScroll("left");
                            pageLoaded();
                        }
                    } catch (e) {}
                }, 30);
            }
        }

        //Check if document already complete, and if so, just trigger page load
        //listeners.
        if (document.readyState === "complete") {
            pageLoaded();
        }
    }

    /** START OF PUBLIC API **/

    /**
     * Registers a callback for DOM ready. If DOM is already ready, the
     * callback is called immediately.
     * @param {Function} callback
     */
    function domReady(callback) {
        if (isPageLoaded) {
            callback(doc);
        } else {
            readyCalls.push(callback);
        }
        return domReady;
    }

    /**
     * Callback that waits for DOM ready as well as any outstanding
     * loader resources. Useful when there are implicit dependencies.
     * This method should be avoided, and always use explicit
     * dependency resolution, with just regular DOM ready callbacks.
     * The callback passed to this method will be called immediately
     * if the DOM and loader are already ready.
     * @param {Function} callback
     */
    domReady.withResources = function (callback) {
        if (isPageLoaded && req.resourcesDone) {
            callback(doc);
        } else {
            readyLoaderCalls.push(callback);
        }
        return domReady;
    };

    domReady.version = '1.0.0';

    /**
     * Loader Plugin API method
     */
    domReady.load = function (name, req, onLoad, config) {
        if (config.isBuild) {
            onLoad(null);
        } else {
            domReady(onLoad);
        }
    };

    /** END OF PUBLIC API **/

    return domReady;
});
