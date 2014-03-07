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
#include "lithium.h"
#include "scopes.h"

#if V8_TARGET_ARCH_IA32
#include "ia32/lithium-ia32.h"
#include "ia32/lithium-codegen-ia32.h"
#elif V8_TARGET_ARCH_X64
#include "x64/lithium-x64.h"
#include "x64/lithium-codegen-x64.h"
#elif V8_TARGET_ARCH_ARM
#include "arm/lithium-arm.h"
#include "arm/lithium-codegen-arm.h"
#elif V8_TARGET_ARCH_MIPS
#include "mips/lithium-mips.h"
#include "mips/lithium-codegen-mips.h"
#else
#error "Unknown architecture."
#endif

namespace v8 {
namespace internal {


void LOperand::PrintTo(StringStream* stream) {
  LUnallocated* unalloc = NULL;
  switch (kind()) {
    case INVALID:
      stream->Add("(0)");
      break;
    case UNALLOCATED:
      unalloc = LUnallocated::cast(this);
      stream->Add("v%d", unalloc->virtual_register());
      if (unalloc->basic_policy() == LUnallocated::FIXED_SLOT) {
        stream->Add("(=%dS)", unalloc->fixed_slot_index());
        break;
      }
      switch (unalloc->extended_policy()) {
        case LUnallocated::NONE:
          break;
        case LUnallocated::FIXED_REGISTER: {
          int reg_index = unalloc->fixed_register_index();
          const char* register_name =
              Register::AllocationIndexToString(reg_index);
          stream->Add("(=%s)", register_name);
          break;
        }
        case LUnallocated::FIXED_DOUBLE_REGISTER: {
          int reg_index = unalloc->fixed_register_index();
          const char* double_register_name =
              DoubleRegister::AllocationIndexToString(reg_index);
          stream->Add("(=%s)", double_register_name);
          break;
        }
        case LUnallocated::MUST_HAVE_REGISTER:
          stream->Add("(R)");
          break;
        case LUnallocated::WRITABLE_REGISTER:
          stream->Add("(WR)");
          break;
        case LUnallocated::SAME_AS_FIRST_INPUT:
          stream->Add("(1)");
          break;
        case LUnallocated::ANY:
          stream->Add("(-)");
          break;
      }
      break;
    case CONSTANT_OPERAND:
      stream->Add("[constant:%d]", index());
      break;
    case STACK_SLOT:
      stream->Add("[stack:%d]", index());
      break;
    case DOUBLE_STACK_SLOT:
      stream->Add("[double_stack:%d]", index());
      break;
    case REGISTER:
      stream->Add("[%s|R]", Register::AllocationIndexToString(index()));
      break;
    case DOUBLE_REGISTER:
      stream->Add("[%s|R]", DoubleRegister::AllocationIndexToString(index()));
      break;
    case ARGUMENT:
      stream->Add("[arg:%d]", index());
      break;
  }
}

#define DEFINE_OPERAND_CACHE(name, type)                      \
  L##name* L##name::cache = NULL;                             \
                                                              \
  void L##name::SetUpCache() {                                \
    if (cache) return;                                        \
    cache = new L##name[kNumCachedOperands];                  \
    for (int i = 0; i < kNumCachedOperands; i++) {            \
      cache[i].ConvertTo(type, i);                            \
    }                                                         \
  }                                                           \
                                                              \
  void L##name::TearDownCache() {                             \
    delete[] cache;                                           \
  }

LITHIUM_OPERAND_LIST(DEFINE_OPERAND_CACHE)
#undef DEFINE_OPERAND_CACHE

void LOperand::SetUpCaches() {
#define LITHIUM_OPERAND_SETUP(name, type) L##name::SetUpCache();
  LITHIUM_OPERAND_LIST(LITHIUM_OPERAND_SETUP)
#undef LITHIUM_OPERAND_SETUP
}


void LOperand::TearDownCaches() {
#define LITHIUM_OPERAND_TEARDOWN(name, type) L##name::TearDownCache();
  LITHIUM_OPERAND_LIST(LITHIUM_OPERAND_TEARDOWN)
#undef LITHIUM_OPERAND_TEARDOWN
}


bool LParallelMove::IsRedundant() const {
  for (int i = 0; i < move_operands_.length(); ++i) {
    if (!move_operands_[i].IsRedundant()) return false;
  }
  return true;
}


void LParallelMove::PrintDataTo(StringStream* stream) const {
  bool first = true;
  for (int i = 0; i < move_operands_.length(); ++i) {
    if (!move_operands_[i].IsEliminated()) {
      LOperand* source = move_operands_[i].source();
      LOperand* destination = move_operands_[i].destination();
      if (!first) stream->Add(" ");
      first = false;
      if (source->Equals(destination)) {
        destination->PrintTo(stream);
      } else {
        destination->PrintTo(stream);
        stream->Add(" = ");
        source->PrintTo(stream);
      }
      stream->Add(";");
    }
  }
}


void LEnvironment::PrintTo(StringStream* stream) {
  stream->Add("[id=%d|", ast_id().ToInt());
  if (deoptimization_index() != Safepoint::kNoDeoptimizationIndex) {
    stream->Add("deopt_id=%d|", deoptimization_index());
  }
  stream->Add("parameters=%d|", parameter_count());
  stream->Add("arguments_stack_height=%d|", arguments_stack_height());
  for (int i = 0; i < values_.length(); ++i) {
    if (i != 0) stream->Add(";");
    if (values_[i] == NULL) {
      stream->Add("[hole]");
    } else {
      values_[i]->PrintTo(stream);
    }
  }
  stream->Add("]");
}


void LPointerMap::RecordPointer(LOperand* op, Zone* zone) {
  // Do not record arguments as pointers.
  if (op->IsStackSlot() && op->index() < 0) return;
  ASSERT(!op->IsDoubleRegister() && !op->IsDoubleStackSlot());
  pointer_operands_.Add(op, zone);
}


void LPointerMap::RemovePointer(LOperand* op) {
  // Do not record arguments as pointers.
  if (op->IsStackSlot() && op->index() < 0) return;
  ASSERT(!op->IsDoubleRegister() && !op->IsDoubleStackSlot());
  for (int i = 0; i < pointer_operands_.length(); ++i) {
    if (pointer_operands_[i]->Equals(op)) {
      pointer_operands_.Remove(i);
      --i;
    }
  }
}


void LPointerMap::RecordUntagged(LOperand* op, Zone* zone) {
  // Do not record arguments as pointers.
  if (op->IsStackSlot() && op->index() < 0) return;
  ASSERT(!op->IsDoubleRegister() && !op->IsDoubleStackSlot());
  untagged_operands_.Add(op, zone);
}


void LPointerMap::PrintTo(StringStream* stream) {
  stream->Add("{");
  for (int i = 0; i < pointer_operands_.length(); ++i) {
    if (i != 0) stream->Add(";");
    pointer_operands_[i]->PrintTo(stream);
  }
  stream->Add("}");
}


int StackSlotOffset(int index) {
  if (index >= 0) {
    // Local or spill slot. Skip the frame pointer, function, and
    // context in the fixed part of the frame.
    return -(index + 3) * kPointerSize;
  } else {
    // Incoming parameter. Skip the return address.
    return -(index + 1) * kPointerSize + kFPOnStackSize + kPCOnStackSize;
  }
}


LChunk::LChunk(CompilationInfo* info, HGraph* graph)
    : spill_slot_count_(0),
      info_(info),
      graph_(graph),
      instructions_(32, graph->zone()),
      pointer_maps_(8, graph->zone()),
      inlined_closures_(1, graph->zone()) {
}


LLabel* LChunk::GetLabel(int block_id) const {
  HBasicBlock* block = graph_->blocks()->at(block_id);
  int first_instruction = block->first_instruction_index();
  return LLabel::cast(instructions_[first_instruction]);
}


int LChunk::LookupDestination(int block_id) const {
  LLabel* cur = GetLabel(block_id);
  while (cur->replacement() != NULL) {
    cur = cur->replacement();
  }
  return cur->block_id();
}

Label* LChunk::GetAssemblyLabel(int block_id) const {
  LLabel* label = GetLabel(block_id);
  ASSERT(!label->HasReplacement());
  return label->label();
}


void LChunk::MarkEmptyBlocks() {
  LPhase phase("L_Mark empty blocks", this);
  for (int i = 0; i < graph()->blocks()->length(); ++i) {
    HBasicBlock* block = graph()->blocks()->at(i);
    int first = block->first_instruction_index();
    int last = block->last_instruction_index();
    LInstruction* first_instr = instructions()->at(first);
    LInstruction* last_instr = instructions()->at(last);

    LLabel* label = LLabel::cast(first_instr);
    if (last_instr->IsGoto()) {
      LGoto* goto_instr = LGoto::cast(last_instr);
      if (label->IsRedundant() &&
          !label->is_loop_header()) {
        bool can_eliminate = true;
        for (int i = first + 1; i < last && can_eliminate; ++i) {
          LInstruction* cur = instructions()->at(i);
          if (cur->IsGap()) {
            LGap* gap = LGap::cast(cur);
            if (!gap->IsRedundant()) {
              can_eliminate = false;
            }
          } else {
            can_eliminate = false;
          }
        }
        if (can_eliminate) {
          label->set_replacement(GetLabel(goto_instr->block_id()));
        }
      }
    }
  }
}


void LChunk::AddInstruction(LInstruction* instr, HBasicBlock* block) {
  LInstructionGap* gap = new(graph_->zone()) LInstructionGap(block);
  gap->set_hydrogen_value(instr->hydrogen_value());
  int index = -1;
  if (instr->IsControl()) {
    instructions_.Add(gap, zone());
    index = instructions_.length();
    instructions_.Add(instr, zone());
  } else {
    index = instructions_.length();
    instructions_.Add(instr, zone());
    instructions_.Add(gap, zone());
  }
  if (instr->HasPointerMap()) {
    pointer_maps_.Add(instr->pointer_map(), zone());
    instr->pointer_map()->set_lithium_position(index);
  }
}


LConstantOperand* LChunk::DefineConstantOperand(HConstant* constant) {
  return LConstantOperand::Create(constant->id(), zone());
}


int LChunk::GetParameterStackSlot(int index) const {
  // The receiver is at index 0, the first parameter at index 1, so we
  // shift all parameter indexes down by the number of parameters, and
  // make sure they end up negative so they are distinguishable from
  // spill slots.
  int result = index - info()->scope()->num_parameters() - 1;
  ASSERT(result < 0);
  return result;
}


// A parameter relative to ebp in the arguments stub.
int LChunk::ParameterAt(int index) {
  ASSERT(-1 <= index);  // -1 is the receiver.
  return (1 + info()->scope()->num_parameters() - index) *
      kPointerSize;
}


LGap* LChunk::GetGapAt(int index) const {
  return LGap::cast(instructions_[index]);
}


bool LChunk::IsGapAt(int index) const {
  return instructions_[index]->IsGap();
}


int LChunk::NearestGapPos(int index) const {
  while (!IsGapAt(index)) index--;
  return index;
}


void LChunk::AddGapMove(int index, LOperand* from, LOperand* to) {
  GetGapAt(index)->GetOrCreateParallelMove(
      LGap::START, zone())->AddMove(from, to, zone());
}


HConstant* LChunk::LookupConstant(LConstantOperand* operand) const {
  return HConstant::cast(graph_->LookupValue(operand->index()));
}


Representation LChunk::LookupLiteralRepresentation(
    LConstantOperand* operand) const {
  return graph_->LookupValue(operand->index())->representation();
}


LChunk* LChunk::NewChunk(HGraph* graph) {
  DisallowHandleAllocation no_handles;
  DisallowHeapAllocation no_gc;
  graph->DisallowAddingNewValues();
  int values = graph->GetMaximumValueID();
  CompilationInfo* info = graph->info();
  if (values > LUnallocated::kMaxVirtualRegisters) {
    info->set_bailout_reason(kNotEnoughVirtualRegistersForValues);
    return NULL;
  }
  LAllocator allocator(values, graph);
  LChunkBuilder builder(info, graph, &allocator);
  LChunk* chunk = builder.Build();
  if (chunk == NULL) return NULL;

  if (!allocator.Allocate(chunk)) {
    info->set_bailout_reason(kNotEnoughVirtualRegistersRegalloc);
    return NULL;
  }

  chunk->set_allocated_double_registers(
      allocator.assigned_double_registers());

  return chunk;
}


Handle<Code> LChunk::Codegen() {
  MacroAssembler assembler(info()->isolate(), NULL, 0);
  LOG_CODE_EVENT(info()->isolate(),
                 CodeStartLinePosInfoRecordEvent(
                     assembler.positions_recorder()));
  LCodeGen generator(this, &assembler, info());

  MarkEmptyBlocks();

  if (generator.GenerateCode()) {
    CodeGenerator::MakeCodePrologue(info(), "optimized");
    Code::Flags flags = info()->flags();
    Handle<Code> code =
        CodeGenerator::MakeCodeEpilogue(&assembler, flags, info());
    generator.FinishCode(code);
    code->set_is_crankshafted(true);
    void* jit_handler_data =
        assembler.positions_recorder()->DetachJITHandlerData();
    LOG_CODE_EVENT(info()->isolate(),
                   CodeEndLinePosInfoRecordEvent(*code, jit_handler_data));

    CodeGenerator::PrintCode(code, info());
    return code;
  }
  return Handle<Code>::null();
}


void LChunk::set_allocated_double_registers(BitVector* allocated_registers) {
  allocated_double_registers_ = allocated_registers;
  BitVector* doubles = allocated_double_registers();
  BitVector::Iterator iterator(doubles);
  while (!iterator.Done()) {
    if (info()->saves_caller_doubles()) {
      if (kDoubleSize == kPointerSize * 2) {
        spill_slot_count_ += 2;
      } else {
        spill_slot_count_++;
      }
    }
    iterator.Advance();
  }
}


LInstruction* LChunkBuilder::CheckElideControlInstruction(
    HControlInstruction* instr) {
  HBasicBlock* successor;
  if (!instr->KnownSuccessorBlock(&successor)) return NULL;
  return new(zone()) LGoto(successor);
}


LPhase::~LPhase() {
  if (ShouldProduceTraceOutput()) {
    isolate()->GetHTracer()->TraceLithium(name(), chunk_);
  }
}


} }  // namespace v8::internal
