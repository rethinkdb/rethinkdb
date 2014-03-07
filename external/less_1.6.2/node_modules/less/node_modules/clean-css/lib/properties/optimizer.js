module.exports = function Optimizer() {
  var overridable = {
    'animation-delay': ['animation'],
    'animation-direction': ['animation'],
    'animation-duration': ['animation'],
    'animation-fill-mode': ['animation'],
    'animation-iteration-count': ['animation'],
    'animation-name': ['animation'],
    'animation-play-state': ['animation'],
    'animation-timing-function': ['animation'],
    '-moz-animation-delay': ['-moz-animation'],
    '-moz-animation-direction': ['-moz-animation'],
    '-moz-animation-duration': ['-moz-animation'],
    '-moz-animation-fill-mode': ['-moz-animation'],
    '-moz-animation-iteration-count': ['-moz-animation'],
    '-moz-animation-name': ['-moz-animation'],
    '-moz-animation-play-state': ['-moz-animation'],
    '-moz-animation-timing-function': ['-moz-animation'],
    '-o-animation-delay': ['-o-animation'],
    '-o-animation-direction': ['-o-animation'],
    '-o-animation-duration': ['-o-animation'],
    '-o-animation-fill-mode': ['-o-animation'],
    '-o-animation-iteration-count': ['-o-animation'],
    '-o-animation-name': ['-o-animation'],
    '-o-animation-play-state': ['-o-animation'],
    '-o-animation-timing-function': ['-o-animation'],
    '-webkit-animation-delay': ['-webkit-animation'],
    '-webkit-animation-direction': ['-webkit-animation'],
    '-webkit-animation-duration': ['-webkit-animation'],
    '-webkit-animation-fill-mode': ['-webkit-animation'],
    '-webkit-animation-iteration-count': ['-webkit-animation'],
    '-webkit-animation-name': ['-webkit-animation'],
    '-webkit-animation-play-state': ['-webkit-animation'],
    '-webkit-animation-timing-function': ['-webkit-animation'],
    'background-attachment': ['background'],
    'background-clip': ['background'],
    'background-color': ['background'],
    'background-image': ['background'],
    'background-origin': ['background'],
    'background-position': ['background'],
    'background-repeat': ['background'],
    'background-size': ['background'],
    'border-color': ['border'],
    'border-style': ['border'],
    'border-width': ['border'],
    'border-bottom': ['border'],
    'border-bottom-color': ['border-bottom', 'border-color', 'border'],
    'border-bottom-style': ['border-bottom', 'border-style', 'border'],
    'border-bottom-width': ['border-bottom', 'border-width', 'border'],
    'border-left': ['border'],
    'border-left-color': ['border-left', 'border-color', 'border'],
    'border-left-style': ['border-left', 'border-style', 'border'],
    'border-left-width': ['border-left', 'border-width', 'border'],
    'border-right': ['border'],
    'border-right-color': ['border-right', 'border-color', 'border'],
    'border-right-style': ['border-right', 'border-style', 'border'],
    'border-right-width': ['border-right', 'border-width', 'border'],
    'border-top': ['border'],
    'border-top-color': ['border-top', 'border-color', 'border'],
    'border-top-style': ['border-top', 'border-style', 'border'],
    'border-top-width': ['border-top', 'border-width', 'border'],
    'font-family': ['font'],
    'font-size': ['font'],
    'font-style': ['font'],
    'font-variant': ['font'],
    'font-weight': ['font'],
    'list-style-image': ['list'],
    'list-style-position': ['list'],
    'list-style-type': ['list'],
    'margin-bottom': ['margin'],
    'margin-left': ['margin'],
    'margin-right': ['margin'],
    'margin-top': ['margin'],
    'outline-color': ['outline'],
    'outline-style': ['outline'],
    'outline-width': ['outline'],
    'padding-bottom': ['padding'],
    'padding-left': ['padding'],
    'padding-right': ['padding'],
    'padding-top': ['padding'],
    'transition-delay': ['transition'],
    'transition-duration': ['transition'],
    'transition-property': ['transition'],
    'transition-timing-function': ['transition'],
    '-moz-transition-delay': ['-moz-transition'],
    '-moz-transition-duration': ['-moz-transition'],
    '-moz-transition-property': ['-moz-transition'],
    '-moz-transition-timing-function': ['-moz-transition'],
    '-o-transition-delay': ['-o-transition'],
    '-o-transition-duration': ['-o-transition'],
    '-o-transition-property': ['-o-transition'],
    '-o-transition-timing-function': ['-o-transition'],
    '-webkit-transition-delay': ['-webkit-transition'],
    '-webkit-transition-duration': ['-webkit-transition'],
    '-webkit-transition-property': ['-webkit-transition'],
    '-webkit-transition-timing-function': ['-webkit-transition']
  };

  var overrides = {};
  for (var granular in overridable) {
    for (var i = 0; i < overridable[granular].length; i++) {
      var coarse = overridable[granular][i];
      var list = overrides[coarse];

      if (list)
        list.push(granular);
      else
        overrides[coarse] = [granular];
    }
  }

  var tokenize = function(body) {
    var tokens = body.split(';');
    var keyValues = [];

    for (var i = 0, l = tokens.length; i < l; i++) {
      var token = tokens[i];
      if (token === '')
        continue;

      var firstColon = token.indexOf(':');
      keyValues.push([
        token.substring(0, firstColon),
        token.substring(firstColon + 1),
        token.indexOf('!important') > -1
      ]);
    }

    return keyValues;
  };

  var optimize = function(tokens, allowAdjacent) {
    var merged = [];
    var properties = [];
    var lastProperty = null;
    var rescanTrigger = {};

    var removeOverridenBy = function(property, isImportant) {
      var overrided = overrides[property];
      for (var i = 0, l = overrided.length; i < l; i++) {
        for (var j = 0; j < properties.length; j++) {
          if (properties[j] != overrided[i] || (merged[j][2] && !isImportant))
            continue;

          merged.splice(j, 1);
          properties.splice(j, 1);
          j -= 1;
        }
      }
    };

    var mergeablePosition = function(position) {
      if (allowAdjacent === false || allowAdjacent === true)
        return allowAdjacent;

      return allowAdjacent.indexOf(position) > -1;
    };

    tokensLoop:
    for (var i = 0, l = tokens.length; i < l; i++) {
      var token = tokens[i];
      var property = token[0];
      var isImportant = token[2];
      var _property = (property == '-ms-filter' || property == 'filter') ?
        (lastProperty == 'background' || lastProperty == 'background-image' ? lastProperty : property) :
        property;
      var toOverridePosition = 0;

      // comment is necessary - we assume that if two properties are one after another
      // then it is intentional way of redefining property which may not be widely supported
      // e.g. a{display:inline-block;display:-moz-inline-box}
      // however if `mergeablePosition` yields true then the rule does not apply
      // (e.g merging two adjacent selectors: `a{display:block}a{display:block}`)
      if (_property != lastProperty || mergeablePosition(i)) {
        while (true) {
          toOverridePosition = properties.indexOf(_property, toOverridePosition);
          if (toOverridePosition == -1)
            break;

          if (merged[toOverridePosition][2] && !isImportant)
            continue tokensLoop;

          merged.splice(toOverridePosition, 1);
          properties.splice(toOverridePosition, 1);
        }
      }

      merged.push(token);
      properties.push(_property);

      // certain properties (see values of `overridable`) should trigger removal of
      // more granular properties (see keys of `overridable`)
      if (rescanTrigger[_property])
        removeOverridenBy(_property, isImportant);

      // add rescan triggers - if certain property appears later in the list a rescan needs
      // to be triggered, e.g 'border-top' triggers a rescan after 'border-top-width' and
      // 'border-top-color' as they can be removed
      for (var j = 0, list = overridable[_property] || [], m = list.length; j < m; j++)
        rescanTrigger[list[j]] = true;

      lastProperty = _property;
    }

    return merged;
  };

  var rebuild = function(tokens) {
    var flat = [];

    for (var i = 0, l = tokens.length; i < l; i++) {
      flat.push(tokens[i][0] + ':' + tokens[i][1]);
    }

    return flat.join(';');
  };

  return {
    process: function(body, allowAdjacent) {
      var tokens = tokenize(body);
      if (tokens.length < 2)
        return body;

      var optimized = optimize(tokens, allowAdjacent);
      return rebuild(optimized);
    }
  };
};
