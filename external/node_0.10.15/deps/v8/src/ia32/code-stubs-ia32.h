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

#ifndef V8_IA32_CODE_STUBS_IA32_H_
#define V8_IA32_CODE_STUBS_IA32_H_

#include "macro-assembler.h"
#include "code-stubs.h"
#include "ic-inl.h"

namespace v8 {
namespace internal {


// Compute a transcendental math function natively, or call the
// TranscendentalCache runtime function.
class TranscendentalCacheStub: public CodeStub {
 public:
  enum ArgumentType {
    TAGGED = 0,
    UNTAGGED = 1 << TranscendentalCache::kTranscendentalTypeBits
  };

  TranscendentalCacheStub(TranscendentalCache::Type type,
                          ArgumentType argument_type)
      : type_(type), argument_type_(argument_type) {}
  void Generate(MacroAssembler* masm);
  static void GenerateOperation(MacroAssembler* masm,
                                TranscendentalCache::Type type);
 private:
  TranscendentalCache::Type type_;
  ArgumentType argument_type_;

  Major MajorKey() { return TranscendentalCache; }
  int MinorKey() { return type_ | argument_type_; }
  Runtime::FunctionId RuntimeFunction();
};


class StoreBufferOverflowStub: public CodeStub {
 public:
  explicit StoreBufferOverflowStub(SaveFPRegsMode save_fp)
      : save_doubles_(save_fp) { }

  void Generate(MacroAssembler* masm);

  virtual bool IsPregenerated() { return true; }
  static void GenerateFixedRegStubsAheadOfTime();
  virtual bool SometimesSetsUpAFrame() { return false; }

 private:
  SaveFPRegsMode save_doubles_;

  Major MajorKey() { return StoreBufferOverflow; }
  int MinorKey() { return (save_doubles_ == kSaveFPRegs) ? 1 : 0; }
};


class UnaryOpStub: public CodeStub {
 public:
  UnaryOpStub(Token::Value op,
              UnaryOverwriteMode mode,
              UnaryOpIC::TypeInfo operand_type = UnaryOpIC::UNINITIALIZED)
      : op_(op),
        mode_(mode),
        operand_type_(operand_type) {
  }

 private:
  Token::Value op_;
  UnaryOverwriteMode mode_;

  // Operand type information determined at runtime.
  UnaryOpIC::TypeInfo operand_type_;

  virtual void PrintName(StringStream* stream);

  class ModeBits: public BitField<UnaryOverwriteMode, 0, 1> {};
  class OpBits: public BitField<Token::Value, 1, 7> {};
  class OperandTypeInfoBits: public BitField<UnaryOpIC::TypeInfo, 8, 3> {};

  Major MajorKey() { return UnaryOp; }
  int MinorKey() {
    return ModeBits::encode(mode_)
           | OpBits::encode(op_)
           | OperandTypeInfoBits::encode(operand_type_);
  }

  // Note: A lot of the helper functions below will vanish when we use virtual
  // function instead of switch more often.
  void Generate(MacroAssembler* masm);

  void GenerateTypeTransition(MacroAssembler* masm);

  void GenerateSmiStub(MacroAssembler* masm);
  void GenerateSmiStubSub(MacroAssembler* masm);
  void GenerateSmiStubBitNot(MacroAssembler* masm);
  void GenerateSmiCodeSub(MacroAssembler* masm,
                          Label* non_smi,
                          Label* undo,
                          Label* slow,
                          Label::Distance non_smi_near = Label::kFar,
                          Label::Distance undo_near = Label::kFar,
                          Label::Distance slow_near = Label::kFar);
  void GenerateSmiCodeBitNot(MacroAssembler* masm,
                             Label* non_smi,
                             Label::Distance non_smi_near = Label::kFar);
  void GenerateSmiCodeUndo(MacroAssembler* masm);

  void GenerateHeapNumberStub(MacroAssembler* masm);
  void GenerateHeapNumberStubSub(MacroAssembler* masm);
  void GenerateHeapNumberStubBitNot(MacroAssembler* masm);
  void GenerateHeapNumberCodeSub(MacroAssembler* masm, Label* slow);
  void GenerateHeapNumberCodeBitNot(MacroAssembler* masm, Label* slow);

  void GenerateGenericStub(MacroAssembler* masm);
  void GenerateGenericStubSub(MacroAssembler* masm);
  void GenerateGenericStubBitNot(MacroAssembler* masm);
  void GenerateGenericCodeFallback(MacroAssembler* masm);

  virtual int GetCodeKind() { return Code::UNARY_OP_IC; }

  virtual InlineCacheState GetICState() {
    return UnaryOpIC::ToState(operand_type_);
  }

  virtual void FinishCode(Handle<Code> code) {
    code->set_unary_op_type(operand_type_);
  }
};


class BinaryOpStub: public CodeStub {
 public:
  BinaryOpStub(Token::Value op, OverwriteMode mode)
      : op_(op),
        mode_(mode),
        operands_type_(BinaryOpIC::UNINITIALIZED),
        result_type_(BinaryOpIC::UNINITIALIZED) {
    use_sse3_ = CpuFeatures::IsSupported(SSE3);
    ASSERT(OpBits::is_valid(Token::NUM_TOKENS));
  }

  BinaryOpStub(
      int key,
      BinaryOpIC::TypeInfo operands_type,
      BinaryOpIC::TypeInfo result_type = BinaryOpIC::UNINITIALIZED)
      : op_(OpBits::decode(key)),
        mode_(ModeBits::decode(key)),
        use_sse3_(SSE3Bits::decode(key)),
        operands_type_(operands_type),
        result_type_(result_type) { }

 private:
  enum SmiCodeGenerateHeapNumberResults {
    ALLOW_HEAPNUMBER_RESULTS,
    NO_HEAPNUMBER_RESULTS
  };

  Token::Value op_;
  OverwriteMode mode_;
  bool use_sse3_;

  // Operand type information determined at runtime.
  BinaryOpIC::TypeInfo operands_type_;
  BinaryOpIC::TypeInfo result_type_;

  virtual void PrintName(StringStream* stream);

  // Minor key encoding in 16 bits RRRTTTSOOOOOOOMM.
  class ModeBits: public BitField<OverwriteMode, 0, 2> {};
  class OpBits: public BitField<Token::Value, 2, 7> {};
  class SSE3Bits: public BitField<bool, 9, 1> {};
  class OperandTypeInfoBits: public BitField<BinaryOpIC::TypeInfo, 10, 3> {};
  class ResultTypeInfoBits: public BitField<BinaryOpIC::TypeInfo, 13, 3> {};

  Major MajorKey() { return BinaryOp; }
  int MinorKey() {
    return OpBits::encode(op_)
           | ModeBits::encode(mode_)
           | SSE3Bits::encode(use_sse3_)
           | OperandTypeInfoBits::encode(operands_type_)
           | ResultTypeInfoBits::encode(result_type_);
  }

  void Generate(MacroAssembler* masm);
  void GenerateGeneric(MacroAssembler* masm);
  void GenerateSmiCode(MacroAssembler* masm,
                       Label* slow,
                       SmiCodeGenerateHeapNumberResults heapnumber_results);
  void GenerateLoadArguments(MacroAssembler* masm);
  void GenerateReturn(MacroAssembler* masm);
  void GenerateUninitializedStub(MacroAssembler* masm);
  void GenerateSmiStub(MacroAssembler* masm);
  void GenerateInt32Stub(MacroAssembler* masm);
  void GenerateHeapNumberStub(MacroAssembler* masm);
  void GenerateOddballStub(MacroAssembler* masm);
  void GenerateStringStub(MacroAssembler* masm);
  void GenerateBothStringStub(MacroAssembler* masm);
  void GenerateGenericStub(MacroAssembler* masm);
  void GenerateAddStrings(MacroAssembler* masm);

  void GenerateHeapResultAllocation(MacroAssembler* masm, Label* alloc_failure);
  void GenerateRegisterArgsPush(MacroAssembler* masm);
  void GenerateTypeTransition(MacroAssembler* masm);
  void GenerateTypeTransitionWithSavedArgs(MacroAssembler* masm);

  virtual int GetCodeKind() { return Code::BINARY_OP_IC; }

  virtual InlineCacheState GetICState() {
    return BinaryOpIC::ToState(operands_type_);
  }

  virtual void FinishCode(Handle<Code> code) {
    code->set_binary_op_type(operands_type_);
    code->set_binary_op_result_type(result_type_);
  }

  friend class CodeGenerator;
};


class StringHelper : public AllStatic {
 public:
  // Generate code for copying characters using a simple loop. This should only
  // be used in places where the number of characters is small and the
  // additional setup and checking in GenerateCopyCharactersREP adds too much
  // overhead. Copying of overlapping regions is not supported.
  static void GenerateCopyCharacters(MacroAssembler* masm,
                                     Register dest,
                                     Register src,
                                     Register count,
                                     Register scratch,
                                     bool ascii);

  // Generate code for copying characters using the rep movs instruction.
  // Copies ecx characters from esi to edi. Copying of overlapping regions is
  // not supported.
  static void GenerateCopyCharactersREP(MacroAssembler* masm,
                                        Register dest,     // Must be edi.
                                        Register src,      // Must be esi.
                                        Register count,    // Must be ecx.
                                        Register scratch,  // Neither of above.
                                        bool ascii);

  // Probe the symbol table for a two character string. If the string
  // requires non-standard hashing a jump to the label not_probed is
  // performed and registers c1 and c2 are preserved. In all other
  // cases they are clobbered. If the string is not found by probing a
  // jump to the label not_found is performed. This jump does not
  // guarantee that the string is not in the symbol table. If the
  // string is found the code falls through with the string in
  // register eax.
  static void GenerateTwoCharacterSymbolTableProbe(MacroAssembler* masm,
                                                   Register c1,
                                                   Register c2,
                                                   Register scratch1,
                                                   Register scratch2,
                                                   Register scratch3,
                                                   Label* not_probed,
                                                   Label* not_found);

  // Generate string hash.
  static void GenerateHashInit(MacroAssembler* masm,
                               Register hash,
                               Register character,
                               Register scratch);
  static void GenerateHashAddCharacter(MacroAssembler* masm,
                                       Register hash,
                                       Register character,
                                       Register scratch);
  static void GenerateHashGetHash(MacroAssembler* masm,
                                  Register hash,
                                  Register scratch);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StringHelper);
};


// Flag that indicates how to generate code for the stub StringAddStub.
enum StringAddFlags {
  NO_STRING_ADD_FLAGS = 0,
  // Omit left string check in stub (left is definitely a string).
  NO_STRING_CHECK_LEFT_IN_STUB = 1 << 0,
  // Omit right string check in stub (right is definitely a string).
  NO_STRING_CHECK_RIGHT_IN_STUB = 1 << 1,
  // Omit both string checks in stub.
  NO_STRING_CHECK_IN_STUB =
      NO_STRING_CHECK_LEFT_IN_STUB | NO_STRING_CHECK_RIGHT_IN_STUB
};


class StringAddStub: public CodeStub {
 public:
  explicit StringAddStub(StringAddFlags flags) : flags_(flags) {}

 private:
  Major MajorKey() { return StringAdd; }
  int MinorKey() { return flags_; }

  void Generate(MacroAssembler* masm);

  void GenerateConvertArgument(MacroAssembler* masm,
                               int stack_offset,
                               Register arg,
                               Register scratch1,
                               Register scratch2,
                               Register scratch3,
                               Label* slow);

  const StringAddFlags flags_;
};


class SubStringStub: public CodeStub {
 public:
  SubStringStub() {}

 private:
  Major MajorKey() { return SubString; }
  int MinorKey() { return 0; }

  void Generate(MacroAssembler* masm);
};


class StringCompareStub: public CodeStub {
 public:
  StringCompareStub() { }

  // Compares two flat ASCII strings and returns result in eax.
  static void GenerateCompareFlatAsciiStrings(MacroAssembler* masm,
                                              Register left,
                                              Register right,
                                              Register scratch1,
                                              Register scratch2,
                                              Register scratch3);

  // Compares two flat ASCII strings for equality and returns result
  // in eax.
  static void GenerateFlatAsciiStringEquals(MacroAssembler* masm,
                                            Register left,
                                            Register right,
                                            Register scratch1,
                                            Register scratch2);

 private:
  virtual Major MajorKey() { return StringCompare; }
  virtual int MinorKey() { return 0; }
  virtual void Generate(MacroAssembler* masm);

  static void GenerateAsciiCharsCompareLoop(
      MacroAssembler* masm,
      Register left,
      Register right,
      Register length,
      Register scratch,
      Label* chars_not_equal,
      Label::Distance chars_not_equal_near = Label::kFar);
};


class NumberToStringStub: public CodeStub {
 public:
  NumberToStringStub() { }

  // Generate code to do a lookup in the number string cache. If the number in
  // the register object is found in the cache the generated code falls through
  // with the result in the result register. The object and the result register
  // can be the same. If the number is not found in the cache the code jumps to
  // the label not_found with only the content of register object unchanged.
  static void GenerateLookupNumberStringCache(MacroAssembler* masm,
                                              Register object,
                                              Register result,
                                              Register scratch1,
                                              Register scratch2,
                                              bool object_is_smi,
                                              Label* not_found);

 private:
  Major MajorKey() { return NumberToString; }
  int MinorKey() { return 0; }

  void Generate(MacroAssembler* masm);
};


class StringDictionaryLookupStub: public CodeStub {
 public:
  enum LookupMode { POSITIVE_LOOKUP, NEGATIVE_LOOKUP };

  StringDictionaryLookupStub(Register dictionary,
                             Register result,
                             Register index,
                             LookupMode mode)
      : dictionary_(dictionary), result_(result), index_(index), mode_(mode) { }

  void Generate(MacroAssembler* masm);

  static void GenerateNegativeLookup(MacroAssembler* masm,
                                     Label* miss,
                                     Label* done,
                                     Register properties,
                                     Handle<String> name,
                                     Register r0);

  static void GeneratePositiveLookup(MacroAssembler* masm,
                                     Label* miss,
                                     Label* done,
                                     Register elements,
                                     Register name,
                                     Register r0,
                                     Register r1);

  virtual bool SometimesSetsUpAFrame() { return false; }

 private:
  static const int kInlinedProbes = 4;
  static const int kTotalProbes = 20;

  static const int kCapacityOffset =
      StringDictionary::kHeaderSize +
      StringDictionary::kCapacityIndex * kPointerSize;

  static const int kElementsStartOffset =
      StringDictionary::kHeaderSize +
      StringDictionary::kElementsStartIndex * kPointerSize;

  Major MajorKey() { return StringDictionaryLookup; }

  int MinorKey() {
    return DictionaryBits::encode(dictionary_.code()) |
        ResultBits::encode(result_.code()) |
        IndexBits::encode(index_.code()) |
        LookupModeBits::encode(mode_);
  }

  class DictionaryBits: public BitField<int, 0, 3> {};
  class ResultBits: public BitField<int, 3, 3> {};
  class IndexBits: public BitField<int, 6, 3> {};
  class LookupModeBits: public BitField<LookupMode, 9, 1> {};

  Register dictionary_;
  Register result_;
  Register index_;
  LookupMode mode_;
};


class RecordWriteStub: public CodeStub {
 public:
  RecordWriteStub(Register object,
                  Register value,
                  Register address,
                  RememberedSetAction remembered_set_action,
                  SaveFPRegsMode fp_mode)
      : object_(object),
        value_(value),
        address_(address),
        remembered_set_action_(remembered_set_action),
        save_fp_regs_mode_(fp_mode),
        regs_(object,   // An input reg.
              address,  // An input reg.
              value) {  // One scratch reg.
  }

  enum Mode {
    STORE_BUFFER_ONLY,
    INCREMENTAL,
    INCREMENTAL_COMPACTION
  };

  virtual bool IsPregenerated();
  static void GenerateFixedRegStubsAheadOfTime();
  virtual bool SometimesSetsUpAFrame() { return false; }

  static const byte kTwoByteNopInstruction = 0x3c;  // Cmpb al, #imm8.
  static const byte kTwoByteJumpInstruction = 0xeb;  // Jmp #imm8.

  static const byte kFiveByteNopInstruction = 0x3d;  // Cmpl eax, #imm32.
  static const byte kFiveByteJumpInstruction = 0xe9;  // Jmp #imm32.

  static Mode GetMode(Code* stub) {
    byte first_instruction = stub->instruction_start()[0];
    byte second_instruction = stub->instruction_start()[2];

    if (first_instruction == kTwoByteJumpInstruction) {
      return INCREMENTAL;
    }

    ASSERT(first_instruction == kTwoByteNopInstruction);

    if (second_instruction == kFiveByteJumpInstruction) {
      return INCREMENTAL_COMPACTION;
    }

    ASSERT(second_instruction == kFiveByteNopInstruction);

    return STORE_BUFFER_ONLY;
  }

  static void Patch(Code* stub, Mode mode) {
    switch (mode) {
      case STORE_BUFFER_ONLY:
        ASSERT(GetMode(stub) == INCREMENTAL ||
               GetMode(stub) == INCREMENTAL_COMPACTION);
        stub->instruction_start()[0] = kTwoByteNopInstruction;
        stub->instruction_start()[2] = kFiveByteNopInstruction;
        break;
      case INCREMENTAL:
        ASSERT(GetMode(stub) == STORE_BUFFER_ONLY);
        stub->instruction_start()[0] = kTwoByteJumpInstruction;
        break;
      case INCREMENTAL_COMPACTION:
        ASSERT(GetMode(stub) == STORE_BUFFER_ONLY);
        stub->instruction_start()[0] = kTwoByteNopInstruction;
        stub->instruction_start()[2] = kFiveByteJumpInstruction;
        break;
    }
    ASSERT(GetMode(stub) == mode);
    CPU::FlushICache(stub->instruction_start(), 7);
  }

 private:
  // This is a helper class for freeing up 3 scratch registers, where the third
  // is always ecx (needed for shift operations).  The input is two registers
  // that must be preserved and one scratch register provided by the caller.
  class RegisterAllocation {
   public:
    RegisterAllocation(Register object,
                       Register address,
                       Register scratch0)
        : object_orig_(object),
          address_orig_(address),
          scratch0_orig_(scratch0),
          object_(object),
          address_(address),
          scratch0_(scratch0) {
      ASSERT(!AreAliased(scratch0, object, address, no_reg));
      scratch1_ = GetRegThatIsNotEcxOr(object_, address_, scratch0_);
      if (scratch0.is(ecx)) {
        scratch0_ = GetRegThatIsNotEcxOr(object_, address_, scratch1_);
      }
      if (object.is(ecx)) {
        object_ = GetRegThatIsNotEcxOr(address_, scratch0_, scratch1_);
      }
      if (address.is(ecx)) {
        address_ = GetRegThatIsNotEcxOr(object_, scratch0_, scratch1_);
      }
      ASSERT(!AreAliased(scratch0_, object_, address_, ecx));
    }

    void Save(MacroAssembler* masm) {
      ASSERT(!address_orig_.is(object_));
      ASSERT(object_.is(object_orig_) || address_.is(address_orig_));
      ASSERT(!AreAliased(object_, address_, scratch1_, scratch0_));
      ASSERT(!AreAliased(object_orig_, address_, scratch1_, scratch0_));
      ASSERT(!AreAliased(object_, address_orig_, scratch1_, scratch0_));
      // We don't have to save scratch0_orig_ because it was given to us as
      // a scratch register.  But if we had to switch to a different reg then
      // we should save the new scratch0_.
      if (!scratch0_.is(scratch0_orig_)) masm->push(scratch0_);
      if (!ecx.is(scratch0_orig_) &&
          !ecx.is(object_orig_) &&
          !ecx.is(address_orig_)) {
        masm->push(ecx);
      }
      masm->push(scratch1_);
      if (!address_.is(address_orig_)) {
        masm->push(address_);
        masm->mov(address_, address_orig_);
      }
      if (!object_.is(object_orig_)) {
        masm->push(object_);
        masm->mov(object_, object_orig_);
      }
    }

    void Restore(MacroAssembler* masm) {
      // These will have been preserved the entire time, so we just need to move
      // them back.  Only in one case is the orig_ reg different from the plain
      // one, since only one of them can alias with ecx.
      if (!object_.is(object_orig_)) {
        masm->mov(object_orig_, object_);
        masm->pop(object_);
      }
      if (!address_.is(address_orig_)) {
        masm->mov(address_orig_, address_);
        masm->pop(address_);
      }
      masm->pop(scratch1_);
      if (!ecx.is(scratch0_orig_) &&
          !ecx.is(object_orig_) &&
          !ecx.is(address_orig_)) {
        masm->pop(ecx);
      }
      if (!scratch0_.is(scratch0_orig_)) masm->pop(scratch0_);
    }

    // If we have to call into C then we need to save and restore all caller-
    // saved registers that were not already preserved.  The caller saved
    // registers are eax, ecx and edx.  The three scratch registers (incl. ecx)
    // will be restored by other means so we don't bother pushing them here.
    void SaveCallerSaveRegisters(MacroAssembler* masm, SaveFPRegsMode mode) {
      if (!scratch0_.is(eax) && !scratch1_.is(eax)) masm->push(eax);
      if (!scratch0_.is(edx) && !scratch1_.is(edx)) masm->push(edx);
      if (mode == kSaveFPRegs) {
        CpuFeatures::Scope scope(SSE2);
        masm->sub(esp,
                  Immediate(kDoubleSize * (XMMRegister::kNumRegisters - 1)));
        // Save all XMM registers except XMM0.
        for (int i = XMMRegister::kNumRegisters - 1; i > 0; i--) {
          XMMRegister reg = XMMRegister::from_code(i);
          masm->movdbl(Operand(esp, (i - 1) * kDoubleSize), reg);
        }
      }
    }

    inline void RestoreCallerSaveRegisters(MacroAssembler*masm,
                                           SaveFPRegsMode mode) {
      if (mode == kSaveFPRegs) {
        CpuFeatures::Scope scope(SSE2);
        // Restore all XMM registers except XMM0.
        for (int i = XMMRegister::kNumRegisters - 1; i > 0; i--) {
          XMMRegister reg = XMMRegister::from_code(i);
          masm->movdbl(reg, Operand(esp, (i - 1) * kDoubleSize));
        }
        masm->add(esp,
                  Immediate(kDoubleSize * (XMMRegister::kNumRegisters - 1)));
      }
      if (!scratch0_.is(edx) && !scratch1_.is(edx)) masm->pop(edx);
      if (!scratch0_.is(eax) && !scratch1_.is(eax)) masm->pop(eax);
    }

    inline Register object() { return object_; }
    inline Register address() { return address_; }
    inline Register scratch0() { return scratch0_; }
    inline Register scratch1() { return scratch1_; }

   private:
    Register object_orig_;
    Register address_orig_;
    Register scratch0_orig_;
    Register object_;
    Register address_;
    Register scratch0_;
    Register scratch1_;
    // Third scratch register is always ecx.

    Register GetRegThatIsNotEcxOr(Register r1,
                                  Register r2,
                                  Register r3) {
      for (int i = 0; i < Register::kNumAllocatableRegisters; i++) {
        Register candidate = Register::FromAllocationIndex(i);
        if (candidate.is(ecx)) continue;
        if (candidate.is(r1)) continue;
        if (candidate.is(r2)) continue;
        if (candidate.is(r3)) continue;
        return candidate;
      }
      UNREACHABLE();
      return no_reg;
    }
    friend class RecordWriteStub;
  };

  enum OnNoNeedToInformIncrementalMarker {
    kReturnOnNoNeedToInformIncrementalMarker,
    kUpdateRememberedSetOnNoNeedToInformIncrementalMarker
  }
;
  void Generate(MacroAssembler* masm);
  void GenerateIncremental(MacroAssembler* masm, Mode mode);
  void CheckNeedsToInformIncrementalMarker(
      MacroAssembler* masm,
      OnNoNeedToInformIncrementalMarker on_no_need,
      Mode mode);
  void InformIncrementalMarker(MacroAssembler* masm, Mode mode);

  Major MajorKey() { return RecordWrite; }

  int MinorKey() {
    return ObjectBits::encode(object_.code()) |
        ValueBits::encode(value_.code()) |
        AddressBits::encode(address_.code()) |
        RememberedSetActionBits::encode(remembered_set_action_) |
        SaveFPRegsModeBits::encode(save_fp_regs_mode_);
  }

  void Activate(Code* code) {
    code->GetHeap()->incremental_marking()->ActivateGeneratedStub(code);
  }

  class ObjectBits: public BitField<int, 0, 3> {};
  class ValueBits: public BitField<int, 3, 3> {};
  class AddressBits: public BitField<int, 6, 3> {};
  class RememberedSetActionBits: public BitField<RememberedSetAction, 9, 1> {};
  class SaveFPRegsModeBits: public BitField<SaveFPRegsMode, 10, 1> {};

  Register object_;
  Register value_;
  Register address_;
  RememberedSetAction remembered_set_action_;
  SaveFPRegsMode save_fp_regs_mode_;
  RegisterAllocation regs_;
};


} }  // namespace v8::internal

#endif  // V8_IA32_CODE_STUBS_IA32_H_
