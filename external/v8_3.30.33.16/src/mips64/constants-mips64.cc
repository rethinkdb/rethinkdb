// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#if V8_TARGET_ARCH_MIPS64

#include "src/mips64/constants-mips64.h"

namespace v8 {
namespace internal {


// -----------------------------------------------------------------------------
// Registers.


// These register names are defined in a way to match the native disassembler
// formatting. See for example the command "objdump -d <binary file>".
const char* Registers::names_[kNumSimuRegisters] = {
  "zero_reg",
  "at",
  "v0", "v1",
  "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
  "t0", "t1", "t2", "t3",
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
  "t8", "t9",
  "k0", "k1",
  "gp",
  "sp",
  "fp",
  "ra",
  "LO", "HI",
  "pc"
};


// List of alias names which can be used when referring to MIPS registers.
const Registers::RegisterAlias Registers::aliases_[] = {
  {0, "zero"},
  {23, "cp"},
  {30, "s8"},
  {30, "s8_fp"},
  {kInvalidRegister, NULL}
};


const char* Registers::Name(int reg) {
  const char* result;
  if ((0 <= reg) && (reg < kNumSimuRegisters)) {
    result = names_[reg];
  } else {
    result = "noreg";
  }
  return result;
}


int Registers::Number(const char* name) {
  // Look through the canonical names.
  for (int i = 0; i < kNumSimuRegisters; i++) {
    if (strcmp(names_[i], name) == 0) {
      return i;
    }
  }

  // Look through the alias names.
  int i = 0;
  while (aliases_[i].reg != kInvalidRegister) {
    if (strcmp(aliases_[i].name, name) == 0) {
      return aliases_[i].reg;
    }
    i++;
  }

  // No register with the reguested name found.
  return kInvalidRegister;
}


const char* FPURegisters::names_[kNumFPURegisters] = {
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11",
  "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19", "f20", "f21",
  "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
};


// List of alias names which can be used when referring to MIPS registers.
const FPURegisters::RegisterAlias FPURegisters::aliases_[] = {
  {kInvalidRegister, NULL}
};


const char* FPURegisters::Name(int creg) {
  const char* result;
  if ((0 <= creg) && (creg < kNumFPURegisters)) {
    result = names_[creg];
  } else {
    result = "nocreg";
  }
  return result;
}


int FPURegisters::Number(const char* name) {
  // Look through the canonical names.
  for (int i = 0; i < kNumFPURegisters; i++) {
    if (strcmp(names_[i], name) == 0) {
      return i;
    }
  }

  // Look through the alias names.
  int i = 0;
  while (aliases_[i].creg != kInvalidRegister) {
    if (strcmp(aliases_[i].name, name) == 0) {
      return aliases_[i].creg;
    }
    i++;
  }

  // No Cregister with the reguested name found.
  return kInvalidFPURegister;
}


// -----------------------------------------------------------------------------
// Instructions.

bool Instruction::IsForbiddenInBranchDelay() const {
  const int op = OpcodeFieldRaw();
  switch (op) {
    case J:
    case JAL:
    case BEQ:
    case BNE:
    case BLEZ:
    case BGTZ:
    case BEQL:
    case BNEL:
    case BLEZL:
    case BGTZL:
      return true;
    case REGIMM:
      switch (RtFieldRaw()) {
        case BLTZ:
        case BGEZ:
        case BLTZAL:
        case BGEZAL:
          return true;
        default:
          return false;
      }
      break;
    case SPECIAL:
      switch (FunctionFieldRaw()) {
        case JR:
        case JALR:
          return true;
        default:
          return false;
      }
      break;
    default:
      return false;
  }
}


bool Instruction::IsLinkingInstruction() const {
  const int op = OpcodeFieldRaw();
  switch (op) {
    case JAL:
      return true;
    case REGIMM:
      switch (RtFieldRaw()) {
        case BGEZAL:
        case BLTZAL:
          return true;
      default:
        return false;
      }
    case SPECIAL:
      switch (FunctionFieldRaw()) {
        case JALR:
          return true;
        default:
          return false;
      }
    default:
      return false;
  }
}


bool Instruction::IsTrap() const {
  if (OpcodeFieldRaw() != SPECIAL) {
    return false;
  } else {
    switch (FunctionFieldRaw()) {
      case BREAK:
      case TGE:
      case TGEU:
      case TLT:
      case TLTU:
      case TEQ:
      case TNE:
        return true;
      default:
        return false;
    }
  }
}


Instruction::Type Instruction::InstructionType() const {
  switch (OpcodeFieldRaw()) {
    case SPECIAL:
      switch (FunctionFieldRaw()) {
        case JR:
        case JALR:
        case BREAK:
        case SLL:
        case DSLL:
        case DSLL32:
        case SRL:
        case DSRL:
        case DSRL32:
        case SRA:
        case DSRA:
        case DSRA32:
        case SLLV:
        case DSLLV:
        case SRLV:
        case DSRLV:
        case SRAV:
        case DSRAV:
        case MFHI:
        case MFLO:
        case MULT:
        case DMULT:
        case MULTU:
        case DMULTU:
        case DIV:
        case DDIV:
        case DIVU:
        case DDIVU:
        case ADD:
        case DADD:
        case ADDU:
        case DADDU:
        case SUB:
        case DSUB:
        case SUBU:
        case DSUBU:
        case AND:
        case OR:
        case XOR:
        case NOR:
        case SLT:
        case SLTU:
        case TGE:
        case TGEU:
        case TLT:
        case TLTU:
        case TEQ:
        case TNE:
        case MOVZ:
        case MOVN:
        case MOVCI:
          return kRegisterType;
        default:
          return kUnsupported;
      }
      break;
    case SPECIAL2:
      switch (FunctionFieldRaw()) {
        case MUL:
        case CLZ:
          return kRegisterType;
        default:
          return kUnsupported;
      }
      break;
    case SPECIAL3:
      switch (FunctionFieldRaw()) {
        case INS:
        case EXT:
          return kRegisterType;
        default:
          return kUnsupported;
      }
      break;
    case COP1:    // Coprocessor instructions.
      switch (RsFieldRawNoAssert()) {
        case BC1:   // Branch on coprocessor condition.
        case BC1EQZ:
        case BC1NEZ:
          return kImmediateType;
        default:
          return kRegisterType;
      }
      break;
    case COP1X:
      return kRegisterType;
    // 16 bits Immediate type instructions. e.g.: addi dest, src, imm16.
    case REGIMM:
    case BEQ:
    case BNE:
    case BLEZ:
    case BGTZ:
    case ADDI:
    case DADDI:
    case ADDIU:
    case DADDIU:
    case SLTI:
    case SLTIU:
    case ANDI:
    case ORI:
    case XORI:
    case LUI:
    case BEQL:
    case BNEL:
    case BLEZL:
    case BGTZL:
    case BEQZC:
    case BNEZC:
    case LB:
    case LH:
    case LWL:
    case LW:
    case LWU:
    case LD:
    case LBU:
    case LHU:
    case LWR:
    case SB:
    case SH:
    case SWL:
    case SW:
    case SD:
    case SWR:
    case LWC1:
    case LDC1:
    case SWC1:
    case SDC1:
      return kImmediateType;
    // 26 bits immediate type instructions. e.g.: j imm26.
    case J:
    case JAL:
      return kJumpType;
    default:
      return kUnsupported;
  }
  return kUnsupported;
}


} }   // namespace v8::internal

#endif  // V8_TARGET_ARCH_MIPS64
