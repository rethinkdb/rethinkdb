// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_ARM64_LITHIUM_ARM64_H_
#define V8_ARM64_LITHIUM_ARM64_H_

#include "src/hydrogen.h"
#include "src/lithium.h"
#include "src/lithium-allocator.h"
#include "src/safepoint-table.h"
#include "src/utils.h"

namespace v8 {
namespace internal {

// Forward declarations.
class LCodeGen;

#define LITHIUM_CONCRETE_INSTRUCTION_LIST(V) \
  V(AccessArgumentsAt)                       \
  V(AddE)                                    \
  V(AddI)                                    \
  V(AddS)                                    \
  V(Allocate)                                \
  V(AllocateBlockContext)                    \
  V(ApplyArguments)                          \
  V(ArgumentsElements)                       \
  V(ArgumentsLength)                         \
  V(ArithmeticD)                             \
  V(ArithmeticT)                             \
  V(BitI)                                    \
  V(BitS)                                    \
  V(BoundsCheck)                             \
  V(Branch)                                  \
  V(CallFunction)                            \
  V(CallJSFunction)                          \
  V(CallNew)                                 \
  V(CallNewArray)                            \
  V(CallRuntime)                             \
  V(CallStub)                                \
  V(CallWithDescriptor)                      \
  V(CheckInstanceType)                       \
  V(CheckMapValue)                           \
  V(CheckMaps)                               \
  V(CheckNonSmi)                             \
  V(CheckSmi)                                \
  V(CheckValue)                              \
  V(ClampDToUint8)                           \
  V(ClampIToUint8)                           \
  V(ClampTToUint8)                           \
  V(ClassOfTestAndBranch)                    \
  V(CmpHoleAndBranchD)                       \
  V(CmpHoleAndBranchT)                       \
  V(CmpMapAndBranch)                         \
  V(CmpObjectEqAndBranch)                    \
  V(CmpT)                                    \
  V(CompareMinusZeroAndBranch)               \
  V(CompareNumericAndBranch)                 \
  V(ConstantD)                               \
  V(ConstantE)                               \
  V(ConstantI)                               \
  V(ConstantS)                               \
  V(ConstantT)                               \
  V(ConstructDouble)                         \
  V(Context)                                 \
  V(DateField)                               \
  V(DebugBreak)                              \
  V(DeclareGlobals)                          \
  V(Deoptimize)                              \
  V(DivByConstI)                             \
  V(DivByPowerOf2I)                          \
  V(DivI)                                    \
  V(DoubleBits)                              \
  V(DoubleToIntOrSmi)                        \
  V(Drop)                                    \
  V(Dummy)                                   \
  V(DummyUse)                                \
  V(FlooringDivByConstI)                     \
  V(FlooringDivByPowerOf2I)                  \
  V(FlooringDivI)                            \
  V(ForInCacheArray)                         \
  V(ForInPrepareMap)                         \
  V(FunctionLiteral)                         \
  V(GetCachedArrayIndex)                     \
  V(Goto)                                    \
  V(HasCachedArrayIndexAndBranch)            \
  V(HasInstanceTypeAndBranch)                \
  V(InnerAllocatedObject)                    \
  V(InstanceOf)                              \
  V(InstanceOfKnownGlobal)                   \
  V(InstructionGap)                          \
  V(Integer32ToDouble)                       \
  V(InvokeFunction)                          \
  V(IsConstructCallAndBranch)                \
  V(IsObjectAndBranch)                       \
  V(IsSmiAndBranch)                          \
  V(IsStringAndBranch)                       \
  V(IsUndetectableAndBranch)                 \
  V(Label)                                   \
  V(LazyBailout)                             \
  V(LoadContextSlot)                         \
  V(LoadFieldByIndex)                        \
  V(LoadFunctionPrototype)                   \
  V(LoadGlobalCell)                          \
  V(LoadGlobalGeneric)                       \
  V(LoadKeyedExternal)                       \
  V(LoadKeyedFixed)                          \
  V(LoadKeyedFixedDouble)                    \
  V(LoadKeyedGeneric)                        \
  V(LoadNamedField)                          \
  V(LoadNamedGeneric)                        \
  V(LoadRoot)                                \
  V(MapEnumLength)                           \
  V(MathAbs)                                 \
  V(MathAbsTagged)                           \
  V(MathClz32)                               \
  V(MathExp)                                 \
  V(MathFloorD)                              \
  V(MathFloorI)                              \
  V(MathFround)                              \
  V(MathLog)                                 \
  V(MathMinMax)                              \
  V(MathPowHalf)                             \
  V(MathRoundD)                              \
  V(MathRoundI)                              \
  V(MathSqrt)                                \
  V(ModByConstI)                             \
  V(ModByPowerOf2I)                          \
  V(ModI)                                    \
  V(MulConstIS)                              \
  V(MulI)                                    \
  V(MulS)                                    \
  V(NumberTagD)                              \
  V(NumberTagU)                              \
  V(NumberUntagD)                            \
  V(OsrEntry)                                \
  V(Parameter)                               \
  V(Power)                                   \
  V(PreparePushArguments)                    \
  V(PushArguments)                           \
  V(RegExpLiteral)                           \
  V(Return)                                  \
  V(SeqStringGetChar)                        \
  V(SeqStringSetChar)                        \
  V(ShiftI)                                  \
  V(ShiftS)                                  \
  V(SmiTag)                                  \
  V(SmiUntag)                                \
  V(StackCheck)                              \
  V(StoreCodeEntry)                          \
  V(StoreContextSlot)                        \
  V(StoreFrameContext)                       \
  V(StoreGlobalCell)                         \
  V(StoreKeyedExternal)                      \
  V(StoreKeyedFixed)                         \
  V(StoreKeyedFixedDouble)                   \
  V(StoreKeyedGeneric)                       \
  V(StoreNamedField)                         \
  V(StoreNamedGeneric)                       \
  V(StringAdd)                               \
  V(StringCharCodeAt)                        \
  V(StringCharFromCode)                      \
  V(StringCompareAndBranch)                  \
  V(SubI)                                    \
  V(SubS)                                    \
  V(TaggedToI)                               \
  V(TailCallThroughMegamorphicCache)         \
  V(ThisFunction)                            \
  V(ToFastProperties)                        \
  V(TransitionElementsKind)                  \
  V(TrapAllocationMemento)                   \
  V(TruncateDoubleToIntOrSmi)                \
  V(Typeof)                                  \
  V(TypeofIsAndBranch)                       \
  V(Uint32ToDouble)                          \
  V(UnknownOSRValue)                         \
  V(WrapReceiver)


#define DECLARE_CONCRETE_INSTRUCTION(type, mnemonic)                        \
  virtual Opcode opcode() const FINAL OVERRIDE {                      \
    return LInstruction::k##type;                                           \
  }                                                                         \
  virtual void CompileToNative(LCodeGen* generator) FINAL OVERRIDE;   \
  virtual const char* Mnemonic() const FINAL OVERRIDE {               \
    return mnemonic;                                                        \
  }                                                                         \
  static L##type* cast(LInstruction* instr) {                               \
    DCHECK(instr->Is##type());                                              \
    return reinterpret_cast<L##type*>(instr);                               \
  }


#define DECLARE_HYDROGEN_ACCESSOR(type)           \
  H##type* hydrogen() const {                     \
    return H##type::cast(this->hydrogen_value()); \
  }


class LInstruction : public ZoneObject {
 public:
  LInstruction()
      : environment_(NULL),
        hydrogen_value_(NULL),
        bit_field_(IsCallBits::encode(false)) { }

  virtual ~LInstruction() { }

  virtual void CompileToNative(LCodeGen* generator) = 0;
  virtual const char* Mnemonic() const = 0;
  virtual void PrintTo(StringStream* stream);
  virtual void PrintDataTo(StringStream* stream);
  virtual void PrintOutputOperandTo(StringStream* stream);

  enum Opcode {
    // Declare a unique enum value for each instruction.
#define DECLARE_OPCODE(type) k##type,
    LITHIUM_CONCRETE_INSTRUCTION_LIST(DECLARE_OPCODE)
    kNumberOfInstructions
#undef DECLARE_OPCODE
  };

  virtual Opcode opcode() const = 0;

  // Declare non-virtual type testers for all leaf IR classes.
#define DECLARE_PREDICATE(type) \
  bool Is##type() const { return opcode() == k##type; }
  LITHIUM_CONCRETE_INSTRUCTION_LIST(DECLARE_PREDICATE)
#undef DECLARE_PREDICATE

  // Declare virtual predicates for instructions that don't have
  // an opcode.
  virtual bool IsGap() const { return false; }

  virtual bool IsControl() const { return false; }

  // Try deleting this instruction if possible.
  virtual bool TryDelete() { return false; }

  void set_environment(LEnvironment* env) { environment_ = env; }
  LEnvironment* environment() const { return environment_; }
  bool HasEnvironment() const { return environment_ != NULL; }

  void set_pointer_map(LPointerMap* p) { pointer_map_.set(p); }
  LPointerMap* pointer_map() const { return pointer_map_.get(); }
  bool HasPointerMap() const { return pointer_map_.is_set(); }

  void set_hydrogen_value(HValue* value) { hydrogen_value_ = value; }
  HValue* hydrogen_value() const { return hydrogen_value_; }

  virtual void SetDeferredLazyDeoptimizationEnvironment(LEnvironment* env) { }

  void MarkAsCall() { bit_field_ = IsCallBits::update(bit_field_, true); }
  bool IsCall() const { return IsCallBits::decode(bit_field_); }

  // Interface to the register allocator and iterators.
  bool ClobbersTemps() const { return IsCall(); }
  bool ClobbersRegisters() const { return IsCall(); }
  virtual bool ClobbersDoubleRegisters(Isolate* isolate) const {
    return IsCall();
  }
  bool IsMarkedAsCall() const { return IsCall(); }

  virtual bool HasResult() const = 0;
  virtual LOperand* result() const = 0;

  virtual int InputCount() = 0;
  virtual LOperand* InputAt(int i) = 0;
  virtual int TempCount() = 0;
  virtual LOperand* TempAt(int i) = 0;

  LOperand* FirstInput() { return InputAt(0); }
  LOperand* Output() { return HasResult() ? result() : NULL; }

  virtual bool HasInterestingComment(LCodeGen* gen) const { return true; }

#ifdef DEBUG
  void VerifyCall();
#endif

 private:
  class IsCallBits: public BitField<bool, 0, 1> {};

  LEnvironment* environment_;
  SetOncePointer<LPointerMap> pointer_map_;
  HValue* hydrogen_value_;
  int32_t bit_field_;
};


// R = number of result operands (0 or 1).
template<int R>
class LTemplateResultInstruction : public LInstruction {
 public:
  // Allow 0 or 1 output operands.
  STATIC_ASSERT(R == 0 || R == 1);
  virtual bool HasResult() const FINAL OVERRIDE {
    return (R != 0) && (result() != NULL);
  }
  void set_result(LOperand* operand) { results_[0] = operand; }
  LOperand* result() const { return results_[0]; }

 protected:
  EmbeddedContainer<LOperand*, R> results_;
};


// R = number of result operands (0 or 1).
// I = number of input operands.
// T = number of temporary operands.
template<int R, int I, int T>
class LTemplateInstruction : public LTemplateResultInstruction<R> {
 protected:
  EmbeddedContainer<LOperand*, I> inputs_;
  EmbeddedContainer<LOperand*, T> temps_;

 private:
  // Iterator support.
  virtual int InputCount() FINAL OVERRIDE { return I; }
  virtual LOperand* InputAt(int i) FINAL OVERRIDE { return inputs_[i]; }

  virtual int TempCount() FINAL OVERRIDE { return T; }
  virtual LOperand* TempAt(int i) FINAL OVERRIDE { return temps_[i]; }
};


class LTailCallThroughMegamorphicCache FINAL
    : public LTemplateInstruction<0, 3, 0> {
 public:
  explicit LTailCallThroughMegamorphicCache(LOperand* context,
                                            LOperand* receiver,
                                            LOperand* name) {
    inputs_[0] = context;
    inputs_[1] = receiver;
    inputs_[2] = name;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* receiver() { return inputs_[1]; }
  LOperand* name() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(TailCallThroughMegamorphicCache,
                               "tail-call-through-megamorphic-cache")
  DECLARE_HYDROGEN_ACCESSOR(TailCallThroughMegamorphicCache)
};


class LUnknownOSRValue FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  virtual bool HasInterestingComment(LCodeGen* gen) const OVERRIDE {
    return false;
  }
  DECLARE_CONCRETE_INSTRUCTION(UnknownOSRValue, "unknown-osr-value")
};


template<int I, int T>
class LControlInstruction : public LTemplateInstruction<0, I, T> {
 public:
  LControlInstruction() : false_label_(NULL), true_label_(NULL) { }

  virtual bool IsControl() const FINAL OVERRIDE { return true; }

  int SuccessorCount() { return hydrogen()->SuccessorCount(); }
  HBasicBlock* SuccessorAt(int i) { return hydrogen()->SuccessorAt(i); }

  int TrueDestination(LChunk* chunk) {
    return chunk->LookupDestination(true_block_id());
  }

  int FalseDestination(LChunk* chunk) {
    return chunk->LookupDestination(false_block_id());
  }

  Label* TrueLabel(LChunk* chunk) {
    if (true_label_ == NULL) {
      true_label_ = chunk->GetAssemblyLabel(TrueDestination(chunk));
    }
    return true_label_;
  }

  Label* FalseLabel(LChunk* chunk) {
    if (false_label_ == NULL) {
      false_label_ = chunk->GetAssemblyLabel(FalseDestination(chunk));
    }
    return false_label_;
  }

 protected:
  int true_block_id() { return SuccessorAt(0)->block_id(); }
  int false_block_id() { return SuccessorAt(1)->block_id(); }

 private:
  DECLARE_HYDROGEN_ACCESSOR(ControlInstruction);

  Label* false_label_;
  Label* true_label_;
};


class LGap : public LTemplateInstruction<0, 0, 0> {
 public:
  explicit LGap(HBasicBlock* block)
      : block_(block) {
    parallel_moves_[BEFORE] = NULL;
    parallel_moves_[START] = NULL;
    parallel_moves_[END] = NULL;
    parallel_moves_[AFTER] = NULL;
  }

  // Can't use the DECLARE-macro here because of sub-classes.
  virtual bool IsGap() const OVERRIDE { return true; }
  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
  static LGap* cast(LInstruction* instr) {
    DCHECK(instr->IsGap());
    return reinterpret_cast<LGap*>(instr);
  }

  bool IsRedundant() const;

  HBasicBlock* block() const { return block_; }

  enum InnerPosition {
    BEFORE,
    START,
    END,
    AFTER,
    FIRST_INNER_POSITION = BEFORE,
    LAST_INNER_POSITION = AFTER
  };

  LParallelMove* GetOrCreateParallelMove(InnerPosition pos, Zone* zone)  {
    if (parallel_moves_[pos] == NULL) {
      parallel_moves_[pos] = new(zone) LParallelMove(zone);
    }
    return parallel_moves_[pos];
  }

  LParallelMove* GetParallelMove(InnerPosition pos)  {
    return parallel_moves_[pos];
  }

 private:
  LParallelMove* parallel_moves_[LAST_INNER_POSITION + 1];
  HBasicBlock* block_;
};


class LInstructionGap FINAL : public LGap {
 public:
  explicit LInstructionGap(HBasicBlock* block) : LGap(block) { }

  virtual bool HasInterestingComment(LCodeGen* gen) const OVERRIDE {
    return !IsRedundant();
  }

  DECLARE_CONCRETE_INSTRUCTION(InstructionGap, "gap")
};


class LDrop FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  explicit LDrop(int count) : count_(count) { }

  int count() const { return count_; }

  DECLARE_CONCRETE_INSTRUCTION(Drop, "drop")

 private:
  int count_;
};


class LDummy FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  LDummy() {}
  DECLARE_CONCRETE_INSTRUCTION(Dummy, "dummy")
};


class LDummyUse FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LDummyUse(LOperand* value) {
    inputs_[0] = value;
  }
  DECLARE_CONCRETE_INSTRUCTION(DummyUse, "dummy-use")
};


class LGoto FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  explicit LGoto(HBasicBlock* block) : block_(block) { }

  virtual bool HasInterestingComment(LCodeGen* gen) const OVERRIDE;
  DECLARE_CONCRETE_INSTRUCTION(Goto, "goto")
  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
  virtual bool IsControl() const OVERRIDE { return true; }

  int block_id() const { return block_->block_id(); }

 private:
  HBasicBlock* block_;
};


class LLazyBailout FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  LLazyBailout() : gap_instructions_size_(0) { }

  DECLARE_CONCRETE_INSTRUCTION(LazyBailout, "lazy-bailout")

  void set_gap_instructions_size(int gap_instructions_size) {
    gap_instructions_size_ = gap_instructions_size;
  }
  int gap_instructions_size() { return gap_instructions_size_; }

 private:
  int gap_instructions_size_;
};


class LLabel FINAL : public LGap {
 public:
  explicit LLabel(HBasicBlock* block)
      : LGap(block), replacement_(NULL) { }

  virtual bool HasInterestingComment(LCodeGen* gen) const OVERRIDE {
    return false;
  }
  DECLARE_CONCRETE_INSTRUCTION(Label, "label")

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  int block_id() const { return block()->block_id(); }
  bool is_loop_header() const { return block()->IsLoopHeader(); }
  bool is_osr_entry() const { return block()->is_osr_entry(); }
  Label* label() { return &label_; }
  LLabel* replacement() const { return replacement_; }
  void set_replacement(LLabel* label) { replacement_ = label; }
  bool HasReplacement() const { return replacement_ != NULL; }

 private:
  Label label_;
  LLabel* replacement_;
};


class LOsrEntry FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  LOsrEntry() {}

  virtual bool HasInterestingComment(LCodeGen* gen) const OVERRIDE {
    return false;
  }
  DECLARE_CONCRETE_INSTRUCTION(OsrEntry, "osr-entry")
};


class LAccessArgumentsAt FINAL : public LTemplateInstruction<1, 3, 0> {
 public:
  LAccessArgumentsAt(LOperand* arguments,
                     LOperand* length,
                     LOperand* index) {
    inputs_[0] = arguments;
    inputs_[1] = length;
    inputs_[2] = index;
  }

  DECLARE_CONCRETE_INSTRUCTION(AccessArgumentsAt, "access-arguments-at")

  LOperand* arguments() { return inputs_[0]; }
  LOperand* length() { return inputs_[1]; }
  LOperand* index() { return inputs_[2]; }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LAddE FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LAddE(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(AddE, "add-e")
  DECLARE_HYDROGEN_ACCESSOR(Add)
};


class LAddI FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LAddI(LOperand* left, LOperand* right)
      : shift_(NO_SHIFT), shift_amount_(0)  {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LAddI(LOperand* left, LOperand* right, Shift shift, LOperand* shift_amount)
      : shift_(shift), shift_amount_(shift_amount)  {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  Shift shift() const { return shift_; }
  LOperand* shift_amount() const { return shift_amount_; }

  DECLARE_CONCRETE_INSTRUCTION(AddI, "add-i")
  DECLARE_HYDROGEN_ACCESSOR(Add)

 protected:
  Shift shift_;
  LOperand* shift_amount_;
};


class LAddS FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LAddS(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(AddS, "add-s")
  DECLARE_HYDROGEN_ACCESSOR(Add)
};


class LAllocate FINAL : public LTemplateInstruction<1, 2, 3> {
 public:
  LAllocate(LOperand* context,
            LOperand* size,
            LOperand* temp1,
            LOperand* temp2,
            LOperand* temp3) {
    inputs_[0] = context;
    inputs_[1] = size;
    temps_[0] = temp1;
    temps_[1] = temp2;
    temps_[2] = temp3;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* size() { return inputs_[1]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }
  LOperand* temp3() { return temps_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(Allocate, "allocate")
  DECLARE_HYDROGEN_ACCESSOR(Allocate)
};


class LApplyArguments FINAL : public LTemplateInstruction<1, 4, 0> {
 public:
  LApplyArguments(LOperand* function,
                  LOperand* receiver,
                  LOperand* length,
                  LOperand* elements) {
    inputs_[0] = function;
    inputs_[1] = receiver;
    inputs_[2] = length;
    inputs_[3] = elements;
  }

  DECLARE_CONCRETE_INSTRUCTION(ApplyArguments, "apply-arguments")

  LOperand* function() { return inputs_[0]; }
  LOperand* receiver() { return inputs_[1]; }
  LOperand* length() { return inputs_[2]; }
  LOperand* elements() { return inputs_[3]; }
};


class LArgumentsElements FINAL : public LTemplateInstruction<1, 0, 1> {
 public:
  explicit LArgumentsElements(LOperand* temp) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ArgumentsElements, "arguments-elements")
  DECLARE_HYDROGEN_ACCESSOR(ArgumentsElements)
};


class LArgumentsLength FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LArgumentsLength(LOperand* elements) {
    inputs_[0] = elements;
  }

  LOperand* elements() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ArgumentsLength, "arguments-length")
};


class LArithmeticD FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LArithmeticD(Token::Value op,
               LOperand* left,
               LOperand* right)
      : op_(op) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  Token::Value op() const { return op_; }
  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  virtual Opcode opcode() const OVERRIDE {
    return LInstruction::kArithmeticD;
  }
  virtual void CompileToNative(LCodeGen* generator) OVERRIDE;
  virtual const char* Mnemonic() const OVERRIDE;

 private:
  Token::Value op_;
};


class LArithmeticT FINAL : public LTemplateInstruction<1, 3, 0> {
 public:
  LArithmeticT(Token::Value op,
               LOperand* context,
               LOperand* left,
               LOperand* right)
      : op_(op) {
    inputs_[0] = context;
    inputs_[1] = left;
    inputs_[2] = right;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* left() { return inputs_[1]; }
  LOperand* right() { return inputs_[2]; }
  Token::Value op() const { return op_; }

  virtual Opcode opcode() const OVERRIDE {
    return LInstruction::kArithmeticT;
  }
  virtual void CompileToNative(LCodeGen* generator) OVERRIDE;
  virtual const char* Mnemonic() const OVERRIDE;

 private:
  Token::Value op_;
};


class LBoundsCheck FINAL : public LTemplateInstruction<0, 2, 0> {
 public:
  explicit LBoundsCheck(LOperand* index, LOperand* length) {
    inputs_[0] = index;
    inputs_[1] = length;
  }

  LOperand* index() { return inputs_[0]; }
  LOperand* length() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(BoundsCheck, "bounds-check")
  DECLARE_HYDROGEN_ACCESSOR(BoundsCheck)
};


class LBitI FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LBitI(LOperand* left, LOperand* right)
      : shift_(NO_SHIFT), shift_amount_(0)  {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LBitI(LOperand* left, LOperand* right, Shift shift, LOperand* shift_amount)
      : shift_(shift), shift_amount_(shift_amount)  {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  Shift shift() const { return shift_; }
  LOperand* shift_amount() const { return shift_amount_; }

  Token::Value op() const { return hydrogen()->op(); }

  DECLARE_CONCRETE_INSTRUCTION(BitI, "bit-i")
  DECLARE_HYDROGEN_ACCESSOR(Bitwise)

 protected:
  Shift shift_;
  LOperand* shift_amount_;
};


class LBitS FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LBitS(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  Token::Value op() const { return hydrogen()->op(); }

  DECLARE_CONCRETE_INSTRUCTION(BitS, "bit-s")
  DECLARE_HYDROGEN_ACCESSOR(Bitwise)
};


class LBranch FINAL : public LControlInstruction<1, 2> {
 public:
  explicit LBranch(LOperand* value, LOperand *temp1, LOperand *temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(Branch, "branch")
  DECLARE_HYDROGEN_ACCESSOR(Branch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LCallJSFunction FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LCallJSFunction(LOperand* function) {
    inputs_[0] = function;
  }

  LOperand* function() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CallJSFunction, "call-js-function")
  DECLARE_HYDROGEN_ACCESSOR(CallJSFunction)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  int arity() const { return hydrogen()->argument_count() - 1; }
};


class LCallFunction FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LCallFunction(LOperand* context, LOperand* function) {
    inputs_[0] = context;
    inputs_[1] = function;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* function() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(CallFunction, "call-function")
  DECLARE_HYDROGEN_ACCESSOR(CallFunction)

  int arity() const { return hydrogen()->argument_count() - 1; }
};


class LCallNew FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LCallNew(LOperand* context, LOperand* constructor) {
    inputs_[0] = context;
    inputs_[1] = constructor;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* constructor() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(CallNew, "call-new")
  DECLARE_HYDROGEN_ACCESSOR(CallNew)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  int arity() const { return hydrogen()->argument_count() - 1; }
};


class LCallNewArray FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LCallNewArray(LOperand* context, LOperand* constructor) {
    inputs_[0] = context;
    inputs_[1] = constructor;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* constructor() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(CallNewArray, "call-new-array")
  DECLARE_HYDROGEN_ACCESSOR(CallNewArray)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  int arity() const { return hydrogen()->argument_count() - 1; }
};


class LCallRuntime FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LCallRuntime(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CallRuntime, "call-runtime")
  DECLARE_HYDROGEN_ACCESSOR(CallRuntime)

  virtual bool ClobbersDoubleRegisters(Isolate* isolate) const OVERRIDE {
    return save_doubles() == kDontSaveFPRegs;
  }

  const Runtime::Function* function() const { return hydrogen()->function(); }
  int arity() const { return hydrogen()->argument_count(); }
  SaveFPRegsMode save_doubles() const { return hydrogen()->save_doubles(); }
};


class LCallStub FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LCallStub(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CallStub, "call-stub")
  DECLARE_HYDROGEN_ACCESSOR(CallStub)
};


class LCheckInstanceType FINAL : public LTemplateInstruction<0, 1, 1> {
 public:
  explicit LCheckInstanceType(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CheckInstanceType, "check-instance-type")
  DECLARE_HYDROGEN_ACCESSOR(CheckInstanceType)
};


class LCheckMaps FINAL : public LTemplateInstruction<0, 1, 1> {
 public:
  explicit LCheckMaps(LOperand* value = NULL, LOperand* temp = NULL) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CheckMaps, "check-maps")
  DECLARE_HYDROGEN_ACCESSOR(CheckMaps)
};


class LCheckNonSmi FINAL : public LTemplateInstruction<0, 1, 0> {
 public:
  explicit LCheckNonSmi(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CheckNonSmi, "check-non-smi")
  DECLARE_HYDROGEN_ACCESSOR(CheckHeapObject)
};


class LCheckSmi FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LCheckSmi(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CheckSmi, "check-smi")
};


class LCheckValue FINAL : public LTemplateInstruction<0, 1, 0> {
 public:
  explicit LCheckValue(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CheckValue, "check-value")
  DECLARE_HYDROGEN_ACCESSOR(CheckValue)
};


class LClampDToUint8 FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LClampDToUint8(LOperand* unclamped) {
    inputs_[0] = unclamped;
  }

  LOperand* unclamped() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ClampDToUint8, "clamp-d-to-uint8")
};


class LClampIToUint8 FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LClampIToUint8(LOperand* unclamped) {
    inputs_[0] = unclamped;
  }

  LOperand* unclamped() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ClampIToUint8, "clamp-i-to-uint8")
};


class LClampTToUint8 FINAL : public LTemplateInstruction<1, 1, 1> {
 public:
  LClampTToUint8(LOperand* unclamped, LOperand* temp1) {
    inputs_[0] = unclamped;
    temps_[0] = temp1;
  }

  LOperand* unclamped() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ClampTToUint8, "clamp-t-to-uint8")
};


class LDoubleBits FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LDoubleBits(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(DoubleBits, "double-bits")
  DECLARE_HYDROGEN_ACCESSOR(DoubleBits)
};


class LConstructDouble FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LConstructDouble(LOperand* hi, LOperand* lo) {
    inputs_[0] = hi;
    inputs_[1] = lo;
  }

  LOperand* hi() { return inputs_[0]; }
  LOperand* lo() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(ConstructDouble, "construct-double")
};


class LClassOfTestAndBranch FINAL : public LControlInstruction<1, 2> {
 public:
  LClassOfTestAndBranch(LOperand* value, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(ClassOfTestAndBranch,
                               "class-of-test-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(ClassOfTestAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LCmpHoleAndBranchD FINAL : public LControlInstruction<1, 1> {
 public:
  explicit LCmpHoleAndBranchD(LOperand* object, LOperand* temp) {
    inputs_[0] = object;
    temps_[0] = temp;
  }

  LOperand* object() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CmpHoleAndBranchD, "cmp-hole-and-branch-d")
  DECLARE_HYDROGEN_ACCESSOR(CompareHoleAndBranch)
};


class LCmpHoleAndBranchT FINAL : public LControlInstruction<1, 0> {
 public:
  explicit LCmpHoleAndBranchT(LOperand* object) {
    inputs_[0] = object;
  }

  LOperand* object() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CmpHoleAndBranchT, "cmp-hole-and-branch-t")
  DECLARE_HYDROGEN_ACCESSOR(CompareHoleAndBranch)
};


class LCmpMapAndBranch FINAL : public LControlInstruction<1, 1> {
 public:
  LCmpMapAndBranch(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CmpMapAndBranch, "cmp-map-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(CompareMap)

  Handle<Map> map() const { return hydrogen()->map().handle(); }
};


class LCmpObjectEqAndBranch FINAL : public LControlInstruction<2, 0> {
 public:
  LCmpObjectEqAndBranch(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(CmpObjectEqAndBranch, "cmp-object-eq-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(CompareObjectEqAndBranch)
};


class LCmpT FINAL : public LTemplateInstruction<1, 3, 0> {
 public:
  LCmpT(LOperand* context, LOperand* left, LOperand* right) {
    inputs_[0] = context;
    inputs_[1] = left;
    inputs_[2] = right;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* left() { return inputs_[1]; }
  LOperand* right() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(CmpT, "cmp-t")
  DECLARE_HYDROGEN_ACCESSOR(CompareGeneric)

  Token::Value op() const { return hydrogen()->token(); }
};


class LCompareMinusZeroAndBranch FINAL : public LControlInstruction<1, 1> {
 public:
  LCompareMinusZeroAndBranch(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CompareMinusZeroAndBranch,
                               "cmp-minus-zero-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(CompareMinusZeroAndBranch)
};


class LCompareNumericAndBranch FINAL : public LControlInstruction<2, 0> {
 public:
  LCompareNumericAndBranch(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(CompareNumericAndBranch,
                               "compare-numeric-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(CompareNumericAndBranch)

  Token::Value op() const { return hydrogen()->token(); }
  bool is_double() const {
    return hydrogen()->representation().IsDouble();
  }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LConstantD FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(ConstantD, "constant-d")
  DECLARE_HYDROGEN_ACCESSOR(Constant)

  double value() const { return hydrogen()->DoubleValue(); }
};


class LConstantE FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(ConstantE, "constant-e")
  DECLARE_HYDROGEN_ACCESSOR(Constant)

  ExternalReference value() const {
    return hydrogen()->ExternalReferenceValue();
  }
};


class LConstantI FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(ConstantI, "constant-i")
  DECLARE_HYDROGEN_ACCESSOR(Constant)

  int32_t value() const { return hydrogen()->Integer32Value(); }
};


class LConstantS FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(ConstantS, "constant-s")
  DECLARE_HYDROGEN_ACCESSOR(Constant)

  Smi* value() const { return Smi::FromInt(hydrogen()->Integer32Value()); }
};


class LConstantT FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(ConstantT, "constant-t")
  DECLARE_HYDROGEN_ACCESSOR(Constant)

  Handle<Object> value(Isolate* isolate) const {
    return hydrogen()->handle(isolate);
  }
};


class LContext FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(Context, "context")
  DECLARE_HYDROGEN_ACCESSOR(Context)
};


class LDateField FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  LDateField(LOperand* date, Smi* index) : index_(index) {
    inputs_[0] = date;
  }

  LOperand* date() { return inputs_[0]; }
  Smi* index() const { return index_; }

  DECLARE_CONCRETE_INSTRUCTION(DateField, "date-field")
  DECLARE_HYDROGEN_ACCESSOR(DateField)

 private:
  Smi* index_;
};


class LDebugBreak FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(DebugBreak, "break")
};


class LDeclareGlobals FINAL : public LTemplateInstruction<0, 1, 0> {
 public:
  explicit LDeclareGlobals(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(DeclareGlobals, "declare-globals")
  DECLARE_HYDROGEN_ACCESSOR(DeclareGlobals)
};


class LDeoptimize FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  virtual bool IsControl() const OVERRIDE { return true; }
  DECLARE_CONCRETE_INSTRUCTION(Deoptimize, "deoptimize")
  DECLARE_HYDROGEN_ACCESSOR(Deoptimize)
};


class LDivByPowerOf2I FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  LDivByPowerOf2I(LOperand* dividend, int32_t divisor) {
    inputs_[0] = dividend;
    divisor_ = divisor;
  }

  LOperand* dividend() { return inputs_[0]; }
  int32_t divisor() const { return divisor_; }

  DECLARE_CONCRETE_INSTRUCTION(DivByPowerOf2I, "div-by-power-of-2-i")
  DECLARE_HYDROGEN_ACCESSOR(Div)

 private:
  int32_t divisor_;
};


class LDivByConstI FINAL : public LTemplateInstruction<1, 1, 1> {
 public:
  LDivByConstI(LOperand* dividend, int32_t divisor, LOperand* temp) {
    inputs_[0] = dividend;
    divisor_ = divisor;
    temps_[0] = temp;
  }

  LOperand* dividend() { return inputs_[0]; }
  int32_t divisor() const { return divisor_; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(DivByConstI, "div-by-const-i")
  DECLARE_HYDROGEN_ACCESSOR(Div)

 private:
  int32_t divisor_;
};


class LDivI FINAL : public LTemplateInstruction<1, 2, 1> {
 public:
  LDivI(LOperand* dividend, LOperand* divisor, LOperand* temp) {
    inputs_[0] = dividend;
    inputs_[1] = divisor;
    temps_[0] = temp;
  }

  LOperand* dividend() { return inputs_[0]; }
  LOperand* divisor() { return inputs_[1]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(DivI, "div-i")
  DECLARE_HYDROGEN_ACCESSOR(BinaryOperation)
};


class LDoubleToIntOrSmi FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LDoubleToIntOrSmi(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(DoubleToIntOrSmi, "double-to-int-or-smi")
  DECLARE_HYDROGEN_ACCESSOR(UnaryOperation)

  bool tag_result() { return hydrogen()->representation().IsSmi(); }
};


class LForInCacheArray FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LForInCacheArray(LOperand* map) {
    inputs_[0] = map;
  }

  LOperand* map() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ForInCacheArray, "for-in-cache-array")

  int idx() {
    return HForInCacheArray::cast(this->hydrogen_value())->idx();
  }
};


class LForInPrepareMap FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LForInPrepareMap(LOperand* context, LOperand* object) {
    inputs_[0] = context;
    inputs_[1] = object;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* object() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(ForInPrepareMap, "for-in-prepare-map")
};


class LGetCachedArrayIndex FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LGetCachedArrayIndex(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(GetCachedArrayIndex, "get-cached-array-index")
  DECLARE_HYDROGEN_ACCESSOR(GetCachedArrayIndex)
};


class LHasCachedArrayIndexAndBranch FINAL
    : public LControlInstruction<1, 1> {
 public:
  LHasCachedArrayIndexAndBranch(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(HasCachedArrayIndexAndBranch,
                               "has-cached-array-index-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(HasCachedArrayIndexAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LHasInstanceTypeAndBranch FINAL : public LControlInstruction<1, 1> {
 public:
  LHasInstanceTypeAndBranch(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(HasInstanceTypeAndBranch,
                               "has-instance-type-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(HasInstanceTypeAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LInnerAllocatedObject FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LInnerAllocatedObject(LOperand* base_object, LOperand* offset) {
    inputs_[0] = base_object;
    inputs_[1] = offset;
  }

  LOperand* base_object() const { return inputs_[0]; }
  LOperand* offset() const { return inputs_[1]; }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  DECLARE_CONCRETE_INSTRUCTION(InnerAllocatedObject, "inner-allocated-object")
};


class LInstanceOf FINAL : public LTemplateInstruction<1, 3, 0> {
 public:
  LInstanceOf(LOperand* context, LOperand* left, LOperand* right) {
    inputs_[0] = context;
    inputs_[1] = left;
    inputs_[2] = right;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* left() { return inputs_[1]; }
  LOperand* right() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(InstanceOf, "instance-of")
};


class LInstanceOfKnownGlobal FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LInstanceOfKnownGlobal(LOperand* context, LOperand* value) {
    inputs_[0] = context;
    inputs_[1] = value;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* value() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(InstanceOfKnownGlobal,
                               "instance-of-known-global")
  DECLARE_HYDROGEN_ACCESSOR(InstanceOfKnownGlobal)

  Handle<JSFunction> function() const { return hydrogen()->function(); }
  LEnvironment* GetDeferredLazyDeoptimizationEnvironment() {
    return lazy_deopt_env_;
  }
  virtual void SetDeferredLazyDeoptimizationEnvironment(
      LEnvironment* env) OVERRIDE {
    lazy_deopt_env_ = env;
  }

 private:
  LEnvironment* lazy_deopt_env_;
};


class LInteger32ToDouble FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LInteger32ToDouble(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(Integer32ToDouble, "int32-to-double")
};


class LCallWithDescriptor FINAL : public LTemplateResultInstruction<1> {
 public:
  LCallWithDescriptor(CallInterfaceDescriptor descriptor,
                      const ZoneList<LOperand*>& operands, Zone* zone)
      : descriptor_(descriptor),
        inputs_(descriptor.GetRegisterParameterCount() + 1, zone) {
    DCHECK(descriptor.GetRegisterParameterCount() + 1 == operands.length());
    inputs_.AddAll(operands, zone);
  }

  LOperand* target() const { return inputs_[0]; }

  CallInterfaceDescriptor descriptor() { return descriptor_; }

 private:
  DECLARE_CONCRETE_INSTRUCTION(CallWithDescriptor, "call-with-descriptor")
  DECLARE_HYDROGEN_ACCESSOR(CallWithDescriptor)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  int arity() const { return hydrogen()->argument_count() - 1; }

  CallInterfaceDescriptor descriptor_;
  ZoneList<LOperand*> inputs_;

  // Iterator support.
  virtual int InputCount() FINAL OVERRIDE { return inputs_.length(); }
  virtual LOperand* InputAt(int i) FINAL OVERRIDE { return inputs_[i]; }

  virtual int TempCount() FINAL OVERRIDE { return 0; }
  virtual LOperand* TempAt(int i) FINAL OVERRIDE { return NULL; }
};


class LInvokeFunction FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LInvokeFunction(LOperand* context, LOperand* function) {
    inputs_[0] = context;
    inputs_[1] = function;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* function() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(InvokeFunction, "invoke-function")
  DECLARE_HYDROGEN_ACCESSOR(InvokeFunction)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  int arity() const { return hydrogen()->argument_count() - 1; }
};


class LIsConstructCallAndBranch FINAL : public LControlInstruction<0, 2> {
 public:
  LIsConstructCallAndBranch(LOperand* temp1, LOperand* temp2) {
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(IsConstructCallAndBranch,
                               "is-construct-call-and-branch")
};


class LIsObjectAndBranch FINAL : public LControlInstruction<1, 2> {
 public:
  LIsObjectAndBranch(LOperand* value, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(IsObjectAndBranch, "is-object-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(IsObjectAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LIsStringAndBranch FINAL : public LControlInstruction<1, 1> {
 public:
  LIsStringAndBranch(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(IsStringAndBranch, "is-string-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(IsStringAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LIsSmiAndBranch FINAL : public LControlInstruction<1, 0> {
 public:
  explicit LIsSmiAndBranch(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(IsSmiAndBranch, "is-smi-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(IsSmiAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LIsUndetectableAndBranch FINAL : public LControlInstruction<1, 1> {
 public:
  explicit LIsUndetectableAndBranch(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(IsUndetectableAndBranch,
                               "is-undetectable-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(IsUndetectableAndBranch)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LLoadContextSlot FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LLoadContextSlot(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadContextSlot, "load-context-slot")
  DECLARE_HYDROGEN_ACCESSOR(LoadContextSlot)

  int slot_index() const { return hydrogen()->slot_index(); }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LLoadNamedField FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LLoadNamedField(LOperand* object) {
    inputs_[0] = object;
  }

  LOperand* object() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadNamedField, "load-named-field")
  DECLARE_HYDROGEN_ACCESSOR(LoadNamedField)
};


class LFunctionLiteral FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LFunctionLiteral(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(FunctionLiteral, "function-literal")
  DECLARE_HYDROGEN_ACCESSOR(FunctionLiteral)
};


class LLoadFunctionPrototype FINAL : public LTemplateInstruction<1, 1, 1> {
 public:
  LLoadFunctionPrototype(LOperand* function, LOperand* temp) {
    inputs_[0] = function;
    temps_[0] = temp;
  }

  LOperand* function() { return inputs_[0]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadFunctionPrototype, "load-function-prototype")
  DECLARE_HYDROGEN_ACCESSOR(LoadFunctionPrototype)
};


class LLoadGlobalCell FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(LoadGlobalCell, "load-global-cell")
  DECLARE_HYDROGEN_ACCESSOR(LoadGlobalCell)
};


class LLoadGlobalGeneric FINAL : public LTemplateInstruction<1, 2, 1> {
 public:
  LLoadGlobalGeneric(LOperand* context, LOperand* global_object,
                     LOperand* vector) {
    inputs_[0] = context;
    inputs_[1] = global_object;
    temps_[0] = vector;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* global_object() { return inputs_[1]; }
  LOperand* temp_vector() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadGlobalGeneric, "load-global-generic")
  DECLARE_HYDROGEN_ACCESSOR(LoadGlobalGeneric)

  Handle<Object> name() const { return hydrogen()->name(); }
  bool for_typeof() const { return hydrogen()->for_typeof(); }
};


template<int T>
class LLoadKeyed : public LTemplateInstruction<1, 2, T> {
 public:
  LLoadKeyed(LOperand* elements, LOperand* key) {
    this->inputs_[0] = elements;
    this->inputs_[1] = key;
  }

  LOperand* elements() { return this->inputs_[0]; }
  LOperand* key() { return this->inputs_[1]; }
  ElementsKind elements_kind() const {
    return this->hydrogen()->elements_kind();
  }
  bool is_external() const {
    return this->hydrogen()->is_external();
  }
  bool is_fixed_typed_array() const {
    return hydrogen()->is_fixed_typed_array();
  }
  bool is_typed_elements() const {
    return is_external() || is_fixed_typed_array();
  }
  uint32_t base_offset() const {
    return this->hydrogen()->base_offset();
  }
  void PrintDataTo(StringStream* stream) OVERRIDE {
    this->elements()->PrintTo(stream);
    stream->Add("[");
    this->key()->PrintTo(stream);
    if (this->base_offset() != 0) {
      stream->Add(" + %d]", this->base_offset());
    } else {
      stream->Add("]");
    }
  }

  DECLARE_HYDROGEN_ACCESSOR(LoadKeyed)
};


class LLoadKeyedExternal: public LLoadKeyed<1> {
 public:
  LLoadKeyedExternal(LOperand* elements, LOperand* key, LOperand* temp) :
      LLoadKeyed<1>(elements, key) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadKeyedExternal, "load-keyed-external");
};


class LLoadKeyedFixed: public LLoadKeyed<1> {
 public:
  LLoadKeyedFixed(LOperand* elements, LOperand* key, LOperand* temp) :
      LLoadKeyed<1>(elements, key) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadKeyedFixed, "load-keyed-fixed");
};


class LLoadKeyedFixedDouble: public LLoadKeyed<1> {
 public:
  LLoadKeyedFixedDouble(LOperand* elements, LOperand* key, LOperand* temp) :
      LLoadKeyed<1>(elements, key) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadKeyedFixedDouble, "load-keyed-fixed-double");
};


class LLoadKeyedGeneric FINAL : public LTemplateInstruction<1, 3, 1> {
 public:
  LLoadKeyedGeneric(LOperand* context, LOperand* object, LOperand* key,
                    LOperand* vector) {
    inputs_[0] = context;
    inputs_[1] = object;
    inputs_[2] = key;
    temps_[0] = vector;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* object() { return inputs_[1]; }
  LOperand* key() { return inputs_[2]; }
  LOperand* temp_vector() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadKeyedGeneric, "load-keyed-generic")
  DECLARE_HYDROGEN_ACCESSOR(LoadKeyedGeneric)
};


class LLoadNamedGeneric FINAL : public LTemplateInstruction<1, 2, 1> {
 public:
  LLoadNamedGeneric(LOperand* context, LOperand* object, LOperand* vector) {
    inputs_[0] = context;
    inputs_[1] = object;
    temps_[0] = vector;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* object() { return inputs_[1]; }
  LOperand* temp_vector() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadNamedGeneric, "load-named-generic")
  DECLARE_HYDROGEN_ACCESSOR(LoadNamedGeneric)

  Handle<Object> name() const { return hydrogen()->name(); }
};


class LLoadRoot FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(LoadRoot, "load-root")
  DECLARE_HYDROGEN_ACCESSOR(LoadRoot)

  Heap::RootListIndex index() const { return hydrogen()->index(); }
};


class LMapEnumLength FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LMapEnumLength(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(MapEnumLength, "map-enum-length")
};


template<int T>
class LUnaryMathOperation : public LTemplateInstruction<1, 1, T> {
 public:
  explicit LUnaryMathOperation(LOperand* value) {
    this->inputs_[0] = value;
  }

  LOperand* value() { return this->inputs_[0]; }
  BuiltinFunctionId op() const { return this->hydrogen()->op(); }

  void PrintDataTo(StringStream* stream) OVERRIDE;

  DECLARE_HYDROGEN_ACCESSOR(UnaryMathOperation)
};


class LMathAbs FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathAbs(LOperand* value) : LUnaryMathOperation<0>(value) {}

  DECLARE_CONCRETE_INSTRUCTION(MathAbs, "math-abs")
};


class LMathAbsTagged: public LTemplateInstruction<1, 2, 3> {
 public:
  LMathAbsTagged(LOperand* context, LOperand* value,
                 LOperand* temp1, LOperand* temp2, LOperand* temp3) {
    inputs_[0] = context;
    inputs_[1] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
    temps_[2] = temp3;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* value() { return inputs_[1]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }
  LOperand* temp3() { return temps_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(MathAbsTagged, "math-abs-tagged")
  DECLARE_HYDROGEN_ACCESSOR(UnaryMathOperation)
};


class LMathExp FINAL : public LUnaryMathOperation<4> {
 public:
  LMathExp(LOperand* value,
                LOperand* double_temp1,
                LOperand* temp1,
                LOperand* temp2,
                LOperand* temp3)
      : LUnaryMathOperation<4>(value) {
    temps_[0] = double_temp1;
    temps_[1] = temp1;
    temps_[2] = temp2;
    temps_[3] = temp3;
    ExternalReference::InitializeMathExpData();
  }

  LOperand* double_temp1() { return temps_[0]; }
  LOperand* temp1() { return temps_[1]; }
  LOperand* temp2() { return temps_[2]; }
  LOperand* temp3() { return temps_[3]; }

  DECLARE_CONCRETE_INSTRUCTION(MathExp, "math-exp")
};


// Math.floor with a double result.
class LMathFloorD FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathFloorD(LOperand* value) : LUnaryMathOperation<0>(value) { }
  DECLARE_CONCRETE_INSTRUCTION(MathFloorD, "math-floor-d")
};


// Math.floor with an integer result.
class LMathFloorI FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathFloorI(LOperand* value) : LUnaryMathOperation<0>(value) { }
  DECLARE_CONCRETE_INSTRUCTION(MathFloorI, "math-floor-i")
};


class LFlooringDivByPowerOf2I FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  LFlooringDivByPowerOf2I(LOperand* dividend, int32_t divisor) {
    inputs_[0] = dividend;
    divisor_ = divisor;
  }

  LOperand* dividend() { return inputs_[0]; }
  int32_t divisor() const { return divisor_; }

  DECLARE_CONCRETE_INSTRUCTION(FlooringDivByPowerOf2I,
                               "flooring-div-by-power-of-2-i")
  DECLARE_HYDROGEN_ACCESSOR(MathFloorOfDiv)

 private:
  int32_t divisor_;
};


class LFlooringDivByConstI FINAL : public LTemplateInstruction<1, 1, 2> {
 public:
  LFlooringDivByConstI(LOperand* dividend, int32_t divisor, LOperand* temp) {
    inputs_[0] = dividend;
    divisor_ = divisor;
    temps_[0] = temp;
  }

  LOperand* dividend() { return inputs_[0]; }
  int32_t divisor() const { return divisor_; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(FlooringDivByConstI, "flooring-div-by-const-i")
  DECLARE_HYDROGEN_ACCESSOR(MathFloorOfDiv)

 private:
  int32_t divisor_;
};


class LFlooringDivI FINAL : public LTemplateInstruction<1, 2, 1> {
 public:
  LFlooringDivI(LOperand* dividend, LOperand* divisor, LOperand* temp) {
    inputs_[0] = dividend;
    inputs_[1] = divisor;
    temps_[0] = temp;
  }

  LOperand* dividend() { return inputs_[0]; }
  LOperand* divisor() { return inputs_[1]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(FlooringDivI, "flooring-div-i")
  DECLARE_HYDROGEN_ACCESSOR(MathFloorOfDiv)
};


class LMathLog FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathLog(LOperand* value) : LUnaryMathOperation<0>(value) { }
  DECLARE_CONCRETE_INSTRUCTION(MathLog, "math-log")
};


class LMathClz32 FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathClz32(LOperand* value) : LUnaryMathOperation<0>(value) { }
  DECLARE_CONCRETE_INSTRUCTION(MathClz32, "math-clz32")
};


class LMathMinMax FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LMathMinMax(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(MathMinMax, "math-min-max")
  DECLARE_HYDROGEN_ACCESSOR(MathMinMax)
};


class LMathPowHalf FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathPowHalf(LOperand* value) : LUnaryMathOperation<0>(value) { }
  DECLARE_CONCRETE_INSTRUCTION(MathPowHalf, "math-pow-half")
};


// Math.round with an integer result.
class LMathRoundD FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathRoundD(LOperand* value)
      : LUnaryMathOperation<0>(value) {
  }

  DECLARE_CONCRETE_INSTRUCTION(MathRoundD, "math-round-d")
};


// Math.round with an integer result.
class LMathRoundI FINAL : public LUnaryMathOperation<1> {
 public:
  LMathRoundI(LOperand* value, LOperand* temp1)
      : LUnaryMathOperation<1>(value) {
    temps_[0] = temp1;
  }

  LOperand* temp1() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(MathRoundI, "math-round-i")
};


class LMathFround FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathFround(LOperand* value) : LUnaryMathOperation<0>(value) {}

  DECLARE_CONCRETE_INSTRUCTION(MathFround, "math-fround")
};


class LMathSqrt FINAL : public LUnaryMathOperation<0> {
 public:
  explicit LMathSqrt(LOperand* value) : LUnaryMathOperation<0>(value) { }
  DECLARE_CONCRETE_INSTRUCTION(MathSqrt, "math-sqrt")
};


class LModByPowerOf2I FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  LModByPowerOf2I(LOperand* dividend, int32_t divisor) {
    inputs_[0] = dividend;
    divisor_ = divisor;
  }

  LOperand* dividend() { return inputs_[0]; }
  int32_t divisor() const { return divisor_; }

  DECLARE_CONCRETE_INSTRUCTION(ModByPowerOf2I, "mod-by-power-of-2-i")
  DECLARE_HYDROGEN_ACCESSOR(Mod)

 private:
  int32_t divisor_;
};


class LModByConstI FINAL : public LTemplateInstruction<1, 1, 1> {
 public:
  LModByConstI(LOperand* dividend, int32_t divisor, LOperand* temp) {
    inputs_[0] = dividend;
    divisor_ = divisor;
    temps_[0] = temp;
  }

  LOperand* dividend() { return inputs_[0]; }
  int32_t divisor() const { return divisor_; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ModByConstI, "mod-by-const-i")
  DECLARE_HYDROGEN_ACCESSOR(Mod)

 private:
  int32_t divisor_;
};


class LModI FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LModI(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(ModI, "mod-i")
  DECLARE_HYDROGEN_ACCESSOR(Mod)
};


class LMulConstIS FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LMulConstIS(LOperand* left, LConstantOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LConstantOperand* right() { return LConstantOperand::cast(inputs_[1]); }

  DECLARE_CONCRETE_INSTRUCTION(MulConstIS, "mul-const-i-s")
  DECLARE_HYDROGEN_ACCESSOR(Mul)
};


class LMulI FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LMulI(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(MulI, "mul-i")
  DECLARE_HYDROGEN_ACCESSOR(Mul)
};


class LMulS FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LMulS(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(MulI, "mul-s")
  DECLARE_HYDROGEN_ACCESSOR(Mul)
};


class LNumberTagD FINAL : public LTemplateInstruction<1, 1, 2> {
 public:
  LNumberTagD(LOperand* value, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(NumberTagD, "number-tag-d")
  DECLARE_HYDROGEN_ACCESSOR(Change)
};


class LNumberTagU FINAL : public LTemplateInstruction<1, 1, 2> {
 public:
  explicit LNumberTagU(LOperand* value,
                       LOperand* temp1,
                       LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(NumberTagU, "number-tag-u")
};


class LNumberUntagD FINAL : public LTemplateInstruction<1, 1, 1> {
 public:
  LNumberUntagD(LOperand* value, LOperand* temp) {
    inputs_[0] = value;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(NumberUntagD, "double-untag")
  DECLARE_HYDROGEN_ACCESSOR(Change)
};


class LParameter FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  virtual bool HasInterestingComment(LCodeGen* gen) const { return false; }
  DECLARE_CONCRETE_INSTRUCTION(Parameter, "parameter")
};


class LPower FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LPower(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(Power, "power")
  DECLARE_HYDROGEN_ACCESSOR(Power)
};


class LPreparePushArguments FINAL : public LTemplateInstruction<0, 0, 0> {
 public:
  explicit LPreparePushArguments(int argc) : argc_(argc) {}

  inline int argc() const { return argc_; }

  DECLARE_CONCRETE_INSTRUCTION(PreparePushArguments, "prepare-push-arguments")

 protected:
  int argc_;
};


class LPushArguments FINAL : public LTemplateResultInstruction<0> {
 public:
  explicit LPushArguments(Zone* zone,
                          int capacity = kRecommendedMaxPushedArgs)
      : zone_(zone), inputs_(capacity, zone) {}

  LOperand* argument(int i) { return inputs_[i]; }
  int ArgumentCount() const { return inputs_.length(); }

  void AddArgument(LOperand* arg) { inputs_.Add(arg, zone_); }

  DECLARE_CONCRETE_INSTRUCTION(PushArguments, "push-arguments")

  // It is better to limit the number of arguments pushed simultaneously to
  // avoid pressure on the register allocator.
  static const int kRecommendedMaxPushedArgs = 4;
  bool ShouldSplitPush() const {
    return inputs_.length() >= kRecommendedMaxPushedArgs;
  }

 protected:
  Zone* zone_;
  ZoneList<LOperand*> inputs_;

 private:
  // Iterator support.
  virtual int InputCount() FINAL OVERRIDE { return inputs_.length(); }
  virtual LOperand* InputAt(int i) FINAL OVERRIDE { return inputs_[i]; }

  virtual int TempCount() FINAL OVERRIDE { return 0; }
  virtual LOperand* TempAt(int i) FINAL OVERRIDE { return NULL; }
};


class LRegExpLiteral FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LRegExpLiteral(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(RegExpLiteral, "regexp-literal")
  DECLARE_HYDROGEN_ACCESSOR(RegExpLiteral)
};


class LReturn FINAL : public LTemplateInstruction<0, 3, 0> {
 public:
  LReturn(LOperand* value, LOperand* context, LOperand* parameter_count) {
    inputs_[0] = value;
    inputs_[1] = context;
    inputs_[2] = parameter_count;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* parameter_count() { return inputs_[2]; }

  bool has_constant_parameter_count() {
    return parameter_count()->IsConstantOperand();
  }
  LConstantOperand* constant_parameter_count() {
    DCHECK(has_constant_parameter_count());
    return LConstantOperand::cast(parameter_count());
  }

  DECLARE_CONCRETE_INSTRUCTION(Return, "return")
};


class LSeqStringGetChar FINAL : public LTemplateInstruction<1, 2, 1> {
 public:
  LSeqStringGetChar(LOperand* string,
                    LOperand* index,
                    LOperand* temp) {
    inputs_[0] = string;
    inputs_[1] = index;
    temps_[0] = temp;
  }

  LOperand* string() { return inputs_[0]; }
  LOperand* index() { return inputs_[1]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(SeqStringGetChar, "seq-string-get-char")
  DECLARE_HYDROGEN_ACCESSOR(SeqStringGetChar)
};


class LSeqStringSetChar FINAL : public LTemplateInstruction<1, 4, 1> {
 public:
  LSeqStringSetChar(LOperand* context,
                    LOperand* string,
                    LOperand* index,
                    LOperand* value,
                    LOperand* temp) {
    inputs_[0] = context;
    inputs_[1] = string;
    inputs_[2] = index;
    inputs_[3] = value;
    temps_[0] = temp;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* string() { return inputs_[1]; }
  LOperand* index() { return inputs_[2]; }
  LOperand* value() { return inputs_[3]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(SeqStringSetChar, "seq-string-set-char")
  DECLARE_HYDROGEN_ACCESSOR(SeqStringSetChar)
};


class LSmiTag FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LSmiTag(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(SmiTag, "smi-tag")
  DECLARE_HYDROGEN_ACCESSOR(Change)
};


class LSmiUntag FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  LSmiUntag(LOperand* value, bool needs_check)
      : needs_check_(needs_check) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }
  bool needs_check() const { return needs_check_; }

  DECLARE_CONCRETE_INSTRUCTION(SmiUntag, "smi-untag")

 private:
  bool needs_check_;
};


class LStackCheck FINAL : public LTemplateInstruction<0, 1, 0> {
 public:
  explicit LStackCheck(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(StackCheck, "stack-check")
  DECLARE_HYDROGEN_ACCESSOR(StackCheck)

  Label* done_label() { return &done_label_; }

 private:
  Label done_label_;
};


template<int T>
class LStoreKeyed : public LTemplateInstruction<0, 3, T> {
 public:
  LStoreKeyed(LOperand* elements, LOperand* key, LOperand* value) {
    this->inputs_[0] = elements;
    this->inputs_[1] = key;
    this->inputs_[2] = value;
  }

  bool is_external() const { return this->hydrogen()->is_external(); }
  bool is_fixed_typed_array() const {
    return hydrogen()->is_fixed_typed_array();
  }
  bool is_typed_elements() const {
    return is_external() || is_fixed_typed_array();
  }
  LOperand* elements() { return this->inputs_[0]; }
  LOperand* key() { return this->inputs_[1]; }
  LOperand* value() { return this->inputs_[2]; }
  ElementsKind elements_kind() const {
    return this->hydrogen()->elements_kind();
  }

  bool NeedsCanonicalization() {
    if (hydrogen()->value()->IsAdd() || hydrogen()->value()->IsSub() ||
        hydrogen()->value()->IsMul() || hydrogen()->value()->IsDiv()) {
      return false;
    }
    return this->hydrogen()->NeedsCanonicalization();
  }
  uint32_t base_offset() const { return this->hydrogen()->base_offset(); }

  void PrintDataTo(StringStream* stream) OVERRIDE {
    this->elements()->PrintTo(stream);
    stream->Add("[");
    this->key()->PrintTo(stream);
    if (this->base_offset() != 0) {
      stream->Add(" + %d] <-", this->base_offset());
    } else {
      stream->Add("] <- ");
    }

    if (this->value() == NULL) {
      DCHECK(hydrogen()->IsConstantHoleStore() &&
             hydrogen()->value()->representation().IsDouble());
      stream->Add("<the hole(nan)>");
    } else {
      this->value()->PrintTo(stream);
    }
  }

  DECLARE_HYDROGEN_ACCESSOR(StoreKeyed)
};


class LStoreKeyedExternal FINAL : public LStoreKeyed<1> {
 public:
  LStoreKeyedExternal(LOperand* elements, LOperand* key, LOperand* value,
                      LOperand* temp) :
      LStoreKeyed<1>(elements, key, value) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreKeyedExternal, "store-keyed-external")
};


class LStoreKeyedFixed FINAL : public LStoreKeyed<1> {
 public:
  LStoreKeyedFixed(LOperand* elements, LOperand* key, LOperand* value,
                   LOperand* temp) :
      LStoreKeyed<1>(elements, key, value) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreKeyedFixed, "store-keyed-fixed")
};


class LStoreKeyedFixedDouble FINAL : public LStoreKeyed<1> {
 public:
  LStoreKeyedFixedDouble(LOperand* elements, LOperand* key, LOperand* value,
                         LOperand* temp) :
      LStoreKeyed<1>(elements, key, value) {
    temps_[0] = temp;
  }

  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreKeyedFixedDouble,
                               "store-keyed-fixed-double")
};


class LStoreKeyedGeneric FINAL : public LTemplateInstruction<0, 4, 0> {
 public:
  LStoreKeyedGeneric(LOperand* context,
                     LOperand* obj,
                     LOperand* key,
                     LOperand* value) {
    inputs_[0] = context;
    inputs_[1] = obj;
    inputs_[2] = key;
    inputs_[3] = value;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* object() { return inputs_[1]; }
  LOperand* key() { return inputs_[2]; }
  LOperand* value() { return inputs_[3]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreKeyedGeneric, "store-keyed-generic")
  DECLARE_HYDROGEN_ACCESSOR(StoreKeyedGeneric)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  StrictMode strict_mode() { return hydrogen()->strict_mode(); }
};


class LStoreNamedField FINAL : public LTemplateInstruction<0, 2, 2> {
 public:
  LStoreNamedField(LOperand* object, LOperand* value,
                   LOperand* temp0, LOperand* temp1) {
    inputs_[0] = object;
    inputs_[1] = value;
    temps_[0] = temp0;
    temps_[1] = temp1;
  }

  LOperand* object() { return inputs_[0]; }
  LOperand* value() { return inputs_[1]; }
  LOperand* temp0() { return temps_[0]; }
  LOperand* temp1() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreNamedField, "store-named-field")
  DECLARE_HYDROGEN_ACCESSOR(StoreNamedField)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  Representation representation() const {
    return hydrogen()->field_representation();
  }
};


class LStoreNamedGeneric FINAL: public LTemplateInstruction<0, 3, 0> {
 public:
  LStoreNamedGeneric(LOperand* context, LOperand* object, LOperand* value) {
    inputs_[0] = context;
    inputs_[1] = object;
    inputs_[2] = value;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* object() { return inputs_[1]; }
  LOperand* value() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreNamedGeneric, "store-named-generic")
  DECLARE_HYDROGEN_ACCESSOR(StoreNamedGeneric)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  Handle<Object> name() const { return hydrogen()->name(); }
  StrictMode strict_mode() { return hydrogen()->strict_mode(); }
};


class LStringAdd FINAL : public LTemplateInstruction<1, 3, 0> {
 public:
  LStringAdd(LOperand* context, LOperand* left, LOperand* right) {
    inputs_[0] = context;
    inputs_[1] = left;
    inputs_[2] = right;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* left() { return inputs_[1]; }
  LOperand* right() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(StringAdd, "string-add")
  DECLARE_HYDROGEN_ACCESSOR(StringAdd)
};



class LStringCharCodeAt FINAL : public LTemplateInstruction<1, 3, 0> {
 public:
  LStringCharCodeAt(LOperand* context, LOperand* string, LOperand* index) {
    inputs_[0] = context;
    inputs_[1] = string;
    inputs_[2] = index;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* string() { return inputs_[1]; }
  LOperand* index() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(StringCharCodeAt, "string-char-code-at")
  DECLARE_HYDROGEN_ACCESSOR(StringCharCodeAt)
};


class LStringCharFromCode FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LStringCharFromCode(LOperand* context, LOperand* char_code) {
    inputs_[0] = context;
    inputs_[1] = char_code;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* char_code() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(StringCharFromCode, "string-char-from-code")
  DECLARE_HYDROGEN_ACCESSOR(StringCharFromCode)
};


class LStringCompareAndBranch FINAL : public LControlInstruction<3, 0> {
 public:
  LStringCompareAndBranch(LOperand* context, LOperand* left, LOperand* right) {
    inputs_[0] = context;
    inputs_[1] = left;
    inputs_[2] = right;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* left() { return inputs_[1]; }
  LOperand* right() { return inputs_[2]; }

  DECLARE_CONCRETE_INSTRUCTION(StringCompareAndBranch,
                               "string-compare-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(StringCompareAndBranch)

  Token::Value op() const { return hydrogen()->token(); }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


// Truncating conversion from a tagged value to an int32.
class LTaggedToI FINAL : public LTemplateInstruction<1, 1, 2> {
 public:
  explicit LTaggedToI(LOperand* value, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(TaggedToI, "tagged-to-i")
  DECLARE_HYDROGEN_ACCESSOR(Change)

  bool truncating() { return hydrogen()->CanTruncateToInt32(); }
};


class LShiftI FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LShiftI(Token::Value op, LOperand* left, LOperand* right, bool can_deopt)
      : op_(op), can_deopt_(can_deopt) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  Token::Value op() const { return op_; }
  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }
  bool can_deopt() const { return can_deopt_; }

  DECLARE_CONCRETE_INSTRUCTION(ShiftI, "shift-i")

 private:
  Token::Value op_;
  bool can_deopt_;
};


class LShiftS FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LShiftS(Token::Value op, LOperand* left, LOperand* right, bool can_deopt)
      : op_(op), can_deopt_(can_deopt) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  Token::Value op() const { return op_; }
  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }
  bool can_deopt() const { return can_deopt_; }

  DECLARE_CONCRETE_INSTRUCTION(ShiftS, "shift-s")

 private:
  Token::Value op_;
  bool can_deopt_;
};


class LStoreCodeEntry FINAL: public LTemplateInstruction<0, 2, 1> {
 public:
  LStoreCodeEntry(LOperand* function, LOperand* code_object,
                  LOperand* temp) {
    inputs_[0] = function;
    inputs_[1] = code_object;
    temps_[0] = temp;
  }

  LOperand* function() { return inputs_[0]; }
  LOperand* code_object() { return inputs_[1]; }
  LOperand* temp() { return temps_[0]; }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  DECLARE_CONCRETE_INSTRUCTION(StoreCodeEntry, "store-code-entry")
  DECLARE_HYDROGEN_ACCESSOR(StoreCodeEntry)
};


class LStoreContextSlot FINAL : public LTemplateInstruction<0, 2, 1> {
 public:
  LStoreContextSlot(LOperand* context, LOperand* value, LOperand* temp) {
    inputs_[0] = context;
    inputs_[1] = value;
    temps_[0] = temp;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* value() { return inputs_[1]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreContextSlot, "store-context-slot")
  DECLARE_HYDROGEN_ACCESSOR(StoreContextSlot)

  int slot_index() { return hydrogen()->slot_index(); }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LStoreGlobalCell FINAL : public LTemplateInstruction<0, 1, 2> {
 public:
  LStoreGlobalCell(LOperand* value, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreGlobalCell, "store-global-cell")
  DECLARE_HYDROGEN_ACCESSOR(StoreGlobalCell)
};


class LSubI FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LSubI(LOperand* left, LOperand* right)
      : shift_(NO_SHIFT), shift_amount_(0)  {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LSubI(LOperand* left, LOperand* right, Shift shift, LOperand* shift_amount)
      : shift_(shift), shift_amount_(shift_amount)  {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  Shift shift() const { return shift_; }
  LOperand* shift_amount() const { return shift_amount_; }

  DECLARE_CONCRETE_INSTRUCTION(SubI, "sub-i")
  DECLARE_HYDROGEN_ACCESSOR(Sub)

 protected:
  Shift shift_;
  LOperand* shift_amount_;
};


class LSubS: public LTemplateInstruction<1, 2, 0> {
 public:
  LSubS(LOperand* left, LOperand* right) {
    inputs_[0] = left;
    inputs_[1] = right;
  }

  LOperand* left() { return inputs_[0]; }
  LOperand* right() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(SubS, "sub-s")
  DECLARE_HYDROGEN_ACCESSOR(Sub)
};


class LThisFunction FINAL : public LTemplateInstruction<1, 0, 0> {
 public:
  DECLARE_CONCRETE_INSTRUCTION(ThisFunction, "this-function")
  DECLARE_HYDROGEN_ACCESSOR(ThisFunction)
};


class LToFastProperties FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LToFastProperties(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(ToFastProperties, "to-fast-properties")
  DECLARE_HYDROGEN_ACCESSOR(ToFastProperties)
};


class LTransitionElementsKind FINAL : public LTemplateInstruction<0, 2, 2> {
 public:
  LTransitionElementsKind(LOperand* object,
                          LOperand* context,
                          LOperand* temp1,
                          LOperand* temp2) {
    inputs_[0] = object;
    inputs_[1] = context;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* object() { return inputs_[0]; }
  LOperand* context() { return inputs_[1]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(TransitionElementsKind,
                               "transition-elements-kind")
  DECLARE_HYDROGEN_ACCESSOR(TransitionElementsKind)

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;

  Handle<Map> original_map() { return hydrogen()->original_map().handle(); }
  Handle<Map> transitioned_map() {
    return hydrogen()->transitioned_map().handle();
  }
  ElementsKind from_kind() const { return hydrogen()->from_kind(); }
  ElementsKind to_kind() const { return hydrogen()->to_kind(); }
};


class LTrapAllocationMemento FINAL : public LTemplateInstruction<0, 1, 2> {
 public:
  LTrapAllocationMemento(LOperand* object, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = object;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* object() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(TrapAllocationMemento, "trap-allocation-memento")
};


class LTruncateDoubleToIntOrSmi FINAL
    : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LTruncateDoubleToIntOrSmi(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(TruncateDoubleToIntOrSmi,
                               "truncate-double-to-int-or-smi")
  DECLARE_HYDROGEN_ACCESSOR(UnaryOperation)

  bool tag_result() { return hydrogen()->representation().IsSmi(); }
};


class LTypeof FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LTypeof(LOperand* context, LOperand* value) {
    inputs_[0] = context;
    inputs_[1] = value;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* value() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(Typeof, "typeof")
};


class LTypeofIsAndBranch FINAL : public LControlInstruction<1, 2> {
 public:
  LTypeofIsAndBranch(LOperand* value, LOperand* temp1, LOperand* temp2) {
    inputs_[0] = value;
    temps_[0] = temp1;
    temps_[1] = temp2;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* temp1() { return temps_[0]; }
  LOperand* temp2() { return temps_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(TypeofIsAndBranch, "typeof-is-and-branch")
  DECLARE_HYDROGEN_ACCESSOR(TypeofIsAndBranch)

  Handle<String> type_literal() const { return hydrogen()->type_literal(); }

  virtual void PrintDataTo(StringStream* stream) OVERRIDE;
};


class LUint32ToDouble FINAL : public LTemplateInstruction<1, 1, 0> {
 public:
  explicit LUint32ToDouble(LOperand* value) {
    inputs_[0] = value;
  }

  LOperand* value() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(Uint32ToDouble, "uint32-to-double")
};


class LCheckMapValue FINAL : public LTemplateInstruction<0, 2, 1> {
 public:
  LCheckMapValue(LOperand* value, LOperand* map, LOperand* temp) {
    inputs_[0] = value;
    inputs_[1] = map;
    temps_[0] = temp;
  }

  LOperand* value() { return inputs_[0]; }
  LOperand* map() { return inputs_[1]; }
  LOperand* temp() { return temps_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(CheckMapValue, "check-map-value")
};


class LLoadFieldByIndex FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LLoadFieldByIndex(LOperand* object, LOperand* index) {
    inputs_[0] = object;
    inputs_[1] = index;
  }

  LOperand* object() { return inputs_[0]; }
  LOperand* index() { return inputs_[1]; }

  DECLARE_CONCRETE_INSTRUCTION(LoadFieldByIndex, "load-field-by-index")
};


class LStoreFrameContext: public LTemplateInstruction<0, 1, 0> {
 public:
  explicit LStoreFrameContext(LOperand* context) {
    inputs_[0] = context;
  }

  LOperand* context() { return inputs_[0]; }

  DECLARE_CONCRETE_INSTRUCTION(StoreFrameContext, "store-frame-context")
};


class LAllocateBlockContext: public LTemplateInstruction<1, 2, 0> {
 public:
  LAllocateBlockContext(LOperand* context, LOperand* function) {
    inputs_[0] = context;
    inputs_[1] = function;
  }

  LOperand* context() { return inputs_[0]; }
  LOperand* function() { return inputs_[1]; }

  Handle<ScopeInfo> scope_info() { return hydrogen()->scope_info(); }

  DECLARE_CONCRETE_INSTRUCTION(AllocateBlockContext, "allocate-block-context")
  DECLARE_HYDROGEN_ACCESSOR(AllocateBlockContext)
};


class LWrapReceiver FINAL : public LTemplateInstruction<1, 2, 0> {
 public:
  LWrapReceiver(LOperand* receiver, LOperand* function) {
    inputs_[0] = receiver;
    inputs_[1] = function;
  }

  DECLARE_CONCRETE_INSTRUCTION(WrapReceiver, "wrap-receiver")
  DECLARE_HYDROGEN_ACCESSOR(WrapReceiver)

  LOperand* receiver() { return inputs_[0]; }
  LOperand* function() { return inputs_[1]; }
};


class LChunkBuilder;
class LPlatformChunk FINAL : public LChunk {
 public:
  LPlatformChunk(CompilationInfo* info, HGraph* graph)
      : LChunk(info, graph) { }

  int GetNextSpillIndex();
  LOperand* GetNextSpillSlot(RegisterKind kind);
};


class LChunkBuilder FINAL : public LChunkBuilderBase {
 public:
  LChunkBuilder(CompilationInfo* info, HGraph* graph, LAllocator* allocator)
      : LChunkBuilderBase(info, graph),
        current_instruction_(NULL),
        current_block_(NULL),
        allocator_(allocator) {}

  // Build the sequence for the graph.
  LPlatformChunk* Build();

  // Declare methods that deal with the individual node types.
#define DECLARE_DO(type) LInstruction* Do##type(H##type* node);
  HYDROGEN_CONCRETE_INSTRUCTION_LIST(DECLARE_DO)
#undef DECLARE_DO

  LInstruction* DoDivByPowerOf2I(HDiv* instr);
  LInstruction* DoDivByConstI(HDiv* instr);
  LInstruction* DoDivI(HBinaryOperation* instr);
  LInstruction* DoModByPowerOf2I(HMod* instr);
  LInstruction* DoModByConstI(HMod* instr);
  LInstruction* DoModI(HMod* instr);
  LInstruction* DoFlooringDivByPowerOf2I(HMathFloorOfDiv* instr);
  LInstruction* DoFlooringDivByConstI(HMathFloorOfDiv* instr);
  LInstruction* DoFlooringDivI(HMathFloorOfDiv* instr);

  static bool HasMagicNumberForDivision(int32_t divisor);

 private:
  // Methods for getting operands for Use / Define / Temp.
  LUnallocated* ToUnallocated(Register reg);
  LUnallocated* ToUnallocated(DoubleRegister reg);

  // Methods for setting up define-use relationships.
  MUST_USE_RESULT LOperand* Use(HValue* value, LUnallocated* operand);
  MUST_USE_RESULT LOperand* UseFixed(HValue* value, Register fixed_register);
  MUST_USE_RESULT LOperand* UseFixedDouble(HValue* value,
                                           DoubleRegister fixed_register);

  // A value that is guaranteed to be allocated to a register.
  // The operand created by UseRegister is guaranteed to be live until the end
  // of the instruction. This means that register allocator will not reuse its
  // register for any other operand inside instruction.
  MUST_USE_RESULT LOperand* UseRegister(HValue* value);

  // The operand created by UseRegisterAndClobber is guaranteed to be live until
  // the end of the end of the instruction, and it may also be used as a scratch
  // register by the instruction implementation.
  //
  // This behaves identically to ARM's UseTempRegister. However, it is renamed
  // to discourage its use in ARM64, since in most cases it is better to
  // allocate a temporary register for the Lithium instruction.
  MUST_USE_RESULT LOperand* UseRegisterAndClobber(HValue* value);

  // The operand created by UseRegisterAtStart is guaranteed to be live only at
  // instruction start. The register allocator is free to assign the same
  // register to some other operand used inside instruction (i.e. temporary or
  // output).
  MUST_USE_RESULT LOperand* UseRegisterAtStart(HValue* value);

  // An input operand in a register or a constant operand.
  MUST_USE_RESULT LOperand* UseRegisterOrConstant(HValue* value);
  MUST_USE_RESULT LOperand* UseRegisterOrConstantAtStart(HValue* value);

  // A constant operand.
  MUST_USE_RESULT LConstantOperand* UseConstant(HValue* value);

  // An input operand in register, stack slot or a constant operand.
  // Will not be moved to a register even if one is freely available.
  virtual MUST_USE_RESULT LOperand* UseAny(HValue* value);

  // Temporary operand that must be in a register.
  MUST_USE_RESULT LUnallocated* TempRegister();

  // Temporary operand that must be in a double register.
  MUST_USE_RESULT LUnallocated* TempDoubleRegister();

  MUST_USE_RESULT LOperand* FixedTemp(Register reg);

  // Temporary operand that must be in a fixed double register.
  MUST_USE_RESULT LOperand* FixedTemp(DoubleRegister reg);

  // Methods for setting up define-use relationships.
  // Return the same instruction that they are passed.
  LInstruction* Define(LTemplateResultInstruction<1>* instr,
                       LUnallocated* result);
  LInstruction* DefineAsRegister(LTemplateResultInstruction<1>* instr);
  LInstruction* DefineAsSpilled(LTemplateResultInstruction<1>* instr,
                                int index);

  LInstruction* DefineSameAsFirst(LTemplateResultInstruction<1>* instr);
  LInstruction* DefineFixed(LTemplateResultInstruction<1>* instr,
                            Register reg);
  LInstruction* DefineFixedDouble(LTemplateResultInstruction<1>* instr,
                                  DoubleRegister reg);

  enum CanDeoptimize { CAN_DEOPTIMIZE_EAGERLY, CANNOT_DEOPTIMIZE_EAGERLY };

  // By default we assume that instruction sequences generated for calls
  // cannot deoptimize eagerly and we do not attach environment to this
  // instruction.
  LInstruction* MarkAsCall(
      LInstruction* instr,
      HInstruction* hinstr,
      CanDeoptimize can_deoptimize = CANNOT_DEOPTIMIZE_EAGERLY);

  LInstruction* AssignPointerMap(LInstruction* instr);
  LInstruction* AssignEnvironment(LInstruction* instr);

  void VisitInstruction(HInstruction* current);
  void AddInstruction(LInstruction* instr, HInstruction* current);
  void DoBasicBlock(HBasicBlock* block);

  int JSShiftAmountFromHConstant(HValue* constant) {
    return HConstant::cast(constant)->Integer32Value() & 0x1f;
  }
  bool LikelyFitsImmField(HInstruction* instr, int imm) {
    if (instr->IsAdd() || instr->IsSub()) {
      return Assembler::IsImmAddSub(imm) || Assembler::IsImmAddSub(-imm);
    } else {
      DCHECK(instr->IsBitwise());
      unsigned unused_n, unused_imm_s, unused_imm_r;
      return Assembler::IsImmLogical(imm, kWRegSizeInBits,
                                     &unused_n, &unused_imm_s, &unused_imm_r);
    }
  }

  // Indicates if a sequence of the form
  //   lsl x8, x9, #imm
  //   add x0, x1, x8
  // can be replaced with:
  //   add x0, x1, x9 LSL #imm
  // If this is not possible, the function returns NULL. Otherwise it returns a
  // pointer to the shift instruction that would be optimized away.
  HBitwiseBinaryOperation* CanTransformToShiftedOp(HValue* val,
                                                   HValue** left = NULL);
  // Checks if all uses of the shift operation can optimize it away.
  bool ShiftCanBeOptimizedAway(HBitwiseBinaryOperation* shift);
  // Attempts to merge the binary operation and an eventual previous shift
  // operation into a single operation. Returns the merged instruction on
  // success, and NULL otherwise.
  LInstruction* TryDoOpWithShiftedRightOperand(HBinaryOperation* op);
  LInstruction* DoShiftedBinaryOp(HBinaryOperation* instr,
                                  HValue* left,
                                  HBitwiseBinaryOperation* shift);

  LInstruction* DoShift(Token::Value op, HBitwiseBinaryOperation* instr);
  LInstruction* DoArithmeticD(Token::Value op,
                              HArithmeticBinaryOperation* instr);
  LInstruction* DoArithmeticT(Token::Value op,
                              HBinaryOperation* instr);

  HInstruction* current_instruction_;
  HBasicBlock* current_block_;
  LAllocator* allocator_;

  DISALLOW_COPY_AND_ASSIGN(LChunkBuilder);
};

#undef DECLARE_HYDROGEN_ACCESSOR
#undef DECLARE_CONCRETE_INSTRUCTION

} }  // namespace v8::internal

#endif  // V8_ARM64_LITHIUM_ARM64_H_
