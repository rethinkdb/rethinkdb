// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/arguments.h"
#include "src/debug.h"
#include "src/liveedit.h"
#include "src/runtime/runtime.h"
#include "src/runtime/runtime-utils.h"

namespace v8 {
namespace internal {


static int FindSharedFunctionInfosForScript(HeapIterator* iterator,
                                            Script* script,
                                            FixedArray* buffer) {
  DisallowHeapAllocation no_allocation;
  int counter = 0;
  int buffer_size = buffer->length();
  for (HeapObject* obj = iterator->next(); obj != NULL;
       obj = iterator->next()) {
    DCHECK(obj != NULL);
    if (!obj->IsSharedFunctionInfo()) {
      continue;
    }
    SharedFunctionInfo* shared = SharedFunctionInfo::cast(obj);
    if (shared->script() != script) {
      continue;
    }
    if (counter < buffer_size) {
      buffer->set(counter, shared);
    }
    counter++;
  }
  return counter;
}


// For a script finds all SharedFunctionInfo's in the heap that points
// to this script. Returns JSArray of SharedFunctionInfo wrapped
// in OpaqueReferences.
RUNTIME_FUNCTION(Runtime_LiveEditFindSharedFunctionInfosForScript) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 1);
  CONVERT_ARG_CHECKED(JSValue, script_value, 0);

  RUNTIME_ASSERT(script_value->value()->IsScript());
  Handle<Script> script = Handle<Script>(Script::cast(script_value->value()));

  const int kBufferSize = 32;

  Handle<FixedArray> array;
  array = isolate->factory()->NewFixedArray(kBufferSize);
  int number;
  Heap* heap = isolate->heap();
  {
    HeapIterator heap_iterator(heap);
    Script* scr = *script;
    FixedArray* arr = *array;
    number = FindSharedFunctionInfosForScript(&heap_iterator, scr, arr);
  }
  if (number > kBufferSize) {
    array = isolate->factory()->NewFixedArray(number);
    HeapIterator heap_iterator(heap);
    Script* scr = *script;
    FixedArray* arr = *array;
    FindSharedFunctionInfosForScript(&heap_iterator, scr, arr);
  }

  Handle<JSArray> result = isolate->factory()->NewJSArrayWithElements(array);
  result->set_length(Smi::FromInt(number));

  LiveEdit::WrapSharedFunctionInfos(result);

  return *result;
}


// For a script calculates compilation information about all its functions.
// The script source is explicitly specified by the second argument.
// The source of the actual script is not used, however it is important that
// all generated code keeps references to this particular instance of script.
// Returns a JSArray of compilation infos. The array is ordered so that
// each function with all its descendant is always stored in a continues range
// with the function itself going first. The root function is a script function.
RUNTIME_FUNCTION(Runtime_LiveEditGatherCompileInfo) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_ARG_CHECKED(JSValue, script, 0);
  CONVERT_ARG_HANDLE_CHECKED(String, source, 1);

  RUNTIME_ASSERT(script->value()->IsScript());
  Handle<Script> script_handle = Handle<Script>(Script::cast(script->value()));

  Handle<JSArray> result;
  ASSIGN_RETURN_FAILURE_ON_EXCEPTION(
      isolate, result, LiveEdit::GatherCompileInfo(script_handle, source));
  return *result;
}


// Changes the source of the script to a new_source.
// If old_script_name is provided (i.e. is a String), also creates a copy of
// the script with its original source and sends notification to debugger.
RUNTIME_FUNCTION(Runtime_LiveEditReplaceScript) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 3);
  CONVERT_ARG_CHECKED(JSValue, original_script_value, 0);
  CONVERT_ARG_HANDLE_CHECKED(String, new_source, 1);
  CONVERT_ARG_HANDLE_CHECKED(Object, old_script_name, 2);

  RUNTIME_ASSERT(original_script_value->value()->IsScript());
  Handle<Script> original_script(Script::cast(original_script_value->value()));

  Handle<Object> old_script = LiveEdit::ChangeScriptSource(
      original_script, new_source, old_script_name);

  if (old_script->IsScript()) {
    Handle<Script> script_handle = Handle<Script>::cast(old_script);
    return *Script::GetWrapper(script_handle);
  } else {
    return isolate->heap()->null_value();
  }
}


RUNTIME_FUNCTION(Runtime_LiveEditFunctionSourceUpdated) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 1);
  CONVERT_ARG_HANDLE_CHECKED(JSArray, shared_info, 0);
  RUNTIME_ASSERT(SharedInfoWrapper::IsInstance(shared_info));

  LiveEdit::FunctionSourceUpdated(shared_info);
  return isolate->heap()->undefined_value();
}


// Replaces code of SharedFunctionInfo with a new one.
RUNTIME_FUNCTION(Runtime_LiveEditReplaceFunctionCode) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_ARG_HANDLE_CHECKED(JSArray, new_compile_info, 0);
  CONVERT_ARG_HANDLE_CHECKED(JSArray, shared_info, 1);
  RUNTIME_ASSERT(SharedInfoWrapper::IsInstance(shared_info));

  LiveEdit::ReplaceFunctionCode(new_compile_info, shared_info);
  return isolate->heap()->undefined_value();
}


// Connects SharedFunctionInfo to another script.
RUNTIME_FUNCTION(Runtime_LiveEditFunctionSetScript) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_ARG_HANDLE_CHECKED(Object, function_object, 0);
  CONVERT_ARG_HANDLE_CHECKED(Object, script_object, 1);

  if (function_object->IsJSValue()) {
    Handle<JSValue> function_wrapper = Handle<JSValue>::cast(function_object);
    if (script_object->IsJSValue()) {
      RUNTIME_ASSERT(JSValue::cast(*script_object)->value()->IsScript());
      Script* script = Script::cast(JSValue::cast(*script_object)->value());
      script_object = Handle<Object>(script, isolate);
    }
    RUNTIME_ASSERT(function_wrapper->value()->IsSharedFunctionInfo());
    LiveEdit::SetFunctionScript(function_wrapper, script_object);
  } else {
    // Just ignore this. We may not have a SharedFunctionInfo for some functions
    // and we check it in this function.
  }

  return isolate->heap()->undefined_value();
}


// In a code of a parent function replaces original function as embedded object
// with a substitution one.
RUNTIME_FUNCTION(Runtime_LiveEditReplaceRefToNestedFunction) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 3);

  CONVERT_ARG_HANDLE_CHECKED(JSValue, parent_wrapper, 0);
  CONVERT_ARG_HANDLE_CHECKED(JSValue, orig_wrapper, 1);
  CONVERT_ARG_HANDLE_CHECKED(JSValue, subst_wrapper, 2);
  RUNTIME_ASSERT(parent_wrapper->value()->IsSharedFunctionInfo());
  RUNTIME_ASSERT(orig_wrapper->value()->IsSharedFunctionInfo());
  RUNTIME_ASSERT(subst_wrapper->value()->IsSharedFunctionInfo());

  LiveEdit::ReplaceRefToNestedFunction(parent_wrapper, orig_wrapper,
                                       subst_wrapper);
  return isolate->heap()->undefined_value();
}


// Updates positions of a shared function info (first parameter) according
// to script source change. Text change is described in second parameter as
// array of groups of 3 numbers:
// (change_begin, change_end, change_end_new_position).
// Each group describes a change in text; groups are sorted by change_begin.
RUNTIME_FUNCTION(Runtime_LiveEditPatchFunctionPositions) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_ARG_HANDLE_CHECKED(JSArray, shared_array, 0);
  CONVERT_ARG_HANDLE_CHECKED(JSArray, position_change_array, 1);
  RUNTIME_ASSERT(SharedInfoWrapper::IsInstance(shared_array))

  LiveEdit::PatchFunctionPositions(shared_array, position_change_array);
  return isolate->heap()->undefined_value();
}


// For array of SharedFunctionInfo's (each wrapped in JSValue)
// checks that none of them have activations on stacks (of any thread).
// Returns array of the same length with corresponding results of
// LiveEdit::FunctionPatchabilityStatus type.
RUNTIME_FUNCTION(Runtime_LiveEditCheckAndDropActivations) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_ARG_HANDLE_CHECKED(JSArray, shared_array, 0);
  CONVERT_BOOLEAN_ARG_CHECKED(do_drop, 1);
  RUNTIME_ASSERT(shared_array->length()->IsSmi());
  RUNTIME_ASSERT(shared_array->HasFastElements())
  int array_length = Smi::cast(shared_array->length())->value();
  for (int i = 0; i < array_length; i++) {
    Handle<Object> element =
        Object::GetElement(isolate, shared_array, i).ToHandleChecked();
    RUNTIME_ASSERT(
        element->IsJSValue() &&
        Handle<JSValue>::cast(element)->value()->IsSharedFunctionInfo());
  }

  return *LiveEdit::CheckAndDropActivations(shared_array, do_drop);
}


// Compares 2 strings line-by-line, then token-wise and returns diff in form
// of JSArray of triplets (pos1, pos1_end, pos2_end) describing list
// of diff chunks.
RUNTIME_FUNCTION(Runtime_LiveEditCompareStrings) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_ARG_HANDLE_CHECKED(String, s1, 0);
  CONVERT_ARG_HANDLE_CHECKED(String, s2, 1);

  return *LiveEdit::CompareStrings(s1, s2);
}


// Restarts a call frame and completely drops all frames above.
// Returns true if successful. Otherwise returns undefined or an error message.
RUNTIME_FUNCTION(Runtime_LiveEditRestartFrame) {
  HandleScope scope(isolate);
  CHECK(isolate->debug()->live_edit_enabled());
  DCHECK(args.length() == 2);
  CONVERT_NUMBER_CHECKED(int, break_id, Int32, args[0]);
  RUNTIME_ASSERT(isolate->debug()->CheckExecutionState(break_id));

  CONVERT_NUMBER_CHECKED(int, index, Int32, args[1]);
  Heap* heap = isolate->heap();

  // Find the relevant frame with the requested index.
  StackFrame::Id id = isolate->debug()->break_frame_id();
  if (id == StackFrame::NO_ID) {
    // If there are no JavaScript stack frames return undefined.
    return heap->undefined_value();
  }

  JavaScriptFrameIterator it(isolate, id);
  int inlined_jsframe_index = Runtime::FindIndexedNonNativeFrame(&it, index);
  if (inlined_jsframe_index == -1) return heap->undefined_value();
  // We don't really care what the inlined frame index is, since we are
  // throwing away the entire frame anyways.
  const char* error_message = LiveEdit::RestartFrame(it.frame());
  if (error_message) {
    return *(isolate->factory()->InternalizeUtf8String(error_message));
  }
  return heap->true_value();
}
}
}  // namespace v8::internal
