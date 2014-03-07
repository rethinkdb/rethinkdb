var fs = require('fs');
var path = require('path');

var UrlRewriter = require('../images/url-rewriter');

module.exports = function Inliner(context) {
  var process = function(data, options) {
    var tempData = [];
    var nextStart = 0;
    var nextEnd = 0;
    var cursor = 0;
    var isComment = commentScanner(data);

    options.relativeTo = options.relativeTo || options.root;
    options._baseRelativeTo = options._baseRelativeTo || options.relativeTo;
    options.visited = options.visited || [];

    for (; nextEnd < data.length; ) {
      nextStart = data.indexOf('@import', cursor);
      if (nextStart == -1)
        break;

      if (isComment(nextStart)) {
        cursor = nextStart + 1;
        continue;
      }

      nextEnd = data.indexOf(';', nextStart);
      if (nextEnd == -1) {
        tempData.push('');
        cursor = data.length;
        break;
      }

      tempData.push(data.substring(cursor, nextStart));
      tempData.push(inlinedFile(data, nextStart, nextEnd, options));
      cursor = nextEnd + 1;
    }

    return tempData.length > 0 ?
      tempData.join('') + data.substring(cursor, data.length) :
      data;
  };

  var commentScanner = function(data) {
    var commentRegex = /(\/\*(?!\*\/)[\s\S]*?\*\/)/;
    var lastStartIndex = 0;
    var lastEndIndex = 0;
    var noComments = false;

    // test whether an index is located within a comment
    var scanner = function(idx) {
      var comment;
      var localStartIndex = 0;
      var localEndIndex = 0;
      var globalStartIndex = 0;
      var globalEndIndex = 0;

      // return if we know there are no more comments
      if (noComments)
        return false;

      // idx can be still within last matched comment (many @import statements inside one comment)
      if (idx > lastStartIndex && idx < lastEndIndex)
        return true;

      comment = data.match(commentRegex);

      if (!comment) {
        noComments = true;
        return false;
      }

      // get the indexes relative to the current data chunk
      lastStartIndex = localStartIndex = comment.index;
      localEndIndex = localStartIndex + comment[0].length;

      // calculate the indexes relative to the full original data
      globalEndIndex = localEndIndex + lastEndIndex;
      globalStartIndex = globalEndIndex - comment[0].length;

      // chop off data up to and including current comment block
      data = data.substring(localEndIndex);
      lastEndIndex = globalEndIndex;

      // re-run scan if comment ended before the idx
      if (globalEndIndex < idx)
        return scanner(idx);

      return globalEndIndex > idx && idx > globalStartIndex;
    };

    return scanner;
  };

  var inlinedFile = function(data, nextStart, nextEnd, options) {
    var strippedImport = data
      .substring(data.indexOf(' ', nextStart) + 1, nextEnd)
      .replace(/^url\(/, '')
      .replace(/['"]/g, '');

    var separatorIndex = strippedImport.indexOf(' ');
    var importedFile = strippedImport
      .substring(0, separatorIndex > 0 ? separatorIndex : strippedImport.length)
      .replace(')', '');
    var mediaQuery = strippedImport
      .substring(importedFile.length + 1)
      .trim();

    if (/^(http|https):\/\//.test(importedFile) || /^\/\//.test(importedFile))
      return '@import url(' + importedFile + ')' + (mediaQuery.length > 0 ? ' ' + mediaQuery : '') + ';';

    var relativeTo = importedFile[0] == '/' ?
      options.root :
      options.relativeTo;

    var fullPath = path.resolve(path.join(relativeTo, importedFile));

    if (!fs.existsSync(fullPath) || !fs.statSync(fullPath).isFile()) {
      context.errors.push('Broken @import declaration of "' + importedFile + '"');
      return '';
    }

    if (options.visited.indexOf(fullPath) != -1)
      return '';

    options.visited.push(fullPath);

    var importedData = fs.readFileSync(fullPath, 'utf8');
    var importRelativeTo = path.dirname(fullPath);
    importedData = UrlRewriter.process(importedData, {
      relative: true,
      fromBase: importRelativeTo,
      toBase: options._baseRelativeTo
    });

    var inlinedData = process(importedData, {
      root: options.root,
      relativeTo: importRelativeTo,
      _baseRelativeTo: options.baseRelativeTo,
      visited: options.visited
    });
    return mediaQuery.length > 0 ?
      '@media ' + mediaQuery + '{' + inlinedData + '}' :
      inlinedData;
  };

  return {
    // Inlines all imports taking care of repetitions, unknown files, and circular dependencies
    process: process
  };
};
