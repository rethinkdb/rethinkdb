var EscapeStore = require('./escape-store');

module.exports = function Free() {
  var texts = new EscapeStore('FREE_TEXT');

  var findNonEscapedEnd = function(data, matched, start) {
    var end = start;
    while (true) {
      end = data.indexOf(matched, end);

      if (end > -1 && data[end - 1] == '\\') {
        end += 1;
        continue;
      } else {
        break;
      }
    }

    return end;
  };

  return {
    // Strip content tags by replacing them by the a special
    // marker for further restoring. It's done via string scanning
    // instead of regexps to speed up the process.
    escape: function(data) {
      var tempData = [];
      var nextStart = 0;
      var nextEnd = 0;
      var cursor = 0;
      var matchedParenthesis = null;
      var singleParenthesis = '\'';
      var doubleParenthesis = '"';
      var dataLength = data.length;

      for (; nextEnd < data.length; ) {
        var nextStartSingle = data.indexOf(singleParenthesis, nextEnd + 1);
        var nextStartDouble = data.indexOf(doubleParenthesis, nextEnd + 1);

        if (nextStartSingle == -1)
          nextStartSingle = dataLength;
        if (nextStartDouble == -1)
          nextStartDouble = dataLength;

        if (nextStartSingle < nextStartDouble) {
          nextStart = nextStartSingle;
          matchedParenthesis = singleParenthesis;
        } else {
          nextStart = nextStartDouble;
          matchedParenthesis = doubleParenthesis;
        }

        if (nextStart == -1)
          break;

        nextEnd = findNonEscapedEnd(data, matchedParenthesis, nextStart + 1);
        if (nextEnd == -1)
          break;

        var text = data.substring(nextStart, nextEnd + 1);
        var placeholder = texts.store(text);
        tempData.push(data.substring(cursor, nextStart));
        tempData.push(placeholder);
        cursor = nextEnd + 1;
      }

      return tempData.length > 0 ?
        tempData.join('') + data.substring(cursor, data.length) :
        data;
    },

    restore: function(data) {
      return data.replace(texts.placeholderRegExp, texts.restore);
    }
  };
};
