// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.



#include "src/v8.h"

#if V8_TARGET_ARCH_MIPS

#include "src/codegen.h"
#include "src/debug.h"

namespace v8 {
namespace internal {

bool BreakLocationIterator::IsDebugBreakAtReturn() {
  return Debug::IsDebugBreakAtReturn(rinfo());
}


void BreakLocationIterator::SetDebugBreakAtReturn() {
  // Mips return sequence:
  // mov sp, fp
  // lw fp, sp(0)
  // lw ra, sp(4)
  // addiu sp, sp, 8
  // addiu sp, sp, N
  // jr ra
  // nop (in branch delay slot)

  // Make sure this constant matches the number if instrucntions we emit.
  DCHECK(Assembler::kJSReturnSequenceInstructions == 7);
  CodePatcher patcher(rinfo()->pc(), Assembler::kJSReturnSequenceInstructions);
  // li and Call pseudo-instructions emit two instructions each.
  patcher.masm()->li(v8::internal::t9, Operand(reinterpret_cast<int32_t>(
      debug_info_->GetIsolate()->builtins()->Return_DebugBreak()->entry())));
  patcher.masm()->Call(v8::internal::t9);
  patcher.masm()->nop();
  patcher.masm()->nop();
  patcher.masm()->nop();

  // TODO(mips): Open issue about using breakpoint instruction instead of nops.
  // patcher.masm()->bkpt(0);
}


// Restore the JS frame exit code.
void BreakLocationIterator::ClearDebugBreakAtReturn() {
  rinfo()->PatchCode(original_rinfo()->pc(),
                     Assembler::kJSReturnSequenceInstructions);
}


// A debug break in the exit code is identified by the JS frame exit code
// having been patched with li/call psuedo-instrunction (liu/ori/jalr).
bool Debug::IsDebugBreakAtReturn(RelocInfo* rinfo) {
  DCHECK(RelocInfo::IsJSReturn(rinfo->rmode()));
  return rinfo->IsPatchedReturnSequence();
}


bool BreakLocationIterator::IsDebugBreakAtSlot() {
  DCHECK(IsDebugBreakSlot());
  // Check whether the debug break slot instructions have been patched.
  return rinfo()->IsPatchedDebugBreakSlotSequence();
}


void BreakLocationIterator::SetDebugBreakAtSlot() {
  DCHECK(IsDebugBreakSlot());
  // Patch the code changing the debug break slot code from:
  //   nop(DEBUG_BREAK_NOP) - nop(1) is sll(zero_reg, zero_reg, 1)
  //   nop(DEBUG_BREAK_NOP)
  //   nop(DEBUG_BREAK_NOP)
  //   nop(DEBUG_BREAK_NOP)
  // to a call to the debug break slot code.
  //   li t9, address   (lui t9 / ori t9 instruction pair)
  //   call t9          (jalr t9 / nop instruction pair)
  CodePatcher patcher(rinfo()->pc(), Assembler::kDebugBreakSlotInstructions);
  patcher.masm()->li(v8::internal::t9, Operand(reinterpret_cast<int32_t>(
      debug_info_->GetIsolate()->builtins()->Slot_DebugBreak()->entry())));
  patcher.masm()->Call(v8::internal::t9);
}


void BreakLocationIterator::ClearDebugBreakAtSlot() {
  DCHECK(IsDebugBreakSlot());
  rinfo()->PatchCode(original_rinfo()->pc(),
                     Assembler::kDebugBreakSlotInstructions);
}


#define __ ACCESS_MASM(masm)



static void Generate_DebugBreakCallHelper(MacroAssembler* masm,
                                          RegList object_regs,
                                          RegList non_object_regs) {
  {
    FrameScope scope(masm, StackFrame::INTERNAL);

    // Load padding words on stack.
    __ li(at, Operand(Smi::FromInt(LiveEdit::kFramePaddingValue)));
    __ Subu(sp, sp,
            Operand(kPointerSize * LiveEdit::kFramePaddingInitialSize));
    for (int i = LiveEdit::kFramePaddingInitialSize - 1; i >= 0; i--) {
      __ sw(at, MemOperand(sp, kPointerSize * i));
    }
    __ li(at, Operand(Smi::FromInt(LiveEdit::kFramePaddingInitialSize)));
    __ push(at);

    // Store the registers containing live values on the expression stack to
    // make sure that these are correctly updated during GC. Non object values
    // are stored as a smi causing it to be untouched by GC.
    DCHECK((object_regs & ~kJSCallerSaved) == 0);
    DCHECK((non_object_regs & ~kJSCallerSaved) == 0);
    DCHECK((object_regs & non_object_regs) == 0);
    if ((object_regs | non_object_regs) != 0) {
      for (int i = 0; i < kNumJSCallerSaved; i++) {
        int r = JSCallerSavedCode(i);
        Register reg = { r };
        if ((non_object_regs & (1 << r)) != 0) {
          if (FLAG_debug_code) {
            __ And(at, reg, 0xc0000000);
            __ Assert(eq, kUnableToEncodeValueAsSmi, at, Operand(zero_reg));
          }
          __ sll(reg, reg, kSmiTagSize);
        }
      }
      __ MultiPush(object_regs | non_object_regs);
    }

#ifdef DEBUG
    __ RecordComment("// Calling from debug break to runtime - come in - over");
#endif
    __ PrepareCEntryArgs(0);  // No arguments.
    __ PrepareCEntryFunction(ExternalReference::debug_break(masm->isolate()));

    CEntryStub ceb(masm->isolate(), 1);
    __ CallStub(&ceb);

    // Restore the register values from the expression stack.
    if ((object_regs | non_object_regs) != 0) {
      __ MultiPop(object_regs | non_object_regs);
      for (int i = 0; i < kNumJSCallerSaved; i++) {
        int r = JSCallerSavedCode(i);
        Register reg = { r };
        if ((non_object_regs & (1 << r)) != 0) {
          __ srl(reg, reg, kSmiTagSize);
        }
        if (FLAG_debug_code &&
            (((object_regs |non_object_regs) & (1 << r)) == 0)) {
          __ li(reg, kDebugZapValue);
        }
      }
    }

    // Don't bother removing padding bytes pushed on the stack
    // as the frame is going to be restored right away.

    // Leave the internal frame.
  }

  // Now that the break point has been handled, resume normal execution by
  // jumping to the target address intended by the caller and that was
  // overwritten by the address of DebugBreakXXX.
  ExternalReference after_break_target =
      ExternalReference::debug_after_break_target_address(masm->isolate());
  __ li(t9, Operand(after_break_target));
  __ lw(t9, MemOperand(t9));
  __ Jump(t9);
}


void DebugCodegen::GenerateCallICStubDebugBreak(MacroAssembler* masm) {
  // Register state for CallICStub
  // ----------- S t a t e -------------
  //  -- a1 : function
  //  -- a3 : slot in feedback array (smi)
  // -----------------------------------
  Generate_DebugBreakCallHelper(masm, a1.bit() | a3.bit(), 0);
}


void DebugCodegen::GenerateLoadICDebugBreak(MacroAssembler* masm) {
  Register receiver = LoadDescriptor::ReceiverRegister();
  Register name = LoadDescriptor::NameRegister();
  RegList regs = receiver.bit() | name.bit();
  if (FLAG_vector_ics) {
    regs |= VectorLoadICTrampolineDescriptor::SlotRegister().bit();
  }
  Generate_DebugBreakCallHelper(masm, regs, 0);
}


void DebugCodegen::GenerateStoreICDebugBreak(MacroAssembler* masm) {
  // Calling convention for IC store (from ic-mips.cc).
  Register receiver = StoreDescriptor::ReceiverRegister();
  Register name = StoreDescriptor::NameRegister();
  Register value = StoreDescriptor::ValueRegister();
  Generate_DebugBreakCallHelper(
      masm, receiver.bit() | name.bit() | value.bit(), 0);
}


void DebugCodegen::GenerateKeyedLoadICDebugBreak(MacroAssembler* masm) {
  // Calling convention for keyed IC load (from ic-mips.cc).
  GenerateLoadICDebugBreak(masm);
}


void DebugCodegen::GenerateKeyedStoreICDebugBreak(MacroAssembler* masm) {
  // Calling convention for IC keyed store call (from ic-mips.cc).
  Register receiver = StoreDescriptor::ReceiverRegister();
  Register name = StoreDescriptor::NameRegister();
  Register value = StoreDescriptor::ValueRegister();
  Generate_DebugBreakCallHelper(
      masm, receiver.bit() | name.bit() | value.bit(), 0);
}


void DebugCodegen::GenerateCompareNilICDebugBreak(MacroAssembler* masm) {
  // Register state for CompareNil IC
  // ----------- S t a t e -------------
  //  -- a0    : value
  // -----------------------------------
  Generate_DebugBreakCallHelper(masm, a0.bit(), 0);
}


void DebugCodegen::GenerateReturnDebugBreak(MacroAssembler* masm) {
  // In places other than IC call sites it is expected that v0 is TOS which
  // is an object - this is not generally the case so this should be used with
  // care.
  Generate_DebugBreakCallHelper(masm, v0.bit(), 0);
}


void DebugCodegen::GenerateCallFunctionStubDebugBreak(MacroAssembler* masm) {
  // Register state for CallFunctionStub (from code-stubs-mips.cc).
  // ----------- S t a t e -------------
  //  -- a1 : function
  // -----------------------------------
  Generate_DebugBreakCallHelper(masm, a1.bit(), 0);
}


void DebugCodegen::GenerateCallConstructStubDebugBreak(MacroAssembler* masm) {
  // Calling convention for CallConstructStub (from code-stubs-mips.cc).
  // ----------- S t a t e -------------
  //  -- a0     : number of arguments (not smi)
  //  -- a1     : constructor function
  // -----------------------------------
  Generate_DebugBreakCallHelper(masm, a1.bit() , a0.bit());
}


void DebugCodegen::GenerateCallConstructStubRecordDebugBreak(
    MacroAssembler* masm) {
  // Calling convention for CallConstructStub (from code-stubs-mips.cc).
  // ----------- S t a t e -------------
  //  -- a0     : number of arguments (not smi)
  //  -- a1     : constructor function
  //  -- a2     : feedback array
  //  -- a3     : feedback slot (smi)
  // -----------------------------------
  Generate_DebugBreakCallHelper(masm, a1.bit() | a2.bit() | a3.bit(), a0.bit());
}


void DebugCodegen::GenerateSlot(MacroAssembler* masm) {
  // Generate enough nop's to make space for a call instruction. Avoid emitting
  // the trampoline pool in the debug break slot code.
  Assembler::BlockTrampolinePoolScope block_trampoline_pool(masm);
  Label check_codesize;
  __ bind(&check_codesize);
  __ RecordDebugBreakSlot();
  for (int i = 0; i < Assembler::kDebugBreakSlotInstructions; i++) {
    __ nop(MacroAssembler::DEBUG_BREAK_NOP);
  }
  DCHECK_EQ(Assembler::kDebugBreakSlotInstructions,
            masm->InstructionsGeneratedSince(&check_codesize));
}


void DebugCodegen::GenerateSlotDebugBreak(MacroAssembler* masm) {
  // In the places where a debug break slot is inserted no registers can contain
  // object pointers.
  Generate_DebugBreakCallHelper(masm, 0, 0);
}


void DebugCodegen::GeneratePlainReturnLiveEdit(MacroAssembler* masm) {
  __ Ret();
}


void DebugCodegen::GenerateFrameDropperLiveEdit(MacroAssembler* masm) {
  ExternalReference restarter_frame_function_slot =
      ExternalReference::debug_restarter_frame_function_pointer_address(
          masm->isolate());
  __ li(at, Operand(restarter_frame_function_slot));
  __ sw(zero_reg, MemOperand(at, 0));

  // We do not know our frame height, but set sp based on fp.
  __ Subu(sp, fp, Operand(kPointerSize));

  __ Pop(ra, fp, a1);  // Return address, Frame, Function.

  // Load context from the function.
  __ lw(cp, FieldMemOperand(a1, JSFunction::kContextOffset));

  // Get function code.
  __ lw(at, FieldMemOperand(a1, JSFunction::kSharedFunctionInfoOffset));
  __ lw(at, FieldMemOperand(at, SharedFunctionInfo::kCodeOffset));
  __ Addu(t9, at, Operand(Code::kHeaderSize - kHeapObjectTag));

  // Re-run JSFunction, a1 is function, cp is context.
  __ Jump(t9);
}


const bool LiveEdit::kFrameDropperSupported = true;

#undef __

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_MIPS
