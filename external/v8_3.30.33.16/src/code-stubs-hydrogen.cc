// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/bailout-reason.h"
#include "src/code-stubs.h"
#include "src/field-index.h"
#include "src/hydrogen.h"
#include "src/lithium.h"

namespace v8 {
namespace internal {


static LChunk* OptimizeGraph(HGraph* graph) {
  DisallowHeapAllocation no_allocation;
  DisallowHandleAllocation no_handles;
  DisallowHandleDereference no_deref;

  DCHECK(graph != NULL);
  BailoutReason bailout_reason = kNoReason;
  if (!graph->Optimize(&bailout_reason)) {
    FATAL(GetBailoutReason(bailout_reason));
  }
  LChunk* chunk = LChunk::NewChunk(graph);
  if (chunk == NULL) {
    FATAL(GetBailoutReason(graph->info()->bailout_reason()));
  }
  return chunk;
}


class CodeStubGraphBuilderBase : public HGraphBuilder {
 public:
  CodeStubGraphBuilderBase(Isolate* isolate, HydrogenCodeStub* stub)
      : HGraphBuilder(&info_),
        arguments_length_(NULL),
        info_(stub, isolate),
        descriptor_(stub),
        context_(NULL) {
    int parameter_count = descriptor_.GetEnvironmentParameterCount();
    parameters_.Reset(new HParameter*[parameter_count]);
  }
  virtual bool BuildGraph();

 protected:
  virtual HValue* BuildCodeStub() = 0;
  HParameter* GetParameter(int parameter) {
    DCHECK(parameter < descriptor_.GetEnvironmentParameterCount());
    return parameters_[parameter];
  }
  HValue* GetArgumentsLength() {
    // This is initialized in BuildGraph()
    DCHECK(arguments_length_ != NULL);
    return arguments_length_;
  }
  CompilationInfo* info() { return &info_; }
  HydrogenCodeStub* stub() { return info_.code_stub(); }
  HContext* context() { return context_; }
  Isolate* isolate() { return info_.isolate(); }

  HLoadNamedField* BuildLoadNamedField(HValue* object,
                                       FieldIndex index);
  void BuildStoreNamedField(HValue* object, HValue* value, FieldIndex index,
                            Representation representation,
                            bool transition_to_field);

  enum ArgumentClass {
    NONE,
    SINGLE,
    MULTIPLE
  };

  HValue* UnmappedCase(HValue* elements, HValue* key);

  HValue* BuildArrayConstructor(ElementsKind kind,
                                AllocationSiteOverrideMode override_mode,
                                ArgumentClass argument_class);
  HValue* BuildInternalArrayConstructor(ElementsKind kind,
                                        ArgumentClass argument_class);

  // BuildCheckAndInstallOptimizedCode emits code to install the optimized
  // function found in the optimized code map at map_index in js_function, if
  // the function at map_index matches the given native_context. Builder is
  // left in the "Then()" state after the install.
  void BuildCheckAndInstallOptimizedCode(HValue* js_function,
                                         HValue* native_context,
                                         IfBuilder* builder,
                                         HValue* optimized_map,
                                         HValue* map_index);
  void BuildInstallCode(HValue* js_function, HValue* shared_info);

  HInstruction* LoadFromOptimizedCodeMap(HValue* optimized_map,
                                         HValue* iterator,
                                         int field_offset);
  void BuildInstallFromOptimizedCodeMap(HValue* js_function,
                                        HValue* shared_info,
                                        HValue* native_context);

 private:
  HValue* BuildArraySingleArgumentConstructor(JSArrayBuilder* builder);
  HValue* BuildArrayNArgumentsConstructor(JSArrayBuilder* builder,
                                          ElementsKind kind);

  SmartArrayPointer<HParameter*> parameters_;
  HValue* arguments_length_;
  CompilationInfoWithZone info_;
  CodeStubDescriptor descriptor_;
  HContext* context_;
};


bool CodeStubGraphBuilderBase::BuildGraph() {
  // Update the static counter each time a new code stub is generated.
  isolate()->counters()->code_stubs()->Increment();

  if (FLAG_trace_hydrogen_stubs) {
    const char* name = CodeStub::MajorName(stub()->MajorKey(), false);
    PrintF("-----------------------------------------------------------\n");
    PrintF("Compiling stub %s using hydrogen\n", name);
    isolate()->GetHTracer()->TraceCompilation(&info_);
  }

  int param_count = descriptor_.GetEnvironmentParameterCount();
  HEnvironment* start_environment = graph()->start_environment();
  HBasicBlock* next_block = CreateBasicBlock(start_environment);
  Goto(next_block);
  next_block->SetJoinId(BailoutId::StubEntry());
  set_current_block(next_block);

  bool runtime_stack_params = descriptor_.stack_parameter_count().is_valid();
  HInstruction* stack_parameter_count = NULL;
  for (int i = 0; i < param_count; ++i) {
    Representation r = descriptor_.GetEnvironmentParameterRepresentation(i);
    HParameter* param = Add<HParameter>(i,
                                        HParameter::REGISTER_PARAMETER, r);
    start_environment->Bind(i, param);
    parameters_[i] = param;
    if (descriptor_.IsEnvironmentParameterCountRegister(i)) {
      param->set_type(HType::Smi());
      stack_parameter_count = param;
      arguments_length_ = stack_parameter_count;
    }
  }

  DCHECK(!runtime_stack_params || arguments_length_ != NULL);
  if (!runtime_stack_params) {
    stack_parameter_count = graph()->GetConstantMinus1();
    arguments_length_ = graph()->GetConstant0();
  }

  context_ = Add<HContext>();
  start_environment->BindContext(context_);

  Add<HSimulate>(BailoutId::StubEntry());

  NoObservableSideEffectsScope no_effects(this);

  HValue* return_value = BuildCodeStub();

  // We might have extra expressions to pop from the stack in addition to the
  // arguments above.
  HInstruction* stack_pop_count = stack_parameter_count;
  if (descriptor_.function_mode() == JS_FUNCTION_STUB_MODE) {
    if (!stack_parameter_count->IsConstant() &&
        descriptor_.hint_stack_parameter_count() < 0) {
      HInstruction* constant_one = graph()->GetConstant1();
      stack_pop_count = AddUncasted<HAdd>(stack_parameter_count, constant_one);
      stack_pop_count->ClearFlag(HValue::kCanOverflow);
      // TODO(mvstanton): verify that stack_parameter_count+1 really fits in a
      // smi.
    } else {
      int count = descriptor_.hint_stack_parameter_count();
      stack_pop_count = Add<HConstant>(count);
    }
  }

  if (current_block() != NULL) {
    HReturn* hreturn_instruction = New<HReturn>(return_value,
                                                stack_pop_count);
    FinishCurrentBlock(hreturn_instruction);
  }
  return true;
}


template <class Stub>
class CodeStubGraphBuilder: public CodeStubGraphBuilderBase {
 public:
  CodeStubGraphBuilder(Isolate* isolate, Stub* stub)
      : CodeStubGraphBuilderBase(isolate, stub) {}

 protected:
  virtual HValue* BuildCodeStub() {
    if (casted_stub()->IsUninitialized()) {
      return BuildCodeUninitializedStub();
    } else {
      return BuildCodeInitializedStub();
    }
  }

  virtual HValue* BuildCodeInitializedStub() {
    UNIMPLEMENTED();
    return NULL;
  }

  virtual HValue* BuildCodeUninitializedStub() {
    // Force a deopt that falls back to the runtime.
    HValue* undefined = graph()->GetConstantUndefined();
    IfBuilder builder(this);
    builder.IfNot<HCompareObjectEqAndBranch, HValue*>(undefined, undefined);
    builder.Then();
    builder.ElseDeopt("Forced deopt to runtime");
    return undefined;
  }

  Stub* casted_stub() { return static_cast<Stub*>(stub()); }
};


Handle<Code> HydrogenCodeStub::GenerateLightweightMissCode(
    ExternalReference miss) {
  Factory* factory = isolate()->factory();

  // Generate the new code.
  MacroAssembler masm(isolate(), NULL, 256);

  {
    // Update the static counter each time a new code stub is generated.
    isolate()->counters()->code_stubs()->Increment();

    // Generate the code for the stub.
    masm.set_generating_stub(true);
    // TODO(yangguo): remove this once we can serialize IC stubs.
    masm.enable_serializer();
    NoCurrentFrameScope scope(&masm);
    GenerateLightweightMiss(&masm, miss);
  }

  // Create the code object.
  CodeDesc desc;
  masm.GetCode(&desc);

  // Copy the generated code into a heap object.
  Code::Flags flags = Code::ComputeFlags(
      GetCodeKind(),
      GetICState(),
      GetExtraICState(),
      GetStubType());
  Handle<Code> new_object = factory->NewCode(
      desc, flags, masm.CodeObject(), NeedsImmovableCode());
  return new_object;
}


template <class Stub>
static Handle<Code> DoGenerateCode(Stub* stub) {
  Isolate* isolate = stub->isolate();
  CodeStubDescriptor descriptor(stub);

  // If we are uninitialized we can use a light-weight stub to enter
  // the runtime that is significantly faster than using the standard
  // stub-failure deopt mechanism.
  if (stub->IsUninitialized() && descriptor.has_miss_handler()) {
    DCHECK(!descriptor.stack_parameter_count().is_valid());
    return stub->GenerateLightweightMissCode(descriptor.miss_handler());
  }
  base::ElapsedTimer timer;
  if (FLAG_profile_hydrogen_code_stub_compilation) {
    timer.Start();
  }
  CodeStubGraphBuilder<Stub> builder(isolate, stub);
  LChunk* chunk = OptimizeGraph(builder.CreateGraph());
  Handle<Code> code = chunk->Codegen();
  if (FLAG_profile_hydrogen_code_stub_compilation) {
    OFStream os(stdout);
    os << "[Lazy compilation of " << stub << " took "
       << timer.Elapsed().InMillisecondsF() << " ms]" << std::endl;
  }
  return code;
}


template <>
HValue* CodeStubGraphBuilder<ToNumberStub>::BuildCodeStub() {
  HValue* value = GetParameter(0);

  // Check if the parameter is already a SMI or heap number.
  IfBuilder if_number(this);
  if_number.If<HIsSmiAndBranch>(value);
  if_number.OrIf<HCompareMap>(value, isolate()->factory()->heap_number_map());
  if_number.Then();

  // Return the number.
  Push(value);

  if_number.Else();

  // Convert the parameter to number using the builtin.
  HValue* function = AddLoadJSBuiltin(Builtins::TO_NUMBER);
  Add<HPushArguments>(value);
  Push(Add<HInvokeFunction>(function, 1));

  if_number.End();

  return Pop();
}


Handle<Code> ToNumberStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<NumberToStringStub>::BuildCodeStub() {
  info()->MarkAsSavesCallerDoubles();
  HValue* number = GetParameter(NumberToStringStub::kNumber);
  return BuildNumberToString(number, Type::Number(zone()));
}


Handle<Code> NumberToStringStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<FastCloneShallowArrayStub>::BuildCodeStub() {
  Factory* factory = isolate()->factory();
  HValue* undefined = graph()->GetConstantUndefined();
  AllocationSiteMode alloc_site_mode = casted_stub()->allocation_site_mode();

  // This stub is very performance sensitive, the generated code must be tuned
  // so that it doesn't build and eager frame.
  info()->MarkMustNotHaveEagerFrame();

  HInstruction* allocation_site = Add<HLoadKeyed>(GetParameter(0),
                                                  GetParameter(1),
                                                  static_cast<HValue*>(NULL),
                                                  FAST_ELEMENTS);
  IfBuilder checker(this);
  checker.IfNot<HCompareObjectEqAndBranch, HValue*>(allocation_site,
                                                    undefined);
  checker.Then();

  HObjectAccess access = HObjectAccess::ForAllocationSiteOffset(
      AllocationSite::kTransitionInfoOffset);
  HInstruction* boilerplate = Add<HLoadNamedField>(
      allocation_site, static_cast<HValue*>(NULL), access);
  HValue* elements = AddLoadElements(boilerplate);
  HValue* capacity = AddLoadFixedArrayLength(elements);
  IfBuilder zero_capacity(this);
  zero_capacity.If<HCompareNumericAndBranch>(capacity, graph()->GetConstant0(),
                                           Token::EQ);
  zero_capacity.Then();
  Push(BuildCloneShallowArrayEmpty(boilerplate,
                                   allocation_site,
                                   alloc_site_mode));
  zero_capacity.Else();
  IfBuilder if_fixed_cow(this);
  if_fixed_cow.If<HCompareMap>(elements, factory->fixed_cow_array_map());
  if_fixed_cow.Then();
  Push(BuildCloneShallowArrayCow(boilerplate,
                                 allocation_site,
                                 alloc_site_mode,
                                 FAST_ELEMENTS));
  if_fixed_cow.Else();
  IfBuilder if_fixed(this);
  if_fixed.If<HCompareMap>(elements, factory->fixed_array_map());
  if_fixed.Then();
  Push(BuildCloneShallowArrayNonEmpty(boilerplate,
                                      allocation_site,
                                      alloc_site_mode,
                                      FAST_ELEMENTS));

  if_fixed.Else();
  Push(BuildCloneShallowArrayNonEmpty(boilerplate,
                                      allocation_site,
                                      alloc_site_mode,
                                      FAST_DOUBLE_ELEMENTS));
  if_fixed.End();
  if_fixed_cow.End();
  zero_capacity.End();

  checker.ElseDeopt("Uninitialized boilerplate literals");
  checker.End();

  return environment()->Pop();
}


Handle<Code> FastCloneShallowArrayStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<FastCloneShallowObjectStub>::BuildCodeStub() {
  HValue* undefined = graph()->GetConstantUndefined();

  HInstruction* allocation_site = Add<HLoadKeyed>(GetParameter(0),
                                                  GetParameter(1),
                                                  static_cast<HValue*>(NULL),
                                                  FAST_ELEMENTS);

  IfBuilder checker(this);
  checker.IfNot<HCompareObjectEqAndBranch, HValue*>(allocation_site,
                                                    undefined);
  checker.And();

  HObjectAccess access = HObjectAccess::ForAllocationSiteOffset(
      AllocationSite::kTransitionInfoOffset);
  HInstruction* boilerplate = Add<HLoadNamedField>(
      allocation_site, static_cast<HValue*>(NULL), access);

  int length = casted_stub()->length();
  if (length == 0) {
    // Empty objects have some slack added to them.
    length = JSObject::kInitialGlobalObjectUnusedPropertiesCount;
  }
  int size = JSObject::kHeaderSize + length * kPointerSize;
  int object_size = size;
  if (FLAG_allocation_site_pretenuring) {
    size += AllocationMemento::kSize;
  }

  HValue* boilerplate_map = Add<HLoadNamedField>(
      boilerplate, static_cast<HValue*>(NULL),
      HObjectAccess::ForMap());
  HValue* boilerplate_size = Add<HLoadNamedField>(
      boilerplate_map, static_cast<HValue*>(NULL),
      HObjectAccess::ForMapInstanceSize());
  HValue* size_in_words = Add<HConstant>(object_size >> kPointerSizeLog2);
  checker.If<HCompareNumericAndBranch>(boilerplate_size,
                                       size_in_words, Token::EQ);
  checker.Then();

  HValue* size_in_bytes = Add<HConstant>(size);

  HInstruction* object = Add<HAllocate>(size_in_bytes, HType::JSObject(),
      NOT_TENURED, JS_OBJECT_TYPE);

  for (int i = 0; i < object_size; i += kPointerSize) {
    HObjectAccess access = HObjectAccess::ForObservableJSObjectOffset(i);
    Add<HStoreNamedField>(
        object, access, Add<HLoadNamedField>(
            boilerplate, static_cast<HValue*>(NULL), access));
  }

  DCHECK(FLAG_allocation_site_pretenuring || (size == object_size));
  if (FLAG_allocation_site_pretenuring) {
    BuildCreateAllocationMemento(
        object, Add<HConstant>(object_size), allocation_site);
  }

  environment()->Push(object);
  checker.ElseDeopt("Uninitialized boilerplate in fast clone");
  checker.End();

  return environment()->Pop();
}


Handle<Code> FastCloneShallowObjectStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<CreateAllocationSiteStub>::BuildCodeStub() {
  HValue* size = Add<HConstant>(AllocationSite::kSize);
  HInstruction* object = Add<HAllocate>(size, HType::JSObject(), TENURED,
      JS_OBJECT_TYPE);

  // Store the map
  Handle<Map> allocation_site_map = isolate()->factory()->allocation_site_map();
  AddStoreMapConstant(object, allocation_site_map);

  // Store the payload (smi elements kind)
  HValue* initial_elements_kind = Add<HConstant>(GetInitialFastElementsKind());
  Add<HStoreNamedField>(object,
                        HObjectAccess::ForAllocationSiteOffset(
                            AllocationSite::kTransitionInfoOffset),
                        initial_elements_kind);

  // Unlike literals, constructed arrays don't have nested sites
  Add<HStoreNamedField>(object,
                        HObjectAccess::ForAllocationSiteOffset(
                            AllocationSite::kNestedSiteOffset),
                        graph()->GetConstant0());

  // Pretenuring calculation field.
  Add<HStoreNamedField>(object,
                        HObjectAccess::ForAllocationSiteOffset(
                            AllocationSite::kPretenureDataOffset),
                        graph()->GetConstant0());

  // Pretenuring memento creation count field.
  Add<HStoreNamedField>(object,
                        HObjectAccess::ForAllocationSiteOffset(
                            AllocationSite::kPretenureCreateCountOffset),
                        graph()->GetConstant0());

  // Store an empty fixed array for the code dependency.
  HConstant* empty_fixed_array =
    Add<HConstant>(isolate()->factory()->empty_fixed_array());
  Add<HStoreNamedField>(
      object,
      HObjectAccess::ForAllocationSiteOffset(
          AllocationSite::kDependentCodeOffset),
      empty_fixed_array);

  // Link the object to the allocation site list
  HValue* site_list = Add<HConstant>(
      ExternalReference::allocation_sites_list_address(isolate()));
  HValue* site = Add<HLoadNamedField>(
      site_list, static_cast<HValue*>(NULL),
      HObjectAccess::ForAllocationSiteList());
  // TODO(mvstanton): This is a store to a weak pointer, which we may want to
  // mark as such in order to skip the write barrier, once we have a unified
  // system for weakness. For now we decided to keep it like this because having
  // an initial write barrier backed store makes this pointer strong until the
  // next GC, and allocation sites are designed to survive several GCs anyway.
  Add<HStoreNamedField>(
      object,
      HObjectAccess::ForAllocationSiteOffset(AllocationSite::kWeakNextOffset),
      site);
  Add<HStoreNamedField>(site_list, HObjectAccess::ForAllocationSiteList(),
                        object);

  HInstruction* feedback_vector = GetParameter(0);
  HInstruction* slot = GetParameter(1);
  Add<HStoreKeyed>(feedback_vector, slot, object, FAST_ELEMENTS,
                   INITIALIZING_STORE);
  return feedback_vector;
}


Handle<Code> CreateAllocationSiteStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<LoadFastElementStub>::BuildCodeStub() {
  HInstruction* load = BuildUncheckedMonomorphicElementAccess(
      GetParameter(LoadDescriptor::kReceiverIndex),
      GetParameter(LoadDescriptor::kNameIndex), NULL,
      casted_stub()->is_js_array(), casted_stub()->elements_kind(), LOAD,
      NEVER_RETURN_HOLE, STANDARD_STORE);
  return load;
}


Handle<Code> LoadFastElementStub::GenerateCode() {
  return DoGenerateCode(this);
}


HLoadNamedField* CodeStubGraphBuilderBase::BuildLoadNamedField(
    HValue* object, FieldIndex index) {
  Representation representation = index.is_double()
      ? Representation::Double()
      : Representation::Tagged();
  int offset = index.offset();
  HObjectAccess access = index.is_inobject()
      ? HObjectAccess::ForObservableJSObjectOffset(offset, representation)
      : HObjectAccess::ForBackingStoreOffset(offset, representation);
  if (index.is_double()) {
    // Load the heap number.
    object = Add<HLoadNamedField>(
        object, static_cast<HValue*>(NULL),
        access.WithRepresentation(Representation::Tagged()));
    // Load the double value from it.
    access = HObjectAccess::ForHeapNumberValue();
  }
  return Add<HLoadNamedField>(object, static_cast<HValue*>(NULL), access);
}


template<>
HValue* CodeStubGraphBuilder<LoadFieldStub>::BuildCodeStub() {
  return BuildLoadNamedField(GetParameter(0), casted_stub()->index());
}


Handle<Code> LoadFieldStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<LoadConstantStub>::BuildCodeStub() {
  HValue* map = AddLoadMap(GetParameter(0), NULL);
  HObjectAccess descriptors_access = HObjectAccess::ForObservableJSObjectOffset(
      Map::kDescriptorsOffset, Representation::Tagged());
  HValue* descriptors =
      Add<HLoadNamedField>(map, static_cast<HValue*>(NULL), descriptors_access);
  HObjectAccess value_access = HObjectAccess::ForObservableJSObjectOffset(
      DescriptorArray::GetValueOffset(casted_stub()->constant_index()));
  return Add<HLoadNamedField>(descriptors, static_cast<HValue*>(NULL),
                              value_access);
}


Handle<Code> LoadConstantStub::GenerateCode() { return DoGenerateCode(this); }


HValue* CodeStubGraphBuilderBase::UnmappedCase(HValue* elements, HValue* key) {
  HValue* result;
  HInstruction* backing_store = Add<HLoadKeyed>(
      elements, graph()->GetConstant1(), static_cast<HValue*>(NULL),
      FAST_ELEMENTS, ALLOW_RETURN_HOLE);
  Add<HCheckMaps>(backing_store, isolate()->factory()->fixed_array_map());
  HValue* backing_store_length =
      Add<HLoadNamedField>(backing_store, static_cast<HValue*>(NULL),
                           HObjectAccess::ForFixedArrayLength());
  IfBuilder in_unmapped_range(this);
  in_unmapped_range.If<HCompareNumericAndBranch>(key, backing_store_length,
                                                 Token::LT);
  in_unmapped_range.Then();
  {
    result = Add<HLoadKeyed>(backing_store, key, static_cast<HValue*>(NULL),
                             FAST_HOLEY_ELEMENTS, NEVER_RETURN_HOLE);
  }
  in_unmapped_range.ElseDeopt("Outside of range");
  in_unmapped_range.End();
  return result;
}


template <>
HValue* CodeStubGraphBuilder<KeyedLoadSloppyArgumentsStub>::BuildCodeStub() {
  HValue* receiver = GetParameter(LoadDescriptor::kReceiverIndex);
  HValue* key = GetParameter(LoadDescriptor::kNameIndex);

  // Mapped arguments are actual arguments. Unmapped arguments are values added
  // to the arguments object after it was created for the call. Mapped arguments
  // are stored in the context at indexes given by elements[key + 2]. Unmapped
  // arguments are stored as regular indexed properties in the arguments array,
  // held at elements[1]. See NewSloppyArguments() in runtime.cc for a detailed
  // look at argument object construction.
  //
  // The sloppy arguments elements array has a special format:
  //
  // 0: context
  // 1: unmapped arguments array
  // 2: mapped_index0,
  // 3: mapped_index1,
  // ...
  //
  // length is 2 + min(number_of_actual_arguments, number_of_formal_arguments).
  // If key + 2 >= elements.length then attempt to look in the unmapped
  // arguments array (given by elements[1]) and return the value at key, missing
  // to the runtime if the unmapped arguments array is not a fixed array or if
  // key >= unmapped_arguments_array.length.
  //
  // Otherwise, t = elements[key + 2]. If t is the hole, then look up the value
  // in the unmapped arguments array, as described above. Otherwise, t is a Smi
  // index into the context array given at elements[0]. Return the value at
  // context[t].

  key = AddUncasted<HForceRepresentation>(key, Representation::Smi());
  IfBuilder positive_smi(this);
  positive_smi.If<HCompareNumericAndBranch>(key, graph()->GetConstant0(),
                                            Token::LT);
  positive_smi.ThenDeopt("key is negative");
  positive_smi.End();

  HValue* constant_two = Add<HConstant>(2);
  HValue* elements = AddLoadElements(receiver, static_cast<HValue*>(NULL));
  HValue* elements_length =
      Add<HLoadNamedField>(elements, static_cast<HValue*>(NULL),
                           HObjectAccess::ForFixedArrayLength());
  HValue* adjusted_length = AddUncasted<HSub>(elements_length, constant_two);
  IfBuilder in_range(this);
  in_range.If<HCompareNumericAndBranch>(key, adjusted_length, Token::LT);
  in_range.Then();
  {
    HValue* index = AddUncasted<HAdd>(key, constant_two);
    HInstruction* mapped_index =
        Add<HLoadKeyed>(elements, index, static_cast<HValue*>(NULL),
                        FAST_HOLEY_ELEMENTS, ALLOW_RETURN_HOLE);

    IfBuilder is_valid(this);
    is_valid.IfNot<HCompareObjectEqAndBranch>(mapped_index,
                                              graph()->GetConstantHole());
    is_valid.Then();
    {
      // TODO(mvstanton): I'd like to assert from this point, that if the
      // mapped_index is not the hole that it is indeed, a smi. An unnecessary
      // smi check is being emitted.
      HValue* the_context =
          Add<HLoadKeyed>(elements, graph()->GetConstant0(),
                          static_cast<HValue*>(NULL), FAST_ELEMENTS);
      DCHECK(Context::kHeaderSize == FixedArray::kHeaderSize);
      HValue* result =
          Add<HLoadKeyed>(the_context, mapped_index, static_cast<HValue*>(NULL),
                          FAST_ELEMENTS, ALLOW_RETURN_HOLE);
      environment()->Push(result);
    }
    is_valid.Else();
    {
      HValue* result = UnmappedCase(elements, key);
      environment()->Push(result);
    }
    is_valid.End();
  }
  in_range.Else();
  {
    HValue* result = UnmappedCase(elements, key);
    environment()->Push(result);
  }
  in_range.End();

  return environment()->Pop();
}


Handle<Code> KeyedLoadSloppyArgumentsStub::GenerateCode() {
  return DoGenerateCode(this);
}


void CodeStubGraphBuilderBase::BuildStoreNamedField(
    HValue* object, HValue* value, FieldIndex index,
    Representation representation, bool transition_to_field) {
  DCHECK(!index.is_double() || representation.IsDouble());
  int offset = index.offset();
  HObjectAccess access =
      index.is_inobject()
          ? HObjectAccess::ForObservableJSObjectOffset(offset, representation)
          : HObjectAccess::ForBackingStoreOffset(offset, representation);

  if (representation.IsDouble()) {
    HObjectAccess heap_number_access =
        access.WithRepresentation(Representation::Tagged());
    if (transition_to_field) {
      // The store requires a mutable HeapNumber to be allocated.
      NoObservableSideEffectsScope no_side_effects(this);
      HInstruction* heap_number_size = Add<HConstant>(HeapNumber::kSize);

      // TODO(hpayer): Allocation site pretenuring support.
      HInstruction* heap_number =
          Add<HAllocate>(heap_number_size, HType::HeapObject(), NOT_TENURED,
                         MUTABLE_HEAP_NUMBER_TYPE);
      AddStoreMapConstant(heap_number,
                          isolate()->factory()->mutable_heap_number_map());
      Add<HStoreNamedField>(heap_number, HObjectAccess::ForHeapNumberValue(),
                            value);
      // Store the new mutable heap number into the object.
      access = heap_number_access;
      value = heap_number;
    } else {
      // Load the heap number.
      object = Add<HLoadNamedField>(object, static_cast<HValue*>(NULL),
                                    heap_number_access);
      // Store the double value into it.
      access = HObjectAccess::ForHeapNumberValue();
    }
  } else if (representation.IsHeapObject()) {
    BuildCheckHeapObject(value);
  }

  Add<HStoreNamedField>(object, access, value, INITIALIZING_STORE);
}


template <>
HValue* CodeStubGraphBuilder<StoreFieldStub>::BuildCodeStub() {
  BuildStoreNamedField(GetParameter(0), GetParameter(2), casted_stub()->index(),
                       casted_stub()->representation(), false);
  return GetParameter(2);
}


Handle<Code> StoreFieldStub::GenerateCode() { return DoGenerateCode(this); }


template <>
HValue* CodeStubGraphBuilder<StoreTransitionStub>::BuildCodeStub() {
  HValue* object = GetParameter(StoreTransitionDescriptor::kReceiverIndex);

  switch (casted_stub()->store_mode()) {
    case StoreTransitionStub::ExtendStorageAndStoreMapAndValue: {
      HValue* properties =
          Add<HLoadNamedField>(object, static_cast<HValue*>(NULL),
                               HObjectAccess::ForPropertiesPointer());
      HValue* length = AddLoadFixedArrayLength(properties);
      HValue* delta =
          Add<HConstant>(static_cast<int32_t>(JSObject::kFieldsAdded));
      HValue* new_capacity = AddUncasted<HAdd>(length, delta);

      // Grow properties array.
      ElementsKind kind = FAST_ELEMENTS;
      Add<HBoundsCheck>(new_capacity,
                        Add<HConstant>((Page::kMaxRegularHeapObjectSize -
                                        FixedArray::kHeaderSize) >>
                                       ElementsKindToShiftSize(kind)));

      // Reuse this code for properties backing store allocation.
      HValue* new_properties =
          BuildAllocateAndInitializeArray(kind, new_capacity);

      BuildCopyProperties(properties, new_properties, length, new_capacity);

      Add<HStoreNamedField>(object, HObjectAccess::ForPropertiesPointer(),
                            new_properties);
    }
    // Fall through.
    case StoreTransitionStub::StoreMapAndValue:
      // Store the new value into the "extended" object.
      BuildStoreNamedField(
          object, GetParameter(StoreTransitionDescriptor::kValueIndex),
          casted_stub()->index(), casted_stub()->representation(), true);
    // Fall through.

    case StoreTransitionStub::StoreMapOnly:
      // And finally update the map.
      Add<HStoreNamedField>(object, HObjectAccess::ForMap(),
                            GetParameter(StoreTransitionDescriptor::kMapIndex));
      break;
  }
  return GetParameter(StoreTransitionDescriptor::kValueIndex);
}


Handle<Code> StoreTransitionStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<StringLengthStub>::BuildCodeStub() {
  HValue* string = BuildLoadNamedField(GetParameter(0),
      FieldIndex::ForInObjectOffset(JSValue::kValueOffset));
  return BuildLoadNamedField(string,
      FieldIndex::ForInObjectOffset(String::kLengthOffset));
}


Handle<Code> StringLengthStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<StoreFastElementStub>::BuildCodeStub() {
  BuildUncheckedMonomorphicElementAccess(
      GetParameter(StoreDescriptor::kReceiverIndex),
      GetParameter(StoreDescriptor::kNameIndex),
      GetParameter(StoreDescriptor::kValueIndex), casted_stub()->is_js_array(),
      casted_stub()->elements_kind(), STORE, NEVER_RETURN_HOLE,
      casted_stub()->store_mode());

  return GetParameter(2);
}


Handle<Code> StoreFastElementStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<TransitionElementsKindStub>::BuildCodeStub() {
  info()->MarkAsSavesCallerDoubles();

  BuildTransitionElementsKind(GetParameter(0),
                              GetParameter(1),
                              casted_stub()->from_kind(),
                              casted_stub()->to_kind(),
                              casted_stub()->is_js_array());

  return GetParameter(0);
}


Handle<Code> TransitionElementsKindStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<AllocateHeapNumberStub>::BuildCodeStub() {
  HValue* result =
      Add<HAllocate>(Add<HConstant>(HeapNumber::kSize), HType::HeapNumber(),
                     NOT_TENURED, HEAP_NUMBER_TYPE);
  AddStoreMapConstant(result, isolate()->factory()->heap_number_map());
  return result;
}


Handle<Code> AllocateHeapNumberStub::GenerateCode() {
  return DoGenerateCode(this);
}


HValue* CodeStubGraphBuilderBase::BuildArrayConstructor(
    ElementsKind kind,
    AllocationSiteOverrideMode override_mode,
    ArgumentClass argument_class) {
  HValue* constructor = GetParameter(ArrayConstructorStubBase::kConstructor);
  HValue* alloc_site = GetParameter(ArrayConstructorStubBase::kAllocationSite);
  JSArrayBuilder array_builder(this, kind, alloc_site, constructor,
                               override_mode);
  HValue* result = NULL;
  switch (argument_class) {
    case NONE:
      // This stub is very performance sensitive, the generated code must be
      // tuned so that it doesn't build and eager frame.
      info()->MarkMustNotHaveEagerFrame();
      result = array_builder.AllocateEmptyArray();
      break;
    case SINGLE:
      result = BuildArraySingleArgumentConstructor(&array_builder);
      break;
    case MULTIPLE:
      result = BuildArrayNArgumentsConstructor(&array_builder, kind);
      break;
  }

  return result;
}


HValue* CodeStubGraphBuilderBase::BuildInternalArrayConstructor(
    ElementsKind kind, ArgumentClass argument_class) {
  HValue* constructor = GetParameter(
      InternalArrayConstructorStubBase::kConstructor);
  JSArrayBuilder array_builder(this, kind, constructor);

  HValue* result = NULL;
  switch (argument_class) {
    case NONE:
      // This stub is very performance sensitive, the generated code must be
      // tuned so that it doesn't build and eager frame.
      info()->MarkMustNotHaveEagerFrame();
      result = array_builder.AllocateEmptyArray();
      break;
    case SINGLE:
      result = BuildArraySingleArgumentConstructor(&array_builder);
      break;
    case MULTIPLE:
      result = BuildArrayNArgumentsConstructor(&array_builder, kind);
      break;
  }
  return result;
}


HValue* CodeStubGraphBuilderBase::BuildArraySingleArgumentConstructor(
    JSArrayBuilder* array_builder) {
  // Smi check and range check on the input arg.
  HValue* constant_one = graph()->GetConstant1();
  HValue* constant_zero = graph()->GetConstant0();

  HInstruction* elements = Add<HArgumentsElements>(false);
  HInstruction* argument = Add<HAccessArgumentsAt>(
      elements, constant_one, constant_zero);

  return BuildAllocateArrayFromLength(array_builder, argument);
}


HValue* CodeStubGraphBuilderBase::BuildArrayNArgumentsConstructor(
    JSArrayBuilder* array_builder, ElementsKind kind) {
  // Insert a bounds check because the number of arguments might exceed
  // the kInitialMaxFastElementArray limit. This cannot happen for code
  // that was parsed, but calling via Array.apply(thisArg, [...]) might
  // trigger it.
  HValue* length = GetArgumentsLength();
  HConstant* max_alloc_length =
      Add<HConstant>(JSObject::kInitialMaxFastElementArray);
  HValue* checked_length = Add<HBoundsCheck>(length, max_alloc_length);

  // We need to fill with the hole if it's a smi array in the multi-argument
  // case because we might have to bail out while copying arguments into
  // the array because they aren't compatible with a smi array.
  // If it's a double array, no problem, and if it's fast then no
  // problem either because doubles are boxed.
  //
  // TODO(mvstanton): consider an instruction to memset fill the array
  // with zero in this case instead.
  JSArrayBuilder::FillMode fill_mode = IsFastSmiElementsKind(kind)
      ? JSArrayBuilder::FILL_WITH_HOLE
      : JSArrayBuilder::DONT_FILL_WITH_HOLE;
  HValue* new_object = array_builder->AllocateArray(checked_length,
                                                    max_alloc_length,
                                                    checked_length,
                                                    fill_mode);
  HValue* elements = array_builder->GetElementsLocation();
  DCHECK(elements != NULL);

  // Now populate the elements correctly.
  LoopBuilder builder(this,
                      context(),
                      LoopBuilder::kPostIncrement);
  HValue* start = graph()->GetConstant0();
  HValue* key = builder.BeginBody(start, checked_length, Token::LT);
  HInstruction* argument_elements = Add<HArgumentsElements>(false);
  HInstruction* argument = Add<HAccessArgumentsAt>(
      argument_elements, checked_length, key);

  Add<HStoreKeyed>(elements, key, argument, kind);
  builder.EndBody();
  return new_object;
}


template <>
HValue* CodeStubGraphBuilder<ArrayNoArgumentConstructorStub>::BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  AllocationSiteOverrideMode override_mode = casted_stub()->override_mode();
  return BuildArrayConstructor(kind, override_mode, NONE);
}


Handle<Code> ArrayNoArgumentConstructorStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<ArraySingleArgumentConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  AllocationSiteOverrideMode override_mode = casted_stub()->override_mode();
  return BuildArrayConstructor(kind, override_mode, SINGLE);
}


Handle<Code> ArraySingleArgumentConstructorStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<ArrayNArgumentsConstructorStub>::BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  AllocationSiteOverrideMode override_mode = casted_stub()->override_mode();
  return BuildArrayConstructor(kind, override_mode, MULTIPLE);
}


Handle<Code> ArrayNArgumentsConstructorStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<InternalArrayNoArgumentConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  return BuildInternalArrayConstructor(kind, NONE);
}


Handle<Code> InternalArrayNoArgumentConstructorStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<InternalArraySingleArgumentConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  return BuildInternalArrayConstructor(kind, SINGLE);
}


Handle<Code> InternalArraySingleArgumentConstructorStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<InternalArrayNArgumentsConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  return BuildInternalArrayConstructor(kind, MULTIPLE);
}


Handle<Code> InternalArrayNArgumentsConstructorStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<CompareNilICStub>::BuildCodeInitializedStub() {
  Isolate* isolate = graph()->isolate();
  CompareNilICStub* stub = casted_stub();
  HIfContinuation continuation;
  Handle<Map> sentinel_map(isolate->heap()->meta_map());
  Type* type = stub->GetType(zone(), sentinel_map);
  BuildCompareNil(GetParameter(0), type, &continuation);
  IfBuilder if_nil(this, &continuation);
  if_nil.Then();
  if (continuation.IsFalseReachable()) {
    if_nil.Else();
    if_nil.Return(graph()->GetConstant0());
  }
  if_nil.End();
  return continuation.IsTrueReachable()
      ? graph()->GetConstant1()
      : graph()->GetConstantUndefined();
}


Handle<Code> CompareNilICStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<BinaryOpICStub>::BuildCodeInitializedStub() {
  BinaryOpICState state = casted_stub()->state();

  HValue* left = GetParameter(BinaryOpICStub::kLeft);
  HValue* right = GetParameter(BinaryOpICStub::kRight);

  Type* left_type = state.GetLeftType(zone());
  Type* right_type = state.GetRightType(zone());
  Type* result_type = state.GetResultType(zone());

  DCHECK(!left_type->Is(Type::None()) && !right_type->Is(Type::None()) &&
         (state.HasSideEffects() || !result_type->Is(Type::None())));

  HValue* result = NULL;
  HAllocationMode allocation_mode(NOT_TENURED);
  if (state.op() == Token::ADD &&
      (left_type->Maybe(Type::String()) || right_type->Maybe(Type::String())) &&
      !left_type->Is(Type::String()) && !right_type->Is(Type::String())) {
    // For the generic add stub a fast case for string addition is performance
    // critical.
    if (left_type->Maybe(Type::String())) {
      IfBuilder if_leftisstring(this);
      if_leftisstring.If<HIsStringAndBranch>(left);
      if_leftisstring.Then();
      {
        Push(BuildBinaryOperation(
                    state.op(), left, right,
                    Type::String(zone()), right_type,
                    result_type, state.fixed_right_arg(),
                    allocation_mode));
      }
      if_leftisstring.Else();
      {
        Push(BuildBinaryOperation(
                    state.op(), left, right,
                    left_type, right_type, result_type,
                    state.fixed_right_arg(), allocation_mode));
      }
      if_leftisstring.End();
      result = Pop();
    } else {
      IfBuilder if_rightisstring(this);
      if_rightisstring.If<HIsStringAndBranch>(right);
      if_rightisstring.Then();
      {
        Push(BuildBinaryOperation(
                    state.op(), left, right,
                    left_type, Type::String(zone()),
                    result_type, state.fixed_right_arg(),
                    allocation_mode));
      }
      if_rightisstring.Else();
      {
        Push(BuildBinaryOperation(
                    state.op(), left, right,
                    left_type, right_type, result_type,
                    state.fixed_right_arg(), allocation_mode));
      }
      if_rightisstring.End();
      result = Pop();
    }
  } else {
    result = BuildBinaryOperation(
            state.op(), left, right,
            left_type, right_type, result_type,
            state.fixed_right_arg(), allocation_mode);
  }

  // If we encounter a generic argument, the number conversion is
  // observable, thus we cannot afford to bail out after the fact.
  if (!state.HasSideEffects()) {
    result = EnforceNumberType(result, result_type);
  }

  // Reuse the double box of one of the operands if we are allowed to (i.e.
  // chained binops).
  if (state.CanReuseDoubleBox()) {
    HValue* operand = (state.mode() == OVERWRITE_LEFT) ? left : right;
    IfBuilder if_heap_number(this);
    if_heap_number.If<HHasInstanceTypeAndBranch>(operand, HEAP_NUMBER_TYPE);
    if_heap_number.Then();
    Add<HStoreNamedField>(operand, HObjectAccess::ForHeapNumberValue(), result);
    Push(operand);
    if_heap_number.Else();
    Push(result);
    if_heap_number.End();
    result = Pop();
  }

  return result;
}


Handle<Code> BinaryOpICStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<BinaryOpWithAllocationSiteStub>::BuildCodeStub() {
  BinaryOpICState state = casted_stub()->state();

  HValue* allocation_site = GetParameter(
      BinaryOpWithAllocationSiteStub::kAllocationSite);
  HValue* left = GetParameter(BinaryOpWithAllocationSiteStub::kLeft);
  HValue* right = GetParameter(BinaryOpWithAllocationSiteStub::kRight);

  Type* left_type = state.GetLeftType(zone());
  Type* right_type = state.GetRightType(zone());
  Type* result_type = state.GetResultType(zone());
  HAllocationMode allocation_mode(allocation_site);

  return BuildBinaryOperation(state.op(), left, right,
                              left_type, right_type, result_type,
                              state.fixed_right_arg(), allocation_mode);
}


Handle<Code> BinaryOpWithAllocationSiteStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<StringAddStub>::BuildCodeInitializedStub() {
  StringAddStub* stub = casted_stub();
  StringAddFlags flags = stub->flags();
  PretenureFlag pretenure_flag = stub->pretenure_flag();

  HValue* left = GetParameter(StringAddStub::kLeft);
  HValue* right = GetParameter(StringAddStub::kRight);

  // Make sure that both arguments are strings if not known in advance.
  if ((flags & STRING_ADD_CHECK_LEFT) == STRING_ADD_CHECK_LEFT) {
    left = BuildCheckString(left);
  }
  if ((flags & STRING_ADD_CHECK_RIGHT) == STRING_ADD_CHECK_RIGHT) {
    right = BuildCheckString(right);
  }

  return BuildStringAdd(left, right, HAllocationMode(pretenure_flag));
}


Handle<Code> StringAddStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<ToBooleanStub>::BuildCodeInitializedStub() {
  ToBooleanStub* stub = casted_stub();
  HValue* true_value = NULL;
  HValue* false_value = NULL;

  switch (stub->mode()) {
    case ToBooleanStub::RESULT_AS_SMI:
      true_value = graph()->GetConstant1();
      false_value = graph()->GetConstant0();
      break;
    case ToBooleanStub::RESULT_AS_ODDBALL:
      true_value = graph()->GetConstantTrue();
      false_value = graph()->GetConstantFalse();
      break;
    case ToBooleanStub::RESULT_AS_INVERSE_ODDBALL:
      true_value = graph()->GetConstantFalse();
      false_value = graph()->GetConstantTrue();
      break;
  }

  IfBuilder if_true(this);
  if_true.If<HBranch>(GetParameter(0), stub->types());
  if_true.Then();
  if_true.Return(true_value);
  if_true.Else();
  if_true.End();
  return false_value;
}


Handle<Code> ToBooleanStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<StoreGlobalStub>::BuildCodeInitializedStub() {
  StoreGlobalStub* stub = casted_stub();
  Handle<Object> placeholer_value(Smi::FromInt(0), isolate());
  Handle<PropertyCell> placeholder_cell =
      isolate()->factory()->NewPropertyCell(placeholer_value);

  HParameter* value = GetParameter(StoreDescriptor::kValueIndex);

  if (stub->check_global()) {
    // Check that the map of the global has not changed: use a placeholder map
    // that will be replaced later with the global object's map.
    Handle<Map> placeholder_map = isolate()->factory()->meta_map();
    HValue* global = Add<HConstant>(
        StoreGlobalStub::global_placeholder(isolate()));
    Add<HCheckMaps>(global, placeholder_map);
  }

  HValue* cell = Add<HConstant>(placeholder_cell);
  HObjectAccess access(HObjectAccess::ForCellPayload(isolate()));
  HValue* cell_contents = Add<HLoadNamedField>(
      cell, static_cast<HValue*>(NULL), access);

  if (stub->is_constant()) {
    IfBuilder builder(this);
    builder.If<HCompareObjectEqAndBranch>(cell_contents, value);
    builder.Then();
    builder.ElseDeopt("Unexpected cell contents in constant global store");
    builder.End();
  } else {
    // Load the payload of the global parameter cell. A hole indicates that the
    // property has been deleted and that the store must be handled by the
    // runtime.
    IfBuilder builder(this);
    HValue* hole_value = graph()->GetConstantHole();
    builder.If<HCompareObjectEqAndBranch>(cell_contents, hole_value);
    builder.Then();
    builder.Deopt("Unexpected cell contents in global store");
    builder.Else();
    Add<HStoreNamedField>(cell, access, value);
    builder.End();
  }

  return value;
}


Handle<Code> StoreGlobalStub::GenerateCode() {
  return DoGenerateCode(this);
}


template<>
HValue* CodeStubGraphBuilder<ElementsTransitionAndStoreStub>::BuildCodeStub() {
  HValue* value = GetParameter(ElementsTransitionAndStoreStub::kValueIndex);
  HValue* map = GetParameter(ElementsTransitionAndStoreStub::kMapIndex);
  HValue* key = GetParameter(ElementsTransitionAndStoreStub::kKeyIndex);
  HValue* object = GetParameter(ElementsTransitionAndStoreStub::kObjectIndex);

  if (FLAG_trace_elements_transitions) {
    // Tracing elements transitions is the job of the runtime.
    Add<HDeoptimize>("Tracing elements transitions", Deoptimizer::EAGER);
  } else {
    info()->MarkAsSavesCallerDoubles();

    BuildTransitionElementsKind(object, map,
                                casted_stub()->from_kind(),
                                casted_stub()->to_kind(),
                                casted_stub()->is_jsarray());

    BuildUncheckedMonomorphicElementAccess(object, key, value,
                                           casted_stub()->is_jsarray(),
                                           casted_stub()->to_kind(),
                                           STORE, ALLOW_RETURN_HOLE,
                                           casted_stub()->store_mode());
  }

  return value;
}


Handle<Code> ElementsTransitionAndStoreStub::GenerateCode() {
  return DoGenerateCode(this);
}


void CodeStubGraphBuilderBase::BuildCheckAndInstallOptimizedCode(
    HValue* js_function,
    HValue* native_context,
    IfBuilder* builder,
    HValue* optimized_map,
    HValue* map_index) {
  HValue* osr_ast_id_none = Add<HConstant>(BailoutId::None().ToInt());
  HValue* context_slot = LoadFromOptimizedCodeMap(
      optimized_map, map_index, SharedFunctionInfo::kContextOffset);
  HValue* osr_ast_slot = LoadFromOptimizedCodeMap(
      optimized_map, map_index, SharedFunctionInfo::kOsrAstIdOffset);
  builder->If<HCompareObjectEqAndBranch>(native_context,
                                         context_slot);
  builder->AndIf<HCompareObjectEqAndBranch>(osr_ast_slot, osr_ast_id_none);
  builder->Then();
  HValue* code_object = LoadFromOptimizedCodeMap(optimized_map,
      map_index, SharedFunctionInfo::kCachedCodeOffset);
  // and the literals
  HValue* literals = LoadFromOptimizedCodeMap(optimized_map,
      map_index, SharedFunctionInfo::kLiteralsOffset);

  Counters* counters = isolate()->counters();
  AddIncrementCounter(counters->fast_new_closure_install_optimized());

  // TODO(fschneider): Idea: store proper code pointers in the optimized code
  // map and either unmangle them on marking or do nothing as the whole map is
  // discarded on major GC anyway.
  Add<HStoreCodeEntry>(js_function, code_object);
  Add<HStoreNamedField>(js_function, HObjectAccess::ForLiteralsPointer(),
                        literals);

  // Now link a function into a list of optimized functions.
  HValue* optimized_functions_list = Add<HLoadNamedField>(
      native_context, static_cast<HValue*>(NULL),
      HObjectAccess::ForContextSlot(Context::OPTIMIZED_FUNCTIONS_LIST));
  Add<HStoreNamedField>(js_function,
                        HObjectAccess::ForNextFunctionLinkPointer(),
                        optimized_functions_list);

  // This store is the only one that should have a write barrier.
  Add<HStoreNamedField>(native_context,
           HObjectAccess::ForContextSlot(Context::OPTIMIZED_FUNCTIONS_LIST),
           js_function);

  // The builder continues in the "then" after this function.
}


void CodeStubGraphBuilderBase::BuildInstallCode(HValue* js_function,
                                                HValue* shared_info) {
  Add<HStoreNamedField>(js_function,
                        HObjectAccess::ForNextFunctionLinkPointer(),
                        graph()->GetConstantUndefined());
  HValue* code_object = Add<HLoadNamedField>(
      shared_info, static_cast<HValue*>(NULL), HObjectAccess::ForCodeOffset());
  Add<HStoreCodeEntry>(js_function, code_object);
}


HInstruction* CodeStubGraphBuilderBase::LoadFromOptimizedCodeMap(
    HValue* optimized_map,
    HValue* iterator,
    int field_offset) {
  // By making sure to express these loads in the form [<hvalue> + constant]
  // the keyed load can be hoisted.
  DCHECK(field_offset >= 0 && field_offset < SharedFunctionInfo::kEntryLength);
  HValue* field_slot = iterator;
  if (field_offset > 0) {
    HValue* field_offset_value = Add<HConstant>(field_offset);
    field_slot = AddUncasted<HAdd>(iterator, field_offset_value);
  }
  HInstruction* field_entry = Add<HLoadKeyed>(optimized_map, field_slot,
      static_cast<HValue*>(NULL), FAST_ELEMENTS);
  return field_entry;
}


void CodeStubGraphBuilderBase::BuildInstallFromOptimizedCodeMap(
    HValue* js_function,
    HValue* shared_info,
    HValue* native_context) {
  Counters* counters = isolate()->counters();
  IfBuilder is_optimized(this);
  HInstruction* optimized_map = Add<HLoadNamedField>(
      shared_info, static_cast<HValue*>(NULL),
      HObjectAccess::ForOptimizedCodeMap());
  HValue* null_constant = Add<HConstant>(0);
  is_optimized.If<HCompareObjectEqAndBranch>(optimized_map, null_constant);
  is_optimized.Then();
  {
    BuildInstallCode(js_function, shared_info);
  }
  is_optimized.Else();
  {
    AddIncrementCounter(counters->fast_new_closure_try_optimized());
    // optimized_map points to fixed array of 3-element entries
    // (native context, optimized code, literals).
    // Map must never be empty, so check the first elements.
    HValue* first_entry_index =
        Add<HConstant>(SharedFunctionInfo::kEntriesStart);
    IfBuilder already_in(this);
    BuildCheckAndInstallOptimizedCode(js_function, native_context, &already_in,
                                      optimized_map, first_entry_index);
    already_in.Else();
    {
      // Iterate through the rest of map backwards. Do not double check first
      // entry. After the loop, if no matching optimized code was found,
      // install unoptimized code.
      // for(i = map.length() - SharedFunctionInfo::kEntryLength;
      //     i > SharedFunctionInfo::kEntriesStart;
      //     i -= SharedFunctionInfo::kEntryLength) { .. }
      HValue* shared_function_entry_length =
          Add<HConstant>(SharedFunctionInfo::kEntryLength);
      LoopBuilder loop_builder(this,
                               context(),
                               LoopBuilder::kPostDecrement,
                               shared_function_entry_length);
      HValue* array_length = Add<HLoadNamedField>(
          optimized_map, static_cast<HValue*>(NULL),
          HObjectAccess::ForFixedArrayLength());
      HValue* start_pos = AddUncasted<HSub>(array_length,
                                            shared_function_entry_length);
      HValue* slot_iterator = loop_builder.BeginBody(start_pos,
                                                     first_entry_index,
                                                     Token::GT);
      {
        IfBuilder done_check(this);
        BuildCheckAndInstallOptimizedCode(js_function, native_context,
                                          &done_check,
                                          optimized_map,
                                          slot_iterator);
        // Fall out of the loop
        loop_builder.Break();
      }
      loop_builder.EndBody();

      // If slot_iterator equals first entry index, then we failed to find and
      // install optimized code
      IfBuilder no_optimized_code_check(this);
      no_optimized_code_check.If<HCompareNumericAndBranch>(
          slot_iterator, first_entry_index, Token::EQ);
      no_optimized_code_check.Then();
      {
        // Store the unoptimized code
        BuildInstallCode(js_function, shared_info);
      }
    }
  }
}


template<>
HValue* CodeStubGraphBuilder<FastNewClosureStub>::BuildCodeStub() {
  Counters* counters = isolate()->counters();
  Factory* factory = isolate()->factory();
  HInstruction* empty_fixed_array =
      Add<HConstant>(factory->empty_fixed_array());
  HValue* shared_info = GetParameter(0);

  AddIncrementCounter(counters->fast_new_closure_total());

  // Create a new closure from the given function info in new space
  HValue* size = Add<HConstant>(JSFunction::kSize);
  HInstruction* js_function = Add<HAllocate>(size, HType::JSObject(),
                                             NOT_TENURED, JS_FUNCTION_TYPE);

  int map_index = Context::FunctionMapIndex(casted_stub()->strict_mode(),
                                            casted_stub()->kind());

  // Compute the function map in the current native context and set that
  // as the map of the allocated object.
  HInstruction* native_context = BuildGetNativeContext();
  HInstruction* map_slot_value = Add<HLoadNamedField>(
      native_context, static_cast<HValue*>(NULL),
      HObjectAccess::ForContextSlot(map_index));
  Add<HStoreNamedField>(js_function, HObjectAccess::ForMap(), map_slot_value);

  // Initialize the rest of the function.
  Add<HStoreNamedField>(js_function, HObjectAccess::ForPropertiesPointer(),
                        empty_fixed_array);
  Add<HStoreNamedField>(js_function, HObjectAccess::ForElementsPointer(),
                        empty_fixed_array);
  Add<HStoreNamedField>(js_function, HObjectAccess::ForLiteralsPointer(),
                        empty_fixed_array);
  Add<HStoreNamedField>(js_function, HObjectAccess::ForPrototypeOrInitialMap(),
                        graph()->GetConstantHole());
  Add<HStoreNamedField>(js_function,
                        HObjectAccess::ForSharedFunctionInfoPointer(),
                        shared_info);
  Add<HStoreNamedField>(js_function, HObjectAccess::ForFunctionContextPointer(),
                        context());

  // Initialize the code pointer in the function to be the one
  // found in the shared function info object.
  // But first check if there is an optimized version for our context.
  if (FLAG_cache_optimized_code) {
    BuildInstallFromOptimizedCodeMap(js_function, shared_info, native_context);
  } else {
    BuildInstallCode(js_function, shared_info);
  }

  return js_function;
}


Handle<Code> FastNewClosureStub::GenerateCode() {
  return DoGenerateCode(this);
}


template<>
HValue* CodeStubGraphBuilder<FastNewContextStub>::BuildCodeStub() {
  int length = casted_stub()->slots() + Context::MIN_CONTEXT_SLOTS;

  // Get the function.
  HParameter* function = GetParameter(FastNewContextStub::kFunction);

  // Allocate the context in new space.
  HAllocate* function_context = Add<HAllocate>(
      Add<HConstant>(length * kPointerSize + FixedArray::kHeaderSize),
      HType::HeapObject(), NOT_TENURED, FIXED_ARRAY_TYPE);

  // Set up the object header.
  AddStoreMapConstant(function_context,
                      isolate()->factory()->function_context_map());
  Add<HStoreNamedField>(function_context,
                        HObjectAccess::ForFixedArrayLength(),
                        Add<HConstant>(length));

  // Set up the fixed slots.
  Add<HStoreNamedField>(function_context,
                        HObjectAccess::ForContextSlot(Context::CLOSURE_INDEX),
                        function);
  Add<HStoreNamedField>(function_context,
                        HObjectAccess::ForContextSlot(Context::PREVIOUS_INDEX),
                        context());
  Add<HStoreNamedField>(function_context,
                        HObjectAccess::ForContextSlot(Context::EXTENSION_INDEX),
                        graph()->GetConstant0());

  // Copy the global object from the previous context.
  HValue* global_object = Add<HLoadNamedField>(
      context(), static_cast<HValue*>(NULL),
      HObjectAccess::ForContextSlot(Context::GLOBAL_OBJECT_INDEX));
  Add<HStoreNamedField>(function_context,
                        HObjectAccess::ForContextSlot(
                            Context::GLOBAL_OBJECT_INDEX),
                        global_object);

  // Initialize the rest of the slots to undefined.
  for (int i = Context::MIN_CONTEXT_SLOTS; i < length; ++i) {
    Add<HStoreNamedField>(function_context,
                          HObjectAccess::ForContextSlot(i),
                          graph()->GetConstantUndefined());
  }

  return function_context;
}


Handle<Code> FastNewContextStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<LoadDictionaryElementStub>::BuildCodeStub() {
  HValue* receiver = GetParameter(LoadDescriptor::kReceiverIndex);
  HValue* key = GetParameter(LoadDescriptor::kNameIndex);

  Add<HCheckSmi>(key);

  HValue* elements = AddLoadElements(receiver);

  HValue* hash = BuildElementIndexHash(key);

  return BuildUncheckedDictionaryElementLoad(receiver, elements, key, hash);
}


Handle<Code> LoadDictionaryElementStub::GenerateCode() {
  return DoGenerateCode(this);
}


template<>
HValue* CodeStubGraphBuilder<RegExpConstructResultStub>::BuildCodeStub() {
  // Determine the parameters.
  HValue* length = GetParameter(RegExpConstructResultStub::kLength);
  HValue* index = GetParameter(RegExpConstructResultStub::kIndex);
  HValue* input = GetParameter(RegExpConstructResultStub::kInput);

  info()->MarkMustNotHaveEagerFrame();

  return BuildRegExpConstructResult(length, index, input);
}


Handle<Code> RegExpConstructResultStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
class CodeStubGraphBuilder<KeyedLoadGenericStub>
    : public CodeStubGraphBuilderBase {
 public:
  CodeStubGraphBuilder(Isolate* isolate, KeyedLoadGenericStub* stub)
      : CodeStubGraphBuilderBase(isolate, stub) {}

 protected:
  virtual HValue* BuildCodeStub();

  void BuildElementsKindLimitCheck(HGraphBuilder::IfBuilder* if_builder,
                                   HValue* bit_field2,
                                   ElementsKind kind);

  void BuildFastElementLoad(HGraphBuilder::IfBuilder* if_builder,
                            HValue* receiver,
                            HValue* key,
                            HValue* instance_type,
                            HValue* bit_field2,
                            ElementsKind kind);

  void BuildExternalElementLoad(HGraphBuilder::IfBuilder* if_builder,
                                HValue* receiver,
                                HValue* key,
                                HValue* instance_type,
                                HValue* bit_field2,
                                ElementsKind kind);

  KeyedLoadGenericStub* casted_stub() {
    return static_cast<KeyedLoadGenericStub*>(stub());
  }
};


void CodeStubGraphBuilder<KeyedLoadGenericStub>::BuildElementsKindLimitCheck(
    HGraphBuilder::IfBuilder* if_builder, HValue* bit_field2,
    ElementsKind kind) {
  ElementsKind next_kind = static_cast<ElementsKind>(kind + 1);
  HValue* kind_limit = Add<HConstant>(
      static_cast<int>(Map::ElementsKindBits::encode(next_kind)));

  if_builder->If<HCompareNumericAndBranch>(bit_field2, kind_limit, Token::LT);
  if_builder->Then();
}


void CodeStubGraphBuilder<KeyedLoadGenericStub>::BuildFastElementLoad(
    HGraphBuilder::IfBuilder* if_builder, HValue* receiver, HValue* key,
    HValue* instance_type, HValue* bit_field2, ElementsKind kind) {
  DCHECK(!IsExternalArrayElementsKind(kind));

  BuildElementsKindLimitCheck(if_builder, bit_field2, kind);

  IfBuilder js_array_check(this);
  js_array_check.If<HCompareNumericAndBranch>(
      instance_type, Add<HConstant>(JS_ARRAY_TYPE), Token::EQ);
  js_array_check.Then();
  Push(BuildUncheckedMonomorphicElementAccess(receiver, key, NULL,
                                              true, kind,
                                              LOAD, NEVER_RETURN_HOLE,
                                              STANDARD_STORE));
  js_array_check.Else();
  Push(BuildUncheckedMonomorphicElementAccess(receiver, key, NULL,
                                              false, kind,
                                              LOAD, NEVER_RETURN_HOLE,
                                              STANDARD_STORE));
  js_array_check.End();
}


void CodeStubGraphBuilder<KeyedLoadGenericStub>::BuildExternalElementLoad(
    HGraphBuilder::IfBuilder* if_builder, HValue* receiver, HValue* key,
    HValue* instance_type, HValue* bit_field2, ElementsKind kind) {
  DCHECK(IsExternalArrayElementsKind(kind));

  BuildElementsKindLimitCheck(if_builder, bit_field2, kind);

  Push(BuildUncheckedMonomorphicElementAccess(receiver, key, NULL,
                                              false, kind,
                                              LOAD, NEVER_RETURN_HOLE,
                                              STANDARD_STORE));
}


HValue* CodeStubGraphBuilder<KeyedLoadGenericStub>::BuildCodeStub() {
  HValue* receiver = GetParameter(LoadDescriptor::kReceiverIndex);
  HValue* key = GetParameter(LoadDescriptor::kNameIndex);

  // Split into a smi/integer case and unique string case.
  HIfContinuation index_name_split_continuation(graph()->CreateBasicBlock(),
                                                graph()->CreateBasicBlock());

  BuildKeyedIndexCheck(key, &index_name_split_continuation);

  IfBuilder index_name_split(this, &index_name_split_continuation);
  index_name_split.Then();
  {
    // Key is an index (number)
    key = Pop();

    int bit_field_mask = (1 << Map::kIsAccessCheckNeeded) |
      (1 << Map::kHasIndexedInterceptor);
    BuildJSObjectCheck(receiver, bit_field_mask);

    HValue* map = Add<HLoadNamedField>(receiver, static_cast<HValue*>(NULL),
                                       HObjectAccess::ForMap());

    HValue* instance_type =
      Add<HLoadNamedField>(map, static_cast<HValue*>(NULL),
                           HObjectAccess::ForMapInstanceType());

    HValue* bit_field2 = Add<HLoadNamedField>(map,
                                              static_cast<HValue*>(NULL),
                                              HObjectAccess::ForMapBitField2());

    IfBuilder kind_if(this);
    BuildFastElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                         FAST_HOLEY_ELEMENTS);

    kind_if.Else();
    {
      BuildFastElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                           FAST_HOLEY_DOUBLE_ELEMENTS);
    }
    kind_if.Else();

    // The DICTIONARY_ELEMENTS check generates a "kind_if.Then"
    BuildElementsKindLimitCheck(&kind_if, bit_field2, DICTIONARY_ELEMENTS);
    {
      HValue* elements = AddLoadElements(receiver);

      HValue* hash = BuildElementIndexHash(key);

      Push(BuildUncheckedDictionaryElementLoad(receiver, elements, key, hash));
    }
    kind_if.Else();

    // The SLOPPY_ARGUMENTS_ELEMENTS check generates a "kind_if.Then"
    BuildElementsKindLimitCheck(&kind_if, bit_field2,
                                SLOPPY_ARGUMENTS_ELEMENTS);
    // Non-strict elements are not handled.
    Add<HDeoptimize>("non-strict elements in KeyedLoadGenericStub",
                     Deoptimizer::EAGER);
    Push(graph()->GetConstant0());

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_INT8_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_UINT8_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_INT16_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_UINT16_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_INT32_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_UINT32_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_FLOAT32_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_FLOAT64_ELEMENTS);

    kind_if.Else();
    BuildExternalElementLoad(&kind_if, receiver, key, instance_type, bit_field2,
                             EXTERNAL_UINT8_CLAMPED_ELEMENTS);

    kind_if.ElseDeopt("ElementsKind unhandled in KeyedLoadGenericStub");

    kind_if.End();
  }
  index_name_split.Else();
  {
    // Key is a unique string.
    key = Pop();

    int bit_field_mask = (1 << Map::kIsAccessCheckNeeded) |
        (1 << Map::kHasNamedInterceptor);
    BuildJSObjectCheck(receiver, bit_field_mask);

    HIfContinuation continuation;
    BuildTestForDictionaryProperties(receiver, &continuation);
    IfBuilder if_dict_properties(this, &continuation);
    if_dict_properties.Then();
    {
      //  Key is string, properties are dictionary mode
      BuildNonGlobalObjectCheck(receiver);

      HValue* properties = Add<HLoadNamedField>(
          receiver, static_cast<HValue*>(NULL),
          HObjectAccess::ForPropertiesPointer());

      HValue* hash =
          Add<HLoadNamedField>(key, static_cast<HValue*>(NULL),
          HObjectAccess::ForNameHashField());

      hash = AddUncasted<HShr>(hash, Add<HConstant>(Name::kHashShift));

      HValue* value = BuildUncheckedDictionaryElementLoad(receiver,
                                                          properties,
                                                          key,
                                                          hash);
      Push(value);
    }
    if_dict_properties.Else();
    {
      //  Key is string, properties are fast mode
      HValue* hash = BuildKeyedLookupCacheHash(receiver, key);

      ExternalReference cache_keys_ref =
          ExternalReference::keyed_lookup_cache_keys(isolate());
      HValue* cache_keys = Add<HConstant>(cache_keys_ref);

      HValue* map = Add<HLoadNamedField>(receiver, static_cast<HValue*>(NULL),
                                         HObjectAccess::ForMap());
      HValue* base_index = AddUncasted<HMul>(hash, Add<HConstant>(2));
      base_index->ClearFlag(HValue::kCanOverflow);

      HIfContinuation inline_or_runtime_continuation(
          graph()->CreateBasicBlock(), graph()->CreateBasicBlock());
      {
        IfBuilder lookup_ifs[KeyedLookupCache::kEntriesPerBucket];
        for (int probe = 0; probe < KeyedLookupCache::kEntriesPerBucket;
             ++probe) {
          IfBuilder* lookup_if = &lookup_ifs[probe];
          lookup_if->Initialize(this);
          int probe_base = probe * KeyedLookupCache::kEntryLength;
          HValue* map_index = AddUncasted<HAdd>(
              base_index,
              Add<HConstant>(probe_base + KeyedLookupCache::kMapIndex));
          map_index->ClearFlag(HValue::kCanOverflow);
          HValue* key_index = AddUncasted<HAdd>(
              base_index,
              Add<HConstant>(probe_base + KeyedLookupCache::kKeyIndex));
          key_index->ClearFlag(HValue::kCanOverflow);
          HValue* map_to_check =
              Add<HLoadKeyed>(cache_keys, map_index, static_cast<HValue*>(NULL),
                              FAST_ELEMENTS, NEVER_RETURN_HOLE, 0);
          lookup_if->If<HCompareObjectEqAndBranch>(map_to_check, map);
          lookup_if->And();
          HValue* key_to_check =
              Add<HLoadKeyed>(cache_keys, key_index, static_cast<HValue*>(NULL),
                              FAST_ELEMENTS, NEVER_RETURN_HOLE, 0);
          lookup_if->If<HCompareObjectEqAndBranch>(key_to_check, key);
          lookup_if->Then();
          {
            ExternalReference cache_field_offsets_ref =
                ExternalReference::keyed_lookup_cache_field_offsets(isolate());
            HValue* cache_field_offsets =
                Add<HConstant>(cache_field_offsets_ref);
            HValue* index = AddUncasted<HAdd>(hash, Add<HConstant>(probe));
            index->ClearFlag(HValue::kCanOverflow);
            HValue* property_index = Add<HLoadKeyed>(
                cache_field_offsets, index, static_cast<HValue*>(NULL),
                EXTERNAL_INT32_ELEMENTS, NEVER_RETURN_HOLE, 0);
            Push(property_index);
          }
          lookup_if->Else();
        }
        for (int i = 0; i < KeyedLookupCache::kEntriesPerBucket; ++i) {
          lookup_ifs[i].JoinContinuation(&inline_or_runtime_continuation);
        }
      }

      IfBuilder inline_or_runtime(this, &inline_or_runtime_continuation);
      inline_or_runtime.Then();
      {
        // Found a cached index, load property inline.
        Push(Add<HLoadFieldByIndex>(receiver, Pop()));
      }
      inline_or_runtime.Else();
      {
        // KeyedLookupCache miss; call runtime.
        Add<HPushArguments>(receiver, key);
        Push(Add<HCallRuntime>(
            isolate()->factory()->empty_string(),
            Runtime::FunctionForId(Runtime::kKeyedGetProperty), 2));
      }
      inline_or_runtime.End();
    }
    if_dict_properties.End();
  }
  index_name_split.End();

  return Pop();
}


Handle<Code> KeyedLoadGenericStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<VectorLoadStub>::BuildCodeStub() {
  HValue* receiver = GetParameter(VectorLoadICDescriptor::kReceiverIndex);
  Add<HDeoptimize>("Always deopt", Deoptimizer::EAGER);
  return receiver;
}


Handle<Code> VectorLoadStub::GenerateCode() { return DoGenerateCode(this); }


template <>
HValue* CodeStubGraphBuilder<VectorKeyedLoadStub>::BuildCodeStub() {
  HValue* receiver = GetParameter(VectorLoadICDescriptor::kReceiverIndex);
  Add<HDeoptimize>("Always deopt", Deoptimizer::EAGER);
  return receiver;
}


Handle<Code> VectorKeyedLoadStub::GenerateCode() {
  return DoGenerateCode(this);
}


Handle<Code> MegamorphicLoadStub::GenerateCode() {
  return DoGenerateCode(this);
}


template <>
HValue* CodeStubGraphBuilder<MegamorphicLoadStub>::BuildCodeStub() {
  // The return address is on the stack.
  HValue* receiver = GetParameter(LoadDescriptor::kReceiverIndex);
  HValue* name = GetParameter(LoadDescriptor::kNameIndex);

  // Probe the stub cache.
  Code::Flags flags = Code::RemoveTypeAndHolderFromFlags(
      Code::ComputeHandlerFlags(Code::LOAD_IC));
  Add<HTailCallThroughMegamorphicCache>(receiver, name, flags);

  // We never continue.
  return graph()->GetConstant0();
}
} }  // namespace v8::internal
