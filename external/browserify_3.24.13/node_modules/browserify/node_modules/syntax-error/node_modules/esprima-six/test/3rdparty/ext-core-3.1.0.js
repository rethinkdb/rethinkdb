/*!
 * Ext Core Library 3.0
 * http://extjs.com/
 * Copyright(c) 2006-2009, Ext JS, LLC.
 * 
 * MIT Licensed - http://extjs.com/license/mit.txt
 */

// for old browsers
window.undefined = window.undefined;

/**
 * @class Ext
 * Ext core utilities and functions.
 * @singleton
 */

Ext = {
    /**
     * The version of the framework
     * @type String
     */
    version : '3.1.0'
};

/**
 * Copies all the properties of config to obj.
 * @param {Object} obj The receiver of the properties
 * @param {Object} config The source of the properties
 * @param {Object} defaults A different object that will also be applied for default values
 * @return {Object} returns obj
 * @member Ext apply
 */
Ext.apply = function(o, c, defaults){
    // no "this" reference for friendly out of scope calls
    if(defaults){
        Ext.apply(o, defaults);
    }
    if(o && c && typeof c == 'object'){
        for(var p in c){
            o[p] = c[p];
        }
    }
    return o;
};

(function(){
    var idSeed = 0,
        toString = Object.prototype.toString,
        ua = navigator.userAgent.toLowerCase(),
        check = function(r){
            return r.test(ua);
        },
        DOC = document,
        isStrict = DOC.compatMode == "CSS1Compat",
        isOpera = check(/opera/),
        isChrome = check(/chrome/),
        isWebKit = check(/webkit/),
        isSafari = !isChrome && check(/safari/),
        isSafari2 = isSafari && check(/applewebkit\/4/), // unique to Safari 2
        isSafari3 = isSafari && check(/version\/3/),
        isSafari4 = isSafari && check(/version\/4/),
        isIE = !isOpera && check(/msie/),
        isIE7 = isIE && check(/msie 7/),
        isIE8 = isIE && check(/msie 8/),
        isIE6 = isIE && !isIE7 && !isIE8,
        isGecko = !isWebKit && check(/gecko/),
        isGecko2 = isGecko && check(/rv:1\.8/),
        isGecko3 = isGecko && check(/rv:1\.9/),
        isBorderBox = isIE && !isStrict,
        isWindows = check(/windows|win32/),
        isMac = check(/macintosh|mac os x/),
        isAir = check(/adobeair/),
        isLinux = check(/linux/),
        isSecure = /^https/i.test(window.location.protocol);

    // remove css image flicker
    if(isIE6){
        try{
            DOC.execCommand("BackgroundImageCache", false, true);
        }catch(e){}
    }

    Ext.apply(Ext, {
        /**
         * URL to a blank file used by Ext when in secure mode for iframe src and onReady src to prevent
         * the IE insecure content warning (<tt>'about:blank'</tt>, except for IE in secure mode, which is <tt>'javascript:""'</tt>).
         * @type String
         */
        SSL_SECURE_URL : isSecure && isIE ? 'javascript:""' : 'about:blank',
        /**
         * True if the browser is in strict (standards-compliant) mode, as opposed to quirks mode
         * @type Boolean
         */
        isStrict : isStrict,
        /**
         * True if the page is running over SSL
         * @type Boolean
         */
        isSecure : isSecure,
        /**
         * True when the document is fully initialized and ready for action
         * @type Boolean
         */
        isReady : false,

        /**
         * True if the {@link Ext.Fx} Class is available
         * @type Boolean
         * @property enableFx
         */

        /**
         * True to automatically uncache orphaned Ext.Elements periodically (defaults to true)
         * @type Boolean
         */
        enableGarbageCollector : true,

        /**
         * True to automatically purge event listeners during garbageCollection (defaults to false).
         * @type Boolean
         */
        enableListenerCollection : false,

        /**
         * EXPERIMENTAL - True to cascade listener removal to child elements when an element is removed.
         * Currently not optimized for performance.
         * @type Boolean
         */
        enableNestedListenerRemoval : false,

        /**
         * Indicates whether to use native browser parsing for JSON methods.
         * This option is ignored if the browser does not support native JSON methods.
         * <b>Note: Native JSON methods will not work with objects that have functions.
         * Also, property names must be quoted, otherwise the data will not parse.</b> (Defaults to false)
         * @type Boolean
         */
        USE_NATIVE_JSON : false,

        /**
         * Copies all the properties of config to obj if they don't already exist.
         * @param {Object} obj The receiver of the properties
         * @param {Object} config The source of the properties
         * @return {Object} returns obj
         */
        applyIf : function(o, c){
            if(o){
                for(var p in c){
                    if(!Ext.isDefined(o[p])){
                        o[p] = c[p];
                    }
                }
            }
            return o;
        },

        /**
         * Generates unique ids. If the element already has an id, it is unchanged
         * @param {Mixed} el (optional) The element to generate an id for
         * @param {String} prefix (optional) Id prefix (defaults "ext-gen")
         * @return {String} The generated Id.
         */
        id : function(el, prefix){
            return (el = Ext.getDom(el) || {}).id = el.id || (prefix || "ext-gen") + (++idSeed);
        },

        /**
         * <p>Extends one class to create a subclass and optionally overrides members with the passed literal. This method
         * also adds the function "override()" to the subclass that can be used to override members of the class.</p>
         * For example, to create a subclass of Ext GridPanel:
         * <pre><code>
MyGridPanel = Ext.extend(Ext.grid.GridPanel, {
    constructor: function(config) {

//      Create configuration for this Grid.
        var store = new Ext.data.Store({...});
        var colModel = new Ext.grid.ColumnModel({...});

//      Create a new config object containing our computed properties
//      *plus* whatever was in the config parameter.
        config = Ext.apply({
            store: store,
            colModel: colModel
        }, config);

        MyGridPanel.superclass.constructor.call(this, config);

//      Your postprocessing here
    },

    yourMethod: function() {
        // etc.
    }
});
</code></pre>
         *
         * <p>This function also supports a 3-argument call in which the subclass's constructor is
         * passed as an argument. In this form, the parameters are as follows:</p>
         * <div class="mdetail-params"><ul>
         * <li><code>subclass</code> : Function <div class="sub-desc">The subclass constructor.</div></li>
         * <li><code>superclass</code> : Function <div class="sub-desc">The constructor of class being extended</div></li>
         * <li><code>overrides</code> : Object <div class="sub-desc">A literal with members which are copied into the subclass's
         * prototype, and are therefore shared among all instances of the new class.</div></li>
         * </ul></div>
         *
         * @param {Function} superclass The constructor of class being extended.
         * @param {Object} overrides <p>A literal with members which are copied into the subclass's
         * prototype, and are therefore shared between all instances of the new class.</p>
         * <p>This may contain a special member named <tt><b>constructor</b></tt>. This is used
         * to define the constructor of the new class, and is returned. If this property is
         * <i>not</i> specified, a constructor is generated and returned which just calls the
         * superclass's constructor passing on its parameters.</p>
         * <p><b>It is essential that you call the superclass constructor in any provided constructor. See example code.</b></p>
         * @return {Function} The subclass constructor from the <code>overrides</code> parameter, or a generated one if not provided.
         */
        extend : function(){
            // inline overrides
            var io = function(o){
                for(var m in o){
                    this[m] = o[m];
                }
            };
            var oc = Object.prototype.constructor;

            return function(sb, sp, overrides){
                if(Ext.isObject(sp)){
                    overrides = sp;
                    sp = sb;
                    sb = overrides.constructor != oc ? overrides.constructor : function(){sp.apply(this, arguments);};
                }
                var F = function(){},
                    sbp,
                    spp = sp.prototype;

                F.prototype = spp;
                sbp = sb.prototype = new F();
                sbp.constructor=sb;
                sb.superclass=spp;
                if(spp.constructor == oc){
                    spp.constructor=sp;
                }
                sb.override = function(o){
                    Ext.override(sb, o);
                };
                sbp.superclass = sbp.supr = (function(){
                    return spp;
                });
                sbp.override = io;
                Ext.override(sb, overrides);
                sb.extend = function(o){return Ext.extend(sb, o);};
                return sb;
            };
        }(),

        /**
         * Adds a list of functions to the prototype of an existing class, overwriting any existing methods with the same name.
         * Usage:<pre><code>
Ext.override(MyClass, {
    newMethod1: function(){
        // etc.
    },
    newMethod2: function(foo){
        // etc.
    }
});
</code></pre>
         * @param {Object} origclass The class to override
         * @param {Object} overrides The list of functions to add to origClass.  This should be specified as an object literal
         * containing one or more methods.
         * @method override
         */
        override : function(origclass, overrides){
            if(overrides){
                var p = origclass.prototype;
                Ext.apply(p, overrides);
                if(Ext.isIE && overrides.hasOwnProperty('toString')){
                    p.toString = overrides.toString;
                }
            }
        },

        /**
         * Creates namespaces to be used for scoping variables and classes so that they are not global.
         * Specifying the last node of a namespace implicitly creates all other nodes. Usage:
         * <pre><code>
Ext.namespace('Company', 'Company.data');
Ext.namespace('Company.data'); // equivalent and preferable to above syntax
Company.Widget = function() { ... }
Company.data.CustomStore = function(config) { ... }
</code></pre>
         * @param {String} namespace1
         * @param {String} namespace2
         * @param {String} etc
         * @return {Object} The namespace object. (If multiple arguments are passed, this will be the last namespace created)
         * @method namespace
         */
        namespace : function(){
            var o, d;
            Ext.each(arguments, function(v) {
                d = v.split(".");
                o = window[d[0]] = window[d[0]] || {};
                Ext.each(d.slice(1), function(v2){
                    o = o[v2] = o[v2] || {};
                });
            });
            return o;
        },

        /**
         * Takes an object and converts it to an encoded URL. e.g. Ext.urlEncode({foo: 1, bar: 2}); would return "foo=1&bar=2".  Optionally, property values can be arrays, instead of keys and the resulting string that's returned will contain a name/value pair for each array value.
         * @param {Object} o
         * @param {String} pre (optional) A prefix to add to the url encoded string
         * @return {String}
         */
        urlEncode : function(o, pre){
            var empty,
                buf = [],
                e = encodeURIComponent;

            Ext.iterate(o, function(key, item){
                empty = Ext.isEmpty(item);
                Ext.each(empty ? key : item, function(val){
                    buf.push('&', e(key), '=', (!Ext.isEmpty(val) && (val != key || !empty)) ? (Ext.isDate(val) ? Ext.encode(val).replace(/"/g, '') : e(val)) : '');
                });
            });
            if(!pre){
                buf.shift();
                pre = '';
            }
            return pre + buf.join('');
        },

        /**
         * Takes an encoded URL and and converts it to an object. Example: <pre><code>
Ext.urlDecode("foo=1&bar=2"); // returns {foo: "1", bar: "2"}
Ext.urlDecode("foo=1&bar=2&bar=3&bar=4", false); // returns {foo: "1", bar: ["2", "3", "4"]}
</code></pre>
         * @param {String} string
         * @param {Boolean} overwrite (optional) Items of the same name will overwrite previous values instead of creating an an array (Defaults to false).
         * @return {Object} A literal with members
         */
        urlDecode : function(string, overwrite){
            if(Ext.isEmpty(string)){
                return {};
            }
            var obj = {},
                pairs = string.split('&'),
                d = decodeURIComponent,
                name,
                value;
            Ext.each(pairs, function(pair) {
                pair = pair.split('=');
                name = d(pair[0]);
                value = d(pair[1]);
                obj[name] = overwrite || !obj[name] ? value :
                            [].concat(obj[name]).concat(value);
            });
            return obj;
        },

        /**
         * Appends content to the query string of a URL, handling logic for whether to place
         * a question mark or ampersand.
         * @param {String} url The URL to append to.
         * @param {String} s The content to append to the URL.
         * @return (String) The resulting URL
         */
        urlAppend : function(url, s){
            if(!Ext.isEmpty(s)){
                return url + (url.indexOf('?') === -1 ? '?' : '&') + s;
            }
            return url;
        },

        /**
         * Converts any iterable (numeric indices and a length property) into a true array
         * Don't use this on strings. IE doesn't support "abc"[0] which this implementation depends on.
         * For strings, use this instead: "abc".match(/./g) => [a,b,c];
         * @param {Iterable} the iterable object to be turned into a true Array.
         * @return (Array) array
         */
         toArray : function(){
             return isIE ?
                 function(a, i, j, res){
                     res = [];
                     for(var x = 0, len = a.length; x < len; x++) {
                         res.push(a[x]);
                     }
                     return res.slice(i || 0, j || res.length);
                 } :
                 function(a, i, j){
                     return Array.prototype.slice.call(a, i || 0, j || a.length);
                 }
         }(),

        isIterable : function(v){
            //check for array or arguments
            if(Ext.isArray(v) || v.callee){
                return true;
            }
            //check for node list type
            if(/NodeList|HTMLCollection/.test(toString.call(v))){
                return true;
            }
            //NodeList has an item and length property
            //IXMLDOMNodeList has nextNode method, needs to be checked first.
            return ((typeof v.nextNode != 'undefined' || v.item) && Ext.isNumber(v.length));
        },

        /**
         * Iterates an array calling the supplied function.
         * @param {Array/NodeList/Mixed} array The array to be iterated. If this
         * argument is not really an array, the supplied function is called once.
         * @param {Function} fn The function to be called with each item. If the
         * supplied function returns false, iteration stops and this method returns
         * the current <code>index</code>. This function is called with
         * the following arguments:
         * <div class="mdetail-params"><ul>
         * <li><code>item</code> : <i>Mixed</i>
         * <div class="sub-desc">The item at the current <code>index</code>
         * in the passed <code>array</code></div></li>
         * <li><code>index</code> : <i>Number</i>
         * <div class="sub-desc">The current index within the array</div></li>
         * <li><code>allItems</code> : <i>Array</i>
         * <div class="sub-desc">The <code>array</code> passed as the first
         * argument to <code>Ext.each</code>.</div></li>
         * </ul></div>
         * @param {Object} scope The scope (<code>this</code> reference) in which the specified function is executed.
         * Defaults to the <code>item</code> at the current <code>index</code>
         * within the passed <code>array</code>.
         * @return See description for the fn parameter.
         */
        each : function(array, fn, scope){
            if(Ext.isEmpty(array, true)){
                return;
            }
            if(!Ext.isIterable(array) || Ext.isPrimitive(array)){
                array = [array];
            }
            for(var i = 0, len = array.length; i < len; i++){
                if(fn.call(scope || array[i], array[i], i, array) === false){
                    return i;
                };
            }
        },

        /**
         * Iterates either the elements in an array, or each of the properties in an object.
         * <b>Note</b>: If you are only iterating arrays, it is better to call {@link #each}.
         * @param {Object/Array} object The object or array to be iterated
         * @param {Function} fn The function to be called for each iteration.
         * The iteration will stop if the supplied function returns false, or
         * all array elements / object properties have been covered. The signature
         * varies depending on the type of object being interated:
         * <div class="mdetail-params"><ul>
         * <li>Arrays : <tt>(Object item, Number index, Array allItems)</tt>
         * <div class="sub-desc">
         * When iterating an array, the supplied function is called with each item.</div></li>
         * <li>Objects : <tt>(String key, Object value, Object)</tt>
         * <div class="sub-desc">
         * When iterating an object, the supplied function is called with each key-value pair in
         * the object, and the iterated object</div></li>
         * </ul></div>
         * @param {Object} scope The scope (<code>this</code> reference) in which the specified function is executed. Defaults to
         * the <code>object</code> being iterated.
         */
        iterate : function(obj, fn, scope){
            if(Ext.isEmpty(obj)){
                return;
            }
            if(Ext.isIterable(obj)){
                Ext.each(obj, fn, scope);
                return;
            }else if(Ext.isObject(obj)){
                for(var prop in obj){
                    if(obj.hasOwnProperty(prop)){
                        if(fn.call(scope || obj, prop, obj[prop], obj) === false){
                            return;
                        };
                    }
                }
            }
        },

        /**
         * Return the dom node for the passed String (id), dom node, or Ext.Element.
         * Here are some examples:
         * <pre><code>
// gets dom node based on id
var elDom = Ext.getDom('elId');
// gets dom node based on the dom node
var elDom1 = Ext.getDom(elDom);

// If we don&#39;t know if we are working with an
// Ext.Element or a dom node use Ext.getDom
function(el){
    var dom = Ext.getDom(el);
    // do something with the dom node
}
         * </code></pre>
         * <b>Note</b>: the dom node to be found actually needs to exist (be rendered, etc)
         * when this method is called to be successful.
         * @param {Mixed} el
         * @return HTMLElement
         */
        getDom : function(el){
            if(!el || !DOC){
                return null;
            }
            return el.dom ? el.dom : (Ext.isString(el) ? DOC.getElementById(el) : el);
        },

        /**
         * Returns the current document body as an {@link Ext.Element}.
         * @return Ext.Element The document body
         */
        getBody : function(){
            return Ext.get(DOC.body || DOC.documentElement);
        },

        /**
         * Removes a DOM node from the document.
         */
        /**
         * <p>Removes this element from the document, removes all DOM event listeners, and deletes the cache reference.
         * All DOM event listeners are removed from this element. If {@link Ext#enableNestedListenerRemoval} is
         * <code>true</code>, then DOM event listeners are also removed from all child nodes. The body node
         * will be ignored if passed in.</p>
         * @param {HTMLElement} node The node to remove
         */
        removeNode : isIE && !isIE8 ? function(){
            var d;
            return function(n){
                if(n && n.tagName != 'BODY'){
                    (Ext.enableNestedListenerRemoval) ? Ext.EventManager.purgeElement(n, true) : Ext.EventManager.removeAll(n);
                    d = d || DOC.createElement('div');
                    d.appendChild(n);
                    d.innerHTML = '';
                    delete Ext.elCache[n.id];
                }
            }
        }() : function(n){
            if(n && n.parentNode && n.tagName != 'BODY'){
                (Ext.enableNestedListenerRemoval) ? Ext.EventManager.purgeElement(n, true) : Ext.EventManager.removeAll(n);
                n.parentNode.removeChild(n);
                delete Ext.elCache[n.id];
            }
        },

        /**
         * <p>Returns true if the passed value is empty.</p>
         * <p>The value is deemed to be empty if it is<div class="mdetail-params"><ul>
         * <li>null</li>
         * <li>undefined</li>
         * <li>an empty array</li>
         * <li>a zero length string (Unless the <tt>allowBlank</tt> parameter is <tt>true</tt>)</li>
         * </ul></div>
         * @param {Mixed} value The value to test
         * @param {Boolean} allowBlank (optional) true to allow empty strings (defaults to false)
         * @return {Boolean}
         */
        isEmpty : function(v, allowBlank){
            return v === null || v === undefined || ((Ext.isArray(v) && !v.length)) || (!allowBlank ? v === '' : false);
        },

        /**
         * Returns true if the passed value is a JavaScript array, otherwise false.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isArray : function(v){
            return toString.apply(v) === '[object Array]';
        },

        /**
         * Returns true if the passed object is a JavaScript date object, otherwise false.
         * @param {Object} object The object to test
         * @return {Boolean}
         */
        isDate : function(v){
            return toString.apply(v) === '[object Date]';
        },

        /**
         * Returns true if the passed value is a JavaScript Object, otherwise false.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isObject : function(v){
            return !!v && Object.prototype.toString.call(v) === '[object Object]';
        },

        /**
         * Returns true if the passed value is a JavaScript 'primitive', a string, number or boolean.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isPrimitive : function(v){
            return Ext.isString(v) || Ext.isNumber(v) || Ext.isBoolean(v);
        },

        /**
         * Returns true if the passed value is a JavaScript Function, otherwise false.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isFunction : function(v){
            return toString.apply(v) === '[object Function]';
        },

        /**
         * Returns true if the passed value is a number. Returns false for non-finite numbers.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isNumber : function(v){
            return typeof v === 'number' && isFinite(v);
        },

        /**
         * Returns true if the passed value is a string.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isString : function(v){
            return typeof v === 'string';
        },

        /**
         * Returns true if the passed value is a boolean.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isBoolean : function(v){
            return typeof v === 'boolean';
        },

        /**
         * Returns true if the passed value is an HTMLElement
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isElement : function(v) {
            return !!v && v.tagName;
        },

        /**
         * Returns true if the passed value is not undefined.
         * @param {Mixed} value The value to test
         * @return {Boolean}
         */
        isDefined : function(v){
            return typeof v !== 'undefined';
        },

        /**
         * True if the detected browser is Opera.
         * @type Boolean
         */
        isOpera : isOpera,
        /**
         * True if the detected browser uses WebKit.
         * @type Boolean
         */
        isWebKit : isWebKit,
        /**
         * True if the detected browser is Chrome.
         * @type Boolean
         */
        isChrome : isChrome,
        /**
         * True if the detected browser is Safari.
         * @type Boolean
         */
        isSafari : isSafari,
        /**
         * True if the detected browser is Safari 3.x.
         * @type Boolean
         */
        isSafari3 : isSafari3,
        /**
         * True if the detected browser is Safari 4.x.
         * @type Boolean
         */
        isSafari4 : isSafari4,
        /**
         * True if the detected browser is Safari 2.x.
         * @type Boolean
         */
        isSafari2 : isSafari2,
        /**
         * True if the detected browser is Internet Explorer.
         * @type Boolean
         */
        isIE : isIE,
        /**
         * True if the detected browser is Internet Explorer 6.x.
         * @type Boolean
         */
        isIE6 : isIE6,
        /**
         * True if the detected browser is Internet Explorer 7.x.
         * @type Boolean
         */
        isIE7 : isIE7,
        /**
         * True if the detected browser is Internet Explorer 8.x.
         * @type Boolean
         */
        isIE8 : isIE8,
        /**
         * True if the detected browser uses the Gecko layout engine (e.g. Mozilla, Firefox).
         * @type Boolean
         */
        isGecko : isGecko,
        /**
         * True if the detected browser uses a pre-Gecko 1.9 layout engine (e.g. Firefox 2.x).
         * @type Boolean
         */
        isGecko2 : isGecko2,
        /**
         * True if the detected browser uses a Gecko 1.9+ layout engine (e.g. Firefox 3.x).
         * @type Boolean
         */
        isGecko3 : isGecko3,
        /**
         * True if the detected browser is Internet Explorer running in non-strict mode.
         * @type Boolean
         */
        isBorderBox : isBorderBox,
        /**
         * True if the detected platform is Linux.
         * @type Boolean
         */
        isLinux : isLinux,
        /**
         * True if the detected platform is Windows.
         * @type Boolean
         */
        isWindows : isWindows,
        /**
         * True if the detected platform is Mac OS.
         * @type Boolean
         */
        isMac : isMac,
        /**
         * True if the detected platform is Adobe Air.
         * @type Boolean
         */
        isAir : isAir
    });

    /**
     * Creates namespaces to be used for scoping variables and classes so that they are not global.
     * Specifying the last node of a namespace implicitly creates all other nodes. Usage:
     * <pre><code>
Ext.namespace('Company', 'Company.data');
Ext.namespace('Company.data'); // equivalent and preferable to above syntax
Company.Widget = function() { ... }
Company.data.CustomStore = function(config) { ... }
</code></pre>
     * @param {String} namespace1
     * @param {String} namespace2
     * @param {String} etc
     * @return {Object} The namespace object. (If multiple arguments are passed, this will be the last namespace created)
     * @method ns
     */
    Ext.ns = Ext.namespace;
})();

Ext.ns("Ext.util", "Ext.lib", "Ext.data");

Ext.elCache = {};

/**
 * @class Function
 * These functions are available on every Function object (any JavaScript function).
 */
Ext.apply(Function.prototype, {
     /**
     * Creates an interceptor function. The passed function is called before the original one. If it returns false,
     * the original one is not called. The resulting function returns the results of the original function.
     * The passed function is called with the parameters of the original function. Example usage:
     * <pre><code>
var sayHi = function(name){
    alert('Hi, ' + name);
}

sayHi('Fred'); // alerts "Hi, Fred"

// create a new function that validates input without
// directly modifying the original function:
var sayHiToFriend = sayHi.createInterceptor(function(name){
    return name == 'Brian';
});

sayHiToFriend('Fred');  // no alert
sayHiToFriend('Brian'); // alerts "Hi, Brian"
</code></pre>
     * @param {Function} fcn The function to call before the original
     * @param {Object} scope (optional) The scope (<code><b>this</b></code> reference) in which the passed function is executed.
     * <b>If omitted, defaults to the scope in which the original function is called or the browser window.</b>
     * @return {Function} The new function
     */
    createInterceptor : function(fcn, scope){
        var method = this;
        return !Ext.isFunction(fcn) ?
                this :
                function() {
                    var me = this,
                        args = arguments;
                    fcn.target = me;
                    fcn.method = method;
                    return (fcn.apply(scope || me || window, args) !== false) ?
                            method.apply(me || window, args) :
                            null;
                };
    },

     /**
     * Creates a callback that passes arguments[0], arguments[1], arguments[2], ...
     * Call directly on any function. Example: <code>myFunction.createCallback(arg1, arg2)</code>
     * Will create a function that is bound to those 2 args. <b>If a specific scope is required in the
     * callback, use {@link #createDelegate} instead.</b> The function returned by createCallback always
     * executes in the window scope.
     * <p>This method is required when you want to pass arguments to a callback function.  If no arguments
     * are needed, you can simply pass a reference to the function as a callback (e.g., callback: myFn).
     * However, if you tried to pass a function with arguments (e.g., callback: myFn(arg1, arg2)) the function
     * would simply execute immediately when the code is parsed. Example usage:
     * <pre><code>
var sayHi = function(name){
    alert('Hi, ' + name);
}

// clicking the button alerts "Hi, Fred"
new Ext.Button({
    text: 'Say Hi',
    renderTo: Ext.getBody(),
    handler: sayHi.createCallback('Fred')
});
</code></pre>
     * @return {Function} The new function
    */
    createCallback : function(/*args...*/){
        // make args available, in function below
        var args = arguments,
            method = this;
        return function() {
            return method.apply(window, args);
        };
    },

    /**
     * Creates a delegate (callback) that sets the scope to obj.
     * Call directly on any function. Example: <code>this.myFunction.createDelegate(this, [arg1, arg2])</code>
     * Will create a function that is automatically scoped to obj so that the <tt>this</tt> variable inside the
     * callback points to obj. Example usage:
     * <pre><code>
var sayHi = function(name){
    // Note this use of "this.text" here.  This function expects to
    // execute within a scope that contains a text property.  In this
    // example, the "this" variable is pointing to the btn object that
    // was passed in createDelegate below.
    alert('Hi, ' + name + '. You clicked the "' + this.text + '" button.');
}

var btn = new Ext.Button({
    text: 'Say Hi',
    renderTo: Ext.getBody()
});

// This callback will execute in the scope of the
// button instance. Clicking the button alerts
// "Hi, Fred. You clicked the "Say Hi" button."
btn.on('click', sayHi.createDelegate(btn, ['Fred']));
</code></pre>
     * @param {Object} scope (optional) The scope (<code><b>this</b></code> reference) in which the function is executed.
     * <b>If omitted, defaults to the browser window.</b>
     * @param {Array} args (optional) Overrides arguments for the call. (Defaults to the arguments passed by the caller)
     * @param {Boolean/Number} appendArgs (optional) if True args are appended to call args instead of overriding,
     * if a number the args are inserted at the specified position
     * @return {Function} The new function
     */
    createDelegate : function(obj, args, appendArgs){
        var method = this;
        return function() {
            var callArgs = args || arguments;
            if (appendArgs === true){
                callArgs = Array.prototype.slice.call(arguments, 0);
                callArgs = callArgs.concat(args);
            }else if (Ext.isNumber(appendArgs)){
                callArgs = Array.prototype.slice.call(arguments, 0); // copy arguments first
                var applyArgs = [appendArgs, 0].concat(args); // create method call params
                Array.prototype.splice.apply(callArgs, applyArgs); // splice them in
            }
            return method.apply(obj || window, callArgs);
        };
    },

    /**
     * Calls this function after the number of millseconds specified, optionally in a specific scope. Example usage:
     * <pre><code>
var sayHi = function(name){
    alert('Hi, ' + name);
}

// executes immediately:
sayHi('Fred');

// executes after 2 seconds:
sayHi.defer(2000, this, ['Fred']);

// this syntax is sometimes useful for deferring
// execution of an anonymous function:
(function(){
    alert('Anonymous');
}).defer(100);
</code></pre>
     * @param {Number} millis The number of milliseconds for the setTimeout call (if less than or equal to 0 the function is executed immediately)
     * @param {Object} scope (optional) The scope (<code><b>this</b></code> reference) in which the function is executed.
     * <b>If omitted, defaults to the browser window.</b>
     * @param {Array} args (optional) Overrides arguments for the call. (Defaults to the arguments passed by the caller)
     * @param {Boolean/Number} appendArgs (optional) if True args are appended to call args instead of overriding,
     * if a number the args are inserted at the specified position
     * @return {Number} The timeout id that can be used with clearTimeout
     */
    defer : function(millis, obj, args, appendArgs){
        var fn = this.createDelegate(obj, args, appendArgs);
        if(millis > 0){
            return setTimeout(fn, millis);
        }
        fn();
        return 0;
    }
});

/**
 * @class String
 * These functions are available on every String object.
 */
Ext.applyIf(String, {
    /**
     * Allows you to define a tokenized string and pass an arbitrary number of arguments to replace the tokens.  Each
     * token must be unique, and must increment in the format {0}, {1}, etc.  Example usage:
     * <pre><code>
var cls = 'my-class', text = 'Some text';
var s = String.format('&lt;div class="{0}">{1}&lt;/div>', cls, text);
// s now contains the string: '&lt;div class="my-class">Some text&lt;/div>'
     * </code></pre>
     * @param {String} string The tokenized string to be formatted
     * @param {String} value1 The value to replace token {0}
     * @param {String} value2 Etc...
     * @return {String} The formatted string
     * @static
     */
    format : function(format){
        var args = Ext.toArray(arguments, 1);
        return format.replace(/\{(\d+)\}/g, function(m, i){
            return args[i];
        });
    }
});

/**
 * @class Array
 */
Ext.applyIf(Array.prototype, {
    /**
     * Checks whether or not the specified object exists in the array.
     * @param {Object} o The object to check for
     * @param {Number} from (Optional) The index at which to begin the search
     * @return {Number} The index of o in the array (or -1 if it is not found)
     */
    indexOf : function(o, from){
        var len = this.length;
        from = from || 0;
        from += (from < 0) ? len : 0;
        for (; from < len; ++from){
            if(this[from] === o){
                return from;
            }
        }
        return -1;
    },

    /**
     * Removes the specified object from the array.  If the object is not found nothing happens.
     * @param {Object} o The object to remove
     * @return {Array} this array
     */
    remove : function(o){
        var index = this.indexOf(o);
        if(index != -1){
            this.splice(index, 1);
        }
        return this;
    }
});
/**
 * @class Ext.util.TaskRunner
 * Provides the ability to execute one or more arbitrary tasks in a multithreaded
 * manner.  Generally, you can use the singleton {@link Ext.TaskMgr} instead, but
 * if needed, you can create separate instances of TaskRunner.  Any number of
 * separate tasks can be started at any time and will run independently of each
 * other. Example usage:
 * <pre><code>
// Start a simple clock task that updates a div once per second
var updateClock = function(){
    Ext.fly('clock').update(new Date().format('g:i:s A'));
} 
var task = {
    run: updateClock,
    interval: 1000 //1 second
}
var runner = new Ext.util.TaskRunner();
runner.start(task);

// equivalent using TaskMgr
Ext.TaskMgr.start({
    run: updateClock,
    interval: 1000
});

 * </code></pre>
 * Also see {@link Ext.util.DelayedTask}. 
 * 
 * @constructor
 * @param {Number} interval (optional) The minimum precision in milliseconds supported by this TaskRunner instance
 * (defaults to 10)
 */
Ext.util.TaskRunner = function(interval){
    interval = interval || 10;
    var tasks = [], 
    	removeQueue = [],
    	id = 0,
    	running = false,

    	// private
    	stopThread = function(){
	        running = false;
	        clearInterval(id);
	        id = 0;
	    },

    	// private
    	startThread = function(){
	        if(!running){
	            running = true;
	            id = setInterval(runTasks, interval);
	        }
	    },

    	// private
    	removeTask = function(t){
	        removeQueue.push(t);
	        if(t.onStop){
	            t.onStop.apply(t.scope || t);
	        }
	    },
	    
    	// private
    	runTasks = function(){
	    	var rqLen = removeQueue.length,
	    		now = new Date().getTime();	    			    		
	    
	        if(rqLen > 0){
	            for(var i = 0; i < rqLen; i++){
	                tasks.remove(removeQueue[i]);
	            }
	            removeQueue = [];
	            if(tasks.length < 1){
	                stopThread();
	                return;
	            }
	        }	        
	        for(var i = 0, t, itime, rt, len = tasks.length; i < len; ++i){
	            t = tasks[i];
	            itime = now - t.taskRunTime;
	            if(t.interval <= itime){
	                rt = t.run.apply(t.scope || t, t.args || [++t.taskRunCount]);
	                t.taskRunTime = now;
	                if(rt === false || t.taskRunCount === t.repeat){
	                    removeTask(t);
	                    return;
	                }
	            }
	            if(t.duration && t.duration <= (now - t.taskStartTime)){
	                removeTask(t);
	            }
	        }
	    };

    /**
     * Starts a new task.
     * @method start
     * @param {Object} task A config object that supports the following properties:<ul>
     * <li><code>run</code> : Function<div class="sub-desc">The function to execute each time the task is run. The
     * function will be called at each interval and passed the <code>args</code> argument if specified.  If a
     * particular scope is required, be sure to specify it using the <code>scope</code> argument.</div></li>
     * <li><code>interval</code> : Number<div class="sub-desc">The frequency in milliseconds with which the task
     * should be executed.</div></li>
     * <li><code>args</code> : Array<div class="sub-desc">(optional) An array of arguments to be passed to the function
     * specified by <code>run</code>.</div></li>
     * <li><code>scope</code> : Object<div class="sub-desc">(optional) The scope (<tt>this</tt> reference) in which to execute the
     * <code>run</code> function. Defaults to the task config object.</div></li>
     * <li><code>duration</code> : Number<div class="sub-desc">(optional) The length of time in milliseconds to execute
     * the task before stopping automatically (defaults to indefinite).</div></li>
     * <li><code>repeat</code> : Number<div class="sub-desc">(optional) The number of times to execute the task before
     * stopping automatically (defaults to indefinite).</div></li>
     * </ul>
     * @return {Object} The task
     */
    this.start = function(task){
        tasks.push(task);
        task.taskStartTime = new Date().getTime();
        task.taskRunTime = 0;
        task.taskRunCount = 0;
        startThread();
        return task;
    };

    /**
     * Stops an existing running task.
     * @method stop
     * @param {Object} task The task to stop
     * @return {Object} The task
     */
    this.stop = function(task){
        removeTask(task);
        return task;
    };

    /**
     * Stops all tasks that are currently running.
     * @method stopAll
     */
    this.stopAll = function(){
        stopThread();
        for(var i = 0, len = tasks.length; i < len; i++){
            if(tasks[i].onStop){
                tasks[i].onStop();
            }
        }
        tasks = [];
        removeQueue = [];
    };
};

/**
 * @class Ext.TaskMgr
 * @extends Ext.util.TaskRunner
 * A static {@link Ext.util.TaskRunner} instance that can be used to start and stop arbitrary tasks.  See
 * {@link Ext.util.TaskRunner} for supported methods and task config properties.
 * <pre><code>
// Start a simple clock task that updates a div once per second
var task = {
    run: function(){
        Ext.fly('clock').update(new Date().format('g:i:s A'));
    },
    interval: 1000 //1 second
}
Ext.TaskMgr.start(task);
</code></pre>
 * @singleton
 */
Ext.TaskMgr = new Ext.util.TaskRunner();/**
 * @class Ext.util.DelayedTask
 * <p> The DelayedTask class provides a convenient way to "buffer" the execution of a method,
 * performing setTimeout where a new timeout cancels the old timeout. When called, the
 * task will wait the specified time period before executing. If durng that time period,
 * the task is called again, the original call will be cancelled. This continues so that
 * the function is only called a single time for each iteration.</p>
 * <p>This method is especially useful for things like detecting whether a user has finished
 * typing in a text field. An example would be performing validation on a keypress. You can
 * use this class to buffer the keypress events for a certain number of milliseconds, and
 * perform only if they stop for that amount of time.  Usage:</p><pre><code>
var task = new Ext.util.DelayedTask(function(){
    alert(Ext.getDom('myInputField').value.length);
});
// Wait 500ms before calling our function. If the user presses another key 
// during that 500ms, it will be cancelled and we'll wait another 500ms.
Ext.get('myInputField').on('keypress', function(){
    task.{@link #delay}(500); 
});
 * </code></pre> 
 * <p>Note that we are using a DelayedTask here to illustrate a point. The configuration
 * option <tt>buffer</tt> for {@link Ext.util.Observable#addListener addListener/on} will
 * also setup a delayed task for you to buffer events.</p> 
 * @constructor The parameters to this constructor serve as defaults and are not required.
 * @param {Function} fn (optional) The default function to call.
 * @param {Object} scope The default scope (The <code><b>this</b></code> reference) in which the
 * function is called. If not specified, <code>this</code> will refer to the browser window.
 * @param {Array} args (optional) The default Array of arguments.
 */
Ext.util.DelayedTask = function(fn, scope, args){
    var me = this,
    	id,    	
    	call = function(){
    		clearInterval(id);
	        id = null;
	        fn.apply(scope, args || []);
	    };
	    
    /**
     * Cancels any pending timeout and queues a new one
     * @param {Number} delay The milliseconds to delay
     * @param {Function} newFn (optional) Overrides function passed to constructor
     * @param {Object} newScope (optional) Overrides scope passed to constructor. Remember that if no scope
     * is specified, <code>this</code> will refer to the browser window.
     * @param {Array} newArgs (optional) Overrides args passed to constructor
     */
    me.delay = function(delay, newFn, newScope, newArgs){
        me.cancel();
        fn = newFn || fn;
        scope = newScope || scope;
        args = newArgs || args;
        id = setInterval(call, delay);
    };

    /**
     * Cancel the last queued timeout
     */
    me.cancel = function(){
        if(id){
            clearInterval(id);
            id = null;
        }
    };
};(function(){
	var libFlyweight;
	
	function fly(el) {
        if (!libFlyweight) {
            libFlyweight = new Ext.Element.Flyweight();
        }
        libFlyweight.dom = el;
        return libFlyweight;
    }
    
    (function(){
	var doc = document,
		isCSS1 = doc.compatMode == "CSS1Compat",
		MAX = Math.max,		
        ROUND = Math.round,
		PARSEINT = parseInt;
		
	Ext.lib.Dom = {
	    isAncestor : function(p, c) {
		    var ret = false;
			
			p = Ext.getDom(p);
			c = Ext.getDom(c);
			if (p && c) {
				if (p.contains) {
					return p.contains(c);
				} else if (p.compareDocumentPosition) {
					return !!(p.compareDocumentPosition(c) & 16);
				} else {
					while (c = c.parentNode) {
						ret = c == p || ret;	        			
					}
				}	            
			}	
			return ret;
		},
		
        getViewWidth : function(full) {
            return full ? this.getDocumentWidth() : this.getViewportWidth();
        },

        getViewHeight : function(full) {
            return full ? this.getDocumentHeight() : this.getViewportHeight();
        },

        getDocumentHeight: function() {            
            return MAX(!isCSS1 ? doc.body.scrollHeight : doc.documentElement.scrollHeight, this.getViewportHeight());
        },

        getDocumentWidth: function() {            
            return MAX(!isCSS1 ? doc.body.scrollWidth : doc.documentElement.scrollWidth, this.getViewportWidth());
        },

        getViewportHeight: function(){
	        return Ext.isIE ? 
	        	   (Ext.isStrict ? doc.documentElement.clientHeight : doc.body.clientHeight) :
	        	   self.innerHeight;
        },

        getViewportWidth : function() {
	        return !Ext.isStrict && !Ext.isOpera ? doc.body.clientWidth :
	        	   Ext.isIE ? doc.documentElement.clientWidth : self.innerWidth;
        },
        
        getY : function(el) {
            return this.getXY(el)[1];
        },

        getX : function(el) {
            return this.getXY(el)[0];
        },

        getXY : function(el) {
            var p, 
            	pe, 
            	b,
            	bt, 
            	bl,     
            	dbd,       	
            	x = 0,
            	y = 0, 
            	scroll,
            	hasAbsolute, 
            	bd = (doc.body || doc.documentElement),
            	ret = [0,0];
            	
            el = Ext.getDom(el);

            if(el != bd){
	            if (el.getBoundingClientRect) {
	                b = el.getBoundingClientRect();
	                scroll = fly(document).getScroll();
	                ret = [ROUND(b.left + scroll.left), ROUND(b.top + scroll.top)];
	            } else {  
		            p = el;		
		            hasAbsolute = fly(el).isStyle("position", "absolute");
		
		            while (p) {
			            pe = fly(p);		
		                x += p.offsetLeft;
		                y += p.offsetTop;
		
		                hasAbsolute = hasAbsolute || pe.isStyle("position", "absolute");
		                		
		                if (Ext.isGecko) {		                    
		                    y += bt = PARSEINT(pe.getStyle("borderTopWidth"), 10) || 0;
		                    x += bl = PARSEINT(pe.getStyle("borderLeftWidth"), 10) || 0;	
		
		                    if (p != el && !pe.isStyle('overflow','visible')) {
		                        x += bl;
		                        y += bt;
		                    }
		                }
		                p = p.offsetParent;
		            }
		
		            if (Ext.isSafari && hasAbsolute) {
		                x -= bd.offsetLeft;
		                y -= bd.offsetTop;
		            }
		
		            if (Ext.isGecko && !hasAbsolute) {
		                dbd = fly(bd);
		                x += PARSEINT(dbd.getStyle("borderLeftWidth"), 10) || 0;
		                y += PARSEINT(dbd.getStyle("borderTopWidth"), 10) || 0;
		            }
		
		            p = el.parentNode;
		            while (p && p != bd) {
		                if (!Ext.isOpera || (p.tagName != 'TR' && !fly(p).isStyle("display", "inline"))) {
		                    x -= p.scrollLeft;
		                    y -= p.scrollTop;
		                }
		                p = p.parentNode;
		            }
		            ret = [x,y];
	            }
         	}
            return ret
        },

        setXY : function(el, xy) {
            (el = Ext.fly(el, '_setXY')).position();
            
            var pts = el.translatePoints(xy),
            	style = el.dom.style,
            	pos;            	
            
            for (pos in pts) {	            
	            if(!isNaN(pts[pos])) style[pos] = pts[pos] + "px"
            }
        },

        setX : function(el, x) {
            this.setXY(el, [x, false]);
        },

        setY : function(el, y) {
            this.setXY(el, [false, y]);
        }
    };
})();Ext.lib.Event = function() {
    var loadComplete = false,
        unloadListeners = {},
        retryCount = 0,
        onAvailStack = [],
        _interval,
        locked = false,
        win = window,
        doc = document,

        // constants
        POLL_RETRYS = 200,
        POLL_INTERVAL = 20,
        EL = 0,
        TYPE = 0,
        FN = 1,
        WFN = 2,
        OBJ = 2,
        ADJ_SCOPE = 3,
        SCROLLLEFT = 'scrollLeft',
        SCROLLTOP = 'scrollTop',
        UNLOAD = 'unload',
        MOUSEOVER = 'mouseover',
        MOUSEOUT = 'mouseout',
        // private
        doAdd = function() {
            var ret;
            if (win.addEventListener) {
                ret = function(el, eventName, fn, capture) {
                    if (eventName == 'mouseenter') {
                        fn = fn.createInterceptor(checkRelatedTarget);
                        el.addEventListener(MOUSEOVER, fn, (capture));
                    } else if (eventName == 'mouseleave') {
                        fn = fn.createInterceptor(checkRelatedTarget);
                        el.addEventListener(MOUSEOUT, fn, (capture));
                    } else {
                        el.addEventListener(eventName, fn, (capture));
                    }
                    return fn;
                };
            } else if (win.attachEvent) {
                ret = function(el, eventName, fn, capture) {
                    el.attachEvent("on" + eventName, fn);
                    return fn;
                };
            } else {
                ret = function(){};
            }
            return ret;
        }(),
        // private
        doRemove = function(){
            var ret;
            if (win.removeEventListener) {
                ret = function (el, eventName, fn, capture) {
                    if (eventName == 'mouseenter') {
                        eventName = MOUSEOVER;
                    } else if (eventName == 'mouseleave') {
                        eventName = MOUSEOUT;
                    }
                    el.removeEventListener(eventName, fn, (capture));
                };
            } else if (win.detachEvent) {
                ret = function (el, eventName, fn) {
                    el.detachEvent("on" + eventName, fn);
                };
            } else {
                ret = function(){};
            }
            return ret;
        }();

    function checkRelatedTarget(e) {
        return !elContains(e.currentTarget, pub.getRelatedTarget(e));
    }

    function elContains(parent, child) {
       if(parent && parent.firstChild){
         while(child) {
            if(child === parent) {
                return true;
            }
            child = child.parentNode;
            if(child && (child.nodeType != 1)) {
                child = null;
            }
          }
        }
        return false;
    }

    // private
    function _tryPreloadAttach() {
        var ret = false,
            notAvail = [],
            element, i, len, v,
            tryAgain = !loadComplete || (retryCount > 0);

        if (!locked) {
            locked = true;

            for (i = 0, len = onAvailStack.length; i < len; i++) {
                v = onAvailStack[i];
                if(v && (element = doc.getElementById(v.id))){
                    if(!v.checkReady || loadComplete || element.nextSibling || (doc && doc.body)) {
                        element = v.override ? (v.override === true ? v.obj : v.override) : element;
                        v.fn.call(element, v.obj);
                        onAvailStack.remove(v);
                    } else {
                        notAvail.push(v);
                    }
                }
            }

            retryCount = (notAvail.length === 0) ? 0 : retryCount - 1;

            if (tryAgain) {
                startInterval();
            } else {
                clearInterval(_interval);
                _interval = null;
            }

            ret = !(locked = false);
        }
        return ret;
    }

    // private
    function startInterval() {
        if(!_interval){
            var callback = function() {
                _tryPreloadAttach();
            };
            _interval = setInterval(callback, POLL_INTERVAL);
        }
    }

    // private
    function getScroll() {
        var dd = doc.documentElement,
            db = doc.body;
        if(dd && (dd[SCROLLTOP] || dd[SCROLLLEFT])){
            return [dd[SCROLLLEFT], dd[SCROLLTOP]];
        }else if(db){
            return [db[SCROLLLEFT], db[SCROLLTOP]];
        }else{
            return [0, 0];
        }
    }

    // private
    function getPageCoord (ev, xy) {
        ev = ev.browserEvent || ev;
        var coord  = ev['page' + xy];
        if (!coord && coord !== 0) {
            coord = ev['client' + xy] || 0;

            if (Ext.isIE) {
                coord += getScroll()[xy == "X" ? 0 : 1];
            }
        }

        return coord;
    }

    var pub =  {
        extAdapter: true,
        onAvailable : function(p_id, p_fn, p_obj, p_override) {
            onAvailStack.push({
                id:         p_id,
                fn:         p_fn,
                obj:        p_obj,
                override:   p_override,
                checkReady: false });

            retryCount = POLL_RETRYS;
            startInterval();
        },

        // This function should ALWAYS be called from Ext.EventManager
        addListener: function(el, eventName, fn) {
            el = Ext.getDom(el);
            if (el && fn) {
                if (eventName == UNLOAD) {
                    if (unloadListeners[el.id] === undefined) {
                        unloadListeners[el.id] = [];
                    }
                    unloadListeners[el.id].push([eventName, fn]);
                    return fn;
                }
                return doAdd(el, eventName, fn, false);
            }
            return false;
        },

        // This function should ALWAYS be called from Ext.EventManager
        removeListener: function(el, eventName, fn) {
            el = Ext.getDom(el);
            var i, len, li, lis;
            if (el && fn) {
                if(eventName == UNLOAD){
                    if((lis = unloadListeners[el.id]) !== undefined){
                        for(i = 0, len = lis.length; i < len; i++){
                            if((li = lis[i]) && li[TYPE] == eventName && li[FN] == fn){
                                unloadListeners[id].splice(i, 1);
                            }
                        }
                    }
                    return;
                }
                doRemove(el, eventName, fn, false);
            }
        },

        getTarget : function(ev) {
            ev = ev.browserEvent || ev;
            return this.resolveTextNode(ev.target || ev.srcElement);
        },

        resolveTextNode : Ext.isGecko ? function(node){
            if(!node){
                return;
            }
            // work around firefox bug, https://bugzilla.mozilla.org/show_bug.cgi?id=101197
            var s = HTMLElement.prototype.toString.call(node);
            if(s == '[xpconnect wrapped native prototype]' || s == '[object XULElement]'){
                return;
            }
            return node.nodeType == 3 ? node.parentNode : node;
        } : function(node){
            return node && node.nodeType == 3 ? node.parentNode : node;
        },

        getRelatedTarget : function(ev) {
            ev = ev.browserEvent || ev;
            return this.resolveTextNode(ev.relatedTarget ||
                    (ev.type == MOUSEOUT ? ev.toElement :
                     ev.type == MOUSEOVER ? ev.fromElement : null));
        },

        getPageX : function(ev) {
            return getPageCoord(ev, "X");
        },

        getPageY : function(ev) {
            return getPageCoord(ev, "Y");
        },


        getXY : function(ev) {
            return [this.getPageX(ev), this.getPageY(ev)];
        },

        stopEvent : function(ev) {
            this.stopPropagation(ev);
            this.preventDefault(ev);
        },

        stopPropagation : function(ev) {
            ev = ev.browserEvent || ev;
            if (ev.stopPropagation) {
                ev.stopPropagation();
            } else {
                ev.cancelBubble = true;
            }
        },

        preventDefault : function(ev) {
            ev = ev.browserEvent || ev;
            if (ev.preventDefault) {
                ev.preventDefault();
            } else {
                ev.returnValue = false;
            }
        },

        getEvent : function(e) {
            e = e || win.event;
            if (!e) {
                var c = this.getEvent.caller;
                while (c) {
                    e = c.arguments[0];
                    if (e && Event == e.constructor) {
                        break;
                    }
                    c = c.caller;
                }
            }
            return e;
        },

        getCharCode : function(ev) {
            ev = ev.browserEvent || ev;
            return ev.charCode || ev.keyCode || 0;
        },

        //clearCache: function() {},
        // deprecated, call from EventManager
        getListeners : function(el, eventName) {
            Ext.EventManager.getListeners(el, eventName);
        },

        // deprecated, call from EventManager
        purgeElement : function(el, recurse, eventName) {
            Ext.EventManager.purgeElement(el, recurse, eventName);
        },

        _load : function(e) {
            loadComplete = true;
            var EU = Ext.lib.Event;
            if (Ext.isIE && e !== true) {
        // IE8 complains that _load is null or not an object
        // so lets remove self via arguments.callee
                doRemove(win, "load", arguments.callee);
            }
        },

        _unload : function(e) {
             var EU = Ext.lib.Event,
                i, j, l, v, ul, id, len, index, scope;


            for (id in unloadListeners) {
                ul = unloadListeners[id];
                for (i = 0, len = ul.length; i < len; i++) {
                    v = ul[i];
                    if (v) {
                        try{
                            scope = v[ADJ_SCOPE] ? (v[ADJ_SCOPE] === true ? v[OBJ] : v[ADJ_SCOPE]) :  win;
                            v[FN].call(scope, EU.getEvent(e), v[OBJ]);
                        }catch(ex){}
                    }
                }
            };

            unloadListeners = null;
            Ext.EventManager._unload();

            doRemove(win, UNLOAD, EU._unload);
        }
    };

    // Initialize stuff.
    pub.on = pub.addListener;
    pub.un = pub.removeListener;
    if (doc && doc.body) {
        pub._load(true);
    } else {
        doAdd(win, "load", pub._load);
    }
    doAdd(win, UNLOAD, pub._unload);
    _tryPreloadAttach();

    return pub;
}();
/*
* Portions of this file are based on pieces of Yahoo User Interface Library
* Copyright (c) 2007, Yahoo! Inc. All rights reserved.
* YUI licensed under the BSD License:
* http://developer.yahoo.net/yui/license.txt
*/
Ext.lib.Ajax = function() {     
    var activeX = ['MSXML2.XMLHTTP.3.0',
                   'MSXML2.XMLHTTP',
                   'Microsoft.XMLHTTP'],
        CONTENTTYPE = 'Content-Type';
                   
    // private
    function setHeader(o) {
        var conn = o.conn,
            prop;
        
        function setTheHeaders(conn, headers){
            for (prop in headers) {
                if (headers.hasOwnProperty(prop)) {
                    conn.setRequestHeader(prop, headers[prop]);
                }
            }   
        }       
        
        if (pub.defaultHeaders) {
            setTheHeaders(conn, pub.defaultHeaders);
        }

        if (pub.headers) {
            setTheHeaders(conn, pub.headers);
            delete pub.headers;                
        }
    }    
    
    // private
    function createExceptionObject(tId, callbackArg, isAbort, isTimeout) {          
        return {
            tId : tId,
            status : isAbort ? -1 : 0,
            statusText : isAbort ? 'transaction aborted' : 'communication failure',
            isAbort: isAbort,
            isTimeout: isTimeout,
            argument : callbackArg
        };
    }  
    
    // private 
    function initHeader(label, value) {         
        (pub.headers = pub.headers || {})[label] = value;                       
    }
    
    // private
    function createResponseObject(o, callbackArg) {
        var headerObj = {},
            headerStr,              
            conn = o.conn,
            t,
            s;

        try {
            headerStr = o.conn.getAllResponseHeaders();   
            Ext.each(headerStr.replace(/\r\n/g, '\n').split('\n'), function(v){
                t = v.indexOf(':');
                if(t >= 0){
                    s = v.substr(0, t).toLowerCase();
                    if(v.charAt(t + 1) == ' '){
                        ++t;
                    }
                    headerObj[s] = v.substr(t + 1);
                }
            });
        } catch(e) {}
                    
        return {
            tId : o.tId,
            status : conn.status,
            statusText : conn.statusText,
            getResponseHeader : function(header){return headerObj[header.toLowerCase()];},
            getAllResponseHeaders : function(){return headerStr},
            responseText : conn.responseText,
            responseXML : conn.responseXML,
            argument : callbackArg
        };
    }
    
    // private
    function releaseObject(o) {
        o.conn = null;
        o = null;
    }        
    
    // private
    function handleTransactionResponse(o, callback, isAbort, isTimeout) {
        if (!callback) {
            releaseObject(o);
            return;
        }

        var httpStatus, responseObject;

        try {
            if (o.conn.status !== undefined && o.conn.status != 0) {
                httpStatus = o.conn.status;
            }
            else {
                httpStatus = 13030;
            }
        }
        catch(e) {
            httpStatus = 13030;
        }

        if ((httpStatus >= 200 && httpStatus < 300) || (Ext.isIE && httpStatus == 1223)) {
            responseObject = createResponseObject(o, callback.argument);
            if (callback.success) {
                if (!callback.scope) {
                    callback.success(responseObject);
                }
                else {
                    callback.success.apply(callback.scope, [responseObject]);
                }
            }
        }
        else {
            switch (httpStatus) {
                case 12002:
                case 12029:
                case 12030:
                case 12031:
                case 12152:
                case 13030:
                    responseObject = createExceptionObject(o.tId, callback.argument, (isAbort ? isAbort : false), isTimeout);
                    if (callback.failure) {
                        if (!callback.scope) {
                            callback.failure(responseObject);
                        }
                        else {
                            callback.failure.apply(callback.scope, [responseObject]);
                        }
                    }
                    break;
                default:
                    responseObject = createResponseObject(o, callback.argument);
                    if (callback.failure) {
                        if (!callback.scope) {
                            callback.failure(responseObject);
                        }
                        else {
                            callback.failure.apply(callback.scope, [responseObject]);
                        }
                    }
            }
        }

        releaseObject(o);
        responseObject = null;
    }  
    
    // private
    function handleReadyState(o, callback){
    callback = callback || {};
        var conn = o.conn,
            tId = o.tId,
            poll = pub.poll,
            cbTimeout = callback.timeout || null;

        if (cbTimeout) {
            pub.timeout[tId] = setTimeout(function() {
                pub.abort(o, callback, true);
            }, cbTimeout);
        }

        poll[tId] = setInterval(
            function() {
                if (conn && conn.readyState == 4) {
                    clearInterval(poll[tId]);
                    poll[tId] = null;

                    if (cbTimeout) {
                        clearTimeout(pub.timeout[tId]);
                        pub.timeout[tId] = null;
                    }

                    handleTransactionResponse(o, callback);
                }
            },
            pub.pollInterval);
    }
    
    // private
    function asyncRequest(method, uri, callback, postData) {
        var o = getConnectionObject() || null;

        if (o) {
            o.conn.open(method, uri, true);

            if (pub.useDefaultXhrHeader) {                    
                initHeader('X-Requested-With', pub.defaultXhrHeader);
            }

            if(postData && pub.useDefaultHeader && (!pub.headers || !pub.headers[CONTENTTYPE])){
                initHeader(CONTENTTYPE, pub.defaultPostHeader);
            }

            if (pub.defaultHeaders || pub.headers) {
                setHeader(o);
            }

            handleReadyState(o, callback);
            o.conn.send(postData || null);
        }
        return o;
    }
    
    // private
    function getConnectionObject() {
        var o;          

        try {
            if (o = createXhrObject(pub.transactionId)) {
                pub.transactionId++;
            }
        } catch(e) {
        } finally {
            return o;
        }
    }
       
    // private
    function createXhrObject(transactionId) {
        var http;
            
        try {
            http = new XMLHttpRequest();                
        } catch(e) {
            for (var i = 0; i < activeX.length; ++i) {              
                try {
                    http = new ActiveXObject(activeX[i]);                        
                    break;
                } catch(e) {}
            }
        } finally {
            return {conn : http, tId : transactionId};
        }
    }
         
    var pub = {
        request : function(method, uri, cb, data, options) {
            if(options){
                var me = this,              
                    xmlData = options.xmlData,
                    jsonData = options.jsonData,
                    hs;
                    
                Ext.applyIf(me, options);           
                
                if(xmlData || jsonData){
                    hs = me.headers;
                    if(!hs || !hs[CONTENTTYPE]){
                        initHeader(CONTENTTYPE, xmlData ? 'text/xml' : 'application/json');
                    }
                    data = xmlData || (!Ext.isPrimitive(jsonData) ? Ext.encode(jsonData) : jsonData);
                }
            }                       
            return asyncRequest(method || options.method || "POST", uri, cb, data);
        },

        serializeForm : function(form) {
            var fElements = form.elements || (document.forms[form] || Ext.getDom(form)).elements,
                hasSubmit = false,
                encoder = encodeURIComponent,
                element,
                options, 
                name, 
                val,                
                data = '',
                type;
                
            Ext.each(fElements, function(element) {                 
                name = element.name;                 
                type = element.type;
                
                if (!element.disabled && name){
                    if(/select-(one|multiple)/i.test(type)) {
                        Ext.each(element.options, function(opt) {
                            if (opt.selected) {
                                data += String.format("{0}={1}&", encoder(name), encoder((opt.hasAttribute ? opt.hasAttribute('value') : opt.getAttribute('value') !== null) ? opt.value : opt.text));
                            }                               
                        });
                    } else if(!/file|undefined|reset|button/i.test(type)) {
                            if(!(/radio|checkbox/i.test(type) && !element.checked) && !(type == 'submit' && hasSubmit)){
                                
                                data += encoder(name) + '=' + encoder(element.value) + '&';                     
                                hasSubmit = /submit/i.test(type);    
                            }                       
                    } 
                }
            });            
            return data.substr(0, data.length - 1);
        },
        
        useDefaultHeader : true,
        defaultPostHeader : 'application/x-www-form-urlencoded; charset=UTF-8',
        useDefaultXhrHeader : true,
        defaultXhrHeader : 'XMLHttpRequest',        
        poll : {},
        timeout : {},
        pollInterval : 50,
        transactionId : 0,
        
//  This is never called - Is it worth exposing this?               
//          setProgId : function(id) {
//              activeX.unshift(id);
//          },

//  This is never called - Is it worth exposing this?   
//          setDefaultPostHeader : function(b) {
//              this.useDefaultHeader = b;
//          },
        
//  This is never called - Is it worth exposing this?   
//          setDefaultXhrHeader : function(b) {
//              this.useDefaultXhrHeader = b;
//          },

//  This is never called - Is it worth exposing this?           
//          setPollingInterval : function(i) {
//              if (typeof i == 'number' && isFinite(i)) {
//                  this.pollInterval = i;
//              }
//          },
        
//  This is never called - Is it worth exposing this?
//          resetDefaultHeaders : function() {
//              this.defaultHeaders = null;
//          },
    
            abort : function(o, callback, isTimeout) {
                var me = this,
                    tId = o.tId,
                    isAbort = false;
                
                if (me.isCallInProgress(o)) {
                    o.conn.abort();
                    clearInterval(me.poll[tId]);
                    me.poll[tId] = null;
                    clearTimeout(pub.timeout[tId]);
                    me.timeout[tId] = null;
                    
                    handleTransactionResponse(o, callback, (isAbort = true), isTimeout);                
                }
                return isAbort;
            },
    
            isCallInProgress : function(o) {
                // if there is a connection and readyState is not 0 or 4
                return o.conn && !{0:true,4:true}[o.conn.readyState];           
            }
        };
        return pub;
    }();(function(){    
    var EXTLIB = Ext.lib,
        noNegatives = /width|height|opacity|padding/i,
        offsetAttribute = /^((width|height)|(top|left))$/,
        defaultUnit = /width|height|top$|bottom$|left$|right$/i,
        offsetUnit =  /\d+(em|%|en|ex|pt|in|cm|mm|pc)$/i,
        isset = function(v){
            return typeof v !== 'undefined';
        },
        now = function(){
            return new Date();    
        };
        
    EXTLIB.Anim = {
        motion : function(el, args, duration, easing, cb, scope) {
            return this.run(el, args, duration, easing, cb, scope, Ext.lib.Motion);
        },

        run : function(el, args, duration, easing, cb, scope, type) {
            type = type || Ext.lib.AnimBase;
            if (typeof easing == "string") {
                easing = Ext.lib.Easing[easing];
            }
            var anim = new type(el, args, duration, easing);
            anim.animateX(function() {
                if(Ext.isFunction(cb)){
                    cb.call(scope);
                }
            });
            return anim;
        }
    };
    
    EXTLIB.AnimBase = function(el, attributes, duration, method) {
        if (el) {
            this.init(el, attributes, duration, method);
        }
    };

    EXTLIB.AnimBase.prototype = {
        doMethod: function(attr, start, end) {
            var me = this;
            return me.method(me.curFrame, start, end - start, me.totalFrames);
        },


        setAttr: function(attr, val, unit) {
            if (noNegatives.test(attr) && val < 0) {
                val = 0;
            }
            Ext.fly(this.el, '_anim').setStyle(attr, val + unit);
        },


        getAttr: function(attr) {
            var el = Ext.fly(this.el),
                val = el.getStyle(attr),
                a = offsetAttribute.exec(attr) || []

            if (val !== 'auto' && !offsetUnit.test(val)) {
                return parseFloat(val);
            }

            return (!!(a[2]) || (el.getStyle('position') == 'absolute' && !!(a[3]))) ? el.dom['offset' + a[0].charAt(0).toUpperCase() + a[0].substr(1)] : 0;
        },


        getDefaultUnit: function(attr) {
            return defaultUnit.test(attr) ? 'px' : '';
        },

        animateX : function(callback, scope) {
            var me = this,
                f = function() {
                me.onComplete.removeListener(f);
                if (Ext.isFunction(callback)) {
                    callback.call(scope || me, me);
                }
            };
            me.onComplete.addListener(f, me);
            me.animate();
        },


        setRunAttr: function(attr) {            
            var me = this,
                a = this.attributes[attr],
                to = a.to,
                by = a.by,
                from = a.from,
                unit = a.unit,
                ra = (this.runAttrs[attr] = {}),
                end;

            if (!isset(to) && !isset(by)){
                return false;
            }

            var start = isset(from) ? from : me.getAttr(attr);
            if (isset(to)) {
                end = to;
            }else if(isset(by)) {
                if (Ext.isArray(start)){
                    end = [];
					for(var i=0,len=start.length; i<len; i++) {
						end[i] = start[i] + by[i];
					}
                }else{
                    end = start + by;
                }
            }

            Ext.apply(ra, {
                start: start,
                end: end,
                unit: isset(unit) ? unit : me.getDefaultUnit(attr)
            });
        },


        init: function(el, attributes, duration, method) {
            var me = this,
                actualFrames = 0,
                mgr = EXTLIB.AnimMgr;
                
            Ext.apply(me, {
                isAnimated: false,
                startTime: null,
                el: Ext.getDom(el),
                attributes: attributes || {},
                duration: duration || 1,
                method: method || EXTLIB.Easing.easeNone,
                useSec: true,
                curFrame: 0,
                totalFrames: mgr.fps,
                runAttrs: {},
                animate: function(){
                    var me = this,
                        d = me.duration;
                    
                    if(me.isAnimated){
                        return false;
                    }

                    me.curFrame = 0;
                    me.totalFrames = me.useSec ? Math.ceil(mgr.fps * d) : d;
                    mgr.registerElement(me); 
                },
                
                stop: function(finish){
                    var me = this;
                
                    if(finish){
                        me.curFrame = me.totalFrames;
                        me._onTween.fire();
                    }
                    mgr.stop(me);
                }
            });

            var onStart = function(){
                var me = this,
                    attr;
                
                me.onStart.fire();
                me.runAttrs = {};
                for(attr in this.attributes){
                    this.setRunAttr(attr);
                }

                me.isAnimated = true;
                me.startTime = now();
                actualFrames = 0;
            };


            var onTween = function(){
                var me = this;

                me.onTween.fire({
                    duration: now() - me.startTime,
                    curFrame: me.curFrame
                });

                var ra = me.runAttrs;
                for (var attr in ra) {
                    this.setAttr(attr, me.doMethod(attr, ra[attr].start, ra[attr].end), ra[attr].unit);
                }

                ++actualFrames;
            };

            var onComplete = function() {
                var me = this,
                    actual = (now() - me.startTime) / 1000,
                    data = {
                        duration: actual,
                        frames: actualFrames,
                        fps: actualFrames / actual
                    };

                me.isAnimated = false;
                actualFrames = 0;
                me.onComplete.fire(data);
            };

            me.onStart = new Ext.util.Event(me);
            me.onTween = new Ext.util.Event(me);            
            me.onComplete = new Ext.util.Event(me);
            (me._onStart = new Ext.util.Event(me)).addListener(onStart);
            (me._onTween = new Ext.util.Event(me)).addListener(onTween);
            (me._onComplete = new Ext.util.Event(me)).addListener(onComplete); 
        }
    };


    Ext.lib.AnimMgr = new function() {
        var me = this,
            thread = null,
            queue = [],
            tweenCount = 0;


        Ext.apply(me, {
            fps: 1000,
            delay: 1,
            registerElement: function(tween){
                queue.push(tween);
                ++tweenCount;
                tween._onStart.fire();
                me.start();
            },
            
            unRegister: function(tween, index){
                tween._onComplete.fire();
                index = index || getIndex(tween);
                if (index != -1) {
                    queue.splice(index, 1);
                }

                if (--tweenCount <= 0) {
                    me.stop();
                }
            },
            
            start: function(){
                if(thread === null){
                    thread = setInterval(me.run, me.delay);
                }
            },
            
            stop: function(tween){
                if(!tween){
                    clearInterval(thread);
                    for(var i = 0, len = queue.length; i < len; ++i){
                        if(queue[0].isAnimated){
                            me.unRegister(queue[0], 0);
                        }
                    }

                    queue = [];
                    thread = null;
                    tweenCount = 0;
                }else{
                    me.unRegister(tween);
                }
            },
            
            run: function(){
                var tf, i, len, tween;
                for(i = 0, len = queue.length; i<len; i++) {
                    tween = queue[i];
                    if(tween && tween.isAnimated){
                        tf = tween.totalFrames;
                        if(tween.curFrame < tf || tf === null){
                            ++tween.curFrame;
                            if(tween.useSec){
                                correctFrame(tween);
                            }
                            tween._onTween.fire();
                        }else{
                            me.stop(tween);
                        }
                    }                   
                }
            }
        });

        var getIndex = function(anim) {
            var i, len;
            for(i = 0, len = queue.length; i<len; i++) {
                if(queue[i] === anim) {
                    return i;
                }
            }
            return -1;
        };

        var correctFrame = function(tween) {
            var frames = tween.totalFrames,
                frame = tween.curFrame,
                duration = tween.duration,
                expected = (frame * duration * 1000 / frames),
                elapsed = (now() - tween.startTime),
                tweak = 0;

            if(elapsed < duration * 1000){
                tweak = Math.round((elapsed / expected - 1) * frame);
            }else{
                tweak = frames - (frame + 1);
            }
            if(tweak > 0 && isFinite(tweak)){
                if(tween.curFrame + tweak >= frames){
                    tweak = frames - (frame + 1);
                }
                tween.curFrame += tweak;
            }
        };
    };

    EXTLIB.Bezier = new function() {

        this.getPosition = function(points, t) {
            var n = points.length,
                tmp = [],
                c = 1 - t, 
                i,
                j;

            for (i = 0; i < n; ++i) {
                tmp[i] = [points[i][0], points[i][1]];
            }

            for (j = 1; j < n; ++j) {
                for (i = 0; i < n - j; ++i) {
                    tmp[i][0] = c * tmp[i][0] + t * tmp[parseInt(i + 1, 10)][0];
                    tmp[i][1] = c * tmp[i][1] + t * tmp[parseInt(i + 1, 10)][1];
                }
            }

            return [ tmp[0][0], tmp[0][1] ];

        };
    };


    EXTLIB.Easing = {
        easeNone: function (t, b, c, d) {
            return c * t / d + b;
        },


        easeIn: function (t, b, c, d) {
            return c * (t /= d) * t + b;
        },


        easeOut: function (t, b, c, d) {
            return -c * (t /= d) * (t - 2) + b;
        }
    };

    (function() {
        EXTLIB.Motion = function(el, attributes, duration, method) {
            if (el) {
                EXTLIB.Motion.superclass.constructor.call(this, el, attributes, duration, method);
            }
        };

        Ext.extend(EXTLIB.Motion, Ext.lib.AnimBase);

        var superclass = EXTLIB.Motion.superclass,
            proto = EXTLIB.Motion.prototype,
            pointsRe = /^points$/i;

        Ext.apply(EXTLIB.Motion.prototype, {
            setAttr: function(attr, val, unit){
                var me = this,
                    setAttr = superclass.setAttr;
                    
                if (pointsRe.test(attr)) {
                    unit = unit || 'px';
                    setAttr.call(me, 'left', val[0], unit);
                    setAttr.call(me, 'top', val[1], unit);
                } else {
                    setAttr.call(me, attr, val, unit);
                }
            },
            
            getAttr: function(attr){
                var me = this,
                    getAttr = superclass.getAttr;
                    
                return pointsRe.test(attr) ? [getAttr.call(me, 'left'), getAttr.call(me, 'top')] : getAttr.call(me, attr);
            },
            
            doMethod: function(attr, start, end){
                var me = this;
                
                return pointsRe.test(attr)
                        ? EXTLIB.Bezier.getPosition(me.runAttrs[attr], me.method(me.curFrame, 0, 100, me.totalFrames) / 100)
                        : superclass.doMethod.call(me, attr, start, end);
            },
            
            setRunAttr: function(attr){
                if(pointsRe.test(attr)){
                    
                    var me = this,
                        el = this.el,
                        points = this.attributes.points,
                        control = points.control || [],
                        from = points.from,
                        to = points.to,
                        by = points.by,
                        DOM = EXTLIB.Dom,
                        start,
                        i,
                        end,
                        len,
                        ra;
                  

                    if(control.length > 0 && !Ext.isArray(control[0])){
                        control = [control];
                    }else{
                        /*
                        var tmp = [];
                        for (i = 0,len = control.length; i < len; ++i) {
                            tmp[i] = control[i];
                        }
                        control = tmp;
                        */
                    }

                    Ext.fly(el, '_anim').position();
                    DOM.setXY(el, isset(from) ? from : DOM.getXY(el));
                    start = me.getAttr('points');


                    if(isset(to)){
                        end = translateValues.call(me, to, start);
                        for (i = 0,len = control.length; i < len; ++i) {
                            control[i] = translateValues.call(me, control[i], start);
                        }
                    } else if (isset(by)) {
                        end = [start[0] + by[0], start[1] + by[1]];

                        for (i = 0,len = control.length; i < len; ++i) {
                            control[i] = [ start[0] + control[i][0], start[1] + control[i][1] ];
                        }
                    }

                    ra = this.runAttrs[attr] = [start];
                    if (control.length > 0) {
                        ra = ra.concat(control);
                    }

                    ra[ra.length] = end;
                }else{
                    superclass.setRunAttr.call(this, attr);
                }
            }
        });

        var translateValues = function(val, start) {
            var pageXY = EXTLIB.Dom.getXY(this.el);
            return [val[0] - pageXY[0] + start[0], val[1] - pageXY[1] + start[1]];
        };
    })();
})();// Easing functions
(function(){
	// shortcuts to aid compression
	var abs = Math.abs,
	 	pi = Math.PI,
	 	asin = Math.asin,
	 	pow = Math.pow,
	 	sin = Math.sin,
		EXTLIB = Ext.lib;
	 	
    Ext.apply(EXTLIB.Easing, {
        
        easeBoth: function (t, b, c, d) {
	        return ((t /= d / 2) < 1)  ?  c / 2 * t * t + b  :  -c / 2 * ((--t) * (t - 2) - 1) + b;               
        },
        
        easeInStrong: function (t, b, c, d) {
            return c * (t /= d) * t * t * t + b;
        },

        easeOutStrong: function (t, b, c, d) {
            return -c * ((t = t / d - 1) * t * t * t - 1) + b;
        },

        easeBothStrong: function (t, b, c, d) {
            return ((t /= d / 2) < 1)  ?  c / 2 * t * t * t * t + b  :  -c / 2 * ((t -= 2) * t * t * t - 2) + b;
        },

        elasticIn: function (t, b, c, d, a, p) {
	        if (t == 0 || (t /= d) == 1) {
                return t == 0 ? b : b + c;
            }	            
            p = p || (d * .3);	            

			var s;
			if (a >= abs(c)) {
				s = p / (2 * pi) * asin(c / a);
			} else {
				a = c;
				s = p / 4;
			}
	
            return -(a * pow(2, 10 * (t -= 1)) * sin((t * d - s) * (2 * pi) / p)) + b;
            	      
        }, 	
	
		elasticOut: function (t, b, c, d, a, p) {
	        if (t == 0 || (t /= d) == 1) {
                return t == 0 ? b : b + c;
            }	            
            p = p || (d * .3);	            

			var s;
			if (a >= abs(c)) {
				s = p / (2 * pi) * asin(c / a);
			} else {
				a = c;
				s = p / 4;
			}
	
            return a * pow(2, -10 * t) * sin((t * d - s) * (2 * pi) / p) + c + b;	 
        }, 	
	
        elasticBoth: function (t, b, c, d, a, p) {
            if (t == 0 || (t /= d / 2) == 2) {
                return t == 0 ? b : b + c;
            }		         	
	            
            p = p || (d * (.3 * 1.5)); 	            

            var s;
            if (a >= abs(c)) {
	            s = p / (2 * pi) * asin(c / a);
            } else {
	            a = c;
                s = p / 4;
            }

            return t < 1 ?
            	   	-.5 * (a * pow(2, 10 * (t -= 1)) * sin((t * d - s) * (2 * pi) / p)) + b :
                    a * pow(2, -10 * (t -= 1)) * sin((t * d - s) * (2 * pi) / p) * .5 + c + b;
        },

        backIn: function (t, b, c, d, s) {
            s = s ||  1.70158; 	            
            return c * (t /= d) * t * ((s + 1) * t - s) + b;
        },


        backOut: function (t, b, c, d, s) {
            if (!s) {
                s = 1.70158;
            }
            return c * ((t = t / d - 1) * t * ((s + 1) * t + s) + 1) + b;
        },


        backBoth: function (t, b, c, d, s) {
            s = s || 1.70158; 	            

            return ((t /= d / 2 ) < 1) ?
                    c / 2 * (t * t * (((s *= (1.525)) + 1) * t - s)) + b : 	            
            		c / 2 * ((t -= 2) * t * (((s *= (1.525)) + 1) * t + s) + 2) + b;
        },


        bounceIn: function (t, b, c, d) {
            return c - EXTLIB.Easing.bounceOut(d - t, 0, c, d) + b;
        },


        bounceOut: function (t, b, c, d) {
        if ((t /= d) < (1 / 2.75)) {
                return c * (7.5625 * t * t) + b;
            } else if (t < (2 / 2.75)) {
                return c * (7.5625 * (t -= (1.5 / 2.75)) * t + .75) + b;
            } else if (t < (2.5 / 2.75)) {
                return c * (7.5625 * (t -= (2.25 / 2.75)) * t + .9375) + b;
            }
            return c * (7.5625 * (t -= (2.625 / 2.75)) * t + .984375) + b;
        },


        bounceBoth: function (t, b, c, d) {
            return (t < d / 2) ?
                   EXTLIB.Easing.bounceIn(t * 2, 0, c, d) * .5 + b : 
            	   EXTLIB.Easing.bounceOut(t * 2 - d, 0, c, d) * .5 + c * .5 + b;
        }
    });
})();

(function() {
    var EXTLIB = Ext.lib;
	// Color Animation
	EXTLIB.Anim.color = function(el, args, duration, easing, cb, scope) {
	    return EXTLIB.Anim.run(el, args, duration, easing, cb, scope, EXTLIB.ColorAnim);
	}
	
    EXTLIB.ColorAnim = function(el, attributes, duration, method) {
        EXTLIB.ColorAnim.superclass.constructor.call(this, el, attributes, duration, method);
    };

    Ext.extend(EXTLIB.ColorAnim, EXTLIB.AnimBase);

    var superclass = EXTLIB.ColorAnim.superclass,
    	colorRE = /color$/i,
    	transparentRE = /^transparent|rgba\(0, 0, 0, 0\)$/,
        rgbRE = /^rgb\(([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\)$/i,
        hexRE= /^#?([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})$/i,
        hex3RE = /^#?([0-9A-F]{1})([0-9A-F]{1})([0-9A-F]{1})$/i,
        isset = function(v){
            return typeof v !== 'undefined';
        }
         	
   	// private	
    function parseColor(s) {	
        var pi = parseInt,
            base,
            out = null,
            c;
        
	    if (s.length == 3) {
            return s;
        }
		
        Ext.each([hexRE, rgbRE, hex3RE], function(re, idx){
            base = (idx % 2 == 0) ? 16 : 10;
            c = re.exec(s);
            if(c && c.length == 4){
                out = [pi(c[1], base), pi(c[2], base), pi(c[3], base)];
                return false;
            }
        });
        return out;
    }	

    Ext.apply(EXTLIB.ColorAnim.prototype, {
        getAttr : function(attr) {
            var me = this,
                el = me.el,
                val;                
            if(colorRE.test(attr)){
                while(el && transparentRE.test(val = Ext.fly(el).getStyle(attr))){
                    el = el.parentNode;
                    val = "fff";
                }
            }else{
                val = superclass.getAttr.call(me, attr);
            }
            return val;
        },

        doMethod : function(attr, start, end) {
            var me = this,
            	val,
            	floor = Math.floor,
				i, 
                len,
                v;            

            if(colorRE.test(attr)){
                val = [];
				
				for(i = 0, len = start.length; i < len; i++) {
					v = start[i];
					val[i] = superclass.doMethod.call(me, attr, v, end[i]);
				}
                val = 'rgb(' + floor(val[0]) + ',' + floor(val[1]) + ',' + floor(val[2]) + ')';
            }else{
                val = superclass.doMethod.call(me, attr, start, end);
            }
            return val;
        },

        setRunAttr : function(attr) {
            var me = this,
                a = me.attributes[attr],
                to = a.to,
                by = a.by,
                ra;
                
            superclass.setRunAttr.call(me, attr);
            ra = me.runAttrs[attr];
            if(colorRE.test(attr)){
                var start = parseColor(ra.start),
                    end = parseColor(ra.end);

                if(!isset(to) && isset(by)){
                    end = parseColor(by);
					for(var i=0,len=start.length; i<len; i++) {
						end[i] = start[i] + end[i];
					}
                }
                ra.start = start;
                ra.end = end;
            }
        }
	});
})();	

	
(function() {
	    // Scroll Animation	
    var EXTLIB = Ext.lib;
	EXTLIB.Anim.scroll = function(el, args, duration, easing, cb, scope) {	        
	    return EXTLIB.Anim.run(el, args, duration, easing, cb, scope, EXTLIB.Scroll);
	}
	
    EXTLIB.Scroll = function(el, attributes, duration, method) {
        if(el){
            EXTLIB.Scroll.superclass.constructor.call(this, el, attributes, duration, method);
        }
    };

    Ext.extend(EXTLIB.Scroll, EXTLIB.ColorAnim);

    var superclass = EXTLIB.Scroll.superclass,
    	SCROLL = 'scroll';

    Ext.apply(EXTLIB.Scroll.prototype, {

        doMethod : function(attr, start, end) {
            var val,
            	me = this,
            	curFrame = me.curFrame,
            	totalFrames = me.totalFrames;

            if(attr == SCROLL){
                val = [me.method(curFrame, start[0], end[0] - start[0], totalFrames),
                       me.method(curFrame, start[1], end[1] - start[1], totalFrames)];
            }else{
                val = superclass.doMethod.call(me, attr, start, end);
            }
            return val;
        },

        getAttr : function(attr) {
            var me = this;

            if (attr == SCROLL) {
                return [me.el.scrollLeft, me.el.scrollTop];
            }else{
                return superclass.getAttr.call(me, attr);
            }
        },

        setAttr : function(attr, val, unit) {
            var me = this;

            if(attr == SCROLL){
                me.el.scrollLeft = val[0];
                me.el.scrollTop = val[1];
            }else{
                superclass.setAttr.call(me, attr, val, unit);
            }
        }
    });
})();	
	if(Ext.isIE) {
        function fnCleanUp() {
            var p = Function.prototype;
            delete p.createSequence;
            delete p.defer;
            delete p.createDelegate;
            delete p.createCallback;
            delete p.createInterceptor;

            window.detachEvent("onunload", fnCleanUp);
        }
        window.attachEvent("onunload", fnCleanUp);
    }
})();(function(){

var EXTUTIL = Ext.util,
    TOARRAY = Ext.toArray,
    EACH = Ext.each,
    ISOBJECT = Ext.isObject,
    TRUE = true,
    FALSE = false;
/**
 * @class Ext.util.Observable
 * Base class that provides a common interface for publishing events. Subclasses are expected to
 * to have a property "events" with all the events defined, and, optionally, a property "listeners"
 * with configured listeners defined.<br>
 * For example:
 * <pre><code>
Employee = Ext.extend(Ext.util.Observable, {
    constructor: function(config){
        this.name = config.name;
        this.addEvents({
            "fired" : true,
            "quit" : true
        });

        // Copy configured listeners into *this* object so that the base class&#39;s
        // constructor will add them.
        this.listeners = config.listeners;

        // Call our superclass constructor to complete construction process.
        Employee.superclass.constructor.call(config)
    }
});
</code></pre>
 * This could then be used like this:<pre><code>
var newEmployee = new Employee({
    name: employeeName,
    listeners: {
        quit: function() {
            // By default, "this" will be the object that fired the event.
            alert(this.name + " has quit!");
        }
    }
});
</code></pre>
 */
EXTUTIL.Observable = function(){
    /**
     * @cfg {Object} listeners (optional) <p>A config object containing one or more event handlers to be added to this
     * object during initialization.  This should be a valid listeners config object as specified in the
     * {@link #addListener} example for attaching multiple handlers at once.</p>
     * <br><p><b><u>DOM events from ExtJs {@link Ext.Component Components}</u></b></p>
     * <br><p>While <i>some</i> ExtJs Component classes export selected DOM events (e.g. "click", "mouseover" etc), this
     * is usually only done when extra value can be added. For example the {@link Ext.DataView DataView}'s
     * <b><code>{@link Ext.DataView#click click}</code></b> event passing the node clicked on. To access DOM
     * events directly from a Component's HTMLElement, listeners must be added to the <i>{@link Ext.Component#getEl Element}</i> after the Component
     * has been rendered. A plugin can simplify this step:<pre><code>
// Plugin is configured with a listeners config object.
// The Component is appended to the argument list of all handler functions.
Ext.DomObserver = Ext.extend(Object, {
    constructor: function(config) {
        this.listeners = config.listeners ? config.listeners : config;
    },

    // Component passes itself into plugin&#39;s init method
    init: function(c) {
        var p, l = this.listeners;
        for (p in l) {
            if (Ext.isFunction(l[p])) {
                l[p] = this.createHandler(l[p], c);
            } else {
                l[p].fn = this.createHandler(l[p].fn, c);
            }
        }

        // Add the listeners to the Element immediately following the render call
        c.render = c.render.{@link Function#createSequence createSequence}(function() {
            var e = c.getEl();
            if (e) {
                e.on(l);
            }
        });
    },

    createHandler: function(fn, c) {
        return function(e) {
            fn.call(this, e, c);
        };
    }
});

var combo = new Ext.form.ComboBox({

    // Collapse combo when its element is clicked on
    plugins: [ new Ext.DomObserver({
        click: function(evt, comp) {
            comp.collapse();
        }
    })],
    store: myStore,
    typeAhead: true,
    mode: 'local',
    triggerAction: 'all'
});
     * </code></pre></p>
     */
    var me = this, e = me.events;
    if(me.listeners){
        me.on(me.listeners);
        delete me.listeners;
    }
    me.events = e || {};
};

EXTUTIL.Observable.prototype = {
    // private
    filterOptRe : /^(?:scope|delay|buffer|single)$/,

    /**
     * <p>Fires the specified event with the passed parameters (minus the event name).</p>
     * <p>An event may be set to bubble up an Observable parent hierarchy (See {@link Ext.Component#getBubbleTarget})
     * by calling {@link #enableBubble}.</p>
     * @param {String} eventName The name of the event to fire.
     * @param {Object...} args Variable number of parameters are passed to handlers.
     * @return {Boolean} returns false if any of the handlers return false otherwise it returns true.
     */
    fireEvent : function(){
        var a = TOARRAY(arguments),
            ename = a[0].toLowerCase(),
            me = this,
            ret = TRUE,
            ce = me.events[ename],
            q,
            c;
        if (me.eventsSuspended === TRUE) {
            if (q = me.eventQueue) {
                q.push(a);
            }
        }
        else if(ISOBJECT(ce) && ce.bubble){
            if(ce.fire.apply(ce, a.slice(1)) === FALSE) {
                return FALSE;
            }
            c = me.getBubbleTarget && me.getBubbleTarget();
            if(c && c.enableBubble) {
                if(!c.events[ename] || !Ext.isObject(c.events[ename]) || !c.events[ename].bubble) {
                    c.enableBubble(ename);
                }
                return c.fireEvent.apply(c, a);
            }
        }
        else {
            if (ISOBJECT(ce)) {
                a.shift();
                ret = ce.fire.apply(ce, a);
            }
        }
        return ret;
    },

    /**
     * Appends an event handler to this object.
     * @param {String}   eventName The name of the event to listen for.
     * @param {Function} handler The method the event invokes.
     * @param {Object}   scope (optional) The scope (<code><b>this</b></code> reference) in which the handler function is executed.
     * <b>If omitted, defaults to the object which fired the event.</b>
     * @param {Object}   options (optional) An object containing handler configuration.
     * properties. This may contain any of the following properties:<ul>
     * <li><b>scope</b> : Object<div class="sub-desc">The scope (<code><b>this</b></code> reference) in which the handler function is executed.
     * <b>If omitted, defaults to the object which fired the event.</b></div></li>
     * <li><b>delay</b> : Number<div class="sub-desc">The number of milliseconds to delay the invocation of the handler after the event fires.</div></li>
     * <li><b>single</b> : Boolean<div class="sub-desc">True to add a handler to handle just the next firing of the event, and then remove itself.</div></li>
     * <li><b>buffer</b> : Number<div class="sub-desc">Causes the handler to be scheduled to run in an {@link Ext.util.DelayedTask} delayed
     * by the specified number of milliseconds. If the event fires again within that time, the original
     * handler is <em>not</em> invoked, but the new handler is scheduled in its place.</div></li>
     * <li><b>target</b> : Observable<div class="sub-desc">Only call the handler if the event was fired on the target Observable, <i>not</i>
     * if the event was bubbled up from a child Observable.</div></li>
     * </ul><br>
     * <p>
     * <b>Combining Options</b><br>
     * Using the options argument, it is possible to combine different types of listeners:<br>
     * <br>
     * A delayed, one-time listener.
     * <pre><code>
myDataView.on('click', this.onClick, this, {
single: true,
delay: 100
});</code></pre>
     * <p>
     * <b>Attaching multiple handlers in 1 call</b><br>
     * The method also allows for a single argument to be passed which is a config object containing properties
     * which specify multiple handlers.
     * <p>
     * <pre><code>
myGridPanel.on({
'click' : {
    fn: this.onClick,
    scope: this,
    delay: 100
},
'mouseover' : {
    fn: this.onMouseOver,
    scope: this
},
'mouseout' : {
    fn: this.onMouseOut,
    scope: this
}
});</code></pre>
 * <p>
 * Or a shorthand syntax:<br>
 * <pre><code>
myGridPanel.on({
'click' : this.onClick,
'mouseover' : this.onMouseOver,
'mouseout' : this.onMouseOut,
 scope: this
});</code></pre>
     */
    addListener : function(eventName, fn, scope, o){
        var me = this,
            e,
            oe,
            isF,
        ce;
        if (ISOBJECT(eventName)) {
            o = eventName;
            for (e in o){
                oe = o[e];
                if (!me.filterOptRe.test(e)) {
                    me.addListener(e, oe.fn || oe, oe.scope || o.scope, oe.fn ? oe : o);
                }
            }
        } else {
            eventName = eventName.toLowerCase();
            ce = me.events[eventName] || TRUE;
            if (Ext.isBoolean(ce)) {
                me.events[eventName] = ce = new EXTUTIL.Event(me, eventName);
            }
            ce.addListener(fn, scope, ISOBJECT(o) ? o : {});
        }
    },

    /**
     * Removes an event handler.
     * @param {String}   eventName The type of event the handler was associated with.
     * @param {Function} handler   The handler to remove. <b>This must be a reference to the function passed into the {@link #addListener} call.</b>
     * @param {Object}   scope     (optional) The scope originally specified for the handler.
     */
    removeListener : function(eventName, fn, scope){
        var ce = this.events[eventName.toLowerCase()];
        if (ISOBJECT(ce)) {
            ce.removeListener(fn, scope);
        }
    },

    /**
     * Removes all listeners for this object
     */
    purgeListeners : function(){
        var events = this.events,
            evt,
            key;
        for(key in events){
            evt = events[key];
            if(ISOBJECT(evt)){
                evt.clearListeners();
            }
        }
    },

    /**
     * Adds the specified events to the list of events which this Observable may fire.
     * @param {Object|String} o Either an object with event names as properties with a value of <code>true</code>
     * or the first event name string if multiple event names are being passed as separate parameters.
     * @param {string} Optional. Event name if multiple event names are being passed as separate parameters.
     * Usage:<pre><code>
this.addEvents('storeloaded', 'storecleared');
</code></pre>
     */
    addEvents : function(o){
        var me = this;
        me.events = me.events || {};
        if (Ext.isString(o)) {
            var a = arguments,
                i = a.length;
            while(i--) {
                me.events[a[i]] = me.events[a[i]] || TRUE;
            }
        } else {
            Ext.applyIf(me.events, o);
        }
    },

    /**
     * Checks to see if this object has any listeners for a specified event
     * @param {String} eventName The name of the event to check for
     * @return {Boolean} True if the event is being listened for, else false
     */
    hasListener : function(eventName){
        var e = this.events[eventName];
        return ISOBJECT(e) && e.listeners.length > 0;
    },

    /**
     * Suspend the firing of all events. (see {@link #resumeEvents})
     * @param {Boolean} queueSuspended Pass as true to queue up suspended events to be fired
     * after the {@link #resumeEvents} call instead of discarding all suspended events;
     */
    suspendEvents : function(queueSuspended){
        this.eventsSuspended = TRUE;
        if(queueSuspended && !this.eventQueue){
            this.eventQueue = [];
        }
    },

    /**
     * Resume firing events. (see {@link #suspendEvents})
     * If events were suspended using the <tt><b>queueSuspended</b></tt> parameter, then all
     * events fired during event suspension will be sent to any listeners now.
     */
    resumeEvents : function(){
        var me = this,
            queued = me.eventQueue || [];
        me.eventsSuspended = FALSE;
        delete me.eventQueue;
        EACH(queued, function(e) {
            me.fireEvent.apply(me, e);
        });
    }
};

var OBSERVABLE = EXTUTIL.Observable.prototype;
/**
 * Appends an event handler to this object (shorthand for {@link #addListener}.)
 * @param {String}   eventName     The type of event to listen for
 * @param {Function} handler       The method the event invokes
 * @param {Object}   scope         (optional) The scope (<code><b>this</b></code> reference) in which the handler function is executed.
 * <b>If omitted, defaults to the object which fired the event.</b>
 * @param {Object}   options       (optional) An object containing handler configuration.
 * @method
 */
OBSERVABLE.on = OBSERVABLE.addListener;
/**
 * Removes an event handler (shorthand for {@link #removeListener}.)
 * @param {String}   eventName     The type of event the handler was associated with.
 * @param {Function} handler       The handler to remove. <b>This must be a reference to the function passed into the {@link #addListener} call.</b>
 * @param {Object}   scope         (optional) The scope originally specified for the handler.
 * @method
 */
OBSERVABLE.un = OBSERVABLE.removeListener;

/**
 * Removes <b>all</b> added captures from the Observable.
 * @param {Observable} o The Observable to release
 * @static
 */
EXTUTIL.Observable.releaseCapture = function(o){
    o.fireEvent = OBSERVABLE.fireEvent;
};

function createTargeted(h, o, scope){
    return function(){
        if(o.target == arguments[0]){
            h.apply(scope, TOARRAY(arguments));
        }
    };
};

function createBuffered(h, o, l, scope){
    l.task = new EXTUTIL.DelayedTask();
    return function(){
        l.task.delay(o.buffer, h, scope, TOARRAY(arguments));
    };
};

function createSingle(h, e, fn, scope){
    return function(){
        e.removeListener(fn, scope);
        return h.apply(scope, arguments);
    };
};

function createDelayed(h, o, l, scope){
    return function(){
        var task = new EXTUTIL.DelayedTask();
        if(!l.tasks) {
            l.tasks = [];
        }
        l.tasks.push(task);
        task.delay(o.delay || 10, h, scope, TOARRAY(arguments));
    };
};

EXTUTIL.Event = function(obj, name){
    this.name = name;
    this.obj = obj;
    this.listeners = [];
};

EXTUTIL.Event.prototype = {
    addListener : function(fn, scope, options){
        var me = this,
            l;
        scope = scope || me.obj;
        if(!me.isListening(fn, scope)){
            l = me.createListener(fn, scope, options);
            if(me.firing){ // if we are currently firing this event, don't disturb the listener loop
                me.listeners = me.listeners.slice(0);
            }
            me.listeners.push(l);
        }
    },

    createListener: function(fn, scope, o){
        o = o || {}, scope = scope || this.obj;
        var l = {
            fn: fn,
            scope: scope,
            options: o
        }, h = fn;
        if(o.target){
            h = createTargeted(h, o, scope);
        }
        if(o.delay){
            h = createDelayed(h, o, l, scope);
        }
        if(o.single){
            h = createSingle(h, this, fn, scope);
        }
        if(o.buffer){
            h = createBuffered(h, o, l, scope);
        }
        l.fireFn = h;
        return l;
    },

    findListener : function(fn, scope){
        var list = this.listeners,
            i = list.length,
            l,
            s;
        while(i--) {
            l = list[i];
            if(l) {
                s = l.scope;
                if(l.fn == fn && (s == scope || s == this.obj)){
                    return i;
                }
            }
        }
        return -1;
    },

    isListening : function(fn, scope){
        return this.findListener(fn, scope) != -1;
    },

    removeListener : function(fn, scope){
        var index,
            l,
            k,
            me = this,
            ret = FALSE;
        if((index = me.findListener(fn, scope)) != -1){
            if (me.firing) {
                me.listeners = me.listeners.slice(0);
            }
            l = me.listeners[index];
            if(l.task) {
                l.task.cancel();
                delete l.task;
            }
            k = l.tasks && l.tasks.length;
            if(k) {
                while(k--) {
                    l.tasks[k].cancel();
                }
                delete l.tasks;
            }
            me.listeners.splice(index, 1);
            ret = TRUE;
        }
        return ret;
    },

    // Iterate to stop any buffered/delayed events
    clearListeners : function(){
        var me = this,
            l = me.listeners,
            i = l.length;
        while(i--) {
            me.removeListener(l[i].fn, l[i].scope);
        }
    },

    fire : function(){
        var me = this,
            args = TOARRAY(arguments),
            listeners = me.listeners,
            len = listeners.length,
            i = 0,
            l;

        if(len > 0){
            me.firing = TRUE;
            for (; i < len; i++) {
                l = listeners[i];
                if(l && l.fireFn.apply(l.scope || me.obj || window, args) === FALSE) {
                    return (me.firing = FALSE);
                }
            }
        }
        me.firing = FALSE;
        return TRUE;
    }
};
})();/**
 * @class Ext.DomHelper
 * <p>The DomHelper class provides a layer of abstraction from DOM and transparently supports creating
 * elements via DOM or using HTML fragments. It also has the ability to create HTML fragment templates
 * from your DOM building code.</p>
 *
 * <p><b><u>DomHelper element specification object</u></b></p>
 * <p>A specification object is used when creating elements. Attributes of this object
 * are assumed to be element attributes, except for 4 special attributes:
 * <div class="mdetail-params"><ul>
 * <li><b><tt>tag</tt></b> : <div class="sub-desc">The tag name of the element</div></li>
 * <li><b><tt>children</tt></b> : or <tt>cn</tt><div class="sub-desc">An array of the
 * same kind of element definition objects to be created and appended. These can be nested
 * as deep as you want.</div></li>
 * <li><b><tt>cls</tt></b> : <div class="sub-desc">The class attribute of the element.
 * This will end up being either the "class" attribute on a HTML fragment or className
 * for a DOM node, depending on whether DomHelper is using fragments or DOM.</div></li>
 * <li><b><tt>html</tt></b> : <div class="sub-desc">The innerHTML for the element</div></li>
 * </ul></div></p>
 *
 * <p><b><u>Insertion methods</u></b></p>
 * <p>Commonly used insertion methods:
 * <div class="mdetail-params"><ul>
 * <li><b><tt>{@link #append}</tt></b> : <div class="sub-desc"></div></li>
 * <li><b><tt>{@link #insertBefore}</tt></b> : <div class="sub-desc"></div></li>
 * <li><b><tt>{@link #insertAfter}</tt></b> : <div class="sub-desc"></div></li>
 * <li><b><tt>{@link #overwrite}</tt></b> : <div class="sub-desc"></div></li>
 * <li><b><tt>{@link #createTemplate}</tt></b> : <div class="sub-desc"></div></li>
 * <li><b><tt>{@link #insertHtml}</tt></b> : <div class="sub-desc"></div></li>
 * </ul></div></p>
 *
 * <p><b><u>Example</u></b></p>
 * <p>This is an example, where an unordered list with 3 children items is appended to an existing
 * element with id <tt>'my-div'</tt>:<br>
 <pre><code>
var dh = Ext.DomHelper; // create shorthand alias
// specification object
var spec = {
    id: 'my-ul',
    tag: 'ul',
    cls: 'my-list',
    // append children after creating
    children: [     // may also specify 'cn' instead of 'children'
        {tag: 'li', id: 'item0', html: 'List Item 0'},
        {tag: 'li', id: 'item1', html: 'List Item 1'},
        {tag: 'li', id: 'item2', html: 'List Item 2'}
    ]
};
var list = dh.append(
    'my-div', // the context element 'my-div' can either be the id or the actual node
    spec      // the specification object
);
 </code></pre></p>
 * <p>Element creation specification parameters in this class may also be passed as an Array of
 * specification objects. This can be used to insert multiple sibling nodes into an existing
 * container very efficiently. For example, to add more list items to the example above:<pre><code>
dh.append('my-ul', [
    {tag: 'li', id: 'item3', html: 'List Item 3'},
    {tag: 'li', id: 'item4', html: 'List Item 4'}
]);
 * </code></pre></p>
 *
 * <p><b><u>Templating</u></b></p>
 * <p>The real power is in the built-in templating. Instead of creating or appending any elements,
 * <tt>{@link #createTemplate}</tt> returns a Template object which can be used over and over to
 * insert new elements. Revisiting the example above, we could utilize templating this time:
 * <pre><code>
// create the node
var list = dh.append('my-div', {tag: 'ul', cls: 'my-list'});
// get template
var tpl = dh.createTemplate({tag: 'li', id: 'item{0}', html: 'List Item {0}'});

for(var i = 0; i < 5, i++){
    tpl.append(list, [i]); // use template to append to the actual node
}
 * </code></pre></p>
 * <p>An example using a template:<pre><code>
var html = '<a id="{0}" href="{1}" class="nav">{2}</a>';

var tpl = new Ext.DomHelper.createTemplate(html);
tpl.append('blog-roll', ['link1', 'http://www.jackslocum.com/', "Jack&#39;s Site"]);
tpl.append('blog-roll', ['link2', 'http://www.dustindiaz.com/', "Dustin&#39;s Site"]);
 * </code></pre></p>
 *
 * <p>The same example using named parameters:<pre><code>
var html = '<a id="{id}" href="{url}" class="nav">{text}</a>';

var tpl = new Ext.DomHelper.createTemplate(html);
tpl.append('blog-roll', {
    id: 'link1',
    url: 'http://www.jackslocum.com/',
    text: "Jack&#39;s Site"
});
tpl.append('blog-roll', {
    id: 'link2',
    url: 'http://www.dustindiaz.com/',
    text: "Dustin&#39;s Site"
});
 * </code></pre></p>
 *
 * <p><b><u>Compiling Templates</u></b></p>
 * <p>Templates are applied using regular expressions. The performance is great, but if
 * you are adding a bunch of DOM elements using the same template, you can increase
 * performance even further by {@link Ext.Template#compile "compiling"} the template.
 * The way "{@link Ext.Template#compile compile()}" works is the template is parsed and
 * broken up at the different variable points and a dynamic function is created and eval'ed.
 * The generated function performs string concatenation of these parts and the passed
 * variables instead of using regular expressions.
 * <pre><code>
var html = '<a id="{id}" href="{url}" class="nav">{text}</a>';

var tpl = new Ext.DomHelper.createTemplate(html);
tpl.compile();

//... use template like normal
 * </code></pre></p>
 *
 * <p><b><u>Performance Boost</u></b></p>
 * <p>DomHelper will transparently create HTML fragments when it can. Using HTML fragments instead
 * of DOM can significantly boost performance.</p>
 * <p>Element creation specification parameters may also be strings. If {@link #useDom} is <tt>false</tt>,
 * then the string is used as innerHTML. If {@link #useDom} is <tt>true</tt>, a string specification
 * results in the creation of a text node. Usage:</p>
 * <pre><code>
Ext.DomHelper.useDom = true; // force it to use DOM; reduces performance
 * </code></pre>
 * @singleton
 */
Ext.DomHelper = function(){
    var tempTableEl = null,
        emptyTags = /^(?:br|frame|hr|img|input|link|meta|range|spacer|wbr|area|param|col)$/i,
        tableRe = /^table|tbody|tr|td$/i,
        pub,
        // kill repeat to save bytes
        afterbegin = 'afterbegin',
        afterend = 'afterend',
        beforebegin = 'beforebegin',
        beforeend = 'beforeend',
        ts = '<table>',
        te = '</table>',
        tbs = ts+'<tbody>',
        tbe = '</tbody>'+te,
        trs = tbs + '<tr>',
        tre = '</tr>'+tbe;

    // private
    function doInsert(el, o, returnElement, pos, sibling, append){
        var newNode = pub.insertHtml(pos, Ext.getDom(el), createHtml(o));
        return returnElement ? Ext.get(newNode, true) : newNode;
    }

    // build as innerHTML where available
    function createHtml(o){
        var b = '',
            attr,
            val,
            key,
            keyVal,
            cn;

        if(Ext.isString(o)){
            b = o;
        } else if (Ext.isArray(o)) {
            for (var i=0; i < o.length; i++) {
                if(o[i]) {
                    b += createHtml(o[i]);
                }
            };
        } else {
            b += '<' + (o.tag = o.tag || 'div');
            Ext.iterate(o, function(attr, val){
                if(!/tag|children|cn|html$/i.test(attr)){
                    if (Ext.isObject(val)) {
                        b += ' ' + attr + '="';
                        Ext.iterate(val, function(key, keyVal){
                            b += key + ':' + keyVal + ';';
                        });
                        b += '"';
                    }else{
                        b += ' ' + ({cls : 'class', htmlFor : 'for'}[attr] || attr) + '="' + val + '"';
                    }
                }
            });
            // Now either just close the tag or try to add children and close the tag.
            if (emptyTags.test(o.tag)) {
                b += '/>';
            } else {
                b += '>';
                if ((cn = o.children || o.cn)) {
                    b += createHtml(cn);
                } else if(o.html){
                    b += o.html;
                }
                b += '</' + o.tag + '>';
            }
        }
        return b;
    }

    function ieTable(depth, s, h, e){
        tempTableEl.innerHTML = [s, h, e].join('');
        var i = -1,
            el = tempTableEl,
            ns;
        while(++i < depth){
            el = el.firstChild;
        }
//      If the result is multiple siblings, then encapsulate them into one fragment.
        if(ns = el.nextSibling){
            var df = document.createDocumentFragment();
            while(el){
                ns = el.nextSibling;
                df.appendChild(el);
                el = ns;
            }
            el = df;
        }
        return el;
    }

    /**
     * @ignore
     * Nasty code for IE's broken table implementation
     */
    function insertIntoTable(tag, where, el, html) {
        var node,
            before;

        tempTableEl = tempTableEl || document.createElement('div');

        if(tag == 'td' && (where == afterbegin || where == beforeend) ||
           !/td|tr|tbody/i.test(tag) && (where == beforebegin || where == afterend)) {
            return;
        }
        before = where == beforebegin ? el :
                 where == afterend ? el.nextSibling :
                 where == afterbegin ? el.firstChild : null;

        if (where == beforebegin || where == afterend) {
            el = el.parentNode;
        }

        if (tag == 'td' || (tag == 'tr' && (where == beforeend || where == afterbegin))) {
            node = ieTable(4, trs, html, tre);
        } else if ((tag == 'tbody' && (where == beforeend || where == afterbegin)) ||
                   (tag == 'tr' && (where == beforebegin || where == afterend))) {
            node = ieTable(3, tbs, html, tbe);
        } else {
            node = ieTable(2, ts, html, te);
        }
        el.insertBefore(node, before);
        return node;
    }


    pub = {
        /**
         * Returns the markup for the passed Element(s) config.
         * @param {Object} o The DOM object spec (and children)
         * @return {String}
         */
        markup : function(o){
            return createHtml(o);
        },
        
        /**
         * Applies a style specification to an element.
         * @param {String/HTMLElement} el The element to apply styles to
         * @param {String/Object/Function} styles A style specification string e.g. 'width:100px', or object in the form {width:'100px'}, or
         * a function which returns such a specification.
         */
        applyStyles : function(el, styles){
            if(styles){
                var i = 0,
                    len,
                    style;

                el = Ext.fly(el);
                if(Ext.isFunction(styles)){
                    styles = styles.call();
                }
                if(Ext.isString(styles)){
                    styles = styles.trim().split(/\s*(?::|;)\s*/);
                    for(len = styles.length; i < len;){
                        el.setStyle(styles[i++], styles[i++]);
                    }
                }else if (Ext.isObject(styles)){
                    el.setStyle(styles);
                }
            }
        },

        /**
         * Inserts an HTML fragment into the DOM.
         * @param {String} where Where to insert the html in relation to el - beforeBegin, afterBegin, beforeEnd, afterEnd.
         * @param {HTMLElement} el The context element
         * @param {String} html The HTML fragment
         * @return {HTMLElement} The new node
         */
        insertHtml : function(where, el, html){
            var hash = {},
                hashVal,
                setStart,
                range,
                frag,
                rangeEl,
                rs;

            where = where.toLowerCase();
            // add these here because they are used in both branches of the condition.
            hash[beforebegin] = ['BeforeBegin', 'previousSibling'];
            hash[afterend] = ['AfterEnd', 'nextSibling'];

            if (el.insertAdjacentHTML) {
                if(tableRe.test(el.tagName) && (rs = insertIntoTable(el.tagName.toLowerCase(), where, el, html))){
                    return rs;
                }
                // add these two to the hash.
                hash[afterbegin] = ['AfterBegin', 'firstChild'];
                hash[beforeend] = ['BeforeEnd', 'lastChild'];
                if ((hashVal = hash[where])) {
                    el.insertAdjacentHTML(hashVal[0], html);
                    return el[hashVal[1]];
                }
            } else {
                range = el.ownerDocument.createRange();
                setStart = 'setStart' + (/end/i.test(where) ? 'After' : 'Before');
                if (hash[where]) {
                    range[setStart](el);
                    frag = range.createContextualFragment(html);
                    el.parentNode.insertBefore(frag, where == beforebegin ? el : el.nextSibling);
                    return el[(where == beforebegin ? 'previous' : 'next') + 'Sibling'];
                } else {
                    rangeEl = (where == afterbegin ? 'first' : 'last') + 'Child';
                    if (el.firstChild) {
                        range[setStart](el[rangeEl]);
                        frag = range.createContextualFragment(html);
                        if(where == afterbegin){
                            el.insertBefore(frag, el.firstChild);
                        }else{
                            el.appendChild(frag);
                        }
                    } else {
                        el.innerHTML = html;
                    }
                    return el[rangeEl];
                }
            }
            throw 'Illegal insertion point -> "' + where + '"';
        },

        /**
         * Creates new DOM element(s) and inserts them before el.
         * @param {Mixed} el The context element
         * @param {Object/String} o The DOM object spec (and children) or raw HTML blob
         * @param {Boolean} returnElement (optional) true to return a Ext.Element
         * @return {HTMLElement/Ext.Element} The new node
         */
        insertBefore : function(el, o, returnElement){
            return doInsert(el, o, returnElement, beforebegin);
        },

        /**
         * Creates new DOM element(s) and inserts them after el.
         * @param {Mixed} el The context element
         * @param {Object} o The DOM object spec (and children)
         * @param {Boolean} returnElement (optional) true to return a Ext.Element
         * @return {HTMLElement/Ext.Element} The new node
         */
        insertAfter : function(el, o, returnElement){
            return doInsert(el, o, returnElement, afterend, 'nextSibling');
        },

        /**
         * Creates new DOM element(s) and inserts them as the first child of el.
         * @param {Mixed} el The context element
         * @param {Object/String} o The DOM object spec (and children) or raw HTML blob
         * @param {Boolean} returnElement (optional) true to return a Ext.Element
         * @return {HTMLElement/Ext.Element} The new node
         */
        insertFirst : function(el, o, returnElement){
            return doInsert(el, o, returnElement, afterbegin, 'firstChild');
        },

        /**
         * Creates new DOM element(s) and appends them to el.
         * @param {Mixed} el The context element
         * @param {Object/String} o The DOM object spec (and children) or raw HTML blob
         * @param {Boolean} returnElement (optional) true to return a Ext.Element
         * @return {HTMLElement/Ext.Element} The new node
         */
        append : function(el, o, returnElement){
            return doInsert(el, o, returnElement, beforeend, '', true);
        },

        /**
         * Creates new DOM element(s) and overwrites the contents of el with them.
         * @param {Mixed} el The context element
         * @param {Object/String} o The DOM object spec (and children) or raw HTML blob
         * @param {Boolean} returnElement (optional) true to return a Ext.Element
         * @return {HTMLElement/Ext.Element} The new node
         */
        overwrite : function(el, o, returnElement){
            el = Ext.getDom(el);
            el.innerHTML = createHtml(o);
            return returnElement ? Ext.get(el.firstChild) : el.firstChild;
        },

        createHtml : createHtml
    };
    return pub;
}();/**
 * @class Ext.Template
 * <p>Represents an HTML fragment template. Templates may be {@link #compile precompiled}
 * for greater performance.</p>
 * <p>For example usage {@link #Template see the constructor}.</p>
 * 
 * @constructor
 * An instance of this class may be created by passing to the constructor either
 * a single argument, or multiple arguments:
 * <div class="mdetail-params"><ul>
 * <li><b>single argument</b> : String/Array
 * <div class="sub-desc">
 * The single argument may be either a String or an Array:<ul>
 * <li><tt>String</tt> : </li><pre><code>
var t = new Ext.Template("&lt;div>Hello {0}.&lt;/div>");
t.{@link #append}('some-element', ['foo']);
 * </code></pre>
 * <li><tt>Array</tt> : </li>
 * An Array will be combined with <code>join('')</code>.
<pre><code>
var t = new Ext.Template([
    '&lt;div name="{id}"&gt;',
        '&lt;span class="{cls}"&gt;{name:trim} {value:ellipsis(10)}&lt;/span&gt;',
    '&lt;/div&gt;',
]);
t.{@link #compile}();
t.{@link #append}('some-element', {id: 'myid', cls: 'myclass', name: 'foo', value: 'bar'});
</code></pre>
 * </ul></div></li>
 * <li><b>multiple arguments</b> : String, Object, Array, ...
 * <div class="sub-desc">
 * Multiple arguments will be combined with <code>join('')</code>.
 * <pre><code>
var t = new Ext.Template(
    '&lt;div name="{id}"&gt;',
        '&lt;span class="{cls}"&gt;{name} {value}&lt;/span&gt;',
    '&lt;/div&gt;',
    // a configuration object:
    {
        compiled: true,      // {@link #compile} immediately
        disableFormats: true // See Notes below.
    } 
);
 * </code></pre>
 * <p><b>Notes</b>:</p>
 * <div class="mdetail-params"><ul>
 * <li>Formatting and <code>disableFormats</code> are not applicable for Ext Core.</li>
 * <li>For a list of available format functions, see {@link Ext.util.Format}.</li>
 * <li><code>disableFormats</code> reduces <code>{@link #apply}</code> time
 * when no formatting is required.</li>
 * </ul></div>
 * </div></li>
 * </ul></div>
 * @param {Mixed} config
 */
Ext.Template = function(html){
    var me = this,
    	a = arguments,
    	buf = [];

    if (Ext.isArray(html)) {
        html = html.join("");
    } else if (a.length > 1) {
	    Ext.each(a, function(v) {
            if (Ext.isObject(v)) {
                Ext.apply(me, v);
            } else {
                buf.push(v);
            }
        });
        html = buf.join('');
    }

    /**@private*/
    me.html = html;
    /**
     * @cfg {Boolean} compiled Specify <tt>true</tt> to compile the template
     * immediately (see <code>{@link #compile}</code>).
     * Defaults to <tt>false</tt>.
     */
    if (me.compiled) {
        me.compile();
    }
};
Ext.Template.prototype = {
    /**
     * @cfg {RegExp} re The regular expression used to match template variables.
     * Defaults to:<pre><code>
     * re : /\{([\w-]+)\}/g                                     // for Ext Core
     * re : /\{([\w-]+)(?:\:([\w\.]*)(?:\((.*?)?\))?)?\}/g      // for Ext JS
     * </code></pre>
     */
    re : /\{([\w-]+)\}/g,
    /**
     * See <code>{@link #re}</code>.
     * @type RegExp
     * @property re
     */

    /**
     * Returns an HTML fragment of this template with the specified <code>values</code> applied.
     * @param {Object/Array} values
     * The template values. Can be an array if the params are numeric (i.e. <code>{0}</code>)
     * or an object (i.e. <code>{foo: 'bar'}</code>).
     * @return {String} The HTML fragment
     */
    applyTemplate : function(values){
		var me = this;

        return me.compiled ?
        		me.compiled(values) :
				me.html.replace(me.re, function(m, name){
		        	return values[name] !== undefined ? values[name] : "";
		        });
	},

    /**
     * Sets the HTML used as the template and optionally compiles it.
     * @param {String} html
     * @param {Boolean} compile (optional) True to compile the template (defaults to undefined)
     * @return {Ext.Template} this
     */
    set : function(html, compile){
	    var me = this;
        me.html = html;
        me.compiled = null;
        return compile ? me.compile() : me;
    },

    /**
     * Compiles the template into an internal function, eliminating the RegEx overhead.
     * @return {Ext.Template} this
     */
    compile : function(){
        var me = this,
        	sep = Ext.isGecko ? "+" : ",";

        function fn(m, name){                        
	        name = "values['" + name + "']";
	        return "'"+ sep + '(' + name + " == undefined ? '' : " + name + ')' + sep + "'";
        }
                
        eval("this.compiled = function(values){ return " + (Ext.isGecko ? "'" : "['") +
             me.html.replace(/\\/g, '\\\\').replace(/(\r\n|\n)/g, '\\n').replace(/'/g, "\\'").replace(this.re, fn) +
             (Ext.isGecko ?  "';};" : "'].join('');};"));
        return me;
    },

    /**
     * Applies the supplied values to the template and inserts the new node(s) as the first child of el.
     * @param {Mixed} el The context element
     * @param {Object/Array} values The template values. Can be an array if your params are numeric (i.e. {0}) or an object (i.e. {foo: 'bar'})
     * @param {Boolean} returnElement (optional) true to return a Ext.Element (defaults to undefined)
     * @return {HTMLElement/Ext.Element} The new node or Element
     */
    insertFirst: function(el, values, returnElement){
        return this.doInsert('afterBegin', el, values, returnElement);
    },

    /**
     * Applies the supplied values to the template and inserts the new node(s) before el.
     * @param {Mixed} el The context element
     * @param {Object/Array} values The template values. Can be an array if your params are numeric (i.e. {0}) or an object (i.e. {foo: 'bar'})
     * @param {Boolean} returnElement (optional) true to return a Ext.Element (defaults to undefined)
     * @return {HTMLElement/Ext.Element} The new node or Element
     */
    insertBefore: function(el, values, returnElement){
        return this.doInsert('beforeBegin', el, values, returnElement);
    },

    /**
     * Applies the supplied values to the template and inserts the new node(s) after el.
     * @param {Mixed} el The context element
     * @param {Object/Array} values The template values. Can be an array if your params are numeric (i.e. {0}) or an object (i.e. {foo: 'bar'})
     * @param {Boolean} returnElement (optional) true to return a Ext.Element (defaults to undefined)
     * @return {HTMLElement/Ext.Element} The new node or Element
     */
    insertAfter : function(el, values, returnElement){
        return this.doInsert('afterEnd', el, values, returnElement);
    },

    /**
     * Applies the supplied <code>values</code> to the template and appends
     * the new node(s) to the specified <code>el</code>.
     * <p>For example usage {@link #Template see the constructor}.</p>
     * @param {Mixed} el The context element
     * @param {Object/Array} values
     * The template values. Can be an array if the params are numeric (i.e. <code>{0}</code>)
     * or an object (i.e. <code>{foo: 'bar'}</code>).
     * @param {Boolean} returnElement (optional) true to return an Ext.Element (defaults to undefined)
     * @return {HTMLElement/Ext.Element} The new node or Element
     */
    append : function(el, values, returnElement){
        return this.doInsert('beforeEnd', el, values, returnElement);
    },

    doInsert : function(where, el, values, returnEl){
        el = Ext.getDom(el);
        var newNode = Ext.DomHelper.insertHtml(where, el, this.applyTemplate(values));
        return returnEl ? Ext.get(newNode, true) : newNode;
    },

    /**
     * Applies the supplied values to the template and overwrites the content of el with the new node(s).
     * @param {Mixed} el The context element
     * @param {Object/Array} values The template values. Can be an array if your params are numeric (i.e. {0}) or an object (i.e. {foo: 'bar'})
     * @param {Boolean} returnElement (optional) true to return a Ext.Element (defaults to undefined)
     * @return {HTMLElement/Ext.Element} The new node or Element
     */
    overwrite : function(el, values, returnElement){
        el = Ext.getDom(el);
        el.innerHTML = this.applyTemplate(values);
        return returnElement ? Ext.get(el.firstChild, true) : el.firstChild;
    }
};
/**
 * Alias for {@link #applyTemplate}
 * Returns an HTML fragment of this template with the specified <code>values</code> applied.
 * @param {Object/Array} values
 * The template values. Can be an array if the params are numeric (i.e. <code>{0}</code>)
 * or an object (i.e. <code>{foo: 'bar'}</code>).
 * @return {String} The HTML fragment
 * @member Ext.Template
 * @method apply
 */
Ext.Template.prototype.apply = Ext.Template.prototype.applyTemplate;

/**
 * Creates a template from the passed element's value (<i>display:none</i> textarea, preferred) or innerHTML.
 * @param {String/HTMLElement} el A DOM element or its id
 * @param {Object} config A configuration object
 * @return {Ext.Template} The created template
 * @static
 */
Ext.Template.from = function(el, config){
    el = Ext.getDom(el);
    return new Ext.Template(el.value || el.innerHTML, config || '');
};/*
 * This is code is also distributed under MIT license for use
 * with jQuery and prototype JavaScript libraries.
 */
/**
 * @class Ext.DomQuery
Provides high performance selector/xpath processing by compiling queries into reusable functions. New pseudo classes and matchers can be plugged. It works on HTML and XML documents (if a content node is passed in).
<p>
DomQuery supports most of the <a href="http://www.w3.org/TR/2005/WD-css3-selectors-20051215/#selectors">CSS3 selectors spec</a>, along with some custom selectors and basic XPath.</p>

<p>
All selectors, attribute filters and pseudos below can be combined infinitely in any order. For example "div.foo:nth-child(odd)[@foo=bar].bar:first" would be a perfectly valid selector. Node filters are processed in the order in which they appear, which allows you to optimize your queries for your document structure.
</p>
<h4>Element Selectors:</h4>
<ul class="list">
    <li> <b>*</b> any element</li>
    <li> <b>E</b> an element with the tag E</li>
    <li> <b>E F</b> All descendent elements of E that have the tag F</li>
    <li> <b>E > F</b> or <b>E/F</b> all direct children elements of E that have the tag F</li>
    <li> <b>E + F</b> all elements with the tag F that are immediately preceded by an element with the tag E</li>
    <li> <b>E ~ F</b> all elements with the tag F that are preceded by a sibling element with the tag E</li>
</ul>
<h4>Attribute Selectors:</h4>
<p>The use of &#64; and quotes are optional. For example, div[&#64;foo='bar'] is also a valid attribute selector.</p>
<ul class="list">
    <li> <b>E[foo]</b> has an attribute "foo"</li>
    <li> <b>E[foo=bar]</b> has an attribute "foo" that equals "bar"</li>
    <li> <b>E[foo^=bar]</b> has an attribute "foo" that starts with "bar"</li>
    <li> <b>E[foo$=bar]</b> has an attribute "foo" that ends with "bar"</li>
    <li> <b>E[foo*=bar]</b> has an attribute "foo" that contains the substring "bar"</li>
    <li> <b>E[foo%=2]</b> has an attribute "foo" that is evenly divisible by 2</li>
    <li> <b>E[foo!=bar]</b> has an attribute "foo" that does not equal "bar"</li>
</ul>
<h4>Pseudo Classes:</h4>
<ul class="list">
    <li> <b>E:first-child</b> E is the first child of its parent</li>
    <li> <b>E:last-child</b> E is the last child of its parent</li>
    <li> <b>E:nth-child(<i>n</i>)</b> E is the <i>n</i>th child of its parent (1 based as per the spec)</li>
    <li> <b>E:nth-child(odd)</b> E is an odd child of its parent</li>
    <li> <b>E:nth-child(even)</b> E is an even child of its parent</li>
    <li> <b>E:only-child</b> E is the only child of its parent</li>
    <li> <b>E:checked</b> E is an element that is has a checked attribute that is true (e.g. a radio or checkbox) </li>
    <li> <b>E:first</b> the first E in the resultset</li>
    <li> <b>E:last</b> the last E in the resultset</li>
    <li> <b>E:nth(<i>n</i>)</b> the <i>n</i>th E in the resultset (1 based)</li>
    <li> <b>E:odd</b> shortcut for :nth-child(odd)</li>
    <li> <b>E:even</b> shortcut for :nth-child(even)</li>
    <li> <b>E:contains(foo)</b> E's innerHTML contains the substring "foo"</li>
    <li> <b>E:nodeValue(foo)</b> E contains a textNode with a nodeValue that equals "foo"</li>
    <li> <b>E:not(S)</b> an E element that does not match simple selector S</li>
    <li> <b>E:has(S)</b> an E element that has a descendent that matches simple selector S</li>
    <li> <b>E:next(S)</b> an E element whose next sibling matches simple selector S</li>
    <li> <b>E:prev(S)</b> an E element whose previous sibling matches simple selector S</li>
</ul>
<h4>CSS Value Selectors:</h4>
<ul class="list">
    <li> <b>E{display=none}</b> css value "display" that equals "none"</li>
    <li> <b>E{display^=none}</b> css value "display" that starts with "none"</li>
    <li> <b>E{display$=none}</b> css value "display" that ends with "none"</li>
    <li> <b>E{display*=none}</b> css value "display" that contains the substring "none"</li>
    <li> <b>E{display%=2}</b> css value "display" that is evenly divisible by 2</li>
    <li> <b>E{display!=none}</b> css value "display" that does not equal "none"</li>
</ul>
 * @singleton
 */
Ext.DomQuery = function(){
    var cache = {}, 
    	simpleCache = {}, 
    	valueCache = {},
    	nonSpace = /\S/,
    	trimRe = /^\s+|\s+$/g,
    	tplRe = /\{(\d+)\}/g,
    	modeRe = /^(\s?[\/>+~]\s?|\s|$)/,
    	tagTokenRe = /^(#)?([\w-\*]+)/,
    	nthRe = /(\d*)n\+?(\d*)/, 
    	nthRe2 = /\D/,
    	// This is for IE MSXML which does not support expandos.
	    // IE runs the same speed using setAttribute, however FF slows way down
	    // and Safari completely fails so they need to continue to use expandos.
	    isIE = window.ActiveXObject ? true : false,
	    key = 30803;
	    
    // this eval is stop the compressor from
	// renaming the variable to something shorter
	eval("var batch = 30803;");    	

    function child(p, index){
        var i = 0,
        	n = p.firstChild;
        while(n){
            if(n.nodeType == 1){
               if(++i == index){
                   return n;
               }
            }
            n = n.nextSibling;
        }
        return null;
    };

    function next(n){
        while((n = n.nextSibling) && n.nodeType != 1);
        return n;
    };

    function prev(n){
        while((n = n.previousSibling) && n.nodeType != 1);
        return n;
    };

    function children(d){
        var n = d.firstChild, ni = -1,
        	nx;
 	    while(n){
 	        nx = n.nextSibling;
 	        if(n.nodeType == 3 && !nonSpace.test(n.nodeValue)){
 	            d.removeChild(n);
 	        }else{
 	            n.nodeIndex = ++ni;
 	        }
 	        n = nx;
 	    }
 	    return this;
 	};

    function byClassName(c, a, v){
        if(!v){
            return c;
        }
        var r = [], ri = -1, cn;
        for(var i = 0, ci; ci = c[i]; i++){
            if((' '+ci.className+' ').indexOf(v) != -1){
                r[++ri] = ci;
            }
        }
        return r;
    };

    function attrValue(n, attr){
        if(!n.tagName && typeof n.length != "undefined"){
            n = n[0];
        }
        if(!n){
            return null;
        }
        if(attr == "for"){
            return n.htmlFor;
        }
        if(attr == "class" || attr == "className"){
            return n.className;
        }
        return n.getAttribute(attr) || n[attr];

    };

    function getNodes(ns, mode, tagName){
        var result = [], ri = -1, cs;
        if(!ns){
            return result;
        }
        tagName = tagName || "*";
        if(typeof ns.getElementsByTagName != "undefined"){
            ns = [ns];
        }
        if(!mode){
            for(var i = 0, ni; ni = ns[i]; i++){
                cs = ni.getElementsByTagName(tagName);
                for(var j = 0, ci; ci = cs[j]; j++){
                    result[++ri] = ci;
                }
            }
        }else if(mode == "/" || mode == ">"){
            var utag = tagName.toUpperCase();
            for(var i = 0, ni, cn; ni = ns[i]; i++){
                cn = ni.childNodes;
                for(var j = 0, cj; cj = cn[j]; j++){
                    if(cj.nodeName == utag || cj.nodeName == tagName  || tagName == '*'){
                        result[++ri] = cj;
                    }
                }
            }
        }else if(mode == "+"){
            var utag = tagName.toUpperCase();
            for(var i = 0, n; n = ns[i]; i++){
                while((n = n.nextSibling) && n.nodeType != 1);
                if(n && (n.nodeName == utag || n.nodeName == tagName || tagName == '*')){
                    result[++ri] = n;
                }
            }
        }else if(mode == "~"){
            var utag = tagName.toUpperCase();
            for(var i = 0, n; n = ns[i]; i++){
                while((n = n.nextSibling)){
                    if (n.nodeName == utag || n.nodeName == tagName || tagName == '*'){
                        result[++ri] = n;
                    }
                }
            }
        }
        return result;
    };

    function concat(a, b){
        if(b.slice){
            return a.concat(b);
        }
        for(var i = 0, l = b.length; i < l; i++){
            a[a.length] = b[i];
        }
        return a;
    }

    function byTag(cs, tagName){
        if(cs.tagName || cs == document){
            cs = [cs];
        }
        if(!tagName){
            return cs;
        }
        var r = [], ri = -1;
        tagName = tagName.toLowerCase();
        for(var i = 0, ci; ci = cs[i]; i++){
            if(ci.nodeType == 1 && ci.tagName.toLowerCase()==tagName){
                r[++ri] = ci;
            }
        }
        return r;
    };

    function byId(cs, attr, id){
        if(cs.tagName || cs == document){
            cs = [cs];
        }
        if(!id){
            return cs;
        }
        var r = [], ri = -1;
        for(var i = 0,ci; ci = cs[i]; i++){
            if(ci && ci.id == id){
                r[++ri] = ci;
                return r;
            }
        }
        return r;
    };

    function byAttribute(cs, attr, value, op, custom){
        var r = [], 
            ri = -1, 
            st = custom=="{",
            f = Ext.DomQuery.operators[op],
            a,
            ih;
        for(var i = 0, ci; ci = cs[i]; i++){
            if(ci.nodeType != 1){
                continue;
            }
            ih = ci.innerHTML;
            // we only need to change the property names if we're dealing with html nodes, not XML
            if(ih !== null && ih !== undefined){
                if(st){
                    a = Ext.DomQuery.getStyle(ci, attr);
                }else if(attr == "class" || attr == "className"){
                    a = ci.className;
                }else if(attr == "for"){
                    a = ci.htmlFor;
                }else if(attr == "href"){
                    a = ci.getAttribute("href", 2);
                }else{
                    a = ci.getAttribute(attr);
                }
            }else{
                a = ci.getAttribute(attr);
            }
            if((f && f(a, value)) || (!f && a)){
                r[++ri] = ci;
            }
        }
        return r;
    };

    function byPseudo(cs, name, value){
        return Ext.DomQuery.pseudos[name](cs, value);
    };

    function nodupIEXml(cs){
        var d = ++key, 
        	r;
        cs[0].setAttribute("_nodup", d);
        r = [cs[0]];
        for(var i = 1, len = cs.length; i < len; i++){
            var c = cs[i];
            if(!c.getAttribute("_nodup") != d){
                c.setAttribute("_nodup", d);
                r[r.length] = c;
            }
        }
        for(var i = 0, len = cs.length; i < len; i++){
            cs[i].removeAttribute("_nodup");
        }
        return r;
    }

    function nodup(cs){
        if(!cs){
            return [];
        }
        var len = cs.length, c, i, r = cs, cj, ri = -1;
        if(!len || typeof cs.nodeType != "undefined" || len == 1){
            return cs;
        }
        if(isIE && typeof cs[0].selectSingleNode != "undefined"){
            return nodupIEXml(cs);
        }
        var d = ++key;
        cs[0]._nodup = d;
        for(i = 1; c = cs[i]; i++){
            if(c._nodup != d){
                c._nodup = d;
            }else{
                r = [];
                for(var j = 0; j < i; j++){
                    r[++ri] = cs[j];
                }
                for(j = i+1; cj = cs[j]; j++){
                    if(cj._nodup != d){
                        cj._nodup = d;
                        r[++ri] = cj;
                    }
                }
                return r;
            }
        }
        return r;
    }

    function quickDiffIEXml(c1, c2){
        var d = ++key,
        	r = [];
        for(var i = 0, len = c1.length; i < len; i++){
            c1[i].setAttribute("_qdiff", d);
        }        
        for(var i = 0, len = c2.length; i < len; i++){
            if(c2[i].getAttribute("_qdiff") != d){
                r[r.length] = c2[i];
            }
        }
        for(var i = 0, len = c1.length; i < len; i++){
           c1[i].removeAttribute("_qdiff");
        }
        return r;
    }

    function quickDiff(c1, c2){
        var len1 = c1.length,
        	d = ++key,
        	r = [];
        if(!len1){
            return c2;
        }
        if(isIE && typeof c1[0].selectSingleNode != "undefined"){
            return quickDiffIEXml(c1, c2);
        }        
        for(var i = 0; i < len1; i++){
            c1[i]._qdiff = d;
        }        
        for(var i = 0, len = c2.length; i < len; i++){
            if(c2[i]._qdiff != d){
                r[r.length] = c2[i];
            }
        }
        return r;
    }

    function quickId(ns, mode, root, id){
        if(ns == root){
           var d = root.ownerDocument || root;
           return d.getElementById(id);
        }
        ns = getNodes(ns, mode, "*");
        return byId(ns, null, id);
    }

    return {
        getStyle : function(el, name){
            return Ext.fly(el).getStyle(name);
        },
        /**
         * Compiles a selector/xpath query into a reusable function. The returned function
         * takes one parameter "root" (optional), which is the context node from where the query should start.
         * @param {String} selector The selector/xpath query
         * @param {String} type (optional) Either "select" (the default) or "simple" for a simple selector match
         * @return {Function}
         */
        compile : function(path, type){
            type = type || "select";

            var fn = ["var f = function(root){\n var mode; ++batch; var n = root || document;\n"],
            	q = path, mode, lq,
            	tk = Ext.DomQuery.matchers,
            	tklen = tk.length,
            	mm,
            	// accept leading mode switch
            	lmode = q.match(modeRe);
            
            if(lmode && lmode[1]){
                fn[fn.length] = 'mode="'+lmode[1].replace(trimRe, "")+'";';
                q = q.replace(lmode[1], "");
            }
            // strip leading slashes
            while(path.substr(0, 1)=="/"){
                path = path.substr(1);
            }

            while(q && lq != q){
                lq = q;
                var tm = q.match(tagTokenRe);
                if(type == "select"){
                    if(tm){
                        if(tm[1] == "#"){
                            fn[fn.length] = 'n = quickId(n, mode, root, "'+tm[2]+'");';
                        }else{
                            fn[fn.length] = 'n = getNodes(n, mode, "'+tm[2]+'");';
                        }
                        q = q.replace(tm[0], "");
                    }else if(q.substr(0, 1) != '@'){
                        fn[fn.length] = 'n = getNodes(n, mode, "*");';
                    }
                }else{
                    if(tm){
                        if(tm[1] == "#"){
                            fn[fn.length] = 'n = byId(n, null, "'+tm[2]+'");';
                        }else{
                            fn[fn.length] = 'n = byTag(n, "'+tm[2]+'");';
                        }
                        q = q.replace(tm[0], "");
                    }
                }
                while(!(mm = q.match(modeRe))){
                    var matched = false;
                    for(var j = 0; j < tklen; j++){
                        var t = tk[j];
                        var m = q.match(t.re);
                        if(m){
                            fn[fn.length] = t.select.replace(tplRe, function(x, i){
                                                    return m[i];
                                                });
                            q = q.replace(m[0], "");
                            matched = true;
                            break;
                        }
                    }
                    // prevent infinite loop on bad selector
                    if(!matched){
                        throw 'Error parsing selector, parsing failed at "' + q + '"';
                    }
                }
                if(mm[1]){
                    fn[fn.length] = 'mode="'+mm[1].replace(trimRe, "")+'";';
                    q = q.replace(mm[1], "");
                }
            }
            fn[fn.length] = "return nodup(n);\n}";
            eval(fn.join(""));
            return f;
        },

        /**
         * Selects a group of elements.
         * @param {String} selector The selector/xpath query (can be a comma separated list of selectors)
         * @param {Node} root (optional) The start of the query (defaults to document).
         * @return {Array} An Array of DOM elements which match the selector. If there are
         * no matches, and empty Array is returned.
         */
        select : function(path, root, type){
            if(!root || root == document){
                root = document;
            }
            if(typeof root == "string"){
                root = document.getElementById(root);
            }
            var paths = path.split(","),
            	results = [];
            for(var i = 0, len = paths.length; i < len; i++){
                var p = paths[i].replace(trimRe, "");
                if(!cache[p]){
                    cache[p] = Ext.DomQuery.compile(p);
                    if(!cache[p]){
                        throw p + " is not a valid selector";
                    }
                }
                var result = cache[p](root);
                if(result && result != document){
                    results = results.concat(result);
                }
            }
            if(paths.length > 1){
                return nodup(results);
            }
            return results;
        },

        /**
         * Selects a single element.
         * @param {String} selector The selector/xpath query
         * @param {Node} root (optional) The start of the query (defaults to document).
         * @return {Element} The DOM element which matched the selector.
         */
        selectNode : function(path, root){
            return Ext.DomQuery.select(path, root)[0];
        },

        /**
         * Selects the value of a node, optionally replacing null with the defaultValue.
         * @param {String} selector The selector/xpath query
         * @param {Node} root (optional) The start of the query (defaults to document).
         * @param {String} defaultValue
         * @return {String}
         */
        selectValue : function(path, root, defaultValue){
            path = path.replace(trimRe, "");
            if(!valueCache[path]){
                valueCache[path] = Ext.DomQuery.compile(path, "select");
            }
            var n = valueCache[path](root), v;
            n = n[0] ? n[0] : n;
            
            if (typeof n.normalize == 'function') n.normalize();
            
            v = (n && n.firstChild ? n.firstChild.nodeValue : null);
            return ((v === null||v === undefined||v==='') ? defaultValue : v);
        },

        /**
         * Selects the value of a node, parsing integers and floats. Returns the defaultValue, or 0 if none is specified.
         * @param {String} selector The selector/xpath query
         * @param {Node} root (optional) The start of the query (defaults to document).
         * @param {Number} defaultValue
         * @return {Number}
         */
        selectNumber : function(path, root, defaultValue){
            var v = Ext.DomQuery.selectValue(path, root, defaultValue || 0);
            return parseFloat(v);
        },

        /**
         * Returns true if the passed element(s) match the passed simple selector (e.g. div.some-class or span:first-child)
         * @param {String/HTMLElement/Array} el An element id, element or array of elements
         * @param {String} selector The simple selector to test
         * @return {Boolean}
         */
        is : function(el, ss){
            if(typeof el == "string"){
                el = document.getElementById(el);
            }
            var isArray = Ext.isArray(el),
            	result = Ext.DomQuery.filter(isArray ? el : [el], ss);
            return isArray ? (result.length == el.length) : (result.length > 0);
        },

        /**
         * Filters an array of elements to only include matches of a simple selector (e.g. div.some-class or span:first-child)
         * @param {Array} el An array of elements to filter
         * @param {String} selector The simple selector to test
         * @param {Boolean} nonMatches If true, it returns the elements that DON'T match
         * the selector instead of the ones that match
         * @return {Array} An Array of DOM elements which match the selector. If there are
         * no matches, and empty Array is returned.
         */
        filter : function(els, ss, nonMatches){
            ss = ss.replace(trimRe, "");
            if(!simpleCache[ss]){
                simpleCache[ss] = Ext.DomQuery.compile(ss, "simple");
            }
            var result = simpleCache[ss](els);
            return nonMatches ? quickDiff(result, els) : result;
        },

        /**
         * Collection of matching regular expressions and code snippets.
         */
        matchers : [{
                re: /^\.([\w-]+)/,
                select: 'n = byClassName(n, null, " {1} ");'
            }, {
                re: /^\:([\w-]+)(?:\(((?:[^\s>\/]*|.*?))\))?/,
                select: 'n = byPseudo(n, "{1}", "{2}");'
            },{
                re: /^(?:([\[\{])(?:@)?([\w-]+)\s?(?:(=|.=)\s?['"]?(.*?)["']?)?[\]\}])/,
                select: 'n = byAttribute(n, "{2}", "{4}", "{3}", "{1}");'
            }, {
                re: /^#([\w-]+)/,
                select: 'n = byId(n, null, "{1}");'
            },{
                re: /^@([\w-]+)/,
                select: 'return {firstChild:{nodeValue:attrValue(n, "{1}")}};'
            }
        ],

        /**
         * Collection of operator comparison functions. The default operators are =, !=, ^=, $=, *=, %=, |= and ~=.
         * New operators can be added as long as the match the format <i>c</i>= where <i>c</i> is any character other than space, &gt; &lt;.
         */
        operators : {
            "=" : function(a, v){
                return a == v;
            },
            "!=" : function(a, v){
                return a != v;
            },
            "^=" : function(a, v){
                return a && a.substr(0, v.length) == v;
            },
            "$=" : function(a, v){
                return a && a.substr(a.length-v.length) == v;
            },
            "*=" : function(a, v){
                return a && a.indexOf(v) !== -1;
            },
            "%=" : function(a, v){
                return (a % v) == 0;
            },
            "|=" : function(a, v){
                return a && (a == v || a.substr(0, v.length+1) == v+'-');
            },
            "~=" : function(a, v){
                return a && (' '+a+' ').indexOf(' '+v+' ') != -1;
            }
        },

        /**
         * <p>Object hash of "pseudo class" filter functions which are used when filtering selections. Each function is passed
         * two parameters:</p><div class="mdetail-params"><ul>
         * <li><b>c</b> : Array<div class="sub-desc">An Array of DOM elements to filter.</div></li>
         * <li><b>v</b> : String<div class="sub-desc">The argument (if any) supplied in the selector.</div></li>
         * </ul></div>
         * <p>A filter function returns an Array of DOM elements which conform to the pseudo class.</p>
         * <p>In addition to the provided pseudo classes listed above such as <code>first-child</code> and <code>nth-child</code>,
         * developers may add additional, custom psuedo class filters to select elements according to application-specific requirements.</p>
         * <p>For example, to filter <code>&lt;a></code> elements to only return links to <i>external</i> resources:</p>
         * <code><pre>
Ext.DomQuery.pseudos.external = function(c, v){
    var r = [], ri = -1;
    for(var i = 0, ci; ci = c[i]; i++){
//      Include in result set only if it's a link to an external resource
        if(ci.hostname != location.hostname){
            r[++ri] = ci;
        }
    }
    return r;
};</pre></code>
         * Then external links could be gathered with the following statement:<code><pre>
var externalLinks = Ext.select("a:external");
</code></pre>
         */
        pseudos : {
            "first-child" : function(c){
                var r = [], ri = -1, n;
                for(var i = 0, ci; ci = n = c[i]; i++){
                    while((n = n.previousSibling) && n.nodeType != 1);
                    if(!n){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "last-child" : function(c){
                var r = [], ri = -1, n;
                for(var i = 0, ci; ci = n = c[i]; i++){
                    while((n = n.nextSibling) && n.nodeType != 1);
                    if(!n){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "nth-child" : function(c, a) {
                var r = [], ri = -1,
                	m = nthRe.exec(a == "even" && "2n" || a == "odd" && "2n+1" || !nthRe2.test(a) && "n+" + a || a),
                	f = (m[1] || 1) - 0, l = m[2] - 0;
                for(var i = 0, n; n = c[i]; i++){
                    var pn = n.parentNode;
                    if (batch != pn._batch) {
                        var j = 0;
                        for(var cn = pn.firstChild; cn; cn = cn.nextSibling){
                            if(cn.nodeType == 1){
                               cn.nodeIndex = ++j;
                            }
                        }
                        pn._batch = batch;
                    }
                    if (f == 1) {
                        if (l == 0 || n.nodeIndex == l){
                            r[++ri] = n;
                        }
                    } else if ((n.nodeIndex + l) % f == 0){
                        r[++ri] = n;
                    }
                }

                return r;
            },

            "only-child" : function(c){
                var r = [], ri = -1;;
                for(var i = 0, ci; ci = c[i]; i++){
                    if(!prev(ci) && !next(ci)){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "empty" : function(c){
                var r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    var cns = ci.childNodes, j = 0, cn, empty = true;
                    while(cn = cns[j]){
                        ++j;
                        if(cn.nodeType == 1 || cn.nodeType == 3){
                            empty = false;
                            break;
                        }
                    }
                    if(empty){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "contains" : function(c, v){
                var r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    if((ci.textContent||ci.innerText||'').indexOf(v) != -1){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "nodeValue" : function(c, v){
                var r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    if(ci.firstChild && ci.firstChild.nodeValue == v){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "checked" : function(c){
                var r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    if(ci.checked == true){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "not" : function(c, ss){
                return Ext.DomQuery.filter(c, ss, true);
            },

            "any" : function(c, selectors){
                var ss = selectors.split('|'),
                	r = [], ri = -1, s;
                for(var i = 0, ci; ci = c[i]; i++){
                    for(var j = 0; s = ss[j]; j++){
                        if(Ext.DomQuery.is(ci, s)){
                            r[++ri] = ci;
                            break;
                        }
                    }
                }
                return r;
            },

            "odd" : function(c){
                return this["nth-child"](c, "odd");
            },

            "even" : function(c){
                return this["nth-child"](c, "even");
            },

            "nth" : function(c, a){
                return c[a-1] || [];
            },

            "first" : function(c){
                return c[0] || [];
            },

            "last" : function(c){
                return c[c.length-1] || [];
            },

            "has" : function(c, ss){
                var s = Ext.DomQuery.select,
                	r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    if(s(ss, ci).length > 0){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "next" : function(c, ss){
                var is = Ext.DomQuery.is,
                	r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    var n = next(ci);
                    if(n && is(n, ss)){
                        r[++ri] = ci;
                    }
                }
                return r;
            },

            "prev" : function(c, ss){
                var is = Ext.DomQuery.is,
                	r = [], ri = -1;
                for(var i = 0, ci; ci = c[i]; i++){
                    var n = prev(ci);
                    if(n && is(n, ss)){
                        r[++ri] = ci;
                    }
                }
                return r;
            }
        }
    };
}();

/**
 * Selects an array of DOM nodes by CSS/XPath selector. Shorthand of {@link Ext.DomQuery#select}
 * @param {String} path The selector/xpath query
 * @param {Node} root (optional) The start of the query (defaults to document).
 * @return {Array}
 * @member Ext
 * @method query
 */
Ext.query = Ext.DomQuery.select;
/**
 * @class Ext.EventManager
 * Registers event handlers that want to receive a normalized EventObject instead of the standard browser event and provides
 * several useful events directly.
 * See {@link Ext.EventObject} for more details on normalized event objects.
 * @singleton
 */
Ext.EventManager = function(){
    var docReadyEvent,
        docReadyProcId,
        docReadyState = false,
        E = Ext.lib.Event,
        D = Ext.lib.Dom,
        DOC = document,
        WINDOW = window,
        IEDEFERED = "ie-deferred-loader",
        DOMCONTENTLOADED = "DOMContentLoaded",
        propRe = /^(?:scope|delay|buffer|single|stopEvent|preventDefault|stopPropagation|normalized|args|delegate)$/,
        /*
         * This cache is used to hold special js objects, the document and window, that don't have an id. We need to keep
         * a reference to them so we can look them up at a later point.
         */
        specialElCache = [];

     function getId(el){
        var id = false,
            i = 0,
            len = specialElCache.length,
            id = false,
            skip = false,
            o;
        if(el){
            if(el.getElementById || el.navigator){
                // look up the id
                for(; i < len; ++i){
                    o = specialElCache[i];
                    if(o.el === el){
                        id = o.id;
                        break;
                    }
                }
                if(!id){
                    // for browsers that support it, ensure that give the el the same id
                    id = Ext.id(el);
                    specialElCache.push({
                        id: id,
                        el: el
                    });
                    skip = true;
                }
            }else{
                id = Ext.id(el);
            }
            if(!Ext.elCache[id]){
                Ext.Element.addToCache(new Ext.Element(el), id);
                if(skip){
                    Ext.elCache[id].skipGC = true;
                }
            }
        }
        return id;
     };

    /// There is some jquery work around stuff here that isn't needed in Ext Core.
    function addListener(el, ename, fn, task, wrap, scope){
        el = Ext.getDom(el);
        var id = getId(el),
            es = Ext.elCache[id].events,
            wfn;

        wfn = E.on(el, ename, wrap);
        es[ename] = es[ename] || [];

        /* 0 = Original Function,
           1 = Event Manager Wrapped Function,
           2 = Scope,
           3 = Adapter Wrapped Function,
           4 = Buffered Task
        */
        es[ename].push([fn, wrap, scope, wfn, task]);


        // this is a workaround for jQuery and should somehow be removed from Ext Core in the future
        // without breaking ExtJS.
        if(ename == "mousewheel" && el.addEventListener){ // workaround for jQuery
            var args = ["DOMMouseScroll", wrap, false];
            el.addEventListener.apply(el, args);
            Ext.EventManager.addListener(WINDOW, 'unload', function(){
                el.removeEventListener.apply(el, args);
            });
        }
        if(ename == "mousedown" && el == document){ // fix stopped mousedowns on the document
            Ext.EventManager.stoppedMouseDownEvent.addListener(wrap);
        }
    };

    function fireDocReady(){
        if(!docReadyState){
            Ext.isReady = docReadyState = true;
            if(docReadyProcId){
                clearInterval(docReadyProcId);
            }
            if(Ext.isGecko || Ext.isOpera) {
                DOC.removeEventListener(DOMCONTENTLOADED, fireDocReady, false);
            }
            if(Ext.isIE){
                var defer = DOC.getElementById(IEDEFERED);
                if(defer){
                    defer.onreadystatechange = null;
                    defer.parentNode.removeChild(defer);
                }
            }
            if(docReadyEvent){
                docReadyEvent.fire();
                docReadyEvent.listeners = []; // clearListeners no longer compatible.  Force single: true?
            }
        }
    };

    function initDocReady(){
        var COMPLETE = "complete";

        docReadyEvent = new Ext.util.Event();
        if (Ext.isGecko || Ext.isOpera) {
            DOC.addEventListener(DOMCONTENTLOADED, fireDocReady, false);
        } else if (Ext.isIE){
            DOC.write("<s"+'cript id=' + IEDEFERED + ' defer="defer" src="/'+'/:"></s'+"cript>");
            DOC.getElementById(IEDEFERED).onreadystatechange = function(){
                if(this.readyState == COMPLETE){
                    fireDocReady();
                }
            };
        } else if (Ext.isWebKit){
            docReadyProcId = setInterval(function(){
                if(DOC.readyState == COMPLETE) {
                    fireDocReady();
                 }
            }, 10);
        }
        // no matter what, make sure it fires on load
        E.on(WINDOW, "load", fireDocReady);
    };

    function createTargeted(h, o){
        return function(){
            var args = Ext.toArray(arguments);
            if(o.target == Ext.EventObject.setEvent(args[0]).target){
                h.apply(this, args);
            }
        };
    };

    function createBuffered(h, o, task){
        return function(e){
            // create new event object impl so new events don't wipe out properties
            task.delay(o.buffer, h, null, [new Ext.EventObjectImpl(e)]);
        };
    };

    function createSingle(h, el, ename, fn, scope){
        return function(e){
            Ext.EventManager.removeListener(el, ename, fn, scope);
            h(e);
        };
    };

    function createDelayed(h, o, fn){
        return function(e){
            var task = new Ext.util.DelayedTask(h);
            if(!fn.tasks) {
                fn.tasks = [];
            }
            fn.tasks.push(task);
            task.delay(o.delay || 10, h, null, [new Ext.EventObjectImpl(e)]);
        };
    };

    function listen(element, ename, opt, fn, scope){
        var o = !Ext.isObject(opt) ? {} : opt,
            el = Ext.getDom(element), task;

        fn = fn || o.fn;
        scope = scope || o.scope;

        if(!el){
            throw "Error listening for \"" + ename + '\". Element "' + element + '" doesn\'t exist.';
        }
        function h(e){
            // prevent errors while unload occurring
            if(!Ext){// !window[xname]){  ==> can't we do this?
                return;
            }
            e = Ext.EventObject.setEvent(e);
            var t;
            if (o.delegate) {
                if(!(t = e.getTarget(o.delegate, el))){
                    return;
                }
            } else {
                t = e.target;
            }
            if (o.stopEvent) {
                e.stopEvent();
            }
            if (o.preventDefault) {
               e.preventDefault();
            }
            if (o.stopPropagation) {
                e.stopPropagation();
            }
            if (o.normalized) {
                e = e.browserEvent;
            }

            fn.call(scope || el, e, t, o);
        };
        if(o.target){
            h = createTargeted(h, o);
        }
        if(o.delay){
            h = createDelayed(h, o, fn);
        }
        if(o.single){
            h = createSingle(h, el, ename, fn, scope);
        }
        if(o.buffer){
            task = new Ext.util.DelayedTask(h);
            h = createBuffered(h, o, task);
        }

        addListener(el, ename, fn, task, h, scope);
        return h;
    };

    var pub = {
        /**
         * Appends an event handler to an element.  The shorthand version {@link #on} is equivalent.  Typically you will
         * use {@link Ext.Element#addListener} directly on an Element in favor of calling this version.
         * @param {String/HTMLElement} el The html element or id to assign the event handler to.
         * @param {String} eventName The name of the event to listen for.
         * @param {Function} handler The handler function the event invokes. This function is passed
         * the following parameters:<ul>
         * <li>evt : EventObject<div class="sub-desc">The {@link Ext.EventObject EventObject} describing the event.</div></li>
         * <li>t : Element<div class="sub-desc">The {@link Ext.Element Element} which was the target of the event.
         * Note that this may be filtered by using the <tt>delegate</tt> option.</div></li>
         * <li>o : Object<div class="sub-desc">The options object from the addListener call.</div></li>
         * </ul>
         * @param {Object} scope (optional) The scope (<b><code>this</code></b> reference) in which the handler function is executed. <b>Defaults to the Element</b>.
         * @param {Object} options (optional) An object containing handler configuration properties.
         * This may contain any of the following properties:<ul>
         * <li>scope : Object<div class="sub-desc">The scope (<b><code>this</code></b> reference) in which the handler function is executed. <b>Defaults to the Element</b>.</div></li>
         * <li>delegate : String<div class="sub-desc">A simple selector to filter the target or look for a descendant of the target</div></li>
         * <li>stopEvent : Boolean<div class="sub-desc">True to stop the event. That is stop propagation, and prevent the default action.</div></li>
         * <li>preventDefault : Boolean<div class="sub-desc">True to prevent the default action</div></li>
         * <li>stopPropagation : Boolean<div class="sub-desc">True to prevent event propagation</div></li>
         * <li>normalized : Boolean<div class="sub-desc">False to pass a browser event to the handler function instead of an Ext.EventObject</div></li>
         * <li>delay : Number<div class="sub-desc">The number of milliseconds to delay the invocation of the handler after te event fires.</div></li>
         * <li>single : Boolean<div class="sub-desc">True to add a handler to handle just the next firing of the event, and then remove itself.</div></li>
         * <li>buffer : Number<div class="sub-desc">Causes the handler to be scheduled to run in an {@link Ext.util.DelayedTask} delayed
         * by the specified number of milliseconds. If the event fires again within that time, the original
         * handler is <em>not</em> invoked, but the new handler is scheduled in its place.</div></li>
         * <li>target : Element<div class="sub-desc">Only call the handler if the event was fired on the target Element, <i>not</i> if the event was bubbled up from a child node.</div></li>
         * </ul><br>
         * <p>See {@link Ext.Element#addListener} for examples of how to use these options.</p>
         */
        addListener : function(element, eventName, fn, scope, options){
            if(Ext.isObject(eventName)){
                var o = eventName, e, val;
                for(e in o){
                    val = o[e];
                    if(!propRe.test(e)){
                        if(Ext.isFunction(val)){
                            // shared options
                            listen(element, e, o, val, o.scope);
                        }else{
                            // individual options
                            listen(element, e, val);
                        }
                    }
                }
            } else {
                listen(element, eventName, options, fn, scope);
            }
        },

        /**
         * Removes an event handler from an element.  The shorthand version {@link #un} is equivalent.  Typically
         * you will use {@link Ext.Element#removeListener} directly on an Element in favor of calling this version.
         * @param {String/HTMLElement} el The id or html element from which to remove the listener.
         * @param {String} eventName The name of the event.
         * @param {Function} fn The handler function to remove. <b>This must be a reference to the function passed into the {@link #addListener} call.</b>
         * @param {Object} scope If a scope (<b><code>this</code></b> reference) was specified when the listener was added,
         * then this must refer to the same object.
         */
        removeListener : function(el, eventName, fn, scope){
            el = Ext.getDom(el);
            var id = getId(el),
                f = el && (Ext.elCache[id].events)[eventName] || [],
                wrap, i, l, k, wf, len, fnc;

            for (i = 0, len = f.length; i < len; i++) {

                /* 0 = Original Function,
                   1 = Event Manager Wrapped Function,
                   2 = Scope,
                   3 = Adapter Wrapped Function,
                   4 = Buffered Task
                */
                if (Ext.isArray(fnc = f[i]) && fnc[0] == fn && (!scope || fnc[2] == scope)) {
                    if(fnc[4]) {
                        fnc[4].cancel();
                    }
                    k = fn.tasks && fn.tasks.length;
                    if(k) {
                        while(k--) {
                            fn.tasks[k].cancel();
                        }
                        delete fn.tasks;
                    }
                    wf = wrap = fnc[1];
                    if (E.extAdapter) {
                        wf = fnc[3];
                    }
                    E.un(el, eventName, wf);
                    f.splice(i,1);
                    if (f.length === 0) {
                        delete Ext.elCache[id].events[eventName];
                    }
                    for (k in Ext.elCache[id].events) {
                        return false;
                    }
                    Ext.elCache[id].events = {};
                    return false;
                }
            }

            // jQuery workaround that should be removed from Ext Core
            if(eventName == "mousewheel" && el.addEventListener && wrap){
                el.removeEventListener("DOMMouseScroll", wrap, false);
            }

            if(eventName == "mousedown" && el == DOC && wrap){ // fix stopped mousedowns on the document
                Ext.EventManager.stoppedMouseDownEvent.removeListener(wrap);
            }
        },

        /**
         * Removes all event handers from an element.  Typically you will use {@link Ext.Element#removeAllListeners}
         * directly on an Element in favor of calling this version.
         * @param {String/HTMLElement} el The id or html element from which to remove all event handlers.
         */
        removeAll : function(el){
            el = Ext.getDom(el);
            var id = getId(el),
                ec = Ext.elCache[id] || {},
                es = ec.events || {},
                f, i, len, ename, fn, k;

            for(ename in es){
                if(es.hasOwnProperty(ename)){
                    f = es[ename];
                    /* 0 = Original Function,
                       1 = Event Manager Wrapped Function,
                       2 = Scope,
                       3 = Adapter Wrapped Function,
                       4 = Buffered Task
                    */
                    for (i = 0, len = f.length; i < len; i++) {
                        fn = f[i];
                        if(fn[4]) {
                            fn[4].cancel();
                        }
                        if(fn[0].tasks && (k = fn[0].tasks.length)) {
                            while(k--) {
                                fn[0].tasks[k].cancel();
                            }
                            delete fn.tasks;
                        }
                        E.un(el, ename, E.extAdapter ? fn[3] : fn[1]);
                    }
                }
            }
            if (Ext.elCache[id]) {
                Ext.elCache[id].events = {};
            }
        },

        getListeners : function(el, eventName) {
            el = Ext.getDom(el);
            var id = getId(el),
                ec = Ext.elCache[id] || {},
                es = ec.events || {},
                results = [];
            if (es && es[eventName]) {
                return es[eventName];
            } else {
                return null;
            }
        },

        purgeElement : function(el, recurse, eventName) {
            el = Ext.getDom(el);
            var id = getId(el),
                ec = Ext.elCache[id] || {},
                es = ec.events || {},
                i, f, len;
            if (eventName) {
                if (es && es.hasOwnProperty(eventName)) {
                    f = es[eventName];
                    for (i = 0, len = f.length; i < len; i++) {
                        Ext.EventManager.removeListener(el, eventName, f[i][0]);
                    }
                }
            } else {
                Ext.EventManager.removeAll(el);
            }
            if (recurse && el && el.childNodes) {
                for (i = 0, len = el.childNodes.length; i < len; i++) {
                    Ext.EventManager.purgeElement(el.childNodes[i], recurse, eventName);
                }
            }
        },

        _unload : function() {
            var el;
            for (el in Ext.elCache) {
                Ext.EventManager.removeAll(el);
            }
        },
        /**
         * Adds a listener to be notified when the document is ready (before onload and before images are loaded). Can be
         * accessed shorthanded as Ext.onReady().
         * @param {Function} fn The method the event invokes.
         * @param {Object} scope (optional) The scope (<code>this</code> reference) in which the handler function executes. Defaults to the browser window.
         * @param {boolean} options (optional) Options object as passed to {@link Ext.Element#addListener}. It is recommended that the options
         * <code>{single: true}</code> be used so that the handler is removed on first invocation.
         */
        onDocumentReady : function(fn, scope, options){
            if(docReadyState){ // if it already fired
                docReadyEvent.addListener(fn, scope, options);
                docReadyEvent.fire();
                docReadyEvent.listeners = []; // clearListeners no longer compatible.  Force single: true?
            } else {
                if(!docReadyEvent) initDocReady();
                options = options || {};
                options.delay = options.delay || 1;
                docReadyEvent.addListener(fn, scope, options);
            }
        }
    };
     /**
     * Appends an event handler to an element.  Shorthand for {@link #addListener}.
     * @param {String/HTMLElement} el The html element or id to assign the event handler to
     * @param {String} eventName The name of the event to listen for.
     * @param {Function} handler The handler function the event invokes.
     * @param {Object} scope (optional) (<code>this</code> reference) in which the handler function executes. <b>Defaults to the Element</b>.
     * @param {Object} options (optional) An object containing standard {@link #addListener} options
     * @member Ext.EventManager
     * @method on
     */
    pub.on = pub.addListener;
    /**
     * Removes an event handler from an element.  Shorthand for {@link #removeListener}.
     * @param {String/HTMLElement} el The id or html element from which to remove the listener.
     * @param {String} eventName The name of the event.
     * @param {Function} fn The handler function to remove. <b>This must be a reference to the function passed into the {@link #on} call.</b>
     * @param {Object} scope If a scope (<b><code>this</code></b> reference) was specified when the listener was added,
     * then this must refer to the same object.
     * @member Ext.EventManager
     * @method un
     */
    pub.un = pub.removeListener;

    pub.stoppedMouseDownEvent = new Ext.util.Event();
    return pub;
}();
/**
  * Adds a listener to be notified when the document is ready (before onload and before images are loaded). Shorthand of {@link Ext.EventManager#onDocumentReady}.
  * @param {Function} fn The method the event invokes.
  * @param {Object} scope (optional) The scope (<code>this</code> reference) in which the handler function executes. Defaults to the browser window.
  * @param {boolean} options (optional) Options object as passed to {@link Ext.Element#addListener}. It is recommended that the options
  * <code>{single: true}</code> be used so that the handler is removed on first invocation.
  * @member Ext
  * @method onReady
 */
Ext.onReady = Ext.EventManager.onDocumentReady;


//Initialize doc classes
(function(){

    var initExtCss = function(){
        // find the body element
        var bd = document.body || document.getElementsByTagName('body')[0];
        if(!bd){ return false; }
        var cls = [' ',
                Ext.isIE ? "ext-ie " + (Ext.isIE6 ? 'ext-ie6' : (Ext.isIE7 ? 'ext-ie7' : 'ext-ie8'))
                : Ext.isGecko ? "ext-gecko " + (Ext.isGecko2 ? 'ext-gecko2' : 'ext-gecko3')
                : Ext.isOpera ? "ext-opera"
                : Ext.isWebKit ? "ext-webkit" : ""];

        if(Ext.isSafari){
            cls.push("ext-safari " + (Ext.isSafari2 ? 'ext-safari2' : (Ext.isSafari3 ? 'ext-safari3' : 'ext-safari4')));
        }else if(Ext.isChrome){
            cls.push("ext-chrome");
        }

        if(Ext.isMac){
            cls.push("ext-mac");
        }
        if(Ext.isLinux){
            cls.push("ext-linux");
        }

        if(Ext.isStrict || Ext.isBorderBox){ // add to the parent to allow for selectors like ".ext-strict .ext-ie"
            var p = bd.parentNode;
            if(p){
                p.className += Ext.isStrict ? ' ext-strict' : ' ext-border-box';
            }
        }
        bd.className += cls.join(' ');
        return true;
    }

    if(!initExtCss()){
        Ext.onReady(initExtCss);
    }
})();


/**
 * @class Ext.EventObject
 * Just as {@link Ext.Element} wraps around a native DOM node, Ext.EventObject
 * wraps the browser's native event-object normalizing cross-browser differences,
 * such as which mouse button is clicked, keys pressed, mechanisms to stop
 * event-propagation along with a method to prevent default actions from taking place.
 * <p>For example:</p>
 * <pre><code>
function handleClick(e, t){ // e is not a standard event object, it is a Ext.EventObject
    e.preventDefault();
    var target = e.getTarget(); // same as t (the target HTMLElement)
    ...
}
var myDiv = {@link Ext#get Ext.get}("myDiv");  // get reference to an {@link Ext.Element}
myDiv.on(         // 'on' is shorthand for addListener
    "click",      // perform an action on click of myDiv
    handleClick   // reference to the action handler
);
// other methods to do the same:
Ext.EventManager.on("myDiv", 'click', handleClick);
Ext.EventManager.addListener("myDiv", 'click', handleClick);
 </code></pre>
 * @singleton
 */
Ext.EventObject = function(){
    var E = Ext.lib.Event,
        // safari keypress events for special keys return bad keycodes
        safariKeys = {
            3 : 13, // enter
            63234 : 37, // left
            63235 : 39, // right
            63232 : 38, // up
            63233 : 40, // down
            63276 : 33, // page up
            63277 : 34, // page down
            63272 : 46, // delete
            63273 : 36, // home
            63275 : 35  // end
        },
        // normalize button clicks
        btnMap = Ext.isIE ? {1:0,4:1,2:2} :
                (Ext.isWebKit ? {1:0,2:1,3:2} : {0:0,1:1,2:2});

    Ext.EventObjectImpl = function(e){
        if(e){
            this.setEvent(e.browserEvent || e);
        }
    };

    Ext.EventObjectImpl.prototype = {
           /** @private */
        setEvent : function(e){
            var me = this;
            if(e == me || (e && e.browserEvent)){ // already wrapped
                return e;
            }
            me.browserEvent = e;
            if(e){
                // normalize buttons
                me.button = e.button ? btnMap[e.button] : (e.which ? e.which - 1 : -1);
                if(e.type == 'click' && me.button == -1){
                    me.button = 0;
                }
                me.type = e.type;
                me.shiftKey = e.shiftKey;
                // mac metaKey behaves like ctrlKey
                me.ctrlKey = e.ctrlKey || e.metaKey || false;
                me.altKey = e.altKey;
                // in getKey these will be normalized for the mac
                me.keyCode = e.keyCode;
                me.charCode = e.charCode;
                // cache the target for the delayed and or buffered events
                me.target = E.getTarget(e);
                // same for XY
                me.xy = E.getXY(e);
            }else{
                me.button = -1;
                me.shiftKey = false;
                me.ctrlKey = false;
                me.altKey = false;
                me.keyCode = 0;
                me.charCode = 0;
                me.target = null;
                me.xy = [0, 0];
            }
            return me;
        },

        /**
         * Stop the event (preventDefault and stopPropagation)
         */
        stopEvent : function(){
            var me = this;
            if(me.browserEvent){
                if(me.browserEvent.type == 'mousedown'){
                    Ext.EventManager.stoppedMouseDownEvent.fire(me);
                }
                E.stopEvent(me.browserEvent);
            }
        },

        /**
         * Prevents the browsers default handling of the event.
         */
        preventDefault : function(){
            if(this.browserEvent){
                E.preventDefault(this.browserEvent);
            }
        },

        /**
         * Cancels bubbling of the event.
         */
        stopPropagation : function(){
            var me = this;
            if(me.browserEvent){
                if(me.browserEvent.type == 'mousedown'){
                    Ext.EventManager.stoppedMouseDownEvent.fire(me);
                }
                E.stopPropagation(me.browserEvent);
            }
        },

        /**
         * Gets the character code for the event.
         * @return {Number}
         */
        getCharCode : function(){
            return this.charCode || this.keyCode;
        },

        /**
         * Returns a normalized keyCode for the event.
         * @return {Number} The key code
         */
        getKey : function(){
            return this.normalizeKey(this.keyCode || this.charCode)
        },

        // private
        normalizeKey: function(k){
            return Ext.isSafari ? (safariKeys[k] || k) : k;
        },

        /**
         * Gets the x coordinate of the event.
         * @return {Number}
         */
        getPageX : function(){
            return this.xy[0];
        },

        /**
         * Gets the y coordinate of the event.
         * @return {Number}
         */
        getPageY : function(){
            return this.xy[1];
        },

        /**
         * Gets the page coordinates of the event.
         * @return {Array} The xy values like [x, y]
         */
        getXY : function(){
            return this.xy;
        },

        /**
         * Gets the target for the event.
         * @param {String} selector (optional) A simple selector to filter the target or look for an ancestor of the target
         * @param {Number/Mixed} maxDepth (optional) The max depth to
                search as a number or element (defaults to 10 || document.body)
         * @param {Boolean} returnEl (optional) True to return a Ext.Element object instead of DOM node
         * @return {HTMLelement}
         */
        getTarget : function(selector, maxDepth, returnEl){
            return selector ? Ext.fly(this.target).findParent(selector, maxDepth, returnEl) : (returnEl ? Ext.get(this.target) : this.target);
        },

        /**
         * Gets the related target.
         * @return {HTMLElement}
         */
        getRelatedTarget : function(){
            return this.browserEvent ? E.getRelatedTarget(this.browserEvent) : null;
        },

        /**
         * Normalizes mouse wheel delta across browsers
         * @return {Number} The delta
         */
        getWheelDelta : function(){
            var e = this.browserEvent;
            var delta = 0;
            if(e.wheelDelta){ /* IE/Opera. */
                delta = e.wheelDelta/120;
            }else if(e.detail){ /* Mozilla case. */
                delta = -e.detail/3;
            }
            return delta;
        },

        /**
        * Returns true if the target of this event is a child of el.  Unless the allowEl parameter is set, it will return false if if the target is el.
        * Example usage:<pre><code>
        // Handle click on any child of an element
        Ext.getBody().on('click', function(e){
            if(e.within('some-el')){
                alert('Clicked on a child of some-el!');
            }
        });

        // Handle click directly on an element, ignoring clicks on child nodes
        Ext.getBody().on('click', function(e,t){
            if((t.id == 'some-el') && !e.within(t, true)){
                alert('Clicked directly on some-el!');
            }
        });
        </code></pre>
         * @param {Mixed} el The id, DOM element or Ext.Element to check
         * @param {Boolean} related (optional) true to test if the related target is within el instead of the target
         * @param {Boolean} allowEl {optional} true to also check if the passed element is the target or related target
         * @return {Boolean}
         */
        within : function(el, related, allowEl){
            if(el){
                var t = this[related ? "getRelatedTarget" : "getTarget"]();
                return t && ((allowEl ? (t == Ext.getDom(el)) : false) || Ext.fly(el).contains(t));
            }
            return false;
        }
     };

    return new Ext.EventObjectImpl();
}();
/**
 * @class Ext.Element
 * <p>Encapsulates a DOM element, adding simple DOM manipulation facilities, normalizing for browser differences.</p>
 * <p>All instances of this class inherit the methods of {@link Ext.Fx} making visual effects easily available to all DOM elements.</p>
 * <p>Note that the events documented in this class are not Ext events, they encapsulate browser events. To
 * access the underlying browser event, see {@link Ext.EventObject#browserEvent}. Some older
 * browsers may not support the full range of events. Which events are supported is beyond the control of ExtJs.</p>
 * Usage:<br>
<pre><code>
// by id
var el = Ext.get("my-div");

// by DOM element reference
var el = Ext.get(myDivElement);
</code></pre>
 * <b>Animations</b><br />
 * <p>When an element is manipulated, by default there is no animation.</p>
 * <pre><code>
var el = Ext.get("my-div");

// no animation
el.setWidth(100);
 * </code></pre>
 * <p>Many of the functions for manipulating an element have an optional "animate" parameter.  This
 * parameter can be specified as boolean (<tt>true</tt>) for default animation effects.</p>
 * <pre><code>
// default animation
el.setWidth(100, true);
 * </code></pre>
 *
 * <p>To configure the effects, an object literal with animation options to use as the Element animation
 * configuration object can also be specified. Note that the supported Element animation configuration
 * options are a subset of the {@link Ext.Fx} animation options specific to Fx effects.  The supported
 * Element animation configuration options are:</p>
<pre>
Option    Default   Description
--------- --------  ---------------------------------------------
{@link Ext.Fx#duration duration}  .35       The duration of the animation in seconds
{@link Ext.Fx#easing easing}    easeOut   The easing method
{@link Ext.Fx#callback callback}  none      A function to execute when the anim completes
{@link Ext.Fx#scope scope}     this      The scope (this) of the callback function
</pre>
 *
 * <pre><code>
// Element animation options object
var opt = {
    {@link Ext.Fx#duration duration}: 1,
    {@link Ext.Fx#easing easing}: 'elasticIn',
    {@link Ext.Fx#callback callback}: this.foo,
    {@link Ext.Fx#scope scope}: this
};
// animation with some options set
el.setWidth(100, opt);
 * </code></pre>
 * <p>The Element animation object being used for the animation will be set on the options
 * object as "anim", which allows you to stop or manipulate the animation. Here is an example:</p>
 * <pre><code>
// using the "anim" property to get the Anim object
if(opt.anim.isAnimated()){
    opt.anim.stop();
}
 * </code></pre>
 * <p>Also see the <tt>{@link #animate}</tt> method for another animation technique.</p>
 * <p><b> Composite (Collections of) Elements</b></p>
 * <p>For working with collections of Elements, see {@link Ext.CompositeElement}</p>
 * @constructor Create a new Element directly.
 * @param {String/HTMLElement} element
 * @param {Boolean} forceNew (optional) By default the constructor checks to see if there is already an instance of this element in the cache and if there is it returns the same instance. This will skip that check (useful for extending this class).
 */
(function(){
var DOC = document;

Ext.Element = function(element, forceNew){
    var dom = typeof element == "string" ?
              DOC.getElementById(element) : element,
        id;

    if(!dom) return null;

    id = dom.id;

    if(!forceNew && id && Ext.elCache[id]){ // element object already exists
        return Ext.elCache[id].el;
    }

    /**
     * The DOM element
     * @type HTMLElement
     */
    this.dom = dom;

    /**
     * The DOM element ID
     * @type String
     */
    this.id = id || Ext.id(dom);
};

var D = Ext.lib.Dom,
    DH = Ext.DomHelper,
    E = Ext.lib.Event,
    A = Ext.lib.Anim,
    El = Ext.Element,
    EC = Ext.elCache;

El.prototype = {
    /**
     * Sets the passed attributes as attributes of this element (a style attribute can be a string, object or function)
     * @param {Object} o The object with the attributes
     * @param {Boolean} useSet (optional) false to override the default setAttribute to use expandos.
     * @return {Ext.Element} this
     */
    set : function(o, useSet){
        var el = this.dom,
            attr,
            val,
            useSet = (useSet !== false) && !!el.setAttribute;

        for(attr in o){
            if (o.hasOwnProperty(attr)) {
                val = o[attr];
                if (attr == 'style') {
                    DH.applyStyles(el, val);
                } else if (attr == 'cls') {
                    el.className = val;
                } else if (useSet) {
                    el.setAttribute(attr, val);
                } else {
                    el[attr] = val;
                }
            }
        }
        return this;
    },

//  Mouse events
    /**
     * @event click
     * Fires when a mouse click is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event contextmenu
     * Fires when a right click is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event dblclick
     * Fires when a mouse double click is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mousedown
     * Fires when a mousedown is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mouseup
     * Fires when a mouseup is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mouseover
     * Fires when a mouseover is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mousemove
     * Fires when a mousemove is detected with the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mouseout
     * Fires when a mouseout is detected with the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mouseenter
     * Fires when the mouse enters the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event mouseleave
     * Fires when the mouse leaves the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */

//  Keyboard events
    /**
     * @event keypress
     * Fires when a keypress is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event keydown
     * Fires when a keydown is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event keyup
     * Fires when a keyup is detected within the element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */


//  HTML frame/object events
    /**
     * @event load
     * Fires when the user agent finishes loading all content within the element. Only supported by window, frames, objects and images.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event unload
     * Fires when the user agent removes all content from a window or frame. For elements, it fires when the target element or any of its content has been removed.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event abort
     * Fires when an object/image is stopped from loading before completely loaded.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event error
     * Fires when an object/image/frame cannot be loaded properly.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event resize
     * Fires when a document view is resized.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event scroll
     * Fires when a document view is scrolled.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */

//  Form events
    /**
     * @event select
     * Fires when a user selects some text in a text field, including input and textarea.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event change
     * Fires when a control loses the input focus and its value has been modified since gaining focus.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event submit
     * Fires when a form is submitted.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event reset
     * Fires when a form is reset.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event focus
     * Fires when an element receives focus either via the pointing device or by tab navigation.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event blur
     * Fires when an element loses focus either via the pointing device or by tabbing navigation.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */

//  User Interface events
    /**
     * @event DOMFocusIn
     * Where supported. Similar to HTML focus event, but can be applied to any focusable element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMFocusOut
     * Where supported. Similar to HTML blur event, but can be applied to any focusable element.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMActivate
     * Where supported. Fires when an element is activated, for instance, through a mouse click or a keypress.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */

//  DOM Mutation events
    /**
     * @event DOMSubtreeModified
     * Where supported. Fires when the subtree is modified.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMNodeInserted
     * Where supported. Fires when a node has been added as a child of another node.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMNodeRemoved
     * Where supported. Fires when a descendant node of the element is removed.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMNodeRemovedFromDocument
     * Where supported. Fires when a node is being removed from a document.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMNodeInsertedIntoDocument
     * Where supported. Fires when a node is being inserted into a document.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMAttrModified
     * Where supported. Fires when an attribute has been modified.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */
    /**
     * @event DOMCharacterDataModified
     * Where supported. Fires when the character data has been modified.
     * @param {Ext.EventObject} e The {@link Ext.EventObject} encapsulating the DOM event.
     * @param {HtmlElement} t The target of the event.
     * @param {Object} o The options configuration passed to the {@link #addListener} call.
     */

    /**
     * The default unit to append to CSS values where a unit isn't provided (defaults to px).
     * @type String
     */
    defaultUnit : "px",

    /**
     * Returns true if this element matches the passed simple selector (e.g. div.some-class or span:first-child)
     * @param {String} selector The simple selector to test
     * @return {Boolean} True if this element matches the selector, else false
     */
    is : function(simpleSelector){
        return Ext.DomQuery.is(this.dom, simpleSelector);
    },

    /**
     * Tries to focus the element. Any exceptions are caught and ignored.
     * @param {Number} defer (optional) Milliseconds to defer the focus
     * @return {Ext.Element} this
     */
    focus : function(defer, /* private */ dom) {
        var me = this,
            dom = dom || me.dom;
        try{
            if(Number(defer)){
                me.focus.defer(defer, null, [null, dom]);
            }else{
                dom.focus();
            }
        }catch(e){}
        return me;
    },

    /**
     * Tries to blur the element. Any exceptions are caught and ignored.
     * @return {Ext.Element} this
     */
    blur : function() {
        try{
            this.dom.blur();
        }catch(e){}
        return this;
    },

    /**
     * Returns the value of the "value" attribute
     * @param {Boolean} asNumber true to parse the value as a number
     * @return {String/Number}
     */
    getValue : function(asNumber){
        var val = this.dom.value;
        return asNumber ? parseInt(val, 10) : val;
    },

    /**
     * Appends an event handler to this element.  The shorthand version {@link #on} is equivalent.
     * @param {String} eventName The name of event to handle.
     * @param {Function} fn The handler function the event invokes. This function is passed
     * the following parameters:<ul>
     * <li><b>evt</b> : EventObject<div class="sub-desc">The {@link Ext.EventObject EventObject} describing the event.</div></li>
     * <li><b>el</b> : HtmlElement<div class="sub-desc">The DOM element which was the target of the event.
     * Note that this may be filtered by using the <tt>delegate</tt> option.</div></li>
     * <li><b>o</b> : Object<div class="sub-desc">The options object from the addListener call.</div></li>
     * </ul>
     * @param {Object} scope (optional) The scope (<code><b>this</b></code> reference) in which the handler function is executed.
     * <b>If omitted, defaults to this Element.</b>.
     * @param {Object} options (optional) An object containing handler configuration properties.
     * This may contain any of the following properties:<ul>
     * <li><b>scope</b> Object : <div class="sub-desc">The scope (<code><b>this</b></code> reference) in which the handler function is executed.
     * <b>If omitted, defaults to this Element.</b></div></li>
     * <li><b>delegate</b> String: <div class="sub-desc">A simple selector to filter the target or look for a descendant of the target. See below for additional details.</div></li>
     * <li><b>stopEvent</b> Boolean: <div class="sub-desc">True to stop the event. That is stop propagation, and prevent the default action.</div></li>
     * <li><b>preventDefault</b> Boolean: <div class="sub-desc">True to prevent the default action</div></li>
     * <li><b>stopPropagation</b> Boolean: <div class="sub-desc">True to prevent event propagation</div></li>
     * <li><b>normalized</b> Boolean: <div class="sub-desc">False to pass a browser event to the handler function instead of an Ext.EventObject</div></li>
     * <li><b>target</b> Ext.Element: <div class="sub-desc">Only call the handler if the event was fired on the target Element, <i>not</i> if the event was bubbled up from a child node.</div></li>
     * <li><b>delay</b> Number: <div class="sub-desc">The number of milliseconds to delay the invocation of the handler after the event fires.</div></li>
     * <li><b>single</b> Boolean: <div class="sub-desc">True to add a handler to handle just the next firing of the event, and then remove itself.</div></li>
     * <li><b>buffer</b> Number: <div class="sub-desc">Causes the handler to be scheduled to run in an {@link Ext.util.DelayedTask} delayed
     * by the specified number of milliseconds. If the event fires again within that time, the original
     * handler is <em>not</em> invoked, but the new handler is scheduled in its place.</div></li>
     * </ul><br>
     * <p>
     * <b>Combining Options</b><br>
     * In the following examples, the shorthand form {@link #on} is used rather than the more verbose
     * addListener.  The two are equivalent.  Using the options argument, it is possible to combine different
     * types of listeners:<br>
     * <br>
     * A delayed, one-time listener that auto stops the event and adds a custom argument (forumId) to the
     * options object. The options object is available as the third parameter in the handler function.<div style="margin: 5px 20px 20px;">
     * Code:<pre><code>
el.on('click', this.onClick, this, {
    single: true,
    delay: 100,
    stopEvent : true,
    forumId: 4
});</code></pre></p>
     * <p>
     * <b>Attaching multiple handlers in 1 call</b><br>
     * The method also allows for a single argument to be passed which is a config object containing properties
     * which specify multiple handlers.</p>
     * <p>
     * Code:<pre><code>
el.on({
    'click' : {
        fn: this.onClick,
        scope: this,
        delay: 100
    },
    'mouseover' : {
        fn: this.onMouseOver,
        scope: this
    },
    'mouseout' : {
        fn: this.onMouseOut,
        scope: this
    }
});</code></pre>
     * <p>
     * Or a shorthand syntax:<br>
     * Code:<pre><code></p>
el.on({
    'click' : this.onClick,
    'mouseover' : this.onMouseOver,
    'mouseout' : this.onMouseOut,
    scope: this
});
     * </code></pre></p>
     * <p><b>delegate</b></p>
     * <p>This is a configuration option that you can pass along when registering a handler for
     * an event to assist with event delegation. Event delegation is a technique that is used to
     * reduce memory consumption and prevent exposure to memory-leaks. By registering an event
     * for a container element as opposed to each element within a container. By setting this
     * configuration option to a simple selector, the target element will be filtered to look for
     * a descendant of the target.
     * For example:<pre><code>
// using this markup:
&lt;div id='elId'>
    &lt;p id='p1'>paragraph one&lt;/p>
    &lt;p id='p2' class='clickable'>paragraph two&lt;/p>
    &lt;p id='p3'>paragraph three&lt;/p>
&lt;/div>
// utilize event delegation to registering just one handler on the container element:
el = Ext.get('elId');
el.on(
    'click',
    function(e,t) {
        // handle click
        console.info(t.id); // 'p2'
    },
    this,
    {
        // filter the target element to be a descendant with the class 'clickable'
        delegate: '.clickable'
    }
);
     * </code></pre></p>
     * @return {Ext.Element} this
     */
    addListener : function(eventName, fn, scope, options){
        Ext.EventManager.on(this.dom,  eventName, fn, scope || this, options);
        return this;
    },

    /**
     * Removes an event handler from this element.  The shorthand version {@link #un} is equivalent.
     * <b>Note</b>: if a <i>scope</i> was explicitly specified when {@link #addListener adding} the
     * listener, the same scope must be specified here.
     * Example:
     * <pre><code>
el.removeListener('click', this.handlerFn);
// or
el.un('click', this.handlerFn);
</code></pre>
     * @param {String} eventName The name of the event from which to remove the handler.
     * @param {Function} fn The handler function to remove. <b>This must be a reference to the function passed into the {@link #addListener} call.</b>
     * @param {Object} scope If a scope (<b><code>this</code></b> reference) was specified when the listener was added,
     * then this must refer to the same object.
     * @return {Ext.Element} this
     */
    removeListener : function(eventName, fn, scope){
        Ext.EventManager.removeListener(this.dom,  eventName, fn, scope || this);
        return this;
    },

    /**
     * Removes all previous added listeners from this element
     * @return {Ext.Element} this
     */
    removeAllListeners : function(){
        Ext.EventManager.removeAll(this.dom);
        return this;
    },

    /**
     * Recursively removes all previous added listeners from this element and its children
     * @return {Ext.Element} this
     */
    purgeAllListeners : function() {
        Ext.EventManager.purgeElement(this, true);
        return this;
    },
    /**
     * @private Test if size has a unit, otherwise appends the default
     */
    addUnits : function(size){
        if(size === "" || size == "auto" || size === undefined){
            size = size || '';
        } else if(!isNaN(size) || !unitPattern.test(size)){
            size = size + (this.defaultUnit || 'px');
        }
        return size;
    },

    /**
     * <p>Updates the <a href="http://developer.mozilla.org/en/DOM/element.innerHTML">innerHTML</a> of this Element
     * from a specified URL. Note that this is subject to the <a href="http://en.wikipedia.org/wiki/Same_origin_policy">Same Origin Policy</a></p>
     * <p>Updating innerHTML of an element will <b>not</b> execute embedded <tt>&lt;script></tt> elements. This is a browser restriction.</p>
     * @param {Mixed} options. Either a sring containing the URL from which to load the HTML, or an {@link Ext.Ajax#request} options object specifying
     * exactly how to request the HTML.
     * @return {Ext.Element} this
     */
    load : function(url, params, cb){
        Ext.Ajax.request(Ext.apply({
            params: params,
            url: url.url || url,
            callback: cb,
            el: this.dom,
            indicatorText: url.indicatorText || ''
        }, Ext.isObject(url) ? url : {}));
        return this;
    },

    /**
     * Tests various css rules/browsers to determine if this element uses a border box
     * @return {Boolean}
     */
    isBorderBox : function(){
        return noBoxAdjust[(this.dom.tagName || "").toLowerCase()] || Ext.isBorderBox;
    },

    /**
     * <p>Removes this element's dom reference.  Note that event and cache removal is handled at {@link Ext#removeNode}</p>
     */
    remove : function(){
        var me = this,
            dom = me.dom;

        if (dom) {
            delete me.dom;
            Ext.removeNode(dom);
        }
    },

    /**
     * Sets up event handlers to call the passed functions when the mouse is moved into and out of the Element.
     * @param {Function} overFn The function to call when the mouse enters the Element.
     * @param {Function} outFn The function to call when the mouse leaves the Element.
     * @param {Object} scope (optional) The scope (<code>this</code> reference) in which the functions are executed. Defaults to the Element's DOM element.
     * @param {Object} options (optional) Options for the listener. See {@link Ext.util.Observable#addListener the <tt>options</tt> parameter}.
     * @return {Ext.Element} this
     */
    hover : function(overFn, outFn, scope, options){
        var me = this;
        me.on('mouseenter', overFn, scope || me.dom, options);
        me.on('mouseleave', outFn, scope || me.dom, options);
        return me;
    },

    /**
     * Returns true if this element is an ancestor of the passed element
     * @param {HTMLElement/String} el The element to check
     * @return {Boolean} True if this element is an ancestor of el, else false
     */
    contains : function(el){
        return !el ? false : Ext.lib.Dom.isAncestor(this.dom, el.dom ? el.dom : el);
    },

    /**
     * Returns the value of a namespaced attribute from the element's underlying DOM node.
     * @param {String} namespace The namespace in which to look for the attribute
     * @param {String} name The attribute name
     * @return {String} The attribute value
     * @deprecated
     */
    getAttributeNS : function(ns, name){
        return this.getAttribute(name, ns);
    },

    /**
     * Returns the value of an attribute from the element's underlying DOM node.
     * @param {String} name The attribute name
     * @param {String} namespace (optional) The namespace in which to look for the attribute
     * @return {String} The attribute value
     */
    getAttribute : Ext.isIE ? function(name, ns){
        var d = this.dom,
            type = typeof d[ns + ":" + name];

        if(['undefined', 'unknown'].indexOf(type) == -1){
            return d[ns + ":" + name];
        }
        return d[name];
    } : function(name, ns){
        var d = this.dom;
        return d.getAttributeNS(ns, name) || d.getAttribute(ns + ":" + name) || d.getAttribute(name) || d[name];
    },

    /**
    * Update the innerHTML of this element
    * @param {String} html The new HTML
    * @return {Ext.Element} this
     */
    update : function(html) {
        if (this.dom) {
            this.dom.innerHTML = html;
        }
        return this;
    }
};

var ep = El.prototype;

El.addMethods = function(o){
   Ext.apply(ep, o);
};

/**
 * Appends an event handler (shorthand for {@link #addListener}).
 * @param {String} eventName The name of event to handle.
 * @param {Function} fn The handler function the event invokes.
 * @param {Object} scope (optional) The scope (<code>this</code> reference) in which the handler function is executed.
 * @param {Object} options (optional) An object containing standard {@link #addListener} options
 * @member Ext.Element
 * @method on
 */
ep.on = ep.addListener;

/**
 * Removes an event handler from this element (see {@link #removeListener} for additional notes).
 * @param {String} eventName The name of the event from which to remove the handler.
 * @param {Function} fn The handler function to remove. <b>This must be a reference to the function passed into the {@link #addListener} call.</b>
 * @param {Object} scope If a scope (<b><code>this</code></b> reference) was specified when the listener was added,
 * then this must refer to the same object.
 * @return {Ext.Element} this
 * @member Ext.Element
 * @method un
 */
ep.un = ep.removeListener;

/**
 * true to automatically adjust width and height settings for box-model issues (default to true)
 */
ep.autoBoxAdjust = true;

// private
var unitPattern = /\d+(px|em|%|en|ex|pt|in|cm|mm|pc)$/i,
    docEl;

/**
 * @private
 */

/**
 * Retrieves Ext.Element objects.
 * <p><b>This method does not retrieve {@link Ext.Component Component}s.</b> This method
 * retrieves Ext.Element objects which encapsulate DOM elements. To retrieve a Component by
 * its ID, use {@link Ext.ComponentMgr#get}.</p>
 * <p>Uses simple caching to consistently return the same object. Automatically fixes if an
 * object was recreated with the same id via AJAX or DOM.</p>
 * @param {Mixed} el The id of the node, a DOM Node or an existing Element.
 * @return {Element} The Element object (or null if no matching element was found)
 * @static
 * @member Ext.Element
 * @method get
 */
El.get = function(el){
    var ex,
        elm,
        id;
    if(!el){ return null; }
    if (typeof el == "string") { // element id
        if (!(elm = DOC.getElementById(el))) {
            return null;
        }
        if (EC[el] && EC[el].el) {
            ex = EC[el].el;
            ex.dom = elm;
        } else {
            ex = El.addToCache(new El(elm));
        }
        return ex;
    } else if (el.tagName) { // dom element
        if(!(id = el.id)){
            id = Ext.id(el);
        }
        if (EC[id] && EC[id].el) {
            ex = EC[id].el;
            ex.dom = el;
        } else {
            ex = El.addToCache(new El(el));
        }
        return ex;
    } else if (el instanceof El) {
        if(el != docEl){
            el.dom = DOC.getElementById(el.id) || el.dom; // refresh dom element in case no longer valid,
                                                          // catch case where it hasn't been appended
        }
        return el;
    } else if(el.isComposite) {
        return el;
    } else if(Ext.isArray(el)) {
        return El.select(el);
    } else if(el == DOC) {
        // create a bogus element object representing the document object
        if(!docEl){
            var f = function(){};
            f.prototype = El.prototype;
            docEl = new f();
            docEl.dom = DOC;
        }
        return docEl;
    }
    return null;
};

El.addToCache = function(el, id){
    id = id || el.id;    
    EC[id] = {
        el:  el,
        data: {},
        events: {}
    };
    return el;
};

// private method for getting and setting element data
El.data = function(el, key, value){
    el = El.get(el);
    if (!el) {
        return null;
    }
    var c = EC[el.id].data;
    if(arguments.length == 2){
        return c[key];
    }else{
        return (c[key] = value);
    }
};

// private
// Garbage collection - uncache elements/purge listeners on orphaned elements
// so we don't hold a reference and cause the browser to retain them
function garbageCollect(){
    if(!Ext.enableGarbageCollector){
        clearInterval(El.collectorThreadId);
    } else {
        var eid,
            el,
            d,
            o;

        for(eid in EC){
            o = EC[eid];
            if(o.skipGC){
                continue;
            }
            el = o.el;
            d = el.dom;
            // -------------------------------------------------------
            // Determining what is garbage:
            // -------------------------------------------------------
            // !d
            // dom node is null, definitely garbage
            // -------------------------------------------------------
            // !d.parentNode
            // no parentNode == direct orphan, definitely garbage
            // -------------------------------------------------------
            // !d.offsetParent && !document.getElementById(eid)
            // display none elements have no offsetParent so we will
            // also try to look it up by it's id. However, check
            // offsetParent first so we don't do unneeded lookups.
            // This enables collection of elements that are not orphans
            // directly, but somewhere up the line they have an orphan
            // parent.
            // -------------------------------------------------------
            if(!d || !d.parentNode || (!d.offsetParent && !DOC.getElementById(eid))){
                if(Ext.enableListenerCollection){
                    Ext.EventManager.removeAll(d);
                }
                delete EC[eid];
            }
        }
        // Cleanup IE Object leaks
        if (Ext.isIE) {
            var t = {};
            for (eid in EC) {
                t[eid] = EC[eid];
            }
            EC = Ext.elCache = t;
        }
    }
}
El.collectorThreadId = setInterval(garbageCollect, 30000);

var flyFn = function(){};
flyFn.prototype = El.prototype;

// dom is optional
El.Flyweight = function(dom){
    this.dom = dom;
};

El.Flyweight.prototype = new flyFn();
El.Flyweight.prototype.isFlyweight = true;
El._flyweights = {};

/**
 * <p>Gets the globally shared flyweight Element, with the passed node as the active element. Do not store a reference to this element -
 * the dom node can be overwritten by other code. Shorthand of {@link Ext.Element#fly}</p>
 * <p>Use this to make one-time references to DOM elements which are not going to be accessed again either by
 * application code, or by Ext's classes. If accessing an element which will be processed regularly, then {@link Ext#get}
 * will be more appropriate to take advantage of the caching provided by the Ext.Element class.</p>
 * @param {String/HTMLElement} el The dom node or id
 * @param {String} named (optional) Allows for creation of named reusable flyweights to prevent conflicts
 * (e.g. internally Ext uses "_global")
 * @return {Element} The shared Element object (or null if no matching element was found)
 * @member Ext.Element
 * @method fly
 */
El.fly = function(el, named){
    var ret = null;
    named = named || '_global';

    if (el = Ext.getDom(el)) {
        (El._flyweights[named] = El._flyweights[named] || new El.Flyweight()).dom = el;
        ret = El._flyweights[named];
    }
    return ret;
};

/**
 * Retrieves Ext.Element objects.
 * <p><b>This method does not retrieve {@link Ext.Component Component}s.</b> This method
 * retrieves Ext.Element objects which encapsulate DOM elements. To retrieve a Component by
 * its ID, use {@link Ext.ComponentMgr#get}.</p>
 * <p>Uses simple caching to consistently return the same object. Automatically fixes if an
 * object was recreated with the same id via AJAX or DOM.</p>
 * Shorthand of {@link Ext.Element#get}
 * @param {Mixed} el The id of the node, a DOM Node or an existing Element.
 * @return {Element} The Element object (or null if no matching element was found)
 * @member Ext
 * @method get
 */
Ext.get = El.get;

/**
 * <p>Gets the globally shared flyweight Element, with the passed node as the active element. Do not store a reference to this element -
 * the dom node can be overwritten by other code. Shorthand of {@link Ext.Element#fly}</p>
 * <p>Use this to make one-time references to DOM elements which are not going to be accessed again either by
 * application code, or by Ext's classes. If accessing an element which will be processed regularly, then {@link Ext#get}
 * will be more appropriate to take advantage of the caching provided by the Ext.Element class.</p>
 * @param {String/HTMLElement} el The dom node or id
 * @param {String} named (optional) Allows for creation of named reusable flyweights to prevent conflicts
 * (e.g. internally Ext uses "_global")
 * @return {Element} The shared Element object (or null if no matching element was found)
 * @member Ext
 * @method fly
 */
Ext.fly = El.fly;

// speedy lookup for elements never to box adjust
var noBoxAdjust = Ext.isStrict ? {
    select:1
} : {
    input:1, select:1, textarea:1
};
if(Ext.isIE || Ext.isGecko){
    noBoxAdjust['button'] = 1;
}


Ext.EventManager.on(window, 'unload', function(){
    delete EC;
    delete El._flyweights;
});
})();
/**
 * @class Ext.Element
 */
Ext.Element.addMethods(function(){
	var PARENTNODE = 'parentNode',
		NEXTSIBLING = 'nextSibling',
		PREVIOUSSIBLING = 'previousSibling',
		DQ = Ext.DomQuery,
		GET = Ext.get;
	
	return {
		/**
	     * Looks at this node and then at parent nodes for a match of the passed simple selector (e.g. div.some-class or span:first-child)
	     * @param {String} selector The simple selector to test
	     * @param {Number/Mixed} maxDepth (optional) The max depth to search as a number or element (defaults to 50 || document.body)
	     * @param {Boolean} returnEl (optional) True to return a Ext.Element object instead of DOM node
	     * @return {HTMLElement} The matching DOM node (or null if no match was found)
	     */
	    findParent : function(simpleSelector, maxDepth, returnEl){
	        var p = this.dom,
	        	b = document.body, 
	        	depth = 0, 	        	
	        	stopEl;	        
            if(Ext.isGecko && Object.prototype.toString.call(p) == '[object XULElement]') {
                return null;
            }
	        maxDepth = maxDepth || 50;
	        if (isNaN(maxDepth)) {
	            stopEl = Ext.getDom(maxDepth);
	            maxDepth = Number.MAX_VALUE;
	        }
	        while(p && p.nodeType == 1 && depth < maxDepth && p != b && p != stopEl){
	            if(DQ.is(p, simpleSelector)){
	                return returnEl ? GET(p) : p;
	            }
	            depth++;
	            p = p.parentNode;
	        }
	        return null;
	    },
	
	    /**
	     * Looks at parent nodes for a match of the passed simple selector (e.g. div.some-class or span:first-child)
	     * @param {String} selector The simple selector to test
	     * @param {Number/Mixed} maxDepth (optional) The max depth to
	            search as a number or element (defaults to 10 || document.body)
	     * @param {Boolean} returnEl (optional) True to return a Ext.Element object instead of DOM node
	     * @return {HTMLElement} The matching DOM node (or null if no match was found)
	     */
	    findParentNode : function(simpleSelector, maxDepth, returnEl){
	        var p = Ext.fly(this.dom.parentNode, '_internal');
	        return p ? p.findParent(simpleSelector, maxDepth, returnEl) : null;
	    },
	
	    /**
	     * Walks up the dom looking for a parent node that matches the passed simple selector (e.g. div.some-class or span:first-child).
	     * This is a shortcut for findParentNode() that always returns an Ext.Element.
	     * @param {String} selector The simple selector to test
	     * @param {Number/Mixed} maxDepth (optional) The max depth to
	            search as a number or element (defaults to 10 || document.body)
	     * @return {Ext.Element} The matching DOM node (or null if no match was found)
	     */
	    up : function(simpleSelector, maxDepth){
	        return this.findParentNode(simpleSelector, maxDepth, true);
	    },
	
	    /**
	     * Creates a {@link Ext.CompositeElement} for child nodes based on the passed CSS selector (the selector should not contain an id).
	     * @param {String} selector The CSS selector
	     * @return {CompositeElement/CompositeElementLite} The composite element
	     */
	    select : function(selector){
	        return Ext.Element.select(selector, this.dom);
	    },
	
	    /**
	     * Selects child nodes based on the passed CSS selector (the selector should not contain an id).
	     * @param {String} selector The CSS selector
	     * @return {Array} An array of the matched nodes
	     */
	    query : function(selector){
	        return DQ.select(selector, this.dom);
	    },
	
	    /**
	     * Selects a single child at any depth below this element based on the passed CSS selector (the selector should not contain an id).
	     * @param {String} selector The CSS selector
	     * @param {Boolean} returnDom (optional) True to return the DOM node instead of Ext.Element (defaults to false)
	     * @return {HTMLElement/Ext.Element} The child Ext.Element (or DOM node if returnDom = true)
	     */
	    child : function(selector, returnDom){
	        var n = DQ.selectNode(selector, this.dom);
	        return returnDom ? n : GET(n);
	    },
	
	    /**
	     * Selects a single *direct* child based on the passed CSS selector (the selector should not contain an id).
	     * @param {String} selector The CSS selector
	     * @param {Boolean} returnDom (optional) True to return the DOM node instead of Ext.Element (defaults to false)
	     * @return {HTMLElement/Ext.Element} The child Ext.Element (or DOM node if returnDom = true)
	     */
	    down : function(selector, returnDom){
	        var n = DQ.selectNode(" > " + selector, this.dom);
	        return returnDom ? n : GET(n);
	    },
	
		 /**
	     * Gets the parent node for this element, optionally chaining up trying to match a selector
	     * @param {String} selector (optional) Find a parent node that matches the passed simple selector
	     * @param {Boolean} returnDom (optional) True to return a raw dom node instead of an Ext.Element
	     * @return {Ext.Element/HTMLElement} The parent node or null
		 */
	    parent : function(selector, returnDom){
	        return this.matchNode(PARENTNODE, PARENTNODE, selector, returnDom);
	    },
	
	     /**
	     * Gets the next sibling, skipping text nodes
	     * @param {String} selector (optional) Find the next sibling that matches the passed simple selector
	     * @param {Boolean} returnDom (optional) True to return a raw dom node instead of an Ext.Element
	     * @return {Ext.Element/HTMLElement} The next sibling or null
		 */
	    next : function(selector, returnDom){
	        return this.matchNode(NEXTSIBLING, NEXTSIBLING, selector, returnDom);
	    },
	
	    /**
	     * Gets the previous sibling, skipping text nodes
	     * @param {String} selector (optional) Find the previous sibling that matches the passed simple selector
	     * @param {Boolean} returnDom (optional) True to return a raw dom node instead of an Ext.Element
	     * @return {Ext.Element/HTMLElement} The previous sibling or null
		 */
	    prev : function(selector, returnDom){
	        return this.matchNode(PREVIOUSSIBLING, PREVIOUSSIBLING, selector, returnDom);
	    },
	
	
	    /**
	     * Gets the first child, skipping text nodes
	     * @param {String} selector (optional) Find the next sibling that matches the passed simple selector
	     * @param {Boolean} returnDom (optional) True to return a raw dom node instead of an Ext.Element
	     * @return {Ext.Element/HTMLElement} The first child or null
		 */
	    first : function(selector, returnDom){
	        return this.matchNode(NEXTSIBLING, 'firstChild', selector, returnDom);
	    },
	
	    /**
	     * Gets the last child, skipping text nodes
	     * @param {String} selector (optional) Find the previous sibling that matches the passed simple selector
	     * @param {Boolean} returnDom (optional) True to return a raw dom node instead of an Ext.Element
	     * @return {Ext.Element/HTMLElement} The last child or null
		 */
	    last : function(selector, returnDom){
	        return this.matchNode(PREVIOUSSIBLING, 'lastChild', selector, returnDom);
	    },
	    
	    matchNode : function(dir, start, selector, returnDom){
	        var n = this.dom[start];
	        while(n){
	            if(n.nodeType == 1 && (!selector || DQ.is(n, selector))){
	                return !returnDom ? GET(n) : n;
	            }
	            n = n[dir];
	        }
	        return null;
	    }	
    }
}());/**
 * @class Ext.Element
 */
Ext.Element.addMethods(
function() {
	var GETDOM = Ext.getDom,
		GET = Ext.get,
		DH = Ext.DomHelper;
	
	return {
	    /**
	     * Appends the passed element(s) to this element
	     * @param {String/HTMLElement/Array/Element/CompositeElement} el
	     * @return {Ext.Element} this
	     */
	    appendChild: function(el){        
	        return GET(el).appendTo(this);        
	    },
	
	    /**
	     * Appends this element to the passed element
	     * @param {Mixed} el The new parent element
	     * @return {Ext.Element} this
	     */
	    appendTo: function(el){        
	        GETDOM(el).appendChild(this.dom);        
	        return this;
	    },
	
	    /**
	     * Inserts this element before the passed element in the DOM
	     * @param {Mixed} el The element before which this element will be inserted
	     * @return {Ext.Element} this
	     */
	    insertBefore: function(el){  	          
	        (el = GETDOM(el)).parentNode.insertBefore(this.dom, el);
	        return this;
	    },
	
	    /**
	     * Inserts this element after the passed element in the DOM
	     * @param {Mixed} el The element to insert after
	     * @return {Ext.Element} this
	     */
	    insertAfter: function(el){
	        (el = GETDOM(el)).parentNode.insertBefore(this.dom, el.nextSibling);
	        return this;
	    },
	
	    /**
	     * Inserts (or creates) an element (or DomHelper config) as the first child of this element
	     * @param {Mixed/Object} el The id or element to insert or a DomHelper config to create and insert
	     * @return {Ext.Element} The new child
	     */
	    insertFirst: function(el, returnDom){
            el = el || {};
            if(el.nodeType || el.dom || typeof el == 'string'){ // element
                el = GETDOM(el);
                this.dom.insertBefore(el, this.dom.firstChild);
                return !returnDom ? GET(el) : el;
            }else{ // dh config
                return this.createChild(el, this.dom.firstChild, returnDom);
            }
        },
	
	    /**
	     * Replaces the passed element with this element
	     * @param {Mixed} el The element to replace
	     * @return {Ext.Element} this
	     */
	    replace: function(el){
	        el = GET(el);
	        this.insertBefore(el);
	        el.remove();
	        return this;
	    },
	
	    /**
	     * Replaces this element with the passed element
	     * @param {Mixed/Object} el The new element or a DomHelper config of an element to create
	     * @return {Ext.Element} this
	     */
	    replaceWith: function(el){
		    var me = this;
                
            if(el.nodeType || el.dom || typeof el == 'string'){
                el = GETDOM(el);
                me.dom.parentNode.insertBefore(el, me.dom);
            }else{
                el = DH.insertBefore(me.dom, el);
            }
	        
	        delete Ext.elCache[me.id];
	        Ext.removeNode(me.dom);      
	        me.id = Ext.id(me.dom = el);
	        Ext.Element.addToCache(me.isFlyweight ? new Ext.Element(me.dom) : me);     
            return me;
	    },
	    
		/**
		 * Creates the passed DomHelper config and appends it to this element or optionally inserts it before the passed child element.
		 * @param {Object} config DomHelper element config object.  If no tag is specified (e.g., {tag:'input'}) then a div will be
		 * automatically generated with the specified attributes.
		 * @param {HTMLElement} insertBefore (optional) a child element of this element
		 * @param {Boolean} returnDom (optional) true to return the dom node instead of creating an Element
		 * @return {Ext.Element} The new child element
		 */
		createChild: function(config, insertBefore, returnDom){
		    config = config || {tag:'div'};
		    return insertBefore ? 
		    	   DH.insertBefore(insertBefore, config, returnDom !== true) :	
		    	   DH[!this.dom.firstChild ? 'overwrite' : 'append'](this.dom, config,  returnDom !== true);
		},
		
		/**
		 * Creates and wraps this element with another element
		 * @param {Object} config (optional) DomHelper element config object for the wrapper element or null for an empty div
		 * @param {Boolean} returnDom (optional) True to return the raw DOM element instead of Ext.Element
		 * @return {HTMLElement/Element} The newly created wrapper element
		 */
		wrap: function(config, returnDom){        
		    var newEl = DH.insertBefore(this.dom, config || {tag: "div"}, !returnDom);
		    newEl.dom ? newEl.dom.appendChild(this.dom) : newEl.appendChild(this.dom);
		    return newEl;
		},
		
		/**
		 * Inserts an html fragment into this element
		 * @param {String} where Where to insert the html in relation to this element - beforeBegin, afterBegin, beforeEnd, afterEnd.
		 * @param {String} html The HTML fragment
		 * @param {Boolean} returnEl (optional) True to return an Ext.Element (defaults to false)
		 * @return {HTMLElement/Ext.Element} The inserted node (or nearest related if more than 1 inserted)
		 */
		insertHtml : function(where, html, returnEl){
		    var el = DH.insertHtml(where, this.dom, html);
		    return returnEl ? Ext.get(el) : el;
		}
	}
}());/**
 * @class Ext.Element
 */
Ext.Element.addMethods(function(){
    // local style camelizing for speed
    var propCache = {},
        camelRe = /(-[a-z])/gi,
        classReCache = {},
        view = document.defaultView,
        propFloat = Ext.isIE ? 'styleFloat' : 'cssFloat',
        opacityRe = /alpha\(opacity=(.*)\)/i,
        trimRe = /^\s+|\s+$/g,
        EL = Ext.Element,
        PADDING = "padding",
        MARGIN = "margin",
        BORDER = "border",
        LEFT = "-left",
        RIGHT = "-right",
        TOP = "-top",
        BOTTOM = "-bottom",
        WIDTH = "-width",
        MATH = Math,
        HIDDEN = 'hidden',
        ISCLIPPED = 'isClipped',
        OVERFLOW = 'overflow',
        OVERFLOWX = 'overflow-x',
        OVERFLOWY = 'overflow-y',
        ORIGINALCLIP = 'originalClip',
        // special markup used throughout Ext when box wrapping elements
        borders = {l: BORDER + LEFT + WIDTH, r: BORDER + RIGHT + WIDTH, t: BORDER + TOP + WIDTH, b: BORDER + BOTTOM + WIDTH},
        paddings = {l: PADDING + LEFT, r: PADDING + RIGHT, t: PADDING + TOP, b: PADDING + BOTTOM},
        margins = {l: MARGIN + LEFT, r: MARGIN + RIGHT, t: MARGIN + TOP, b: MARGIN + BOTTOM},
        data = Ext.Element.data;


    // private
    function camelFn(m, a) {
        return a.charAt(1).toUpperCase();
    }

    function chkCache(prop) {
        return propCache[prop] || (propCache[prop] = prop == 'float' ? propFloat : prop.replace(camelRe, camelFn));
    }

    return {
        // private  ==> used by Fx
        adjustWidth : function(width) {
            var me = this;
            var isNum = Ext.isNumber(width);
            if(isNum && me.autoBoxAdjust && !me.isBorderBox()){
               width -= (me.getBorderWidth("lr") + me.getPadding("lr"));
            }
            return (isNum && width < 0) ? 0 : width;
        },

        // private   ==> used by Fx
        adjustHeight : function(height) {
            var me = this;
            var isNum = Ext.isNumber(height);
            if(isNum && me.autoBoxAdjust && !me.isBorderBox()){
               height -= (me.getBorderWidth("tb") + me.getPadding("tb"));
            }
            return (isNum && height < 0) ? 0 : height;
        },


        /**
         * Adds one or more CSS classes to the element. Duplicate classes are automatically filtered out.
         * @param {String/Array} className The CSS class to add, or an array of classes
         * @return {Ext.Element} this
         */
        addClass : function(className){
            var me = this, i, len, v;
            className = Ext.isArray(className) ? className : [className];
            for (i=0, len = className.length; i < len; i++) {
                v = className[i];
                if (v) {
                    me.dom.className += (!me.hasClass(v) && v ? " " + v : "");
                };
            };
            return me;
        },

        /**
         * Adds one or more CSS classes to this element and removes the same class(es) from all siblings.
         * @param {String/Array} className The CSS class to add, or an array of classes
         * @return {Ext.Element} this
         */
        radioClass : function(className){
            var cn = this.dom.parentNode.childNodes, v;
            className = Ext.isArray(className) ? className : [className];
            for (var i=0, len = cn.length; i < len; i++) {
                v = cn[i];
                if(v && v.nodeType == 1) {
                    Ext.fly(v, '_internal').removeClass(className);
                }
            };
            return this.addClass(className);
        },

        /**
         * Removes one or more CSS classes from the element.
         * @param {String/Array} className The CSS class to remove, or an array of classes
         * @return {Ext.Element} this
         */
        removeClass : function(className){
            var me = this, v;
            className = Ext.isArray(className) ? className : [className];
            if (me.dom && me.dom.className) {
                for (var i=0, len=className.length; i < len; i++) {
                    v = className[i];
                    if(v) {
                        me.dom.className = me.dom.className.replace(
                            classReCache[v] = classReCache[v] || new RegExp('(?:^|\\s+)' + v + '(?:\\s+|$)', "g"), " "
                        );
                    }
                };
            }
            return me;
        },

        /**
         * Toggles the specified CSS class on this element (removes it if it already exists, otherwise adds it).
         * @param {String} className The CSS class to toggle
         * @return {Ext.Element} this
         */
        toggleClass : function(className){
            return this.hasClass(className) ? this.removeClass(className) : this.addClass(className);
        },

        /**
         * Checks if the specified CSS class exists on this element's DOM node.
         * @param {String} className The CSS class to check for
         * @return {Boolean} True if the class exists, else false
         */
        hasClass : function(className){
            return className && (' '+this.dom.className+' ').indexOf(' '+className+' ') != -1;
        },

        /**
         * Replaces a CSS class on the element with another.  If the old name does not exist, the new name will simply be added.
         * @param {String} oldClassName The CSS class to replace
         * @param {String} newClassName The replacement CSS class
         * @return {Ext.Element} this
         */
        replaceClass : function(oldClassName, newClassName){
            return this.removeClass(oldClassName).addClass(newClassName);
        },

        isStyle : function(style, val) {
            return this.getStyle(style) == val;
        },

        /**
         * Normalizes currentStyle and computedStyle.
         * @param {String} property The style property whose value is returned.
         * @return {String} The current value of the style property for this element.
         */
        getStyle : function(){
            return view && view.getComputedStyle ?
                function(prop){
                    var el = this.dom,
                        v,
                        cs,
                        out,
                        display,
                        wk = Ext.isWebKit,
                        display;
                        
                    if(el == document){
                        return null;
                    }
                    prop = chkCache(prop);
                    // Fix bug caused by this: https://bugs.webkit.org/show_bug.cgi?id=13343
                    if(wk && /marginRight/.test(prop)){
                        display = this.getStyle('display');
                        el.style.display = 'inline-block';
                    }
                    out = (v = el.style[prop]) ? v :
                           (cs = view.getComputedStyle(el, "")) ? cs[prop] : null;

                    // Webkit returns rgb values for transparent.
                    if(wk){
                        if(out == 'rgba(0, 0, 0, 0)'){
                            out = 'transparent';
                        }else if(display){
                            el.style.display = display;
                        }
                    }
                    return out;
                } :
                function(prop){
                    var el = this.dom,
                        m,
                        cs;

                    if(el == document) return null;
                    if (prop == 'opacity') {
                        if (el.style.filter.match) {
                            if(m = el.style.filter.match(opacityRe)){
                                var fv = parseFloat(m[1]);
                                if(!isNaN(fv)){
                                    return fv ? fv / 100 : 0;
                                }
                            }
                        }
                        return 1;
                    }
                    prop = chkCache(prop);
                    return el.style[prop] || ((cs = el.currentStyle) ? cs[prop] : null);
                };
        }(),

        /**
         * Return the CSS color for the specified CSS attribute. rgb, 3 digit (like #fff) and valid values
         * are convert to standard 6 digit hex color.
         * @param {String} attr The css attribute
         * @param {String} defaultValue The default value to use when a valid color isn't found
         * @param {String} prefix (optional) defaults to #. Use an empty string when working with
         * color anims.
         */
        getColor : function(attr, defaultValue, prefix){
            var v = this.getStyle(attr),
                color = Ext.isDefined(prefix) ? prefix : '#',
                h;

            if(!v || /transparent|inherit/.test(v)){
                return defaultValue;
            }
            if(/^r/.test(v)){
                Ext.each(v.slice(4, v.length -1).split(','), function(s){
                    h = parseInt(s, 10);
                    color += (h < 16 ? '0' : '') + h.toString(16);
                });
            }else{
                v = v.replace('#', '');
                color += v.length == 3 ? v.replace(/^(\w)(\w)(\w)$/, '$1$1$2$2$3$3') : v;
            }
            return(color.length > 5 ? color.toLowerCase() : defaultValue);
        },

        /**
         * Wrapper for setting style properties, also takes single object parameter of multiple styles.
         * @param {String/Object} property The style property to be set, or an object of multiple styles.
         * @param {String} value (optional) The value to apply to the given property, or null if an object was passed.
         * @return {Ext.Element} this
         */
        setStyle : function(prop, value){
            var tmp,
                style,
                camel;
            if (!Ext.isObject(prop)) {
                tmp = {};
                tmp[prop] = value;
                prop = tmp;
            }
            for (style in prop) {
                value = prop[style];
                style == 'opacity' ?
                    this.setOpacity(value) :
                    this.dom.style[chkCache(style)] = value;
            }
            return this;
        },

        /**
         * Set the opacity of the element
         * @param {Float} opacity The new opacity. 0 = transparent, .5 = 50% visibile, 1 = fully visible, etc
         * @param {Boolean/Object} animate (optional) a standard Element animation config object or <tt>true</tt> for
         * the default animation (<tt>{duration: .35, easing: 'easeIn'}</tt>)
         * @return {Ext.Element} this
         */
         setOpacity : function(opacity, animate){
            var me = this,
                s = me.dom.style;

            if(!animate || !me.anim){
                if(Ext.isIE){
                    var opac = opacity < 1 ? 'alpha(opacity=' + opacity * 100 + ')' : '',
                    val = s.filter.replace(opacityRe, '').replace(trimRe, '');

                    s.zoom = 1;
                    s.filter = val + (val.length > 0 ? ' ' : '') + opac;
                }else{
                    s.opacity = opacity;
                }
            }else{
                me.anim({opacity: {to: opacity}}, me.preanim(arguments, 1), null, .35, 'easeIn');
            }
            return me;
        },

        /**
         * Clears any opacity settings from this element. Required in some cases for IE.
         * @return {Ext.Element} this
         */
        clearOpacity : function(){
            var style = this.dom.style;
            if(Ext.isIE){
                if(!Ext.isEmpty(style.filter)){
                    style.filter = style.filter.replace(opacityRe, '').replace(trimRe, '');
                }
            }else{
                style.opacity = style['-moz-opacity'] = style['-khtml-opacity'] = '';
            }
            return this;
        },

        /**
         * Returns the offset height of the element
         * @param {Boolean} contentHeight (optional) true to get the height minus borders and padding
         * @return {Number} The element's height
         */
        getHeight : function(contentHeight){
            var me = this,
                dom = me.dom,
                hidden = Ext.isIE && me.isStyle('display', 'none'),
                h = MATH.max(dom.offsetHeight, hidden ? 0 : dom.clientHeight) || 0;

            h = !contentHeight ? h : h - me.getBorderWidth("tb") - me.getPadding("tb");
            return h < 0 ? 0 : h;
        },

        /**
         * Returns the offset width of the element
         * @param {Boolean} contentWidth (optional) true to get the width minus borders and padding
         * @return {Number} The element's width
         */
        getWidth : function(contentWidth){
            var me = this,
                dom = me.dom,
                hidden = Ext.isIE && me.isStyle('display', 'none'),
                w = MATH.max(dom.offsetWidth, hidden ? 0 : dom.clientWidth) || 0;
            w = !contentWidth ? w : w - me.getBorderWidth("lr") - me.getPadding("lr");
            return w < 0 ? 0 : w;
        },

        /**
         * Set the width of this Element.
         * @param {Mixed} width The new width. This may be one of:<div class="mdetail-params"><ul>
         * <li>A Number specifying the new width in this Element's {@link #defaultUnit}s (by default, pixels).</li>
         * <li>A String used to set the CSS width style. Animation may <b>not</b> be used.
         * </ul></div>
         * @param {Boolean/Object} animate (optional) true for the default animation or a standard Element animation config object
         * @return {Ext.Element} this
         */
        setWidth : function(width, animate){
            var me = this;
            width = me.adjustWidth(width);
            !animate || !me.anim ?
                me.dom.style.width = me.addUnits(width) :
                me.anim({width : {to : width}}, me.preanim(arguments, 1));
            return me;
        },

        /**
         * Set the height of this Element.
         * <pre><code>
// change the height to 200px and animate with default configuration
Ext.fly('elementId').setHeight(200, true);

// change the height to 150px and animate with a custom configuration
Ext.fly('elId').setHeight(150, {
    duration : .5, // animation will have a duration of .5 seconds
    // will change the content to "finished"
    callback: function(){ this.{@link #update}("finished"); }
});
         * </code></pre>
         * @param {Mixed} height The new height. This may be one of:<div class="mdetail-params"><ul>
         * <li>A Number specifying the new height in this Element's {@link #defaultUnit}s (by default, pixels.)</li>
         * <li>A String used to set the CSS height style. Animation may <b>not</b> be used.</li>
         * </ul></div>
         * @param {Boolean/Object} animate (optional) true for the default animation or a standard Element animation config object
         * @return {Ext.Element} this
         */
         setHeight : function(height, animate){
            var me = this;
            height = me.adjustHeight(height);
            !animate || !me.anim ?
                me.dom.style.height = me.addUnits(height) :
                me.anim({height : {to : height}}, me.preanim(arguments, 1));
            return me;
        },

        /**
         * Gets the width of the border(s) for the specified side(s)
         * @param {String} side Can be t, l, r, b or any combination of those to add multiple values. For example,
         * passing <tt>'lr'</tt> would get the border <b><u>l</u></b>eft width + the border <b><u>r</u></b>ight width.
         * @return {Number} The width of the sides passed added together
         */
        getBorderWidth : function(side){
            return this.addStyles(side, borders);
        },

        /**
         * Gets the width of the padding(s) for the specified side(s)
         * @param {String} side Can be t, l, r, b or any combination of those to add multiple values. For example,
         * passing <tt>'lr'</tt> would get the padding <b><u>l</u></b>eft + the padding <b><u>r</u></b>ight.
         * @return {Number} The padding of the sides passed added together
         */
        getPadding : function(side){
            return this.addStyles(side, paddings);
        },

        /**
         *  Store the current overflow setting and clip overflow on the element - use <tt>{@link #unclip}</tt> to remove
         * @return {Ext.Element} this
         */
        clip : function(){
            var me = this,
                dom = me.dom;

            if(!data(dom, ISCLIPPED)){
                data(dom, ISCLIPPED, true);
                data(dom, ORIGINALCLIP, {
                    o: me.getStyle(OVERFLOW),
                    x: me.getStyle(OVERFLOWX),
                    y: me.getStyle(OVERFLOWY)
                });
                me.setStyle(OVERFLOW, HIDDEN);
                me.setStyle(OVERFLOWX, HIDDEN);
                me.setStyle(OVERFLOWY, HIDDEN);
            }
            return me;
        },

        /**
         *  Return clipping (overflow) to original clipping before <tt>{@link #clip}</tt> was called
         * @return {Ext.Element} this
         */
        unclip : function(){
            var me = this,
                dom = me.dom;

            if(data(dom, ISCLIPPED)){
                data(dom, ISCLIPPED, false);
                var o = data(dom, ORIGINALCLIP);
                if(o.o){
                    me.setStyle(OVERFLOW, o.o);
                }
                if(o.x){
                    me.setStyle(OVERFLOWX, o.x);
                }
                if(o.y){
                    me.setStyle(OVERFLOWY, o.y);
                }
            }
            return me;
        },

        // private
        addStyles : function(sides, styles){
            var val = 0,
                m = sides.match(/\w/g),
                s;
            for (var i=0, len=m.length; i<len; i++) {
                s = m[i] && parseInt(this.getStyle(styles[m[i]]), 10);
                if (s) {
                    val += MATH.abs(s);
                }
            }
            return val;
        },

        margins : margins
    }
}()
);
/**
 * @class Ext.Element
 */
(function(){
var D = Ext.lib.Dom,
        LEFT = "left",
        RIGHT = "right",
        TOP = "top",
        BOTTOM = "bottom",
        POSITION = "position",
        STATIC = "static",
        RELATIVE = "relative",
        AUTO = "auto",
        ZINDEX = "z-index";

Ext.Element.addMethods({
	/**
      * Gets the current X position of the element based on page coordinates.  Element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
      * @return {Number} The X position of the element
      */
    getX : function(){
        return D.getX(this.dom);
    },

    /**
      * Gets the current Y position of the element based on page coordinates.  Element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
      * @return {Number} The Y position of the element
      */
    getY : function(){
        return D.getY(this.dom);
    },

    /**
      * Gets the current position of the element based on page coordinates.  Element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
      * @return {Array} The XY position of the element
      */
    getXY : function(){
        return D.getXY(this.dom);
    },

    /**
      * Returns the offsets of this element from the passed element. Both element must be part of the DOM tree and not have display:none to have page coordinates.
      * @param {Mixed} element The element to get the offsets from.
      * @return {Array} The XY page offsets (e.g. [100, -200])
      */
    getOffsetsTo : function(el){
        var o = this.getXY(),
        	e = Ext.fly(el, '_internal').getXY();
        return [o[0]-e[0],o[1]-e[1]];
    },

    /**
     * Sets the X position of the element based on page coordinates.  Element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
     * @param {Number} The X position of the element
     * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
     * @return {Ext.Element} this
     */
    setX : function(x, animate){	    
	    return this.setXY([x, this.getY()], this.animTest(arguments, animate, 1));
    },

    /**
     * Sets the Y position of the element based on page coordinates.  Element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
     * @param {Number} The Y position of the element
     * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
     * @return {Ext.Element} this
     */
    setY : function(y, animate){	    
	    return this.setXY([this.getX(), y], this.animTest(arguments, animate, 1));
    },

    /**
     * Sets the element's left position directly using CSS style (instead of {@link #setX}).
     * @param {String} left The left CSS property value
     * @return {Ext.Element} this
     */
    setLeft : function(left){
        this.setStyle(LEFT, this.addUnits(left));
        return this;
    },

    /**
     * Sets the element's top position directly using CSS style (instead of {@link #setY}).
     * @param {String} top The top CSS property value
     * @return {Ext.Element} this
     */
    setTop : function(top){
        this.setStyle(TOP, this.addUnits(top));
        return this;
    },

    /**
     * Sets the element's CSS right style.
     * @param {String} right The right CSS property value
     * @return {Ext.Element} this
     */
    setRight : function(right){
        this.setStyle(RIGHT, this.addUnits(right));
        return this;
    },

    /**
     * Sets the element's CSS bottom style.
     * @param {String} bottom The bottom CSS property value
     * @return {Ext.Element} this
     */
    setBottom : function(bottom){
        this.setStyle(BOTTOM, this.addUnits(bottom));
        return this;
    },

    /**
     * Sets the position of the element in page coordinates, regardless of how the element is positioned.
     * The element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
     * @param {Array} pos Contains X & Y [x, y] values for new position (coordinates are page-based)
     * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
     * @return {Ext.Element} this
     */
    setXY : function(pos, animate){
	    var me = this;
        if(!animate || !me.anim){
            D.setXY(me.dom, pos);
        }else{
            me.anim({points: {to: pos}}, me.preanim(arguments, 1), 'motion');
        }
        return me;
    },

    /**
     * Sets the position of the element in page coordinates, regardless of how the element is positioned.
     * The element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
     * @param {Number} x X value for new position (coordinates are page-based)
     * @param {Number} y Y value for new position (coordinates are page-based)
     * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
     * @return {Ext.Element} this
     */
    setLocation : function(x, y, animate){
        return this.setXY([x, y], this.animTest(arguments, animate, 2));
    },

    /**
     * Sets the position of the element in page coordinates, regardless of how the element is positioned.
     * The element must be part of the DOM tree to have page coordinates (display:none or elements not appended return false).
     * @param {Number} x X value for new position (coordinates are page-based)
     * @param {Number} y Y value for new position (coordinates are page-based)
     * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
     * @return {Ext.Element} this
     */
    moveTo : function(x, y, animate){
        return this.setXY([x, y], this.animTest(arguments, animate, 2));        
    },    
    
    /**
     * Gets the left X coordinate
     * @param {Boolean} local True to get the local css position instead of page coordinate
     * @return {Number}
     */
    getLeft : function(local){
	    return !local ? this.getX() : parseInt(this.getStyle(LEFT), 10) || 0;
    },

    /**
     * Gets the right X coordinate of the element (element X position + element width)
     * @param {Boolean} local True to get the local css position instead of page coordinate
     * @return {Number}
     */
    getRight : function(local){
	    var me = this;
	    return !local ? me.getX() + me.getWidth() : (me.getLeft(true) + me.getWidth()) || 0;
    },

    /**
     * Gets the top Y coordinate
     * @param {Boolean} local True to get the local css position instead of page coordinate
     * @return {Number}
     */
    getTop : function(local) {
	    return !local ? this.getY() : parseInt(this.getStyle(TOP), 10) || 0;
    },

    /**
     * Gets the bottom Y coordinate of the element (element Y position + element height)
     * @param {Boolean} local True to get the local css position instead of page coordinate
     * @return {Number}
     */
    getBottom : function(local){
	    var me = this;
	    return !local ? me.getY() + me.getHeight() : (me.getTop(true) + me.getHeight()) || 0;
    },

    /**
    * Initializes positioning on this element. If a desired position is not passed, it will make the
    * the element positioned relative IF it is not already positioned.
    * @param {String} pos (optional) Positioning to use "relative", "absolute" or "fixed"
    * @param {Number} zIndex (optional) The zIndex to apply
    * @param {Number} x (optional) Set the page X position
    * @param {Number} y (optional) Set the page Y position
    */
    position : function(pos, zIndex, x, y){
	    var me = this;
	    
        if(!pos && me.isStyle(POSITION, STATIC)){           
            me.setStyle(POSITION, RELATIVE);           
        } else if(pos) {
            me.setStyle(POSITION, pos);
        }
        if(zIndex){
            me.setStyle(ZINDEX, zIndex);
        }
        if(x || y) me.setXY([x || false, y || false]);
    },

    /**
    * Clear positioning back to the default when the document was loaded
    * @param {String} value (optional) The value to use for the left,right,top,bottom, defaults to '' (empty string). You could use 'auto'.
    * @return {Ext.Element} this
     */
    clearPositioning : function(value){
        value = value || '';
        this.setStyle({
            left : value,
            right : value,
            top : value,
            bottom : value,
            "z-index" : "",
            position : STATIC
        });
        return this;
    },

    /**
    * Gets an object with all CSS positioning properties. Useful along with setPostioning to get
    * snapshot before performing an update and then restoring the element.
    * @return {Object}
    */
    getPositioning : function(){
        var l = this.getStyle(LEFT);
        var t = this.getStyle(TOP);
        return {
            "position" : this.getStyle(POSITION),
            "left" : l,
            "right" : l ? "" : this.getStyle(RIGHT),
            "top" : t,
            "bottom" : t ? "" : this.getStyle(BOTTOM),
            "z-index" : this.getStyle(ZINDEX)
        };
    },
    
    /**
    * Set positioning with an object returned by getPositioning().
    * @param {Object} posCfg
    * @return {Ext.Element} this
     */
    setPositioning : function(pc){
	    var me = this,
	    	style = me.dom.style;
	    	
        me.setStyle(pc);
        
        if(pc.right == AUTO){
            style.right = "";
        }
        if(pc.bottom == AUTO){
            style.bottom = "";
        }
        
        return me;
    },    
	
    /**
     * Translates the passed page coordinates into left/top css values for this element
     * @param {Number/Array} x The page x or an array containing [x, y]
     * @param {Number} y (optional) The page y, required if x is not an array
     * @return {Object} An object with left and top properties. e.g. {left: (value), top: (value)}
     */
    translatePoints : function(x, y){        	     
	    y = isNaN(x[1]) ? y : x[1];
        x = isNaN(x[0]) ? x : x[0];
        var me = this,
        	relative = me.isStyle(POSITION, RELATIVE),
        	o = me.getXY(),
        	l = parseInt(me.getStyle(LEFT), 10),
        	t = parseInt(me.getStyle(TOP), 10);
        
        l = !isNaN(l) ? l : (relative ? 0 : me.dom.offsetLeft);
        t = !isNaN(t) ? t : (relative ? 0 : me.dom.offsetTop);        

        return {left: (x - o[0] + l), top: (y - o[1] + t)}; 
    },
    
    animTest : function(args, animate, i) {
        return !!animate && this.preanim ? this.preanim(args, i) : false;
    }
});
})();/**
 * @class Ext.Element
 */
Ext.Element.addMethods({
    /**
     * Returns true if this element is scrollable.
     * @return {Boolean}
     */
    isScrollable : function(){
        var dom = this.dom;
        return dom.scrollHeight > dom.clientHeight || dom.scrollWidth > dom.clientWidth;
    },

    /**
     * Scrolls this element the specified scroll point. It does NOT do bounds checking so if you scroll to a weird value it will try to do it. For auto bounds checking, use scroll().
     * @param {String} side Either "left" for scrollLeft values or "top" for scrollTop values.
     * @param {Number} value The new scroll value.
     * @return {Element} this
     */
    scrollTo : function(side, value){
        this.dom["scroll" + (/top/i.test(side) ? "Top" : "Left")] = value;
        return this;
    },

    /**
     * Returns the current scroll position of the element.
     * @return {Object} An object containing the scroll position in the format {left: (scrollLeft), top: (scrollTop)}
     */
    getScroll : function(){
        var d = this.dom, 
            doc = document,
            body = doc.body,
            docElement = doc.documentElement,
            l,
            t,
            ret;

        if(d == doc || d == body){
            if(Ext.isIE && Ext.isStrict){
                l = docElement.scrollLeft; 
                t = docElement.scrollTop;
            }else{
                l = window.pageXOffset;
                t = window.pageYOffset;
            }
            ret = {left: l || (body ? body.scrollLeft : 0), top: t || (body ? body.scrollTop : 0)};
        }else{
            ret = {left: d.scrollLeft, top: d.scrollTop};
        }
        return ret;
    }
});/**
 * @class Ext.Element
 */
/**
 * Visibility mode constant for use with {@link #setVisibilityMode}. Use visibility to hide element
 * @static
 * @type Number
 */
Ext.Element.VISIBILITY = 1;
/**
 * Visibility mode constant for use with {@link #setVisibilityMode}. Use display to hide element
 * @static
 * @type Number
 */
Ext.Element.DISPLAY = 2;

Ext.Element.addMethods(function(){
    var VISIBILITY = "visibility",
        DISPLAY = "display",
        HIDDEN = "hidden",
        NONE = "none",      
        ORIGINALDISPLAY = 'originalDisplay',
        VISMODE = 'visibilityMode',
        ELDISPLAY = Ext.Element.DISPLAY,
        data = Ext.Element.data,
        getDisplay = function(dom){
            var d = data(dom, ORIGINALDISPLAY);
            if(d === undefined){
                data(dom, ORIGINALDISPLAY, d = '');
            }
            return d;
        },
        getVisMode = function(dom){
            var m = data(dom, VISMODE);
            if(m === undefined){
                data(dom, VISMODE, m = 1)
            }
            return m;
        };
    
    return {
        /**
         * The element's default display mode  (defaults to "")
         * @type String
         */
        originalDisplay : "",
        visibilityMode : 1,
        
        /**
         * Sets the element's visibility mode. When setVisible() is called it
         * will use this to determine whether to set the visibility or the display property.
         * @param {Number} visMode Ext.Element.VISIBILITY or Ext.Element.DISPLAY
         * @return {Ext.Element} this
         */
        setVisibilityMode : function(visMode){  
            data(this.dom, VISMODE, visMode);
            return this;
        },
        
        /**
         * Perform custom animation on this element.
         * <div><ul class="mdetail-params">
         * <li><u>Animation Properties</u></li>
         * 
         * <p>The Animation Control Object enables gradual transitions for any member of an
         * element's style object that takes a numeric value including but not limited to
         * these properties:</p><div><ul class="mdetail-params">
         * <li><tt>bottom, top, left, right</tt></li>
         * <li><tt>height, width</tt></li>
         * <li><tt>margin, padding</tt></li>
         * <li><tt>borderWidth</tt></li>
         * <li><tt>opacity</tt></li>
         * <li><tt>fontSize</tt></li>
         * <li><tt>lineHeight</tt></li>
         * </ul></div>
         * 
         * 
         * <li><u>Animation Property Attributes</u></li>
         * 
         * <p>Each Animation Property is a config object with optional properties:</p>
         * <div><ul class="mdetail-params">
         * <li><tt>by</tt>*  : relative change - start at current value, change by this value</li>
         * <li><tt>from</tt> : ignore current value, start from this value</li>
         * <li><tt>to</tt>*  : start at current value, go to this value</li>
         * <li><tt>unit</tt> : any allowable unit specification</li>
         * <p>* do not specify both <tt>to</tt> and <tt>by</tt> for an animation property</p>
         * </ul></div>
         * 
         * <li><u>Animation Types</u></li>
         * 
         * <p>The supported animation types:</p><div><ul class="mdetail-params">
         * <li><tt>'run'</tt> : Default
         * <pre><code>
var el = Ext.get('complexEl');
el.animate(
    // animation control object
    {
        borderWidth: {to: 3, from: 0},
        opacity: {to: .3, from: 1},
        height: {to: 50, from: el.getHeight()},
        width: {to: 300, from: el.getWidth()},
        top  : {by: - 100, unit: 'px'},
    },
    0.35,      // animation duration
    null,      // callback
    'easeOut', // easing method
    'run'      // animation type ('run','color','motion','scroll')    
);
         * </code></pre>
         * </li>
         * <li><tt>'color'</tt>
         * <p>Animates transition of background, text, or border colors.</p>
         * <pre><code>
el.animate(
    // animation control object
    {
        color: { to: '#06e' },
        backgroundColor: { to: '#e06' }
    },
    0.35,      // animation duration
    null,      // callback
    'easeOut', // easing method
    'color'    // animation type ('run','color','motion','scroll')    
);
         * </code></pre> 
         * </li>
         * 
         * <li><tt>'motion'</tt>
         * <p>Animates the motion of an element to/from specific points using optional bezier
         * way points during transit.</p>
         * <pre><code>
el.animate(
    // animation control object
    {
        borderWidth: {to: 3, from: 0},
        opacity: {to: .3, from: 1},
        height: {to: 50, from: el.getHeight()},
        width: {to: 300, from: el.getWidth()},
        top  : {by: - 100, unit: 'px'},
        points: {
            to: [50, 100],  // go to this point
            control: [      // optional bezier way points
                [ 600, 800],
                [-100, 200]
            ]
        }
    },
    3000,      // animation duration (milliseconds!)
    null,      // callback
    'easeOut', // easing method
    'motion'   // animation type ('run','color','motion','scroll')    
);
         * </code></pre> 
         * </li>
         * <li><tt>'scroll'</tt>
         * <p>Animate horizontal or vertical scrolling of an overflowing page element.</p>
         * <pre><code>
el.animate(
    // animation control object
    {
        scroll: {to: [400, 300]}
    },
    0.35,      // animation duration
    null,      // callback
    'easeOut', // easing method
    'scroll'   // animation type ('run','color','motion','scroll')    
);
         * </code></pre> 
         * </li>
         * </ul></div>
         * 
         * </ul></div>
         * 
         * @param {Object} args The animation control args
         * @param {Float} duration (optional) How long the animation lasts in seconds (defaults to <tt>.35</tt>)
         * @param {Function} onComplete (optional) Function to call when animation completes
         * @param {String} easing (optional) {@link Ext.Fx#easing} method to use (defaults to <tt>'easeOut'</tt>)
         * @param {String} animType (optional) <tt>'run'</tt> is the default. Can also be <tt>'color'</tt>,
         * <tt>'motion'</tt>, or <tt>'scroll'</tt>
         * @return {Ext.Element} this
         */
        animate : function(args, duration, onComplete, easing, animType){       
            this.anim(args, {duration: duration, callback: onComplete, easing: easing}, animType);
            return this;
        },
    
        /*
         * @private Internal animation call
         */
        anim : function(args, opt, animType, defaultDur, defaultEase, cb){
            animType = animType || 'run';
            opt = opt || {};
            var me = this,              
                anim = Ext.lib.Anim[animType](
                    me.dom, 
                    args,
                    (opt.duration || defaultDur) || .35,
                    (opt.easing || defaultEase) || 'easeOut',
                    function(){
                        if(cb) cb.call(me);
                        if(opt.callback) opt.callback.call(opt.scope || me, me, opt);
                    },
                    me
                );
            opt.anim = anim;
            return anim;
        },
    
        // private legacy anim prep
        preanim : function(a, i){
            return !a[i] ? false : (Ext.isObject(a[i]) ? a[i]: {duration: a[i+1], callback: a[i+2], easing: a[i+3]});
        },
        
        /**
         * Checks whether the element is currently visible using both visibility and display properties.         
         * @return {Boolean} True if the element is currently visible, else false
         */
        isVisible : function() {
            return !this.isStyle(VISIBILITY, HIDDEN) && !this.isStyle(DISPLAY, NONE);
        },
        
        /**
         * Sets the visibility of the element (see details). If the visibilityMode is set to Element.DISPLAY, it will use
         * the display property to hide the element, otherwise it uses visibility. The default is to hide and show using the visibility property.
         * @param {Boolean} visible Whether the element is visible
         * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
         * @return {Ext.Element} this
         */
         setVisible : function(visible, animate){
            var me = this,
                dom = me.dom,
                isDisplay = getVisMode(this.dom) == ELDISPLAY;
                
            if (!animate || !me.anim) {
                if(isDisplay){
                    me.setDisplayed(visible);
                }else{
                    me.fixDisplay();
                    dom.style.visibility = visible ? "visible" : HIDDEN;
                }
            }else{
                // closure for composites            
                if(visible){
                    me.setOpacity(.01);
                    me.setVisible(true);
                }
                me.anim({opacity: { to: (visible?1:0) }},
                        me.preanim(arguments, 1),
                        null,
                        .35,
                        'easeIn',
                        function(){
                             if(!visible){
                                 dom.style[isDisplay ? DISPLAY : VISIBILITY] = (isDisplay) ? NONE : HIDDEN;                     
                                 Ext.fly(dom).setOpacity(1);
                             }
                        });
            }
            return me;
        },
    
        /**
         * Toggles the element's visibility or display, depending on visibility mode.
         * @param {Boolean/Object} animate (optional) True for the default animation, or a standard Element animation config object
         * @return {Ext.Element} this
         */
        toggle : function(animate){
            var me = this;
            me.setVisible(!me.isVisible(), me.preanim(arguments, 0));
            return me;
        },
    
        /**
         * Sets the CSS display property. Uses originalDisplay if the specified value is a boolean true.
         * @param {Mixed} value Boolean value to display the element using its default display, or a string to set the display directly.
         * @return {Ext.Element} this
         */
        setDisplayed : function(value) {            
            if(typeof value == "boolean"){
               value = value ? getDisplay(this.dom) : NONE;
            }
            this.setStyle(DISPLAY, value);
            return this;
        },
        
        // private
        fixDisplay : function(){
            var me = this;
            if(me.isStyle(DISPLAY, NONE)){
                me.setStyle(VISIBILITY, HIDDEN);
                me.setStyle(DISPLAY, getDisplay(this.dom)); // first try reverting to default
                if(me.isStyle(DISPLAY, NONE)){ // if that fails, default to block
                    me.setStyle(DISPLAY, "block");
                }
            }
        },
    
        /**
         * Hide this element - Uses display mode to determine whether to use "display" or "visibility". See {@link #setVisible}.
         * @param {Boolean/Object} animate (optional) true for the default animation or a standard Element animation config object
         * @return {Ext.Element} this
         */
        hide : function(animate){
            this.setVisible(false, this.preanim(arguments, 0));
            return this;
        },
    
        /**
        * Show this element - Uses display mode to determine whether to use "display" or "visibility". See {@link #setVisible}.
        * @param {Boolean/Object} animate (optional) true for the default animation or a standard Element animation config object
         * @return {Ext.Element} this
         */
        show : function(animate){
            this.setVisible(true, this.preanim(arguments, 0));
            return this;
        }
    }
}());(function(){
    // contants
    var NULL = null,
        UNDEFINED = undefined,
        TRUE = true,
        FALSE = false,
        SETX = "setX",
        SETY = "setY",
        SETXY = "setXY",
        LEFT = "left",
        BOTTOM = "bottom",
        TOP = "top",
        RIGHT = "right",
        HEIGHT = "height",
        WIDTH = "width",
        POINTS = "points",
        HIDDEN = "hidden",
        ABSOLUTE = "absolute",
        VISIBLE = "visible",
        MOTION = "motion",
        POSITION = "position",
        EASEOUT = "easeOut",
        /*
         * Use a light flyweight here since we are using so many callbacks and are always assured a DOM element
         */
        flyEl = new Ext.Element.Flyweight(),
        queues = {},
        getObject = function(o){
            return o || {};
        },
        fly = function(dom){
            flyEl.dom = dom;
            flyEl.id = Ext.id(dom);
            return flyEl;
        },
        /*
         * Queueing now stored outside of the element due to closure issues
         */
        getQueue = function(id){
            if(!queues[id]){
                queues[id] = [];
            }
            return queues[id];
        },
        setQueue = function(id, value){
            queues[id] = value;
        };
        
//Notifies Element that fx methods are available
Ext.enableFx = TRUE;

/**
 * @class Ext.Fx
 * <p>A class to provide basic animation and visual effects support.  <b>Note:</b> This class is automatically applied
 * to the {@link Ext.Element} interface when included, so all effects calls should be performed via {@link Ext.Element}.
 * Conversely, since the effects are not actually defined in {@link Ext.Element}, Ext.Fx <b>must</b> be
 * {@link Ext#enableFx included} in order for the Element effects to work.</p><br/>
 * 
 * <p><b><u>Method Chaining</u></b></p>
 * <p>It is important to note that although the Fx methods and many non-Fx Element methods support "method chaining" in that
 * they return the Element object itself as the method return value, it is not always possible to mix the two in a single
 * method chain.  The Fx methods use an internal effects queue so that each effect can be properly timed and sequenced.
 * Non-Fx methods, on the other hand, have no such internal queueing and will always execute immediately.  For this reason,
 * while it may be possible to mix certain Fx and non-Fx method calls in a single chain, it may not always provide the
 * expected results and should be done with care.  Also see <tt>{@link #callback}</tt>.</p><br/>
 *
 * <p><b><u>Anchor Options for Motion Effects</u></b></p>
 * <p>Motion effects support 8-way anchoring, meaning that you can choose one of 8 different anchor points on the Element
 * that will serve as either the start or end point of the animation.  Following are all of the supported anchor positions:</p>
<pre>
Value  Description
-----  -----------------------------
tl     The top left corner
t      The center of the top edge
tr     The top right corner
l      The center of the left edge
r      The center of the right edge
bl     The bottom left corner
b      The center of the bottom edge
br     The bottom right corner
</pre>
 * <b>Note</b>: some Fx methods accept specific custom config parameters.  The options shown in the Config Options
 * section below are common options that can be passed to any Fx method unless otherwise noted.</b>
 * 
 * @cfg {Function} callback A function called when the effect is finished.  Note that effects are queued internally by the
 * Fx class, so a callback is not required to specify another effect -- effects can simply be chained together
 * and called in sequence (see note for <b><u>Method Chaining</u></b> above), for example:<pre><code>
 * el.slideIn().highlight();
 * </code></pre>
 * The callback is intended for any additional code that should run once a particular effect has completed. The Element
 * being operated upon is passed as the first parameter.
 * 
 * @cfg {Object} scope The scope (<code>this</code> reference) in which the <tt>{@link #callback}</tt> function is executed. Defaults to the browser window.
 * 
 * @cfg {String} easing A valid Ext.lib.Easing value for the effect:</p><div class="mdetail-params"><ul>
 * <li><b><tt>backBoth</tt></b></li>
 * <li><b><tt>backIn</tt></b></li>
 * <li><b><tt>backOut</tt></b></li>
 * <li><b><tt>bounceBoth</tt></b></li>
 * <li><b><tt>bounceIn</tt></b></li>
 * <li><b><tt>bounceOut</tt></b></li>
 * <li><b><tt>easeBoth</tt></b></li>
 * <li><b><tt>easeBothStrong</tt></b></li>
 * <li><b><tt>easeIn</tt></b></li>
 * <li><b><tt>easeInStrong</tt></b></li>
 * <li><b><tt>easeNone</tt></b></li>
 * <li><b><tt>easeOut</tt></b></li>
 * <li><b><tt>easeOutStrong</tt></b></li>
 * <li><b><tt>elasticBoth</tt></b></li>
 * <li><b><tt>elasticIn</tt></b></li>
 * <li><b><tt>elasticOut</tt></b></li>
 * </ul></div>
 *
 * @cfg {String} afterCls A css class to apply after the effect
 * @cfg {Number} duration The length of time (in seconds) that the effect should last
 * 
 * @cfg {Number} endOpacity Only applicable for {@link #fadeIn} or {@link #fadeOut}, a number between
 * <tt>0</tt> and <tt>1</tt> inclusive to configure the ending opacity value.
 *  
 * @cfg {Boolean} remove Whether the Element should be removed from the DOM and destroyed after the effect finishes
 * @cfg {Boolean} useDisplay Whether to use the <i>display</i> CSS property instead of <i>visibility</i> when hiding Elements (only applies to 
 * effects that end with the element being visually hidden, ignored otherwise)
 * @cfg {String/Object/Function} afterStyle A style specification string, e.g. <tt>"width:100px"</tt>, or an object
 * in the form <tt>{width:"100px"}</tt>, or a function which returns such a specification that will be applied to the
 * Element after the effect finishes.
 * @cfg {Boolean} block Whether the effect should block other effects from queueing while it runs
 * @cfg {Boolean} concurrent Whether to allow subsequently-queued effects to run at the same time as the current effect, or to ensure that they run in sequence
 * @cfg {Boolean} stopFx Whether preceding effects should be stopped and removed before running current effect (only applies to non blocking effects)
 */
Ext.Fx = {
    
    // private - calls the function taking arguments from the argHash based on the key.  Returns the return value of the function.
    //           this is useful for replacing switch statements (for example).
    switchStatements : function(key, fn, argHash){
        return fn.apply(this, argHash[key]);
    },
    
    /**
     * Slides the element into view.  An anchor point can be optionally passed to set the point of
     * origin for the slide effect.  This function automatically handles wrapping the element with
     * a fixed-size container if needed.  See the Fx class overview for valid anchor point options.
     * Usage:
     *<pre><code>
// default: slide the element in from the top
el.slideIn();

// custom: slide the element in from the right with a 2-second duration
el.slideIn('r', { duration: 2 });

// common config options shown with default values
el.slideIn('t', {
    easing: 'easeOut',
    duration: .5
});
</code></pre>
     * @param {String} anchor (optional) One of the valid Fx anchor positions (defaults to top: 't')
     * @param {Object} options (optional) Object literal with any of the Fx config options
     * @return {Ext.Element} The Element
     */
    slideIn : function(anchor, o){ 
        o = getObject(o);
        var me = this,
            dom = me.dom,
            st = dom.style,
            xy,
            r,
            b,              
            wrap,               
            after,
            st,
            args, 
            pt,
            bw,
            bh;
            
        anchor = anchor || "t";

        me.queueFx(o, function(){            
            xy = fly(dom).getXY();
            // fix display to visibility
            fly(dom).fixDisplay();            
            
            // restore values after effect
            r = fly(dom).getFxRestore();      
            b = {x: xy[0], y: xy[1], 0: xy[0], 1: xy[1], width: dom.offsetWidth, height: dom.offsetHeight};
            b.right = b.x + b.width;
            b.bottom = b.y + b.height;
            
            // fixed size for slide
            fly(dom).setWidth(b.width).setHeight(b.height);            
            
            // wrap if needed
            wrap = fly(dom).fxWrap(r.pos, o, HIDDEN);
            
            st.visibility = VISIBLE;
            st.position = ABSOLUTE;
            
            // clear out temp styles after slide and unwrap
            function after(){
                 fly(dom).fxUnwrap(wrap, r.pos, o);
                 st.width = r.width;
                 st.height = r.height;
                 fly(dom).afterFx(o);
            }
            
            // time to calculate the positions        
            pt = {to: [b.x, b.y]}; 
            bw = {to: b.width};
            bh = {to: b.height};
                
            function argCalc(wrap, style, ww, wh, sXY, sXYval, s1, s2, w, h, p){                    
                var ret = {};
                fly(wrap).setWidth(ww).setHeight(wh);
                if(fly(wrap)[sXY]){
                    fly(wrap)[sXY](sXYval);                  
                }
                style[s1] = style[s2] = "0";                    
                if(w){
                    ret.width = w
                };
                if(h){
                    ret.height = h;
                }
                if(p){
                    ret.points = p;
                }
                return ret;
            };

            args = fly(dom).switchStatements(anchor.toLowerCase(), argCalc, {
                    t  : [wrap, st, b.width, 0, NULL, NULL, LEFT, BOTTOM, NULL, bh, NULL],
                    l  : [wrap, st, 0, b.height, NULL, NULL, RIGHT, TOP, bw, NULL, NULL],
                    r  : [wrap, st, b.width, b.height, SETX, b.right, LEFT, TOP, NULL, NULL, pt],
                    b  : [wrap, st, b.width, b.height, SETY, b.bottom, LEFT, TOP, NULL, bh, pt],
                    tl : [wrap, st, 0, 0, NULL, NULL, RIGHT, BOTTOM, bw, bh, pt],
                    bl : [wrap, st, 0, 0, SETY, b.y + b.height, RIGHT, TOP, bw, bh, pt],
                    br : [wrap, st, 0, 0, SETXY, [b.right, b.bottom], LEFT, TOP, bw, bh, pt],
                    tr : [wrap, st, 0, 0, SETX, b.x + b.width, LEFT, BOTTOM, bw, bh, pt]
                });
            
            st.visibility = VISIBLE;
            fly(wrap).show();

            arguments.callee.anim = fly(wrap).fxanim(args,
                o,
                MOTION,
                .5,
                EASEOUT, 
                after);
        });
        return me;
    },
    
    /**
     * Slides the element out of view.  An anchor point can be optionally passed to set the end point
     * for the slide effect.  When the effect is completed, the element will be hidden (visibility = 
     * 'hidden') but block elements will still take up space in the document.  The element must be removed
     * from the DOM using the 'remove' config option if desired.  This function automatically handles 
     * wrapping the element with a fixed-size container if needed.  See the Fx class overview for valid anchor point options.
     * Usage:
     *<pre><code>
// default: slide the element out to the top
el.slideOut();

// custom: slide the element out to the right with a 2-second duration
el.slideOut('r', { duration: 2 });

// common config options shown with default values
el.slideOut('t', {
    easing: 'easeOut',
    duration: .5,
    remove: false,
    useDisplay: false
});
</code></pre>
     * @param {String} anchor (optional) One of the valid Fx anchor positions (defaults to top: 't')
     * @param {Object} options (optional) Object literal with any of the Fx config options
     * @return {Ext.Element} The Element
     */
    slideOut : function(anchor, o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            st = dom.style,
            xy = me.getXY(),
            wrap,
            r,
            b,
            a,
            zero = {to: 0}; 
                    
        anchor = anchor || "t";

        me.queueFx(o, function(){
            
            // restore values after effect
            r = fly(dom).getFxRestore(); 
            b = {x: xy[0], y: xy[1], 0: xy[0], 1: xy[1], width: dom.offsetWidth, height: dom.offsetHeight};
            b.right = b.x + b.width;
            b.bottom = b.y + b.height;
                
            // fixed size for slide   
            fly(dom).setWidth(b.width).setHeight(b.height);

            // wrap if needed
            wrap = fly(dom).fxWrap(r.pos, o, VISIBLE);
                
            st.visibility = VISIBLE;
            st.position = ABSOLUTE;
            fly(wrap).setWidth(b.width).setHeight(b.height);            

            function after(){
                o.useDisplay ? fly(dom).setDisplayed(FALSE) : fly(dom).hide();                
                fly(dom).fxUnwrap(wrap, r.pos, o);
                st.width = r.width;
                st.height = r.height;
                fly(dom).afterFx(o);
            }            
            
            function argCalc(style, s1, s2, p1, v1, p2, v2, p3, v3){                    
                var ret = {};
                
                style[s1] = style[s2] = "0";
                ret[p1] = v1;               
                if(p2){
                    ret[p2] = v2;               
                }
                if(p3){
                    ret[p3] = v3;
                }
                
                return ret;
            };
            
            a = fly(dom).switchStatements(anchor.toLowerCase(), argCalc, {
                t  : [st, LEFT, BOTTOM, HEIGHT, zero],
                l  : [st, RIGHT, TOP, WIDTH, zero],
                r  : [st, LEFT, TOP, WIDTH, zero, POINTS, {to : [b.right, b.y]}],
                b  : [st, LEFT, TOP, HEIGHT, zero, POINTS, {to : [b.x, b.bottom]}],
                tl : [st, RIGHT, BOTTOM, WIDTH, zero, HEIGHT, zero],
                bl : [st, RIGHT, TOP, WIDTH, zero, HEIGHT, zero, POINTS, {to : [b.x, b.bottom]}],
                br : [st, LEFT, TOP, WIDTH, zero, HEIGHT, zero, POINTS, {to : [b.x + b.width, b.bottom]}],
                tr : [st, LEFT, BOTTOM, WIDTH, zero, HEIGHT, zero, POINTS, {to : [b.right, b.y]}]
            });
            
            arguments.callee.anim = fly(wrap).fxanim(a,
                o,
                MOTION,
                .5,
                EASEOUT, 
                after);
        });
        return me;
    },

    /**
     * Fades the element out while slowly expanding it in all directions.  When the effect is completed, the 
     * element will be hidden (visibility = 'hidden') but block elements will still take up space in the document. 
     * The element must be removed from the DOM using the 'remove' config option if desired.
     * Usage:
     *<pre><code>
// default
el.puff();

// common config options shown with default values
el.puff({
    easing: 'easeOut',
    duration: .5,
    remove: false,
    useDisplay: false
});
</code></pre>
     * @param {Object} options (optional) Object literal with any of the Fx config options
     * @return {Ext.Element} The Element
     */
    puff : function(o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            st = dom.style,
            width,
            height,
            r;

        me.queueFx(o, function(){
            width = fly(dom).getWidth();
            height = fly(dom).getHeight();
            fly(dom).clearOpacity();
            fly(dom).show();

            // restore values after effect
            r = fly(dom).getFxRestore();                   
            
            function after(){
                o.useDisplay ? fly(dom).setDisplayed(FALSE) : fly(dom).hide();                  
                fly(dom).clearOpacity();  
                fly(dom).setPositioning(r.pos);
                st.width = r.width;
                st.height = r.height;
                st.fontSize = '';
                fly(dom).afterFx(o);
            }   

            arguments.callee.anim = fly(dom).fxanim({
                    width : {to : fly(dom).adjustWidth(width * 2)},
                    height : {to : fly(dom).adjustHeight(height * 2)},
                    points : {by : [-width * .5, -height * .5]},
                    opacity : {to : 0},
                    fontSize: {to : 200, unit: "%"}
                },
                o,
                MOTION,
                .5,
                EASEOUT,
                 after);
        });
        return me;
    },

    /**
     * Blinks the element as if it was clicked and then collapses on its center (similar to switching off a television).
     * When the effect is completed, the element will be hidden (visibility = 'hidden') but block elements will still 
     * take up space in the document. The element must be removed from the DOM using the 'remove' config option if desired.
     * Usage:
     *<pre><code>
// default
el.switchOff();

// all config options shown with default values
el.switchOff({
    easing: 'easeIn',
    duration: .3,
    remove: false,
    useDisplay: false
});
</code></pre>
     * @param {Object} options (optional) Object literal with any of the Fx config options
     * @return {Ext.Element} The Element
     */
    switchOff : function(o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            st = dom.style,
            r;

        me.queueFx(o, function(){
            fly(dom).clearOpacity();
            fly(dom).clip();

            // restore values after effect
            r = fly(dom).getFxRestore();
                
            function after(){
                o.useDisplay ? fly(dom).setDisplayed(FALSE) : fly(dom).hide();  
                fly(dom).clearOpacity();
                fly(dom).setPositioning(r.pos);
                st.width = r.width;
                st.height = r.height;   
                fly(dom).afterFx(o);
            };

            fly(dom).fxanim({opacity : {to : 0.3}}, 
                NULL, 
                NULL, 
                .1, 
                NULL, 
                function(){                                 
                    fly(dom).clearOpacity();
                        (function(){                            
                            fly(dom).fxanim({
                                height : {to : 1},
                                points : {by : [0, fly(dom).getHeight() * .5]}
                            }, 
                            o, 
                            MOTION, 
                            0.3, 
                            'easeIn', 
                            after);
                        }).defer(100);
                });
        });
        return me;
    },

    /**
     * Highlights the Element by setting a color (applies to the background-color by default, but can be
     * changed using the "attr" config option) and then fading back to the original color. If no original
     * color is available, you should provide the "endColor" config option which will be cleared after the animation.
     * Usage:
<pre><code>
// default: highlight background to yellow
el.highlight();

// custom: highlight foreground text to blue for 2 seconds
el.highlight("0000ff", { attr: 'color', duration: 2 });

// common config options shown with default values
el.highlight("ffff9c", {
    attr: "background-color", //can be any valid CSS property (attribute) that supports a color value
    endColor: (current color) or "ffffff",
    easing: 'easeIn',
    duration: 1
});
</code></pre>
     * @param {String} color (optional) The highlight color. Should be a 6 char hex color without the leading # (defaults to yellow: 'ffff9c')
     * @param {Object} options (optional) Object literal with any of the Fx config options
     * @return {Ext.Element} The Element
     */ 
    highlight : function(color, o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            attr = o.attr || "backgroundColor",
            a = {},
            restore;

        me.queueFx(o, function(){
            fly(dom).clearOpacity();
            fly(dom).show();

            function after(){
                dom.style[attr] = restore;
                fly(dom).afterFx(o);
            }            
            restore = dom.style[attr];
            a[attr] = {from: color || "ffff9c", to: o.endColor || fly(dom).getColor(attr) || "ffffff"};
            arguments.callee.anim = fly(dom).fxanim(a,
                o,
                'color',
                1,
                'easeIn', 
                after);
        });
        return me;
    },

   /**
    * Shows a ripple of exploding, attenuating borders to draw attention to an Element.
    * Usage:
<pre><code>
// default: a single light blue ripple
el.frame();

// custom: 3 red ripples lasting 3 seconds total
el.frame("ff0000", 3, { duration: 3 });

// common config options shown with default values
el.frame("C3DAF9", 1, {
    duration: 1 //duration of each individual ripple.
    // Note: Easing is not configurable and will be ignored if included
});
</code></pre>
    * @param {String} color (optional) The color of the border.  Should be a 6 char hex color without the leading # (defaults to light blue: 'C3DAF9').
    * @param {Number} count (optional) The number of ripples to display (defaults to 1)
    * @param {Object} options (optional) Object literal with any of the Fx config options
    * @return {Ext.Element} The Element
    */
    frame : function(color, count, o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            proxy,
            active;

        me.queueFx(o, function(){
            color = color || '#C3DAF9';
            if(color.length == 6){
                color = '#' + color;
            }            
            count = count || 1;
            fly(dom).show();

            var xy = fly(dom).getXY(),
                b = {x: xy[0], y: xy[1], 0: xy[0], 1: xy[1], width: dom.offsetWidth, height: dom.offsetHeight},
                queue = function(){
                    proxy = fly(document.body || document.documentElement).createChild({
                        style:{
                            position : ABSOLUTE,
                            'z-index': 35000, // yee haw
                            border : '0px solid ' + color
                        }
                    });
                    return proxy.queueFx({}, animFn);
                };
            
            
            arguments.callee.anim = {
                isAnimated: true,
                stop: function() {
                    count = 0;
                    proxy.stopFx();
                }
            };
            
            function animFn(){
                var scale = Ext.isBorderBox ? 2 : 1;
                active = proxy.anim({
                    top : {from : b.y, to : b.y - 20},
                    left : {from : b.x, to : b.x - 20},
                    borderWidth : {from : 0, to : 10},
                    opacity : {from : 1, to : 0},
                    height : {from : b.height, to : b.height + 20 * scale},
                    width : {from : b.width, to : b.width + 20 * scale}
                },{
                    duration: o.duration || 1,
                    callback: function() {
                        proxy.remove();
                        --count > 0 ? queue() : fly(dom).afterFx(o);
                    }
                });
                arguments.callee.anim = {
                    isAnimated: true,
                    stop: function(){
                        active.stop();
                    }
                };
            };
            queue();
        });
        return me;
    },

   /**
    * Creates a pause before any subsequent queued effects begin.  If there are
    * no effects queued after the pause it will have no effect.
    * Usage:
<pre><code>
el.pause(1);
</code></pre>
    * @param {Number} seconds The length of time to pause (in seconds)
    * @return {Ext.Element} The Element
    */
    pause : function(seconds){        
        var dom = this.dom,
            t;

        this.queueFx({}, function(){
            t = setTimeout(function(){
                fly(dom).afterFx({});
            }, seconds * 1000);
            arguments.callee.anim = {
                isAnimated: true,
                stop: function(){
                    clearTimeout(t);
                    fly(dom).afterFx({});
                }
            };
        });
        return this;
    },

   /**
    * Fade an element in (from transparent to opaque).  The ending opacity can be specified
    * using the <tt>{@link #endOpacity}</tt> config option.
    * Usage:
<pre><code>
// default: fade in from opacity 0 to 100%
el.fadeIn();

// custom: fade in from opacity 0 to 75% over 2 seconds
el.fadeIn({ endOpacity: .75, duration: 2});

// common config options shown with default values
el.fadeIn({
    endOpacity: 1, //can be any value between 0 and 1 (e.g. .5)
    easing: 'easeOut',
    duration: .5
});
</code></pre>
    * @param {Object} options (optional) Object literal with any of the Fx config options
    * @return {Ext.Element} The Element
    */
    fadeIn : function(o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            to = o.endOpacity || 1;
        
        me.queueFx(o, function(){
            fly(dom).setOpacity(0);
            fly(dom).fixDisplay();
            dom.style.visibility = VISIBLE;
            arguments.callee.anim = fly(dom).fxanim({opacity:{to:to}},
                o, NULL, .5, EASEOUT, function(){
                if(to == 1){
                    fly(dom).clearOpacity();
                }
                fly(dom).afterFx(o);
            });
        });
        return me;
    },

   /**
    * Fade an element out (from opaque to transparent).  The ending opacity can be specified
    * using the <tt>{@link #endOpacity}</tt> config option.  Note that IE may require
    * <tt>{@link #useDisplay}:true</tt> in order to redisplay correctly.
    * Usage:
<pre><code>
// default: fade out from the element's current opacity to 0
el.fadeOut();

// custom: fade out from the element's current opacity to 25% over 2 seconds
el.fadeOut({ endOpacity: .25, duration: 2});

// common config options shown with default values
el.fadeOut({
    endOpacity: 0, //can be any value between 0 and 1 (e.g. .5)
    easing: 'easeOut',
    duration: .5,
    remove: false,
    useDisplay: false
});
</code></pre>
    * @param {Object} options (optional) Object literal with any of the Fx config options
    * @return {Ext.Element} The Element
    */
    fadeOut : function(o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            style = dom.style,
            to = o.endOpacity || 0;         
        
        me.queueFx(o, function(){  
            arguments.callee.anim = fly(dom).fxanim({ 
                opacity : {to : to}},
                o, 
                NULL, 
                .5, 
                EASEOUT, 
                function(){
                    if(to == 0){
                        Ext.Element.data(dom, 'visibilityMode') == Ext.Element.DISPLAY || o.useDisplay ? 
                            style.display = "none" :
                            style.visibility = HIDDEN;
                            
                        fly(dom).clearOpacity();
                    }
                    fly(dom).afterFx(o);
            });
        });
        return me;
    },

   /**
    * Animates the transition of an element's dimensions from a starting height/width
    * to an ending height/width.  This method is a convenience implementation of {@link shift}.
    * Usage:
<pre><code>
// change height and width to 100x100 pixels
el.scale(100, 100);

// common config options shown with default values.  The height and width will default to
// the element&#39;s existing values if passed as null.
el.scale(
    [element&#39;s width],
    [element&#39;s height], {
        easing: 'easeOut',
        duration: .35
    }
);
</code></pre>
    * @param {Number} width  The new width (pass undefined to keep the original width)
    * @param {Number} height  The new height (pass undefined to keep the original height)
    * @param {Object} options (optional) Object literal with any of the Fx config options
    * @return {Ext.Element} The Element
    */
    scale : function(w, h, o){
        this.shift(Ext.apply({}, o, {
            width: w,
            height: h
        }));
        return this;
    },

   /**
    * Animates the transition of any combination of an element's dimensions, xy position and/or opacity.
    * Any of these properties not specified in the config object will not be changed.  This effect 
    * requires that at least one new dimension, position or opacity setting must be passed in on
    * the config object in order for the function to have any effect.
    * Usage:
<pre><code>
// slide the element horizontally to x position 200 while changing the height and opacity
el.shift({ x: 200, height: 50, opacity: .8 });

// common config options shown with default values.
el.shift({
    width: [element&#39;s width],
    height: [element&#39;s height],
    x: [element&#39;s x position],
    y: [element&#39;s y position],
    opacity: [element&#39;s opacity],
    easing: 'easeOut',
    duration: .35
});
</code></pre>
    * @param {Object} options  Object literal with any of the Fx config options
    * @return {Ext.Element} The Element
    */
    shift : function(o){
        o = getObject(o);
        var dom = this.dom,
            a = {};
                
        this.queueFx(o, function(){
            for (var prop in o) {
                if (o[prop] != UNDEFINED) {                                                 
                    a[prop] = {to : o[prop]};                   
                }
            } 
            
            a.width ? a.width.to = fly(dom).adjustWidth(o.width) : a;
            a.height ? a.height.to = fly(dom).adjustWidth(o.height) : a;   
            
            if (a.x || a.y || a.xy) {
                a.points = a.xy || 
                           {to : [ a.x ? a.x.to : fly(dom).getX(),
                                   a.y ? a.y.to : fly(dom).getY()]};                  
            }

            arguments.callee.anim = fly(dom).fxanim(a,
                o, 
                MOTION, 
                .35, 
                EASEOUT, 
                function(){
                    fly(dom).afterFx(o);
                });
        });
        return this;
    },

    /**
     * Slides the element while fading it out of view.  An anchor point can be optionally passed to set the 
     * ending point of the effect.
     * Usage:
     *<pre><code>
// default: slide the element downward while fading out
el.ghost();

// custom: slide the element out to the right with a 2-second duration
el.ghost('r', { duration: 2 });

// common config options shown with default values
el.ghost('b', {
    easing: 'easeOut',
    duration: .5,
    remove: false,
    useDisplay: false
});
</code></pre>
     * @param {String} anchor (optional) One of the valid Fx anchor positions (defaults to bottom: 'b')
     * @param {Object} options (optional) Object literal with any of the Fx config options
     * @return {Ext.Element} The Element
     */
    ghost : function(anchor, o){
        o = getObject(o);
        var me = this,
            dom = me.dom,
            st = dom.style,
            a = {opacity: {to: 0}, points: {}},
            pt = a.points,
            r,
            w,
            h;
            
        anchor = anchor || "b";

        me.queueFx(o, function(){
            // restore values after effect
            r = fly(dom).getFxRestore();
            w = fly(dom).getWidth();
            h = fly(dom).getHeight();
            
            function after(){
                o.useDisplay ? fly(dom).setDisplayed(FALSE) : fly(dom).hide();   
                fly(dom).clearOpacity();
                fly(dom).setPositioning(r.pos);
                st.width = r.width;
                st.height = r.height;
                fly(dom).afterFx(o);
            }
                
            pt.by = fly(dom).switchStatements(anchor.toLowerCase(), function(v1,v2){ return [v1, v2];}, {
               t  : [0, -h],
               l  : [-w, 0],
               r  : [w, 0],
               b  : [0, h],
               tl : [-w, -h],
               bl : [-w, h],
               br : [w, h],
               tr : [w, -h] 
            });
                
            arguments.callee.anim = fly(dom).fxanim(a,
                o,
                MOTION,
                .5,
                EASEOUT, after);
        });
        return me;
    },

    /**
     * Ensures that all effects queued after syncFx is called on the element are
     * run concurrently.  This is the opposite of {@link #sequenceFx}.
     * @return {Ext.Element} The Element
     */
    syncFx : function(){
        var me = this;
        me.fxDefaults = Ext.apply(me.fxDefaults || {}, {
            block : FALSE,
            concurrent : TRUE,
            stopFx : FALSE
        });
        return me;
    },

    /**
     * Ensures that all effects queued after sequenceFx is called on the element are
     * run in sequence.  This is the opposite of {@link #syncFx}.
     * @return {Ext.Element} The Element
     */
    sequenceFx : function(){
        var me = this;
        me.fxDefaults = Ext.apply(me.fxDefaults || {}, {
            block : FALSE,
            concurrent : FALSE,
            stopFx : FALSE
        });
        return me;
    },

    /* @private */
    nextFx : function(){        
        var ef = getQueue(this.dom.id)[0];
        if(ef){
            ef.call(this);
        }
    },

    /**
     * Returns true if the element has any effects actively running or queued, else returns false.
     * @return {Boolean} True if element has active effects, else false
     */
    hasActiveFx : function(){
        return getQueue(this.dom.id)[0];
    },

    /**
     * Stops any running effects and clears the element's internal effects queue if it contains
     * any additional effects that haven't started yet.
     * @return {Ext.Element} The Element
     */
    stopFx : function(finish){
        var me = this,
            id = me.dom.id;
        if(me.hasActiveFx()){
            var cur = getQueue(id)[0];
            if(cur && cur.anim){
                if(cur.anim.isAnimated){
                    setQueue(id, [cur]); //clear
                    cur.anim.stop(finish !== undefined ? finish : TRUE);
                }else{
                    setQueue(id, []);
                }
            }
        }
        return me;
    },

    /* @private */
    beforeFx : function(o){
        if(this.hasActiveFx() && !o.concurrent){
           if(o.stopFx){
               this.stopFx();
               return TRUE;
           }
           return FALSE;
        }
        return TRUE;
    },

    /**
     * Returns true if the element is currently blocking so that no other effect can be queued
     * until this effect is finished, else returns false if blocking is not set.  This is commonly
     * used to ensure that an effect initiated by a user action runs to completion prior to the
     * same effect being restarted (e.g., firing only one effect even if the user clicks several times).
     * @return {Boolean} True if blocking, else false
     */
    hasFxBlock : function(){
        var q = getQueue(this.dom.id);
        return q && q[0] && q[0].block;
    },

    /* @private */
    queueFx : function(o, fn){
        var me = fly(this.dom);
        if(!me.hasFxBlock()){
            Ext.applyIf(o, me.fxDefaults);
            if(!o.concurrent){
                var run = me.beforeFx(o);
                fn.block = o.block;
                getQueue(me.dom.id).push(fn);
                if(run){
                    me.nextFx();
                }
            }else{
                fn.call(me);
            }
        }
        return me;
    },

    /* @private */
    fxWrap : function(pos, o, vis){ 
        var dom = this.dom,
            wrap,
            wrapXY;
        if(!o.wrap || !(wrap = Ext.getDom(o.wrap))){            
            if(o.fixPosition){
                wrapXY = fly(dom).getXY();
            }
            var div = document.createElement("div");
            div.style.visibility = vis;
            wrap = dom.parentNode.insertBefore(div, dom);
            fly(wrap).setPositioning(pos);
            if(fly(wrap).isStyle(POSITION, "static")){
                fly(wrap).position("relative");
            }
            fly(dom).clearPositioning('auto');
            fly(wrap).clip();
            wrap.appendChild(dom);
            if(wrapXY){
                fly(wrap).setXY(wrapXY);
            }
        }
        return wrap;
    },

    /* @private */
    fxUnwrap : function(wrap, pos, o){      
        var dom = this.dom;
        fly(dom).clearPositioning();
        fly(dom).setPositioning(pos);
        if(!o.wrap){
            var pn = fly(wrap).dom.parentNode;
            pn.insertBefore(dom, wrap); 
            fly(wrap).remove();
        }
    },

    /* @private */
    getFxRestore : function(){
        var st = this.dom.style;
        return {pos: this.getPositioning(), width: st.width, height : st.height};
    },

    /* @private */
    afterFx : function(o){
        var dom = this.dom,
            id = dom.id;
        if(o.afterStyle){
            fly(dom).setStyle(o.afterStyle);            
        }
        if(o.afterCls){
            fly(dom).addClass(o.afterCls);
        }
        if(o.remove == TRUE){
            fly(dom).remove();
        }
        if(o.callback){
            o.callback.call(o.scope, fly(dom));
        }
        if(!o.concurrent){
            getQueue(id).shift();
            fly(dom).nextFx();
        }
    },

    /* @private */
    fxanim : function(args, opt, animType, defaultDur, defaultEase, cb){
        animType = animType || 'run';
        opt = opt || {};
        var anim = Ext.lib.Anim[animType](
                this.dom, 
                args,
                (opt.duration || defaultDur) || .35,
                (opt.easing || defaultEase) || EASEOUT,
                cb,            
                this
            );
        opt.anim = anim;
        return anim;
    }
};

// backwards compat
Ext.Fx.resize = Ext.Fx.scale;

//When included, Ext.Fx is automatically applied to Element so that all basic
//effects are available directly via the Element API
Ext.Element.addMethods(Ext.Fx);
})();
/**
 * @class Ext.CompositeElementLite
 * <p>This class encapsulates a <i>collection</i> of DOM elements, providing methods to filter
 * members, or to perform collective actions upon the whole set.</p>
 * <p>Although they are not listed, this class supports all of the methods of {@link Ext.Element} and
 * {@link Ext.Fx}. The methods from these classes will be performed on all the elements in this collection.</p>
 * Example:<pre><code>
var els = Ext.select("#some-el div.some-class");
// or select directly from an existing element
var el = Ext.get('some-el');
el.select('div.some-class');

els.setWidth(100); // all elements become 100 width
els.hide(true); // all elements fade out and hide
// or
els.setWidth(100).hide(true);
</code>
 */
Ext.CompositeElementLite = function(els, root){
    /**
     * <p>The Array of DOM elements which this CompositeElement encapsulates. Read-only.</p>
     * <p>This will not <i>usually</i> be accessed in developers' code, but developers wishing
     * to augment the capabilities of the CompositeElementLite class may use it when adding
     * methods to the class.</p>
     * <p>For example to add the <code>nextAll</code> method to the class to <b>add</b> all
     * following siblings of selected elements, the code would be</p><code><pre>
Ext.override(Ext.CompositeElementLite, {
    nextAll: function() {
        var els = this.elements, i, l = els.length, n, r = [], ri = -1;

//      Loop through all elements in this Composite, accumulating
//      an Array of all siblings.
        for (i = 0; i < l; i++) {
            for (n = els[i].nextSibling; n; n = n.nextSibling) {
                r[++ri] = n;
            }
        }

//      Add all found siblings to this Composite
        return this.add(r);
    }
});</pre></code>
     * @type Array
     * @property elements
     */
    this.elements = [];
    this.add(els, root);
    this.el = new Ext.Element.Flyweight();
};

Ext.CompositeElementLite.prototype = {
    isComposite: true,    
    
    // private
    getElement : function(el){
        // Set the shared flyweight dom property to the current element
        var e = this.el;
        e.dom = el;
        e.id = el.id;
        return e;
    },
    
    // private
    transformElement : function(el){
        return Ext.getDom(el);
    },
    
    /**
     * Returns the number of elements in this Composite.
     * @return Number
     */
    getCount : function(){
        return this.elements.length;
    },    
    /**
     * Adds elements to this Composite object.
     * @param {Mixed} els Either an Array of DOM elements to add, or another Composite object who's elements should be added.
     * @return {CompositeElement} This Composite object.
     */
    add : function(els, root){
        var me = this,
            elements = me.elements;
        if(!els){
            return this;
        }
        if(Ext.isString(els)){
            els = Ext.Element.selectorFunction(els, root);
        }else if(els.isComposite){
            els = els.elements;
        }else if(!Ext.isIterable(els)){
            els = [els];
        }
        
        for(var i = 0, len = els.length; i < len; ++i){
            elements.push(me.transformElement(els[i]));
        }
        return me;
    },
    
    invoke : function(fn, args){
        var me = this,
            els = me.elements,
            len = els.length, 
            e, 
            i;
            
        for(i = 0; i < len; i++) {
            e = els[i];
            if(e){
                Ext.Element.prototype[fn].apply(me.getElement(e), args);
            }
        }
        return me;
    },
    /**
     * Returns a flyweight Element of the dom element object at the specified index
     * @param {Number} index
     * @return {Ext.Element}
     */
    item : function(index){
        var me = this,
            el = me.elements[index],
            out = null;

        if(el){
            out = me.getElement(el);
        }
        return out;
    },

    // fixes scope with flyweight
    addListener : function(eventName, handler, scope, opt){
        var els = this.elements,
            len = els.length,
            i, e;
        
        for(i = 0; i<len; i++) {
            e = els[i];
            if(e) {
                Ext.EventManager.on(e, eventName, handler, scope || e, opt);
            }
        }
        return this;
    },
    /**
     * <p>Calls the passed function for each element in this composite.</p>
     * @param {Function} fn The function to call. The function is passed the following parameters:<ul>
     * <li><b>el</b> : Element<div class="sub-desc">The current Element in the iteration.
     * <b>This is the flyweight (shared) Ext.Element instance, so if you require a
     * a reference to the dom node, use el.dom.</b></div></li>
     * <li><b>c</b> : Composite<div class="sub-desc">This Composite object.</div></li>
     * <li><b>idx</b> : Number<div class="sub-desc">The zero-based index in the iteration.</div></li>
     * </ul>
     * @param {Object} scope (optional) The scope (<i>this</i> reference) in which the function is executed. (defaults to the Element)
     * @return {CompositeElement} this
     */
    each : function(fn, scope){       
        var me = this,
            els = me.elements,
            len = els.length,
            i, e;
        
        for(i = 0; i<len; i++) {
            e = els[i];
            if(e){
                e = this.getElement(e);
                if(fn.call(scope || e, e, me, i)){
                    break;
                }
            }
        }
        return me;
    },
    
    /**
    * Clears this Composite and adds the elements passed.
    * @param {Mixed} els Either an array of DOM elements, or another Composite from which to fill this Composite.
    * @return {CompositeElement} this
    */
    fill : function(els){
        var me = this;
        me.elements = [];
        me.add(els);
        return me;
    },
    
    /**
     * Filters this composite to only elements that match the passed selector.
     * @param {String/Function} selector A string CSS selector or a comparison function.
     * The comparison function will be called with the following arguments:<ul>
     * <li><code>el</code> : Ext.Element<div class="sub-desc">The current DOM element.</div></li>
     * <li><code>index</code> : Number<div class="sub-desc">The current index within the collection.</div></li>
     * </ul>
     * @return {CompositeElement} this
     */
    filter : function(selector){
        var els = [],
            me = this,
            elements = me.elements,
            fn = Ext.isFunction(selector) ? selector
                : function(el){
                    return el.is(selector);
                };
                
        
        me.each(function(el, self, i){
            if(fn(el, i) !== false){
                els[els.length] = me.transformElement(el);
            }
        });
        me.elements = els;
        return me;
    },
    
    /**
     * Find the index of the passed element within the composite collection.
     * @param el {Mixed} The id of an element, or an Ext.Element, or an HtmlElement to find within the composite collection.
     * @return Number The index of the passed Ext.Element in the composite collection, or -1 if not found.
     */
    indexOf : function(el){
        return this.elements.indexOf(this.transformElement(el));
    },
    
    /**
    * Replaces the specified element with the passed element.
    * @param {Mixed} el The id of an element, the Element itself, the index of the element in this composite
    * to replace.
    * @param {Mixed} replacement The id of an element or the Element itself.
    * @param {Boolean} domReplace (Optional) True to remove and replace the element in the document too.
    * @return {CompositeElement} this
    */    
    replaceElement : function(el, replacement, domReplace){
        var index = !isNaN(el) ? el : this.indexOf(el),
            d;
        if(index > -1){
            replacement = Ext.getDom(replacement);
            if(domReplace){
                d = this.elements[index];
                d.parentNode.insertBefore(replacement, d);
                Ext.removeNode(d);
            }
            this.elements.splice(index, 1, replacement);
        }
        return this;
    },
    
    /**
     * Removes all elements.
     */
    clear : function(){
        this.elements = [];
    }
};

Ext.CompositeElementLite.prototype.on = Ext.CompositeElementLite.prototype.addListener;

(function(){
var fnName,
    ElProto = Ext.Element.prototype,
    CelProto = Ext.CompositeElementLite.prototype;
    
for(fnName in ElProto){
    if(Ext.isFunction(ElProto[fnName])){
        (function(fnName){ 
            CelProto[fnName] = CelProto[fnName] || function(){
                return this.invoke(fnName, arguments);
            };
        }).call(CelProto, fnName);
        
    }
}
})();

if(Ext.DomQuery){
    Ext.Element.selectorFunction = Ext.DomQuery.select;
} 

/**
 * Selects elements based on the passed CSS selector to enable {@link Ext.Element Element} methods
 * to be applied to many related elements in one statement through the returned {@link Ext.CompositeElement CompositeElement} or
 * {@link Ext.CompositeElementLite CompositeElementLite} object.
 * @param {String/Array} selector The CSS selector or an array of elements
 * @param {HTMLElement/String} root (optional) The root element of the query or id of the root
 * @return {CompositeElementLite/CompositeElement}
 * @member Ext.Element
 * @method select
 */
Ext.Element.select = function(selector, root){
    var els;
    if(typeof selector == "string"){
        els = Ext.Element.selectorFunction(selector, root);
    }else if(selector.length !== undefined){
        els = selector;
    }else{
        throw "Invalid selector";
    }
    return new Ext.CompositeElementLite(els);
};
/**
 * Selects elements based on the passed CSS selector to enable {@link Ext.Element Element} methods
 * to be applied to many related elements in one statement through the returned {@link Ext.CompositeElement CompositeElement} or
 * {@link Ext.CompositeElementLite CompositeElementLite} object.
 * @param {String/Array} selector The CSS selector or an array of elements
 * @param {HTMLElement/String} root (optional) The root element of the query or id of the root
 * @return {CompositeElementLite/CompositeElement}
 * @member Ext
 * @method select
 */
Ext.select = Ext.Element.select;(function(){
    var BEFOREREQUEST = "beforerequest",
        REQUESTCOMPLETE = "requestcomplete",
        REQUESTEXCEPTION = "requestexception",
        UNDEFINED = undefined,
        LOAD = 'load',
        POST = 'POST',
        GET = 'GET',
        WINDOW = window;

    /**
     * @class Ext.data.Connection
     * @extends Ext.util.Observable
     * <p>The class encapsulates a connection to the page's originating domain, allowing requests to be made
     * either to a configured URL, or to a URL specified at request time.</p>
     * <p>Requests made by this class are asynchronous, and will return immediately. No data from
     * the server will be available to the statement immediately following the {@link #request} call.
     * To process returned data, use a
     * <a href="#request-option-success" ext:member="request-option-success" ext:cls="Ext.data.Connection">success callback</a>
     * in the request options object,
     * or an {@link #requestcomplete event listener}.</p>
     * <p><h3>File Uploads</h3><a href="#request-option-isUpload" ext:member="request-option-isUpload" ext:cls="Ext.data.Connection">File uploads</a> are not performed using normal "Ajax" techniques, that
     * is they are <b>not</b> performed using XMLHttpRequests. Instead the form is submitted in the standard
     * manner with the DOM <tt>&lt;form></tt> element temporarily modified to have its
     * <a href="http://www.w3.org/TR/REC-html40/present/frames.html#adef-target">target</a> set to refer
     * to a dynamically generated, hidden <tt>&lt;iframe></tt> which is inserted into the document
     * but removed after the return data has been gathered.</p>
     * <p>The server response is parsed by the browser to create the document for the IFRAME. If the
     * server is using JSON to send the return object, then the
     * <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.17">Content-Type</a> header
     * must be set to "text/html" in order to tell the browser to insert the text unchanged into the document body.</p>
     * <p>Characters which are significant to an HTML parser must be sent as HTML entities, so encode
     * "&lt;" as "&amp;lt;", "&amp;" as "&amp;amp;" etc.</p>
     * <p>The response text is retrieved from the document, and a fake XMLHttpRequest object
     * is created containing a <tt>responseText</tt> property in order to conform to the
     * requirements of event handlers and callbacks.</p>
     * <p>Be aware that file upload packets are sent with the content type <a href="http://www.faqs.org/rfcs/rfc2388.html">multipart/form</a>
     * and some server technologies (notably JEE) may require some custom processing in order to
     * retrieve parameter names and parameter values from the packet content.</p>
     * @constructor
     * @param {Object} config a configuration object.
     */
    Ext.data.Connection = function(config){
        Ext.apply(this, config);
        this.addEvents(
            /**
             * @event beforerequest
             * Fires before a network request is made to retrieve a data object.
             * @param {Connection} conn This Connection object.
             * @param {Object} options The options config object passed to the {@link #request} method.
             */
            BEFOREREQUEST,
            /**
             * @event requestcomplete
             * Fires if the request was successfully completed.
             * @param {Connection} conn This Connection object.
             * @param {Object} response The XHR object containing the response data.
             * See <a href="http://www.w3.org/TR/XMLHttpRequest/">The XMLHttpRequest Object</a>
             * for details.
             * @param {Object} options The options config object passed to the {@link #request} method.
             */
            REQUESTCOMPLETE,
            /**
             * @event requestexception
             * Fires if an error HTTP status was returned from the server.
             * See <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html">HTTP Status Code Definitions</a>
             * for details of HTTP status codes.
             * @param {Connection} conn This Connection object.
             * @param {Object} response The XHR object containing the response data.
             * See <a href="http://www.w3.org/TR/XMLHttpRequest/">The XMLHttpRequest Object</a>
             * for details.
             * @param {Object} options The options config object passed to the {@link #request} method.
             */
            REQUESTEXCEPTION
        );
        Ext.data.Connection.superclass.constructor.call(this);
    };

    Ext.extend(Ext.data.Connection, Ext.util.Observable, {
        /**
         * @cfg {String} url (Optional) <p>The default URL to be used for requests to the server. Defaults to undefined.</p>
         * <p>The <code>url</code> config may be a function which <i>returns</i> the URL to use for the Ajax request. The scope
         * (<code><b>this</b></code> reference) of the function is the <code>scope</code> option passed to the {@link #request} method.</p>
         */
        /**
         * @cfg {Object} extraParams (Optional) An object containing properties which are used as
         * extra parameters to each request made by this object. (defaults to undefined)
         */
        /**
         * @cfg {Object} defaultHeaders (Optional) An object containing request headers which are added
         *  to each request made by this object. (defaults to undefined)
         */
        /**
         * @cfg {String} method (Optional) The default HTTP method to be used for requests.
         * (defaults to undefined; if not set, but {@link #request} params are present, POST will be used;
         * otherwise, GET will be used.)
         */
        /**
         * @cfg {Number} timeout (Optional) The timeout in milliseconds to be used for requests. (defaults to 30000)
         */
        timeout : 30000,
        /**
         * @cfg {Boolean} autoAbort (Optional) Whether this request should abort any pending requests. (defaults to false)
         * @type Boolean
         */
        autoAbort:false,

        /**
         * @cfg {Boolean} disableCaching (Optional) True to add a unique cache-buster param to GET requests. (defaults to true)
         * @type Boolean
         */
        disableCaching: true,

        /**
         * @cfg {String} disableCachingParam (Optional) Change the parameter which is sent went disabling caching
         * through a cache buster. Defaults to '_dc'
         * @type String
         */
        disableCachingParam: '_dc',

        /**
         * <p>Sends an HTTP request to a remote server.</p>
         * <p><b>Important:</b> Ajax server requests are asynchronous, and this call will
         * return before the response has been received. Process any returned data
         * in a callback function.</p>
         * <pre><code>
Ext.Ajax.request({
   url: 'ajax_demo/sample.json',
   success: function(response, opts) {
      var obj = Ext.decode(response.responseText);
      console.dir(obj);
   },
   failure: function(response, opts) {
      console.log('server-side failure with status code ' + response.status);
   }
});
         * </code></pre>
         * <p>To execute a callback function in the correct scope, use the <tt>scope</tt> option.</p>
         * @param {Object} options An object which may contain the following properties:<ul>
         * <li><b>url</b> : String/Function (Optional)<div class="sub-desc">The URL to
         * which to send the request, or a function to call which returns a URL string. The scope of the
         * function is specified by the <tt>scope</tt> option. Defaults to the configured
         * <tt>{@link #url}</tt>.</div></li>
         * <li><b>params</b> : Object/String/Function (Optional)<div class="sub-desc">
         * An object containing properties which are used as parameters to the
         * request, a url encoded string or a function to call to get either. The scope of the function
         * is specified by the <tt>scope</tt> option.</div></li>
         * <li><b>method</b> : String (Optional)<div class="sub-desc">The HTTP method to use
         * for the request. Defaults to the configured method, or if no method was configured,
         * "GET" if no parameters are being sent, and "POST" if parameters are being sent.  Note that
         * the method name is case-sensitive and should be all caps.</div></li>
         * <li><b>callback</b> : Function (Optional)<div class="sub-desc">The
         * function to be called upon receipt of the HTTP response. The callback is
         * called regardless of success or failure and is passed the following
         * parameters:<ul>
         * <li><b>options</b> : Object<div class="sub-desc">The parameter to the request call.</div></li>
         * <li><b>success</b> : Boolean<div class="sub-desc">True if the request succeeded.</div></li>
         * <li><b>response</b> : Object<div class="sub-desc">The XMLHttpRequest object containing the response data.
         * See <a href="http://www.w3.org/TR/XMLHttpRequest/">http://www.w3.org/TR/XMLHttpRequest/</a> for details about
         * accessing elements of the response.</div></li>
         * </ul></div></li>
         * <li><a id="request-option-success"></a><b>success</b> : Function (Optional)<div class="sub-desc">The function
         * to be called upon success of the request. The callback is passed the following
         * parameters:<ul>
         * <li><b>response</b> : Object<div class="sub-desc">The XMLHttpRequest object containing the response data.</div></li>
         * <li><b>options</b> : Object<div class="sub-desc">The parameter to the request call.</div></li>
         * </ul></div></li>
         * <li><b>failure</b> : Function (Optional)<div class="sub-desc">The function
         * to be called upon failure of the request. The callback is passed the
         * following parameters:<ul>
         * <li><b>response</b> : Object<div class="sub-desc">The XMLHttpRequest object containing the response data.</div></li>
         * <li><b>options</b> : Object<div class="sub-desc">The parameter to the request call.</div></li>
         * </ul></div></li>
         * <li><b>scope</b> : Object (Optional)<div class="sub-desc">The scope in
         * which to execute the callbacks: The "this" object for the callback function. If the <tt>url</tt>, or <tt>params</tt> options were
         * specified as functions from which to draw values, then this also serves as the scope for those function calls.
         * Defaults to the browser window.</div></li>
         * <li><b>timeout</b> : Number (Optional)<div class="sub-desc">The timeout in milliseconds to be used for this request. Defaults to 30 seconds.</div></li>
         * <li><b>form</b> : Element/HTMLElement/String (Optional)<div class="sub-desc">The <tt>&lt;form&gt;</tt>
         * Element or the id of the <tt>&lt;form&gt;</tt> to pull parameters from.</div></li>
         * <li><a id="request-option-isUpload"></a><b>isUpload</b> : Boolean (Optional)<div class="sub-desc"><b>Only meaningful when used
         * with the <tt>form</tt> option</b>.
         * <p>True if the form object is a file upload (will be set automatically if the form was
         * configured with <b><tt>enctype</tt></b> "multipart/form-data").</p>
         * <p>File uploads are not performed using normal "Ajax" techniques, that is they are <b>not</b>
         * performed using XMLHttpRequests. Instead the form is submitted in the standard manner with the
         * DOM <tt>&lt;form></tt> element temporarily modified to have its
         * <a href="http://www.w3.org/TR/REC-html40/present/frames.html#adef-target">target</a> set to refer
         * to a dynamically generated, hidden <tt>&lt;iframe></tt> which is inserted into the document
         * but removed after the return data has been gathered.</p>
         * <p>The server response is parsed by the browser to create the document for the IFRAME. If the
         * server is using JSON to send the return object, then the
         * <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.17">Content-Type</a> header
         * must be set to "text/html" in order to tell the browser to insert the text unchanged into the document body.</p>
         * <p>The response text is retrieved from the document, and a fake XMLHttpRequest object
         * is created containing a <tt>responseText</tt> property in order to conform to the
         * requirements of event handlers and callbacks.</p>
         * <p>Be aware that file upload packets are sent with the content type <a href="http://www.faqs.org/rfcs/rfc2388.html">multipart/form</a>
         * and some server technologies (notably JEE) may require some custom processing in order to
         * retrieve parameter names and parameter values from the packet content.</p>
         * </div></li>
         * <li><b>headers</b> : Object (Optional)<div class="sub-desc">Request
         * headers to set for the request.</div></li>
         * <li><b>xmlData</b> : Object (Optional)<div class="sub-desc">XML document
         * to use for the post. Note: This will be used instead of params for the post
         * data. Any params will be appended to the URL.</div></li>
         * <li><b>jsonData</b> : Object/String (Optional)<div class="sub-desc">JSON
         * data to use as the post. Note: This will be used instead of params for the post
         * data. Any params will be appended to the URL.</div></li>
         * <li><b>disableCaching</b> : Boolean (Optional)<div class="sub-desc">True
         * to add a unique cache-buster param to GET requests.</div></li>
         * </ul></p>
         * <p>The options object may also contain any other property which might be needed to perform
         * postprocessing in a callback because it is passed to callback functions.</p>
         * @return {Number} transactionId The id of the server transaction. This may be used
         * to cancel the request.
         */
        request : function(o){
            var me = this;
            if(me.fireEvent(BEFOREREQUEST, me, o)){
                if (o.el) {
                    if(!Ext.isEmpty(o.indicatorText)){
                        me.indicatorText = '<div class="loading-indicator">'+o.indicatorText+"</div>";
                    }
                    if(me.indicatorText) {
                        Ext.getDom(o.el).innerHTML = me.indicatorText;
                    }
                    o.success = (Ext.isFunction(o.success) ? o.success : function(){}).createInterceptor(function(response) {
                        Ext.getDom(o.el).innerHTML = response.responseText;
                    });
                }

                var p = o.params,
                    url = o.url || me.url,
                    method,
                    cb = {success: me.handleResponse,
                          failure: me.handleFailure,
                          scope: me,
                          argument: {options: o},
                          timeout : o.timeout || me.timeout
                    },
                    form,
                    serForm;


                if (Ext.isFunction(p)) {
                    p = p.call(o.scope||WINDOW, o);
                }

                p = Ext.urlEncode(me.extraParams, Ext.isObject(p) ? Ext.urlEncode(p) : p);

                if (Ext.isFunction(url)) {
                    url = url.call(o.scope || WINDOW, o);
                }

                if((form = Ext.getDom(o.form))){
                    url = url || form.action;
                     if(o.isUpload || /multipart\/form-data/i.test(form.getAttribute("enctype"))) {
                         return me.doFormUpload.call(me, o, p, url);
                     }
                    serForm = Ext.lib.Ajax.serializeForm(form);
                    p = p ? (p + '&' + serForm) : serForm;
                }

                method = o.method || me.method || ((p || o.xmlData || o.jsonData) ? POST : GET);

                if(method === GET && (me.disableCaching && o.disableCaching !== false) || o.disableCaching === true){
                    var dcp = o.disableCachingParam || me.disableCachingParam;
                    url = Ext.urlAppend(url, dcp + '=' + (new Date().getTime()));
                }

                o.headers = Ext.apply(o.headers || {}, me.defaultHeaders || {});

                if(o.autoAbort === true || me.autoAbort) {
                    me.abort();
                }

                if((method == GET || o.xmlData || o.jsonData) && p){
                    url = Ext.urlAppend(url, p);
                    p = '';
                }
                return (me.transId = Ext.lib.Ajax.request(method, url, cb, p, o));
            }else{
                return o.callback ? o.callback.apply(o.scope, [o,UNDEFINED,UNDEFINED]) : null;
            }
        },

        /**
         * Determine whether this object has a request outstanding.
         * @param {Number} transactionId (Optional) defaults to the last transaction
         * @return {Boolean} True if there is an outstanding request.
         */
        isLoading : function(transId){
            return transId ? Ext.lib.Ajax.isCallInProgress(transId) : !! this.transId;
        },

        /**
         * Aborts any outstanding request.
         * @param {Number} transactionId (Optional) defaults to the last transaction
         */
        abort : function(transId){
            if(transId || this.isLoading()){
                Ext.lib.Ajax.abort(transId || this.transId);
            }
        },

        // private
        handleResponse : function(response){
            this.transId = false;
            var options = response.argument.options;
            response.argument = options ? options.argument : null;
            this.fireEvent(REQUESTCOMPLETE, this, response, options);
            if(options.success){
                options.success.call(options.scope, response, options);
            }
            if(options.callback){
                options.callback.call(options.scope, options, true, response);
            }
        },

        // private
        handleFailure : function(response, e){
            this.transId = false;
            var options = response.argument.options;
            response.argument = options ? options.argument : null;
            this.fireEvent(REQUESTEXCEPTION, this, response, options, e);
            if(options.failure){
                options.failure.call(options.scope, response, options);
            }
            if(options.callback){
                options.callback.call(options.scope, options, false, response);
            }
        },

        // private
        doFormUpload : function(o, ps, url){
            var id = Ext.id(),
                doc = document,
                frame = doc.createElement('iframe'),
                form = Ext.getDom(o.form),
                hiddens = [],
                hd,
                encoding = 'multipart/form-data',
                buf = {
                    target: form.target,
                    method: form.method,
                    encoding: form.encoding,
                    enctype: form.enctype,
                    action: form.action
                };

            Ext.fly(frame).set({
                id: id,
                name: id,
                cls: 'x-hidden'

            });

            doc.body.appendChild(frame);

            //Reset the Frame to neutral domain
            Ext.fly(frame).set({
               src : Ext.SSL_SECURE_URL
            });

            // This is required so that IE doesn't pop the response up in a new window.
            if(Ext.isIE){
               document.frames[id].name = id;
            }


            Ext.fly(form).set({
                target: id,
                method: POST,
                enctype: encoding,
                encoding: encoding,
                action: url || buf.action
            });

            // add dynamic params
            Ext.iterate(Ext.urlDecode(ps, false), function(k, v){
                hd = doc.createElement('input');
                Ext.fly(hd).set({
                    type: 'hidden',
                    value: v,
                    name: k
                });
                form.appendChild(hd);
                hiddens.push(hd);
            });

            function cb(){
                var me = this,
                    // bogus response object
                    r = {responseText : '',
                         responseXML : null,
                         argument : o.argument},
                    doc,
                    firstChild;

                try{
                    doc = frame.contentWindow.document || frame.contentDocument || WINDOW.frames[id].document;
                    if(doc){
                        if(doc.body){
                            if(/textarea/i.test((firstChild = doc.body.firstChild || {}).tagName)){ // json response wrapped in textarea
                                r.responseText = firstChild.value;
                            }else{
                                r.responseText = doc.body.innerHTML;
                            }
                        }
                        //in IE the document may still have a body even if returns XML.
                        r.responseXML = doc.XMLDocument || doc;
                    }
                }
                catch(e) {}

                Ext.EventManager.removeListener(frame, LOAD, cb, me);

                me.fireEvent(REQUESTCOMPLETE, me, r, o);

                function runCallback(fn, scope, args){
                    if(Ext.isFunction(fn)){
                        fn.apply(scope, args);
                    }
                }

                runCallback(o.success, o.scope, [r, o]);
                runCallback(o.callback, o.scope, [o, true, r]);

                if(!me.debugUploads){
                    setTimeout(function(){Ext.removeNode(frame);}, 100);
                }
            }

            Ext.EventManager.on(frame, LOAD, cb, this);
            form.submit();

            Ext.fly(form).set(buf);
            Ext.each(hiddens, function(h) {
                Ext.removeNode(h);
            });
        }
    });
})();

/**
 * @class Ext.Ajax
 * @extends Ext.data.Connection
 * <p>The global Ajax request class that provides a simple way to make Ajax requests
 * with maximum flexibility.</p>
 * <p>Since Ext.Ajax is a singleton, you can set common properties/events for it once
 * and override them at the request function level only if necessary.</p>
 * <p>Common <b>Properties</b> you may want to set are:<div class="mdetail-params"><ul>
 * <li><b><tt>{@link #method}</tt></b><p class="sub-desc"></p></li>
 * <li><b><tt>{@link #extraParams}</tt></b><p class="sub-desc"></p></li>
 * <li><b><tt>{@link #url}</tt></b><p class="sub-desc"></p></li>
 * </ul></div>
 * <pre><code>
// Default headers to pass in every request
Ext.Ajax.defaultHeaders = {
    'Powered-By': 'Ext'
};
 * </code></pre>
 * </p>
 * <p>Common <b>Events</b> you may want to set are:<div class="mdetail-params"><ul>
 * <li><b><tt>{@link Ext.data.Connection#beforerequest beforerequest}</tt></b><p class="sub-desc"></p></li>
 * <li><b><tt>{@link Ext.data.Connection#requestcomplete requestcomplete}</tt></b><p class="sub-desc"></p></li>
 * <li><b><tt>{@link Ext.data.Connection#requestexception requestexception}</tt></b><p class="sub-desc"></p></li>
 * </ul></div>
 * <pre><code>
// Example: show a spinner during all Ajax requests
Ext.Ajax.on('beforerequest', this.showSpinner, this);
Ext.Ajax.on('requestcomplete', this.hideSpinner, this);
Ext.Ajax.on('requestexception', this.hideSpinner, this);
 * </code></pre>
 * </p>
 * <p>An example request:</p>
 * <pre><code>
// Basic request
Ext.Ajax.{@link Ext.data.Connection#request request}({
   url: 'foo.php',
   success: someFn,
   failure: otherFn,
   headers: {
       'my-header': 'foo'
   },
   params: { foo: 'bar' }
});

// Simple ajax form submission
Ext.Ajax.{@link Ext.data.Connection#request request}({
    form: 'some-form',
    params: 'foo=bar'
});
 * </code></pre>
 * </p>
 * @singleton
 */
Ext.Ajax = new Ext.data.Connection({
    /**
     * @cfg {String} url @hide
     */
    /**
     * @cfg {Object} extraParams @hide
     */
    /**
     * @cfg {Object} defaultHeaders @hide
     */
    /**
     * @cfg {String} method (Optional) @hide
     */
    /**
     * @cfg {Number} timeout (Optional) @hide
     */
    /**
     * @cfg {Boolean} autoAbort (Optional) @hide
     */

    /**
     * @cfg {Boolean} disableCaching (Optional) @hide
     */

    /**
     * @property  disableCaching
     * True to add a unique cache-buster param to GET requests. (defaults to true)
     * @type Boolean
     */
    /**
     * @property  url
     * The default URL to be used for requests to the server. (defaults to undefined)
     * If the server receives all requests through one URL, setting this once is easier than
     * entering it on every request.
     * @type String
     */
    /**
     * @property  extraParams
     * An object containing properties which are used as extra parameters to each request made
     * by this object (defaults to undefined). Session information and other data that you need
     * to pass with each request are commonly put here.
     * @type Object
     */
    /**
     * @property  defaultHeaders
     * An object containing request headers which are added to each request made by this object
     * (defaults to undefined).
     * @type Object
     */
    /**
     * @property  method
     * The default HTTP method to be used for requests. Note that this is case-sensitive and
     * should be all caps (defaults to undefined; if not set but params are present will use
     * <tt>"POST"</tt>, otherwise will use <tt>"GET"</tt>.)
     * @type String
     */
    /**
     * @property  timeout
     * The timeout in milliseconds to be used for requests. (defaults to 30000)
     * @type Number
     */

    /**
     * @property  autoAbort
     * Whether a new request should abort any pending requests. (defaults to false)
     * @type Boolean
     */
    autoAbort : false,

    /**
     * Serialize the passed form into a url encoded string
     * @param {String/HTMLElement} form
     * @return {String}
     */
    serializeForm : function(form){
        return Ext.lib.Ajax.serializeForm(form);
    }
});
/**
 * @class Ext.util.JSON
 * Modified version of Douglas Crockford"s json.js that doesn"t
 * mess with the Object prototype
 * http://www.json.org/js.html
 * @singleton
 */
Ext.util.JSON = new (function(){
    var useHasOwn = !!{}.hasOwnProperty,
        isNative = function() {
            var useNative = null;

            return function() {
                if (useNative === null) {
                    useNative = Ext.USE_NATIVE_JSON && window.JSON && JSON.toString() == '[object JSON]';
                }
        
                return useNative;
            };
        }(),
        pad = function(n) {
            return n < 10 ? "0" + n : n;
        },
        doDecode = function(json){
            return eval("(" + json + ')');    
        },
        doEncode = function(o){
            if(!Ext.isDefined(o) || o === null){
                return "null";
            }else if(Ext.isArray(o)){
                return encodeArray(o);
            }else if(Ext.isDate(o)){
                return Ext.util.JSON.encodeDate(o);
            }else if(Ext.isString(o)){
                return encodeString(o);
            }else if(typeof o == "number"){
                //don't use isNumber here, since finite checks happen inside isNumber
                return isFinite(o) ? String(o) : "null";
            }else if(Ext.isBoolean(o)){
                return String(o);
            }else {
                var a = ["{"], b, i, v;
                for (i in o) {
                    // don't encode DOM objects
                    if(!o.getElementsByTagName){
                        if(!useHasOwn || o.hasOwnProperty(i)) {
                            v = o[i];
                            switch (typeof v) {
                            case "undefined":
                            case "function":
                            case "unknown":
                                break;
                            default:
                                if(b){
                                    a.push(',');
                                }
                                a.push(doEncode(i), ":",
                                        v === null ? "null" : doEncode(v));
                                b = true;
                            }
                        }
                    }
                }
                a.push("}");
                return a.join("");
            }    
        },
        m = {
            "\b": '\\b',
            "\t": '\\t',
            "\n": '\\n',
            "\f": '\\f',
            "\r": '\\r',
            '"' : '\\"',
            "\\": '\\\\'
        },
        encodeString = function(s){
            if (/["\\\x00-\x1f]/.test(s)) {
                return '"' + s.replace(/([\x00-\x1f\\"])/g, function(a, b) {
                    var c = m[b];
                    if(c){
                        return c;
                    }
                    c = b.charCodeAt();
                    return "\\u00" +
                        Math.floor(c / 16).toString(16) +
                        (c % 16).toString(16);
                }) + '"';
            }
            return '"' + s + '"';
        },
        encodeArray = function(o){
            var a = ["["], b, i, l = o.length, v;
                for (i = 0; i < l; i += 1) {
                    v = o[i];
                    switch (typeof v) {
                        case "undefined":
                        case "function":
                        case "unknown":
                            break;
                        default:
                            if (b) {
                                a.push(',');
                            }
                            a.push(v === null ? "null" : Ext.util.JSON.encode(v));
                            b = true;
                    }
                }
                a.push("]");
                return a.join("");
        };

    /**
     * <p>Encodes a Date. This returns the actual string which is inserted into the JSON string as the literal expression.
     * <b>The returned value includes enclosing double quotation marks.</b></p>
     * <p>The default return format is "yyyy-mm-ddThh:mm:ss".</p>
     * <p>To override this:</p><pre><code>
Ext.util.JSON.encodeDate = function(d) {
    return d.format('"Y-m-d"');
};
</code></pre>
     * @param {Date} d The Date to encode
     * @return {String} The string literal to use in a JSON string.
     */
    this.encodeDate = function(o){
        return '"' + o.getFullYear() + "-" +
                pad(o.getMonth() + 1) + "-" +
                pad(o.getDate()) + "T" +
                pad(o.getHours()) + ":" +
                pad(o.getMinutes()) + ":" +
                pad(o.getSeconds()) + '"';
    };

    /**
     * Encodes an Object, Array or other value
     * @param {Mixed} o The variable to encode
     * @return {String} The JSON string
     */
    this.encode = function() {
        var ec;
        return function(o) {
            if (!ec) {
                // setup encoding function on first access
                ec = isNative() ? JSON.stringify : doEncode;
            }
            return ec(o);
        };
    }();


    /**
     * Decodes (parses) a JSON string to an object. If the JSON is invalid, this function throws a SyntaxError unless the safe option is set.
     * @param {String} json The JSON string
     * @return {Object} The resulting object
     */
    this.decode = function() {
        var dc;
        return function(json) {
            if (!dc) {
                // setup decoding function on first access
                dc = isNative() ? JSON.parse : doDecode;
            }
            return dc(json);
        };
    }();

})();
/**
 * Shorthand for {@link Ext.util.JSON#encode}
 * @param {Mixed} o The variable to encode
 * @return {String} The JSON string
 * @member Ext
 * @method encode
 */
Ext.encode = Ext.util.JSON.encode;
/**
 * Shorthand for {@link Ext.util.JSON#decode}
 * @param {String} json The JSON string
 * @param {Boolean} safe (optional) Whether to return null or throw an exception if the JSON is invalid.
 * @return {Object} The resulting object
 * @member Ext
 * @method decode
 */
Ext.decode = Ext.util.JSON.decode;
