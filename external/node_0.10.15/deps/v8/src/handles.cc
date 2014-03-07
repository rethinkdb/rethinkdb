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

#include "v8.h"

#include "accessors.h"
#include "api.h"
#include "arguments.h"
#include "bootstrapper.h"
#include "compiler.h"
#include "debug.h"
#include "execution.h"
#include "global-handles.h"
#include "natives.h"
#include "runtime.h"
#include "string-search.h"
#include "stub-cache.h"
#include "vm-state-inl.h"

namespace v8 {
namespace internal {


int HandleScope::NumberOfHandles() {
  Isolate* isolate = Isolate::Current();
  HandleScopeImplementer* impl = isolate->handle_scope_implementer();
  int n = impl->blocks()->length();
  if (n == 0) return 0;
  return ((n - 1) * kHandleBlockSize) + static_cast<int>(
      (isolate->handle_scope_data()->next - impl->blocks()->last()));
}


Object** HandleScope::Extend() {
  Isolate* isolate = Isolate::Current();
  v8::ImplementationUtilities::HandleScopeData* current =
      isolate->handle_scope_data();

  Object** result = current->next;

  ASSERT(result == current->limit);
  // Make sure there's at least one scope on the stack and that the
  // top of the scope stack isn't a barrier.
  if (current->level == 0) {
    Utils::ReportApiFailure("v8::HandleScope::CreateHandle()",
                            "Cannot create a handle without a HandleScope");
    return NULL;
  }
  HandleScopeImplementer* impl = isolate->handle_scope_implementer();
  // If there's more room in the last block, we use that. This is used
  // for fast creation of scopes after scope barriers.
  if (!impl->blocks()->is_empty()) {
    Object** limit = &impl->blocks()->last()[kHandleBlockSize];
    if (current->limit != limit) {
      current->limit = limit;
      ASSERT(limit - current->next < kHandleBlockSize);
    }
  }

  // If we still haven't found a slot for the handle, we extend the
  // current handle scope by allocating a new handle block.
  if (result == current->limit) {
    // If there's a spare block, use it for growing the current scope.
    result = impl->GetSpareOrNewBlock();
    // Add the extension to the global list of blocks, but count the
    // extension as part of the current scope.
    impl->blocks()->Add(result);
    current->limit = &result[kHandleBlockSize];
  }

  return result;
}


void HandleScope::DeleteExtensions(Isolate* isolate) {
  ASSERT(isolate == Isolate::Current());
  v8::ImplementationUtilities::HandleScopeData* current =
      isolate->handle_scope_data();
  isolate->handle_scope_implementer()->DeleteExtensions(current->limit);
}


void HandleScope::ZapRange(Object** start, Object** end) {
  ASSERT(end - start <= kHandleBlockSize);
  for (Object** p = start; p != end; p++) {
    *reinterpret_cast<Address*>(p) = v8::internal::kHandleZapValue;
  }
}


Address HandleScope::current_level_address() {
  return reinterpret_cast<Address>(
      &Isolate::Current()->handle_scope_data()->level);
}


Address HandleScope::current_next_address() {
  return reinterpret_cast<Address>(
      &Isolate::Current()->handle_scope_data()->next);
}


Address HandleScope::current_limit_address() {
  return reinterpret_cast<Address>(
      &Isolate::Current()->handle_scope_data()->limit);
}


Handle<FixedArray> AddKeysFromJSArray(Handle<FixedArray> content,
                                      Handle<JSArray> array) {
  CALL_HEAP_FUNCTION(content->GetIsolate(),
                     content->AddKeysFromJSArray(*array), FixedArray);
}


Handle<FixedArray> UnionOfKeys(Handle<FixedArray> first,
                               Handle<FixedArray> second) {
  CALL_HEAP_FUNCTION(first->GetIsolate(),
                     first->UnionOfKeys(*second), FixedArray);
}


Handle<JSGlobalProxy> ReinitializeJSGlobalProxy(
    Handle<JSFunction> constructor,
    Handle<JSGlobalProxy> global) {
  CALL_HEAP_FUNCTION(
      constructor->GetIsolate(),
      constructor->GetHeap()->ReinitializeJSGlobalProxy(*constructor, *global),
      JSGlobalProxy);
}


void SetExpectedNofProperties(Handle<JSFunction> func, int nof) {
  // If objects constructed from this function exist then changing
  // 'estimated_nof_properties' is dangerous since the previous value might
  // have been compiled into the fast construct stub. More over, the inobject
  // slack tracking logic might have adjusted the previous value, so even
  // passing the same value is risky.
  if (func->shared()->live_objects_may_exist()) return;

  func->shared()->set_expected_nof_properties(nof);
  if (func->has_initial_map()) {
    Handle<Map> new_initial_map =
        func->GetIsolate()->factory()->CopyMap(
            Handle<Map>(func->initial_map()));
    new_initial_map->set_unused_property_fields(nof);
    func->set_initial_map(*new_initial_map);
  }
}


void SetPrototypeProperty(Handle<JSFunction> func, Handle<JSObject> value) {
  CALL_HEAP_FUNCTION_VOID(func->GetIsolate(),
                          func->SetPrototype(*value));
}


static int ExpectedNofPropertiesFromEstimate(int estimate) {
  // If no properties are added in the constructor, they are more likely
  // to be added later.
  if (estimate == 0) estimate = 2;

  // We do not shrink objects that go into a snapshot (yet), so we adjust
  // the estimate conservatively.
  if (Serializer::enabled()) return estimate + 2;

  // Inobject slack tracking will reclaim redundant inobject space later,
  // so we can afford to adjust the estimate generously.
  if (FLAG_clever_optimizations) {
    return estimate + 8;
  } else {
    return estimate + 3;
  }
}


void SetExpectedNofPropertiesFromEstimate(Handle<SharedFunctionInfo> shared,
                                          int estimate) {
  // See the comment in SetExpectedNofProperties.
  if (shared->live_objects_may_exist()) return;

  shared->set_expected_nof_properties(
      ExpectedNofPropertiesFromEstimate(estimate));
}


void FlattenString(Handle<String> string) {
  CALL_HEAP_FUNCTION_VOID(string->GetIsolate(), string->TryFlatten());
}


Handle<String> FlattenGetString(Handle<String> string) {
  CALL_HEAP_FUNCTION(string->GetIsolate(), string->TryFlatten(), String);
}


Handle<Object> SetPrototype(Handle<JSFunction> function,
                            Handle<Object> prototype) {
  ASSERT(function->should_have_prototype());
  CALL_HEAP_FUNCTION(function->GetIsolate(),
                     Accessors::FunctionSetPrototype(*function,
                                                     *prototype,
                                                     NULL),
                     Object);
}


Handle<Object> SetProperty(Handle<Object> object,
                           Handle<Object> key,
                           Handle<Object> value,
                           PropertyAttributes attributes,
                           StrictModeFlag strict_mode) {
  Isolate* isolate = Isolate::Current();
  CALL_HEAP_FUNCTION(
      isolate,
      Runtime::SetObjectProperty(
          isolate, object, key, value, attributes, strict_mode),
      Object);
}


Handle<Object> ForceSetProperty(Handle<JSObject> object,
                                Handle<Object> key,
                                Handle<Object> value,
                                PropertyAttributes attributes) {
  Isolate* isolate = object->GetIsolate();
  CALL_HEAP_FUNCTION(
      isolate,
      Runtime::ForceSetObjectProperty(
          isolate, object, key, value, attributes),
      Object);
}


Handle<Object> ForceDeleteProperty(Handle<JSObject> object,
                                   Handle<Object> key) {
  Isolate* isolate = object->GetIsolate();
  CALL_HEAP_FUNCTION(isolate,
                     Runtime::ForceDeleteObjectProperty(isolate, object, key),
                     Object);
}


Handle<Object> SetPropertyWithInterceptor(Handle<JSObject> object,
                                          Handle<String> key,
                                          Handle<Object> value,
                                          PropertyAttributes attributes,
                                          StrictModeFlag strict_mode) {
  CALL_HEAP_FUNCTION(object->GetIsolate(),
                     object->SetPropertyWithInterceptor(*key,
                                                        *value,
                                                        attributes,
                                                        strict_mode),
                     Object);
}


Handle<Object> GetProperty(Handle<JSReceiver> obj,
                           const char* name) {
  Isolate* isolate = obj->GetIsolate();
  Handle<String> str = isolate->factory()->LookupAsciiSymbol(name);
  CALL_HEAP_FUNCTION(isolate, obj->GetProperty(*str), Object);
}


Handle<Object> GetProperty(Handle<Object> obj,
                           Handle<Object> key) {
  Isolate* isolate = Isolate::Current();
  CALL_HEAP_FUNCTION(isolate,
                     Runtime::GetObjectProperty(isolate, obj, key), Object);
}


Handle<Object> GetPropertyWithInterceptor(Handle<JSObject> receiver,
                                          Handle<JSObject> holder,
                                          Handle<String> name,
                                          PropertyAttributes* attributes) {
  Isolate* isolate = receiver->GetIsolate();
  CALL_HEAP_FUNCTION(isolate,
                     holder->GetPropertyWithInterceptor(*receiver,
                                                        *name,
                                                        attributes),
                     Object);
}


Handle<Object> SetPrototype(Handle<JSObject> obj, Handle<Object> value) {
  const bool skip_hidden_prototypes = false;
  CALL_HEAP_FUNCTION(obj->GetIsolate(),
                     obj->SetPrototype(*value, skip_hidden_prototypes), Object);
}


Handle<Object> LookupSingleCharacterStringFromCode(uint32_t index) {
  Isolate* isolate = Isolate::Current();
  CALL_HEAP_FUNCTION(
      isolate,
      isolate->heap()->LookupSingleCharacterStringFromCode(index), Object);
}


Handle<String> SubString(Handle<String> str,
                         int start,
                         int end,
                         PretenureFlag pretenure) {
  CALL_HEAP_FUNCTION(str->GetIsolate(),
                     str->SubString(start, end, pretenure), String);
}


Handle<JSObject> Copy(Handle<JSObject> obj) {
  Isolate* isolate = obj->GetIsolate();
  CALL_HEAP_FUNCTION(isolate,
                     isolate->heap()->CopyJSObject(*obj), JSObject);
}


Handle<Object> SetAccessor(Handle<JSObject> obj, Handle<AccessorInfo> info) {
  CALL_HEAP_FUNCTION(obj->GetIsolate(), obj->DefineAccessor(*info), Object);
}


// Wrappers for scripts are kept alive and cached in weak global
// handles referred from foreign objects held by the scripts as long as
// they are used. When they are not used anymore, the garbage
// collector will call the weak callback on the global handle
// associated with the wrapper and get rid of both the wrapper and the
// handle.
static void ClearWrapperCache(Persistent<v8::Value> handle, void*) {
  Handle<Object> cache = Utils::OpenHandle(*handle);
  JSValue* wrapper = JSValue::cast(*cache);
  Foreign* foreign = Script::cast(wrapper->value())->wrapper();
  ASSERT(foreign->foreign_address() ==
         reinterpret_cast<Address>(cache.location()));
  foreign->set_foreign_address(0);
  Isolate* isolate = Isolate::Current();
  isolate->global_handles()->Destroy(cache.location());
  isolate->counters()->script_wrappers()->Decrement();
}


Handle<JSValue> GetScriptWrapper(Handle<Script> script) {
  if (script->wrapper()->foreign_address() != NULL) {
    // Return the script wrapper directly from the cache.
    return Handle<JSValue>(
        reinterpret_cast<JSValue**>(script->wrapper()->foreign_address()));
  }
  Isolate* isolate = Isolate::Current();
  // Construct a new script wrapper.
  isolate->counters()->script_wrappers()->Increment();
  Handle<JSFunction> constructor = isolate->script_function();
  Handle<JSValue> result =
      Handle<JSValue>::cast(isolate->factory()->NewJSObject(constructor));
  result->set_value(*script);

  // Create a new weak global handle and use it to cache the wrapper
  // for future use. The cache will automatically be cleared by the
  // garbage collector when it is not used anymore.
  Handle<Object> handle = isolate->global_handles()->Create(*result);
  isolate->global_handles()->MakeWeak(handle.location(), NULL,
                                      &ClearWrapperCache);
  script->wrapper()->set_foreign_address(
      reinterpret_cast<Address>(handle.location()));
  return result;
}


// Init line_ends array with code positions of line ends inside script
// source.
void InitScriptLineEnds(Handle<Script> script) {
  if (!script->line_ends()->IsUndefined()) return;

  Isolate* isolate = script->GetIsolate();

  if (!script->source()->IsString()) {
    ASSERT(script->source()->IsUndefined());
    Handle<FixedArray> empty = isolate->factory()->NewFixedArray(0);
    script->set_line_ends(*empty);
    ASSERT(script->line_ends()->IsFixedArray());
    return;
  }

  Handle<String> src(String::cast(script->source()), isolate);

  Handle<FixedArray> array = CalculateLineEnds(src, true);

  if (*array != isolate->heap()->empty_fixed_array()) {
    array->set_map(isolate->heap()->fixed_cow_array_map());
  }

  script->set_line_ends(*array);
  ASSERT(script->line_ends()->IsFixedArray());
}


template <typename SourceChar>
static void CalculateLineEnds(Isolate* isolate,
                              List<int>* line_ends,
                              Vector<const SourceChar> src,
                              bool with_last_line) {
  const int src_len = src.length();
  StringSearch<char, SourceChar> search(isolate, CStrVector("\n"));

  // Find and record line ends.
  int position = 0;
  while (position != -1 && position < src_len) {
    position = search.Search(src, position);
    if (position != -1) {
      line_ends->Add(position);
      position++;
    } else if (with_last_line) {
      // Even if the last line misses a line end, it is counted.
      line_ends->Add(src_len);
      return;
    }
  }
}


Handle<FixedArray> CalculateLineEnds(Handle<String> src,
                                     bool with_last_line) {
  src = FlattenGetString(src);
  // Rough estimate of line count based on a roughly estimated average
  // length of (unpacked) code.
  int line_count_estimate = src->length() >> 4;
  List<int> line_ends(line_count_estimate);
  Isolate* isolate = src->GetIsolate();
  {
    AssertNoAllocation no_heap_allocation;  // ensure vectors stay valid.
    // Dispatch on type of strings.
    String::FlatContent content = src->GetFlatContent();
    ASSERT(content.IsFlat());
    if (content.IsAscii()) {
      CalculateLineEnds(isolate,
                        &line_ends,
                        content.ToAsciiVector(),
                        with_last_line);
    } else {
      CalculateLineEnds(isolate,
                        &line_ends,
                        content.ToUC16Vector(),
                        with_last_line);
    }
  }
  int line_count = line_ends.length();
  Handle<FixedArray> array = isolate->factory()->NewFixedArray(line_count);
  for (int i = 0; i < line_count; i++) {
    array->set(i, Smi::FromInt(line_ends[i]));
  }
  return array;
}


// Convert code position into line number.
int GetScriptLineNumber(Handle<Script> script, int code_pos) {
  InitScriptLineEnds(script);
  AssertNoAllocation no_allocation;
  FixedArray* line_ends_array = FixedArray::cast(script->line_ends());
  const int line_ends_len = line_ends_array->length();

  if (!line_ends_len) return -1;

  if ((Smi::cast(line_ends_array->get(0)))->value() >= code_pos) {
    return script->line_offset()->value();
  }

  int left = 0;
  int right = line_ends_len;
  while (int half = (right - left) / 2) {
    if ((Smi::cast(line_ends_array->get(left + half)))->value() > code_pos) {
      right -= half;
    } else {
      left += half;
    }
  }
  return right + script->line_offset()->value();
}

// Convert code position into column number.
int GetScriptColumnNumber(Handle<Script> script, int code_pos) {
  int line_number = GetScriptLineNumber(script, code_pos);
  if (line_number == -1) return -1;

  AssertNoAllocation no_allocation;
  FixedArray* line_ends_array = FixedArray::cast(script->line_ends());
  line_number = line_number - script->line_offset()->value();
  if (line_number == 0) return code_pos + script->column_offset()->value();
  int prev_line_end_pos =
      Smi::cast(line_ends_array->get(line_number - 1))->value();
  return code_pos - (prev_line_end_pos + 1);
}

int GetScriptLineNumberSafe(Handle<Script> script, int code_pos) {
  AssertNoAllocation no_allocation;
  if (!script->line_ends()->IsUndefined()) {
    return GetScriptLineNumber(script, code_pos);
  }
  // Slow mode: we do not have line_ends. We have to iterate through source.
  if (!script->source()->IsString()) {
    return -1;
  }
  String* source = String::cast(script->source());
  int line = 0;
  int len = source->length();
  for (int pos = 0; pos < len; pos++) {
    if (pos == code_pos) {
      break;
    }
    if (source->Get(pos) == '\n') {
      line++;
    }
  }
  return line;
}


void CustomArguments::IterateInstance(ObjectVisitor* v) {
  v->VisitPointers(values_, values_ + ARRAY_SIZE(values_));
}


// Compute the property keys from the interceptor.
v8::Handle<v8::Array> GetKeysForNamedInterceptor(Handle<JSReceiver> receiver,
                                                 Handle<JSObject> object) {
  Isolate* isolate = receiver->GetIsolate();
  Handle<InterceptorInfo> interceptor(object->GetNamedInterceptor());
  CustomArguments args(isolate, interceptor->data(), *receiver, *object);
  v8::AccessorInfo info(args.end());
  v8::Handle<v8::Array> result;
  if (!interceptor->enumerator()->IsUndefined()) {
    v8::NamedPropertyEnumerator enum_fun =
        v8::ToCData<v8::NamedPropertyEnumerator>(interceptor->enumerator());
    LOG(isolate, ApiObjectAccess("interceptor-named-enum", *object));
    {
      // Leaving JavaScript.
      VMState state(isolate, EXTERNAL);
      result = enum_fun(info);
    }
  }
#if ENABLE_EXTRA_CHECKS
  CHECK(result.IsEmpty() || v8::Utils::OpenHandle(*result)->IsJSObject());
#endif
  return result;
}


// Compute the element keys from the interceptor.
v8::Handle<v8::Array> GetKeysForIndexedInterceptor(Handle<JSReceiver> receiver,
                                                   Handle<JSObject> object) {
  Isolate* isolate = receiver->GetIsolate();
  Handle<InterceptorInfo> interceptor(object->GetIndexedInterceptor());
  CustomArguments args(isolate, interceptor->data(), *receiver, *object);
  v8::AccessorInfo info(args.end());
  v8::Handle<v8::Array> result;
  if (!interceptor->enumerator()->IsUndefined()) {
    v8::IndexedPropertyEnumerator enum_fun =
        v8::ToCData<v8::IndexedPropertyEnumerator>(interceptor->enumerator());
    LOG(isolate, ApiObjectAccess("interceptor-indexed-enum", *object));
    {
      // Leaving JavaScript.
      VMState state(isolate, EXTERNAL);
      result = enum_fun(info);
#if ENABLE_EXTRA_CHECKS
      CHECK(result.IsEmpty() || v8::Utils::OpenHandle(*result)->IsJSObject());
#endif
    }
  }
  return result;
}


static bool ContainsOnlyValidKeys(Handle<FixedArray> array) {
  int len = array->length();
  for (int i = 0; i < len; i++) {
    Object* e = array->get(i);
    if (!(e->IsString() || e->IsNumber())) return false;
  }
  return true;
}


Handle<FixedArray> GetKeysInFixedArrayFor(Handle<JSReceiver> object,
                                          KeyCollectionType type,
                                          bool* threw) {
  USE(ContainsOnlyValidKeys);
  Isolate* isolate = object->GetIsolate();
  Handle<FixedArray> content = isolate->factory()->empty_fixed_array();
  Handle<JSObject> arguments_boilerplate = Handle<JSObject>(
      isolate->context()->native_context()->arguments_boilerplate(),
      isolate);
  Handle<JSFunction> arguments_function = Handle<JSFunction>(
      JSFunction::cast(arguments_boilerplate->map()->constructor()),
      isolate);

  // Only collect keys if access is permitted.
  for (Handle<Object> p = object;
       *p != isolate->heap()->null_value();
       p = Handle<Object>(p->GetPrototype(), isolate)) {
    if (p->IsJSProxy()) {
      Handle<JSProxy> proxy(JSProxy::cast(*p), isolate);
      Handle<Object> args[] = { proxy };
      Handle<Object> names = Execution::Call(
          isolate->proxy_enumerate(), object, ARRAY_SIZE(args), args, threw);
      if (*threw) return content;
      content = AddKeysFromJSArray(content, Handle<JSArray>::cast(names));
      break;
    }

    Handle<JSObject> current(JSObject::cast(*p), isolate);

    // Check access rights if required.
    if (current->IsAccessCheckNeeded() &&
        !isolate->MayNamedAccess(*current,
                                 isolate->heap()->undefined_value(),
                                 v8::ACCESS_KEYS)) {
      isolate->ReportFailedAccessCheck(*current, v8::ACCESS_KEYS);
      break;
    }

    // Compute the element keys.
    Handle<FixedArray> element_keys =
        isolate->factory()->NewFixedArray(current->NumberOfEnumElements());
    current->GetEnumElementKeys(*element_keys);
    content = UnionOfKeys(content, element_keys);
    ASSERT(ContainsOnlyValidKeys(content));

    // Add the element keys from the interceptor.
    if (current->HasIndexedInterceptor()) {
      v8::Handle<v8::Array> result =
          GetKeysForIndexedInterceptor(object, current);
      if (!result.IsEmpty())
        content = AddKeysFromJSArray(content, v8::Utils::OpenHandle(*result));
      ASSERT(ContainsOnlyValidKeys(content));
    }

    // We can cache the computed property keys if access checks are
    // not needed and no interceptors are involved.
    //
    // We do not use the cache if the object has elements and
    // therefore it does not make sense to cache the property names
    // for arguments objects.  Arguments objects will always have
    // elements.
    // Wrapped strings have elements, but don't have an elements
    // array or dictionary.  So the fast inline test for whether to
    // use the cache says yes, so we should not create a cache.
    bool cache_enum_keys =
        ((current->map()->constructor() != *arguments_function) &&
         !current->IsJSValue() &&
         !current->IsAccessCheckNeeded() &&
         !current->HasNamedInterceptor() &&
         !current->HasIndexedInterceptor());
    // Compute the property keys and cache them if possible.
    content =
        UnionOfKeys(content, GetEnumPropertyKeys(current, cache_enum_keys));
    ASSERT(ContainsOnlyValidKeys(content));

    // Add the property keys from the interceptor.
    if (current->HasNamedInterceptor()) {
      v8::Handle<v8::Array> result =
          GetKeysForNamedInterceptor(object, current);
      if (!result.IsEmpty())
        content = AddKeysFromJSArray(content, v8::Utils::OpenHandle(*result));
      ASSERT(ContainsOnlyValidKeys(content));
    }

    // If we only want local properties we bail out after the first
    // iteration.
    if (type == LOCAL_ONLY)
      break;
  }
  return content;
}


Handle<JSArray> GetKeysFor(Handle<JSReceiver> object, bool* threw) {
  Isolate* isolate = object->GetIsolate();
  isolate->counters()->for_in()->Increment();
  Handle<FixedArray> elements =
      GetKeysInFixedArrayFor(object, INCLUDE_PROTOS, threw);
  return isolate->factory()->NewJSArrayWithElements(elements);
}


Handle<FixedArray> ReduceFixedArrayTo(Handle<FixedArray> array, int length) {
  ASSERT(array->length() >= length);
  if (array->length() == length) return array;

  Handle<FixedArray> new_array =
      array->GetIsolate()->factory()->NewFixedArray(length);
  for (int i = 0; i < length; ++i) new_array->set(i, array->get(i));
  return new_array;
}


Handle<FixedArray> GetEnumPropertyKeys(Handle<JSObject> object,
                                       bool cache_result) {
  Isolate* isolate = object->GetIsolate();
  if (object->HasFastProperties()) {
    if (object->map()->instance_descriptors()->HasEnumCache()) {
      int own_property_count = object->map()->EnumLength();
      // If we have an enum cache, but the enum length of the given map is set
      // to kInvalidEnumCache, this means that the map itself has never used the
      // present enum cache. The first step to using the cache is to set the
      // enum length of the map by counting the number of own descriptors that
      // are not DONT_ENUM.
      if (own_property_count == Map::kInvalidEnumCache) {
        own_property_count = object->map()->NumberOfDescribedProperties(
            OWN_DESCRIPTORS, DONT_ENUM);

        if (cache_result) object->map()->SetEnumLength(own_property_count);
      }

      DescriptorArray* desc = object->map()->instance_descriptors();
      Handle<FixedArray> keys(desc->GetEnumCache(), isolate);

      // In case the number of properties required in the enum are actually
      // present, we can reuse the enum cache. Otherwise, this means that the
      // enum cache was generated for a previous (smaller) version of the
      // Descriptor Array. In that case we regenerate the enum cache.
      if (own_property_count <= keys->length()) {
        isolate->counters()->enum_cache_hits()->Increment();
        return ReduceFixedArrayTo(keys, own_property_count);
      }
    }

    Handle<Map> map(object->map());

    if (map->instance_descriptors()->IsEmpty()) {
      isolate->counters()->enum_cache_hits()->Increment();
      if (cache_result) map->SetEnumLength(0);
      return isolate->factory()->empty_fixed_array();
    }

    isolate->counters()->enum_cache_misses()->Increment();
    int num_enum = map->NumberOfDescribedProperties(ALL_DESCRIPTORS, DONT_ENUM);

    Handle<FixedArray> storage = isolate->factory()->NewFixedArray(num_enum);
    Handle<FixedArray> indices = isolate->factory()->NewFixedArray(num_enum);

    Handle<DescriptorArray> descs =
        Handle<DescriptorArray>(object->map()->instance_descriptors(), isolate);

    int real_size = map->NumberOfOwnDescriptors();
    int enum_size = 0;
    int index = 0;

    for (int i = 0; i < descs->number_of_descriptors(); i++) {
      PropertyDetails details = descs->GetDetails(i);
      if (!details.IsDontEnum()) {
        if (i < real_size) ++enum_size;
        storage->set(index, descs->GetKey(i));
        if (!indices.is_null()) {
          if (details.type() != FIELD) {
            indices = Handle<FixedArray>();
          } else {
            int field_index = Descriptor::IndexFromValue(descs->GetValue(i));
            if (field_index >= map->inobject_properties()) {
              field_index = -(field_index - map->inobject_properties() + 1);
            }
            indices->set(index, Smi::FromInt(field_index));
          }
        }
        index++;
      }
    }
    ASSERT(index == storage->length());

    Handle<FixedArray> bridge_storage =
        isolate->factory()->NewFixedArray(
            DescriptorArray::kEnumCacheBridgeLength);
    DescriptorArray* desc = object->map()->instance_descriptors();
    desc->SetEnumCache(*bridge_storage,
                       *storage,
                       indices.is_null() ? Object::cast(Smi::FromInt(0))
                                         : Object::cast(*indices));
    if (cache_result) {
      object->map()->SetEnumLength(enum_size);
    }

    return ReduceFixedArrayTo(storage, enum_size);
  } else {
    Handle<StringDictionary> dictionary(object->property_dictionary());

    int length = dictionary->NumberOfElements();
    if (length == 0) {
      return Handle<FixedArray>(isolate->heap()->empty_fixed_array());
    }

    // The enumeration array is generated by allocating an array big enough to
    // hold all properties that have been seen, whether they are are deleted or
    // not. Subsequently all visible properties are added to the array. If some
    // properties were not visible, the array is trimmed so it only contains
    // visible properties. This improves over adding elements and sorting by
    // index by having linear complexity rather than n*log(n).

    // By comparing the monotonous NextEnumerationIndex to the NumberOfElements,
    // we can predict the number of holes in the final array. If there will be
    // more than 50% holes, regenerate the enumeration indices to reduce the
    // number of holes to a minimum. This avoids allocating a large array if
    // many properties were added but subsequently deleted.
    int next_enumeration = dictionary->NextEnumerationIndex();
    if (!object->IsGlobalObject() && next_enumeration > (length * 3) / 2) {
      StringDictionary::DoGenerateNewEnumerationIndices(dictionary);
      next_enumeration = dictionary->NextEnumerationIndex();
    }

    Handle<FixedArray> storage =
        isolate->factory()->NewFixedArray(next_enumeration);

    storage = Handle<FixedArray>(dictionary->CopyEnumKeysTo(*storage));
    ASSERT(storage->length() == object->NumberOfLocalProperties(DONT_ENUM));
    return storage;
  }
}


Handle<ObjectHashSet> ObjectHashSetAdd(Handle<ObjectHashSet> table,
                                       Handle<Object> key) {
  CALL_HEAP_FUNCTION(table->GetIsolate(),
                     table->Add(*key),
                     ObjectHashSet);
}


Handle<ObjectHashSet> ObjectHashSetRemove(Handle<ObjectHashSet> table,
                                          Handle<Object> key) {
  CALL_HEAP_FUNCTION(table->GetIsolate(),
                     table->Remove(*key),
                     ObjectHashSet);
}


Handle<ObjectHashTable> PutIntoObjectHashTable(Handle<ObjectHashTable> table,
                                               Handle<Object> key,
                                               Handle<Object> value) {
  CALL_HEAP_FUNCTION(table->GetIsolate(),
                     table->Put(*key, *value),
                     ObjectHashTable);
}


// This method determines the type of string involved and then gets the UTF8
// length of the string.  It doesn't flatten the string and has log(n) recursion
// for a string of length n.  If the failure flag gets set, then we have to
// flatten the string and retry.  Failures are caused by surrogate pairs in deep
// cons strings.

// Single surrogate characters that are encountered in the UTF-16 character
// sequence of the input string get counted as 3 UTF-8 bytes, because that
// is the way that WriteUtf8 will encode them.  Surrogate pairs are counted and
// encoded as one 4-byte UTF-8 sequence.

// This function conceptually uses recursion on the two halves of cons strings.
// However, in order to avoid the recursion going too deep it recurses on the
// second string of the cons, but iterates on the first substring (by manually
// eliminating it as a tail recursion).  This means it counts the UTF-8 length
// from the end to the start, which makes no difference to the total.

// Surrogate pairs are recognized even if they are split across two sides of a
// cons, which complicates the implementation somewhat.  Therefore, too deep
// recursion cannot always be avoided.  This case is detected, and the failure
// flag is set, a signal to the caller that the string should be flattened and
// the operation retried.
int Utf8LengthHelper(String* input,
                     int from,
                     int to,
                     bool followed_by_surrogate,
                     int max_recursion,
                     bool* failure,
                     bool* starts_with_surrogate) {
  if (from == to) return 0;
  int total = 0;
  bool dummy;
  while (true) {
    if (input->IsAsciiRepresentation()) {
      *starts_with_surrogate = false;
      return total + to - from;
    }
    switch (StringShape(input).representation_tag()) {
      case kConsStringTag: {
        ConsString* str = ConsString::cast(input);
        String* first = str->first();
        String* second = str->second();
        int first_length = first->length();
        if (first_length - from > to - first_length) {
          if (first_length < to) {
            // Right hand side is shorter.  No need to check the recursion depth
            // since this can only happen log(n) times.
            bool right_starts_with_surrogate = false;
            total += Utf8LengthHelper(second,
                                      0,
                                      to - first_length,
                                      followed_by_surrogate,
                                      max_recursion - 1,
                                      failure,
                                      &right_starts_with_surrogate);
            if (*failure) return 0;
            followed_by_surrogate = right_starts_with_surrogate;
            input = first;
            to = first_length;
          } else {
            // We only need the left hand side.
            input = first;
          }
        } else {
          if (first_length > from) {
            // Left hand side is shorter.
            if (first->IsAsciiRepresentation()) {
              total += first_length - from;
              *starts_with_surrogate = false;
              starts_with_surrogate = &dummy;
              input = second;
              from = 0;
              to -= first_length;
            } else if (second->IsAsciiRepresentation()) {
              followed_by_surrogate = false;
              total += to - first_length;
              input = first;
              to = first_length;
            } else if (max_recursion > 0) {
              bool right_starts_with_surrogate = false;
              // Recursing on the long one.  This may fail.
              total += Utf8LengthHelper(second,
                                        0,
                                        to - first_length,
                                        followed_by_surrogate,
                                        max_recursion - 1,
                                        failure,
                                        &right_starts_with_surrogate);
              if (*failure) return 0;
              input = first;
              to = first_length;
              followed_by_surrogate = right_starts_with_surrogate;
            } else {
              *failure = true;
              return 0;
            }
          } else {
            // We only need the right hand side.
            input = second;
            from = 0;
            to -= first_length;
          }
        }
        continue;
      }
      case kExternalStringTag:
      case kSeqStringTag: {
        Vector<const uc16> vector = input->GetFlatContent().ToUC16Vector();
        const uc16* p = vector.start();
        int previous = unibrow::Utf16::kNoPreviousCharacter;
        for (int i = from; i < to; i++) {
          uc16 c = p[i];
          total += unibrow::Utf8::Length(c, previous);
          previous = c;
        }
        if (to - from > 0) {
          if (unibrow::Utf16::IsLeadSurrogate(previous) &&
              followed_by_surrogate) {
            total -= unibrow::Utf8::kBytesSavedByCombiningSurrogates;
          }
          if (unibrow::Utf16::IsTrailSurrogate(p[from])) {
            *starts_with_surrogate = true;
          }
        }
        return total;
      }
      case kSlicedStringTag: {
        SlicedString* str = SlicedString::cast(input);
        int offset = str->offset();
        input = str->parent();
        from += offset;
        to += offset;
        continue;
      }
      default:
        break;
    }
    UNREACHABLE();
    return 0;
  }
  return 0;
}


int Utf8Length(Handle<String> str) {
  bool dummy;
  bool failure;
  int len;
  const int kRecursionBudget = 100;
  do {
    failure = false;
    len = Utf8LengthHelper(
        *str, 0, str->length(), false, kRecursionBudget, &failure, &dummy);
    if (failure) FlattenString(str);
  } while (failure);
  return len;
}


DeferredHandleScope::DeferredHandleScope(Isolate* isolate)
    : impl_(isolate->handle_scope_implementer()) {
  ASSERT(impl_->isolate() == Isolate::Current());
  impl_->BeginDeferredScope();
  v8::ImplementationUtilities::HandleScopeData* data =
      impl_->isolate()->handle_scope_data();
  Object** new_next = impl_->GetSpareOrNewBlock();
  Object** new_limit = &new_next[kHandleBlockSize];
  ASSERT(data->limit == &impl_->blocks()->last()[kHandleBlockSize]);
  impl_->blocks()->Add(new_next);

#ifdef DEBUG
  prev_level_ = data->level;
#endif
  data->level++;
  prev_limit_ = data->limit;
  prev_next_ = data->next;
  data->next = new_next;
  data->limit = new_limit;
}


DeferredHandleScope::~DeferredHandleScope() {
  impl_->isolate()->handle_scope_data()->level--;
  ASSERT(handles_detached_);
  ASSERT(impl_->isolate()->handle_scope_data()->level == prev_level_);
}


DeferredHandles* DeferredHandleScope::Detach() {
  DeferredHandles* deferred = impl_->Detach(prev_limit_);
  v8::ImplementationUtilities::HandleScopeData* data =
      impl_->isolate()->handle_scope_data();
  data->next = prev_next_;
  data->limit = prev_limit_;
#ifdef DEBUG
  handles_detached_ = true;
#endif
  return deferred;
}


} }  // namespace v8::internal
