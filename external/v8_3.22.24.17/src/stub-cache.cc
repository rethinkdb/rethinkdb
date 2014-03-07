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

#include "api.h"
#include "arguments.h"
#include "ast.h"
#include "code-stubs.h"
#include "cpu-profiler.h"
#include "gdb-jit.h"
#include "ic-inl.h"
#include "stub-cache.h"
#include "vm-state-inl.h"

namespace v8 {
namespace internal {

// -----------------------------------------------------------------------
// StubCache implementation.


StubCache::StubCache(Isolate* isolate)
    : isolate_(isolate) { }


void StubCache::Initialize() {
  ASSERT(IsPowerOf2(kPrimaryTableSize));
  ASSERT(IsPowerOf2(kSecondaryTableSize));
  Clear();
}


Code* StubCache::Set(Name* name, Map* map, Code* code) {
  // Get the flags from the code.
  Code::Flags flags = Code::RemoveTypeFromFlags(code->flags());

  // Validate that the name does not move on scavenge, and that we
  // can use identity checks instead of structural equality checks.
  ASSERT(!heap()->InNewSpace(name));
  ASSERT(name->IsUniqueName());

  // The state bits are not important to the hash function because
  // the stub cache only contains monomorphic stubs. Make sure that
  // the bits are the least significant so they will be the ones
  // masked out.
  ASSERT(Code::ExtractICStateFromFlags(flags) == MONOMORPHIC);
  STATIC_ASSERT((Code::ICStateField::kMask & 1) == 1);

  // Make sure that the code type is not included in the hash.
  ASSERT(Code::ExtractTypeFromFlags(flags) == 0);

  // Compute the primary entry.
  int primary_offset = PrimaryOffset(name, flags, map);
  Entry* primary = entry(primary_, primary_offset);
  Code* old_code = primary->value;

  // If the primary entry has useful data in it, we retire it to the
  // secondary cache before overwriting it.
  if (old_code != isolate_->builtins()->builtin(Builtins::kIllegal)) {
    Map* old_map = primary->map;
    Code::Flags old_flags = Code::RemoveTypeFromFlags(old_code->flags());
    int seed = PrimaryOffset(primary->key, old_flags, old_map);
    int secondary_offset = SecondaryOffset(primary->key, old_flags, seed);
    Entry* secondary = entry(secondary_, secondary_offset);
    *secondary = *primary;
  }

  // Update primary cache.
  primary->key = name;
  primary->value = code;
  primary->map = map;
  isolate()->counters()->megamorphic_stub_cache_updates()->Increment();
  return code;
}


Handle<Code> StubCache::FindIC(Handle<Name> name,
                               Handle<Map> stub_holder_map,
                               Code::Kind kind,
                               Code::ExtraICState extra_state) {
  Code::Flags flags = Code::ComputeMonomorphicFlags(kind, extra_state);
  Handle<Object> probe(stub_holder_map->FindInCodeCache(*name, flags),
                       isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);
  return Handle<Code>::null();
}


Handle<Code> StubCache::FindIC(Handle<Name> name,
                               Handle<JSObject> stub_holder,
                               Code::Kind kind,
                               Code::ExtraICState extra_ic_state) {
  return FindIC(name, Handle<Map>(stub_holder->map()), kind, extra_ic_state);
}


Handle<Code> StubCache::FindHandler(Handle<Name> name,
                                    Handle<JSObject> receiver,
                                    Code::Kind kind,
                                    StrictModeFlag strict_mode) {
  Code::ExtraICState extra_ic_state = Code::kNoExtraICState;
  if (kind == Code::STORE_IC || kind == Code::KEYED_STORE_IC) {
    extra_ic_state = Code::ComputeExtraICState(
        STANDARD_STORE, strict_mode);
  }
  Code::Flags flags = Code::ComputeMonomorphicFlags(
      Code::HANDLER, extra_ic_state, Code::NORMAL, kind);
  Handle<Object> probe(receiver->map()->FindInCodeCache(*name, flags),
                       isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);
  return Handle<Code>::null();
}


Handle<Code> StubCache::ComputeMonomorphicIC(Handle<HeapObject> receiver,
                                             Handle<Code> handler,
                                             Handle<Name> name,
                                             StrictModeFlag strict_mode) {
  Code::Kind kind = handler->handler_kind();
  Handle<Map> map(receiver->map());
  Handle<Code> ic = FindIC(name, map, kind, strict_mode);
  if (!ic.is_null()) return ic;

  if (kind == Code::LOAD_IC) {
    LoadStubCompiler ic_compiler(isolate());
    ic = ic_compiler.CompileMonomorphicIC(map, handler, name);
  } else if (kind == Code::KEYED_LOAD_IC) {
    KeyedLoadStubCompiler ic_compiler(isolate());
    ic = ic_compiler.CompileMonomorphicIC(map, handler, name);
  } else if (kind == Code::STORE_IC) {
    StoreStubCompiler ic_compiler(isolate(), strict_mode);
    ic = ic_compiler.CompileMonomorphicIC(map, handler, name);
  } else {
    ASSERT(kind == Code::KEYED_STORE_IC);
    KeyedStoreStubCompiler ic_compiler(isolate(), strict_mode, STANDARD_STORE);
    ic = ic_compiler.CompileMonomorphicIC(map, handler, name);
  }

  HeapObject::UpdateMapCodeCache(receiver, name, ic);
  return ic;
}


Handle<Code> StubCache::ComputeLoadNonexistent(Handle<Name> name,
                                               Handle<JSObject> receiver) {
  // If no global objects are present in the prototype chain, the load
  // nonexistent IC stub can be shared for all names for a given map
  // and we use the empty string for the map cache in that case.  If
  // there are global objects involved, we need to check global
  // property cells in the stub and therefore the stub will be
  // specific to the name.
  Handle<Name> cache_name = factory()->empty_string();
  Handle<JSObject> current;
  Handle<Object> next = receiver;
  Handle<JSGlobalObject> global;
  do {
    current = Handle<JSObject>::cast(next);
    next = Handle<Object>(current->GetPrototype(), isolate_);
    if (current->IsJSGlobalObject()) {
      global = Handle<JSGlobalObject>::cast(current);
      cache_name = name;
    } else if (!current->HasFastProperties()) {
      cache_name = name;
    }
  } while (!next->IsNull());

  // Compile the stub that is either shared for all names or
  // name specific if there are global objects involved.
  Handle<Code> handler = FindHandler(cache_name, receiver, Code::LOAD_IC);
  if (!handler.is_null()) return handler;

  LoadStubCompiler compiler(isolate_);
  handler =
      compiler.CompileLoadNonexistent(receiver, current, cache_name, global);
  HeapObject::UpdateMapCodeCache(receiver, cache_name, handler);
  return handler;
}


Handle<Code> StubCache::ComputeLoadGlobal(Handle<Name> name,
                                          Handle<JSObject> receiver,
                                          Handle<GlobalObject> holder,
                                          Handle<PropertyCell> cell,
                                          bool is_dont_delete) {
  Handle<Code> stub = FindIC(name, receiver, Code::LOAD_IC);
  if (!stub.is_null()) return stub;

  LoadStubCompiler compiler(isolate_);
  Handle<Code> ic =
      compiler.CompileLoadGlobal(receiver, holder, cell, name, is_dont_delete);
  HeapObject::UpdateMapCodeCache(receiver, name, ic);
  return ic;
}


Handle<Code> StubCache::ComputeKeyedLoadElement(Handle<Map> receiver_map) {
  Code::Flags flags = Code::ComputeMonomorphicFlags(Code::KEYED_LOAD_IC);
  Handle<Name> name =
      isolate()->factory()->KeyedLoadElementMonomorphic_string();

  Handle<Object> probe(receiver_map->FindInCodeCache(*name, flags), isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  KeyedLoadStubCompiler compiler(isolate());
  Handle<Code> code = compiler.CompileLoadElement(receiver_map);

  Map::UpdateCodeCache(receiver_map, name, code);
  return code;
}


Handle<Code> StubCache::ComputeKeyedStoreElement(
    Handle<Map> receiver_map,
    StrictModeFlag strict_mode,
    KeyedAccessStoreMode store_mode) {
  Code::ExtraICState extra_state =
      Code::ComputeExtraICState(store_mode, strict_mode);
  Code::Flags flags = Code::ComputeMonomorphicFlags(
      Code::KEYED_STORE_IC, extra_state);

  ASSERT(store_mode == STANDARD_STORE ||
         store_mode == STORE_AND_GROW_NO_TRANSITION ||
         store_mode == STORE_NO_TRANSITION_IGNORE_OUT_OF_BOUNDS ||
         store_mode == STORE_NO_TRANSITION_HANDLE_COW);

  Handle<String> name =
      isolate()->factory()->KeyedStoreElementMonomorphic_string();
  Handle<Object> probe(receiver_map->FindInCodeCache(*name, flags), isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  KeyedStoreStubCompiler compiler(isolate(), strict_mode, store_mode);
  Handle<Code> code = compiler.CompileStoreElement(receiver_map);

  Map::UpdateCodeCache(receiver_map, name, code);
  ASSERT(Code::GetKeyedAccessStoreMode(code->extra_ic_state()) == store_mode);
  return code;
}


Handle<Code> StubCache::ComputeStoreGlobal(Handle<Name> name,
                                           Handle<GlobalObject> receiver,
                                           Handle<PropertyCell> cell,
                                           Handle<Object> value,
                                           StrictModeFlag strict_mode) {
  Handle<Type> union_type = PropertyCell::UpdatedType(cell, value);
  bool is_constant = union_type->IsConstant();
  StoreGlobalStub stub(strict_mode, is_constant);

  Handle<Code> code = FindIC(
      name, Handle<JSObject>::cast(receiver),
      Code::STORE_IC, stub.GetExtraICState());
  if (!code.is_null()) return code;

  // Replace the placeholder cell and global object map with the actual global
  // cell and receiver map.
  Handle<Map> meta_map(isolate_->heap()->meta_map());
  Handle<Object> receiver_map(receiver->map(), isolate_);
  code = stub.GetCodeCopyFromTemplate(isolate_);
  code->ReplaceNthObject(1, *meta_map, *receiver_map);
  Handle<Map> cell_map(isolate_->heap()->global_property_cell_map());
  code->ReplaceNthObject(1, *cell_map, *cell);

  HeapObject::UpdateMapCodeCache(receiver, name, code);

  return code;
}


#define CALL_LOGGER_TAG(kind, type) \
    (kind == Code::CALL_IC ? Logger::type : Logger::KEYED_##type)

Handle<Code> StubCache::ComputeCallConstant(int argc,
                                            Code::Kind kind,
                                            Code::ExtraICState extra_state,
                                            Handle<Name> name,
                                            Handle<Object> object,
                                            Handle<JSObject> holder,
                                            Handle<JSFunction> function) {
  // Compute the check type and the map.
  InlineCacheHolderFlag cache_holder =
      IC::GetCodeCacheForObject(*object, *holder);
  Handle<JSObject> stub_holder(IC::GetCodeCacheHolder(
      isolate_, *object, cache_holder));

  // Compute check type based on receiver/holder.
  CheckType check = RECEIVER_MAP_CHECK;
  if (object->IsString()) {
    check = STRING_CHECK;
  } else if (object->IsSymbol()) {
    check = SYMBOL_CHECK;
  } else if (object->IsNumber()) {
    check = NUMBER_CHECK;
  } else if (object->IsBoolean()) {
    check = BOOLEAN_CHECK;
  }

  if (check != RECEIVER_MAP_CHECK &&
      !function->IsBuiltin() &&
      function->shared()->is_classic_mode()) {
    // Calling non-strict non-builtins with a value as the receiver
    // requires boxing.
    return Handle<Code>::null();
  }

  Code::Flags flags = Code::ComputeMonomorphicFlags(
      kind, extra_state, Code::CONSTANT, argc, cache_holder);
  Handle<Object> probe(stub_holder->map()->FindInCodeCache(*name, flags),
                       isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  CallStubCompiler compiler(isolate_, argc, kind, extra_state, cache_holder);
  Handle<Code> code =
      compiler.CompileCallConstant(object, holder, name, check, function);
  code->set_check_type(check);
  ASSERT(flags == code->flags());
  PROFILE(isolate_,
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_IC_TAG), *code, *name));
  GDBJIT(AddCode(GDBJITInterface::CALL_IC, *name, *code));

  if (CallStubCompiler::CanBeCached(function)) {
    HeapObject::UpdateMapCodeCache(stub_holder, name, code);
  }
  return code;
}


Handle<Code> StubCache::ComputeCallField(int argc,
                                         Code::Kind kind,
                                         Code::ExtraICState extra_state,
                                         Handle<Name> name,
                                         Handle<Object> object,
                                         Handle<JSObject> holder,
                                         PropertyIndex index) {
  // Compute the check type and the map.
  InlineCacheHolderFlag cache_holder =
      IC::GetCodeCacheForObject(*object, *holder);
  Handle<JSObject> stub_holder(IC::GetCodeCacheHolder(
      isolate_, *object, cache_holder));

  // TODO(1233596): We cannot do receiver map check for non-JS objects
  // because they may be represented as immediates without a
  // map. Instead, we check against the map in the holder.
  if (object->IsNumber() || object->IsSymbol() ||
      object->IsBoolean() || object->IsString()) {
    object = holder;
  }

  Code::Flags flags = Code::ComputeMonomorphicFlags(
      kind, extra_state, Code::FIELD, argc, cache_holder);
  Handle<Object> probe(stub_holder->map()->FindInCodeCache(*name, flags),
                       isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  CallStubCompiler compiler(isolate_, argc, kind, extra_state, cache_holder);
  Handle<Code> code =
      compiler.CompileCallField(Handle<JSObject>::cast(object),
                                holder, index, name);
  ASSERT(flags == code->flags());
  PROFILE(isolate_,
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_IC_TAG), *code, *name));
  GDBJIT(AddCode(GDBJITInterface::CALL_IC, *name, *code));
  HeapObject::UpdateMapCodeCache(stub_holder, name, code);
  return code;
}


Handle<Code> StubCache::ComputeCallInterceptor(int argc,
                                               Code::Kind kind,
                                               Code::ExtraICState extra_state,
                                               Handle<Name> name,
                                               Handle<Object> object,
                                               Handle<JSObject> holder) {
  // Compute the check type and the map.
  InlineCacheHolderFlag cache_holder =
      IC::GetCodeCacheForObject(*object, *holder);
  Handle<JSObject> stub_holder(IC::GetCodeCacheHolder(
      isolate_, *object, cache_holder));

  // TODO(1233596): We cannot do receiver map check for non-JS objects
  // because they may be represented as immediates without a
  // map. Instead, we check against the map in the holder.
  if (object->IsNumber() || object->IsSymbol() ||
      object->IsBoolean() || object->IsString()) {
    object = holder;
  }

  Code::Flags flags = Code::ComputeMonomorphicFlags(
      kind, extra_state, Code::INTERCEPTOR, argc, cache_holder);
  Handle<Object> probe(stub_holder->map()->FindInCodeCache(*name, flags),
                       isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  CallStubCompiler compiler(isolate(), argc, kind, extra_state, cache_holder);
  Handle<Code> code =
      compiler.CompileCallInterceptor(Handle<JSObject>::cast(object),
                                      holder, name);
  ASSERT(flags == code->flags());
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_IC_TAG), *code, *name));
  GDBJIT(AddCode(GDBJITInterface::CALL_IC, *name, *code));
  HeapObject::UpdateMapCodeCache(stub_holder, name, code);
  return code;
}


Handle<Code> StubCache::ComputeCallGlobal(int argc,
                                          Code::Kind kind,
                                          Code::ExtraICState extra_state,
                                          Handle<Name> name,
                                          Handle<JSObject> receiver,
                                          Handle<GlobalObject> holder,
                                          Handle<PropertyCell> cell,
                                          Handle<JSFunction> function) {
  Code::Flags flags = Code::ComputeMonomorphicFlags(
      kind, extra_state, Code::NORMAL, argc);
  Handle<Object> probe(receiver->map()->FindInCodeCache(*name, flags),
                       isolate_);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  CallStubCompiler compiler(isolate(), argc, kind, extra_state);
  Handle<Code> code =
      compiler.CompileCallGlobal(receiver, holder, cell, function, name);
  ASSERT(flags == code->flags());
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_IC_TAG), *code, *name));
  GDBJIT(AddCode(GDBJITInterface::CALL_IC, *name, *code));
  if (CallStubCompiler::CanBeCached(function)) {
    HeapObject::UpdateMapCodeCache(receiver, name, code);
  }
  return code;
}


static void FillCache(Isolate* isolate, Handle<Code> code) {
  Handle<UnseededNumberDictionary> dictionary =
      UnseededNumberDictionary::Set(isolate->factory()->non_monomorphic_cache(),
                                    code->flags(),
                                    code);
  isolate->heap()->public_set_non_monomorphic_cache(*dictionary);
}


Code* StubCache::FindCallInitialize(int argc,
                                    RelocInfo::Mode mode,
                                    Code::Kind kind) {
  Code::ExtraICState extra_state =
      CallICBase::StringStubState::encode(DEFAULT_STRING_STUB) |
      CallICBase::Contextual::encode(mode == RelocInfo::CODE_TARGET_CONTEXT);
  Code::Flags flags =
      Code::ComputeFlags(kind, UNINITIALIZED, extra_state, Code::NORMAL, argc);
  UnseededNumberDictionary* dictionary =
      isolate()->heap()->non_monomorphic_cache();
  int entry = dictionary->FindEntry(isolate(), flags);
  ASSERT(entry != -1);
  Object* code = dictionary->ValueAt(entry);
  // This might be called during the marking phase of the collector
  // hence the unchecked cast.
  return reinterpret_cast<Code*>(code);
}


Handle<Code> StubCache::ComputeCallInitialize(int argc,
                                              RelocInfo::Mode mode,
                                              Code::Kind kind) {
  Code::ExtraICState extra_state =
      CallICBase::StringStubState::encode(DEFAULT_STRING_STUB) |
      CallICBase::Contextual::encode(mode == RelocInfo::CODE_TARGET_CONTEXT);
  Code::Flags flags =
      Code::ComputeFlags(kind, UNINITIALIZED, extra_state, Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallInitialize(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCallInitialize(int argc, RelocInfo::Mode mode) {
  return ComputeCallInitialize(argc, mode, Code::CALL_IC);
}


Handle<Code> StubCache::ComputeKeyedCallInitialize(int argc) {
  return ComputeCallInitialize(argc, RelocInfo::CODE_TARGET,
                               Code::KEYED_CALL_IC);
}


Handle<Code> StubCache::ComputeCallPreMonomorphic(
    int argc,
    Code::Kind kind,
    Code::ExtraICState extra_state) {
  Code::Flags flags =
      Code::ComputeFlags(kind, PREMONOMORPHIC, extra_state, Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallPreMonomorphic(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCallNormal(int argc,
                                          Code::Kind kind,
                                          Code::ExtraICState extra_state) {
  Code::Flags flags =
      Code::ComputeFlags(kind, MONOMORPHIC, extra_state, Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallNormal(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCallArguments(int argc) {
  Code::Flags flags =
      Code::ComputeFlags(Code::KEYED_CALL_IC, MEGAMORPHIC,
                         Code::kNoExtraICState, Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallArguments(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCallMegamorphic(
    int argc,
    Code::Kind kind,
    Code::ExtraICState extra_state) {
  Code::Flags flags =
      Code::ComputeFlags(kind, MEGAMORPHIC, extra_state,
                         Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallMegamorphic(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCallMiss(int argc,
                                        Code::Kind kind,
                                        Code::ExtraICState extra_state) {
  // MONOMORPHIC_PROTOTYPE_FAILURE state is used to make sure that miss stubs
  // and monomorphic stubs are not mixed up together in the stub cache.
  Code::Flags flags =
      Code::ComputeFlags(kind, MONOMORPHIC_PROTOTYPE_FAILURE, extra_state,
                         Code::NORMAL, argc, OWN_MAP);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallMiss(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCompareNil(Handle<Map> receiver_map,
                                          CompareNilICStub& stub) {
  Handle<String> name(isolate_->heap()->empty_string());
  if (!receiver_map->is_shared()) {
    Handle<Code> cached_ic = FindIC(name, receiver_map, Code::COMPARE_NIL_IC,
                                    stub.GetExtraICState());
    if (!cached_ic.is_null()) return cached_ic;
  }

  Handle<Code> ic = stub.GetCodeCopyFromTemplate(isolate_);
  ic->ReplaceNthObject(1, isolate_->heap()->meta_map(), *receiver_map);

  if (!receiver_map->is_shared()) {
    Map::UpdateCodeCache(receiver_map, name, ic);
  }

  return ic;
}


Handle<Code> StubCache::ComputeLoadElementPolymorphic(
    MapHandleList* receiver_maps) {
  Code::Flags flags = Code::ComputeFlags(Code::KEYED_LOAD_IC, POLYMORPHIC);
  Handle<PolymorphicCodeCache> cache =
      isolate_->factory()->polymorphic_code_cache();
  Handle<Object> probe = cache->Lookup(receiver_maps, flags);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  CodeHandleList handlers(receiver_maps->length());
  KeyedLoadStubCompiler compiler(isolate_);
  compiler.CompileElementHandlers(receiver_maps, &handlers);
  Handle<Code> code = compiler.CompilePolymorphicIC(
      receiver_maps, &handlers, factory()->empty_string(),
      Code::NORMAL, ELEMENT);

  isolate()->counters()->keyed_load_polymorphic_stubs()->Increment();

  PolymorphicCodeCache::Update(cache, receiver_maps, flags, code);
  return code;
}


Handle<Code> StubCache::ComputePolymorphicIC(MapHandleList* receiver_maps,
                                             CodeHandleList* handlers,
                                             int number_of_valid_maps,
                                             Handle<Name> name,
                                             StrictModeFlag strict_mode) {
  Handle<Code> handler = handlers->at(0);
  Code::Kind kind = handler->handler_kind();
  Code::StubType type = number_of_valid_maps == 1 ? handler->type()
                                                  : Code::NORMAL;
  if (kind == Code::LOAD_IC) {
    LoadStubCompiler ic_compiler(isolate_);
    return ic_compiler.CompilePolymorphicIC(
        receiver_maps, handlers, name, type, PROPERTY);
  } else {
    ASSERT(kind == Code::STORE_IC);
    StoreStubCompiler ic_compiler(isolate_, strict_mode);
    return ic_compiler.CompilePolymorphicIC(
        receiver_maps, handlers, name, type, PROPERTY);
  }
}


Handle<Code> StubCache::ComputeStoreElementPolymorphic(
    MapHandleList* receiver_maps,
    KeyedAccessStoreMode store_mode,
    StrictModeFlag strict_mode) {
  ASSERT(store_mode == STANDARD_STORE ||
         store_mode == STORE_AND_GROW_NO_TRANSITION ||
         store_mode == STORE_NO_TRANSITION_IGNORE_OUT_OF_BOUNDS ||
         store_mode == STORE_NO_TRANSITION_HANDLE_COW);
  Handle<PolymorphicCodeCache> cache =
      isolate_->factory()->polymorphic_code_cache();
  Code::ExtraICState extra_state = Code::ComputeExtraICState(store_mode,
                                                             strict_mode);
  Code::Flags flags =
      Code::ComputeFlags(Code::KEYED_STORE_IC, POLYMORPHIC, extra_state);
  Handle<Object> probe = cache->Lookup(receiver_maps, flags);
  if (probe->IsCode()) return Handle<Code>::cast(probe);

  KeyedStoreStubCompiler compiler(isolate_, strict_mode, store_mode);
  Handle<Code> code = compiler.CompileStoreElementPolymorphic(receiver_maps);
  PolymorphicCodeCache::Update(cache, receiver_maps, flags, code);
  return code;
}


#ifdef ENABLE_DEBUGGER_SUPPORT
Handle<Code> StubCache::ComputeCallDebugBreak(int argc,
                                              Code::Kind kind) {
  // Extra IC state is irrelevant for debug break ICs. They jump to
  // the actual call ic to carry out the work.
  Code::Flags flags =
      Code::ComputeFlags(kind, DEBUG_STUB, DEBUG_BREAK,
                         Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallDebugBreak(flags);
  FillCache(isolate_, code);
  return code;
}


Handle<Code> StubCache::ComputeCallDebugPrepareStepIn(int argc,
                                                      Code::Kind kind) {
  // Extra IC state is irrelevant for debug break ICs. They jump to
  // the actual call ic to carry out the work.
  Code::Flags flags =
      Code::ComputeFlags(kind, DEBUG_STUB, DEBUG_PREPARE_STEP_IN,
                         Code::NORMAL, argc);
  Handle<UnseededNumberDictionary> cache =
      isolate_->factory()->non_monomorphic_cache();
  int entry = cache->FindEntry(isolate_, flags);
  if (entry != -1) return Handle<Code>(Code::cast(cache->ValueAt(entry)));

  StubCompiler compiler(isolate_);
  Handle<Code> code = compiler.CompileCallDebugPrepareStepIn(flags);
  FillCache(isolate_, code);
  return code;
}
#endif


void StubCache::Clear() {
  Code* empty = isolate_->builtins()->builtin(Builtins::kIllegal);
  for (int i = 0; i < kPrimaryTableSize; i++) {
    primary_[i].key = heap()->empty_string();
    primary_[i].map = NULL;
    primary_[i].value = empty;
  }
  for (int j = 0; j < kSecondaryTableSize; j++) {
    secondary_[j].key = heap()->empty_string();
    secondary_[j].map = NULL;
    secondary_[j].value = empty;
  }
}


void StubCache::CollectMatchingMaps(SmallMapList* types,
                                    Handle<Name> name,
                                    Code::Flags flags,
                                    Handle<Context> native_context,
                                    Zone* zone) {
  for (int i = 0; i < kPrimaryTableSize; i++) {
    if (primary_[i].key == *name) {
      Map* map = primary_[i].map;
      // Map can be NULL, if the stub is constant function call
      // with a primitive receiver.
      if (map == NULL) continue;

      int offset = PrimaryOffset(*name, flags, map);
      if (entry(primary_, offset) == &primary_[i] &&
          !TypeFeedbackOracle::CanRetainOtherContext(map, *native_context)) {
        types->AddMapIfMissing(Handle<Map>(map), zone);
      }
    }
  }

  for (int i = 0; i < kSecondaryTableSize; i++) {
    if (secondary_[i].key == *name) {
      Map* map = secondary_[i].map;
      // Map can be NULL, if the stub is constant function call
      // with a primitive receiver.
      if (map == NULL) continue;

      // Lookup in primary table and skip duplicates.
      int primary_offset = PrimaryOffset(*name, flags, map);

      // Lookup in secondary table and add matches.
      int offset = SecondaryOffset(*name, flags, primary_offset);
      if (entry(secondary_, offset) == &secondary_[i] &&
          !TypeFeedbackOracle::CanRetainOtherContext(map, *native_context)) {
        types->AddMapIfMissing(Handle<Map>(map), zone);
      }
    }
  }
}


// ------------------------------------------------------------------------
// StubCompiler implementation.


RUNTIME_FUNCTION(MaybeObject*, StoreCallbackProperty) {
  JSObject* recv = JSObject::cast(args[0]);
  ExecutableAccessorInfo* callback = ExecutableAccessorInfo::cast(args[1]);
  Address setter_address = v8::ToCData<Address>(callback->setter());
  v8::AccessorSetterCallback fun =
      FUNCTION_CAST<v8::AccessorSetterCallback>(setter_address);
  ASSERT(fun != NULL);
  ASSERT(callback->IsCompatibleReceiver(recv));
  Handle<Name> name = args.at<Name>(2);
  Handle<Object> value = args.at<Object>(3);
  HandleScope scope(isolate);

  // TODO(rossberg): Support symbols in the API.
  if (name->IsSymbol()) return *value;
  Handle<String> str = Handle<String>::cast(name);

  LOG(isolate, ApiNamedPropertyAccess("store", recv, *name));
  PropertyCallbackArguments
      custom_args(isolate, callback->data(), recv, recv);
  custom_args.Call(fun, v8::Utils::ToLocal(str), v8::Utils::ToLocal(value));
  RETURN_IF_SCHEDULED_EXCEPTION(isolate);
  return *value;
}


/**
 * Attempts to load a property with an interceptor (which must be present),
 * but doesn't search the prototype chain.
 *
 * Returns |Heap::no_interceptor_result_sentinel()| if interceptor doesn't
 * provide any value for the given name.
 */
RUNTIME_FUNCTION(MaybeObject*, LoadPropertyWithInterceptorOnly) {
  ASSERT(args.length() == StubCache::kInterceptorArgsLength);
  Handle<Name> name_handle =
      args.at<Name>(StubCache::kInterceptorArgsNameIndex);
  Handle<InterceptorInfo> interceptor_info =
      args.at<InterceptorInfo>(StubCache::kInterceptorArgsInfoIndex);

  // TODO(rossberg): Support symbols in the API.
  if (name_handle->IsSymbol())
    return isolate->heap()->no_interceptor_result_sentinel();
  Handle<String> name = Handle<String>::cast(name_handle);

  Address getter_address = v8::ToCData<Address>(interceptor_info->getter());
  v8::NamedPropertyGetterCallback getter =
      FUNCTION_CAST<v8::NamedPropertyGetterCallback>(getter_address);
  ASSERT(getter != NULL);

  Handle<JSObject> receiver =
      args.at<JSObject>(StubCache::kInterceptorArgsThisIndex);
  Handle<JSObject> holder =
      args.at<JSObject>(StubCache::kInterceptorArgsHolderIndex);
  PropertyCallbackArguments callback_args(
      isolate, interceptor_info->data(), *receiver, *holder);
  {
    // Use the interceptor getter.
    HandleScope scope(isolate);
    v8::Handle<v8::Value> r =
        callback_args.Call(getter, v8::Utils::ToLocal(name));
    RETURN_IF_SCHEDULED_EXCEPTION(isolate);
    if (!r.IsEmpty()) {
      Handle<Object> result = v8::Utils::OpenHandle(*r);
      result->VerifyApiCallResultType();
      return *v8::Utils::OpenHandle(*r);
    }
  }

  return isolate->heap()->no_interceptor_result_sentinel();
}


static MaybeObject* ThrowReferenceError(Isolate* isolate, Name* name) {
  // If the load is non-contextual, just return the undefined result.
  // Note that both keyed and non-keyed loads may end up here, so we
  // can't use either LoadIC or KeyedLoadIC constructors.
  HandleScope scope(isolate);
  IC ic(IC::NO_EXTRA_FRAME, isolate);
  ASSERT(ic.target()->is_load_stub() || ic.target()->is_keyed_load_stub());
  if (!ic.SlowIsUndeclaredGlobal()) return isolate->heap()->undefined_value();

  // Throw a reference error.
  Handle<Name> name_handle(name);
  Handle<Object> error =
      isolate->factory()->NewReferenceError("not_defined",
                                            HandleVector(&name_handle, 1));
  return isolate->Throw(*error);
}


static Handle<Object> LoadWithInterceptor(Arguments* args,
                                          PropertyAttributes* attrs) {
  ASSERT(args->length() == StubCache::kInterceptorArgsLength);
  Handle<Name> name_handle =
      args->at<Name>(StubCache::kInterceptorArgsNameIndex);
  Handle<InterceptorInfo> interceptor_info =
      args->at<InterceptorInfo>(StubCache::kInterceptorArgsInfoIndex);
  Handle<JSObject> receiver_handle =
      args->at<JSObject>(StubCache::kInterceptorArgsThisIndex);
  Handle<JSObject> holder_handle =
      args->at<JSObject>(StubCache::kInterceptorArgsHolderIndex);

  Isolate* isolate = receiver_handle->GetIsolate();

  // TODO(rossberg): Support symbols in the API.
  if (name_handle->IsSymbol()) {
    return JSObject::GetPropertyPostInterceptor(
        holder_handle, receiver_handle, name_handle, attrs);
  }
  Handle<String> name = Handle<String>::cast(name_handle);

  Address getter_address = v8::ToCData<Address>(interceptor_info->getter());
  v8::NamedPropertyGetterCallback getter =
      FUNCTION_CAST<v8::NamedPropertyGetterCallback>(getter_address);
  ASSERT(getter != NULL);

  PropertyCallbackArguments callback_args(isolate,
                                          interceptor_info->data(),
                                          *receiver_handle,
                                          *holder_handle);
  {
    HandleScope scope(isolate);
    // Use the interceptor getter.
    v8::Handle<v8::Value> r =
        callback_args.Call(getter, v8::Utils::ToLocal(name));
    RETURN_HANDLE_IF_SCHEDULED_EXCEPTION(isolate, Object);
    if (!r.IsEmpty()) {
      *attrs = NONE;
      Handle<Object> result = v8::Utils::OpenHandle(*r);
      result->VerifyApiCallResultType();
      return scope.CloseAndEscape(result);
    }
  }

  Handle<Object> result = JSObject::GetPropertyPostInterceptor(
      holder_handle, receiver_handle, name_handle, attrs);
  return result;
}


/**
 * Loads a property with an interceptor performing post interceptor
 * lookup if interceptor failed.
 */
RUNTIME_FUNCTION(MaybeObject*, LoadPropertyWithInterceptorForLoad) {
  PropertyAttributes attr = NONE;
  HandleScope scope(isolate);
  Handle<Object> result = LoadWithInterceptor(&args, &attr);
  RETURN_IF_EMPTY_HANDLE(isolate, result);

  // If the property is present, return it.
  if (attr != ABSENT) return *result;
  return ThrowReferenceError(isolate, Name::cast(args[0]));
}


RUNTIME_FUNCTION(MaybeObject*, LoadPropertyWithInterceptorForCall) {
  PropertyAttributes attr;
  HandleScope scope(isolate);
  Handle<Object> result = LoadWithInterceptor(&args, &attr);
  RETURN_IF_EMPTY_HANDLE(isolate, result);
  // This is call IC. In this case, we simply return the undefined result which
  // will lead to an exception when trying to invoke the result as a
  // function.
  return *result;
}


RUNTIME_FUNCTION(MaybeObject*, StoreInterceptorProperty) {
  HandleScope scope(isolate);
  ASSERT(args.length() == 4);
  Handle<JSObject> recv(JSObject::cast(args[0]));
  Handle<Name> name(Name::cast(args[1]));
  Handle<Object> value(args[2], isolate);
  ASSERT(args.smi_at(3) == kStrictMode || args.smi_at(3) == kNonStrictMode);
  StrictModeFlag strict_mode = static_cast<StrictModeFlag>(args.smi_at(3));
  ASSERT(recv->HasNamedInterceptor());
  PropertyAttributes attr = NONE;
  Handle<Object> result = JSObject::SetPropertyWithInterceptor(
      recv, name, value, attr, strict_mode);
  RETURN_IF_EMPTY_HANDLE(isolate, result);
  return *result;
}


RUNTIME_FUNCTION(MaybeObject*, KeyedLoadPropertyWithInterceptor) {
  JSObject* receiver = JSObject::cast(args[0]);
  ASSERT(args.smi_at(1) >= 0);
  uint32_t index = args.smi_at(1);
  return receiver->GetElementWithInterceptor(receiver, index);
}


Handle<Code> StubCompiler::CompileCallInitialize(Code::Flags flags) {
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  Code::Kind kind = Code::ExtractKindFromFlags(flags);
  Code::ExtraICState extra_state = Code::ExtractExtraICStateFromFlags(flags);
  if (kind == Code::CALL_IC) {
    CallIC::GenerateInitialize(masm(), argc, extra_state);
  } else {
    KeyedCallIC::GenerateInitialize(masm(), argc);
  }
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallInitialize");
  isolate()->counters()->call_initialize_stubs()->Increment();
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_INITIALIZE_TAG),
                          *code, code->arguments_count()));
  GDBJIT(AddCode(GDBJITInterface::CALL_INITIALIZE, *code));
  return code;
}


Handle<Code> StubCompiler::CompileCallPreMonomorphic(Code::Flags flags) {
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  // The code of the PreMonomorphic stub is the same as the code
  // of the Initialized stub.  They just differ on the code object flags.
  Code::Kind kind = Code::ExtractKindFromFlags(flags);
  Code::ExtraICState extra_state = Code::ExtractExtraICStateFromFlags(flags);
  if (kind == Code::CALL_IC) {
    CallIC::GenerateInitialize(masm(), argc, extra_state);
  } else {
    KeyedCallIC::GenerateInitialize(masm(), argc);
  }
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallPreMonomorphic");
  isolate()->counters()->call_premonomorphic_stubs()->Increment();
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_PRE_MONOMORPHIC_TAG),
                          *code, code->arguments_count()));
  GDBJIT(AddCode(GDBJITInterface::CALL_PRE_MONOMORPHIC, *code));
  return code;
}


Handle<Code> StubCompiler::CompileCallNormal(Code::Flags flags) {
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  Code::Kind kind = Code::ExtractKindFromFlags(flags);
  if (kind == Code::CALL_IC) {
    // Call normal is always with a explict receiver.
    ASSERT(!CallIC::Contextual::decode(
        Code::ExtractExtraICStateFromFlags(flags)));
    CallIC::GenerateNormal(masm(), argc);
  } else {
    KeyedCallIC::GenerateNormal(masm(), argc);
  }
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallNormal");
  isolate()->counters()->call_normal_stubs()->Increment();
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_NORMAL_TAG),
                          *code, code->arguments_count()));
  GDBJIT(AddCode(GDBJITInterface::CALL_NORMAL, *code));
  return code;
}


Handle<Code> StubCompiler::CompileCallMegamorphic(Code::Flags flags) {
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  Code::Kind kind = Code::ExtractKindFromFlags(flags);
  Code::ExtraICState extra_state = Code::ExtractExtraICStateFromFlags(flags);
  if (kind == Code::CALL_IC) {
    CallIC::GenerateMegamorphic(masm(), argc, extra_state);
  } else {
    KeyedCallIC::GenerateMegamorphic(masm(), argc);
  }
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallMegamorphic");
  isolate()->counters()->call_megamorphic_stubs()->Increment();
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_MEGAMORPHIC_TAG),
                          *code, code->arguments_count()));
  GDBJIT(AddCode(GDBJITInterface::CALL_MEGAMORPHIC, *code));
  return code;
}


Handle<Code> StubCompiler::CompileCallArguments(Code::Flags flags) {
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  KeyedCallIC::GenerateNonStrictArguments(masm(), argc);
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallArguments");
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(Code::ExtractKindFromFlags(flags),
                                          CALL_MEGAMORPHIC_TAG),
                          *code, code->arguments_count()));
  GDBJIT(AddCode(GDBJITInterface::CALL_MEGAMORPHIC, *code));
  return code;
}


Handle<Code> StubCompiler::CompileCallMiss(Code::Flags flags) {
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  Code::Kind kind = Code::ExtractKindFromFlags(flags);
  Code::ExtraICState extra_state = Code::ExtractExtraICStateFromFlags(flags);
  if (kind == Code::CALL_IC) {
    CallIC::GenerateMiss(masm(), argc, extra_state);
  } else {
    KeyedCallIC::GenerateMiss(masm(), argc);
  }
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallMiss");
  isolate()->counters()->call_megamorphic_stubs()->Increment();
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(kind, CALL_MISS_TAG),
                          *code, code->arguments_count()));
  GDBJIT(AddCode(GDBJITInterface::CALL_MISS, *code));
  return code;
}


#ifdef ENABLE_DEBUGGER_SUPPORT
Handle<Code> StubCompiler::CompileCallDebugBreak(Code::Flags flags) {
  Debug::GenerateCallICDebugBreak(masm());
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallDebugBreak");
  PROFILE(isolate(),
          CodeCreateEvent(CALL_LOGGER_TAG(Code::ExtractKindFromFlags(flags),
                                          CALL_DEBUG_BREAK_TAG),
                          *code, code->arguments_count()));
  return code;
}


Handle<Code> StubCompiler::CompileCallDebugPrepareStepIn(Code::Flags flags) {
  // Use the same code for the the step in preparations as we do for the
  // miss case.
  int argc = Code::ExtractArgumentsCountFromFlags(flags);
  Code::Kind kind = Code::ExtractKindFromFlags(flags);
  if (kind == Code::CALL_IC) {
    // For the debugger extra ic state is irrelevant.
    CallIC::GenerateMiss(masm(), argc, Code::kNoExtraICState);
  } else {
    KeyedCallIC::GenerateMiss(masm(), argc);
  }
  Handle<Code> code = GetCodeWithFlags(flags, "CompileCallDebugPrepareStepIn");
  PROFILE(isolate(),
          CodeCreateEvent(
              CALL_LOGGER_TAG(kind, CALL_DEBUG_PREPARE_STEP_IN_TAG),
              *code,
              code->arguments_count()));
  return code;
}
#endif  // ENABLE_DEBUGGER_SUPPORT

#undef CALL_LOGGER_TAG


Handle<Code> StubCompiler::GetCodeWithFlags(Code::Flags flags,
                                            const char* name) {
  // Create code object in the heap.
  CodeDesc desc;
  masm_.GetCode(&desc);
  Handle<Code> code = factory()->NewCode(desc, flags, masm_.CodeObject());
#ifdef ENABLE_DISASSEMBLER
  if (FLAG_print_code_stubs) code->Disassemble(name);
#endif
  return code;
}


Handle<Code> StubCompiler::GetCodeWithFlags(Code::Flags flags,
                                            Handle<Name> name) {
  return (FLAG_print_code_stubs && !name.is_null() && name->IsString())
      ? GetCodeWithFlags(flags, *Handle<String>::cast(name)->ToCString())
      : GetCodeWithFlags(flags, NULL);
}


void StubCompiler::LookupPostInterceptor(Handle<JSObject> holder,
                                         Handle<Name> name,
                                         LookupResult* lookup) {
  holder->LocalLookupRealNamedProperty(*name, lookup);
  if (lookup->IsFound()) return;
  if (holder->GetPrototype()->IsNull()) return;
  holder->GetPrototype()->Lookup(*name, lookup);
}


#define __ ACCESS_MASM(masm())


Register LoadStubCompiler::HandlerFrontendHeader(
    Handle<JSObject> object,
    Register object_reg,
    Handle<JSObject> holder,
    Handle<Name> name,
    Label* miss) {
  return CheckPrototypes(object, object_reg, holder,
                         scratch1(), scratch2(), scratch3(),
                         name, miss, SKIP_RECEIVER);
}


// HandlerFrontend for store uses the name register. It has to be restored
// before a miss.
Register StoreStubCompiler::HandlerFrontendHeader(
    Handle<JSObject> object,
    Register object_reg,
    Handle<JSObject> holder,
    Handle<Name> name,
    Label* miss) {
  return CheckPrototypes(object, object_reg, holder,
                         this->name(), scratch1(), scratch2(),
                         name, miss, SKIP_RECEIVER);
}


Register BaseLoadStoreStubCompiler::HandlerFrontend(Handle<JSObject> object,
                                                    Register object_reg,
                                                    Handle<JSObject> holder,
                                                    Handle<Name> name,
                                                    Label* success) {
  Label miss;

  Register reg = HandlerFrontendHeader(object, object_reg, holder, name, &miss);

  HandlerFrontendFooter(name, success, &miss);
  return reg;
}


void LoadStubCompiler::NonexistentHandlerFrontend(
    Handle<JSObject> object,
    Handle<JSObject> last,
    Handle<Name> name,
    Label* success,
    Handle<JSGlobalObject> global) {
  Label miss;

  Register holder =
      HandlerFrontendHeader(object, receiver(), last, name, &miss);

  if (!last->HasFastProperties() &&
      !last->IsJSGlobalObject() &&
      !last->IsJSGlobalProxy()) {
    if (!name->IsUniqueName()) {
      ASSERT(name->IsString());
      name = factory()->InternalizeString(Handle<String>::cast(name));
    }
    ASSERT(last->property_dictionary()->FindEntry(*name) ==
        NameDictionary::kNotFound);
    GenerateDictionaryNegativeLookup(masm(), &miss, holder, name,
                                     scratch2(), scratch3());
  }

  // If the last object in the prototype chain is a global object,
  // check that the global property cell is empty.
  if (!global.is_null()) {
    GenerateCheckPropertyCell(masm(), global, name, scratch2(), &miss);
  }

  HandlerFrontendFooter(name, success, &miss);
}


Handle<Code> LoadStubCompiler::CompileLoadField(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    PropertyIndex field,
    Representation representation) {
  Label miss;

  Register reg = HandlerFrontendHeader(object, receiver(), holder, name, &miss);

  GenerateLoadField(reg, holder, field, representation);

  __ bind(&miss);
  TailCallBuiltin(masm(), MissBuiltin(kind()));

  // Return the generated code.
  return GetCode(kind(), Code::FIELD, name);
}


Handle<Code> LoadStubCompiler::CompileLoadConstant(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    Handle<Object> value) {
  Label success;
  HandlerFrontend(object, receiver(), holder, name, &success);
  __ bind(&success);
  GenerateLoadConstant(value);

  // Return the generated code.
  return GetCode(kind(), Code::CONSTANT, name);
}


Handle<Code> LoadStubCompiler::CompileLoadCallback(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    Handle<ExecutableAccessorInfo> callback) {
  Label success;

  Register reg = CallbackHandlerFrontend(
      object, receiver(), holder, name, &success, callback);
  __ bind(&success);
  GenerateLoadCallback(reg, callback);

  // Return the generated code.
  return GetCode(kind(), Code::CALLBACKS, name);
}


Handle<Code> LoadStubCompiler::CompileLoadCallback(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    const CallOptimization& call_optimization) {
  ASSERT(call_optimization.is_simple_api_call());
  Label success;

  Handle<JSFunction> callback = call_optimization.constant_function();
  CallbackHandlerFrontend(
      object, receiver(), holder, name, &success, callback);
  __ bind(&success);
  GenerateLoadCallback(call_optimization);

  // Return the generated code.
  return GetCode(kind(), Code::CALLBACKS, name);
}


Handle<Code> LoadStubCompiler::CompileLoadInterceptor(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name) {
  Label success;

  LookupResult lookup(isolate());
  LookupPostInterceptor(holder, name, &lookup);

  Register reg = HandlerFrontend(object, receiver(), holder, name, &success);
  __ bind(&success);
  // TODO(368): Compile in the whole chain: all the interceptors in
  // prototypes and ultimate answer.
  GenerateLoadInterceptor(reg, object, holder, &lookup, name);

  // Return the generated code.
  return GetCode(kind(), Code::INTERCEPTOR, name);
}


void LoadStubCompiler::GenerateLoadPostInterceptor(
    Register interceptor_reg,
    Handle<JSObject> interceptor_holder,
    Handle<Name> name,
    LookupResult* lookup) {
  Label success;
  Handle<JSObject> holder(lookup->holder());
  if (lookup->IsField()) {
    PropertyIndex field = lookup->GetFieldIndex();
    if (interceptor_holder.is_identical_to(holder)) {
      GenerateLoadField(
          interceptor_reg, holder, field, lookup->representation());
    } else {
      // We found FIELD property in prototype chain of interceptor's holder.
      // Retrieve a field from field's holder.
      Register reg = HandlerFrontend(
          interceptor_holder, interceptor_reg, holder, name, &success);
      __ bind(&success);
      GenerateLoadField(
          reg, holder, field, lookup->representation());
    }
  } else {
    // We found CALLBACKS property in prototype chain of interceptor's
    // holder.
    ASSERT(lookup->type() == CALLBACKS);
    Handle<ExecutableAccessorInfo> callback(
        ExecutableAccessorInfo::cast(lookup->GetCallbackObject()));
    ASSERT(callback->getter() != NULL);

    Register reg = CallbackHandlerFrontend(
        interceptor_holder, interceptor_reg, holder, name, &success, callback);
    __ bind(&success);
    GenerateLoadCallback(reg, callback);
  }
}


Handle<Code> BaseLoadStoreStubCompiler::CompileMonomorphicIC(
    Handle<Map> receiver_map,
    Handle<Code> handler,
    Handle<Name> name) {
  MapHandleList receiver_maps(1);
  receiver_maps.Add(receiver_map);
  CodeHandleList handlers(1);
  handlers.Add(handler);
  Code::StubType type = handler->type();
  return CompilePolymorphicIC(&receiver_maps, &handlers, name, type, PROPERTY);
}


Handle<Code> LoadStubCompiler::CompileLoadViaGetter(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    Handle<JSFunction> getter) {
  Label success;
  HandlerFrontend(object, receiver(), holder, name, &success);

  __ bind(&success);
  GenerateLoadViaGetter(masm(), receiver(), getter);

  // Return the generated code.
  return GetCode(kind(), Code::CALLBACKS, name);
}


Handle<Code> StoreStubCompiler::CompileStoreTransition(
    Handle<JSObject> object,
    LookupResult* lookup,
    Handle<Map> transition,
    Handle<Name> name) {
  Label miss, slow;

  // Ensure no transitions to deprecated maps are followed.
  __ CheckMapDeprecated(transition, scratch1(), &miss);

  // Check that we are allowed to write this.
  if (object->GetPrototype()->IsJSObject()) {
    Handle<JSObject> holder;
    // holder == object indicates that no property was found.
    if (lookup->holder() != *object) {
      holder = Handle<JSObject>(lookup->holder());
    } else {
      // Find the top object.
      holder = object;
      do {
        holder = Handle<JSObject>(JSObject::cast(holder->GetPrototype()));
      } while (holder->GetPrototype()->IsJSObject());
    }

    Register holder_reg =
        HandlerFrontendHeader(object, receiver(), holder, name, &miss);

    // If no property was found, and the holder (the last object in the
    // prototype chain) is in slow mode, we need to do a negative lookup on the
    // holder.
    if (lookup->holder() == *object) {
      GenerateNegativeHolderLookup(masm(), holder, holder_reg, name, &miss);
    }
  }

  GenerateStoreTransition(masm(),
                          object,
                          lookup,
                          transition,
                          name,
                          receiver(), this->name(), value(),
                          scratch1(), scratch2(), scratch3(),
                          &miss,
                          &slow);

  // Handle store cache miss.
  GenerateRestoreName(masm(), &miss, name);
  TailCallBuiltin(masm(), MissBuiltin(kind()));

  GenerateRestoreName(masm(), &slow, name);
  TailCallBuiltin(masm(), SlowBuiltin(kind()));

  // Return the generated code.
  return GetCode(kind(), Code::TRANSITION, name);
}


Handle<Code> StoreStubCompiler::CompileStoreField(Handle<JSObject> object,
                                                  LookupResult* lookup,
                                                  Handle<Name> name) {
  Label miss;

  HandlerFrontendHeader(object, receiver(), object, name, &miss);

  // Generate store field code.
  GenerateStoreField(masm(),
                     object,
                     lookup,
                     receiver(), this->name(), value(), scratch1(), scratch2(),
                     &miss);

  // Handle store cache miss.
  __ bind(&miss);
  TailCallBuiltin(masm(), MissBuiltin(kind()));

  // Return the generated code.
  return GetCode(kind(), Code::FIELD, name);
}


Handle<Code> StoreStubCompiler::CompileStoreViaSetter(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    Handle<JSFunction> setter) {
  Label success;
  HandlerFrontend(object, receiver(), holder, name, &success);

  __ bind(&success);
  GenerateStoreViaSetter(masm(), setter);

  return GetCode(kind(), Code::CALLBACKS, name);
}


Handle<Code> KeyedLoadStubCompiler::CompileLoadElement(
    Handle<Map> receiver_map) {
  ElementsKind elements_kind = receiver_map->elements_kind();
  if (receiver_map->has_fast_elements() ||
      receiver_map->has_external_array_elements()) {
    Handle<Code> stub = KeyedLoadFastElementStub(
        receiver_map->instance_type() == JS_ARRAY_TYPE,
        elements_kind).GetCode(isolate());
    __ DispatchMap(receiver(), scratch1(), receiver_map, stub, DO_SMI_CHECK);
  } else {
    Handle<Code> stub =
        KeyedLoadDictionaryElementStub().GetCode(isolate());
    __ DispatchMap(receiver(), scratch1(), receiver_map, stub, DO_SMI_CHECK);
  }

  TailCallBuiltin(masm(), Builtins::kKeyedLoadIC_Miss);

  // Return the generated code.
  return GetICCode(kind(), Code::NORMAL, factory()->empty_string());
}


Handle<Code> KeyedStoreStubCompiler::CompileStoreElement(
    Handle<Map> receiver_map) {
  ElementsKind elements_kind = receiver_map->elements_kind();
  bool is_jsarray = receiver_map->instance_type() == JS_ARRAY_TYPE;
  Handle<Code> stub;
  if (receiver_map->has_fast_elements() ||
      receiver_map->has_external_array_elements()) {
    stub = KeyedStoreFastElementStub(
        is_jsarray,
        elements_kind,
        store_mode_).GetCode(isolate());
  } else {
    stub = KeyedStoreElementStub(is_jsarray,
                                 elements_kind,
                                 store_mode_).GetCode(isolate());
  }

  __ DispatchMap(receiver(), scratch1(), receiver_map, stub, DO_SMI_CHECK);

  TailCallBuiltin(masm(), Builtins::kKeyedStoreIC_Miss);

  // Return the generated code.
  return GetICCode(kind(), Code::NORMAL, factory()->empty_string());
}


#undef __


void StubCompiler::TailCallBuiltin(MacroAssembler* masm, Builtins::Name name) {
  Handle<Code> code(masm->isolate()->builtins()->builtin(name));
  GenerateTailCall(masm, code);
}


void BaseLoadStoreStubCompiler::JitEvent(Handle<Name> name, Handle<Code> code) {
#ifdef ENABLE_GDB_JIT_INTERFACE
  GDBJITInterface::CodeTag tag;
  if (kind_ == Code::LOAD_IC) {
    tag = GDBJITInterface::LOAD_IC;
  } else if (kind_ == Code::KEYED_LOAD_IC) {
    tag = GDBJITInterface::KEYED_LOAD_IC;
  } else if (kind_ == Code::STORE_IC) {
    tag = GDBJITInterface::STORE_IC;
  } else {
    tag = GDBJITInterface::KEYED_STORE_IC;
  }
  GDBJIT(AddCode(tag, *name, *code));
#endif
}


void BaseLoadStoreStubCompiler::InitializeRegisters() {
  if (kind_ == Code::LOAD_IC) {
    registers_ = LoadStubCompiler::registers();
  } else if (kind_ == Code::KEYED_LOAD_IC) {
    registers_ = KeyedLoadStubCompiler::registers();
  } else if (kind_ == Code::STORE_IC) {
    registers_ = StoreStubCompiler::registers();
  } else {
    registers_ = KeyedStoreStubCompiler::registers();
  }
}


Handle<Code> BaseLoadStoreStubCompiler::GetICCode(Code::Kind kind,
                                                  Code::StubType type,
                                                  Handle<Name> name,
                                                  InlineCacheState state) {
  Code::Flags flags = Code::ComputeFlags(kind, state, extra_state(), type);
  Handle<Code> code = GetCodeWithFlags(flags, name);
  PROFILE(isolate(), CodeCreateEvent(log_kind(code), *code, *name));
  JitEvent(name, code);
  return code;
}


Handle<Code> BaseLoadStoreStubCompiler::GetCode(Code::Kind kind,
                                                Code::StubType type,
                                                Handle<Name> name) {
  ASSERT(type != Code::NORMAL);
  Code::Flags flags = Code::ComputeFlags(
      Code::HANDLER, MONOMORPHIC, extra_state(), type, kind);
  Handle<Code> code = GetCodeWithFlags(flags, name);
  PROFILE(isolate(), CodeCreateEvent(log_kind(code), *code, *name));
  JitEvent(name, code);
  return code;
}


void KeyedLoadStubCompiler::CompileElementHandlers(MapHandleList* receiver_maps,
                                                   CodeHandleList* handlers) {
  for (int i = 0; i < receiver_maps->length(); ++i) {
    Handle<Map> receiver_map = receiver_maps->at(i);
    Handle<Code> cached_stub;

    if ((receiver_map->instance_type() & kNotStringTag) == 0) {
      cached_stub = isolate()->builtins()->KeyedLoadIC_String();
    } else {
      bool is_js_array = receiver_map->instance_type() == JS_ARRAY_TYPE;
      ElementsKind elements_kind = receiver_map->elements_kind();

      if (IsFastElementsKind(elements_kind) ||
          IsExternalArrayElementsKind(elements_kind)) {
        cached_stub =
            KeyedLoadFastElementStub(is_js_array,
                                     elements_kind).GetCode(isolate());
      } else {
        ASSERT(elements_kind == DICTIONARY_ELEMENTS);
        cached_stub = KeyedLoadDictionaryElementStub().GetCode(isolate());
      }
    }

    handlers->Add(cached_stub);
  }
}


Handle<Code> KeyedStoreStubCompiler::CompileStoreElementPolymorphic(
    MapHandleList* receiver_maps) {
  // Collect MONOMORPHIC stubs for all |receiver_maps|.
  CodeHandleList handlers(receiver_maps->length());
  MapHandleList transitioned_maps(receiver_maps->length());
  for (int i = 0; i < receiver_maps->length(); ++i) {
    Handle<Map> receiver_map(receiver_maps->at(i));
    Handle<Code> cached_stub;
    Handle<Map> transitioned_map =
        receiver_map->FindTransitionedMap(receiver_maps);

    // TODO(mvstanton): The code below is doing pessimistic elements
    // transitions. I would like to stop doing that and rely on Allocation Site
    // Tracking to do a better job of ensuring the data types are what they need
    // to be. Not all the elements are in place yet, pessimistic elements
    // transitions are still important for performance.
    bool is_js_array = receiver_map->instance_type() == JS_ARRAY_TYPE;
    ElementsKind elements_kind = receiver_map->elements_kind();
    if (!transitioned_map.is_null()) {
      cached_stub = ElementsTransitionAndStoreStub(
          elements_kind,
          transitioned_map->elements_kind(),
          is_js_array,
          store_mode_).GetCode(isolate());
    } else {
      if (receiver_map->has_fast_elements() ||
          receiver_map->has_external_array_elements()) {
        cached_stub = KeyedStoreFastElementStub(
            is_js_array,
            elements_kind,
            store_mode_).GetCode(isolate());
      } else {
        cached_stub = KeyedStoreElementStub(
            is_js_array,
            elements_kind,
            store_mode_).GetCode(isolate());
      }
    }
    ASSERT(!cached_stub.is_null());
    handlers.Add(cached_stub);
    transitioned_maps.Add(transitioned_map);
  }
  Handle<Code> code =
      CompileStorePolymorphic(receiver_maps, &handlers, &transitioned_maps);
  isolate()->counters()->keyed_store_polymorphic_stubs()->Increment();
  PROFILE(isolate(),
          CodeCreateEvent(Logger::KEYED_STORE_POLYMORPHIC_IC_TAG, *code, 0));
  return code;
}


void KeyedStoreStubCompiler::GenerateStoreDictionaryElement(
    MacroAssembler* masm) {
  KeyedStoreIC::GenerateSlow(masm);
}


CallStubCompiler::CallStubCompiler(Isolate* isolate,
                                   int argc,
                                   Code::Kind kind,
                                   Code::ExtraICState extra_state,
                                   InlineCacheHolderFlag cache_holder)
    : StubCompiler(isolate),
      arguments_(argc),
      kind_(kind),
      extra_state_(extra_state),
      cache_holder_(cache_holder) {
}


bool CallStubCompiler::HasCustomCallGenerator(Handle<JSFunction> function) {
  if (function->shared()->HasBuiltinFunctionId()) {
    BuiltinFunctionId id = function->shared()->builtin_function_id();
#define CALL_GENERATOR_CASE(name) if (id == k##name) return true;
    CUSTOM_CALL_IC_GENERATORS(CALL_GENERATOR_CASE)
#undef CALL_GENERATOR_CASE
  }

  CallOptimization optimization(function);
  return optimization.is_simple_api_call();
}


bool CallStubCompiler::CanBeCached(Handle<JSFunction> function) {
  if (function->shared()->HasBuiltinFunctionId()) {
    BuiltinFunctionId id = function->shared()->builtin_function_id();
#define CALL_GENERATOR_CASE(name) if (id == k##name) return false;
    SITE_SPECIFIC_CALL_GENERATORS(CALL_GENERATOR_CASE)
#undef CALL_GENERATOR_CASE
  }

  return true;
}


Handle<Code> CallStubCompiler::CompileCustomCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> fname,
    Code::StubType type) {
  ASSERT(HasCustomCallGenerator(function));

  if (function->shared()->HasBuiltinFunctionId()) {
    BuiltinFunctionId id = function->shared()->builtin_function_id();
#define CALL_GENERATOR_CASE(name)                               \
    if (id == k##name) {                                        \
      return CallStubCompiler::Compile##name##Call(object,      \
                                                   holder,      \
                                                   cell,        \
                                                   function,    \
                                                   fname,       \
                                                   type);       \
    }
    CUSTOM_CALL_IC_GENERATORS(CALL_GENERATOR_CASE)
#undef CALL_GENERATOR_CASE
  }
  CallOptimization optimization(function);
  ASSERT(optimization.is_simple_api_call());
  return CompileFastApiCall(optimization,
                            object,
                            holder,
                            cell,
                            function,
                            fname);
}


Handle<Code> CallStubCompiler::GetCode(Code::StubType type,
                                       Handle<Name> name) {
  int argc = arguments_.immediate();
  Code::Flags flags = Code::ComputeMonomorphicFlags(kind_,
                                                    extra_state_,
                                                    type,
                                                    argc,
                                                    cache_holder_);
  return GetCodeWithFlags(flags, name);
}


Handle<Code> CallStubCompiler::GetCode(Handle<JSFunction> function) {
  Handle<String> function_name;
  if (function->shared()->name()->IsString()) {
    function_name = Handle<String>(String::cast(function->shared()->name()));
  }
  return GetCode(Code::CONSTANT, function_name);
}


CallOptimization::CallOptimization(LookupResult* lookup) {
  if (lookup->IsFound() &&
      lookup->IsCacheable() &&
      lookup->IsConstantFunction()) {
    // We only optimize constant function calls.
    Initialize(Handle<JSFunction>(lookup->GetConstantFunction()));
  } else {
    Initialize(Handle<JSFunction>::null());
  }
}


CallOptimization::CallOptimization(Handle<JSFunction> function) {
  Initialize(function);
}


int CallOptimization::GetPrototypeDepthOfExpectedType(
    Handle<JSObject> object,
    Handle<JSObject> holder) const {
  ASSERT(is_simple_api_call());
  if (expected_receiver_type_.is_null()) return 0;
  int depth = 0;
  while (!object.is_identical_to(holder)) {
    if (object->IsInstanceOf(*expected_receiver_type_)) return depth;
    object = Handle<JSObject>(JSObject::cast(object->GetPrototype()));
    if (!object->map()->is_hidden_prototype()) return kInvalidProtoDepth;
    ++depth;
  }
  if (holder->IsInstanceOf(*expected_receiver_type_)) return depth;
  return kInvalidProtoDepth;
}


void CallOptimization::Initialize(Handle<JSFunction> function) {
  constant_function_ = Handle<JSFunction>::null();
  is_simple_api_call_ = false;
  expected_receiver_type_ = Handle<FunctionTemplateInfo>::null();
  api_call_info_ = Handle<CallHandlerInfo>::null();

  if (function.is_null() || !function->is_compiled()) return;

  constant_function_ = function;
  AnalyzePossibleApiFunction(function);
}


void CallOptimization::AnalyzePossibleApiFunction(Handle<JSFunction> function) {
  if (!function->shared()->IsApiFunction()) return;
  Handle<FunctionTemplateInfo> info(function->shared()->get_api_func_data());

  // Require a C++ callback.
  if (info->call_code()->IsUndefined()) return;
  api_call_info_ =
      Handle<CallHandlerInfo>(CallHandlerInfo::cast(info->call_code()));

  // Accept signatures that either have no restrictions at all or
  // only have restrictions on the receiver.
  if (!info->signature()->IsUndefined()) {
    Handle<SignatureInfo> signature =
        Handle<SignatureInfo>(SignatureInfo::cast(info->signature()));
    if (!signature->args()->IsUndefined()) return;
    if (!signature->receiver()->IsUndefined()) {
      expected_receiver_type_ =
          Handle<FunctionTemplateInfo>(
              FunctionTemplateInfo::cast(signature->receiver()));
    }
  }

  is_simple_api_call_ = true;
}


} }  // namespace v8::internal
