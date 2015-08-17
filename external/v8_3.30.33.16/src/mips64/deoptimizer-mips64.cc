// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/codegen.h"
#include "src/deoptimizer.h"
#include "src/full-codegen.h"
#include "src/safepoint-table.h"

namespace v8 {
namespace internal {


int Deoptimizer::patch_size() {
  const int kCallInstructionSizeInWords = 6;
  return kCallInstructionSizeInWords * Assembler::kInstrSize;
}


void Deoptimizer::PatchCodeForDeoptimization(Isolate* isolate, Code* code) {
  Address code_start_address = code->instruction_start();
  // Invalidate the relocation information, as it will become invalid by the
  // code patching below, and is not needed any more.
  code->InvalidateRelocation();

  if (FLAG_zap_code_space) {
    // Fail hard and early if we enter this code object again.
    byte* pointer = code->FindCodeAgeSequence();
    if (pointer != NULL) {
      pointer += kNoCodeAgeSequenceLength;
    } else {
      pointer = code->instruction_start();
    }
    CodePatcher patcher(pointer, 1);
    patcher.masm()->break_(0xCC);

    DeoptimizationInputData* data =
        DeoptimizationInputData::cast(code->deoptimization_data());
    int osr_offset = data->OsrPcOffset()->value();
    if (osr_offset > 0) {
      CodePatcher osr_patcher(code->instruction_start() + osr_offset, 1);
      osr_patcher.masm()->break_(0xCC);
    }
  }

  DeoptimizationInputData* deopt_data =
      DeoptimizationInputData::cast(code->deoptimization_data());
#ifdef DEBUG
  Address prev_call_address = NULL;
#endif
  // For each LLazyBailout instruction insert a call to the corresponding
  // deoptimization entry.
  for (int i = 0; i < deopt_data->DeoptCount(); i++) {
    if (deopt_data->Pc(i)->value() == -1) continue;
    Address call_address = code_start_address + deopt_data->Pc(i)->value();
    Address deopt_entry = GetDeoptimizationEntry(isolate, i, LAZY);
    int call_size_in_bytes = MacroAssembler::CallSize(deopt_entry,
                                                      RelocInfo::NONE32);
    int call_size_in_words = call_size_in_bytes / Assembler::kInstrSize;
    DCHECK(call_size_in_bytes % Assembler::kInstrSize == 0);
    DCHECK(call_size_in_bytes <= patch_size());
    CodePatcher patcher(call_address, call_size_in_words);
    patcher.masm()->Call(deopt_entry, RelocInfo::NONE32);
    DCHECK(prev_call_address == NULL ||
           call_address >= prev_call_address + patch_size());
    DCHECK(call_address + patch_size() <= code->instruction_end());

#ifdef DEBUG
    prev_call_address = call_address;
#endif
  }
}


void Deoptimizer::FillInputFrame(Address tos, JavaScriptFrame* frame) {
  // Set the register values. The values are not important as there are no
  // callee saved registers in JavaScript frames, so all registers are
  // spilled. Registers fp and sp are set to the correct values though.

  for (int i = 0; i < Register::kNumRegisters; i++) {
    input_->SetRegister(i, i * 4);
  }
  input_->SetRegister(sp.code(), reinterpret_cast<intptr_t>(frame->sp()));
  input_->SetRegister(fp.code(), reinterpret_cast<intptr_t>(frame->fp()));
  for (int i = 0; i < DoubleRegister::NumAllocatableRegisters(); i++) {
    input_->SetDoubleRegister(i, 0.0);
  }

  // Fill the frame content from the actual data on the frame.
  for (unsigned i = 0; i < input_->GetFrameSize(); i += kPointerSize) {
    input_->SetFrameSlot(i, Memory::uint64_at(tos + i));
  }
}


void Deoptimizer::SetPlatformCompiledStubRegisters(
    FrameDescription* output_frame, CodeStubDescriptor* descriptor) {
  ApiFunction function(descriptor->deoptimization_handler());
  ExternalReference xref(&function, ExternalReference::BUILTIN_CALL, isolate_);
  intptr_t handler = reinterpret_cast<intptr_t>(xref.address());
  int params = descriptor->GetHandlerParameterCount();
  output_frame->SetRegister(s0.code(), params);
  output_frame->SetRegister(s1.code(), (params - 1) * kPointerSize);
  output_frame->SetRegister(s2.code(), handler);
}


void Deoptimizer::CopyDoubleRegisters(FrameDescription* output_frame) {
  for (int i = 0; i < DoubleRegister::kMaxNumRegisters; ++i) {
    double double_value = input_->GetDoubleRegister(i);
    output_frame->SetDoubleRegister(i, double_value);
  }
}


bool Deoptimizer::HasAlignmentPadding(JSFunction* function) {
  // There is no dynamic alignment padding on MIPS in the input frame.
  return false;
}


#define __ masm()->


// This code tries to be close to ia32 code so that any changes can be
// easily ported.
void Deoptimizer::EntryGenerator::Generate() {
  GeneratePrologue();

  // Unlike on ARM we don't save all the registers, just the useful ones.
  // For the rest, there are gaps on the stack, so the offsets remain the same.
  const int kNumberOfRegisters = Register::kNumRegisters;

  RegList restored_regs = kJSCallerSaved | kCalleeSaved;
  RegList saved_regs = restored_regs | sp.bit() | ra.bit();

  const int kDoubleRegsSize =
      kDoubleSize * FPURegister::kMaxNumAllocatableRegisters;

  // Save all FPU registers before messing with them.
  __ Dsubu(sp, sp, Operand(kDoubleRegsSize));
  for (int i = 0; i < FPURegister::kMaxNumAllocatableRegisters; ++i) {
    FPURegister fpu_reg = FPURegister::FromAllocationIndex(i);
    int offset = i * kDoubleSize;
    __ sdc1(fpu_reg, MemOperand(sp, offset));
  }

  // Push saved_regs (needed to populate FrameDescription::registers_).
  // Leave gaps for other registers.
  __ Dsubu(sp, sp, kNumberOfRegisters * kPointerSize);
  for (int16_t i = kNumberOfRegisters - 1; i >= 0; i--) {
    if ((saved_regs & (1 << i)) != 0) {
      __ sd(ToRegister(i), MemOperand(sp, kPointerSize * i));
    }
  }

  const int kSavedRegistersAreaSize =
      (kNumberOfRegisters * kPointerSize) + kDoubleRegsSize;

  // Get the bailout id from the stack.
  __ ld(a2, MemOperand(sp, kSavedRegistersAreaSize));

  // Get the address of the location in the code object (a3) (return
  // address for lazy deoptimization) and compute the fp-to-sp delta in
  // register a4.
  __ mov(a3, ra);
  // Correct one word for bailout id.
  __ Daddu(a4, sp, Operand(kSavedRegistersAreaSize + (1 * kPointerSize)));

  __ Dsubu(a4, fp, a4);

  // Allocate a new deoptimizer object.
  __ PrepareCallCFunction(6, a5);
  // Pass six arguments, according to O32 or n64 ABI. a0..a3 are same for both.
  __ li(a1, Operand(type()));  // bailout type,
  __ ld(a0, MemOperand(fp, JavaScriptFrameConstants::kFunctionOffset));
  // a2: bailout id already loaded.
  // a3: code address or 0 already loaded.
  if (kMipsAbi == kN64) {
    // a4: already has fp-to-sp delta.
    __ li(a5, Operand(ExternalReference::isolate_address(isolate())));
  } else {  // O32 abi.
    // Pass four arguments in a0 to a3 and fifth & sixth arguments on stack.
    __ sd(a4, CFunctionArgumentOperand(5));  // Fp-to-sp delta.
    __ li(a5, Operand(ExternalReference::isolate_address(isolate())));
    __ sd(a5, CFunctionArgumentOperand(6));  // Isolate.
  }
  // Call Deoptimizer::New().
  {
    AllowExternalCallThatCantCauseGC scope(masm());
    __ CallCFunction(ExternalReference::new_deoptimizer_function(isolate()), 6);
  }

  // Preserve "deoptimizer" object in register v0 and get the input
  // frame descriptor pointer to a1 (deoptimizer->input_);
  // Move deopt-obj to a0 for call to Deoptimizer::ComputeOutputFrames() below.
  __ mov(a0, v0);
  __ ld(a1, MemOperand(v0, Deoptimizer::input_offset()));

  // Copy core registers into FrameDescription::registers_[kNumRegisters].
  DCHECK(Register::kNumRegisters == kNumberOfRegisters);
  for (int i = 0; i < kNumberOfRegisters; i++) {
    int offset = (i * kPointerSize) + FrameDescription::registers_offset();
    if ((saved_regs & (1 << i)) != 0) {
      __ ld(a2, MemOperand(sp, i * kPointerSize));
      __ sd(a2, MemOperand(a1, offset));
    } else if (FLAG_debug_code) {
      __ li(a2, kDebugZapValue);
      __ sd(a2, MemOperand(a1, offset));
    }
  }

  int double_regs_offset = FrameDescription::double_registers_offset();
  // Copy FPU registers to
  // double_registers_[DoubleRegister::kNumAllocatableRegisters]
  for (int i = 0; i < FPURegister::NumAllocatableRegisters(); ++i) {
    int dst_offset = i * kDoubleSize + double_regs_offset;
    int src_offset = i * kDoubleSize + kNumberOfRegisters * kPointerSize;
    __ ldc1(f0, MemOperand(sp, src_offset));
    __ sdc1(f0, MemOperand(a1, dst_offset));
  }

  // Remove the bailout id and the saved registers from the stack.
  __ Daddu(sp, sp, Operand(kSavedRegistersAreaSize + (1 * kPointerSize)));

  // Compute a pointer to the unwinding limit in register a2; that is
  // the first stack slot not part of the input frame.
  __ ld(a2, MemOperand(a1, FrameDescription::frame_size_offset()));
  __ Daddu(a2, a2, sp);

  // Unwind the stack down to - but not including - the unwinding
  // limit and copy the contents of the activation frame to the input
  // frame description.
  __ Daddu(a3, a1, Operand(FrameDescription::frame_content_offset()));
  Label pop_loop;
  Label pop_loop_header;
  __ BranchShort(&pop_loop_header);
  __ bind(&pop_loop);
  __ pop(a4);
  __ sd(a4, MemOperand(a3, 0));
  __ daddiu(a3, a3, sizeof(uint64_t));
  __ bind(&pop_loop_header);
  __ BranchShort(&pop_loop, ne, a2, Operand(sp));
  // Compute the output frame in the deoptimizer.
  __ push(a0);  // Preserve deoptimizer object across call.
  // a0: deoptimizer object; a1: scratch.
  __ PrepareCallCFunction(1, a1);
  // Call Deoptimizer::ComputeOutputFrames().
  {
    AllowExternalCallThatCantCauseGC scope(masm());
    __ CallCFunction(
        ExternalReference::compute_output_frames_function(isolate()), 1);
  }
  __ pop(a0);  // Restore deoptimizer object (class Deoptimizer).

  // Replace the current (input) frame with the output frames.
  Label outer_push_loop, inner_push_loop,
      outer_loop_header, inner_loop_header;
  // Outer loop state: a4 = current "FrameDescription** output_",
  // a1 = one past the last FrameDescription**.
  __ lw(a1, MemOperand(a0, Deoptimizer::output_count_offset()));
  __ ld(a4, MemOperand(a0, Deoptimizer::output_offset()));  // a4 is output_.
  __ dsll(a1, a1, kPointerSizeLog2);  // Count to offset.
  __ daddu(a1, a4, a1);  // a1 = one past the last FrameDescription**.
  __ jmp(&outer_loop_header);
  __ bind(&outer_push_loop);
  // Inner loop state: a2 = current FrameDescription*, a3 = loop index.
  __ ld(a2, MemOperand(a4, 0));  // output_[ix]
  __ ld(a3, MemOperand(a2, FrameDescription::frame_size_offset()));
  __ jmp(&inner_loop_header);
  __ bind(&inner_push_loop);
  __ Dsubu(a3, a3, Operand(sizeof(uint64_t)));
  __ Daddu(a6, a2, Operand(a3));
  __ ld(a7, MemOperand(a6, FrameDescription::frame_content_offset()));
  __ push(a7);
  __ bind(&inner_loop_header);
  __ BranchShort(&inner_push_loop, ne, a3, Operand(zero_reg));

  __ Daddu(a4, a4, Operand(kPointerSize));
  __ bind(&outer_loop_header);
  __ BranchShort(&outer_push_loop, lt, a4, Operand(a1));

  __ ld(a1, MemOperand(a0, Deoptimizer::input_offset()));
  for (int i = 0; i < FPURegister::kMaxNumAllocatableRegisters; ++i) {
    const FPURegister fpu_reg = FPURegister::FromAllocationIndex(i);
    int src_offset = i * kDoubleSize + double_regs_offset;
    __ ldc1(fpu_reg, MemOperand(a1, src_offset));
  }

  // Push state, pc, and continuation from the last output frame.
  __ ld(a6, MemOperand(a2, FrameDescription::state_offset()));
  __ push(a6);

  __ ld(a6, MemOperand(a2, FrameDescription::pc_offset()));
  __ push(a6);
  __ ld(a6, MemOperand(a2, FrameDescription::continuation_offset()));
  __ push(a6);


  // Technically restoring 'at' should work unless zero_reg is also restored
  // but it's safer to check for this.
  DCHECK(!(at.bit() & restored_regs));
  // Restore the registers from the last output frame.
  __ mov(at, a2);
  for (int i = kNumberOfRegisters - 1; i >= 0; i--) {
    int offset = (i * kPointerSize) + FrameDescription::registers_offset();
    if ((restored_regs & (1 << i)) != 0) {
      __ ld(ToRegister(i), MemOperand(at, offset));
    }
  }

  __ InitializeRootRegister();

  __ pop(at);  // Get continuation, leave pc on stack.
  __ pop(ra);
  __ Jump(at);
  __ stop("Unreachable.");
}


// Maximum size of a table entry generated below.
const int Deoptimizer::table_entry_size_ = 11 * Assembler::kInstrSize;

void Deoptimizer::TableEntryGenerator::GeneratePrologue() {
  Assembler::BlockTrampolinePoolScope block_trampoline_pool(masm());

  // Create a sequence of deoptimization entries.
  // Note that registers are still live when jumping to an entry.
  Label table_start;
  __ bind(&table_start);
  for (int i = 0; i < count(); i++) {
    Label start;
    __ bind(&start);
    __ daddiu(sp, sp, -1 * kPointerSize);
    // Jump over the remaining deopt entries (including this one).
    // This code is always reached by calling Jump, which puts the target (label
    // start) into t9.
    const int remaining_entries = (count() - i) * table_entry_size_;
    __ Daddu(t9, t9, remaining_entries);
    // 'at' was clobbered so we can only load the current entry value here.
    __ li(t8, i);
    __ jr(t9);  // Expose delay slot.
    __ sd(t8, MemOperand(sp, 0 * kPointerSize));  // In the delay slot.

    // Pad the rest of the code.
    while (table_entry_size_ > (masm()->SizeOfCodeGeneratedSince(&start))) {
      __ nop();
    }

    DCHECK_EQ(table_entry_size_, masm()->SizeOfCodeGeneratedSince(&start));
  }

  DCHECK_EQ(masm()->SizeOfCodeGeneratedSince(&table_start),
      count() * table_entry_size_);
}


void FrameDescription::SetCallerPc(unsigned offset, intptr_t value) {
  SetFrameSlot(offset, value);
}


void FrameDescription::SetCallerFp(unsigned offset, intptr_t value) {
  SetFrameSlot(offset, value);
}


void FrameDescription::SetCallerConstantPool(unsigned offset, intptr_t value) {
  // No out-of-line constant pool support.
  UNREACHABLE();
}


#undef __


} }  // namespace v8::internal
