var path = require('path');

module.exports = {
  process: function(data, options) {
    var tempData = [];
    var nextStart = 0;
    var nextEnd = 0;
    var cursor = 0;

    for (; nextEnd < data.length; ) {
      nextStart = data.indexOf('url(', nextEnd);
      if (nextStart == -1)
        break;

      nextEnd = data.indexOf(')', nextStart + 4);
      if (nextEnd == -1)
        break;

      tempData.push(data.substring(cursor, nextStart));
      var url = data.substring(nextStart + 4, nextEnd).replace(/['"]/g, '');
      tempData.push('url(' + this._rebased(url, options) + ')');
      cursor = nextEnd + 1;
    }

    return tempData.length > 0 ?
      tempData.join('') + data.substring(cursor, data.length) :
      data;
  },

  _rebased: function(url, options) {
    var specialUrl = url[0] == '/' ||
      url.substring(url.length - 4) == '.css' ||
      url.indexOf('data:') === 0 ||
      /^https?:\/\//.exec(url) !== null ||
      /__\w+__/.exec(url) !== null;
    var rebased;

    if (specialUrl)
      return url;

    if (options.absolute) {
      rebased = path
        .resolve(path.join(options.fromBase, url))
        .replace(options.toBase, '');
    } else {
      rebased = path.relative(options.toBase, path.join(options.fromBase, url));
    }

    return process.platform == 'win32' ?
      rebased.replace(/\\/g, '/') :
      rebased;
  }
};
