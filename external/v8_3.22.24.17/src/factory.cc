// Copyright 2013 the V8 project authors. All rights reserved.
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

#include "v8.h"

#include "api.h"
#include "debug.h"
#include "execution.h"
#include "factory.h"
#include "isolate-inl.h"
#include "macro-assembler.h"
#include "objects.h"
#include "objects-visiting.h"
#include "platform.h"
#include "scopeinfo.h"

namespace v8 {
namespace internal {


Handle<Box> Factory::NewBox(Handle<Object> value, PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateBox(*value, pretenure),
      Box);
}


Handle<FixedArray> Factory::NewFixedArray(int size, PretenureFlag pretenure) {
  ASSERT(0 <= size);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateFixedArray(size, pretenure),
      FixedArray);
}


Handle<FixedArray> Factory::NewFixedArrayWithHoles(int size,
                                                   PretenureFlag pretenure) {
  ASSERT(0 <= size);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateFixedArrayWithHoles(size, pretenure),
      FixedArray);
}


Handle<FixedDoubleArray> Factory::NewFixedDoubleArray(int size,
                                                      PretenureFlag pretenure) {
  ASSERT(0 <= size);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateUninitializedFixedDoubleArray(size, pretenure),
      FixedDoubleArray);
}


Handle<ConstantPoolArray> Factory::NewConstantPoolArray(
    int number_of_int64_entries,
    int number_of_ptr_entries,
    int number_of_int32_entries) {
  ASSERT(number_of_int64_entries > 0 || number_of_ptr_entries > 0 ||
         number_of_int32_entries > 0);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateConstantPoolArray(number_of_int64_entries,
                                                   number_of_ptr_entries,
                                                   number_of_int32_entries),
      ConstantPoolArray);
}


Handle<NameDictionary> Factory::NewNameDictionary(int at_least_space_for) {
  ASSERT(0 <= at_least_space_for);
  CALL_HEAP_FUNCTION(isolate(),
                     NameDictionary::Allocate(isolate()->heap(),
                                              at_least_space_for),
                     NameDictionary);
}


Handle<SeededNumberDictionary> Factory::NewSeededNumberDictionary(
    int at_least_space_for) {
  ASSERT(0 <= at_least_space_for);
  CALL_HEAP_FUNCTION(isolate(),
                     SeededNumberDictionary::Allocate(isolate()->heap(),
                                                      at_least_space_for),
                     SeededNumberDictionary);
}


Handle<UnseededNumberDictionary> Factory::NewUnseededNumberDictionary(
    int at_least_space_for) {
  ASSERT(0 <= at_least_space_for);
  CALL_HEAP_FUNCTION(isolate(),
                     UnseededNumberDictionary::Allocate(isolate()->heap(),
                                                        at_least_space_for),
                     UnseededNumberDictionary);
}


Handle<ObjectHashSet> Factory::NewObjectHashSet(int at_least_space_for) {
  ASSERT(0 <= at_least_space_for);
  CALL_HEAP_FUNCTION(isolate(),
                     ObjectHashSet::Allocate(isolate()->heap(),
                                             at_least_space_for),
                     ObjectHashSet);
}


Handle<ObjectHashTable> Factory::NewObjectHashTable(int at_least_space_for) {
  ASSERT(0 <= at_least_space_for);
  CALL_HEAP_FUNCTION(isolate(),
                     ObjectHashTable::Allocate(isolate()->heap(),
                                               at_least_space_for),
                     ObjectHashTable);
}


Handle<WeakHashTable> Factory::NewWeakHashTable(int at_least_space_for) {
  ASSERT(0 <= at_least_space_for);
  CALL_HEAP_FUNCTION(
      isolate(),
      WeakHashTable::Allocate(isolate()->heap(),
                              at_least_space_for,
                              WeakHashTable::USE_DEFAULT_MINIMUM_CAPACITY,
                              TENURED),
      WeakHashTable);
}


Handle<DescriptorArray> Factory::NewDescriptorArray(int number_of_descriptors,
                                                    int slack) {
  ASSERT(0 <= number_of_descriptors);
  CALL_HEAP_FUNCTION(isolate(),
                     DescriptorArray::Allocate(
                         isolate(), number_of_descriptors, slack),
                     DescriptorArray);
}


Handle<DeoptimizationInputData> Factory::NewDeoptimizationInputData(
    int deopt_entry_count,
    PretenureFlag pretenure) {
  ASSERT(deopt_entry_count > 0);
  CALL_HEAP_FUNCTION(isolate(),
                     DeoptimizationInputData::Allocate(isolate(),
                                                       deopt_entry_count,
                                                       pretenure),
                     DeoptimizationInputData);
}


Handle<DeoptimizationOutputData> Factory::NewDeoptimizationOutputData(
    int deopt_entry_count,
    PretenureFlag pretenure) {
  ASSERT(deopt_entry_count > 0);
  CALL_HEAP_FUNCTION(isolate(),
                     DeoptimizationOutputData::Allocate(isolate(),
                                                        deopt_entry_count,
                                                        pretenure),
                     DeoptimizationOutputData);
}


Handle<AccessorPair> Factory::NewAccessorPair() {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateAccessorPair(),
                     AccessorPair);
}


Handle<TypeFeedbackInfo> Factory::NewTypeFeedbackInfo() {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateTypeFeedbackInfo(),
                     TypeFeedbackInfo);
}


// Internalized strings are created in the old generation (data space).
Handle<String> Factory::InternalizeUtf8String(Vector<const char> string) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->InternalizeUtf8String(string),
                     String);
}


// Internalized strings are created in the old generation (data space).
Handle<String> Factory::InternalizeString(Handle<String> string) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->InternalizeString(*string),
                     String);
}


Handle<String> Factory::InternalizeOneByteString(Vector<const uint8_t> string) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->InternalizeOneByteString(string),
                     String);
}


Handle<String> Factory::InternalizeOneByteString(
    Handle<SeqOneByteString> string, int from, int length) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->InternalizeOneByteString(
                         string, from, length),
                     String);
}


Handle<String> Factory::InternalizeTwoByteString(Vector<const uc16> string) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->InternalizeTwoByteString(string),
                     String);
}


Handle<String> Factory::NewStringFromOneByte(Vector<const uint8_t> string,
                                             PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateStringFromOneByte(string, pretenure),
      String);
}

Handle<String> Factory::NewStringFromUtf8(Vector<const char> string,
                                          PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateStringFromUtf8(string, pretenure),
      String);
}


Handle<String> Factory::NewStringFromTwoByte(Vector<const uc16> string,
                                             PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateStringFromTwoByte(string, pretenure),
      String);
}


Handle<SeqOneByteString> Factory::NewRawOneByteString(int length,
                                                  PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateRawOneByteString(length, pretenure),
      SeqOneByteString);
}


Handle<SeqTwoByteString> Factory::NewRawTwoByteString(int length,
                                                      PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateRawTwoByteString(length, pretenure),
      SeqTwoByteString);
}


Handle<String> Factory::NewConsString(Handle<String> first,
                                      Handle<String> second) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateConsString(*first, *second),
                     String);
}


template<typename SinkChar, typename StringType>
Handle<String> ConcatStringContent(Handle<StringType> result,
                                   Handle<String> first,
                                   Handle<String> second) {
  DisallowHeapAllocation pointer_stays_valid;
  SinkChar* sink = result->GetChars();
  String::WriteToFlat(*first, sink, 0, first->length());
  String::WriteToFlat(*second, sink + first->length(), 0, second->length());
  return result;
}


Handle<String> Factory::NewFlatConcatString(Handle<String> first,
                                            Handle<String> second) {
  int total_length = first->length() + second->length();
  if (first->IsOneByteRepresentationUnderneath() &&
      second->IsOneByteRepresentationUnderneath()) {
    return ConcatStringContent<uint8_t>(
        NewRawOneByteString(total_length), first, second);
  } else {
    return ConcatStringContent<uc16>(
        NewRawTwoByteString(total_length), first, second);
  }
}


Handle<String> Factory::NewSubString(Handle<String> str,
                                     int begin,
                                     int end) {
  CALL_HEAP_FUNCTION(isolate(),
                     str->SubString(begin, end),
                     String);
}


Handle<String> Factory::NewProperSubString(Handle<String> str,
                                           int begin,
                                           int end) {
  ASSERT(begin > 0 || end < str->length());
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateSubString(*str, begin, end),
                     String);
}


Handle<String> Factory::NewExternalStringFromAscii(
    const ExternalAsciiString::Resource* resource) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateExternalStringFromAscii(resource),
      String);
}


Handle<String> Factory::NewExternalStringFromTwoByte(
    const ExternalTwoByteString::Resource* resource) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateExternalStringFromTwoByte(resource),
      String);
}


Handle<Symbol> Factory::NewSymbol() {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateSymbol(),
      Symbol);
}


Handle<Context> Factory::NewNativeContext() {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateNativeContext(),
      Context);
}


Handle<Context> Factory::NewGlobalContext(Handle<JSFunction> function,
                                          Handle<ScopeInfo> scope_info) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateGlobalContext(*function, *scope_info),
      Context);
}


Handle<Context> Factory::NewModuleContext(Handle<ScopeInfo> scope_info) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateModuleContext(*scope_info),
      Context);
}


Handle<Context> Factory::NewFunctionContext(int length,
                                            Handle<JSFunction> function) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateFunctionContext(length, *function),
      Context);
}


Handle<Context> Factory::NewCatchContext(Handle<JSFunction> function,
                                         Handle<Context> previous,
                                         Handle<String> name,
                                         Handle<Object> thrown_object) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateCatchContext(*function,
                                              *previous,
                                              *name,
                                              *thrown_object),
      Context);
}


Handle<Context> Factory::NewWithContext(Handle<JSFunction> function,
                                        Handle<Context> previous,
                                        Handle<JSObject> extension) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateWithContext(*function, *previous, *extension),
      Context);
}


Handle<Context> Factory::NewBlockContext(Handle<JSFunction> function,
                                         Handle<Context> previous,
                                         Handle<ScopeInfo> scope_info) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateBlockContext(*function,
                                              *previous,
                                              *scope_info),
      Context);
}


Handle<Struct> Factory::NewStruct(InstanceType type) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateStruct(type),
      Struct);
}


Handle<DeclaredAccessorDescriptor> Factory::NewDeclaredAccessorDescriptor() {
  return Handle<DeclaredAccessorDescriptor>::cast(
      NewStruct(DECLARED_ACCESSOR_DESCRIPTOR_TYPE));
}


Handle<DeclaredAccessorInfo> Factory::NewDeclaredAccessorInfo() {
  Handle<DeclaredAccessorInfo> info =
      Handle<DeclaredAccessorInfo>::cast(
          NewStruct(DECLARED_ACCESSOR_INFO_TYPE));
  info->set_flag(0);  // Must clear the flag, it was initialized as undefined.
  return info;
}


Handle<ExecutableAccessorInfo> Factory::NewExecutableAccessorInfo() {
  Handle<ExecutableAccessorInfo> info =
      Handle<ExecutableAccessorInfo>::cast(
          NewStruct(EXECUTABLE_ACCESSOR_INFO_TYPE));
  info->set_flag(0);  // Must clear the flag, it was initialized as undefined.
  return info;
}


Handle<Script> Factory::NewScript(Handle<String> source) {
  // Generate id for this script.
  Heap* heap = isolate()->heap();
  int id = heap->last_script_id()->value() + 1;
  if (!Smi::IsValid(id) || id < 0) id = 1;
  heap->set_last_script_id(Smi::FromInt(id));

  // Create and initialize script object.
  Handle<Foreign> wrapper = NewForeign(0, TENURED);
  Handle<Script> script = Handle<Script>::cast(NewStruct(SCRIPT_TYPE));
  script->set_source(*source);
  script->set_name(heap->undefined_value());
  script->set_id(Smi::FromInt(id));
  script->set_line_offset(Smi::FromInt(0));
  script->set_column_offset(Smi::FromInt(0));
  script->set_data(heap->undefined_value());
  script->set_context_data(heap->undefined_value());
  script->set_type(Smi::FromInt(Script::TYPE_NORMAL));
  script->set_wrapper(*wrapper);
  script->set_line_ends(heap->undefined_value());
  script->set_eval_from_shared(heap->undefined_value());
  script->set_eval_from_instructions_offset(Smi::FromInt(0));
  script->set_flags(Smi::FromInt(0));

  return script;
}


Handle<Foreign> Factory::NewForeign(Address addr, PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateForeign(addr, pretenure),
                     Foreign);
}


Handle<Foreign> Factory::NewForeign(const AccessorDescriptor* desc) {
  return NewForeign((Address) desc, TENURED);
}


Handle<ByteArray> Factory::NewByteArray(int length, PretenureFlag pretenure) {
  ASSERT(0 <= length);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateByteArray(length, pretenure),
      ByteArray);
}


Handle<ExternalArray> Factory::NewExternalArray(int length,
                                                ExternalArrayType array_type,
                                                void* external_pointer,
                                                PretenureFlag pretenure) {
  ASSERT(0 <= length);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateExternalArray(length,
                                               array_type,
                                               external_pointer,
                                               pretenure),
      ExternalArray);
}


Handle<Cell> Factory::NewCell(Handle<Object> value) {
  AllowDeferredHandleDereference convert_to_cell;
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateCell(*value),
      Cell);
}


Handle<PropertyCell> Factory::NewPropertyCellWithHole() {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocatePropertyCell(),
      PropertyCell);
}


Handle<PropertyCell> Factory::NewPropertyCell(Handle<Object> value) {
  AllowDeferredHandleDereference convert_to_cell;
  Handle<PropertyCell> cell = NewPropertyCellWithHole();
  PropertyCell::SetValueInferType(cell, value);
  return cell;
}


Handle<AllocationSite> Factory::NewAllocationSite() {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateAllocationSite(),
      AllocationSite);
}


Handle<Map> Factory::NewMap(InstanceType type,
                            int instance_size,
                            ElementsKind elements_kind) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateMap(type, instance_size, elements_kind),
      Map);
}


Handle<JSObject> Factory::NewFunctionPrototype(Handle<JSFunction> function) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateFunctionPrototype(*function),
      JSObject);
}


Handle<Map> Factory::CopyWithPreallocatedFieldDescriptors(Handle<Map> src) {
  CALL_HEAP_FUNCTION(
      isolate(), src->CopyWithPreallocatedFieldDescriptors(), Map);
}


Handle<Map> Factory::CopyMap(Handle<Map> src,
                             int extra_inobject_properties) {
  Handle<Map> copy = CopyWithPreallocatedFieldDescriptors(src);
  // Check that we do not overflow the instance size when adding the
  // extra inobject properties.
  int instance_size_delta = extra_inobject_properties * kPointerSize;
  int max_instance_size_delta =
      JSObject::kMaxInstanceSize - copy->instance_size();
  if (instance_size_delta > max_instance_size_delta) {
    // If the instance size overflows, we allocate as many properties
    // as we can as inobject properties.
    instance_size_delta = max_instance_size_delta;
    extra_inobject_properties = max_instance_size_delta >> kPointerSizeLog2;
  }
  // Adjust the map with the extra inobject properties.
  int inobject_properties =
      copy->inobject_properties() + extra_inobject_properties;
  copy->set_inobject_properties(inobject_properties);
  copy->set_unused_property_fields(inobject_properties);
  copy->set_instance_size(copy->instance_size() + instance_size_delta);
  copy->set_visitor_id(StaticVisitorBase::GetVisitorId(*copy));
  return copy;
}


Handle<Map> Factory::CopyMap(Handle<Map> src) {
  CALL_HEAP_FUNCTION(isolate(), src->Copy(), Map);
}


Handle<Map> Factory::GetElementsTransitionMap(
    Handle<JSObject> src,
    ElementsKind elements_kind) {
  Isolate* i = isolate();
  CALL_HEAP_FUNCTION(i,
                     src->GetElementsTransitionMap(i, elements_kind),
                     Map);
}


Handle<FixedArray> Factory::CopyFixedArray(Handle<FixedArray> array) {
  CALL_HEAP_FUNCTION(isolate(), array->Copy(), FixedArray);
}


Handle<FixedArray> Factory::CopySizeFixedArray(Handle<FixedArray> array,
                                               int new_length,
                                               PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(isolate(),
                     array->CopySize(new_length, pretenure),
                     FixedArray);
}


Handle<FixedDoubleArray> Factory::CopyFixedDoubleArray(
    Handle<FixedDoubleArray> array) {
  CALL_HEAP_FUNCTION(isolate(), array->Copy(), FixedDoubleArray);
}


Handle<ConstantPoolArray> Factory::CopyConstantPoolArray(
    Handle<ConstantPoolArray> array) {
  CALL_HEAP_FUNCTION(isolate(), array->Copy(), ConstantPoolArray);
}


Handle<JSFunction> Factory::BaseNewFunctionFromSharedFunctionInfo(
    Handle<SharedFunctionInfo> function_info,
    Handle<Map> function_map,
    PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateFunction(*function_map,
                                          *function_info,
                                          isolate()->heap()->the_hole_value(),
                                          pretenure),
                     JSFunction);
}


static Handle<Map> MapForNewFunction(Isolate *isolate,
                                     Handle<SharedFunctionInfo> function_info) {
  Context *context = isolate->context()->native_context();
  int map_index = Context::FunctionMapIndex(function_info->language_mode(),
                                            function_info->is_generator());
  return Handle<Map>(Map::cast(context->get(map_index)));
}


Handle<JSFunction> Factory::NewFunctionFromSharedFunctionInfo(
    Handle<SharedFunctionInfo> function_info,
    Handle<Context> context,
    PretenureFlag pretenure) {
  Handle<JSFunction> result = BaseNewFunctionFromSharedFunctionInfo(
      function_info,
      MapForNewFunction(isolate(), function_info),
      pretenure);

  if (function_info->ic_age() != isolate()->heap()->global_ic_age()) {
    function_info->ResetForNewContext(isolate()->heap()->global_ic_age());
  }

  result->set_context(*context);

  int index = function_info->SearchOptimizedCodeMap(context->native_context());
  if (!function_info->bound() && index < 0) {
    int number_of_literals = function_info->num_literals();
    Handle<FixedArray> literals = NewFixedArray(number_of_literals, pretenure);
    if (number_of_literals > 0) {
      // Store the native context in the literals array prefix. This
      // context will be used when creating object, regexp and array
      // literals in this function.
      literals->set(JSFunction::kLiteralNativeContextIndex,
                    context->native_context());
    }
    result->set_literals(*literals);
  }

  if (index > 0) {
    // Caching of optimized code enabled and optimized code found.
    function_info->InstallFromOptimizedCodeMap(*result, index);
    return result;
  }

  if (isolate()->use_crankshaft() &&
      FLAG_always_opt &&
      result->is_compiled() &&
      !function_info->is_toplevel() &&
      function_info->allows_lazy_compilation() &&
      !function_info->optimization_disabled() &&
      !isolate()->DebuggerHasBreakPoints()) {
    result->MarkForLazyRecompilation();
  }
  return result;
}


Handle<Object> Factory::NewNumber(double value,
                                  PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->NumberFromDouble(value, pretenure), Object);
}


Handle<Object> Factory::NewNumberFromInt(int32_t value,
                                         PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->NumberFromInt32(value, pretenure), Object);
}


Handle<Object> Factory::NewNumberFromUint(uint32_t value,
                                         PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->NumberFromUint32(value, pretenure), Object);
}


Handle<HeapNumber> Factory::NewHeapNumber(double value,
                                          PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateHeapNumber(value, pretenure), HeapNumber);
}


Handle<JSObject> Factory::NewNeanderObject() {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSObjectFromMap(
          isolate()->heap()->neander_map()),
      JSObject);
}


Handle<Object> Factory::NewTypeError(const char* message,
                                     Vector< Handle<Object> > args) {
  return NewError("MakeTypeError", message, args);
}


Handle<Object> Factory::NewTypeError(Handle<String> message) {
  return NewError("$TypeError", message);
}


Handle<Object> Factory::NewRangeError(const char* message,
                                      Vector< Handle<Object> > args) {
  return NewError("MakeRangeError", message, args);
}


Handle<Object> Factory::NewRangeError(Handle<String> message) {
  return NewError("$RangeError", message);
}


Handle<Object> Factory::NewSyntaxError(const char* message,
                                       Handle<JSArray> args) {
  return NewError("MakeSyntaxError", message, args);
}


Handle<Object> Factory::NewSyntaxError(Handle<String> message) {
  return NewError("$SyntaxError", message);
}


Handle<Object> Factory::NewReferenceError(const char* message,
                                          Vector< Handle<Object> > args) {
  return NewError("MakeReferenceError", message, args);
}


Handle<Object> Factory::NewReferenceError(Handle<String> message) {
  return NewError("$ReferenceError", message);
}


Handle<Object> Factory::NewError(const char* maker,
                                 const char* message,
                                 Vector< Handle<Object> > args) {
  // Instantiate a closeable HandleScope for EscapeFrom.
  v8::HandleScope scope(reinterpret_cast<v8::Isolate*>(isolate()));
  Handle<FixedArray> array = NewFixedArray(args.length());
  for (int i = 0; i < args.length(); i++) {
    array->set(i, *args[i]);
  }
  Handle<JSArray> object = NewJSArrayWithElements(array);
  Handle<Object> result = NewError(maker, message, object);
  return result.EscapeFrom(&scope);
}


Handle<Object> Factory::NewEvalError(const char* message,
                                     Vector< Handle<Object> > args) {
  return NewError("MakeEvalError", message, args);
}


Handle<Object> Factory::NewError(const char* message,
                                 Vector< Handle<Object> > args) {
  return NewError("MakeError", message, args);
}


Handle<String> Factory::EmergencyNewError(const char* message,
                                          Handle<JSArray> args) {
  const int kBufferSize = 1000;
  char buffer[kBufferSize];
  size_t space = kBufferSize;
  char* p = &buffer[0];

  Vector<char> v(buffer, kBufferSize);
  OS::StrNCpy(v, message, space);
  space -= Min(space, strlen(message));
  p = &buffer[kBufferSize] - space;

  for (unsigned i = 0; i < ARRAY_SIZE(args); i++) {
    if (space > 0) {
      *p++ = ' ';
      space--;
      if (space > 0) {
        MaybeObject* maybe_arg = args->GetElement(isolate(), i);
        Handle<String> arg_str(reinterpret_cast<String*>(maybe_arg));
        const char* arg = *arg_str->ToCString();
        Vector<char> v2(p, static_cast<int>(space));
        OS::StrNCpy(v2, arg, space);
        space -= Min(space, strlen(arg));
        p = &buffer[kBufferSize] - space;
      }
    }
  }
  if (space > 0) {
    *p = '\0';
  } else {
    buffer[kBufferSize - 1] = '\0';
  }
  Handle<String> error_string = NewStringFromUtf8(CStrVector(buffer), TENURED);
  return error_string;
}


Handle<Object> Factory::NewError(const char* maker,
                                 const char* message,
                                 Handle<JSArray> args) {
  Handle<String> make_str = InternalizeUtf8String(maker);
  Handle<Object> fun_obj(
      isolate()->js_builtins_object()->GetPropertyNoExceptionThrown(*make_str),
      isolate());
  // If the builtins haven't been properly configured yet this error
  // constructor may not have been defined.  Bail out.
  if (!fun_obj->IsJSFunction()) {
    return EmergencyNewError(message, args);
  }
  Handle<JSFunction> fun = Handle<JSFunction>::cast(fun_obj);
  Handle<Object> message_obj = InternalizeUtf8String(message);
  Handle<Object> argv[] = { message_obj, args };

  // Invoke the JavaScript factory method. If an exception is thrown while
  // running the factory method, use the exception as the result.
  bool caught_exception;
  Handle<Object> result = Execution::TryCall(fun,
                                             isolate()->js_builtins_object(),
                                             ARRAY_SIZE(argv),
                                             argv,
                                             &caught_exception);
  return result;
}


Handle<Object> Factory::NewError(Handle<String> message) {
  return NewError("$Error", message);
}


Handle<Object> Factory::NewError(const char* constructor,
                                 Handle<String> message) {
  Handle<String> constr = InternalizeUtf8String(constructor);
  Handle<JSFunction> fun = Handle<JSFunction>(
      JSFunction::cast(isolate()->js_builtins_object()->
                       GetPropertyNoExceptionThrown(*constr)));
  Handle<Object> argv[] = { message };

  // Invoke the JavaScript factory method. If an exception is thrown while
  // running the factory method, use the exception as the result.
  bool caught_exception;
  Handle<Object> result = Execution::TryCall(fun,
                                             isolate()->js_builtins_object(),
                                             ARRAY_SIZE(argv),
                                             argv,
                                             &caught_exception);
  return result;
}


Handle<JSFunction> Factory::NewFunction(Handle<String> name,
                                        InstanceType type,
                                        int instance_size,
                                        Handle<Code> code,
                                        bool force_initial_map) {
  // Allocate the function
  Handle<JSFunction> function = NewFunction(name, the_hole_value());

  // Set up the code pointer in both the shared function info and in
  // the function itself.
  function->shared()->set_code(*code);
  function->set_code(*code);

  if (force_initial_map ||
      type != JS_OBJECT_TYPE ||
      instance_size != JSObject::kHeaderSize) {
    Handle<Map> initial_map = NewMap(type, instance_size);
    Handle<JSObject> prototype = NewFunctionPrototype(function);
    initial_map->set_prototype(*prototype);
    function->set_initial_map(*initial_map);
    initial_map->set_constructor(*function);
  } else {
    ASSERT(!function->has_initial_map());
    ASSERT(!function->has_prototype());
  }

  return function;
}


Handle<JSFunction> Factory::NewFunctionWithPrototype(Handle<String> name,
                                                     InstanceType type,
                                                     int instance_size,
                                                     Handle<JSObject> prototype,
                                                     Handle<Code> code,
                                                     bool force_initial_map) {
  // Allocate the function.
  Handle<JSFunction> function = NewFunction(name, prototype);

  // Set up the code pointer in both the shared function info and in
  // the function itself.
  function->shared()->set_code(*code);
  function->set_code(*code);

  if (force_initial_map ||
      type != JS_OBJECT_TYPE ||
      instance_size != JSObject::kHeaderSize) {
    Handle<Map> initial_map = NewMap(type,
                                     instance_size,
                                     GetInitialFastElementsKind());
    function->set_initial_map(*initial_map);
    initial_map->set_constructor(*function);
  }

  JSFunction::SetPrototype(function, prototype);
  return function;
}


Handle<JSFunction> Factory::NewFunctionWithoutPrototype(Handle<String> name,
                                                        Handle<Code> code) {
  Handle<JSFunction> function = NewFunctionWithoutPrototype(name,
                                                            CLASSIC_MODE);
  function->shared()->set_code(*code);
  function->set_code(*code);
  ASSERT(!function->has_initial_map());
  ASSERT(!function->has_prototype());
  return function;
}


Handle<ScopeInfo> Factory::NewScopeInfo(int length) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateScopeInfo(length),
      ScopeInfo);
}


Handle<JSObject> Factory::NewExternal(void* value) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateExternal(value),
                     JSObject);
}


Handle<Code> Factory::NewCode(const CodeDesc& desc,
                              Code::Flags flags,
                              Handle<Object> self_ref,
                              bool immovable,
                              bool crankshafted,
                              int prologue_offset) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->CreateCode(
                         desc, flags, self_ref, immovable, crankshafted,
                         prologue_offset),
                     Code);
}


Handle<Code> Factory::CopyCode(Handle<Code> code) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->CopyCode(*code),
                     Code);
}


Handle<Code> Factory::CopyCode(Handle<Code> code, Vector<byte> reloc_info) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->CopyCode(*code, reloc_info),
                     Code);
}


Handle<String> Factory::InternalizedStringFromString(Handle<String> value) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->InternalizeString(*value), String);
}


Handle<JSObject> Factory::NewJSObject(Handle<JSFunction> constructor,
                                      PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSObject(*constructor, pretenure), JSObject);
}


Handle<JSModule> Factory::NewJSModule(Handle<Context> context,
                                      Handle<ScopeInfo> scope_info) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSModule(*context, *scope_info), JSModule);
}


// TODO(mstarzinger): Temporary wrapper until handlified.
static Handle<NameDictionary> NameDictionaryAdd(Handle<NameDictionary> dict,
                                                Handle<Name> name,
                                                Handle<Object> value,
                                                PropertyDetails details) {
  CALL_HEAP_FUNCTION(dict->GetIsolate(),
                     dict->Add(*name, *value, details),
                     NameDictionary);
}


static Handle<GlobalObject> NewGlobalObjectFromMap(Isolate* isolate,
                                                   Handle<Map> map) {
  CALL_HEAP_FUNCTION(isolate,
                     isolate->heap()->Allocate(*map, OLD_POINTER_SPACE),
                     GlobalObject);
}


Handle<GlobalObject> Factory::NewGlobalObject(Handle<JSFunction> constructor) {
  ASSERT(constructor->has_initial_map());
  Handle<Map> map(constructor->initial_map());
  ASSERT(map->is_dictionary_map());

  // Make sure no field properties are described in the initial map.
  // This guarantees us that normalizing the properties does not
  // require us to change property values to PropertyCells.
  ASSERT(map->NextFreePropertyIndex() == 0);

  // Make sure we don't have a ton of pre-allocated slots in the
  // global objects. They will be unused once we normalize the object.
  ASSERT(map->unused_property_fields() == 0);
  ASSERT(map->inobject_properties() == 0);

  // Initial size of the backing store to avoid resize of the storage during
  // bootstrapping. The size differs between the JS global object ad the
  // builtins object.
  int initial_size = map->instance_type() == JS_GLOBAL_OBJECT_TYPE ? 64 : 512;

  // Allocate a dictionary object for backing storage.
  int at_least_space_for = map->NumberOfOwnDescriptors() * 2 + initial_size;
  Handle<NameDictionary> dictionary = NewNameDictionary(at_least_space_for);

  // The global object might be created from an object template with accessors.
  // Fill these accessors into the dictionary.
  Handle<DescriptorArray> descs(map->instance_descriptors());
  for (int i = 0; i < map->NumberOfOwnDescriptors(); i++) {
    PropertyDetails details = descs->GetDetails(i);
    ASSERT(details.type() == CALLBACKS);  // Only accessors are expected.
    PropertyDetails d = PropertyDetails(details.attributes(), CALLBACKS, i + 1);
    Handle<Name> name(descs->GetKey(i));
    Handle<Object> value(descs->GetCallbacksObject(i), isolate());
    Handle<PropertyCell> cell = NewPropertyCell(value);
    NameDictionaryAdd(dictionary, name, cell, d);
  }

  // Allocate the global object and initialize it with the backing store.
  Handle<GlobalObject> global = NewGlobalObjectFromMap(isolate(), map);
  isolate()->heap()->InitializeJSObjectFromMap(*global, *dictionary, *map);

  // Create a new map for the global object.
  Handle<Map> new_map = Map::CopyDropDescriptors(map);
  new_map->set_dictionary_map(true);

  // Set up the global object as a normalized object.
  global->set_map(*new_map);
  global->set_properties(*dictionary);

  // Make sure result is a global object with properties in dictionary.
  ASSERT(global->IsGlobalObject() && !global->HasFastProperties());
  return global;
}


Handle<JSObject> Factory::NewJSObjectFromMap(Handle<Map> map,
                                             PretenureFlag pretenure,
                                             bool alloc_props) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSObjectFromMap(*map, pretenure, alloc_props),
      JSObject);
}


Handle<JSArray> Factory::NewJSArray(int capacity,
                                    ElementsKind elements_kind,
                                    PretenureFlag pretenure) {
  if (capacity != 0) {
    elements_kind = GetHoleyElementsKind(elements_kind);
  }
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateJSArrayAndStorage(
                         elements_kind,
                         0,
                         capacity,
                         INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE,
                         pretenure),
                     JSArray);
}


Handle<JSArray> Factory::NewJSArrayWithElements(Handle<FixedArrayBase> elements,
                                                ElementsKind elements_kind,
                                                PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSArrayWithElements(*elements,
                                                     elements_kind,
                                                     elements->length(),
                                                     pretenure),
      JSArray);
}


void Factory::SetElementsCapacityAndLength(Handle<JSArray> array,
                                           int capacity,
                                           int length) {
  ElementsAccessor* accessor = array->GetElementsAccessor();
  CALL_HEAP_FUNCTION_VOID(
      isolate(),
      accessor->SetCapacityAndLength(*array, capacity, length));
}


void Factory::SetContent(Handle<JSArray> array,
                         Handle<FixedArrayBase> elements) {
  CALL_HEAP_FUNCTION_VOID(
      isolate(),
      array->SetContent(*elements));
}


Handle<JSArrayBuffer> Factory::NewJSArrayBuffer() {
  Handle<JSFunction> array_buffer_fun(
      isolate()->context()->native_context()->array_buffer_fun());
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSObject(*array_buffer_fun),
      JSArrayBuffer);
}


Handle<JSDataView> Factory::NewJSDataView() {
  Handle<JSFunction> data_view_fun(
      isolate()->context()->native_context()->data_view_fun());
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSObject(*data_view_fun),
      JSDataView);
}


static JSFunction* GetTypedArrayFun(ExternalArrayType type,
                                    Isolate* isolate) {
  Context* native_context = isolate->context()->native_context();
  switch (type) {
    case kExternalUnsignedByteArray:
      return native_context->uint8_array_fun();

    case kExternalByteArray:
      return native_context->int8_array_fun();

    case kExternalUnsignedShortArray:
      return native_context->uint16_array_fun();

    case kExternalShortArray:
      return native_context->int16_array_fun();

    case kExternalUnsignedIntArray:
      return native_context->uint32_array_fun();

    case kExternalIntArray:
      return native_context->int32_array_fun();

    case kExternalFloatArray:
      return native_context->float_array_fun();

    case kExternalDoubleArray:
      return native_context->double_array_fun();

    case kExternalPixelArray:
      return native_context->uint8c_array_fun();

    default:
      UNREACHABLE();
      return NULL;
  }
}


Handle<JSTypedArray> Factory::NewJSTypedArray(ExternalArrayType type) {
  Handle<JSFunction> typed_array_fun_handle(GetTypedArrayFun(type, isolate()));

  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSObject(*typed_array_fun_handle),
      JSTypedArray);
}


Handle<JSProxy> Factory::NewJSProxy(Handle<Object> handler,
                                    Handle<Object> prototype) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateJSProxy(*handler, *prototype),
      JSProxy);
}


void Factory::BecomeJSObject(Handle<JSReceiver> object) {
  CALL_HEAP_FUNCTION_VOID(
      isolate(),
      isolate()->heap()->ReinitializeJSReceiver(
          *object, JS_OBJECT_TYPE, JSObject::kHeaderSize));
}


void Factory::BecomeJSFunction(Handle<JSReceiver> object) {
  CALL_HEAP_FUNCTION_VOID(
      isolate(),
      isolate()->heap()->ReinitializeJSReceiver(
          *object, JS_FUNCTION_TYPE, JSFunction::kSize));
}


Handle<SharedFunctionInfo> Factory::NewSharedFunctionInfo(
    Handle<String> name,
    int number_of_literals,
    bool is_generator,
    Handle<Code> code,
    Handle<ScopeInfo> scope_info) {
  Handle<SharedFunctionInfo> shared = NewSharedFunctionInfo(name);
  shared->set_code(*code);
  shared->set_scope_info(*scope_info);
  int literals_array_size = number_of_literals;
  // If the function contains object, regexp or array literals,
  // allocate extra space for a literals array prefix containing the
  // context.
  if (number_of_literals > 0) {
    literals_array_size += JSFunction::kLiteralsPrefixSize;
  }
  shared->set_num_literals(literals_array_size);
  if (is_generator) {
    shared->set_instance_class_name(isolate()->heap()->Generator_string());
    shared->DisableOptimization(kGenerator);
  }
  return shared;
}


Handle<JSMessageObject> Factory::NewJSMessageObject(
    Handle<String> type,
    Handle<JSArray> arguments,
    int start_position,
    int end_position,
    Handle<Object> script,
    Handle<Object> stack_trace,
    Handle<Object> stack_frames) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateJSMessageObject(*type,
                         *arguments,
                         start_position,
                         end_position,
                         *script,
                         *stack_trace,
                         *stack_frames),
                     JSMessageObject);
}


Handle<SharedFunctionInfo> Factory::NewSharedFunctionInfo(Handle<String> name) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateSharedFunctionInfo(*name),
                     SharedFunctionInfo);
}


Handle<String> Factory::NumberToString(Handle<Object> number) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->NumberToString(*number), String);
}


Handle<String> Factory::Uint32ToString(uint32_t value) {
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->Uint32ToString(value), String);
}


Handle<SeededNumberDictionary> Factory::DictionaryAtNumberPut(
    Handle<SeededNumberDictionary> dictionary,
    uint32_t key,
    Handle<Object> value) {
  CALL_HEAP_FUNCTION(isolate(),
                     dictionary->AtNumberPut(key, *value),
                     SeededNumberDictionary);
}


Handle<UnseededNumberDictionary> Factory::DictionaryAtNumberPut(
    Handle<UnseededNumberDictionary> dictionary,
    uint32_t key,
    Handle<Object> value) {
  CALL_HEAP_FUNCTION(isolate(),
                     dictionary->AtNumberPut(key, *value),
                     UnseededNumberDictionary);
}


Handle<JSFunction> Factory::NewFunctionHelper(Handle<String> name,
                                              Handle<Object> prototype) {
  Handle<SharedFunctionInfo> function_share = NewSharedFunctionInfo(name);
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateFunction(*isolate()->function_map(),
                                          *function_share,
                                          *prototype),
      JSFunction);
}


Handle<JSFunction> Factory::NewFunction(Handle<String> name,
                                        Handle<Object> prototype) {
  Handle<JSFunction> fun = NewFunctionHelper(name, prototype);
  fun->set_context(isolate()->context()->native_context());
  return fun;
}


Handle<JSFunction> Factory::NewFunctionWithoutPrototypeHelper(
    Handle<String> name,
    LanguageMode language_mode) {
  Handle<SharedFunctionInfo> function_share = NewSharedFunctionInfo(name);
  Handle<Map> map = (language_mode == CLASSIC_MODE)
      ? isolate()->function_without_prototype_map()
      : isolate()->strict_mode_function_without_prototype_map();
  CALL_HEAP_FUNCTION(isolate(),
                     isolate()->heap()->AllocateFunction(
                         *map,
                         *function_share,
                         *the_hole_value()),
                     JSFunction);
}


Handle<JSFunction> Factory::NewFunctionWithoutPrototype(
    Handle<String> name,
    LanguageMode language_mode) {
  Handle<JSFunction> fun =
      NewFunctionWithoutPrototypeHelper(name, language_mode);
  fun->set_context(isolate()->context()->native_context());
  return fun;
}


Handle<Object> Factory::ToObject(Handle<Object> object) {
  CALL_HEAP_FUNCTION(isolate(), object->ToObject(isolate()), Object);
}


Handle<Object> Factory::ToObject(Handle<Object> object,
                                 Handle<Context> native_context) {
  CALL_HEAP_FUNCTION(isolate(), object->ToObject(*native_context), Object);
}


#ifdef ENABLE_DEBUGGER_SUPPORT
Handle<DebugInfo> Factory::NewDebugInfo(Handle<SharedFunctionInfo> shared) {
  // Get the original code of the function.
  Handle<Code> code(shared->code());

  // Create a copy of the code before allocating the debug info object to avoid
  // allocation while setting up the debug info object.
  Handle<Code> original_code(*Factory::CopyCode(code));

  // Allocate initial fixed array for active break points before allocating the
  // debug info object to avoid allocation while setting up the debug info
  // object.
  Handle<FixedArray> break_points(
      NewFixedArray(Debug::kEstimatedNofBreakPointsInFunction));

  // Create and set up the debug info object. Debug info contains function, a
  // copy of the original code, the executing code and initial fixed array for
  // active break points.
  Handle<DebugInfo> debug_info =
      Handle<DebugInfo>::cast(NewStruct(DEBUG_INFO_TYPE));
  debug_info->set_shared(*shared);
  debug_info->set_original_code(*original_code);
  debug_info->set_code(*code);
  debug_info->set_break_points(*break_points);

  // Link debug info to function.
  shared->set_debug_info(*debug_info);

  return debug_info;
}
#endif


Handle<JSObject> Factory::NewArgumentsObject(Handle<Object> callee,
                                             int length) {
  CALL_HEAP_FUNCTION(
      isolate(),
      isolate()->heap()->AllocateArgumentsObject(*callee, length), JSObject);
}


Handle<JSFunction> Factory::CreateApiFunction(
    Handle<FunctionTemplateInfo> obj, ApiInstanceType instance_type) {
  Handle<Code> code = isolate()->builtins()->HandleApiCall();
  Handle<Code> construct_stub = isolate()->builtins()->JSConstructStubApi();

  int internal_field_count = 0;
  if (!obj->instance_template()->IsUndefined()) {
    Handle<ObjectTemplateInfo> instance_template =
        Handle<ObjectTemplateInfo>(
            ObjectTemplateInfo::cast(obj->instance_template()));
    internal_field_count =
        Smi::cast(instance_template->internal_field_count())->value();
  }

  // TODO(svenpanne) Kill ApiInstanceType and refactor things by generalizing
  // JSObject::GetHeaderSize.
  int instance_size = kPointerSize * internal_field_count;
  InstanceType type;
  switch (instance_type) {
    case JavaScriptObject:
      type = JS_OBJECT_TYPE;
      instance_size += JSObject::kHeaderSize;
      break;
    case InnerGlobalObject:
      type = JS_GLOBAL_OBJECT_TYPE;
      instance_size += JSGlobalObject::kSize;
      break;
    case OuterGlobalObject:
      type = JS_GLOBAL_PROXY_TYPE;
      instance_size += JSGlobalProxy::kSize;
      break;
    default:
      UNREACHABLE();
      type = JS_OBJECT_TYPE;  // Keep the compiler happy.
      break;
  }

  Handle<JSFunction> result =
      NewFunction(Factory::empty_string(),
                  type,
                  instance_size,
                  code,
                  true);

  // Set length.
  result->shared()->set_length(obj->length());

  // Set class name.
  Handle<Object> class_name = Handle<Object>(obj->class_name(), isolate());
  if (class_name->IsString()) {
    result->shared()->set_instance_class_name(*class_name);
    result->shared()->set_name(*class_name);
  }

  Handle<Map> map = Handle<Map>(result->initial_map());

  // Mark as undetectable if needed.
  if (obj->undetectable()) {
    map->set_is_undetectable();
  }

  // Mark as hidden for the __proto__ accessor if needed.
  if (obj->hidden_prototype()) {
    map->set_is_hidden_prototype();
  }

  // Mark as needs_access_check if needed.
  if (obj->needs_access_check()) {
    map->set_is_access_check_needed(true);
  }

  // Set interceptor information in the map.
  if (!obj->named_property_handler()->IsUndefined()) {
    map->set_has_named_interceptor();
  }
  if (!obj->indexed_property_handler()->IsUndefined()) {
    map->set_has_indexed_interceptor();
  }

  // Set instance call-as-function information in the map.
  if (!obj->instance_call_handler()->IsUndefined()) {
    map->set_has_instance_call_handler();
  }

  result->shared()->set_function_data(*obj);
  result->shared()->set_construct_stub(*construct_stub);
  result->shared()->DontAdaptArguments();

  // Recursively copy parent instance templates' accessors,
  // 'data' may be modified.
  int max_number_of_additional_properties = 0;
  int max_number_of_static_properties = 0;
  FunctionTemplateInfo* info = *obj;
  while (true) {
    if (!info->instance_template()->IsUndefined()) {
      Object* props =
          ObjectTemplateInfo::cast(
              info->instance_template())->property_accessors();
      if (!props->IsUndefined()) {
        Handle<Object> props_handle(props, isolate());
        NeanderArray props_array(props_handle);
        max_number_of_additional_properties += props_array.length();
      }
    }
    if (!info->property_accessors()->IsUndefined()) {
      Object* props = info->property_accessors();
      if (!props->IsUndefined()) {
        Handle<Object> props_handle(props, isolate());
        NeanderArray props_array(props_handle);
        max_number_of_static_properties += props_array.length();
      }
    }
    Object* parent = info->parent_template();
    if (parent->IsUndefined()) break;
    info = FunctionTemplateInfo::cast(parent);
  }

  Map::EnsureDescriptorSlack(map, max_number_of_additional_properties);

  // Use a temporary FixedArray to acculumate static accessors
  int valid_descriptors = 0;
  Handle<FixedArray> array;
  if (max_number_of_static_properties > 0) {
    array = NewFixedArray(max_number_of_static_properties);
  }

  while (true) {
    // Install instance descriptors
    if (!obj->instance_template()->IsUndefined()) {
      Handle<ObjectTemplateInfo> instance =
          Handle<ObjectTemplateInfo>(
              ObjectTemplateInfo::cast(obj->instance_template()), isolate());
      Handle<Object> props = Handle<Object>(instance->property_accessors(),
                                            isolate());
      if (!props->IsUndefined()) {
        Map::AppendCallbackDescriptors(map, props);
      }
    }
    // Accumulate static accessors
    if (!obj->property_accessors()->IsUndefined()) {
      Handle<Object> props = Handle<Object>(obj->property_accessors(),
                                            isolate());
      valid_descriptors =
          AccessorInfo::AppendUnique(props, array, valid_descriptors);
    }
    // Climb parent chain
    Handle<Object> parent = Handle<Object>(obj->parent_template(), isolate());
    if (parent->IsUndefined()) break;
    obj = Handle<FunctionTemplateInfo>::cast(parent);
  }

  // Install accumulated static accessors
  for (int i = 0; i < valid_descriptors; i++) {
    Handle<AccessorInfo> accessor(AccessorInfo::cast(array->get(i)));
    JSObject::SetAccessor(result, accessor);
  }

  ASSERT(result->shared()->IsApiFunction());
  return result;
}


Handle<MapCache> Factory::NewMapCache(int at_least_space_for) {
  CALL_HEAP_FUNCTION(isolate(),
                     MapCache::Allocate(isolate()->heap(),
                                        at_least_space_for),
                     MapCache);
}


MUST_USE_RESULT static MaybeObject* UpdateMapCacheWith(Context* context,
                                                       FixedArray* keys,
                                                       Map* map) {
  Object* result;
  { MaybeObject* maybe_result =
        MapCache::cast(context->map_cache())->Put(keys, map);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  context->set_map_cache(MapCache::cast(result));
  return result;
}


Handle<MapCache> Factory::AddToMapCache(Handle<Context> context,
                                        Handle<FixedArray> keys,
                                        Handle<Map> map) {
  CALL_HEAP_FUNCTION(isolate(),
                     UpdateMapCacheWith(*context, *keys, *map), MapCache);
}


Handle<Map> Factory::ObjectLiteralMapFromCache(Handle<Context> context,
                                               Handle<FixedArray> keys) {
  if (context->map_cache()->IsUndefined()) {
    // Allocate the new map cache for the native context.
    Handle<MapCache> new_cache = NewMapCache(24);
    context->set_map_cache(*new_cache);
  }
  // Check to see whether there is a matching element in the cache.
  Handle<MapCache> cache =
      Handle<MapCache>(MapCache::cast(context->map_cache()));
  Handle<Object> result = Handle<Object>(cache->Lookup(*keys), isolate());
  if (result->IsMap()) return Handle<Map>::cast(result);
  // Create a new map and add it to the cache.
  Handle<Map> map =
      CopyMap(Handle<Map>(context->object_function()->initial_map()),
              keys->length());
  AddToMapCache(context, keys, map);
  return Handle<Map>(map);
}


void Factory::SetRegExpAtomData(Handle<JSRegExp> regexp,
                                JSRegExp::Type type,
                                Handle<String> source,
                                JSRegExp::Flags flags,
                                Handle<Object> data) {
  Handle<FixedArray> store = NewFixedArray(JSRegExp::kAtomDataSize);

  store->set(JSRegExp::kTagIndex, Smi::FromInt(type));
  store->set(JSRegExp::kSourceIndex, *source);
  store->set(JSRegExp::kFlagsIndex, Smi::FromInt(flags.value()));
  store->set(JSRegExp::kAtomPatternIndex, *data);
  regexp->set_data(*store);
}

void Factory::SetRegExpIrregexpData(Handle<JSRegExp> regexp,
                                    JSRegExp::Type type,
                                    Handle<String> source,
                                    JSRegExp::Flags flags,
                                    int capture_count) {
  Handle<FixedArray> store = NewFixedArray(JSRegExp::kIrregexpDataSize);
  Smi* uninitialized = Smi::FromInt(JSRegExp::kUninitializedValue);
  store->set(JSRegExp::kTagIndex, Smi::FromInt(type));
  store->set(JSRegExp::kSourceIndex, *source);
  store->set(JSRegExp::kFlagsIndex, Smi::FromInt(flags.value()));
  store->set(JSRegExp::kIrregexpASCIICodeIndex, uninitialized);
  store->set(JSRegExp::kIrregexpUC16CodeIndex, uninitialized);
  store->set(JSRegExp::kIrregexpASCIICodeSavedIndex, uninitialized);
  store->set(JSRegExp::kIrregexpUC16CodeSavedIndex, uninitialized);
  store->set(JSRegExp::kIrregexpMaxRegisterCountIndex, Smi::FromInt(0));
  store->set(JSRegExp::kIrregexpCaptureCountIndex,
             Smi::FromInt(capture_count));
  regexp->set_data(*store);
}



void Factory::ConfigureInstance(Handle<FunctionTemplateInfo> desc,
                                Handle<JSObject> instance,
                                bool* pending_exception) {
  // Configure the instance by adding the properties specified by the
  // instance template.
  Handle<Object> instance_template(desc->instance_template(), isolate());
  if (!instance_template->IsUndefined()) {
    Execution::ConfigureInstance(isolate(),
                                 instance,
                                 instance_template,
                                 pending_exception);
  } else {
    *pending_exception = false;
  }
}


Handle<Object> Factory::GlobalConstantFor(Handle<String> name) {
  Heap* h = isolate()->heap();
  if (name->Equals(h->undefined_string())) return undefined_value();
  if (name->Equals(h->nan_string())) return nan_value();
  if (name->Equals(h->infinity_string())) return infinity_value();
  return Handle<Object>::null();
}


Handle<Object> Factory::ToBoolean(bool value) {
  return value ? true_value() : false_value();
}


} }  // namespace v8::internal
