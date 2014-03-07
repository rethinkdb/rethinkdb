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

#include "code-stubs.h"
#include "hydrogen.h"
#include "lithium.h"

namespace v8 {
namespace internal {


static LChunk* OptimizeGraph(HGraph* graph) {
  DisallowHeapAllocation no_allocation;
  DisallowHandleAllocation no_handles;
  DisallowHandleDereference no_deref;

  ASSERT(graph != NULL);
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
        context_(NULL) {
    descriptor_ = stub->GetInterfaceDescriptor(isolate);
    parameters_.Reset(new HParameter*[descriptor_->register_param_count_]);
  }
  virtual bool BuildGraph();

 protected:
  virtual HValue* BuildCodeStub() = 0;
  HParameter* GetParameter(int parameter) {
    ASSERT(parameter < descriptor_->register_param_count_);
    return parameters_[parameter];
  }
  HValue* GetArgumentsLength() {
    // This is initialized in BuildGraph()
    ASSERT(arguments_length_ != NULL);
    return arguments_length_;
  }
  CompilationInfo* info() { return &info_; }
  HydrogenCodeStub* stub() { return info_.code_stub(); }
  HContext* context() { return context_; }
  Isolate* isolate() { return info_.isolate(); }

  class ArrayContextChecker {
   public:
    ArrayContextChecker(HGraphBuilder* builder, HValue* constructor,
                        HValue* array_function)
        : checker_(builder) {
      checker_.If<HCompareObjectEqAndBranch, HValue*>(constructor,
                                                      array_function);
      checker_.Then();
    }

    ~ArrayContextChecker() {
      checker_.ElseDeopt("Array constructor called from different context");
      checker_.End();
    }
   private:
    IfBuilder checker_;
  };

  enum ArgumentClass {
    NONE,
    SINGLE,
    MULTIPLE
  };

  HValue* BuildArrayConstructor(ElementsKind kind,
                                ContextCheckMode context_mode,
                                AllocationSiteOverrideMode override_mode,
                                ArgumentClass argument_class);
  HValue* BuildInternalArrayConstructor(ElementsKind kind,
                                        ArgumentClass argument_class);

  void BuildInstallOptimizedCode(HValue* js_function, HValue* native_context,
                                 HValue* code_object);
  void BuildInstallCode(HValue* js_function, HValue* shared_info);
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
  CodeStubInterfaceDescriptor* descriptor_;
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

  int param_count = descriptor_->register_param_count_;
  HEnvironment* start_environment = graph()->start_environment();
  HBasicBlock* next_block = CreateBasicBlock(start_environment);
  Goto(next_block);
  next_block->SetJoinId(BailoutId::StubEntry());
  set_current_block(next_block);

  for (int i = 0; i < param_count; ++i) {
    HParameter* param =
        Add<HParameter>(i, HParameter::REGISTER_PARAMETER);
    start_environment->Bind(i, param);
    parameters_[i] = param;
  }

  HInstruction* stack_parameter_count;
  if (descriptor_->stack_parameter_count_.is_valid()) {
    ASSERT(descriptor_->environment_length() == (param_count + 1));
    stack_parameter_count = New<HParameter>(param_count,
                                            HParameter::REGISTER_PARAMETER,
                                            Representation::Integer32());
    stack_parameter_count->set_type(HType::Smi());
    // It's essential to bind this value to the environment in case of deopt.
    AddInstruction(stack_parameter_count);
    start_environment->Bind(param_count, stack_parameter_count);
    arguments_length_ = stack_parameter_count;
  } else {
    ASSERT(descriptor_->environment_length() == param_count);
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
  if (descriptor_->function_mode_ == JS_FUNCTION_STUB_MODE) {
    if (!stack_parameter_count->IsConstant() &&
        descriptor_->hint_stack_parameter_count_ < 0) {
      HInstruction* amount = graph()->GetConstant1();
      stack_pop_count = Add<HAdd>(stack_parameter_count, amount);
      stack_pop_count->ChangeRepresentation(Representation::Integer32());
      stack_pop_count->ClearFlag(HValue::kCanOverflow);
    } else {
      int count = descriptor_->hint_stack_parameter_count_;
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
  explicit CodeStubGraphBuilder(Isolate* isolate, Stub* stub)
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


Handle<Code> HydrogenCodeStub::GenerateLightweightMissCode(Isolate* isolate) {
  Factory* factory = isolate->factory();

  // Generate the new code.
  MacroAssembler masm(isolate, NULL, 256);

  {
    // Update the static counter each time a new code stub is generated.
    isolate->counters()->code_stubs()->Increment();

    // Nested stubs are not allowed for leaves.
    AllowStubCallsScope allow_scope(&masm, false);

    // Generate the code for the stub.
    masm.set_generating_stub(true);
    NoCurrentFrameScope scope(&masm);
    GenerateLightweightMiss(&masm);
  }

  // Create the code object.
  CodeDesc desc;
  masm.GetCode(&desc);

  // Copy the generated code into a heap object.
  Code::Flags flags = Code::ComputeFlags(
      GetCodeKind(),
      GetICState(),
      GetExtraICState(),
      GetStubType(),
      GetStubFlags());
  Handle<Code> new_object = factory->NewCode(
      desc, flags, masm.CodeObject(), NeedsImmovableCode());
  return new_object;
}


template <class Stub>
static Handle<Code> DoGenerateCode(Isolate* isolate, Stub* stub) {
  CodeStub::Major  major_key =
      static_cast<HydrogenCodeStub*>(stub)->MajorKey();
  CodeStubInterfaceDescriptor* descriptor =
      isolate->code_stub_interface_descriptor(major_key);
  if (descriptor->register_param_count_ < 0) {
    stub->InitializeInterfaceDescriptor(isolate, descriptor);
  }

  // If we are uninitialized we can use a light-weight stub to enter
  // the runtime that is significantly faster than using the standard
  // stub-failure deopt mechanism.
  if (stub->IsUninitialized() && descriptor->has_miss_handler()) {
    ASSERT(!descriptor->stack_parameter_count_.is_valid());
    return stub->GenerateLightweightMissCode(isolate);
  }
  ElapsedTimer timer;
  if (FLAG_profile_hydrogen_code_stub_compilation) {
    timer.Start();
  }
  CodeStubGraphBuilder<Stub> builder(isolate, stub);
  LChunk* chunk = OptimizeGraph(builder.CreateGraph());
  Handle<Code> code = chunk->Codegen();
  if (FLAG_profile_hydrogen_code_stub_compilation) {
    double ms = timer.Elapsed().InMillisecondsF();
    PrintF("[Lazy compilation of %s took %0.3f ms]\n", *stub->GetName(), ms);
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
  Add<HPushArgument>(value);
  Push(Add<HInvokeFunction>(function, 1));

  if_number.End();

  return Pop();
}


Handle<Code> ToNumberStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<NumberToStringStub>::BuildCodeStub() {
  info()->MarkAsSavesCallerDoubles();
  HValue* number = GetParameter(NumberToStringStub::kNumber);
  return BuildNumberToString(number, handle(Type::Number(), isolate()));
}


Handle<Code> NumberToStringStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<FastCloneShallowArrayStub>::BuildCodeStub() {
  Factory* factory = isolate()->factory();
  HValue* undefined = graph()->GetConstantUndefined();
  AllocationSiteMode alloc_site_mode = casted_stub()->allocation_site_mode();
  FastCloneShallowArrayStub::Mode mode = casted_stub()->mode();
  int length = casted_stub()->length();

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
  HInstruction* boilerplate = Add<HLoadNamedField>(allocation_site, access);
  HValue* push_value;
  if (mode == FastCloneShallowArrayStub::CLONE_ANY_ELEMENTS) {
    HValue* elements = AddLoadElements(boilerplate);

    IfBuilder if_fixed_cow(this);
    if_fixed_cow.If<HCompareMap>(elements, factory->fixed_cow_array_map());
    if_fixed_cow.Then();
    push_value = BuildCloneShallowArray(boilerplate,
                                        allocation_site,
                                        alloc_site_mode,
                                        FAST_ELEMENTS,
                                        0/*copy-on-write*/);
    environment()->Push(push_value);
    if_fixed_cow.Else();

    IfBuilder if_fixed(this);
    if_fixed.If<HCompareMap>(elements, factory->fixed_array_map());
    if_fixed.Then();
    push_value = BuildCloneShallowArray(boilerplate,
                                        allocation_site,
                                        alloc_site_mode,
                                        FAST_ELEMENTS,
                                        length);
    environment()->Push(push_value);
    if_fixed.Else();
    push_value = BuildCloneShallowArray(boilerplate,
                                        allocation_site,
                                        alloc_site_mode,
                                        FAST_DOUBLE_ELEMENTS,
                                        length);
    environment()->Push(push_value);
  } else {
    ElementsKind elements_kind = casted_stub()->ComputeElementsKind();
    push_value = BuildCloneShallowArray(boilerplate,
                                        allocation_site,
                                        alloc_site_mode,
                                        elements_kind,
                                        length);
    environment()->Push(push_value);
  }

  checker.ElseDeopt("Uninitialized boilerplate literals");
  checker.End();

  return environment()->Pop();
}


Handle<Code> FastCloneShallowArrayStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
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
  HInstruction* boilerplate = Add<HLoadNamedField>(allocation_site, access);

  int size = JSObject::kHeaderSize + casted_stub()->length() * kPointerSize;
  int object_size = size;
  if (FLAG_allocation_site_pretenuring) {
    size += AllocationMemento::kSize;
  }

  HValue* boilerplate_map = Add<HLoadNamedField>(
      boilerplate, HObjectAccess::ForMap());
  HValue* boilerplate_size = Add<HLoadNamedField>(
      boilerplate_map, HObjectAccess::ForMapInstanceSize());
  HValue* size_in_words = Add<HConstant>(object_size >> kPointerSizeLog2);
  checker.If<HCompareNumericAndBranch>(boilerplate_size,
                                       size_in_words, Token::EQ);
  checker.Then();

  HValue* size_in_bytes = Add<HConstant>(size);

  HInstruction* object = Add<HAllocate>(size_in_bytes, HType::JSObject(),
      isolate()->heap()->GetPretenureMode(), JS_OBJECT_TYPE);

  for (int i = 0; i < object_size; i += kPointerSize) {
    HObjectAccess access = HObjectAccess::ForJSObjectOffset(i);
    Add<HStoreNamedField>(object, access,
                          Add<HLoadNamedField>(boilerplate, access));
  }

  ASSERT(FLAG_allocation_site_pretenuring || (size == object_size));
  if (FLAG_allocation_site_pretenuring) {
    BuildCreateAllocationMemento(object, object_size, allocation_site);
  }

  environment()->Push(object);
  checker.ElseDeopt("Uninitialized boilerplate in fast clone");
  checker.End();

  return environment()->Pop();
}


Handle<Code> FastCloneShallowObjectStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
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

  // Store an empty fixed array for the code dependency.
  HConstant* empty_fixed_array =
    Add<HConstant>(isolate()->factory()->empty_fixed_array());
  HStoreNamedField* store = Add<HStoreNamedField>(
      object,
      HObjectAccess::ForAllocationSiteOffset(
          AllocationSite::kDependentCodeOffset),
      empty_fixed_array);

  // Link the object to the allocation site list
  HValue* site_list = Add<HConstant>(
      ExternalReference::allocation_sites_list_address(isolate()));
  HValue* site = Add<HLoadNamedField>(site_list,
                                      HObjectAccess::ForAllocationSiteList());
  store = Add<HStoreNamedField>(object,
      HObjectAccess::ForAllocationSiteOffset(AllocationSite::kWeakNextOffset),
      site);
  store->SkipWriteBarrier();
  Add<HStoreNamedField>(site_list, HObjectAccess::ForAllocationSiteList(),
                        object);

  // We use a hammer (SkipWriteBarrier()) to indicate that we know the input
  // cell is really a Cell, and so no write barrier is needed.
  // TODO(mvstanton): Add a debug_code check to verify the input cell is really
  // a cell. (perhaps with a new instruction, HAssert).
  HInstruction* cell = GetParameter(0);
  HObjectAccess access = HObjectAccess::ForCellValue();
  store = Add<HStoreNamedField>(cell, access, object);
  store->SkipWriteBarrier();
  return cell;
}


Handle<Code> CreateAllocationSiteStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<KeyedLoadFastElementStub>::BuildCodeStub() {
  HInstruction* load = BuildUncheckedMonomorphicElementAccess(
      GetParameter(0), GetParameter(1), NULL,
      casted_stub()->is_js_array(), casted_stub()->elements_kind(),
      false, NEVER_RETURN_HOLE, STANDARD_STORE);
  return load;
}


Handle<Code> KeyedLoadFastElementStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template<>
HValue* CodeStubGraphBuilder<LoadFieldStub>::BuildCodeStub() {
  Representation rep = casted_stub()->representation();
  HObjectAccess access = casted_stub()->is_inobject() ?
      HObjectAccess::ForJSObjectOffset(casted_stub()->offset(), rep) :
      HObjectAccess::ForBackingStoreOffset(casted_stub()->offset(), rep);
  return AddLoadNamedField(GetParameter(0), access);
}


Handle<Code> LoadFieldStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template<>
HValue* CodeStubGraphBuilder<KeyedLoadFieldStub>::BuildCodeStub() {
  Representation rep = casted_stub()->representation();
  HObjectAccess access = casted_stub()->is_inobject() ?
      HObjectAccess::ForJSObjectOffset(casted_stub()->offset(), rep) :
      HObjectAccess::ForBackingStoreOffset(casted_stub()->offset(), rep);
  return AddLoadNamedField(GetParameter(0), access);
}


Handle<Code> KeyedLoadFieldStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<KeyedStoreFastElementStub>::BuildCodeStub() {
  BuildUncheckedMonomorphicElementAccess(
      GetParameter(0), GetParameter(1), GetParameter(2),
      casted_stub()->is_js_array(), casted_stub()->elements_kind(),
      true, NEVER_RETURN_HOLE, casted_stub()->store_mode());

  return GetParameter(2);
}


Handle<Code> KeyedStoreFastElementStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<TransitionElementsKindStub>::BuildCodeStub() {
  info()->MarkAsSavesCallerDoubles();

  BuildTransitionElementsKind(GetParameter(0),
                              GetParameter(1),
                              casted_stub()->from_kind(),
                              casted_stub()->to_kind(),
                              true);

  return GetParameter(0);
}


Handle<Code> TransitionElementsKindStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}

HValue* CodeStubGraphBuilderBase::BuildArrayConstructor(
    ElementsKind kind,
    ContextCheckMode context_mode,
    AllocationSiteOverrideMode override_mode,
    ArgumentClass argument_class) {
  HValue* constructor = GetParameter(ArrayConstructorStubBase::kConstructor);
  if (context_mode == CONTEXT_CHECK_REQUIRED) {
    HInstruction* array_function = BuildGetArrayFunction();
    ArrayContextChecker checker(this, constructor, array_function);
  }

  HValue* property_cell = GetParameter(ArrayConstructorStubBase::kPropertyCell);
  // Walk through the property cell to the AllocationSite
  HValue* alloc_site = Add<HLoadNamedField>(property_cell,
                                            HObjectAccess::ForCellValue());
  JSArrayBuilder array_builder(this, kind, alloc_site, constructor,
                               override_mode);
  HValue* result = NULL;
  switch (argument_class) {
    case NONE:
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

  HConstant* max_alloc_length =
      Add<HConstant>(JSObject::kInitialMaxFastElementArray);
  const int initial_capacity = JSArray::kPreallocatedArrayElements;
  HConstant* initial_capacity_node = Add<HConstant>(initial_capacity);

  HInstruction* checked_arg = Add<HBoundsCheck>(argument, max_alloc_length);
  IfBuilder if_builder(this);
  if_builder.If<HCompareNumericAndBranch>(checked_arg, constant_zero,
                                          Token::EQ);
  if_builder.Then();
  Push(initial_capacity_node);  // capacity
  Push(constant_zero);  // length
  if_builder.Else();
  Push(checked_arg);  // capacity
  Push(checked_arg);  // length
  if_builder.End();

  // Figure out total size
  HValue* length = Pop();
  HValue* capacity = Pop();
  return array_builder->AllocateArray(capacity, length, true);
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
  bool fill_with_hole = IsFastSmiElementsKind(kind);
  HValue* new_object = array_builder->AllocateArray(checked_length,
                                                    checked_length,
                                                    fill_with_hole);
  HValue* elements = array_builder->GetElementsLocation();
  ASSERT(elements != NULL);

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
  ContextCheckMode context_mode = casted_stub()->context_mode();
  AllocationSiteOverrideMode override_mode = casted_stub()->override_mode();
  return BuildArrayConstructor(kind, context_mode, override_mode, NONE);
}


Handle<Code> ArrayNoArgumentConstructorStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<ArraySingleArgumentConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  ContextCheckMode context_mode = casted_stub()->context_mode();
  AllocationSiteOverrideMode override_mode = casted_stub()->override_mode();
  return BuildArrayConstructor(kind, context_mode, override_mode, SINGLE);
}


Handle<Code> ArraySingleArgumentConstructorStub::GenerateCode(
    Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<ArrayNArgumentsConstructorStub>::BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  ContextCheckMode context_mode = casted_stub()->context_mode();
  AllocationSiteOverrideMode override_mode = casted_stub()->override_mode();
  return BuildArrayConstructor(kind, context_mode, override_mode, MULTIPLE);
}


Handle<Code> ArrayNArgumentsConstructorStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<InternalArrayNoArgumentConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  return BuildInternalArrayConstructor(kind, NONE);
}


Handle<Code> InternalArrayNoArgumentConstructorStub::GenerateCode(
    Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<InternalArraySingleArgumentConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  return BuildInternalArrayConstructor(kind, SINGLE);
}


Handle<Code> InternalArraySingleArgumentConstructorStub::GenerateCode(
    Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<InternalArrayNArgumentsConstructorStub>::
    BuildCodeStub() {
  ElementsKind kind = casted_stub()->elements_kind();
  return BuildInternalArrayConstructor(kind, MULTIPLE);
}


Handle<Code> InternalArrayNArgumentsConstructorStub::GenerateCode(
    Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<CompareNilICStub>::BuildCodeInitializedStub() {
  Isolate* isolate = graph()->isolate();
  CompareNilICStub* stub = casted_stub();
  HIfContinuation continuation;
  Handle<Map> sentinel_map(isolate->heap()->meta_map());
  Handle<Type> type = stub->GetType(isolate, sentinel_map);
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


Handle<Code> CompareNilICStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<BinaryOpStub>::BuildCodeInitializedStub() {
  BinaryOpStub* stub = casted_stub();
  HValue* left = GetParameter(0);
  HValue* right = GetParameter(1);

  Handle<Type> left_type = stub->GetLeftType(isolate());
  Handle<Type> right_type = stub->GetRightType(isolate());
  Handle<Type> result_type = stub->GetResultType(isolate());

  ASSERT(!left_type->Is(Type::None()) && !right_type->Is(Type::None()) &&
         (stub->HasSideEffects(isolate()) || !result_type->Is(Type::None())));

  HValue* result = NULL;
  if (stub->operation() == Token::ADD &&
      (left_type->Maybe(Type::String()) || right_type->Maybe(Type::String())) &&
      !left_type->Is(Type::String()) && !right_type->Is(Type::String())) {
    // For the generic add stub a fast case for string addition is performance
    // critical.
    if (left_type->Maybe(Type::String())) {
      IfBuilder if_leftisstring(this);
      if_leftisstring.If<HIsStringAndBranch>(left);
      if_leftisstring.Then();
      {
        Push(AddInstruction(BuildBinaryOperation(
                    stub->operation(), left, right,
                    handle(Type::String(), isolate()), right_type,
                    result_type, stub->fixed_right_arg(), true)));
      }
      if_leftisstring.Else();
      {
        Push(AddInstruction(BuildBinaryOperation(
                    stub->operation(), left, right,
                    left_type, right_type, result_type,
                    stub->fixed_right_arg(), true)));
      }
      if_leftisstring.End();
      result = Pop();
    } else {
      IfBuilder if_rightisstring(this);
      if_rightisstring.If<HIsStringAndBranch>(right);
      if_rightisstring.Then();
      {
        Push(AddInstruction(BuildBinaryOperation(
                    stub->operation(), left, right,
                    left_type, handle(Type::String(), isolate()),
                    result_type, stub->fixed_right_arg(), true)));
      }
      if_rightisstring.Else();
      {
        Push(AddInstruction(BuildBinaryOperation(
                    stub->operation(), left, right,
                    left_type, right_type, result_type,
                    stub->fixed_right_arg(), true)));
      }
      if_rightisstring.End();
      result = Pop();
    }
  } else {
    result = AddInstruction(BuildBinaryOperation(
            stub->operation(), left, right,
            left_type, right_type, result_type,
            stub->fixed_right_arg(), true));
  }

  // If we encounter a generic argument, the number conversion is
  // observable, thus we cannot afford to bail out after the fact.
  if (!stub->HasSideEffects(isolate())) {
    if (result_type->Is(Type::Smi())) {
      if (stub->operation() == Token::SHR) {
        // TODO(olivf) Replace this by a SmiTagU Instruction.
        // 0x40000000: this number would convert to negative when interpreting
        // the register as signed value;
        IfBuilder if_of(this);
        if_of.IfNot<HCompareNumericAndBranch>(result,
            Add<HConstant>(static_cast<int>(SmiValuesAre32Bits()
                ? 0x80000000 : 0x40000000)), Token::EQ_STRICT);
        if_of.Then();
        if_of.ElseDeopt("UInt->Smi oveflow");
        if_of.End();
      }
    }
    result = EnforceNumberType(result, result_type);
  }

  // Reuse the double box of one of the operands if we are allowed to (i.e.
  // chained binops).
  if (stub->CanReuseDoubleBox()) {
    HValue* operand = (stub->mode() == OVERWRITE_LEFT) ? left : right;
    IfBuilder if_heap_number(this);
    if_heap_number.IfNot<HIsSmiAndBranch>(operand);
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


Handle<Code> BinaryOpStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<ToBooleanStub>::BuildCodeInitializedStub() {
  ToBooleanStub* stub = casted_stub();

  IfBuilder if_true(this);
  if_true.If<HBranch>(GetParameter(0), stub->GetTypes());
  if_true.Then();
  if_true.Return(graph()->GetConstant1());
  if_true.Else();
  if_true.End();
  return graph()->GetConstant0();
}


Handle<Code> ToBooleanStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template <>
HValue* CodeStubGraphBuilder<StoreGlobalStub>::BuildCodeInitializedStub() {
  StoreGlobalStub* stub = casted_stub();
  Handle<Object> hole(isolate()->heap()->the_hole_value(), isolate());
  Handle<Object> placeholer_value(Smi::FromInt(0), isolate());
  Handle<PropertyCell> placeholder_cell =
      isolate()->factory()->NewPropertyCell(placeholer_value);

  HParameter* receiver = GetParameter(0);
  HParameter* value = GetParameter(2);

  // Check that the map of the global has not changed: use a placeholder map
  // that will be replaced later with the global object's map.
  Handle<Map> placeholder_map = isolate()->factory()->meta_map();
  Add<HCheckMaps>(receiver, placeholder_map, top_info());

  HValue* cell = Add<HConstant>(placeholder_cell);
  HObjectAccess access(HObjectAccess::ForCellPayload(isolate()));
  HValue* cell_contents = Add<HLoadNamedField>(cell, access);

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
    HValue* hole_value = Add<HConstant>(hole);
    builder.If<HCompareObjectEqAndBranch>(cell_contents, hole_value);
    builder.Then();
    builder.Deopt("Unexpected cell contents in global store");
    builder.Else();
    Add<HStoreNamedField>(cell, access, value);
    builder.End();
  }

  return value;
}


Handle<Code> StoreGlobalStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


template<>
HValue* CodeStubGraphBuilder<ElementsTransitionAndStoreStub>::BuildCodeStub() {
  HValue* value = GetParameter(0);
  HValue* map = GetParameter(1);
  HValue* key = GetParameter(2);
  HValue* object = GetParameter(3);

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
                                           true, ALLOW_RETURN_HOLE,
                                           casted_stub()->store_mode());
  }

  return value;
}


Handle<Code> ElementsTransitionAndStoreStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


void CodeStubGraphBuilderBase::BuildInstallOptimizedCode(
    HValue* js_function,
    HValue* native_context,
    HValue* code_object) {
  Counters* counters = isolate()->counters();
  AddIncrementCounter(counters->fast_new_closure_install_optimized());

  // TODO(fschneider): Idea: store proper code pointers in the optimized code
  // map and either unmangle them on marking or do nothing as the whole map is
  // discarded on major GC anyway.
  Add<HStoreCodeEntry>(js_function, code_object);

  // Now link a function into a list of optimized functions.
  HValue* optimized_functions_list = Add<HLoadNamedField>(native_context,
      HObjectAccess::ForContextSlot(Context::OPTIMIZED_FUNCTIONS_LIST));
  Add<HStoreNamedField>(js_function,
                        HObjectAccess::ForNextFunctionLinkPointer(),
                        optimized_functions_list);

  // This store is the only one that should have a write barrier.
  Add<HStoreNamedField>(native_context,
           HObjectAccess::ForContextSlot(Context::OPTIMIZED_FUNCTIONS_LIST),
           js_function);
}


void CodeStubGraphBuilderBase::BuildInstallCode(HValue* js_function,
                                                HValue* shared_info) {
  Add<HStoreNamedField>(js_function,
                        HObjectAccess::ForNextFunctionLinkPointer(),
                        graph()->GetConstantUndefined());
  HValue* code_object = Add<HLoadNamedField>(shared_info,
                                             HObjectAccess::ForCodeOffset());
  Add<HStoreCodeEntry>(js_function, code_object);
}


void CodeStubGraphBuilderBase::BuildInstallFromOptimizedCodeMap(
    HValue* js_function,
    HValue* shared_info,
    HValue* native_context) {
  Counters* counters = isolate()->counters();
  IfBuilder is_optimized(this);
  HInstruction* optimized_map = Add<HLoadNamedField>(shared_info,
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
    Label install_optimized;
    HValue* first_context_slot = Add<HLoadNamedField>(optimized_map,
        HObjectAccess::ForFirstContextSlot());
    IfBuilder already_in(this);
    already_in.If<HCompareObjectEqAndBranch>(native_context,
                                             first_context_slot);
    already_in.Then();
    {
      HValue* code_object = Add<HLoadNamedField>(optimized_map,
        HObjectAccess::ForFirstCodeSlot());
      BuildInstallOptimizedCode(js_function, native_context, code_object);
    }
    already_in.Else();
    {
      HValue* shared_function_entry_length =
          Add<HConstant>(SharedFunctionInfo::kEntryLength);
      LoopBuilder loop_builder(this,
                               context(),
                               LoopBuilder::kPostDecrement,
                               shared_function_entry_length);
      HValue* array_length = Add<HLoadNamedField>(optimized_map,
          HObjectAccess::ForFixedArrayLength());
      HValue* key = loop_builder.BeginBody(array_length,
                                           graph()->GetConstant0(),
                                           Token::GT);
      {
        // Iterate through the rest of map backwards.
        // Do not double check first entry.
        HValue* second_entry_index =
            Add<HConstant>(SharedFunctionInfo::kSecondEntryIndex);
        IfBuilder restore_check(this);
        restore_check.If<HCompareNumericAndBranch>(key, second_entry_index,
                                                   Token::EQ);
        restore_check.Then();
        {
          // Store the unoptimized code
          BuildInstallCode(js_function, shared_info);
          loop_builder.Break();
        }
        restore_check.Else();
        {
          HValue* keyed_minus = AddUncasted<HSub>(
              key, shared_function_entry_length);
          HInstruction* keyed_lookup = Add<HLoadKeyed>(optimized_map,
              keyed_minus, static_cast<HValue*>(NULL), FAST_ELEMENTS);
          IfBuilder done_check(this);
          done_check.If<HCompareObjectEqAndBranch>(native_context,
                                                   keyed_lookup);
          done_check.Then();
          {
            // Hit: fetch the optimized code.
            HValue* keyed_plus = AddUncasted<HAdd>(
                keyed_minus, graph()->GetConstant1());
            HValue* code_object = Add<HLoadKeyed>(optimized_map,
                keyed_plus, static_cast<HValue*>(NULL), FAST_ELEMENTS);
            BuildInstallOptimizedCode(js_function, native_context, code_object);

            // Fall out of the loop
            loop_builder.Break();
          }
          done_check.Else();
          done_check.End();
        }
        restore_check.End();
      }
      loop_builder.EndBody();
    }
    already_in.End();
  }
  is_optimized.End();
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

  int map_index = Context::FunctionMapIndex(casted_stub()->language_mode(),
                                            casted_stub()->is_generator());

  // Compute the function map in the current native context and set that
  // as the map of the allocated object.
  HInstruction* native_context = BuildGetNativeContext();
  HInstruction* map_slot_value = Add<HLoadNamedField>(native_context,
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


Handle<Code> FastNewClosureStub::GenerateCode(Isolate* isolate) {
  return DoGenerateCode(isolate, this);
}


} }  // namespace v8::internal
