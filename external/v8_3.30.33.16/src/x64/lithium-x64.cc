// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "src/v8.h"

#if V8_TARGET_ARCH_X64

#include "src/hydrogen-osr.h"
#include "src/lithium-inl.h"
#include "src/x64/lithium-codegen-x64.h"

namespace v8 {
namespace internal {

#define DEFINE_COMPILE(type)                            \
  void L##type::CompileToNative(LCodeGen* generator) {  \
    generator->Do##type(this);                          \
  }
LITHIUM_CONCRETE_INSTRUCTION_LIST(DEFINE_COMPILE)
#undef DEFINE_COMPILE


#ifdef DEBUG
void LInstruction::VerifyCall() {
  // Call instructions can use only fixed registers as temporaries and
  // outputs because all registers are blocked by the calling convention.
  // Inputs operands must use a fixed register or use-at-start policy or
  // a non-register policy.
  DCHECK(Output() == NULL ||
         LUnallocated::cast(Output())->HasFixedPolicy() ||
         !LUnallocated::cast(Output())->HasRegisterPolicy());
  for (UseIterator it(this); !it.Done(); it.Advance()) {
    LUnallocated* operand = LUnallocated::cast(it.Current());
    DCHECK(operand->HasFixedPolicy() ||
           operand->IsUsedAtStart());
  }
  for (TempIterator it(this); !it.Done(); it.Advance()) {
    LUnallocated* operand = LUnallocated::cast(it.Current());
    DCHECK(operand->HasFixedPolicy() ||!operand->HasRegisterPolicy());
  }
}
#endif


void LInstruction::PrintTo(StringStream* stream) {
  stream->Add("%s ", this->Mnemonic());

  PrintOutputOperandTo(stream);

  PrintDataTo(stream);

  if (HasEnvironment()) {
    stream->Add(" ");
    environment()->PrintTo(stream);
  }

  if (HasPointerMap()) {
    stream->Add(" ");
    pointer_map()->PrintTo(stream);
  }
}


void LInstruction::PrintDataTo(StringStream* stream) {
  stream->Add("= ");
  for (int i = 0; i < InputCount(); i++) {
    if (i > 0) stream->Add(" ");
    if (InputAt(i) == NULL) {
      stream->Add("NULL");
    } else {
      InputAt(i)->PrintTo(stream);
    }
  }
}


void LInstruction::PrintOutputOperandTo(StringStream* stream) {
  if (HasResult()) result()->PrintTo(stream);
}


void LLabel::PrintDataTo(StringStream* stream) {
  LGap::PrintDataTo(stream);
  LLabel* rep = replacement();
  if (rep != NULL) {
    stream->Add(" Dead block replaced with B%d", rep->block_id());
  }
}


bool LGap::IsRedundant() const {
  for (int i = 0; i < 4; i++) {
    if (parallel_moves_[i] != NULL && !parallel_moves_[i]->IsRedundant()) {
      return false;
    }
  }

  return true;
}


void LGap::PrintDataTo(StringStream* stream) {
  for (int i = 0; i < 4; i++) {
    stream->Add("(");
    if (parallel_moves_[i] != NULL) {
      parallel_moves_[i]->PrintDataTo(stream);
    }
    stream->Add(") ");
  }
}


const char* LArithmeticD::Mnemonic() const {
  switch (op()) {
    case Token::ADD: return "add-d";
    case Token::SUB: return "sub-d";
    case Token::MUL: return "mul-d";
    case Token::DIV: return "div-d";
    case Token::MOD: return "mod-d";
    default:
      UNREACHABLE();
      return NULL;
  }
}


const char* LArithmeticT::Mnemonic() const {
  switch (op()) {
    case Token::ADD: return "add-t";
    case Token::SUB: return "sub-t";
    case Token::MUL: return "mul-t";
    case Token::MOD: return "mod-t";
    case Token::DIV: return "div-t";
    case Token::BIT_AND: return "bit-and-t";
    case Token::BIT_OR: return "bit-or-t";
    case Token::BIT_XOR: return "bit-xor-t";
    case Token::ROR: return "ror-t";
    case Token::SHL: return "sal-t";
    case Token::SAR: return "sar-t";
    case Token::SHR: return "shr-t";
    default:
      UNREACHABLE();
      return NULL;
  }
}


bool LGoto::HasInterestingComment(LCodeGen* gen) const {
  return !gen->IsNextEmittedBlock(block_id());
}


template<int R>
bool LTemplateResultInstruction<R>::MustSignExtendResult(
    LPlatformChunk* chunk) const {
  HValue* hvalue = this->hydrogen_value();
  return hvalue != NULL &&
      hvalue->representation().IsInteger32() &&
      chunk->GetDehoistedKeyIds()->Contains(hvalue->id());
}


void LGoto::PrintDataTo(StringStream* stream) {
  stream->Add("B%d", block_id());
}


void LBranch::PrintDataTo(StringStream* stream) {
  stream->Add("B%d | B%d on ", true_block_id(), false_block_id());
  value()->PrintTo(stream);
}


void LCompareNumericAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if ");
  left()->PrintTo(stream);
  stream->Add(" %s ", Token::String(op()));
  right()->PrintTo(stream);
  stream->Add(" then B%d else B%d", true_block_id(), false_block_id());
}


void LIsObjectAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if is_object(");
  value()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LIsStringAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if is_string(");
  value()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LIsSmiAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if is_smi(");
  value()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LIsUndetectableAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if is_undetectable(");
  value()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LStringCompareAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if string_compare(");
  left()->PrintTo(stream);
  right()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LHasInstanceTypeAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if has_instance_type(");
  value()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LHasCachedArrayIndexAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if has_cached_array_index(");
  value()->PrintTo(stream);
  stream->Add(") then B%d else B%d", true_block_id(), false_block_id());
}


void LClassOfTestAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if class_of_test(");
  value()->PrintTo(stream);
  stream->Add(", \"%o\") then B%d else B%d",
              *hydrogen()->class_name(),
              true_block_id(),
              false_block_id());
}


void LTypeofIsAndBranch::PrintDataTo(StringStream* stream) {
  stream->Add("if typeof ");
  value()->PrintTo(stream);
  stream->Add(" == \"%s\" then B%d else B%d",
              hydrogen()->type_literal()->ToCString().get(),
              true_block_id(), false_block_id());
}


void LStoreCodeEntry::PrintDataTo(StringStream* stream) {
  stream->Add(" = ");
  function()->PrintTo(stream);
  stream->Add(".code_entry = ");
  code_object()->PrintTo(stream);
}


void LInnerAllocatedObject::PrintDataTo(StringStream* stream) {
  stream->Add(" = ");
  base_object()->PrintTo(stream);
  stream->Add(" + ");
  offset()->PrintTo(stream);
}


void LCallJSFunction::PrintDataTo(StringStream* stream) {
  stream->Add("= ");
  function()->PrintTo(stream);
  stream->Add("#%d / ", arity());
}


void LCallWithDescriptor::PrintDataTo(StringStream* stream) {
  for (int i = 0; i < InputCount(); i++) {
    InputAt(i)->PrintTo(stream);
    stream->Add(" ");
  }
  stream->Add("#%d / ", arity());
}


void LLoadContextSlot::PrintDataTo(StringStream* stream) {
  context()->PrintTo(stream);
  stream->Add("[%d]", slot_index());
}


void LStoreContextSlot::PrintDataTo(StringStream* stream) {
  context()->PrintTo(stream);
  stream->Add("[%d] <- ", slot_index());
  value()->PrintTo(stream);
}


void LInvokeFunction::PrintDataTo(StringStream* stream) {
  stream->Add("= ");
  function()->PrintTo(stream);
  stream->Add(" #%d / ", arity());
}


void LCallNew::PrintDataTo(StringStream* stream) {
  stream->Add("= ");
  constructor()->PrintTo(stream);
  stream->Add(" #%d / ", arity());
}


void LCallNewArray::PrintDataTo(StringStream* stream) {
  stream->Add("= ");
  constructor()->PrintTo(stream);
  stream->Add(" #%d / ", arity());
  ElementsKind kind = hydrogen()->elements_kind();
  stream->Add(" (%s) ", ElementsKindToString(kind));
}


void LAccessArgumentsAt::PrintDataTo(StringStream* stream) {
  arguments()->PrintTo(stream);

  stream->Add(" length ");
  length()->PrintTo(stream);

  stream->Add(" index ");
  index()->PrintTo(stream);
}


int LPlatformChunk::GetNextSpillIndex(RegisterKind kind) {
  if (kind == DOUBLE_REGISTERS && kDoubleSize == 2 * kPointerSize) {
    // Skip a slot if for a double-width slot for x32 port.
    spill_slot_count_++;
    // The spill slot's address is at rbp - (index + 1) * kPointerSize -
    // StandardFrameConstants::kFixedFrameSizeFromFp. kFixedFrameSizeFromFp is
    // 2 * kPointerSize, if rbp is aligned at 8-byte boundary, the below "|= 1"
    // will make sure the spilled doubles are aligned at 8-byte boundary.
    // TODO(haitao): make sure rbp is aligned at 8-byte boundary for x32 port.
    spill_slot_count_ |= 1;
  }
  return spill_slot_count_++;
}


LOperand* LPlatformChunk::GetNextSpillSlot(RegisterKind kind) {
  // All stack slots are Double stack slots on x64.
  // Alternatively, at some point, start using half-size
  // stack slots for int32 values.
  int index = GetNextSpillIndex(kind);
  if (kind == DOUBLE_REGISTERS) {
    return LDoubleStackSlot::Create(index, zone());
  } else {
    DCHECK(kind == GENERAL_REGISTERS);
    return LStackSlot::Create(index, zone());
  }
}


void LStoreNamedField::PrintDataTo(StringStream* stream) {
  object()->PrintTo(stream);
  std::ostringstream os;
  os << hydrogen()->access() << " <- ";
  stream->Add(os.str().c_str());
  value()->PrintTo(stream);
}


void LStoreNamedGeneric::PrintDataTo(StringStream* stream) {
  object()->PrintTo(stream);
  stream->Add(".");
  stream->Add(String::cast(*name())->ToCString().get());
  stream->Add(" <- ");
  value()->PrintTo(stream);
}


void LLoadKeyed::PrintDataTo(StringStream* stream) {
  elements()->PrintTo(stream);
  stream->Add("[");
  key()->PrintTo(stream);
  if (hydrogen()->IsDehoisted()) {
    stream->Add(" + %d]", base_offset());
  } else {
    stream->Add("]");
  }
}


void LStoreKeyed::PrintDataTo(StringStream* stream) {
  elements()->PrintTo(stream);
  stream->Add("[");
  key()->PrintTo(stream);
  if (hydrogen()->IsDehoisted()) {
    stream->Add(" + %d] <-", base_offset());
  } else {
    stream->Add("] <- ");
  }

  if (value() == NULL) {
    DCHECK(hydrogen()->IsConstantHoleStore() &&
           hydrogen()->value()->representation().IsDouble());
    stream->Add("<the hole(nan)>");
  } else {
    value()->PrintTo(stream);
  }
}


void LStoreKeyedGeneric::PrintDataTo(StringStream* stream) {
  object()->PrintTo(stream);
  stream->Add("[");
  key()->PrintTo(stream);
  stream->Add("] <- ");
  value()->PrintTo(stream);
}


void LTransitionElementsKind::PrintDataTo(StringStream* stream) {
  object()->PrintTo(stream);
  stream->Add(" %p -> %p", *original_map(), *transitioned_map());
}


LPlatformChunk* LChunkBuilder::Build() {
  DCHECK(is_unused());
  chunk_ = new(zone()) LPlatformChunk(info(), graph());
  LPhase phase("L_Building chunk", chunk_);
  status_ = BUILDING;

  // If compiling for OSR, reserve space for the unoptimized frame,
  // which will be subsumed into this frame.
  if (graph()->has_osr()) {
    for (int i = graph()->osr()->UnoptimizedFrameSlots(); i > 0; i--) {
      chunk_->GetNextSpillIndex(GENERAL_REGISTERS);
    }
  }

  const ZoneList<HBasicBlock*>* blocks = graph()->blocks();
  for (int i = 0; i < blocks->length(); i++) {
    HBasicBlock* next = NULL;
    if (i < blocks->length() - 1) next = blocks->at(i + 1);
    DoBasicBlock(blocks->at(i), next);
    if (is_aborted()) return NULL;
  }
  status_ = DONE;
  return chunk_;
}


LUnallocated* LChunkBuilder::ToUnallocated(Register reg) {
  return new(zone()) LUnallocated(LUnallocated::FIXED_REGISTER,
                                  Register::ToAllocationIndex(reg));
}


LUnallocated* LChunkBuilder::ToUnallocated(XMMRegister reg) {
  return new(zone()) LUnallocated(LUnallocated::FIXED_DOUBLE_REGISTER,
                                  XMMRegister::ToAllocationIndex(reg));
}


LOperand* LChunkBuilder::UseFixed(HValue* value, Register fixed_register) {
  return Use(value, ToUnallocated(fixed_register));
}


LOperand* LChunkBuilder::UseFixedDouble(HValue* value, XMMRegister reg) {
  return Use(value, ToUnallocated(reg));
}


LOperand* LChunkBuilder::UseRegister(HValue* value) {
  return Use(value, new(zone()) LUnallocated(LUnallocated::MUST_HAVE_REGISTER));
}


LOperand* LChunkBuilder::UseRegisterAtStart(HValue* value) {
  return Use(value,
             new(zone()) LUnallocated(LUnallocated::MUST_HAVE_REGISTER,
                              LUnallocated::USED_AT_START));
}


LOperand* LChunkBuilder::UseTempRegister(HValue* value) {
  return Use(value, new(zone()) LUnallocated(LUnallocated::WRITABLE_REGISTER));
}


LOperand* LChunkBuilder::UseTempRegisterOrConstant(HValue* value) {
  return value->IsConstant()
      ? chunk_->DefineConstantOperand(HConstant::cast(value))
      : UseTempRegister(value);
}


LOperand* LChunkBuilder::Use(HValue* value) {
  return Use(value, new(zone()) LUnallocated(LUnallocated::NONE));
}


LOperand* LChunkBuilder::UseAtStart(HValue* value) {
  return Use(value, new(zone()) LUnallocated(LUnallocated::NONE,
                                     LUnallocated::USED_AT_START));
}


LOperand* LChunkBuilder::UseOrConstant(HValue* value) {
  return value->IsConstant()
      ? chunk_->DefineConstantOperand(HConstant::cast(value))
      : Use(value);
}


LOperand* LChunkBuilder::UseOrConstantAtStart(HValue* value) {
  return value->IsConstant()
      ? chunk_->DefineConstantOperand(HConstant::cast(value))
      : UseAtStart(value);
}


LOperand* LChunkBuilder::UseRegisterOrConstant(HValue* value) {
  return value->IsConstant()
      ? chunk_->DefineConstantOperand(HConstant::cast(value))
      : UseRegister(value);
}


LOperand* LChunkBuilder::UseRegisterOrConstantAtStart(HValue* value) {
  return value->IsConstant()
      ? chunk_->DefineConstantOperand(HConstant::cast(value))
      : UseRegisterAtStart(value);
}


LOperand* LChunkBuilder::UseConstant(HValue* value) {
  return chunk_->DefineConstantOperand(HConstant::cast(value));
}


LOperand* LChunkBuilder::UseAny(HValue* value) {
  return value->IsConstant()
      ? chunk_->DefineConstantOperand(HConstant::cast(value))
      :  Use(value, new(zone()) LUnallocated(LUnallocated::ANY));
}


LOperand* LChunkBuilder::Use(HValue* value, LUnallocated* operand) {
  if (value->EmitAtUses()) {
    HInstruction* instr = HInstruction::cast(value);
    VisitInstruction(instr);
  }
  operand->set_virtual_register(value->id());
  return operand;
}


LInstruction* LChunkBuilder::Define(LTemplateResultInstruction<1>* instr,
                                    LUnallocated* result) {
  result->set_virtual_register(current_instruction_->id());
  instr->set_result(result);
  return instr;
}


LInstruction* LChunkBuilder::DefineAsRegister(
    LTemplateResultInstruction<1>* instr) {
  return Define(instr,
                new(zone()) LUnallocated(LUnallocated::MUST_HAVE_REGISTER));
}


LInstruction* LChunkBuilder::DefineAsSpilled(
    LTemplateResultInstruction<1>* instr,
    int index) {
  return Define(instr,
                new(zone()) LUnallocated(LUnallocated::FIXED_SLOT, index));
}


LInstruction* LChunkBuilder::DefineSameAsFirst(
    LTemplateResultInstruction<1>* instr) {
  return Define(instr,
                new(zone()) LUnallocated(LUnallocated::SAME_AS_FIRST_INPUT));
}


LInstruction* LChunkBuilder::DefineFixed(LTemplateResultInstruction<1>* instr,
                                         Register reg) {
  return Define(instr, ToUnallocated(reg));
}


LInstruction* LChunkBuilder::DefineFixedDouble(
    LTemplateResultInstruction<1>* instr,
    XMMRegister reg) {
  return Define(instr, ToUnallocated(reg));
}


LInstruction* LChunkBuilder::AssignEnvironment(LInstruction* instr) {
  HEnvironment* hydrogen_env = current_block_->last_environment();
  int argument_index_accumulator = 0;
  ZoneList<HValue*> objects_to_materialize(0, zone());
  instr->set_environment(CreateEnvironment(
      hydrogen_env, &argument_index_accumulator, &objects_to_materialize));
  return instr;
}


LInstruction* LChunkBuilder::MarkAsCall(LInstruction* instr,
                                        HInstruction* hinstr,
                                        CanDeoptimize can_deoptimize) {
  info()->MarkAsNonDeferredCalling();

#ifdef DEBUG
  instr->VerifyCall();
#endif
  instr->MarkAsCall();
  instr = AssignPointerMap(instr);

  // If instruction does not have side-effects lazy deoptimization
  // after the call will try to deoptimize to the point before the call.
  // Thus we still need to attach environment to this call even if
  // call sequence can not deoptimize eagerly.
  bool needs_environment =
      (can_deoptimize == CAN_DEOPTIMIZE_EAGERLY) ||
      !hinstr->HasObservableSideEffects();
  if (needs_environment && !instr->HasEnvironment()) {
    instr = AssignEnvironment(instr);
    // We can't really figure out if the environment is needed or not.
    instr->environment()->set_has_been_used();
  }

  return instr;
}


LInstruction* LChunkBuilder::AssignPointerMap(LInstruction* instr) {
  DCHECK(!instr->HasPointerMap());
  instr->set_pointer_map(new(zone()) LPointerMap(zone()));
  return instr;
}


LUnallocated* LChunkBuilder::TempRegister() {
  LUnallocated* operand =
      new(zone()) LUnallocated(LUnallocated::MUST_HAVE_REGISTER);
  int vreg = allocator_->GetVirtualRegister();
  if (!allocator_->AllocationOk()) {
    Abort(kOutOfVirtualRegistersWhileTryingToAllocateTempRegister);
    vreg = 0;
  }
  operand->set_virtual_register(vreg);
  return operand;
}


LOperand* LChunkBuilder::FixedTemp(Register reg) {
  LUnallocated* operand = ToUnallocated(reg);
  DCHECK(operand->HasFixedPolicy());
  return operand;
}


LOperand* LChunkBuilder::FixedTemp(XMMRegister reg) {
  LUnallocated* operand = ToUnallocated(reg);
  DCHECK(operand->HasFixedPolicy());
  return operand;
}


LInstruction* LChunkBuilder::DoBlockEntry(HBlockEntry* instr) {
  return new(zone()) LLabel(instr->block());
}


LInstruction* LChunkBuilder::DoDummyUse(HDummyUse* instr) {
  return DefineAsRegister(new(zone()) LDummyUse(UseAny(instr->value())));
}


LInstruction* LChunkBuilder::DoEnvironmentMarker(HEnvironmentMarker* instr) {
  UNREACHABLE();
  return NULL;
}


LInstruction* LChunkBuilder::DoDeoptimize(HDeoptimize* instr) {
  return AssignEnvironment(new(zone()) LDeoptimize);
}


LInstruction* LChunkBuilder::DoShift(Token::Value op,
                                     HBitwiseBinaryOperation* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    DCHECK(instr->left()->representation().Equals(instr->representation()));
    DCHECK(instr->right()->representation().Equals(instr->representation()));
    LOperand* left = UseRegisterAtStart(instr->left());

    HValue* right_value = instr->right();
    LOperand* right = NULL;
    int constant_value = 0;
    bool does_deopt = false;
    if (right_value->IsConstant()) {
      HConstant* constant = HConstant::cast(right_value);
      right = chunk_->DefineConstantOperand(constant);
      constant_value = constant->Integer32Value() & 0x1f;
      if (SmiValuesAre31Bits() && instr->representation().IsSmi() &&
          constant_value > 0) {
        // Left shift can deoptimize if we shift by > 0 and the result
        // cannot be truncated to smi.
        does_deopt = !instr->CheckUsesForFlag(HValue::kTruncatingToSmi);
      }
    } else {
      right = UseFixed(right_value, rcx);
    }

    // Shift operations can only deoptimize if we do a logical shift by 0 and
    // the result cannot be truncated to int32.
    if (op == Token::SHR && constant_value == 0) {
      does_deopt = !instr->CheckFlag(HInstruction::kUint32);
    }

    LInstruction* result =
        DefineSameAsFirst(new(zone()) LShiftI(op, left, right, does_deopt));
    return does_deopt ? AssignEnvironment(result) : result;
  } else {
    return DoArithmeticT(op, instr);
  }
}


LInstruction* LChunkBuilder::DoArithmeticD(Token::Value op,
                                           HArithmeticBinaryOperation* instr) {
  DCHECK(instr->representation().IsDouble());
  DCHECK(instr->left()->representation().IsDouble());
  DCHECK(instr->right()->representation().IsDouble());
  if (op == Token::MOD) {
    LOperand* left = UseRegisterAtStart(instr->BetterLeftOperand());
    LOperand* right = UseFixedDouble(instr->BetterRightOperand(), xmm1);
    LArithmeticD* result = new(zone()) LArithmeticD(op, left, right);
    return MarkAsCall(DefineSameAsFirst(result), instr);
  } else {
    LOperand* left = UseRegisterAtStart(instr->BetterLeftOperand());
    LOperand* right = UseRegisterAtStart(instr->BetterRightOperand());
    LArithmeticD* result = new(zone()) LArithmeticD(op, left, right);
    return DefineSameAsFirst(result);
  }
}


LInstruction* LChunkBuilder::DoArithmeticT(Token::Value op,
                                           HBinaryOperation* instr) {
  HValue* left = instr->left();
  HValue* right = instr->right();
  DCHECK(left->representation().IsTagged());
  DCHECK(right->representation().IsTagged());
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* left_operand = UseFixed(left, rdx);
  LOperand* right_operand = UseFixed(right, rax);
  LArithmeticT* result =
      new(zone()) LArithmeticT(op, context, left_operand, right_operand);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


void LChunkBuilder::DoBasicBlock(HBasicBlock* block, HBasicBlock* next_block) {
  DCHECK(is_building());
  current_block_ = block;
  next_block_ = next_block;
  if (block->IsStartBlock()) {
    block->UpdateEnvironment(graph_->start_environment());
    argument_count_ = 0;
  } else if (block->predecessors()->length() == 1) {
    // We have a single predecessor => copy environment and outgoing
    // argument count from the predecessor.
    DCHECK(block->phis()->length() == 0);
    HBasicBlock* pred = block->predecessors()->at(0);
    HEnvironment* last_environment = pred->last_environment();
    DCHECK(last_environment != NULL);
    // Only copy the environment, if it is later used again.
    if (pred->end()->SecondSuccessor() == NULL) {
      DCHECK(pred->end()->FirstSuccessor() == block);
    } else {
      if (pred->end()->FirstSuccessor()->block_id() > block->block_id() ||
          pred->end()->SecondSuccessor()->block_id() > block->block_id()) {
        last_environment = last_environment->Copy();
      }
    }
    block->UpdateEnvironment(last_environment);
    DCHECK(pred->argument_count() >= 0);
    argument_count_ = pred->argument_count();
  } else {
    // We are at a state join => process phis.
    HBasicBlock* pred = block->predecessors()->at(0);
    // No need to copy the environment, it cannot be used later.
    HEnvironment* last_environment = pred->last_environment();
    for (int i = 0; i < block->phis()->length(); ++i) {
      HPhi* phi = block->phis()->at(i);
      if (phi->HasMergedIndex()) {
        last_environment->SetValueAt(phi->merged_index(), phi);
      }
    }
    for (int i = 0; i < block->deleted_phis()->length(); ++i) {
      if (block->deleted_phis()->at(i) < last_environment->length()) {
        last_environment->SetValueAt(block->deleted_phis()->at(i),
                                     graph_->GetConstantUndefined());
      }
    }
    block->UpdateEnvironment(last_environment);
    // Pick up the outgoing argument count of one of the predecessors.
    argument_count_ = pred->argument_count();
  }
  HInstruction* current = block->first();
  int start = chunk_->instructions()->length();
  while (current != NULL && !is_aborted()) {
    // Code for constants in registers is generated lazily.
    if (!current->EmitAtUses()) {
      VisitInstruction(current);
    }
    current = current->next();
  }
  int end = chunk_->instructions()->length() - 1;
  if (end >= start) {
    block->set_first_instruction_index(start);
    block->set_last_instruction_index(end);
  }
  block->set_argument_count(argument_count_);
  next_block_ = NULL;
  current_block_ = NULL;
}


void LChunkBuilder::VisitInstruction(HInstruction* current) {
  HInstruction* old_current = current_instruction_;
  current_instruction_ = current;

  LInstruction* instr = NULL;
  if (current->CanReplaceWithDummyUses()) {
    if (current->OperandCount() == 0) {
      instr = DefineAsRegister(new(zone()) LDummy());
    } else {
      DCHECK(!current->OperandAt(0)->IsControlInstruction());
      instr = DefineAsRegister(new(zone())
          LDummyUse(UseAny(current->OperandAt(0))));
    }
    for (int i = 1; i < current->OperandCount(); ++i) {
      if (current->OperandAt(i)->IsControlInstruction()) continue;
      LInstruction* dummy =
          new(zone()) LDummyUse(UseAny(current->OperandAt(i)));
      dummy->set_hydrogen_value(current);
      chunk_->AddInstruction(dummy, current_block_);
    }
  } else {
    HBasicBlock* successor;
    if (current->IsControlInstruction() &&
        HControlInstruction::cast(current)->KnownSuccessorBlock(&successor) &&
        successor != NULL) {
      instr = new(zone()) LGoto(successor);
    } else {
      instr = current->CompileToLithium(this);
    }
  }

  argument_count_ += current->argument_delta();
  DCHECK(argument_count_ >= 0);

  if (instr != NULL) {
    AddInstruction(instr, current);
  }

  current_instruction_ = old_current;
}


void LChunkBuilder::AddInstruction(LInstruction* instr,
                                   HInstruction* hydrogen_val) {
  // Associate the hydrogen instruction first, since we may need it for
  // the ClobbersRegisters() or ClobbersDoubleRegisters() calls below.
  instr->set_hydrogen_value(hydrogen_val);

#if DEBUG
  // Make sure that the lithium instruction has either no fixed register
  // constraints in temps or the result OR no uses that are only used at
  // start. If this invariant doesn't hold, the register allocator can decide
  // to insert a split of a range immediately before the instruction due to an
  // already allocated register needing to be used for the instruction's fixed
  // register constraint. In this case, The register allocator won't see an
  // interference between the split child and the use-at-start (it would if
  // the it was just a plain use), so it is free to move the split child into
  // the same register that is used for the use-at-start.
  // See https://code.google.com/p/chromium/issues/detail?id=201590
  if (!(instr->ClobbersRegisters() &&
        instr->ClobbersDoubleRegisters(isolate()))) {
    int fixed = 0;
    int used_at_start = 0;
    for (UseIterator it(instr); !it.Done(); it.Advance()) {
      LUnallocated* operand = LUnallocated::cast(it.Current());
      if (operand->IsUsedAtStart()) ++used_at_start;
    }
    if (instr->Output() != NULL) {
      if (LUnallocated::cast(instr->Output())->HasFixedPolicy()) ++fixed;
    }
    for (TempIterator it(instr); !it.Done(); it.Advance()) {
      LUnallocated* operand = LUnallocated::cast(it.Current());
      if (operand->HasFixedPolicy()) ++fixed;
    }
    DCHECK(fixed == 0 || used_at_start == 0);
  }
#endif

  if (FLAG_stress_pointer_maps && !instr->HasPointerMap()) {
    instr = AssignPointerMap(instr);
  }
  if (FLAG_stress_environments && !instr->HasEnvironment()) {
    instr = AssignEnvironment(instr);
  }
  chunk_->AddInstruction(instr, current_block_);

  if (instr->IsCall()) {
    HValue* hydrogen_value_for_lazy_bailout = hydrogen_val;
    LInstruction* instruction_needing_environment = NULL;
    if (hydrogen_val->HasObservableSideEffects()) {
      HSimulate* sim = HSimulate::cast(hydrogen_val->next());
      instruction_needing_environment = instr;
      sim->ReplayEnvironment(current_block_->last_environment());
      hydrogen_value_for_lazy_bailout = sim;
    }
    LInstruction* bailout = AssignEnvironment(new(zone()) LLazyBailout());
    bailout->set_hydrogen_value(hydrogen_value_for_lazy_bailout);
    chunk_->AddInstruction(bailout, current_block_);
    if (instruction_needing_environment != NULL) {
      // Store the lazy deopt environment with the instruction if needed.
      // Right now it is only used for LInstanceOfKnownGlobal.
      instruction_needing_environment->
          SetDeferredLazyDeoptimizationEnvironment(bailout->environment());
    }
  }
}


LInstruction* LChunkBuilder::DoGoto(HGoto* instr) {
  return new(zone()) LGoto(instr->FirstSuccessor());
}


LInstruction* LChunkBuilder::DoDebugBreak(HDebugBreak* instr) {
  return new(zone()) LDebugBreak();
}


LInstruction* LChunkBuilder::DoBranch(HBranch* instr) {
  HValue* value = instr->value();
  Representation r = value->representation();
  HType type = value->type();
  ToBooleanStub::Types expected = instr->expected_input_types();
  if (expected.IsEmpty()) expected = ToBooleanStub::Types::Generic();

  bool easy_case = !r.IsTagged() || type.IsBoolean() || type.IsSmi() ||
      type.IsJSArray() || type.IsHeapNumber() || type.IsString();
  LInstruction* branch = new(zone()) LBranch(UseRegister(value));
  if (!easy_case &&
      ((!expected.Contains(ToBooleanStub::SMI) && expected.NeedsMap()) ||
       !expected.IsGeneric())) {
    branch = AssignEnvironment(branch);
  }
  return branch;
}


LInstruction* LChunkBuilder::DoCompareMap(HCompareMap* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  LOperand* value = UseRegisterAtStart(instr->value());
  return new(zone()) LCmpMapAndBranch(value);
}


LInstruction* LChunkBuilder::DoArgumentsLength(HArgumentsLength* length) {
  info()->MarkAsRequiresFrame();
  return DefineAsRegister(new(zone()) LArgumentsLength(Use(length->value())));
}


LInstruction* LChunkBuilder::DoArgumentsElements(HArgumentsElements* elems) {
  info()->MarkAsRequiresFrame();
  return DefineAsRegister(new(zone()) LArgumentsElements);
}


LInstruction* LChunkBuilder::DoInstanceOf(HInstanceOf* instr) {
  LOperand* left = UseFixed(instr->left(), rax);
  LOperand* right = UseFixed(instr->right(), rdx);
  LOperand* context = UseFixed(instr->context(), rsi);
  LInstanceOf* result = new(zone()) LInstanceOf(context, left, right);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoInstanceOfKnownGlobal(
    HInstanceOfKnownGlobal* instr) {
  LInstanceOfKnownGlobal* result =
      new(zone()) LInstanceOfKnownGlobal(UseFixed(instr->context(), rsi),
                                         UseFixed(instr->left(), rax),
                                         FixedTemp(rdi));
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoWrapReceiver(HWrapReceiver* instr) {
  LOperand* receiver = UseRegister(instr->receiver());
  LOperand* function = UseRegisterAtStart(instr->function());
  LWrapReceiver* result = new(zone()) LWrapReceiver(receiver, function);
  return AssignEnvironment(DefineSameAsFirst(result));
}


LInstruction* LChunkBuilder::DoApplyArguments(HApplyArguments* instr) {
  LOperand* function = UseFixed(instr->function(), rdi);
  LOperand* receiver = UseFixed(instr->receiver(), rax);
  LOperand* length = UseFixed(instr->length(), rbx);
  LOperand* elements = UseFixed(instr->elements(), rcx);
  LApplyArguments* result = new(zone()) LApplyArguments(function,
                                                receiver,
                                                length,
                                                elements);
  return MarkAsCall(DefineFixed(result, rax), instr, CAN_DEOPTIMIZE_EAGERLY);
}


LInstruction* LChunkBuilder::DoPushArguments(HPushArguments* instr) {
  int argc = instr->OperandCount();
  for (int i = 0; i < argc; ++i) {
    LOperand* argument = UseOrConstant(instr->argument(i));
    AddInstruction(new(zone()) LPushArgument(argument), instr);
  }
  return NULL;
}


LInstruction* LChunkBuilder::DoStoreCodeEntry(
    HStoreCodeEntry* store_code_entry) {
  LOperand* function = UseRegister(store_code_entry->function());
  LOperand* code_object = UseTempRegister(store_code_entry->code_object());
  return new(zone()) LStoreCodeEntry(function, code_object);
}


LInstruction* LChunkBuilder::DoInnerAllocatedObject(
    HInnerAllocatedObject* instr) {
  LOperand* base_object = UseRegisterAtStart(instr->base_object());
  LOperand* offset = UseRegisterOrConstantAtStart(instr->offset());
  return DefineAsRegister(
      new(zone()) LInnerAllocatedObject(base_object, offset));
}


LInstruction* LChunkBuilder::DoThisFunction(HThisFunction* instr) {
  return instr->HasNoUses()
      ? NULL
      : DefineAsRegister(new(zone()) LThisFunction);
}


LInstruction* LChunkBuilder::DoContext(HContext* instr) {
  if (instr->HasNoUses()) return NULL;

  if (info()->IsStub()) {
    return DefineFixed(new(zone()) LContext, rsi);
  }

  return DefineAsRegister(new(zone()) LContext);
}


LInstruction* LChunkBuilder::DoDeclareGlobals(HDeclareGlobals* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  return MarkAsCall(new(zone()) LDeclareGlobals(context), instr);
}


LInstruction* LChunkBuilder::DoCallJSFunction(
    HCallJSFunction* instr) {
  LOperand* function = UseFixed(instr->function(), rdi);

  LCallJSFunction* result = new(zone()) LCallJSFunction(function);

  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoCallWithDescriptor(
    HCallWithDescriptor* instr) {
  CallInterfaceDescriptor descriptor = instr->descriptor();

  LOperand* target = UseRegisterOrConstantAtStart(instr->target());
  ZoneList<LOperand*> ops(instr->OperandCount(), zone());
  ops.Add(target, zone());
  for (int i = 1; i < instr->OperandCount(); i++) {
    LOperand* op =
        UseFixed(instr->OperandAt(i), descriptor.GetParameterRegister(i - 1));
    ops.Add(op, zone());
  }

  LCallWithDescriptor* result = new(zone()) LCallWithDescriptor(
      descriptor, ops, zone());
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoTailCallThroughMegamorphicCache(
    HTailCallThroughMegamorphicCache* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* receiver_register =
      UseFixed(instr->receiver(), LoadDescriptor::ReceiverRegister());
  LOperand* name_register =
      UseFixed(instr->name(), LoadDescriptor::NameRegister());
  // Not marked as call. It can't deoptimize, and it never returns.
  return new (zone()) LTailCallThroughMegamorphicCache(
      context, receiver_register, name_register);
}


LInstruction* LChunkBuilder::DoInvokeFunction(HInvokeFunction* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* function = UseFixed(instr->function(), rdi);
  LInvokeFunction* result = new(zone()) LInvokeFunction(context, function);
  return MarkAsCall(DefineFixed(result, rax), instr, CANNOT_DEOPTIMIZE_EAGERLY);
}


LInstruction* LChunkBuilder::DoUnaryMathOperation(HUnaryMathOperation* instr) {
  switch (instr->op()) {
    case kMathFloor:
      return DoMathFloor(instr);
    case kMathRound:
      return DoMathRound(instr);
    case kMathFround:
      return DoMathFround(instr);
    case kMathAbs:
      return DoMathAbs(instr);
    case kMathLog:
      return DoMathLog(instr);
    case kMathExp:
      return DoMathExp(instr);
    case kMathSqrt:
      return DoMathSqrt(instr);
    case kMathPowHalf:
      return DoMathPowHalf(instr);
    case kMathClz32:
      return DoMathClz32(instr);
    default:
      UNREACHABLE();
      return NULL;
  }
}


LInstruction* LChunkBuilder::DoMathFloor(HUnaryMathOperation* instr) {
  LOperand* input = UseRegisterAtStart(instr->value());
  LMathFloor* result = new(zone()) LMathFloor(input);
  return AssignEnvironment(DefineAsRegister(result));
}


LInstruction* LChunkBuilder::DoMathRound(HUnaryMathOperation* instr) {
  LOperand* input = UseRegister(instr->value());
  LOperand* temp = FixedTemp(xmm4);
  LMathRound* result = new(zone()) LMathRound(input, temp);
  return AssignEnvironment(DefineAsRegister(result));
}


LInstruction* LChunkBuilder::DoMathFround(HUnaryMathOperation* instr) {
  LOperand* input = UseRegister(instr->value());
  LMathFround* result = new (zone()) LMathFround(input);
  return DefineAsRegister(result);
}


LInstruction* LChunkBuilder::DoMathAbs(HUnaryMathOperation* instr) {
  LOperand* context = UseAny(instr->context());
  LOperand* input = UseRegisterAtStart(instr->value());
  LInstruction* result =
      DefineSameAsFirst(new(zone()) LMathAbs(context, input));
  Representation r = instr->value()->representation();
  if (!r.IsDouble() && !r.IsSmiOrInteger32()) result = AssignPointerMap(result);
  if (!r.IsDouble()) result = AssignEnvironment(result);
  return result;
}


LInstruction* LChunkBuilder::DoMathLog(HUnaryMathOperation* instr) {
  DCHECK(instr->representation().IsDouble());
  DCHECK(instr->value()->representation().IsDouble());
  LOperand* input = UseRegisterAtStart(instr->value());
  return MarkAsCall(DefineSameAsFirst(new(zone()) LMathLog(input)), instr);
}


LInstruction* LChunkBuilder::DoMathClz32(HUnaryMathOperation* instr) {
  LOperand* input = UseRegisterAtStart(instr->value());
  LMathClz32* result = new(zone()) LMathClz32(input);
  return DefineAsRegister(result);
}


LInstruction* LChunkBuilder::DoMathExp(HUnaryMathOperation* instr) {
  DCHECK(instr->representation().IsDouble());
  DCHECK(instr->value()->representation().IsDouble());
  LOperand* value = UseTempRegister(instr->value());
  LOperand* temp1 = TempRegister();
  LOperand* temp2 = TempRegister();
  LMathExp* result = new(zone()) LMathExp(value, temp1, temp2);
  return DefineAsRegister(result);
}


LInstruction* LChunkBuilder::DoMathSqrt(HUnaryMathOperation* instr) {
  LOperand* input = UseAtStart(instr->value());
  return DefineAsRegister(new(zone()) LMathSqrt(input));
}


LInstruction* LChunkBuilder::DoMathPowHalf(HUnaryMathOperation* instr) {
  LOperand* input = UseRegisterAtStart(instr->value());
  LMathPowHalf* result = new(zone()) LMathPowHalf(input);
  return DefineSameAsFirst(result);
}


LInstruction* LChunkBuilder::DoCallNew(HCallNew* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* constructor = UseFixed(instr->constructor(), rdi);
  LCallNew* result = new(zone()) LCallNew(context, constructor);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoCallNewArray(HCallNewArray* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* constructor = UseFixed(instr->constructor(), rdi);
  LCallNewArray* result = new(zone()) LCallNewArray(context, constructor);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoCallFunction(HCallFunction* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* function = UseFixed(instr->function(), rdi);
  LCallFunction* call = new(zone()) LCallFunction(context, function);
  return MarkAsCall(DefineFixed(call, rax), instr);
}


LInstruction* LChunkBuilder::DoCallRuntime(HCallRuntime* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LCallRuntime* result = new(zone()) LCallRuntime(context);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoRor(HRor* instr) {
  return DoShift(Token::ROR, instr);
}


LInstruction* LChunkBuilder::DoShr(HShr* instr) {
  return DoShift(Token::SHR, instr);
}


LInstruction* LChunkBuilder::DoSar(HSar* instr) {
  return DoShift(Token::SAR, instr);
}


LInstruction* LChunkBuilder::DoShl(HShl* instr) {
  return DoShift(Token::SHL, instr);
}


LInstruction* LChunkBuilder::DoBitwise(HBitwise* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    DCHECK(instr->left()->representation().Equals(instr->representation()));
    DCHECK(instr->right()->representation().Equals(instr->representation()));
    DCHECK(instr->CheckFlag(HValue::kTruncatingToInt32));

    LOperand* left = UseRegisterAtStart(instr->BetterLeftOperand());
    LOperand* right = UseOrConstantAtStart(instr->BetterRightOperand());
    return DefineSameAsFirst(new(zone()) LBitI(left, right));
  } else {
    return DoArithmeticT(instr->op(), instr);
  }
}


LInstruction* LChunkBuilder::DoDivByPowerOf2I(HDiv* instr) {
  DCHECK(instr->representation().IsSmiOrInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseRegister(instr->left());
  int32_t divisor = instr->right()->GetInteger32Constant();
  LInstruction* result = DefineAsRegister(new(zone()) LDivByPowerOf2I(
          dividend, divisor));
  if ((instr->CheckFlag(HValue::kBailoutOnMinusZero) && divisor < 0) ||
      (instr->CheckFlag(HValue::kCanOverflow) && divisor == -1) ||
      (!instr->CheckFlag(HInstruction::kAllUsesTruncatingToInt32) &&
       divisor != 1 && divisor != -1)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoDivByConstI(HDiv* instr) {
  DCHECK(instr->representation().IsInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseRegister(instr->left());
  int32_t divisor = instr->right()->GetInteger32Constant();
  LOperand* temp1 = FixedTemp(rax);
  LOperand* temp2 = FixedTemp(rdx);
  LInstruction* result = DefineFixed(new(zone()) LDivByConstI(
          dividend, divisor, temp1, temp2), rdx);
  if (divisor == 0 ||
      (instr->CheckFlag(HValue::kBailoutOnMinusZero) && divisor < 0) ||
      !instr->CheckFlag(HInstruction::kAllUsesTruncatingToInt32)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoDivI(HDiv* instr) {
  DCHECK(instr->representation().IsSmiOrInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseFixed(instr->left(), rax);
  LOperand* divisor = UseRegister(instr->right());
  LOperand* temp = FixedTemp(rdx);
  LInstruction* result = DefineFixed(new(zone()) LDivI(
          dividend, divisor, temp), rax);
  if (instr->CheckFlag(HValue::kCanBeDivByZero) ||
      instr->CheckFlag(HValue::kBailoutOnMinusZero) ||
      instr->CheckFlag(HValue::kCanOverflow) ||
      !instr->CheckFlag(HValue::kAllUsesTruncatingToInt32)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoDiv(HDiv* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    if (instr->RightIsPowerOf2()) {
      return DoDivByPowerOf2I(instr);
    } else if (instr->right()->IsConstant()) {
      return DoDivByConstI(instr);
    } else {
      return DoDivI(instr);
    }
  } else if (instr->representation().IsDouble()) {
    return DoArithmeticD(Token::DIV, instr);
  } else {
    return DoArithmeticT(Token::DIV, instr);
  }
}


LInstruction* LChunkBuilder::DoFlooringDivByPowerOf2I(HMathFloorOfDiv* instr) {
  LOperand* dividend = UseRegisterAtStart(instr->left());
  int32_t divisor = instr->right()->GetInteger32Constant();
  LInstruction* result = DefineSameAsFirst(new(zone()) LFlooringDivByPowerOf2I(
          dividend, divisor));
  if ((instr->CheckFlag(HValue::kBailoutOnMinusZero) && divisor < 0) ||
      (instr->CheckFlag(HValue::kLeftCanBeMinInt) && divisor == -1)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoFlooringDivByConstI(HMathFloorOfDiv* instr) {
  DCHECK(instr->representation().IsInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseRegister(instr->left());
  int32_t divisor = instr->right()->GetInteger32Constant();
  LOperand* temp1 = FixedTemp(rax);
  LOperand* temp2 = FixedTemp(rdx);
  LOperand* temp3 =
      ((divisor > 0 && !instr->CheckFlag(HValue::kLeftCanBeNegative)) ||
       (divisor < 0 && !instr->CheckFlag(HValue::kLeftCanBePositive))) ?
      NULL : TempRegister();
  LInstruction* result =
      DefineFixed(new(zone()) LFlooringDivByConstI(dividend,
                                                   divisor,
                                                   temp1,
                                                   temp2,
                                                   temp3),
                  rdx);
  if (divisor == 0 ||
      (instr->CheckFlag(HValue::kBailoutOnMinusZero) && divisor < 0)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoFlooringDivI(HMathFloorOfDiv* instr) {
  DCHECK(instr->representation().IsSmiOrInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseFixed(instr->left(), rax);
  LOperand* divisor = UseRegister(instr->right());
  LOperand* temp = FixedTemp(rdx);
  LInstruction* result = DefineFixed(new(zone()) LFlooringDivI(
          dividend, divisor, temp), rax);
  if (instr->CheckFlag(HValue::kCanBeDivByZero) ||
      instr->CheckFlag(HValue::kBailoutOnMinusZero) ||
      instr->CheckFlag(HValue::kCanOverflow)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoMathFloorOfDiv(HMathFloorOfDiv* instr) {
  if (instr->RightIsPowerOf2()) {
    return DoFlooringDivByPowerOf2I(instr);
  } else if (instr->right()->IsConstant()) {
    return DoFlooringDivByConstI(instr);
  } else {
    return DoFlooringDivI(instr);
  }
}


LInstruction* LChunkBuilder::DoModByPowerOf2I(HMod* instr) {
  DCHECK(instr->representation().IsSmiOrInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseRegisterAtStart(instr->left());
  int32_t divisor = instr->right()->GetInteger32Constant();
  LInstruction* result = DefineSameAsFirst(new(zone()) LModByPowerOf2I(
          dividend, divisor));
  if (instr->CheckFlag(HValue::kLeftCanBeNegative) &&
      instr->CheckFlag(HValue::kBailoutOnMinusZero)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoModByConstI(HMod* instr) {
  DCHECK(instr->representation().IsSmiOrInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseRegister(instr->left());
  int32_t divisor = instr->right()->GetInteger32Constant();
  LOperand* temp1 = FixedTemp(rax);
  LOperand* temp2 = FixedTemp(rdx);
  LInstruction* result = DefineFixed(new(zone()) LModByConstI(
          dividend, divisor, temp1, temp2), rax);
  if (divisor == 0 || instr->CheckFlag(HValue::kBailoutOnMinusZero)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoModI(HMod* instr) {
  DCHECK(instr->representation().IsSmiOrInteger32());
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  LOperand* dividend = UseFixed(instr->left(), rax);
  LOperand* divisor = UseRegister(instr->right());
  LOperand* temp = FixedTemp(rdx);
  LInstruction* result = DefineFixed(new(zone()) LModI(
          dividend, divisor, temp), rdx);
  if (instr->CheckFlag(HValue::kCanBeDivByZero) ||
      instr->CheckFlag(HValue::kBailoutOnMinusZero)) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoMod(HMod* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    if (instr->RightIsPowerOf2()) {
      return DoModByPowerOf2I(instr);
    } else if (instr->right()->IsConstant()) {
      return DoModByConstI(instr);
    } else {
      return DoModI(instr);
    }
  } else if (instr->representation().IsDouble()) {
    return DoArithmeticD(Token::MOD, instr);
  } else {
    return DoArithmeticT(Token::MOD, instr);
  }
}


LInstruction* LChunkBuilder::DoMul(HMul* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    DCHECK(instr->left()->representation().Equals(instr->representation()));
    DCHECK(instr->right()->representation().Equals(instr->representation()));
    LOperand* left = UseRegisterAtStart(instr->BetterLeftOperand());
    LOperand* right = UseOrConstant(instr->BetterRightOperand());
    LMulI* mul = new(zone()) LMulI(left, right);
    if (instr->CheckFlag(HValue::kCanOverflow) ||
        instr->CheckFlag(HValue::kBailoutOnMinusZero)) {
      AssignEnvironment(mul);
    }
    return DefineSameAsFirst(mul);
  } else if (instr->representation().IsDouble()) {
    return DoArithmeticD(Token::MUL, instr);
  } else {
    return DoArithmeticT(Token::MUL, instr);
  }
}


LInstruction* LChunkBuilder::DoSub(HSub* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    DCHECK(instr->left()->representation().Equals(instr->representation()));
    DCHECK(instr->right()->representation().Equals(instr->representation()));
    LOperand* left = UseRegisterAtStart(instr->left());
    LOperand* right = UseOrConstantAtStart(instr->right());
    LSubI* sub = new(zone()) LSubI(left, right);
    LInstruction* result = DefineSameAsFirst(sub);
    if (instr->CheckFlag(HValue::kCanOverflow)) {
      result = AssignEnvironment(result);
    }
    return result;
  } else if (instr->representation().IsDouble()) {
    return DoArithmeticD(Token::SUB, instr);
  } else {
    return DoArithmeticT(Token::SUB, instr);
  }
}


LInstruction* LChunkBuilder::DoAdd(HAdd* instr) {
  if (instr->representation().IsSmiOrInteger32()) {
    // Check to see if it would be advantageous to use an lea instruction rather
    // than an add. This is the case when no overflow check is needed and there
    // are multiple uses of the add's inputs, so using a 3-register add will
    // preserve all input values for later uses.
    bool use_lea = LAddI::UseLea(instr);
    DCHECK(instr->left()->representation().Equals(instr->representation()));
    DCHECK(instr->right()->representation().Equals(instr->representation()));
    LOperand* left = UseRegisterAtStart(instr->BetterLeftOperand());
    HValue* right_candidate = instr->BetterRightOperand();
    LOperand* right;
    if (SmiValuesAre32Bits() && instr->representation().IsSmi()) {
      // We cannot add a tagged immediate to a tagged value,
      // so we request it in a register.
      right = UseRegisterAtStart(right_candidate);
    } else {
      right = use_lea ? UseRegisterOrConstantAtStart(right_candidate)
                      : UseOrConstantAtStart(right_candidate);
    }
    LAddI* add = new(zone()) LAddI(left, right);
    bool can_overflow = instr->CheckFlag(HValue::kCanOverflow);
    LInstruction* result = use_lea ? DefineAsRegister(add)
                                   : DefineSameAsFirst(add);
    if (can_overflow) {
      result = AssignEnvironment(result);
    }
    return result;
  } else if (instr->representation().IsExternal()) {
    DCHECK(instr->left()->representation().IsExternal());
    DCHECK(instr->right()->representation().IsInteger32());
    DCHECK(!instr->CheckFlag(HValue::kCanOverflow));
    bool use_lea = LAddI::UseLea(instr);
    LOperand* left = UseRegisterAtStart(instr->left());
    HValue* right_candidate = instr->right();
    LOperand* right = use_lea
        ? UseRegisterOrConstantAtStart(right_candidate)
        : UseOrConstantAtStart(right_candidate);
    LAddI* add = new(zone()) LAddI(left, right);
    LInstruction* result = use_lea
        ? DefineAsRegister(add)
        : DefineSameAsFirst(add);
    return result;
  } else if (instr->representation().IsDouble()) {
    return DoArithmeticD(Token::ADD, instr);
  } else {
    return DoArithmeticT(Token::ADD, instr);
  }
  return NULL;
}


LInstruction* LChunkBuilder::DoMathMinMax(HMathMinMax* instr) {
  LOperand* left = NULL;
  LOperand* right = NULL;
  DCHECK(instr->left()->representation().Equals(instr->representation()));
  DCHECK(instr->right()->representation().Equals(instr->representation()));
  if (instr->representation().IsSmi()) {
    left = UseRegisterAtStart(instr->BetterLeftOperand());
    right = UseAtStart(instr->BetterRightOperand());
  } else if (instr->representation().IsInteger32()) {
    left = UseRegisterAtStart(instr->BetterLeftOperand());
    right = UseOrConstantAtStart(instr->BetterRightOperand());
  } else {
    DCHECK(instr->representation().IsDouble());
    left = UseRegisterAtStart(instr->left());
    right = UseRegisterAtStart(instr->right());
  }
  LMathMinMax* minmax = new(zone()) LMathMinMax(left, right);
  return DefineSameAsFirst(minmax);
}


LInstruction* LChunkBuilder::DoPower(HPower* instr) {
  DCHECK(instr->representation().IsDouble());
  // We call a C function for double power. It can't trigger a GC.
  // We need to use fixed result register for the call.
  Representation exponent_type = instr->right()->representation();
  DCHECK(instr->left()->representation().IsDouble());
  LOperand* left = UseFixedDouble(instr->left(), xmm2);
  LOperand* right =
      exponent_type.IsDouble()
          ? UseFixedDouble(instr->right(), xmm1)
          : UseFixed(instr->right(), MathPowTaggedDescriptor::exponent());
  LPower* result = new(zone()) LPower(left, right);
  return MarkAsCall(DefineFixedDouble(result, xmm3), instr,
                    CAN_DEOPTIMIZE_EAGERLY);
}


LInstruction* LChunkBuilder::DoCompareGeneric(HCompareGeneric* instr) {
  DCHECK(instr->left()->representation().IsTagged());
  DCHECK(instr->right()->representation().IsTagged());
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* left = UseFixed(instr->left(), rdx);
  LOperand* right = UseFixed(instr->right(), rax);
  LCmpT* result = new(zone()) LCmpT(context, left, right);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoCompareNumericAndBranch(
    HCompareNumericAndBranch* instr) {
  Representation r = instr->representation();
  if (r.IsSmiOrInteger32()) {
    DCHECK(instr->left()->representation().Equals(r));
    DCHECK(instr->right()->representation().Equals(r));
    LOperand* left = UseRegisterOrConstantAtStart(instr->left());
    LOperand* right = UseOrConstantAtStart(instr->right());
    return new(zone()) LCompareNumericAndBranch(left, right);
  } else {
    DCHECK(r.IsDouble());
    DCHECK(instr->left()->representation().IsDouble());
    DCHECK(instr->right()->representation().IsDouble());
    LOperand* left;
    LOperand* right;
    if (instr->left()->IsConstant() && instr->right()->IsConstant()) {
      left = UseRegisterOrConstantAtStart(instr->left());
      right = UseRegisterOrConstantAtStart(instr->right());
    } else {
      left = UseRegisterAtStart(instr->left());
      right = UseRegisterAtStart(instr->right());
    }
    return new(zone()) LCompareNumericAndBranch(left, right);
  }
}


LInstruction* LChunkBuilder::DoCompareObjectEqAndBranch(
    HCompareObjectEqAndBranch* instr) {
  LOperand* left = UseRegisterAtStart(instr->left());
  LOperand* right = UseRegisterOrConstantAtStart(instr->right());
  return new(zone()) LCmpObjectEqAndBranch(left, right);
}


LInstruction* LChunkBuilder::DoCompareHoleAndBranch(
    HCompareHoleAndBranch* instr) {
  LOperand* value = UseRegisterAtStart(instr->value());
  return new(zone()) LCmpHoleAndBranch(value);
}


LInstruction* LChunkBuilder::DoCompareMinusZeroAndBranch(
    HCompareMinusZeroAndBranch* instr) {
  LOperand* value = UseRegister(instr->value());
  return new(zone()) LCompareMinusZeroAndBranch(value);
}


LInstruction* LChunkBuilder::DoIsObjectAndBranch(HIsObjectAndBranch* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  return new(zone()) LIsObjectAndBranch(UseRegisterAtStart(instr->value()));
}


LInstruction* LChunkBuilder::DoIsStringAndBranch(HIsStringAndBranch* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  LOperand* value = UseRegisterAtStart(instr->value());
  LOperand* temp = TempRegister();
  return new(zone()) LIsStringAndBranch(value, temp);
}


LInstruction* LChunkBuilder::DoIsSmiAndBranch(HIsSmiAndBranch* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  return new(zone()) LIsSmiAndBranch(Use(instr->value()));
}


LInstruction* LChunkBuilder::DoIsUndetectableAndBranch(
    HIsUndetectableAndBranch* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  LOperand* value = UseRegisterAtStart(instr->value());
  LOperand* temp = TempRegister();
  return new(zone()) LIsUndetectableAndBranch(value, temp);
}


LInstruction* LChunkBuilder::DoStringCompareAndBranch(
    HStringCompareAndBranch* instr) {

  DCHECK(instr->left()->representation().IsTagged());
  DCHECK(instr->right()->representation().IsTagged());
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* left = UseFixed(instr->left(), rdx);
  LOperand* right = UseFixed(instr->right(), rax);
  LStringCompareAndBranch* result =
      new(zone()) LStringCompareAndBranch(context, left, right);

  return MarkAsCall(result, instr);
}


LInstruction* LChunkBuilder::DoHasInstanceTypeAndBranch(
    HHasInstanceTypeAndBranch* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  LOperand* value = UseRegisterAtStart(instr->value());
  return new(zone()) LHasInstanceTypeAndBranch(value);
}


LInstruction* LChunkBuilder::DoGetCachedArrayIndex(
    HGetCachedArrayIndex* instr)  {
  DCHECK(instr->value()->representation().IsTagged());
  LOperand* value = UseRegisterAtStart(instr->value());

  return DefineAsRegister(new(zone()) LGetCachedArrayIndex(value));
}


LInstruction* LChunkBuilder::DoHasCachedArrayIndexAndBranch(
    HHasCachedArrayIndexAndBranch* instr) {
  DCHECK(instr->value()->representation().IsTagged());
  LOperand* value = UseRegisterAtStart(instr->value());
  return new(zone()) LHasCachedArrayIndexAndBranch(value);
}


LInstruction* LChunkBuilder::DoClassOfTestAndBranch(
    HClassOfTestAndBranch* instr) {
  LOperand* value = UseRegister(instr->value());
  return new(zone()) LClassOfTestAndBranch(value,
                                           TempRegister(),
                                           TempRegister());
}


LInstruction* LChunkBuilder::DoMapEnumLength(HMapEnumLength* instr) {
  LOperand* map = UseRegisterAtStart(instr->value());
  return DefineAsRegister(new(zone()) LMapEnumLength(map));
}


LInstruction* LChunkBuilder::DoDateField(HDateField* instr) {
  LOperand* object = UseFixed(instr->value(), rax);
  LDateField* result = new(zone()) LDateField(object, instr->index());
  return MarkAsCall(DefineFixed(result, rax), instr, CAN_DEOPTIMIZE_EAGERLY);
}


LInstruction* LChunkBuilder::DoSeqStringGetChar(HSeqStringGetChar* instr) {
  LOperand* string = UseRegisterAtStart(instr->string());
  LOperand* index = UseRegisterOrConstantAtStart(instr->index());
  return DefineAsRegister(new(zone()) LSeqStringGetChar(string, index));
}


LInstruction* LChunkBuilder::DoSeqStringSetChar(HSeqStringSetChar* instr) {
  LOperand* string = UseRegisterAtStart(instr->string());
  LOperand* index = FLAG_debug_code
      ? UseRegisterAtStart(instr->index())
      : UseRegisterOrConstantAtStart(instr->index());
  LOperand* value = FLAG_debug_code
      ? UseRegisterAtStart(instr->value())
      : UseRegisterOrConstantAtStart(instr->value());
  LOperand* context = FLAG_debug_code ? UseFixed(instr->context(), rsi) : NULL;
  LInstruction* result = new(zone()) LSeqStringSetChar(context, string,
                                                       index, value);
  if (FLAG_debug_code) {
    result = MarkAsCall(result, instr);
  }
  return result;
}


LInstruction* LChunkBuilder::DoBoundsCheck(HBoundsCheck* instr) {
  if (!FLAG_debug_code && instr->skip_check()) return NULL;
  LOperand* index = UseRegisterOrConstantAtStart(instr->index());
  LOperand* length = !index->IsConstantOperand()
      ? UseOrConstantAtStart(instr->length())
      : UseAtStart(instr->length());
  LInstruction* result = new(zone()) LBoundsCheck(index, length);
  if (!FLAG_debug_code || !instr->skip_check()) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoBoundsCheckBaseIndexInformation(
    HBoundsCheckBaseIndexInformation* instr) {
  UNREACHABLE();
  return NULL;
}


LInstruction* LChunkBuilder::DoAbnormalExit(HAbnormalExit* instr) {
  // The control instruction marking the end of a block that completed
  // abruptly (e.g., threw an exception).  There is nothing specific to do.
  return NULL;
}


LInstruction* LChunkBuilder::DoUseConst(HUseConst* instr) {
  return NULL;
}


LInstruction* LChunkBuilder::DoForceRepresentation(HForceRepresentation* bad) {
  // All HForceRepresentation instructions should be eliminated in the
  // representation change phase of Hydrogen.
  UNREACHABLE();
  return NULL;
}


LInstruction* LChunkBuilder::DoChange(HChange* instr) {
  Representation from = instr->from();
  Representation to = instr->to();
  HValue* val = instr->value();
  if (from.IsSmi()) {
    if (to.IsTagged()) {
      LOperand* value = UseRegister(val);
      return DefineSameAsFirst(new(zone()) LDummyUse(value));
    }
    from = Representation::Tagged();
  }
  if (from.IsTagged()) {
    if (to.IsDouble()) {
      LOperand* value = UseRegister(val);
      LInstruction* result = DefineAsRegister(new(zone()) LNumberUntagD(value));
      if (!val->representation().IsSmi()) result = AssignEnvironment(result);
      return result;
    } else if (to.IsSmi()) {
      LOperand* value = UseRegister(val);
      if (val->type().IsSmi()) {
        return DefineSameAsFirst(new(zone()) LDummyUse(value));
      }
      return AssignEnvironment(DefineSameAsFirst(new(zone()) LCheckSmi(value)));
    } else {
      DCHECK(to.IsInteger32());
      if (val->type().IsSmi() || val->representation().IsSmi()) {
        LOperand* value = UseRegister(val);
        return DefineSameAsFirst(new(zone()) LSmiUntag(value, false));
      } else {
        LOperand* value = UseRegister(val);
        bool truncating = instr->CanTruncateToInt32();
        LOperand* xmm_temp = truncating ? NULL : FixedTemp(xmm1);
        LInstruction* result =
            DefineSameAsFirst(new(zone()) LTaggedToI(value, xmm_temp));
        if (!val->representation().IsSmi()) result = AssignEnvironment(result);
        return result;
      }
    }
  } else if (from.IsDouble()) {
    if (to.IsTagged()) {
      info()->MarkAsDeferredCalling();
      LOperand* value = UseRegister(val);
      LOperand* temp = TempRegister();
      LUnallocated* result_temp = TempRegister();
      LNumberTagD* result = new(zone()) LNumberTagD(value, temp);
      return AssignPointerMap(Define(result, result_temp));
    } else if (to.IsSmi()) {
      LOperand* value = UseRegister(val);
      return AssignEnvironment(
          DefineAsRegister(new(zone()) LDoubleToSmi(value)));
    } else {
      DCHECK(to.IsInteger32());
      LOperand* value = UseRegister(val);
      LInstruction* result = DefineAsRegister(new(zone()) LDoubleToI(value));
      if (!instr->CanTruncateToInt32()) result = AssignEnvironment(result);
      return result;
    }
  } else if (from.IsInteger32()) {
    info()->MarkAsDeferredCalling();
    if (to.IsTagged()) {
      if (!instr->CheckFlag(HValue::kCanOverflow)) {
        LOperand* value = UseRegister(val);
        return DefineAsRegister(new(zone()) LSmiTag(value));
      } else if (val->CheckFlag(HInstruction::kUint32)) {
        LOperand* value = UseRegister(val);
        LOperand* temp1 = TempRegister();
        LOperand* temp2 = FixedTemp(xmm1);
        LNumberTagU* result = new(zone()) LNumberTagU(value, temp1, temp2);
        return AssignPointerMap(DefineSameAsFirst(result));
      } else {
        LOperand* value = UseRegister(val);
        LOperand* temp1 = SmiValuesAre32Bits() ? NULL : TempRegister();
        LOperand* temp2 = SmiValuesAre32Bits() ? NULL : FixedTemp(xmm1);
        LNumberTagI* result = new(zone()) LNumberTagI(value, temp1, temp2);
        return AssignPointerMap(DefineSameAsFirst(result));
      }
    } else if (to.IsSmi()) {
      LOperand* value = UseRegister(val);
      LInstruction* result = DefineAsRegister(new(zone()) LSmiTag(value));
      if (instr->CheckFlag(HValue::kCanOverflow)) {
        result = AssignEnvironment(result);
      }
      return result;
    } else {
      DCHECK(to.IsDouble());
      if (val->CheckFlag(HInstruction::kUint32)) {
        return DefineAsRegister(new(zone()) LUint32ToDouble(UseRegister(val)));
      } else {
        LOperand* value = Use(val);
        return DefineAsRegister(new(zone()) LInteger32ToDouble(value));
      }
    }
  }
  UNREACHABLE();
  return NULL;
}


LInstruction* LChunkBuilder::DoCheckHeapObject(HCheckHeapObject* instr) {
  LOperand* value = UseRegisterAtStart(instr->value());
  LInstruction* result = new(zone()) LCheckNonSmi(value);
  if (!instr->value()->type().IsHeapObject()) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoCheckSmi(HCheckSmi* instr) {
  LOperand* value = UseRegisterAtStart(instr->value());
  return AssignEnvironment(new(zone()) LCheckSmi(value));
}


LInstruction* LChunkBuilder::DoCheckInstanceType(HCheckInstanceType* instr) {
  LOperand* value = UseRegisterAtStart(instr->value());
  LCheckInstanceType* result = new(zone()) LCheckInstanceType(value);
  return AssignEnvironment(result);
}


LInstruction* LChunkBuilder::DoCheckValue(HCheckValue* instr) {
  LOperand* value = UseRegisterAtStart(instr->value());
  return AssignEnvironment(new(zone()) LCheckValue(value));
}


LInstruction* LChunkBuilder::DoCheckMaps(HCheckMaps* instr) {
  if (instr->IsStabilityCheck()) return new(zone()) LCheckMaps;
  LOperand* value = UseRegisterAtStart(instr->value());
  LInstruction* result = AssignEnvironment(new(zone()) LCheckMaps(value));
  if (instr->HasMigrationTarget()) {
    info()->MarkAsDeferredCalling();
    result = AssignPointerMap(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoClampToUint8(HClampToUint8* instr) {
  HValue* value = instr->value();
  Representation input_rep = value->representation();
  LOperand* reg = UseRegister(value);
  if (input_rep.IsDouble()) {
    return DefineAsRegister(new(zone()) LClampDToUint8(reg));
  } else if (input_rep.IsInteger32()) {
    return DefineSameAsFirst(new(zone()) LClampIToUint8(reg));
  } else {
    DCHECK(input_rep.IsSmiOrTagged());
    // Register allocator doesn't (yet) support allocation of double
    // temps. Reserve xmm1 explicitly.
    LClampTToUint8* result = new(zone()) LClampTToUint8(reg,
                                                        FixedTemp(xmm1));
    return AssignEnvironment(DefineSameAsFirst(result));
  }
}


LInstruction* LChunkBuilder::DoDoubleBits(HDoubleBits* instr) {
  HValue* value = instr->value();
  DCHECK(value->representation().IsDouble());
  return DefineAsRegister(new(zone()) LDoubleBits(UseRegister(value)));
}


LInstruction* LChunkBuilder::DoConstructDouble(HConstructDouble* instr) {
  LOperand* lo = UseRegister(instr->lo());
  LOperand* hi = UseRegister(instr->hi());
  return DefineAsRegister(new(zone()) LConstructDouble(hi, lo));
}


LInstruction* LChunkBuilder::DoReturn(HReturn* instr) {
  LOperand* context = info()->IsStub() ? UseFixed(instr->context(), rsi) : NULL;
  LOperand* parameter_count = UseRegisterOrConstant(instr->parameter_count());
  return new(zone()) LReturn(
      UseFixed(instr->value(), rax), context, parameter_count);
}


LInstruction* LChunkBuilder::DoConstant(HConstant* instr) {
  Representation r = instr->representation();
  if (r.IsSmi()) {
    return DefineAsRegister(new(zone()) LConstantS);
  } else if (r.IsInteger32()) {
    return DefineAsRegister(new(zone()) LConstantI);
  } else if (r.IsDouble()) {
    LOperand* temp = TempRegister();
    return DefineAsRegister(new(zone()) LConstantD(temp));
  } else if (r.IsExternal()) {
    return DefineAsRegister(new(zone()) LConstantE);
  } else if (r.IsTagged()) {
    return DefineAsRegister(new(zone()) LConstantT);
  } else {
    UNREACHABLE();
    return NULL;
  }
}


LInstruction* LChunkBuilder::DoLoadGlobalCell(HLoadGlobalCell* instr) {
  LLoadGlobalCell* result = new(zone()) LLoadGlobalCell;
  return instr->RequiresHoleCheck()
      ? AssignEnvironment(DefineAsRegister(result))
      : DefineAsRegister(result);
}


LInstruction* LChunkBuilder::DoLoadGlobalGeneric(HLoadGlobalGeneric* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* global_object =
      UseFixed(instr->global_object(), LoadDescriptor::ReceiverRegister());
  LOperand* vector = NULL;
  if (FLAG_vector_ics) {
    vector = FixedTemp(VectorLoadICDescriptor::VectorRegister());
  }

  LLoadGlobalGeneric* result =
      new(zone()) LLoadGlobalGeneric(context, global_object, vector);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoStoreGlobalCell(HStoreGlobalCell* instr) {
  LOperand* value = UseRegister(instr->value());
  // Use a temp to avoid reloading the cell value address in the case where
  // we perform a hole check.
  return instr->RequiresHoleCheck()
      ? AssignEnvironment(new(zone()) LStoreGlobalCell(value, TempRegister()))
      : new(zone()) LStoreGlobalCell(value, NULL);
}


LInstruction* LChunkBuilder::DoLoadContextSlot(HLoadContextSlot* instr) {
  LOperand* context = UseRegisterAtStart(instr->value());
  LInstruction* result =
      DefineAsRegister(new(zone()) LLoadContextSlot(context));
  if (instr->RequiresHoleCheck() && instr->DeoptimizesOnHole()) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoStoreContextSlot(HStoreContextSlot* instr) {
  LOperand* context;
  LOperand* value;
  LOperand* temp;
  context = UseRegister(instr->context());
  if (instr->NeedsWriteBarrier()) {
    value = UseTempRegister(instr->value());
    temp = TempRegister();
  } else {
    value = UseRegister(instr->value());
    temp = NULL;
  }
  LInstruction* result = new(zone()) LStoreContextSlot(context, value, temp);
  if (instr->RequiresHoleCheck() && instr->DeoptimizesOnHole()) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoLoadNamedField(HLoadNamedField* instr) {
  // Use the special mov rax, moffs64 encoding for external
  // memory accesses with 64-bit word-sized values.
  if (instr->access().IsExternalMemory() &&
      instr->access().offset() == 0 &&
      (instr->access().representation().IsSmi() ||
       instr->access().representation().IsTagged() ||
       instr->access().representation().IsHeapObject() ||
       instr->access().representation().IsExternal())) {
    LOperand* obj = UseRegisterOrConstantAtStart(instr->object());
    return DefineFixed(new(zone()) LLoadNamedField(obj), rax);
  }
  LOperand* obj = UseRegisterAtStart(instr->object());
  return DefineAsRegister(new(zone()) LLoadNamedField(obj));
}


LInstruction* LChunkBuilder::DoLoadNamedGeneric(HLoadNamedGeneric* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* object =
      UseFixed(instr->object(), LoadDescriptor::ReceiverRegister());
  LOperand* vector = NULL;
  if (FLAG_vector_ics) {
    vector = FixedTemp(VectorLoadICDescriptor::VectorRegister());
  }
  LLoadNamedGeneric* result = new(zone()) LLoadNamedGeneric(
      context, object, vector);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoLoadFunctionPrototype(
    HLoadFunctionPrototype* instr) {
  return AssignEnvironment(DefineAsRegister(
      new(zone()) LLoadFunctionPrototype(UseRegister(instr->function()))));
}


LInstruction* LChunkBuilder::DoLoadRoot(HLoadRoot* instr) {
  return DefineAsRegister(new(zone()) LLoadRoot);
}


void LChunkBuilder::FindDehoistedKeyDefinitions(HValue* candidate) {
  // We sign extend the dehoisted key at the definition point when the pointer
  // size is 64-bit. For x32 port, we sign extend the dehoisted key at the use
  // points and should not invoke this function. We can't use STATIC_ASSERT
  // here as the pointer size is 32-bit for x32.
  DCHECK(kPointerSize == kInt64Size);
  BitVector* dehoisted_key_ids = chunk_->GetDehoistedKeyIds();
  if (dehoisted_key_ids->Contains(candidate->id())) return;
  dehoisted_key_ids->Add(candidate->id());
  if (!candidate->IsPhi()) return;
  for (int i = 0; i < candidate->OperandCount(); ++i) {
    FindDehoistedKeyDefinitions(candidate->OperandAt(i));
  }
}


LInstruction* LChunkBuilder::DoLoadKeyed(HLoadKeyed* instr) {
  DCHECK((kPointerSize == kInt64Size &&
          instr->key()->representation().IsInteger32()) ||
         (kPointerSize == kInt32Size &&
          instr->key()->representation().IsSmiOrInteger32()));
  ElementsKind elements_kind = instr->elements_kind();
  LOperand* key = NULL;
  LInstruction* result = NULL;

  if (kPointerSize == kInt64Size) {
    key = UseRegisterOrConstantAtStart(instr->key());
  } else {
    bool clobbers_key = ExternalArrayOpRequiresTemp(
        instr->key()->representation(), elements_kind);
    key = clobbers_key
        ? UseTempRegister(instr->key())
        : UseRegisterOrConstantAtStart(instr->key());
  }

  if ((kPointerSize == kInt64Size) && instr->IsDehoisted()) {
    FindDehoistedKeyDefinitions(instr->key());
  }

  if (!instr->is_typed_elements()) {
    LOperand* obj = UseRegisterAtStart(instr->elements());
    result = DefineAsRegister(new(zone()) LLoadKeyed(obj, key));
  } else {
    DCHECK(
        (instr->representation().IsInteger32() &&
         !(IsDoubleOrFloatElementsKind(elements_kind))) ||
        (instr->representation().IsDouble() &&
         (IsDoubleOrFloatElementsKind(elements_kind))));
    LOperand* backing_store = UseRegister(instr->elements());
    result = DefineAsRegister(new(zone()) LLoadKeyed(backing_store, key));
  }

  if ((instr->is_external() || instr->is_fixed_typed_array()) ?
      // see LCodeGen::DoLoadKeyedExternalArray
      ((elements_kind == EXTERNAL_UINT32_ELEMENTS ||
        elements_kind == UINT32_ELEMENTS) &&
       !instr->CheckFlag(HInstruction::kUint32)) :
      // see LCodeGen::DoLoadKeyedFixedDoubleArray and
      // LCodeGen::DoLoadKeyedFixedArray
      instr->RequiresHoleCheck()) {
    result = AssignEnvironment(result);
  }
  return result;
}


LInstruction* LChunkBuilder::DoLoadKeyedGeneric(HLoadKeyedGeneric* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* object =
      UseFixed(instr->object(), LoadDescriptor::ReceiverRegister());
  LOperand* key = UseFixed(instr->key(), LoadDescriptor::NameRegister());
  LOperand* vector = NULL;
  if (FLAG_vector_ics) {
    vector = FixedTemp(VectorLoadICDescriptor::VectorRegister());
  }

  LLoadKeyedGeneric* result =
      new(zone()) LLoadKeyedGeneric(context, object, key, vector);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoStoreKeyed(HStoreKeyed* instr) {
  ElementsKind elements_kind = instr->elements_kind();

  if ((kPointerSize == kInt64Size) && instr->IsDehoisted()) {
    FindDehoistedKeyDefinitions(instr->key());
  }

  if (!instr->is_typed_elements()) {
    DCHECK(instr->elements()->representation().IsTagged());
    bool needs_write_barrier = instr->NeedsWriteBarrier();
    LOperand* object = NULL;
    LOperand* key = NULL;
    LOperand* val = NULL;

    Representation value_representation = instr->value()->representation();
    if (value_representation.IsDouble()) {
      object = UseRegisterAtStart(instr->elements());
      val = UseRegisterAtStart(instr->value());
      key = UseRegisterOrConstantAtStart(instr->key());
    } else {
      DCHECK(value_representation.IsSmiOrTagged() ||
             value_representation.IsInteger32());
      if (needs_write_barrier) {
        object = UseTempRegister(instr->elements());
        val = UseTempRegister(instr->value());
        key = UseTempRegister(instr->key());
      } else {
        object = UseRegisterAtStart(instr->elements());
        val = UseRegisterOrConstantAtStart(instr->value());
        key = UseRegisterOrConstantAtStart(instr->key());
      }
    }

    return new(zone()) LStoreKeyed(object, key, val);
  }

  DCHECK(
       (instr->value()->representation().IsInteger32() &&
       !IsDoubleOrFloatElementsKind(elements_kind)) ||
       (instr->value()->representation().IsDouble() &&
       IsDoubleOrFloatElementsKind(elements_kind)));
  DCHECK((instr->is_fixed_typed_array() &&
          instr->elements()->representation().IsTagged()) ||
         (instr->is_external() &&
          instr->elements()->representation().IsExternal()));
  bool val_is_temp_register =
      elements_kind == EXTERNAL_UINT8_CLAMPED_ELEMENTS ||
      elements_kind == EXTERNAL_FLOAT32_ELEMENTS ||
      elements_kind == FLOAT32_ELEMENTS;
  LOperand* val = val_is_temp_register ? UseTempRegister(instr->value())
      : UseRegister(instr->value());
  LOperand* key = NULL;
  if (kPointerSize == kInt64Size) {
    key = UseRegisterOrConstantAtStart(instr->key());
  } else {
    bool clobbers_key = ExternalArrayOpRequiresTemp(
        instr->key()->representation(), elements_kind);
    key = clobbers_key
        ? UseTempRegister(instr->key())
        : UseRegisterOrConstantAtStart(instr->key());
  }
  LOperand* backing_store = UseRegister(instr->elements());
  return new(zone()) LStoreKeyed(backing_store, key, val);
}


LInstruction* LChunkBuilder::DoStoreKeyedGeneric(HStoreKeyedGeneric* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* object =
      UseFixed(instr->object(), StoreDescriptor::ReceiverRegister());
  LOperand* key = UseFixed(instr->key(), StoreDescriptor::NameRegister());
  LOperand* value = UseFixed(instr->value(), StoreDescriptor::ValueRegister());

  DCHECK(instr->object()->representation().IsTagged());
  DCHECK(instr->key()->representation().IsTagged());
  DCHECK(instr->value()->representation().IsTagged());

  LStoreKeyedGeneric* result =
      new(zone()) LStoreKeyedGeneric(context, object, key, value);
  return MarkAsCall(result, instr);
}


LInstruction* LChunkBuilder::DoTransitionElementsKind(
    HTransitionElementsKind* instr) {
  if (IsSimpleMapChangeTransition(instr->from_kind(), instr->to_kind())) {
    LOperand* object = UseRegister(instr->object());
    LOperand* new_map_reg = TempRegister();
    LOperand* temp_reg = TempRegister();
    LTransitionElementsKind* result = new(zone()) LTransitionElementsKind(
        object, NULL, new_map_reg, temp_reg);
    return result;
  } else {
    LOperand* object = UseFixed(instr->object(), rax);
    LOperand* context = UseFixed(instr->context(), rsi);
    LTransitionElementsKind* result =
        new(zone()) LTransitionElementsKind(object, context, NULL, NULL);
    return MarkAsCall(result, instr);
  }
}


LInstruction* LChunkBuilder::DoTrapAllocationMemento(
    HTrapAllocationMemento* instr) {
  LOperand* object = UseRegister(instr->object());
  LOperand* temp = TempRegister();
  LTrapAllocationMemento* result =
      new(zone()) LTrapAllocationMemento(object, temp);
  return AssignEnvironment(result);
}


LInstruction* LChunkBuilder::DoStoreNamedField(HStoreNamedField* instr) {
  bool is_in_object = instr->access().IsInobject();
  bool is_external_location = instr->access().IsExternalMemory() &&
      instr->access().offset() == 0;
  bool needs_write_barrier = instr->NeedsWriteBarrier();
  bool needs_write_barrier_for_map = instr->has_transition() &&
      instr->NeedsWriteBarrierForMap();

  LOperand* obj;
  if (needs_write_barrier) {
    obj = is_in_object
        ? UseRegister(instr->object())
        : UseTempRegister(instr->object());
  } else if (is_external_location) {
    DCHECK(!is_in_object);
    DCHECK(!needs_write_barrier);
    DCHECK(!needs_write_barrier_for_map);
    obj = UseRegisterOrConstant(instr->object());
  } else {
    obj = needs_write_barrier_for_map
        ? UseRegister(instr->object())
        : UseRegisterAtStart(instr->object());
  }

  bool can_be_constant = instr->value()->IsConstant() &&
      HConstant::cast(instr->value())->NotInNewSpace() &&
      !instr->field_representation().IsDouble();

  LOperand* val;
  if (needs_write_barrier) {
    val = UseTempRegister(instr->value());
  } else if (is_external_location) {
    val = UseFixed(instr->value(), rax);
  } else if (can_be_constant) {
    val = UseRegisterOrConstant(instr->value());
  } else if (instr->field_representation().IsDouble()) {
    val = UseRegisterAtStart(instr->value());
  } else {
    val = UseRegister(instr->value());
  }

  // We only need a scratch register if we have a write barrier or we
  // have a store into the properties array (not in-object-property).
  LOperand* temp = (!is_in_object || needs_write_barrier ||
      needs_write_barrier_for_map) ? TempRegister() : NULL;

  return new(zone()) LStoreNamedField(obj, val, temp);
}


LInstruction* LChunkBuilder::DoStoreNamedGeneric(HStoreNamedGeneric* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* object =
      UseFixed(instr->object(), StoreDescriptor::ReceiverRegister());
  LOperand* value = UseFixed(instr->value(), StoreDescriptor::ValueRegister());

  LStoreNamedGeneric* result =
      new(zone()) LStoreNamedGeneric(context, object, value);
  return MarkAsCall(result, instr);
}


LInstruction* LChunkBuilder::DoStringAdd(HStringAdd* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* left = UseFixed(instr->left(), rdx);
  LOperand* right = UseFixed(instr->right(), rax);
  return MarkAsCall(
      DefineFixed(new(zone()) LStringAdd(context, left, right), rax), instr);
}


LInstruction* LChunkBuilder::DoStringCharCodeAt(HStringCharCodeAt* instr) {
  LOperand* string = UseTempRegister(instr->string());
  LOperand* index = UseTempRegister(instr->index());
  LOperand* context = UseAny(instr->context());
  LStringCharCodeAt* result =
      new(zone()) LStringCharCodeAt(context, string, index);
  return AssignPointerMap(DefineAsRegister(result));
}


LInstruction* LChunkBuilder::DoStringCharFromCode(HStringCharFromCode* instr) {
  LOperand* char_code = UseRegister(instr->value());
  LOperand* context = UseAny(instr->context());
  LStringCharFromCode* result =
      new(zone()) LStringCharFromCode(context, char_code);
  return AssignPointerMap(DefineAsRegister(result));
}


LInstruction* LChunkBuilder::DoAllocate(HAllocate* instr) {
  info()->MarkAsDeferredCalling();
  LOperand* context = UseAny(instr->context());
  LOperand* size = instr->size()->IsConstant()
      ? UseConstant(instr->size())
      : UseTempRegister(instr->size());
  LOperand* temp = TempRegister();
  LAllocate* result = new(zone()) LAllocate(context, size, temp);
  return AssignPointerMap(DefineAsRegister(result));
}


LInstruction* LChunkBuilder::DoRegExpLiteral(HRegExpLiteral* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LRegExpLiteral* result = new(zone()) LRegExpLiteral(context);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoFunctionLiteral(HFunctionLiteral* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LFunctionLiteral* result = new(zone()) LFunctionLiteral(context);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoOsrEntry(HOsrEntry* instr) {
  DCHECK(argument_count_ == 0);
  allocator_->MarkAsOsrEntry();
  current_block_->last_environment()->set_ast_id(instr->ast_id());
  return AssignEnvironment(new(zone()) LOsrEntry);
}


LInstruction* LChunkBuilder::DoParameter(HParameter* instr) {
  LParameter* result = new(zone()) LParameter;
  if (instr->kind() == HParameter::STACK_PARAMETER) {
    int spill_index = chunk()->GetParameterStackSlot(instr->index());
    return DefineAsSpilled(result, spill_index);
  } else {
    DCHECK(info()->IsStub());
    CallInterfaceDescriptor descriptor =
        info()->code_stub()->GetCallInterfaceDescriptor();
    int index = static_cast<int>(instr->index());
    Register reg = descriptor.GetEnvironmentParameterRegister(index);
    return DefineFixed(result, reg);
  }
}


LInstruction* LChunkBuilder::DoUnknownOSRValue(HUnknownOSRValue* instr) {
  // Use an index that corresponds to the location in the unoptimized frame,
  // which the optimized frame will subsume.
  int env_index = instr->index();
  int spill_index = 0;
  if (instr->environment()->is_parameter_index(env_index)) {
    spill_index = chunk()->GetParameterStackSlot(env_index);
  } else {
    spill_index = env_index - instr->environment()->first_local_index();
    if (spill_index > LUnallocated::kMaxFixedSlotIndex) {
      Retry(kTooManySpillSlotsNeededForOSR);
      spill_index = 0;
    }
  }
  return DefineAsSpilled(new(zone()) LUnknownOSRValue, spill_index);
}


LInstruction* LChunkBuilder::DoCallStub(HCallStub* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LCallStub* result = new(zone()) LCallStub(context);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoArgumentsObject(HArgumentsObject* instr) {
  // There are no real uses of the arguments object.
  // arguments.length and element access are supported directly on
  // stack arguments, and any real arguments object use causes a bailout.
  // So this value is never used.
  return NULL;
}


LInstruction* LChunkBuilder::DoCapturedObject(HCapturedObject* instr) {
  instr->ReplayEnvironment(current_block_->last_environment());

  // There are no real uses of a captured object.
  return NULL;
}


LInstruction* LChunkBuilder::DoAccessArgumentsAt(HAccessArgumentsAt* instr) {
  info()->MarkAsRequiresFrame();
  LOperand* args = UseRegister(instr->arguments());
  LOperand* length;
  LOperand* index;
  if (instr->length()->IsConstant() && instr->index()->IsConstant()) {
    length = UseRegisterOrConstant(instr->length());
    index = UseOrConstant(instr->index());
  } else {
    length = UseTempRegister(instr->length());
    index = Use(instr->index());
  }
  return DefineAsRegister(new(zone()) LAccessArgumentsAt(args, length, index));
}


LInstruction* LChunkBuilder::DoToFastProperties(HToFastProperties* instr) {
  LOperand* object = UseFixed(instr->value(), rax);
  LToFastProperties* result = new(zone()) LToFastProperties(object);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoTypeof(HTypeof* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* value = UseAtStart(instr->value());
  LTypeof* result = new(zone()) LTypeof(context, value);
  return MarkAsCall(DefineFixed(result, rax), instr);
}


LInstruction* LChunkBuilder::DoTypeofIsAndBranch(HTypeofIsAndBranch* instr) {
  return new(zone()) LTypeofIsAndBranch(UseTempRegister(instr->value()));
}


LInstruction* LChunkBuilder::DoIsConstructCallAndBranch(
    HIsConstructCallAndBranch* instr) {
  return new(zone()) LIsConstructCallAndBranch(TempRegister());
}


LInstruction* LChunkBuilder::DoSimulate(HSimulate* instr) {
  instr->ReplayEnvironment(current_block_->last_environment());
  return NULL;
}


LInstruction* LChunkBuilder::DoStackCheck(HStackCheck* instr) {
  info()->MarkAsDeferredCalling();
  if (instr->is_function_entry()) {
    LOperand* context = UseFixed(instr->context(), rsi);
    return MarkAsCall(new(zone()) LStackCheck(context), instr);
  } else {
    DCHECK(instr->is_backwards_branch());
    LOperand* context = UseAny(instr->context());
    return AssignEnvironment(
        AssignPointerMap(new(zone()) LStackCheck(context)));
  }
}


LInstruction* LChunkBuilder::DoEnterInlined(HEnterInlined* instr) {
  HEnvironment* outer = current_block_->last_environment();
  outer->set_ast_id(instr->ReturnId());
  HConstant* undefined = graph()->GetConstantUndefined();
  HEnvironment* inner = outer->CopyForInlining(instr->closure(),
                                               instr->arguments_count(),
                                               instr->function(),
                                               undefined,
                                               instr->inlining_kind());
  // Only replay binding of arguments object if it wasn't removed from graph.
  if (instr->arguments_var() != NULL && instr->arguments_object()->IsLinked()) {
    inner->Bind(instr->arguments_var(), instr->arguments_object());
  }
  inner->BindContext(instr->closure_context());
  inner->set_entry(instr);
  current_block_->UpdateEnvironment(inner);
  chunk_->AddInlinedClosure(instr->closure());
  return NULL;
}


LInstruction* LChunkBuilder::DoLeaveInlined(HLeaveInlined* instr) {
  LInstruction* pop = NULL;

  HEnvironment* env = current_block_->last_environment();

  if (env->entry()->arguments_pushed()) {
    int argument_count = env->arguments_environment()->parameter_count();
    pop = new(zone()) LDrop(argument_count);
    DCHECK(instr->argument_delta() == -argument_count);
  }

  HEnvironment* outer = current_block_->last_environment()->
      DiscardInlined(false);
  current_block_->UpdateEnvironment(outer);

  return pop;
}


LInstruction* LChunkBuilder::DoForInPrepareMap(HForInPrepareMap* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* object = UseFixed(instr->enumerable(), rax);
  LForInPrepareMap* result = new(zone()) LForInPrepareMap(context, object);
  return MarkAsCall(DefineFixed(result, rax), instr, CAN_DEOPTIMIZE_EAGERLY);
}


LInstruction* LChunkBuilder::DoForInCacheArray(HForInCacheArray* instr) {
  LOperand* map = UseRegister(instr->map());
  return AssignEnvironment(DefineAsRegister(
      new(zone()) LForInCacheArray(map)));
}


LInstruction* LChunkBuilder::DoCheckMapValue(HCheckMapValue* instr) {
  LOperand* value = UseRegisterAtStart(instr->value());
  LOperand* map = UseRegisterAtStart(instr->map());
  return AssignEnvironment(new(zone()) LCheckMapValue(value, map));
}


LInstruction* LChunkBuilder::DoLoadFieldByIndex(HLoadFieldByIndex* instr) {
  LOperand* object = UseRegister(instr->object());
  LOperand* index = UseTempRegister(instr->index());
  LLoadFieldByIndex* load = new(zone()) LLoadFieldByIndex(object, index);
  LInstruction* result = DefineSameAsFirst(load);
  return AssignPointerMap(result);
}


LInstruction* LChunkBuilder::DoStoreFrameContext(HStoreFrameContext* instr) {
  LOperand* context = UseRegisterAtStart(instr->context());
  return new(zone()) LStoreFrameContext(context);
}


LInstruction* LChunkBuilder::DoAllocateBlockContext(
    HAllocateBlockContext* instr) {
  LOperand* context = UseFixed(instr->context(), rsi);
  LOperand* function = UseRegisterAtStart(instr->function());
  LAllocateBlockContext* result =
      new(zone()) LAllocateBlockContext(context, function);
  return MarkAsCall(DefineFixed(result, rsi), instr);
}


} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_X64
