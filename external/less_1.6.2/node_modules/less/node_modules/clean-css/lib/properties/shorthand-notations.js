module.exports = function ShorthandNotations(data) {
  // shorthand notations
  var shorthandRegex = function(repeats, hasSuffix) {
    var pattern = '(padding|margin|border\\-width|border\\-color|border\\-style|border\\-radius):';
    for (var i = 0; i < repeats; i++)
      pattern += '([\\d\\w\\.%#\\(\\),]+)' + (i < repeats - 1 ? ' ' : '');
    return new RegExp(pattern + (hasSuffix ? '([;}])' : ''), 'g');
  };

  var from4Values = function() {
    return data.replace(shorthandRegex(4), function(match, property, size1, size2, size3, size4) {
      if (size1 === size2 && size1 === size3 && size1 === size4)
        return property + ':' + size1;
      else if (size1 === size3 && size2 === size4)
        return property + ':' + size1 + ' ' + size2;
      else if (size2 === size4)
        return property + ':' + size1 + ' ' + size2 + ' ' + size3;
      else
        return match;
    });
  };

  var from3Values = function() {
    return data.replace(shorthandRegex(3, true), function(match, property, size1, size2, size3, suffix) {
      if (size1 === size2 && size1 === size3)
        return property + ':' + size1 + suffix;
      else if (size1 === size3)
        return property + ':' + size1 + ' ' + size2 + suffix;
      else
        return match;
    });
  };

  var from2Values = function() {
    return data.replace(shorthandRegex(2, true), function(match, property, size1, size2, suffix) {
      if (size1 === size2)
        return property + ':' + size1 + suffix;
      else
        return match;
    });
  };

  return {
    process: function() {
      data = from4Values();
      data = from3Values();
      return from2Values();
    }
  };
};
