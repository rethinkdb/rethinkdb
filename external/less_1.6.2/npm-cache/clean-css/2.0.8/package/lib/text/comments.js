var EscapeStore = require('./escape-store');

module.exports = function Comments(keepSpecialComments, keepBreaks, lineBreak) {
  var comments = new EscapeStore('COMMENT');

  return {
    // Strip special comments (/*! ... */) by replacing them by a special marker
    // for further restoring. Plain comments are removed. It's done by scanning data using
    // String#indexOf scanning instead of regexps to speed up the process.
    escape: function(data) {
      var tempData = [];
      var nextStart = 0;
      var nextEnd = 0;
      var cursor = 0;

      for (; nextEnd < data.length; ) {
        nextStart = data.indexOf('/*', nextEnd);
        nextEnd = data.indexOf('*/', nextStart + 2);
        if (nextStart == -1 || nextEnd == -1)
          break;

        tempData.push(data.substring(cursor, nextStart));
        if (data[nextStart + 2] == '!') {
          // in case of special comments, replace them with a placeholder
          var comment = data.substring(nextStart, nextEnd + 2);
          var placeholder = comments.store(comment);
          tempData.push(placeholder);
        }
        cursor = nextEnd + 2;
      }

      return tempData.length > 0 ?
        tempData.join('') + data.substring(cursor, data.length) :
        data;
    },

    restore: function(data) {
      var restored = 0;
      var breakSuffix = keepBreaks ? lineBreak : '';

      return data.replace(new RegExp(comments.placeholderPattern + '(' + lineBreak + '| )?', 'g'), function(match, placeholder) {
        restored++;

        switch (keepSpecialComments) {
          case '*':
            return comments.restore(placeholder) + breakSuffix;
          case 1:
          case '1':
            return restored == 1 ?
              comments.restore(placeholder) + breakSuffix :
              '';
          case 0:
          case '0':
            return '';
        }
      });
    }
  };
};
