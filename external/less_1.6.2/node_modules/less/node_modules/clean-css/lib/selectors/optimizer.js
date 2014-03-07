var Tokenizer = require('./tokenizer');
var PropertyOptimizer = require('../properties/optimizer');

module.exports = function Optimizer(data, context, options) {
  var specialSelectors = {
    '*': /\-(moz|ms|o|webkit)\-/,
    'ie8': /(\-moz\-|\-ms\-|\-o\-|\-webkit\-|:not|:target|:visited|:empty|:first\-of|:last|:nth|:only|:root)/
  };

  var propertyOptimizer = new PropertyOptimizer();

  var cleanUpSelector = function(selectors) {
    var plain = [];
    selectors = selectors.split(',');

    for (var i = 0, l = selectors.length; i < l; i++) {
      var sel = selectors[i];

      if (plain.indexOf(sel) == -1)
        plain.push(sel);
    }

    return plain.sort().join(',');
  };

  var isSpecial = function(selector) {
    return specialSelectors[options.selectorsMergeMode || '*'].test(selector);
  };

  var removeDuplicates = function(tokens) {
    var matched = {};
    var forRemoval = [];

    for (var i = 0, l = tokens.length; i < l; i++) {
      if (typeof(tokens[i]) == 'string' || tokens[i].block)
        continue;

      var selector = tokens[i].selector;
      var body = tokens[i].body;
      var id = body + '@' + selector;
      var alreadyMatched = matched[id];

      if (alreadyMatched) {
        forRemoval.push(alreadyMatched[alreadyMatched.length - 1]);
        alreadyMatched.push(i);
      } else {
        matched[id] = [i];
      }
    }

    forRemoval = forRemoval.sort(function(a, b) { return a > b ? 1 : -1; });
    for (var j = 0, n = forRemoval.length; j < n; j++) {
      tokens.splice(forRemoval[j] - j, 1);
    }
  };

  var mergeAdjacent = function(tokens) {
    var forRemoval = [];
    var lastToken = { selector: null, body: null };

    for (var i = 0, l = tokens.length; i < l; i++) {
      var token = tokens[i];

      if (typeof(token) == 'string' || token.block)
        continue;

      if (token.selector == lastToken.selector) {
        var joinAt = [lastToken.body.split(';').length];
        lastToken.body = propertyOptimizer.process(lastToken.body + ';' + token.body, joinAt);
        forRemoval.push(i);
      } else if (token.body == lastToken.body && !isSpecial(token.selector) && !isSpecial(lastToken.selector)) {
        lastToken.selector = cleanUpSelector(lastToken.selector + ',' + token.selector);
        forRemoval.push(i);
      } else {
        lastToken = token;
      }
    }

    for (var j = 0, m = forRemoval.length; j < m; j++) {
      tokens.splice(forRemoval[j] - j, 1);
    }
  };

  var reduceNonAdjacent = function(tokens) {
    var matched = {};
    var matchedMoreThanOnce = [];
    var partiallyReduced = [];
    var token, selector, selectors;
    var removeEmpty = function(value) {
      return value.length > 0 ? value : '';
    };

    for (var i = 0, l = tokens.length; i < l; i++) {
      token = tokens[i];
      selector = token.selector;

      if (typeof(token) == 'string' || token.block)
        continue;

      selectors = selector.split(',');
      if (selectors.length > 1)
        selectors.push(selector);

      for (var j = 0, m = selectors.length; j < m; j++) {
        var sel = selectors[j];
        var alreadyMatched = matched[sel];
        if (alreadyMatched) {
          if (alreadyMatched.length == 1)
            matchedMoreThanOnce.push(sel);
          alreadyMatched.push(i);
        } else {
          matched[sel] = [i];
        }
      }
    }

    matchedMoreThanOnce.forEach(function(selector) {
      var matchPositions = matched[selector];
      var bodies = [];
      var joinsAt = [];
      for (var j = 0, m = matchPositions.length; j < m; j++) {
        var body = tokens[matchPositions[j]].body;
        bodies.push(body);
        joinsAt.push((joinsAt[j - 1] || 0) + body.split(';').length);
      }

      var optimizedBody = propertyOptimizer.process(bodies.join(';'), joinsAt);
      var optimizedTokens = optimizedBody.split(';');

      var k = optimizedTokens.length - 1;
      var currentMatch = matchPositions.length - 1;

      while (currentMatch >= 0) {
        if (bodies[currentMatch].indexOf(optimizedTokens[k]) > -1 && k > -1) {
          k -= 1;
          continue;
        }

        var tokenIndex = matchPositions[currentMatch];
        var token = tokens[tokenIndex];
        var reducedBody = optimizedTokens
          .splice(k + 1)
          .filter(removeEmpty)
          .join(';');

        if (token.selector == selector) {
          token.body = reducedBody;
        } else {
          token._partials = token._partials || [];
          token._partials.push(reducedBody);

          if (partiallyReduced.indexOf(tokenIndex) == -1)
            partiallyReduced.push(tokenIndex);
        }

        currentMatch -= 1;
      }
    });

    // process those tokens which were partially reduced
    // i.e. at least one of token's selectors saw reduction
    // if all selectors were reduced to same value we can override it
    for (i = 0, l = partiallyReduced.length; i < l; i++) {
      token = tokens[partiallyReduced[i]];
      selectors = token.selector.split(',');

      if (token._partials.length == selectors.length && token.body != token._partials[0]) {
        var newBody = token._partials[0];
        for (var k = 1, n = token._partials.length; k < n; k++) {
          if (token._partials[k] != newBody)
            break;
        }

        if (k == n)
          token.body = newBody;
      }

      delete token._partials;
    }
  };

  var optimize = function(tokens) {
    tokens = (Array.isArray(tokens) ? tokens : [tokens]);
    for (var i = 0, l = tokens.length; i < l; i++) {
      var token = tokens[i];

      if (token.selector) {
        token.selector = cleanUpSelector(token.selector);
        token.body = propertyOptimizer.process(token.body, false);
      } else if (token.block) {
        optimize(token.body);
      }
    }

    removeDuplicates(tokens);
    mergeAdjacent(tokens);
    reduceNonAdjacent(tokens);
  };

  var rebuild = function(tokens) {
    return (Array.isArray(tokens) ? tokens : [tokens])
      .map(function(token) {
        if (typeof token == 'string')
          return token;

        if (token.block)
          return token.block + '{' + rebuild(token.body) + '}';
        else
          return token.selector + '{' + token.body + '}';
      })
      .join(options.keepBreaks ? options.lineBreak : '');
  };

  return {
    process: function() {
      var tokenized = new Tokenizer(data, context).process();
      optimize(tokenized);
      return rebuild(tokenized);
    }
  };
};
