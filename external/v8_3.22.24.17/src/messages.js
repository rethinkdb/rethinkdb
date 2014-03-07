// Copyright 2012 the V8 project authors. All rights reserved.
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

// -------------------------------------------------------------------

var kMessages = {
  // Error
  cyclic_proto:                  ["Cyclic __proto__ value"],
  code_gen_from_strings:         ["%0"],
  generator_running:             ["Generator is already running"],
  generator_finished:            ["Generator has already finished"],
  // TypeError
  unexpected_token:              ["Unexpected token ", "%0"],
  unexpected_token_number:       ["Unexpected number"],
  unexpected_token_string:       ["Unexpected string"],
  unexpected_token_identifier:   ["Unexpected identifier"],
  unexpected_reserved:           ["Unexpected reserved word"],
  unexpected_strict_reserved:    ["Unexpected strict mode reserved word"],
  unexpected_eos:                ["Unexpected end of input"],
  malformed_regexp:              ["Invalid regular expression: /", "%0", "/: ", "%1"],
  unterminated_regexp:           ["Invalid regular expression: missing /"],
  regexp_flags:                  ["Cannot supply flags when constructing one RegExp from another"],
  incompatible_method_receiver:  ["Method ", "%0", " called on incompatible receiver ", "%1"],
  invalid_lhs_in_assignment:     ["Invalid left-hand side in assignment"],
  invalid_lhs_in_for_in:         ["Invalid left-hand side in for-in"],
  invalid_lhs_in_postfix_op:     ["Invalid left-hand side expression in postfix operation"],
  invalid_lhs_in_prefix_op:      ["Invalid left-hand side expression in prefix operation"],
  multiple_defaults_in_switch:   ["More than one default clause in switch statement"],
  newline_after_throw:           ["Illegal newline after throw"],
  redeclaration:                 ["%0", " '", "%1", "' has already been declared"],
  no_catch_or_finally:           ["Missing catch or finally after try"],
  unknown_label:                 ["Undefined label '", "%0", "'"],
  uncaught_exception:            ["Uncaught ", "%0"],
  stack_trace:                   ["Stack Trace:\n", "%0"],
  called_non_callable:           ["%0", " is not a function"],
  undefined_method:              ["Object ", "%1", " has no method '", "%0", "'"],
  property_not_function:         ["Property '", "%0", "' of object ", "%1", " is not a function"],
  cannot_convert_to_primitive:   ["Cannot convert object to primitive value"],
  not_constructor:               ["%0", " is not a constructor"],
  not_defined:                   ["%0", " is not defined"],
  non_object_property_load:      ["Cannot read property '", "%0", "' of ", "%1"],
  non_object_property_store:     ["Cannot set property '", "%0", "' of ", "%1"],
  non_object_property_call:      ["Cannot call method '", "%0", "' of ", "%1"],
  with_expression:               ["%0", " has no properties"],
  illegal_invocation:            ["Illegal invocation"],
  no_setter_in_callback:         ["Cannot set property ", "%0", " of ", "%1", " which has only a getter"],
  apply_non_function:            ["Function.prototype.apply was called on ", "%0", ", which is a ", "%1", " and not a function"],
  apply_wrong_args:              ["Function.prototype.apply: Arguments list has wrong type"],
  invalid_in_operator_use:       ["Cannot use 'in' operator to search for '", "%0", "' in ", "%1"],
  instanceof_function_expected:  ["Expecting a function in instanceof check, but got ", "%0"],
  instanceof_nonobject_proto:    ["Function has non-object prototype '", "%0", "' in instanceof check"],
  undefined_or_null_to_object:   ["Cannot convert undefined or null to object"],
  reduce_no_initial:             ["Reduce of empty array with no initial value"],
  getter_must_be_callable:       ["Getter must be a function: ", "%0"],
  setter_must_be_callable:       ["Setter must be a function: ", "%0"],
  value_and_accessor:            ["Invalid property.  A property cannot both have accessors and be writable or have a value, ", "%0"],
  proto_object_or_null:          ["Object prototype may only be an Object or null"],
  property_desc_object:          ["Property description must be an object: ", "%0"],
  redefine_disallowed:           ["Cannot redefine property: ", "%0"],
  define_disallowed:             ["Cannot define property:", "%0", ", object is not extensible."],
  non_extensible_proto:          ["%0", " is not extensible"],
  handler_non_object:            ["Proxy.", "%0", " called with non-object as handler"],
  proto_non_object:              ["Proxy.", "%0", " called with non-object as prototype"],
  trap_function_expected:        ["Proxy.", "%0", " called with non-function for '", "%1", "' trap"],
  handler_trap_missing:          ["Proxy handler ", "%0", " has no '", "%1", "' trap"],
  handler_trap_must_be_callable: ["Proxy handler ", "%0", " has non-callable '", "%1", "' trap"],
  handler_returned_false:        ["Proxy handler ", "%0", " returned false from '", "%1", "' trap"],
  handler_returned_undefined:    ["Proxy handler ", "%0", " returned undefined from '", "%1", "' trap"],
  proxy_prop_not_configurable:   ["Proxy handler ", "%0", " returned non-configurable descriptor for property '", "%2", "' from '", "%1", "' trap"],
  proxy_non_object_prop_names:   ["Trap '", "%1", "' returned non-object ", "%0"],
  proxy_repeated_prop_name:      ["Trap '", "%1", "' returned repeated property name '", "%2", "'"],
  invalid_weakmap_key:           ["Invalid value used as weak map key"],
  invalid_weakset_value:         ["Invalid value used in weak set"],
  not_date_object:               ["this is not a Date object."],
  observe_non_object:            ["Object.", "%0", " cannot ", "%0", " non-object"],
  observe_non_function:          ["Object.", "%0", " cannot deliver to non-function"],
  observe_callback_frozen:       ["Object.observe cannot deliver to a frozen function object"],
  observe_invalid_accept:        ["Object.observe accept must be an array of strings."],
  observe_type_non_string:       ["Invalid changeRecord with non-string 'type' property"],
  observe_perform_non_string:    ["Invalid non-string changeType"],
  observe_perform_non_function:  ["Cannot perform non-function"],
  observe_notify_non_notifier:   ["notify called on non-notifier object"],
  proto_poison_pill:             ["Generic use of __proto__ accessor not allowed"],
  not_typed_array:               ["this is not a typed array."],
  invalid_argument:              ["invalid_argument"],
  data_view_not_array_buffer:    ["First argument to DataView constructor must be an ArrayBuffer"],
  constructor_not_function:      ["Constructor ", "%0", " requires 'new'"],
  // RangeError
  invalid_array_length:          ["Invalid array length"],
  invalid_array_buffer_length:   ["Invalid array buffer length"],
  invalid_typed_array_offset:    ["Start offset is too large:"],
  invalid_typed_array_length:    ["Invalid typed array length"],
  invalid_typed_array_alignment: ["%0", "of", "%1", "should be a multiple of", "%3"],
  typed_array_set_source_too_large:
                                 ["Source is too large"],
  typed_array_set_negative_offset:
                                 ["Start offset is negative"],
  invalid_data_view_offset:      ["Start offset is outside the bounds of the buffer"],
  invalid_data_view_length:      ["Invalid data view length"],
  invalid_data_view_accessor_offset:
                                 ["Offset is outside the bounds of the DataView"],

  stack_overflow:                ["Maximum call stack size exceeded"],
  invalid_time_value:            ["Invalid time value"],
  invalid_count_value:           ["Invalid count value"],
  // SyntaxError
  paren_in_arg_string:           ["Function arg string contains parenthesis"],
  not_isvar:                     ["builtin %IS_VAR: not a variable"],
  single_function_literal:       ["Single function literal required"],
  invalid_regexp_flags:          ["Invalid flags supplied to RegExp constructor '", "%0", "'"],
  invalid_regexp:                ["Invalid RegExp pattern /", "%0", "/"],
  illegal_break:                 ["Illegal break statement"],
  illegal_continue:              ["Illegal continue statement"],
  illegal_return:                ["Illegal return statement"],
  illegal_let:                   ["Illegal let declaration outside extended mode"],
  error_loading_debugger:        ["Error loading debugger"],
  no_input_to_regexp:            ["No input to ", "%0"],
  invalid_json:                  ["String '", "%0", "' is not valid JSON"],
  circular_structure:            ["Converting circular structure to JSON"],
  called_on_non_object:          ["%0", " called on non-object"],
  called_on_null_or_undefined:   ["%0", " called on null or undefined"],
  array_indexof_not_defined:     ["Array.getIndexOf: Argument undefined"],
  object_not_extensible:         ["Can't add property ", "%0", ", object is not extensible"],
  illegal_access:                ["Illegal access"],
  invalid_preparser_data:        ["Invalid preparser data for function ", "%0"],
  strict_mode_with:              ["Strict mode code may not include a with statement"],
  strict_catch_variable:         ["Catch variable may not be eval or arguments in strict mode"],
  too_many_arguments:            ["Too many arguments in function call (only 32766 allowed)"],
  too_many_parameters:           ["Too many parameters in function definition (only 32766 allowed)"],
  too_many_variables:            ["Too many variables declared (only 131071 allowed)"],
  strict_param_name:             ["Parameter name eval or arguments is not allowed in strict mode"],
  strict_param_dupe:             ["Strict mode function may not have duplicate parameter names"],
  strict_var_name:               ["Variable name may not be eval or arguments in strict mode"],
  strict_function_name:          ["Function name may not be eval or arguments in strict mode"],
  strict_octal_literal:          ["Octal literals are not allowed in strict mode."],
  strict_duplicate_property:     ["Duplicate data property in object literal not allowed in strict mode"],
  accessor_data_property:        ["Object literal may not have data and accessor property with the same name"],
  accessor_get_set:              ["Object literal may not have multiple get/set accessors with the same name"],
  strict_lhs_assignment:         ["Assignment to eval or arguments is not allowed in strict mode"],
  strict_lhs_postfix:            ["Postfix increment/decrement may not have eval or arguments operand in strict mode"],
  strict_lhs_prefix:             ["Prefix increment/decrement may not have eval or arguments operand in strict mode"],
  strict_reserved_word:          ["Use of future reserved word in strict mode"],
  strict_delete:                 ["Delete of an unqualified identifier in strict mode."],
  strict_delete_property:        ["Cannot delete property '", "%0", "' of ", "%1"],
  strict_const:                  ["Use of const in strict mode."],
  strict_function:               ["In strict mode code, functions can only be declared at top level or immediately within another function." ],
  strict_read_only_property:     ["Cannot assign to read only property '", "%0", "' of ", "%1"],
  strict_cannot_assign:          ["Cannot assign to read only '", "%0", "' in strict mode"],
  strict_poison_pill:            ["'caller', 'callee', and 'arguments' properties may not be accessed on strict mode functions or the arguments objects for calls to them"],
  strict_caller:                 ["Illegal access to a strict mode caller function."],
  unprotected_let:               ["Illegal let declaration in unprotected statement context."],
  unprotected_const:             ["Illegal const declaration in unprotected statement context."],
  cant_prevent_ext_external_array_elements: ["Cannot prevent extension of an object with external array elements"],
  redef_external_array_element:  ["Cannot redefine a property of an object with external array elements"],
  harmony_const_assign:          ["Assignment to constant variable."],
  symbol_to_string:              ["Conversion from symbol to string"],
  invalid_module_path:           ["Module does not export '", "%0", "', or export is not itself a module"],
  module_type_error:             ["Module '", "%0", "' used improperly"],
  module_export_undefined:       ["Export '", "%0", "' is not defined in module"]
};


function FormatString(format, args) {
  var result = "";
  var arg_num = 0;
  for (var i = 0; i < format.length; i++) {
    var str = format[i];
    if (str.length == 2 && %_StringCharCodeAt(str, 0) == 0x25) {
      // Two-char string starts with "%".
      var arg_num = (%_StringCharCodeAt(str, 1) - 0x30) >>> 0;
      if (arg_num < 4) {
        // str is one of %0, %1, %2 or %3.
        try {
          str = NoSideEffectToString(args[arg_num]);
        } catch (e) {
          if (%IsJSModule(args[arg_num]))
            str = "module";
          else if (IS_SPEC_OBJECT(args[arg_num]))
            str = "object";
          else
            str = "#<error>";
        }
      }
    }
    result += str;
  }
  return result;
}


function NoSideEffectToString(obj) {
  if (IS_STRING(obj)) return obj;
  if (IS_NUMBER(obj)) return %_NumberToString(obj);
  if (IS_BOOLEAN(obj)) return x ? 'true' : 'false';
  if (IS_UNDEFINED(obj)) return 'undefined';
  if (IS_NULL(obj)) return 'null';
  if (IS_FUNCTION(obj)) return  %_CallFunction(obj, FunctionToString);
  if (IS_OBJECT(obj) && %GetDataProperty(obj, "toString") === ObjectToString) {
    var constructor = %GetDataProperty(obj, "constructor");
    if (typeof constructor == "function") {
      var constructorName = constructor.name;
      if (IS_STRING(constructorName) && constructorName !== "") {
        return "#<" + constructorName + ">";
      }
    }
  }
  if (CanBeSafelyTreatedAsAnErrorObject(obj)) {
    return %_CallFunction(obj, ErrorToString);
  }
  return %_CallFunction(obj, ObjectToString);
}

// To determine whether we can safely stringify an object using ErrorToString
// without the risk of side-effects, we need to check whether the object is
// either an instance of a native error type (via '%_ClassOf'), or has $Error
// in its prototype chain and hasn't overwritten 'toString' with something
// strange and unusual.
function CanBeSafelyTreatedAsAnErrorObject(obj) {
  switch (%_ClassOf(obj)) {
    case 'Error':
    case 'EvalError':
    case 'RangeError':
    case 'ReferenceError':
    case 'SyntaxError':
    case 'TypeError':
    case 'URIError':
      return true;
  }

  var objToString = %GetDataProperty(obj, "toString");
  return obj instanceof $Error && objToString === ErrorToString;
}


// When formatting internally created error messages, do not
// invoke overwritten error toString methods but explicitly use
// the error to string method. This is to avoid leaking error
// objects between script tags in a browser setting.
function ToStringCheckErrorObject(obj) {
  if (CanBeSafelyTreatedAsAnErrorObject(obj)) {
    return %_CallFunction(obj, ErrorToString);
  } else {
    return ToString(obj);
  }
}


function ToDetailString(obj) {
  if (obj != null && IS_OBJECT(obj) && obj.toString === ObjectToString) {
    var constructor = obj.constructor;
    if (typeof constructor == "function") {
      var constructorName = constructor.name;
      if (IS_STRING(constructorName) && constructorName !== "") {
        return "#<" + constructorName + ">";
      }
    }
  }
  return ToStringCheckErrorObject(obj);
}


function MakeGenericError(constructor, type, args) {
  if (IS_UNDEFINED(args)) args = [];
  return new constructor(FormatMessage(type, args));
}


/**
 * Set up the Script function and constructor.
 */
%FunctionSetInstanceClassName(Script, 'Script');
%SetProperty(Script.prototype, 'constructor', Script,
             DONT_ENUM | DONT_DELETE | READ_ONLY);
%SetCode(Script, function(x) {
  // Script objects can only be created by the VM.
  throw new $Error("Not supported");
});


// Helper functions; called from the runtime system.
function FormatMessage(type, args) {
  var format = kMessages[type];
  if (!format) return "<unknown message " + type + ">";
  return FormatString(format, args);
}


function GetLineNumber(message) {
  var start_position = %MessageGetStartPosition(message);
  if (start_position == -1) return kNoLineNumberInfo;
  var script = %MessageGetScript(message);
  var location = script.locationFromPosition(start_position, true);
  if (location == null) return kNoLineNumberInfo;
  return location.line + 1;
}


// Returns the source code line containing the given source
// position, or the empty string if the position is invalid.
function GetSourceLine(message) {
  var script = %MessageGetScript(message);
  var start_position = %MessageGetStartPosition(message);
  var location = script.locationFromPosition(start_position, true);
  if (location == null) return "";
  location.restrict();
  return location.sourceText();
}


function MakeTypeError(type, args) {
  return MakeGenericError($TypeError, type, args);
}


function MakeRangeError(type, args) {
  return MakeGenericError($RangeError, type, args);
}


function MakeSyntaxError(type, args) {
  return MakeGenericError($SyntaxError, type, args);
}


function MakeReferenceError(type, args) {
  return MakeGenericError($ReferenceError, type, args);
}


function MakeEvalError(type, args) {
  return MakeGenericError($EvalError, type, args);
}


function MakeError(type, args) {
  return MakeGenericError($Error, type, args);
}

/**
 * Find a line number given a specific source position.
 * @param {number} position The source position.
 * @return {number} 0 if input too small, -1 if input too large,
       else the line number.
 */
function ScriptLineFromPosition(position) {
  var lower = 0;
  var upper = this.lineCount() - 1;
  var line_ends = this.line_ends;

  // We'll never find invalid positions so bail right away.
  if (position > line_ends[upper]) {
    return -1;
  }

  // This means we don't have to safe-guard indexing line_ends[i - 1].
  if (position <= line_ends[0]) {
    return 0;
  }

  // Binary search to find line # from position range.
  while (upper >= 1) {
    var i = (lower + upper) >> 1;

    if (position > line_ends[i]) {
      lower = i + 1;
    } else if (position <= line_ends[i - 1]) {
      upper = i - 1;
    } else {
      return i;
    }
  }

  return -1;
}

/**
 * Get information on a specific source position.
 * @param {number} position The source position
 * @param {boolean} include_resource_offset Set to true to have the resource
 *     offset added to the location
 * @return {SourceLocation}
 *     If line is negative or not in the source null is returned.
 */
function ScriptLocationFromPosition(position,
                                    include_resource_offset) {
  var line = this.lineFromPosition(position);
  if (line == -1) return null;

  // Determine start, end and column.
  var line_ends = this.line_ends;
  var start = line == 0 ? 0 : line_ends[line - 1] + 1;
  var end = line_ends[line];
  if (end > 0 && %_CallFunction(this.source, end - 1, StringCharAt) == '\r') {
    end--;
  }
  var column = position - start;

  // Adjust according to the offset within the resource.
  if (include_resource_offset) {
    line += this.line_offset;
    if (line == this.line_offset) {
      column += this.column_offset;
    }
  }

  return new SourceLocation(this, position, line, column, start, end);
}


/**
 * Get information on a specific source line and column possibly offset by a
 * fixed source position. This function is used to find a source position from
 * a line and column position. The fixed source position offset is typically
 * used to find a source position in a function based on a line and column in
 * the source for the function alone. The offset passed will then be the
 * start position of the source for the function within the full script source.
 * @param {number} opt_line The line within the source. Default value is 0
 * @param {number} opt_column The column in within the line. Default value is 0
 * @param {number} opt_offset_position The offset from the begining of the
 *     source from where the line and column calculation starts.
 *     Default value is 0
 * @return {SourceLocation}
 *     If line is negative or not in the source null is returned.
 */
function ScriptLocationFromLine(opt_line, opt_column, opt_offset_position) {
  // Default is the first line in the script. Lines in the script is relative
  // to the offset within the resource.
  var line = 0;
  if (!IS_UNDEFINED(opt_line)) {
    line = opt_line - this.line_offset;
  }

  // Default is first column. If on the first line add the offset within the
  // resource.
  var column = opt_column || 0;
  if (line == 0) {
    column -= this.column_offset;
  }

  var offset_position = opt_offset_position || 0;
  if (line < 0 || column < 0 || offset_position < 0) return null;
  if (line == 0) {
    return this.locationFromPosition(offset_position + column, false);
  } else {
    // Find the line where the offset position is located.
    var offset_line = this.lineFromPosition(offset_position);

    if (offset_line == -1 || offset_line + line >= this.lineCount()) {
      return null;
    }

    return this.locationFromPosition(
        this.line_ends[offset_line + line - 1] + 1 + column);  // line > 0 here.
  }
}


/**
 * Get a slice of source code from the script. The boundaries for the slice is
 * specified in lines.
 * @param {number} opt_from_line The first line (zero bound) in the slice.
 *     Default is 0
 * @param {number} opt_to_column The last line (zero bound) in the slice (non
 *     inclusive). Default is the number of lines in the script
 * @return {SourceSlice} The source slice or null of the parameters where
 *     invalid
 */
function ScriptSourceSlice(opt_from_line, opt_to_line) {
  var from_line = IS_UNDEFINED(opt_from_line) ? this.line_offset
                                              : opt_from_line;
  var to_line = IS_UNDEFINED(opt_to_line) ? this.line_offset + this.lineCount()
                                          : opt_to_line;

  // Adjust according to the offset within the resource.
  from_line -= this.line_offset;
  to_line -= this.line_offset;
  if (from_line < 0) from_line = 0;
  if (to_line > this.lineCount()) to_line = this.lineCount();

  // Check parameters.
  if (from_line >= this.lineCount() ||
      to_line < 0 ||
      from_line > to_line) {
    return null;
  }

  var line_ends = this.line_ends;
  var from_position = from_line == 0 ? 0 : line_ends[from_line - 1] + 1;
  var to_position = to_line == 0 ? 0 : line_ends[to_line - 1] + 1;

  // Return a source slice with line numbers re-adjusted to the resource.
  return new SourceSlice(this,
                         from_line + this.line_offset,
                         to_line + this.line_offset,
                          from_position, to_position);
}


function ScriptSourceLine(opt_line) {
  // Default is the first line in the script. Lines in the script are relative
  // to the offset within the resource.
  var line = 0;
  if (!IS_UNDEFINED(opt_line)) {
    line = opt_line - this.line_offset;
  }

  // Check parameter.
  if (line < 0 || this.lineCount() <= line) {
    return null;
  }

  // Return the source line.
  var line_ends = this.line_ends;
  var start = line == 0 ? 0 : line_ends[line - 1] + 1;
  var end = line_ends[line];
  return %_CallFunction(this.source, start, end, StringSubstring);
}


/**
 * Returns the number of source lines.
 * @return {number}
 *     Number of source lines.
 */
function ScriptLineCount() {
  // Return number of source lines.
  return this.line_ends.length;
}


/**
 * If sourceURL comment is available and script starts at zero returns sourceURL
 * comment contents. Otherwise, script name is returned. See
 * http://fbug.googlecode.com/svn/branches/firebug1.1/docs/ReleaseNotes_1.1.txt
 * and Source Map Revision 3 proposal for details on using //# sourceURL and
 * deprecated //@ sourceURL comment to identify scripts that don't have name.
 *
 * @return {?string} script name if present, value for //# sourceURL or
 * deprecated //@ sourceURL comment otherwise.
 */
function ScriptNameOrSourceURL() {
  if (this.line_offset > 0 || this.column_offset > 0) {
    return this.name;
  }

  // The result is cached as on long scripts it takes noticable time to search
  // for the sourceURL.
  if (this.hasCachedNameOrSourceURL) {
    return this.cachedNameOrSourceURL;
  }
  this.hasCachedNameOrSourceURL = true;

  // TODO(608): the spaces in a regexp below had to be escaped as \040
  // because this file is being processed by js2c whose handling of spaces
  // in regexps is broken. Also, ['"] are excluded from allowed URLs to
  // avoid matches against sources that invoke evals with sourceURL.
  // A better solution would be to detect these special comments in
  // the scanner/parser.
  var source = ToString(this.source);
  var sourceUrlPos = %StringIndexOf(source, "sourceURL=", 0);
  this.cachedNameOrSourceURL = this.name;
  if (sourceUrlPos > 4) {
    var sourceUrlPattern =
        /\/\/[#@][\040\t]sourceURL=[\040\t]*([^\s\'\"]*)[\040\t]*$/gm;
    // Don't reuse lastMatchInfo here, so we create a new array with room
    // for four captures (array with length one longer than the index
    // of the fourth capture, where the numbering is zero-based).
    var matchInfo = new InternalArray(CAPTURE(3) + 1);
    var match =
        %_RegExpExec(sourceUrlPattern, source, sourceUrlPos - 4, matchInfo);
    if (match) {
      this.cachedNameOrSourceURL =
          %_SubString(source, matchInfo[CAPTURE(2)], matchInfo[CAPTURE(3)]);
    }
  }
  return this.cachedNameOrSourceURL;
}


SetUpLockedPrototype(Script,
  $Array("source", "name", "line_ends", "line_offset", "column_offset",
         "cachedNameOrSourceURL", "hasCachedNameOrSourceURL" ),
  $Array(
    "lineFromPosition", ScriptLineFromPosition,
    "locationFromPosition", ScriptLocationFromPosition,
    "locationFromLine", ScriptLocationFromLine,
    "sourceSlice", ScriptSourceSlice,
    "sourceLine", ScriptSourceLine,
    "lineCount", ScriptLineCount,
    "nameOrSourceURL", ScriptNameOrSourceURL
  )
);


/**
 * Class for source location. A source location is a position within some
 * source with the following properties:
 *   script   : script object for the source
 *   line     : source line number
 *   column   : source column within the line
 *   position : position within the source
 *   start    : position of start of source context (inclusive)
 *   end      : position of end of source context (not inclusive)
 * Source text for the source context is the character interval
 * [start, end[. In most cases end will point to a newline character.
 * It might point just past the final position of the source if the last
 * source line does not end with a newline character.
 * @param {Script} script The Script object for which this is a location
 * @param {number} position Source position for the location
 * @param {number} line The line number for the location
 * @param {number} column The column within the line for the location
 * @param {number} start Source position for start of source context
 * @param {number} end Source position for end of source context
 * @constructor
 */
function SourceLocation(script, position, line, column, start, end) {
  this.script = script;
  this.position = position;
  this.line = line;
  this.column = column;
  this.start = start;
  this.end = end;
}

var kLineLengthLimit = 78;

/**
 * Restrict source location start and end positions to make the source slice
 * no more that a certain number of characters wide.
 * @param {number} opt_limit The with limit of the source text with a default
 *     of 78
 * @param {number} opt_before The number of characters to prefer before the
 *     position with a default value of 10 less that the limit
 */
function SourceLocationRestrict(opt_limit, opt_before) {
  // Find the actual limit to use.
  var limit;
  var before;
  if (!IS_UNDEFINED(opt_limit)) {
    limit = opt_limit;
  } else {
    limit = kLineLengthLimit;
  }
  if (!IS_UNDEFINED(opt_before)) {
    before = opt_before;
  } else {
    // If no before is specified center for small limits and perfer more source
    // before the the position that after for longer limits.
    if (limit <= 20) {
      before = $floor(limit / 2);
    } else {
      before = limit - 10;
    }
  }
  if (before >= limit) {
    before = limit - 1;
  }

  // If the [start, end[ interval is too big we restrict
  // it in one or both ends. We make sure to always produce
  // restricted intervals of maximum allowed size.
  if (this.end - this.start > limit) {
    var start_limit = this.position - before;
    var end_limit = this.position + limit - before;
    if (this.start < start_limit && end_limit < this.end) {
      this.start = start_limit;
      this.end = end_limit;
    } else if (this.start < start_limit) {
      this.start = this.end - limit;
    } else {
      this.end = this.start + limit;
    }
  }
}


/**
 * Get the source text for a SourceLocation
 * @return {String}
 *     Source text for this location.
 */
function SourceLocationSourceText() {
  return %_CallFunction(this.script.source,
                        this.start,
                        this.end,
                        StringSubstring);
}


SetUpLockedPrototype(SourceLocation,
  $Array("script", "position", "line", "column", "start", "end"),
  $Array(
    "restrict", SourceLocationRestrict,
    "sourceText", SourceLocationSourceText
 )
);


/**
 * Class for a source slice. A source slice is a part of a script source with
 * the following properties:
 *   script        : script object for the source
 *   from_line     : line number for the first line in the slice
 *   to_line       : source line number for the last line in the slice
 *   from_position : position of the first character in the slice
 *   to_position   : position of the last character in the slice
 * The to_line and to_position are not included in the slice, that is the lines
 * in the slice are [from_line, to_line[. Likewise the characters in the slice
 * are [from_position, to_position[.
 * @param {Script} script The Script object for the source slice
 * @param {number} from_line
 * @param {number} to_line
 * @param {number} from_position
 * @param {number} to_position
 * @constructor
 */
function SourceSlice(script, from_line, to_line, from_position, to_position) {
  this.script = script;
  this.from_line = from_line;
  this.to_line = to_line;
  this.from_position = from_position;
  this.to_position = to_position;
}

/**
 * Get the source text for a SourceSlice
 * @return {String} Source text for this slice. The last line will include
 *     the line terminating characters (if any)
 */
function SourceSliceSourceText() {
  return %_CallFunction(this.script.source,
                        this.from_position,
                        this.to_position,
                        StringSubstring);
}

SetUpLockedPrototype(SourceSlice,
  $Array("script", "from_line", "to_line", "from_position", "to_position"),
  $Array("sourceText", SourceSliceSourceText)
);


// Returns the offset of the given position within the containing
// line.
function GetPositionInLine(message) {
  var script = %MessageGetScript(message);
  var start_position = %MessageGetStartPosition(message);
  var location = script.locationFromPosition(start_position, false);
  if (location == null) return -1;
  location.restrict();
  return start_position - location.start;
}


function GetStackTraceLine(recv, fun, pos, isGlobal) {
  return new CallSite(recv, fun, pos, false).toString();
}

// ----------------------------------------------------------------------------
// Error implementation

var CallSiteReceiverKey = %CreateSymbol("receiver");
var CallSiteFunctionKey = %CreateSymbol("function");
var CallSitePositionKey = %CreateSymbol("position");
var CallSiteStrictModeKey = %CreateSymbol("strict mode");

function CallSite(receiver, fun, pos, strict_mode) {
  this[CallSiteReceiverKey] = receiver;
  this[CallSiteFunctionKey] = fun;
  this[CallSitePositionKey] = pos;
  this[CallSiteStrictModeKey] = strict_mode;
}

function CallSiteGetThis() {
  return this[CallSiteStrictModeKey] ? UNDEFINED : this[CallSiteReceiverKey];
}

function CallSiteGetTypeName() {
  return GetTypeName(this[CallSiteReceiverKey], false);
}

function CallSiteIsToplevel() {
  if (this[CallSiteReceiverKey] == null) {
    return true;
  }
  return IS_GLOBAL(this[CallSiteReceiverKey]);
}

function CallSiteIsEval() {
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  return script && script.compilation_type == COMPILATION_TYPE_EVAL;
}

function CallSiteGetEvalOrigin() {
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  return FormatEvalOrigin(script);
}

function CallSiteGetScriptNameOrSourceURL() {
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  return script ? script.nameOrSourceURL() : null;
}

function CallSiteGetFunction() {
  return this[CallSiteStrictModeKey] ? UNDEFINED : this[CallSiteFunctionKey];
}

function CallSiteGetFunctionName() {
  // See if the function knows its own name
  var name = this[CallSiteFunctionKey].name;
  if (name) {
    return name;
  }
  name = %FunctionGetInferredName(this[CallSiteFunctionKey]);
  if (name) {
    return name;
  }
  // Maybe this is an evaluation?
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  if (script && script.compilation_type == COMPILATION_TYPE_EVAL) {
    return "eval";
  }
  return null;
}

function CallSiteGetMethodName() {
  // See if we can find a unique property on the receiver that holds
  // this function.
  var receiver = this[CallSiteReceiverKey];
  var fun = this[CallSiteFunctionKey];
  var ownName = fun.name;
  if (ownName && receiver &&
      (%_CallFunction(receiver, ownName, ObjectLookupGetter) === fun ||
       %_CallFunction(receiver, ownName, ObjectLookupSetter) === fun ||
       (IS_OBJECT(receiver) && %GetDataProperty(receiver, ownName) === fun))) {
    // To handle DontEnum properties we guess that the method has
    // the same name as the function.
    return ownName;
  }
  var name = null;
  for (var prop in receiver) {
    if (%_CallFunction(receiver, prop, ObjectLookupGetter) === fun ||
        %_CallFunction(receiver, prop, ObjectLookupSetter) === fun ||
        (IS_OBJECT(receiver) && %GetDataProperty(receiver, prop) === fun)) {
      // If we find more than one match bail out to avoid confusion.
      if (name) {
        return null;
      }
      name = prop;
    }
  }
  if (name) {
    return name;
  }
  return null;
}

function CallSiteGetFileName() {
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  return script ? script.name : null;
}

function CallSiteGetLineNumber() {
  if (this[CallSitePositionKey] == -1) {
    return null;
  }
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  var location = null;
  if (script) {
    location = script.locationFromPosition(this[CallSitePositionKey], true);
  }
  return location ? location.line + 1 : null;
}

function CallSiteGetColumnNumber() {
  if (this[CallSitePositionKey] == -1) {
    return null;
  }
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  var location = null;
  if (script) {
    location = script.locationFromPosition(this[CallSitePositionKey], true);
  }
  return location ? location.column + 1: null;
}

function CallSiteIsNative() {
  var script = %FunctionGetScript(this[CallSiteFunctionKey]);
  return script ? (script.type == TYPE_NATIVE) : false;
}

function CallSiteGetPosition() {
  return this[CallSitePositionKey];
}

function CallSiteIsConstructor() {
  var receiver = this[CallSiteReceiverKey];
  var constructor = (receiver != null && IS_OBJECT(receiver))
                        ? %GetDataProperty(receiver, "constructor") : null;
  if (!constructor) return false;
  return this[CallSiteFunctionKey] === constructor;
}

function CallSiteToString() {
  var fileName;
  var fileLocation = "";
  if (this.isNative()) {
    fileLocation = "native";
  } else {
    if (this.isEval()) {
      fileName = this.getScriptNameOrSourceURL();
      if (!fileName) {
        fileLocation = this.getEvalOrigin();
        fileLocation += ", ";  // Expecting source position to follow.
      }
    } else {
      fileName = this.getFileName();
    }

    if (fileName) {
      fileLocation += fileName;
    } else {
      // Source code does not originate from a file and is not native, but we
      // can still get the source position inside the source string, e.g. in
      // an eval string.
      fileLocation += "<anonymous>";
    }
    var lineNumber = this.getLineNumber();
    if (lineNumber != null) {
      fileLocation += ":" + lineNumber;
      var columnNumber = this.getColumnNumber();
      if (columnNumber) {
        fileLocation += ":" + columnNumber;
      }
    }
  }

  var line = "";
  var functionName = this.getFunctionName();
  var addSuffix = true;
  var isConstructor = this.isConstructor();
  var isMethodCall = !(this.isToplevel() || isConstructor);
  if (isMethodCall) {
    var typeName = GetTypeName(this[CallSiteReceiverKey], true);
    var methodName = this.getMethodName();
    if (functionName) {
      if (typeName &&
          %_CallFunction(functionName, typeName, StringIndexOf) != 0) {
        line += typeName + ".";
      }
      line += functionName;
      if (methodName &&
          (%_CallFunction(functionName, "." + methodName, StringIndexOf) !=
           functionName.length - methodName.length - 1)) {
        line += " [as " + methodName + "]";
      }
    } else {
      line += typeName + "." + (methodName || "<anonymous>");
    }
  } else if (isConstructor) {
    line += "new " + (functionName || "<anonymous>");
  } else if (functionName) {
    line += functionName;
  } else {
    line += fileLocation;
    addSuffix = false;
  }
  if (addSuffix) {
    line += " (" + fileLocation + ")";
  }
  return line;
}

SetUpLockedPrototype(CallSite, $Array("receiver", "fun", "pos"), $Array(
  "getThis", CallSiteGetThis,
  "getTypeName", CallSiteGetTypeName,
  "isToplevel", CallSiteIsToplevel,
  "isEval", CallSiteIsEval,
  "getEvalOrigin", CallSiteGetEvalOrigin,
  "getScriptNameOrSourceURL", CallSiteGetScriptNameOrSourceURL,
  "getFunction", CallSiteGetFunction,
  "getFunctionName", CallSiteGetFunctionName,
  "getMethodName", CallSiteGetMethodName,
  "getFileName", CallSiteGetFileName,
  "getLineNumber", CallSiteGetLineNumber,
  "getColumnNumber", CallSiteGetColumnNumber,
  "isNative", CallSiteIsNative,
  "getPosition", CallSiteGetPosition,
  "isConstructor", CallSiteIsConstructor,
  "toString", CallSiteToString
));


function FormatEvalOrigin(script) {
  var sourceURL = script.nameOrSourceURL();
  if (sourceURL) {
    return sourceURL;
  }

  var eval_origin = "eval at ";
  if (script.eval_from_function_name) {
    eval_origin += script.eval_from_function_name;
  } else {
    eval_origin +=  "<anonymous>";
  }

  var eval_from_script = script.eval_from_script;
  if (eval_from_script) {
    if (eval_from_script.compilation_type == COMPILATION_TYPE_EVAL) {
      // eval script originated from another eval.
      eval_origin += " (" + FormatEvalOrigin(eval_from_script) + ")";
    } else {
      // eval script originated from "real" source.
      if (eval_from_script.name) {
        eval_origin += " (" + eval_from_script.name;
        var location = eval_from_script.locationFromPosition(
            script.eval_from_script_position, true);
        if (location) {
          eval_origin += ":" + (location.line + 1);
          eval_origin += ":" + (location.column + 1);
        }
        eval_origin += ")";
      } else {
        eval_origin += " (unknown source)";
      }
    }
  }

  return eval_origin;
}


function FormatErrorString(error) {
  try {
    return %_CallFunction(error, ErrorToString);
  } catch (e) {
    try {
      return "<error: " + e + ">";
    } catch (ee) {
      return "<error>";
    }
  }
}


function GetStackFrames(raw_stack) {
  var frames = new InternalArray();
  var non_strict_frames = raw_stack[0];
  for (var i = 1; i < raw_stack.length; i += 4) {
    var recv = raw_stack[i];
    var fun = raw_stack[i + 1];
    var code = raw_stack[i + 2];
    var pc = raw_stack[i + 3];
    var pos = %FunctionGetPositionForOffset(code, pc);
    non_strict_frames--;
    frames.push(new CallSite(recv, fun, pos, (non_strict_frames < 0)));
  }
  return frames;
}


// Flag to prevent recursive call of Error.prepareStackTrace.
var formatting_custom_stack_trace = false;


function FormatStackTrace(obj, error_string, frames) {
  if (IS_FUNCTION($Error.prepareStackTrace) && !formatting_custom_stack_trace) {
    var array = [];
    %MoveArrayContents(frames, array);
    formatting_custom_stack_trace = true;
    var stack_trace = UNDEFINED;
    try {
      stack_trace = $Error.prepareStackTrace(obj, array);
    } catch (e) {
      throw e;  // The custom formatting function threw.  Rethrow.
    } finally {
      formatting_custom_stack_trace = false;
    }
    return stack_trace;
  }

  var lines = new InternalArray();
  lines.push(error_string);
  for (var i = 0; i < frames.length; i++) {
    var frame = frames[i];
    var line;
    try {
      line = frame.toString();
    } catch (e) {
      try {
        line = "<error: " + e + ">";
      } catch (ee) {
        // Any code that reaches this point is seriously nasty!
        line = "<error>";
      }
    }
    lines.push("    at " + line);
  }
  return %_CallFunction(lines, "\n", ArrayJoin);
}


function GetTypeName(receiver, requireConstructor) {
  var constructor = receiver.constructor;
  if (!constructor) {
    return requireConstructor ? null :
        %_CallFunction(receiver, ObjectToString);
  }
  var constructorName = constructor.name;
  if (!constructorName) {
    return requireConstructor ? null :
        %_CallFunction(receiver, ObjectToString);
  }
  return constructorName;
}


function captureStackTrace(obj, cons_opt) {
  var stackTraceLimit = $Error.stackTraceLimit;
  if (!stackTraceLimit || !IS_NUMBER(stackTraceLimit)) return;
  if (stackTraceLimit < 0 || stackTraceLimit > 10000) {
    stackTraceLimit = 10000;
  }
  var stack = %CollectStackTrace(obj,
                                 cons_opt ? cons_opt : captureStackTrace,
                                 stackTraceLimit);

  var error_string = FormatErrorString(obj);
  // The holder of this getter ('obj') may not be the receiver ('this').
  // When this getter is called the first time, we use the context values to
  // format a stack trace string and turn this accessor pair into a data
  // property (on the holder).
  var getter = function() {
    // Stack is still a raw array awaiting to be formatted.
    var result = FormatStackTrace(obj, error_string, GetStackFrames(stack));
    // Turn this accessor into a data property.
    %DefineOrRedefineDataProperty(obj, 'stack', result, NONE);
    // Release context values.
    stack = error_string = UNDEFINED;
    return result;
  };

  // Set the 'stack' property on the receiver.  If the receiver is the same as
  // holder of this setter, the accessor pair is turned into a data property.
  var setter = function(v) {
    // Set data property on the receiver (not necessarily holder).
    %DefineOrRedefineDataProperty(this, 'stack', v, NONE);
    if (this === obj) {
      // Release context values if holder is the same as the receiver.
      stack = error_string = UNDEFINED;
    }
  };

  %DefineOrRedefineAccessorProperty(obj, 'stack', getter, setter, DONT_ENUM);
}


function SetUpError() {
  // Define special error type constructors.

  var DefineError = function(f) {
    // Store the error function in both the global object
    // and the runtime object. The function is fetched
    // from the runtime object when throwing errors from
    // within the runtime system to avoid strange side
    // effects when overwriting the error functions from
    // user code.
    var name = f.name;
    %SetProperty(global, name, f, DONT_ENUM);
    %SetProperty(builtins, '$' + name, f, DONT_ENUM | DONT_DELETE | READ_ONLY);
    // Configure the error function.
    if (name == 'Error') {
      // The prototype of the Error object must itself be an error.
      // However, it can't be an instance of the Error object because
      // it hasn't been properly configured yet.  Instead we create a
      // special not-a-true-error-but-close-enough object.
      var ErrorPrototype = function() {};
      %FunctionSetPrototype(ErrorPrototype, $Object.prototype);
      %FunctionSetInstanceClassName(ErrorPrototype, 'Error');
      %FunctionSetPrototype(f, new ErrorPrototype());
    } else {
      %FunctionSetPrototype(f, new $Error());
    }
    %FunctionSetInstanceClassName(f, 'Error');
    %SetProperty(f.prototype, 'constructor', f, DONT_ENUM);
    %SetProperty(f.prototype, "name", name, DONT_ENUM);
    %SetCode(f, function(m) {
      if (%_IsConstructCall()) {
        // Define all the expected properties directly on the error
        // object. This avoids going through getters and setters defined
        // on prototype objects.
        %IgnoreAttributesAndSetProperty(this, 'stack', UNDEFINED, DONT_ENUM);
        if (!IS_UNDEFINED(m)) {
          %IgnoreAttributesAndSetProperty(
            this, 'message', ToString(m), DONT_ENUM);
        }
        captureStackTrace(this, f);
      } else {
        return new f(m);
      }
    });
    %SetNativeFlag(f);
  };

  DefineError(function Error() { });
  DefineError(function TypeError() { });
  DefineError(function RangeError() { });
  DefineError(function SyntaxError() { });
  DefineError(function ReferenceError() { });
  DefineError(function EvalError() { });
  DefineError(function URIError() { });
}

SetUpError();

$Error.captureStackTrace = captureStackTrace;

%SetProperty($Error.prototype, 'message', '', DONT_ENUM);

// Global list of error objects visited during ErrorToString. This is
// used to detect cycles in error toString formatting.
var visited_errors = new InternalArray();
var cyclic_error_marker = new $Object();

function GetPropertyWithoutInvokingMonkeyGetters(error, name) {
  // Climb the prototype chain until we find the holder.
  while (error && !%HasLocalProperty(error, name)) {
    error = %GetPrototype(error);
  }
  if (IS_NULL(error)) return UNDEFINED;
  if (!IS_OBJECT(error)) return error[name];
  // If the property is an accessor on one of the predefined errors that can be
  // generated statically by the compiler, don't touch it. This is to address
  // http://code.google.com/p/chromium/issues/detail?id=69187
  var desc = %GetOwnProperty(error, name);
  if (desc && desc[IS_ACCESSOR_INDEX]) {
    var isName = name === "name";
    if (error === $ReferenceError.prototype)
      return isName ? "ReferenceError" : UNDEFINED;
    if (error === $SyntaxError.prototype)
      return isName ? "SyntaxError" : UNDEFINED;
    if (error === $TypeError.prototype)
      return isName ? "TypeError" : UNDEFINED;
  }
  // Otherwise, read normally.
  return error[name];
}

function ErrorToStringDetectCycle(error) {
  if (!%PushIfAbsent(visited_errors, error)) throw cyclic_error_marker;
  try {
    var name = GetPropertyWithoutInvokingMonkeyGetters(error, "name");
    name = IS_UNDEFINED(name) ? "Error" : TO_STRING_INLINE(name);
    var message = GetPropertyWithoutInvokingMonkeyGetters(error, "message");
    message = IS_UNDEFINED(message) ? "" : TO_STRING_INLINE(message);
    if (name === "") return message;
    if (message === "") return name;
    return name + ": " + message;
  } finally {
    visited_errors.length = visited_errors.length - 1;
  }
}

function ErrorToString() {
  if (!IS_SPEC_OBJECT(this)) {
    throw MakeTypeError("called_on_non_object", ["Error.prototype.toString"]);
  }

  try {
    return ErrorToStringDetectCycle(this);
  } catch(e) {
    // If this error message was encountered already return the empty
    // string for it instead of recursively formatting it.
    if (e === cyclic_error_marker) {
      return '';
    }
    throw e;
  }
}


InstallFunctions($Error.prototype, DONT_ENUM, ['toString', ErrorToString]);

// Boilerplate for exceptions for stack overflows. Used from
// Isolate::StackOverflow().
function SetUpStackOverflowBoilerplate() {
  var boilerplate = MakeRangeError('stack_overflow', []);

  var error_string = boilerplate.name + ": " + boilerplate.message;

  // The raw stack trace is stored as a hidden property on the holder of this
  // getter, which may not be the same as the receiver.  Find the holder to
  // retrieve the raw stack trace and then turn this accessor pair into a
  // data property.
  var getter = function() {
    var holder = this;
    while (!IS_ERROR(holder)) {
      holder = %GetPrototype(holder);
      if (IS_NULL(holder)) return MakeSyntaxError('illegal_access', []);
    }
    var stack = %GetAndClearOverflowedStackTrace(holder);
    // We may not have captured any stack trace.
    if (IS_UNDEFINED(stack)) return stack;

    var result = FormatStackTrace(holder, error_string, GetStackFrames(stack));
    // Replace this accessor with a data property.
    %DefineOrRedefineDataProperty(holder, 'stack', result, NONE);
    return result;
  };

  // Set the 'stack' property on the receiver.  If the receiver is the same as
  // holder of this setter, the accessor pair is turned into a data property.
  var setter = function(v) {
    %DefineOrRedefineDataProperty(this, 'stack', v, NONE);
    // Tentatively clear the hidden property. If the receiver is the same as
    // holder, we release the raw stack trace this way.
    %GetAndClearOverflowedStackTrace(this);
  };

  %DefineOrRedefineAccessorProperty(
      boilerplate, 'stack', getter, setter, DONT_ENUM);

  return boilerplate;
}

var kStackOverflowBoilerplate = SetUpStackOverflowBoilerplate();
