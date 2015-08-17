// Copyright (c) 1994-2006 Sun Microsystems Inc.
// All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// - Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// - Redistribution in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// - Neither the name of Sun Microsystems or the names of contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// The original source code covered by the above license above has been
// modified significantly by Google Inc.
// Copyright 2011 the V8 project authors. All rights reserved.

// A light-weight IA32 Assembler.

#ifndef V8_X87_ASSEMBLER_X87_H_
#define V8_X87_ASSEMBLER_X87_H_

#include "src/isolate.h"
#include "src/serialize.h"

namespace v8 {
namespace internal {

// CPU Registers.
//
// 1) We would prefer to use an enum, but enum values are assignment-
// compatible with int, which has caused code-generation bugs.
//
// 2) We would prefer to use a class instead of a struct but we don't like
// the register initialization to depend on the particular initialization
// order (which appears to be different on OS X, Linux, and Windows for the
// installed versions of C++ we tried). Using a struct permits C-style
// "initialization". Also, the Register objects cannot be const as this
// forces initialization stubs in MSVC, making us dependent on initialization
// order.
//
// 3) By not using an enum, we are possibly preventing the compiler from
// doing certain constant folds, which may significantly reduce the
// code generated for some assembly instructions (because they boil down
// to a few constants). If this is a problem, we could change the code
// such that we use an enum in optimized mode, and the struct in debug
// mode. This way we get the compile-time error checking in debug mode
// and best performance in optimized code.
//
struct Register {
  static const int kMaxNumAllocatableRegisters = 6;
  static int NumAllocatableRegisters() {
    return kMaxNumAllocatableRegisters;
  }
  static const int kNumRegisters = 8;

  static inline const char* AllocationIndexToString(int index);

  static inline int ToAllocationIndex(Register reg);

  static inline Register FromAllocationIndex(int index);

  static Register from_code(int code) {
    DCHECK(code >= 0);
    DCHECK(code < kNumRegisters);
    Register r = { code };
    return r;
  }
  bool is_valid() const { return 0 <= code_ && code_ < kNumRegisters; }
  bool is(Register reg) const { return code_ == reg.code_; }
  // eax, ebx, ecx and edx are byte registers, the rest are not.
  bool is_byte_register() const { return code_ <= 3; }
  int code() const {
    DCHECK(is_valid());
    return code_;
  }
  int bit() const {
    DCHECK(is_valid());
    return 1 << code_;
  }

  // Unfortunately we can't make this private in a struct.
  int code_;
};

const int kRegister_eax_Code = 0;
const int kRegister_ecx_Code = 1;
const int kRegister_edx_Code = 2;
const int kRegister_ebx_Code = 3;
const int kRegister_esp_Code = 4;
const int kRegister_ebp_Code = 5;
const int kRegister_esi_Code = 6;
const int kRegister_edi_Code = 7;
const int kRegister_no_reg_Code = -1;

const Register eax = { kRegister_eax_Code };
const Register ecx = { kRegister_ecx_Code };
const Register edx = { kRegister_edx_Code };
const Register ebx = { kRegister_ebx_Code };
const Register esp = { kRegister_esp_Code };
const Register ebp = { kRegister_ebp_Code };
const Register esi = { kRegister_esi_Code };
const Register edi = { kRegister_edi_Code };
const Register no_reg = { kRegister_no_reg_Code };


inline const char* Register::AllocationIndexToString(int index) {
  DCHECK(index >= 0 && index < kMaxNumAllocatableRegisters);
  // This is the mapping of allocation indices to registers.
  const char* const kNames[] = { "eax", "ecx", "edx", "ebx", "esi", "edi" };
  return kNames[index];
}


inline int Register::ToAllocationIndex(Register reg) {
  DCHECK(reg.is_valid() && !reg.is(esp) && !reg.is(ebp));
  return (reg.code() >= 6) ? reg.code() - 2 : reg.code();
}


inline Register Register::FromAllocationIndex(int index)  {
  DCHECK(index >= 0 && index < kMaxNumAllocatableRegisters);
  return (index >= 4) ? from_code(index + 2) : from_code(index);
}


struct X87Register {
  static const int kMaxNumAllocatableRegisters = 6;
  static const int kMaxNumRegisters = 8;
  static int NumAllocatableRegisters() {
    return kMaxNumAllocatableRegisters;
  }


  // TODO(turbofan): Proper support for float32.
  static int NumAllocatableAliasedRegisters() {
    return NumAllocatableRegisters();
  }


  static int ToAllocationIndex(X87Register reg) {
    return reg.code_;
  }

  static const char* AllocationIndexToString(int index) {
    DCHECK(index >= 0 && index < kMaxNumAllocatableRegisters);
    const char* const names[] = {
      "stX_0", "stX_1", "stX_2", "stX_3", "stX_4",
      "stX_5", "stX_6", "stX_7"
    };
    return names[index];
  }

  static X87Register FromAllocationIndex(int index) {
    DCHECK(index >= 0 && index < kMaxNumAllocatableRegisters);
    X87Register result;
    result.code_ = index;
    return result;
  }

  bool is_valid() const {
    return 0 <= code_ && code_ < kMaxNumRegisters;
  }

  int code() const {
    DCHECK(is_valid());
    return code_;
  }

  bool is(X87Register reg) const {
    return code_ == reg.code_;
  }

  int code_;
};


typedef X87Register DoubleRegister;


const X87Register stX_0 = { 0 };
const X87Register stX_1 = { 1 };
const X87Register stX_2 = { 2 };
const X87Register stX_3 = { 3 };
const X87Register stX_4 = { 4 };
const X87Register stX_5 = { 5 };
const X87Register stX_6 = { 6 };
const X87Register stX_7 = { 7 };


enum Condition {
  // any value < 0 is considered no_condition
  no_condition  = -1,

  overflow      =  0,
  no_overflow   =  1,
  below         =  2,
  above_equal   =  3,
  equal         =  4,
  not_equal     =  5,
  below_equal   =  6,
  above         =  7,
  negative      =  8,
  positive      =  9,
  parity_even   = 10,
  parity_odd    = 11,
  less          = 12,
  greater_equal = 13,
  less_equal    = 14,
  greater       = 15,

  // aliases
  carry         = below,
  not_carry     = above_equal,
  zero          = equal,
  not_zero      = not_equal,
  sign          = negative,
  not_sign      = positive
};


// Returns the equivalent of !cc.
// Negation of the default no_condition (-1) results in a non-default
// no_condition value (-2). As long as tests for no_condition check
// for condition < 0, this will work as expected.
inline Condition NegateCondition(Condition cc) {
  return static_cast<Condition>(cc ^ 1);
}


// Commute a condition such that {a cond b == b cond' a}.
inline Condition CommuteCondition(Condition cc) {
  switch (cc) {
    case below:
      return above;
    case above:
      return below;
    case above_equal:
      return below_equal;
    case below_equal:
      return above_equal;
    case less:
      return greater;
    case greater:
      return less;
    case greater_equal:
      return less_equal;
    case less_equal:
      return greater_equal;
    default:
      return cc;
  }
}


// -----------------------------------------------------------------------------
// Machine instruction Immediates

class Immediate BASE_EMBEDDED {
 public:
  inline explicit Immediate(int x);
  inline explicit Immediate(const ExternalReference& ext);
  inline explicit Immediate(Handle<Object> handle);
  inline explicit Immediate(Smi* value);
  inline explicit Immediate(Address addr);

  static Immediate CodeRelativeOffset(Label* label) {
    return Immediate(label);
  }

  bool is_zero() const { return x_ == 0 && RelocInfo::IsNone(rmode_); }
  bool is_int8() const {
    return -128 <= x_ && x_ < 128 && RelocInfo::IsNone(rmode_);
  }
  bool is_int16() const {
    return -32768 <= x_ && x_ < 32768 && RelocInfo::IsNone(rmode_);
  }

 private:
  inline explicit Immediate(Label* value);

  int x_;
  RelocInfo::Mode rmode_;

  friend class Operand;
  friend class Assembler;
  friend class MacroAssembler;
};


// -----------------------------------------------------------------------------
// Machine instruction Operands

enum ScaleFactor {
  times_1 = 0,
  times_2 = 1,
  times_4 = 2,
  times_8 = 3,
  times_int_size = times_4,
  times_half_pointer_size = times_2,
  times_pointer_size = times_4,
  times_twice_pointer_size = times_8
};


class Operand BASE_EMBEDDED {
 public:
  // reg
  INLINE(explicit Operand(Register reg));

  // [disp/r]
  INLINE(explicit Operand(int32_t disp, RelocInfo::Mode rmode));

  // [disp/r]
  INLINE(explicit Operand(Immediate imm));

  // [base + disp/r]
  explicit Operand(Register base, int32_t disp,
                   RelocInfo::Mode rmode = RelocInfo::NONE32);

  // [base + index*scale + disp/r]
  explicit Operand(Register base,
                   Register index,
                   ScaleFactor scale,
                   int32_t disp,
                   RelocInfo::Mode rmode = RelocInfo::NONE32);

  // [index*scale + disp/r]
  explicit Operand(Register index,
                   ScaleFactor scale,
                   int32_t disp,
                   RelocInfo::Mode rmode = RelocInfo::NONE32);

  static Operand StaticVariable(const ExternalReference& ext) {
    return Operand(reinterpret_cast<int32_t>(ext.address()),
                   RelocInfo::EXTERNAL_REFERENCE);
  }

  static Operand StaticArray(Register index,
                             ScaleFactor scale,
                             const ExternalReference& arr) {
    return Operand(index, scale, reinterpret_cast<int32_t>(arr.address()),
                   RelocInfo::EXTERNAL_REFERENCE);
  }

  static Operand ForCell(Handle<Cell> cell) {
    AllowDeferredHandleDereference embedding_raw_address;
    return Operand(reinterpret_cast<int32_t>(cell.location()),
                   RelocInfo::CELL);
  }

  static Operand ForRegisterPlusImmediate(Register base, Immediate imm) {
    return Operand(base, imm.x_, imm.rmode_);
  }

  // Returns true if this Operand is a wrapper for the specified register.
  bool is_reg(Register reg) const;

  // Returns true if this Operand is a wrapper for one register.
  bool is_reg_only() const;

  // Asserts that this Operand is a wrapper for one register and returns the
  // register.
  Register reg() const;

 private:
  // Set the ModRM byte without an encoded 'reg' register. The
  // register is encoded later as part of the emit_operand operation.
  inline void set_modrm(int mod, Register rm);

  inline void set_sib(ScaleFactor scale, Register index, Register base);
  inline void set_disp8(int8_t disp);
  inline void set_dispr(int32_t disp, RelocInfo::Mode rmode);

  byte buf_[6];
  // The number of bytes in buf_.
  unsigned int len_;
  // Only valid if len_ > 4.
  RelocInfo::Mode rmode_;

  friend class Assembler;
  friend class MacroAssembler;
};


// -----------------------------------------------------------------------------
// A Displacement describes the 32bit immediate field of an instruction which
// may be used together with a Label in order to refer to a yet unknown code
// position. Displacements stored in the instruction stream are used to describe
// the instruction and to chain a list of instructions using the same Label.
// A Displacement contains 2 different fields:
//
// next field: position of next displacement in the chain (0 = end of list)
// type field: instruction type
//
// A next value of null (0) indicates the end of a chain (note that there can
// be no displacement at position zero, because there is always at least one
// instruction byte before the displacement).
//
// Displacement _data field layout
//
// |31.....2|1......0|
// [  next  |  type  |

class Displacement BASE_EMBEDDED {
 public:
  enum Type {
    UNCONDITIONAL_JUMP,
    CODE_RELATIVE,
    OTHER
  };

  int data() const { return data_; }
  Type type() const { return TypeField::decode(data_); }
  void next(Label* L) const {
    int n = NextField::decode(data_);
    n > 0 ? L->link_to(n) : L->Unuse();
  }
  void link_to(Label* L) { init(L, type()); }

  explicit Displacement(int data) { data_ = data; }

  Displacement(Label* L, Type type) { init(L, type); }

  void print() {
    PrintF("%s (%x) ", (type() == UNCONDITIONAL_JUMP ? "jmp" : "[other]"),
                       NextField::decode(data_));
  }

 private:
  int data_;

  class TypeField: public BitField<Type, 0, 2> {};
  class NextField: public BitField<int,  2, 32-2> {};

  void init(Label* L, Type type);
};


class Assembler : public AssemblerBase {
 private:
  // We check before assembling an instruction that there is sufficient
  // space to write an instruction and its relocation information.
  // The relocation writer's position must be kGap bytes above the end of
  // the generated instructions. This leaves enough space for the
  // longest possible ia32 instruction, 15 bytes, and the longest possible
  // relocation information encoding, RelocInfoWriter::kMaxLength == 16.
  // (There is a 15 byte limit on ia32 instruction length that rules out some
  // otherwise valid instructions.)
  // This allows for a single, fast space check per instruction.
  static const int kGap = 32;

 public:
  // Create an assembler. Instructions and relocation information are emitted
  // into a buffer, with the instructions starting from the beginning and the
  // relocation information starting from the end of the buffer. See CodeDesc
  // for a detailed comment on the layout (globals.h).
  //
  // If the provided buffer is NULL, the assembler allocates and grows its own
  // buffer, and buffer_size determines the initial buffer size. The buffer is
  // owned by the assembler and deallocated upon destruction of the assembler.
  //
  // If the provided buffer is not NULL, the assembler uses the provided buffer
  // for code generation and assumes its size to be buffer_size. If the buffer
  // is too small, a fatal error occurs. No deallocation of the buffer is done
  // upon destruction of the assembler.
  // TODO(vitalyr): the assembler does not need an isolate.
  Assembler(Isolate* isolate, void* buffer, int buffer_size);
  virtual ~Assembler() { }

  // GetCode emits any pending (non-emitted) code and fills the descriptor
  // desc. GetCode() is idempotent; it returns the same result if no other
  // Assembler functions are invoked in between GetCode() calls.
  void GetCode(CodeDesc* desc);

  // Read/Modify the code target in the branch/call instruction at pc.
  inline static Address target_address_at(Address pc,
                                          ConstantPoolArray* constant_pool);
  inline static void set_target_address_at(Address pc,
                                           ConstantPoolArray* constant_pool,
                                           Address target,
                                           ICacheFlushMode icache_flush_mode =
                                               FLUSH_ICACHE_IF_NEEDED);
  static inline Address target_address_at(Address pc, Code* code) {
    ConstantPoolArray* constant_pool = code ? code->constant_pool() : NULL;
    return target_address_at(pc, constant_pool);
  }
  static inline void set_target_address_at(Address pc,
                                           Code* code,
                                           Address target,
                                           ICacheFlushMode icache_flush_mode =
                                               FLUSH_ICACHE_IF_NEEDED) {
    ConstantPoolArray* constant_pool = code ? code->constant_pool() : NULL;
    set_target_address_at(pc, constant_pool, target);
  }

  // Return the code target address at a call site from the return address
  // of that call in the instruction stream.
  inline static Address target_address_from_return_address(Address pc);

  // Return the code target address of the patch debug break slot
  inline static Address break_address_from_return_address(Address pc);

  // This sets the branch destination (which is in the instruction on x86).
  // This is for calls and branches within generated code.
  inline static void deserialization_set_special_target_at(
      Address instruction_payload, Code* code, Address target) {
    set_target_address_at(instruction_payload, code, target);
  }

  static const int kSpecialTargetSize = kPointerSize;

  // Distance between the address of the code target in the call instruction
  // and the return address
  static const int kCallTargetAddressOffset = kPointerSize;
  // Distance between start of patched return sequence and the emitted address
  // to jump to.
  static const int kPatchReturnSequenceAddressOffset = 1;  // JMP imm32.

  // Distance between start of patched debug break slot and the emitted address
  // to jump to.
  static const int kPatchDebugBreakSlotAddressOffset = 1;  // JMP imm32.

  static const int kCallInstructionLength = 5;
  static const int kPatchDebugBreakSlotReturnOffset = kPointerSize;
  static const int kJSReturnSequenceLength = 6;

  // The debug break slot must be able to contain a call instruction.
  static const int kDebugBreakSlotLength = kCallInstructionLength;

  // One byte opcode for test al, 0xXX.
  static const byte kTestAlByte = 0xA8;
  // One byte opcode for nop.
  static const byte kNopByte = 0x90;

  // One byte opcode for a short unconditional jump.
  static const byte kJmpShortOpcode = 0xEB;
  // One byte prefix for a short conditional jump.
  static const byte kJccShortPrefix = 0x70;
  static const byte kJncShortOpcode = kJccShortPrefix | not_carry;
  static const byte kJcShortOpcode = kJccShortPrefix | carry;
  static const byte kJnzShortOpcode = kJccShortPrefix | not_zero;
  static const byte kJzShortOpcode = kJccShortPrefix | zero;


  // ---------------------------------------------------------------------------
  // Code generation
  //
  // - function names correspond one-to-one to ia32 instruction mnemonics
  // - unless specified otherwise, instructions operate on 32bit operands
  // - instructions on 8bit (byte) operands/registers have a trailing '_b'
  // - instructions on 16bit (word) operands/registers have a trailing '_w'
  // - naming conflicts with C++ keywords are resolved via a trailing '_'

  // NOTE ON INTERFACE: Currently, the interface is not very consistent
  // in the sense that some operations (e.g. mov()) can be called in more
  // the one way to generate the same instruction: The Register argument
  // can in some cases be replaced with an Operand(Register) argument.
  // This should be cleaned up and made more orthogonal. The questions
  // is: should we always use Operands instead of Registers where an
  // Operand is possible, or should we have a Register (overloaded) form
  // instead? We must be careful to make sure that the selected instruction
  // is obvious from the parameters to avoid hard-to-find code generation
  // bugs.

  // Insert the smallest number of nop instructions
  // possible to align the pc offset to a multiple
  // of m. m must be a power of 2.
  void Align(int m);
  void Nop(int bytes = 1);
  // Aligns code to something that's optimal for a jump target for the platform.
  void CodeTargetAlign();

  // Stack
  void pushad();
  void popad();

  void pushfd();
  void popfd();

  void push(const Immediate& x);
  void push_imm32(int32_t imm32);
  void push(Register src);
  void push(const Operand& src);

  void pop(Register dst);
  void pop(const Operand& dst);

  void enter(const Immediate& size);
  void leave();

  // Moves
  void mov_b(Register dst, Register src) { mov_b(dst, Operand(src)); }
  void mov_b(Register dst, const Operand& src);
  void mov_b(Register dst, int8_t imm8) { mov_b(Operand(dst), imm8); }
  void mov_b(const Operand& dst, int8_t imm8);
  void mov_b(const Operand& dst, Register src);

  void mov_w(Register dst, const Operand& src);
  void mov_w(const Operand& dst, Register src);
  void mov_w(const Operand& dst, int16_t imm16);

  void mov(Register dst, int32_t imm32);
  void mov(Register dst, const Immediate& x);
  void mov(Register dst, Handle<Object> handle);
  void mov(Register dst, const Operand& src);
  void mov(Register dst, Register src);
  void mov(const Operand& dst, const Immediate& x);
  void mov(const Operand& dst, Handle<Object> handle);
  void mov(const Operand& dst, Register src);

  void movsx_b(Register dst, Register src) { movsx_b(dst, Operand(src)); }
  void movsx_b(Register dst, const Operand& src);

  void movsx_w(Register dst, Register src) { movsx_w(dst, Operand(src)); }
  void movsx_w(Register dst, const Operand& src);

  void movzx_b(Register dst, Register src) { movzx_b(dst, Operand(src)); }
  void movzx_b(Register dst, const Operand& src);

  void movzx_w(Register dst, Register src) { movzx_w(dst, Operand(src)); }
  void movzx_w(Register dst, const Operand& src);

  // Flag management.
  void cld();

  // Repetitive string instructions.
  void rep_movs();
  void rep_stos();
  void stos();

  // Exchange
  void xchg(Register dst, Register src);
  void xchg(Register dst, const Operand& src);

  // Arithmetics
  void adc(Register dst, int32_t imm32);
  void adc(Register dst, const Operand& src);

  void add(Register dst, Register src) { add(dst, Operand(src)); }
  void add(Register dst, const Operand& src);
  void add(const Operand& dst, Register src);
  void add(Register dst, const Immediate& imm) { add(Operand(dst), imm); }
  void add(const Operand& dst, const Immediate& x);

  void and_(Register dst, int32_t imm32);
  void and_(Register dst, const Immediate& x);
  void and_(Register dst, Register src) { and_(dst, Operand(src)); }
  void and_(Register dst, const Operand& src);
  void and_(const Operand& dst, Register src);
  void and_(const Operand& dst, const Immediate& x);

  void cmpb(Register reg, int8_t imm8) { cmpb(Operand(reg), imm8); }
  void cmpb(const Operand& op, int8_t imm8);
  void cmpb(Register reg, const Operand& op);
  void cmpb(const Operand& op, Register reg);
  void cmpb_al(const Operand& op);
  void cmpw_ax(const Operand& op);
  void cmpw(const Operand& op, Immediate imm16);
  void cmp(Register reg, int32_t imm32);
  void cmp(Register reg, Handle<Object> handle);
  void cmp(Register reg0, Register reg1) { cmp(reg0, Operand(reg1)); }
  void cmp(Register reg, const Operand& op);
  void cmp(Register reg, const Immediate& imm) { cmp(Operand(reg), imm); }
  void cmp(const Operand& op, const Immediate& imm);
  void cmp(const Operand& op, Handle<Object> handle);

  void dec_b(Register dst);
  void dec_b(const Operand& dst);

  void dec(Register dst);
  void dec(const Operand& dst);

  void cdq();

  void idiv(Register src) { idiv(Operand(src)); }
  void idiv(const Operand& src);
  void div(Register src) { div(Operand(src)); }
  void div(const Operand& src);

  // Signed multiply instructions.
  void imul(Register src);                               // edx:eax = eax * src.
  void imul(Register dst, Register src) { imul(dst, Operand(src)); }
  void imul(Register dst, const Operand& src);           // dst = dst * src.
  void imul(Register dst, Register src, int32_t imm32);  // dst = src * imm32.
  void imul(Register dst, const Operand& src, int32_t imm32);

  void inc(Register dst);
  void inc(const Operand& dst);

  void lea(Register dst, const Operand& src);

  // Unsigned multiply instruction.
  void mul(Register src);                                // edx:eax = eax * reg.

  void neg(Register dst);
  void neg(const Operand& dst);

  void not_(Register dst);
  void not_(const Operand& dst);

  void or_(Register dst, int32_t imm32);
  void or_(Register dst, Register src) { or_(dst, Operand(src)); }
  void or_(Register dst, const Operand& src);
  void or_(const Operand& dst, Register src);
  void or_(Register dst, const Immediate& imm) { or_(Operand(dst), imm); }
  void or_(const Operand& dst, const Immediate& x);

  void rcl(Register dst, uint8_t imm8);
  void rcr(Register dst, uint8_t imm8);

  void ror(Register dst, uint8_t imm8) { ror(Operand(dst), imm8); }
  void ror(const Operand& dst, uint8_t imm8);
  void ror_cl(Register dst) { ror_cl(Operand(dst)); }
  void ror_cl(const Operand& dst);

  void sar(Register dst, uint8_t imm8) { sar(Operand(dst), imm8); }
  void sar(const Operand& dst, uint8_t imm8);
  void sar_cl(Register dst) { sar_cl(Operand(dst)); }
  void sar_cl(const Operand& dst);

  void sbb(Register dst, const Operand& src);

  void shld(Register dst, Register src) { shld(dst, Operand(src)); }
  void shld(Register dst, const Operand& src);

  void shl(Register dst, uint8_t imm8) { shl(Operand(dst), imm8); }
  void shl(const Operand& dst, uint8_t imm8);
  void shl_cl(Register dst) { shl_cl(Operand(dst)); }
  void shl_cl(const Operand& dst);

  void shrd(Register dst, Register src) { shrd(dst, Operand(src)); }
  void shrd(Register dst, const Operand& src);

  void shr(Register dst, uint8_t imm8) { shr(Operand(dst), imm8); }
  void shr(const Operand& dst, uint8_t imm8);
  void shr_cl(Register dst) { shr_cl(Operand(dst)); }
  void shr_cl(const Operand& dst);

  void sub(Register dst, const Immediate& imm) { sub(Operand(dst), imm); }
  void sub(const Operand& dst, const Immediate& x);
  void sub(Register dst, Register src) { sub(dst, Operand(src)); }
  void sub(Register dst, const Operand& src);
  void sub(const Operand& dst, Register src);

  void test(Register reg, const Immediate& imm);
  void test(Register reg0, Register reg1) { test(reg0, Operand(reg1)); }
  void test(Register reg, const Operand& op);
  void test_b(Register reg, const Operand& op);
  void test(const Operand& op, const Immediate& imm);
  void test_b(Register reg, uint8_t imm8);
  void test_b(const Operand& op, uint8_t imm8);

  void xor_(Register dst, int32_t imm32);
  void xor_(Register dst, Register src) { xor_(dst, Operand(src)); }
  void xor_(Register dst, const Operand& src);
  void xor_(const Operand& dst, Register src);
  void xor_(Register dst, const Immediate& imm) { xor_(Operand(dst), imm); }
  void xor_(const Operand& dst, const Immediate& x);

  // Bit operations.
  void bt(const Operand& dst, Register src);
  void bts(Register dst, Register src) { bts(Operand(dst), src); }
  void bts(const Operand& dst, Register src);
  void bsr(Register dst, Register src) { bsr(dst, Operand(src)); }
  void bsr(Register dst, const Operand& src);

  // Miscellaneous
  void hlt();
  void int3();
  void nop();
  void ret(int imm16);

  // Label operations & relative jumps (PPUM Appendix D)
  //
  // Takes a branch opcode (cc) and a label (L) and generates
  // either a backward branch or a forward branch and links it
  // to the label fixup chain. Usage:
  //
  // Label L;    // unbound label
  // j(cc, &L);  // forward branch to unbound label
  // bind(&L);   // bind label to the current pc
  // j(cc, &L);  // backward branch to bound label
  // bind(&L);   // illegal: a label may be bound only once
  //
  // Note: The same Label can be used for forward and backward branches
  // but it may be bound only once.

  void bind(Label* L);  // binds an unbound label L to the current code position

  // Calls
  void call(Label* L);
  void call(byte* entry, RelocInfo::Mode rmode);
  int CallSize(const Operand& adr);
  void call(Register reg) { call(Operand(reg)); }
  void call(const Operand& adr);
  int CallSize(Handle<Code> code, RelocInfo::Mode mode);
  void call(Handle<Code> code,
            RelocInfo::Mode rmode,
            TypeFeedbackId id = TypeFeedbackId::None());

  // Jumps
  // unconditional jump to L
  void jmp(Label* L, Label::Distance distance = Label::kFar);
  void jmp(byte* entry, RelocInfo::Mode rmode);
  void jmp(Register reg) { jmp(Operand(reg)); }
  void jmp(const Operand& adr);
  void jmp(Handle<Code> code, RelocInfo::Mode rmode);

  // Conditional jumps
  void j(Condition cc,
         Label* L,
         Label::Distance distance = Label::kFar);
  void j(Condition cc, byte* entry, RelocInfo::Mode rmode);
  void j(Condition cc, Handle<Code> code);

  // Floating-point operations
  void fld(int i);
  void fstp(int i);

  void fld1();
  void fldz();
  void fldpi();
  void fldln2();

  void fld_s(const Operand& adr);
  void fld_d(const Operand& adr);

  void fstp_s(const Operand& adr);
  void fst_s(const Operand& adr);
  void fstp_d(const Operand& adr);
  void fst_d(const Operand& adr);

  void fild_s(const Operand& adr);
  void fild_d(const Operand& adr);

  void fist_s(const Operand& adr);

  void fistp_s(const Operand& adr);
  void fistp_d(const Operand& adr);

  // The fisttp instructions require SSE3.
  void fisttp_s(const Operand& adr);
  void fisttp_d(const Operand& adr);

  void fabs();
  void fchs();
  void fsqrt();
  void fcos();
  void fsin();
  void fptan();
  void fyl2x();
  void f2xm1();
  void fscale();
  void fninit();

  void fadd(int i);
  void fadd_i(int i);
  void fadd_d(const Operand& adr);
  void fsub(int i);
  void fsub_i(int i);
  void fmul(int i);
  void fmul_i(int i);
  void fdiv(int i);
  void fdiv_i(int i);

  void fisub_s(const Operand& adr);

  void faddp(int i = 1);
  void fsubp(int i = 1);
  void fsubrp(int i = 1);
  void fmulp(int i = 1);
  void fdivp(int i = 1);
  void fprem();
  void fprem1();

  void fxch(int i = 1);
  void fincstp();
  void ffree(int i = 0);

  void ftst();
  void fxam();
  void fucomp(int i);
  void fucompp();
  void fucomi(int i);
  void fucomip();
  void fcompp();
  void fnstsw_ax();
  void fldcw(const Operand& adr);
  void fnstcw(const Operand& adr);
  void fwait();
  void fnclex();
  void fnsave(const Operand& adr);
  void frstor(const Operand& adr);

  void frndint();

  void sahf();
  void setcc(Condition cc, Register reg);

  void cpuid();

  // TODO(lrn): Need SFENCE for movnt?

  // Check the code size generated from label to here.
  int SizeOfCodeGeneratedSince(Label* label) {
    return pc_offset() - label->pos();
  }

  // Mark address of the ExitJSFrame code.
  void RecordJSReturn();

  // Mark address of a debug break slot.
  void RecordDebugBreakSlot();

  // Record a comment relocation entry that can be used by a disassembler.
  // Use --code-comments to enable, or provide "force = true" flag to always
  // write a comment.
  void RecordComment(const char* msg, bool force = false);

  // Writes a single byte or word of data in the code stream.  Used for
  // inline tables, e.g., jump-tables.
  void db(uint8_t data);
  void dd(uint32_t data);

  // Check if there is less than kGap bytes available in the buffer.
  // If this is the case, we need to grow the buffer before emitting
  // an instruction or relocation information.
  inline bool buffer_overflow() const {
    return pc_ >= reloc_info_writer.pos() - kGap;
  }

  // Get the number of bytes available in the buffer.
  inline int available_space() const { return reloc_info_writer.pos() - pc_; }

  static bool IsNop(Address addr);

  PositionsRecorder* positions_recorder() { return &positions_recorder_; }

  int relocation_writer_size() {
    return (buffer_ + buffer_size_) - reloc_info_writer.pos();
  }

  // Avoid overflows for displacements etc.
  static const int kMaximalBufferSize = 512*MB;

  byte byte_at(int pos) { return buffer_[pos]; }
  void set_byte_at(int pos, byte value) { buffer_[pos] = value; }

  // Allocate a constant pool of the correct size for the generated code.
  Handle<ConstantPoolArray> NewConstantPool(Isolate* isolate);

  // Generate the constant pool for the generated code.
  void PopulateConstantPool(ConstantPoolArray* constant_pool);

 protected:
  byte* addr_at(int pos) { return buffer_ + pos; }


 private:
  uint32_t long_at(int pos)  {
    return *reinterpret_cast<uint32_t*>(addr_at(pos));
  }
  void long_at_put(int pos, uint32_t x)  {
    *reinterpret_cast<uint32_t*>(addr_at(pos)) = x;
  }

  // code emission
  void GrowBuffer();
  inline void emit(uint32_t x);
  inline void emit(Handle<Object> handle);
  inline void emit(uint32_t x,
                   RelocInfo::Mode rmode,
                   TypeFeedbackId id = TypeFeedbackId::None());
  inline void emit(Handle<Code> code,
                   RelocInfo::Mode rmode,
                   TypeFeedbackId id = TypeFeedbackId::None());
  inline void emit(const Immediate& x);
  inline void emit_w(const Immediate& x);

  // Emit the code-object-relative offset of the label's position
  inline void emit_code_relative_offset(Label* label);

  // instruction generation
  void emit_arith_b(int op1, int op2, Register dst, int imm8);

  // Emit a basic arithmetic instruction (i.e. first byte of the family is 0x81)
  // with a given destination expression and an immediate operand.  It attempts
  // to use the shortest encoding possible.
  // sel specifies the /n in the modrm byte (see the Intel PRM).
  void emit_arith(int sel, Operand dst, const Immediate& x);

  void emit_operand(Register reg, const Operand& adr);

  void emit_farith(int b1, int b2, int i);

  // labels
  void print(Label* L);
  void bind_to(Label* L, int pos);

  // displacements
  inline Displacement disp_at(Label* L);
  inline void disp_at_put(Label* L, Displacement disp);
  inline void emit_disp(Label* L, Displacement::Type type);
  inline void emit_near_disp(Label* L);

  // record reloc info for current pc_
  void RecordRelocInfo(RelocInfo::Mode rmode, intptr_t data = 0);

  friend class CodePatcher;
  friend class EnsureSpace;

  // code generation
  RelocInfoWriter reloc_info_writer;

  PositionsRecorder positions_recorder_;
  friend class PositionsRecorder;
};


// Helper class that ensures that there is enough space for generating
// instructions and relocation information.  The constructor makes
// sure that there is enough space and (in debug mode) the destructor
// checks that we did not generate too much.
class EnsureSpace BASE_EMBEDDED {
 public:
  explicit EnsureSpace(Assembler* assembler) : assembler_(assembler) {
    if (assembler_->buffer_overflow()) assembler_->GrowBuffer();
#ifdef DEBUG
    space_before_ = assembler_->available_space();
#endif
  }

#ifdef DEBUG
  ~EnsureSpace() {
    int bytes_generated = space_before_ - assembler_->available_space();
    DCHECK(bytes_generated < assembler_->kGap);
  }
#endif

 private:
  Assembler* assembler_;
#ifdef DEBUG
  int space_before_;
#endif
};

} }  // namespace v8::internal

#endif  // V8_X87_ASSEMBLER_X87_H_
