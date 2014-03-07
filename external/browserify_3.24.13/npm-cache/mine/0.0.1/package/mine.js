"use strict";

// Mine a string for require calls and export the module names
// Extract all require calls using a proper state-machine parser.
module.exports = mine;
function mine(js) {
  js = String(js);
  var names = [];
  var state = 0;
  var ident;
  var quote;
  var name;
  var start;

  var isIdent = /[a-z0-9_.]/i;
  var isWhitespace = /[ \r\n\t]/;

  function $start(char) {
    if (char === "/") {
      return $slash;
    }
    if (char === "'" || char === '"') {
      quote = char;
      return $string;
    }
    if (isIdent.test(char)) {
      ident = char;
      return $ident;
    }
    return $start;
  }

  function $ident(char) {
    if (isIdent.test(char)) {
      ident += char;
      return $ident;
    }
    if (char === "(" && ident === "require") {
      ident = undefined;
      return $call;
    }else{
      if (isWhitespace.test(char)){
        if (ident !== 'yield' && ident !== 'return'){
          return $ident;
        }
      }
    }
    return $start(char);
  }

  function $call(char) {
    if (isWhitespace.test(char)) return $call;
    if (char === "'" || char === '"') {
      quote = char;
      name = "";
      start = i + 1;
      return $name;
    }
    return $start(char);
  }

  function $name(char) {
    if (char === quote) {
      return $close;
    }
    name += char;
    return $name;
  }

  function $close(char) {
    if (isWhitespace.test(char)) return $close;
    if (char === ")" || char === ',') {
      names.push({
        name: name,
        offset: start
      });
    }
    name = undefined;
    return $start(char);
  }

  function $string(char) {
    if (char === "\\") {
      return $escape;
    }
    if (char === quote) {
      return $start;
    }
    return $string;
  }

  function $escape(char) {
    return $string;
  }

  function $slash(char) {
    if (char === "/") return $lineComment;
    if (char === "*") return $multilineComment;
    return $start(char);
  }

  function $lineComment(char) {
    if (char === "\r" || char === "\n") return $start;
    return $lineComment;
  }

  function $multilineComment(char) {
    if (char === "*") return $multilineEnding;
    return $multilineComment;
  }

  function $multilineEnding(char) {
    if (char === "/") return $start;
    if (char === "*") return $multilineEnding;
    return $multilineComment;
  }

  var state = $start;
  for (var i = 0, l = js.length; i < l; i++) {
    state = state(js[i]);
  }
  return names;
}