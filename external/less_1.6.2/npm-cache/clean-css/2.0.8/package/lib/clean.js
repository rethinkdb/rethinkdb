/**
 * Clean-css - https://github.com/GoalSmashers/clean-css
 * Released under the terms of MIT license
 *
 * Copyright (C) 2011-2013 GoalSmashers.com
 */

var ColorShortener = require('./colors/shortener');
var ColorHSLToHex = require('./colors/hsl-to-hex');
var ColorRGBToHex = require('./colors/rgb-to-hex');
var ColorLongToShortHex = require('./colors/long-to-short-hex');

var ShorthandNotations = require('./properties/shorthand-notations');
var ImportInliner = require('./imports/inliner');
var UrlRebase = require('./images/url-rebase');
var EmptyRemoval = require('./selectors/empty-removal');

var CommentsProcessor = require('./text/comments');
var ExpressionsProcessor = require('./text/expressions');
var FreeTextProcessor = require('./text/free');
var UrlsProcessor = require('./text/urls');

var SelectorsOptimizer = require('./selectors/optimizer');

module.exports = function(options) {
  var lineBreak = process.platform == 'win32' ? '\r\n' : '\n';
  var stats = {};
  var context = {
    errors: [],
    warnings: []
  };

  options = options || {};
  options.keepBreaks = options.keepBreaks || false;

  //active by default
  if (options.processImport === undefined)
    options.processImport = true;

  var minify = function(data) {
    if (Buffer.isBuffer(data))
      data = data.toString();

    var startedAt;
    if (options.debug) {
      startedAt = process.hrtime();
      stats.originalSize = data.length;
    }

    var replace = function() {
      if (typeof arguments[0] == 'function')
        arguments[0]();
      else
        data = data.replace.apply(data, arguments);
    };

    // replace function
    if (options.benchmark) {
      var originalReplace = replace;
      replace = function(pattern, replacement) {
        var name = typeof pattern == 'function' ?
          /function (\w+)\(/.exec(pattern.toString())[1] :
          pattern;

        var start = process.hrtime();
        originalReplace(pattern, replacement);

        var itTook = process.hrtime(start);
        console.log('%d ms: ' + name, 1000 * itTook[0] + itTook[1] / 1000000.0);
      };
    }

    var commentsProcessor = new CommentsProcessor(
      'keepSpecialComments' in options ? options.keepSpecialComments : '*',
      options.keepBreaks,
      lineBreak
    );
    var expressionsProcessor = new ExpressionsProcessor();
    var freeTextProcessor = new FreeTextProcessor();
    var urlsProcessor = new UrlsProcessor();
    var importInliner = new ImportInliner(context);

    if (options.processImport) {
      // inline all imports
      replace(function inlineImports() {
        data = importInliner.process(data, {
          root: options.root || process.cwd(),
          relativeTo: options.relativeTo
        });
      });
    }

    replace(function escapeComments() {
      data = commentsProcessor.escape(data);
    });

    // replace all escaped line breaks
    replace(/\\(\r\n|\n)/mg, '');

    // strip parentheses in urls if possible (no spaces inside)
    replace(/url\((['"])([^\)]+)['"]\)/g, function(match, quote, url) {
      var unsafeDataURI = url.indexOf('data:') === 0 && url.match(/data:\w+\/[^;]+;base64,/) === null;
      if (url.match(/[ \t]/g) !== null || unsafeDataURI)
        return 'url(' + quote + url + quote + ')';
      else
        return 'url(' + url + ')';
    });

    // strip parentheses in animation & font names
    replace(/(animation|animation\-name|font|font\-family):([^;}]+)/g, function(match, propertyName, def) {
      return propertyName + ':' + def.replace(/['"]([a-zA-Z][a-zA-Z\d\-_]+)['"]/g, '$1');
    });

    // strip parentheses in @keyframes
    replace(/@(\-moz\-|\-o\-|\-webkit\-)?keyframes ([^{]+)/g, function(match, prefix, name) {
      prefix = prefix || '';
      return '@' + prefix + 'keyframes ' + (name.indexOf(' ') > -1 ? name : name.replace(/['"]/g, ''));
    });

    // IE shorter filters, but only if single (IE 7 issue)
    replace(/progid:DXImageTransform\.Microsoft\.(Alpha|Chroma)(\([^\)]+\))([;}'"])/g, function(match, filter, args, suffix) {
      return filter.toLowerCase() + args + suffix;
    });

    replace(function escapeExpressions() {
      data = expressionsProcessor.escape(data);
    });

    // strip parentheses in attribute values
    replace(/\[([^\]]+)\]/g, function(match, content) {
      var eqIndex = content.indexOf('=');
      var singleQuoteIndex = content.indexOf('\'');
      var doubleQuoteIndex = content.indexOf('"');
      if (eqIndex < 0 && singleQuoteIndex < 0 && doubleQuoteIndex < 0)
        return match;
      if (singleQuoteIndex === 0 || doubleQuoteIndex === 0)
        return match;

      var key = content.substring(0, eqIndex);
      var value = content.substring(eqIndex + 1, content.length);

      if (/^['"](?:[a-zA-Z][a-zA-Z\d\-_]+)['"]$/.test(value))
        return '[' + key + '=' + value.substring(1, value.length - 1) + ']';
      else
        return match;
    });

    replace(function escapeFreeText() {
      data = freeTextProcessor.escape(data);
    });

    replace(function escapeUrls() {
      data = urlsProcessor.escape(data);
    });

    // line breaks
    replace(/[\r]?\n/g, ' ');

    // multiple whitespace
    replace(/[\t ]+/g, ' ');

    // multiple semicolons (with optional whitespace)
    replace(/;[ ]?;+/g, ';');

    // multiple line breaks to one
    replace(/ (?:\r\n|\n)/g, lineBreak);
    replace(/(?:\r\n|\n)+/g, lineBreak);

    // remove spaces around selectors
    replace(/ ([+~>]) /g, '$1');

    // remove extra spaces inside content
    replace(/([!\(\{\}:;=,\n]) /g, '$1');
    replace(/ ([!\)\{\};=,\n])/g, '$1');
    replace(/(?:\r\n|\n)\}/g, '}');
    replace(/([\{;,])(?:\r\n|\n)/g, '$1');
    replace(/ :([^\{\};]+)([;}])/g, ':$1$2');

    // restore spaces inside IE filters (IE 7 issue)
    replace(/progid:[^(]+\(([^\)]+)/g, function(match) {
      return match.replace(/,/g, ', ');
    });

    // trailing semicolons
    replace(/;\}/g, '}');

    replace(function hsl2Hex() {
      data = new ColorHSLToHex(data).process();
    });

    replace(function rgb2Hex() {
      data = new ColorRGBToHex(data).process();
    });

    replace(function longToShortHex() {
      data = new ColorLongToShortHex(data).process();
    });

    replace(function shortenColors() {
      data = new ColorShortener(data).process();
    });

    // replace font weight with numerical value
    replace(/(font\-weight|font):(normal|bold)([ ;\}!])(\w*)/g, function(match, property, weight, suffix, next) {
      if (suffix == ' ' && next.length > 0 && !/[.\d]/.test(next))
        return match;

      if (weight == 'normal')
        return property + ':400' + suffix + next;
      else if (weight == 'bold')
        return property + ':700' + suffix + next;
      else
        return match;
    });

    // zero + unit to zero
    replace(/(\s|:|,)0(?:px|em|ex|cm|mm|in|pt|pc|%)/g, '$1' + '0');
    replace(/rect\(0(?:px|em|ex|cm|mm|in|pt|pc|%)/g, 'rect(0');

    // fraction zeros removal
    replace(/\.([1-9]*)0+(\D)/g, function(match, nonZeroPart, suffix) {
      return (nonZeroPart ? '.' : '') + nonZeroPart + suffix;
    });

    // restore 0% in hsl/hsla
    replace(/(hsl|hsla)\(([^\)]+)\)/g, function(match, colorFunction, colorDef) {
      var tokens = colorDef.split(',');
      if (tokens[1] == '0')
        tokens[1] = '0%';
      if (tokens[2] == '0')
        tokens[2] = '0%';
      return colorFunction + '(' + tokens.join(',') + ')';
    });

    // none to 0
    replace(/(border|border-top|border-right|border-bottom|border-left|outline):none/g, '$1:0');

    // background:none to background:0 0
    replace(/background:(?:none|transparent)([;}])/g, 'background:0 0$1');

    // multiple zeros into one
    replace(/box-shadow:0 0 0 0([^\.])/g, 'box-shadow:0 0$1');
    replace(/:0 0 0 0([^\.])/g, ':0$1');
    replace(/([: ,=\-])0\.(\d)/g, '$1.$2');

    replace(function shorthandNotations() {
      data = new ShorthandNotations(data).process();
    });

    // restore rect(...) zeros syntax for 4 zeros
    replace(/rect\(\s?0(\s|,)0[ ,]0[ ,]0\s?\)/g, 'rect(0$10$10$10)');

    // remove universal selector when not needed (*#id, *.class etc)
    replace(/\*([\.#:\[])/g, '$1');

    // Restore spaces inside calc back
    replace(/calc\([^\}]+\}/g, function(match) {
      return match.replace(/\+/g, ' + ');
    });

    if (options.noAdvanced) {
      if (options.keepBreaks)
        replace(/\}/g, '}' + lineBreak);
    } else {
      replace(function optimizeSelectors() {
        data = new SelectorsOptimizer(data, context, {
          keepBreaks: options.keepBreaks,
          lineBreak: lineBreak,
          selectorsMergeMode: options.selectorsMergeMode
        }).process();
      });
    }

    replace(function restoreUrls() {
      data = urlsProcessor.restore(data);
    });
    replace(function rebaseUrls() {
      data = options.noRebase ? data : new UrlRebase(options, context).process(data);
    });
    replace(function restoreFreeText() {
      data = freeTextProcessor.restore(data);
    });
    replace(function restoreComments() {
      data = commentsProcessor.restore(data);
    });
    replace(function restoreExpressions() {
      data = expressionsProcessor.restore(data);
    });

    // move first charset to the beginning
    replace(function moveCharset() {
      // get first charset in stylesheet
      var match = data.match(/@charset [^;]+;/);
      var firstCharset = match ? match[0] : null;
      if (!firstCharset)
        return;

      // reattach first charset and remove all subsequent
      data = firstCharset +
        (options.keepBreaks ? lineBreak : '') +
        data.replace(new RegExp('@charset [^;]+;(' + lineBreak + ')?', 'g'), '').trim();
    });

    replace(function removeEmptySelectors() {
      data = new EmptyRemoval(data).process();
    });

    // trim spaces at beginning and end
    data = data.trim();

    if (options.debug) {
      var elapsed = process.hrtime(startedAt);
      stats.timeSpent = ~~(elapsed[0] * 1e3 + elapsed[1] / 1e6);
      stats.efficiency = 1 - data.length / stats.originalSize;
      stats.minifiedSize = data.length;
    }

    return data;
  };

  return {
    errors: context.errors,
    lineBreak: lineBreak,
    options: options,
    minify: minify,
    stats: stats,
    warnings: context.warnings
  };
};
