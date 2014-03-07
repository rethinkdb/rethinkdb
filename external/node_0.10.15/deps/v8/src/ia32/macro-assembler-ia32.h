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

#ifndef V8_IA32_MACRO_ASSEMBLER_IA32_H_
#define V8_IA32_MACRO_ASSEMBLER_IA32_H_

#include "assembler.h"
#include "frames.h"
#include "v8globals.h"

namespace v8 {
namespace internal {

// Flags used for the AllocateInNewSpace functions.
enum AllocationFlags {
  // No special flags.
  NO_ALLOCATION_FLAGS = 0,
  // Return the pointer to the allocated already tagged as a heap object.
  TAG_OBJECT = 1 << 0,
  // The content of the result register already contains the allocation top in
  // new space.
  RESULT_CONTAINS_TOP = 1 << 1
};


// Convenience for platform-independent signatures.  We do not normally
// distinguish memory operands from other operands on ia32.
typedef Operand MemOperand;

enum RememberedSetAction { EMIT_REMEMBERED_SET, OMIT_REMEMBERED_SET };
enum SmiCheck { INLINE_SMI_CHECK, OMIT_SMI_CHECK };


bool AreAliased(Register r1, Register r2, Register r3, Register r4);


// MacroAssembler implements a collection of frequently used macros.
class MacroAssembler: public Assembler {
 public:
  // The isolate parameter can be NULL if the macro assembler should
  // not use isolate-dependent functionality. In this case, it's the
  // responsibility of the caller to never invoke such function on the
  // macro assembler.
  MacroAssembler(Isolate* isolate, void* buffer, int size);

  // ---------------------------------------------------------------------------
  // GC Support
  enum RememberedSetFinalAction {
    kReturnAtEnd,
    kFallThroughAtEnd
  };

  // Record in the remembered set the fact that we have a pointer to new space
  // at the address pointed to by the addr register.  Only works if addr is not
  // in new space.
  void RememberedSetHelper(Register object,  // Used for debug code.
                           Register addr,
                           Register scratch,
                           SaveFPRegsMode save_fp,
                           RememberedSetFinalAction and_then);

  void CheckPageFlag(Register object,
                     Register scratch,
                     int mask,
                     Condition cc,
                     Label* condition_met,
                     Label::Distance condition_met_distance = Label::kFar);

  void CheckPageFlagForMap(
      Handle<Map> map,
      int mask,
      Condition cc,
      Label* condition_met,
      Label::Distance condition_met_distance = Label::kFar);

  // Check if object is in new space.  Jumps if the object is not in new space.
  // The register scratch can be object itself, but scratch will be clobbered.
  void JumpIfNotInNewSpace(Register object,
                           Register scratch,
                           Label* branch,
                           Label::Distance distance = Label::kFar) {
    InNewSpace(object, scratch, zero, branch, distance);
  }

  // Check if object is in new space.  Jumps if the object is in new space.
  // The register scratch can be object itself, but it will be clobbered.
  void JumpIfInNewSpace(Register object,
                        Register scratch,
                        Label* branch,
                        Label::Distance distance = Label::kFar) {
    InNewSpace(object, scratch, not_zero, branch, distance);
  }

  // Check if an object has a given incremental marking color.  Also uses ecx!
  void HasColor(Register object,
                Register scratch0,
                Register scratch1,
                Label* has_color,
                Label::Distance has_color_distance,
                int first_bit,
                int second_bit);

  void JumpIfBlack(Register object,
                   Register scratch0,
                   Register scratch1,
                   Label* on_black,
                   Label::Distance on_black_distance = Label::kFar);

  // Checks the color of an object.  If the object is already grey or black
  // then we just fall through, since it is already live.  If it is white and
  // we can determine that it doesn't need to be scanned, then we just mark it
  // black and fall through.  For the rest we jump to the label so the
  // incremental marker can fix its assumptions.
  void EnsureNotWhite(Register object,
                      Register scratch1,
                      Register scratch2,
                      Label* object_is_white_and_not_data,
                      Label::Distance distance);

  // Notify the garbage collector that we wrote a pointer into an object.
  // |object| is the object being stored into, |value| is the object being
  // stored.  value and scratch registers are clobbered by the operation.
  // The offset is the offset from the start of the object, not the offset from
  // the tagged HeapObject pointer.  For use with FieldOperand(reg, off).
  void RecordWriteField(
      Register object,
      int offset,
      Register value,
      Register scratch,
      SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = EMIT_REMEMBERED_SET,
      SmiCheck smi_check = INLINE_SMI_CHECK);

  // As above, but the offset has the tag presubtracted.  For use with
  // Operand(reg, off).
  void RecordWriteContextSlot(
      Register context,
      int offset,
      Register value,
      Register scratch,
      SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = EMIT_REMEMBERED_SET,
      SmiCheck smi_check = INLINE_SMI_CHECK) {
    RecordWriteField(context,
                     offset + kHeapObjectTag,
                     value,
                     scratch,
                     save_fp,
                     remembered_set_action,
                     smi_check);
  }

  // Notify the garbage collector that we wrote a pointer into a fixed array.
  // |array| is the array being stored into, |value| is the
  // object being stored.  |index| is the array index represented as a
  // Smi. All registers are clobbered by the operation RecordWriteArray
  // filters out smis so it does not update the write barrier if the
  // value is a smi.
  void RecordWriteArray(
      Register array,
      Register value,
      Register index,
      SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = EMIT_REMEMBERED_SET,
      SmiCheck smi_check = INLINE_SMI_CHECK);

  // For page containing |object| mark region covering |address|
  // dirty. |object| is the object being stored into, |value| is the
  // object being stored. The address and value registers are clobbered by the
  // operation. RecordWrite filters out smis so it does not update the
  // write barrier if the value is a smi.
  void RecordWrite(
      Register object,
      Register address,
      Register value,
      SaveFPRegsMode save_fp,
      RememberedSetAction remembered_set_action = EMIT_REMEMBERED_SET,
      SmiCheck smi_check = INLINE_SMI_CHECK);

  // For page containing |object| mark the region covering the object's map
  // dirty. |object| is the object being stored into, |map| is the Map object
  // that was stored.
  void RecordWriteForMap(
      Register object,
      Handle<Map> map,
      Register scratch1,
      Register scratch2,
      SaveFPRegsMode save_fp);

#ifdef ENABLE_DEBUGGER_SUPPORT
  // ---------------------------------------------------------------------------
  // Debugger Support

  void DebugBreak();
#endif

  // Enter specific kind of exit frame. Expects the number of
  // arguments in register eax and sets up the number of arguments in
  // register edi and the pointer to the first argument in register
  // esi.
  void EnterExitFrame(bool save_doubles);

  void EnterApiExitFrame(int argc);

  // Leave the current exit frame. Expects the return value in
  // register eax:edx (untouched) and the pointer to the first
  // argument in register esi.
  void LeaveExitFrame(bool save_doubles);

  // Leave the current exit frame. Expects the return value in
  // register eax (untouched).
  void LeaveApiExitFrame();

  // Find the function context up the context chain.
  void LoadContext(Register dst, int context_chain_length);

  // Conditionally load the cached Array transitioned map of type
  // transitioned_kind from the native context if the map in register
  // map_in_out is the cached Array map in the native context of
  // expected_kind.
  void LoadTransitionedArrayMapConditional(
      ElementsKind expected_kind,
      ElementsKind transitioned_kind,
      Register map_in_out,
      Register scratch,
      Label* no_map_match);

  // Load the initial map for new Arrays from a JSFunction.
  void LoadInitialArrayMap(Register function_in,
                           Register scratch,
                           Register map_out,
                           bool can_have_holes);

  // Load the global function with the given index.
  void LoadGlobalFunction(int index, Register function);

  // Load the initial map from the global function. The registers
  // function and map can be the same.
  void LoadGlobalFunctionInitialMap(Register function, Register map);

  // Push and pop the registers that can hold pointers.
  void PushSafepointRegisters() { pushad(); }
  void PopSafepointRegisters() { popad(); }
  // Store the value in register/immediate src in the safepoint
  // register stack slot for register dst.
  void StoreToSafepointRegisterSlot(Register dst, Register src);
  void StoreToSafepointRegisterSlot(Register dst, Immediate src);
  void LoadFromSafepointRegisterSlot(Register dst, Register src);

  void LoadHeapObject(Register result, Handle<HeapObject> object);
  void PushHeapObject(Handle<HeapObject> object);

  void LoadObject(Register result, Handle<Object> object) {
    if (object->IsHeapObject()) {
      LoadHeapObject(result, Handle<HeapObject>::cast(object));
    } else {
      Set(result, Immediate(object));
    }
  }

  // ---------------------------------------------------------------------------
  // JavaScript invokes

  // Set up call kind marking in ecx. The method takes ecx as an
  // explicit first parameter to make the code more readable at the
  // call sites.
  void SetCallKind(Register dst, CallKind kind);

  // Invoke the JavaScript function code by either calling or jumping.
  void InvokeCode(Register code,
                  const ParameterCount& expected,
                  const ParameterCount& actual,
                  InvokeFlag flag,
                  const CallWrapper& call_wrapper,
                  CallKind call_kind) {
    InvokeCode(Operand(code), expected, actual, flag, call_wrapper, call_kind);
  }

  void InvokeCode(const Operand& code,
                  const ParameterCount& expected,
                  const ParameterCount& actual,
                  InvokeFlag flag,
                  const CallWrapper& call_wrapper,
                  CallKind call_kind);

  void InvokeCode(Handle<Code> code,
                  const ParameterCount& expected,
                  const ParameterCount& actual,
                  RelocInfo::Mode rmode,
                  InvokeFlag flag,
                  const CallWrapper& call_wrapper,
                  CallKind call_kind);

  // Invoke the JavaScript function in the given register. Changes the
  // current context to the context in the function before invoking.
  void InvokeFunction(Register function,
                      const ParameterCount& actual,
                      InvokeFlag flag,
                      const CallWrapper& call_wrapper,
                      CallKind call_kind);

  void InvokeFunction(Handle<JSFunction> function,
                      const ParameterCount& actual,
                      InvokeFlag flag,
                      const CallWrapper& call_wrapper,
                      CallKind call_kind);

  // Invoke specified builtin JavaScript function. Adds an entry to
  // the unresolved list if the name does not resolve.
  void InvokeBuiltin(Builtins::JavaScript id,
                     InvokeFlag flag,
                     const CallWrapper& call_wrapper = NullCallWrapper());

  // Store the function for the given builtin in the target register.
  void GetBuiltinFunction(Register target, Builtins::JavaScript id);

  // Store the code object for the given builtin in the target register.
  void GetBuiltinEntry(Register target, Builtins::JavaScript id);

  // Expression support
  void Set(Register dst, const Immediate& x);
  void Set(const Operand& dst, const Immediate& x);

  // Support for constant splitting.
  bool IsUnsafeImmediate(const Immediate& x);
  void SafeSet(Register dst, const Immediate& x);
  void SafePush(const Immediate& x);

  // Compare against a known root, e.g. undefined, null, true, ...
  void CompareRoot(Register with, Heap::RootListIndex index);
  void CompareRoot(const Operand& with, Heap::RootListIndex index);

  // Compare object type for heap object.
  // Incoming register is heap_object and outgoing register is map.
  void CmpObjectType(Register heap_object, InstanceType type, Register map);

  // Compare instance type for map.
  void CmpInstanceType(Register map, InstanceType type);

  // Check if a map for a JSObject indicates that the object has fast elements.
  // Jump to the specified label if it does not.
  void CheckFastElements(Register map,
                         Label* fail,
                         Label::Distance distance = Label::kFar);

  // Check if a map for a JSObject indicates that the object can have both smi
  // and HeapObject elements.  Jump to the specified label if it does not.
  void CheckFastObjectElements(Register map,
                               Label* fail,
                               Label::Distance distance = Label::kFar);

  // Check if a map for a JSObject indicates that the object has fast smi only
  // elements.  Jump to the specified label if it does not.
  void CheckFastSmiElements(Register map,
                            Label* fail,
                            Label::Distance distance = Label::kFar);

  // Check to see if maybe_number can be stored as a double in
  // FastDoubleElements. If it can, store it at the index specified by key in
  // the FastDoubleElements array elements, otherwise jump to fail.
  void StoreNumberToDoubleElements(Register maybe_number,
                                   Register elements,
                                   Register key,
                                   Register scratch1,
                                   XMMRegister scratch2,
                                   Label* fail,
                                   bool specialize_for_processor);

  // Compare an object's map with the specified map and its transitioned
  // elements maps if mode is ALLOW_ELEMENT_TRANSITION_MAPS. FLAGS are set with
  // result of map compare. If multiple map compares are required, the compare
  // sequences branches to early_success.
  void CompareMap(Register obj,
                  Handle<Map> map,
                  Label* early_success,
                  CompareMapMode mode = REQUIRE_EXACT_MAP);

  // Check if the map of an object is equal to a specified map and branch to
  // label if not. Skip the smi check if not required (object is known to be a
  // heap object). If mode is ALLOW_ELEMENT_TRANSITION_MAPS, then also match
  // against maps that are ElementsKind transition maps of the specified map.
  void CheckMap(Register obj,
                Handle<Map> map,
                Label* fail,
                SmiCheckType smi_check_type,
                CompareMapMode mode = REQUIRE_EXACT_MAP);

  // Check if the map of an object is equal to a specified map and branch to a
  // specified target if equal. Skip the smi check if not required (object is
  // known to be a heap object)
  void DispatchMap(Register obj,
                   Handle<Map> map,
                   Handle<Code> success,
                   SmiCheckType smi_check_type);

  // Check if the object in register heap_object is a string. Afterwards the
  // register map contains the object map and the register instance_type
  // contains the instance_type. The registers map and instance_type can be the
  // same in which case it contains the instance type afterwards. Either of the
  // registers map and instance_type can be the same as heap_object.
  Condition IsObjectStringType(Register heap_object,
                               Register map,
                               Register instance_type);

  // Check if a heap object's type is in the JSObject range, not including
  // JSFunction.  The object's map will be loaded in the map register.
  // Any or all of the three registers may be the same.
  // The contents of the scratch register will always be overwritten.
  void IsObjectJSObjectType(Register heap_object,
                            Register map,
                            Register scratch,
                            Label* fail);

  // The contents of the scratch register will be overwritten.
  void IsInstanceJSObjectType(Register map, Register scratch, Label* fail);

  // FCmp is similar to integer cmp, but requires unsigned
  // jcc instructions (je, ja, jae, jb, jbe, je, and jz).
  void FCmp();

  void ClampUint8(Register reg);

  void ClampDoubleToUint8(XMMRegister input_reg,
                          XMMRegister scratch_reg,
                          Register result_reg);


  // Smi tagging support.
  void SmiTag(Register reg) {
    STATIC_ASSERT(kSmiTag == 0);
    STATIC_ASSERT(kSmiTagSize == 1);
    add(reg, reg);
  }
  void SmiUntag(Register reg) {
    sar(reg, kSmiTagSize);
  }

  // Modifies the register even if it does not contain a Smi!
  void SmiUntag(Register reg, Label* is_smi) {
    STATIC_ASSERT(kSmiTagSize == 1);
    sar(reg, kSmiTagSize);
    STATIC_ASSERT(kSmiTag == 0);
    j(not_carry, is_smi);
  }

  void LoadUint32(XMMRegister dst, Register src, XMMRegister scratch);

  // Jump the register contains a smi.
  inline void JumpIfSmi(Register value,
                        Label* smi_label,
                        Label::Distance distance = Label::kFar) {
    test(value, Immediate(kSmiTagMask));
    j(zero, smi_label, distance);
  }
  // Jump if the operand is a smi.
  inline void JumpIfSmi(Operand value,
                        Label* smi_label,
                        Label::Distance distance = Label::kFar) {
    test(value, Immediate(kSmiTagMask));
    j(zero, smi_label, distance);
  }
  // Jump if register contain a non-smi.
  inline void JumpIfNotSmi(Register value,
                           Label* not_smi_label,
                           Label::Distance distance = Label::kFar) {
    test(value, Immediate(kSmiTagMask));
    j(not_zero, not_smi_label, distance);
  }

  void LoadInstanceDescriptors(Register map, Register descriptors);
  void EnumLength(Register dst, Register map);
  void NumberOfOwnDescriptors(Register dst, Register map);

  template<typename Field>
  void DecodeField(Register reg) {
    static const int shift = Field::kShift;
    static const int mask = (Field::kMask >> Field::kShift) << kSmiTagSize;
    sar(reg, shift);
    and_(reg, Immediate(mask));
  }
  void LoadPowerOf2(XMMRegister dst, Register scratch, int power);

  // Abort execution if argument is not a number, enabled via --debug-code.
  void AssertNumber(Register object);

  // Abort execution if argument is not a smi, enabled via --debug-code.
  void AssertSmi(Register object);

  // Abort execution if argument is a smi, enabled via --debug-code.
  void AssertNotSmi(Register object);

  // Abort execution if argument is not a string, enabled via --debug-code.
  void AssertString(Register object);

  // ---------------------------------------------------------------------------
  // Exception handling

  // Push a new try handler and link it into try handler chain.
  void PushTryHandler(StackHandler::Kind kind, int handler_index);

  // Unlink the stack handler on top of the stack from the try handler chain.
  void PopTryHandler();

  // Throw to the top handler in the try hander chain.
  void Throw(Register value);

  // Throw past all JS frames to the top JS entry frame.
  void ThrowUncatchable(Register value);

  // ---------------------------------------------------------------------------
  // Inline caching support

  // Generate code for checking access rights - used for security checks
  // on access to global objects across environments. The holder register
  // is left untouched, but the scratch register is clobbered.
  void CheckAccessGlobalProxy(Register holder_reg,
                              Register scratch,
                              Label* miss);

  void GetNumberHash(Register r0, Register scratch);

  void LoadFromNumberDictionary(Label* miss,
                                Register elements,
                                Register key,
                                Register r0,
                                Register r1,
                                Register r2,
                                Register result);


  // ---------------------------------------------------------------------------
  // Allocation support

  // Allocate an object in new space. If the new space is exhausted control
  // continues at the gc_required label. The allocated object is returned in
  // result and end of the new object is returned in result_end. The register
  // scratch can be passed as no_reg in which case an additional object
  // reference will be added to the reloc info. The returned pointers in result
  // and result_end have not yet been tagged as heap objects. If
  // result_contains_top_on_entry is true the content of result is known to be
  // the allocation top on entry (could be result_end from a previous call to
  // AllocateInNewSpace). If result_contains_top_on_entry is true scratch
  // should be no_reg as it is never used.
  void AllocateInNewSpace(int object_size,
                          Register result,
                          Register result_end,
                          Register scratch,
                          Label* gc_required,
                          AllocationFlags flags);

  void AllocateInNewSpace(int header_size,
                          ScaleFactor element_size,
                          Register element_count,
                          Register result,
                          Register result_end,
                          Register scratch,
                          Label* gc_required,
                          AllocationFlags flags);

  void AllocateInNewSpace(Register object_size,
                          Register result,
                          Register result_end,
                          Register scratch,
                          Label* gc_required,
                          AllocationFlags flags);

  // Undo allocation in new space. The object passed and objects allocated after
  // it will no longer be allocated. Make sure that no pointers are left to the
  // object(s) no longer allocated as they would be invalid when allocation is
  // un-done.
  void UndoAllocationInNewSpace(Register object);

  // Allocate a heap number in new space with undefined value. The
  // register scratch2 can be passed as no_reg; the others must be
  // valid registers. Returns tagged pointer in result register, or
  // jumps to gc_required if new space is full.
  void AllocateHeapNumber(Register result,
                          Register scratch1,
                          Register scratch2,
                          Label* gc_required);

  // Allocate a sequential string. All the header fields of the string object
  // are initialized.
  void AllocateTwoByteString(Register result,
                             Register length,
                             Register scratch1,
                             Register scratch2,
                             Register scratch3,
                             Label* gc_required);
  void AllocateAsciiString(Register result,
                           Register length,
                           Register scratch1,
                           Register scratch2,
                           Register scratch3,
                           Label* gc_required);
  void AllocateAsciiString(Register result,
                           int length,
                           Register scratch1,
                           Register scratch2,
                           Label* gc_required);

  // Allocate a raw cons string object. Only the map field of the result is
  // initialized.
  void AllocateTwoByteConsString(Register result,
                          Register scratch1,
                          Register scratch2,
                          Label* gc_required);
  void AllocateAsciiConsString(Register result,
                               Register scratch1,
                               Register scratch2,
                               Label* gc_required);

  // Allocate a raw sliced string object. Only the map field of the result is
  // initialized.
  void AllocateTwoByteSlicedString(Register result,
                            Register scratch1,
                            Register scratch2,
                            Label* gc_required);
  void AllocateAsciiSlicedString(Register result,
                                 Register scratch1,
                                 Register scratch2,
                                 Label* gc_required);

  // Copy memory, byte-by-byte, from source to destination.  Not optimized for
  // long or aligned copies.
  // The contents of index and scratch are destroyed.
  void CopyBytes(Register source,
                 Register destination,
                 Register length,
                 Register scratch);

  // Initialize fields with filler values.  Fields starting at |start_offset|
  // not including end_offset are overwritten with the value in |filler|.  At
  // the end the loop, |start_offset| takes the value of |end_offset|.
  void InitializeFieldsWithFiller(Register start_offset,
                                  Register end_offset,
                                  Register filler);

  // ---------------------------------------------------------------------------
  // Support functions.

  // Check a boolean-bit of a Smi field.
  void BooleanBitTest(Register object, int field_offset, int bit_index);

  // Check if result is zero and op is negative.
  void NegativeZeroTest(Register result, Register op, Label* then_label);

  // Check if result is zero and any of op1 and op2 are negative.
  // Register scratch is destroyed, and it must be different from op2.
  void NegativeZeroTest(Register result, Register op1, Register op2,
                        Register scratch, Label* then_label);

  // Try to get function prototype of a function and puts the value in
  // the result register. Checks that the function really is a
  // function and jumps to the miss label if the fast checks fail. The
  // function register will be untouched; the other registers may be
  // clobbered.
  void TryGetFunctionPrototype(Register function,
                               Register result,
                               Register scratch,
                               Label* miss,
                               bool miss_on_bound_function = false);

  // Generates code for reporting that an illegal operation has
  // occurred.
  void IllegalOperation(int num_arguments);

  // Picks out an array index from the hash field.
  // Register use:
  //   hash - holds the index's hash. Clobbered.
  //   index - holds the overwritten index on exit.
  void IndexFromHash(Register hash, Register index);

  // ---------------------------------------------------------------------------
  // Runtime calls

  // Call a code stub.  Generate the code if necessary.
  void CallStub(CodeStub* stub, TypeFeedbackId ast_id = TypeFeedbackId::None());

  // Tail call a code stub (jump).  Generate the code if necessary.
  void TailCallStub(CodeStub* stub);

  // Return from a code stub after popping its arguments.
  void StubReturn(int argc);

  // Call a runtime routine.
  void CallRuntime(const Runtime::Function* f, int num_arguments);
  void CallRuntimeSaveDoubles(Runtime::FunctionId id);

  // Convenience function: Same as above, but takes the fid instead.
  void CallRuntime(Runtime::FunctionId id, int num_arguments);

  // Convenience function: call an external reference.
  void CallExternalReference(ExternalReference ref, int num_arguments);

  // Tail call of a runtime routine (jump).
  // Like JumpToExternalReference, but also takes care of passing the number
  // of parameters.
  void TailCallExternalReference(const ExternalReference& ext,
                                 int num_arguments,
                                 int result_size);

  // Convenience function: tail call a runtime routine (jump).
  void TailCallRuntime(Runtime::FunctionId fid,
                       int num_arguments,
                       int result_size);

  // Before calling a C-function from generated code, align arguments on stack.
  // After aligning the frame, arguments must be stored in esp[0], esp[4],
  // etc., not pushed. The argument count assumes all arguments are word sized.
  // Some compilers/platforms require the stack to be aligned when calling
  // C++ code.
  // Needs a scratch register to do some arithmetic. This register will be
  // trashed.
  void PrepareCallCFunction(int num_arguments, Register scratch);

  // Calls a C function and cleans up the space for arguments allocated
  // by PrepareCallCFunction. The called function is not allowed to trigger a
  // garbage collection, since that might move the code and invalidate the
  // return address (unless this is somehow accounted for by the called
  // function).
  void CallCFunction(ExternalReference function, int num_arguments);
  void CallCFunction(Register function, int num_arguments);

  // Prepares stack to put arguments (aligns and so on). Reserves
  // space for return value if needed (assumes the return value is a handle).
  // Arguments must be stored in ApiParameterOperand(0), ApiParameterOperand(1)
  // etc. Saves context (esi). If space was reserved for return value then
  // stores the pointer to the reserved slot into esi.
  void PrepareCallApiFunction(int argc);

  // Calls an API function.  Allocates HandleScope, extracts returned value
  // from handle and propagates exceptions.  Clobbers ebx, edi and
  // caller-save registers.  Restores context.  On return removes
  // stack_space * kPointerSize (GCed).
  void CallApiFunctionAndReturn(Address function_address, int stack_space);

  // Jump to a runtime routine.
  void JumpToExternalReference(const ExternalReference& ext);

  // ---------------------------------------------------------------------------
  // Utilities

  void Ret();

  // Return and drop arguments from stack, where the number of arguments
  // may be bigger than 2^16 - 1.  Requires a scratch register.
  void Ret(int bytes_dropped, Register scratch);

  // Emit code to discard a non-negative number of pointer-sized elements
  // from the stack, clobbering only the esp register.
  void Drop(int element_count);

  void Call(Label* target) { call(target); }

  // Emit call to the code we are currently generating.
  void CallSelf() {
    Handle<Code> self(reinterpret_cast<Code**>(CodeObject().location()));
    call(self, RelocInfo::CODE_TARGET);
  }

  // Move if the registers are not identical.
  void Move(Register target, Register source);

  // Push a handle value.
  void Push(Handle<Object> handle) { push(Immediate(handle)); }

  Handle<Object> CodeObject() {
    ASSERT(!code_object_.is_null());
    return code_object_;
  }


  // ---------------------------------------------------------------------------
  // StatsCounter support

  void SetCounter(StatsCounter* counter, int value);
  void IncrementCounter(StatsCounter* counter, int value);
  void DecrementCounter(StatsCounter* counter, int value);
  void IncrementCounter(Condition cc, StatsCounter* counter, int value);
  void DecrementCounter(Condition cc, StatsCounter* counter, int value);


  // ---------------------------------------------------------------------------
  // Debugging

  // Calls Abort(msg) if the condition cc is not satisfied.
  // Use --debug_code to enable.
  void Assert(Condition cc, const char* msg);

  void AssertFastElements(Register elements);

  // Like Assert(), but always enabled.
  void Check(Condition cc, const char* msg);

  // Print a message to stdout and abort execution.
  void Abort(const char* msg);

  // Check that the stack is aligned.
  void CheckStackAlignment();

  // Verify restrictions about code generated in stubs.
  void set_generating_stub(bool value) { generating_stub_ = value; }
  bool generating_stub() { return generating_stub_; }
  void set_allow_stub_calls(bool value) { allow_stub_calls_ = value; }
  bool allow_stub_calls() { return allow_stub_calls_; }
  void set_has_frame(bool value) { has_frame_ = value; }
  bool has_frame() { return has_frame_; }
  inline bool AllowThisStubCall(CodeStub* stub);

  // ---------------------------------------------------------------------------
  // String utilities.

  // Check whether the instance type represents a flat ASCII string. Jump to the
  // label if not. If the instance type can be scratched specify same register
  // for both instance type and scratch.
  void JumpIfInstanceTypeIsNotSequentialAscii(Register instance_type,
                                              Register scratch,
                                              Label* on_not_flat_ascii_string);

  // Checks if both objects are sequential ASCII strings, and jumps to label
  // if either is not.
  void JumpIfNotBothSequentialAsciiStrings(Register object1,
                                           Register object2,
                                           Register scratch1,
                                           Register scratch2,
                                           Label* on_not_flat_ascii_strings);

  static int SafepointRegisterStackIndex(Register reg) {
    return SafepointRegisterStackIndex(reg.code());
  }

  // Activation support.
  void EnterFrame(StackFrame::Type type);
  void LeaveFrame(StackFrame::Type type);

  // Expects object in eax and returns map with validated enum cache
  // in eax.  Assumes that any other register can be used as a scratch.
  void CheckEnumCache(Label* call_runtime);

 private:
  bool generating_stub_;
  bool allow_stub_calls_;
  bool has_frame_;
  // This handle will be patched with the code object on installation.
  Handle<Object> code_object_;

  // Helper functions for generating invokes.
  void InvokePrologue(const ParameterCount& expected,
                      const ParameterCount& actual,
                      Handle<Code> code_constant,
                      const Operand& code_operand,
                      Label* done,
                      bool* definitely_mismatches,
                      InvokeFlag flag,
                      Label::Distance done_distance,
                      const CallWrapper& call_wrapper = NullCallWrapper(),
                      CallKind call_kind = CALL_AS_METHOD);

  void EnterExitFramePrologue();
  void EnterExitFrameEpilogue(int argc, bool save_doubles);

  void LeaveExitFrameEpilogue();

  // Allocation support helpers.
  void LoadAllocationTopHelper(Register result,
                               Register scratch,
                               AllocationFlags flags);
  void UpdateAllocationTopHelper(Register result_end, Register scratch);

  // Helper for PopHandleScope.  Allowed to perform a GC and returns
  // NULL if gc_allowed.  Does not perform a GC if !gc_allowed, and
  // possibly returns a failure object indicating an allocation failure.
  MUST_USE_RESULT MaybeObject* PopHandleScopeHelper(Register saved,
                                                    Register scratch,
                                                    bool gc_allowed);

  // Helper for implementing JumpIfNotInNewSpace and JumpIfInNewSpace.
  void InNewSpace(Register object,
                  Register scratch,
                  Condition cc,
                  Label* condition_met,
                  Label::Distance condition_met_distance = Label::kFar);

  // Helper for finding the mark bits for an address.  Afterwards, the
  // bitmap register points at the word with the mark bits and the mask
  // the position of the first bit.  Uses ecx as scratch and leaves addr_reg
  // unchanged.
  inline void GetMarkBits(Register addr_reg,
                          Register bitmap_reg,
                          Register mask_reg);

  // Helper for throwing exceptions.  Compute a handler address and jump to
  // it.  See the implementation for register usage.
  void JumpToHandlerEntry();

  // Compute memory operands for safepoint stack slots.
  Operand SafepointRegisterSlot(Register reg);
  static int SafepointRegisterStackIndex(int reg_code);

  // Needs access to SafepointRegisterStackIndex for optimized frame
  // traversal.
  friend class OptimizedFrame;
};


// The code patcher is used to patch (typically) small parts of code e.g. for
// debugging and other types of instrumentation. When using the code patcher
// the exact number of bytes specified must be emitted. Is not legal to emit
// relocation information. If any of these constraints are violated it causes
// an assertion.
class CodePatcher {
 public:
  CodePatcher(byte* address, int size);
  virtual ~CodePatcher();

  // Macro assembler to emit code.
  MacroAssembler* masm() { return &masm_; }

 private:
  byte* address_;  // The address of the code being patched.
  int size_;  // Number of bytes of the expected patch size.
  MacroAssembler masm_;  // Macro assembler used to generate the code.
};


// -----------------------------------------------------------------------------
// Static helper functions.

// Generate an Operand for loading a field from an object.
inline Operand FieldOperand(Register object, int offset) {
  return Operand(object, offset - kHeapObjectTag);
}


// Generate an Operand for loading an indexed field from an object.
inline Operand FieldOperand(Register object,
                            Register index,
                            ScaleFactor scale,
                            int offset) {
  return Operand(object, index, scale, offset - kHeapObjectTag);
}


inline Operand ContextOperand(Register context, int index) {
  return Operand(context, Context::SlotOffset(index));
}


inline Operand GlobalObjectOperand() {
  return ContextOperand(esi, Context::GLOBAL_OBJECT_INDEX);
}


// Generates an Operand for saving parameters after PrepareCallApiFunction.
Operand ApiParameterOperand(int index);


#ifdef GENERATED_CODE_COVERAGE
extern void LogGeneratedCodeCoverage(const char* file_line);
#define CODE_COVERAGE_STRINGIFY(x) #x
#define CODE_COVERAGE_TOSTRING(x) CODE_COVERAGE_STRINGIFY(x)
#define __FILE_LINE__ __FILE__ ":" CODE_COVERAGE_TOSTRING(__LINE__)
#define ACCESS_MASM(masm) {                                               \
    byte* ia32_coverage_function =                                        \
        reinterpret_cast<byte*>(FUNCTION_ADDR(LogGeneratedCodeCoverage)); \
    masm->pushfd();                                                       \
    masm->pushad();                                                       \
    masm->push(Immediate(reinterpret_cast<int>(&__FILE_LINE__)));         \
    masm->call(ia32_coverage_function, RelocInfo::RUNTIME_ENTRY);         \
    masm->pop(eax);                                                       \
    masm->popad();                                                        \
    masm->popfd();                                                        \
  }                                                                       \
  masm->
#else
#define ACCESS_MASM(masm) masm->
#endif


} }  // namespace v8::internal

#endif  // V8_IA32_MACRO_ASSEMBLER_IA32_H_
