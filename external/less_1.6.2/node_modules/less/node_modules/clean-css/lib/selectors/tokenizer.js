module.exports = function Tokenizer(data, minifyContext) {
  var flatBlock = /^@(font\-face|page|\-ms\-viewport|\-o\-viewport|viewport)/;

  var whatsNext = function(context) {
    var cursor = context.cursor;
    var mode = context.mode;
    var closest;

    if (mode == 'body') {
      closest = data.indexOf('}', cursor);
      return closest > -1 ?
        [closest, 'bodyEnd'] :
        null;
    }

    var nextSpecial = data.indexOf('@', cursor);
    var nextEscape = mode == 'top' ? data.indexOf('__ESCAPED_COMMENT_CLEAN_CSS', cursor) : -1;
    var nextBodyStart = data.indexOf('{', cursor);
    var nextBodyEnd = data.indexOf('}', cursor);

    closest = nextSpecial;
    if (closest == -1 || (nextEscape > -1 && nextEscape < closest))
      closest = nextEscape;
    if (closest == -1 || (nextBodyStart > -1 && nextBodyStart < closest))
      closest = nextBodyStart;
    if (closest == -1 || (nextBodyEnd > -1 && nextBodyEnd < closest))
      closest = nextBodyEnd;

    if (closest == -1)
      return;
    if (nextEscape === closest)
      return [closest, 'escape'];
    if (nextBodyStart === closest)
      return [closest, 'bodyStart'];
    if (nextBodyEnd === closest)
      return [closest, 'bodyEnd'];
    if (nextSpecial === closest)
      return [closest, 'special'];
  };

  var tokenize = function(context) {
    var tokenized = [];

    context = context || { cursor: 0, mode: 'top' };

    while (true) {
      var next = whatsNext(context);
      if (!next) {
        var whatsLeft = data.substring(context.cursor);
        if (whatsLeft.length > 0) {
          tokenized.push(whatsLeft);
          context.cursor += whatsLeft.length;
        }
        break;
      }

      var nextSpecial = next[0];
      var what = next[1];
      var nextEnd;
      var oldMode;

      if (what == 'special') {
        var fragment = data.substring(nextSpecial, context.cursor + '@font-face'.length + 1);
        var isSingle = fragment.indexOf('@import') === 0 || fragment.indexOf('@charset') === 0;
        if (isSingle) {
          nextEnd = data.indexOf(';', nextSpecial + 1);
          tokenized.push(data.substring(context.cursor, nextEnd + 1));

          context.cursor = nextEnd + 1;
        } else {
          nextEnd = data.indexOf('{', nextSpecial + 1);
          var block = data.substring(context.cursor, nextEnd).trim();

          var isFlat = flatBlock.test(block);
          oldMode = context.mode;
          context.cursor = nextEnd + 1;
          context.mode = isFlat ? 'body' : 'block';
          var specialBody = tokenize(context);
          context.mode = oldMode;

          tokenized.push({ block: block, body: specialBody });
        }
      } else if (what == 'escape') {
        nextEnd = data.indexOf('__', nextSpecial + 1);
        var escaped = data.substring(context.cursor, nextEnd + 2);
        tokenized.push(escaped);

        context.cursor = nextEnd + 2;
      } else if (what == 'bodyStart') {
        var selector = data.substring(context.cursor, nextSpecial).trim();

        oldMode = context.mode;
        context.cursor = nextSpecial + 1;
        context.mode = 'body';
        var body = tokenize(context);
        context.mode = oldMode;

        tokenized.push({ selector: selector, body: body });
      } else if (what == 'bodyEnd') {
        // extra closing brace at the top level can be safely ignored
        if (context.mode == 'top') {
          var at = context.cursor;
          var warning = data[context.cursor] == '}' ?
            'Unexpected \'}\' in \'' + data.substring(at - 20, at + 20) + '\'. Ignoring.' :
            'Unexpected content: \'' + data.substring(at, nextSpecial + 1) + '\'. Ignoring.';

          minifyContext.warnings.push(warning);
          context.cursor = nextSpecial + 1;
          continue;
        }

        if (context.mode != 'block')
          tokenized = data.substring(context.cursor, nextSpecial);

        context.cursor = nextSpecial + 1;

        break;
      }
    }

    return tokenized;
  };

  return {
    process: function() {
      return tokenize();
    }
  };
};
