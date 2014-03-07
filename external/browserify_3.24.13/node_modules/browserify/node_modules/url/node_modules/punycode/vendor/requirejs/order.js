/**
 * @license RequireJS order 1.0.5 Copyright (c) 2010-2011, The Dojo Foundation All Rights Reserved.
 * Available via the MIT or new BSD license.
 * see: http://github.com/jrburke/requirejs for details
 */
/*jslint nomen: false, plusplus: false, strict: false */
/*global require: false, define: false, window: false, document: false,
  setTimeout: false */

//Specify that requirejs optimizer should wrap this code in a closure that
//maps the namespaced requirejs API to non-namespaced local variables.
/*requirejs namespace: true */

(function () {

    //Sadly necessary browser inference due to differences in the way
    //that browsers load and execute dynamically inserted javascript
    //and whether the script/cache method works when ordered execution is
    //desired. Currently, Gecko and Opera do not load/fire onload for scripts with
    //type="script/cache" but they execute injected scripts in order
    //unless the 'async' flag is present.
    //However, this is all changing in latest browsers implementing HTML5
    //spec. With compliant browsers .async true by default, and
    //if false, then it will execute in order. Favor that test first for forward
    //compatibility.
    var testScript = typeof document !== "undefined" &&
                 typeof window !== "undefined" &&
                 document.createElement("script"),

        supportsInOrderExecution = testScript && (testScript.async ||
                               ((window.opera &&
                                 Object.prototype.toString.call(window.opera) === "[object Opera]") ||
                               //If Firefox 2 does not have to be supported, then
                               //a better check may be:
                               //('mozIsLocallyAvailable' in window.navigator)
                               ("MozAppearance" in document.documentElement.style))),

        //This test is true for IE browsers, which will load scripts but only
        //execute them once the script is added to the DOM.
        supportsLoadSeparateFromExecute = testScript &&
                                          testScript.readyState === 'uninitialized',

        readyRegExp = /^(complete|loaded)$/,
        cacheWaiting = [],
        cached = {},
        scriptNodes = {},
        scriptWaiting = [];

    //Done with the test script.
    testScript = null;

    //Callback used by the type="script/cache" callback that indicates a script
    //has finished downloading.
    function scriptCacheCallback(evt) {
        var node = evt.currentTarget || evt.srcElement, i,
            moduleName, resource;

        if (evt.type === "load" || readyRegExp.test(node.readyState)) {
            //Pull out the name of the module and the context.
            moduleName = node.getAttribute("data-requiremodule");

            //Mark this cache request as loaded
            cached[moduleName] = true;

            //Find out how many ordered modules have loaded
            for (i = 0; (resource = cacheWaiting[i]); i++) {
                if (cached[resource.name]) {
                    resource.req([resource.name], resource.onLoad);
                } else {
                    //Something in the ordered list is not loaded,
                    //so wait.
                    break;
                }
            }

            //If just loaded some items, remove them from cacheWaiting.
            if (i > 0) {
                cacheWaiting.splice(0, i);
            }

            //Remove this script tag from the DOM
            //Use a setTimeout for cleanup because some older IE versions vomit
            //if removing a script node while it is being evaluated.
            setTimeout(function () {
                node.parentNode.removeChild(node);
            }, 15);
        }
    }

    /**
     * Used for the IE case, where fetching is done by creating script element
     * but not attaching it to the DOM. This function will be called when that
     * happens so it can be determined when the node can be attached to the
     * DOM to trigger its execution.
     */
    function onFetchOnly(node) {
        var i, loadedNode, resourceName;

        //Mark this script as loaded.
        node.setAttribute('data-orderloaded', 'loaded');

        //Cycle through waiting scripts. If the matching node for them
        //is loaded, and is in the right order, add it to the DOM
        //to execute the script.
        for (i = 0; (resourceName = scriptWaiting[i]); i++) {
            loadedNode = scriptNodes[resourceName];
            if (loadedNode &&
                loadedNode.getAttribute('data-orderloaded') === 'loaded') {
                delete scriptNodes[resourceName];
                require.addScriptToDom(loadedNode);
            } else {
                break;
            }
        }

        //If just loaded some items, remove them from waiting.
        if (i > 0) {
            scriptWaiting.splice(0, i);
        }
    }

    define({
        version: '1.0.5',

        load: function (name, req, onLoad, config) {
            var hasToUrl = !!req.nameToUrl,
                url, node, context;

            //If no nameToUrl, then probably a build with a loader that
            //does not support it, and all modules are inlined.
            if (!hasToUrl) {
                req([name], onLoad);
                return;
            }

            url = req.nameToUrl(name, null);

            //Make sure the async attribute is not set for any pathway involving
            //this script.
            require.s.skipAsync[url] = true;
            if (supportsInOrderExecution || config.isBuild) {
                //Just a normal script tag append, but without async attribute
                //on the script.
                req([name], onLoad);
            } else if (supportsLoadSeparateFromExecute) {
                //Just fetch the URL, but do not execute it yet. The
                //non-standards IE case. Really not so nice because it is
                //assuming and touching requrejs internals. OK though since
                //ordered execution should go away after a long while.
                context = require.s.contexts._;

                if (!context.urlFetched[url] && !context.loaded[name]) {
                    //Indicate the script is being fetched.
                    context.urlFetched[url] = true;

                    //Stuff from require.load
                    require.resourcesReady(false);
                    context.scriptCount += 1;

                    //Fetch the script now, remember it.
                    node = require.attach(url, context, name, null, null, onFetchOnly);
                    scriptNodes[name] = node;
                    scriptWaiting.push(name);
                }

                //Do a normal require for it, once it loads, use it as return
                //value.
                req([name], onLoad);
            } else {
                //Credit to LABjs author Kyle Simpson for finding that scripts
                //with type="script/cache" allow scripts to be downloaded into
                //browser cache but not executed. Use that
                //so that subsequent addition of a real type="text/javascript"
                //tag will cause the scripts to be executed immediately in the
                //correct order.
                if (req.specified(name)) {
                    req([name], onLoad);
                } else {
                    cacheWaiting.push({
                        name: name,
                        req: req,
                        onLoad: onLoad
                    });
                    require.attach(url, null, name, scriptCacheCallback, "script/cache");
                }
            }
        }
    });
}());
