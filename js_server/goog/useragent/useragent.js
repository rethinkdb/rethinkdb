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
 * @fileoverview Rendering engine detection.
 * @see <a href="http://www.useragentstring.com/">User agent strings</a>
 * For information on the browser brand (such as Safari versus Chrome), see
 * goog.userAgent.product.
 * @see ../demos/useragent.html
 */

goog.provide('goog.userAgent');

goog.require('goog.string');


/**
 * @define {boolean} Whether we know at compile-time that the browser is IE.
 */
goog.userAgent.ASSUME_IE = false;


/**
 * @define {boolean} Whether we know at compile-time that the browser is GECKO.
 */
goog.userAgent.ASSUME_GECKO = false;


/**
 * @define {boolean} Whether we know at compile-time that the browser is WEBKIT.
 */
goog.userAgent.ASSUME_WEBKIT = false;


/**
 * @define {boolean} Whether we know at compile-time that the browser is a
 *     mobile device running WebKit e.g. iPhone or Android.
 */
goog.userAgent.ASSUME_MOBILE_WEBKIT = false;


/**
 * @define {boolean} Whether we know at compile-time that the browser is OPERA.
 */
goog.userAgent.ASSUME_OPERA = false;


/**
 * @define {boolean} Whether the {@code goog.userAgent.isVersion} function will
 *     return true for any version.
 */
goog.userAgent.ASSUME_ANY_VERSION = false;


/**
 * Whether we know the browser engine at compile-time.
 * @type {boolean}
 * @private
 */
goog.userAgent.BROWSER_KNOWN_ =
    goog.userAgent.ASSUME_IE ||
    goog.userAgent.ASSUME_GECKO ||
    goog.userAgent.ASSUME_MOBILE_WEBKIT ||
    goog.userAgent.ASSUME_WEBKIT ||
    goog.userAgent.ASSUME_OPERA;


/**
 * Returns the userAgent string for the current browser.
 * Some user agents (I'm thinking of you, Gears WorkerPool) do not expose a
 * navigator object off the global scope.  In that case we return null.
 *
 * @return {?string} The userAgent string or null if there is none.
 */
goog.userAgent.getUserAgentString = function() {
  return goog.global['navigator'] ? goog.global['navigator'].userAgent : null;
};


/**
 * @return {Object} The native navigator object.
 */
goog.userAgent.getNavigator = function() {
  // Need a local navigator reference instead of using the global one,
  // to avoid the rare case where they reference different objects.
  // (in a WorkerPool, for example).
  return goog.global['navigator'];
};


/**
 * Initializer for goog.userAgent.
 *
 * This is a named function so that it can be stripped via the jscompiler
 * option for stripping types.
 * @private
 */
goog.userAgent.init_ = function() {
  /**
   * Whether the user agent string denotes Opera.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedOpera_ = false;

  /**
   * Whether the user agent string denotes Internet Explorer. This includes
   * other browsers using Trident as its rendering engine. For example AOL
   * and Netscape 8
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedIe_ = false;

  /**
   * Whether the user agent string denotes WebKit. WebKit is the rendering
   * engine that Safari, Android and others use.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedWebkit_ = false;

  /**
   * Whether the user agent string denotes a mobile device.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedMobile_ = false;

  /**
   * Whether the user agent string denotes Gecko. Gecko is the rendering
   * engine used by Mozilla, Mozilla Firefox, Camino and many more.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedGecko_ = false;

  var ua;
  if (!goog.userAgent.BROWSER_KNOWN_ &&
      (ua = goog.userAgent.getUserAgentString())) {
    var navigator = goog.userAgent.getNavigator();
    goog.userAgent.detectedOpera_ = ua.indexOf('Opera') == 0;
    goog.userAgent.detectedIe_ = !goog.userAgent.detectedOpera_ &&
        ua.indexOf('MSIE') != -1;
    goog.userAgent.detectedWebkit_ = !goog.userAgent.detectedOpera_ &&
        ua.indexOf('WebKit') != -1;
    // WebKit also gives navigator.product string equal to 'Gecko'.
    goog.userAgent.detectedMobile_ = goog.userAgent.detectedWebkit_ &&
        ua.indexOf('Mobile') != -1;
    goog.userAgent.detectedGecko_ = !goog.userAgent.detectedOpera_ &&
        !goog.userAgent.detectedWebkit_ && navigator.product == 'Gecko';
  }
};


if (!goog.userAgent.BROWSER_KNOWN_) {
  goog.userAgent.init_();
}


/**
 * Whether the user agent is Opera.
 * @type {boolean}
 */
goog.userAgent.OPERA = goog.userAgent.BROWSER_KNOWN_ ?
    goog.userAgent.ASSUME_OPERA : goog.userAgent.detectedOpera_;


/**
 * Whether the user agent is Internet Explorer. This includes other browsers
 * using Trident as its rendering engine. For example AOL and Netscape 8
 * @type {boolean}
 */
goog.userAgent.IE = goog.userAgent.BROWSER_KNOWN_ ?
    goog.userAgent.ASSUME_IE : goog.userAgent.detectedIe_;


/**
 * Whether the user agent is Gecko. Gecko is the rendering engine used by
 * Mozilla, Mozilla Firefox, Camino and many more.
 * @type {boolean}
 */
goog.userAgent.GECKO = goog.userAgent.BROWSER_KNOWN_ ?
    goog.userAgent.ASSUME_GECKO :
    goog.userAgent.detectedGecko_;


/**
 * Whether the user agent is WebKit. WebKit is the rendering engine that
 * Safari, Android and others use.
 * @type {boolean}
 */
goog.userAgent.WEBKIT = goog.userAgent.BROWSER_KNOWN_ ?
    goog.userAgent.ASSUME_WEBKIT || goog.userAgent.ASSUME_MOBILE_WEBKIT :
    goog.userAgent.detectedWebkit_;


/**
 * Whether the user agent is running on a mobile device.
 * @type {boolean}
 */
goog.userAgent.MOBILE = goog.userAgent.ASSUME_MOBILE_WEBKIT ||
                        goog.userAgent.detectedMobile_;


/**
 * Used while transitioning code to use WEBKIT instead.
 * @type {boolean}
 * @deprecated Use {@link goog.userAgent.product.SAFARI} instead.
 * TODO(nicksantos): Delete this from goog.userAgent.
 */
goog.userAgent.SAFARI = goog.userAgent.WEBKIT;


/**
 * @return {string} the platform (operating system) the user agent is running
 *     on. Default to empty string because navigator.platform may not be defined
 *     (on Rhino, for example).
 * @private
 */
goog.userAgent.determinePlatform_ = function() {
  var navigator = goog.userAgent.getNavigator();
  return navigator && navigator.platform || '';
};


/**
 * The platform (operating system) the user agent is running on. Default to
 * empty string because navigator.platform may not be defined (on Rhino, for
 * example).
 * @type {string}
 */
goog.userAgent.PLATFORM = goog.userAgent.determinePlatform_();


/**
 * @define {boolean} Whether the user agent is running on a Macintosh operating
 *     system.
 */
goog.userAgent.ASSUME_MAC = false;


/**
 * @define {boolean} Whether the user agent is running on a Windows operating
 *     system.
 */
goog.userAgent.ASSUME_WINDOWS = false;


/**
 * @define {boolean} Whether the user agent is running on a Linux operating
 *     system.
 */
goog.userAgent.ASSUME_LINUX = false;


/**
 * @define {boolean} Whether the user agent is running on a X11 windowing
 *     system.
 */
goog.userAgent.ASSUME_X11 = false;


/**
 * @type {boolean}
 * @private
 */
goog.userAgent.PLATFORM_KNOWN_ =
    goog.userAgent.ASSUME_MAC ||
    goog.userAgent.ASSUME_WINDOWS ||
    goog.userAgent.ASSUME_LINUX ||
    goog.userAgent.ASSUME_X11;


/**
 * Initialize the goog.userAgent constants that define which platform the user
 * agent is running on.
 * @private
 */
goog.userAgent.initPlatform_ = function() {
  /**
   * Whether the user agent is running on a Macintosh operating system.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedMac_ = goog.string.contains(goog.userAgent.PLATFORM,
      'Mac');

  /**
   * Whether the user agent is running on a Windows operating system.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedWindows_ = goog.string.contains(
      goog.userAgent.PLATFORM, 'Win');

  /**
   * Whether the user agent is running on a Linux operating system.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedLinux_ = goog.string.contains(goog.userAgent.PLATFORM,
      'Linux');

  /**
   * Whether the user agent is running on a X11 windowing system.
   * @type {boolean}
   * @private
   */
  goog.userAgent.detectedX11_ = !!goog.userAgent.getNavigator() &&
      goog.string.contains(goog.userAgent.getNavigator()['appVersion'] || '',
          'X11');
};


if (!goog.userAgent.PLATFORM_KNOWN_) {
  goog.userAgent.initPlatform_();
}


/**
 * Whether the user agent is running on a Macintosh operating system.
 * @type {boolean}
 */
goog.userAgent.MAC = goog.userAgent.PLATFORM_KNOWN_ ?
    goog.userAgent.ASSUME_MAC : goog.userAgent.detectedMac_;


/**
 * Whether the user agent is running on a Windows operating system.
 * @type {boolean}
 */
goog.userAgent.WINDOWS = goog.userAgent.PLATFORM_KNOWN_ ?
    goog.userAgent.ASSUME_WINDOWS : goog.userAgent.detectedWindows_;


/**
 * Whether the user agent is running on a Linux operating system.
 * @type {boolean}
 */
goog.userAgent.LINUX = goog.userAgent.PLATFORM_KNOWN_ ?
    goog.userAgent.ASSUME_LINUX : goog.userAgent.detectedLinux_;


/**
 * Whether the user agent is running on a X11 windowing system.
 * @type {boolean}
 */
goog.userAgent.X11 = goog.userAgent.PLATFORM_KNOWN_ ?
    goog.userAgent.ASSUME_X11 : goog.userAgent.detectedX11_;


/**
 * @return {string} The string that describes the version number of the user
 *     agent.
 * @private
 */
goog.userAgent.determineVersion_ = function() {
  // All browsers have different ways to detect the version and they all have
  // different naming schemes.

  // version is a string rather than a number because it may contain 'b', 'a',
  // and so on.
  var version = '', re;

  if (goog.userAgent.OPERA && goog.global['opera']) {
    var operaVersion = goog.global['opera'].version;
    version = typeof operaVersion == 'function' ? operaVersion() : operaVersion;
  } else {
    if (goog.userAgent.GECKO) {
      re = /rv\:([^\);]+)(\)|;)/;
    } else if (goog.userAgent.IE) {
      re = /MSIE\s+([^\);]+)(\)|;)/;
    } else if (goog.userAgent.WEBKIT) {
      // WebKit/125.4
      re = /WebKit\/(\S+)/;
    }
    if (re) {
      var arr = re.exec(goog.userAgent.getUserAgentString());
      version = arr ? arr[1] : '';
    }
  }
  if (goog.userAgent.IE) {
    // IE9 can be in document mode 9 but be reporting an inconsistent user agent
    // version.  If it is identifying as a version lower than 9 we take the
    // documentMode as the version instead.  IE8 has similar behavior.
    // It is recommended to set the X-UA-Compatible header to ensure that IE9
    // uses documentMode 9.
    var docMode = goog.userAgent.getDocumentMode_();
    if (docMode > parseFloat(version)) {
      return String(docMode);
    }
  }
  return version;
};


/**
 * @return {number|undefined} Returns the document mode (for testing).
 * @private
 */
goog.userAgent.getDocumentMode_ = function() {
  // NOTE(user): goog.userAgent may be used in context where there is no DOM.
  var doc = goog.global['document'];
  return doc ? doc['documentMode'] : undefined;
};


/**
 * The version of the user agent. This is a string because it might contain
 * 'b' (as in beta) as well as multiple dots.
 * @type {string}
 */
goog.userAgent.VERSION = goog.userAgent.determineVersion_();


/**
 * Compares two version numbers.
 *
 * @param {string} v1 Version of first item.
 * @param {string} v2 Version of second item.
 *
 * @return {number}  1 if first argument is higher
 *                   0 if arguments are equal
 *                  -1 if second argument is higher.
 * @deprecated Use goog.string.compareVersions.
 */
goog.userAgent.compare = function(v1, v2) {
  return goog.string.compareVersions(v1, v2);
};


/**
 * Cache for {@link goog.userAgent.isVersion}. Calls to compareVersions are
 * surprisingly expensive and as a browsers version number is unlikely to change
 * during a session we cache the results.
 * @type {Object}
 * @private
 */
goog.userAgent.isVersionCache_ = {};


/**
 * Whether the user agent version is higher or the same as the given version.
 * NOTE: When checking the version numbers for Firefox and Safari, be sure to
 * use the engine's version, not the browser's version number.  For example,
 * Firefox 3.0 corresponds to Gecko 1.9 and Safari 3.0 to Webkit 522.11.
 * Opera and Internet Explorer versions match the product release number.<br>
 * @see <a href="http://en.wikipedia.org/wiki/Safari_version_history">
 *     Webkit</a>
 * @see <a href="http://en.wikipedia.org/wiki/Gecko_engine">Gecko</a>
 *
 * @param {string|number} version The version to check.
 * @return {boolean} Whether the user agent version is higher or the same as
 *     the given version.
 */
goog.userAgent.isVersion = function(version) {
  return goog.userAgent.ASSUME_ANY_VERSION ||
      goog.userAgent.isVersionCache_[version] ||
      (goog.userAgent.isVersionCache_[version] =
          goog.string.compareVersions(goog.userAgent.VERSION, version) >= 0);
};


/**
 * Cache for {@link goog.userAgent.isDocumentMode}.
 * Browsers document mode version number is unlikely to change during a session
 * we cache the results.
 * @type {Object}
 * @private
 */
goog.userAgent.isDocumentModeCache_ = {};


/**
 * Whether the IE effective document mode is higher or the same as the given
 * document mode version.
 * NOTE: Only for IE, return false for another browser.
 *
 * @param {number} documentMode The document mode version to check.
 * @return {boolean} Whether the IE effective document mode is higher or the
 *     same as the given version.
 */
goog.userAgent.isDocumentMode = function(documentMode) {
  return goog.userAgent.isDocumentModeCache_[documentMode] ||
      (goog.userAgent.isDocumentModeCache_[documentMode] = goog.userAgent.IE &&
      !!document.documentMode && document.documentMode >= documentMode);
};
