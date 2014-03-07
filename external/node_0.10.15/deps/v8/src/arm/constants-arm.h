// Copyright 2011 the V8 project authors. All rights reserved.
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

#ifndef V8_ARM_CONSTANTS_ARM_H_
#define V8_ARM_CONSTANTS_ARM_H_

// ARM EABI is required.
#if defined(__arm__) && !defined(__ARM_EABI__)
#error ARM EABI support is required.
#endif

// This means that interwork-compatible jump instructions are generated.  We
// want to generate them on the simulator too so it makes snapshots that can
// be used on real hardware.
#if defined(__THUMB_INTERWORK__) || !defined(__arm__)
# define USE_THUMB_INTERWORK 1
#endif

#if defined(__ARM_ARCH_7A__) || \
    defined(__ARM_ARCH_7R__) || \
    defined(__ARM_ARCH_7__)
# define CAN_USE_ARMV7_INSTRUCTIONS 1
#endif

#if defined(__ARM_ARCH_6__) ||   \
    defined(__ARM_ARCH_6J__) ||  \
    defined(__ARM_ARCH_6K__) ||  \
    defined(__ARM_ARCH_6Z__) ||  \
    defined(__ARM_ARCH_6ZK__) || \
    defined(__ARM_ARCH_6T2__) || \
    defined(CAN_USE_ARMV7_INSTRUCTIONS)
# define CAN_USE_ARMV6_INSTRUCTIONS 1
#endif

#if defined(__ARM_ARCH_5T__)             || \
    defined(__ARM_ARCH_5TE__)            || \
    defined(__ARM_ARCH_5TEJ__)           || \
    defined(CAN_USE_ARMV6_INSTRUCTIONS)
# define CAN_USE_ARMV5_INSTRUCTIONS 1
# define CAN_USE_THUMB_INSTRUCTIONS 1
#endif

// Simulator should support ARM5 instructions and unaligned access by default.
#if !defined(__arm__)
# define CAN_USE_ARMV5_INSTRUCTIONS 1
# define CAN_USE_THUMB_INSTRUCTIONS 1

# ifndef CAN_USE_UNALIGNED_ACCESSES
#  define CAN_USE_UNALIGNED_ACCESSES 1
# endif

#endif

// Using blx may yield better code, so use it when required or when available
#if defined(USE_THUMB_INTERWORK) || defined(CAN_USE_ARMV5_INSTRUCTIONS)
#define USE_BLX 1
#endif

namespace v8 {
namespace internal {

// Constant pool marker.
const int kConstantPoolMarkerMask = 0xffe00000;
const int kConstantPoolMarker = 0x0c000000;
const int kConstantPoolLengthMask = 0x001ffff;

// Number of registers in normal ARM mode.
const int kNumRegisters = 16;

// VFP support.
const int kNumVFPSingleRegisters = 32;
const int kNumVFPDoubleRegisters = 16;
const int kNumVFPRegisters = kNumVFPSingleRegisters + kNumVFPDoubleRegisters;

// PC is register 15.
const int kPCRegister = 15;
const int kNoRegister = -1;

// -----------------------------------------------------------------------------
// Conditions.

// Defines constants and accessor classes to assemble, disassemble and
// simulate ARM instructions.
//
// Section references in the code refer to the "ARM Architecture Reference
// Manual" from July 2005 (available at http://www.arm.com/miscPDFs/14128.pdf)
//
// Constants for specific fields are defined in their respective named enums.
// General constants are in an anonymous enum in class Instr.

// Values for the condition field as defined in section A3.2
enum Condition {
  kNoCondition = -1,

  eq =  0 << 28,                 // Z set            Equal.
  ne =  1 << 28,                 // Z clear          Not equal.
  cs =  2 << 28,                 // C set            Unsigned higher or same.
  cc =  3 << 28,                 // C clear          Unsigned lower.
  mi =  4 << 28,                 // N set            Negative.
  pl =  5 << 28,                 // N clear          Positive or zero.
  vs =  6 << 28,                 // V set            Overflow.
  vc =  7 << 28,                 // V clear          No overflow.
  hi =  8 << 28,                 // C set, Z clear   Unsigned higher.
  ls =  9 << 28,                 // C clear or Z set Unsigned lower or same.
  ge = 10 << 28,                 // N == V           Greater or equal.
  lt = 11 << 28,                 // N != V           Less than.
  gt = 12 << 28,                 // Z clear, N == V  Greater than.
  le = 13 << 28,                 // Z set or N != V  Less then or equal
  al = 14 << 28,                 //                  Always.

  kSpecialCondition = 15 << 28,  // Special condition (refer to section A3.2.1).
  kNumberOfConditions = 16,

  // Aliases.
  hs = cs,                       // C set            Unsigned higher or same.
  lo = cc                        // C clear          Unsigned lower.
};


inline Condition NegateCondition(Condition cond) {
  ASSERT(cond != al);
  return static_cast<Condition>(cond ^ ne);
}


// Corresponds to transposing the operands of a comparison.
inline Condition ReverseCondition(Condition cond) {
  switch (cond) {
    case lo:
      return hi;
    case hi:
      return lo;
    case hs:
      return ls;
    case ls:
      return hs;
    case lt:
      return gt;
    case gt:
      return lt;
    case ge:
      return le;
    case le:
      return ge;
    default:
      return cond;
  };
}


// -----------------------------------------------------------------------------
// Instructions encoding.

// Instr is merely used by the Assembler to distinguish 32bit integers
// representing instructions from usual 32 bit values.
// Instruction objects are pointers to 32bit values, and provide methods to
// access the various ISA fields.
typedef int32_t Instr;


// Opcodes for Data-processing instructions (instructions with a type 0 and 1)
// as defined in section A3.4
enum Opcode {
  AND =  0 << 21,  // Logical AND.
  EOR =  1 << 21,  // Logical Exclusive OR.
  SUB =  2 << 21,  // Subtract.
  RSB =  3 << 21,  // Reverse Subtract.
  ADD =  4 << 21,  // Add.
  ADC =  5 << 21,  // Add with Carry.
  SBC =  6 << 21,  // Subtract with Carry.
  RSC =  7 << 21,  // Reverse Subtract with Carry.
  TST =  8 << 21,  // Test.
  TEQ =  9 << 21,  // Test Equivalence.
  CMP = 10 << 21,  // Compare.
  CMN = 11 << 21,  // Compare Negated.
  ORR = 12 << 21,  // Logical (inclusive) OR.
  MOV = 13 << 21,  // Move.
  BIC = 14 << 21,  // Bit Clear.
  MVN = 15 << 21   // Move Not.
};


// The bits for bit 7-4 for some type 0 miscellaneous instructions.
enum MiscInstructionsBits74 {
  // With bits 22-21 01.
  BX   =  1 << 4,
  BXJ  =  2 << 4,
  BLX  =  3 << 4,
  BKPT =  7 << 4,

  // With bits 22-21 11.
  CLZ  =  1 << 4
};


// Instruction encoding bits and masks.
enum {
  H   = 1 << 5,   // Halfword (or byte).
  S6  = 1 << 6,   // Signed (or unsigned).
  L   = 1 << 20,  // Load (or store).
  S   = 1 << 20,  // Set condition code (or leave unchanged).
  W   = 1 << 21,  // Writeback base register (or leave unchanged).
  A   = 1 << 21,  // Accumulate in multiply instruction (or not).
  B   = 1 << 22,  // Unsigned byte (or word).
  N   = 1 << 22,  // Long (or short).
  U   = 1 << 23,  // Positive (or negative) offset/index.
  P   = 1 << 24,  // Offset/pre-indexed addressing (or post-indexed addressing).
  I   = 1 << 25,  // Immediate shifter operand (or not).

  B4  = 1 << 4,
  B5  = 1 << 5,
  B6  = 1 << 6,
  B7  = 1 << 7,
  B8  = 1 << 8,
  B9  = 1 << 9,
  B12 = 1 << 12,
  B16 = 1 << 16,
  B18 = 1 << 18,
  B19 = 1 << 19,
  B20 = 1 << 20,
  B21 = 1 << 21,
  B22 = 1 << 22,
  B23 = 1 << 23,
  B24 = 1 << 24,
  B25 = 1 << 25,
  B26 = 1 << 26,
  B27 = 1 << 27,
  B28 = 1 << 28,

  // Instruction bit masks.
  kCondMask   = 15 << 28,
  kALUMask    = 0x6f << 21,
  kRdMask     = 15 << 12,  // In str instruction.
  kCoprocessorMask = 15 << 8,
  kOpCodeMask = 15 << 21,  // In data-processing instructions.
  kImm24Mask  = (1 << 24) - 1,
  kOff12Mask  = (1 << 12) - 1
};


// -----------------------------------------------------------------------------
// Addressing modes and instruction variants.

// Condition code updating mode.
enum SBit {
  SetCC   = 1 << 20,  // Set condition code.
  LeaveCC = 0 << 20   // Leave condition code unchanged.
};


// Status register selection.
enum SRegister {
  CPSR = 0 << 22,
  SPSR = 1 << 22
};


// Shifter types for Data-processing operands as defined in section A5.1.2.
enum ShiftOp {
  LSL = 0 << 5,   // Logical shift left.
  LSR = 1 << 5,   // Logical shift right.
  ASR = 2 << 5,   // Arithmetic shift right.
  ROR = 3 << 5,   // Rotate right.

  // RRX is encoded as ROR with shift_imm == 0.
  // Use a special code to make the distinction. The RRX ShiftOp is only used
  // as an argument, and will never actually be encoded. The Assembler will
  // detect it and emit the correct ROR shift operand with shift_imm == 0.
  RRX = -1,
  kNumberOfShifts = 4
};


// Status register fields.
enum SRegisterField {
  CPSR_c = CPSR | 1 << 16,
  CPSR_x = CPSR | 1 << 17,
  CPSR_s = CPSR | 1 << 18,
  CPSR_f = CPSR | 1 << 19,
  SPSR_c = SPSR | 1 << 16,
  SPSR_x = SPSR | 1 << 17,
  SPSR_s = SPSR | 1 << 18,
  SPSR_f = SPSR | 1 << 19
};

// Status register field mask (or'ed SRegisterField enum values).
typedef uint32_t SRegisterFieldMask;


// Memory operand addressing mode.
enum AddrMode {
  // Bit encoding P U W.
  Offset       = (8|4|0) << 21,  // Offset (without writeback to base).
  PreIndex     = (8|4|1) << 21,  // Pre-indexed addressing with writeback.
  PostIndex    = (0|4|0) << 21,  // Post-indexed addressing with writeback.
  NegOffset    = (8|0|0) << 21,  // Negative offset (without writeback to base).
  NegPreIndex  = (8|0|1) << 21,  // Negative pre-indexed with writeback.
  NegPostIndex = (0|0|0) << 21   // Negative post-indexed with writeback.
};


// Load/store multiple addressing mode.
enum BlockAddrMode {
  // Bit encoding P U W .
  da           = (0|0|0) << 21,  // Decrement after.
  ia           = (0|4|0) << 21,  // Increment after.
  db           = (8|0|0) << 21,  // Decrement before.
  ib           = (8|4|0) << 21,  // Increment before.
  da_w         = (0|0|1) << 21,  // Decrement after with writeback to base.
  ia_w         = (0|4|1) << 21,  // Increment after with writeback to base.
  db_w         = (8|0|1) << 21,  // Decrement before with writeback to base.
  ib_w         = (8|4|1) << 21,  // Increment before with writeback to base.

  // Alias modes for comparison when writeback does not matter.
  da_x         = (0|0|0) << 21,  // Decrement after.
  ia_x         = (0|4|0) << 21,  // Increment after.
  db_x         = (8|0|0) << 21,  // Decrement before.
  ib_x         = (8|4|0) << 21,  // Increment before.

  kBlockAddrModeMask = (8|4|1) << 21
};


// Coprocessor load/store operand size.
enum LFlag {
  Long  = 1 << 22,  // Long load/store coprocessor.
  Short = 0 << 22   // Short load/store coprocessor.
};


// -----------------------------------------------------------------------------
// Supervisor Call (svc) specific support.

// Special Software Interrupt codes when used in the presence of the ARM
// simulator.
// svc (formerly swi) provides a 24bit immediate value. Use bits 22:0 for
// standard SoftwareInterrupCode. Bit 23 is reserved for the stop feature.
enum SoftwareInterruptCodes {
  // transition to C code
  kCallRtRedirected= 0x10,
  // break point
  kBreakpoint= 0x20,
  // stop
  kStopCode = 1 << 23
};
const uint32_t kStopCodeMask = kStopCode - 1;
const uint32_t kMaxStopCode = kStopCode - 1;
const int32_t  kDefaultStopCode = -1;


// Type of VFP register. Determines register encoding.
enum VFPRegPrecision {
  kSinglePrecision = 0,
  kDoublePrecision = 1
};


// VFP FPSCR constants.
enum VFPConversionMode {
  kFPSCRRounding = 0,
  kDefaultRoundToZero = 1
};

// This mask does not include the "inexact" or "input denormal" cumulative
// exceptions flags, because we usually don't want to check for it.
const uint32_t kVFPExceptionMask = 0xf;
const uint32_t kVFPInvalidOpExceptionBit = 1 << 0;
const uint32_t kVFPOverflowExceptionBit = 1 << 2;
const uint32_t kVFPUnderflowExceptionBit = 1 << 3;
const uint32_t kVFPInexactExceptionBit = 1 << 4;
const uint32_t kVFPFlushToZeroMask = 1 << 24;

const uint32_t kVFPNConditionFlagBit = 1 << 31;
const uint32_t kVFPZConditionFlagBit = 1 << 30;
const uint32_t kVFPCConditionFlagBit = 1 << 29;
const uint32_t kVFPVConditionFlagBit = 1 << 28;


// VFP rounding modes. See ARM DDI 0406B Page A2-29.
enum VFPRoundingMode {
  RN = 0 << 22,   // Round to Nearest.
  RP = 1 << 22,   // Round towards Plus Infinity.
  RM = 2 << 22,   // Round towards Minus Infinity.
  RZ = 3 << 22,   // Round towards zero.

  // Aliases.
  kRoundToNearest = RN,
  kRoundToPlusInf = RP,
  kRoundToMinusInf = RM,
  kRoundToZero = RZ
};

const uint32_t kVFPRoundingModeMask = 3 << 22;

enum CheckForInexactConversion {
  kCheckForInexactConversion,
  kDontCheckForInexactConversion
};

// -----------------------------------------------------------------------------
// Hints.

// Branch hints are not used on the ARM.  They are defined so that they can
// appear in shared function signatures, but will be ignored in ARM
// implementations.
enum Hint { no_hint };

// Hints are not used on the arm.  Negating is trivial.
inline Hint NegateHint(Hint ignored) { return no_hint; }


// -----------------------------------------------------------------------------
// Specific instructions, constants, and masks.
// These constants are declared in assembler-arm.cc, as they use named registers
// and other constants.


// add(sp, sp, 4) instruction (aka Pop())
extern const Instr kPopInstruction;

// str(r, MemOperand(sp, 4, NegPreIndex), al) instruction (aka push(r))
// register r is not encoded.
extern const Instr kPushRegPattern;

// ldr(r, MemOperand(sp, 4, PostIndex), al) instruction (aka pop(r))
// register r is not encoded.
extern const Instr kPopRegPattern;

// mov lr, pc
extern const Instr kMovLrPc;
// ldr rd, [pc, #offset]
extern const Instr kLdrPCMask;
extern const Instr kLdrPCPattern;
// blxcc rm
extern const Instr kBlxRegMask;

extern const Instr kBlxRegPattern;

extern const Instr kMovMvnMask;
extern const Instr kMovMvnPattern;
extern const Instr kMovMvnFlip;
extern const Instr kMovLeaveCCMask;
extern const Instr kMovLeaveCCPattern;
extern const Instr kMovwMask;
extern const Instr kMovwPattern;
extern const Instr kMovwLeaveCCFlip;
extern const Instr kCmpCmnMask;
extern const Instr kCmpCmnPattern;
extern const Instr kCmpCmnFlip;
extern const Instr kAddSubFlip;
extern const Instr kAndBicFlip;

// A mask for the Rd register for push, pop, ldr, str instructions.
extern const Instr kLdrRegFpOffsetPattern;

extern const Instr kStrRegFpOffsetPattern;

extern const Instr kLdrRegFpNegOffsetPattern;

extern const Instr kStrRegFpNegOffsetPattern;

extern const Instr kLdrStrInstrTypeMask;
extern const Instr kLdrStrInstrArgumentMask;
extern const Instr kLdrStrOffsetMask;


// -----------------------------------------------------------------------------
// Instruction abstraction.

// The class Instruction enables access to individual fields defined in the ARM
// architecture instruction set encoding as described in figure A3-1.
// Note that the Assembler uses typedef int32_t Instr.
//
// Example: Test whether the instruction at ptr does set the condition code
// bits.
//
// bool InstructionSetsConditionCodes(byte* ptr) {
//   Instruction* instr = Instruction::At(ptr);
//   int type = instr->TypeValue();
//   return ((type == 0) || (type == 1)) && instr->HasS();
// }
//
class Instruction {
 public:
  enum {
    kInstrSize = 4,
    kInstrSizeLog2 = 2,
    kPCReadOffset = 8
  };

  // Helper macro to define static accessors.
  // We use the cast to char* trick to bypass the strict anti-aliasing rules.
  #define DECLARE_STATIC_TYPED_ACCESSOR(return_type, Name)                     \
    static inline return_type Name(Instr instr) {                              \
      char* temp = reinterpret_cast<char*>(&instr);                            \
      return reinterpret_cast<Instruction*>(temp)->Name();                     \
    }

  #define DECLARE_STATIC_ACCESSOR(Name) DECLARE_STATIC_TYPED_ACCESSOR(int, Name)

  // Get the raw instruction bits.
  inline Instr InstructionBits() const {
    return *reinterpret_cast<const Instr*>(this);
  }

  // Set the raw instruction bits to value.
  inline void SetInstructionBits(Instr value) {
    *reinterpret_cast<Instr*>(this) = value;
  }

  // Read one particular bit out of the instruction bits.
  inline int Bit(int nr) const {
    return (InstructionBits() >> nr) & 1;
  }

  // Read a bit field's value out of the instruction bits.
  inline int Bits(int hi, int lo) const {
    return (InstructionBits() >> lo) & ((2 << (hi - lo)) - 1);
  }

  // Read a bit field out of the instruction bits.
  inline int BitField(int hi, int lo) const {
    return InstructionBits() & (((2 << (hi - lo)) - 1) << lo);
  }

  // Static support.

  // Read one particular bit out of the instruction bits.
  static inline int Bit(Instr instr, int nr) {
    return (instr >> nr) & 1;
  }

  // Read the value of a bit field out of the instruction bits.
  static inline int Bits(Instr instr, int hi, int lo) {
    return (instr >> lo) & ((2 << (hi - lo)) - 1);
  }


  // Read a bit field out of the instruction bits.
  static inline int BitField(Instr instr, int hi, int lo) {
    return instr & (((2 << (hi - lo)) - 1) << lo);
  }


  // Accessors for the different named fields used in the ARM encoding.
  // The naming of these accessor corresponds to figure A3-1.
  //
  // Two kind of accessors are declared:
  // - <Name>Field() will return the raw field, i.e. the field's bits at their
  //   original place in the instruction encoding.
  //   e.g. if instr is the 'addgt r0, r1, r2' instruction, encoded as
  //   0xC0810002 ConditionField(instr) will return 0xC0000000.
  // - <Name>Value() will return the field value, shifted back to bit 0.
  //   e.g. if instr is the 'addgt r0, r1, r2' instruction, encoded as
  //   0xC0810002 ConditionField(instr) will return 0xC.


  // Generally applicable fields
  inline Condition ConditionValue() const {
    return static_cast<Condition>(Bits(31, 28));
  }
  inline Condition ConditionField() const {
    return static_cast<Condition>(BitField(31, 28));
  }
  DECLARE_STATIC_TYPED_ACCESSOR(Condition, ConditionValue);
  DECLARE_STATIC_TYPED_ACCESSOR(Condition, ConditionField);

  inline int TypeValue() const { return Bits(27, 25); }

  inline int RnValue() const { return Bits(19, 16); }
  DECLARE_STATIC_ACCESSOR(RnValue);
  inline int RdValue() const { return Bits(15, 12); }
  DECLARE_STATIC_ACCESSOR(RdValue);

  inline int CoprocessorValue() const { return Bits(11, 8); }
  // Support for VFP.
  // Vn(19-16) | Vd(15-12) |  Vm(3-0)
  inline int VnValue() const { return Bits(19, 16); }
  inline int VmValue() const { return Bits(3, 0); }
  inline int VdValue() const { return Bits(15, 12); }
  inline int NValue() const { return Bit(7); }
  inline int MValue() const { return Bit(5); }
  inline int DValue() const { return Bit(22); }
  inline int RtValue() const { return Bits(15, 12); }
  inline int PValue() const { return Bit(24); }
  inline int UValue() const { return Bit(23); }
  inline int Opc1Value() const { return (Bit(23) << 2) | Bits(21, 20); }
  inline int Opc2Value() const { return Bits(19, 16); }
  inline int Opc3Value() const { return Bits(7, 6); }
  inline int SzValue() const { return Bit(8); }
  inline int VLValue() const { return Bit(20); }
  inline int VCValue() const { return Bit(8); }
  inline int VAValue() const { return Bits(23, 21); }
  inline int VBValue() const { return Bits(6, 5); }
  inline int VFPNRegValue(VFPRegPrecision pre) {
    return VFPGlueRegValue(pre, 16, 7);
  }
  inline int VFPMRegValue(VFPRegPrecision pre) {
    return VFPGlueRegValue(pre, 0, 5);
  }
  inline int VFPDRegValue(VFPRegPrecision pre) {
    return VFPGlueRegValue(pre, 12, 22);
  }

  // Fields used in Data processing instructions
  inline int OpcodeValue() const {
    return static_cast<Opcode>(Bits(24, 21));
  }
  inline Opcode OpcodeField() const {
    return static_cast<Opcode>(BitField(24, 21));
  }
  inline int SValue() const { return Bit(20); }
    // with register
  inline int RmValue() const { return Bits(3, 0); }
  DECLARE_STATIC_ACCESSOR(RmValue);
  inline int ShiftValue() const { return static_cast<ShiftOp>(Bits(6, 5)); }
  inline ShiftOp ShiftField() const {
    return static_cast<ShiftOp>(BitField(6, 5));
  }
  inline int RegShiftValue() const { return Bit(4); }
  inline int RsValue() const { return Bits(11, 8); }
  inline int ShiftAmountValue() const { return Bits(11, 7); }
    // with immediate
  inline int RotateValue() const { return Bits(11, 8); }
  inline int Immed8Value() const { return Bits(7, 0); }
  inline int Immed4Value() const { return Bits(19, 16); }
  inline int ImmedMovwMovtValue() const {
      return Immed4Value() << 12 | Offset12Value(); }

  // Fields used in Load/Store instructions
  inline int PUValue() const { return Bits(24, 23); }
  inline int PUField() const { return BitField(24, 23); }
  inline int  BValue() const { return Bit(22); }
  inline int  WValue() const { return Bit(21); }
  inline int  LValue() const { return Bit(20); }
    // with register uses same fields as Data processing instructions above
    // with immediate
  inline int Offset12Value() const { return Bits(11, 0); }
    // multiple
  inline int RlistValue() const { return Bits(15, 0); }
    // extra loads and stores
  inline int SignValue() const { return Bit(6); }
  inline int HValue() const { return Bit(5); }
  inline int ImmedHValue() const { return Bits(11, 8); }
  inline int ImmedLValue() const { return Bits(3, 0); }

  // Fields used in Branch instructions
  inline int LinkValue() const { return Bit(24); }
  inline int SImmed24Value() const { return ((InstructionBits() << 8) >> 8); }

  // Fields used in Software interrupt instructions
  inline SoftwareInterruptCodes SvcValue() const {
    return static_cast<SoftwareInterruptCodes>(Bits(23, 0));
  }

  // Test for special encodings of type 0 instructions (extra loads and stores,
  // as well as multiplications).
  inline bool IsSpecialType0() const { return (Bit(7) == 1) && (Bit(4) == 1); }

  // Test for miscellaneous instructions encodings of type 0 instructions.
  inline bool IsMiscType0() const { return (Bit(24) == 1)
                                           && (Bit(23) == 0)
                                           && (Bit(20) == 0)
                                           && ((Bit(7) == 0)); }

  // Test for a nop instruction, which falls under type 1.
  inline bool IsNopType1() const { return Bits(24, 0) == 0x0120F000; }

  // Test for a stop instruction.
  inline bool IsStop() const {
    return (TypeValue() == 7) && (Bit(24) == 1) && (SvcValue() >= kStopCode);
  }

  // Special accessors that test for existence of a value.
  inline bool HasS()    const { return SValue() == 1; }
  inline bool HasB()    const { return BValue() == 1; }
  inline bool HasW()    const { return WValue() == 1; }
  inline bool HasL()    const { return LValue() == 1; }
  inline bool HasU()    const { return UValue() == 1; }
  inline bool HasSign() const { return SignValue() == 1; }
  inline bool HasH()    const { return HValue() == 1; }
  inline bool HasLink() const { return LinkValue() == 1; }

  // Decoding the double immediate in the vmov instruction.
  double DoubleImmedVmov() const;

  // Instructions are read of out a code stream. The only way to get a
  // reference to an instruction is to convert a pointer. There is no way
  // to allocate or create instances of class Instruction.
  // Use the At(pc) function to create references to Instruction.
  static Instruction* At(byte* pc) {
    return reinterpret_cast<Instruction*>(pc);
  }


 private:
  // Join split register codes, depending on single or double precision.
  // four_bit is the position of the least-significant bit of the four
  // bit specifier. one_bit is the position of the additional single bit
  // specifier.
  inline int VFPGlueRegValue(VFPRegPrecision pre, int four_bit, int one_bit) {
    if (pre == kSinglePrecision) {
      return (Bits(four_bit + 3, four_bit) << 1) | Bit(one_bit);
    }
    return (Bit(one_bit) << 4) | Bits(four_bit + 3, four_bit);
  }

  // We need to prevent the creation of instances of class Instruction.
  DISALLOW_IMPLICIT_CONSTRUCTORS(Instruction);
};


// Helper functions for converting between register numbers and names.
class Registers {
 public:
  // Return the name of the register.
  static const char* Name(int reg);

  // Lookup the register number for the name provided.
  static int Number(const char* name);

  struct RegisterAlias {
    int reg;
    const char* name;
  };

 private:
  static const char* names_[kNumRegisters];
  static const RegisterAlias aliases_[];
};

// Helper functions for converting between VFP register numbers and names.
class VFPRegisters {
 public:
  // Return the name of the register.
  static const char* Name(int reg, bool is_double);

  // Lookup the register number for the name provided.
  // Set flag pointed by is_double to true if register
  // is double-precision.
  static int Number(const char* name, bool* is_double);

 private:
  static const char* names_[kNumVFPRegisters];
};


} }  // namespace v8::internal

#endif  // V8_ARM_CONSTANTS_ARM_H_
