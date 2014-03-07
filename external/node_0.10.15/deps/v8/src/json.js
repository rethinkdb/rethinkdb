// Copyright 2009 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

var $JSON = global.JSON;

function Revive(holder, name, reviver) {
  var val = holder[name];
  if (IS_OBJECT(val)) {
    if (IS_ARRAY(val)) {
      var length = val.length;
      for (var i = 0; i < length; i++) {
        var newElement = Revive(val, $String(i), reviver);
        val[i] = newElement;
      }
    } else {
      for (var p in val) {
        if (%_CallFunction(val, p, ObjectHasOwnProperty)) {
          var newElement = Revive(val, p, reviver);
          if (IS_UNDEFINED(newElement)) {
            delete val[p];
          } else {
            val[p] = newElement;
          }
        }
      }
    }
  }
  return %_CallFunction(holder, name, val, reviver);
}

function JSONParse(text, reviver) {
  var unfiltered = %ParseJson(TO_STRING_INLINE(text));
  if (IS_SPEC_FUNCTION(reviver)) {
    return Revive({'': unfiltered}, '', reviver);
  } else {
    return unfiltered;
  }
}

function SerializeArray(value, replacer, stack, indent, gap) {
  if (!%PushIfAbsent(stack, value)) {
    throw MakeTypeError('circular_structure', $Array());
  }
  var stepback = indent;
  indent += gap;
  var partial = new InternalArray();
  var len = value.length;
  for (var i = 0; i < len; i++) {
    var strP = JSONSerialize($String(i), value, replacer, stack,
                             indent, gap);
    if (IS_UNDEFINED(strP)) {
      strP = "null";
    }
    partial.push(strP);
  }
  var final;
  if (gap == "") {
    final = "[" + partial.join(",") + "]";
  } else if (partial.length > 0) {
    var separator = ",\n" + indent;
    final = "[\n" + indent + partial.join(separator) + "\n" +
        stepback + "]";
  } else {
    final = "[]";
  }
  stack.pop();
  return final;
}

function SerializeObject(value, replacer, stack, indent, gap) {
  if (!%PushIfAbsent(stack, value)) {
    throw MakeTypeError('circular_structure', $Array());
  }
  var stepback = indent;
  indent += gap;
  var partial = new InternalArray();
  if (IS_ARRAY(replacer)) {
    var length = replacer.length;
    for (var i = 0; i < length; i++) {
      if (%_CallFunction(replacer, i, ObjectHasOwnProperty)) {
        var p = replacer[i];
        var strP = JSONSerialize(p, value, replacer, stack, indent, gap);
        if (!IS_UNDEFINED(strP)) {
          var member = %QuoteJSONString(p) + ":";
          if (gap != "") member += " ";
          member += strP;
          partial.push(member);
        }
      }
    }
  } else {
    for (var p in value) {
      if (%_CallFunction(value, p, ObjectHasOwnProperty)) {
        var strP = JSONSerialize(p, value, replacer, stack, indent, gap);
        if (!IS_UNDEFINED(strP)) {
          var member = %QuoteJSONString(p) + ":";
          if (gap != "") member += " ";
          member += strP;
          partial.push(member);
        }
      }
    }
  }
  var final;
  if (gap == "") {
    final = "{" + partial.join(",") + "}";
  } else if (partial.length > 0) {
    var separator = ",\n" + indent;
    final = "{\n" + indent + partial.join(separator) + "\n" +
        stepback + "}";
  } else {
    final = "{}";
  }
  stack.pop();
  return final;
}

function JSONSerialize(key, holder, replacer, stack, indent, gap) {
  var value = holder[key];
  if (IS_SPEC_OBJECT(value)) {
    var toJSON = value.toJSON;
    if (IS_SPEC_FUNCTION(toJSON)) {
      value = %_CallFunction(value, key, toJSON);
    }
  }
  if (IS_SPEC_FUNCTION(replacer)) {
    value = %_CallFunction(holder, key, value, replacer);
  }
  if (IS_STRING(value)) {
    return %QuoteJSONString(value);
  } else if (IS_NUMBER(value)) {
    return JSON_NUMBER_TO_STRING(value);
  } else if (IS_BOOLEAN(value)) {
    return value ? "true" : "false";
  } else if (IS_NULL(value)) {
    return "null";
  } else if (IS_SPEC_OBJECT(value) && !(typeof value == "function")) {
    // Non-callable object. If it's a primitive wrapper, it must be unwrapped.
    if (IS_ARRAY(value)) {
      return SerializeArray(value, replacer, stack, indent, gap);
    } else if (IS_NUMBER_WRAPPER(value)) {
      value = ToNumber(value);
      return JSON_NUMBER_TO_STRING(value);
    } else if (IS_STRING_WRAPPER(value)) {
      return %QuoteJSONString(ToString(value));
    } else if (IS_BOOLEAN_WRAPPER(value)) {
      return %_ValueOf(value) ? "true" : "false";
    } else {
      return SerializeObject(value, replacer, stack, indent, gap);
    }
  }
  // Undefined or a callable object.
  return void 0;
}


function BasicSerializeArray(value, stack, builder) {
  var len = value.length;
  if (len == 0) {
    builder.push("[]");
    return;
  }
  if (!%PushIfAbsent(stack, value)) {
    throw MakeTypeError('circular_structure', $Array());
  }
  builder.push("[");
  var val = value[0];
  if (IS_STRING(val)) {
    // First entry is a string. Remaining entries are likely to be strings too.
    var array_string = %QuoteJSONStringArray(value);
    if (!IS_UNDEFINED(array_string)) {
      // array_string also includes bracket characters so we are done.
      builder[builder.length - 1] = array_string;
      stack.pop();
      return;
    } else {
      builder.push(%QuoteJSONString(val));
      for (var i = 1; i < len; i++) {
        val = value[i];
        if (IS_STRING(val)) {
          builder.push(%QuoteJSONStringComma(val));
        } else {
          builder.push(",");
          var before = builder.length;
          BasicJSONSerialize(i, val, stack, builder);
          if (before == builder.length) builder[before - 1] = ",null";
        }
      }
    }
  } else if (IS_NUMBER(val)) {
    // First entry is a number. Remaining entries are likely to be numbers too.
    builder.push(JSON_NUMBER_TO_STRING(val));
    for (var i = 1; i < len; i++) {
      builder.push(",");
      val = value[i];
      if (IS_NUMBER(val)) {
        builder.push(JSON_NUMBER_TO_STRING(val));
      } else {
        var before = builder.length;
        BasicJSONSerialize(i, val, stack, builder);
        if (before == builder.length) builder[before - 1] = ",null";
      }
    }
  } else {
    var before = builder.length;
    BasicJSONSerialize(0, val, stack, builder);
    if (before == builder.length) builder.push("null");
    for (var i = 1; i < len; i++) {
      builder.push(",");
      before = builder.length;
      BasicJSONSerialize(i, value[i], stack, builder);
      if (before == builder.length) builder[before - 1] = ",null";
    }
  }
  stack.pop();
  builder.push("]");
}


function BasicSerializeObject(value, stack, builder) {
  if (!%PushIfAbsent(stack, value)) {
    throw MakeTypeError('circular_structure', $Array());
  }
  builder.push("{");
  var first = true;
  var keys = %ObjectKeys(value);
  var len = keys.length;
  for (var i = 0; i < len; i++) {
    var p = keys[i];
    if (!first) {
      builder.push(%QuoteJSONStringComma(p));
    } else {
      builder.push(%QuoteJSONString(p));
    }
    builder.push(":");
    var before = builder.length;
    BasicJSONSerialize(p, value[p], stack, builder);
    if (before == builder.length) {
      builder.pop();
      builder.pop();
    } else {
      first = false;
    }
  }
  stack.pop();
  builder.push("}");
}


function BasicJSONSerialize(key, value, stack, builder) {
  if (IS_SPEC_OBJECT(value)) {
    var toJSON = value.toJSON;
    if (IS_SPEC_FUNCTION(toJSON)) {
      value = %_CallFunction(value, ToString(key), toJSON);
    }
  }
  if (IS_STRING(value)) {
    builder.push(value !== "" ? %QuoteJSONString(value) : '""');
  } else if (IS_NUMBER(value)) {
    builder.push(JSON_NUMBER_TO_STRING(value));
  } else if (IS_BOOLEAN(value)) {
    builder.push(value ? "true" : "false");
  } else if (IS_NULL(value)) {
    builder.push("null");
  } else if (IS_SPEC_OBJECT(value) && !(typeof value == "function")) {
    // Value is a non-callable object.
    // Unwrap value if necessary
    if (IS_NUMBER_WRAPPER(value)) {
      value = ToNumber(value);
      builder.push(JSON_NUMBER_TO_STRING(value));
    } else if (IS_STRING_WRAPPER(value)) {
      builder.push(%QuoteJSONString(ToString(value)));
    } else if (IS_BOOLEAN_WRAPPER(value)) {
      builder.push(%_ValueOf(value) ? "true" : "false");
    } else if (IS_ARRAY(value)) {
      BasicSerializeArray(value, stack, builder);
    } else {
      BasicSerializeObject(value, stack, builder);
    }
  }
}


function JSONStringify(value, replacer, space) {
  if (%_ArgumentsLength() == 1) {
    var builder = new InternalArray();
    BasicJSONSerialize('', value, new InternalArray(), builder);
    if (builder.length == 0) return;
    var result = %_FastAsciiArrayJoin(builder, "");
    if (!IS_UNDEFINED(result)) return result;
    return %StringBuilderConcat(builder, builder.length, "");
  }
  if (IS_OBJECT(space)) {
    // Unwrap 'space' if it is wrapped
    if (IS_NUMBER_WRAPPER(space)) {
      space = ToNumber(space);
    } else if (IS_STRING_WRAPPER(space)) {
      space = ToString(space);
    }
  }
  var gap;
  if (IS_NUMBER(space)) {
    space = MathMax(0, MathMin(ToInteger(space), 10));
    gap = SubString("          ", 0, space);
  } else if (IS_STRING(space)) {
    if (space.length > 10) {
      gap = SubString(space, 0, 10);
    } else {
      gap = space;
    }
  } else {
    gap = "";
  }
  return JSONSerialize('', {'': value}, replacer, new InternalArray(), "", gap);
}

function SetUpJSON() {
  %CheckIsBootstrapping();
  InstallFunctions($JSON, DONT_ENUM, $Array(
    "parse", JSONParse,
    "stringify", JSONStringify
  ));
}

SetUpJSON();
