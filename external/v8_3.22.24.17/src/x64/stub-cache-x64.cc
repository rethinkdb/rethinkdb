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

#if V8_TARGET_ARCH_X64

#include "arguments.h"
#include "ic-inl.h"
#include "codegen.h"
#include "stub-cache.h"

namespace v8 {
namespace internal {

#define __ ACCESS_MASM(masm)


static void ProbeTable(Isolate* isolate,
                       MacroAssembler* masm,
                       Code::Flags flags,
                       StubCache::Table table,
                       Register receiver,
                       Register name,
                       // The offset is scaled by 4, based on
                       // kHeapObjectTagSize, which is two bits
                       Register offset) {
  // We need to scale up the pointer by 2 because the offset is scaled by less
  // than the pointer size.
  ASSERT(kPointerSizeLog2 == kHeapObjectTagSize + 1);
  ScaleFactor scale_factor = times_2;

  ASSERT_EQ(3 * kPointerSize, sizeof(StubCache::Entry));
  // The offset register holds the entry offset times four (due to masking
  // and shifting optimizations).
  ExternalReference key_offset(isolate->stub_cache()->key_reference(table));
  ExternalReference value_offset(isolate->stub_cache()->value_reference(table));
  Label miss;

  // Multiply by 3 because there are 3 fields per entry (name, code, map).
  __ lea(offset, Operand(offset, offset, times_2, 0));

  __ LoadAddress(kScratchRegister, key_offset);

  // Check that the key in the entry matches the name.
  // Multiply entry offset by 16 to get the entry address. Since the
  // offset register already holds the entry offset times four, multiply
  // by a further four.
  __ cmpl(name, Operand(kScratchRegister, offset, scale_factor, 0));
  __ j(not_equal, &miss);

  // Get the map entry from the cache.
  // Use key_offset + kPointerSize * 2, rather than loading map_offset.
  __ movq(kScratchRegister,
          Operand(kScratchRegister, offset, scale_factor, kPointerSize * 2));
  __ cmpq(kScratchRegister, FieldOperand(receiver, HeapObject::kMapOffset));
  __ j(not_equal, &miss);

  // Get the code entry from the cache.
  __ LoadAddress(kScratchRegister, value_offset);
  __ movq(kScratchRegister,
          Operand(kScratchRegister, offset, scale_factor, 0));

  // Check that the flags match what we're looking for.
  __ movl(offset, FieldOperand(kScratchRegister, Code::kFlagsOffset));
  __ and_(offset, Immediate(~Code::kFlagsNotUsedInLookup));
  __ cmpl(offset, Immediate(flags));
  __ j(not_equal, &miss);

#ifdef DEBUG
    if (FLAG_test_secondary_stub_cache && table == StubCache::kPrimary) {
      __ jmp(&miss);
    } else if (FLAG_test_primary_stub_cache && table == StubCache::kSecondary) {
      __ jmp(&miss);
    }
#endif

  // Jump to the first instruction in the code stub.
  __ addq(kScratchRegister, Immediate(Code::kHeaderSize - kHeapObjectTag));
  __ jmp(kScratchRegister);

  __ bind(&miss);
}


void StubCompiler::GenerateDictionaryNegativeLookup(MacroAssembler* masm,
                                                    Label* miss_label,
                                                    Register receiver,
                                                    Handle<Name> name,
                                                    Register scratch0,
                                                    Register scratch1) {
  ASSERT(name->IsUniqueName());
  ASSERT(!receiver.is(scratch0));
  Counters* counters = masm->isolate()->counters();
  __ IncrementCounter(counters->negative_lookups(), 1);
  __ IncrementCounter(counters->negative_lookups_miss(), 1);

  __ movq(scratch0, FieldOperand(receiver, HeapObject::kMapOffset));

  const int kInterceptorOrAccessCheckNeededMask =
      (1 << Map::kHasNamedInterceptor) | (1 << Map::kIsAccessCheckNeeded);

  // Bail out if the receiver has a named interceptor or requires access checks.
  __ testb(FieldOperand(scratch0, Map::kBitFieldOffset),
           Immediate(kInterceptorOrAccessCheckNeededMask));
  __ j(not_zero, miss_label);

  // Check that receiver is a JSObject.
  __ CmpInstanceType(scratch0, FIRST_SPEC_OBJECT_TYPE);
  __ j(below, miss_label);

  // Load properties array.
  Register properties = scratch0;
  __ movq(properties, FieldOperand(receiver, JSObject::kPropertiesOffset));

  // Check that the properties array is a dictionary.
  __ CompareRoot(FieldOperand(properties, HeapObject::kMapOffset),
                 Heap::kHashTableMapRootIndex);
  __ j(not_equal, miss_label);

  Label done;
  NameDictionaryLookupStub::GenerateNegativeLookup(masm,
                                                   miss_label,
                                                   &done,
                                                   properties,
                                                   name,
                                                   scratch1);
  __ bind(&done);
  __ DecrementCounter(counters->negative_lookups_miss(), 1);
}


void StubCache::GenerateProbe(MacroAssembler* masm,
                              Code::Flags flags,
                              Register receiver,
                              Register name,
                              Register scratch,
                              Register extra,
                              Register extra2,
                              Register extra3) {
  Isolate* isolate = masm->isolate();
  Label miss;
  USE(extra);   // The register extra is not used on the X64 platform.
  USE(extra2);  // The register extra2 is not used on the X64 platform.
  USE(extra3);  // The register extra2 is not used on the X64 platform.
  // Make sure that code is valid. The multiplying code relies on the
  // entry size being 3 * kPointerSize.
  ASSERT(sizeof(Entry) == 3 * kPointerSize);

  // Make sure the flags do not name a specific type.
  ASSERT(Code::ExtractTypeFromFlags(flags) == 0);

  // Make sure that there are no register conflicts.
  ASSERT(!scratch.is(receiver));
  ASSERT(!scratch.is(name));

  // Check scratch register is valid, extra and extra2 are unused.
  ASSERT(!scratch.is(no_reg));
  ASSERT(extra2.is(no_reg));
  ASSERT(extra3.is(no_reg));

  Counters* counters = masm->isolate()->counters();
  __ IncrementCounter(counters->megamorphic_stub_cache_probes(), 1);

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(receiver, &miss);

  // Get the map of the receiver and compute the hash.
  __ movl(scratch, FieldOperand(name, Name::kHashFieldOffset));
  // Use only the low 32 bits of the map pointer.
  __ addl(scratch, FieldOperand(receiver, HeapObject::kMapOffset));
  __ xor_(scratch, Immediate(flags));
  // We mask out the last two bits because they are not part of the hash and
  // they are always 01 for maps.  Also in the two 'and' instructions below.
  __ and_(scratch, Immediate((kPrimaryTableSize - 1) << kHeapObjectTagSize));

  // Probe the primary table.
  ProbeTable(isolate, masm, flags, kPrimary, receiver, name, scratch);

  // Primary miss: Compute hash for secondary probe.
  __ movl(scratch, FieldOperand(name, Name::kHashFieldOffset));
  __ addl(scratch, FieldOperand(receiver, HeapObject::kMapOffset));
  __ xor_(scratch, Immediate(flags));
  __ and_(scratch, Immediate((kPrimaryTableSize - 1) << kHeapObjectTagSize));
  __ subl(scratch, name);
  __ addl(scratch, Immediate(flags));
  __ and_(scratch, Immediate((kSecondaryTableSize - 1) << kHeapObjectTagSize));

  // Probe the secondary table.
  ProbeTable(isolate, masm, flags, kSecondary, receiver, name, scratch);

  // Cache miss: Fall-through and let caller handle the miss by
  // entering the runtime system.
  __ bind(&miss);
  __ IncrementCounter(counters->megamorphic_stub_cache_misses(), 1);
}


void StubCompiler::GenerateLoadGlobalFunctionPrototype(MacroAssembler* masm,
                                                       int index,
                                                       Register prototype) {
  // Load the global or builtins object from the current context.
  __ movq(prototype,
          Operand(rsi, Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX)));
  // Load the native context from the global or builtins object.
  __ movq(prototype,
          FieldOperand(prototype, GlobalObject::kNativeContextOffset));
  // Load the function from the native context.
  __ movq(prototype, Operand(prototype, Context::SlotOffset(index)));
  // Load the initial map.  The global functions all have initial maps.
  __ movq(prototype,
          FieldOperand(prototype, JSFunction::kPrototypeOrInitialMapOffset));
  // Load the prototype from the initial map.
  __ movq(prototype, FieldOperand(prototype, Map::kPrototypeOffset));
}


void StubCompiler::GenerateDirectLoadGlobalFunctionPrototype(
    MacroAssembler* masm,
    int index,
    Register prototype,
    Label* miss) {
  Isolate* isolate = masm->isolate();
  // Check we're still in the same context.
  __ Move(prototype, isolate->global_object());
  __ cmpq(Operand(rsi, Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX)),
          prototype);
  __ j(not_equal, miss);
  // Get the global function with the given index.
  Handle<JSFunction> function(
      JSFunction::cast(isolate->native_context()->get(index)));
  // Load its initial map. The global functions all have initial maps.
  __ Move(prototype, Handle<Map>(function->initial_map()));
  // Load the prototype from the initial map.
  __ movq(prototype, FieldOperand(prototype, Map::kPrototypeOffset));
}


void StubCompiler::GenerateLoadArrayLength(MacroAssembler* masm,
                                           Register receiver,
                                           Register scratch,
                                           Label* miss_label) {
  // Check that the receiver isn't a smi.
  __ JumpIfSmi(receiver, miss_label);

  // Check that the object is a JS array.
  __ CmpObjectType(receiver, JS_ARRAY_TYPE, scratch);
  __ j(not_equal, miss_label);

  // Load length directly from the JS array.
  __ movq(rax, FieldOperand(receiver, JSArray::kLengthOffset));
  __ ret(0);
}


// Generate code to check if an object is a string.  If the object is
// a string, the map's instance type is left in the scratch register.
static void GenerateStringCheck(MacroAssembler* masm,
                                Register receiver,
                                Register scratch,
                                Label* smi,
                                Label* non_string_object) {
  // Check that the object isn't a smi.
  __ JumpIfSmi(receiver, smi);

  // Check that the object is a string.
  __ movq(scratch, FieldOperand(receiver, HeapObject::kMapOffset));
  __ movzxbq(scratch, FieldOperand(scratch, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kNotStringTag != 0);
  __ testl(scratch, Immediate(kNotStringTag));
  __ j(not_zero, non_string_object);
}


void StubCompiler::GenerateLoadStringLength(MacroAssembler* masm,
                                            Register receiver,
                                            Register scratch1,
                                            Register scratch2,
                                            Label* miss) {
  Label check_wrapper;

  // Check if the object is a string leaving the instance type in the
  // scratch register.
  GenerateStringCheck(masm, receiver, scratch1, miss, &check_wrapper);

  // Load length directly from the string.
  __ movq(rax, FieldOperand(receiver, String::kLengthOffset));
  __ ret(0);

  // Check if the object is a JSValue wrapper.
  __ bind(&check_wrapper);
  __ cmpl(scratch1, Immediate(JS_VALUE_TYPE));
  __ j(not_equal, miss);

  // Check if the wrapped value is a string and load the length
  // directly if it is.
  __ movq(scratch2, FieldOperand(receiver, JSValue::kValueOffset));
  GenerateStringCheck(masm, scratch2, scratch1, miss, miss);
  __ movq(rax, FieldOperand(scratch2, String::kLengthOffset));
  __ ret(0);
}


void StubCompiler::GenerateLoadFunctionPrototype(MacroAssembler* masm,
                                                 Register receiver,
                                                 Register result,
                                                 Register scratch,
                                                 Label* miss_label) {
  __ TryGetFunctionPrototype(receiver, result, miss_label);
  if (!result.is(rax)) __ movq(rax, result);
  __ ret(0);
}


void StubCompiler::GenerateFastPropertyLoad(MacroAssembler* masm,
                                            Register dst,
                                            Register src,
                                            bool inobject,
                                            int index,
                                            Representation representation) {
  ASSERT(!FLAG_track_double_fields || !representation.IsDouble());
  int offset = index * kPointerSize;
  if (!inobject) {
    // Calculate the offset into the properties array.
    offset = offset + FixedArray::kHeaderSize;
    __ movq(dst, FieldOperand(src, JSObject::kPropertiesOffset));
    src = dst;
  }
  __ movq(dst, FieldOperand(src, offset));
}


static void PushInterceptorArguments(MacroAssembler* masm,
                                     Register receiver,
                                     Register holder,
                                     Register name,
                                     Handle<JSObject> holder_obj) {
  STATIC_ASSERT(StubCache::kInterceptorArgsNameIndex == 0);
  STATIC_ASSERT(StubCache::kInterceptorArgsInfoIndex == 1);
  STATIC_ASSERT(StubCache::kInterceptorArgsThisIndex == 2);
  STATIC_ASSERT(StubCache::kInterceptorArgsHolderIndex == 3);
  STATIC_ASSERT(StubCache::kInterceptorArgsLength == 4);
  __ push(name);
  Handle<InterceptorInfo> interceptor(holder_obj->GetNamedInterceptor());
  ASSERT(!masm->isolate()->heap()->InNewSpace(*interceptor));
  __ Move(kScratchRegister, interceptor);
  __ push(kScratchRegister);
  __ push(receiver);
  __ push(holder);
}


static void CompileCallLoadPropertyWithInterceptor(
    MacroAssembler* masm,
    Register receiver,
    Register holder,
    Register name,
    Handle<JSObject> holder_obj) {
  PushInterceptorArguments(masm, receiver, holder, name, holder_obj);

  ExternalReference ref =
      ExternalReference(IC_Utility(IC::kLoadPropertyWithInterceptorOnly),
                        masm->isolate());
  __ Set(rax, StubCache::kInterceptorArgsLength);
  __ LoadAddress(rbx, ref);

  CEntryStub stub(1);
  __ CallStub(&stub);
}


// Number of pointers to be reserved on stack for fast API call.
static const int kFastApiCallArguments = FunctionCallbackArguments::kArgsLength;


// Reserves space for the extra arguments to API function in the
// caller's frame.
//
// These arguments are set by CheckPrototypes and GenerateFastApiCall.
static void ReserveSpaceForFastApiCall(MacroAssembler* masm, Register scratch) {
  // ----------- S t a t e -------------
  //  -- rsp[0] : return address
  //  -- rsp[8] : last argument in the internal frame of the caller
  // -----------------------------------
  __ movq(scratch, StackOperandForReturnAddress(0));
  __ subq(rsp, Immediate(kFastApiCallArguments * kPointerSize));
  __ movq(StackOperandForReturnAddress(0), scratch);
  __ Move(scratch, Smi::FromInt(0));
  StackArgumentsAccessor args(rsp, kFastApiCallArguments,
                              ARGUMENTS_DONT_CONTAIN_RECEIVER);
  for (int i = 0; i < kFastApiCallArguments; i++) {
     __ movq(args.GetArgumentOperand(i), scratch);
  }
}


// Undoes the effects of ReserveSpaceForFastApiCall.
static void FreeSpaceForFastApiCall(MacroAssembler* masm, Register scratch) {
  // ----------- S t a t e -------------
  //  -- rsp[0]                             : return address.
  //  -- rsp[8]                             : last fast api call extra argument.
  //  -- ...
  //  -- rsp[kFastApiCallArguments * 8]     : first fast api call extra
  //                                          argument.
  //  -- rsp[kFastApiCallArguments * 8 + 8] : last argument in the internal
  //                                          frame.
  // -----------------------------------
  __ movq(scratch, StackOperandForReturnAddress(0));
  __ movq(StackOperandForReturnAddress(kFastApiCallArguments * kPointerSize),
          scratch);
  __ addq(rsp, Immediate(kPointerSize * kFastApiCallArguments));
}


// Generates call to API function.
static void GenerateFastApiCall(MacroAssembler* masm,
                                const CallOptimization& optimization,
                                int argc,
                                bool restore_context) {
  // ----------- S t a t e -------------
  //  -- rsp[0]              : return address
  //  -- rsp[8] - rsp[56]    : FunctionCallbackInfo, incl.
  //                         :  object passing the type check
  //                            (set by CheckPrototypes)
  //  -- rsp[64]             : last argument
  //  -- ...
  //  -- rsp[(argc + 7) * 8] : first argument
  //  -- rsp[(argc + 8) * 8] : receiver
  // -----------------------------------
  typedef FunctionCallbackArguments FCA;
  StackArgumentsAccessor args(rsp, argc + kFastApiCallArguments);

  // Save calling context.
  int offset = argc + kFastApiCallArguments;
  __ movq(args.GetArgumentOperand(offset - FCA::kContextSaveIndex), rsi);

  // Get the function and setup the context.
  Handle<JSFunction> function = optimization.constant_function();
  __ Move(rdi, function);
  __ movq(rsi, FieldOperand(rdi, JSFunction::kContextOffset));
  // Construct the FunctionCallbackInfo on the stack.
  __ movq(args.GetArgumentOperand(offset - FCA::kCalleeIndex), rdi);
  Handle<CallHandlerInfo> api_call_info = optimization.api_call_info();
  Handle<Object> call_data(api_call_info->data(), masm->isolate());
  if (masm->isolate()->heap()->InNewSpace(*call_data)) {
    __ Move(rcx, api_call_info);
    __ movq(rbx, FieldOperand(rcx, CallHandlerInfo::kDataOffset));
    __ movq(args.GetArgumentOperand(offset - FCA::kDataIndex), rbx);
  } else {
    __ Move(args.GetArgumentOperand(offset - FCA::kDataIndex), call_data);
  }
  __ movq(kScratchRegister,
          ExternalReference::isolate_address(masm->isolate()));
  __ movq(args.GetArgumentOperand(offset - FCA::kIsolateIndex),
          kScratchRegister);
  __ LoadRoot(kScratchRegister, Heap::kUndefinedValueRootIndex);
  __ movq(args.GetArgumentOperand(offset - FCA::kReturnValueDefaultValueIndex),
          kScratchRegister);
  __ movq(args.GetArgumentOperand(offset - FCA::kReturnValueOffset),
          kScratchRegister);

  // Prepare arguments.
  STATIC_ASSERT(kFastApiCallArguments == 7);
  __ lea(rbx, Operand(rsp, 1 * kPointerSize));

  // Function address is a foreign pointer outside V8's heap.
  Address function_address = v8::ToCData<Address>(api_call_info->callback());

  // Allocate the v8::Arguments structure in the arguments' space since
  // it's not controlled by GC.
  const int kApiStackSpace = 4;

  __ PrepareCallApiFunction(kApiStackSpace);

  __ movq(StackSpaceOperand(0), rbx);  // FunctionCallbackInfo::implicit_args_.
  __ addq(rbx, Immediate((argc + kFastApiCallArguments - 1) * kPointerSize));
  __ movq(StackSpaceOperand(1), rbx);  // FunctionCallbackInfo::values_.
  __ Set(StackSpaceOperand(2), argc);  // FunctionCallbackInfo::length_.
  // FunctionCallbackInfo::is_construct_call_.
  __ Set(StackSpaceOperand(3), 0);

#if defined(__MINGW64__) || defined(_WIN64)
  Register arguments_arg = rcx;
  Register callback_arg = rdx;
#else
  Register arguments_arg = rdi;
  Register callback_arg = rsi;
#endif

  // v8::InvocationCallback's argument.
  __ lea(arguments_arg, StackSpaceOperand(0));

  Address thunk_address = FUNCTION_ADDR(&InvokeFunctionCallback);

  StackArgumentsAccessor args_from_rbp(rbp, kFastApiCallArguments,
                                       ARGUMENTS_DONT_CONTAIN_RECEIVER);
  Operand context_restore_operand = args_from_rbp.GetArgumentOperand(
      kFastApiCallArguments - 1 - FCA::kContextSaveIndex);
  Operand return_value_operand = args_from_rbp.GetArgumentOperand(
      kFastApiCallArguments - 1 - FCA::kReturnValueOffset);
  __ CallApiFunctionAndReturn(
      function_address,
      thunk_address,
      callback_arg,
      argc + kFastApiCallArguments + 1,
      return_value_operand,
      restore_context ? &context_restore_operand : NULL);
}


// Generate call to api function.
static void GenerateFastApiCall(MacroAssembler* masm,
                                const CallOptimization& optimization,
                                Register receiver,
                                Register scratch,
                                int argc,
                                Register* values) {
  ASSERT(optimization.is_simple_api_call());
  ASSERT(!receiver.is(scratch));

  const int fast_api_call_argc = argc + kFastApiCallArguments;
  StackArgumentsAccessor args(rsp, fast_api_call_argc);
  // argc + 1 is the argument number before FastApiCall arguments, 1 ~ receiver
  const int kHolderIndex = argc + 1 +
      kFastApiCallArguments - 1 - FunctionCallbackArguments::kHolderIndex;
  __ movq(scratch, StackOperandForReturnAddress(0));
  // Assign stack space for the call arguments and receiver.
  __ subq(rsp, Immediate((fast_api_call_argc + 1) * kPointerSize));
  __ movq(StackOperandForReturnAddress(0), scratch);
  // Write holder to stack frame.
  __ movq(args.GetArgumentOperand(kHolderIndex), receiver);
  __ movq(args.GetReceiverOperand(), receiver);
  // Write the arguments to stack frame.
  for (int i = 0; i < argc; i++) {
    ASSERT(!receiver.is(values[i]));
    ASSERT(!scratch.is(values[i]));
    __ movq(args.GetArgumentOperand(i + 1), values[i]);
  }

  GenerateFastApiCall(masm, optimization, argc, true);
}


class CallInterceptorCompiler BASE_EMBEDDED {
 public:
  CallInterceptorCompiler(StubCompiler* stub_compiler,
                          const ParameterCount& arguments,
                          Register name,
                          Code::ExtraICState extra_ic_state)
      : stub_compiler_(stub_compiler),
        arguments_(arguments),
        name_(name),
        extra_ic_state_(extra_ic_state) {}

  void Compile(MacroAssembler* masm,
               Handle<JSObject> object,
               Handle<JSObject> holder,
               Handle<Name> name,
               LookupResult* lookup,
               Register receiver,
               Register scratch1,
               Register scratch2,
               Register scratch3,
               Label* miss) {
    ASSERT(holder->HasNamedInterceptor());
    ASSERT(!holder->GetNamedInterceptor()->getter()->IsUndefined());

    // Check that the receiver isn't a smi.
    __ JumpIfSmi(receiver, miss);

    CallOptimization optimization(lookup);
    if (optimization.is_constant_call()) {
      CompileCacheable(masm, object, receiver, scratch1, scratch2, scratch3,
                       holder, lookup, name, optimization, miss);
    } else {
      CompileRegular(masm, object, receiver, scratch1, scratch2, scratch3,
                     name, holder, miss);
    }
  }

 private:
  void CompileCacheable(MacroAssembler* masm,
                        Handle<JSObject> object,
                        Register receiver,
                        Register scratch1,
                        Register scratch2,
                        Register scratch3,
                        Handle<JSObject> interceptor_holder,
                        LookupResult* lookup,
                        Handle<Name> name,
                        const CallOptimization& optimization,
                        Label* miss_label) {
    ASSERT(optimization.is_constant_call());
    ASSERT(!lookup->holder()->IsGlobalObject());

    int depth1 = kInvalidProtoDepth;
    int depth2 = kInvalidProtoDepth;
    bool can_do_fast_api_call = false;
    if (optimization.is_simple_api_call() &&
        !lookup->holder()->IsGlobalObject()) {
      depth1 = optimization.GetPrototypeDepthOfExpectedType(
          object, interceptor_holder);
      if (depth1 == kInvalidProtoDepth) {
        depth2 = optimization.GetPrototypeDepthOfExpectedType(
            interceptor_holder, Handle<JSObject>(lookup->holder()));
      }
      can_do_fast_api_call =
          depth1 != kInvalidProtoDepth || depth2 != kInvalidProtoDepth;
    }

    Counters* counters = masm->isolate()->counters();
    __ IncrementCounter(counters->call_const_interceptor(), 1);

    if (can_do_fast_api_call) {
      __ IncrementCounter(counters->call_const_interceptor_fast_api(), 1);
      ReserveSpaceForFastApiCall(masm, scratch1);
    }

    // Check that the maps from receiver to interceptor's holder
    // haven't changed and thus we can invoke interceptor.
    Label miss_cleanup;
    Label* miss = can_do_fast_api_call ? &miss_cleanup : miss_label;
    Register holder =
        stub_compiler_->CheckPrototypes(object, receiver, interceptor_holder,
                                        scratch1, scratch2, scratch3,
                                        name, depth1, miss);

    // Invoke an interceptor and if it provides a value,
    // branch to |regular_invoke|.
    Label regular_invoke;
    LoadWithInterceptor(masm, receiver, holder, interceptor_holder,
                        &regular_invoke);

    // Interceptor returned nothing for this property.  Try to use cached
    // constant function.

    // Check that the maps from interceptor's holder to constant function's
    // holder haven't changed and thus we can use cached constant function.
    if (*interceptor_holder != lookup->holder()) {
      stub_compiler_->CheckPrototypes(interceptor_holder, receiver,
                                      Handle<JSObject>(lookup->holder()),
                                      scratch1, scratch2, scratch3,
                                      name, depth2, miss);
    } else {
      // CheckPrototypes has a side effect of fetching a 'holder'
      // for API (object which is instanceof for the signature).  It's
      // safe to omit it here, as if present, it should be fetched
      // by the previous CheckPrototypes.
      ASSERT(depth2 == kInvalidProtoDepth);
    }

    // Invoke function.
    if (can_do_fast_api_call) {
      GenerateFastApiCall(masm, optimization, arguments_.immediate(), false);
    } else {
      CallKind call_kind = CallICBase::Contextual::decode(extra_ic_state_)
          ? CALL_AS_FUNCTION
          : CALL_AS_METHOD;
      Handle<JSFunction> fun = optimization.constant_function();
      ParameterCount expected(fun);
      __ InvokeFunction(fun, expected, arguments_,
                        JUMP_FUNCTION, NullCallWrapper(), call_kind);
    }

    // Deferred code for fast API call case---clean preallocated space.
    if (can_do_fast_api_call) {
      __ bind(&miss_cleanup);
      FreeSpaceForFastApiCall(masm, scratch1);
      __ jmp(miss_label);
    }

    // Invoke a regular function.
    __ bind(&regular_invoke);
    if (can_do_fast_api_call) {
      FreeSpaceForFastApiCall(masm, scratch1);
    }
  }

  void CompileRegular(MacroAssembler* masm,
                      Handle<JSObject> object,
                      Register receiver,
                      Register scratch1,
                      Register scratch2,
                      Register scratch3,
                      Handle<Name> name,
                      Handle<JSObject> interceptor_holder,
                      Label* miss_label) {
    Register holder =
        stub_compiler_->CheckPrototypes(object, receiver, interceptor_holder,
                                        scratch1, scratch2, scratch3,
                                        name, miss_label);

    FrameScope scope(masm, StackFrame::INTERNAL);
    // Save the name_ register across the call.
    __ push(name_);

    PushInterceptorArguments(masm, receiver, holder, name_, interceptor_holder);

    __ CallExternalReference(
        ExternalReference(IC_Utility(IC::kLoadPropertyWithInterceptorForCall),
                          masm->isolate()),
        StubCache::kInterceptorArgsLength);

    // Restore the name_ register.
    __ pop(name_);

    // Leave the internal frame.
  }

  void LoadWithInterceptor(MacroAssembler* masm,
                           Register receiver,
                           Register holder,
                           Handle<JSObject> holder_obj,
                           Label* interceptor_succeeded) {
    {
      FrameScope scope(masm, StackFrame::INTERNAL);
      __ push(holder);  // Save the holder.
      __ push(name_);  // Save the name.

      CompileCallLoadPropertyWithInterceptor(masm,
                                             receiver,
                                             holder,
                                             name_,
                                             holder_obj);

      __ pop(name_);  // Restore the name.
      __ pop(receiver);  // Restore the holder.
      // Leave the internal frame.
    }

    __ CompareRoot(rax, Heap::kNoInterceptorResultSentinelRootIndex);
    __ j(not_equal, interceptor_succeeded);
  }

  StubCompiler* stub_compiler_;
  const ParameterCount& arguments_;
  Register name_;
  Code::ExtraICState extra_ic_state_;
};


void StoreStubCompiler::GenerateRestoreName(MacroAssembler* masm,
                                            Label* label,
                                            Handle<Name> name) {
  if (!label->is_unused()) {
    __ bind(label);
    __ Move(this->name(), name);
  }
}


void StubCompiler::GenerateCheckPropertyCell(MacroAssembler* masm,
                                             Handle<JSGlobalObject> global,
                                             Handle<Name> name,
                                             Register scratch,
                                             Label* miss) {
  Handle<PropertyCell> cell =
      JSGlobalObject::EnsurePropertyCell(global, name);
  ASSERT(cell->value()->IsTheHole());
  __ Move(scratch, cell);
  __ Cmp(FieldOperand(scratch, Cell::kValueOffset),
         masm->isolate()->factory()->the_hole_value());
  __ j(not_equal, miss);
}


void StoreStubCompiler::GenerateNegativeHolderLookup(
    MacroAssembler* masm,
    Handle<JSObject> holder,
    Register holder_reg,
    Handle<Name> name,
    Label* miss) {
  if (holder->IsJSGlobalObject()) {
    GenerateCheckPropertyCell(
        masm, Handle<JSGlobalObject>::cast(holder), name, scratch1(), miss);
  } else if (!holder->HasFastProperties() && !holder->IsJSGlobalProxy()) {
    GenerateDictionaryNegativeLookup(
        masm, miss, holder_reg, name, scratch1(), scratch2());
  }
}


// Receiver_reg is preserved on jumps to miss_label, but may be destroyed if
// store is successful.
void StoreStubCompiler::GenerateStoreTransition(MacroAssembler* masm,
                                                Handle<JSObject> object,
                                                LookupResult* lookup,
                                                Handle<Map> transition,
                                                Handle<Name> name,
                                                Register receiver_reg,
                                                Register storage_reg,
                                                Register value_reg,
                                                Register scratch1,
                                                Register scratch2,
                                                Register unused,
                                                Label* miss_label,
                                                Label* slow) {
  int descriptor = transition->LastAdded();
  DescriptorArray* descriptors = transition->instance_descriptors();
  PropertyDetails details = descriptors->GetDetails(descriptor);
  Representation representation = details.representation();
  ASSERT(!representation.IsNone());

  if (details.type() == CONSTANT) {
    Handle<Object> constant(descriptors->GetValue(descriptor), masm->isolate());
    __ Cmp(value_reg, constant);
    __ j(not_equal, miss_label);
  } else if (FLAG_track_fields && representation.IsSmi()) {
    __ JumpIfNotSmi(value_reg, miss_label);
  } else if (FLAG_track_heap_object_fields && representation.IsHeapObject()) {
    __ JumpIfSmi(value_reg, miss_label);
  } else if (FLAG_track_double_fields && representation.IsDouble()) {
    Label do_store, heap_number;
    __ AllocateHeapNumber(storage_reg, scratch1, slow);

    __ JumpIfNotSmi(value_reg, &heap_number);
    __ SmiToInteger32(scratch1, value_reg);
    __ Cvtlsi2sd(xmm0, scratch1);
    __ jmp(&do_store);

    __ bind(&heap_number);
    __ CheckMap(value_reg, masm->isolate()->factory()->heap_number_map(),
                miss_label, DONT_DO_SMI_CHECK);
    __ movsd(xmm0, FieldOperand(value_reg, HeapNumber::kValueOffset));

    __ bind(&do_store);
    __ movsd(FieldOperand(storage_reg, HeapNumber::kValueOffset), xmm0);
  }

  // Stub never generated for non-global objects that require access
  // checks.
  ASSERT(object->IsJSGlobalProxy() || !object->IsAccessCheckNeeded());

  // Perform map transition for the receiver if necessary.
  if (details.type() == FIELD &&
      object->map()->unused_property_fields() == 0) {
    // The properties must be extended before we can store the value.
    // We jump to a runtime call that extends the properties array.
    __ PopReturnAddressTo(scratch1);
    __ push(receiver_reg);
    __ Push(transition);
    __ push(value_reg);
    __ PushReturnAddressFrom(scratch1);
    __ TailCallExternalReference(
        ExternalReference(IC_Utility(IC::kSharedStoreIC_ExtendStorage),
                          masm->isolate()),
        3,
        1);
    return;
  }

  // Update the map of the object.
  __ Move(scratch1, transition);
  __ movq(FieldOperand(receiver_reg, HeapObject::kMapOffset), scratch1);

  // Update the write barrier for the map field.
  __ RecordWriteField(receiver_reg,
                      HeapObject::kMapOffset,
                      scratch1,
                      scratch2,
                      kDontSaveFPRegs,
                      OMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);

  if (details.type() == CONSTANT) {
    ASSERT(value_reg.is(rax));
    __ ret(0);
    return;
  }

  int index = transition->instance_descriptors()->GetFieldIndex(
      transition->LastAdded());

  // Adjust for the number of properties stored in the object. Even in the
  // face of a transition we can use the old map here because the size of the
  // object and the number of in-object properties is not going to change.
  index -= object->map()->inobject_properties();

  // TODO(verwaest): Share this code as a code stub.
  SmiCheck smi_check = representation.IsTagged()
      ? INLINE_SMI_CHECK : OMIT_SMI_CHECK;
  if (index < 0) {
    // Set the property straight into the object.
    int offset = object->map()->instance_size() + (index * kPointerSize);
    if (FLAG_track_double_fields && representation.IsDouble()) {
      __ movq(FieldOperand(receiver_reg, offset), storage_reg);
    } else {
      __ movq(FieldOperand(receiver_reg, offset), value_reg);
    }

    if (!FLAG_track_fields || !representation.IsSmi()) {
      // Update the write barrier for the array address.
      if (!FLAG_track_double_fields || !representation.IsDouble()) {
        __ movq(storage_reg, value_reg);
      }
      __ RecordWriteField(
          receiver_reg, offset, storage_reg, scratch1, kDontSaveFPRegs,
          EMIT_REMEMBERED_SET, smi_check);
    }
  } else {
    // Write to the properties array.
    int offset = index * kPointerSize + FixedArray::kHeaderSize;
    // Get the properties array (optimistically).
    __ movq(scratch1, FieldOperand(receiver_reg, JSObject::kPropertiesOffset));
    if (FLAG_track_double_fields && representation.IsDouble()) {
      __ movq(FieldOperand(scratch1, offset), storage_reg);
    } else {
      __ movq(FieldOperand(scratch1, offset), value_reg);
    }

    if (!FLAG_track_fields || !representation.IsSmi()) {
      // Update the write barrier for the array address.
      if (!FLAG_track_double_fields || !representation.IsDouble()) {
        __ movq(storage_reg, value_reg);
      }
      __ RecordWriteField(
          scratch1, offset, storage_reg, receiver_reg, kDontSaveFPRegs,
          EMIT_REMEMBERED_SET, smi_check);
    }
  }

  // Return the value (register rax).
  ASSERT(value_reg.is(rax));
  __ ret(0);
}


// Both name_reg and receiver_reg are preserved on jumps to miss_label,
// but may be destroyed if store is successful.
void StoreStubCompiler::GenerateStoreField(MacroAssembler* masm,
                                           Handle<JSObject> object,
                                           LookupResult* lookup,
                                           Register receiver_reg,
                                           Register name_reg,
                                           Register value_reg,
                                           Register scratch1,
                                           Register scratch2,
                                           Label* miss_label) {
  // Stub never generated for non-global objects that require access
  // checks.
  ASSERT(object->IsJSGlobalProxy() || !object->IsAccessCheckNeeded());

  int index = lookup->GetFieldIndex().field_index();

  // Adjust for the number of properties stored in the object. Even in the
  // face of a transition we can use the old map here because the size of the
  // object and the number of in-object properties is not going to change.
  index -= object->map()->inobject_properties();

  Representation representation = lookup->representation();
  ASSERT(!representation.IsNone());
  if (FLAG_track_fields && representation.IsSmi()) {
    __ JumpIfNotSmi(value_reg, miss_label);
  } else if (FLAG_track_heap_object_fields && representation.IsHeapObject()) {
    __ JumpIfSmi(value_reg, miss_label);
  } else if (FLAG_track_double_fields && representation.IsDouble()) {
    // Load the double storage.
    if (index < 0) {
      int offset = object->map()->instance_size() + (index * kPointerSize);
      __ movq(scratch1, FieldOperand(receiver_reg, offset));
    } else {
      __ movq(scratch1,
              FieldOperand(receiver_reg, JSObject::kPropertiesOffset));
      int offset = index * kPointerSize + FixedArray::kHeaderSize;
      __ movq(scratch1, FieldOperand(scratch1, offset));
    }

    // Store the value into the storage.
    Label do_store, heap_number;
    __ JumpIfNotSmi(value_reg, &heap_number);
    __ SmiToInteger32(scratch2, value_reg);
    __ Cvtlsi2sd(xmm0, scratch2);
    __ jmp(&do_store);

    __ bind(&heap_number);
    __ CheckMap(value_reg, masm->isolate()->factory()->heap_number_map(),
                miss_label, DONT_DO_SMI_CHECK);
    __ movsd(xmm0, FieldOperand(value_reg, HeapNumber::kValueOffset));
    __ bind(&do_store);
    __ movsd(FieldOperand(scratch1, HeapNumber::kValueOffset), xmm0);
    // Return the value (register rax).
    ASSERT(value_reg.is(rax));
    __ ret(0);
    return;
  }

  // TODO(verwaest): Share this code as a code stub.
  SmiCheck smi_check = representation.IsTagged()
      ? INLINE_SMI_CHECK : OMIT_SMI_CHECK;
  if (index < 0) {
    // Set the property straight into the object.
    int offset = object->map()->instance_size() + (index * kPointerSize);
    __ movq(FieldOperand(receiver_reg, offset), value_reg);

    if (!FLAG_track_fields || !representation.IsSmi()) {
      // Update the write barrier for the array address.
      // Pass the value being stored in the now unused name_reg.
      __ movq(name_reg, value_reg);
      __ RecordWriteField(
          receiver_reg, offset, name_reg, scratch1, kDontSaveFPRegs,
          EMIT_REMEMBERED_SET, smi_check);
    }
  } else {
    // Write to the properties array.
    int offset = index * kPointerSize + FixedArray::kHeaderSize;
    // Get the properties array (optimistically).
    __ movq(scratch1, FieldOperand(receiver_reg, JSObject::kPropertiesOffset));
    __ movq(FieldOperand(scratch1, offset), value_reg);

    if (!FLAG_track_fields || !representation.IsSmi()) {
      // Update the write barrier for the array address.
      // Pass the value being stored in the now unused name_reg.
      __ movq(name_reg, value_reg);
      __ RecordWriteField(
          scratch1, offset, name_reg, receiver_reg, kDontSaveFPRegs,
          EMIT_REMEMBERED_SET, smi_check);
    }
  }

  // Return the value (register rax).
  ASSERT(value_reg.is(rax));
  __ ret(0);
}


void StubCompiler::GenerateCheckPropertyCells(MacroAssembler* masm,
                                              Handle<JSObject> object,
                                              Handle<JSObject> holder,
                                              Handle<Name> name,
                                              Register scratch,
                                              Label* miss) {
  Handle<JSObject> current = object;
  while (!current.is_identical_to(holder)) {
    if (current->IsJSGlobalObject()) {
      GenerateCheckPropertyCell(masm,
                                Handle<JSGlobalObject>::cast(current),
                                name,
                                scratch,
                                miss);
    }
    current = Handle<JSObject>(JSObject::cast(current->GetPrototype()));
  }
}


void StubCompiler::GenerateTailCall(MacroAssembler* masm, Handle<Code> code) {
  __ jmp(code, RelocInfo::CODE_TARGET);
}


#undef __
#define __ ACCESS_MASM((masm()))


Register StubCompiler::CheckPrototypes(Handle<JSObject> object,
                                       Register object_reg,
                                       Handle<JSObject> holder,
                                       Register holder_reg,
                                       Register scratch1,
                                       Register scratch2,
                                       Handle<Name> name,
                                       int save_at_depth,
                                       Label* miss,
                                       PrototypeCheckType check) {
  // Make sure that the type feedback oracle harvests the receiver map.
  // TODO(svenpanne) Remove this hack when all ICs are reworked.
  __ Move(scratch1, Handle<Map>(object->map()));

  Handle<JSObject> first = object;
  // Make sure there's no overlap between holder and object registers.
  ASSERT(!scratch1.is(object_reg) && !scratch1.is(holder_reg));
  ASSERT(!scratch2.is(object_reg) && !scratch2.is(holder_reg)
         && !scratch2.is(scratch1));

  // Keep track of the current object in register reg.  On the first
  // iteration, reg is an alias for object_reg, on later iterations,
  // it is an alias for holder_reg.
  Register reg = object_reg;
  int depth = 0;

  StackArgumentsAccessor args(rsp, kFastApiCallArguments,
                              ARGUMENTS_DONT_CONTAIN_RECEIVER);
  const int kHolderIndex = kFastApiCallArguments - 1 -
      FunctionCallbackArguments::kHolderIndex;

  if (save_at_depth == depth) {
    __ movq(args.GetArgumentOperand(kHolderIndex), object_reg);
  }

  // Check the maps in the prototype chain.
  // Traverse the prototype chain from the object and do map checks.
  Handle<JSObject> current = object;
  while (!current.is_identical_to(holder)) {
    ++depth;

    // Only global objects and objects that do not require access
    // checks are allowed in stubs.
    ASSERT(current->IsJSGlobalProxy() || !current->IsAccessCheckNeeded());

    Handle<JSObject> prototype(JSObject::cast(current->GetPrototype()));
    if (!current->HasFastProperties() &&
        !current->IsJSGlobalObject() &&
        !current->IsJSGlobalProxy()) {
      if (!name->IsUniqueName()) {
        ASSERT(name->IsString());
        name = factory()->InternalizeString(Handle<String>::cast(name));
      }
      ASSERT(current->property_dictionary()->FindEntry(*name) ==
             NameDictionary::kNotFound);

      GenerateDictionaryNegativeLookup(masm(), miss, reg, name,
                                       scratch1, scratch2);

      __ movq(scratch1, FieldOperand(reg, HeapObject::kMapOffset));
      reg = holder_reg;  // From now on the object will be in holder_reg.
      __ movq(reg, FieldOperand(scratch1, Map::kPrototypeOffset));
    } else {
      bool in_new_space = heap()->InNewSpace(*prototype);
      Handle<Map> current_map(current->map());
      if (in_new_space) {
        // Save the map in scratch1 for later.
        __ movq(scratch1, FieldOperand(reg, HeapObject::kMapOffset));
      }
      if (!current.is_identical_to(first) || check == CHECK_ALL_MAPS) {
        __ CheckMap(reg, current_map, miss, DONT_DO_SMI_CHECK);
      }

      // Check access rights to the global object.  This has to happen after
      // the map check so that we know that the object is actually a global
      // object.
      if (current->IsJSGlobalProxy()) {
        __ CheckAccessGlobalProxy(reg, scratch2, miss);
      }
      reg = holder_reg;  // From now on the object will be in holder_reg.

      if (in_new_space) {
        // The prototype is in new space; we cannot store a reference to it
        // in the code.  Load it from the map.
        __ movq(reg, FieldOperand(scratch1, Map::kPrototypeOffset));
      } else {
        // The prototype is in old space; load it directly.
        __ Move(reg, prototype);
      }
    }

    if (save_at_depth == depth) {
      __ movq(args.GetArgumentOperand(kHolderIndex), reg);
    }

    // Go to the next object in the prototype chain.
    current = prototype;
  }
  ASSERT(current.is_identical_to(holder));

  // Log the check depth.
  LOG(isolate(), IntEvent("check-maps-depth", depth + 1));

  if (!holder.is_identical_to(first) || check == CHECK_ALL_MAPS) {
    // Check the holder map.
    __ CheckMap(reg, Handle<Map>(holder->map()), miss, DONT_DO_SMI_CHECK);
  }

  // Perform security check for access to the global object.
  ASSERT(current->IsJSGlobalProxy() || !current->IsAccessCheckNeeded());
  if (current->IsJSGlobalProxy()) {
    __ CheckAccessGlobalProxy(reg, scratch1, miss);
  }

  // If we've skipped any global objects, it's not enough to verify that
  // their maps haven't changed.  We also need to check that the property
  // cell for the property is still empty.
  GenerateCheckPropertyCells(masm(), object, holder, name, scratch1, miss);

  // Return the register containing the holder.
  return reg;
}


void LoadStubCompiler::HandlerFrontendFooter(Handle<Name> name,
                                             Label* success,
                                             Label* miss) {
  if (!miss->is_unused()) {
    __ jmp(success);
    __ bind(miss);
    TailCallBuiltin(masm(), MissBuiltin(kind()));
  }
}


void StoreStubCompiler::HandlerFrontendFooter(Handle<Name> name,
                                              Label* success,
                                              Label* miss) {
  if (!miss->is_unused()) {
    __ jmp(success);
    GenerateRestoreName(masm(), miss, name);
    TailCallBuiltin(masm(), MissBuiltin(kind()));
  }
}


Register LoadStubCompiler::CallbackHandlerFrontend(
    Handle<JSObject> object,
    Register object_reg,
    Handle<JSObject> holder,
    Handle<Name> name,
    Label* success,
    Handle<Object> callback) {
  Label miss;

  Register reg = HandlerFrontendHeader(object, object_reg, holder, name, &miss);

  if (!holder->HasFastProperties() && !holder->IsJSGlobalObject()) {
    ASSERT(!reg.is(scratch2()));
    ASSERT(!reg.is(scratch3()));
    ASSERT(!reg.is(scratch4()));

    // Load the properties dictionary.
    Register dictionary = scratch4();
    __ movq(dictionary, FieldOperand(reg, JSObject::kPropertiesOffset));

    // Probe the dictionary.
    Label probe_done;
    NameDictionaryLookupStub::GeneratePositiveLookup(masm(),
                                                     &miss,
                                                     &probe_done,
                                                     dictionary,
                                                     this->name(),
                                                     scratch2(),
                                                     scratch3());
    __ bind(&probe_done);

    // If probing finds an entry in the dictionary, scratch3 contains the
    // index into the dictionary. Check that the value is the callback.
    Register index = scratch3();
    const int kElementsStartOffset =
        NameDictionary::kHeaderSize +
        NameDictionary::kElementsStartIndex * kPointerSize;
    const int kValueOffset = kElementsStartOffset + kPointerSize;
    __ movq(scratch2(),
            Operand(dictionary, index, times_pointer_size,
                    kValueOffset - kHeapObjectTag));
    __ movq(scratch3(), callback, RelocInfo::EMBEDDED_OBJECT);
    __ cmpq(scratch2(), scratch3());
    __ j(not_equal, &miss);
  }

  HandlerFrontendFooter(name, success, &miss);
  return reg;
}


void LoadStubCompiler::GenerateLoadField(Register reg,
                                         Handle<JSObject> holder,
                                         PropertyIndex field,
                                         Representation representation) {
  if (!reg.is(receiver())) __ movq(receiver(), reg);
  if (kind() == Code::LOAD_IC) {
    LoadFieldStub stub(field.is_inobject(holder),
                       field.translate(holder),
                       representation);
    GenerateTailCall(masm(), stub.GetCode(isolate()));
  } else {
    KeyedLoadFieldStub stub(field.is_inobject(holder),
                            field.translate(holder),
                            representation);
    GenerateTailCall(masm(), stub.GetCode(isolate()));
  }
}


void LoadStubCompiler::GenerateLoadCallback(
    const CallOptimization& call_optimization) {
  GenerateFastApiCall(
      masm(), call_optimization, receiver(), scratch3(), 0, NULL);
}


void LoadStubCompiler::GenerateLoadCallback(
    Register reg,
    Handle<ExecutableAccessorInfo> callback) {
  // Insert additional parameters into the stack frame above return address.
  ASSERT(!scratch4().is(reg));
  __ PopReturnAddressTo(scratch4());

  STATIC_ASSERT(PropertyCallbackArguments::kHolderIndex == 0);
  STATIC_ASSERT(PropertyCallbackArguments::kIsolateIndex == 1);
  STATIC_ASSERT(PropertyCallbackArguments::kReturnValueDefaultValueIndex == 2);
  STATIC_ASSERT(PropertyCallbackArguments::kReturnValueOffset == 3);
  STATIC_ASSERT(PropertyCallbackArguments::kDataIndex == 4);
  STATIC_ASSERT(PropertyCallbackArguments::kThisIndex == 5);
  STATIC_ASSERT(PropertyCallbackArguments::kArgsLength == 6);
  __ push(receiver());  // receiver
  if (heap()->InNewSpace(callback->data())) {
    ASSERT(!scratch2().is(reg));
    __ Move(scratch2(), callback);
    __ push(FieldOperand(scratch2(),
                         ExecutableAccessorInfo::kDataOffset));  // data
  } else {
    __ Push(Handle<Object>(callback->data(), isolate()));
  }
  ASSERT(!kScratchRegister.is(reg));
  __ LoadRoot(kScratchRegister, Heap::kUndefinedValueRootIndex);
  __ push(kScratchRegister);  // return value
  __ push(kScratchRegister);  // return value default
  __ PushAddress(ExternalReference::isolate_address(isolate()));
  __ push(reg);  // holder
  __ push(name());  // name
  // Save a pointer to where we pushed the arguments pointer.  This will be
  // passed as the const PropertyAccessorInfo& to the C++ callback.

  Address getter_address = v8::ToCData<Address>(callback->getter());

#if defined(__MINGW64__) || defined(_WIN64)
  Register getter_arg = r8;
  Register accessor_info_arg = rdx;
  Register name_arg = rcx;
#else
  Register getter_arg = rdx;
  Register accessor_info_arg = rsi;
  Register name_arg = rdi;
#endif

  ASSERT(!name_arg.is(scratch4()));
  __ movq(name_arg, rsp);
  __ PushReturnAddressFrom(scratch4());

  // v8::Arguments::values_ and handler for name.
  const int kStackSpace = PropertyCallbackArguments::kArgsLength + 1;

  // Allocate v8::AccessorInfo in non-GCed stack space.
  const int kArgStackSpace = 1;

  __ PrepareCallApiFunction(kArgStackSpace);
  __ lea(rax, Operand(name_arg, 1 * kPointerSize));

  // v8::PropertyAccessorInfo::args_.
  __ movq(StackSpaceOperand(0), rax);

  // The context register (rsi) has been saved in PrepareCallApiFunction and
  // could be used to pass arguments.
  __ lea(accessor_info_arg, StackSpaceOperand(0));

  Address thunk_address = FUNCTION_ADDR(&InvokeAccessorGetterCallback);

  // The name handler is counted as an argument.
  StackArgumentsAccessor args(rbp, PropertyCallbackArguments::kArgsLength);
  Operand return_value_operand = args.GetArgumentOperand(
      PropertyCallbackArguments::kArgsLength - 1 -
      PropertyCallbackArguments::kReturnValueOffset);
  __ CallApiFunctionAndReturn(getter_address,
                              thunk_address,
                              getter_arg,
                              kStackSpace,
                              return_value_operand,
                              NULL);
}


void LoadStubCompiler::GenerateLoadConstant(Handle<Object> value) {
  // Return the constant value.
  __ Move(rax, value);
  __ ret(0);
}


void LoadStubCompiler::GenerateLoadInterceptor(
    Register holder_reg,
    Handle<JSObject> object,
    Handle<JSObject> interceptor_holder,
    LookupResult* lookup,
    Handle<Name> name) {
  ASSERT(interceptor_holder->HasNamedInterceptor());
  ASSERT(!interceptor_holder->GetNamedInterceptor()->getter()->IsUndefined());

  // So far the most popular follow ups for interceptor loads are FIELD
  // and CALLBACKS, so inline only them, other cases may be added
  // later.
  bool compile_followup_inline = false;
  if (lookup->IsFound() && lookup->IsCacheable()) {
    if (lookup->IsField()) {
      compile_followup_inline = true;
    } else if (lookup->type() == CALLBACKS &&
               lookup->GetCallbackObject()->IsExecutableAccessorInfo()) {
      ExecutableAccessorInfo* callback =
          ExecutableAccessorInfo::cast(lookup->GetCallbackObject());
      compile_followup_inline = callback->getter() != NULL &&
          callback->IsCompatibleReceiver(*object);
    }
  }

  if (compile_followup_inline) {
    // Compile the interceptor call, followed by inline code to load the
    // property from further up the prototype chain if the call fails.
    // Check that the maps haven't changed.
    ASSERT(holder_reg.is(receiver()) || holder_reg.is(scratch1()));

    // Preserve the receiver register explicitly whenever it is different from
    // the holder and it is needed should the interceptor return without any
    // result. The CALLBACKS case needs the receiver to be passed into C++ code,
    // the FIELD case might cause a miss during the prototype check.
    bool must_perfrom_prototype_check = *interceptor_holder != lookup->holder();
    bool must_preserve_receiver_reg = !receiver().is(holder_reg) &&
        (lookup->type() == CALLBACKS || must_perfrom_prototype_check);

    // Save necessary data before invoking an interceptor.
    // Requires a frame to make GC aware of pushed pointers.
    {
      FrameScope frame_scope(masm(), StackFrame::INTERNAL);

      if (must_preserve_receiver_reg) {
        __ push(receiver());
      }
      __ push(holder_reg);
      __ push(this->name());

      // Invoke an interceptor.  Note: map checks from receiver to
      // interceptor's holder has been compiled before (see a caller
      // of this method.)
      CompileCallLoadPropertyWithInterceptor(masm(),
                                             receiver(),
                                             holder_reg,
                                             this->name(),
                                             interceptor_holder);

      // Check if interceptor provided a value for property.  If it's
      // the case, return immediately.
      Label interceptor_failed;
      __ CompareRoot(rax, Heap::kNoInterceptorResultSentinelRootIndex);
      __ j(equal, &interceptor_failed);
      frame_scope.GenerateLeaveFrame();
      __ ret(0);

      __ bind(&interceptor_failed);
      __ pop(this->name());
      __ pop(holder_reg);
      if (must_preserve_receiver_reg) {
        __ pop(receiver());
      }

      // Leave the internal frame.
    }

    GenerateLoadPostInterceptor(holder_reg, interceptor_holder, name, lookup);
  } else {  // !compile_followup_inline
    // Call the runtime system to load the interceptor.
    // Check that the maps haven't changed.
    __ PopReturnAddressTo(scratch2());
    PushInterceptorArguments(masm(), receiver(), holder_reg,
                             this->name(), interceptor_holder);
    __ PushReturnAddressFrom(scratch2());

    ExternalReference ref = ExternalReference(
        IC_Utility(IC::kLoadPropertyWithInterceptorForLoad), isolate());
    __ TailCallExternalReference(ref, StubCache::kInterceptorArgsLength, 1);
  }
}


void CallStubCompiler::GenerateNameCheck(Handle<Name> name, Label* miss) {
  if (kind_ == Code::KEYED_CALL_IC) {
    __ Cmp(rcx, name);
    __ j(not_equal, miss);
  }
}


void CallStubCompiler::GenerateGlobalReceiverCheck(Handle<JSObject> object,
                                                   Handle<JSObject> holder,
                                                   Handle<Name> name,
                                                   Label* miss) {
  ASSERT(holder->IsGlobalObject());

  StackArgumentsAccessor args(rsp, arguments());
  __ movq(rdx, args.GetReceiverOperand());


  // Check that the maps haven't changed.
  __ JumpIfSmi(rdx, miss);
  CheckPrototypes(object, rdx, holder, rbx, rax, rdi, name, miss);
}


void CallStubCompiler::GenerateLoadFunctionFromCell(
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Label* miss) {
  // Get the value from the cell.
  __ Move(rdi, cell);
  __ movq(rdi, FieldOperand(rdi, Cell::kValueOffset));

  // Check that the cell contains the same function.
  if (heap()->InNewSpace(*function)) {
    // We can't embed a pointer to a function in new space so we have
    // to verify that the shared function info is unchanged. This has
    // the nice side effect that multiple closures based on the same
    // function can all use this call IC. Before we load through the
    // function, we have to verify that it still is a function.
    __ JumpIfSmi(rdi, miss);
    __ CmpObjectType(rdi, JS_FUNCTION_TYPE, rax);
    __ j(not_equal, miss);

    // Check the shared function info. Make sure it hasn't changed.
    __ Move(rax, Handle<SharedFunctionInfo>(function->shared()));
    __ cmpq(FieldOperand(rdi, JSFunction::kSharedFunctionInfoOffset), rax);
  } else {
    __ Cmp(rdi, function);
  }
  __ j(not_equal, miss);
}


void CallStubCompiler::GenerateMissBranch() {
  Handle<Code> code =
      isolate()->stub_cache()->ComputeCallMiss(arguments().immediate(),
                                               kind_,
                                               extra_state_);
  __ Jump(code, RelocInfo::CODE_TARGET);
}


Handle<Code> CallStubCompiler::CompileCallField(Handle<JSObject> object,
                                                Handle<JSObject> holder,
                                                PropertyIndex index,
                                                Handle<Name> name) {
  // ----------- S t a t e -------------
  // rcx                 : function name
  // rsp[0]              : return address
  // rsp[8]              : argument argc
  // rsp[16]             : argument argc - 1
  // ...
  // rsp[argc * 8]       : argument 1
  // rsp[(argc + 1) * 8] : argument 0 = receiver
  // -----------------------------------
  Label miss;

  GenerateNameCheck(name, &miss);

  StackArgumentsAccessor args(rsp, arguments());
  __ movq(rdx, args.GetReceiverOperand());

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(rdx, &miss);

  // Do the right check and compute the holder register.
  Register reg = CheckPrototypes(object, rdx, holder, rbx, rax, rdi,
                                 name, &miss);

  GenerateFastPropertyLoad(masm(), rdi, reg, index.is_inobject(holder),
                           index.translate(holder), Representation::Tagged());

  // Check that the function really is a function.
  __ JumpIfSmi(rdi, &miss);
  __ CmpObjectType(rdi, JS_FUNCTION_TYPE, rbx);
  __ j(not_equal, &miss);

  // Patch the receiver on the stack with the global proxy if
  // necessary.
  if (object->IsGlobalObject()) {
    __ movq(rdx, FieldOperand(rdx, GlobalObject::kGlobalReceiverOffset));
    __ movq(args.GetReceiverOperand(), rdx);
  }

  // Invoke the function.
  CallKind call_kind = CallICBase::Contextual::decode(extra_state_)
      ? CALL_AS_FUNCTION
      : CALL_AS_METHOD;
  __ InvokeFunction(rdi, arguments(), JUMP_FUNCTION,
                    NullCallWrapper(), call_kind);

  // Handle call cache miss.
  __ bind(&miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(Code::FIELD, name);
}


Handle<Code> CallStubCompiler::CompileArrayCodeCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  Label miss;

  // Check that function is still array
  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);
  GenerateNameCheck(name, &miss);

  if (cell.is_null()) {
    __ movq(rdx, args.GetReceiverOperand());

    // Check that the receiver isn't a smi.
    __ JumpIfSmi(rdx, &miss);
    CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                    name, &miss);
  } else {
    ASSERT(cell->value() == *function);
    GenerateGlobalReceiverCheck(Handle<JSObject>::cast(object), holder, name,
                                &miss);
    GenerateLoadFunctionFromCell(cell, function, &miss);
  }

  Handle<AllocationSite> site = isolate()->factory()->NewAllocationSite();
  site->set_transition_info(Smi::FromInt(GetInitialFastElementsKind()));
  Handle<Cell> site_feedback_cell = isolate()->factory()->NewCell(site);
  __ movq(rax, Immediate(argc));
  __ Move(rbx, site_feedback_cell);
  __ Move(rdi, function);

  ArrayConstructorStub stub(isolate());
  __ TailCallStub(&stub);

  __ bind(&miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileArrayPushCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 8] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 8] : receiver
  // -----------------------------------

  // If object is not an array, bail out to regular call.
  if (!object->IsJSArray() || !cell.is_null()) return Handle<Code>::null();

  Label miss;
  GenerateNameCheck(name, &miss);

  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);
  __ movq(rdx, args.GetReceiverOperand());

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(rdx, &miss);

  CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                  name, &miss);

  if (argc == 0) {
    // Noop, return the length.
    __ movq(rax, FieldOperand(rdx, JSArray::kLengthOffset));
    __ ret((argc + 1) * kPointerSize);
  } else {
    Label call_builtin;

    if (argc == 1) {  // Otherwise fall through to call builtin.
      Label attempt_to_grow_elements, with_write_barrier, check_double;

      // Get the elements array of the object.
      __ movq(rdi, FieldOperand(rdx, JSArray::kElementsOffset));

      // Check that the elements are in fast mode and writable.
      __ Cmp(FieldOperand(rdi, HeapObject::kMapOffset),
             factory()->fixed_array_map());
      __ j(not_equal, &check_double);

      // Get the array's length into rax and calculate new length.
      __ SmiToInteger32(rax, FieldOperand(rdx, JSArray::kLengthOffset));
      STATIC_ASSERT(FixedArray::kMaxLength < Smi::kMaxValue);
      __ addl(rax, Immediate(argc));

      // Get the elements' length into rcx.
      __ SmiToInteger32(rcx, FieldOperand(rdi, FixedArray::kLengthOffset));

      // Check if we could survive without allocation.
      __ cmpl(rax, rcx);
      __ j(greater, &attempt_to_grow_elements);

      // Check if value is a smi.
      __ movq(rcx, args.GetArgumentOperand(1));
      __ JumpIfNotSmi(rcx, &with_write_barrier);

      // Save new length.
      __ Integer32ToSmiField(FieldOperand(rdx, JSArray::kLengthOffset), rax);

      // Store the value.
      __ movq(FieldOperand(rdi,
                           rax,
                           times_pointer_size,
                           FixedArray::kHeaderSize - argc * kPointerSize),
              rcx);

      __ Integer32ToSmi(rax, rax);  // Return new length as smi.
      __ ret((argc + 1) * kPointerSize);

      __ bind(&check_double);

      // Check that the elements are in double mode.
      __ Cmp(FieldOperand(rdi, HeapObject::kMapOffset),
             factory()->fixed_double_array_map());
      __ j(not_equal, &call_builtin);

      // Get the array's length into rax and calculate new length.
      __ SmiToInteger32(rax, FieldOperand(rdx, JSArray::kLengthOffset));
      STATIC_ASSERT(FixedArray::kMaxLength < Smi::kMaxValue);
      __ addl(rax, Immediate(argc));

      // Get the elements' length into rcx.
      __ SmiToInteger32(rcx, FieldOperand(rdi, FixedArray::kLengthOffset));

      // Check if we could survive without allocation.
      __ cmpl(rax, rcx);
      __ j(greater, &call_builtin);

      __ movq(rcx, args.GetArgumentOperand(1));
      __ StoreNumberToDoubleElements(
          rcx, rdi, rax, xmm0, &call_builtin, argc * kDoubleSize);

      // Save new length.
      __ Integer32ToSmiField(FieldOperand(rdx, JSArray::kLengthOffset), rax);
      __ Integer32ToSmi(rax, rax);  // Return new length as smi.
      __ ret((argc + 1) * kPointerSize);

      __ bind(&with_write_barrier);

      __ movq(rbx, FieldOperand(rdx, HeapObject::kMapOffset));

      if (FLAG_smi_only_arrays  && !FLAG_trace_elements_transitions) {
        Label fast_object, not_fast_object;
        __ CheckFastObjectElements(rbx, &not_fast_object, Label::kNear);
        __ jmp(&fast_object);
        // In case of fast smi-only, convert to fast object, otherwise bail out.
        __ bind(&not_fast_object);
        __ CheckFastSmiElements(rbx, &call_builtin);
        __ Cmp(FieldOperand(rcx, HeapObject::kMapOffset),
               factory()->heap_number_map());
        __ j(equal, &call_builtin);
        // rdx: receiver
        // rbx: map

        Label try_holey_map;
        __ LoadTransitionedArrayMapConditional(FAST_SMI_ELEMENTS,
                                               FAST_ELEMENTS,
                                               rbx,
                                               rdi,
                                               &try_holey_map);

        ElementsTransitionGenerator::
            GenerateMapChangeElementsTransition(masm(),
                                                DONT_TRACK_ALLOCATION_SITE,
                                                NULL);
        // Restore edi.
        __ movq(rdi, FieldOperand(rdx, JSArray::kElementsOffset));
        __ jmp(&fast_object);

        __ bind(&try_holey_map);
        __ LoadTransitionedArrayMapConditional(FAST_HOLEY_SMI_ELEMENTS,
                                               FAST_HOLEY_ELEMENTS,
                                               rbx,
                                               rdi,
                                               &call_builtin);
        ElementsTransitionGenerator::
            GenerateMapChangeElementsTransition(masm(),
                                                DONT_TRACK_ALLOCATION_SITE,
                                                NULL);
        __ movq(rdi, FieldOperand(rdx, JSArray::kElementsOffset));
        __ bind(&fast_object);
      } else {
        __ CheckFastObjectElements(rbx, &call_builtin);
      }

      // Save new length.
      __ Integer32ToSmiField(FieldOperand(rdx, JSArray::kLengthOffset), rax);

      // Store the value.
      __ lea(rdx, FieldOperand(rdi,
                               rax, times_pointer_size,
                               FixedArray::kHeaderSize - argc * kPointerSize));
      __ movq(Operand(rdx, 0), rcx);

      __ RecordWrite(rdi, rdx, rcx, kDontSaveFPRegs, EMIT_REMEMBERED_SET,
                     OMIT_SMI_CHECK);

      __ Integer32ToSmi(rax, rax);  // Return new length as smi.
      __ ret((argc + 1) * kPointerSize);

      __ bind(&attempt_to_grow_elements);
      if (!FLAG_inline_new) {
        __ jmp(&call_builtin);
      }

      __ movq(rbx, args.GetArgumentOperand(1));
      // Growing elements that are SMI-only requires special handling in case
      // the new element is non-Smi. For now, delegate to the builtin.
      Label no_fast_elements_check;
      __ JumpIfSmi(rbx, &no_fast_elements_check);
      __ movq(rcx, FieldOperand(rdx, HeapObject::kMapOffset));
      __ CheckFastObjectElements(rcx, &call_builtin, Label::kFar);
      __ bind(&no_fast_elements_check);

      ExternalReference new_space_allocation_top =
          ExternalReference::new_space_allocation_top_address(isolate());
      ExternalReference new_space_allocation_limit =
          ExternalReference::new_space_allocation_limit_address(isolate());

      const int kAllocationDelta = 4;
      // Load top.
      __ Load(rcx, new_space_allocation_top);

      // Check if it's the end of elements.
      __ lea(rdx, FieldOperand(rdi,
                               rax, times_pointer_size,
                               FixedArray::kHeaderSize - argc * kPointerSize));
      __ cmpq(rdx, rcx);
      __ j(not_equal, &call_builtin);
      __ addq(rcx, Immediate(kAllocationDelta * kPointerSize));
      Operand limit_operand =
          masm()->ExternalOperand(new_space_allocation_limit);
      __ cmpq(rcx, limit_operand);
      __ j(above, &call_builtin);

      // We fit and could grow elements.
      __ Store(new_space_allocation_top, rcx);

      // Push the argument...
      __ movq(Operand(rdx, 0), rbx);
      // ... and fill the rest with holes.
      __ LoadRoot(kScratchRegister, Heap::kTheHoleValueRootIndex);
      for (int i = 1; i < kAllocationDelta; i++) {
        __ movq(Operand(rdx, i * kPointerSize), kScratchRegister);
      }

      // We know the elements array is in new space so we don't need the
      // remembered set, but we just pushed a value onto it so we may have to
      // tell the incremental marker to rescan the object that we just grew.  We
      // don't need to worry about the holes because they are in old space and
      // already marked black.
      __ RecordWrite(rdi, rdx, rbx, kDontSaveFPRegs, OMIT_REMEMBERED_SET);

      // Restore receiver to rdx as finish sequence assumes it's here.
      __ movq(rdx, args.GetReceiverOperand());

      // Increment element's and array's sizes.
      __ SmiAddConstant(FieldOperand(rdi, FixedArray::kLengthOffset),
                        Smi::FromInt(kAllocationDelta));

      // Make new length a smi before returning it.
      __ Integer32ToSmi(rax, rax);
      __ movq(FieldOperand(rdx, JSArray::kLengthOffset), rax);

      __ ret((argc + 1) * kPointerSize);
    }

    __ bind(&call_builtin);
    __ TailCallExternalReference(ExternalReference(Builtins::c_ArrayPush,
                                                   isolate()),
                                 argc + 1,
                                 1);
  }

  __ bind(&miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileArrayPopCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 8] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 8] : receiver
  // -----------------------------------

  // If object is not an array, bail out to regular call.
  if (!object->IsJSArray() || !cell.is_null()) return Handle<Code>::null();

  Label miss, return_undefined, call_builtin;
  GenerateNameCheck(name, &miss);

  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);
  __ movq(rdx, args.GetReceiverOperand());

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(rdx, &miss);

  CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                  name, &miss);

  // Get the elements array of the object.
  __ movq(rbx, FieldOperand(rdx, JSArray::kElementsOffset));

  // Check that the elements are in fast mode and writable.
  __ CompareRoot(FieldOperand(rbx, HeapObject::kMapOffset),
                 Heap::kFixedArrayMapRootIndex);
  __ j(not_equal, &call_builtin);

  // Get the array's length into rcx and calculate new length.
  __ SmiToInteger32(rcx, FieldOperand(rdx, JSArray::kLengthOffset));
  __ subl(rcx, Immediate(1));
  __ j(negative, &return_undefined);

  // Get the last element.
  __ LoadRoot(r9, Heap::kTheHoleValueRootIndex);
  __ movq(rax, FieldOperand(rbx,
                            rcx, times_pointer_size,
                            FixedArray::kHeaderSize));
  // Check if element is already the hole.
  __ cmpq(rax, r9);
  // If so, call slow-case to also check prototypes for value.
  __ j(equal, &call_builtin);

  // Set the array's length.
  __ Integer32ToSmiField(FieldOperand(rdx, JSArray::kLengthOffset), rcx);

  // Fill with the hole and return original value.
  __ movq(FieldOperand(rbx,
                       rcx, times_pointer_size,
                       FixedArray::kHeaderSize),
          r9);
  __ ret((argc + 1) * kPointerSize);

  __ bind(&return_undefined);
  __ LoadRoot(rax, Heap::kUndefinedValueRootIndex);
  __ ret((argc + 1) * kPointerSize);

  __ bind(&call_builtin);
  __ TailCallExternalReference(
      ExternalReference(Builtins::c_ArrayPop, isolate()),
      argc + 1,
      1);

  __ bind(&miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileStringCharCodeAtCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : function name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 8] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 8] : receiver
  // -----------------------------------

  // If object is not a string, bail out to regular call.
  if (!object->IsString() || !cell.is_null()) return Handle<Code>::null();

  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);

  Label miss;
  Label name_miss;
  Label index_out_of_range;
  Label* index_out_of_range_label = &index_out_of_range;
  if (kind_ == Code::CALL_IC &&
      (CallICBase::StringStubState::decode(extra_state_) ==
       DEFAULT_STRING_STUB)) {
    index_out_of_range_label = &miss;
  }
  GenerateNameCheck(name, &name_miss);

  // Check that the maps starting from the prototype haven't changed.
  GenerateDirectLoadGlobalFunctionPrototype(masm(),
                                            Context::STRING_FUNCTION_INDEX,
                                            rax,
                                            &miss);
  ASSERT(!object.is_identical_to(holder));
  CheckPrototypes(
      Handle<JSObject>(JSObject::cast(object->GetPrototype(isolate()))),
      rax, holder, rbx, rdx, rdi, name, &miss);

  Register receiver = rbx;
  Register index = rdi;
  Register result = rax;
  __ movq(receiver, args.GetReceiverOperand());
  if (argc > 0) {
    __ movq(index, args.GetArgumentOperand(1));
  } else {
    __ LoadRoot(index, Heap::kUndefinedValueRootIndex);
  }

  StringCharCodeAtGenerator generator(receiver,
                                      index,
                                      result,
                                      &miss,  // When not a string.
                                      &miss,  // When not a number.
                                      index_out_of_range_label,
                                      STRING_INDEX_IS_NUMBER);
  generator.GenerateFast(masm());
  __ ret((argc + 1) * kPointerSize);

  StubRuntimeCallHelper call_helper;
  generator.GenerateSlow(masm(), call_helper);

  if (index_out_of_range.is_linked()) {
    __ bind(&index_out_of_range);
    __ LoadRoot(rax, Heap::kNanValueRootIndex);
    __ ret((argc + 1) * kPointerSize);
  }

  __ bind(&miss);
  // Restore function name in rcx.
  __ Move(rcx, name);
  __ bind(&name_miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileStringCharAtCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : function name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 8] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 8] : receiver
  // -----------------------------------

  // If object is not a string, bail out to regular call.
  if (!object->IsString() || !cell.is_null()) return Handle<Code>::null();

  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);

  Label miss;
  Label name_miss;
  Label index_out_of_range;
  Label* index_out_of_range_label = &index_out_of_range;
  if (kind_ == Code::CALL_IC &&
      (CallICBase::StringStubState::decode(extra_state_) ==
       DEFAULT_STRING_STUB)) {
    index_out_of_range_label = &miss;
  }
  GenerateNameCheck(name, &name_miss);

  // Check that the maps starting from the prototype haven't changed.
  GenerateDirectLoadGlobalFunctionPrototype(masm(),
                                            Context::STRING_FUNCTION_INDEX,
                                            rax,
                                            &miss);
  ASSERT(!object.is_identical_to(holder));
  CheckPrototypes(
      Handle<JSObject>(JSObject::cast(object->GetPrototype(isolate()))),
      rax, holder, rbx, rdx, rdi, name, &miss);

  Register receiver = rax;
  Register index = rdi;
  Register scratch = rdx;
  Register result = rax;
  __ movq(receiver, args.GetReceiverOperand());
  if (argc > 0) {
    __ movq(index, args.GetArgumentOperand(1));
  } else {
    __ LoadRoot(index, Heap::kUndefinedValueRootIndex);
  }

  StringCharAtGenerator generator(receiver,
                                  index,
                                  scratch,
                                  result,
                                  &miss,  // When not a string.
                                  &miss,  // When not a number.
                                  index_out_of_range_label,
                                  STRING_INDEX_IS_NUMBER);
  generator.GenerateFast(masm());
  __ ret((argc + 1) * kPointerSize);

  StubRuntimeCallHelper call_helper;
  generator.GenerateSlow(masm(), call_helper);

  if (index_out_of_range.is_linked()) {
    __ bind(&index_out_of_range);
    __ LoadRoot(rax, Heap::kempty_stringRootIndex);
    __ ret((argc + 1) * kPointerSize);
  }
  __ bind(&miss);
  // Restore function name in rcx.
  __ Move(rcx, name);
  __ bind(&name_miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileStringFromCharCodeCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : function name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 8] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 8] : receiver
  // -----------------------------------

  // If the object is not a JSObject or we got an unexpected number of
  // arguments, bail out to the regular call.
  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);
  if (!object->IsJSObject() || argc != 1) return Handle<Code>::null();

  Label miss;
  GenerateNameCheck(name, &miss);

  if (cell.is_null()) {
    __ movq(rdx, args.GetReceiverOperand());
    __ JumpIfSmi(rdx, &miss);
    CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                    name, &miss);
  } else {
    ASSERT(cell->value() == *function);
    GenerateGlobalReceiverCheck(Handle<JSObject>::cast(object), holder, name,
                                &miss);
    GenerateLoadFunctionFromCell(cell, function, &miss);
  }

  // Load the char code argument.
  Register code = rbx;
  __ movq(code, args.GetArgumentOperand(1));

  // Check the code is a smi.
  Label slow;
  __ JumpIfNotSmi(code, &slow);

  // Convert the smi code to uint16.
  __ SmiAndConstant(code, code, Smi::FromInt(0xffff));

  StringCharFromCodeGenerator generator(code, rax);
  generator.GenerateFast(masm());
  __ ret(2 * kPointerSize);

  StubRuntimeCallHelper call_helper;
  generator.GenerateSlow(masm(), call_helper);

  // Tail call the full function. We do not have to patch the receiver
  // because the function makes no use of it.
  __ bind(&slow);
  CallKind call_kind = CallICBase::Contextual::decode(extra_state_)
      ? CALL_AS_FUNCTION
      : CALL_AS_METHOD;
  ParameterCount expected(function);
  __ InvokeFunction(function, expected, arguments(),
                    JUMP_FUNCTION, NullCallWrapper(), call_kind);

  __ bind(&miss);
  // rcx: function name.
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileMathFloorCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 4] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 4] : receiver
  // -----------------------------------
  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);

  // If the object is not a JSObject or we got an unexpected number of
  // arguments, bail out to the regular call.
  if (!object->IsJSObject() || argc != 1) {
    return Handle<Code>::null();
  }

  Label miss;
  GenerateNameCheck(name, &miss);

  if (cell.is_null()) {
    __ movq(rdx, args.GetReceiverOperand());

    STATIC_ASSERT(kSmiTag == 0);
    __ JumpIfSmi(rdx, &miss);

    CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                    name, &miss);
  } else {
    ASSERT(cell->value() == *function);
    GenerateGlobalReceiverCheck(Handle<JSObject>::cast(object), holder, name,
                                &miss);
    GenerateLoadFunctionFromCell(cell, function, &miss);
  }

  // Load the (only) argument into rax.
  __ movq(rax, args.GetArgumentOperand(1));

  // Check if the argument is a smi.
  Label smi;
  STATIC_ASSERT(kSmiTag == 0);
  __ JumpIfSmi(rax, &smi);

  // Check if the argument is a heap number and load its value into xmm0.
  Label slow;
  __ CheckMap(rax, factory()->heap_number_map(), &slow, DONT_DO_SMI_CHECK);
  __ movsd(xmm0, FieldOperand(rax, HeapNumber::kValueOffset));

  // Check if the argument is strictly positive. Note this also discards NaN.
  __ xorpd(xmm1, xmm1);
  __ ucomisd(xmm0, xmm1);
  __ j(below_equal, &slow);

  // Do a truncating conversion.
  __ cvttsd2si(rax, xmm0);

  // Checks for 0x80000000 which signals a failed conversion.
  Label conversion_failure;
  __ cmpl(rax, Immediate(0x80000000));
  __ j(equal, &conversion_failure);

  // Smi tag and return.
  __ Integer32ToSmi(rax, rax);
  __ bind(&smi);
  __ ret(2 * kPointerSize);

  // Check if the argument is < 2^kMantissaBits.
  Label already_round;
  __ bind(&conversion_failure);
  int64_t kTwoMantissaBits= V8_INT64_C(0x4330000000000000);
  __ movq(rbx, kTwoMantissaBits, RelocInfo::NONE64);
  __ movq(xmm1, rbx);
  __ ucomisd(xmm0, xmm1);
  __ j(above_equal, &already_round);

  // Save a copy of the argument.
  __ movaps(xmm2, xmm0);

  // Compute (argument + 2^kMantissaBits) - 2^kMantissaBits.
  __ addsd(xmm0, xmm1);
  __ subsd(xmm0, xmm1);

  // Compare the argument and the tentative result to get the right mask:
  //   if xmm2 < xmm0:
  //     xmm2 = 1...1
  //   else:
  //     xmm2 = 0...0
  __ cmpltsd(xmm2, xmm0);

  // Subtract 1 if the argument was less than the tentative result.
  int64_t kOne = V8_INT64_C(0x3ff0000000000000);
  __ movq(rbx, kOne, RelocInfo::NONE64);
  __ movq(xmm1, rbx);
  __ andpd(xmm1, xmm2);
  __ subsd(xmm0, xmm1);

  // Return a new heap number.
  __ AllocateHeapNumber(rax, rbx, &slow);
  __ movsd(FieldOperand(rax, HeapNumber::kValueOffset), xmm0);
  __ ret(2 * kPointerSize);

  // Return the argument (when it's an already round heap number).
  __ bind(&already_round);
  __ movq(rax, args.GetArgumentOperand(1));
  __ ret(2 * kPointerSize);

  // Tail call the full function. We do not have to patch the receiver
  // because the function makes no use of it.
  __ bind(&slow);
  ParameterCount expected(function);
  __ InvokeFunction(function, expected, arguments(),
                    JUMP_FUNCTION, NullCallWrapper(), CALL_AS_METHOD);

  __ bind(&miss);
  // rcx: function name.
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileMathAbsCall(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name,
    Code::StubType type) {
  // ----------- S t a t e -------------
  //  -- rcx                 : function name
  //  -- rsp[0]              : return address
  //  -- rsp[(argc - n) * 8] : arg[n] (zero-based)
  //  -- ...
  //  -- rsp[(argc + 1) * 8] : receiver
  // -----------------------------------

  // If the object is not a JSObject or we got an unexpected number of
  // arguments, bail out to the regular call.
  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);
  if (!object->IsJSObject() || argc != 1) return Handle<Code>::null();

  Label miss;
  GenerateNameCheck(name, &miss);

  if (cell.is_null()) {
    __ movq(rdx, args.GetReceiverOperand());
    __ JumpIfSmi(rdx, &miss);
    CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                    name, &miss);
  } else {
    ASSERT(cell->value() == *function);
    GenerateGlobalReceiverCheck(Handle<JSObject>::cast(object), holder, name,
                                &miss);
    GenerateLoadFunctionFromCell(cell, function, &miss);
  }
  // Load the (only) argument into rax.
  __ movq(rax, args.GetArgumentOperand(1));

  // Check if the argument is a smi.
  Label not_smi;
  STATIC_ASSERT(kSmiTag == 0);
  __ JumpIfNotSmi(rax, &not_smi);

  // Branchless abs implementation, refer to below:
  // http://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs
  // Set ebx to 1...1 (== -1) if the argument is negative, or to 0...0
  // otherwise.
  __ movq(rbx, rax);
  __ sar(rbx, Immediate(kBitsPerPointer - 1));

  // Do bitwise not or do nothing depending on ebx.
  __ xor_(rax, rbx);

  // Add 1 or do nothing depending on ebx.
  __ subq(rax, rbx);

  // If the result is still negative, go to the slow case.
  // This only happens for the most negative smi.
  Label slow;
  __ j(negative, &slow);

  __ ret(2 * kPointerSize);

  // Check if the argument is a heap number and load its value.
  __ bind(&not_smi);
  __ CheckMap(rax, factory()->heap_number_map(), &slow, DONT_DO_SMI_CHECK);
  __ MoveDouble(rbx, FieldOperand(rax, HeapNumber::kValueOffset));

  // Check the sign of the argument. If the argument is positive,
  // just return it.
  Label negative_sign;
  const int sign_mask_shift =
      (HeapNumber::kExponentOffset - HeapNumber::kValueOffset) * kBitsPerByte;
  __ movq(rdi, static_cast<int64_t>(HeapNumber::kSignMask) << sign_mask_shift,
          RelocInfo::NONE64);
  __ testq(rbx, rdi);
  __ j(not_zero, &negative_sign);
  __ ret(2 * kPointerSize);

  // If the argument is negative, clear the sign, and return a new
  // number. We still have the sign mask in rdi.
  __ bind(&negative_sign);
  __ xor_(rbx, rdi);
  __ AllocateHeapNumber(rax, rdx, &slow);
  __ MoveDouble(FieldOperand(rax, HeapNumber::kValueOffset), rbx);
  __ ret(2 * kPointerSize);

  // Tail call the full function. We do not have to patch the receiver
  // because the function makes no use of it.
  __ bind(&slow);
  CallKind call_kind = CallICBase::Contextual::decode(extra_state_)
      ? CALL_AS_FUNCTION
      : CALL_AS_METHOD;
  ParameterCount expected(function);
  __ InvokeFunction(function, expected, arguments(),
                    JUMP_FUNCTION, NullCallWrapper(), call_kind);

  __ bind(&miss);
  // rcx: function name.
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(type, name);
}


Handle<Code> CallStubCompiler::CompileFastApiCall(
    const CallOptimization& optimization,
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Cell> cell,
    Handle<JSFunction> function,
    Handle<String> name) {
  ASSERT(optimization.is_simple_api_call());
  // Bail out if object is a global object as we don't want to
  // repatch it to global receiver.
  if (object->IsGlobalObject()) return Handle<Code>::null();
  if (!cell.is_null()) return Handle<Code>::null();
  if (!object->IsJSObject()) return Handle<Code>::null();
  int depth = optimization.GetPrototypeDepthOfExpectedType(
      Handle<JSObject>::cast(object), holder);
  if (depth == kInvalidProtoDepth) return Handle<Code>::null();

  Label miss, miss_before_stack_reserved;
  GenerateNameCheck(name, &miss_before_stack_reserved);

  const int argc = arguments().immediate();
  StackArgumentsAccessor args(rsp, argc);
  __ movq(rdx, args.GetReceiverOperand());

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(rdx, &miss_before_stack_reserved);

  Counters* counters = isolate()->counters();
  __ IncrementCounter(counters->call_const(), 1);
  __ IncrementCounter(counters->call_const_fast_api(), 1);

  // Allocate space for v8::Arguments implicit values. Must be initialized
  // before calling any runtime function.
  __ subq(rsp, Immediate(kFastApiCallArguments * kPointerSize));

  // Check that the maps haven't changed and find a Holder as a side effect.
  CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax, rdi,
                  name, depth, &miss);

  // Move the return address on top of the stack.
  __ movq(rax,
          StackOperandForReturnAddress(kFastApiCallArguments * kPointerSize));
  __ movq(StackOperandForReturnAddress(0), rax);

  GenerateFastApiCall(masm(), optimization, argc, false);

  __ bind(&miss);
  __ addq(rsp, Immediate(kFastApiCallArguments * kPointerSize));

  __ bind(&miss_before_stack_reserved);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(function);
}


void CallStubCompiler::CompileHandlerFrontend(Handle<Object> object,
                                              Handle<JSObject> holder,
                                              Handle<Name> name,
                                              CheckType check,
                                              Label* success) {
  // ----------- S t a t e -------------
  // rcx                 : function name
  // rsp[0]              : return address
  // rsp[8]              : argument argc
  // rsp[16]             : argument argc - 1
  // ...
  // rsp[argc * 8]       : argument 1
  // rsp[(argc + 1) * 8] : argument 0 = receiver
  // -----------------------------------
  Label miss;
  GenerateNameCheck(name, &miss);

  StackArgumentsAccessor args(rsp, arguments());
  __ movq(rdx, args.GetReceiverOperand());

  // Check that the receiver isn't a smi.
  if (check != NUMBER_CHECK) {
    __ JumpIfSmi(rdx, &miss);
  }

  // Make sure that it's okay not to patch the on stack receiver
  // unless we're doing a receiver map check.
  ASSERT(!object->IsGlobalObject() || check == RECEIVER_MAP_CHECK);

  Counters* counters = isolate()->counters();
  switch (check) {
    case RECEIVER_MAP_CHECK:
      __ IncrementCounter(counters->call_const(), 1);

      // Check that the maps haven't changed.
      CheckPrototypes(Handle<JSObject>::cast(object), rdx, holder, rbx, rax,
                      rdi, name, &miss);

      // Patch the receiver on the stack with the global proxy if
      // necessary.
      if (object->IsGlobalObject()) {
        __ movq(rdx, FieldOperand(rdx, GlobalObject::kGlobalReceiverOffset));
        __ movq(args.GetReceiverOperand(), rdx);
      }
      break;

    case STRING_CHECK:
      // Check that the object is a string.
      __ CmpObjectType(rdx, FIRST_NONSTRING_TYPE, rax);
      __ j(above_equal, &miss);
      // Check that the maps starting from the prototype haven't changed.
      GenerateDirectLoadGlobalFunctionPrototype(
          masm(), Context::STRING_FUNCTION_INDEX, rax, &miss);
      CheckPrototypes(
          Handle<JSObject>(JSObject::cast(object->GetPrototype(isolate()))),
          rax, holder, rbx, rdx, rdi, name, &miss);
      break;

    case SYMBOL_CHECK:
      // Check that the object is a symbol.
      __ CmpObjectType(rdx, SYMBOL_TYPE, rax);
      __ j(not_equal, &miss);
      // Check that the maps starting from the prototype haven't changed.
      GenerateDirectLoadGlobalFunctionPrototype(
          masm(), Context::SYMBOL_FUNCTION_INDEX, rax, &miss);
      CheckPrototypes(
          Handle<JSObject>(JSObject::cast(object->GetPrototype(isolate()))),
          rax, holder, rbx, rdx, rdi, name, &miss);
      break;

    case NUMBER_CHECK: {
      Label fast;
      // Check that the object is a smi or a heap number.
      __ JumpIfSmi(rdx, &fast);
      __ CmpObjectType(rdx, HEAP_NUMBER_TYPE, rax);
      __ j(not_equal, &miss);
      __ bind(&fast);
      // Check that the maps starting from the prototype haven't changed.
      GenerateDirectLoadGlobalFunctionPrototype(
          masm(), Context::NUMBER_FUNCTION_INDEX, rax, &miss);
      CheckPrototypes(
          Handle<JSObject>(JSObject::cast(object->GetPrototype(isolate()))),
          rax, holder, rbx, rdx, rdi, name, &miss);
      break;
    }
    case BOOLEAN_CHECK: {
      Label fast;
      // Check that the object is a boolean.
      __ CompareRoot(rdx, Heap::kTrueValueRootIndex);
      __ j(equal, &fast);
      __ CompareRoot(rdx, Heap::kFalseValueRootIndex);
      __ j(not_equal, &miss);
      __ bind(&fast);
      // Check that the maps starting from the prototype haven't changed.
      GenerateDirectLoadGlobalFunctionPrototype(
          masm(), Context::BOOLEAN_FUNCTION_INDEX, rax, &miss);
      CheckPrototypes(
          Handle<JSObject>(JSObject::cast(object->GetPrototype(isolate()))),
          rax, holder, rbx, rdx, rdi, name, &miss);
      break;
    }
  }

  __ jmp(success);

  // Handle call cache miss.
  __ bind(&miss);
  GenerateMissBranch();
}


void CallStubCompiler::CompileHandlerBackend(Handle<JSFunction> function) {
  CallKind call_kind = CallICBase::Contextual::decode(extra_state_)
      ? CALL_AS_FUNCTION
      : CALL_AS_METHOD;
  ParameterCount expected(function);
  __ InvokeFunction(function, expected, arguments(),
                    JUMP_FUNCTION, NullCallWrapper(), call_kind);
}


Handle<Code> CallStubCompiler::CompileCallConstant(
    Handle<Object> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    CheckType check,
    Handle<JSFunction> function) {
  if (HasCustomCallGenerator(function)) {
    Handle<Code> code = CompileCustomCall(object, holder,
                                          Handle<PropertyCell>::null(),
                                          function, Handle<String>::cast(name),
                                          Code::CONSTANT);
    // A null handle means bail out to the regular compiler code below.
    if (!code.is_null()) return code;
  }

  Label success;

  CompileHandlerFrontend(object, holder, name, check, &success);
  __ bind(&success);
  CompileHandlerBackend(function);

  // Return the generated code.
  return GetCode(function);
}


Handle<Code> CallStubCompiler::CompileCallInterceptor(Handle<JSObject> object,
                                                      Handle<JSObject> holder,
                                                      Handle<Name> name) {
  // ----------- S t a t e -------------
  // rcx                 : function name
  // rsp[0]              : return address
  // rsp[8]              : argument argc
  // rsp[16]             : argument argc - 1
  // ...
  // rsp[argc * 8]       : argument 1
  // rsp[(argc + 1) * 8] : argument 0 = receiver
  // -----------------------------------
  Label miss;
  GenerateNameCheck(name, &miss);


  LookupResult lookup(isolate());
  LookupPostInterceptor(holder, name, &lookup);

  // Get the receiver from the stack.
  StackArgumentsAccessor args(rsp, arguments());
  __ movq(rdx, args.GetReceiverOperand());

  CallInterceptorCompiler compiler(this, arguments(), rcx, extra_state_);
  compiler.Compile(masm(), object, holder, name, &lookup, rdx, rbx, rdi, rax,
                   &miss);

  // Restore receiver.
  __ movq(rdx, args.GetReceiverOperand());

  // Check that the function really is a function.
  __ JumpIfSmi(rax, &miss);
  __ CmpObjectType(rax, JS_FUNCTION_TYPE, rbx);
  __ j(not_equal, &miss);

  // Patch the receiver on the stack with the global proxy if
  // necessary.
  if (object->IsGlobalObject()) {
    __ movq(rdx, FieldOperand(rdx, GlobalObject::kGlobalReceiverOffset));
    __ movq(args.GetReceiverOperand(), rdx);
  }

  // Invoke the function.
  __ movq(rdi, rax);
  CallKind call_kind = CallICBase::Contextual::decode(extra_state_)
      ? CALL_AS_FUNCTION
      : CALL_AS_METHOD;
  __ InvokeFunction(rdi, arguments(), JUMP_FUNCTION,
                    NullCallWrapper(), call_kind);

  // Handle load cache miss.
  __ bind(&miss);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(Code::INTERCEPTOR, name);
}


Handle<Code> CallStubCompiler::CompileCallGlobal(
    Handle<JSObject> object,
    Handle<GlobalObject> holder,
    Handle<PropertyCell> cell,
    Handle<JSFunction> function,
    Handle<Name> name) {
  // ----------- S t a t e -------------
  // rcx                 : function name
  // rsp[0]              : return address
  // rsp[8]              : argument argc
  // rsp[16]             : argument argc - 1
  // ...
  // rsp[argc * 8]       : argument 1
  // rsp[(argc + 1) * 8] : argument 0 = receiver
  // -----------------------------------

  if (HasCustomCallGenerator(function)) {
    Handle<Code> code = CompileCustomCall(
        object, holder, cell, function, Handle<String>::cast(name),
        Code::NORMAL);
    // A null handle means bail out to the regular compiler code below.
    if (!code.is_null()) return code;
  }

  Label miss;
  GenerateNameCheck(name, &miss);

  StackArgumentsAccessor args(rsp, arguments());
  GenerateGlobalReceiverCheck(object, holder, name, &miss);
  GenerateLoadFunctionFromCell(cell, function, &miss);

  // Patch the receiver on the stack with the global proxy.
  if (object->IsGlobalObject()) {
    __ movq(rdx, FieldOperand(rdx, GlobalObject::kGlobalReceiverOffset));
    __ movq(args.GetReceiverOperand(), rdx);
  }

  // Set up the context (function already in rdi).
  __ movq(rsi, FieldOperand(rdi, JSFunction::kContextOffset));

  // Jump to the cached code (tail call).
  Counters* counters = isolate()->counters();
  __ IncrementCounter(counters->call_global_inline(), 1);
  ParameterCount expected(function->shared()->formal_parameter_count());
  CallKind call_kind = CallICBase::Contextual::decode(extra_state_)
      ? CALL_AS_FUNCTION
      : CALL_AS_METHOD;
  // We call indirectly through the code field in the function to
  // allow recompilation to take effect without changing any of the
  // call sites.
  __ movq(rdx, FieldOperand(rdi, JSFunction::kCodeEntryOffset));
  __ InvokeCode(rdx, expected, arguments(), JUMP_FUNCTION,
                NullCallWrapper(), call_kind);

  // Handle call cache miss.
  __ bind(&miss);
  __ IncrementCounter(counters->call_global_inline_miss(), 1);
  GenerateMissBranch();

  // Return the generated code.
  return GetCode(Code::NORMAL, name);
}


Handle<Code> StoreStubCompiler::CompileStoreCallback(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    Handle<ExecutableAccessorInfo> callback) {
  Label success;
  HandlerFrontend(object, receiver(), holder, name, &success);
  __ bind(&success);

  __ PopReturnAddressTo(scratch1());
  __ push(receiver());
  __ Push(callback);  // callback info
  __ Push(name);
  __ push(value());
  __ PushReturnAddressFrom(scratch1());

  // Do tail-call to the runtime system.
  ExternalReference store_callback_property =
      ExternalReference(IC_Utility(IC::kStoreCallbackProperty), isolate());
  __ TailCallExternalReference(store_callback_property, 4, 1);

  // Return the generated code.
  return GetCode(kind(), Code::CALLBACKS, name);
}


Handle<Code> StoreStubCompiler::CompileStoreCallback(
    Handle<JSObject> object,
    Handle<JSObject> holder,
    Handle<Name> name,
    const CallOptimization& call_optimization) {
  Label success;
  HandlerFrontend(object, receiver(), holder, name, &success);
  __ bind(&success);

  Register values[] = { value() };
  GenerateFastApiCall(
      masm(), call_optimization, receiver(), scratch3(), 1, values);

  // Return the generated code.
  return GetCode(kind(), Code::CALLBACKS, name);
}


#undef __
#define __ ACCESS_MASM(masm)


void StoreStubCompiler::GenerateStoreViaSetter(
    MacroAssembler* masm,
    Handle<JSFunction> setter) {
  // ----------- S t a t e -------------
  //  -- rax    : value
  //  -- rcx    : name
  //  -- rdx    : receiver
  //  -- rsp[0] : return address
  // -----------------------------------
  {
    FrameScope scope(masm, StackFrame::INTERNAL);

    // Save value register, so we can restore it later.
    __ push(rax);

    if (!setter.is_null()) {
      // Call the JavaScript setter with receiver and value on the stack.
      __ push(rdx);
      __ push(rax);
      ParameterCount actual(1);
      ParameterCount expected(setter);
      __ InvokeFunction(setter, expected, actual,
                        CALL_FUNCTION, NullCallWrapper(), CALL_AS_METHOD);
    } else {
      // If we generate a global code snippet for deoptimization only, remember
      // the place to continue after deoptimization.
      masm->isolate()->heap()->SetSetterStubDeoptPCOffset(masm->pc_offset());
    }

    // We have to return the passed value, not the return value of the setter.
    __ pop(rax);

    // Restore context register.
    __ movq(rsi, Operand(rbp, StandardFrameConstants::kContextOffset));
  }
  __ ret(0);
}


#undef __
#define __ ACCESS_MASM(masm())


Handle<Code> StoreStubCompiler::CompileStoreInterceptor(
    Handle<JSObject> object,
    Handle<Name> name) {
  __ PopReturnAddressTo(scratch1());
  __ push(receiver());
  __ push(this->name());
  __ push(value());
  __ Push(Smi::FromInt(strict_mode()));
  __ PushReturnAddressFrom(scratch1());

  // Do tail-call to the runtime system.
  ExternalReference store_ic_property =
      ExternalReference(IC_Utility(IC::kStoreInterceptorProperty), isolate());
  __ TailCallExternalReference(store_ic_property, 4, 1);

  // Return the generated code.
  return GetCode(kind(), Code::INTERCEPTOR, name);
}


Handle<Code> KeyedStoreStubCompiler::CompileStorePolymorphic(
    MapHandleList* receiver_maps,
    CodeHandleList* handler_stubs,
    MapHandleList* transitioned_maps) {
  Label miss;
  __ JumpIfSmi(receiver(), &miss, Label::kNear);

  __ movq(scratch1(), FieldOperand(receiver(), HeapObject::kMapOffset));
  int receiver_count = receiver_maps->length();
  for (int i = 0; i < receiver_count; ++i) {
    // Check map and tail call if there's a match
    __ Cmp(scratch1(), receiver_maps->at(i));
    if (transitioned_maps->at(i).is_null()) {
      __ j(equal, handler_stubs->at(i), RelocInfo::CODE_TARGET);
    } else {
      Label next_map;
      __ j(not_equal, &next_map, Label::kNear);
      __ movq(transition_map(),
              transitioned_maps->at(i),
              RelocInfo::EMBEDDED_OBJECT);
      __ jmp(handler_stubs->at(i), RelocInfo::CODE_TARGET);
      __ bind(&next_map);
    }
  }

  __ bind(&miss);

  TailCallBuiltin(masm(), MissBuiltin(kind()));

  // Return the generated code.
  return GetICCode(
      kind(), Code::NORMAL, factory()->empty_string(), POLYMORPHIC);
}


Handle<Code> LoadStubCompiler::CompileLoadNonexistent(
    Handle<JSObject> object,
    Handle<JSObject> last,
    Handle<Name> name,
    Handle<JSGlobalObject> global) {
  Label success;

  NonexistentHandlerFrontend(object, last, name, &success, global);

  __ bind(&success);
  // Return undefined if maps of the full prototype chain are still the
  // same and no global property with this name contains a value.
  __ LoadRoot(rax, Heap::kUndefinedValueRootIndex);
  __ ret(0);

  // Return the generated code.
  return GetCode(kind(), Code::NONEXISTENT, name);
}


Register* LoadStubCompiler::registers() {
  // receiver, name, scratch1, scratch2, scratch3, scratch4.
  static Register registers[] = { rax, rcx, rdx, rbx, rdi, r8 };
  return registers;
}


Register* KeyedLoadStubCompiler::registers() {
  // receiver, name, scratch1, scratch2, scratch3, scratch4.
  static Register registers[] = { rdx, rax, rbx, rcx, rdi, r8 };
  return registers;
}


Register* StoreStubCompiler::registers() {
  // receiver, name, value, scratch1, scratch2, scratch3.
  static Register registers[] = { rdx, rcx, rax, rbx, rdi, r8 };
  return registers;
}


Register* KeyedStoreStubCompiler::registers() {
  // receiver, name, value, scratch1, scratch2, scratch3.
  static Register registers[] = { rdx, rcx, rax, rbx, rdi, r8 };
  return registers;
}


void KeyedLoadStubCompiler::GenerateNameCheck(Handle<Name> name,
                                              Register name_reg,
                                              Label* miss) {
  __ Cmp(name_reg, name);
  __ j(not_equal, miss);
}


void KeyedStoreStubCompiler::GenerateNameCheck(Handle<Name> name,
                                               Register name_reg,
                                               Label* miss) {
  __ Cmp(name_reg, name);
  __ j(not_equal, miss);
}


#undef __
#define __ ACCESS_MASM(masm)


void LoadStubCompiler::GenerateLoadViaGetter(MacroAssembler* masm,
                                             Register receiver,
                                             Handle<JSFunction> getter) {
  // ----------- S t a t e -------------
  //  -- rax    : receiver
  //  -- rcx    : name
  //  -- rsp[0] : return address
  // -----------------------------------
  {
    FrameScope scope(masm, StackFrame::INTERNAL);

    if (!getter.is_null()) {
      // Call the JavaScript getter with the receiver on the stack.
      __ push(receiver);
      ParameterCount actual(0);
      ParameterCount expected(getter);
      __ InvokeFunction(getter, expected, actual,
                        CALL_FUNCTION, NullCallWrapper(), CALL_AS_METHOD);
    } else {
      // If we generate a global code snippet for deoptimization only, remember
      // the place to continue after deoptimization.
      masm->isolate()->heap()->SetGetterStubDeoptPCOffset(masm->pc_offset());
    }

    // Restore context register.
    __ movq(rsi, Operand(rbp, StandardFrameConstants::kContextOffset));
  }
  __ ret(0);
}


#undef __
#define __ ACCESS_MASM(masm())


Handle<Code> LoadStubCompiler::CompileLoadGlobal(
    Handle<JSObject> object,
    Handle<GlobalObject> global,
    Handle<PropertyCell> cell,
    Handle<Name> name,
    bool is_dont_delete) {
  Label success, miss;
  // TODO(verwaest): Directly store to rax. Currently we cannot do this, since
  // rax is used as receiver(), which we would otherwise clobber before a
  // potential miss.

  __ CheckMap(receiver(), Handle<Map>(object->map()), &miss, DO_SMI_CHECK);
  HandlerFrontendHeader(
      object, receiver(), Handle<JSObject>::cast(global), name, &miss);

  // Get the value from the cell.
  __ Move(rbx, cell);
  __ movq(rbx, FieldOperand(rbx, PropertyCell::kValueOffset));

  // Check for deleted property if property can actually be deleted.
  if (!is_dont_delete) {
    __ CompareRoot(rbx, Heap::kTheHoleValueRootIndex);
    __ j(equal, &miss);
  } else if (FLAG_debug_code) {
    __ CompareRoot(rbx, Heap::kTheHoleValueRootIndex);
    __ Check(not_equal, kDontDeleteCellsCannotContainTheHole);
  }

  HandlerFrontendFooter(name, &success, &miss);
  __ bind(&success);

  Counters* counters = isolate()->counters();
  __ IncrementCounter(counters->named_load_global_stub(), 1);
  __ movq(rax, rbx);
  __ ret(0);

  // Return the generated code.
  return GetICCode(kind(), Code::NORMAL, name);
}


Handle<Code> BaseLoadStoreStubCompiler::CompilePolymorphicIC(
    MapHandleList* receiver_maps,
    CodeHandleList* handlers,
    Handle<Name> name,
    Code::StubType type,
    IcCheckType check) {
  Label miss;

  if (check == PROPERTY) {
    GenerateNameCheck(name, this->name(), &miss);
  }

  __ JumpIfSmi(receiver(), &miss);
  Register map_reg = scratch1();
  __ movq(map_reg, FieldOperand(receiver(), HeapObject::kMapOffset));
  int receiver_count = receiver_maps->length();
  int number_of_handled_maps = 0;
  for (int current = 0; current < receiver_count; ++current) {
    Handle<Map> map = receiver_maps->at(current);
    if (!map->is_deprecated()) {
      number_of_handled_maps++;
      // Check map and tail call if there's a match
      __ Cmp(map_reg, receiver_maps->at(current));
      __ j(equal, handlers->at(current), RelocInfo::CODE_TARGET);
    }
  }
  ASSERT(number_of_handled_maps > 0);

  __  bind(&miss);
  TailCallBuiltin(masm(), MissBuiltin(kind()));

  // Return the generated code.
  InlineCacheState state =
      number_of_handled_maps > 1 ? POLYMORPHIC : MONOMORPHIC;
  return GetICCode(kind(), type, name, state);
}


#undef __
#define __ ACCESS_MASM(masm)


void KeyedLoadStubCompiler::GenerateLoadDictionaryElement(
    MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- rax    : key
  //  -- rdx    : receiver
  //  -- rsp[0] : return address
  // -----------------------------------
  Label slow, miss_force_generic;

  // This stub is meant to be tail-jumped to, the receiver must already
  // have been verified by the caller to not be a smi.

  __ JumpIfNotSmi(rax, &miss_force_generic);
  __ SmiToInteger32(rbx, rax);
  __ movq(rcx, FieldOperand(rdx, JSObject::kElementsOffset));

  // Check whether the elements is a number dictionary.
  // rdx: receiver
  // rax: key
  // rbx: key as untagged int32
  // rcx: elements
  __ LoadFromNumberDictionary(&slow, rcx, rax, rbx, r9, rdi, rax);
  __ ret(0);

  __ bind(&slow);
  // ----------- S t a t e -------------
  //  -- rax    : key
  //  -- rdx    : receiver
  //  -- rsp[0] : return address
  // -----------------------------------
  TailCallBuiltin(masm, Builtins::kKeyedLoadIC_Slow);

  __ bind(&miss_force_generic);
  // ----------- S t a t e -------------
  //  -- rax    : key
  //  -- rdx    : receiver
  //  -- rsp[0] : return address
  // -----------------------------------
  TailCallBuiltin(masm, Builtins::kKeyedLoadIC_MissForceGeneric);
}


#undef __

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_X64
