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

// Default number of frames to include in the response to backtrace request.
var kDefaultBacktraceLength = 10;

var Debug = {};

// Regular expression to skip "crud" at the beginning of a source line which is
// not really code. Currently the regular expression matches whitespace and
// comments.
var sourceLineBeginningSkip = /^(?:\s*(?:\/\*.*?\*\/)*)*/;

// Debug events which can occour in the V8 JavaScript engine. These originate
// from the API include file debug.h.
Debug.DebugEvent = { Break: 1,
                     Exception: 2,
                     NewFunction: 3,
                     BeforeCompile: 4,
                     AfterCompile: 5,
                     ScriptCollected: 6 };

// Types of exceptions that can be broken upon.
Debug.ExceptionBreak = { Caught : 0,
                         Uncaught: 1 };

// The different types of steps.
Debug.StepAction = { StepOut: 0,
                     StepNext: 1,
                     StepIn: 2,
                     StepMin: 3,
                     StepInMin: 4 };

// The different types of scripts matching enum ScriptType in objects.h.
Debug.ScriptType = { Native: 0,
                     Extension: 1,
                     Normal: 2 };

// The different types of script compilations matching enum
// Script::CompilationType in objects.h.
Debug.ScriptCompilationType = { Host: 0,
                                Eval: 1,
                                JSON: 2 };

// The different script break point types.
Debug.ScriptBreakPointType = { ScriptId: 0,
                               ScriptName: 1,
                               ScriptRegExp: 2 };

// The different types of breakpoint position alignments.
// Must match BreakPositionAlignment in debug.h.
Debug.BreakPositionAlignment = {
  Statement: 0,
  BreakPosition: 1
};

function ScriptTypeFlag(type) {
  return (1 << type);
}

// Globals.
var next_response_seq = 0;
var next_break_point_number = 1;
var break_points = [];
var script_break_points = [];
var debugger_flags = {
  breakPointsActive: {
    value: true,
    getValue: function() { return this.value; },
    setValue: function(value) {
      this.value = !!value;
      %SetDisableBreak(!this.value);
    }
  },
  breakOnCaughtException: {
    getValue: function() { return Debug.isBreakOnException(); },
    setValue: function(value) {
      if (value) {
        Debug.setBreakOnException();
      } else {
        Debug.clearBreakOnException();
      }
    }
  },
  breakOnUncaughtException: {
    getValue: function() { return Debug.isBreakOnUncaughtException(); },
    setValue: function(value) {
      if (value) {
        Debug.setBreakOnUncaughtException();
      } else {
        Debug.clearBreakOnUncaughtException();
      }
    }
  },
};


// Create a new break point object and add it to the list of break points.
function MakeBreakPoint(source_position, opt_script_break_point) {
  var break_point = new BreakPoint(source_position, opt_script_break_point);
  break_points.push(break_point);
  return break_point;
}


// Object representing a break point.
// NOTE: This object does not have a reference to the function having break
// point as this would cause function not to be garbage collected when it is
// not used any more. We do not want break points to keep functions alive.
function BreakPoint(source_position, opt_script_break_point) {
  this.source_position_ = source_position;
  if (opt_script_break_point) {
    this.script_break_point_ = opt_script_break_point;
  } else {
    this.number_ = next_break_point_number++;
  }
  this.hit_count_ = 0;
  this.active_ = true;
  this.condition_ = null;
  this.ignoreCount_ = 0;
}


BreakPoint.prototype.number = function() {
  return this.number_;
};


BreakPoint.prototype.func = function() {
  return this.func_;
};


BreakPoint.prototype.source_position = function() {
  return this.source_position_;
};


BreakPoint.prototype.hit_count = function() {
  return this.hit_count_;
};


BreakPoint.prototype.active = function() {
  if (this.script_break_point()) {
    return this.script_break_point().active();
  }
  return this.active_;
};


BreakPoint.prototype.condition = function() {
  if (this.script_break_point() && this.script_break_point().condition()) {
    return this.script_break_point().condition();
  }
  return this.condition_;
};


BreakPoint.prototype.ignoreCount = function() {
  return this.ignoreCount_;
};


BreakPoint.prototype.script_break_point = function() {
  return this.script_break_point_;
};


BreakPoint.prototype.enable = function() {
  this.active_ = true;
};


BreakPoint.prototype.disable = function() {
  this.active_ = false;
};


BreakPoint.prototype.setCondition = function(condition) {
  this.condition_ = condition;
};


BreakPoint.prototype.setIgnoreCount = function(ignoreCount) {
  this.ignoreCount_ = ignoreCount;
};


BreakPoint.prototype.isTriggered = function(exec_state) {
  // Break point not active - not triggered.
  if (!this.active()) return false;

  // Check for conditional break point.
  if (this.condition()) {
    // If break point has condition try to evaluate it in the top frame.
    try {
      var mirror = exec_state.frame(0).evaluate(this.condition());
      // If no sensible mirror or non true value break point not triggered.
      if (!(mirror instanceof ValueMirror) || !%ToBoolean(mirror.value_)) {
        return false;
      }
    } catch (e) {
      // Exception evaluating condition counts as not triggered.
      return false;
    }
  }

  // Update the hit count.
  this.hit_count_++;
  if (this.script_break_point_) {
    this.script_break_point_.hit_count_++;
  }

  // If the break point has an ignore count it is not triggered.
  if (this.ignoreCount_ > 0) {
    this.ignoreCount_--;
    return false;
  }

  // Break point triggered.
  return true;
};


// Function called from the runtime when a break point is hit. Returns true if
// the break point is triggered and supposed to break execution.
function IsBreakPointTriggered(break_id, break_point) {
  return break_point.isTriggered(MakeExecutionState(break_id));
}


// Object representing a script break point. The script is referenced by its
// script name or script id and the break point is represented as line and
// column.
function ScriptBreakPoint(type, script_id_or_name, opt_line, opt_column,
                          opt_groupId, opt_position_alignment) {
  this.type_ = type;
  if (type == Debug.ScriptBreakPointType.ScriptId) {
    this.script_id_ = script_id_or_name;
  } else if (type == Debug.ScriptBreakPointType.ScriptName) {
    this.script_name_ = script_id_or_name;
  } else if (type == Debug.ScriptBreakPointType.ScriptRegExp) {
    this.script_regexp_object_ = new RegExp(script_id_or_name);
  } else {
    throw new Error("Unexpected breakpoint type " + type);
  }
  this.line_ = opt_line || 0;
  this.column_ = opt_column;
  this.groupId_ = opt_groupId;
  this.position_alignment_ = IS_UNDEFINED(opt_position_alignment)
      ? Debug.BreakPositionAlignment.Statement : opt_position_alignment;
  this.hit_count_ = 0;
  this.active_ = true;
  this.condition_ = null;
  this.ignoreCount_ = 0;
  this.break_points_ = [];
}


//Creates a clone of script breakpoint that is linked to another script.
ScriptBreakPoint.prototype.cloneForOtherScript = function (other_script) {
  var copy = new ScriptBreakPoint(Debug.ScriptBreakPointType.ScriptId,
      other_script.id, this.line_, this.column_, this.groupId_,
      this.position_alignment_);
  copy.number_ = next_break_point_number++;
  script_break_points.push(copy);

  copy.hit_count_ = this.hit_count_;
  copy.active_ = this.active_;
  copy.condition_ = this.condition_;
  copy.ignoreCount_ = this.ignoreCount_;
  return copy;
};


ScriptBreakPoint.prototype.number = function() {
  return this.number_;
};


ScriptBreakPoint.prototype.groupId = function() {
  return this.groupId_;
};


ScriptBreakPoint.prototype.type = function() {
  return this.type_;
};


ScriptBreakPoint.prototype.script_id = function() {
  return this.script_id_;
};


ScriptBreakPoint.prototype.script_name = function() {
  return this.script_name_;
};


ScriptBreakPoint.prototype.script_regexp_object = function() {
  return this.script_regexp_object_;
};


ScriptBreakPoint.prototype.line = function() {
  return this.line_;
};


ScriptBreakPoint.prototype.column = function() {
  return this.column_;
};


ScriptBreakPoint.prototype.actual_locations = function() {
  var locations = [];
  for (var i = 0; i < this.break_points_.length; i++) {
    locations.push(this.break_points_[i].actual_location);
  }
  return locations;
};


ScriptBreakPoint.prototype.update_positions = function(line, column) {
  this.line_ = line;
  this.column_ = column;
};


ScriptBreakPoint.prototype.hit_count = function() {
  return this.hit_count_;
};


ScriptBreakPoint.prototype.active = function() {
  return this.active_;
};


ScriptBreakPoint.prototype.condition = function() {
  return this.condition_;
};


ScriptBreakPoint.prototype.ignoreCount = function() {
  return this.ignoreCount_;
};


ScriptBreakPoint.prototype.enable = function() {
  this.active_ = true;
};


ScriptBreakPoint.prototype.disable = function() {
  this.active_ = false;
};


ScriptBreakPoint.prototype.setCondition = function(condition) {
  this.condition_ = condition;
};


ScriptBreakPoint.prototype.setIgnoreCount = function(ignoreCount) {
  this.ignoreCount_ = ignoreCount;

  // Set ignore count on all break points created from this script break point.
  for (var i = 0; i < this.break_points_.length; i++) {
    this.break_points_[i].setIgnoreCount(ignoreCount);
  }
};


// Check whether a script matches this script break point. Currently this is
// only based on script name.
ScriptBreakPoint.prototype.matchesScript = function(script) {
  if (this.type_ == Debug.ScriptBreakPointType.ScriptId) {
    return this.script_id_ == script.id;
  } else {
    // We might want to account columns here as well.
    if (!(script.line_offset <= this.line_  &&
          this.line_ < script.line_offset + script.lineCount())) {
      return false;
    }
    if (this.type_ == Debug.ScriptBreakPointType.ScriptName) {
      return this.script_name_ == script.nameOrSourceURL();
    } else if (this.type_ == Debug.ScriptBreakPointType.ScriptRegExp) {
      return this.script_regexp_object_.test(script.nameOrSourceURL());
    } else {
      throw new Error("Unexpected breakpoint type " + this.type_);
    }
  }
};


// Set the script break point in a script.
ScriptBreakPoint.prototype.set = function (script) {
  var column = this.column();
  var line = this.line();
  // If the column is undefined the break is on the line. To help locate the
  // first piece of breakable code on the line try to find the column on the
  // line which contains some source.
  if (IS_UNDEFINED(column)) {
    var source_line = script.sourceLine(this.line());

    // Allocate array for caching the columns where the actual source starts.
    if (!script.sourceColumnStart_) {
      script.sourceColumnStart_ = new Array(script.lineCount());
    }

    // Fill cache if needed and get column where the actual source starts.
    if (IS_UNDEFINED(script.sourceColumnStart_[line])) {
      script.sourceColumnStart_[line] =
          source_line.match(sourceLineBeginningSkip)[0].length;
    }
    column = script.sourceColumnStart_[line];
  }

  // Convert the line and column into an absolute position within the script.
  var position = Debug.findScriptSourcePosition(script, this.line(), column);

  // If the position is not found in the script (the script might be shorter
  // than it used to be) just ignore it.
  if (IS_NULL(position)) return;

  // Create a break point object and set the break point.
  break_point = MakeBreakPoint(position, this);
  break_point.setIgnoreCount(this.ignoreCount());
  var actual_position = %SetScriptBreakPoint(script, position,
                                             this.position_alignment_,
                                             break_point);
  if (IS_UNDEFINED(actual_position)) {
    actual_position = position;
  }
  var actual_location = script.locationFromPosition(actual_position, true);
  break_point.actual_location = { line: actual_location.line,
                                  column: actual_location.column,
                                  script_id: script.id };
  this.break_points_.push(break_point);
  return break_point;
};


// Clear all the break points created from this script break point
ScriptBreakPoint.prototype.clear = function () {
  var remaining_break_points = [];
  for (var i = 0; i < break_points.length; i++) {
    if (break_points[i].script_break_point() &&
        break_points[i].script_break_point() === this) {
      %ClearBreakPoint(break_points[i]);
    } else {
      remaining_break_points.push(break_points[i]);
    }
  }
  break_points = remaining_break_points;
  this.break_points_ = [];
};


// Function called from runtime when a new script is compiled to set any script
// break points set in this script.
function UpdateScriptBreakPoints(script) {
  for (var i = 0; i < script_break_points.length; i++) {
    var break_point = script_break_points[i];
    if ((break_point.type() == Debug.ScriptBreakPointType.ScriptName ||
         break_point.type() == Debug.ScriptBreakPointType.ScriptRegExp) &&
        break_point.matchesScript(script)) {
      break_point.set(script);
    }
  }
}


function GetScriptBreakPoints(script) {
  var result = [];
  for (var i = 0; i < script_break_points.length; i++) {
    if (script_break_points[i].matchesScript(script)) {
      result.push(script_break_points[i]);
    }
  }
  return result;
}


Debug.setListener = function(listener, opt_data) {
  if (!IS_FUNCTION(listener) && !IS_UNDEFINED(listener) && !IS_NULL(listener)) {
    throw new Error('Parameters have wrong types.');
  }
  %SetDebugEventListener(listener, opt_data);
};


Debug.breakExecution = function(f) {
  %Break();
};

Debug.breakLocations = function(f, opt_position_aligment) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  var position_aligment = IS_UNDEFINED(opt_position_aligment)
      ? Debug.BreakPositionAlignment.Statement : opt_position_aligment;
  return %GetBreakLocations(f, position_aligment);
};

// Returns a Script object. If the parameter is a function the return value
// is the script in which the function is defined. If the parameter is a string
// the return value is the script for which the script name has that string
// value.  If it is a regexp and there is a unique script whose name matches
// we return that, otherwise undefined.
Debug.findScript = function(func_or_script_name) {
  if (IS_FUNCTION(func_or_script_name)) {
    return %FunctionGetScript(func_or_script_name);
  } else if (IS_REGEXP(func_or_script_name)) {
    var scripts = Debug.scripts();
    var last_result = null;
    var result_count = 0;
    for (var i in scripts) {
      var script = scripts[i];
      if (func_or_script_name.test(script.name)) {
        last_result = script;
        result_count++;
      }
    }
    // Return the unique script matching the regexp.  If there are more
    // than one we don't return a value since there is no good way to
    // decide which one to return.  Returning a "random" one, say the
    // first, would introduce nondeterminism (or something close to it)
    // because the order is the heap iteration order.
    if (result_count == 1) {
      return last_result;
    } else {
      return undefined;
    }
  } else {
    return %GetScript(func_or_script_name);
  }
};

// Returns the script source. If the parameter is a function the return value
// is the script source for the script in which the function is defined. If the
// parameter is a string the return value is the script for which the script
// name has that string value.
Debug.scriptSource = function(func_or_script_name) {
  return this.findScript(func_or_script_name).source;
};

Debug.source = function(f) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  return %FunctionGetSourceCode(f);
};

Debug.disassemble = function(f) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  return %DebugDisassembleFunction(f);
};

Debug.disassembleConstructor = function(f) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  return %DebugDisassembleConstructor(f);
};

Debug.ExecuteInDebugContext = function(f, without_debugger) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  return %ExecuteInDebugContext(f, !!without_debugger);
};

Debug.sourcePosition = function(f) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  return %FunctionGetScriptSourcePosition(f);
};


Debug.findFunctionSourceLocation = function(func, opt_line, opt_column) {
  var script = %FunctionGetScript(func);
  var script_offset = %FunctionGetScriptSourcePosition(func);
  return script.locationFromLine(opt_line, opt_column, script_offset);
};


// Returns the character position in a script based on a line number and an
// optional position within that line.
Debug.findScriptSourcePosition = function(script, opt_line, opt_column) {
  var location = script.locationFromLine(opt_line, opt_column);
  return location ? location.position : null;
};


Debug.findBreakPoint = function(break_point_number, remove) {
  var break_point;
  for (var i = 0; i < break_points.length; i++) {
    if (break_points[i].number() == break_point_number) {
      break_point = break_points[i];
      // Remove the break point from the list if requested.
      if (remove) {
        break_points.splice(i, 1);
      }
      break;
    }
  }
  if (break_point) {
    return break_point;
  } else {
    return this.findScriptBreakPoint(break_point_number, remove);
  }
};

Debug.findBreakPointActualLocations = function(break_point_number) {
  for (var i = 0; i < script_break_points.length; i++) {
    if (script_break_points[i].number() == break_point_number) {
      return script_break_points[i].actual_locations();
    }
  }
  for (var i = 0; i < break_points.length; i++) {
    if (break_points[i].number() == break_point_number) {
      return [break_points[i].actual_location];
    }
  }
  return [];
};

Debug.setBreakPoint = function(func, opt_line, opt_column, opt_condition) {
  if (!IS_FUNCTION(func)) throw new Error('Parameters have wrong types.');
  // Break points in API functions are not supported.
  if (%FunctionIsAPIFunction(func)) {
    throw new Error('Cannot set break point in native code.');
  }
  // Find source position relative to start of the function
  var break_position =
      this.findFunctionSourceLocation(func, opt_line, opt_column).position;
  var source_position = break_position - this.sourcePosition(func);
  // Find the script for the function.
  var script = %FunctionGetScript(func);
  // Break in builtin JavaScript code is not supported.
  if (script.type == Debug.ScriptType.Native) {
    throw new Error('Cannot set break point in native code.');
  }
  // If the script for the function has a name convert this to a script break
  // point.
  if (script && script.id) {
    // Adjust the source position to be script relative.
    source_position += %FunctionGetScriptSourcePosition(func);
    // Find line and column for the position in the script and set a script
    // break point from that.
    var location = script.locationFromPosition(source_position, false);
    return this.setScriptBreakPointById(script.id,
                                        location.line, location.column,
                                        opt_condition);
  } else {
    // Set a break point directly on the function.
    var break_point = MakeBreakPoint(source_position);
    var actual_position =
        %SetFunctionBreakPoint(func, source_position, break_point);
    actual_position += this.sourcePosition(func);
    var actual_location = script.locationFromPosition(actual_position, true);
    break_point.actual_location = { line: actual_location.line,
                                    column: actual_location.column,
                                    script_id: script.id };
    break_point.setCondition(opt_condition);
    return break_point.number();
  }
};


Debug.setBreakPointByScriptIdAndPosition = function(script_id, position,
                                                    condition, enabled,
                                                    opt_position_alignment)
{
  break_point = MakeBreakPoint(position);
  break_point.setCondition(condition);
  if (!enabled) {
    break_point.disable();
  }
  var scripts = this.scripts();
  var position_alignment = IS_UNDEFINED(opt_position_alignment)
      ? Debug.BreakPositionAlignment.Statement : opt_position_alignment;
  for (var i = 0; i < scripts.length; i++) {
    if (script_id == scripts[i].id) {
      break_point.actual_position = %SetScriptBreakPoint(scripts[i], position,
          position_alignment, break_point);
      break;
    }
  }
  return break_point;
};


Debug.enableBreakPoint = function(break_point_number) {
  var break_point = this.findBreakPoint(break_point_number, false);
  // Only enable if the breakpoint hasn't been deleted:
  if (break_point) {
    break_point.enable();
  }
};


Debug.disableBreakPoint = function(break_point_number) {
  var break_point = this.findBreakPoint(break_point_number, false);
  // Only enable if the breakpoint hasn't been deleted:
  if (break_point) {
    break_point.disable();
  }
};


Debug.changeBreakPointCondition = function(break_point_number, condition) {
  var break_point = this.findBreakPoint(break_point_number, false);
  break_point.setCondition(condition);
};


Debug.changeBreakPointIgnoreCount = function(break_point_number, ignoreCount) {
  if (ignoreCount < 0) {
    throw new Error('Invalid argument');
  }
  var break_point = this.findBreakPoint(break_point_number, false);
  break_point.setIgnoreCount(ignoreCount);
};


Debug.clearBreakPoint = function(break_point_number) {
  var break_point = this.findBreakPoint(break_point_number, true);
  if (break_point) {
    return %ClearBreakPoint(break_point);
  } else {
    break_point = this.findScriptBreakPoint(break_point_number, true);
    if (!break_point) {
      throw new Error('Invalid breakpoint');
    }
  }
};


Debug.clearAllBreakPoints = function() {
  for (var i = 0; i < break_points.length; i++) {
    break_point = break_points[i];
    %ClearBreakPoint(break_point);
  }
  break_points = [];
};


Debug.disableAllBreakPoints = function() {
  // Disable all user defined breakpoints:
  for (var i = 1; i < next_break_point_number; i++) {
    Debug.disableBreakPoint(i);
  }
  // Disable all exception breakpoints:
  %ChangeBreakOnException(Debug.ExceptionBreak.Caught, false);
  %ChangeBreakOnException(Debug.ExceptionBreak.Uncaught, false);
};


Debug.findScriptBreakPoint = function(break_point_number, remove) {
  var script_break_point;
  for (var i = 0; i < script_break_points.length; i++) {
    if (script_break_points[i].number() == break_point_number) {
      script_break_point = script_break_points[i];
      // Remove the break point from the list if requested.
      if (remove) {
        script_break_point.clear();
        script_break_points.splice(i,1);
      }
      break;
    }
  }
  return script_break_point;
};


// Sets a breakpoint in a script identified through id or name at the
// specified source line and column within that line.
Debug.setScriptBreakPoint = function(type, script_id_or_name,
                                     opt_line, opt_column, opt_condition,
                                     opt_groupId, opt_position_alignment) {
  // Create script break point object.
  var script_break_point =
      new ScriptBreakPoint(type, script_id_or_name, opt_line, opt_column,
                           opt_groupId, opt_position_alignment);

  // Assign number to the new script break point and add it.
  script_break_point.number_ = next_break_point_number++;
  script_break_point.setCondition(opt_condition);
  script_break_points.push(script_break_point);

  // Run through all scripts to see if this script break point matches any
  // loaded scripts.
  var scripts = this.scripts();
  for (var i = 0; i < scripts.length; i++) {
    if (script_break_point.matchesScript(scripts[i])) {
      script_break_point.set(scripts[i]);
    }
  }

  return script_break_point.number();
};


Debug.setScriptBreakPointById = function(script_id,
                                         opt_line, opt_column,
                                         opt_condition, opt_groupId,
                                         opt_position_alignment) {
  return this.setScriptBreakPoint(Debug.ScriptBreakPointType.ScriptId,
                                  script_id, opt_line, opt_column,
                                  opt_condition, opt_groupId,
                                  opt_position_alignment);
};


Debug.setScriptBreakPointByName = function(script_name,
                                           opt_line, opt_column,
                                           opt_condition, opt_groupId) {
  return this.setScriptBreakPoint(Debug.ScriptBreakPointType.ScriptName,
                                  script_name, opt_line, opt_column,
                                  opt_condition, opt_groupId);
};


Debug.setScriptBreakPointByRegExp = function(script_regexp,
                                             opt_line, opt_column,
                                             opt_condition, opt_groupId) {
  return this.setScriptBreakPoint(Debug.ScriptBreakPointType.ScriptRegExp,
                                  script_regexp, opt_line, opt_column,
                                  opt_condition, opt_groupId);
};


Debug.enableScriptBreakPoint = function(break_point_number) {
  var script_break_point = this.findScriptBreakPoint(break_point_number, false);
  script_break_point.enable();
};


Debug.disableScriptBreakPoint = function(break_point_number) {
  var script_break_point = this.findScriptBreakPoint(break_point_number, false);
  script_break_point.disable();
};


Debug.changeScriptBreakPointCondition = function(
    break_point_number, condition) {
  var script_break_point = this.findScriptBreakPoint(break_point_number, false);
  script_break_point.setCondition(condition);
};


Debug.changeScriptBreakPointIgnoreCount = function(
    break_point_number, ignoreCount) {
  if (ignoreCount < 0) {
    throw new Error('Invalid argument');
  }
  var script_break_point = this.findScriptBreakPoint(break_point_number, false);
  script_break_point.setIgnoreCount(ignoreCount);
};


Debug.scriptBreakPoints = function() {
  return script_break_points;
};


Debug.clearStepping = function() {
  %ClearStepping();
};

Debug.setBreakOnException = function() {
  return %ChangeBreakOnException(Debug.ExceptionBreak.Caught, true);
};

Debug.clearBreakOnException = function() {
  return %ChangeBreakOnException(Debug.ExceptionBreak.Caught, false);
};

Debug.isBreakOnException = function() {
  return !!%IsBreakOnException(Debug.ExceptionBreak.Caught);
};

Debug.setBreakOnUncaughtException = function() {
  return %ChangeBreakOnException(Debug.ExceptionBreak.Uncaught, true);
};

Debug.clearBreakOnUncaughtException = function() {
  return %ChangeBreakOnException(Debug.ExceptionBreak.Uncaught, false);
};

Debug.isBreakOnUncaughtException = function() {
  return !!%IsBreakOnException(Debug.ExceptionBreak.Uncaught);
};

Debug.showBreakPoints = function(f, full, opt_position_alignment) {
  if (!IS_FUNCTION(f)) throw new Error('Parameters have wrong types.');
  var source = full ? this.scriptSource(f) : this.source(f);
  var offset = full ? this.sourcePosition(f) : 0;
  var locations = this.breakLocations(f, opt_position_alignment);
  if (!locations) return source;
  locations.sort(function(x, y) { return x - y; });
  var result = "";
  var prev_pos = 0;
  var pos;
  for (var i = 0; i < locations.length; i++) {
    pos = locations[i] - offset;
    result += source.slice(prev_pos, pos);
    result += "[B" + i + "]";
    prev_pos = pos;
  }
  pos = source.length;
  result += source.substring(prev_pos, pos);
  return result;
};


// Get all the scripts currently loaded. Locating all the scripts is based on
// scanning the heap.
Debug.scripts = function() {
  // Collect all scripts in the heap.
  return %DebugGetLoadedScripts();
};


Debug.debuggerFlags = function() {
  return debugger_flags;
};

Debug.MakeMirror = MakeMirror;

function MakeExecutionState(break_id) {
  return new ExecutionState(break_id);
}

function ExecutionState(break_id) {
  this.break_id = break_id;
  this.selected_frame = 0;
}

ExecutionState.prototype.prepareStep = function(opt_action, opt_count,
    opt_callframe) {
  var action = Debug.StepAction.StepIn;
  if (!IS_UNDEFINED(opt_action)) action = %ToNumber(opt_action);
  var count = opt_count ? %ToNumber(opt_count) : 1;
  var callFrameId = 0;
  if (!IS_UNDEFINED(opt_callframe)) {
    callFrameId = opt_callframe.details_.frameId();
  }

  return %PrepareStep(this.break_id, action, count, callFrameId);
};

ExecutionState.prototype.evaluateGlobal = function(source, disable_break,
    opt_additional_context) {
  return MakeMirror(%DebugEvaluateGlobal(this.break_id, source,
                                         Boolean(disable_break),
                                         opt_additional_context));
};

ExecutionState.prototype.frameCount = function() {
  return %GetFrameCount(this.break_id);
};

ExecutionState.prototype.threadCount = function() {
  return %GetThreadCount(this.break_id);
};

ExecutionState.prototype.frame = function(opt_index) {
  // If no index supplied return the selected frame.
  if (opt_index == null) opt_index = this.selected_frame;
  if (opt_index < 0 || opt_index >= this.frameCount()) {
    throw new Error('Illegal frame index.');
  }
  return new FrameMirror(this.break_id, opt_index);
};

ExecutionState.prototype.setSelectedFrame = function(index) {
  var i = %ToNumber(index);
  if (i < 0 || i >= this.frameCount()) throw new Error('Illegal frame index.');
  this.selected_frame = i;
};

ExecutionState.prototype.selectedFrame = function() {
  return this.selected_frame;
};

ExecutionState.prototype.debugCommandProcessor = function(opt_is_running) {
  return new DebugCommandProcessor(this, opt_is_running);
};


function MakeBreakEvent(exec_state, break_points_hit) {
  return new BreakEvent(exec_state, break_points_hit);
}


function BreakEvent(exec_state, break_points_hit) {
  this.exec_state_ = exec_state;
  this.break_points_hit_ = break_points_hit;
}


BreakEvent.prototype.executionState = function() {
  return this.exec_state_;
};


BreakEvent.prototype.eventType = function() {
  return Debug.DebugEvent.Break;
};


BreakEvent.prototype.func = function() {
  return this.exec_state_.frame(0).func();
};


BreakEvent.prototype.sourceLine = function() {
  return this.exec_state_.frame(0).sourceLine();
};


BreakEvent.prototype.sourceColumn = function() {
  return this.exec_state_.frame(0).sourceColumn();
};


BreakEvent.prototype.sourceLineText = function() {
  return this.exec_state_.frame(0).sourceLineText();
};


BreakEvent.prototype.breakPointsHit = function() {
  return this.break_points_hit_;
};


BreakEvent.prototype.toJSONProtocol = function() {
  var o = { seq: next_response_seq++,
            type: "event",
            event: "break",
            body: { invocationText: this.exec_state_.frame(0).invocationText(),
                  }
          };

  // Add script related information to the event if available.
  var script = this.func().script();
  if (script) {
    o.body.sourceLine = this.sourceLine(),
    o.body.sourceColumn = this.sourceColumn(),
    o.body.sourceLineText = this.sourceLineText(),
    o.body.script = MakeScriptObject_(script, false);
  }

  // Add an Array of break points hit if any.
  if (this.breakPointsHit()) {
    o.body.breakpoints = [];
    for (var i = 0; i < this.breakPointsHit().length; i++) {
      // Find the break point number. For break points originating from a
      // script break point supply the script break point number.
      var breakpoint = this.breakPointsHit()[i];
      var script_break_point = breakpoint.script_break_point();
      var number;
      if (script_break_point) {
        number = script_break_point.number();
      } else {
        number = breakpoint.number();
      }
      o.body.breakpoints.push(number);
    }
  }
  return JSON.stringify(ObjectToProtocolObject_(o));
};


function MakeExceptionEvent(exec_state, exception, uncaught) {
  return new ExceptionEvent(exec_state, exception, uncaught);
}


function ExceptionEvent(exec_state, exception, uncaught) {
  this.exec_state_ = exec_state;
  this.exception_ = exception;
  this.uncaught_ = uncaught;
}


ExceptionEvent.prototype.executionState = function() {
  return this.exec_state_;
};


ExceptionEvent.prototype.eventType = function() {
  return Debug.DebugEvent.Exception;
};


ExceptionEvent.prototype.exception = function() {
  return this.exception_;
};


ExceptionEvent.prototype.uncaught = function() {
  return this.uncaught_;
};


ExceptionEvent.prototype.func = function() {
  return this.exec_state_.frame(0).func();
};


ExceptionEvent.prototype.sourceLine = function() {
  return this.exec_state_.frame(0).sourceLine();
};


ExceptionEvent.prototype.sourceColumn = function() {
  return this.exec_state_.frame(0).sourceColumn();
};


ExceptionEvent.prototype.sourceLineText = function() {
  return this.exec_state_.frame(0).sourceLineText();
};


ExceptionEvent.prototype.toJSONProtocol = function() {
  var o = new ProtocolMessage();
  o.event = "exception";
  o.body = { uncaught: this.uncaught_,
             exception: MakeMirror(this.exception_)
           };

  // Exceptions might happen whithout any JavaScript frames.
  if (this.exec_state_.frameCount() > 0) {
    o.body.sourceLine = this.sourceLine();
    o.body.sourceColumn = this.sourceColumn();
    o.body.sourceLineText = this.sourceLineText();

    // Add script information to the event if available.
    var script = this.func().script();
    if (script) {
      o.body.script = MakeScriptObject_(script, false);
    }
  } else {
    o.body.sourceLine = -1;
  }

  return o.toJSONProtocol();
};


function MakeCompileEvent(exec_state, script, before) {
  return new CompileEvent(exec_state, script, before);
}


function CompileEvent(exec_state, script, before) {
  this.exec_state_ = exec_state;
  this.script_ = MakeMirror(script);
  this.before_ = before;
}


CompileEvent.prototype.executionState = function() {
  return this.exec_state_;
};


CompileEvent.prototype.eventType = function() {
  if (this.before_) {
    return Debug.DebugEvent.BeforeCompile;
  } else {
    return Debug.DebugEvent.AfterCompile;
  }
};


CompileEvent.prototype.script = function() {
  return this.script_;
};


CompileEvent.prototype.toJSONProtocol = function() {
  var o = new ProtocolMessage();
  o.running = true;
  if (this.before_) {
    o.event = "beforeCompile";
  } else {
    o.event = "afterCompile";
  }
  o.body = {};
  o.body.script = this.script_;

  return o.toJSONProtocol();
};


function MakeNewFunctionEvent(func) {
  return new NewFunctionEvent(func);
}


function NewFunctionEvent(func) {
  this.func = func;
}


NewFunctionEvent.prototype.eventType = function() {
  return Debug.DebugEvent.NewFunction;
};


NewFunctionEvent.prototype.name = function() {
  return this.func.name;
};


NewFunctionEvent.prototype.setBreakPoint = function(p) {
  Debug.setBreakPoint(this.func, p || 0);
};


function MakeScriptCollectedEvent(exec_state, id) {
  return new ScriptCollectedEvent(exec_state, id);
}


function ScriptCollectedEvent(exec_state, id) {
  this.exec_state_ = exec_state;
  this.id_ = id;
}


ScriptCollectedEvent.prototype.id = function() {
  return this.id_;
};


ScriptCollectedEvent.prototype.executionState = function() {
  return this.exec_state_;
};


ScriptCollectedEvent.prototype.toJSONProtocol = function() {
  var o = new ProtocolMessage();
  o.running = true;
  o.event = "scriptCollected";
  o.body = {};
  o.body.script = { id: this.id() };
  return o.toJSONProtocol();
};


function MakeScriptObject_(script, include_source) {
  var o = { id: script.id(),
            name: script.name(),
            lineOffset: script.lineOffset(),
            columnOffset: script.columnOffset(),
            lineCount: script.lineCount(),
          };
  if (!IS_UNDEFINED(script.data())) {
    o.data = script.data();
  }
  if (include_source) {
    o.source = script.source();
  }
  return o;
}


function DebugCommandProcessor(exec_state, opt_is_running) {
  this.exec_state_ = exec_state;
  this.running_ = opt_is_running || false;
}


DebugCommandProcessor.prototype.processDebugRequest = function (request) {
  return this.processDebugJSONRequest(request);
};


function ProtocolMessage(request) {
  // Update sequence number.
  this.seq = next_response_seq++;

  if (request) {
    // If message is based on a request this is a response. Fill the initial
    // response from the request.
    this.type = 'response';
    this.request_seq = request.seq;
    this.command = request.command;
  } else {
    // If message is not based on a request it is a dabugger generated event.
    this.type = 'event';
  }
  this.success = true;
  // Handler may set this field to control debugger state.
  this.running = undefined;
}


ProtocolMessage.prototype.setOption = function(name, value) {
  if (!this.options_) {
    this.options_ = {};
  }
  this.options_[name] = value;
};


ProtocolMessage.prototype.failed = function(message, opt_details) {
  this.success = false;
  this.message = message;
  if (IS_OBJECT(opt_details)) {
    this.error_details = opt_details;
  }
};


ProtocolMessage.prototype.toJSONProtocol = function() {
  // Encode the protocol header.
  var json = {};
  json.seq= this.seq;
  if (this.request_seq) {
    json.request_seq = this.request_seq;
  }
  json.type = this.type;
  if (this.event) {
    json.event = this.event;
  }
  if (this.command) {
    json.command = this.command;
  }
  if (this.success) {
    json.success = this.success;
  } else {
    json.success = false;
  }
  if (this.body) {
    // Encode the body part.
    var bodyJson;
    var serializer = MakeMirrorSerializer(true, this.options_);
    if (this.body instanceof Mirror) {
      bodyJson = serializer.serializeValue(this.body);
    } else if (this.body instanceof Array) {
      bodyJson = [];
      for (var i = 0; i < this.body.length; i++) {
        if (this.body[i] instanceof Mirror) {
          bodyJson.push(serializer.serializeValue(this.body[i]));
        } else {
          bodyJson.push(ObjectToProtocolObject_(this.body[i], serializer));
        }
      }
    } else {
      bodyJson = ObjectToProtocolObject_(this.body, serializer);
    }
    json.body = bodyJson;
    json.refs = serializer.serializeReferencedObjects();
  }
  if (this.message) {
    json.message = this.message;
  }
  if (this.error_details) {
    json.error_details = this.error_details;
  }
  json.running = this.running;
  return JSON.stringify(json);
};


DebugCommandProcessor.prototype.createResponse = function(request) {
  return new ProtocolMessage(request);
};


DebugCommandProcessor.prototype.processDebugJSONRequest = function(
    json_request) {
  var request;  // Current request.
  var response;  // Generated response.
  try {
    try {
      // Convert the JSON string to an object.
      request = JSON.parse(json_request);

      // Create an initial response.
      response = this.createResponse(request);

      if (!request.type) {
        throw new Error('Type not specified');
      }

      if (request.type != 'request') {
        throw new Error("Illegal type '" + request.type + "' in request");
      }

      if (!request.command) {
        throw new Error('Command not specified');
      }

      if (request.arguments) {
        var args = request.arguments;
        // TODO(yurys): remove request.arguments.compactFormat check once
        // ChromeDevTools are switched to 'inlineRefs'
        if (args.inlineRefs || args.compactFormat) {
          response.setOption('inlineRefs', true);
        }
        if (!IS_UNDEFINED(args.maxStringLength)) {
          response.setOption('maxStringLength', args.maxStringLength);
        }
      }

      if (request.command == 'continue') {
        this.continueRequest_(request, response);
      } else if (request.command == 'break') {
        this.breakRequest_(request, response);
      } else if (request.command == 'setbreakpoint') {
        this.setBreakPointRequest_(request, response);
      } else if (request.command == 'changebreakpoint') {
        this.changeBreakPointRequest_(request, response);
      } else if (request.command == 'clearbreakpoint') {
        this.clearBreakPointRequest_(request, response);
      } else if (request.command == 'clearbreakpointgroup') {
        this.clearBreakPointGroupRequest_(request, response);
      } else if (request.command == 'disconnect') {
        this.disconnectRequest_(request, response);
      } else if (request.command == 'setexceptionbreak') {
        this.setExceptionBreakRequest_(request, response);
      } else if (request.command == 'listbreakpoints') {
        this.listBreakpointsRequest_(request, response);
      } else if (request.command == 'backtrace') {
        this.backtraceRequest_(request, response);
      } else if (request.command == 'frame') {
        this.frameRequest_(request, response);
      } else if (request.command == 'scopes') {
        this.scopesRequest_(request, response);
      } else if (request.command == 'scope') {
        this.scopeRequest_(request, response);
      } else if (request.command == 'setVariableValue') {
        this.setVariableValueRequest_(request, response);
      } else if (request.command == 'evaluate') {
        this.evaluateRequest_(request, response);
      } else if (request.command == 'lookup') {
        this.lookupRequest_(request, response);
      } else if (request.command == 'references') {
        this.referencesRequest_(request, response);
      } else if (request.command == 'source') {
        this.sourceRequest_(request, response);
      } else if (request.command == 'scripts') {
        this.scriptsRequest_(request, response);
      } else if (request.command == 'threads') {
        this.threadsRequest_(request, response);
      } else if (request.command == 'suspend') {
        this.suspendRequest_(request, response);
      } else if (request.command == 'version') {
        this.versionRequest_(request, response);
      } else if (request.command == 'changelive') {
        this.changeLiveRequest_(request, response);
      } else if (request.command == 'restartframe') {
        this.restartFrameRequest_(request, response);
      } else if (request.command == 'flags') {
        this.debuggerFlagsRequest_(request, response);
      } else if (request.command == 'v8flags') {
        this.v8FlagsRequest_(request, response);

      // GC tools:
      } else if (request.command == 'gc') {
        this.gcRequest_(request, response);

      } else {
        throw new Error('Unknown command "' + request.command + '" in request');
      }
    } catch (e) {
      // If there is no response object created one (without command).
      if (!response) {
        response = this.createResponse();
      }
      response.success = false;
      response.message = %ToString(e);
    }

    // Return the response as a JSON encoded string.
    try {
      if (!IS_UNDEFINED(response.running)) {
        // Response controls running state.
        this.running_ = response.running;
      }
      response.running = this.running_;
      return response.toJSONProtocol();
    } catch (e) {
      // Failed to generate response - return generic error.
      return '{"seq":' + response.seq + ',' +
              '"request_seq":' + request.seq + ',' +
              '"type":"response",' +
              '"success":false,' +
              '"message":"Internal error: ' + %ToString(e) + '"}';
    }
  } catch (e) {
    // Failed in one of the catch blocks above - most generic error.
    return '{"seq":0,"type":"response","success":false,"message":"Internal error"}';
  }
};


DebugCommandProcessor.prototype.continueRequest_ = function(request, response) {
  // Check for arguments for continue.
  if (request.arguments) {
    var count = 1;
    var action = Debug.StepAction.StepIn;

    // Pull out arguments.
    var stepaction = request.arguments.stepaction;
    var stepcount = request.arguments.stepcount;

    // Get the stepcount argument if any.
    if (stepcount) {
      count = %ToNumber(stepcount);
      if (count < 0) {
        throw new Error('Invalid stepcount argument "' + stepcount + '".');
      }
    }

    // Get the stepaction argument.
    if (stepaction) {
      if (stepaction == 'in') {
        action = Debug.StepAction.StepIn;
      } else if (stepaction == 'min') {
        action = Debug.StepAction.StepMin;
      } else if (stepaction == 'next') {
        action = Debug.StepAction.StepNext;
      } else if (stepaction == 'out') {
        action = Debug.StepAction.StepOut;
      } else {
        throw new Error('Invalid stepaction argument "' + stepaction + '".');
      }
    }

    // Set up the VM for stepping.
    this.exec_state_.prepareStep(action, count);
  }

  // VM should be running after executing this request.
  response.running = true;
};


DebugCommandProcessor.prototype.breakRequest_ = function(request, response) {
  // Ignore as break command does not do anything when broken.
};


DebugCommandProcessor.prototype.setBreakPointRequest_ =
    function(request, response) {
  // Check for legal request.
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  // Pull out arguments.
  var type = request.arguments.type;
  var target = request.arguments.target;
  var line = request.arguments.line;
  var column = request.arguments.column;
  var enabled = IS_UNDEFINED(request.arguments.enabled) ?
      true : request.arguments.enabled;
  var condition = request.arguments.condition;
  var ignoreCount = request.arguments.ignoreCount;
  var groupId = request.arguments.groupId;

  // Check for legal arguments.
  if (!type || IS_UNDEFINED(target)) {
    response.failed('Missing argument "type" or "target"');
    return;
  }

  // Either function or script break point.
  var break_point_number;
  if (type == 'function') {
    // Handle function break point.
    if (!IS_STRING(target)) {
      response.failed('Argument "target" is not a string value');
      return;
    }
    var f;
    try {
      // Find the function through a global evaluate.
      f = this.exec_state_.evaluateGlobal(target).value();
    } catch (e) {
      response.failed('Error: "' + %ToString(e) +
                      '" evaluating "' + target + '"');
      return;
    }
    if (!IS_FUNCTION(f)) {
      response.failed('"' + target + '" does not evaluate to a function');
      return;
    }

    // Set function break point.
    break_point_number = Debug.setBreakPoint(f, line, column, condition);
  } else if (type == 'handle') {
    // Find the object pointed by the specified handle.
    var handle = parseInt(target, 10);
    var mirror = LookupMirror(handle);
    if (!mirror) {
      return response.failed('Object #' + handle + '# not found');
    }
    if (!mirror.isFunction()) {
      return response.failed('Object #' + handle + '# is not a function');
    }

    // Set function break point.
    break_point_number = Debug.setBreakPoint(mirror.value(),
                                             line, column, condition);
  } else if (type == 'script') {
    // set script break point.
    break_point_number =
        Debug.setScriptBreakPointByName(target, line, column, condition,
                                        groupId);
  } else if (type == 'scriptId') {
    break_point_number =
        Debug.setScriptBreakPointById(target, line, column, condition, groupId);
  } else if (type == 'scriptRegExp') {
    break_point_number =
        Debug.setScriptBreakPointByRegExp(target, line, column, condition,
                                          groupId);
  } else {
    response.failed('Illegal type "' + type + '"');
    return;
  }

  // Set additional break point properties.
  var break_point = Debug.findBreakPoint(break_point_number);
  if (ignoreCount) {
    Debug.changeBreakPointIgnoreCount(break_point_number, ignoreCount);
  }
  if (!enabled) {
    Debug.disableBreakPoint(break_point_number);
  }

  // Add the break point number to the response.
  response.body = { type: type,
                    breakpoint: break_point_number };

  // Add break point information to the response.
  if (break_point instanceof ScriptBreakPoint) {
    if (break_point.type() == Debug.ScriptBreakPointType.ScriptId) {
      response.body.type = 'scriptId';
      response.body.script_id = break_point.script_id();
    } else if (break_point.type() == Debug.ScriptBreakPointType.ScriptName) {
      response.body.type = 'scriptName';
      response.body.script_name = break_point.script_name();
    } else if (break_point.type() == Debug.ScriptBreakPointType.ScriptRegExp) {
      response.body.type = 'scriptRegExp';
      response.body.script_regexp = break_point.script_regexp_object().source;
    } else {
      throw new Error("Internal error: Unexpected breakpoint type: " +
                      break_point.type());
    }
    response.body.line = break_point.line();
    response.body.column = break_point.column();
    response.body.actual_locations = break_point.actual_locations();
  } else {
    response.body.type = 'function';
    response.body.actual_locations = [break_point.actual_location];
  }
};


DebugCommandProcessor.prototype.changeBreakPointRequest_ = function(
    request, response) {
  // Check for legal request.
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  // Pull out arguments.
  var break_point = %ToNumber(request.arguments.breakpoint);
  var enabled = request.arguments.enabled;
  var condition = request.arguments.condition;
  var ignoreCount = request.arguments.ignoreCount;

  // Check for legal arguments.
  if (!break_point) {
    response.failed('Missing argument "breakpoint"');
    return;
  }

  // Change enabled state if supplied.
  if (!IS_UNDEFINED(enabled)) {
    if (enabled) {
      Debug.enableBreakPoint(break_point);
    } else {
      Debug.disableBreakPoint(break_point);
    }
  }

  // Change condition if supplied
  if (!IS_UNDEFINED(condition)) {
    Debug.changeBreakPointCondition(break_point, condition);
  }

  // Change ignore count if supplied
  if (!IS_UNDEFINED(ignoreCount)) {
    Debug.changeBreakPointIgnoreCount(break_point, ignoreCount);
  }
};


DebugCommandProcessor.prototype.clearBreakPointGroupRequest_ = function(
    request, response) {
  // Check for legal request.
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  // Pull out arguments.
  var group_id = request.arguments.groupId;

  // Check for legal arguments.
  if (!group_id) {
    response.failed('Missing argument "groupId"');
    return;
  }

  var cleared_break_points = [];
  var new_script_break_points = [];
  for (var i = 0; i < script_break_points.length; i++) {
    var next_break_point = script_break_points[i];
    if (next_break_point.groupId() == group_id) {
      cleared_break_points.push(next_break_point.number());
      next_break_point.clear();
    } else {
      new_script_break_points.push(next_break_point);
    }
  }
  script_break_points = new_script_break_points;

  // Add the cleared break point numbers to the response.
  response.body = { breakpoints: cleared_break_points };
};


DebugCommandProcessor.prototype.clearBreakPointRequest_ = function(
    request, response) {
  // Check for legal request.
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  // Pull out arguments.
  var break_point = %ToNumber(request.arguments.breakpoint);

  // Check for legal arguments.
  if (!break_point) {
    response.failed('Missing argument "breakpoint"');
    return;
  }

  // Clear break point.
  Debug.clearBreakPoint(break_point);

  // Add the cleared break point number to the response.
  response.body = { breakpoint: break_point };
};


DebugCommandProcessor.prototype.listBreakpointsRequest_ = function(
    request, response) {
  var array = [];
  for (var i = 0; i < script_break_points.length; i++) {
    var break_point = script_break_points[i];

    var description = {
      number: break_point.number(),
      line: break_point.line(),
      column: break_point.column(),
      groupId: break_point.groupId(),
      hit_count: break_point.hit_count(),
      active: break_point.active(),
      condition: break_point.condition(),
      ignoreCount: break_point.ignoreCount(),
      actual_locations: break_point.actual_locations()
    };

    if (break_point.type() == Debug.ScriptBreakPointType.ScriptId) {
      description.type = 'scriptId';
      description.script_id = break_point.script_id();
    } else if (break_point.type() == Debug.ScriptBreakPointType.ScriptName) {
      description.type = 'scriptName';
      description.script_name = break_point.script_name();
    } else if (break_point.type() == Debug.ScriptBreakPointType.ScriptRegExp) {
      description.type = 'scriptRegExp';
      description.script_regexp = break_point.script_regexp_object().source;
    } else {
      throw new Error("Internal error: Unexpected breakpoint type: " +
                      break_point.type());
    }
    array.push(description);
  }

  response.body = {
    breakpoints: array,
    breakOnExceptions: Debug.isBreakOnException(),
    breakOnUncaughtExceptions: Debug.isBreakOnUncaughtException()
  };
};


DebugCommandProcessor.prototype.disconnectRequest_ =
    function(request, response) {
  Debug.disableAllBreakPoints();
  this.continueRequest_(request, response);
};


DebugCommandProcessor.prototype.setExceptionBreakRequest_ =
    function(request, response) {
  // Check for legal request.
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  // Pull out and check the 'type' argument:
  var type = request.arguments.type;
  if (!type) {
    response.failed('Missing argument "type"');
    return;
  }

  // Initialize the default value of enable:
  var enabled;
  if (type == 'all') {
    enabled = !Debug.isBreakOnException();
  } else if (type == 'uncaught') {
    enabled = !Debug.isBreakOnUncaughtException();
  }

  // Pull out and check the 'enabled' argument if present:
  if (!IS_UNDEFINED(request.arguments.enabled)) {
    enabled = request.arguments.enabled;
    if ((enabled != true) && (enabled != false)) {
      response.failed('Illegal value for "enabled":"' + enabled + '"');
    }
  }

  // Now set the exception break state:
  if (type == 'all') {
    %ChangeBreakOnException(Debug.ExceptionBreak.Caught, enabled);
  } else if (type == 'uncaught') {
    %ChangeBreakOnException(Debug.ExceptionBreak.Uncaught, enabled);
  } else {
    response.failed('Unknown "type":"' + type + '"');
  }

  // Add the cleared break point number to the response.
  response.body = { 'type': type, 'enabled': enabled };
};


DebugCommandProcessor.prototype.backtraceRequest_ = function(
    request, response) {
  // Get the number of frames.
  var total_frames = this.exec_state_.frameCount();

  // Create simple response if there are no frames.
  if (total_frames == 0) {
    response.body = {
      totalFrames: total_frames
    };
    return;
  }

  // Default frame range to include in backtrace.
  var from_index = 0;
  var to_index = kDefaultBacktraceLength;

  // Get the range from the arguments.
  if (request.arguments) {
    if (request.arguments.fromFrame) {
      from_index = request.arguments.fromFrame;
    }
    if (request.arguments.toFrame) {
      to_index = request.arguments.toFrame;
    }
    if (request.arguments.bottom) {
      var tmp_index = total_frames - from_index;
      from_index = total_frames - to_index;
      to_index = tmp_index;
    }
    if (from_index < 0 || to_index < 0) {
      return response.failed('Invalid frame number');
    }
  }

  // Adjust the index.
  to_index = Math.min(total_frames, to_index);

  if (to_index <= from_index) {
    var error = 'Invalid frame range';
    return response.failed(error);
  }

  // Create the response body.
  var frames = [];
  for (var i = from_index; i < to_index; i++) {
    frames.push(this.exec_state_.frame(i));
  }
  response.body = {
    fromFrame: from_index,
    toFrame: to_index,
    totalFrames: total_frames,
    frames: frames
  };
};


DebugCommandProcessor.prototype.frameRequest_ = function(request, response) {
  // No frames no source.
  if (this.exec_state_.frameCount() == 0) {
    return response.failed('No frames');
  }

  // With no arguments just keep the selected frame.
  if (request.arguments) {
    var index = request.arguments.number;
    if (index < 0 || this.exec_state_.frameCount() <= index) {
      return response.failed('Invalid frame number');
    }

    this.exec_state_.setSelectedFrame(request.arguments.number);
  }
  response.body = this.exec_state_.frame();
};


DebugCommandProcessor.prototype.resolveFrameFromScopeDescription_ =
    function(scope_description) {
  // Get the frame for which the scope or scopes are requested.
  // With no frameNumber argument use the currently selected frame.
  if (scope_description && !IS_UNDEFINED(scope_description.frameNumber)) {
    frame_index = scope_description.frameNumber;
    if (frame_index < 0 || this.exec_state_.frameCount() <= frame_index) {
      throw new Error('Invalid frame number');
    }
    return this.exec_state_.frame(frame_index);
  } else {
    return this.exec_state_.frame();
  }
};


// Gets scope host object from request. It is either a function
// ('functionHandle' argument must be specified) or a stack frame
// ('frameNumber' may be specified and the current frame is taken by default).
DebugCommandProcessor.prototype.resolveScopeHolder_ =
    function(scope_description) {
  if (scope_description && "functionHandle" in scope_description) {
    if (!IS_NUMBER(scope_description.functionHandle)) {
      throw new Error('Function handle must be a number');
    }
    var function_mirror = LookupMirror(scope_description.functionHandle);
    if (!function_mirror) {
      throw new Error('Failed to find function object by handle');
    }
    if (!function_mirror.isFunction()) {
      throw new Error('Value of non-function type is found by handle');
    }
    return function_mirror;
  } else {
    // No frames no scopes.
    if (this.exec_state_.frameCount() == 0) {
      throw new Error('No scopes');
    }

    // Get the frame for which the scopes are requested.
    var frame = this.resolveFrameFromScopeDescription_(scope_description);
    return frame;
  }
}


DebugCommandProcessor.prototype.scopesRequest_ = function(request, response) {
  var scope_holder = this.resolveScopeHolder_(request.arguments);

  // Fill all scopes for this frame or function.
  var total_scopes = scope_holder.scopeCount();
  var scopes = [];
  for (var i = 0; i < total_scopes; i++) {
    scopes.push(scope_holder.scope(i));
  }
  response.body = {
    fromScope: 0,
    toScope: total_scopes,
    totalScopes: total_scopes,
    scopes: scopes
  };
};


DebugCommandProcessor.prototype.scopeRequest_ = function(request, response) {
  // Get the frame or function for which the scope is requested.
  var scope_holder = this.resolveScopeHolder_(request.arguments);

  // With no scope argument just return top scope.
  var scope_index = 0;
  if (request.arguments && !IS_UNDEFINED(request.arguments.number)) {
    scope_index = %ToNumber(request.arguments.number);
    if (scope_index < 0 || scope_holder.scopeCount() <= scope_index) {
      return response.failed('Invalid scope number');
    }
  }

  response.body = scope_holder.scope(scope_index);
};


// Reads value from protocol description. Description may be in form of type
// (for singletons), raw value (primitive types supported in JSON),
// string value description plus type (for primitive values) or handle id.
// Returns raw value or throws exception.
DebugCommandProcessor.resolveValue_ = function(value_description) {
  if ("handle" in value_description) {
    var value_mirror = LookupMirror(value_description.handle);
    if (!value_mirror) {
      throw new Error("Failed to resolve value by handle, ' #" +
          mapping.handle + "# not found");
    }
    return value_mirror.value();
  } else if ("stringDescription" in value_description) {
    if (value_description.type == BOOLEAN_TYPE) {
      return Boolean(value_description.stringDescription);
    } else if (value_description.type == NUMBER_TYPE) {
      return Number(value_description.stringDescription);
    } if (value_description.type == STRING_TYPE) {
      return String(value_description.stringDescription);
    } else {
      throw new Error("Unknown type");
    }
  } else if ("value" in value_description) {
    return value_description.value;
  } else if (value_description.type == UNDEFINED_TYPE) {
    return UNDEFINED;
  } else if (value_description.type == NULL_TYPE) {
    return null;
  } else {
    throw new Error("Failed to parse value description");
  }
};


DebugCommandProcessor.prototype.setVariableValueRequest_ =
    function(request, response) {
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  if (IS_UNDEFINED(request.arguments.name)) {
    response.failed('Missing variable name');
  }
  var variable_name = request.arguments.name;

  var scope_description = request.arguments.scope;

  // Get the frame or function for which the scope is requested.
  var scope_holder = this.resolveScopeHolder_(scope_description);

  if (IS_UNDEFINED(scope_description.number)) {
    response.failed('Missing scope number');
  }
  var scope_index = %ToNumber(scope_description.number);

  var scope = scope_holder.scope(scope_index);

  var new_value =
      DebugCommandProcessor.resolveValue_(request.arguments.newValue);

  scope.setVariableValue(variable_name, new_value);

  var new_value_mirror = MakeMirror(new_value);

  response.body = {
    newValue: new_value_mirror
  };
};


DebugCommandProcessor.prototype.evaluateRequest_ = function(request, response) {
  if (!request.arguments) {
    return response.failed('Missing arguments');
  }

  // Pull out arguments.
  var expression = request.arguments.expression;
  var frame = request.arguments.frame;
  var global = request.arguments.global;
  var disable_break = request.arguments.disable_break;
  var additional_context = request.arguments.additional_context;

  // The expression argument could be an integer so we convert it to a
  // string.
  try {
    expression = String(expression);
  } catch(e) {
    return response.failed('Failed to convert expression argument to string');
  }

  // Check for legal arguments.
  if (!IS_UNDEFINED(frame) && global) {
    return response.failed('Arguments "frame" and "global" are exclusive');
  }

  var additional_context_object;
  if (additional_context) {
    additional_context_object = {};
    for (var i = 0; i < additional_context.length; i++) {
      var mapping = additional_context[i];

      if (!IS_STRING(mapping.name)) {
        return response.failed("Context element #" + i +
            " doesn't contain name:string property");
      }

      var raw_value = DebugCommandProcessor.resolveValue_(mapping);
      additional_context_object[mapping.name] = raw_value;
    }
  }

  // Global evaluate.
  if (global) {
    // Evaluate in the native context.
    response.body = this.exec_state_.evaluateGlobal(
        expression, Boolean(disable_break), additional_context_object);
    return;
  }

  // Default value for disable_break is true.
  if (IS_UNDEFINED(disable_break)) {
    disable_break = true;
  }

  // No frames no evaluate in frame.
  if (this.exec_state_.frameCount() == 0) {
    return response.failed('No frames');
  }

  // Check whether a frame was specified.
  if (!IS_UNDEFINED(frame)) {
    var frame_number = %ToNumber(frame);
    if (frame_number < 0 || frame_number >= this.exec_state_.frameCount()) {
      return response.failed('Invalid frame "' + frame + '"');
    }
    // Evaluate in the specified frame.
    response.body = this.exec_state_.frame(frame_number).evaluate(
        expression, Boolean(disable_break), additional_context_object);
    return;
  } else {
    // Evaluate in the selected frame.
    response.body = this.exec_state_.frame().evaluate(
        expression, Boolean(disable_break), additional_context_object);
    return;
  }
};


DebugCommandProcessor.prototype.lookupRequest_ = function(request, response) {
  if (!request.arguments) {
    return response.failed('Missing arguments');
  }

  // Pull out arguments.
  var handles = request.arguments.handles;

  // Check for legal arguments.
  if (IS_UNDEFINED(handles)) {
    return response.failed('Argument "handles" missing');
  }

  // Set 'includeSource' option for script lookup.
  if (!IS_UNDEFINED(request.arguments.includeSource)) {
    includeSource = %ToBoolean(request.arguments.includeSource);
    response.setOption('includeSource', includeSource);
  }

  // Lookup handles.
  var mirrors = {};
  for (var i = 0; i < handles.length; i++) {
    var handle = handles[i];
    var mirror = LookupMirror(handle);
    if (!mirror) {
      return response.failed('Object #' + handle + '# not found');
    }
    mirrors[handle] = mirror;
  }
  response.body = mirrors;
};


DebugCommandProcessor.prototype.referencesRequest_ =
    function(request, response) {
  if (!request.arguments) {
    return response.failed('Missing arguments');
  }

  // Pull out arguments.
  var type = request.arguments.type;
  var handle = request.arguments.handle;

  // Check for legal arguments.
  if (IS_UNDEFINED(type)) {
    return response.failed('Argument "type" missing');
  }
  if (IS_UNDEFINED(handle)) {
    return response.failed('Argument "handle" missing');
  }
  if (type != 'referencedBy' && type != 'constructedBy') {
    return response.failed('Invalid type "' + type + '"');
  }

  // Lookup handle and return objects with references the object.
  var mirror = LookupMirror(handle);
  if (mirror) {
    if (type == 'referencedBy') {
      response.body = mirror.referencedBy();
    } else {
      response.body = mirror.constructedBy();
    }
  } else {
    return response.failed('Object #' + handle + '# not found');
  }
};


DebugCommandProcessor.prototype.sourceRequest_ = function(request, response) {
  // No frames no source.
  if (this.exec_state_.frameCount() == 0) {
    return response.failed('No source');
  }

  var from_line;
  var to_line;
  var frame = this.exec_state_.frame();
  if (request.arguments) {
    // Pull out arguments.
    from_line = request.arguments.fromLine;
    to_line = request.arguments.toLine;

    if (!IS_UNDEFINED(request.arguments.frame)) {
      var frame_number = %ToNumber(request.arguments.frame);
      if (frame_number < 0 || frame_number >= this.exec_state_.frameCount()) {
        return response.failed('Invalid frame "' + frame + '"');
      }
      frame = this.exec_state_.frame(frame_number);
    }
  }

  // Get the script selected.
  var script = frame.func().script();
  if (!script) {
    return response.failed('No source');
  }

  // Get the source slice and fill it into the response.
  var slice = script.sourceSlice(from_line, to_line);
  if (!slice) {
    return response.failed('Invalid line interval');
  }
  response.body = {};
  response.body.source = slice.sourceText();
  response.body.fromLine = slice.from_line;
  response.body.toLine = slice.to_line;
  response.body.fromPosition = slice.from_position;
  response.body.toPosition = slice.to_position;
  response.body.totalLines = script.lineCount();
};


DebugCommandProcessor.prototype.scriptsRequest_ = function(request, response) {
  var types = ScriptTypeFlag(Debug.ScriptType.Normal);
  var includeSource = false;
  var idsToInclude = null;
  if (request.arguments) {
    // Pull out arguments.
    if (!IS_UNDEFINED(request.arguments.types)) {
      types = %ToNumber(request.arguments.types);
      if (isNaN(types) || types < 0) {
        return response.failed('Invalid types "' +
                               request.arguments.types + '"');
      }
    }

    if (!IS_UNDEFINED(request.arguments.includeSource)) {
      includeSource = %ToBoolean(request.arguments.includeSource);
      response.setOption('includeSource', includeSource);
    }

    if (IS_ARRAY(request.arguments.ids)) {
      idsToInclude = {};
      var ids = request.arguments.ids;
      for (var i = 0; i < ids.length; i++) {
        idsToInclude[ids[i]] = true;
      }
    }

    var filterStr = null;
    var filterNum = null;
    if (!IS_UNDEFINED(request.arguments.filter)) {
      var num = %ToNumber(request.arguments.filter);
      if (!isNaN(num)) {
        filterNum = num;
      }
      filterStr = request.arguments.filter;
    }
  }

  // Collect all scripts in the heap.
  var scripts = %DebugGetLoadedScripts();

  response.body = [];

  for (var i = 0; i < scripts.length; i++) {
    if (idsToInclude && !idsToInclude[scripts[i].id]) {
      continue;
    }
    if (filterStr || filterNum) {
      var script = scripts[i];
      var found = false;
      if (filterNum && !found) {
        if (script.id && script.id === filterNum) {
          found = true;
        }
      }
      if (filterStr && !found) {
        if (script.name && script.name.indexOf(filterStr) >= 0) {
          found = true;
        }
      }
      if (!found) continue;
    }
    if (types & ScriptTypeFlag(scripts[i].type)) {
      response.body.push(MakeMirror(scripts[i]));
    }
  }
};


DebugCommandProcessor.prototype.threadsRequest_ = function(request, response) {
  // Get the number of threads.
  var total_threads = this.exec_state_.threadCount();

  // Get information for all threads.
  var threads = [];
  for (var i = 0; i < total_threads; i++) {
    var details = %GetThreadDetails(this.exec_state_.break_id, i);
    var thread_info = { current: details[0],
                        id: details[1]
                      };
    threads.push(thread_info);
  }

  // Create the response body.
  response.body = {
    totalThreads: total_threads,
    threads: threads
  };
};


DebugCommandProcessor.prototype.suspendRequest_ = function(request, response) {
  response.running = false;
};


DebugCommandProcessor.prototype.versionRequest_ = function(request, response) {
  response.body = {
    V8Version: %GetV8Version()
  };
};


DebugCommandProcessor.prototype.changeLiveRequest_ = function(
    request, response) {
  if (!request.arguments) {
    return response.failed('Missing arguments');
  }
  var script_id = request.arguments.script_id;
  var preview_only = !!request.arguments.preview_only;

  var scripts = %DebugGetLoadedScripts();

  var the_script = null;
  for (var i = 0; i < scripts.length; i++) {
    if (scripts[i].id == script_id) {
      the_script = scripts[i];
    }
  }
  if (!the_script) {
    response.failed('Script not found');
    return;
  }

  var change_log = new Array();

  if (!IS_STRING(request.arguments.new_source)) {
    throw "new_source argument expected";
  }

  var new_source = request.arguments.new_source;

  var result_description;
  try {
    result_description = Debug.LiveEdit.SetScriptSource(the_script,
        new_source, preview_only, change_log);
  } catch (e) {
    if (e instanceof Debug.LiveEdit.Failure && "details" in e) {
      response.failed(e.message, e.details);
      return;
    }
    throw e;
  }
  response.body = {change_log: change_log, result: result_description};

  if (!preview_only && !this.running_ && result_description.stack_modified) {
    response.body.stepin_recommended = true;
  }
};


DebugCommandProcessor.prototype.restartFrameRequest_ = function(
    request, response) {
  if (!request.arguments) {
    return response.failed('Missing arguments');
  }
  var frame = request.arguments.frame;

  // No frames to evaluate in frame.
  if (this.exec_state_.frameCount() == 0) {
    return response.failed('No frames');
  }

  var frame_mirror;
  // Check whether a frame was specified.
  if (!IS_UNDEFINED(frame)) {
    var frame_number = %ToNumber(frame);
    if (frame_number < 0 || frame_number >= this.exec_state_.frameCount()) {
      return response.failed('Invalid frame "' + frame + '"');
    }
    // Restart specified frame.
    frame_mirror = this.exec_state_.frame(frame_number);
  } else {
    // Restart selected frame.
    frame_mirror = this.exec_state_.frame();
  }

  var result_description = Debug.LiveEdit.RestartFrame(frame_mirror);
  response.body = {result: result_description};
};


DebugCommandProcessor.prototype.debuggerFlagsRequest_ = function(request,
                                                                 response) {
  // Check for legal request.
  if (!request.arguments) {
    response.failed('Missing arguments');
    return;
  }

  // Pull out arguments.
  var flags = request.arguments.flags;

  response.body = { flags: [] };
  if (!IS_UNDEFINED(flags)) {
    for (var i = 0; i < flags.length; i++) {
      var name = flags[i].name;
      var debugger_flag = debugger_flags[name];
      if (!debugger_flag) {
        continue;
      }
      if ('value' in flags[i]) {
        debugger_flag.setValue(flags[i].value);
      }
      response.body.flags.push({ name: name, value: debugger_flag.getValue() });
    }
  } else {
    for (var name in debugger_flags) {
      var value = debugger_flags[name].getValue();
      response.body.flags.push({ name: name, value: value });
    }
  }
};


DebugCommandProcessor.prototype.v8FlagsRequest_ = function(request, response) {
  var flags = request.arguments.flags;
  if (!flags) flags = '';
  %SetFlags(flags);
};


DebugCommandProcessor.prototype.gcRequest_ = function(request, response) {
  var type = request.arguments.type;
  if (!type) type = 'all';

  var before = %GetHeapUsage();
  %CollectGarbage(type);
  var after = %GetHeapUsage();

  response.body = { "before": before, "after": after };
};


// Check whether the previously processed command caused the VM to become
// running.
DebugCommandProcessor.prototype.isRunning = function() {
  return this.running_;
};


DebugCommandProcessor.prototype.systemBreak = function(cmd, args) {
  return %SystemBreak();
};


function NumberToHex8Str(n) {
  var r = "";
  for (var i = 0; i < 8; ++i) {
    var c = hexCharArray[n & 0x0F];  // hexCharArray is defined in uri.js
    r = c + r;
    n = n >>> 4;
  }
  return r;
}


/**
 * Convert an Object to its debugger protocol representation. The representation
 * may be serilized to a JSON object using JSON.stringify().
 * This implementation simply runs through all string property names, converts
 * each property value to a protocol value and adds the property to the result
 * object. For type "object" the function will be called recursively. Note that
 * circular structures will cause infinite recursion.
 * @param {Object} object The object to format as protocol object.
 * @param {MirrorSerializer} mirror_serializer The serializer to use if any
 *     mirror objects are encountered.
 * @return {Object} Protocol object value.
 */
function ObjectToProtocolObject_(object, mirror_serializer) {
  var content = {};
  for (var key in object) {
    // Only consider string keys.
    if (typeof key == 'string') {
      // Format the value based on its type.
      var property_value_json = ValueToProtocolValue_(object[key],
                                                      mirror_serializer);
      // Add the property if relevant.
      if (!IS_UNDEFINED(property_value_json)) {
        content[key] = property_value_json;
      }
    }
  }

  return content;
}


/**
 * Convert an array to its debugger protocol representation. It will convert
 * each array element to a protocol value.
 * @param {Array} array The array to format as protocol array.
 * @param {MirrorSerializer} mirror_serializer The serializer to use if any
 *     mirror objects are encountered.
 * @return {Array} Protocol array value.
 */
function ArrayToProtocolArray_(array, mirror_serializer) {
  var json = [];
  for (var i = 0; i < array.length; i++) {
    json.push(ValueToProtocolValue_(array[i], mirror_serializer));
  }
  return json;
}


/**
 * Convert a value to its debugger protocol representation.
 * @param {*} value The value to format as protocol value.
 * @param {MirrorSerializer} mirror_serializer The serializer to use if any
 *     mirror objects are encountered.
 * @return {*} Protocol value.
 */
function ValueToProtocolValue_(value, mirror_serializer) {
  // Format the value based on its type.
  var json;
  switch (typeof value) {
    case 'object':
      if (value instanceof Mirror) {
        json = mirror_serializer.serializeValue(value);
      } else if (IS_ARRAY(value)){
        json = ArrayToProtocolArray_(value, mirror_serializer);
      } else {
        json = ObjectToProtocolObject_(value, mirror_serializer);
      }
      break;

    case 'boolean':
    case 'string':
    case 'number':
      json = value;
      break;

    default:
      json = null;
  }
  return json;
}

Debug.TestApi = {
  CommandProcessorResolveValue: DebugCommandProcessor.resolveValue_
};
