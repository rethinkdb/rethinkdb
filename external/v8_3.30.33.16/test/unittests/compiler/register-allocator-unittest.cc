// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/utils/random-number-generator.h"
#include "src/compiler/register-allocator.h"
#include "test/unittests/test-utils.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace v8 {
namespace internal {
namespace compiler {

typedef BasicBlock::RpoNumber Rpo;

namespace {

static const char*
    general_register_names_[RegisterConfiguration::kMaxGeneralRegisters];
static const char*
    double_register_names_[RegisterConfiguration::kMaxDoubleRegisters];
static char register_names_[10 * (RegisterConfiguration::kMaxGeneralRegisters +
                                  RegisterConfiguration::kMaxDoubleRegisters)];


static void InitializeRegisterNames() {
  char* loc = register_names_;
  for (int i = 0; i < RegisterConfiguration::kMaxGeneralRegisters; ++i) {
    general_register_names_[i] = loc;
    loc += base::OS::SNPrintF(loc, 100, "gp_%d", i);
    *loc++ = 0;
  }
  for (int i = 0; i < RegisterConfiguration::kMaxDoubleRegisters; ++i) {
    double_register_names_[i] = loc;
    loc += base::OS::SNPrintF(loc, 100, "fp_%d", i) + 1;
    *loc++ = 0;
  }
}

}  // namespace


// TODO(dcarney): fake opcodes.
// TODO(dcarney): fix printing of sequence w.r.t fake opcodes and registers.
class RegisterAllocatorTest : public TestWithZone {
 public:
  static const int kDefaultNRegs = 4;

  RegisterAllocatorTest()
      : num_general_registers_(kDefaultNRegs),
        num_double_registers_(kDefaultNRegs),
        basic_blocks_(zone()),
        instruction_blocks_(zone()),
        current_block_(NULL) {
    InitializeRegisterNames();
  }

  RegisterConfiguration* config() {
    if (config_.is_empty()) {
      config_.Reset(new RegisterConfiguration(
          num_general_registers_, num_double_registers_, num_double_registers_,
          general_register_names_, double_register_names_));
    }
    return config_.get();
  }

  Frame* frame() {
    if (frame_.is_empty()) {
      frame_.Reset(new Frame());
    }
    return frame_.get();
  }

  InstructionSequence* sequence() {
    if (sequence_.is_empty()) {
      sequence_.Reset(new InstructionSequence(zone(), &instruction_blocks_));
    }
    return sequence_.get();
  }

  RegisterAllocator* allocator() {
    if (allocator_.is_empty()) {
      allocator_.Reset(
          new RegisterAllocator(config(), zone(), frame(), sequence()));
    }
    return allocator_.get();
  }

  InstructionBlock* StartBlock(Rpo loop_header = Rpo::Invalid(),
                               Rpo loop_end = Rpo::Invalid()) {
    CHECK(current_block_ == NULL);
    BasicBlock::Id block_id =
        BasicBlock::Id::FromSize(instruction_blocks_.size());
    BasicBlock* basic_block = new (zone()) BasicBlock(zone(), block_id);
    basic_block->set_rpo_number(block_id.ToInt());
    basic_block->set_ao_number(block_id.ToInt());
    if (loop_header.IsValid()) {
      basic_block->set_loop_depth(1);
      basic_block->set_loop_header(basic_blocks_[loop_header.ToSize()]);
      basic_block->set_loop_end(loop_end.ToInt());
    }
    InstructionBlock* instruction_block =
        new (zone()) InstructionBlock(zone(), basic_block);
    basic_blocks_.push_back(basic_block);
    instruction_blocks_.push_back(instruction_block);
    current_block_ = instruction_block;
    sequence()->StartBlock(basic_block);
    return instruction_block;
  }

  void EndBlock() {
    CHECK(current_block_ != NULL);
    sequence()->EndBlock(basic_blocks_[current_block_->rpo_number().ToSize()]);
    current_block_ = NULL;
  }

  void Allocate() {
    if (FLAG_trace_alloc) {
      OFStream os(stdout);
      PrintableInstructionSequence printable = {config(), sequence()};
      os << "Before: " << std::endl << printable << std::endl;
    }
    allocator()->Allocate();
    if (FLAG_trace_alloc) {
      OFStream os(stdout);
      PrintableInstructionSequence printable = {config(), sequence()};
      os << "After: " << std::endl << printable << std::endl;
    }
  }

  int NewReg() { return sequence()->NextVirtualRegister(); }

  int Parameter() {
    // TODO(dcarney): assert parameters before other instructions.
    int vreg = NewReg();
    InstructionOperand* outputs[1]{
        Unallocated(UnallocatedOperand::MUST_HAVE_REGISTER, vreg)};
    sequence()->AddInstruction(
        Instruction::New(zone(), kArchNop, 1, outputs, 0, NULL, 0, NULL));
    return vreg;
  }

  void Return(int vreg) {
    InstructionOperand* inputs[1]{
        Unallocated(UnallocatedOperand::MUST_HAVE_REGISTER, vreg)};
    sequence()->AddInstruction(
        Instruction::New(zone(), kArchNop, 0, NULL, 1, inputs, 0, NULL));
  }

  Instruction* Emit(int output_vreg, int input_vreg_0, int input_vreg_1) {
    InstructionOperand* outputs[1]{
        Unallocated(UnallocatedOperand::MUST_HAVE_REGISTER, output_vreg)};
    InstructionOperand* inputs[2]{
        Unallocated(UnallocatedOperand::MUST_HAVE_REGISTER, input_vreg_0),
        Unallocated(UnallocatedOperand::MUST_HAVE_REGISTER, input_vreg_1)};
    Instruction* instruction =
        Instruction::New(zone(), kArchNop, 1, outputs, 2, inputs, 0, NULL);
    sequence()->AddInstruction(instruction);
    return instruction;
  }

 private:
  InstructionOperand* Unallocated(UnallocatedOperand::ExtendedPolicy policy,
                                  int vreg) {
    UnallocatedOperand* op =
        new (zone()) UnallocatedOperand(UnallocatedOperand::MUST_HAVE_REGISTER);
    op->set_virtual_register(vreg);
    return op;
  }

  int num_general_registers_;
  int num_double_registers_;
  SmartPointer<RegisterConfiguration> config_;
  ZoneVector<BasicBlock*> basic_blocks_;
  InstructionBlocks instruction_blocks_;
  InstructionBlock* current_block_;
  SmartPointer<Frame> frame_;
  SmartPointer<RegisterAllocator> allocator_;
  SmartPointer<InstructionSequence> sequence_;
};


TEST_F(RegisterAllocatorTest, CanAllocateThreeRegisters) {
  StartBlock();
  int a_reg = Parameter();
  int b_reg = Parameter();
  int c_reg = NewReg();
  Instruction* res = Emit(c_reg, a_reg, b_reg);
  Return(c_reg);
  EndBlock();

  Allocate();

  ASSERT_TRUE(res->OutputAt(0)->IsRegister());
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
