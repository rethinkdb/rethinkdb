// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_IA32_REGEXP_MACRO_ASSEMBLER_IA32_H_
#define V8_IA32_REGEXP_MACRO_ASSEMBLER_IA32_H_

#include "src/ia32/assembler-ia32.h"
#include "src/ia32/assembler-ia32-inl.h"
#include "src/macro-assembler.h"

namespace v8 {
namespace internal {

#ifndef V8_INTERPRETED_REGEXP
class RegExpMacroAssemblerIA32: public NativeRegExpMacroAssembler {
 public:
  RegExpMacroAssemblerIA32(Mode mode, int registers_to_save, Zone* zone);
  virtual ~RegExpMacroAssemblerIA32();
  virtual int stack_limit_slack();
  virtual void AdvanceCurrentPosition(int by);
  virtual void AdvanceRegister(int reg, int by);
  virtual void Backtrack();
  virtual void Bind(Label* label);
  virtual void CheckAtStart(Label* on_at_start);
  virtual void CheckCharacter(uint32_t c, Label* on_equal);
  virtual void CheckCharacterAfterAnd(uint32_t c,
                                      uint32_t mask,
                                      Label* on_equal);
  virtual void CheckCharacterGT(uc16 limit, Label* on_greater);
  virtual void CheckCharacterLT(uc16 limit, Label* on_less);
  // A "greedy loop" is a loop that is both greedy and with a simple
  // body. It has a particularly simple implementation.
  virtual void CheckGreedyLoop(Label* on_tos_equals_current_position);
  virtual void CheckNotAtStart(Label* on_not_at_start);
  virtual void CheckNotBackReference(int start_reg, Label* on_no_match);
  virtual void CheckNotBackReferenceIgnoreCase(int start_reg,
                                               Label* on_no_match);
  virtual void CheckNotCharacter(uint32_t c, Label* on_not_equal);
  virtual void CheckNotCharacterAfterAnd(uint32_t c,
                                         uint32_t mask,
                                         Label* on_not_equal);
  virtual void CheckNotCharacterAfterMinusAnd(uc16 c,
                                              uc16 minus,
                                              uc16 mask,
                                              Label* on_not_equal);
  virtual void CheckCharacterInRange(uc16 from,
                                     uc16 to,
                                     Label* on_in_range);
  virtual void CheckCharacterNotInRange(uc16 from,
                                        uc16 to,
                                        Label* on_not_in_range);
  virtual void CheckBitInTable(Handle<ByteArray> table, Label* on_bit_set);

  // Checks whether the given offset from the current position is before
  // the end of the string.
  virtual void CheckPosition(int cp_offset, Label* on_outside_input);
  virtual bool CheckSpecialCharacterClass(uc16 type, Label* on_no_match);
  virtual void Fail();
  virtual Handle<HeapObject> GetCode(Handle<String> source);
  virtual void GoTo(Label* label);
  virtual void IfRegisterGE(int reg, int comparand, Label* if_ge);
  virtual void IfRegisterLT(int reg, int comparand, Label* if_lt);
  virtual void IfRegisterEqPos(int reg, Label* if_eq);
  virtual IrregexpImplementation Implementation();
  virtual void LoadCurrentCharacter(int cp_offset,
                                    Label* on_end_of_input,
                                    bool check_bounds = true,
                                    int characters = 1);
  virtual void PopCurrentPosition();
  virtual void PopRegister(int register_index);
  virtual void PushBacktrack(Label* label);
  virtual void PushCurrentPosition();
  virtual void PushRegister(int register_index,
                            StackCheckFlag check_stack_limit);
  virtual void ReadCurrentPositionFromRegister(int reg);
  virtual void ReadStackPointerFromRegister(int reg);
  virtual void SetCurrentPositionFromEnd(int by);
  virtual void SetRegister(int register_index, int to);
  virtual bool Succeed();
  virtual void WriteCurrentPositionToRegister(int reg, int cp_offset);
  virtual void ClearRegisters(int reg_from, int reg_to);
  virtual void WriteStackPointerToRegister(int reg);

  // Called from RegExp if the stack-guard is triggered.
  // If the code object is relocated, the return address is fixed before
  // returning.
  static int CheckStackGuardState(Address* return_address,
                                  Code* re_code,
                                  Address re_frame);

 private:
  // Offsets from ebp of function parameters and stored registers.
  static const int kFramePointer = 0;
  // Above the frame pointer - function parameters and return address.
  static const int kReturn_eip = kFramePointer + kPointerSize;
  static const int kFrameAlign = kReturn_eip + kPointerSize;
  // Parameters.
  static const int kInputString = kFrameAlign;
  static const int kStartIndex = kInputString + kPointerSize;
  static const int kInputStart = kStartIndex + kPointerSize;
  static const int kInputEnd = kInputStart + kPointerSize;
  static const int kRegisterOutput = kInputEnd + kPointerSize;
  // For the case of global regular expression, we have room to store at least
  // one set of capture results.  For the case of non-global regexp, we ignore
  // this value.
  static const int kNumOutputRegisters = kRegisterOutput + kPointerSize;
  static const int kStackHighEnd = kNumOutputRegisters + kPointerSize;
  static const int kDirectCall = kStackHighEnd + kPointerSize;
  static const int kIsolate = kDirectCall + kPointerSize;
  // Below the frame pointer - local stack variables.
  // When adding local variables remember to push space for them in
  // the frame in GetCode.
  static const int kBackup_esi = kFramePointer - kPointerSize;
  static const int kBackup_edi = kBackup_esi - kPointerSize;
  static const int kBackup_ebx = kBackup_edi - kPointerSize;
  static const int kSuccessfulCaptures = kBackup_ebx - kPointerSize;
  static const int kInputStartMinusOne = kSuccessfulCaptures - kPointerSize;
  // First register address. Following registers are below it on the stack.
  static const int kRegisterZero = kInputStartMinusOne - kPointerSize;

  // Initial size of code buffer.
  static const size_t kRegExpCodeSize = 1024;

  // Load a number of characters at the given offset from the
  // current position, into the current-character register.
  void LoadCurrentCharacterUnchecked(int cp_offset, int character_count);

  // Check whether preemption has been requested.
  void CheckPreemption();

  // Check whether we are exceeding the stack limit on the backtrack stack.
  void CheckStackLimit();

  // Generate a call to CheckStackGuardState.
  void CallCheckStackGuardState(Register scratch);

  // The ebp-relative location of a regexp register.
  Operand register_location(int register_index);

  // The register containing the current character after LoadCurrentCharacter.
  inline Register current_character() { return edx; }

  // The register containing the backtrack stack top. Provides a meaningful
  // name to the register.
  inline Register backtrack_stackpointer() { return ecx; }

  // Byte size of chars in the string to match (decided by the Mode argument)
  inline int char_size() { return static_cast<int>(mode_); }

  // Equivalent to a conditional branch to the label, unless the label
  // is NULL, in which case it is a conditional Backtrack.
  void BranchOrBacktrack(Condition condition, Label* to);

  // Call and return internally in the generated code in a way that
  // is GC-safe (i.e., doesn't leave absolute code addresses on the stack)
  inline void SafeCall(Label* to);
  inline void SafeReturn();
  inline void SafeCallTarget(Label* name);

  // Pushes the value of a register on the backtrack stack. Decrements the
  // stack pointer (ecx) by a word size and stores the register's value there.
  inline void Push(Register source);

  // Pushes a value on the backtrack stack. Decrements the stack pointer (ecx)
  // by a word size and stores the value there.
  inline void Push(Immediate value);

  // Pops a value from the backtrack stack. Reads the word at the stack pointer
  // (ecx) and increments it by a word size.
  inline void Pop(Register target);

  Isolate* isolate() const { return masm_->isolate(); }

  MacroAssembler* masm_;

  // Which mode to generate code for (LATIN1 or UC16).
  Mode mode_;

  // One greater than maximal register index actually used.
  int num_registers_;

  // Number of registers to output at the end (the saved registers
  // are always 0..num_saved_registers_-1)
  int num_saved_registers_;

  // Labels used internally.
  Label entry_label_;
  Label start_label_;
  Label success_label_;
  Label backtrack_label_;
  Label exit_label_;
  Label check_preempt_label_;
  Label stack_overflow_label_;
};
#endif  // V8_INTERPRETED_REGEXP

}}  // namespace v8::internal

#endif  // V8_IA32_REGEXP_MACRO_ASSEMBLER_IA32_H_
